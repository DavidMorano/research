/* fcount */

/* function-count object */


#define	CF_DEBUGS	0
#define	CF_DEBUGSBIND	0
#define	CF_DEBUGSUPDATE	0
#define	CF_SAFE		0
#define	CF_NULLENTRY	0


/* revision history:

	= 02/05/01, David Morano

	This object module was created for Levo research.  


*/


/******************************************************************************

	This object module implements a function coverage profile
	counter.


*****************************************************************************/


#define	FCOUNT_MASTER	0		/* include public declarations */


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/mman.h>		/* Memory Management */
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<elf.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<char.h>
#include	<vecitem.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"fcount.h"



/* local defines */

#define	FCOUNT_MAGIC	0x23456787
#define	FCOUNT_DEFFUNCS	300		/* default entries */

#define	MODP2(v,n)	((v) & ((n) - 1))

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

extern int	readsginm(vecitem *,const char *) ;


/* local structures */


/* forward references */

#if	(FCOUNT_MASTER != 0)
int		fcount_get(FCOUNT *,int,FCOUNT_ENTRY **) ;
#endif

static int	cmpfunc(FCOUNT_ENTRY **, FCOUNT_ENTRY **) ;
static int	incfunc(FCOUNT_ENTRY **, FCOUNT_ENTRY **) ;
static int	insfunc(FCOUNT_ENTRY **, FCOUNT_ENTRY **) ;







