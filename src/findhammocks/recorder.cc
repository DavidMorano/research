/* recorder SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* IA recorder object */
/* version %I% last-modified %G% */

#define	CF_DEBUG	0		/* non-switchable print-outs */
#define	CF_FASTGROW	1		/* grow exponetially? */
#define	CF_UCMALLOC	1
#define	CF_SAFE		1
#define	CF_WEIRD	0		/* do NOT use */

/* revision history:

	= 2002-04-29, David A-D- Morano
	This object module was created for Levo research.
	We need to identify SS-Hammock style conditional branches.

*/

/* Copyright © 2002 David A-D- Morano.  All rights reserved. */

/******************************************************************************

  	Description:
	This object module creates a data base of SS-Hammock style
	branches.  This object can form the "lower-half" of a system
	to identify SS-Hammocks and to write the resulting data base
	information out to a file.  This object only manages an
	in-core version of the database.  Another "upper-level"
	component must manage:

	+ identification of SS-Hammocks
	+ call this module to track them
	+ write out the resulting data base to a file for storage

*****************************************************************************/

#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<cstddef>		/* CSTD */
#include	<cstdlib>		/* CSTD */
#include	<cstring>		/* CSTD */
#include	<clanguage.h>		/* LIBU */
#include	<usysbase.h>		/* LIBU */
#include	<strwcpy.h>		/* LIBUC */
#include	<hash.h>		/* LIBUC */
#include	<powtwo.h>		/* LIBUC */
#include	<localmisc.h>		/* LIBU */
#include	<libdebug.h>		/* LIBDEBUG |DEBUGPRINTF(3debug)| */

#include	"recorder.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |lenstr(3u)| */

/* local defines */

#define	RC		recorder
#define	RC_ENT		recorder_ent

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
#endif /* ENDIAN */


/* external subroutines */


/* external variables */


/* forward references */

local int	recorder_extend(RC *) noex ;


/* local variables */


/* exported variables */


/* exported subroutines */

int recorder_init(RC *asp,int n) noex {
	int	rs ;
	int	sz ;

	if (asp == nullptr)
	    return SR_FAULT ;

	memclear(asp) ;

	if (n < RECORDER_STARTNUM)
	    n = RECORDER_STARTNUM ;

#if	CF_DEBUG
	DEBUGPRINTF("allocating RECTAB n=%d\n",n) ;
#endif

	sz = n * szof(struct recorder_entry) ;
	rs = uc_malloc(sz,&asp->rectab) ;

	if (rs < 0)
	    goto bad1 ;

#if	CF_DEBUG
	DEBUGPRINTF("rectab=%p\n",asp->rectab) ;
#endif

	asp->e = n ;
	asp->c = 0 ;

/* create a dummy first record for implmentation reasons */

	asp->i = 1 ;
	asp->rectab[asp->i].ia = 0xFFFFFFFF ;


	asp->magval = RECORDER_MAGIC ;
	return rs ;

/* bad things */
bad1:
	asp->rectab = nullptr ;
	asp->e = 0 ;

bad0:
	return rs ;
} /* end subroutine (recorder_init) */

/* add a record to this object */
int recorder_add(RC *asp,uint ia,uint ta,uint dsz,uint type) noex {
	int	rs, i ;

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (asp->i < 0)
	    return asp->i ;

/* do we need to extend the table? */

	if ((asp->i + 1) > asp->e) {
	    if ((rs = recorder_extend(asp)) < 0)
	        goto bad0 ;
	} /* end if */

/* copy the new one in */

	i = asp->i ;

#if	CF_DEBUG
	DEBUGPRINTF("i=%d ia=%08x\n",i,ia) ;
#ifdef	COMMENT
	DEBUGPRINTF("IA=%p\n",&asp->rectab[i].ia) ;
#endif
#endif

	asp->rectab[i].ia = ia ;
	asp->rectab[i].ta = ta ;
	asp->rectab[i].domainsize = dsz ;
	asp->rectab[i].type = type ;
	asp->i = i + 1 ;

#if	CF_DEBUG
	DEBUGPRINTF("rectab[%d].ia=%08x\n",i,asp->rectab[i].ia) ;
#endif

	asp->c += 1 ;
	return i ;

/* bad things */
bad0:
	return rs ;
} /* end subroutine (recorder_add) */

/* is a given string IA represented? */
int recorder_already(RC *asp,uint ia) noex {
	int	i ;

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (asp->i < 0)
	    return asp->i ;

	for (i = 0 ; i < asp->i ; i += 1) {
	    if (ia == asp->rectab[i].ia) break ;
	} /* end for */

	return (i < asp->i) ? i : SR_NOTFOUND ;
} /* end subroutine (recorder_already) */

/* get the length (total number of entries) of the table */
int recorder_rtlen(RC *asp) noex {
	if (asp == nullptr)
		return SR_FAULT ;

	if (asp->magval != RECORDER_MAGIC)
		return SR_NOTOPEN ;

	return asp->i ;
} /* end subroutine (recorder_rtlen) */

/* get the count of valid entries in the table */
int recorder_count(RC *asp) noex {

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	return asp->c ;
} /* end subroutine (recorder_count) */

/* calculate the index table length (number of entries) at this point */
int recorder_indexlen(RC *asp) noex {
	int	n ;

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	n = nextpowtwo(asp->i) ;

	return n ;
} /* end subroutine (recorder_indexlen) */

