/* ssfetch */

/* fetch unit */


#define	CF_DEBUG	0		/* switchable debugging */
#define	CF_SAFE		1		/* safe mode */
#define	CF_SAFE2	1		/* safe mode */


/* revision history:

	= 00/02/04, Dave Morano

	Module was originally written for the LEVO simulator LEVOSIM.


	= 04/07/21, Dave Morano

	I grabbed this code from another object and will turn it
	into the fetch unit.


	= 06/12/11, Dave Morano

	Rather than holding a copy of an IS that was fetched we now
	hold a pointer to the IS that already exists in the CHECKER!
	I do not see why we need to copy the IS since it is already in
	the CHECKER (I hope there was no need to copy it).


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/**************************************************************************

	This module provides the functions for the fetching of
	instructions.


**************************************************************************/


#define	SSFETCH_MASTER	0


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"ss.h"
#include	"xmlinfo.h"

#include	"ssinfo.h"
#include	"sscommon.h"
#include	"instrclass.h"
#include	"ssas.h"
#include	"ssfetch.h"
#include	"regs.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	SSFETCH_MAGIC		0x09089827
#define	SSFETCH_NMIN		1



/* external subroutines */


/* forward references */

static int	ssfetch_freeup(SSFETCH *) ;
static int	ssfetch_startops(SSFETCH *,int) ;
static int	ssfetch_decrement(SSFETCH *,struct proginfo *) ;
static int	ssfetch_combs(SSFETCH *,struct proginfo *,struct ssinfo *) ;


/* local variables */






/* initialize this SSFETCH object */
int ssfetch_init(op,pip,mip,lip,ap)
SSFETCH		*op ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
SSFETCH_INITARGS	*ap ;
{
	int	rs = SR_OK, i ;
	int	n, size ;
	int	npred ;
	int	maxspan ;


	if (op == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(SSFETCH)) ;

	op->magic = 0 ;

#ifdef	OPTIONAL
	(void) memset(&op->f,0,sizeof(struct ssfetch_flags)) ;
#endif

	op->pip = pip ;
	op->mip = mip ;
	op->lip = lip ;

	if (ap != NULL) {

	    op->fqsize = ap->nstages ;	/* number of Q entries */

	}

	if (op->fqsize < SSFETCH_NMIN)
	    op->fqsize = SSFETCH_NMIN ;

/* allocate the Q slots */

	size = 2 * op->fqsize * sizeof(SSAS **) ;
	rs = uc_malloc(size,&op->a.cas) ;

	if (rs < 0)
	    goto bad0 ;

	memset(op->a.cas,0,size) ;

	op->n.cas = op->a.cas + 0 ;
	op->c.cas = op->a.cas + op->fqsize ;

/* allocate the tracking structures */

	size = 2 * op->fqsize * sizeof(struct ssfetch_track) ;
	rs = uc_malloc(size,&op->a.t) ;

	if (rs < 0)
	    goto bad1 ;

	memset(op->a.t,0,size) ;

	op->n.t = op->a.t + 0 ;
	op->c.t = op->a.t + op->fqsize ;

/* zero all machine state */

#ifdef	OPTIONAL
	(void) memset(&op->c,0,sizeof(struct ssfetch_state)) ;

	(void) memset(&op->n,0,sizeof(struct ssfetch_state)) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(1))
	    eprintf("ssfetch_init: ret rs=%d fqsize=%d\n",
	        rs,op->nfqsize) ;
#endif

	op->magic = SSFETCH_MAGIC ;

ret0:
	return (rs >= 0) ? op->fqsize : rs ;

/* bad stuff */
bad1:
bad0:
	ssfetch_freeup(op) ;

	goto ret0 ;
}
/* end subroutine (ssfetch_init) */


/* free up the object */
int ssfetch_free(op)
SSFETCH		*op ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_free: SSFETCH(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

	ssfetch_freeup(op) ;

	return rs ;
}
/* end subroutine (ssfetch_free) */


/* perform our combinatorial logic */
int ssfetch_comb(op,phase)
SSFETCH		*op ;
int		phase ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	ULONG	clock ;

	int	rs = SR_OK, rs1, i ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_comb: SSFETCH(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	mip = op->mip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssfetch_comb: entered ck=%llu:%u \n",
	        mip->ck,phase) ;
#endif

