/* paramfile */

/* read in a file of parameters */


#define	CF_DEBUGS	0		/* non-switchable debug print-outs */
#define	CF_DEBUGSFIELD	1		/* extra debug print-outs */


/* revision history:

	- 2000-02-05, Dave Morano

	Some code for this subroutine was taken from something that
	did something similar to what we are doing here.  The rest was
	originally written for LevoSim.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This object reads in the parameter file and makes the parameter
	pairs available thought a key search.


******************************************************************************/


#define	PARAMFILE_MASTER	1


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<regexpr.h>
#include	<netdb.h>

#include	"vsystem.h"
#include	<bfile.h>
#include	<field.h>
#include	<vecobj.h>
#include	<vecstr.h>
#include	<sbuf.h>
#include	<char.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"paramfile.h"



/* local defines */

#define	PARAMFILE_MAGIC		0x12349876
#define	PARAMFILE_ICHECKTIME	2	/* file check interval (seconds) */
#define	PARAMFILE_ICHANGETIME	2	/* wait change interval (seconds) */

#define	PARAMFILE_FILE		struct paramfile_file

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif

#undef	BUFLEN
#define	BUFLEN		((20 * MAXPATHLEN) + (10 * MAXHOSTNAMELEN))



/* external subroutines */

extern int	mkpath2(char *,const char *,const char *) ;
extern int	getpwd(char *,int) ;

extern char	*strwcpy(char *,const char *,int) ;


/* external variables */


/* local structures */

struct paramfile_file {
	char	*fname ;
	time_t	mtime ;
	dev_t	dev ;
	ino_t	ino ;
	int	size ;
} ;


/* forward references */

int		paramfile_fileadd(PARAMFILE *,const char *) ;

static int	paramfile_getpwd(PARAMFILE *) ;
static int	paramfile_freefiles(PARAMFILE *) ;
static int	paramfile_freeentries(PARAMFILE *) ;
static int	paramfile_fileparse(PARAMFILE *,int) ;
static int	paramfile_filedump(PARAMFILE *,int) ;
static int	paramfile_filedel(PARAMFILE *,int) ;
static int	paramfile_addentry() ;
static int	paramfile_subentry(PARAMFILE *,PARAMFILE_ENTRY *,char **) ;
static int	paramfile_loaddefs(PARAMFILE *) ;

static int	file_init(struct paramfile_file *,const char *) ;
static int	file_release(struct paramfile_file *) ;
static int	file_free(struct paramfile_file *) ;

static int	entry_init(PARAMFILE_ENTRY *,int) ;
static int	entry_load(PARAMFILE_ENTRY *,char *,int,char *,int) ;
static int	entry_enough(PARAMFILE_ENTRY *) ;
static int	entry_release(PARAMFILE_ENTRY *) ;
static int	entry_free(PARAMFILE_ENTRY *) ;

static int	cmpfunc() ;

static void	freeit(char **) ;


/* local variables */

/* key field terminators (pound, equal, colon, and all white space) */
static const unsigned char 	key_terms[32] = {
	0x00, 0x1B, 0x00, 0x00,
	0x09, 0x00, 0x00, 0x20,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
} ;

/* argument field terminators (pound and all white space) */
static const unsigned char 	arg_terms[32] = {
	0x00, 0x1B, 0x00, 0x00,
	0x09, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
} ;


/* exported subroutines */


