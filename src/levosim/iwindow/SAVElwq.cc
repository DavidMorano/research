/* lwq */

/* Levo Write Queue (for storing writes at commit time) */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		1		/* switchable debug print-outs */
#define	F_FORCE1000	1		/* force 100 for depth ? */
#define	F_RESETTT	0		/* plan for reset TTs */


/* revision history :

	= 00/07/14, Alireza Khalafi

	This code was started basically from scratch !


	= 01/01/24, Dave Morano

	I added the machine "shift" code to this object.


	= 01/02/09, Dave Morano

	I added some code to perform a sanity check on the LBUSINT
	objects that are subobjects (instantiated) within this object.


	= 01/06/13, Dave Morano

	I added code to properly check for readiness for commitment.


	= 01/06/22, Dave morano

	Alireza fixed some bug that had to do with the necessary
	and proper snooping of previous committed writes.


	= 01/06/27, Dave Morano

	I fixed a small bug that was an uninitialized variable.
	It was not causing any real problem because it was also not
	being used in a meaningful way !

	
*/



/**************************************************************************

	This object module provides the functions for storing writes
	that occur from ASes at commit time.


**************************************************************************/


#define	LWQ_MASTER	0


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>
#include	<assert.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"levoinfo.h"
#include	"lwq.h"
#include        "ldecodeinstrclass.h"
#include        "lexec.h"
#include	"rfbus.h"
#include	"las.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif


/* local defines */

#define	LWQ_MAGIC	0x84352346



/* external subroutines */

extern int	getnumbuses(uint) ;


/* forward references */

static int	lwq_handleshift(LWQ *,struct proginfo *,struct levoinfo *) ;






/* initialize this object */
int 
lwq_init(lwqp,pip,mip,lip, ap)
LWQ             *lwqp ;
struct proginfo *pip ;
LSIM	 	*mip ;
struct levoinfo *lip ;
LWQ_INITARGS	*ap ;
{
	int	rs ;
	int	i ;
	int	size ;


	if (lwqp == NULL)
	    return SR_FAULT ;

	lwqp->magic = 0 ;

	lwqp->pip = pip ;
	lwqp->mip = mip ;
	lwqp->lip = lip ;

/* save the pointers to the buses that go to the memory subsystem */

	lwqp->memrequest = ap->memrequest ;	/* load request buses */
	lwqp->memresponse = ap->memresponse ;	/* load response buses */
	lwqp->memwrite = ap->memwrite ;		/* write buses to memory */

	lwqp->wmfinter = ap->wmfinter ;
	lwqp->wmbinter = ap->wmbinter ;

	lwqp->norows = lip->totalrows ;
	lwqp->nmb = lip->nmembuses ;

/* precalculate the number of execution window buses */

	lwqp->nwmfbuses = getnumbuses(ap->wmfinter) ;

	lwqp->nwmbbuses = getnumbuses(ap->wmbinter) ;


	lwqp->f.shift = FALSE ;

#if	F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("lwq_init: norows=%d\n",lwqp->norows) ;
#endif

/* allocate some stuff */

	size = lwqp->norows * sizeof(LSQ) ;
	rs = uc_malloc(size,(void **) &lwqp->lsqes) ;

	assert(rs > 0) ;

	(void) memset(lwqp->lsqes,0,size) ;

	size = lwqp->norows * sizeof(struct lwq_entry) ;
	rs = uc_malloc(size,(void **) &lwqp->c.as_fifo) ;
	assert(rs > 0) ;

	rs = uc_malloc(size,(void **) &lwqp->n.as_fifo) ;
	assert(rs > 0) ;


/* initialize the wait queue */

#if	F_FORCE1000
	lwqp->wq.qsize = 1000 ;
#else
	lwqp->wq.qsize = ap->wqsize ;
#endif

	size = lwqp->wq.qsize * sizeof(struct lwq_wqentry) ;
	rs = uc_malloc(size,(void **) &lwqp->wq.en) ;
	assert(rs > 0) ;

	lwqp->wq.ql = 0 ;
	lwqp->wq.qs = 0 ;
	lwqp->wq.qr = 0 ;
	lwqp->wq.f_qempty = 1 ;
	lwqp->wq.f_qfull = 0 ;
	for(i=0; i< lwqp->wq.qsize ; i += 1) {

	    lwqp->wq.en[i].full = 0 ;
	    lwqp->wq.en[i].serv = 0 ;
	    lwqp->wq.en[i].ready = 0 ;
	}

/* initialize the LBUSINT objects */

	size = lwqp->nwmbbuses * sizeof(LBUSINT) ;
	rs = uc_malloc(size,(void **) &lwqp->mbwbus) ;

	assert(rs > 0) ;

	size = lwqp->nwmfbuses * sizeof(LBUSINT) ;
	rs = uc_malloc(size,(void **) &lwqp->mfwbus) ;

	assert(rs > 0) ;


	lsq_init(&lwqp->lsq,pip,mip,lip,lwqp->wq.qsize) ;

	for (i=0; i<lwqp->norows; ++i) {

	    lsq_init(lwqp->lsqes+i,pip,mip,lip,lwqp->wq.qsize) ;

	    lwqp->c.as_fifo[i].valid = lwqp->n.as_fifo[i].valid = 0 ;

	} /* end for */

/* initializing busintgroup for memery backwarding buses */

#if	F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("lwq_init: MBBes\n") ;
#endif

	for (i = 0 ; i < lwqp->nwmbbuses ; i += 1) {

	    rs = lbusint_init(lwqp->mbwbus+i,pip,lip,
	        ap->mbwbus+i,ap->busid,MBWB,i) ;

#if	F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("lwq_init: LBUSINT(%d)=%08lx rs=%d\n",
	            i,(lwqp->mbwbus + i),rs) ;
#endif

	    assert (rs >= 0) ;

	} /* end for */

