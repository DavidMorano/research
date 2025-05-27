/* debugsched */

/* read in a file of parameters */


#define	CF_DEBUGS	0
#define	CF_DEBUGSFIELD	0


/* revision history :

	- 01/02/05, Dave Morano

	Some code for this subroutine was taken from something
	that did something similar to what we are doing here.
	The rest was originally written for LevoSim.


*/


/******************************************************************************

	This object manages the Debug Schedule data base.  Among the
	subroutines available, there is something that will read in a
	Debug Sched file and put the entries into the database.


******************************************************************************/


#define	DEBUGSCHED_MASTER	1


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<netdb.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<cstdlib>
#include	<cstring>
#include	<ctype.h>

#include	<vsystem.h>
#include	<bio.h>
#include	<field.h>
#include	<vecitem.h>
#include	<vecstr.h>
#include	<char.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"debugsched.h"



/* local defines */

#define	DEBUGSCHED_ICHECKTIME	2	/* file check interval (seconds) */
#define	DEBUGSCHED_ICHANGETIME	2	/* wait change interval (seconds) */
#define	DEBUGSCHED_DEFNETGROUP	"DEFAULT"
#define	DEBUGSCHED_IMAGIC	0x12349476

#define	LINELEN			200
#undef	BUFLEN
#define	BUFLEN			((20 * MAXPATHLEN) + (10 * MAXHOSTNAMELEN))



/* external subroutines */

extern char	*strwcpy(char *,char *,int) ;


/* externals variables */


/* forward references */

static int	debugsched_parsefile() ;
static int	debugsched_freeentries(), debugsched_freefiles() ;
static int	debugsched_addentry() ;
static int	debugsched_freefile() ;

static int	entry_load(DEBUGSCHED_ENTRY *,char *,int,char *,int) ;
static int	entry_enough(DEBUGSCHED_ENTRY *) ;

static int	cmpfunc() ;

static void	entry_init(DEBUGSCHED_ENTRY *,int) ;
static void	entry_free(DEBUGSCHED_ENTRY *) ;
static void	freeit(char **) ;


/* local static data */

/* key field terminators (pound, colon, and all white space) */

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






