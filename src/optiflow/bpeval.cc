/* bpeval */

/* load a branch predictor dynamically */


#define	CF_DEBUGS	0		/* non-switchable debug print-outs */
#define	CF_SAFE		0		/* extra safety */
#define	CF_DELAYSLOT	0		/* ISA has a delay slot */


/* revision history:

	= 02/05/01, David Morano

	This object module was originally written (since I am really
	getting tired of manually integrating new predictors into
	the code all of the time !).


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This object module allows for branch predictor modules to be
	loaded dynamically into the tracing tool.  This alleviates
	having to manually code-in any new branch predictors (which is
	getting real, real old) !

	Usage note:

	This module was designed primarily for the MIPS platform and
	its behavior with delay slots.  It was also designed so that a
	trace of the MIPS execution could be followed and the outcomes
	of the branches could be determined from the trace control flow
	(when a branch is found we look where execution goes).  The
	problem is that this is severely not easy on an ISA with
	delayed branches (like MIPS).  Another design idea was to
	allowed for the delayed update of the branch predictors
	depending on a specified number of instructions being
	executed.  This sort of fit in with the problem of handling
	delay slots so we tried to combine these ideas as best as we
	could.

	Three extra subroutines are provided to handle the delayed
	predictor updates due to both delay-slot delays and just
	waiting for some instructions to execute.  These subroutines are :

	bpeval_outcome()		called to record branch outcome
	bpeval_checkmid()		called in middle of EX cycle
	bpeval_checkend()		called at end of EX cycle

	Hopefully, this module can be used without the above subroutines,
	at least that was intended (I think).  The compile-time define
	'CF_DELAYSLOT' hopefully makes these subroutines works for
	those ISAs that do not have delay slots while still allowing
	for delayed predictor update.  Check the code below to make
	sure that you follow this stuff (I barely do now) ! :-)


*****************************************************************************/


#define	BPEVAL_MASTER	0


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<dlfcn.h>
#include	<link.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"bpfifo.h"
#include	"bpload.h"
#include	"bpeval.h"



/* local defines */

#define	BPEVAL_MAGIC		0x29456781
#define	BPEVAL_DEFGLEN		4		/* default entries */

#ifndef	ENDIAN
#if	defined(SOLARIS) && defined(__sparc)
#define	ENDIAN		1
#else
#ifdef	_BIG_ENDIAN
#define	ENDIAN		1
#endif
#ifdef	_LITTLE_ENDIAN
#define	ENDIAN		0
#endif
#ifndef	ENDIAN
#error	"could not determine endianness of this machine"
#endif
#endif
#endif



/* external subroutines */

extern uint	nextpowtwo(uint) ;

extern int	mkpath2(char *,const char *,const char *) ;
extern int	flbsi(uint) ;

extern char	*strwcpy(char *,const char *,int) ;


/* local structures */


/* forward references */

static int filesearch(BPEVAL *,char *,int) ;


/* local variables */

static const char	*suffixes[] = {
	".so",
	".o",
	"",
	NULL
} ;

static const struct bpeval_ii	ii_zero = { 0, 0 } ;







int bpeval_init(op,dir,rows,delay)
BPEVAL		*op ;
char		dir[] ;
int		rows, delay ;
{
	int	rs ;
	int	size ;

	char	tmpfname[MAXPATHLEN + 1] ;


	if (op == NULL)
	    return SR_FAULT ;

	if (dir == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(BPEVAL)) ;

	if ((dir != NULL) && (dir[0] != '\0')) {

	    op->dir = mallocstr(dir) ;

		if (op->dir == NULL) {

			rs = SR_NOMEM ;
			goto bad0 ;
		}
	}

	if (rows < 1)
	    rows = 1 ;

	if (delay < 1)
	    delay = 1 ;

#if	CF_DEBUGS
	eprintf("bpeval_init: rows=%d delay=%d\n",rows,delay) ;
#endif

	op->rows = rows ;
	op->delay = delay ;
	op->bpsel = -1 ;

/* get the present working directory in case we need it later */

	rs = getpwd(tmpfname,MAXPATHLEN) ;

	if (rs < 0)
		goto bad1 ;

	op->pwd = mallocstrn(tmpfname,rs) ;

	if (op->pwd == NULL) {

			rs = SR_NOMEM ;
			goto bad1 ;
	}

/* continue */

	rs = vecitem_init(&op->bps,5,0) ;

	if (rs < 0)
	    goto bad2 ;

	rs = bpfifo_init(&op->fifo,(op->delay + 4)) ;

	if (rs < 0)
	    goto bad3 ;

/* we're out of here */

	op->magic = BPEVAL_MAGIC ;
	return rs ;

/* we're out of here */
bad3:
	vecitem_free(&op->bps) ;

bad2:
	if (op->pwd != NULL)
		free(op->pwd) ;

bad1:
	if (op->dir != NULL)
	    free(op->dir) ;

bad0:
	return rs ;
}
/* end subroutine (bpeval_init) */