/* initializing busintgroup for memery forwarding buses */

#if	F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("lwq_init: MFBes\n") ;
#endif

	for (i = 0 ; i < lwqp->nwmfbuses ; i += 1) {

	    rs = lbusint_init(lwqp->mfwbus+i,pip,lip,
	        ap->mfwbus+i,ap->busid,MFWB,i) ;

#if	F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("lwq_init: LBUSINT(%d)=%08lx rs=%d\n",
	            i,(lwqp->mfwbus + i),rs) ;
#endif

	    assert (rs >= 0) ;

	} /* end for */

	lwqp->magic = LWQ_MAGIC ;
	return SR_OK ;
}
/* end subroutine (lwq_init) */


/* free up this object */
int lwq_free(lwqp)
LWQ	*lwqp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lwqp->pip ;

/* free the individual FIFOs */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: array_of lsq_free()\n") ;
#endif

	for (i = 0 ; i < lwqp->norows ; i += 1) {

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: i=%d lsq_free()\n",i) ;
#endif

	    rs = lsq_free(&lwqp->lsqes[i]) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: i=%d lsq_free() rs=%d\n",i,rs) ;
#endif

	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: extra_single lsq_free()\n",i) ;
#endif

	rs = lsq_free(&lwqp->lsq) ;
	
#if	F_DEBUG
	if ((pip->debuglevel > 3) && (rs < 0))
	eprintf("lwq_free: lsq_free() rs=%d\n",rs) ;
#endif


#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: MFBes\n") ;
#endif

	for (i = 0 ; i < lwqp->nwmfbuses ; i += 1) {

	    rs = lbusint_free(lwqp->mfwbus + i) ;

#if	F_DEBUG
	if ((pip->debuglevel > 3) && (rs < 0))
	eprintf("lwq_free: MFB i=%d lbusint_free() rs=%d\n",i,rs) ;
#endif

	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: MBBes\n") ;
#endif

	for (i = 0 ; i < lwqp->nwmbbuses ; i += 1) {

	    rs = lbusint_free(lwqp->mbwbus + i) ;

#if	F_DEBUG
	if ((pip->debuglevel > 3) && (rs < 0))
	eprintf("lwq_free: MBB i=%d lbusint_free() rs=%d\n",i,rs) ;
#endif

	} /* end for */

/* free up the memory */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: freeing memory\n") ;
#endif

	free(lwqp->mfwbus) ;

	free(lwqp->mbwbus) ;

	free(lwqp->wq.en) ;

	free(lwqp->n.as_fifo) ;

	free(lwqp->c.as_fifo) ;

	free(lwqp->lsqes) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_free: exiting rs=%d\n",rs) ;
#endif

	lwqp->magic = 0 ;
	return rs ;
}
/* end subroutine (lwq_free) */


/* do our combinatorial stuff */
int 
lwq_comb(lwqp,phase)
LWQ	*lwqp ;
int	phase ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	struct lwq_entry  *ep ;

	int	rs = SR_OK ;
	int	tt1;
	int	row,i ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lwqp->pip ;
	lip = lwqp->lip ;

/* pop our subobjects */

/* bus interfaces */

	for (i = 0 ; i < lwqp->nwmbbuses ; i += 1) {

	    rs = lbusint_comb(lwqp->mbwbus + i, phase) ;

	}

	for (i = 0 ; i < lwqp->nwmfbuses ; i += 1) {

	    rs = lbusint_comb(lwqp->mfwbus + i, phase) ;

	} /* end for */

/* FIFO */

	for (row=0;row<lwqp->norows;++row)
	    lsq_comb(&lwqp->lsqes[row],phase) ;

	rs = lsq_comb(&lwqp->lsq,phase) ;


/* do us ! */

	switch (phase) {

	case 1:

	    lwq_snoop_mbwb(lwqp) ;

	    lwq_service_next_req(lwqp) ;

	    lwq_fw_mfwb(lwqp) ;

	    break ;

	case 2:
/* update FIFOes with data from ASes */
/* make sure that data with lower tt is written first */
	    tt1 = -100000;
	    for (row=0; row<lwqp->norows; ++row) {

	        ep = &lwqp->c.as_fifo[row] ;
	        if(ep->valid == 1) {
		    /* insert in the store_queue */
	      /* lsq_insert(&lwqp->lsqes[row],ep->addr,ep->data,ep->tt) ; */
		    assert(tt1 < ep->tt);
if(0) 
	lsq_insert(&lwqp->lsq,ep->addr,ep->data,ep->tt,ep->dp);
else
	if(lsim_writeint(lwqp->mip,ep->addr, ep->data,ep->dp) != SR_OK) {
		/* speculative memory access */
		printf("WQ: unsuccessful Write to memory(0x%x),dp=0x%x, data=0x%x\n",ep->addr,ep->dp,ep->data);

	}
	else {
		printf("WQ: wrote to  memory(0x%x),dp=0x%x, data=0x%x\n",ep->addr,ep->dp,ep->data);
	}
		    lwqp->n.as_fifo[row].valid = 0;
		    tt1 = ep->tt;
	        }

	    } /* end for */

	    break ;

	case 3:
	    rs = lwq_handleshift(lwqp,pip,lip) ;

	    break ;

	} /* end switch */


	return rs ;
}
/* end subroutine (lwq_comb) */


