/* bpalpha SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* this is a ALPHA branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	This object module was created for Levo research.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

  	Object:
	bpalpha

	Description:
	This object module implements a branch predictor.  This BP
	looks like a Tournament type branch predictor (see McFarling
	and then Alpha-21264).  How thi BP differs from the TOURNA
	predictor (also in this code group) is not known.

*******************************************************************************/

#include	<envstandards.h>	/* MUST be ordered first to configure */
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<numeric>		/* |cast_saturate(3c++)| */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<usyscalls.h>
#include	<ucmem.h>		/* |mem(3uc)| */
#include	<nextpowtwo.h>
#include	<satcount.h>
#include	<localmisc.h>

#include	"levomod.h"
#include	"bpalpha.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */

#define	BPALPHA_DEFGLEN		4		/* default entries */
#define	BPALPHA_DEFLPLEN	4		/* default entries */
#define	BPALPHA_DEFLBLEN	4		/* default entries */
#define	BPALPHA_LPHSTATES	8		/* LPHT states */
#define	BPALPHA_GPHSTATES	4		/* GPHT states */
#define	BPALPHA_GCHSTATES	4		/* CPHT states */

#define	GETPRED(c)	!!(((c) >> 1) & 1)
#define	GETPRED2(c)	!!(((c) >> 1) & 1)
#define	GETPRED3(c)	!!(((c) >> 2) & 1)


/* imported namespaces */

using std::cast_saturate ;		/* subroutine */
using levomod::flbsi ;			/* subroutine */
using libuc::mem ;			/* variable */


/* local typedefs */

typedef uchar           cpht_t ;         /* choice PHT */
typedef uchar           gpht_t ;         /* global PHT */
typedef uint            lbht_t ;         /* local BHT */
typedef uchar           lpht_t ;         /* local PHT */


/* external subroutines */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int bpalpha_magic(bpalpha *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == BPALPHA_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (bpalpha_magic) */


/* local variables */


/* exported variables */

const levomod_obj	bpalpha_mod = {
	"bpalpha",
	szof(bpalpha)
} ;


/* exported subroutines */

int bpalpha_start(bpalpha *op,int lhlen,int lplen,int glen) noex {
	int		rs = SR_FAULT ;
	if (glen <= 2) glen = BPALPHA_DEFGLEN ;
	if (op) {
	    cint	npt = nextpowtwo(glen) ;
	    rs = memclear(op) ;
	    {
	        int sz = npt * szof(uchar) ;
	        /* choice PHT */
	        if (void *p ; (rs = mem.call(1,sz,&p)) >= 0) {
		    op->cpht = resumelife<cpht_t>(p) ;
		    if ((rs = mem.call(1,sz,&p)) >= 0) {
			op->gpht = resumelife<gpht_t>(p) ;
		        if (lhlen <= 2) lhlen = BPALPHA_DEFLBLEN ;
			op->lhlen = nextpowtwo(lhlen) ;
			sz = op->lhlen * szof(uint) ;
			if ((rs = mem.call(1,sz,&p)) >= 0) {
			    op->lbht = resumelife<lbht_t>(p) ;
			    if (lplen <= 2) lplen = BPALPHA_DEFLPLEN ;
			    op->lplen = nextpowtwo(lplen) ;
			    sz = op->lplen * szof(uchar) ;
			    if ((rs = mem.call(1,sz,&p)) >= 0) {
				op->lpht = resumelife<lpht_t>(p) ;
				op->historymask = (op->glen - 1) ;
				op->magval = BPALPHA_MAGIC ;
			    } /* end if (memory-allocation) */
			    if (rs < 0) {
			        mem.free(op->lbht) ;
				op->lbht = nullptr ;
			    }
			} /* end if (memory-allocation) */
			if (rs < 0) {
			    mem.free(op->gpht) ;
			    op->gpht = nullptr ;
			}
		    } /* end if (memory-allocation) */
		    if (rs < 0) {
			mem.free(op->cpht) ;
			op->cpht = nullptr ;
		    }
		} /* end if (memory-allocation) */
	    } /* end block */
	} /* end if (non-null) */
	return rs ;
}
/* end subroutine (bpalpha_start) */

int bpalpha_finish(bpalpha *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = bpalpha_magic(op)) >= 0) {
	    if (op->lpht) {
	        rs1 = mem.free(op->lpht) ;
	        if (rs >= 0) rs = rs1 ;
	        op->lpht = nullptr ;
	    }
	    if (op->lbht) {
	        rs1 = mem.free(op->lbht) ;
	        if (rs >= 0) rs = rs1 ;
	        op->lbht = nullptr ;
	    }
	    if (op->gpht) {
	        rs1 = mem.free(op->gpht) ;
	        if (rs >= 0) rs = rs1 ;
	        op->gpht = nullptr ;
	    }
	    if (op->cpht) {
	        rs1 = mem.free(op->cpht) ;
	        if (rs >= 0) rs = rs1 ;
	        op->cpht = nullptr ;
	    }
	    op->magval = 0 ;
	} /* end if (bpalpha_magic) */
	return rs ;
}
/* end subroutine (bpalpha_finish) */

