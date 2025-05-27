/* recorder */

/* IA recorder object */


#define	CF_DEBUGS	0		/* non-switchable print-outs */
#define	CF_FASTGROW	1		/* grow exponetially ? */
#define	CF_UCMALLOC	1
#define	CF_SAFE		1
#define	CF_WEIRD	0		/* do NOT use */


/* revision history:

	= 2002-04-29, David Morano

	This object module was created for Levo research.
	We need to identify SS-Hammock style conditional branches.


*/

/* Copyright © 2002 David Morano.  All rights reserved. */

/******************************************************************************

	This object module creates a data base of SS-Hammock style
	branches.  This object can form the "lower-half" of a system
	to identify SS-Hammocks and to write the resulting data base
	information out to a file.  This object only manages an
	in-core version of the database.  Another "upper-level"
	component must manage :

	+ identification of SS-Hammocks
	+ call this module to track them
	+ write out the resulting data base to a file for storage


*****************************************************************************/


#define	RECORDER_MASTER	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<localmisc.h>

#include	"recorder.h"


/* local defines */

#define	RECORDER_MAGIC	0x12356734

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


/* external subroutines */

extern uint	nextpowtwo(uint) ;
extern uint	hashelf(const void *,int) ;

extern char	*strwcpy(char *,const char *,int) ;


/* forward references */

static int	recorder_extend(RECORDER *) ;

static int	hashindex(uint,uint) ;


/* exported subroutines */


int recorder_init(asp,n)
RECORDER	*asp ;
int		n ;
{
	int	rs ;
	int	size ;


	if (asp == NULL)
	    return SR_FAULT ;

	(void) memset(asp,0,sizeof(RECORDER)) ;

	if (n < RECORDER_STARTNUM)
	    n = RECORDER_STARTNUM ;

#if	CF_DEBUGS
	debugprintf("recorder_init: allocating RECTAB n=%d\n",n) ;
#endif

	size = n * sizeof(struct recorder_entry) ;
	rs = uc_malloc(size,&asp->rectab) ;

	if (rs < 0)
	    goto bad1 ;

#if	CF_DEBUGS
	debugprintf("recorder_init: rectab=%p\n",asp->rectab) ;
#endif

	asp->e = n ;
	asp->c = 0 ;

/* create a dummy first record for implmentation reasons */

	asp->i = 1 ;
	asp->rectab[asp->i].ia = 0xFFFFFFFF ;


	asp->magic = RECORDER_MAGIC ;
	return rs ;

/* bad things */
bad1:
	asp->rectab = NULL ;
	asp->e = 0 ;

bad0:
	return rs ;
}
/* end subroutine (recorder_init) */


/* add a record to this object */
int recorder_add(asp,ia,ta,dsize,type)
RECORDER	*asp ;
uint		ia, ta, dsize, type ;
{
	int	rs, i ;


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (asp->i < 0)
	    return asp->i ;

/* do we need to extend the table ? */

	if ((asp->i + 1) > asp->e) {

	    if ((rs = recorder_extend(asp)) < 0)
	        goto bad0 ;

	} /* end if */

/* copy the new one in */

	i = asp->i ;

#if	CF_DEBUGS
	debugprintf("recorder_add: i=%d ia=%08x\n",i,ia) ;
#ifdef	COMMENT
	debugprintf("recorder_add: IA=%p\n",&asp->rectab[i].ia) ;
#endif
#endif

	asp->rectab[i].ia = ia ;
	asp->rectab[i].ta = ta ;
	asp->rectab[i].domainsize = dsize ;
	asp->rectab[i].type = type ;
	asp->i = i + 1 ;

#if	CF_DEBUGS
	debugprintf("recorder_add: rectab[%d].ia=%08x\n",
		i, asp->rectab[i].ia) ;
#endif

	asp->c += 1 ;
	return i ;

/* bad things */
bad0:
	return rs ;
}
/* end subroutine (recorder_add) */


/* is a given string IA represented ? */
int recorder_already(asp,ia)
RECORDER	*asp ;
uint		ia ;
{
	int	i ;


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (asp->i < 0)
	    return asp->i ;

	for (i = 0 ; i < asp->i ; i += 1) {

	    if (ia == asp->rectab[i].ia)
	        break ;

	} /* end for */

	return (i < asp->i) ? i : SR_NOTFOUND ;
}
/* end subroutine (recorder_already) */


/* get the length (total number of entries) of the table */
int recorder_rtlen(asp)
RECORDER	*asp ;
{


	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;

	return asp->i ;
}
/* end subroutine (recorder_rtlen) */


/* get the count of valid entries in the table */
int recorder_count(asp)
RECORDER	*asp ;
{


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	return asp->c ;
}
/* end subroutine (recorder_count) */


/* calculate the index table length (number of entries) at this point */
int recorder_indexlen(asp)
RECORDER	*asp ;
{
	int	n ;


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	n = nextpowtwo(asp->i) ;

	return n ;
}
/* end subroutine (recorder_indexlen) */


/* calculate the index table size */
int recorder_indexsize(asp)
RECORDER	*asp ;
{
	int	n ;


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	n = nextpowtwo(asp->i) ;

	return (n * 2 * sizeof(int)) ;
}
/* end subroutine (recorder_indexsize) */


/* get the address of the rectab array */
int recorder_gettab(asp,rpp)
RECORDER	*asp ;
uint		**rpp ;
{
	int	size ;


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (rpp != NULL)
	    *rpp = (uint *) asp->rectab ;

	size = asp->i * sizeof(struct recorder_entry) ;
	return size ;
}
/* end subroutine (recorder_gettab) */