int paramfile_open(op,envv,fname)
PARAMFILE		*op ;
const char		*envv[] ;
const char		fname[] ;
{
	int	rs = SR_OK ;
	int	size ;


	if (op == NULL)
	    return SR_FAULT ;

	if (fname == NULL)
	    return SR_FAULT ;

	if (fname[0] == '\0')
	    return SR_INVALID ;

#if	CF_DEBUGS
	debugprintf("paramfile_open: fname=%s\n",fname) ;
#endif

	memset(op,0,sizeof(PARAMFILE)) ;

	op->envv = envv ;
	op->checktime = time(NULL) ;

	op->f.init_vsd = op->f.init_vse = FALSE ;

/* initialize */

	size = sizeof(PARAMFILE_FILE) ;
	rs = vecobj_init(&op->files,size,10,VECOBJ_PNOHOLES) ;
	if (rs < 0)
	    goto bad1 ;

/* keep this not-sorted so that the original order is maintained */

	size = sizeof(PARAMFILE_ENTRY) ;
	rs = vecobj_init(&op->entries,size,10,VECOBJ_PNOHOLES) ;
	if (rs < 0)
	    goto bad2 ;

#if	CF_DEBUGS
	debugprintf("paramfile_open: about to load environment\n") ;
#endif

	if (op->envv != NULL) {

	    rs = varsub_init(&op->e,VARSUB_MBADNOKEY) ;
	    if (rs < 0)
	        goto bad3 ;

	    op->f.init_vse = TRUE ;
	    rs = varsub_loadenv(&op->e,op->envv) ;
	    if (rs < 0)
		goto bad4 ;

	} /* end if */

#if	CF_DEBUGS
	debugprintf("paramfile_open: about to parse, file=%s\n",fname) ;
#endif

	op->magic = PARAMFILE_MAGIC ;
	if (fname != NULL) {

	    rs = paramfile_fileadd(op,fname) ;
	    if (rs < 0)
	        goto bad4 ;

	} /* end if (had an initial file to load) */

ret0:

#if	CF_DEBUGS
	debugprintf("paramfile_open: ret rs=%d\n",rs) ;
#endif

	return rs ;

/* handle bad things */
bad4:
	if (op->f.init_vse)
	    varsub_free(&op->e) ;

bad3:
	vecobj_free(&op->entries) ;

bad2:
	vecobj_free(&op->files) ;

bad1:
	op->magic = 0 ;
	goto ret0 ;
}
/* end subroutine (paramfile_open) */


/* load up the defines for substitution purposes */
int paramfile_setdefines(op,dvp)
PARAMFILE		*op ;
VECSTR			*dvp ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

	if (dvp == NULL)
	    return SR_FAULT ;

/* if we already had some, load them into the VARSUB object */

	if (op->dvp != NULL) {

	    if (op->f.init_vsd)
	        varsub_free(&op->d) ;

	    rs = varsub_init(&op->d,0) ;
	    if (rs < 0)
	        goto bad0 ;

	    op->f.init_vsd = TRUE ;
	    rs = paramfile_loaddefs(op) ;
	    if (rs < 0)
	        goto bad1 ;

	} /* end if */

/* OK, set the new ones (for possible future use) */

	op->dvp = dvp ;

ret0:
	return rs ;

/* bad stuff */
bad1:
	varsub_free(&op->d) ;

bad0:
	goto ret0 ;
}
/* end subroutine (paramfile_setdefines) */


/* add a file to the list of files */
int paramfile_fileadd(op,fname)
PARAMFILE		*op ;
const char		fname[] ;
{
	PARAMFILE_FILE	fe ;

	int	rs = SR_OK ;
	int	fi ;

	char	tmpfname[MAXPATHLEN + 1] ;
	char	*fnp ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

#if	CF_DEBUGS
	debugprintf("paramfile_fileadd: fname=%s\n",fname) ;
#endif

	fnp = (char *) fname ;
	if (fname[0] != '/') {

#if	CF_DEBUGS
	debugprintf("paramfile_fileadd: paramfile_getpwd()\n") ;
#endif

	    fnp = tmpfname ;
	    rs = paramfile_getpwd(op) ;

#if	CF_DEBUGS
	debugprintf("paramfile_fileadd: paramfile_getpwd() rs=%d\n",rs) ;
#endif

	    if (rs >= 0)
	        rs = mkpath2(tmpfname,op->pwd,fname) ;

	} /* end if (rooting file) */

#if	CF_DEBUGS
	debugprintf("paramfile_fileadd: fnp=%s\n",fnp) ;
#endif

	if (rs >= 0)
	    rs = file_init(&fe,fnp) ;

	if (rs < 0)
	    goto bad1 ;

	rs = vecobj_add(&op->files,&fe) ;
	fi = rs ;
	if (rs < 0)
	    goto bad2 ;

	file_release(&fe) ;

#if	CF_DEBUGS
	debugprintf("paramfile_fileadd: paramfile_fileparse()\n") ;
#endif

	rs = paramfile_fileparse(op,fi) ;
	if (rs < 0)
	    goto bad3 ;

ret0:

#if	CF_DEBUGS
	debugprintf("paramfile_fileadd: ret rs=%d\n",rs) ;
#endif

	return rs ;

/* handle bad things */
bad3:
	paramfile_filedel(op,fi) ;

bad2:
	file_free(&fe) ;

bad1:
bad0:
	goto ret0 ;
}
/* end subroutine (paramfile_fileadd) */


/* free up the resources occupied by a PARAMFILE list object */
int paramfile_close(op)
PARAMFILE		*op ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

#if	CF_DEBUGS
	debugprintf("paramfile_close: entered\n") ;
#endif

/* secondary items */

	paramfile_freeentries(op) ;

	paramfile_freefiles(op) ;

