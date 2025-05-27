/* ss */

/* Levo simulator state sequencing operations */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		1		/* switchable debugging */
#define	F_SAFE		1		/* play it safe ? */
#define	F_MACHINE	1		/* call the LEVO machine methods */
#define	F_MAXCLOCKS	1		/* maximum numbers of clocks */
#define	F_PROGINFOCHECK	1		/* do 'proginfo_check()' */
#define	F_TRACE		0		/* debug trace */


/* revision history:

	= 00/02/15, Dave Morano

	This code was started (for LevoSim).


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This is the object (roughly) for handling simulator specific
	operations such as call-backs.


******************************************************************************/


#define	SS_MASTER	1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>
#include <math.h>
#include <cstdio>

#include	<vsystem.h>
#include	<veclist.h>
#include	<vecitem.h>
#include	<paramfile.h>
#include	<hdb.h>
#include	<plainq.h>
#include	<bio.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"

#include	"ss.h"
#include	"ssinfo.h"
#include	"sscheckstate.h"
#include	"machstate.h"
#include	"ssdecode.h"
#include	"ssiw.h"
#include	"ssas.h"


/* local defines */

#define	SS_MAGIC		0x95827364
#define	SS_CLOCKINTERVAL	200

#ifndef	VBUFLEN
#define	VBUFLEN			100
#endif



/* external subroutines */

extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecui(const char *,int,int *) ;
extern int	cfdecull(const char *,int,ULONG *) ;
extern int	cfnumui(const char *,int,uint *) ;
extern int	cfnumull(const char *,int,ULONG *) ;


/* external variables */

extern struct proginfo	pi ;


/* local structures */


/* forward references */

static int	ss_callthem(SS *) ;

#if	F_DEBUG || F_DEBUGS
static int	ss_prpq(SS *,ULONG) ;
#endif

extern int	sim_exec(SS *,uint) ;
extern int	sim_release(SS *,uint) ;
extern int	sim_avail(SS *) ;

extern int	setregval(struct regs_t *,int,ULONG) ;
extern int	getregval(struct regs_t *,int,ULONG *) ;


/* local variables */

static const char	*lparams[] = {
	"sim:maxclocks",	/* maximum number of clocks */
	"sim:fastqueues",	/* number of fast queues */
	NULL
} ;

enum lparams {
	lparam_maxclocks,
	lparam_fastqueues,
	lparam_overlast
} ;







int ss_init(mip,pip,maxclocks,machobj)
SS		*mip ;
struct proginfo	*pip ;
ULONG		maxclocks ;
SS_MACHOBJ	*machobj ;
{
	PARAMFILE	*pfp = &pip->pf ;

	ULONG	ulw ;

	uint	uiw ;

	int	rs = SR_OK ;
	int	i, j ;
	int	vl ;
	int	size ;
	int	opts ;

	char	vbuf[VBUFLEN + 1] ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(5))
	    eprintf("ss_init: entered\n") ;
#endif

	(void) memset(mip,0,sizeof(SS)) ;

	mip->magic = 0 ;
	mip->pip = pip ;
	mip->nentries = 0 ;
	mip->nq = SS_NQUEUES ;
	mip->cqi = 0 ;
	mip->ck = 0 ;
	mip->ck_count = 0 ;

	mip->f.syscalls = FALSE ;

/* store what we have so far */

/* the program information */


/* the machine object information */

	mip->topobjp = NULL ;
	mip->topcombp = NULL ;
	mip->topclockp = NULL ;
	if (machobj != NULL) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	        eprintf("ss_init: got a machobj, LEVO=%08lx\n",
	            machobj->topobjp) ;
#endif

	    mip->topobjp = machobj->topobjp ;
	    mip->topcombp = machobj->topcombp ;
	    mip->topclockp = machobj->topclockp ;

	}

