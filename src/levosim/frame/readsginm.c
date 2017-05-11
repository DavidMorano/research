/* readsginm */

/* read the stupid SGI name-list file */


#define	CF_DEBUGS	0
#define	CF_NULLENTRY	0


/* revision history:

	= 2002-05-01, David Morano

	This subroutine was created for Levo research.  


*/


/******************************************************************************

	This subroutine just reads the stupid SGI name-list file.  This
	is necessary to get the stupid way in which the stupid SGI
	system stores static function symbols in the object file.  It
	does NOT store them in the object file symbol table as expected
	(and as what is normally done).  Rather it stores them in some
	sort of stupid proprietary "debug" section in the object file.

	The file is in ASCII but has a weird format that is not at all
	typical of the AT&T System V Release 4 format (which seems to
	be claimed by the SGI people).

	The strategy is parse the records and store all static
	procedures (statically declared function symbols -- an SGI
	'StaticProc'-type record) in a vector-of-elements container
	class.  This is a straight linear arrangement (vector) of
	arbitrary sized elements.  When we find an 'End'-type record,
	we search the vector for the record ID named in the 'End'
	record.  The 'End' record provides the size of the static
	function.  There are not a lot of static functions in most code
	so searching linearly is not really a problem.  Also, this
	whole stupid excercise (for the stupid SGI screw-up of not
	putting static function symbols in the object file symbol
	table) is only done once.  If you want something faster, like
	if you somehow had a billion static functions or something like
	that, then you can substitute a hash table container class
	instead of the vector one.


*****************************************************************************/


#define	FCOUNT_MASTER	0		/* include public declarations */


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/mman.h>		/* Memory Management */
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>
#include	<bitops.h>
#include	<char.h>
#include	<bfile.h>
#include	<vecitem.h>
#include	<field.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"fcount.h"



/* local defines */

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

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif



/* external subroutines */

extern int	cfdeci(const char *,int,int *) ;
extern int	cfhexui(const char *,int,uint *) ;
extern int	sfshrink(const char *,int,char **) ;

extern char	*strwcpy(char *,const char *,int) ;


/* local structures */

struct record {
	int	id ;
	uint	ia, size ;
	char	*name ;			/* allocated memory */
} ;


/* forward references */

static int	record_init(struct record *) ;
static int	record_id(struct record *,FIELD *) ;
static int	record_ia(struct record *,FIELD *) ;
static int	record_free(struct record *) ;

static int	field_getref(FIELD *) ;
static int	procref(vecitem *,int,uint) ;

#if	CF_DEBUGS && 0
static int	showterms(uchar *) ;
#endif

static int	cmpfunc(FCOUNT_ENTRY **, FCOUNT_ENTRY **) ;