/* we just got clocked */
int 
lwq_clock(lwqp)
LWQ	*lwqp ;
{
	struct proginfo	*pip ;
	struct lwq_entry *p ;

	int	rs, i ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lwqp->pip ;

/*	p = lwqp->c.as_fifo;
		lwqp->c = lwqp->n; 
		lwqp->c.as_fifo = p;
	*/

	for(i=0;i<lwqp->norows;++i) {
	    lwqp->c.as_fifo[i] = lwqp->n.as_fifo[i] ;
	}

/* execute all subobject clock methods */

	for(i=0;i<lwqp->norows;++i)
	    lsq_clock(&lwqp->lsqes[i]) ;

	lsq_clock(&lwqp->lsq) ;

	for (i = 0 ; i < lwqp->nwmbbuses ; i += 1) {

	    rs = lbusint_clock(lwqp->mbwbus + i) ;

	}

	for (i = 0 ; i < lwqp->nwmfbuses ; i += 1) {

	    rs = lbusint_clock(lwqp->mfwbus + i) ;

	}

	return rs ;
}
/* end subroutine (lwq_clock) */


/* take a request for a machine shift */
int 
lwq_shift(lwqp)
LWQ	*lwqp ;
{
	int	rs = SR_OK, i ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	lwqp->f.shift = TRUE ;

/* do our subobjects */

/* do the LSQ stuff */

	for (i = 0 ; i < lwqp->norows ; i += 1)
	    lsq_shift(lwqp->lsqes + i) ;

	rs = lsq_shift(&lwqp->lsq) ;

/* do the LBUSINTs that we have */

	for (i = 0 ; i < lwqp->nwmbbuses ; i += 1) {

	    rs = lbusint_shift(lwqp->mbwbus + i) ;

	}

	for (i = 0 ; i < lwqp->nwmfbuses ; i += 1) {

	    rs = lbusint_shift(lwqp->mfwbus + i) ;

	} /* end for */

/* we're done (for now !) */

	return rs ;
}
/* end subroutine (lwq_shift) */