int fcount_init(op,mp,fname)
FCOUNT		*op ;
LMAPPROG	*mp ;
const char	fname[] ;
{
	FCOUNT_ENTRY		e, *ep, *pep, *nep ;

	LMAPPROG_SNCURSOR	cur ;

	Elf32_Sym	*eep ;

	int	rs, rs1 ;
	int	size ;
	int	i ;

	char	*namep ;
	char	*cp ;


	if (op == NULL)
	    return SR_FAULT ;

	if (mp == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(FCOUNT)) ;


	rs = vecitem_init(&op->table,FCOUNT_DEFFUNCS,VECITEM_PSORTED) ;

	if (rs < 0)
	    goto bad0 ;


	(void) memset(&e,0,sizeof(FCOUNT_ENTRY)) ;

	lmapprog_sncursorinit(mp,&cur) ;

	while (TRUE) {

	    rs1 = lmapprog_enumsym(mp,&cur,&namep,&eep) ;

	    if (rs1 < 0)
	        break ;

#if	CF_DEBUGS
	    {
	        debugprintf("fcount_init: name=%s ia=%08x size=%d rs=%d\n",
	            namep,eep->st_value,eep->st_size,rs) ;

#if	CF_DEBUGSBIND
	        debugprintf("fcount_init: TYPE=%d BIND=%d\n",
	            ELF32_ST_TYPE(eep->st_info),
	            ELF32_ST_BIND(eep->st_info)) ;
#endif

	    }
#endif /* CF_DEBUGS */

	    if (ELF32_ST_TYPE(eep->st_info) == STT_FUNC) {

	        e.ia = eep->st_value ;
	        e.size = eep->st_size ;
	        e.name = mallocstr(namep) ;

		rs = SR_NOMEM ;
		if (e.name != NULL)
	        	rs = vecitem_add(&op->table,&e,sizeof(FCOUNT_ENTRY)) ;

	        if (rs < 0)
	            break ;

	    } /* end if (got one) */

	} /* end while (looping through candidate symbols) */

	lmapprog_sncursorfree(mp,&cur) ;

	if (rs < 0)
	    goto bad1 ;


/* OK, add any from the stupid SGI name-list file */

#if	CF_DEBUGS
	debugprintf("fcount_init: fname=%s\n",fname) ;
#endif

	if ((fname != NULL) && (fname[0] != '\0')) {

		rs = vecitem_sort(&op->table,cmpfunc) ;

		rs = readsginm(&op->table,fname) ;

#if	CF_DEBUGS
	debugprintf("fcount_init: readsginm() rs=%d\n",rs) ;
#endif

	} /* end if */


/* OK, make sure they are sorted */

#if	CF_DEBUGS
	for (i = 0 ; vecitem_get(&op->table,i,&ep) >= 0 ; i += 1) {

	        debugprintf("fcount_init: R i=%d ep=%p\n",i,ep) ;
		if (ep == NULL)
		debugprintf("fcount_init: R got_one i=%d\n",i) ;

		else
	        debugprintf("fcount_init: R name=%s ia=%08x size=%d\n",
	            ep->name,ep->ia,ep->size) ;

	}
#endif /* CF_DEBUGS */

#if	CF_DEBUGS
	        debugprintf("fcount_init: vecitem_sort() \n") ;
#endif

	rs = vecitem_sort(&op->table,cmpfunc) ;


/* delete any duplicate entries that also start with an underscore '_' ! */

#if	CF_DEBUGS
	        debugprintf("fcount_init: deleting duplicates\n") ;
#endif

	for (i = 0 ; vecitem_get(&op->table,i,&ep) >= 0 ; i += 1) {

	    int	j, k ;


#if	CF_DEBUGS
	        debugprintf("fcount_init: i=%d ep=%p\n",i,ep) ;
		if (ep == NULL)
		debugprintf("fcount_init: got_one i=%d\n",i) ;

		else
	        debugprintf("fcount_init: name=%s ia=%08x size=%d\n",
	            ep->name,ep->ia,ep->size) ;
#endif

	    if (ep == NULL) continue ;

	    j = i + 1 ;
	    while (vecitem_get(&op->table,j,&nep) >= 0) {

		if (nep == NULL) {

			j += 1 ;
			continue ;
		}

	        if (ep->ia != nep->ia)
			break ;

	        k = (ep->name[0] == '_') ? i : j ;
	        vecitem_del(&op->table,k) ;

#if	CF_DEBUGS
	        debugprintf("fcount_init: deleted i=%d nn=%s nia=%08x\n",
			k,nep->name,nep->ia) ;
#endif

	        if (k == i)
	            break ;

	    } /* end while */

	} /* end for */

/* OK, make sure they are sorted */

	rs = vecitem_sort(&op->table,cmpfunc) ;

	if (rs < 0)
	    goto bad1 ;

/* OK, now try to fill in any missing size fields */

	pep = NULL ;
	for (i = 0 ; vecitem_get(&op->table,i,&ep) >= 0 ; i += 1) {

	    int	j, k ;


	    if (ep == NULL) continue ;

	    if (pep != NULL)
	        pep->size = ep->ia - pep->ia ;

	    pep = (ep->size == 0) ? ep : NULL ;

	} /* end for */


#if	CF_DEBUGS
	for (i = 0 ; vecitem_get(&op->table,i,&ep) >= 0 ; i += 1) {

	    debugprintf("fcount_init: name=%s ia=%08x size=%d\n",
	        ep->name,ep->ia,ep->size) ;

	}
#endif /* CF_DEBUGS */


/* we're out of here */

	op->magic = FCOUNT_MAGIC ;
	return rs ;

/* bad stuff */
bad1:
	for (i = 0 ; vecitem_get(&op->table,i,&ep) >= 0 ; i += 1) {

		if (ep == NULL) continue ;

		if (ep->name != NULL)
			free(ep->name) ;

	} /* end for */

	vecitem_free(&op->table) ;

bad0:
	return rs ;
}
/* end subroutine (fcount_init) */


