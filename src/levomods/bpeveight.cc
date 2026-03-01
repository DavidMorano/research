/* bpeveight SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* this is a EVEIGHT branch predictor */
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
	eveight

	Description:
	This object module implements the GSKEW (2Bc-gskew) branch
	predictor.  This is also known as the "2Bc-gskew-pskew"
	predictor.  This was very famously used in the Alpha-21464
	(EV8) processor.

*******************************************************************************/

#include	<envstandards.h>	/* MUST be ordered first to configure */
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<numeric>		/* |cast_saturate(3c++)| */
#include	<algorithm>		/* |min(3c++)| + |max(3c++)| */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<ucmem.h>		/* |mem(3uc)| */
#include	<nextpowtwo.h>
#include	<satcount.h>
#include	<localmisc.h>

#include	"levomod.h"
#include	"bpeveight.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */

#define	BPEVEIGHT_DEFLEN	(64 * 1024)
#define	BPEVEIGHT_STATES	4
#define	BPEVEIGHT_DEFGLEN	4		/* default entries */
#define	BPEVEIGHT_DEFLPLEN	4		/* default entries */
#define	BPEVEIGHT_DEFLBLEN	4		/* default entries */
#define	BPEVEIGHT_LPHSTATES	8		/* LPHT states */
#define	BPEVEIGHT_GPHSTATES	4		/* GPHT states */
#define	BPEVEIGHT_GCHSTATES	4		/* CPHT states */

#define	GETPRED(c)	!!(((c) >> 1) & 1)
#define	GETPRED2(c)	!!(((c) >> 1) & 1)
#define	GETPRED3(c)	!!(((c) >> 2) & 1)

#define	BIT(w,n)	(((w) >> (n)) & 1)


/* imported namespaces */

using std::cast_saturate ;		/* subroutine */
using std::min ;			/* subroutine-template */
using std::max ;			/* subroutine-template */
using levomod::flbsi ;			/* subroutine */
using libuc::mem ;			/* variable */


/* local typedefs */


/* external subroutines */


/* external variablessubroutines */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int bpeveight_magic(bpeveight *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == BPEVEIGHT_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (bpeveight_magic) */

local uint	fi_bim(uint,uint) noex ;
local uint	fi_g0(uint,uint) noex ;
local uint	fi_g1(uint,uint) noex ;
local uint	fi_meta(uint,uint) noex ;


/* local variables */


/* exported variables */

const levomod_obj	bpeveight_mod = {
	"bpeveight",
	szof(bpeveight)
} ;


/* exported subroutines */

int bpeveight_start(bpeveight *op,int p1,int p2,int p3,int p4) noex {
	int		rs = SR_FAULT ;
	if (op) {
	    int		sz ;
	    int		mnum = max(p1,p2) ;
	    memclear(op) ;
	    if (p3 > mnum) mnum = p3 ;
	    if (p4 > mnum) mnum = p4 ;
	    if (mnum < 0) mnum = BPEVEIGHT_DEFLEN ;
	    op->tlen = nextpowtwo(mnum) ;
	    op->tmask = op->tlen - 1 ;
	    /* allocate the space */
	    sz = op->tlen * szof(bpeveight_ba) ;
	    if (void *p ; (rs = mem.call(1,sz,&p)) >= 0) {
	        op->table = resumelife<bpeveight_ba>(p) ;
	        /* we are out of here */
	        op->magval = BPEVEIGHT_MAGIC ;
	    } /* end if (memory-allocation) */
	} /* end if (non-null) */
	return rs ;
}
/* end subroutine (bpeveight_start) */

int bpeveight_finish(bpeveight *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = bpeveight_magic(op)) >= 0) {
	    if (op->table) {
	        rs1 = mem.free(op->table) ;
	        if (rs >= 0) rs = rs1 ;
	        op->table = nullptr ;
	    }
	    op->magval = 0 ;
	} /* end if (bpeveight_magic) */
	return rs ;
}
/* end subroutine (bpeveight_finish) */