/* read the stupid SGI name-list file */
int readsginm(lp,fname)
vecitem		*lp ;
const char	fname[] ;
{
	bfile	nm ;

	FIELD	lf ;

	FCOUNT_ENTRY	e ;

	vecitem		stage ;

	struct record	r, *rp ;

	int	rs, rs1 ;
	int	i, len ;
	int	fl ;
	int	sl, cl ;

	uchar	terms[32 + 1] ;

	char	linebuf[LINEBUFLEN + 1] ;
	char	*sp, *cp ;
	char	*fp ;


	if (lp == NULL)
	    return SR_FAULT ;

	if ((fname == NULL) || (fname[0] == '\0'))
	    return SR_INVALID ;

#if	CF_DEBUGS
	debugprintf("readsginm: fname=%s\n",fname) ;
#endif

	rs = bopen(&nm,fname,"r",0666) ;
	if (rs < 0)
	    goto ret0 ;

	rs = vecitem_init(&stage,200,0) ;
	if (rs < 0)
	    goto ret1 ;

	fieldterms(terms,0,"|") ;

#if	CF_DEBUGS && 0
	showterms(terms) ;
#endif

	while ((rs = breadline(&nm,linebuf,LINEBUFLEN)) > 0) {

#if	CF_DEBUGS
	    debugprintf("readsginm: line> %w\n",linebuf,(len - 1)) ;
#endif

	    len = rs ;
	    linebuf[len] = '\0' ;
	    sp = linebuf ;
	    for (i = 0 ; (i < len) && CHAR_ISWHITE(*sp) ; i += 1)
	        sp += 1 ;

	    len -= i ;
	    if (len == 0)
	        continue ;

	    if ((*sp != '[') || (strchr(sp,'|') == NULL))
	        continue ;

#if	CF_DEBUGS
	    debugprintf("readsginm: got one >%w<\n",sp,(len -1)) ;
#endif

	    memset(&r,0,sizeof(struct record)) ;

	    if ((rs = field_init(&lf,sp,len)) >= 0) {

/* field 1 (the record ID) */

	    field_term(&lf,terms,&fp) ;

	    record_id(&r,&lf) ;

#if	CF_DEBUGS
	    debugprintf("readsginm: RID=%d\n",r.id) ;
#endif

/* field 2 (IA or size for 'End'-type records) */

	    field_term(&lf,terms,&fp) ;

#if	CF_DEBUGS
	    debugprintf("readsginm: field=>%w<\n",fp,fl) ;
#endif

	    record_ia(&r,&lf) ;

#if	CF_DEBUGS
	    debugprintf("readsginm: value=%08x\n",r.ia) ;
#endif

/* field 3 (size, but the stupid SGI makes it essentially *UNUSED*) */

#if	CF_DEBUGS && 0
	    showterms(terms) ;
#endif

	    fl = field_term(&lf,terms,&fp) ;

#if	CF_DEBUGS
	    debugprintf("readsginm: field=>%w<\n",fp,fl) ;
#endif

/* field 4 (the type field) */

	    fl = field_term(&lf,terms,&fp) ;

#if	CF_DEBUGS
	    debugprintf("readsginm: type field=>%w<\n",fp,fl) ;
#endif

	    sl = sfshrink(fp,fl,&sp) ;

	    if ((sl == 10) && 
	        (strncmp(sp,"StaticProc",sl) == 0)) {

#if	CF_DEBUGS
	        debugprintf("readsginm: StaticProc\n") ;
#endif

	        fl = field_term(&lf,terms,&fp) ;

#if	CF_DEBUGS
	        debugprintf("readsginm: field=>%w<\n",fp,fl) ;
#endif

	        fl = field_term(&lf,terms,&fp) ;

#if	CF_DEBUGS
	        debugprintf("readsginm: field=>%w<\n",fp,fl) ;
#endif

	        fl = field_term(&lf,terms,&fp) ;

#if	CF_DEBUGS
	        debugprintf("readsginm: name field=>%w<\n",fp,fl) ;
#endif

	        cl = sfshrink(fp,fl,&cp) ;

	        if (fl >= 0) {

	            r.name = mallocstrw(cp,cl) ;

	            if (r.name != NULL) {

	                rs = vecitem_add(&stage,&r,sizeof(struct record)) ;

#if	CF_DEBUGS
	                debugprintf("readsginm: added rs=%d\n",rs) ;
#endif

	            }
	        }

	    } else if ((sl == 3) && 
	        (strncmp(sp,"End",sl) == 0)) {

	        int	ref ;


#if	CF_DEBUGS
	        debugprintf("readsginm: End\n") ;
#endif

	        field_term(&lf,terms,&fp) ;

	        ref = field_getref(&lf) ;

	        fl = field_term(&lf,terms,&fp) ;

	        cl = sfshrink(fp,fl,&cp) ;

	        if (strncmp(cp,"Text",cl) == 0)
	            procref(&stage,ref,r.ia) ;

	    } /* end if (type of record) */

	    field_free(&lf) ;

	    } /* end if (field) */

	} /* end while (reading line records) */


/* clean-up, delete all entries and get out */

	for (i = 0 ; vecitem_get(&stage,i,&rp) >= 0 ; i += 1) {

	    if (rp == NULL)
	        continue ;

#if	CF_DEBUGS
	    debugprintf("readsginm: R ia=%08x size=%d name=%s\n",
	        rp->ia,rp->size,rp->name) ;
#endif

	    memset(&e,0,sizeof(FCOUNT_ENTRY)) ;

	    e.ia = rp->ia ;
	    e.size = rp->size ;
	    e.name = rp->name ;	/* transfer memory allocation */
	    rs1 = vecitem_search(lp,&e,cmpfunc,NULL) ;

	    if (rs1 < 0)
	        vecitem_add(lp,&e,sizeof(FCOUNT_ENTRY)) ;

	    vecitem_del(&stage,i) ;

	    i -= 1 ;

	} /* end for (deleting) */

ret2:
	vecitem_free(&stage) ;

ret1:
	bclose(&nm) ;

ret0:
	return rs ;
}
/* end subroutine (readsginm) */



