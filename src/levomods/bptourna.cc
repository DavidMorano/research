/* bptourna SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* this is a TOURNA (Tournament) branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	This object module was created for Levo research.  It is a
	value predictor.  This is not coded as hardware.  It is
	like Atom analysis subroutines!

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

  	Object:
	bptourna

	Description:
	This object module implements a branch predictor.  This BP
	is a Tournament type branch predictor (see McFarling and
	then Alpha-21264).  The Tournament predictor was first
	designed for and used in the Alpha-21264 (EV6) and then
	also used in the Alpha-21364 (EV7).

*******************************************************************************/

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
#include	"bptourna.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */

#define	BPTOURNA_DEFGLEN	4		/* default entries */
#define	BPTOURNA_DEFLPLEN	4		/* default entries */
#define	BPTOURNA_DEFLBLEN	4		/* default entries */
#define	BPTOURNA_LPHSTATES	8		/* LPHT states */
#define	BPTOURNA_GPHSTATES	4		/* GPHT states */
#define	BPTOURNA_GCHSTATES	4		/* CPHT states */

#define	GETPRED(c)	!!(((c) >> 1) & 1)
#define	GETPRED2(c)	!!(((c) >> 1) & 1)
#define	GETPRED3(c)	!!(((c) >> 2) & 1)


/* imported namespaces */

using std::cast_saturate ;		/* subroutine */
using levomod::flbsi ;			/* subroutine */
using libuc::mem ;			/* variable */


/* local typedefs */

typedef uchar		cpht_t ;	/* choice PHT */
typedef uchar		gpht_t ;	/* global PHT */
typedef uint		lbht_t ;	/* local BHT */
typedef uchar		lpht_t ;	/* local PHT */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int bptourna_magic(bptourna *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == BPTOURNA_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (bptourna_magic) */

/* local variables */


/* exported variables */

const levomod_obj	bpbptourna_mod = {
	"bptourna",
	szof(bptourna)
} ;


/* exported subroutines */

int bptourna_start(bptourna *op,int lhlen,int lplen,int glen) noex {
	int		rs = SR_FAULT ;
	if (op) {
	    int npt ;
	    int sz ;
	    memclear(op) ;
	    /* the choice PHT */
	    if (glen <= 2) glen = BPTOURNA_DEFGLEN ;
	    {
	        npt = nextpowtwo(glen) ;
	        op->glen = uint(npt) ;
	    }
	    sz = op->glen * szof(uchar) ;
	    /* choice PHT */
	    if (char *p ; (rs = mem.call(1,sz,&p)) >= 0) {
	        op->cpht = resumelife<cpht_t>(p) ;
		/* global PHT */
		if ((rs = mem.call(1,sz,&p)) >= 0) {
		    op->gpht = resumelife<gpht_t>(p) ;
		    /* local BHT */
		    if (lhlen <= 2) lhlen = BPTOURNA_DEFLBLEN ;
		    {
		        npt = nextpowtwo(lhlen) ;
		        op->lhlen = uint(npt) ;
		    }
		    sz = op->lhlen * szof(uint) ;
		    if ((rs = mem.call(1,sz,&p)) >= 0) {
		        op->lbht = resumelife<lbht_t>(p) ;
			/* local PHT */
			if (lplen <= 2) lplen = BPTOURNA_DEFLPLEN ;
			{
			    npt = nextpowtwo(lplen) ;
			    op->lplen = uint(npt) ;
			}
			sz = op->lplen * szof(uchar) ;
			if ((rs = mem.call(1,sz,&p)) >= 0) {
			    op->lpht = resumelife<lpht_t>(p) ;
			    /* global branch history register */
			    op->historymask = (op->glen - 1) ;
			    /* we are out of here */
			    op->magval = BPTOURNA_MAGIC ;
			} /* end if (memory-allocation) */
			if (rs < 0) {
			    mem.free(op->lbht) ;
			    op->lbht = nullptr ;
			}
		    } /* end if (memory-alloction) */
		    if (rs < 0) {
			mem.free(op->gpht) ;
			op->gpht = nullptr ;
		    }
	        } /* end if (memory-alloction) */
		if (rs < 0) {
		    mem.free(op->cpht) ;
		    op->cpht = nullptr ;
		}
	    } /* end if (memory-alloction) */
	} /* end if (bptourna_magic) */
	return rs ;
}
/* end subroutine (bptourna_start) */

int bptourna_finish(bptourna *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = bptourna_magic(op)) >= 0) {
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
	} /* end if (bptourna_magic) */
	return rs ;
}
/* end subroutine (bptourna_finish) */