/* free up this fcount object */
int fcount_free(op)
FCOUNT		*op ;
{
	FCOUNT_ENTRY	*ep ;

	int	rs, i ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

#if	CF_DEBUGS
	for (i = 0 ; vecitem_get(&op->table,i,&ep) >= 0 ; i += 1) {

		if (ep == NULL) continue ;

		debugprintf("fcount_free: name=%s ins=%d calls=%d\n",
			ep->name,ep->ins,ep->calls) ;

	}
#endif /* CF_DEBUGS */

	for (i = 0 ; vecitem_get(&op->table,i,&ep) >= 0 ; i += 1) {

		if (ep == NULL) continue ;

		if (ep->name != NULL)
			free(ep->name) ;

	} /* end for */

	rs = vecitem_free(&op->table) ;

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (fcount_free) */


/* update an entry */
int fcount_update(op,ia,type)
FCOUNT		*op ;
uint		ia ;
int		type ;
{
	FCOUNT_ENTRY	e, *ep ;

	int	rs ;
	int	i ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSUPDATE && 0
	debugprintf("fcount_update: ia=%08x\n",ia) ;
#endif

/* check if it matches the cached entry */

	rs = SR_OK ;
	if ((op->func_ia > 0) &&
	    (ia >= op->func_ia) && (ia < (op->func_ia + op->func_size))) {

	    ep = (FCOUNT_ENTRY *) op->table.va[op->func_i] ;
	    ep->ins += 1 ;

	} else {

	    e.ia = ia ;
	    e.size = 0 ;
	    e.name = NULL ;
	    rs = vecitem_search(&op->table,&e,incfunc,(void **) &ep) ;

	    if (rs >= 0) {

	        ep->ins += 1 ;
	        op->func_i = rs ;
	        op->func_ia = ep->ia ;
	        op->func_size = ep->size ;

#if	CF_DEBUGSUPDATE
	debugprintf("fcount_update: => in=%d ia=%08x n=%s\n",
		op->in,ia,ep->name) ;
#endif

	    }

	} /* end if */

	if (rs >= 0) {

	    if (ia == ep->ia)
	        ep->calls += 1 ;

	} else
	    op->other_ins += 1 ;

	op->in += 1 ;

	return rs ;
}
/* end subroutine (fcount_update) */


/* prepare for read-out of functions sorted by instructions in each */
int fcount_done(op)
FCOUNT		*op ;
{
	FCOUNT_ENTRY	*ep ;

	int	i ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

#if	CF_DEBUGS
	debugprintf("fcount_done: entered \n") ;
#endif

#if	CF_DEBUGS
	if (op->table.va[0] == NULL)
	debugprintf("fcount_done: 1 got_one i=0 ep=00000000\n") ;
#endif

#if	CF_DEBUGS
	debugprintf("fcount_done: reviewing\n") ;
	for (i = 0 ; fcount_get(op,i,&ep) >= 0 ; i += 1) {
		if (ep == NULL)
		debugprintf("fcount_done: 2 got_one i=%d ep=%p\n",i,ep) ;
	}
#endif /* CF_DEBUGS */

	if (op->table.i > 0)
	(void) qsort(op->table.va,op->table.i,sizeof(void *),
		((int (*)()) insfunc)) ;

#if	CF_DEBUGS
	debugprintf("fcount_done: OK\n") ;
#endif

	return op->table.i ;
}
/* end subroutine (fcount_done) */


/* sort the entries according to the user comparison function */
int fcount_sort(op,userfunc)
FCOUNT		*op ;
int		(*userfunc)() ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

	if (userfunc == NULL)
		return SR_FAULT ;

#if	CF_DEBUGS
	debugprintf("fcount_sort: entered \n") ;
#endif

	if (op->table.i > 0)
	(void) qsort(op->table.va,op->table.i,sizeof(void *),userfunc) ;

#if	CF_DEBUGS
	debugprintf("fcount_sort: OK\n") ;
#endif

	return op->table.i ;
}
/* end subroutine (fcount_sort) */


/* get the entries (serially) */
int fcount_get(op,ri,rpp)
FCOUNT		*op ;
int		ri ;
FCOUNT_ENTRY	**rpp ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

	if (rpp == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	debugprintf("fcount_get: entered ri=%d\n",ri) ;
#endif

	*rpp = NULL ;
	rs = vecitem_get(&op->table,ri,rpp) ;

#if	CF_DEBUGS
	debugprintf("fcount_get: rs=%d rp=%p\n",rs,(*rpp)) ;
	if (rs >= 0)
	debugprintf("fcount_get: rp=%p n=%s\n",(*rpp),(*rpp)->name) ;
#endif

	return rs ;
}
/* end subroutine (fcount_get) */


/* the other instructions not mapped to functions */
int fcount_getother(op,r1pp,r2pp)
FCOUNT		*op ;
uint		*r1pp, *r2pp ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

	if (r1pp != NULL)
		*r1pp = op->other_ins ;

	if (r2pp != NULL)
		*r2pp = op->other_calls ;

	return rs ;
}
/* end subroutine (fcount_getother) */


