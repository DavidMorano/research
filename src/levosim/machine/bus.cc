/* bus */

/* bus to handle data transactions (read/write) to and from memory */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		0		/* switchable debug print-outs */
#define	F_WIDEBUS	0		/* handle wide data path ? */
#define	F_PRIORITY	1		/* priority arbitration ? */
#define	F_SAFE		1		/* run in extra SAFE mode ? */
#define	F_WRITENEXT	0		/* write on next clock ? */


/* revision history :

	= 00/05/26, Dave Morano

	This object was taken from the previous 'busmemdata' object.


*/


/******************************************************************************

	This object is a bus component object.   This bus is
	use (or will try to be used) by most all components in the
	Levo machine.

	Operating rules :

	+ make requests for the bus in phase 0 (special)
	+ request holds of the bus in phase 1 or later (normal rule)
	+ arbitration of the bus occurs in phase 1
	+ check to see if you have won the bus for the next clock
	  in phase 2
	+ bus holds to not propagate to an idle clock on the bus until
          the second clock after you make the 'bus_hold()' call
	+ write to the current clock ('bus_writevur()') in phase
	  1 or later (normal rule)


	Note: As far as the usage data that is collected on the bus,
	the sum of the following makes up all clocks on the bus :

	- idle
	- stalls
	- used
	- unused

	The data on 'holds' is partly :

	- idle
	- used
	- unused



******************************************************************************/


#define	BUS_MASTER	0


#include	<sys/types.h>
#include	<malloc.h>
#include	<cstring>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lflowgroup.h"
#include	"bus.h"



/* local defines */

#define	BUS_MAGIC	0x98877665


/* external subroutines */


/* forward references */

static int	bus_arbitrate(BUS *,struct proginfo *) ;
static int	bus_clear(BUS *,struct proginfo *) ;






int bus_init(busp,pip,nrequests,width)
BUS		*busp ;
struct proginfo	*pip ;
int		nrequests ;
int		width ;
{
	int	rs, i ;
	int	size ;


	if ((busp == NULL) || (pip == NULL))
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("bus_init: BUS=%08lx nreqs=%d\n",
	        busp,nrequests) ;
	    eprintf("bus_init: state size=%d\n",
	        sizeof(struct bus_state)) ;
	}
#endif

/* do not allow wide buses for now ) */

	if ((width > 1) || (nrequests < 1))
	    return SR_NOTSUP ;

	(void) memset(busp,0,sizeof(BUS)) ;

	busp->pip = pip ;
	busp->nmasters = nrequests ;
	busp->width = width ;

/* allocate space for the requests needed */

	    size = nrequests * sizeof(int) ;
	    if ((rs = uc_malloc(size,&busp->requests)) < 0)
	        goto bad1 ;

	    for (i = 0 ; i < nrequests ; i += 1)
	        busp->requests[i] = -1 ;


	busp->c.cmid = busp->n.cmid = -1 ;


#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&busp->c,sizeof(struct bus_state),&busp->c.checksum) ;
#endif

	busp->clock = 0 ;


	busp->magic = BUS_MAGIC ;
	return SR_OK ;

/* things come here */
bad2:
	if (busp->requests != NULL)
	    free(busp->requests) ;

bad1:
	return rs ;
}
/* end subroutine (bus_init) */