int bpeval_bpsel(op,name)
BPEVAL		*op ;
char		name[] ;
{
	struct bpeval_entry	*ep ;

	int	rs = SR_NOTFOUND, i ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if ((name == NULL) || (name[0] == '\0'))
	    return SR_INVALID ;

	for (i = 0 ; (rs = vecitem_get(&op->bps,i,&ep)) >= 0 ; i += 1) {

	    if (ep == NULL) continue ;

	    if (strcmp(ep->s.name,name) == 0) {

	        op->bpsel = i ;
	        break ;
	    }

	} /* end for */

	return (rs >= 0) ? i : rs ;
}
/* end subroutine (bpeval_bpsel) */


/* add a new branch predictor module */
int bpeval_add(op,name,fname,p1,p2,p3,p4)
BPEVAL		*op ;
char		name[] ;
char		fname[] ;
int		p1, p2, p3, p4 ;
{
	struct bpeval_entry	e ;

	struct bpload		*lp ;

	int	rs = SR_BADFMT, i ;
	int	bl ;
	int	lfl, sl, cl ;
	int	rlen ;
	int	size ;

	char	loadfname[MAXPATHLEN + 1] ;
	char	bname[MAXNAMELEN + 1] ;
	char	*bn ;
	char	*sp, *cp ;

	void	*symp ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;

	if (name == NULL)
	    return SR_FAULT ;

	if (name[0] == '\0')
	    return SR_INVALID ;

/* get the basename of the module in 'bn' */

	bn = name ;
	bl = -1 ;
	if (((name == NULL) || (name[0] == '\0')) &&
	    (fname != NULL) && (fname[0] != '\0')) {

	    bl = sfbasename(fname,-1,&bn) ;

	    if ((cp = strrchr(bn,'.')) != NULL) {

	        bl = (cp - bn) ;
	        strwcpy(bname,bn,bl) ;

	        bn = bname ;
	    }
	}

	if (bl <= 0)
	    bl = strlen(bn) ;

#if	CF_DEBUGS
	eprintf("bpeval_add: bn=%w\n",bn,bl) ;
#endif

/* construct the file path for the code module */

	if ((fname != NULL) && (fname[0] != '\0')) {

	    sp = fname ;
	    sl = strlen(fname) ;

	} else {

	    sp = bn ;
	    sl = (bl >= 0) ? bl : strlen(bn) ;

	}

/* searching for file plainly first */

	if (sp[0] != '/')
		lfl = mkpath2(loadfname,"./",sp) ;

	else
		lfl = strwcpy(loadfname,sp,MAXPATHLEN) - loadfname ;

#if	CF_DEBUGS
	eprintf("bpeval_add: searching regular loadfname=%w\n",
		loadfname,lfl) ;
#endif

	rs = filesearch(op,loadfname,lfl) ;

/* should we search further ? */

	if ((rs < 0) && (loadfname[0] != '/') &&
	    (op->dir != NULL) && (op->dir[0] != '\0')) {

	    lfl = mkpath2(loadfname,op->dir,sp) ;

#if	CF_DEBUGS
	eprintf("bpeval_add: searching dir loadfname=%w\n",
		loadfname,lfl) ;
#endif

/* search for the file using the possible suffixes */

		rs = filesearch(op,loadfname,lfl) ;

	} /* end subroutine (missed first pass) */

#if	CF_DEBUGS
	eprintf("bpeval_add: all filesearch() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		goto bad0 ;

#if	CF_DEBUGS
	eprintf("bpeval_add: found loadfname=%s\n",loadfname) ;
#endif

/* do it */

	(void) memset(&e,0,sizeof(struct bpeval_entry)) ;

/* load the name */

	strwcpy(e.s.name,bn,MIN(bl,(MAXNAMELEN - 1))) ;

/* load up the parameters before we forget */

	e.s.p.p1 = p1 ;
	e.s.p.p2 = p2 ;
	e.s.p.p3 = p3 ;
	e.s.p.p4 = p4 ;

/* try to open the object module */

	e.dlp = dlopen(loadfname,RTLD_LAZY | RTLD_LOCAL) ;

#if	CF_DEBUGS
	if (e.dlp == NULL)
	    eprintf("bpeval_add: dlopen() rs=%s\n",dlerror()) ;
#endif

	if (e.dlp == NULL)
	    return SR_NOENT ;

#if	CF_DEBUGS
	eprintf("bpeval_add: dlsym() bn=%s\n",bn) ;
#endif

	lp = (struct bpload *) dlsym(e.dlp,bn) ;

	if (lp == NULL) {

#if	CF_DEBUGS
	    eprintf("bpeval_add: no object description in module\n") ;
#endif

	    rs = SR_INVALID ;
	    goto bad1 ;
	}

/* OK, get all of the entry points we care about */

	sp = loadfname ;		/* large enough to hold sym-name */
	cp = strwcpy(sp,bn,(MAXPATHLEN - 1)) ;

	rlen = (MAXPATHLEN - 1) - (cp - sp) ;

#if	CF_DEBUGS
	eprintf("bpeval_add: get symbols sp=%s\n",sp) ;
#endif

/* init */

	strwcpy(cp,"_init",rlen) ;

	e.call.init = (int (*)()) dlsym(e.dlp,sp) ;

/* lookup */

	strwcpy(cp,"_lookup",rlen) ;

	e.call.lookup = (int (*)()) dlsym(e.dlp,sp) ;

/* confidence */

	strwcpy(cp,"_confidence",rlen) ;

	e.call.confidence = (int (*)()) dlsym(e.dlp,sp) ;

/* update */

	strwcpy(cp,"_update",rlen) ;

	e.call.update = (int (*)()) dlsym(e.dlp,sp) ;

/* stats */

	strwcpy(cp,"_stats",rlen) ;

	e.call.stats = (int (*)()) dlsym(e.dlp,sp) ;

/* free */

	strwcpy(cp,"_free",rlen) ;

	e.call.free = (int (*)()) dlsym(e.dlp,sp) ;

/* allocate space */

	e.size = lp->size ;

#if	CF_DEBUGS
	eprintf("bpeval_add: objsize=%d rows=%d\n",e.size,op->rows) ;
#endif

	size = op->rows * e.size ;

#if	CF_DEBUGS
	eprintf("bpeval_add: size=%d\n",size) ;
#endif

	rs = uc_malloc(size,&e.op) ;

#if	CF_DEBUGS
	eprintf("bpeval_add: uc_malloc() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad1 ;

	(void) memset(e.op,0,size) ;

/* OK, initialize the new object */

	for (i = 0 ; i < op->rows ; i += 1) {

	    char	*cop ;


	    cop = ((char *) e.op) + (i * e.size) ;
	    rs = (*e.call.init)(cop,
	        e.s.p.p1,e.s.p.p2,e.s.p.p3,e.s.p.p4) ;

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0) {

	    int	j ;


#if	CF_DEBUGS
	    eprintf("bpeval_add: <bp>_init() rs=%d\n",rs) ;
#endif

	    for (j = 0 ; j < i ; j += 1) {

	        char	*cop ;


	        cop = ((char *) e.op) + (j * e.size) ;
	        (void) (*e.call.free)(cop) ;

	    }

	    goto bad2 ;
	}

/* store it away */

	rs = vecitem_add(&op->bps,&e,sizeof(struct bpeval_entry)) ;

	if (rs < 0)
	    goto bad3 ;

/* we're good and we're out of here ! */
ret0:
	return rs ;

/* bad stuff */
bad3:
	for (i = 0 ; i < op->rows ; i += 1) {

	    char	*cop ;


	    cop = ((char *) e.op) + (i * e.size) ;
	    (void) (*e.call.free)(cop) ;

	}

bad2:
	free(e.op) ;

bad1:
	dlclose(e.dlp) ;

bad0:
	return rs ;
}
/* end subroutine (bpeval_add) */


/* free up this bpeval object */
int bpeval_free(op)
BPEVAL		*op ;
{
	struct bpeval_entry	*ep ;

	int	rs = SR_BADFMT ;
	int	i, j ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;


	for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	    if (ep == NULL) continue ;

	    if (ep->call.free != NULL) {

#if	CF_DEBUGS
	        eprintf("bpeval_free: bp=%s lu=%u cor=%u\n",
	            ep->s.name,ep->s.lookups,ep->s.corrects) ;
#endif

	        for (j = 0 ; j < op->rows ; j += 1) {

	            char	*cop ;


	            cop = ((char *) ep->op) + (j * ep->size) ;
	            (*ep->call.free)(cop) ;

	        }

	    }

	    free(ep->op) ;

	    dlclose(ep->dlp) ;

	} /* end for */

	bpfifo_free(&op->fifo) ;

	vecitem_free(&op->bps) ;

	if (op->pwd != NULL)
	    free(op->pwd) ;

	if (op->dir != NULL)
	    free(op->dir) ;

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (bpeval_free) */


/* lookup an IA */
int bpeval_lookup(op,in,ia,f_se)
BPEVAL		*op ;
ULONG		in ;
ULONG		ia ;
int		f_se ;
{
	struct bpeval_entry	*ep ;

	int	rs, i, j ;
	int	rc = 8, c ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	j = in % op->rows ;

#if	CF_DEBUGS
	eprintf("bpeval_lookup: in=%u ia=%016llx row=%d\n",
	    in,ia,j) ;
#endif

	for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	    if (ep == NULL) continue ;

	    rs = -1 ;
	    if (ep->call.confidence != NULL) {

	        char	*cop ;


	        if (f_se)
	            ep->s.lookups += 1 ;

	        cop = ((char *) ep->op) + (j * ep->size) ;
	        rs = (*ep->call.confidence)(cop,ia) ;

	        if (rs > 7)
	            rs = 7 ;

	        c = rs ;

#if	CF_DEBUGS
	        eprintf("bpeval_lookup: bp=%s f_pred=%d\n",
	            ep->s.name,ep->cii.f_pred) ;
#endif

	    } else if (ep->call.lookup != NULL) {

	        char	*cop ;


	        if (f_se)
	            ep->s.lookups += 1 ;

	        cop = ((char *) ep->op) + (j * ep->size) ;
	        rs = (*ep->call.lookup)(cop,ia) ;

	        if (rs > 1)
	            rs = 1 ;

	        c = (rs * 4) ;
	    }

	    if (rs >= 0) {

	        ep->cii.f_pred = (c >= 4) ;
	        ep->cii.confidence = c ;
	        ep->s.confidence[c] += 1 ;
	        if (i == op->bpsel)
	            rc = c ;

	    }

	} /* end for */

	return (rc >= 4) ;
}
/* end subroutine (bpeval_lookup) */


/* get confidence */
int bpeval_confidence(op,in,ia,f_se)
BPEVAL		*op ;
ULONG		in ;
ULONG		ia ;
int		f_se ;
{
	struct bpeval_entry	*ep ;

	int	rs, i, j ;
	int	rc = 8, c ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	j = in % op->rows ;
	for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	    if (ep == NULL) continue ;

	    if (ep->call.confidence != NULL) {

	        char	*cop ;


	        if (f_se)
	            ep->s.lookups += 1 ;

	        cop = ((char *) ep->op) + (j * ep->size) ;
	        rs = (*ep->call.confidence)(cop,ia) ;

	        if (rs > 7)
	            rs = 7 ;

	        c = rs ;
	        ep->cii.f_pred = (c >= 4) ;
	        ep->cii.confidence = c ;
	        ep->s.confidence[c] += 1 ;
	        if (i == op->bpsel)
	            rc = c ;

	    }

	} /* end for */

	return rc ;
}
/* end subroutine (bpeval_confidence) */