/* get some parameters */

	for (i = 0 ; lparams[i] != NULL ; i += 1) {

	    vl = paramfile_fetch(pfp,lparams[i],NULL,vbuf,VBUFLEN) ;

	    if (vl >= 0) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {
	            eprintf("ss_init: %i lparam=%s \n",i,lparams[i]) ;
	            eprintf("ss_init: %i val=%s\n",i,vbuf) ;
	        }
#endif

	        switch (i) {

	        case lparam_maxclocks:
	            rs = cfnumull(vbuf,vl,&ulw) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	                eprintf("ss_init: rs=%d maxclocks=%llu\n",
	                    rs,ulw) ;
#endif

	            if (rs >= 0) {

	                switch (i) {

	                case lparam_maxclocks:
				if (maxclocks == ((ULONG) -1))
	                        maxclocks = ulw ;

	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        case lparam_fastqueues:
	            rs = cfnumui(vbuf,vl,&uiw) ;

	            if (rs >= 0) {

	                switch (i) {

	                case lparam_fastqueues:
	                    mip->nq = uiw ;
	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        } /* end swtich */

	    } /* end if */

	} /* end for (getting simulation parameters) */

/* check and store any new parameters */

	if (maxclocks == ((ULONG) -1))
	    maxclocks = SS_MAXCLOCKS ;

	mip->maxclocks = maxclocks ;

	if (mip->nq < 5)
	    mip->nq = 5 ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ss_init: maxclock=%llu\n",mip->maxclocks) ;
	    eprintf("ss_init: fastqueues=%u\n",mip->nq) ;
	}
#endif /* F_DEBUG */

/* start initializing our scheduling apparatus */

	size = mip->nq * sizeof(veclist) ;
	rs = uc_malloc(size,&mip->qes) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("ss_init: Qs malloc failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(mip->qes,size,"ss_init:calloutqueues") ;
#endif

	opts = VECLIST_OSWAP ;
	for (i = 0 ; i < mip->nq ; i += 1) {

	    rs = veclist_init((mip->qes + i),0,opts) ;

#if	CF_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("ss_init: Qs veclist(%d) failed rs=%d\n",i,rs) ;
#endif

	    if (rs < 0)
	        break ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(5))
	        eprintf("ss_init: queue list(%d)=%08lx\n",(mip->qes + i)) ;
#endif

	} /* end for */

	    if (rs < 0)
	        goto bad1 ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(5)) {
	    eprintf("ss_init: ck=%llu \n",mip->ck) ;
	    eprintf("ss_init: 1 c0=%08x c1=%08x\n",
	        getarrayint(&mip->ck,0),
	        getarrayint(&mip->ck,1)) ;
	}
#endif /* F_DEBUG */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_init: plainq_init() FQ=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_init(&mip->fq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("ss_init: plainq failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_printf("plainq_init") ;
#endif

#if	F_DEBUG && 0
	if (DEBUGLEVEL(5)) {
	    eprintf("ss_init: ck=%llu \n",mip->ck) ;
	    eprintf("ss_init: 2 c0=%08x c1=%08x\n",
	        getarrayint(&mip->ck,0),
	        getarrayint(&mip->ck,1)) ;
	}
#endif /* F_DEBUG */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_init: hdb_init()\n") ;
#endif

	rs = hdb_init(&mip->pq,mip->nq,NULL,NULL) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("ss_init: PQ failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad3 ;

#ifdef	MALLOCLOG
	malloclog_printf("hdb_init") ;
#endif


/* supposedly, we are fairly well done ! (need to index SYSCALLs later) */

	mip->magic = SS_MAGIC ;


#if	F_DEBUG && 0
	if (pip->debuglevel >= 2) {
	    eprintf("ss_init: ck=%llu \n",mip->ck) ;
	    eprintf("ss_init: 3 c0=%08x c1=%08x\n",
	        getarrayint(&mip->ck,0),
	        getarrayint(&mip->ck,1)) ;
	    eprintf("ss_init: ret OK rs=%d\n",rs) ;
	}
#endif /* F_DEBUG */

	return rs ;

/* bad things */
bad5:

bad4:
	(void) hdb_free(&mip->pq) ;

bad3:
	(void) plainq_free(&mip->fq) ;

bad2:
bad1:
	for (j = 0 ; j < i ; j += 1)
	    (void) veclist_free(mip->qes + j) ;

	free(mip->qes) ;

bad0:

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("ss_init: ret BAD rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ss_init) */


/* set the machine object information */
int ss_setmach(mip,machobj)
SS		*mip ;
SS_MACHOBJ	*machobj ;
{
	struct proginfo	*pip ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SS_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(5))
	    eprintf("ss_setmach: entered\n") ;
#endif


	if (machobj->topobjp == NULL)
	    return SR_FAULT ;

	if (machobj->topcombp == NULL)
	    return SR_FAULT ;

	if (machobj->topclockp == NULL)
	    return SR_FAULT ;

/* the machine object information */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_setmach: got a machobj, LEVO=%08lx\n",
	        machobj->topobjp) ;
#endif

	mip->topobjp = machobj->topobjp ;
	mip->topcombp = machobj->topcombp ;
	mip->topclockp = machobj->topclockp ;

	return SR_OK ;
}
/* end subroutine (ss_setmach) */


/* free up whatever ! */
int ss_free(mip)
SS		*mip ;
{
	struct proginfo	*pip ;

	SS_COE	*ep ;

	veclist		*cqp ;

	int	i, j ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SS_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: entered\n") ;
#endif

/* free up any and all entries that are out there ! */

/* free all entries on the free queue */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: free Q entries\n") ;
#endif

	while (plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) != PLAINQ_RUNDER)
	    free(ep) ;

/* free all entries on the future pending queue */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: pending Q entries\n") ;
#endif

	{
	    HDB_DATUM	key, value ;

	    HDB_CURSOR	hcur ;


	    hdb_cursorinit(&mip->pq,&hcur) ;

	    while (hdb_enum(&mip->pq,&hcur,&key,&value) >= 0) {

	        if (value.buf != NULL)
	            free(value.buf) ;	/* the key was in there also ! */

	    } /* end while */

	    hdb_cursorfree(&mip->pq,&hcur) ;

	} /* end block (freeing pending queue) */

/* free all entries on the individual (fast) queues */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: fast Q entries\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    cqp = mip->qes + i ;
	    for (j = 0 ; veclist_get(cqp,j,&ep) >= 0 ; j += 1) {

	        if (ep == NULL) continue ;

	        free(ep) ;

	    } /* end while */

	} /* end for */


