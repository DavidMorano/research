/* ssclocking */

/* Levo simulator state sequencing operations */


#define	F_DEBUGS	1		/* non-switchable debug print-outs */
#define	F_DEBUG		1		/* switchable debugging */
#define	F_SAFE		1		/* play it safe ? */
#define	F_MACHINE	1		/* call the LEVO machine methods */
#define	F_MEMSET	0		/* zero out object memory */
#define	F_MAXCLOCKS	1		/* maximum numbers of clocks */
#define	F_PROGINFOCHECK	0		/* do 'proginfo_check()' */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started (for LevoSim).


*/


/******************************************************************************

	This is the object (roughly) for handling simulator specific
	operations such as call-backs.



******************************************************************************/


#define	SSCLOCKING_MASTER	1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>
#include	<veclist.h>
#include	<vecelem.h>
#include	<hdb.h>
#include	<plainq.h>

#include	"localmisc.h"
#include	"paramfile.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"ssclocking.h"		/* ourselves */



/* local defines */

#define	SSCLOCKING_MAGIC		0x95827364
#define	SSCLOCKING_CLOCKINTERVAL	200



/* external subroutines */


/* local structures */


/* forward references */

static int	ssclocking_callthem(SSCLOCKING *) ;

#if	F_DEBUG || F_DEBUGS
static int	ssclocking_prpq(SSCLOCKING *,ULONG) ;
#endif


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







int ssclocking_init(mip,pip,maxclocks,machobj)
SSCLOCKING		*mip ;
struct proginfo		*pip ;
ULONG			maxclocks ;
SSCLOCKING_MACHOBJ	*machobj ;
{
	PARAMFILE		*pfp = &pip->pf ;

	ULONG	ulw ;

	int	rs = SR_OK ;
	int	i, j ;
	int	iw, sl ;
	int	size ;

	char	*cp ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_DEBUG && 0
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_init: entered\n") ;
#endif

	(void) memset(mip,0,sizeof(SSCLOCKING)) ;

	mip->magic = 0 ;
	mip->pip = pip ;
	mip->nentries = 0 ;
	mip->nq = SSCLOCKING_NQUEUES ;
	mip->cqi = 0 ;
	mip->clock = 0 ;
	mip->clock_count = 0 ;

	mip->f.syscalls = FALSE ;


/* store what we have so far */

/* the program information */


/* the machine object information */

	mip->topobjp = NULL ;
	mip->topcombp = NULL ;
	mip->topclockp = NULL ;
	if (machobj != NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_init: got a machobj, LEVO=%08lx\n",
	            machobj->topobjp) ;
#endif

	    mip->topobjp = machobj->topobjp ;
	    mip->topcombp = machobj->topcombp ;
	    mip->topclockp = machobj->topclockp ;

	}