/* record the outcome of a branch */
int bpeval_outcome(op,in,ia,f_se,f_outcome)
BPEVAL		*op ;
ULONG		in ;
ULONG		ia ;
int		f_se ;
int		f_outcome ;
{
	struct bpeval_entry	*ep ;

	ULONG	fin ;

	int	i, j ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	j = in % op->rows ;

#if	CF_DEBUGS
	eprintf("bpeval_outcome: in=%u ia=%016llx row=%d foc=%d\n",
	    in,ia,j,f_outcome) ;
#endif

/* accumulate statistics if requested */

	if (f_se) {

	    for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	        if (ep == NULL) continue ;

#if	CF_DEBUGS
	        eprintf("bpeval_outcome: bp=%s f_pred=%d\n",
	            ep->s.name,ep->bii.f_pred) ;
#endif

	        if (LEQUIV(ep->bii.f_pred,f_outcome)) {

		    uint	c ;


	            ep->s.corrects += 1 ;
	            c = ep->bii.confidence ;
	            ep->s.cc[c] += 1 ;

		}

	    } /* end for */

	} /* end if (statistics selection) */

/* queue up the outcome for later update of the predictors */

	fin = in + op->delay ;
	bpfifo_add(&op->fifo,fin, ia,j,f_outcome) ;

	return i ;
}
/* end subroutine (bpeval_outcome) */


