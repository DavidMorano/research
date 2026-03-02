/* bpgskew SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* this is a BPGSKEW branch predictor */
/* version %I% last-modified %G% */

#define	CF_ALLONES	0		/* initialize META to ones */
#define	CF_ALLMIDDLE	1
#define	CF_MUSTAGREE	0		/* predictor must agree w/ outcome */

/* revision history:

	= 2000-03-15, David A­D­ Morano
	This object module was created for Levo research.  It is a
	value predictor.  This is not coded as hardware.  It is
	like Atom analysis subroutines!

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	Object:
	gskew

	Description:
	This object module implements the GSKEW (2Bc-gskew) branch
	predictor.  This is also known as the "2Bc-gskew-pskew"
	predictor.  This was very famously used in the Alpha-21464
	(EV8) processor.

	Synopsis:
	int bpgskew_start(bpgskew *op,int p1,int p2,int p3,int p4) noex

	Arguments:
	op	object pointer
	p1	table length
	p2	number of history bits
	p3
	p4

	Returns:
	>=0	OK
	<0	error-code (system-return)

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
#include	"bpgskew.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */

#define	BPGSKEW_DEFLEN		(64 * 1024)
#define	BPGSKEW_STATES		4
#define	BPGSKEW_DEFGLEN		4		/* default entries */
#define	BPGSKEW_DEFHIST		15		/* default history bits */
#ifdef	COMMENT
#define	BPGSKEW_DEFHIST		9		/* default history bits */
#define	BPGSKEW_DEFHIST		11		/* default history bits */
#endif /* COMMENT */

#define	GETPRED(c)	!!(((c) >> 1) & 1)
#define	GETPRED2(c)	!!(((c) >> 1) & 1)
#define	GETPRED3(c)	!!(((c) >> 2) & 1)

#define	BIT(w,n)	(((w) >> (n)) & 1)


/* imported namespaces */

using std::cast_saturate ;		/* subroutine */
using levomod::flbsi ;			/* subroutine */
using libuc::mem ;			/* variable */


/* local typedefs */

typedef	bpgskew_ba	table_t ;


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int bpgskew_magic(bpgskew *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == BPGSKEW_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (bpgskew_magic) */

local uint	h(int,uint) noex ;
local uint	hinv(int,uint) noex ;
local uint	fi_bim(int,uint,uint) noex ;
local uint	fi_g0(int,uint,uint) noex ;
local uint	fi_g1(int,uint,uint) noex ;
local uint	fi_meta(int,uint,uint) noex ;


/* local variables */

cbool		f_allmiddle	= CF_ALLMIDDLE ;
cbool		f_allones	= CF_ALLONES ;
cbool		f_mustagree	= CF_MUSTAGREE ;


/* exported variables */

const levomod_obj	bpgskew_mod = {
	"bpgskew",
	szof(bpgskew)
} ;


/* exported subroutines */

int bpgskew_start(bpgskew *op,int p1,int p2,int p3,int p4) noex {
	int		rs = SR_FAULT ;
	(void) p2 ;
	(void) p3 ;
	if (op) {
	    int		sz ;
	    int		mnum = p1 ;
	    memclear(op) ;
	    if (mnum < 0) mnum = BPGSKEW_DEFLEN ;
	    {
		cint npt = nextpowtwo(mnum) ;
	        op->tlen = uint(npt) ;
	    }
	    op->tmask = (op->tlen - 1) ;
	    op->n = flbsi(op->tlen) ;
	    if (p4 < 0) p4 = BPGSKEW_DEFHIST ;
	    op->nhist = p4 ;
	    op->hmask = ((1 << op->nhist) - 1) ;
	    /* allocate the space */
	    sz = op->tlen * szof(table_t) ;
	    if (void *p ; (rs = mem.call(1,sz,&p)) >= 0) {
	        op->table = resumelife<table_t>(p) ;
	        if_constexpr (f_allmiddle) {
		    cint n = int(op->tlen) ;
	            for (int i = 0 ; i < n ; i += 1) {
	                op->table[i].bim = 1 ;
	                op->table[i].g0 = 1 ;
	                op->table[i].g1 = 1 ;
	                op->table[i].meta = 1 ;
	            } /* end for */
	        } /* end if_constexpr (f_allmiddle) */
	        if_constexpr (f_allones) {
		    cint n = int(op->tlen) ;
	            for (int i = 0 ; i < n ; i += 1) {
	                op->table[i].meta = 3 ;
	            } /* end for */
	        } /* end if_constexpr (f_allones) */
	        /* we are out of here */
	        op->magval = BPGSKEW_MAGIC ;
	    } /* end if (memory-allocation) */
	} /* end if (non-null) */
	return rs ;
}
/* end subroutine (bpgskew_start) */

int bpgskew_finish(bpgskew *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = bpgskew_magic(op)) >= 0) {
	    if (op->table) {
	        rs1 =mem.free(op->table) ;
	        if (rs >= 0) rs = rs1 ;
		op->table = nullptr ;
	    }
	    op->magval = 0 ;
	} /* end if (bpgskew_magic) */
	return rs ;
}
/* end subroutine (bpgskew_free) */