/* free up all queue objects */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: fast Qs\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    if ((mip->qes + i) != NULL)
	    	veclist_free(mip->qes + i) ;

	} /* end for */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: plain-Q free\n") ;
#endif

	(void) plainq_free(&mip->fq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: PQ free\n") ;
#endif

	(void) hdb_free(&mip->pq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: QES \n") ;
#endif

	if (mip->qes != NULL)
		free(mip->qes) ;

	mip->qes = NULL ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_free: ret\n") ;
#endif

	mip->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (ss_free) */


/* take call-back requests from other objects */
int ss_callout(mip,func,objp,argp,nclocks,phase)
SS	*mip ;
int		(*func)() ;
void		*objp, *argp ;
uint		nclocks ;
int		phase ;
{
	struct proginfo	*pip ;

	SS_COE	*ep ;

	int	rs = SR_OK ;
	int	i ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SS_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ss_callout: entered func=%08lx objp=%08lx argp=%08lx\n",
	        func,objp,argp) ;
	    eprintf("ss_callout: ck=%llu nclocks=%u\n",
	        mip->ck,nclocks) ;
	}
#endif /* F_DEBUG */

	if (nclocks == 0)
	    return SR_INVALID ;

/* get a free entry */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_callout: plainq_rem() FQa=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_callout: plainq_rem() rs=%d\n",rs) ;
#endif

	if (rs == PLAINQ_RUNDER) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	        eprintf("ss_callout: plainq_rem() underflow\n") ;
#endif

	    rs = uc_malloc(sizeof(SS_COE),(void *) &ep) ;

	    if (rs < 0)
	        return rs ;

#ifdef	MALLOCLOG
	    malloclog_alloc(ep,sizeof(SS_COE),"ss_callout:coe") ;
#endif

	    mip->nentries += 1 ;

	} /* end if */

/* fill it in */

	ep->wake = mip->ck + nclocks ;
	ep->relative = nclocks ;
	ep->phase = phase ;
	ep->func = func ;
	ep->objp = objp ;
	ep->argp = argp ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_callout: wake=%llu\n",ep->wake) ;
#endif

	if (nclocks < mip->nq) {

/* short time in the future */

	    i = (mip->cqi + nclocks) % mip->nq ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	        eprintf("ss_callout: short=%u wake=%llu\n",i,ep->wake) ;
#endif

	    rs = veclist_add(mip->qes + i,ep) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	        eprintf("ss_callout: veclist_add rs=%d\n",rs) ;
#endif

	} else {

	    HDB_DATUM	key, value ;


/* handle requests for longer time into the future */

	    ep->key = ep->wake / ((ULONG) mip->nq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	        eprintf("ss_callout: long future, key=%llu\n",
	            ep->key) ;
#endif

	    key.buf = (char *) &ep->key ;
	    key.len = sizeof(ULONG) ;

	    value.buf = ep ;
	    value.len = sizeof(SS_COE) ;

	    rs = hdb_store(&mip->pq,key,value) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	        eprintf("ss_callout: hdb_store rs=%d\n",rs) ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {

	        rs = ss_prpq(mip,ep->key) ;

	        eprintf("ss_callout: pending on key=%llu n=%d\n",
	            ep->key,rs) ;
	    }
#endif /* F_DEBUG */

	} /* end if */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_callout: ret rs=%d\n",
	        rs) ;