/* get some parameters */

	for (i = 0 ; lparams[i] != NULL ; i += 1) {

	    if ((sl = paramfile_fetch(pfp,lparams[i],NULL,&cp)) >= 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4) {
	            eprintf("ssclocking_init: %i lparam=%s \n",i,lparams[i]) ;
	            eprintf("ssclocking_init: %i val=%s\n",i,cp) ;
	        }
#endif

	        switch (i) {

	        case lparam_maxclocks:
	            rs = cfnumull(cp,sl,&ulw) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 4)
	                eprintf("ssclocking_init: rs=%d maxclocks=%llu\n",
	                    rs,ulw) ;
#endif

	            if (rs >= 0) {

	                switch (i) {

	                case lparam_maxclocks:
	                    if (maxclocks == 0)
	                        maxclocks = ulw ;

	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        case lparam_fastqueues:
	            rs = cfnumui(cp,sl,&iw) ;

	            if (rs >= 0) {

	                switch (i) {

	                case lparam_fastqueues:
	                    mip->nq = iw ;
	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        } /* end swtich */

	    } /* end if */

	} /* end for (getting simulation parameters) */


/* check and store any new parameters */

	if (maxclocks == 0)
	    maxclocks = SSCLOCKING_MAXCLOCKS ;

	mip->maxclocks = maxclocks ;

	if (mip->nq < 5)
	    mip->nq = 5 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4) {
	    eprintf("ssclocking_init: maxclock=%llu\n",mip->maxclocks) ;
	    eprintf("ssclocking_init: fastqueues=%u\n",mip->nq) ;
	}
#endif /* F_DEBUG */


/* start initializing our scheduling apparatus */

	size = mip->nq * sizeof(veclist) ;
	rs = uc_malloc(size,&mip->qes) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("ssclocking_init: Qs malloc failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(mip->qes,size,"ssclocking_init:calloutqueues") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    iw = i ;
	    rs = veclist_init(mip->qes + i,0,VECLIST_PSWAP) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("ssclocking_init: Qs veclist(%d) failed rs=%d\n",i,rs) ;
#endif

	    if (rs < 0)
	        goto bad1 ;

#if	F_DEBUG && 0
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_init: queue list(%d)=%08lx\n",(mip->qes + i)) ;
#endif

	} /* end for */

#if	F_DEBUG && 0
	if (pip->debuglevel >= 4) {
	    eprintf("ssclocking_init: ck=%llu \n",mip->clock) ;
	    eprintf("ssclocking_init: 1 c0=%08x c1=%08x\n",
	        getarrayint(&mip->clock,0),
	        getarrayint(&mip->clock,1)) ;
	}
#endif /* F_DEBUG */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_init: plainq_init() FQ=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_init(&mip->fq,0) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("ssclocking_init: plainq failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_printf("plainq_init") ;
#endif

#if	F_DEBUG && 0
	if (pip->debuglevel >= 4) {
	    eprintf("ssclocking_init: ck=%llu \n",mip->clock) ;
	    eprintf("ssclocking_init: 2 c0=%08x c1=%08x\n",
	        getarrayint(&mip->clock,0),
	        getarrayint(&mip->clock,1)) ;
	}
#endif /* F_DEBUG */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_init: hdb_init()\n") ;
#endif

	rs = hdb_init(&mip->pq,mip->nq,NULL) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("ssclocking_init: PQ failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad3 ;

#ifdef	MALLOCLOG
	malloclog_printf("hdb_init") ;
#endif


/* supposedly, we are fairly well done ! (need to index SYSCALLs later) */

	mip->magic = SSCLOCKING_MAGIC ;


#if	F_DEBUG && 0
	if (pip->debuglevel >= 2) {
	    eprintf("ssclocking_init: ck=%llu \n",mip->clock) ;
	    eprintf("ssclocking_init: 3 c0=%08x c1=%08x\n",
	        getarrayint(&mip->clock,0),
	        getarrayint(&mip->clock,1)) ;
	    eprintf("ssclocking_init: exiting OK rs=%d\n",rs) ;
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	    eprintf("ssclocking_init: exiting BAD rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssclocking_init) */


/* set the machine object information */
int ssclocking_setmach(mip,machobj)
SSCLOCKING		*mip ;
SSCLOCKING_MACHOBJ	*machobj ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSCLOCKING_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSCLOCKING_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	F_DEBUG && 0
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_setmach: entered\n") ;
#endif


	if (machobj->topobjp == NULL)
	    return SR_FAULT ;

	if (machobj->topcombp == NULL)
	    return SR_FAULT ;

	if (machobj->topclockp == NULL)
	    return SR_FAULT ;

/* the machine object information */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_setmach: got a machobj, LEVO=%08lx\n",
	        machobj->topobjp) ;
#endif

	mip->topobjp = machobj->topobjp ;
	mip->topcombp = machobj->topcombp ;
	mip->topclockp = machobj->topclockp ;


	return SR_OK ;
}
/* end subroutine (ssclocking_setmach) */


/* free up whatever ! */
int ssclocking_free(mip)
SSCLOCKING		*mip ;
{
	struct proginfo	*pip ;

	SSCLOCKING_COE	*ep ;

	veclist		*cqp ;

	int	rs, i, j ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSCLOCKING_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSCLOCKING_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_free: entered\n") ;
#endif

/* free up any and all entries that are out there ! */

/* free all entries on the free queue */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_free: free Q entries\n") ;
#endif

	while (plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) != PLAINQ_RUNDER)
	    free(ep) ;


/* free all entries on the future pending queue */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_free: pending Q entries\n") ;
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_free: fast Q entries\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    cqp = mip->qes + i ;
	    for (j = 0 ; (rs = veclist_get(cqp,j,&ep)) >= 0 ; j += 1) {

	        if (ep == NULL) continue ;

	        free(ep) ;

	    } /* end while */

	} /* end for */


/* free up all queue objects */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_free: fast Qs\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    (void) veclist_free(mip->qes + i) ;

	} /* end for */

	(void) plainq_free(&mip->fq) ;

	(void) hdb_free(&mip->pq) ;

	free(mip->qes) ;

	mip->qes = NULL ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_free: exiting\n") ;