/* check in the middle of the execution cycle */
int bpeval_checkmid(op,in)
BPEVAL		*op ;
ULONG		in ;
{
	struct bpeval_entry	*ep ;

	ULONG	fin, fia, lin ;

	uint	frow, foutcome ;
	uint	sin ;

	int	rs, i ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	rs = bpfifo_read(&op->fifo,&fin,&fia,&frow,&foutcome) ;

	lin = in ;
	if ((rs >= 0) && (lin >= fin)) {

#if	CF_MASTERDEBUG && CF_DEBUGS
	    eprintf("bpeval_checkmid: in=%u fin=%llu fia=%016llx\n",
	        in,fin,fia) ;
#endif

	    sin = fin ;
	    bpeval_update(op,frow,fia,foutcome) ;

	    bpfifo_remove(&op->fifo,
	        &fin,&fia,&frow,&foutcome) ;

	} /* end if (update came due) */

	return i ;
}
/* end subroutine (bpeval_checkmid) */


/* rotate the predictions since an instruction has finished */
int bpeval_checkend(op,in)
BPEVAL		*op ;
ULONG		in ;
{
	struct bpeval_entry	*ep ;

	int	i ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	    if (ep == NULL) continue ;

#if	CF_DELAYSLOT
	    ep->bii = ep->pii ;
	    ep->pii = ep->cii ;
	    ep->cii = ii_zero ;
#else /* CF_DELAYSLOT */
	    ep->bii = ep->cii ;
	    ep->cii = ii_zero ;
#endif /* CF_DELAYSLOT */

	} /* end for */

	return i ;
}
/* end subroutine (bpeval_checkend) */


