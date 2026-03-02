/* bpyags SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* this is a YAGS branch predictor */
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
	yags

	Description:
	This object module implements a branch predictor.  This BP
	is a YAGS (see Mudge) type branch predictor.  This BP *may*
	be among the best of the "share" type predictors.

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
#include	"bpyags.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */

#define	BPYAGS_DEFCH	4		/* default entries */
#define	BPYAGS_DEFCA	4		/* default entries */

#define	GETPRED(c)	!!(((c) >> 1) & 1)


/* imported namespaces */

using std::cast_saturate ;		/* subroutine */
using levomod::flbsi ;			/* subroutine */
using libuc::mem ;			/* variable */


/* local typedefs */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int bpyags_magic(bpyags *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == BPYAGS_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (bpyags_magic) */

local int	calu(bpyags_ca *,uint,uint,uint *) noex ;
local int	caup(bpyags_ca *,uint,uint,int,int) noex ;


/* local variables */


/* exported variables */

const levomod_obj	bpbpyags_mod = {
	"bpyags",
	szof(bpyags)
} ;


/* exported subroutines */

int bpyags_init(bpyags *op,int chlen,int calen) noex {
	int		rs = SR_FAULT ;
	if (op) {
	    int		npt ;
	    int		sz ;
	    memclear(op) ;
	    /* the choice PHT */
	    if (chlen <= 2) chlen = BPYAGS_DEFCH ;
	    {
	        npt = nextpowtwo(chlen) ;
	        op->chlen = uint(npt) ;
	    }
	    sz = op->chlen * szof(bpyags_pht) ;
	    if (void *p ; (rs = mem.call(1,sz,&p)) >= 0) {
	        op->choice = resumelife<bpyags_pht>(p) ;
	        if (calen <= 2) calen = BPYAGS_DEFCA ;
		{
	            npt = nextpowtwo(calen) ;
	            op->calen = uint(npt) ;
		}
	        sz = op->calen * szof(bpyags_ca) ;
	        if ((rs = mem.call(1,sz,&p)) >= 0) {
		    op->taken = resumelife<bpyags_ca>(p) ;
		    if ((rs = mem.call(1,sz,&p)) >= 0) {
		        op->nottaken = resumelife<bpyags_ca>(p) ;
		        op->magval = BPYAGS_MAGIC ;
		    } /* end if (memory-allocation) */
		    if (rs < 0) {
			mem.free(op->taken) ;
			op->taken = nullptr ;
		    }
	        } /* end if (memory-allocation) */
		if (rs < 0) {
		    mem.free(op->choice) ;
		    op->choice = nullptr ;
		}
	    } /* end if (memory-allocation) */
	} /* end if (non-null) */
	return rs ;
}
/* end subroutine (bpyags_start) */

int bpyags_finish(bpyags *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = bpyags_magic(op)) >= 0) {
	    if (op->nottaken) {
	        rs1 = mem.free(op->nottaken) ;
		if (rs >= 0) rs = rs1 ;
	        op->nottaken = nullptr ;
	    }
	    if (op->taken) {
	        rs1 = mem.free(op->taken) ;
		if (rs >= 0) rs = rs1 ;
	        op->taken = nullptr ;
	    }
	    if (op->choice) {
	        rs1 = mem.free(op->choice) ;
		if (rs >= 0) rs = rs1 ;
	        op->choice = nullptr ;
	    }
	    op->magval = 0 ;
	} /* end if (bpyags_magic) */
	return rs ;
}
/* end subroutine (bpyags_free) */