#endif

	return rs ;
}
/* end subroutine (ss_callout) */


/* do the main loop */
int ss_loop(mip,in,ck)
SS		*mip ;
ULONG		in, ck ;
{
	struct proginfo	*pip ;

	ULONG	clocklast = 0 ;

	int	rs, lrs = 0 ;
	int	f_exit = FALSE ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SS_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;
	if (mip->topobjp == NULL)
	    return SR_INVALID ;

	if (mip->topcombp == NULL)
	    return SR_INVALID ;

	if (mip->topclockp == NULL)
	    return SR_INVALID ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_loop: LEVO topobj=%p\n",
	        mip->topobjp) ;
#endif

/* set starting clock and instruction number */

	mip->ck = ck ;
	mip->ck_start = ck ;

	mip->in = in ;
	mip->in_start = in ;

/* we're good, let's go */

	while (! f_exit) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	            eprintf("ss_loop: ck=%llu\n",mip->ck) ;
#endif

/* call the stuff for combinatorial processing */

	    rs = ss_callthem(mip) ;

	    if ((rs != 0) && (! f_exit)) {

		lrs = rs ;
		f_exit = TRUE ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(3))
	            eprintf("ss_loop: ret from combinatorial rs=%d\n",
	                rs) ;
#endif

	    } /* end if (combinatorial exit) */

/* should we do any maintenance activities ? */

#if	F_PROGINFOCHECK
	    if ((rs >= 0) && ((mip->ck - clocklast) > SS_CLOCKINTERVAL)) {

	        clocklast = mip->ck ;
	        proginfo_check(pip,0,mip->ck,mip->in) ;

	    } /* end if (maintenance activities) */
#endif /* F_PROGINFOCHECK */


/* call the stuff to transition the clock */

	if (rs >= 0) {

#if	F_MACHINE

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	            eprintf("ss_loop: clock transition ck=%llu\n",
	                mip->ck) ;
#endif

	    rs = (*mip->topclockp)(mip->topobjp) ;

	    if ((rs != 0) && (! f_exit)) {
		lrs = rs ;
		f_exit = TRUE ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(3))
	            eprintf("ss_loop: ret from clock transition rs=%d\n",
	                rs) ;
#endif

	    }
#endif /* F_MACHINE */

	    mip->ck += 1 ;

	}

#if	F_MAXCLOCKS
	    if ((rs >= 0) &&
		(mip->maxclocks > 0) && (mip->ck >= mip->maxclocks)) {

		f_exit = TRUE ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(3))
	            eprintf("ss_loop: ret from maxclocks=%llu ck=%llu\n",
			mip->maxclocks,mip->ck) ;
#endif

	    }
#endif /* F_MAXCLOCKS */

	} /* end while */

	if (rs >= 0)
	    proginfo_check(pip,1,mip->ck,mip->in) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ss_loop: ret rs=%d lrs=%d\n",rs,lrs) ;
#endif

	return (rs >= 0) ? lrs : rs ;
}
/* end subroutine (ss_loop) */


/* return the clock number to the caller */
int ss_getclock(mip,lp)
SS		*mip ;
ULONG		*lp ;
{


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SS_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (lp == NULL)
	    return SR_FAULT ;

	*lp = mip->ck ;
	return SR_OK ;
}
/* end subroutine (ss_getclock) */


/* return the instruction number */
int ss_getinstr(mip,lp)
SS		*mip ;
ULONG		*lp ;
{


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SS_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (lp == NULL)
	    return SR_FAULT ;

	*lp = mip->in ;
	return SR_OK ;
}
/* end subroutine (ss_getinstr) */


#ifdef	COMMENT