/* free up this object */
int bus_free(busp)
BUS	*busp ;
{


	if (busp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_SAFE
	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_free: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */


/* de-allocate the requests array */

	if (busp->requests != NULL) {

	    free(busp->requests) ;

	    busp->requests = NULL ;
	}

	busp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (bus_free) */


/* perform our combinatorial work */
int bus_comb(busp,phase)
BUS	*busp ;
int	phase ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_comb: bad format BUS=%08lx\n",busp) ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

#if	F_DEBUGS
	eprintf("bus_comb: BUS=%08lx\n",
	    busp) ;
	eprintf("bus_comb: pip=%08lx\n",
	    busp->pip) ;
#endif

	pip = busp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_comb: BUS=%08lx ph=%d\n",
	        busp,phase) ;
#endif

	switch (phase) {

/* requests happen here */
	case 0:
	    busp->f.written = FALSE ;		/* used in phase 4 */
	    busp->n.f.hold = FALSE ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("bus_comb: c_mp=%d\n",busp->c.mp) ;
#endif

	    break ;

/* we are computing the next master here */
	case 1:
	    (void) bus_arbitrate(busp,pip) ;

	    break ;

/* grants are available here */
	case 2:
	    break ;

	case 3:
	    if (busp->c.f.busy && (! busp->c.f.hold)) {

	        if (busp->f.dp)
	            busp->n.s.used += 1 ;

	        else
	            busp->n.s.unused += 1 ;

	    } /* end if (bus statistics) */

	    break ;

	case 4:
	    rs = bus_clear(busp,pip) ;

	    break ;

	} /* end switch */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_comb: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (bus_comb) */


/* we get clocked somehow */
int bus_clock(busp)
BUS	*busp ;
{
	struct proginfo	*pip ;

	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_clock: BUS=%08lx bad format\n",
	        busp) ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = busp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_clock: BUS=%08lx c_mp=%d\n",busp,busp->c.mp) ;
#endif

#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&busp->c,sizeof(struct bus_state),
	    &busp->c.checksum) ;

	if (rs < 0) {

	    eprintf("bus_clock: BUS=%08lx check bad format\n",
	        busp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */


/* transfer the stuff */


	busp->c = busp->n ;


#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&busp->c,sizeof(struct bus_state),&busp->c.checksum) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_clock: f_busy=%d f_hold=%d c_mp=%d\n",
		busp->c.f.busy,busp->c.f.hold,busp->c.mp) ;
#endif

	busp->clock += 1 ;
	return SR_OK ;
}
/* end subroutine (bus_clock) */


/* wanna-ba masters request the bus (do this in phase 0) */
int bus_request(busp,bid,pri)
BUS	*busp ;
int	bid, pri ;
{
	struct proginfo	*pip ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_request: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = busp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_request: %08lx entered id=%d pri=%d\n",
	        busp,bid,pri) ;
#endif

	if ((bid < 0) || (bid >= busp->nmasters))
	    return SR_INVALID ;

	if (busp->requests[bid] >= 0)
	    return SR_ALREADY ;

	busp->requests[bid] = MAX(pri,0) ;
	return SR_OK ;
}
/* end subroutine (bus_request) */


/* requesters call to see if they are the next master */
int bus_grantednext(busp,id)
BUS	*busp ;
int	id ;
{
	struct proginfo	*pip ;

	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_greantednext: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = busp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_grantednext: BUS=%08lx id=%d\n",
	        busp,id) ;
#endif

	rs = SR_INVALID ;
	if ((id >= 0) && (id < busp->nmasters)) {

	    rs = SR_NOTCONN ;
	    if (busp->requests[id] >= 0)
	        rs = (busp->n.cmid == id) && (! busp->c.f.hold) ;

	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_grantednext: rs=%d\n",
	        rs) ;
#endif

	return rs ;
}
/* end subroutine (bus_grantednext) */


/* return the number of bus masters for this bus */
int bus_nmasters(busp)
BUS	*busp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_nmasters: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return busp->nmasters ;
}
/* end subroutine (bus_nmasters) */


/* return the current bus master */
int bus_curmaster(busp)
BUS	*busp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_curmaster: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return busp->c.cmid ;
}
/* end subroutine (bus_curmaster) */


/* hold the bus (stop new grants) */
int bus_hold(busp)
BUS	*busp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


#if	F_DEBUGS
	eprintf("bus_hold: BUS=%p entered\n",busp) ;
#endif

#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_hold: bad format BUS=%08lx\n",busp) ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = busp->pip ;


#if	F_DEBUGS
	eprintf("bus_hold: 2 \n") ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_hold: BUS=%08lx\n",
	        busp) ;
#endif

	busp->n.f.hold = TRUE ;

	return SR_OK ;
}
/* end subroutine (bus_hold) */