int bpyags_lookup(bpyags *op,uint ia) noex {
	int		rs ;
	int		fpred = 0 ; /* return-value */
	if ((rs = bpyags_magic(op)) >= 0) {
	    uint	count_ch ;
	    uint	count_ca ;
	    int		chi ;
	    int		cai ;
	    int		tag ;
	    bool	f_chpred ;
	    chi = (ia >> 2) % op->chlen ;
	    cai = ((ia >> 2) ^ (op->bhistory >> 1)) % op->calen ;
	    tag = (ia >> 2) & BPYAGS_TAGMASK ;
	    tag <<= 1 ;
	    tag |= (op->bhistory & 1) ;
	    count_ch = op->choice[chi].counter ;
	    f_chpred = !!((count_ch >> 1) & 1) ;
	    if (f_chpred) {
	        /* choice says "taken" so use the "not-taken" cache */
	        rs = calu(op->nottaken,cai,tag,&count_ca) ;
	    } else {
	        /* choice says "not-taken" so use the "taken" cache */
	        rs = calu(op->taken,cai,tag,&count_ca) ;
	    } /* end if (taken/not-taken) */
	    if (rs >= 0) {
	        op->nottaken[cai].lru = ((rs == 0) ? 1 : 0) ;
	        fpred = ((count_ca >> 1) & 1) ;
	    } else {
	        fpred = f_chpred ;
	    }
	} /* end if (bpyags_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bpyags_lookup) */

int bpyags_update(bpyags *op,uint ia,int f_outcome) noex {
	int		rs ;
	if ((rs = bpyags_magic(op)) >= 0) {
	    uint	count_ch ;
	    uint	count_ca ;
	    uint	ncount ;
	    int		chi ;
	    int		cai ;
	    int		tag ;
	    bool	f_hit ;
	    bool	f_pred ;
	    bool	f_chpred ;
	    chi = (ia >> 2) % op->chlen ;
	    cai = ((ia >> 2) ^ (op->bhistory >> 1)) % op->calen ;
	    tag = (ia >> 2) & BPYAGS_TAGMASK ;
	    tag <<= 1 ;
	    tag |= (op->bhistory & 1) ;
	    count_ch = op->choice[chi].counter ;
	    f_chpred = !!((count_ch >> 1) & 1) ;
	    if (f_chpred) {
    		/* choice says "taken" so use the "not-taken" cache */
	        rs = calu(op->nottaken,cai,tag,&count_ca) ;
	    } else {
    		/* choice says "not-taken" so use the "taken" cache */
	        rs = calu(op->taken,cai,tag,&count_ca) ;
	    } /* end if (taken/not-taken) */
	    f_hit = false ;
	    if (rs >= 0) {
	        f_hit = true ;
	        f_pred = ((count_ca >> 1) & 1) ;
	    } else {
	        f_pred = f_chpred ;
	    }
            /* update stuff */
	    /* choice PHT */
	    {
	        if (! ((! LEQUIV(f_chpred,f_outcome)) && 
	                f_hit && LEQUIV(f_outcome,f_pred))) {
	            ncount = satcount(count_ca,BPYAGS_COUNTBITS,f_outcome) ;
	            op->choice[chi].counter = uint(ncount & 0x03) ;
		}
	    } /* end if (choice PHT conditional update) */
            /* the direction caches */
            /* update "not-taken" cache */
	    if (f_hit && f_chpred) {
	        caup(op->nottaken,cai,tag,f_outcome,0) ;
	    } else if (f_chpred && (! f_outcome)) {
	        caup(op->nottaken,cai,tag,f_outcome,0) ;
	    }
	    /* update "taken" cache */
	    if (f_hit && (! f_chpred)) {
	        caup(op->taken,cai,tag,f_outcome,1) ;
	    } else if ((! f_chpred) && (! f_outcome)) {
	        caup(op->taken,cai,tag,f_outcome,1) ;
	    } /* end if */
	    /* update the branch history register */
	    op->bhistory = ((op->bhistory << 1) | uint(f_outcome)) ;
	} /* end if (bpyags_magic) */
	return rs ;
}
/* end subroutine (bpyags_update) */

int bpyags_zerostats(bpyags *op) noex {
	int		rs ;
	if ((rs = bpyags_magic(op)) >= 0) {
	    op->s = {} ;
	} /* end if (bpyags_magic) */
	return rs ;
}
/* end subroutine (bpyags_zerostats) */

int bpyags_getstats(bpyags *op,bpyags_st *rp) noex {
    	int		rs ;
	int		bitstotal = 0 ; /* return-value */;
	if ((rs = bpyags_magic(op,rp)) >= 0) {
	    /* calculate the bits for this predictor */
	    {
		uint	bits_cpht ;
		uint	bits_dpht ;
		uint	bits_caentry ;
		uint	bits_history ;
		bits_history = flbsi(op->calen) ;
		bits_cpht = op->chlen * BPYAGS_COUNTBITS ;
		bits_caentry = (BPYAGS_TAGBITS + 1 + BPYAGS_COUNTBITS) ;
		bits_dpht = 2 * op->calen * bits_caentry ;
		bitstotal = bits_cpht + bits_dpht + bits_history ;
	    }
	    /* fill in the extra stuff */
	    if (rp) {
		*rp = op->s ;
	        rp->cpht = op->chlen ;
	        rp->dpht = op->calen ;
	        rp->bits = bitstotal ;
	    }
	} /* end if (bpyags_magic) */
	return (rs >= 0) ? bitstotal : rs ;
}
/* end subroutine (bpyags_getstats) */


/* private subroutines */

/* cache lookup */
local int calu(bpyags_ca *cp,uint ci,uint tag,uint *rp) noex {
	int		rs = SR_NOTFOUND ;
	if (cp[ci].tag0 == tag) {
	    rs = 0 ;
	    *rp = cp[ci].counter0 ;
	} else if (cp[ci].tag1 == tag) {
	    rs = 1 ;
	    *rp = cp[ci].counter1 ;
	}
	return rs ;
}
/* end subroutine (calu) */

local int caup(bpyags_ca *cp,uint ci,uint tag,int f_outcome,int f_type) noex {
    	constexpr uint	tmask = ((1 << (BPYAGS_TAGBITS + 1)) - 1) ; 
	int		rs = SR_NOTFOUND ;
	uint		count, ncount ;
	uint		tag0, tag1 ;
	uint		p0, p1 ;
	tag0 = cp[ci].tag0 ;
	tag1 = cp[ci].tag1 ;
	if (tag0 == tag) {
	    rs = 0 ;
	    count = cp[ci].counter0 ;
	} else if (tag1 == tag) {
	    rs = 1 ;
	    count = cp[ci].counter1 ;
	}
	/* cache update */
	if (rs >= 0) {
	    /* it was a hit */
	    ncount = satcount(count,BPYAGS_COUNTBITS,f_outcome) ;
	    if (rs == 0) {
	        cp[ci].counter0 = uint(ncount & 0x03) ;
	    } else {
	        cp[ci].counter1 = uint(ncount & 0x03) ;
	    }
	} else {
	    /* it was a miss */
	    if (f_type) {
	        p0 = GETPRED(cp[ci].counter0) ;
	        p1 = GETPRED(cp[ci].counter1) ;
	        if (! LEQUIV(p0,p1)) {
	            if (! p0) {
	                cp[ci].tag0 = (tag & tmask) ;
	                cp[ci].counter0 = uint(f_outcome & 1) ;
	            } else {
	                cp[ci].tag1 = (tag & tmask) ;
	                cp[ci].counter1 = uint(f_outcome & 1) ;
	            }
	        } else {
	            if (cp[ci].lru) {
	                cp[ci].tag0 = (tag & tmask) ;
	                cp[ci].counter0 = uint(f_outcome & 1) ;
	            } else {
	                cp[ci].tag1 = (tag & tmask) ;
	                cp[ci].counter1 = uint(f_outcome & 1) ;
	            }
	        } /* end if */
	    } else {
	        if (cp[ci].lru) {
	            cp[ci].tag0 = (tag & tmask) ;
	            cp[ci].counter0 = uint(f_outcome & 1) ;
	        } else {
	            cp[ci].tag1 = (tag & tmask) ;
	            cp[ci].counter1 = uint(f_outcome & 1) ;
	        }
	    } /* end if */
	} /* end if */
	return rs ;
}
/* end subroutine (caup) */