/* LOCAL SUBROUTINES */



static int record_init(rp)
struct record	*rp ;
{


	(void) memset(rp,0,sizeof(struct record)) ;

	return 0 ;
}


static int record_id(rp,lfp)
struct record	*rp ;
FIELD		*lfp ;
{
	int	rs = -1 ;

	char	*sp, *cp ;


	sp = lfp->fp + 1 ;
	cp = strchr(sp,']') ;

	if (cp != NULL)
	    rs = cfdeci(sp,(cp - sp),&rp->id) ;

	return rs ;
}
/* end subroutine (record_id) */


static int record_ia(rp,lfp)
struct record	*rp ;
FIELD		*lfp ;
{
	int	rs ;


	rs = cfhexui(lfp->fp,lfp->fl,&rp->ia) ;

	return rs ;
}
/* end subroutine (record_ia) */


static int record_free(rp)
struct record	*rp ;
{


	return 0 ;
}


static int field_getref(lfp)
FIELD		*lfp ;
{
	int	rs = -1 ;
	int	sl ;
	int	ref ;

	char	*sp ;


	sp = strchr(lfp->fp,'=') ;

	if ((strncmp(lfp->fp,"ref",3) == 0) && (sp != NULL)) {

	    sp += 1 ;
	    sl = lfp->fp + lfp->fl - sp ;
	    if (sl > 0)
	        rs = cfdeci(sp,sl,&ref) ;

	}

	return (rs >= 0) ? ref : rs ;
}
/* end subroutine (field_getref) */


static int procref(stagep,ref,size)
vecitem		*stagep ;
int		ref ;
uint		size ;
{
	struct record	*rp ;

	int	rs ;
	int	i ;


	for (i = 0 ; (rs = vecitem_get(stagep,i,&rp)) >= 0 ; i += 1) {

	    if (rp == NULL)
	        continue ;

	    if (rp->id == ref) {

	        rp->size = size ;
	        break ;
	    }

	} /* end for */

	return rs ;
}
/* end subroutine (procref) */


#if	CF_DEBUGS && 0

static int	showterms(terms)
uchar	terms[] ;
{
	int	i ;
	int	c = 0 ;


	for (i = 0 ; i < 256 ; i += 1) {

	    if (BTST(terms,i)) {

	        c += 1 ;
	        debugprintf("showterms: > %02x\n",i) ;

	    }
	}

	return c ;
}

#endif /* CF_DEBUGS */


/* sort by function starting IA */
static int cmpfunc(e1pp,e2pp)
FCOUNT_ENTRY	**e1pp, **e2pp ;
{


#if	CF_DEBUGS
	if ((*e1pp) == NULL)
	    debugprintf("cmpfunc: got_one *e1pp=%p\n",(*e1pp)) ;
	if ((*e2pp) == NULL)
	    debugprintf("cmpfunc: got_one *e2pp=%p\n",(*e2pp)) ;
#endif

#if	CF_NULLENTRY
	if (((*e1pp) == NULL) && ((*e2pp) == NULL))
	    return 0 ;

	else if ((*e1pp) == NULL)
	    return 1 ;

	else if ((*e2pp) == NULL)
	    return -1 ;
#endif /* F_NULLENTRY */

	return ((*e1pp)->ia - (*e2pp)->ia) ;
}
/* end subroutine (cmpfunc) */