/* Use this function to insert entries into the store queue */
int 
lwq_store(lwqp, addr, data, tt, row, dp)
LWQ	*lwqp ;
uint addr;	/* address */
int data;	/* data */
int tt;		/* time tag */
int row;	/* Row index of the AS */
int dp;		/* data present flags */
{
	struct lwq_entry	*ep ;
	int	i ;
	int	noe ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	if (row < 0 || row > lwqp->norows)
	    return SR_INVALID ;

	lwqp->n.as_fifo[row].valid = 1 ;
	lwqp->n.as_fifo[row].addr = addr ;
	lwqp->n.as_fifo[row].data = data ;
	lwqp->n.as_fifo[row].tt = tt ;
	lwqp->n.as_fifo[row].dp = dp ;

	return SR_OK ;
}
/* end subroutine (lwq_load) */


/* does a lookup in all the internal store queues and returns the latest value
   Returns the index of the lsq that has the lastes value, -1 if couldn't find a match */

int 
lwq_lookup(lwqp, addr, datap, ttp)
LWQ	*lwqp ;
uint	addr ;
int	*datap ;
int	*ttp ;
{
	int data,i,j ;
	int mtt = -1000000 ;
	int ret = -1 ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	for (i = lwqp->norows-1; i>=0 ; --i ) {
		if(lwqp->c.as_fifo[i].valid) {
			if(lwqp->c.as_fifo[i].addr == addr) {
				*datap = lwqp->c.as_fifo[i].data;
				return 1;
			}
		}
			
	}
	ret = lsq_lookup(&lwqp->lsq,addr,datap,ttp) ;
	if(ret < 0)
		return -1;

	return ret ;
}
/* end subroutine (lwq_loopup) */


/* removes the entries from the store queues that match the address */
int
lwq_remove(lwqp, addr)
LWQ	*lwqp ;
uint	addr ;
{
	int	i ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	lsq_remove(&lwqp->lsq,addr) ;

#ifdef	COMMENT
	for(i=0; i<lwqp->norows; ++i) {
	    lsq_remove(&lwqp->lsqes[i],addr) ;
	}
#endif /* COMMENT */

	return SR_OK ;
}
/* end subroutine (lwq_remove) */


/* are we ready to take on committed writes in the rows designated ? */
int lwq_readycommit(lwqp,i)
LWQ	*lwqp ;
int	i ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, j ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lwqp->pip ;

	if ((i < -1) || (i > lwqp->norows))
		return SR_INVALID ;

	if (i == -1) {

		rs = lsq_readywrite(&lwqp->lsq) ;

		if (rs >= 1) {

	    for (j = 0 ; j < lwqp->nwmbbuses ; j += 1) {

		    rs = lbusint_done(lwqp->mbwbus+ j);

			if (rs < 0)
				break ;

		    if (rs == 0) 
			return 0;

		} /* end for */

	    for (j = 0 ; j < lwqp->nwmfbuses ; j += 1) {

		    rs = lbusint_done(lwqp->mfwbus+ j);

			if (rs < 0)
				break ;

		    if (rs == 0) 
			return 0;

	    } /* end for */

		} /* end if */

	} else {

#if	F_DEBUG
		if (pip->debuglevel > 3) 
		eprintf("lwq_readycommit: Q=%d \n",i) ;
#endif

		rs = lsq_readywrite(lwqp->lsqes + i) ;

#if	F_DEBUG
		if ((pip->debuglevel > 3) && (rs <= 0))
		eprintf("lwq_readycommit: Q=%d not ready rs=%d\n",i,rs) ;
#endif

	} /* end if */

	return rs ;			/* return YES ! */
}
/* end subroutine (lwq_readycommit) */