/* do our own combinatorial work */

	switch (phase) {

	case 0:

#ifdef	COMMENT
	    ssfetch_startops(op,0) ;
#endif

	    break ;

/* all operands are exhanged in this phase (in zero time !) */
	case 1:
	    break ;

	case 2:
	    break ;

	case 3:
	    break ;

	case 4:
	    break ;

	case 5:
	    rs = ssfetch_decrement(op,pip) ;

#ifdef	COMMENT
	    if (rs >= 0)
	    	rs = ssfetch_combs(op,pip,lip) ;
#endif

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (ssfetch_comb) */


/* preform a clocked state transition */
int ssfetch_clock(op)
SSFETCH		*op ;
{
	struct proginfo	*pip ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_clock: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

/* transition ourself */

/* careful here, swap the pointers */

	{
	    struct ssfetch_track	*c_t ;

	    SSAS			**c_cas ;

	    int		size ;


/* copy everything but pointers (restore them) */

	    c_t = op->c.t ;
	    c_cas = op->c.cas ;

	    op->c = op->n ;

	    op->c.t = c_t ;
	    op->c.cas = c_cas ;

/* copy the stuff that was in the pointers */

	    size = op->fqsize * sizeof(SSAS **) ;
	    memcpy(op->c.cas,op->n.cas,size) ;

	    size = op->fqsize * sizeof(struct ssfetch_track) ;
	    memcpy(op->c.t,op->n.t,size) ;

	} /* end block */

/* execute all subobject clock methods */

	return SR_OK ;
}
/* end subroutine (ssfetch_clock) */


/* how many fetch slots are currently available (empty) */
int ssfetch_nfetch(op)
SSFETCH		*op ;
{
	int	rs = SR_OK, n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_nfetch: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	n = op->fqsize - op->c.nactive ;

	if (n < 0)
		rs = SR_BADFMT ;

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (ssfetch_nfetch) */


#ifdef	COMMENT

/* are we ready to fetch something (some slots available?) */
int ssfetch_readyfetch(op)
SSFETCH		*op ;
{
	int	rs = SR_OK, n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_readyfetch: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	n = op->fqsize - op->c.nactive ;
	if (n < 0)
	    rs = SR_BADFMT ;

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (ssfetch_readyfetch) */

#endif /* COMMENT */


/* do the fetch to the next state (write entry into fetch queue) */
int ssfetch_fetch(op,ncasp,lat)
SSFETCH		*op ;
SSAS		*ncasp ;
int		lat ;
{
	struct ssfetch_track	*tp ;

	struct proginfo	*pip ;

	SSAS	*casp ;

	int	rs = SR_OK ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_fetch: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

	f = ((op->fqsize - op->n.nactive) > 0) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssfetch_fetch: f=%d\n",f) ;
#endif

	if (f) {

	    op->n.cas[op->n.wi] = ncasp ;

	    tp = op->n.t + op->n.wi ;
	    tp->waitcount = MAX(lat,0) ;
	    tp->f_fetched = (tp->waitcount == 0) ;

/* update state */

	    op->n.wi = (op->n.wi + 1) % op->fqsize ;
	    op->n.nactive += 1 ;

	} else
	    rs = SR_DOM ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssfetch_fetch: ret rs=%d nactive=%d\n",
	        f,op->n.nactive) ;
#endif

	return (rs >= 0) ? op->n.nactive : rs ;
}
/* end subroutine (ssfetch_fetch) */


/* get some information */
int ssfetch_info(op,ip)
SSFETCH		*op ;
SSFETCH_INFO	*ip ;
{
	struct ssfetch_track	*btp, *tp ;

	struct proginfo	*pip ;

	int	rs = SR_OK, n ;
	int	fqsize ;
	int	ri ;
	int	i = 0 ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_ndispatch: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

	if (ip != NULL)
		memset(ip,0,sizeof(SSFETCH_INFO)) ;

	n = op->c.nactive ;

#if	CF_SAFE2
	if (n < 0) {
		rs = SR_BADFMT ;
		goto ret0 ;
	}
#endif

#if	CF_DEBUG && CF_MASTERDEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssfetch_ndispatch: n=%d\n",n) ;
#endif

	fqsize = op->fqsize ;
	btp = op->c.t ;
	ri = op->c.ri ;
	for (i = 0 ; i < n ; i += 1) {

		tp = btp + ri ;
	        f = (tp->waitcount == 0) ;
	        if (! f)
	            break ;

	        ri = (ri + 1) % fqsize ;

	} /* end for */

	if (ip != NULL) {

	    ip->n = n ;
	    ip->ndispatch = i ;
	    ip->waitcount = btp[ri].waitcount ;

	}

ret0:
	return (rs >= 0) ? i : rs ;
}
/* end subroutine (ssfetch_info) */


/* return the consecutive number that are ready to be dispatched */
int ssfetch_ndispatch(op)
SSFETCH		*op ;
{
	struct ssfetch_track	*btp, *tp ;

	struct proginfo	*pip ;

	int	rs = SR_OK, n ;
	int	fqsize ;
	int	ri ;
	int	i = 0 ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_ndispatch: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

	n = op->c.nactive ;

#if	CF_DEBUG && CF_MASTERDEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssfetch_ndispatch: n=%d\n",n) ;
#endif

#if	CF_SAFE2
	if (n < 0) {
		rs = SR_BADFMT ;
		goto ret0 ;
	}
#endif

	fqsize = op->fqsize ;
	btp = op->c.t ;
	ri = op->c.ri ;
	for (i = 0 ; i < n ; i += 1) {

	    tp = btp + ri ;
	        f = (tp->waitcount == 0) ;
	        if (! f)
	            break ;

	    ri = (ri + 1) % fqsize ;

	} /* end for */

ret0:

#if	CF_DEBUG && CF_MASTERDEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssfetch_ndispatch: ret rs=%d i=%d\n",rs,i) ;
#endif

	return (rs >= 0) ? i : rs ;
}
/* end subroutine (ssfetch_ndispatch) */


/* dispatch an instruction and turn the window */
int ssfetch_dispatch(op,i,caspp)
SSFETCH		*op ;
int		i ;
SSAS		**caspp ;
{
	struct ssfetch_track	*tp ;

	int	rs = SR_OK, ri ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_dispatch: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_SAFE2
	if (caspp != NULL)
	    *caspp = NULL ;
#endif

	if (i < 0)
		return SR_INVALID ;

	if (i >= op->c.nactive) {
		rs = SR_DOM ;
		goto ret0 ;
	}

	ri = (op->c.ri + i) % op->fqsize ;

	*caspp = op->c.cas[ri] ;

ret0:
	return (rs >= 0) ? ri : rs ;
}
/* end subroutine (ssfetch_dispatch) */


ssfetch_dispatchu(op,n)
SSFETCH		*op ;
int		n ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSFETCH_MAGIC) && (op->magic != 0)) {

	    eprintf("ssfetch_dispatch: SSFETCH(%08lx) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (n > op->n.nactive)
		return SR_DOM ;

	op->n.ri = (op->n.ri + n) % op->fqsize ;
	op->n.nactive -= n ;
	return SR_OK ;
}
/* end subroutine (ssfetch_dispatchu) */



/* PRIVATE SUBROUTINES */



/* free up the object */
int ssfetch_freeup(op)
SSFETCH		*op ;
{


	if (op->a.t != NULL)
	    free(op->a.t) ;

	if (op->a.cas != NULL)
	    free(op->a.cas) ;

	op->a.t = NULL ;
	op->a.cas = NULL ;
	op->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (ssfetch_freeup) */


/* initialize next-state that is associated with the operands */
static int ssfetch_startops(op,wset)
SSFETCH		*op ;
int		wset ;
{
	int	rs = SR_OK ;


	return rs ;
}
/* end subroutine (ssfetch_startops) */


#ifdef	COMMENT

/* try to load fetch buffer slots */
static int ssfetch_load(op,pip,lip)
SSFETCH		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	md_inst_t	inst ;

	enum md_opcode	op ;

	int	rs, n, i ;
	int	lat ;
	int	f_pred = TRUE ;


	n = op->fqsize - op->c.nactive ;
	if (n > lip->fwidth)
	    n = lip->fwidth ;


	for (i = 0 ; i < n ; i += 1) {

/* fetch an instruction at the next predicted fetch address */
	    fetch_regs_PC = fetch_pred_PC ;


/* is this a bogus text address? (can happen on mis-spec path) */
	    if (ld_text_base <= fetch_regs_PC
	        && fetch_regs_PC < (ld_text_base+ld_text_size)
	        && !(fetch_regs_PC & (sizeof(md_inst_t)-1)))
	    {
/* read instruction from memory */
	        MD_FETCH_INST(inst, mem, fetch_regs_PC) ;

/* address is within program text, read instruction from memory */
	        lat = cache_il1_lat ;
	        if (cache_il1)
	        {
/* access the I-cache */
	            lat =
	                cache_access(cache_il1, Read, IACOMPRESS(fetch_regs_PC),
	                NULL, ISCOMPRESS(sizeof(md_inst_t)), sim_cycle,
	                NULL, NULL) ;
	            if (lat > cache_il1_lat)
	                last_inst_missed = TRUE ;
	        }

#ifdef	COMMENT

	        if (f_itlb) {

/* access the I-TLB, NOTE: this code will initiate
						 speculative TLB misses */
	            tlb_lat =
	                cache_access(itlb, Read, IACOMPRESS(fetch_regs_PC),
	                NULL, ISCOMPRESS(sizeof(md_inst_t)), sim_cycle,
	                NULL, NULL) ;
	            if (tlb_lat > 1)
	                last_inst_tmissed = TRUE ;

/* I-cache/I-TLB accesses occur in parallel */
	            lat = MAX(tlb_lat, lat) ;
	        }

/* I-cache/I-TLB miss? assumes I-cache hit >= I-TLB hit */
	        if (lat != cache_il1_lat)
	        {
/* I-cache miss, block fetch until it is resolved */
	            ruu_fetch_issue_delay += lat - 1 ;
	            break ;
	        }

#endif /* COMMENT */

/* else, I-cache/I-TLB hit */

	    } else {

/* fetch PC is bogus, send a NOP down the pipeline */
	        inst = MD_NOP_INST ;
	    }

/* have a valid inst, here */

/* pre-decode instruction */
	    MD_SET_OPCODE(op, inst) ;

/* possibly use the BTB target */
	    if (f_pred)
	    {
/* get the next predicted fetch address; only use branch predictor
				     result for branches (assumes pre-decode bits); NOTE: returned
				     value may be 1 if bpred can only predict a direction */
	        if (MD_OP_FLAGS(op) & F_CTRL) {
	            fetch_pred_PC =
	                bpred_lookup(pred,
	                /* branch address */fetch_regs_PC,
	                /* target address *//* FIXME: not computed */0,
	                /* opcode */op,
	                /* call? */MD_IS_CALL(op),
	                /* return? */MD_IS_RETURN(op),
	                /* updt */&(fetch_data[fetch_tail].dir_update),
	                /* RSB index */&stack_recover_idx) ;
	        } else
	            fetch_pred_PC = 0 ;

/* valid address returned from branch predictor? */
	        if (!fetch_pred_PC)
	        {
/* no predicted taken target, attempt not taken target */
	            fetch_pred_PC = fetch_regs_PC + sizeof(md_inst_t) ;
	        }
	        else
	        {
/* go with target, NOTE: discontinuous fetch, so terminate */
	            branch_cnt++ ;
	            if (branch_cnt >= fetch_speed)
	                done = TRUE ;
	        }
	    }
	    else
	    {
/* no predictor, just default to predict not taken, and
				     continue fetching instructions linearly */
	        fetch_pred_PC = fetch_regs_PC + sizeof(md_inst_t) ;
	    }

/* commit this instruction to the IFETCH -> DISPATCH queue */
	    fetch_data[fetch_tail].IR = inst ;
	    fetch_data[fetch_tail].regs_PC = fetch_regs_PC ;
	    fetch_data[fetch_tail].pred_PC = fetch_pred_PC ;
	    fetch_data[fetch_tail].stack_recover_idx = stack_recover_idx ;
	    fetch_data[fetch_tail].ptrace_seq = ptrace_seq++ ;

	    last_inst_missed = FALSE ;
	    last_inst_tmissed = FALSE ;

/* adjust instruction fetch queue */
	    fetch_tail = (fetch_tail + 1) & (ruu_ifq_size - 1) ;
	    fetch_num++ ;

	} /* end for */

	return 0 ;
}
/* end subroutine (ssfetch_load) */

#endif /* COMMENT */


static int ssfetch_decrement(op,pip)
SSFETCH		*op ;
struct proginfo	*pip ;
{
	struct ssfetch_track	*btp, *tp ;

	int	i ;
	int	ri ;
	int	nactive ;
	int	fqsize ;


/* scan all used slots to check for non-zero count and decrement */

	fqsize = op->fqsize ;
	btp = op->n.t ;
	ri = op->n.ri ;
	nactive = op->n.nactive ;
	for (i = 0 ; i < nactive ; i += 1) {

	    tp = btp + ri ;
	    if (tp->waitcount > 0)
	        tp->waitcount -= 1 ;

	    tp->f_fetched = (tp->waitcount == 0) ;
	    ri = (ri + 1) % fqsize ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssfetch_decrement) */


#ifdef	COMMENT

static int ssfetch_combs(op,pip,lip)
SSFETCH		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{



	return SR_OK ;
}
/* end subroutine (ssfetch_combs) */

#endif /* COMMENT */