int debugsched_init(pfp,envv,pfname,eep)
DEBUGSCHED	*pfp ;
char		pfname[] ;
char		*envv[] ;
VECITEM		*eep ;
{
	time_t	daytime ;

	int	rs = SR_OK ;
	int	rs2 ;


#if	CF_DEBUGS
	eprintf("debugsched_init: entered, file=%s\n",pfname) ;
#endif

	if (pfp == NULL)
	    return SR_FAULT ;

	pfp->f.init_vsd = pfp->f.init_vse = FALSE ;

	pfp->magic = 0 ;
	pfp->defines = NULL ;
	pfp->envv = envv ;
	(void) time(&pfp->checktime) ;

/* initialize */

	if ((rs = vecitem_init(&pfp->files,10,VECITEM_PNOHOLES)) < 0)
	    goto bad1 ;

	if ((rs = vecitem_init(&pfp->entries,10,VECITEM_PNOHOLES)) < 0)
	    goto bad2 ;

#if	CF_DEBUGS
	eprintf("debugsched_init: about to load environment\n") ;
#endif

	if (pfp->envv != NULL) {

	    rs = varsub_init(&pfp->e,VARSUB_MBADNOKEY) ;

#if	CF_DEBUGS
	    eprintf("debugsched_init: varsub_init rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        goto bad3 ;

	    pfp->f.init_vse = TRUE ;
	    (void) varsub_loadenv(&pfp->e,pfp->envv) ;

	} /* end if */

#if	CF_DEBUGS
	eprintf("debugsched_init: about to parse, file=%s\n",pfname) ;
#endif

	pfp->magic = DEBUGSCHED_IMAGIC ;
	rs = SR_OK ;
	if (pfname != NULL) {

	    rs2 = debugsched_addfile(pfp,pfname,eep) ;

#if	CF_DEBUGS
	    eprintf("debugsched_init: parsed result rs=%d\n",rs2) ;
#endif

	}

#if	CF_DEBUGS
	eprintf("debugsched_init: exiting rs=%d\n",rs) ;
#endif

	return rs ;

/* handle bad things */
bad3:
	(void) vecitem_free(&pfp->entries) ;

bad2:
	(void) vecitem_free(&pfp->files) ;

bad1:
	return rs ;
}
/* end subroutine (debugsched_init) */


int debugsched_setdefines(pfp,dvp)
DEBUGSCHED	*pfp ;
VECSTR		*dvp ;
{


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

	if (dvp == NULL)
	    return SR_FAULT ;

	if (pfp->defines != NULL) {

	    if (pfp->f.init_vsd)
	        varsub_free(&pfp->d) ;

	    pfp->f.init_vsd = TRUE ;
	    varsub_init(&pfp->d,0) ;

	    varsub_loadvec(&pfp->d,&pfp->defines) ;

	} /* end if */

	pfp->defines = dvp ;
	return SR_OK ;
}
/* end subroutine (debugsched_setdefines) */


/* add a file to the list of files */
int debugsched_addfile(pfp,pfname,eep)
DEBUGSCHED	*pfp ;
char		pfname[] ;
VECITEM		*eep ;
{
	DEBUGSCHED_FILE	fe ;

	int	rs, fi ;


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

#if	CF_DEBUGS
	eprintf("debugsched_addfile: entered, file=%s\n",pfname) ;
#endif

	if ((fe.filename = mallocstr(pfname)) == NULL) {

	    rs = SR_NOMEM ;
	    goto badmal ;
	}

	fe.mtime = 0 ;
	if ((rs = vecitem_add(&pfp->files,&fe,sizeof(DEBUGSCHED_FILE))) < 0)
	    goto badaddfile ;

	fi = rs ;

/* parse the file for the first time */

#if	CF_DEBUGS
	eprintf("debugsched_addfile: calling debugsched_parsefile\n") ;
#endif

	rs = debugsched_parsefile(pfp,fi,eep) ;

#if	CF_DEBUGS
	eprintf("debugsched_addfile: exiting, rs=%d\n",rs) ;
#endif

ret:
	return rs ;

/* handle bad things */
badparse:

badaddfile:
	(void) free(fe.filename) ;

badmal:
	return rs ;
}
/* end subroutine (debugsched_addfile) */


/* free up the resources occupied by an DEBUGSCHED list object */
int debugsched_free(pfp)
DEBUGSCHED		*pfp ;
{


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

	(void) debugsched_freeentries(pfp) ;

	(void) debugsched_freefiles(pfp) ;

	pfp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (debugsched_free) */


/* search the parameters for a match */
int debugsched_fetch(pfp,key,cp,vpp)
DEBUGSCHED		*pfp ;
DEBUGSCHED_CURSOR	*cp ;
char			key[] ;
char			**vpp ;
{
	DEBUGSCHED_ENTRY	ke, *pep ;

	VECITEM	*slp ;

	int	i, j ;
	int	vlen ;
	int	rs ;

	char	*v ;


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

	if ((key == NULL) || (key[0] == '\0'))
	    return SR_INVALID ;

#if	CF_DEBUGS
	eprintf("debugsched_fetch: entered, key=%s\n",key) ;
#endif

/* create a key entry */

	ke.fi = -1 ;
	ke.key = key ;

/* go ! */

	slp = &pfp->entries ;
	if (cp == NULL) {

#if	CF_DEBUGS
	    eprintf("debugsched_fetch: NULL cursor\n") ;
#endif

	    rs = vecitem_search(slp,&ke,cmpfunc,(void **) &pep) ;

	} else {

#if	CF_DEBUGS
	    eprintf("debugsched_fetch: non-NULL cursor\n") ;
#endif

	    j = (cp->i < 0) ? 0 : (cp->i + 1) ;
	    for (i = j ; (rs = vecitem_get(slp,i,&pep)) >= 0 ; i += 1) {

#if	CF_DEBUGS
	        eprintf("debugsched_fetch: loop i=%d\n",i) ;
#endif

	        if (pep == NULL) continue ;

	        if (pep->key == NULL) continue ;

#if	CF_DEBUGS
	        eprintf("debugsched_fetch: got entry\n") ;
#endif

	        if (strcmp(key,pep->key) == 0)
	            break ;

	    } /* end for (looping through entries) */

	    if (rs >= 0)
	        cp->i = i ;

	} /* end if */


/* OK, if we have one, let's perform the substitutions on it */

	if ((rs >= 0) && (pep != NULL)) {

	    rs = 0 ;
	    v = NULL ;
	    if (pep->orig != NULL) {

	        rs = pep->vlen ;
	        if (pep->value == NULL) {

	            int		rs2, rs3 ;

	            char	buf[BUFLEN + 1] ;


#if	CF_DEBUGS
	            eprintf("debugsched_fetch: processing\n") ;
#endif

	            if (pfp->defines != NULL) {

#if	CF_DEBUGS
	                    eprintf("debugsched_fetch: have defines\n") ;
#endif

	                if (! pfp->f.init_vsd) {

#if	CF_DEBUGS
	                    eprintf("debugsched_fetch: loading defines %s\n",
	                        ((pfp->defines != NULL) ? "OK" : "BAD")) ;
#endif

	                    pfp->f.init_vsd = TRUE ;
	                    varsub_init(&pfp->d,VARSUB_MBADNOKEY) ;

	                    varsub_loadvec(&pfp->d,pfp->defines) ;

	                } /* end if (loading defines) */

#if	CF_DEBUGS
	                eprintf("debugsched_fetch: define subbing\n") ;
#endif

	                rs2 = varsub_buf(&pfp->d,pep->orig,pep->vlen,
	                    buf,BUFLEN) ;

#if	CF_DEBUGS
	                    eprintf("debugsched_fetch: have defines, rs2=%d\n",
	                        rs2) ;
#endif

	            } else
	                rs2 = SR_NOTFOUND ;

#if	CF_DEBUGS
	            eprintf("debugsched_fetch: try 1 rs=%d\n",rs2) ;
#endif

	            if (rs2 < 0) {

#if	CF_DEBUGS
	            eprintf("debugsched_fetch: trying 2\n") ;
#endif

	                if (! pfp->f.init_vse) {

#if	CF_DEBUGS
	            eprintf("debugsched_fetch: loading ENVs\n") ;
#endif

	                    pfp->f.init_vse = TRUE ;
	                    varsub_loadenv(&pfp->e,pfp->envv) ;

	                } /* end if (loading environment) */

	                rs2 = varsub_buf(&pfp->e,pep->orig,pep->vlen,
	                    buf,BUFLEN) ;

#if	CF_DEBUGS
	                eprintf("debugsched_fetch: try 2 rs=%d\n",rs2) ;
#endif

	            } /* end if (second try) */

#if	CF_DEBUGS
	                eprintf("debugsched_fetch: store or not, rs2=%d\n",
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
	        eprintf("debugsched_fetch: rs=%d v=%s\n",rs,v) ;
#endif

	    } /* end if */

	    if (vpp != NULL)
	        *vpp = v ;

	} /* end if (substituting one) */


#if	CF_DEBUGS
	eprintf("debugsched_fetch: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (debugsched_fetch) */


/* cursor manipulations */
int debugsched_cursorinit(pfp,cp)
DEBUGSCHED		*pfp ;
DEBUGSCHED_CURSOR	*cp ;
{


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

	cp->i = -1 ;
	return SR_OK ;
}


int debugsched_cursorfree(pfp,cp)
DEBUGSCHED		*pfp ;
DEBUGSCHED_CURSOR	*cp ;
{


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

	cp->i = -1 ;
	return SR_OK ;
}


/* enumerate the entries */
int debugsched_enum(pfp,cp,pepp)
DEBUGSCHED		*pfp ;
DEBUGSCHED_CURSOR	*cp ;
DEBUGSCHED_ENTRY		**pepp ;
{
	VECITEM	*slp ;

	int	rs, i, j ;


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

	if (cp == NULL)
	    return SR_FAULT ;

	if (pepp == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	eprintf("debugsched_enum: entered, i=%d\n",cp->i) ;
#endif

	rs = SR_RANGE ;
	slp = &pfp->entries ;
	i = (cp->i < 0) ? 0 : (cp->i + 1) ;
	for (j = i ; (rs = vecitem_get(slp,j,pepp)) >= 0 ; j += 1)
	    if (*pepp != NULL) break ;

#if	CF_DEBUGS
	eprintf("debugsched_enum: vecitem_get j=%d rs=%d\n",j,rs) ;
#endif

	cp->i = j ;

#if	CF_DEBUGS
	eprintf("debugsched_enum: intermediate rs=%d cur_i=%d \n",
	    rs,cp->i) ;
#endif


#if	CF_DEBUGS
	eprintf("debugsched_enum: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (debugsched_enum) */


/* check if the parameter file has changed */
int debugsched_check(pfp,eep)
DEBUGSCHED	*pfp ;
VECITEM		*eep ;
{
	USTAT	sb ;

	DEBUGSCHED_FILE	*fep ;

	time_t	daytime ;

	int	rs, i ;
	int	c_changed = 0 ;


	if (pfp == NULL)
	    return SR_FAULT ;

	if (pfp->magic != DEBUGSCHED_IMAGIC)
	    return SR_NOTOPEN ;

	(void) time(&daytime) ;


/* should we even check ? */

	if ((daytime - pfp->checktime) <= DEBUGSCHED_ICHECKTIME)
	    return SR_OK ;

	pfp->checktime = daytime ;

/* check the files */

#if	CF_DEBUGS
	eprintf("debugsched_check: about to loop\n",i) ;
#endif

	rs = SR_OK ;
	for (i = 0 ; vecitem_get(&pfp->files,i,&fep) >= 0 ; i += 1) {

	    if (fep == NULL) continue ;

	    if ((u_stat(fep->filename,&sb) >= 0) &&
	        (sb.st_mtime > fep->mtime) &&
	        ((daytime - sb.st_mtime) >= DEBUGSCHED_ICHANGETIME)) {

#if	CF_DEBUGS
	        eprintf("debugsched_check: file=%d changed\n",i) ;
	        eprintf("debugsched_check: freeing file entries\n") ;
#endif

	        debugsched_freefile(pfp,i) ;

#if	CF_DEBUGS
	        eprintf("debugsched_check: parsing the file again\n") ;
#endif

	        if ((rs = debugsched_parsefile(pfp,i,eep)) >= 0)
	            c_changed += 1 ;

#if	CF_DEBUGS
	        eprintf("debugsched_check: debugsched_parsefile rs=%d\n",rs) ;
	        eprintf("debugsched_check: sorting STD entries\n") ;
#endif

#ifdef	COMMENT
	        vecitem_sort(&pfp->aes_std,cmpfunc) ;
#endif

	    } /* end if */

	} /* end for */

#if	CF_DEBUGS
	eprintf("debugsched_check: exiting rs=%d changed=%d\n",
	    rs,c_changed) ;
#endif

	return ((rs >= 0) ? c_changed : rs) ;
}
/* end subroutine (debugsched_check) */



/* INTERNAL SUBROUTINES */



/* parse a parameter file */
static int debugsched_parsefile(pfp,fi,eep)
DEBUGSCHED	*pfp ;
int		fi ;
VECITEM		*eep ;
{
	bfile		file, *fp = &file ;

	DEBUGSCHED_FILE	*fep ;

	DEBUGSCHED_ENTRY	pe ;

	USTAT	sb ;

	FIELD	fsb ;

	int	rs = SR_OK ;
	int	i, c, c_added = 0 ;
	int	len, line = 0 ;
	int	klen, vlen ;

	char	linebuf[LINELEN + 1] ;
	char	*key, *value ;
	char	*cp ;


#if	CF_DEBUGS
	eprintf("debugsched_parsefile: entered\n") ;
#endif

	if ((rs = vecitem_get(&pfp->files,fi,&fep)) < 0)
	    return rs ;

	if (fep == NULL)
	    return SR_NOTFOUND ;

#if	CF_DEBUGS
	eprintf("debugsched_parsefile: 2\n") ;
#endif

	if ((rs = bopen(fp,fep->filename,"r",0664)) < 0)
	    return rs ;

#if	CF_DEBUGS
	eprintf("debugsched_parsefile: bopen rs=%d\n",
	    rs) ;
#endif

	if ((rs = bcontrol(fp,BC_STAT,&sb)) < 0)
	    goto done ;

/* have we already parsed this one ? */

#if	CF_DEBUGS
	eprintf("debugsched_parsefile: 4\n") ;
#endif

	rs = SR_OK ;
	c_added = 0 ;
	if (fep->mtime >= sb.st_mtime)
	    goto done ;

#if	CF_DEBUGS
	eprintf("debugsched_parsefile: 5\n") ;
#endif

	fep->mtime = sb.st_mtime ;

/* start processing the configuration file */

#if	CF_DEBUGS
	eprintf("debugsched_parsefile: start processing\n") ;
#endif

	while ((len = bgetline(fp,linebuf,LINELEN)) > 0) {

	    line += 1 ;
	    if (len == 1) continue ;	/* blank line */

	    if (linebuf[len - 1] != '\n') {

	        while ((c = bgetc(fp)) >= 0)
	            if (c == '\n') break ;

	        continue ;
	    }

	    linebuf[--len] = '\0' ;
	    cp = linebuf ;
	    while (CHAR_ISWHITE(*cp)) {

	        cp += 1 ;
	        len -= 1 ;

	    }

	    if (*cp == '#') continue ;

	    fsb.lp = cp ;
	    fsb.rlen = len ;

#if	CF_DEBUGSFIELD
	    eprintf("debugsched_parsefile: line> %W\n",
	        fsb.lp,fsb.rlen) ;
#endif

/* get a key */

	    (void) field_get(&fsb,key_terms) ;

#if	CF_DEBUGSFIELD
	    eprintf("debugsched_parsefile: field key>%W<\n",
	        fsb.fp,fsb.flen) ;
#endif

/* empty or comment-only line */

	    if (fsb.flen <= 0) continue ;

/* we have something ! */

	    key = fsb.fp ;
	    klen = fsb.flen ;
	    value = NULL ;
	    vlen = -1 ;

	    entry_init(&pe,fi) ;

/* do we have a value ? */

	    if (field_get(&fsb,arg_terms) >= 0) {

#if	CF_DEBUGSFIELD
	        eprintf("debugsched_parsefile: field value>%W<\n",
	            fsb.fp,fsb.flen) ;
#endif

	        value = fsb.fp ;
	        vlen = fsb.flen ;

	    }

	    rs = entry_load(&pe,key,klen,value,vlen) ;

	    if (rs >= 0) {

	        rs = debugsched_addentry(pfp,&pe) ;

#if	CF_DEBUGS
	        eprintf("debugsched_parsefile: added entry ?, rs=%d\n",rs) ;
#endif

	    }

	    if (rs >= 0)
	        c_added += 1 ;

	        else
	        entry_free(&pe) ;

	} /* end while (reading lines) */


/* done with configuration file processing */
done:

#if	CF_DEBUGS
	eprintf("debugsched_parsefile: added=%d exiting, rs=%d\n",
	    c_added,rs) ;
#endif

	(void) bclose(fp) ;

	return ((rs >= 0) ? c_added : rs) ;

badmal:

#if	CF_DEBUGS
	eprintf("debugsched_parsefile: bad malloc !\n") ;
#endif

	rs = SR_NOMEM ;
	entry_free(&pe) ;

	goto done ;
}
/* end subroutine (debugsched_parsefile) */


/* add an entry to something */
static int debugsched_addentry(pfp,pep)
DEBUGSCHED	*pfp ;
DEBUGSCHED_ENTRY	*pep ;
{
	int	rs ;


	rs = vecitem_add(&pfp->entries,pep,sizeof(DEBUGSCHED_ENTRY)) ;

	return rs ;
}
/* end subroutine (debugsched_addentry) */


/* free up ALL of the entries in this DEBUGSCHED list */
static int debugsched_freeentries(pfp)
DEBUGSCHED		*pfp ;
{
	DEBUGSCHED_ENTRY	*pep ;

	VECITEM		*slp ;

	int		i, rs ;


	slp = &pfp->entries ;

/* pop 'em off as if they were not sorted */

	for (i = 0 ; (rs = vecitem_get(slp,i,&pep)) >= 0 ; i += 1) {

	    if (pep == NULL) continue ;

	    entry_free(pep) ;

	} /* end for */

/* again, but do differently incase the vector was sorted ! */

	for (i = 0 ; (rs = vecitem_get(slp,i,&pep)) >= 0 ; i += 1) {

	    if (pep == NULL) continue ;

	    entry_free(pep) ;
	    i -= 1 ;

	} /* end for */

	(void) vecitem_free(slp) ;

	return SR_OK ;
}
/* end subroutine (debugsched_freeentries) */


/* free up all of the entries in this DEBUGSCHED list associated w/ a file */
static int debugsched_freefile(pfp,fi)
DEBUGSCHED	*pfp ;
int		fi ;
{
	DEBUGSCHED_ENTRY	*pep ;

	VECITEM		*slp ;

	int		i, rs ;


#if	CF_DEBUGS
	eprintf("debugsched_freefile: want to delete all fi=%d\n",fi) ;
#endif

	slp = &pfp->entries ;
	for (i = 0 ; (rs = vecitem_get(slp,i,&pep)) >= 0 ; i += 1) {

	    if (pep == NULL) continue ;

#if	CF_DEBUGS
	    eprintf("debugsched_freefile: i=%d fi=%d\n",i,pep->fi) ;
#endif

	    if ((pep->fi == fi) || (fi < 0)) {

#if	CF_DEBUGS
	        eprintf("debugsched_freefile: got one\n") ;
#endif

	        entry_free(pep) ;

	        vecitem_del(slp,i) ;

	        i -= 1 ;

	    } /* end if (got one) */

	} /* end for */

#if	CF_DEBUGS
	eprintf("debugsched_freefile: exiting\n") ;
#endif

	return SR_OK ;
}
/* end subroutine (debugsched_freefile) */


/* free up all of the files in this DEBUGSCHED list */
static int debugsched_freefiles(pfp)
DEBUGSCHED	*pfp ;
{
	DEBUGSCHED_FILE	*pep ;

	VECITEM		*slp ;

	int		i, rs ;


	slp = &pfp->files ;
	for (i = 0 ; (rs = vecitem_get(slp,i,&pep)) >= 0 ; i += 1) {

	    if (pep == NULL) continue ;

	    if (pep->filename != NULL) {

	        i -= 1 ;
	        free(pep->filename) ;

	    }

	} /* end for */

	return vecitem_free(slp) ;
}
/* end subroutine (debugsched_freefiles) */


/* initialize an entry */
static void entry_init(pep,fi)
DEBUGSCHED_ENTRY	*pep ;
int		fi ;
{


	pep->fi = fi ;
	pep->key = NULL ;
	pep->orig = NULL ;
	pep->value = NULL ;

}
/* end subroutine (entry_init) */


static int entry_load(pep,k,klen,v,vlen)
DEBUGSCHED_ENTRY	*pep ;
char		k[], v[] ;
int		klen, vlen ;
{
	int	len ;
	int	rs ;

	char	*mkp, *mvp ;


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

	len = klen + vlen + 2 ;
	if ((rs = uc_malloc(len,&mkp)) < 0)
	    return rs ;

	mvp = strwcpy(mkp,k,klen) + 1 ;

	pep->key = mkp ;
	if (v != NULL) {

	    pep->orig = mvp ;
	    strwcpy(mvp,v,vlen) ;

	}

	return SR_OK ;
}
/* end subroutine (entry_load) */


static int entry_enough(pep)
DEBUGSCHED_ENTRY	*pep ;
{


	if (pep == NULL)
	    return SR_FAULT ;

	if (pep->key == NULL)
	    return SR_INVALID ;

	return SR_OK ;
}
/* end subroutine (entry_enough) */


#ifdef	DEBUGSCHED_COMMENT

/* compare if all but the netgroup are equal (or matched) */
static int entry_mat2(e1p,e2p)
DEBUGSCHED_ENTRY	*e1p, *e2p ;
{


#if	CF_DEBUGS
	eprintf("debugsched/entry_mat2: entered\n") ;
	eprintf("debugsched/entry_mat2: m=%s u=%s p=%s\n",
	    e2p->machine.std,
	    e2p->username.std,
	    e2p->password.std) ;
#endif

#ifdef	OPTIONAL
	if (! part_match(&e1p->netgroup,e2p->netgroup.std))
	    return FALSE ;
#endif

	if (! part_match(&e1p->machine,e2p->machine.std))
	    return FALSE ;

	if (! part_match(&e1p->username,e2p->username.std))
	    return FALSE ;

	if (! part_match(&e1p->password,e2p->password.std))
	    return FALSE ;

	return TRUE ;
}
/* end subroutine (entry_mat2) */

#endif /* DEBUGSCHED_COMMENT */


#ifdef	DEBUGSCHED_COMMENT

/* compare if all of the entry is equal (or matched) */
static int entry_mat3(e1p,e2p)
DEBUGSCHED_ENTRY	*e1p, *e2p ;
{


	if (! part_match(&e1p->netgroup,e2p->netgroup.std))
	    return FALSE ;

	if (! part_match(&e1p->machine,e2p->machine.std))
	    return FALSE ;

	if (! part_match(&e1p->username,e2p->username.std))
	    return FALSE ;

	if (! part_match(&e1p->password,e2p->password.std))
	    return FALSE ;

	return TRUE ;
}
/* end subroutine (entry_mat3) */

#endif /* DEBUGSCHED_COMMENT */


/* free up an entry */
static void entry_free(pep)
DEBUGSCHED_ENTRY	*pep ;
{


#if	CF_DEBUGS
	eprintf("entry_free: entered\n") ;
#endif

	pep->fi = -1 ;
	freeit(&pep->key) ;

	freeit(&pep->orig) ;

	freeit(&pep->value) ;


}
/* end subroutine (entry_free) */


static void freeit(pp)
char	**pp ;
{


	if (*pp != NULL) {

	    free(*pp) ;

	    *pp = NULL ;
	}
}
/* end subroutine (freeit) */


/* compare just the 'netgroup' part of entries (used for sorting) */
static int cmpfunc(e1pp,e2pp)
DEBUGSCHED_ENTRY	**e1pp, **e2pp ;
{


#if	CF_DEBUGS
	eprintf("debugsched/cmpfunc: e1=%s e2=%s\n",
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