/* check them */
int lwq_check(lwqp)
LWQ	*lwqp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;
	int	i ;


	if (lwqp == NULL)
		return SR_FAULT ;

	if (lwqp->magic != LWQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lwqp->pip ;

	for (i = 0 ; i < lwqp->nwmbbuses ; i += 1) {

	    rs = lbusint_check(lwqp->mbwbus + i) ;

#if	F_DEBUG
	if ((pip->debuglevel > 3) && (rs < 0))
	eprintf("lwq_check: MFB lbusint_comb() rs=%d\n",rs) ;
#endif

		if (rs < 0)
			break ;

	} /* end for */

	for (i = 0 ; i < lwqp->nwmfbuses ; i += 1) {

	    rs = lbusint_check(lwqp->mfwbus + i) ;

#if	F_DEBUG
	if ((pip->debuglevel > 3) && (rs < 0))
	eprintf("lwq_check: MFB lbusint_comb() rs=%d\n",rs) ;
#endif

		if (rs < 0)
			break ;

	} /* end for */

	return rs ;
}
/* end subroutine (lwq_check) */



/* PRIVATE SUBROUTINES */



int
lwq_snoop_mbwb(lwqp)
LWQ	*lwqp ;
{
	struct proginfo	*pip ;

	struct lwq_waitqueue *wqp ;

	LFLOWGROUP	rd ;

	LBUSINT		*busintp ;

	int 		intl ;


	pip = lwqp->pip ;

	for (intl=0; intl < lwqp->nwmbbuses ; intl += 1) {

	    busintp = lwqp->mbwbus + intl ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lwq_snoop_mbwb: lbusint_read() LBUSINT=%08lx\n",
		busintp) ;
#endif

	    if (lbusint_read(busintp, &rd) > 0) {

	        wqp = &lwqp->wq ;
	        if(wqp->f_qfull)
	            assert(0) ;
	        else {
	            printf("snooped at WQ: Mem[0x%x],tt=%d, dv=%08x\n",
				rd.addr,rd.tt,rd.dv) ;

	            assert(wqp->en[wqp->qs].full == 0) ;
	            wqp->en[wqp->qs].d = rd ;
	            wqp->en[wqp->qs].full = 1 ;
	            wqp->en[wqp->qs].ready = 0 ;
	            wqp->en[wqp->qs].serv = 0 ;
	            wqp->en[wqp->qs].index = wqp->qs ;
	            wqp->f_qempty = 0 ;
	            if(++(wqp->qs) == wqp->qsize)
	                wqp->qs = 0 ;
	            if(wqp->en[wqp->qs].full)
	                wqp->f_qfull = 1 ;
	        }
	    }
	}

	return SR_OK ;
}


/* read the next entry from wait queue and either satisfy the 
request through store queues or send a request to the memory.
If request satisfied, sets the valid flag. */

