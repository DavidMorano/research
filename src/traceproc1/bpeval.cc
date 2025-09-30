/* bpeval */

/* load a branch predictor dynamically */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_SAFE		1


/* revision history:

	= 2002-05-01, David Morano

	This object module was originally written (since I am really
	getting tired of manually integrating new predictors into
	the code all of the time!).


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This object module allows for branch predictor modules to be
	loaded dynamically into the tracing tool.  This alleviates
	having to manually code-in any new branch predictors (which is
	getting real, real old)!


*******************************************************************************/


#define	BPEVAL_MASTER	0


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<dlfcn.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>
#include	<mallocstuff.h>
#include	<localmisc.h>

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


/* local variables */

static const char	*suffixes[] = {
	".so",
	".o",
	"",
	NULL
} ;

static const struct bpeval_ii	ii_zero = { 0, 0 } ;


/* exported subroutines */


int bpeval_init(op,dir,rows,delay)
BPEVAL		*op ;
char		dir[] ;
int		rows, delay ;
{
	int	rs ;
	int	size ;


	if (op == NULL)
	    return SR_FAULT ;

	if (dir == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(BPEVAL)) ;

	if ((dir != NULL) && (dir[0] != '\0'))
	    op->dir = mallocstr(dir) ;

	if (rows < 1)
	    rows = 1 ;

	if (delay < 1)
	    delay = 1 ;

#if	CF_DEBUGS
	debugprintf("bpeval_init: rows=%d delay=%d\n",rows,delay) ;
#endif

	op->rows = rows ;
	op->delay = delay ;
	op->bpsel = -1 ;

	rs = vecitem_start(&op->bps,5,0) ;

	if (rs < 0)
	    goto bad1 ;

	rs = bpfifo_init(&op->fifo,(op->delay + 4)) ;

	if (rs < 0)
	    goto bad2 ;

/* we're out of here */

	op->magic = BPEVAL_MAGIC ;
	return rs ;

/* we're out of here */
bad2:
	vecitem_finish(&op->bps) ;

bad1:
	if (op->dir != NULL) {
	    uc_free(op->dir) ;
	    op->dir = NULL ;
	}

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
#endif /* F_SAFE */

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
	int	sl, cl ;
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
	debugprintf("bpeval_add: bn=%t\n",bn,bl) ;
#endif

/* construct the file path for the code module */

	if ((fname != NULL) && (fname[0] != '\0')) {

	    sp = fname ;
	    sl = strlen(fname) ;

	} else {

	    sp = bn ;
	    sl = (bl >= 0) ? bl : strlen(bn) ;

	}

#if	CF_DEBUGS
	debugprintf("bpeval_add: before dir sp=%t\n",sp,sl) ;
#endif

	if ((sp[0] != '/') &&
	    (op->dir != NULL) && (op->dir[0] != '\0')) {

	    cp = loadfname ;
	    cl = mkpath2(loadfname,op->dir,sp) ;

	    sp = cp ;
	    sl = cl ;

	} /* end if (rooting the file name if necessary) */

#if	CF_DEBUGS
	debugprintf("bpeval_add: after dir sp=%t\n",sp,sl) ;
#endif

/* search for the file using the possible suffixes */

	for (i = 0 ; suffixes[i] != NULL ; i += 1) {

	    cl = strlen(suffixes[i]) ;

	    if ((sl + cl) >= MAXPATHLEN)
	        continue ;

	    strcpy((sp + sl),suffixes[i]) ;

#if	CF_DEBUGS
	    debugprintf("bpeval_add: trying sp=%s\n",sp) ;
#endif

	    if (perm(sp,-1,-1,NULL,R_OK) >= 0)
	        break ;

	} /* end for */

	if (suffixes[i] == NULL)
	    return SR_NOENT ;

#if	CF_DEBUGS
	debugprintf("bpeval_add: found sp=%s\n",sp) ;
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

	e.dlp = dlopen(sp,RTLD_LAZY | RTLD_LOCAL) ;

#if	CF_DEBUGS
	if (e.dlp == NULL)
	    debugprintf("bpeval_add: dlopen() rs=%s\n",dlerror()) ;
#endif

	if (e.dlp == NULL)
	    return SR_NOENT ;

#if	CF_DEBUGS
	debugprintf("bpeval_add: dlsym() bn=%s\n",bn) ;
#endif

	lp = (struct bpload *) dlsym(e.dlp,bn) ;

	if (lp == NULL) {

#if	CF_DEBUGS
	    debugprintf("bpeval_add: no object description in module\n") ;
#endif

	    rs = SR_INVALID ;
	    goto bad1 ;
	}

/* OK, get all of the entry points we care about */

	sp = loadfname ;		/* large enough to hold sym-name */
	cp = strwcpy(sp,bn,(MAXPATHLEN - 1)) ;

	rlen = (MAXPATHLEN - 1) - (cp - sp) ;

#if	CF_DEBUGS
	debugprintf("bpeval_add: get symbols sp=%s\n",sp) ;
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
	debugprintf("bpeval_add: objsize=%d rows=%d\n",e.size,op->rows) ;
#endif

	size = op->rows * e.size ;

#if	CF_DEBUGS
	debugprintf("bpeval_add: size=%d\n",size) ;
#endif

	rs = uc_malloc(size,&e.op) ;

#if	CF_DEBUGS
	debugprintf("bpeval_add: uc_malloc() rs=%d\n",rs) ;
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
	    debugprintf("bpeval_add: <bp>_init() rs=%d\n",rs) ;
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

/* we're good and we're out of here! */

	return rs ;

/* bad stuff */
bad3:
	for (i = 0 ; i < op->rows ; i += 1) {

	    char	*cop ;


	    cop = ((char *) e.op) + (i * e.size) ;
	    (void) (*e.call.free)(cop) ;

	}

bad2:
	uc_free(e.op) ;

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
	        debugprintf("bpeval_free: bp=%s lu=%u cor=%u\n",
	            ep->s.name,ep->s.lookups,ep->s.corrects) ;
#endif

	        for (j = 0 ; j < op->rows ; j += 1) {

	            char	*cop ;


	            cop = ((char *) ep->op) + (j * ep->size) ;
	            (*ep->call.free)(cop) ;

	        }

	    }

	    uc_free(ep->op) ;

	    dlclose(ep->dlp) ;

	} /* end for */

	bpfifo_free(&op->fifo) ;

	vecitem_finish(&op->bps) ;

	if (op->dir != NULL) {
	    uc_free(op->dir) ;
	    op->dir = NULL ;
	}

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
#endif /* F_SAFE */

	j = in % op->rows ;