int fcount_gettotal(op,r1pp,r2pp)
FCOUNT		*op ;
uint		*r1pp, *r2pp ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

	if (r1pp != NULL)
		*r1pp = op->in ;

	if (r2pp != NULL)
		*r2pp = 0 ;

	return rs ;
}
/* end subroutine (fcount_gettotal) */


#ifdef	COMMENT

/* zero out the statistics */
int fcount_zerostats(op)
FCOUNT		*op ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

	(void) memset(&op->s,0,sizeof(FCOUNT_STATS)) ;

	return rs ;
}
/* end subroutine (fcount_zerostats) */


/* get information about all static branches */
int fcount_stats(op,rp)
FCOUNT		*op ;
FCOUNT_STATS	*rp ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FCOUNT_MAGIC)
	    return SR_NOTOPEN ;

	if (rp == NULL)
	    return SR_FAULT ;

	(void) memcpy(rp,&op->s,sizeof(FCOUNT_STATS)) ;

	return rs ;
}
/* end subroutine (fcount_stats) */

#endif /* COMMENT */



/* INTERNAL SUBROUTINES */



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
#endif /* CF_NULLENTRY */

	return ((*e1pp)->ia - (*e2pp)->ia) ;
}
/* end subroutine (cmpfunc) */


/* sort an IA by if it is before, inside, or after a function */
static int incfunc(e1pp,e2pp)
FCOUNT_ENTRY	**e1pp, **e2pp ;
{
	FCOUNT_ENTRY	*e1p, *e2p ;

	int	rs ;


#if	CF_DEBUGS
	if ((*e1pp) == NULL)
	debugprintf("incfunc: got_one *e1pp=%p\n",(*e1pp)) ;
	if ((*e2pp) == NULL)
	debugprintf("incfunc: got_one *e2pp=%p\n",(*e2pp)) ;
#endif

	e1p = (*e1pp) ;
	e2p = (*e2pp) ;
	if (e1p->name == NULL) {

	    if (e1p->ia < e2p->ia)
	        return -1 ;

	    if (e1p->ia >= (e2p->ia + e2p->size))
	        return 1 ;

	} else {

	    if (e2p->ia < e1p->ia)
	        return -1 ;

	    if (e2p->ia >= (e1p->ia + e1p->size))
	        return 1 ;

	}

	return 0 ;
}
/* end subroutine (incfunc) */


/* sort by number of instructions */
static int insfunc(e1pp,e2pp)
FCOUNT_ENTRY	**e1pp, **e2pp ;
{
	FCOUNT_ENTRY	*e1p, *e2p ;

	int	rs ;


#if	CF_DEBUGS
	if ((*e1pp) == NULL)
	debugprintf("insfunc: got_one *e1pp=%p\n",(*e1pp)) ;
	if ((*e2pp) == NULL)
	debugprintf("insfunc: got_one *e2pp=%p\n",(*e2pp)) ;
#endif

	e1p = (*e1pp) ;
	e2p = (*e2pp) ;
	return (- (e1p->ins - e2p->ins)) ;
}
/* end subroutine (insfunc) */


#ifdef	COMMENT

/* read the stupid SGI name-list file */
static readsgi(lp,fname)
vecitem		*lp ;
const char	fname[] ;
{
	bfile	nm ;

	FIELD	lf ;

	int	rs, i, len ;
	int	sl, cl ;

	uchar	terms[32 + 1] ;

	char	linebuf[LINEBUFLEN + 1] ;
	char	*sp, *cp ;


	if (lp == NULL)
		return SR_FAULT ;

	rs = bfile(&nm,fname,"r",0666) ;

	if (rs < 0)
		return rs ;

	fieldterms(terms,0,"|") ;

	while ((len = breadline(&nm,linebuf,LINEBUFLEN)) > 0) {

		sp = linebuf ;
		for (i = 0 ; (i < len) && CHAR_ISWHITE(*sp) ; i += 1)
			sp += 1 ;

		len -= i ;
		if (len == 0)
			continue ;

		if (*sp !- '[')
			continue ;

		field_init(&lf,sp,len) ;


		field_get(&lf,NULL)


		field_free(&lf) ;

	} /* end while */

	bclose(&nm) ;

	return rs ;
}
/* end subroutine (readsgi) */

#endif /* COMMENT */