int bpalpha_lookup(bpalpha *op,uint ia) noex {
    	int		rs ;
	int		fpred = false ; /* return-value */
	if ((rs = bpalpha_magic(op)) >= 0) {
	    int		lbi ;
	    int		lpi ;
	    int		gi = (op->bhistory & op->historymask) ;
	    int		f_select ;
	    f_select = GETPRED(op->cpht[gi]) ;
	    if (f_select) {
	        fpred = GETPRED(op->gpht[gi]) ;
	    } else {
	        lbi = (ia >> 2) % op->lhlen ;
	        lpi = op->lbht[lbi] % op->lplen ;
	        fpred = GETPRED3(op->lpht[lpi]) ;
	    }
	} /* end if (bpalpha_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bpalpha_lookup) */

int bpalpha_confidence(bpalpha *op,uint ia) noex {
    	int		rs ;
	int		pred = 0 ; /* return-value */
	if ((rs = bpalpha_magic(op)) >= 0) {
	    int		lbi, lpi ;
	    int		gi ;
	    int		f_select ;
	    gi = op->bhistory & op->historymask ;
	    f_select = GETPRED(op->cpht[gi]) ;
	    if (f_select) {
	        pred = op->gpht[gi] * 2 ;
	    } else {
	        lbi = (ia >> 2) % op->lhlen ;
	        lpi = op->lbht[lbi] % op->lplen ;
	        pred = op->lpht[lpi] ;
	    }
	} /* end if (bpalpha_magic) */
	return (rs >= 0) ? pred : rs ;
}
/* end subroutine (bpalpha_confidence) */

/* update on branch resolution */
int bpalpha_update(bpalpha *op,uint ia,int f_outcome) noex {
    	int		rs ;
	if ((rs = bpalpha_magic(op)) >= 0) {
	    uint	ncount ;
	    int		lbi, lpi ;
	    int		gi ;
	    bool	f_lpred, f_gpred ;
	    bool	f_lagree, f_gagree ;
	    lbi = (ia >> 2) % op->lhlen ;
	    /* update local PHT */
	    lpi = op->lbht[lbi] % op->lplen ;
	    f_lpred = GETPRED3(op->lpht[lpi]) ;
	    ncount = satcount(op->lpht[lpi],BPALPHA_LPHSTATES,f_outcome) ;
	    op->lpht[lpi] = cast_saturate<lpht_t>(ncount) ;
	    /* update local BHT */
	    op->lbht[lbi] = (op->lbht[lbi] << 1) | f_outcome ;
	    /* update GPHT */
	    gi = op->bhistory & op->historymask ;
	    f_gpred = GETPRED(op->gpht[gi]) ;
	    ncount = satcount(op->gpht[gi],BPALPHA_GPHSTATES,f_outcome) ;
	    op->gpht[gi] = cast_saturate<gpht_t>(ncount) ;
	    /* update CPHT (global is UP, and local is DOWN) */
	    f_lagree = LEQUIV(f_lpred,f_outcome) ;
	    f_gagree = LEQUIV(f_gpred,f_outcome) ;
	    if (! LEQUIV(f_lagree,f_gagree)) {
	        ncount = satcount(op->cpht[gi],BPALPHA_GCHSTATES,f_gagree) ;
	        op->cpht[gi] = cast_saturate<cpht_t>(ncount) ;
	    } /* end if (conditional update) */
	    /* update global branch history register */
	    op->bhistory = (op->bhistory << 1) | f_outcome ;
	} /* end if (bpalpha_magic) */
	return rs ;
}
/* end subroutine (bpalpha_update) */

int bpalpha_zerostats(bpalpha *op) noex {
	int		rs ;
	if ((rs = bpalpha_magic(op)) >= 0) {
	    op->s = {} ;
	} /* end if (bpalpha_magic) */
	return rs ;
}
/* end subroutine (bpalpha_zerostats) */

int bpalpha_getstats(bpalpha *op,bpalpha_st *rp) noex {
    	int		rs ;
	int		bitstotal = 0 ; /* return-value */
	if ((rs = bpalpha_magic(op,rp)) >= 0) {
	    /* calculate the bits */
	    uint	bits_lbht ;
	    uint	bits_lpht ;
	    uint	bits_lhistory ;
	    uint	bits_ghistory ;
	    bits_ghistory = flbsi(op->lplen) ; /* is this correct? */
	    bits_lbht = op->lhlen * bits_ghistory ;
	    bits_lpht = op->lplen * 3 ;
	    bits_lhistory = flbsi(op->glen) ; /* is this correct? */
	    bitstotal += bits_lbht ;
	    bitstotal += bits_lpht ;
	    bitstotal += bits_lhistory ;
	    bitstotal += bits_ghistory ;
	    /* fill in the extra stuff */
	    if (rp) {
	        rp = {} ;
	        rp->lbht = op->lhlen ;
	        rp->lpht = op->lplen ;
	        rp->gpht = op->glen ;
	        rp->bits = bitstotal ;
	    } /* end if (return-stats) */
	} /* end if (bpalpha_magic) */
	return (rs >= 0) ? bitstotal : rs ;
}
/* end subroutine (bpalpha_getstats) */


/* private subroutines */


