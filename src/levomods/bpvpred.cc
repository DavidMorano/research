/* bpvpred SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* value prediction object */
/* version %I% last-modified %G% */

#define	CF_VOTEREPLACE	0		/* replace by voting among counters */
#define	CF_COUNTREPLACE	1		/* replace by counting all counters */

/* revision history:

	= 2000-03-15, David A­D­ Morano
	This object module was created for Levo research.  It is a
	value predictor.  This is not coded as hardware. It is like
	Atom analysis subroutines!

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

  	Object:
	bpvpred

	Description:
	This object module implements a value predictor.  It is not
	coded as if it was real hardware (like LevoSim for example).
	It is coded like an analysis subroutine for Atom.

*****************************************************************************/

#include	<envstandards.h>	/* MUST be ordered first to configure */
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>		/* |getenv(3c)| */
#include	<numeric>		/* |cast_saturate(3c++)| */
#include	<algorithm>		/* |min(3c++)| + |max(3c++)| */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<ucmem.h>		/* |mem(3uc)| */
#include	<nextpowtwo.h>
#include	<satcount.h>
#include	<localmisc.h>

#include	"levomod.h"
#include	"bpvpred.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */

#define	BPVPRED_DEFN		4	/* default entries */
#define	BPVPRED_REPE		3	/* entry threshold (n-ops) */
#define	BPVPRED_REPO		2	/* operand threshold (counter) */
#define	BPVPRED_REPS		1	/* stride threshold (counter) */

#ifndef	CF_VOTEREPLACE
#define	CF_VOTEREPLACE		0	/* replace by voting among counters */
#endif
#ifndef	CF_COUNTREPLACE
#define	CF_COUNTREPLACE		1	/* replace by counting all counters */
#endif


/* imported namespaces */

using std::cast_saturate ;		/* subroutine */
using std::min ;			/* subroutine */
using std::max ;			/* subroutine */
using levomod::flbsi ;			/* subroutine */
using libuc::mem ;			/* variable */


/* local typedefs */

typedef bpvpred_ent	table_t ;


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int bpvpred_magic(bpvpred *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == BPVPRED_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (bpvpred_magic) */


/* local variables */

cbool		f_voter		= CF_VOTEREPLACE ;
cbool		f_counter	= CF_COUNTREPLACE ;


/* exported variables */

const levomod_obj	bpvpred_mod = {
    	"bpvpred",
	szof(bpvpred)
} ;


/* exported subroutines */

int bpvpred_start(bpvpred *op,int nentry,int nops,int sbits) noex {
	int		rs = SR_FAULT ;
	if (op) {
	    int		sz ;
	    memclear(op) ;
	    if (nentry <= 0) nentry = BPVPRED_DEFN ;
	    op->tablen = nextpowtwo(nentry) ;
	    sz = op->tablen * szof(bpvpred_ent) ;
	    if (void *p ; (rs = mem.call(1,sz,&p)) >= 0) {
		op->table = resumelife<bpvpred_ent>(p) ;
		/* calculate how much to shift the IA (right) */
		cint n = flbsi(op->tablen) ;
		op->tagshift = 2 + n ;
		/* number of operands to predict */
		if (nops <= 0) nops = 1 ;
		if (nops > BPVPRED_NOPS) nops = BPVPRED_NOPS ;
		op->nops = nops ;
		/* calculate the stride mask */
		op->stridemask = ((1u << uint(sbits)) - 1u) ;
		/* how many different counts (states) are there? */
		op->ncount = (1U << BPVPRED_COUNTBITS) ;
		/* load some initial statistic data */
		op->s.tablen = op->tablen ;
		/* we are out of here */
		op->magval = BPVPRED_MAGIC ;
		if (rs < 0) {
		    mem.free(op->table) ;
		    op->table = nullptr ;
		}
	    } /* end if (memory-allocation) */
	} /* end if (bpvpred_magic) */
	return rs ;
}
/* end subroutine (bpvpred_start) */

int bpvpred_finish(bpvpred *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = bpvpred_magic(op)) >= 0) {
	    if (op->table) {
	        rs1 = mem.free(op->table) ;
	        if (rs >= 0) rs = rs1 ;
	        op->table = nullptr ;
	    }
	    op->magval = 0 ;
	} /* end if (bpvpred_magic) */
	return rs ;
}
/* end subroutine (bpvpred_free) */