int bpgskew_lookup(bpgskew *op,uint ia) noex {
    	int		rs ;
	int		fpred = 0 ; /* return-value */
	if ((rs = bpgskew_magic(op)) >= 0) {
	    uint	a ;
	    uint	v1, v2 ;
	    int		ibim, ig0, ig1, imeta ;
	    int		f_meta ;
	    bool	f_bim, f_g0, f_g1 ;
	    {
	        ulong	v ;
	        op->s.lu += 1 ;
	        a = ia >> 2 ;
	        v = ulong(a) ;
	        v = (v << op->nhist) ;
	        v = (v | ulong(op->bhistory & op->hmask)) ;
	        v1 = uintconv(v & op->tmask) ;
	        v2 = uintconv((v >> op->n) & op->tmask) ;
	    }
	    {
	        imeta = fi_meta(op->n,v1,v2) ;
	        ibim = fi_bim(op->n,op->bhistory,a) & op->tmask ;
	    }
	    {
	        f_meta = GETPRED(op->table[imeta].meta) ;
	        f_bim = GETPRED(op->table[ibim].bim) ;
	    }
	    /* BIM will be "UP", ESKEW will be "DOWN" */
	    if (f_meta) {
	        op->s.use_bim += 1 ;
	        fpred = f_bim ;
	    } else {
	        int	vote ;
	        op->s.use_eskew += 1 ;
	        ig0 = fi_g0(op->n,v1,v2) ;
	        ig1 = fi_g1(op->n,v1,v2) ;
	        f_g0 = GETPRED(op->table[ig0].g0) ;
	        f_g1 = GETPRED(op->table[ig1].g1) ;
	        vote = int(f_bim) + int(f_g0) + int(f_g1) ;
	        fpred = (vote >= 2) ;
	    } /* end if */
	} /* end if (bpgskew_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bpgskew_lookup) */

int bpgskew_confidence(bpgskew *op,uint ia) noex {
    	int		rs ;
	int		rv = 0 ; /* return-value */
	if ((rs = bpgskew_magic(op)) >= 0) {
	    uint	a ;
	    uint	v1, v2 ;
	    uint	c ;
	    int		ibim, ig0, ig1, imeta ;
	    bool	f_meta ;
	    {
	        ulong	v ;
	        a = ia >> 2 ;
	        v = ulong(a) ;
	        v = (v << op->nhist) ;
	        v = v | ulong(op->bhistory & op->hmask) ;
	        v1 = uintconv(v & op->tmask) ;
	        v2 = uintconv((v >> op->n) & op->tmask) ;
	    }
	    {
	        imeta = fi_meta(op->n,v1,v2) ;
	        ibim = fi_bim(op->n,op->bhistory,a) & op->tmask ;
	        f_meta = GETPRED(op->table[imeta].meta) ;
	        c = op->table[ibim].bim ;
	    }
	    /* BIM will be "UP", ESKEW will be "DOWN" */
	    if (f_meta) {
	        op->s.use_bim += 1 ;
	    } else {
	        uint	sum ;
	        uint	vote ;
	        op->s.use_eskew += 1 ;
	        ig0 = fi_g0(op->n,v1,v2) ;
	        ig1 = fi_g1(op->n,v1,v2) ;
	        vote = GETPRED(c) ;
	        vote = vote | (GETPRED(op->table[ig0].g0) << 1) ;
	        vote = vote | (GETPRED(op->table[ig1].g1) << 2) ;
	        switch (vote) {
	        case 0:
	            c = 0 ;
	            break ;
	        case 1:
	            sum = op->table[ig1].g1 + op->table[ig0].g0 ;
	            c = sum / 2 ;
	            break ;
	        case 2:
	            sum = op->table[ig1].g1 + c ;
	            c = sum / 2 ;
	            break ;
	        case 3:
	            sum = op->table[ig0].g0 + c ;
	            c = sum / 2 ;
	            break ;
	        case 4:
	            sum = op->table[ig0].g0 + c ;
	            c = sum / 2 ;
	            break ;
	        case 5:
	            sum = op->table[ig1].g1 + c ;
	            c = sum / 2 ;
	            break ;
	        case 6:
	            sum = op->table[ig1].g1 + op->table[ig0].g0 ;
	            c = sum / 2 ;
	            break ;
	        case 7:
	            sum = op->table[ig1].g1 + op->table[ig0].g0 + c ;
	            c = sum / 3 ;
	            break ;
	        } /* end switch */
	    } /* end if (which predictor) */
	    rv = (c << 1) ;
	} /* end if (bpgskew_magic) */
	return (rs >= 0) ? rv : rs ;
}
/* end subroutine (bpgskew_confidence) */

int bpgskew_update(bpgskew *op,uint ia,int f_outcome) noex {
    	constexpr int	nstates = BPGSKEW_STATES ;
	constexpr uint	bmask = uint((1 << 2) - 1) ; /* Bank-Mask */
    	int		rs ;
	int		fpred = 0 ; /* return-value */
	if ((rs = bpgskew_magic(op)) >= 0) {
	    uint	nc ;
	    uint	a ;
	    uint	v1, v2 ;
	    int		ibim, ig0, ig1, imeta ;
	    int		vote ;
	    bool	f_meta ;
	    bool	f_bim, f_eskew, f_g0, f_g1 ;
	    bool	f_bimagree ;
	    {
	        ulong	v ;
	        a = ia >> 2 ;
	        v = a ;
	        v = (v << op->nhist) ;
	        v = (v | (op->bhistory & op->hmask)) ;
	        v1 = uintconv(v & op->tmask) ;
	        v2 = uintconv((v >> op->n) & op->tmask) ;
	    }
	    {
	        imeta = fi_meta(op->n,v1,v2) ;
	        ibim = fi_bim(op->n,op->bhistory,a) & op->tmask ;
	        ig0 = fi_g0(op->n,v1,v2) ;
	        ig1 = fi_g1(op->n,v1,v2) ;
	    }
	    {
	        f_meta = GETPRED(op->table[imeta].meta) ;
	        f_bim = GETPRED(op->table[ibim].bim) ;
	        f_g0 = GETPRED(op->table[ig0].g0) ;
	        f_g1 = GETPRED(op->table[ig1].g1) ;
	        vote = int(f_bim) + int(f_g0) + int(f_g1) ;
	        f_eskew = (vote >= 2) ;
	        /* BIM will be "UP", ESKEW will be "DOWN" */
	        fpred = (f_meta) ? f_bim : f_eskew ;
	    }
	    /* do the updating */
	    if (! LEQUIV(f_outcome,fpred)) {
                /* incorrect prediction */
	        op->s.update_all += 1 ;
	        nc = satcount(op->table[ibim].bim,nstates,f_outcome) ;
	        op->table[ibim].bim = (nc & bmask) ;
	        nc = satcount(op->table[ig0].g0,nstates,f_outcome) ;
	    	op->table[ig0].g0 = (nc & bmask) ;
	    	nc = satcount(op->table[ig1].g1,nstates,f_outcome) ;
	    	op->table[ig1].g1 = (nc & bmask) ;
	    } else {
	        /* correct prediction */
	        if (f_meta) {
	            bool f ;
	            op->s.update_bim += 1 ;
		    if_constexpr (f_mustagree) {
	        	f = LEQUIV(f_bim,f_outcome) ;
		    } else {
	        	f = true ;
		    } /* end if_constexpr (f_mustagree) */
	            if (f) {
	                nc = satcount(op->table[ibim].bim,nstates,f_outcome) ;
	                op->table[ibim].bim = (nc & bmask) ;
	            }
	        } else {
		    /* strengthen correct tables */
	            op->s.update_eskew += 1 ;
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
	        }
	    } /* end if (prediction/misprediction) */
	    /* update the META predictor */
	    if (! LEQUIV(f_bim,f_eskew)) {
	        op->s.update_meta += 1 ;
	        f_bimagree = LEQUIV(f_bim,f_outcome) ;
	        if (f_bimagree) {
	            op->s.updateup_meta += 1 ;
	        }
	        nc = satcount(op->table[imeta].meta,nstates,f_bimagree) ;
	        op->table[imeta].meta = (nc & bmask) ;
	    } /* end if (updated META) */
	    /* update global branch history register */
	    op->bhistory = ((op->bhistory << 1) | uint(f_outcome)) ;
	} /* end if (bpgskew_magic) */
	return (rs >= 0) ? fpred : rs ;
}
/* end subroutine (bpgskew_update) */

int bpgskew_zerostats(bpgskew *op) noex {
	int		rs ;
	if ((rs = bpgskew_magic(op)) >= 0) {
	    op->s = {} ;
	} /* end if (bpgskew_magic) */
	return rs ;
}
/* end subroutine (bpgskew_zerostats) */

int bpgskew_getstats(bpgskew *op,bpgskew_st *rp) noex {
	int		rs ;
	int		bitstotal = 0 ; /* return-value */
	if ((rs = bpgskew_magic(op)) >= 0) {
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
	        bits_history = op->nhist ;
	        bitstotal += bits_bim ;
	        bitstotal += bits_g0 ;
	        bitstotal += bits_g1 ;
	        bitstotal += bits_meta ;
	        bitstotal += bits_history ;
	    } /* end block */
	    /* fill in the extra stuff */
	    if (rp) {
	        *rp = op->s ;
	        rp->tlen = op->tlen ;
	        rp->bits = bitstotal ;
	    }
	} /* end if (bpgskew_magic) */
	return (rs >= 0) ? bitstotal : rs ;
}
/* end subroutine (bpgskew_getstats) */


/* private subroutines */

/* index function for the BIM */
local uint fi_bim(int n,uint h,uint a) noex {
    	(void) n ;
	(void) h ;
	return a ;
}
/* end subroutine (fi_bim) */

local uint fi_g0(int n,uint v1,uint v2) noex {
	return h(n,v1) ^ hinv(n,v2) ^ v1 ;
}
/* end subroutine (fi_g0) */

local uint fi_g1(int n,uint v1,uint v2) noex {
	return hinv(n,v1) ^ h(n,v2) ^ v2 ;
}
/* end subroutine (fi_g1) */

local uint fi_meta(int n,uint v1,uint v2) noex {
	return h(n,v1) ^ hinv(n,v2) ^ v2 ;
}
/* end subroutine (fi_meta) */

/* forward H function */
local uint h(int n,uint v) noex {
	return (v >> 1) | ((BIT(v,(n - 1)) ^ (v & 1)) << (n - 1)) ;
}
/* end subroutine (h) */

/* inverse H function */
local uint hinv(int n,uint v) noex {
	return (v << 1) | (BIT(v,(n - 1)) ^ BIT(v,(n - 2))) ;
}
/* end subroutine (hinv) */