/* create a record index for the caller */
int recorder_mkindex(asp,it,itsize)
RECORDER	*asp ;
uint		it[][2] ;
int		itsize ;
{
	uint	key, rhash ;
	uint	ri, hi ;

	int	rs, i ;
	int	n, size ;

	char	*sp ;


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (it == NULL)
	    return SR_FAULT ;

	n = nextpowtwo(asp->i) ;

	size = n * 2 * sizeof(uint) ;

#if	CF_DEBUGS
	debugprintf("recorder_mkindex: n=%d itsize=%d size=%d\n",
		n,itsize,size) ;
#endif

	if (size > itsize)
	    return SR_OVERFLOW ;

	(void) memset(it,0,size) ;

	for (i = 1 ; i < asp->i ; i += 1) {

#if	CF_DEBUGS
	debugprintf("recorder_mkindex: i=%d ia=%08x \n",i,asp->rectab[i].ia) ;
#ifdef	COMMENT
	debugprintf("recorder_mkindex: IA=%p\n",&asp->rectab[i].ia) ;
#endif
#endif

	    key = asp->rectab[i].ia ;
	    rhash = hashelf(&key,sizeof(uint)) ;

#if	CF_DEBUGS
	debugprintf("recorder_mkindex: rhash=%08x\n",rhash) ;
#endif

	    hi = hashindex(rhash,n) ;

#if	CF_DEBUGS
	debugprintf("recorder_mkindex: hi=%d\n",hi) ;
#endif

	    if (it[hi][0] != 0) {

	        int	lhi ;


#if	CF_DEBUGS
	debugprintf("recorder_mkindex: not empty, ri=%d\n",it[hi][0]) ;
#endif

	        while (it[hi][1] != 0) {

#if	CF_DEBUGS
		{
			uint	hi1, hi2, ri2 ;


	        	debugprintf("recorder_mkindex: hi=%d ri=%d nhi=%d\n",
				hi,it[hi][0],it[hi][1]) ;

			hi2 = it[hi][1] ;
			ri2 = it[hi2][0] ;
				
	        	debugprintf("recorder_mkindex: next ri2=%d ia=%08x\n",
				ri2,asp->rectab[ri2].ia) ;

		}
#endif /* CF_DEBUG */

	            hi = it[hi][1] ;

		} /* end while */

#if	CF_DEBUGS
	debugprintf("recorder_mkindex: last hash index, lhi=%d\n",hi) ;
#endif

	        lhi = hi ;			/* save last hash-index value */
	        hi = hashindex((hi + 1),n) ;

#if	CF_DEBUGS
	debugprintf("recorder_mkindex: next hash index, hi=%d\n",hi) ;
#endif

	        while (it[hi][0] != 0)
	            hi = hashindex((hi + 1),n) ;

#if	CF_DEBUGS
	debugprintf("recorder_mkindex: final hash index, hi=%d\n",hi) ;
#endif

	        it[lhi][1] = hi ;		/* update the previous slot */

	    } /* end if (got a hash collision) */

	    it[hi][0] = i ;
	    it[hi][1] = 0 ;

	} /* end while (looping through records) */

	rs = SR_OK ;

ret0:
	return (rs >= 0) ? n : rs ;
}
/* end subroutine (recorder_mkindex) */


/* free up this recorder object */
int recorder_free(asp)
RECORDER	*asp ;
{
	int	rs ;


#if	CF_SAFE
	if (asp == NULL)
		return SR_FAULT ;

	if (asp->magic != RECORDER_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (asp->rectab != NULL)
	    free(asp->rectab) ;

	rs = asp->i ;
	asp->e = 0 ;
	asp->i = 0 ;
	asp->magic = 0 ;
	return rs ;
}
/* end subroutine (recorder_free) */



/* PRIVATE SUBROUTINES */



/* extend the object recorder */
static int recorder_extend(asp)
RECORDER	*asp ;
{
	int	rs ;
	int	n, size ;

	uint	*nrt ;


#if	CF_DEBUGS
	debugprintf("recorder_extend: entered e=%d\n",asp->e) ;
#endif

#if	CF_FASTGROW
	asp->e = (asp->e * 2) ;
#else
	asp->e = (asp->e + RECORDER_STARTNUM) ;
#endif /* F_FASTGROW */

	n = asp->e ;
	size = n * sizeof(struct recorder_entry) ;

#if	CF_UCMALLOC
	if ((rs = uc_realloc(asp->rectab,size,&nrt)) < 0) {

	    asp->i = SR_NOMEM ;
	    return rs ;
	}
#else
	if ((nrt = (uint *) realloc(asp->rectab,size)) == NULL) {

	    asp->i = SR_NOMEM ;
	    return SR_NOMEM ;
	}
#endif /* F_UCMALLOC */

	asp->rectab = (struct recorder_entry *) nrt ;

#if	CF_DEBUGS
	debugprintf("recorder_extend: returning e=%d\n",asp->e) ;
#endif

	return (asp->e) ;
}
/* end subroutine (recorder_extend) */


/* calculate the next hash from a given one */
static int hashindex(i,n)
uint	i, n ;
{
	int	hi ;


	hi = MODP2(i,n) ;

	if (hi == 0)
		hi = 1 ;

	return hi ;
}
/* end if (hashindex) */