int bpvpred_lookup(bpvpred *op,uint ia,uint *values,int n) noex {
	int		rs ;
	int		mops = 0 ; /* return-values */
	if ((rs = bpvpred_magic(op,values)) >= 0) {
	    uint	ti ;
	    uint	tag ;
	    ti = (ia >> 2) % op->tablen ;
	    if (n > BPVPRED_NOPS) n = BPVPRED_NOPS ;
	    mops = min(n,op->nops) ;
	    op->s.in_lu += 1 ;
	    op->s.op_lu += mops ;
	    /* start searching! */
	    tag = ia >> op->tagshift ;
	    if (tag == op->table[ti].tag) {
	        /* we got a hit */
	        op->s.in_hit += 1 ;
	        op->s.op_hit += mops ;
	        for (int i = 0 ; i < mops ; i += 1) {
		    uint sum = 0 ;
		    sum += op->table[ti].ops[i].last ;
		    sum += op->table[ti].ops[i].stride ;
	            values[i] = sum ;
	        } /* end for */
	        for (int i ; i < n ; i += 1) {
		    values[i] = 0 ;
	        }
	    } else {
	        /* we got a miss */
	        cint sz = n * szof(uint) ;
	        memclear(values,sz) ;
	    }
	} /* end if (bpvpred_magic) */
	return (rs >= 0) ? mops : rs ;
}
/* end subroutine (bpvpred_lookup) */

namespace {
    struct updater {
	bpvpred		*op ;		/* argument */
	bpvpred_oper	*ops ;
	uint		*values ;	/* argument */
	uint		ia ;		/* argument */
	uint		ti ;
	uint		tag ;
	int		n ;		/* argument */
	int		sz ;
	int		mops ;
	bool		f_miss = false ;
	bool		f_same ;
	updater(bpvpred *o,uint *v,uint ªia,int ªn) noex : op(o), values(v) {
	    ia = ªia ;
	    n = ªn ;
	} ; /* end ctor */
	operator int () noex ;
	int verarg() noex ;
	int prep() noex ;
	int miss() noex ;
	int upd() noex ;
    } ; /* end struct (updater) */
    typedef int (updater::*updater_m)() noex ;
} /* end namespace */

constexpr updater_m	mems[] = {
	&updater::verarg,
	&updater::prep,
	&updater::upd
} ; /* end array (mems) */

int bpvpred_update(bpvpred *op,uint ia,uint *values,int n) noex {
	int		rs ;
	int		rn = 0 ; /* return-value */
	if ((rs = bpvpred_magic(op,values)) >= 0) {
	    if (updater uo(op,values,ia,n) ; (rs = uo) >= 0) {
	        rn = rs ;
	    }
	} /* end if (bpvpred_magic) */
	return (rs >= 0) ? rn : rs ;
}
/* end subroutine (bpvpred_update) */

updater::operator int () noex {
    	int		rs = SR_OK ;
	for (cauto &m : mems) {
	    rs = (this->*m)() ;
	    if (rs < 0) break ;
	} /* end for */
	return rs ;
} /* end method (updater::operator) */

int updater::verarg() noex {
    	int		rs = SR_NOENT ;
	if (n > BPVPRED_NOPS) n = BPVPRED_NOPS ;
	if (n > 0) {
	    rs = SR_OK ;
	}
	return rs ;
} /* end method ((updater::verarg) */

int updater::prep() noex {
    	int		rs = SR_OK ;
	{
	   ti = (ia >> 2) % op->tablen ;
	   ops = op->table[ti].ops ;
	}
	{
	   mops = min(n,op->nops) ;
	   op->s.in_up += 1 ;
	   op->s.op_up += mops ;
	}
	/* do we have a hit? */
	tag = ia >> op->tagshift ;
	if (tag != op->table[ti].tag) {
	    rs = miss() ;
	} /* end if (miss specific processing) */
	return rs ;
} /* end method (updater::prep) */