/* update on branch resolution */
int bpeval_update(op,in,ia,f_outcome)
BPEVAL		*op ;
ULONG		in ;
ULONG		ia ;
int		f_outcome ;
{
	struct bpeval_entry	*ep ;

	int	i, j ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	j = in % op->rows ;
	for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	    if (ep == NULL) continue ;

	    if (ep->call.update != NULL) {

	        char	*cop ;


	        cop = ((char *) ep->op) + (j * ep->size) ;
	        (*ep->call.update)(cop,ia,f_outcome) ;

	    }

	} /* end for */

	return i ;
}
/* end subroutine (bpeval_update) */


/* get the statistics about a particular predictor */
int bpeval_stats(op,i,rp)
BPEVAL		*op ;
int		i ;
BPEVAL_STATS	*rp ;
{
	struct bpeval_entry	*ep ;

	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != BPEVAL_MAGIC)
	    return SR_NOTOPEN ;

	if (i < 0)
	    i = op->bpsel ;

	if (i < 0)
	    return SR_NOENT ;

	rs = vecitem_get(&op->bps,i,&ep) ;

#if	CF_DEBUGS
	eprintf("bpeval_stats: i=%d vecitem_get() rs=%d\n",i,rs) ;
#endif

	if (rs >= 0) {

	    memcpy(rp,&ep->s,sizeof(BPEVAL_STATS)) ;

	    if (ep->call.stats != NULL) {

	        char	*cop ;


	        cop = ((char *) ep->op) + (0 * ep->size) ;
	        rs = (*ep->call.stats)(cop,NULL) ;

#if	CF_DEBUGS
	        eprintf("bpeval_stats: call.stats() rs=%d\n",rs) ;
#endif

	        if (rp != NULL)
	            rp->bits = rs ;

	    }

	}

	return rs ;
}
/* end subroutine (bpeval_stats) */



/* INTERNAL SUBROUTINES */



static int filesearch(op,sp,sl)
BPEVAL	*op ;
char	*sp ;
int	sl ;
{
	int	rs, i ;
	int	cl ;


#if	CF_DEBUGS
	    eprintf("bpeval/filesearch: sp(%p) sl=%d\n",sp,sl) ;
#endif

	for (i = 0 ; suffixes[i] != NULL ; i += 1) {

	    cl = strlen(suffixes[i]) ;

	    if ((sl + cl) >= MAXPATHLEN)
	        continue ;

	    strcpy((sp + sl),suffixes[i]) ;

#if	CF_DEBUGS
	    eprintf("bpeval/filesearch: trying sp=%s\n",sp) ;
#endif

	    if (perm(sp,-1,-1,NULL,R_OK) >= 0)
	        break ;

	} /* end for */

	return (suffixes[i] == NULL) ? SR_NOENT : SR_OK ;
}
/* end subroutine (filesearch) */



