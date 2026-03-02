/* bpgspag SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* this is a gshare-PAg branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	This object module was created for Levo research.  It is a
	value predictor.  This is not coded as hardware.  It is
	like Atom analysis subroutines!

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

  	Object:
	gspag

	Description:
	This object module implements a branch predictor.  This BP
	is a GSPAG (see Patt and then McFarling) type branch
	predictor.

*****************************************************************************/

#include	<envstandards.h>	/* MUST be ordered first to configure */
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<numeric>		/* |cast_saturate(3c++)| */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<ucmem.h>		/* |mem(3uc)| */
#include	<nextpowtwo.h>
#include	<satcount.h>
#include	<localmisc.h>

#include	"levomod.h"
#include	"bpgspag.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */

#define	GSPAG_DEFBHLEN	4		/* default entries */
#define	GSPAG_DEFPHLEN	4		/* default entries */
#define	GSPAG_GPHSTATES	4		/* GPHT states */

#define	GETPRED(c)	!!(((c) >> 1) & 1)
#define	GETPRED2(c)	!!(((c) >> 1) & 1)
#define	GETPRED3(c)	!!(((c) >> 2) & 1)


/* imported namespaces */

using std::cast_saturate ;		/* subroutine */
using levomod::flbsi ;			/* subroutine */
using libuc::mem ;			/* variable */


/* local typedefs */

typedef uint		lbht_t ;	/* local BHT */
typedef uchar		gpht_t ;	/* global PHT */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int bpgspag_magic(bpgspag *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == BPGSPAG_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (bpgspag_magic) */


/* local variables */


/* exported variables */

const levomod_obj	bpgspag_mod = {
	"bpgspag",
	szof(bpgspag)
} ;


/* exported subroutines */

int bpgspag_start(bpgspag *op,int bhlen,int phlen) noex {
	int		rs = SR_FAULT ;
	if (op) {
	    int npt ;
	    memclear(op) ;
	    /* BHT */
	    if (bhlen < 2) bhlen = GSPAG_DEFBHLEN ;
	    {
	        npt = nextpowtwo(bhlen) ;
	        op->bhlen = uint(npt) ;
	    }
	    int sz ; sz = op->bhlen * szof(lbht_t) ;
	    if (void *p ; (rs = mem.call(1,sz,&p)) >= 0) {
		op->lbht = resumelife<lbht_t>(p) ;
		/* global PHT */
		if (phlen < 2) phlen = GSPAG_DEFPHLEN ;
		{
		    npt = nextpowtwo(phlen) ;
		    op->phlen = uint(npt) ;
		}
		sz = op->phlen * szof(gpht_t) ;
		if ((rs = mem.call(1,sz,&p)) >= 0) {
		    op->gpht = resumelife<gpht_t>(p) ;
		    /* we are out of here */
		    op->magval = BPGSPAG_MAGIC ;
		} /* emd if (memory-allocation) */
		if (rs < 0) {
		    mem.free(op->lbht) ;
		    op->lbht = nullptr ;
		}
	    } /* emd if (memory-allocation) */
	} /* end if (non-null) */
	return rs ;
}
/* end subroutine (bpgspag_start) */

int bpgspag_finish(bpgspag *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = bpgspag_magic(op)) >= 0) {
	    if (op->gpht) {
	        rs1 = mem.free(op->gpht) ;
		if (rs >= 0) rs = rs1 ;
	        op->gpht = nullptr ;
	    }
	    if (op->lbht) {
	        rs1 = mem.free(op->lbht) ;
		if (rs >= 0) rs = rs1 ;
	        op->lbht = nullptr;
	    }
	    op->magval = 0 ;
	} /* end if (bpgspag_magic) */
	return rs ;
}
/* end subroutine (bpgspag_finish) */

int bpgspag_lookup(bpgspag *op,uint ia) noex {
    	int		rs ;
	int		fpred = 0 ; /* return-value */
	if ((rs = bpgspag_magic(op)) >= 0) {
	    int		lbi ;
	    int		gpi ;
	    lbi = (ia >> 2) % op->bhlen ;
	    gpi = (op->lbht[lbi] ^ (ia >> 2)) % op->phlen ;
	    fpred = GETPRED(op->gpht[gpi]) ;
	} /* end if (bpgspag_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bpgspag_lookup) */

int bpgspag_confidence(bpgspag *op,uint ia) noex {
	int		rs ;
	int		val = 0 ; /* return-value */
	if ((rs = bpgspag_magic(op)) >= 0) {
	    int		lbi ;
	    int		gpi ;
	    lbi = (ia >> 2) % op->bhlen ;
	    gpi = (op->lbht[lbi] ^ (ia >> 2)) % op->phlen ;
	    val = op->gpht[gpi] ;
	    val <<= 1 ;
	} /* end if (bpgspag_magic) */
	return (rs >= 0) ? val : rs ;
}
/* end subroutine (bpgspag_confidence) */

int bpgspag_update(bpgspag *op,uint ia,int f_outcome) noex {
    	int		rs ;
	int		fpred = 0 ; /* return-value */
	if ((rs = bpgspag_magic(op)) >= 0) {
	    uint	ncount ;
	    int		lbi ;
	    int		gpi ;
	    lbi = (ia >> 2) % op->bhlen ;
	    gpi = (op->lbht[lbi] ^ (ia >> 2)) % op->phlen ;
	    fpred = GETPRED(op->gpht[gpi]) ;
	    /* update GPHT */
	    ncount = satcount(op->gpht[gpi],GSPAG_GPHSTATES,f_outcome) ;
	    op->gpht[gpi] = cast_saturate<gpht_t>(ncount) ;
	    /* update local BHT */
	    op->lbht[lbi] = ((op->lbht[lbi] << 1) | uint(f_outcome)) ;
	} /* end if (bpgspag_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bpgspag_update) */

int bpgspag_zerostats(bpgspag *op) noex {
	int		rs ;
	if ((rs = bpgspag_magic(op)) >= 0) {
	    op->s = {} ;
	} /* end if (bpgspag_magic) */
	return rs ;
}
/* end subroutine (bpgspag_zerostats) */

int bpgspag_getstats(bpgspag *op,bpgspag_st *rp) noex {
    	int		rs ;
	int		bitstotal = 0 ; /* return-value */
	if ((rs = bpgspag_magic(op)) >= 0) {
	    /* calculate the bits */
	    {
		uint	bits_lbht ;
		uint	bits_gpht ;
		uint	bits_history ;
		int	n = flbsi(op->phlen) ;
		bits_lbht = op->bhlen * n ;
		bits_history = flbsi(op->phlen) ;
		bits_gpht = op->phlen * BPGSPAG_COUNTBITS ;
		bitstotal = bits_lbht + bits_gpht + bits_history ;
	    } /* end block */
	    /* fill in the extra stuff */
	    if (rp) {
	        op->s = {} ;
	        rp->lbht = op->bhlen ;
	        rp->gpht = op->phlen ;
	        rp->bits = bitstotal ;
	    }
	} /* end if (bpgspag_magic) */
	return (rs >= 0) ? bitstotal : rs ;
}
/* end subroutine (bpgspag_getstats) */


/* private subroutines */