/* primary items */

	if (op->f.init_vsd)
	    varsub_free(&op->d) ;

	if (op->f.init_vse)
	    varsub_free(&op->e) ;

	vecobj_free(&op->entries) ;

	vecobj_free(&op->files) ;

	if (op->pwd != NULL) {
	    uc_free(op->pwd) ;
	    op->pwd = NULL ;
	}

	op->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (paramfile_close) */


/* cursor manipulations */
int paramfile_cursorinit(op,cp)
PARAMFILE		*op ;
PARAMFILE_CURSOR	*cp ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

	cp->i = -1 ;
	return SR_OK ;
}
/* end subroutine (paramfile_cursorinit) */


int paramfile_cursorfree(op,cp)
PARAMFILE		*op ;
PARAMFILE_CURSOR	*cp ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

	cp->i = -1 ;
	return SR_OK ;
}
/* end subroutine (paramfile_cursorfree) */


/* search the parameters for a match */
int paramfile_fetch(op,key,cp,vbuf,vbuflen)
PARAMFILE		*op ;
PARAMFILE_CURSOR	*cp ;
const char		key[] ;
char			vbuf[] ;
int			vbuflen ;
{
	PARAMFILE_ENTRY	ke, *pep ;

	VECOBJ	*slp ;

	int	rs ;
	int	i, j ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

	if ((key == NULL) || (key[0] == '\0'))
	    return SR_INVALID ;

#if	CF_DEBUGS
	debugprintf("paramfile_fetch: entered, key=%s\n",key) ;
#endif

/* create a key entry */

	ke.fi = -1 ;
	ke.key = (char *) key ;

/* go! */

	slp = &op->entries ;
	if (cp == NULL) {

#if	CF_DEBUGS
	    debugprintf("paramfile_fetch: NULL cursor\n") ;
#endif

	    rs = vecobj_search(slp,&ke,cmpfunc,(void **) &pep) ;

	} else {

#if	CF_DEBUGS
	    debugprintf("paramfile_fetch: non-NULL cursor\n") ;
#endif

	    j = (cp->i < 0) ? 0 : (cp->i + 1) ;
	    for (i = j ; (rs = vecobj_get(slp,i,&pep)) >= 0 ; i += 1) {

#if	CF_DEBUGS
	        debugprintf("paramfile_fetch: loop i=%d\n",i) ;
#endif

	        if (pep == NULL) continue ;

	        if (pep->key == NULL) continue ;

#if	CF_DEBUGS
	        debugprintf("paramfile_fetch: got entry kl=%d key=%s\n",
	            pep->klen,pep->key) ;
#endif

	        if (strcmp(key,pep->key) == 0)
	            break ;

	    } /* end for (looping through entries) */

	    if (rs >= 0)
	        cp->i = i ;

	} /* end if */

/* OK, if we have one, let's perform the substitutions on it */

	if ((rs >= 0) && (pep != NULL)) {

	    char	*cp, *vp ;


	    rs = paramfile_subentry(op,pep,&vp) ;

	    if ((rs >= 0) && (vbuf != NULL)) {

	        cp = strwcpy(vbuf,vp,vbuflen) ;

	        rs = (vp[cp - vbuf] == '\0') ? (cp - vbuf) : SR_OVERFLOW ;
	    }
	}

#if	CF_DEBUGS
	debugprintf("paramfile_fetch: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (paramfile_fetch) */


/* enumerate the entries */
int paramfile_enum(op,cp,ep,ebuf,ebuflen)
PARAMFILE		*op ;
PARAMFILE_CURSOR	*cp ;
PARAMFILE_ENTRY		*ep ;
char			ebuf[] ;
int			ebuflen ;
{
	PARAMFILE_ENTRY	*pep ;

	VECOBJ	*slp ;

	int	rs ;
	int	i, j ;
	int	kl = 0 ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

	if (cp == NULL)
	    return SR_FAULT ;

	if ((ep == NULL) || (ebuf == NULL))
	    return SR_FAULT ;

#if	CF_DEBUGS
	debugprintf("paramfile_enum: entered, i=%d\n",cp->i) ;
#endif

	slp = &op->entries ;
	i = (cp->i < 0) ? 0 : (cp->i + 1) ;
	for (j = i ; (rs = vecobj_get(slp,j,&pep)) >= 0 ; j += 1) {

	    if (pep != NULL)
	        break ;

	} /* end for */

#if	CF_DEBUGS
	debugprintf("paramfile_enum: vecobj_get j=%d rs=%d\n",j,rs) ;
#endif

	cp->i = j ;

#if	CF_DEBUGS
	debugprintf("paramfile_enum: intermediate rs=%d cur_i=%d \n",
	    rs,cp->i) ;
#endif

	if ((rs >= 0) && (pep != NULL)) {

	    char	*cp, *vp ;


	    rs = paramfile_subentry(op,pep,&vp) ;

	    if (rs >= 0) {

	        memset(ep,0,sizeof(PARAMFILE_ENTRY)) ;

	        if ((ebuflen < 0) ||
	            (ebuflen >= (pep->klen + pep->vlen + 2))) {

	            cp = ebuf ;
	            ep->key = cp ;
	            ep->klen = strwcpy(cp,pep->key,pep->klen) - cp ;
		    kl = ep->klen ;

	            cp += (ep->klen + 1) ;
	            ep->value = cp ;
	            rs = strwcpy(cp,vp,-1) - cp ;
	            ep->vlen = rs ;

	        } else
	            rs = SR_OVERFLOW ;

	    }

	} /* end if (got one) */

#if	CF_DEBUGS
	debugprintf("paramfile_enum: ret rs=%d kl=%u\n",rs,kl) ;
#endif

	return (rs >= 0) ? kl : rs ;
}
/* end subroutine (paramfile_enum) */


/* check if the parameter file has changed */
int paramfile_check(op,daytime)
PARAMFILE		*op ;
time_t			daytime ;
{
	USTAT	sb ;

	PARAMFILE_FILE	*fep ;

	int	rs ;
	int	i ;
	int	c_changed = 0 ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != PARAMFILE_MAGIC)
	    return SR_NOTOPEN ;

/* should we even check? */

	if ((daytime - op->checktime) <= PARAMFILE_ICHECKTIME)
	    return SR_OK ;

	op->checktime = daytime ;

/* check the files */

#if	CF_DEBUGS
	debugprintf("paramfile_check: about to loop\n",i) ;
#endif

	rs = SR_OK ;
	for (i = 0 ; vecobj_get(&op->files,i,&fep) >= 0 ; i += 1) {

	    if (fep == NULL) continue ;

	    if ((u_stat(fep->fname,&sb) >= 0) &&
	        (sb.st_mtime > fep->mtime) &&
	        ((daytime - sb.st_mtime) >= PARAMFILE_ICHANGETIME)) {

#if	CF_DEBUGS
	        debugprintf("paramfile_check: file=%d changed\n",i) ;
	        debugprintf("paramfile_check: freeing file entries\n") ;
#endif

	        paramfile_filedump(op,i) ;

#if	CF_DEBUGS
	        debugprintf("paramfile_check: parsing the file again\n") ;
#endif

	        rs = paramfile_fileparse(op,i) ;
	        if (rs >= 0)
	            c_changed += 1 ;

#if	CF_DEBUGS
	        debugprintf("paramfile_check: paramfile_fileparse rs=%d\n",rs) ;
	        debugprintf("paramfile_check: sorting STD entries\n") ;
#endif

#ifdef	COMMENT
	        vecobj_sort(&op->aes_std,cmpfunc) ;
#endif

	    } /* end if */

	} /* end for */

#if	CF_DEBUGS
	debugprintf("paramfile_check: ret rs=%d changed=%d\n",
	    rs,c_changed) ;
#endif

	return (rs >= 0) ? c_changed : rs ;
}
/* end subroutine (paramfile_check) */


/* private subroutines */


static int paramfile_getpwd(op)
PARAMFILE		*op ;
{
	USTAT	sb ;

	int	rs = SR_OK ;

	char	tmpfname[MAXPATHLEN + 1] ;


	if ((op->pwd != NULL) &&
		(op->pwd[0] != '\0') && (op->pwd_len > 0)) {

	    rs = u_stat(".",&sb) ;

	    if (rs >= 0) {
	        if ((sb.st_ino != op->pwd_ino) ||
	            (sb.st_dev != op->pwd_dev)) {
	            op->pwd[0] = '\0' ;
	            op->pwd_len = 0 ;
	        }
	    }
	}

	if ((op->pwd == NULL) || (op->pwd[0] == '\0')) {

/* load up the PWD */

	    op->pwd_len = 0 ;
	    rs = getpwd(tmpfname,MAXPATHLEN) ;

	    if (rs >= 0)
	        op->pwd_len = rs ;

/* get the new information on the PWD */

	    rs = u_stat(tmpfname,&sb) ;

	    if (rs >= 0) {
	        op->pwd_ino = sb.st_ino ;
	        op->pwd_dev = sb.st_dev ;
		rs = uc_mallocstrw(tmpfname,op->pwd_len,&op->pwd) ;
	    }

	} /* end if (needed new PWD) */

	return (rs >= 0) ? op->pwd_len : rs ;
}
/* end subroutine (paramfile_getpwd) */


/* parse a parameter file */
static int paramfile_fileparse(op,fi)
PARAMFILE		*op ;
int			fi ;
{
	USTAT	sb ;

	bfile		loadfile, *lfp = &loadfile ;

	PARAMFILE_FILE	*fep ;

	PARAMFILE_ENTRY	pe ;

	FIELD	fsb ;

	int	rs ;
	int	i, c, len ;
	int	klen, vlen ;
	int	cl, fl ;
	int	c_added = 0 ;
	int	line = 0 ;

	char	linebuf[LINEBUFLEN + 1] ;
	char	*key ;
	char	*cp ;
	char	*fp ;


#if	CF_DEBUGS
	debugprintf("paramfile_fileparse: entered fi=%u\n",fi) ;
#endif

	rs = vecobj_get(&op->files,fi,&fep) ;
	if (rs < 0)
	    goto bad0 ;

	if (fep == NULL) {
	    rs = SR_NOTFOUND ;
	    goto bad0 ;
	}

#if	CF_DEBUGS
	debugprintf("paramfile_fileparse: fname=%s\n",fep->fname) ;
#endif

	rs = bopen(lfp,fep->fname,"r",0664) ;
	if (rs < 0)
	    goto bad0 ;

#if	CF_DEBUGS
	debugprintf("paramfile_fileparse: bopen rs=%d\n", rs) ;
#endif

	rs = bcontrol(lfp,BC_STAT,&sb) ;
	if (rs < 0)
	    goto bad0 ;

/* have we already parsed this one? */

#if	CF_DEBUGS
	debugprintf("paramfile_fileparse: 4\n") ;
#endif

	rs = SR_OK ;
	if (fep->mtime >= sb.st_mtime)
	    goto done ;

#if	CF_DEBUGS
	debugprintf("paramfile_fileparse: 5\n") ;
#endif

	fep->dev = sb.st_dev ;
	fep->ino = sb.st_ino ;
	fep->mtime = sb.st_mtime ;
	fep->size = sb.st_size ;

/* start processing the configuration file */

#if	CF_DEBUGS
	debugprintf("paramfile_fileparse: start processing\n") ;
#endif

	while ((rs = breadlines(lfp,linebuf,LINEBUFLEN,NULL)) > 0) {

	    len = rs ;
	    line += 1 ;
	    if (len == 1) continue ;	/* blank line */

	    if (linebuf[len - 1] != '\n') {
	        while ((c = bgetc(lfp)) >= 0) {
	            if (c == '\n')
	                break ;
	        }
	        continue ;
	    }


	    cp = linebuf ;
	    cl = len ;
	    linebuf[--cl] = '\0' ;
	    while (CHAR_ISWHITE(*cp)) {
	        cp += 1 ;
	        cl -= 1 ;
	    }

	    if ((*cp == '\0') || (*cp == '#')) continue ;

#if	CF_DEBUGS && CF_DEBUGSFIELD
	    debugprintf("paramfile_fileparse: line> %w\n",
	        cp,len) ;
#endif

	    if ((rs = field_init(&fsb,cp,cl)) >= 0) {

	        fl = field_get(&fsb,key_terms,&fp) ;

#if	CF_DEBUGS && CF_DEBUGSFIELD
	        debugprintf("paramfile_fileparse: field key>%w<\n",
	            fp,fl) ;
#endif

/* ignore empty or comment-only lines */

	        if (fl > 0) {

	            SBUF	vb ;

	            char	vbuf[LINEBUFLEN + 1] ;


/* we have something! */

	            key = fp ;
	            klen = fl ;
	            entry_init(&pe,fi) ;

/* do we have a value? */

	            rs = sbuf_init(&vb,vbuf,LINEBUFLEN) ;

	            vlen = 0 ;
	            i = 0 ;
	            while ((rs >= 0) && 
	                ((fl = field_get(&fsb,arg_terms,&fp)) >= 0)) {

#if	CF_DEBUGS && CF_DEBUGSFIELD
	                debugprintf("paramfile_fileparse: field value>%w<\n",
	                    fp,fl) ;
#endif

	                if (fl > 0) {

	                    if (i > 0)
	                        sbuf_char(&vb,' ') ;

	                    sbuf_strw(&vb,fp,fl) ;

	                    i += 1 ;

	                } /* end if */

	                if (fsb.term == '#')
	                    break ;

	            } /* end while (fields) */

	            len = sbuf_getlen(&vb) ;

	            sbuf_free(&vb) ;

	            vlen = len ;
	            if ((rs >= 0) && (klen > 0))
	                rs = entry_load(&pe,key,klen,vbuf,vlen) ;

	            if (rs >= 0) {

	                rs = paramfile_addentry(op,&pe) ;

#if	CF_DEBUGS
	                debugprintf("paramfile_fileparse: added entry rs=%d\n",
	                    rs) ;
#endif

#ifdef	COMMENT
	                if (rs >= 0)
	                    entry_release(&pe) ;
#endif

	            } /* end if */

	            if (rs < 0)
	                entry_free(&pe) ;

	        } /* end if (got a key) */

	        field_free(&fsb) ;

	    } /* end if (field) */

	    if (rs < 0)
	        break ;

	    c_added += 1 ;

	} /* end while (reading lines) */

	if (rs < 0)
	    goto bad1 ;

/* done with configuration file processing */
done:
ret1:
	bclose(lfp) ;

ret0:

#if	CF_DEBUGS
	debugprintf("paramfile_fileparse: ret rs=%d added=%d\n",
	    rs,c_added) ;
#endif

	return (rs >= 0) ? c_added : rs ;

/* bad stuff */
bad1:
	paramfile_filedump(op,fi) ;

bad0:
	goto ret1 ;
}
/* end subroutine (paramfile_fileparse) */


/* add an entry to something */
static int paramfile_addentry(op,pep)
PARAMFILE		*op ;
PARAMFILE_ENTRY		*pep ;
{
	int	rs ;


	rs = vecobj_add(&op->entries,pep) ;

	return rs ;
}
/* end subroutine (paramfile_addentry) */


/* perform the variable substitution on an entry */
static int paramfile_subentry(op,pep,vpp)
PARAMFILE		*op ;
PARAMFILE_ENTRY		*pep ;
char			**vpp ;
{
	int	rs = SR_OK ;
	int	rs1 ;

	char	*v ;


#if	CF_DEBUGS
	debugprintf("paramfile_subentry: entered\n") ;
#endif

	if (pep == NULL)
	    return SR_FAULT ;

	v = NULL ;
	if (pep->orig != NULL) {

#if	CF_DEBUGS
	    debugprintf("paramfile_subentry: orig=>%s<\n",pep->orig) ;
#endif

	    rs = pep->vlen ;
	    if (pep->value == NULL) {

	        int	rs2 ;

	        char	buf[BUFLEN + 1] ;


#if	CF_DEBUGS
	        debugprintf("paramfile_subentry: processing\n") ;
#endif

	        if (op->dvp != NULL) {

#if	CF_DEBUGS
	            debugprintf("paramfile_subentry: have defines\n") ;
#endif

	            if (! op->f.init_vsd) {

#if	CF_DEBUGS
	                debugprintf("paramfile_subentry: loading defines %s\n",
	                    ((op->dvp != NULL) ? "OK" : "BAD")) ;
#endif

	                op->f.init_vsd = TRUE ;
	                rs1 = varsub_init(&op->d,VARSUB_MBADNOKEY) ;

	                if (rs1 >= 0)
	                    rs1 = paramfile_loaddefs(op) ;

	            } /* end if (loading defines) */

#if	CF_DEBUGS
	            debugprintf("paramfile_subentry: 1 varsub_buf()\n") ;
#endif

	            rs2 = varsub_buf(&op->d,pep->orig,pep->vlen,
	                buf,BUFLEN) ;

#if	CF_DEBUGS
	            debugprintf("paramfile_subentry: varsub_buf() rs2=%d\n",
	                rs2) ;
#endif

	        } else
	            rs2 = SR_NOTFOUND ;

#if	CF_DEBUGS
	        debugprintf("paramfile_subentry: try 1 rs=%d\n",rs2) ;
#endif

	        if (rs2 < 0) {

#if	CF_DEBUGS
	            debugprintf("paramfile_subentry: trying 2\n") ;
#endif

	            if (! op->f.init_vse) {

#if	CF_DEBUGS
	                debugprintf("paramfile_subentry: loading ENVs\n") ;
#endif

	                op->f.init_vse = TRUE ;
	                varsub_loadenv(&op->e,op->envv) ;

	            } /* end if (loading environment) */

#if	CF_DEBUGS
	            debugprintf("paramfile_subentry: 2 varsub_buf()\n") ;
#endif

	            rs2 = varsub_buf(&op->e,pep->orig,pep->vlen,
	                buf,BUFLEN) ;

#if	CF_DEBUGS
	            debugprintf("paramfile_subentry: varsub_buf() rs=%d\n",
	                rs2) ;
#endif

	        } /* end if (second try) */

#if	CF_DEBUGS
	        debugprintf("paramfile_subentry: store or not, rs2=%d\n",
	            rs2) ;
#endif

	        if (rs2 >= 0) {

	            rs = rs2 ;
	            rs2 = uc_malloc((rs2 + 1),&pep->value) ;

	            if (rs2 > 0) {

	                v = pep->value ;
	                pep->vlen = rs ;
	                strcpy(pep->value,buf) ;

	            } else
	                rs = rs2 ;

	        } else
	            v = pep->orig ;

	    } else
	        v = pep->value ;

#if	CF_DEBUGS
	    debugprintf("paramfile_subentry: rs=%d v=>%s<\n",rs,v) ;
#endif

	} /* end if */

	if ((rs >= 0) && (vpp != NULL))
	    *vpp = v ;

	return rs ;
}
/* end subroutine (paramfile_subentry) */


/* free up all of the files in this PARAMFILE list */
static int paramfile_freefiles(op)
PARAMFILE		*op ;
{
	PARAMFILE_FILE	*fep ;

	int	i ;


	for (i = 0 ; vecobj_get(&op->files,i,&fep) >= 0 ; i += 1) {

	    if (fep == NULL) continue ;

	    file_free(fep) ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (paramfile_freefiles) */


/* free up ALL of the entries in this PARAMFILE list */
static int paramfile_freeentries(op)
PARAMFILE		*op ;
{
	PARAMFILE_ENTRY	*pep ;

	int	i ;


#if	CF_DEBUGS
	debugprintf("paramfile_freeentries: entered\n") ;
#endif

	for (i = 0 ; vecobj_get(&op->entries,i,&pep) >= 0 ; i += 1) {

	    if (pep == NULL) continue ;

#if	CF_DEBUGS
	    debugprintf("paramfile_freeentries: i=%d pep=%p\n",i,pep) ;
#endif

	    entry_free(pep) ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (paramfile_freeentries) */


/* free up all of the entries in this PARAMFILE list associated w/ a file */
static int paramfile_filedump(op,fi)
PARAMFILE		*op ;
int			fi ;
{
	PARAMFILE_ENTRY	*pep ;

	VECOBJ		*slp ;

	int		i ;


#if	CF_DEBUGS
	debugprintf("paramfile_filedump: want to delete all fi=%d\n",fi) ;
#endif

	slp = &op->entries ;
	for (i = 0 ; vecobj_get(slp,i,&pep) >= 0 ; i += 1) {

	    if (pep == NULL) continue ;

#if	CF_DEBUGS
	    debugprintf("paramfile_filedump: i=%d fi=%d\n",i,pep->fi) ;
#endif

	    if ((pep->fi == fi) || (fi < 0)) {

#if	CF_DEBUGS
	        debugprintf("paramfile_filedump: got one\n") ;
#endif

	        entry_free(pep) ;

	        vecobj_del(slp,i--) ;

	    } /* end if (got one) */

	} /* end for */

#if	CF_DEBUGS
	debugprintf("paramfile_filedump: ret\n") ;
#endif

	return SR_OK ;
}
/* end subroutine (paramfile_filedump) */


static int paramfile_filedel(op,fi)
PARAMFILE		*op ;
int			fi ;
{
	PARAMFILE_FILE	*fep ;

	int	rs ;


	rs = vecobj_get(&op->files,fi,&fep) ;

	if ((rs >= 0) && (fep != NULL)) {

	    file_free(fep) ;

	    rs = vecobj_del(&op->files,fi) ;

	}

	return rs ;
}
/* end subroutine (paramfile_filedel) */


/* load up something */
static int paramfile_loaddefs(op)
PARAMFILE	*op ;
{
	int	rs ;
	int	i ;

	char	*tp, *cp ;


	for (i = 0 ; vecstr_get(op->dvp,i,&cp) >= 0 ; i += 1) {

	    if (cp == NULL) continue ;

	    tp = strchr(cp,'=') ;

	    if (tp != NULL)
	        rs = varsub_add(&op->d,cp,(tp - cp),(tp + 1),-1) ;

	    else
	        rs = varsub_add(&op->d,cp,-1,NULL,0) ;

	} /* end for */

	return rs ;
}
/* end subroutine (paramfile_loaddefs) */


static int file_init(fep,fname)
PARAMFILE_FILE	*fep ;
const char	fname[] ;
{
	int	rs = SR_OK ;


	if (fname == NULL)
	    return SR_FAULT ;

	memset(fep,0,sizeof(PARAMFILE_FILE)) ;

	fep->fname = mallocstr(fname) ;

	if (fep->fname == NULL)
	    rs = SR_NOMEM ;

	return rs ;
}
/* end subroutine (file_init) */


static int file_release(fep)
PARAMFILE_FILE	*fep ;
{


	fep->fname = NULL ;
	return 0 ;
}


static int file_free(fep)
PARAMFILE_FILE	*fep ;
{


	if (fep == NULL)
	    return SR_FAULT ;

	if (fep->fname != NULL)
	    uc_free(fep->fname) ;

	fep->fname = NULL ;
	return 0 ;
}
/* end subroutine (file_free) */


/* initialize an entry */
static int entry_init(pep,fi)
PARAMFILE_ENTRY	*pep ;
int		fi ;
{


	pep->fi = fi ;
	pep->key = NULL ;
	pep->orig = NULL ;
	pep->value = NULL ;
	return 0 ;
}
/* end subroutine (entry_init) */


static int entry_load(pep,k,klen,v,vlen)
PARAMFILE_ENTRY	*pep ;
char		k[], v[] ;
int		klen, vlen ;
{
	int	len ;
	int	rs ;

	char	*mkp, *mvp ;


	pep->key = NULL ;
	pep->orig = NULL ;
	pep->value = NULL ;

	if (k == NULL)
	    return SR_INVALID ;

	if (klen < 0)
	    klen = strlen(k) ;

	pep->klen = klen ;
	pep->vlen = -1 ;
	if (v != NULL) {

	    if (vlen < 0)
	        vlen = strlen(v) ;

	    pep->vlen = vlen ;

	} else
	    vlen = 0 ;

#if	CF_DEBUGS
	debugprintf("paramfile/entry_load: klen=%d vlen=%d\n",klen,vlen) ;
#endif

	len = klen + vlen + 2 ;
	rs = uc_malloc(len,&mkp) ;

	if (rs < 0)
	    return rs ;

	pep->key = mkp ;
	mvp = strwcpy(mkp,k,klen) + 1 ;

#if	CF_DEBUGS
	debugprintf("paramfile/entry_load: key=>%s<\n",mkp) ;
#endif

	if (v != NULL) {

	    pep->orig = mvp ;
	    strwcpy(mvp,v,vlen) ;

#if	CF_DEBUGS
	    debugprintf("paramfile/entry_load: value=>%s<\n",mvp) ;
#endif

	}

	return SR_OK ;
}
/* end subroutine (entry_load) */


static int entry_enough(pep)
PARAMFILE_ENTRY	*pep ;
{


	if (pep == NULL)
	    return SR_FAULT ;

	if (pep->key == NULL)
	    return SR_INVALID ;

	return SR_OK ;
}
/* end subroutine (entry_enough) */


#ifdef	COMMENT /* amazingly not needed since all entries are on one line ! */

static int entry_release(pep)
PARAMFILE_ENTRY	*pep ;
{


	pep->fi = -1 ;
	return SR_OK ;
}
/* end subroutine (entry_release) */

#endif /* COMMENT */


/* free up an entry */
static int entry_free(pep)
PARAMFILE_ENTRY	*pep ;
{


#if	CF_DEBUGS
	debugprintf("entry_free: entered this=%p\n",pep) ;
#endif

	pep->fi = -1 ;

#if	CF_DEBUGS
	debugprintf("entry_free: key=%p\n",pep->key) ;
#endif

	freeit(&pep->key) ;

#if	CF_DEBUGS
	debugprintf("entry_free: value=%p\n",pep->value) ;
#endif

	freeit(&pep->value) ;

	return 0 ;
}
/* end subroutine (entry_free) */


static void freeit(pp)
char	**pp ;
{


	if (*pp != NULL) {

	    uc_free(*pp) ;

	    *pp = NULL ;
	}
}
/* end subroutine (freeit) */


/* compare just the 'netgroup' part of entries (used for sorting) */
static int cmpfunc(e1pp,e2pp)
PARAMFILE_ENTRY	**e1pp, **e2pp ;
{


#if	CF_DEBUGS
	debugprintf("paramfile/cmpfunc: e1=%s e2=%s\n",
	    (*e1pp)->key,(*e2pp)->key) ;
#endif

	if ((*e1pp == NULL) && (*e2pp == NULL))
	    return 0 ;

	if (*e1pp == NULL)
	    return 1 ;

	if (*e2pp == NULL)
	    return -1 ;

	return strcmp((*e1pp)->key,(*e2pp)->key) ;
}
/* end subroutine (cmpfunc) */