#endif

	mip->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (ssclocking_free) */


/* take call-back requests from other objects */
int ssclocking_callout(mip,func,objp,argp,nclocks,phase)
SSCLOCKING	*mip ;
int		(*func)() ;
void		*objp, *argp ;
uint		nclocks ;
int		phase ;
{
	struct proginfo	*pip ;

	SSCLOCKING_COE	*ep ;

	int	rs = SR_OK ;
	int	i ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSCLOCKING_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSCLOCKING_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4) {
	    eprintf("ssclocking_callout: entered func=%08lx objp=%08lx argp=%08lx\n",
	        func,objp,argp) ;
	    eprintf("ssclocking_callout: ck=%llu nclocks=%u\n",
	        mip->clock,nclocks) ;
	}
#endif /* F_DEBUG */

	if (nclocks == 0)
	    return SR_INVALID ;

/* get a free entry */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callout: plainq_rem() FQa=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callout: plainq_rem() rs=%d\n",rs) ;
#endif

	if (rs == PLAINQ_RUNDER) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_callout: plainq_rem() underflow\n") ;
#endif

	    rs = uc_malloc(sizeof(SSCLOCKING_COE),(void *) &ep) ;

	    if (rs < 0)
	        return rs ;

#ifdef	MALLOCLOG
	    malloclog_alloc(ep,sizeof(SSCLOCKING_COE),"ssclocking_callout:coe") ;
#endif

	    mip->nentries += 1 ;

	} /* end if */

/* fill it in */

	ep->wake = mip->clock + nclocks ;
	ep->relative = nclocks ;
	ep->phase = phase ;
	ep->func = func ;
	ep->objp = objp ;
	ep->argp = argp ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callout: wake=%llu\n",ep->wake) ;
#endif

	if (nclocks < mip->nq) {

/* short time in the future */

	    i = (mip->cqi + nclocks) % mip->nq ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_callout: short=%u wake=%llu\n",i,ep->wake) ;
#endif

	    rs = veclist_add(mip->qes + i,ep) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_callout: veclist_add rs=%d\n",rs) ;
#endif

	} else {

	    HDB_DATUM	key, value ;


/* handle requests for longer time into the future */

	    ep->key = ep->wake / ((ULONG) mip->nq) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_callout: long future, key=%llu\n",
	            ep->key) ;
#endif

	    key.buf = (char *) &ep->key ;
	    key.len = sizeof(ULONG) ;

	    value.buf = ep ;
	    value.len = sizeof(SSCLOCKING_COE) ;

	    rs = hdb_store(&mip->pq,key,value) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_callout: hdb_store rs=%d\n",rs) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4) {

	        rs = ssclocking_prpq(mip,ep->key) ;

	        eprintf("ssclocking_callout: pending on key=%llu n=%d\n",
	            ep->key,rs) ;
	    }
#endif /* F_DEBUG */

	} /* end if */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callout: exiting rs=%d\n",
	        rs) ;
#endif

	return rs ;
}
/* end subroutine (ssclocking_callout) */


/* do the main loop */
int ssclocking_loop(mip)
SSCLOCKING		*mip ;
{
	struct proginfo	*pip ;

	ULONG	clocklast = 0 ;

	int	rs ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSCLOCKING_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSCLOCKING_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;
	if (mip->topobjp == NULL)
	    return SR_FAULT ;

	if (mip->topcombp == NULL)
	    return SR_FAULT ;

	if (mip->topclockp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_loop: LEVO topobj=%p\n",
	        mip->topobjp) ;
#endif

/* we're good, let's go */

	mip->clock = 0 ;

	while (TRUE) {

/* call the stuff for combinatorial processing */

	    rs = ssclocking_callthem(mip) ;

	    if (rs != 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_loop: exiting from combinatorial rs=%d\n",
	                rs) ;
#endif

	        break ;

	    } /* end if (combinatorial exit) */


/* should we do any maintenance activities ? */

#if	F_PROGINFOCHECK
	    if ((mip->clock - clocklast) > SSCLOCKING_CLOCKINTERVAL) {

	        clocklast = mip->clock ;
	        proginfo_check(pip,mip->clock) ;

	    } /* end if (maintenance activities) */
#endif /* F_PROGINFOCHECK */


/* call the stuff to transition the clock */

#if	F_MACHINE
	    rs = (*mip->topclockp)(mip->topobjp) ;

	    if (rs != 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_loop: exiting from clock transition rs=%d\n",
	                rs) ;
#endif

	        break ;
	    }