/* write the CURRENT clock cycle */
int bus_writecur(busp,dp)
BUS		*busp ;
LFLOWGROUP	*dp ;
{
	struct proginfo	*pip ;

	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_writecur: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = busp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_writecur: BUS=%08lx a=%08x dv=%08x dp=%d\n",
		busp,dp->addr,dp->dv,dp->dp) ;
#endif

#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&busp->c,sizeof(struct bus_state),
	    &busp->c.checksum) ;

	if (rs < 0) {

	    eprintf("bus_writecur: BUS=%08lx check bad format\n",
	        busp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */

	busp->content = *dp ;
	busp->f.dp = TRUE ;

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&busp->c,sizeof(struct bus_state),&busp->c.checksum) ;
#endif

	return SR_OK ;
}
/* end subroutine (bus_writecur) */


#if	F_WRITENEXT

/* write in the NEXT clock cycle */
int bus_writenext(busp,dp)
BUS		*busp ;
LFLOWGROUP	*dp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_writenext: bad format BUS=%08lx\n",busp) ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	busp->n.content = *dp ;
	busp->n.f.dp = TRUE ;

	busp->f.written = TRUE ;
	return SR_OK ;
}
/* end subroutine (bus_writenext) */

#endif /* F_WRITENEXT */


/* read the data off of the bus (if there was any) */
int bus_read(busp,dp)
BUS		*busp ;
LFLOWGROUP	*dp ;
{
	struct proginfo	*pip ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_read: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = busp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("bus_read: BUS=%08lx busy=%d dp=%d\n",
	    busp,busp->c.f.busy,busp->f.dp) ;
#endif

	if ((! busp->c.f.busy) || (! busp->f.dp))
	    return 0 ;

	*dp = busp->content ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("bus_read: a=%08x dv=%08x dp=%d\n",
		dp->addr,dp->dv,dp->dp) ;
#endif

	return 1 ;
}
/* end subroutine (bus_read) */


/* sanity check stuff */
int bus_sanitycheck(busp)
BUS	*busp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_sanitycheck: bad format BUS=%08lx\n",busp) ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

#if	F_DEBUGS
	eprintf("bus_sanitycheck: BUS=%08lx\n",
	    busp) ;
	eprintf("bus_sanitycheck: pip=%08lx\n",
	    busp->pip) ;
#endif

	pip = busp->pip ;

	if (pip->magic != PROGMAGIC) {

#if	F_DEBUGS
	    eprintf("bus_sanitycheck: bad PROGMAGIC=%08x\n",
	        pip->magic) ;
#endif

	    return SR_BADFMT ;
	}


	return rs ;
}
/* end subroutine (bus_sanitycheck) */


/* get statistics */
int bus_getstats(busp,sp)
BUS		*busp ;
BUS_STATS	*sp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_getstats: bad format BUS=%08lx\n",busp) ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (sp == NULL)
	    return SR_FAULT ;

	*sp = busp->c.s ;
	return SR_OK ;
}
/* end subroutine (bus_getstats) */


/* get info */
int bus_getinfo(busp,ip)
BUS		*busp ;
BUS_INFO	*ip ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != BUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("bus_getinfo: bad format BUS=%08lx\n",busp) ;

	    return SR_BADFMT ;
	}

	if (busp->magic != BUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (ip == NULL)
	    return SR_FAULT ;

	ip->nreqs = busp->c.nreqs ;
	ip->cmid = busp->c.cmid ;
	ip->f = busp->c.f ;

	return SR_OK ;
}
/* end subroutine (bus_getinfo) */



/* PRIVATE SUBROUTINES */



/* the bus performs arbitration on phase 1 for the next clock period */
static int bus_arbitrate(busp,pip)
BUS		*busp ;
struct proginfo	*pip ;
{
	int	i, j ;
	int	wid = -1, mpri = -1 ;
	int	nreqs ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_arbitrate: BUS=%p f_hold=%d mp=%d\n",
	        busp,busp->c.f.hold,busp->c.mp) ;
#endif

/* how many requests are there in this clock ? */

	nreqs = 0 ;
	for (i = 0 ; i < busp->nmasters ; i += 1) {

	    if (busp->requests[i] >= 0)
	        nreqs += 1 ;

	} /* end for */

	busp->n.nreqs = nreqs ;
	if (nreqs == 0)
	    busp->n.s.idle += 1 ;