/* expand the checker window */
int ss_expand(mip,n)
SS	*mip ;
int	n ;
{
	struct proginfo	*pip ;

	int	rs ;


	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_expand: entered\n") ;
#endif

	rs = sim_exec(mip,n) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_expand: sim_exec() rs=%d\n",rs) ;
#endif

#ifdef	COMMENT
	if ((rs >= 0) && (rs < n))
		f_exit = TRUE ;
#endif

	return rs ;
}
/* end subroutine (ss_expand) */


/* release (collapse) the checker window */
int ss_release(mip,n)
SS	*mip ;
int	n ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_release: entered\n") ;
#endif

	rs = sim_release(mip,n) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_release: sim_commit() rs=%d\n",rs) ;
#endif

#ifdef	COMMENT /* not used */
	if ((rs >= 0) && (rs < n))
		f_exit = TRUE ;
#endif

	return rs ;
}
/* end subroutine (ss_release) */

#endif /* COMMENT */



/* PRIVATE SUBROUTINES */



/* subroutine that is called on each clock */

/*

	Here we want to make the calls for combinatorial logic
	processing.

	The basic flow of this subroutine is :
	
		for each phase starting from zero
		loop for a minimum number of phases 
		and while there are user call-backs to call
			call the top object in the machine
			call any user call-back subroutines

		if we are done with the current fast queues
			grab entries for next block of call-backs
			and put them in the fast queues

*/

static int ss_callthem(mip)
SS		*mip ;
{
	struct proginfo	*pip ;

	veclist		*cqp ;

	SS_COE	*ep ;

	int	ph ;
	int	rs = 0, rs1, lrs, i, j ;
	int	n = 0 ;
	int	f_exit ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SS_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	    eprintf("ss_callthem: entering ck=%llu comb is LEVO=%08lx\n",
	        mip->ck,mip->topobjp) ;
#endif

/* loop through all of the phases */

	lrs = SR_OK ;
	f_exit = FALSE ;
	for (ph = 0 ; (rs >= 0) && (ph < SS_MINPHASES) ; ph += 1) {

	    mip->phase = ph ;

#if	F_MACHINE
	    rs = (*mip->topcombp)(mip->topobjp,ph) ;

	    if ((rs != 0) && (! f_exit)) {
		lrs = rs ;
	        f_exit = TRUE ;
	    }

		if (rs < 0)
			break ;

#else
	    f_exit = TRUE ;
#endif /* F_MACHINE */

/* go through the current queue and call all the ones with the current phase */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6)) {
	        eprintf("ss_callthem: finding callbacks ck=%llu ph=%d\n",
	            mip->ck,ph) ;
	        eprintf("ss_callthem: count Q index=%d\n",mip->cqi) ;
	    }
#endif

	    cqp = mip->qes + mip->cqi ;
	    for (j = 0 ; veclist_get(cqp,j,&ep) >= 0 ; j += 1) {

	        if (ep == NULL) continue ;

	        if (ep->phase == ph) {

	            n += 1 ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6)) {
	                eprintf("ss_callthem: calling callback\n") ;
	                eprintf("ss_callthem: callback=%p objp=%p\n",
	                    ep->func,ep->objp) ;
	            }
#endif

/* call him !! */

#if	F_MACHINE
	            rs = (*ep->func)(ep->objp,ep->argp,ph) ;

	    if ((rs != 0) && (! f_exit)) {
		lrs = rs ;
	        f_exit = TRUE ;
	    }

		if (rs < 0)
			break ;
#endif /* F_MACHINE */

/* delete him */

	            (void) veclist_del(cqp,j) ;

	            j -= 1 ;	/* compensate for loop counter */

/* but save the memory data structure for quick access later */

	            (void) plainq_ins(&mip->fq,(PLAINQ_ENTRY *) ep) ;

	        } /* end if (got a live one !) */

	    } /* end for (call outs) */

		if (rs < 0)
			break ;

	} /* end for (looping through clock phases) */