int
lwq_service_next_req(lwqp)
LWQ		*lwqp ;
{
	int 		i,data,tt,rs ;
	LFLOWGROUP	*rdp ;
	struct lwq_waitqueue *wqp ;

	wqp = &lwqp->wq ;
	if(wqp->f_qempty)
	    return SR_OK ;

	for(i=0;i<LWQ_MAXSERVICABLE; ++i) {
	    if(wqp->en[wqp->ql].full == 0) {
	        wqp->f_qempty = 1 ;
	        return SR_OK ;
	    }
	    rdp = &wqp->en[wqp->ql].d ;
	    rs = lwq_lookup(lwqp, rdp->addr, &data, &tt) ;

	    if(rs != -1) {
	        rdp->dv = data ;
	        wqp->en[wqp->ql].ready = 1 ;
	    }
	    else {
/* send req to mem */
	        if(lsim_readint(lwqp->mip,rdp->addr, &rdp->dv) != SR_OK)
			rdp->dv == 1000; /* What to do with wrong addresses ?!!*/
	        wqp->en[wqp->ql].ready = 1 ;
	    }
	    wqp->en[wqp->ql].serv = 1 ;
	    if(++wqp->ql == wqp->qsize)
	        wqp->ql = 0 ;
	}



}


int
lwq_fw_mfwb(lwqp)
LWQ		*lwqp ;
{
	int		intl ;
	int		mi ;
	int		ret ;
	LFLOWGROUP	*wrp ;
	struct lwq_waitqueue *wqp ;

	wqp = &lwqp->wq ;
	do {

	    if (wqp -> en[wqp->qr].ready) {

	        wrp = &wqp -> en[wqp->qr].d ;

#if	F_RESETTT
	        wrp -> tt = -10110111 ;
	        wrp -> Prtt = -10110111 ;
	        wrp -> Prtt = -1;
#endif

	        wrp -> tt = -1;
	        wrp -> trans = LFLOWGROUP_TSTORE ;
	        wrp -> dp = LFLOWGROUP_DPALL ;

	        mi = getinterleave(lwqp->lip->mfinter,wrp->addr) ;

	        ret = lbusint_write(lwqp->mfwbus + mi,lwqp->buspriority,wrp) ;

	        if(ret >= 0) {
	            printf("Successful sending data on "
				"MFWB at WQ:Mem(0x%x)=0x%x\n",wrp->addr,wrp->dv) ;
	            wqp->en[wqp->qr].full = 0 ;
	            wqp->en[wqp->qr].serv = 0 ;
	            wqp->en[wqp->qr].ready = 0 ;
	            if(++wqp->qr == wqp->qsize)
	                wqp->qr = 0 ;
	        }
	        else {
	            printf("Unsuccessful sending data on MFWB at WQ :"
			"Mem(0x%x)=0x%x\n",
			wrp->addr,wrp->dv) ;
	        }

	    } else {
	        ret = -1 ;
	    }

	} while(ret >= 0) ;

	return SR_OK ;
}
/* end subroutine (lwq_fw_mfwb) */


int
lwq_print(LWQ * lwqp) {

	int i ;


	printf("Main Store Queue:\n") ;
	lsq_print(lwqp->lsq) ;

/*
	for(i=0; i < lwqp->norows; ++i) {
	    printf("Store Queue[%d]:\n",i) ;
	    lsq_print(lwqp->lsqes[i]) ;
	}
*/

	return 0 ;
}

int
lwq_print_wq(LWQ *lwqp) {

	int i ;

	printf("Wait Queue:\n") ;
	printf("ql=%d, qr=%d, qs=%d\n",lwqp->wq.ql,lwqp->wq.qr,lwqp->wq.qs) ;
	for(i=0; i<lwqp->wq.qsize; ++i) {

	    if(lwqp->wq.en[i].full) {
	        printf("en[%d]:Mem(%d)=%d, serv=%d,ready=%d\n",
	            i,lwqp->wq.en[i].d.addr,
	            lwqp->wq.en[i].d.dv,
	            lwqp->wq.en[i].serv,
	            lwqp->wq.en[i].ready) ;
	    }
	}
}


/* perform a machine shift if one was called for */
static int 
lwq_handleshift(lwqp,pip,lip)
LWQ		*lwqp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK, i ;
	int	totalrows ;


	if (! lwqp->f.shift)
		return rs ;

	    lwqp->f.shift = FALSE ;

/* "do" us */

	totalrows = lip->totalrows ;
	for (i = 0 ; i < lwqp->norows ; i += 1) {

	    lwqp->n.as_fifo[i].tt -= totalrows ;

	} /* end for */


	return SR_OK ;
}
/* end subroutine (lwq_handleshift) */