#endif /* F_MACHINE */

	    mip->clock += 1 ;

#if	F_MAXCLOCKS
	    if ((mip->maxclocks > 0) && (mip->clock >= mip->maxclocks)) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_loop: exiting from maxclocks=%lld\n",
			mip->clock) ;
#endif

	        break ;
	    }
#endif /* F_MAXCLOCKS */

	} /* end while */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_loop: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssclocking_loop) */


/* return the clock number to the caller */
int ssclocking_getclock(mip,lp)
SSCLOCKING	*mip ;
ULONG		*lp ;
{


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSCLOCKING_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSCLOCKING_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (lp == NULL)
	    return SR_FAULT ;

	*lp = mip->clock ;
	return SR_OK ;
}
/* end subroutine (ssclocking_getclock) */



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

static int ssclocking_callthem(mip)
SSCLOCKING		*mip ;
{
	struct proginfo	*pip ;

	veclist		*cqp ;

	SSCLOCKING_COE	*ep ;

	int	phase ;
	int	lrs, rs, i, j ;
	int	n = 0 ;
	int	f_exit ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSCLOCKING_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSCLOCKING_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callthem: entering ck=%llu comb is LEVO=%08lx\n",
	        mip->clock,mip->topobjp) ;
#endif

/* loop through all of the phases */

	lrs = SR_OK ;
	phase = 0 ;
	f_exit = FALSE ;
	while ((! f_exit) || (phase < SSCLOCKING_MINPHASES)) {

	    mip->phase = phase ;

#if	F_MACHINE
	    lrs = (*mip->topcombp)(mip->topobjp,phase) ;

	    if (lrs != 0)
	        f_exit = TRUE ;
#else
	    f_exit = TRUE ;
#endif /* F_MACHINE */

/* go through the current queue and call all the ones with the current phase */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4) {
	        eprintf("ssclocking_callthem: finding callbacks ph=%d\n",
	            phase) ;
	        eprintf("ssclocking_callthem: count Q index=%d\n",mip->cqi) ;
	    }
#endif

	    f_exit = TRUE ;
	    cqp = mip->qes + mip->cqi ;
	    for (j = 0 ; (rs = veclist_get(cqp,j,&ep)) >= 0 ; j += 1) {

	        if (ep == NULL) continue ;

	        f_exit = FALSE ;
	        if (ep->phase == phase) {

	            n += 1 ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 4) {
	                eprintf("ssclocking_callthem: calling callback\n") ;
	                eprintf("ssclocking_callthem: callback=%08lx objp=%08lx\n",
	                    ep->func,ep->objp) ;
	            }
#endif

/* call him !! */

#if	F_MACHINE
	            lrs = (*ep->func)(ep->objp,ep->argp,phase) ;

	            if (lrs != 0)
	                f_exit = TRUE ;
#endif /* F_MACHINE */

/* delete him */

	            (void) veclist_del(cqp,j) ;

	            j -= 1 ;	/* compensate for loop counter */

/* but save the memory data structure for quick access later */

	            (void) plainq_ins(&mip->fq,(PLAINQ_ENTRY *) ep) ;

	        } /* end if (got a live one !) */

	        if (f_exit)
	            break ;

	    } /* end for */

	    phase += 1 ;
	    if (lrs != 0)
	        break ;

	} /* end while (looping through clock phases) */

/* the fast queue for this clock period should be empty now ! */

	mip->cqi = (mip->cqi + 1) % mip->nq ;