int bpeveight_lookup(bpeveight *op,uint ia) noex {
	int		rs ;
	int		fpred = 0 ; /* return-value */
	if ((rs = bpeveight_magic(op)) >= 0) {
	    uint	a ;
	    int		ibim, ig0, ig1, imeta ;
	    bool	f_meta ;
	    bool	f_bim, f_g0, f_g1 ;
	    a = ia >> 2 ;
	    imeta = fi_meta(op->bhistory,a) & op->tmask ;
	    ibim = fi_bim(op->bhistory,a) & op->tmask ;
	    f_meta = GETPRED(op->table[imeta].meta) ;
	    f_bim = GETPRED(op->table[ibim].bim) ;
	    /* BIM will be "UP", ESKEW will be "DOWN" */
	    if (f_meta) {
	        fpred = f_bim ;
	    } else {
	        int	c ;
	        ig0 = fi_g0(op->bhistory,a) & op->tmask ;
	        ig1 = fi_g1(op->bhistory,a) & op->tmask ;
	        f_g0 = GETPRED(op->table[ig0].g0) ;
	        f_g1 = GETPRED(op->table[ig1].g1) ;
	        c = int(f_bim) + int(f_g0) + int(f_g1) ;
	        fpred = (c >= 2) ;
	    }
	} /* end if (bpeveight_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bpeveight_lookup) */

int bpeveight_update(bpeveight *op,uint ia,int f_outcome) noex {
    	constexpr uint	bmask = uint((1 << 2) - 1) ;
	constexpr uint	nstates = BPEVEIGHT_STATES ;
	int		rs ;
	if ((rs = bpeveight_magic(op)) >= 0) {
	    uint	a ;
	    uint	nc ;
	    int		ibim, ig0, ig1, imeta ;
	    int		vote ;
	    bool	f_meta ;
	    bool	f_bim, f_eskew, f_g0, f_g1 ;
	    bool	f_agree ;
	    bool	f_predict ;
	    bool	f_bimagree ;
	    a = ia >> 2 ;
	    imeta = fi_meta(op->bhistory,a) & op->tmask ;
	    ibim = fi_bim(op->bhistory,a) & op->tmask ;
	    ig0 = fi_g0(op->bhistory,a) & op->tmask ;
	    ig1 = fi_g1(op->bhistory,a) & op->tmask ;
	    {
	        f_meta = GETPRED(op->table[imeta].meta) ;
	        f_bim = GETPRED(op->table[ibim].bim) ;
	        f_g0 = GETPRED(op->table[ig0].g0) ;
	        f_g1 = GETPRED(op->table[ig1].g1) ;
	        vote = int(f_bim) + int(f_g0) + int(f_g1) ;
	        f_eskew = (vote >= 2) ;
	        /* BIM will be "UP", ESKEW will be "DOWN" */
	        f_predict = (f_meta) ? f_bim : f_eskew ;
	    } /* end block */
            /* do the updating */
	    f_agree = LEQUIV(f_bim,f_g0) && LEQUIV(f_bim,f_g1) ;
	    if (LEQUIV(f_outcome,f_predict) && (! f_agree)) {
	        if (! LEQUIV(f_bim,f_eskew)) {
		    /* strengthen META */
	            nc = satcount(op->table[imeta].meta,nstates,f_meta) ;
	            op->table[imeta].meta = (nc & bmask) ;
	        }
	        /* strengthen others */
	        if (f_meta) {
		    /* strengthen BIM */
	            nc = satcount(op->table[ibim].bim,nstates,f_outcome) ;
	            op->table[ibim].bim = (nc & bmask) ;
	        } else {
		    /* strengthen correct tables */
	            if (LEQUIV(f_bim,f_outcome)) {
	                nc = satcount(op->table[ibim].bim,nstates,f_outcome) ;
	                op->table[ibim].bim = (nc & bmask) ;
	            }
	            if (LEQUIV(f_g0,f_outcome)) {
	                nc = satcount(op->table[ig0].g0,nstates,f_outcome) ;
	                op->table[ig0].g0 = (nc & bmask) ;
	            }
	            if (LEQUIV(f_g1,f_outcome)) {
	                nc = satcount(op->table[ig1].g1,nstates,f_outcome) ;
	                op->table[ig1].g1 = (nc & bmask) ;
	            }
	        } /* end if */
	    } else {
	        if (! LEQUIV(f_bim,f_eskew)) {
		    /* strengthen META */
		    f_bimagree = LEQUIV(f_bim,f_outcome) ;
	            nc = satcount(op->table[imeta].meta,nstates,f_bimagree) ;
	            op->table[imeta].meta = (nc & bmask) ;
		    /* re-compute */
	            {
	                f_meta = GETPRED(op->table[imeta].meta) ;
	                f_bim = GETPRED(op->table[ibim].bim) ;
	                f_g0 = GETPRED(op->table[ig0].g0) ;
	                f_g1 = GETPRED(op->table[ig1].g1) ;
	                vote = int(f_bim) + int(f_g0) + int(f_g1) ;
	                f_eskew = (vote >= 2) ;
		        /* BIM will be "UP", ESKEW will be "DOWN" */
	                f_predict = (f_meta) ? f_bim : f_eskew ;
	            } /* end block */
	            if (LEQUIV(f_predict,f_outcome)) {
		        /* "strengthen participating tables" */
	                nc = satcount(op->table[ibim].bim,nstates,f_outcome) ;
	                op->table[ibim].bim = (nc & bmask) ;
	                if (! f_meta) {
	                    nc = satcount(op->table[ig0].g0,nstates,f_outcome) ;
	                    op->table[ig0].g0 = (nc & bmask) ;
	                    nc = satcount(op->table[ig1].g1,nstates,f_outcome) ;
	                    op->table[ig1].g1 = (nc & bmask) ;
	                } /* end if */
	            } else {
		        /* update all "banks" */
	                nc = satcount(op->table[ibim].bim,nstates,f_outcome) ;
	                op->table[ibim].bim = (nc & bmask) ;
	                nc = satcount(op->table[ig0].g0,nstates,f_outcome) ;
	                op->table[ig0].g0 = (nc & bmask) ;
	                nc = satcount(op->table[ig1].g1,nstates,f_outcome) ;
	                op->table[ig1].g1 = (nc & bmask) ;
	            } /* end if */
	        } /* end if */
	    } /* end if (prediction/misprediction) */
	    /* update global branch history register */
	    op->bhistory = (op->bhistory << 1) | f_outcome ;
	} /* end if (bpeveight_magic) */
	return rs ;
}
/* end subroutine (bpeveight_update) */

int bpeveight_zerostats(bpeveight *op) noex {
	int		rs ;
	if ((rs = bpeveight_magic(op)) >= 0) {
	    op->s = {} ;
	} /* end if (bpeveight_magic) */
	return rs ;
}
/* end subroutine (bpeveight_zerostats) */

int bpeveight_getstats(bpeveight *op,bpeveight_st *rp) noex {
    	int		rs ;
	int		bits_total = 0 ; /* return-value */
	if ((rs = bpeveight_magic(op)) >= 0) {
	    /* calculate the bits */
	    {
	        uint	bits_bim ;
	        uint	bits_g0 ;
	        uint	bits_g1 ;
	        uint	bits_meta ;
	        uint	bits_history ;
	        bits_bim = op->tlen * 2 ;
	        bits_g0 = op->tlen * 2 ;
	        bits_g1 = op->tlen * 2 ;
	        bits_meta = op->tlen * 2 ;
	        bits_history = 30 ;
	        bits_total += bits_bim ;
	        bits_total += bits_g0 ;
	        bits_total += bits_g1 ;
	        bits_total += bits_meta ;
	        bits_total += bits_history ;
	    } /* end block */
	    /* fill in the extra stuff */
	    if (rp) {
	        *rp = op->s ;
	        rp->tlen = op->tlen ;
	        rp->bits = bits_total ;
	    }
	} /* end if (bpeveight_magic) */
	return (rs >= 0) ? bits_total : rs ;
}
/* end subroutine (bpeveight_getstats) */


/* private subroutines */

local uint fi_bim(uint h,uint a) noex {
	int	ibank, iline, iextra_lo, iextra_hi ;
	int	imore ;
	int	index ;

	ibank = (BIT(a,6) << 1) | BIT(a,5) ;

	iline = ((h & 0x0f) << 2) | ((a >> 7) & 0x03) ;

	iextra_hi = (BIT(a,11) << 2) |
	    ((BIT(a,9) ^ BIT(a,5)) << 1) |
	    (BIT(a,10) ^ BIT(a,6)) ;

	iextra_lo = (BIT(a,4)  << 2) |
	    ((BIT(a,3) ^ BIT(a,6)) << 1) |
	    (BIT(a,2) ^ BIT(a,5)) ;

	imore = (BIT(a,15) << 1) | BIT(a,14) ;

	index = (imore << 14) | (iextra_hi << 11) | (iline << 5) | 
	    (iextra_lo << 2) | ibank ;

	return index ;
}
/* end subroutine (fi_bim) */

local uint fi_g0(uint h,uint a) noex {
	int	ibank, iline, iextra_lo, iextra_hi ;
	int	index ;

	ibank = (BIT(a,6) << 1) | BIT(a,5) ;

	iline = ((h & 0x0f) << 2) | ((a >> 7) & 0x03) ;

	iextra_hi = ((BIT(h,7) ^ BIT(h,11)) << 4) |
	    ((BIT(h,8) ^ BIT(h,12)) << 3) |
	    ((BIT(h,4) ^ BIT(h,5)) << 2) |
	    ((BIT(a,9) ^ BIT(h,9)) << 1) |
	    ((BIT(h,10) ^ BIT(h,6)) << 0) ;

	{
	    uint	i4, i3, i2 ;


	    i4 = BIT(a,4) ^ BIT(a,9) ^ BIT(a,13) ^
	        BIT(a,12) ^ BIT(h,5) ^ 
	        BIT(h,5) ^ BIT(h,11) ^ BIT(h,8) ^
	        BIT(a,5) ;

	    i3 = BIT(a,3) ^ BIT(a,11) ^
	        BIT(h,9) ^ BIT(h,10) ^ BIT(h,12) ^
	        BIT(a,6) ^ BIT(a,5) ;

	    i2 = BIT(a,2) ^ BIT(a,14) ^ BIT(a,10) ^
	        BIT(h,6) ^ BIT(h,4) ^ BIT(h,7) ^
	        BIT(a,6) ;

	    iextra_lo = (i4 << 2) | (i3 << 1) | i2 ;

	} /* end block */

	index = (iextra_hi << 11) | (iline << 5) | 
	    (iextra_lo << 2) | ibank ;

	return index ;
}
/* end subroutine (fi_g0) */

local uint fi_g1(uint h,uint a) noex {
	int	ibank, iline, iextra_lo, iextra_hi ;
	int	index ;

	ibank = (BIT(a,6) << 1) | BIT(a,5) ;

	iline = ((h & 0x0f) << 2) | ((a >> 7) & 0x03) ;

	iextra_hi = ((BIT(h,19) ^ BIT(h,12)) << 4) |
	    ((BIT(h,18) ^ BIT(h,11)) << 3) |
	    ((BIT(h,17) ^ BIT(h,10)) << 2) |
	    ((BIT(a,16) ^ BIT(h,4)) << 1) |
	    ((BIT(h,15) ^ BIT(h,20)) << 0) ;

	{
	    uint	i4, i3, i2 ;

	    i4 = BIT(a,4) ^ BIT(a,11) ^ BIT(a,14) ^ BIT(a,6) ^
	        BIT(h,4) ^ BIT(h,6) ^ BIT(h,9) ^
	        BIT(h,14) ^ BIT(h,15) ^ BIT(h,16) ^
	        BIT(a,6) ;

	    i3 = BIT(a,3) ^ BIT(a,10) ^ BIT(a,13) ^
	        BIT(h,5) ^ BIT(h,11) ^ BIT(h,13) ^ 
	        BIT(h,18) ^ BIT(h,19) ^ BIT(h,20) ^
	        BIT(a,5) ;

	    i2 = BIT(a,2) ^ BIT(a,5) ^ BIT(a,9) ^
	        BIT(h,4) ^ BIT(h,8) ^ BIT(h,7) ^ BIT(a,10) ^
	        BIT(h,12) ^ BIT(h,13) ^ BIT(h,14) ^ BIT(h,17) ;

	    iextra_lo = (i4 << 2) | (i3 << 1) | i2 ;

	} /* end block */

	index = (iextra_hi << 11) | (iline << 5) | 
	    (iextra_lo << 2) | ibank ;

	return index ;
}
/* end subroutine (fi_g1) */

local uint fi_meta(uint h,uint a) noex {
	int	ibank, iline, iextra_lo, iextra_hi ;
	int	index ;
	ibank = (BIT(a,6) << 1) | BIT(a,5) ;
	iline = ((h & 0x0f) << 2) | ((a >> 7) & 0x03) ;
	iextra_hi = ((BIT(h,7) ^ BIT(h,11)) << 4) |
	    ((BIT(h,8) ^ BIT(h,12)) << 3) |
	    ((BIT(h,5) ^ BIT(h,13)) << 2) |
	    ((BIT(a,4) ^ BIT(h,9)) << 1) |
	    ((BIT(h,9) ^ BIT(h,6)) << 0) ;

	{
	    uint	i4, i3, i2 ;

	    i4 = BIT(a,4) ^ BIT(a,10) ^ BIT(a,5) ^
	        BIT(h,7) ^ BIT(h,10) ^ BIT(h,14) ^ BIT(h,13) ^
	        BIT(a,5) ;

	    i3 = BIT(a,3) ^ BIT(a,12) ^ BIT(a,14) ^ BIT(a,6) ^
	        BIT(h,4) ^ BIT(h,6) ^ BIT(h,8) ^ BIT(h,14) ;

	    i2 = BIT(a,2) ^ BIT(a,9) ^ BIT(a,11) ^ BIT(a,13) ^
	        BIT(h,5) ^ BIT(h,9) ^ BIT(h,11) ^ BIT(a,12) ^
	        BIT(a,6) ;

	    iextra_lo = (i4 << 2) | (i3 << 1) | i2 ;

	} /* end block */
	index = (iextra_hi << 11) | (iline << 5) | (iextra_lo << 2) | ibank ;
	return index ;
}
/* end subroutine (fi_meta) */