/* the fast queue for this clock period should be empty now ! */

	if (rs >= 0) {

	mip->cqi = (mip->cqi + 1) % mip->nq ;

/* if advanced (mip->nq) clocks, then bring forward pending requests */

	mip->ck_count += 1 ;
	if (mip->ck_count == mip->nq) {

	    HDB_DATUM	key, value ;

	    HDB_CURSOR	cur ;

	    ULONG	clockkey ;

	    uint	ci ;

	    int		f_next ;


	    clockkey = (mip->ck + 1) / ((ULONG) mip->nq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	        eprintf("ss_callthem: bring forward, ck=%llu key=%llu\n",
	            mip->ck,clockkey) ;
#endif

	    key.buf = (char *) &clockkey ;
	    key.len = sizeof(LONG) ;

/* find all entries with the same key */

	    hdb_cursorinit(&mip->pq,&cur) ;

	    f_next = TRUE ;
	    while (TRUE) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	            eprintf("ss_callthem: HDB loop f_next=%d\n",f_next) ;
#endif

	        if (f_next)
	            rs1 = hdb_fetch(&mip->pq,key,&cur,&value) ;

	        else
	            rs1 = hdb_getkeyrec(&mip->pq,key,&cur,NULL,&value) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6)) {
	            if (rs1 >= 0)
	                eprintf("ss_callthem: HDB loop rs=%d\n",rs1) ;

	            else if (rs1 == SR_NOTFOUND)
	                eprintf("ss_callthem: HDB loop rs=NOTFOUND\n") ;

	            else
	                eprintf("ss_callthem: HDB loop rs=**BAD**\n") ;

	        }
#endif /* F_DEBUG */

	        if (rs1 < 0)
	            break ;

	        ep = (SS_COE *) value.buf ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6)) {
	            eprintf("ss_callthem: HDB loop entry=%08lx\n",
	                ep) ;
	            eprintf("ss_callthem: HDB loop entry wake=%llu\n",
	                ep->wake) ;
	        }
#endif

	        ci = (uint) (ep->wake - (mip->ck + 1)) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	            eprintf("ss_callthem: HDB loop ci=%u\n",ci) ;
#endif

	        i = (mip->cqi + ci) % mip->nq ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	            eprintf("ss_callthem: HDB loop adding i=%d QESa=%08lx\n",
	                i,mip->qes) ;
#endif

	        (void) veclist_add(mip->qes + i,ep) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	            eprintf("ss_callthem: HDB loop deleting\n") ;
#endif

	        (void) hdb_delcursor(&mip->pq,&cur) ;

	        f_next = FALSE ;	/* deletion, do NOT advance cursor */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	            eprintf("ss_callthem: HDB loop pending key=%llu\n",
	                clockkey) ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6)) {

	            rs = ss_prpq(mip,clockkey) ;

	            eprintf("ss_callthem: HDB pending remaining=%d\n",
	                rs) ;
	        }
#endif /* F_DEBUG */

	    } /* end while */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	        eprintf("ss_callthem: HDB past loop \n") ;
#endif

	    hdb_cursorfree(&mip->pq,&cur) ;

	    mip->ck_count = 0 ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	        eprintf("ss_callthem: brought forward, ck=%llu\n",
	            mip->ck) ;
#endif

	} /* end if (advancing call-outs) */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6)) {

	    eprintf("ss_callthem: pending Q ckey=%llu\n",
	        2ULL) ;

	    rs = ss_prpq(mip,2ULL) ;

	    eprintf("ss_callthem: pending Q remaining=%d\n",
	        rs) ;
	    eprintf("ss_callthem: FQa=%08lx QESa=%08lx\n",
	        &mip->fq,mip->qes) ;
	}
#endif /* F_DEBUG */

	}

/* we-re out of here */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	    eprintf("ss_callthem: ret ck=%llu rs=%d\n",
		mip->ck,lrs) ;
#endif

	return (rs >= 0) ? lrs : rs ;
}
/* end subroutine (ss_callthem) */


#if	F_DEBUG || F_DEBUGS

static int ss_prpq(mip,ckey)
SS	*mip ;
ULONG		ckey ;
{
	SS_COE	*ep ;

	HDB_DATUM	key, value ;

	HDB_CURSOR	cur ;

	int	n = 0 ;


	key.buf = &ckey ;
	key.len = sizeof(ULONG) ;

	eprintf("ss_prpq: pending Q for ckey=%llu\n",ckey) ;

	hdb_cursorinit(&mip->pq,&cur) ;

	while (hdb_fetch(&mip->pq,key,&cur,&value) >= 0) {

	    ep = (SS_COE *) value.buf ;

	    eprintf("ss_prpq: ep=%08lx wake=%llu\n",ep,ep->wake) ;

	    n += 1 ;
	}

	hdb_cursorfree(&mip->pq,&cur) ;

	return n ;
}
/* end subroutine (ss_prpq) */

#endif /* F_DEBUG */