/* proceed with the arbitration */

	if (! busp->c.f.hold) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf("bus_arbitrate: nm=%d\n", busp->nmasters) ;
	        eprintf("bus_arbitrate: reqs>") ;
	        for (i = 0 ; i < busp->nmasters ; i += 1) {
	            if (busp->requests[i] >= 0)
	                eprintf(" %d",i) ;
	        }
	        eprintf("\n") ;
	    }
#endif

	    wid = -1 ;
	    if (nreqs > 0) {

	        busp->n.s.conflicts += (nreqs - 1) ;

	        i = (busp->c.mp + 1) % busp->nmasters ;
	        for (j = 0 ; j < busp->nmasters ; j += 1) {

#if	F_PRIORITY
	            if (busp->requests[i] > mpri) {

	                wid = i ;
	                mpri = busp->requests[i] ;
	            }
#else
	            if (busp->requests[i] >= 0)
	                break ;
#endif /* F_PRIORITY */

	            i = (i + 1) % busp->nmasters ;

	        } /* end for */

#if	(! F_PRIORITY)
	        if (j < busp->nmasters)
	            wid = i ;
#endif

	    } /* end if (there were requests) */

/* do we have a winner ? (winner's ID in 'wid') ? */

	    if (wid >= 0) {

	        busp->n.mp = wid ;
	        busp->n.cmid = wid ;
	        busp->n.f.busy = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("bus_arbitrate: winner_id=%d n_mp=%d\n",
	                wid,busp->n.mp) ;
#endif

	    } else {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("bus_arbitrate: no requests found\n") ;
#endif

	        busp->n.mp = busp->c.mp ;
	        busp->n.cmid = -1 ;
	        busp->n.f.busy = FALSE ;

	    }

	} else {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("bus_arbitrate: being held \n") ;
#endif

	    busp->n.mp = busp->c.mp ;		/* next position (no change) */
	    busp->n.cmid = -1 ;			/* next master */
	    busp->n.f.busy = TRUE ;

	    busp->n.s.conflicts += nreqs ;
	    busp->n.s.holds += 1 ;
	    if (nreqs > 0)
	        busp->n.s.stalls += 1 ;

	} /* end if (bus held or not) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("bus_arbitrate: ck=%lld, n_mid=%d n_mp=%d\n",
	        busp->clock,busp->n.cmid,busp->n.mp) ;
	    eprintf("bus_arbitrate: f_hold=%d f_nextbusy=%d cmid=%d\n",
	        busp->c.f.hold, busp->n.f.busy,busp->c.cmid) ;
	}
#endif

	return busp->n.cmid ;
}
/* end subroutine (bus_arbitrate) */


/* clear the data off of the bus for the NEXT cycle */
static int bus_clear(busp,pip)
BUS		*busp ;
struct proginfo	*pip ;
{
	int	cmid ;


/* clear the data present bits of the bus */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    int	i ;
	    eprintf("bus_clear: BUS=%08lx f_hold=%d n_mid=%d\n",
	        busp, busp->c.f.hold, busp->n.cmid) ;
	    eprintf("bus_clear: reqs>") ;
	    for (i = 0 ; i < busp->nmasters ; i += 1) {
	        if (busp->requests[i] >= 0)
	            eprintf(" %d",i) ;
	    }
	    eprintf("\n") ;
	}
#endif /* F_DEBUG */


/* retire current bus winner */

	if ((busp->n.cmid >= 0) && (! busp->c.f.hold)) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("bus_clear: retiring id=%d\n",
	            busp->n.cmid) ;
#endif

	    busp->requests[busp->n.cmid] = -1 ;

	} /* end if (retiring current bus winner) */


/* clear the data on the bus */

	if (! busp->f.written) {

	    busp->f.dp = FALSE ;
	    busp->content.dp = LFLOWGROUP_DPNONE ;

	} /* end if (written or not for next cycle) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bus_clear: n_mp=%d\n",
	        busp->n.mp) ;
#endif


	return SR_OK ;
}
/* end subroutine (bus_clear) */