#if	CF_DEBUGS
	debugprintf("bpeval_lookup: in=%u ia=%08x row=%d\n",
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
	        debugprintf("bpeval_lookup: bp=%s f_pred=%d\n",
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
#endif /* F_SAFE */

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
#endif /* F_SAFE */

	j = in % op->rows ;

#if	CF_DEBUGS
	debugprintf("bpeval_outcome: in=%u ia=%08x row=%d foc=%d\n",
	    in,ia,j,f_outcome) ;
#endif

/* accumulate statistics if requested */

	if (f_se) {

	    for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	        if (ep == NULL) continue ;

#if	CF_DEBUGS
	        debugprintf("bpeval_outcome: bp=%s f_pred=%d\n",
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
#endif /* F_SAFE */

	rs = bpfifo_read(&op->fifo,&fin,&fia,&frow,&foutcome) ;

	lin = in ;
	if ((rs >= 0) && (lin >= fin)) {

#if	CF_MASTERDEBUG && CF_DEBUGS
	    debugprintf("bpeval_checkmid: in=%u fin=%llu fia=%08x\n",
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
#endif /* F_SAFE */

	for (i = 0 ; vecitem_get(&op->bps,i,&ep) >= 0 ; i += 1) {

	    if (ep == NULL) continue ;

	    ep->bii = ep->pii ;
	    ep->pii = ep->cii ;
	    ep->cii = ii_zero ;

	} /* end for */

	return i ;
}
/* end subroutine (bpeval_checkend) */


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
	debugprintf("bpeval_stats: i=%d vecitem_get() rs=%d\n",i,rs) ;
#endif

	if (rs >= 0) {

	    memcpy(rp,&ep->s,sizeof(BPEVAL_STATS)) ;

	    if (ep->call.stats != NULL) {

	        char	*cop ;


	        cop = ((char *) ep->op) + (0 * ep->size) ;
	        rs = (*ep->call.stats)(cop,NULL) ;

#if	CF_DEBUGS
	        debugprintf("bpeval_stats: call.stats() rs=%d\n",rs) ;
#endif

	        if (rp != NULL)
	            rp->bits = rs ;

	    }

	}

	return rs ;
}
/* end subroutine (bpeval_stats) */


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
#endif /* F_SAFE */

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



/* INTERNAL SUBROUTINES */