/* lookup an IA */
int bptourna_lookup(bptourna *op,uint ia) noex {
    	int		rs ;
	int		fpred = 0 ; /* return-value */
	if ((rs = bptourna_magic(op)) >= 0) {
	    int		lbi, lpi ;
	    int		gi ;
	    bool	f_select ;
	    gi = op->bhistory & op->historymask ;
	    f_select = GETPRED(op->cpht[gi]) ;
	    if (f_select) {
	        fpred = GETPRED(op->gpht[gi]) ;
	    } else {
	        lbi = (ia >> 2) % op->lhlen ;
	        lpi = op->lbht[lbi] % op->lplen ;
	        fpred = GETPRED3(op->lpht[lpi]) ;
	    }
	} /* end if (bptourna_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bptourna_lookup) */

int bptourna_confidence(bptourna *op,uint ia) noex {
    	int		rs ;
	int		pred = 0 ; /* return-value */
	if ((rs = bptourna_magic(op)) >= 0) {
	    int		lbi, lpi ;
	    int		gi ;
	    bool	f_select ;
	    gi = op->bhistory & op->historymask ;
	    f_select = GETPRED(op->cpht[gi]) ;
	    if (f_select) {
	        pred = op->gpht[gi] * 2 ;
	    } else {
	        lbi = (ia >> 2) % op->lhlen ;
	        lpi = op->lbht[lbi] % op->lplen ;
	        pred = op->lpht[lpi] ;
	    }
	} /* end if (bptourna_magic) */
	return (rs >= 0) ? pred : rs ;
}
/* end subroutine (bptourna_confidence) */

int bptourna_update(bptourna *op,uint ia,int f_outcome) noex {
    	int		rs ;
	if ((rs = bptourna_magic(op)) >= 0) {
	    uint	ncount ;
	    int		lbi, lpi ;
	    int		gi ;
	    bool	f_lpred, f_gpred ;
	    bool	f_lagree, f_gagree ;
	    lbi = (ia >> 2) % op->lhlen ;
	    /* update local PHT */
	    lpi = op->lbht[lbi] % op->lplen ;
	    f_lpred = GETPRED3(op->lpht[lpi]) ;
	    ncount = satcount(op->lpht[lpi],BPTOURNA_LPHSTATES,f_outcome) ;
	    op->lpht[lpi] = cast_saturate<lpht_t>(ncount) ;
	    /* update local BHT */
	    op->lbht[lbi] = ((op->lbht[lbi] << 1) | uint(f_outcome)) ;
	    /* update GPHT */
	    gi = op->bhistory & op->historymask ;
	    f_gpred = GETPRED(op->gpht[gi]) ;
	    ncount = satcount(op->gpht[gi],BPTOURNA_GPHSTATES,f_outcome) ;
	    op->gpht[gi] = cast_saturate<gpht_t>(ncount) ;
	    /* update CPHT (global is UP, and local is DOWN) */
	    f_lagree = LEQUIV(f_lpred,f_outcome) ;
	    f_gagree = LEQUIV(f_gpred,f_outcome) ;
	    if (! LEQUIV(f_lagree,f_gagree)) {
	        ncount = satcount(op->cpht[gi],BPTOURNA_GCHSTATES,f_gagree) ;
	        op->cpht[gi] = cast_saturate<cpht_t>(ncount) ;
	    } /* end if (conditional update) */
	    /* update global branch history register */
	    op->bhistory = ((op->bhistory << 1) | uint(f_outcome)) ;
	} /* end if (bptourna_magic) */
	return rs ;
}
/* end subroutine (bptourna_update) */

int bptourna_zerostats(bptourna *op) noex {
	int		rs ;
	if ((rs = bptourna_magic(op)) >= 0) {
	    op->s = {} ;
	} /* end if (bptourna_magic) */
	return rs ;
}
/* end subroutine (bptourna_zerostats) */

int bptourna_getstats(bptourna *op,bptourna_st *rp) noex {
    	int		rs ;
	int		bitstotal = 0 ; /* return-value */
	if ((rs = bptourna_magic(op,rp)) >= 0) {
	    /* calculate the bits */
	    {
	        uint	bits_lhistory ;
	        uint	bits_lbht ;
	        uint	bits_lpht ;
	        uint	bits_ghistory ;
	        uint	bits_gpht ;
	        bits_lhistory = flbsi(op->lhlen) ;
	        bits_lbht = op->lhlen * bits_lhistory ;
	        bits_lpht = op->lplen * 3 ;
	        bits_ghistory = flbsi(op->glen) ;
	        /* two of these tables (the selector and the real GPHT) */
	        bits_gpht = 2 * op->glen * 2 ;
	        bitstotal += bits_lbht ;
	        bitstotal += bits_lpht ;
	        bitstotal += bits_lhistory ;
	        bitstotal += bits_gpht ;
	        bitstotal += bits_ghistory ;
	    } /* end block */
	    /* fill in the extra stuff */
	    if (rp) {
	        op->s = {} ;
	        rp->lbht = op->lhlen ;
	        rp->lpht = op->lplen ;
	        rp->gpht = op->glen ;
	        rp->bits = bitstotal ;
	    }
	} /* end if (bptourna_magic) */
	return (rs >= 0) ? bitstotal : rs ;
}
/* end subroutine (bptourna_getstats) */


/* private subroutines */