/* miss, should we replace? */
int updater::miss() noex {
    	int		rs = SR_OK ;
	bool		fdone = false ;
	f_miss = true ;
	if_constexpr (f_voter) {
	    if (! fdone) {
	        int c = 0 ;
	        for (int i = 0 ; i < mops ; i += 1) {
	            if (ops[i].counter > BPVPRED_REPO) {
	                c += 1 ;
		    }
	        } /* end for */
	        fdone = (c > BPVPRED_REPE) ;
	    } /* end if (not-done) */
	} /* end if_constexpr (f_voter) */
	if_constexpr (f_counter) {
	    if (! fdone) {
	        int c = 0 ;
	        for (int i = 0 ; i < mops ; i += 1) {
		    c += ops[i].counter ;
	        }
	        fdone = (c > (mops * BPVPRED_REPE)) ;
	    } /* end if (not-done) */
	} /* end if_constexpr (f_counter) */
	if (! fdone) {
	    /* replace! */
	    op->table[ti].replaces += 1 ;
	    sz = op->nops * szof(bpvpred_oper) ;
	    memclear(ops,sz) ;
	    /* load our tag into the entry */
	    op->table[ti].tag = tag ;
	    op->s.in_replace += 1 ;
	    op->s.op_replace += mops ;
	} /* end if (not-done) */
	return rs ;
} /* end method (updater::miss) */

int updater::upd() noex {
    	int		rs = SR_OK ;
	int i ;
	if (! f_miss) {
	    op->table[ti].hits += 1 ;
	    op->s.in_update += 1 ;
	    op->s.op_update += mops ;
	} /* end if (not-miss) */
	for (i = 0 ; i < mops ; i += 1) {
	    if (f_miss || (ops[i].counter <= BPVPRED_REPS)) {
	        ops[i].stride = (values[i] - ops[i].last) & op->stridemask ;
	    }
	    {
	        f_same = (ops[i].last == values[i]) ;
	        ops[i].last = values[i] ;
	    }
	    {
		cuint cmask = uint((1u << BPVPRED_COUNTBITS) - 1u) ;
	        cint c = ops[i].counter ;
		uint cnt = satcount(c,op->ncount,f_same) ;
	        ops[i].counter = uchar(cnt & cmask) ;
	    }
	} /* end for */
	for ( ; i < op->nops ; i += 1) {
	    ops[i].last = 0 ;
	    ops[i].stride = 0 ;
	    ops[i].counter = 0 ;
	} /* end for */
	return rs ;
} /* end method (updater::upd) */

int bpvpred_get(bpvpred *op,int ri,bpvpred_ent **rpp) noex {
    	int		rs ;
	if ((rs = bpvpred_magic(op,rpp)) >= 0) {
	    int		rlen = intconv(op->tablen) ;
	    rs = SR_NOTFOUND ;
	    if ((ri >= 0) && (ri < rlen)) {
		rs = SR_OK ;
	        *rpp = op->table + ri ;
	    } /* end if (valid) */
	} /* end if (bpvpred_magic) */
	return (rs >= 0) ? ri : rs ;
}
/* end subroutine (bpvpred_get) */

int bpvpred_zerostats(bpvpred *op) noex {
	int		rs ;
	if ((rs = bpvpred_magic(op)) >= 0) {
	    op->s = {} ;
	} /* end if (bpvpred_magic) */
	return rs ;
}
/* end subroutine (bpvpred_zerostats) */

int bpvpred_getstats(bpvpred *op,bpvpred_st *rp) noex {
	int		rs ;
	if ((rs = bpvpred_magic(op,rp)) >= 0) {
	    *rp = op->s ;
	} /* end if (bpvpred_magic) */
	return rs ;
}
/* end subroutine (bpvpred_getstats) */


/* private subroutines */