/* if advanced (mip->nq) clocks, then bring forward pending requests */

	mip->clock_count += 1 ;
	if (mip->clock_count == mip->nq) {

	    HDB_DATUM	key, value ;

	    HDB_CURSOR	cur ;

	    ULONG	clockkey ;

	    uint	ci ;

	    int		f_next ;


	    clockkey = (mip->clock + 1) / ((ULONG) mip->nq) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_callthem: bring forward, ck=%llu key=%llu\n",
	            mip->clock,clockkey) ;
#endif

	    key.buf = (char *) &clockkey ;
	    key.len = sizeof(LONG) ;

/* find all entries with the same key */

	    hdb_cursorinit(&mip->pq,&cur) ;

	    f_next = TRUE ;
	    while (TRUE) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_callthem: HDB loop f_next=%d\n",f_next) ;
#endif

	        if (f_next)
	            rs = hdb_fetch(&mip->pq,key,&cur,&value) ;

	            else
	            rs = hdb_getkeyrec(&mip->pq,key,&cur,NULL,&value) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4) {
	            if (rs >= 0)
	                eprintf("ssclocking_callthem: HDB loop rs=%d\n",rs) ;

	            else if (rs == -2)
	                eprintf("ssclocking_callthem: HDB loop rs=NOTFOUND\n") ;

	                else
	                eprintf("ssclocking_callthem: HDB loop rs=**BAD**\n") ;

	        }
#endif /* F_DEBUG */

	        if (rs < 0)
	            break ;

	        ep = (SSCLOCKING_COE *) value.buf ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4) {
	            eprintf("ssclocking_callthem: HDB loop entry=%08lx\n",
	                ep) ;
	            eprintf("ssclocking_callthem: HDB loop entry wake=%llu\n",
	                ep->wake) ;
	        }
#endif

	        ci = (uint) (ep->wake - (mip->clock + 1)) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_callthem: HDB loop ci=%u\n",ci) ;
#endif

	        i = (mip->cqi + ci) % mip->nq ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_callthem: HDB loop adding i=%d QESa=%08lx\n",
	                i,mip->qes) ;
#endif

	        (void) veclist_add(mip->qes + i,ep) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_callthem: HDB loop deleting\n") ;
#endif

	        (void) hdb_delcursor(&mip->pq,&cur) ;

	        f_next = FALSE ;	/* deletion, do NOT advance cursor */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("ssclocking_callthem: HDB loop pending key=%llu\n",
	                clockkey) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4) {

	            rs = ssclocking_prpq(mip,clockkey) ;

	            eprintf("ssclocking_callthem: HDB pending remaining=%d\n",
	                rs) ;
	        }
#endif /* F_DEBUG */

	    } /* end while */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("ssclocking_callthem: HDB past loop \n") ;
#endif

	    hdb_cursorfree(&mip->pq,&cur) ;

	    mip->clock_count = 0 ;

	} /* end if */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callthem: pending Q ckey=%llu\n",
	        2ULL) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4) {

	    rs = ssclocking_prpq(mip,2ULL) ;

	    eprintf("ssclocking_callthem: pending Q remaining=%d\n",
	        rs) ;
	}
#endif /* F_DEBUG */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callthem: FQa=%08lx QESa=%08lx\n",
	        &mip->fq,mip->qes) ;
#endif


/* we-re out of here */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("ssclocking_callthem: ret rs=%d\n",lrs) ;
#endif

	return lrs ;
}
/* end subroutine (ssclocking_callthem) */


#if	F_DEBUG || F_DEBUGS

static int ssclocking_prpq(mip,ckey)
SSCLOCKING	*mip ;
ULONG		ckey ;
{
	SSCLOCKING_COE	*ep ;

	HDB_DATUM	key, value ;

	HDB_CURSOR	cur ;

	int	rs, n = 0 ;


	key.buf = &ckey ;
	key.len = sizeof(ULONG) ;

	eprintf("ssclocking_prpq: pending Q for ckey=%llu\n",ckey) ;

	hdb_cursorinit(&mip->pq,&cur) ;

	while ((rs = hdb_fetch(&mip->pq,key,&cur,&value)) >= 0) {

	    ep = (SSCLOCKING_COE *) value.buf ;

	    eprintf("ssclocking_prpq: ep=%08lx wake=%llu\n",ep,ep->wake) ;

	    n += 1 ;
	}

	hdb_cursorfree(&mip->pq,&cur) ;

	return n ;
}
/* end subroutine (ssclocking_prpq) */

#endif /* F_DEBUG */