/* calculate the index table size */
int recorder_indexsize(RC *asp) noex {
	int	n ;

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	n = nextpowtwo(asp->i) ;

	return (n * 2 * szof(int)) ;
} /* end subroutine (recorder_indexsize) */

/* get the address of the rectab array */
int recorder_gettab(RC *asp,uint **rpp) noex {
	int	sz ;

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (rpp != nullptr) {
	    *rpp = (uint *) asp->rectab ;
	}
	sz = asp->i * szof(struct recorder_entry) ;
	return sz ;
} /* end subroutine (recorder_gettab) */

/* create a record index for the caller */
int recorder_mkindex(RC *asp,uint (*it)[2],int itsz) noex {
	uint	key, rhash ;
	uint	ri, hi ;
	int	rs, i ;
	int	n, sz ;
	char	*sp ;

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (it == nullptr) return SR_FAULT ;

	n = nextpowtwo(asp->i) ;

	sz = n * 2 * szof(uint) ;

#if	CF_DEBUG
	DEBUGPRINTF("n=%d itsz=%d sz=%d\n", n,itsz,sz) ;
#endif

	if (sz > itsz)
	    return SR_OVERFLOW ;

	memclear(it,sz) ;

	for (i = 1 ; i < asp->i ; i += 1) {

#if	CF_DEBUG
	DEBUGPRINTF("i=%d ia=%08x \n",i,asp->rectab[i].ia) ;
#ifdef	COMMENT
	DEBUGPRINTF("IA=%p\n",&asp->rectab[i].ia) ;
#endif
#endif

	    key = asp->rectab[i].ia ;
	    rhash = hashelf(&key,szof(uint)) ;

#if	CF_DEBUG
	DEBUGPRINTF("rhash=%08x\n",rhash) ;
#endif

	    hi = hashindex(rhash,n) ;

#if	CF_DEBUG
	DEBUGPRINTF("hi=%d\n",hi) ;
#endif

	    if (it[hi][0] != 0) {

	        int	lhi ;


#if	CF_DEBUG
	DEBUGPRINTF("not empty, ri=%d\n",it[hi][0]) ;
#endif

	        while (it[hi][1] != 0) {

#if	CF_DEBUG
		{
			uint	hi1, hi2, ri2 ;
	        	DEBUGPRINTF("hi=%d ri=%d nhi=%d\n",
				hi,it[hi][0],it[hi][1]) ;
			hi2 = it[hi][1] ;
			ri2 = it[hi2][0] ;
	        	DEBUGPRINTF("next ri2=%d ia=%08x\n",
				ri2,asp->rectab[ri2].ia) ;
		}
#endif /* CF_DEBUG */

	            hi = it[hi][1] ;

		} /* end while */

#if	CF_DEBUG
	DEBUGPRINTF("last hash index, lhi=%d\n",hi) ;
#endif

	        lhi = hi ;			/* save last hash-index value */
	        hi = hashindex((hi + 1),n) ;

#if	CF_DEBUG
	DEBUGPRINTF("next hash index, hi=%d\n",hi) ;
#endif

	        while (it[hi][0] != 0)
	            hi = hashindex((hi + 1),n) ;

#if	CF_DEBUG
	DEBUGPRINTF("final hash index, hi=%d\n",hi) ;
#endif

	        it[lhi][1] = hi ;		/* update the previous slot */

	    } /* end if (got a hash collision) */

	    it[hi][0] = i ;
	    it[hi][1] = 0 ;

	} /* end while (looping through records) */

	rs = SR_OK ;

ret0:
	return (rs >= 0) ? n : rs ;
} /* end subroutine (recorder_mkindex) */

/* free up this recorder object */
int recorder_free(RC *asp) noex {
	int	rs ;

#if	CF_SAFE
	if (asp == nullptr) return SR_FAULT ;
	if (asp->magval != RECORDER_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (asp->rectab != nullptr)
	    free(asp->rectab) ;

	rs = asp->i ;
	asp->e = 0 ;
	asp->i = 0 ;
	asp->magval = 0 ;
	return rs ;
} /* end subroutine (recorder_free) */


/* private subroutines */

/* extend the object recorder */
local int recorder_extend(RC *asp) noex {
	int	rs ;
	int	n, sz ;
	uint	*nrt ;

#if	CF_DEBUG
	DEBUGPRINTF("ent e=%d\n",asp->e) ;
#endif

#if	CF_FASTGROW
	asp->e = (asp->e * 2) ;
#else
	asp->e = (asp->e + RECORDER_STARTNUM) ;
#endif /* F_FASTGROW */

	n = asp->e ;
	sz = n * szof(RC_ENT) ;

#if	CF_UCMALLOC
	if ((rs = uc_realloc(asp->rectab,sz,&nrt)) < 0) {
	    asp->i = SR_NOMEM ;
	    return rs ;
	}
#else
	if ((nrt = (uint *) realloc(asp->rectab,sz)) == nullptr) {

	    asp->i = SR_NOMEM ;
	    return SR_NOMEM ;
	}
#endif /* F_UCMALLOC */

	asp->rectab = (struct recorder_entry *) nrt ;

#if	CF_DEBUG
	DEBUGPRINTF("returning e=%d\n",asp->e) ;
#endif

	return (asp->e) ;
} /* end subroutine (recorder_extend) */


