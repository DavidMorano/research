/* regfilter */

/* Levo Forwarding Register Filter */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		0		/* switchable debug print-outs */
#define	F_SAFE		1		/* extra safe mode */
#define	F_SAFE2		1		/* extra safe mode */
#define	F_SETCURVAL	0		/* set a current value ? */
#define	F_FPRINTF	1		/* do fprintf()'s ? */
#define	F_MERGEFWD	0		/* forward registers on merge ? */
#define	F_SHIFTDELAY	1		/* do delayed FIFO shift-ins ? */
#define	F_CANCELFWD	0		/* try to cancel FWDs ? */
#define	F_BWDDATA	1		/* put current data in RB requests */
#define	F_EXTRABROUT	0		/* extra 'brout' ? */
#define	F_EXTRADATA	1		/* forward data if same value */
#define	F_EXTRANULL	1		/* forward all NULLIFYs */
#define	F_PRENULL	0		/* forward previous data on NULLIFYs */


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written.


	= 00/12/05, Dave Morano

	Alireza just figured out that he is using NULLIFY transactions
	to cancel the (bad) effects of ASes executing when they were
	not supposed to.  Previously, they relayed their previous
	register values.  He thinks that this may be less bandwidth on
	the register buses than forwarding previous values.  When
	previous values are forwarded (when as AS is currently
	disabled), the AS has to continously forward new previous
	values as they become available since they are later in time
	than where the good values are coming from.  This does use
	bandwidth but so does forwarding NULLIFYs.  Each AS in a branch
	domain that is disabled will probably forward NULLIYs
	redundantly using up a good bit of forwarding bandwidth also !
	In the end, we should simulate both strategies and see which
	provides faster execution times !

	Anyway, I have to make changes in this code to reflect doing
	something reasonably with register NULLIFYs !


	= 00/12/14, Dave Morano

	I have changed a lot of stuff that has to do with the bus
	handling.  I have made a more logical code arrangement for
	handling both forwarding and backwarding buses that hopefully
	is less prone to error.  After all, getting the errors out of
	this stuff is the whole name of the ball game !


	= 01/02/27, Dave Morano

	I added some code to not "backward" a request on our outgoing
	backwarding bus if we are at the very top of the logical
	machine.  

	I am supposedly updating the code to add a "ready commit"
	type method and feature so that column commitments will
	not happen if we have some stuff outstanding on an outgoing
	bus.  I think that we only have to check forwarding buses
	since if something is on a backwarding bus, some AS someplace
	will know about it (the one who initiated it) and will not
	singal a readiness to commit in that case.


	= 01/03/07, Dave Morano

	After some hard and long soul searching, I think that we all
	now know that Ali should not have changed the "original design"
	away from using relay updates and towards his use of NULLIFYs !  
	But we all encouraged him so we all have to share some of
	the consequences.  The problem for us in this module is that
	NULLIFYs do not work the same way as regular data at all.  They
	are part of a totally weirdo scheme to somehow to something
	that few people can understand.  I still do not know the scoop
	on the exact way in which they are supposed to work.  I only
	know that we have to forward all NULLIFYs since they do not
	"collapse" like other data transactions do.  We should be going
	back to the "original design" using the relay-data updates but
	we will continue with this NULLIFY madness for a little while
	longer !  Output stage FIFOs have to be put back into this code
	in order to forward all NULLIFYs that we may see on our primary
	bus.  We still do not have to forward NULLIFYs on the other
	buses that are snooped since we will leave that to the other
	REGFILTERs respectively.

	The normal rules :

	+ new datum on primary bus gets saved if it has a same or higher TT,
	  and then is forwarded if its datum is different than the current

	+ new data on other buses update data if they have a same or higher
	  TT

	+ never return a NULLIFY on a backwarding request (OK, reasonable)

	+ backward backwarding requests if we have invalid datum (obvious)

	The new "Ali" rules :

	+ all NULLIFYs on primary input-read bus are forwarded regardless
	  of TT

	+ all NULLIFYs snarfed change valid data to invalid if they have a
	  matching (only MATCHING) TT than the current data

	+ if we already have invalid data, we update its TT on
	  a greater valued TT on a received NULLIFY

	+ NULLIFYs form forwarding barriers !  all previous data that
	  needed to be forwarded is forwarded before the NULLIFY


	= 01/05/14, Dave Morano

	Oh man, this is getting hairy !  If we are at the bottom of a
	column and we get a NULLIFY that has a TT equal to or greater
	than our current register TT value, then initiate a backwarding
	request to see if a newer value than our own is out there some
	place !  It is possible to get a NULLIFY that does *NOT* match
	our current registered TT value and there *still* be a newer
	value out there in the machine someplace !


	= 01/05/14, Dave Morano

	I had to make a change to add one set of FIFOs for each input
	bus that is snooped.  This is needed because we can get more
	than one NULLIFY during the same clock.  They will come in on
	separate buses.  Yes, duh !!!


*/


/**************************************************************************

	This object module provides the function of the Forwarding
	Register Bus Filter hardware component in the Levo machine.

	Rules of operation:

	+ forward anything (stores or NULLIFYs) that needs forwarding
	that is snarfed on our primary input forwarding bus (only) ;
	**NOTE** currently, we are forwarding all NULLIFYs or other
	updates regardless of which bus they come in on !

	+ backward any NULLIFYs that are snarfed on our primary
	backwarding bus


	Coding notes: 

	Remember to track the use of LFLOWGROUP objects out-of-band !
	Some users of LFLOWGROUPS do not mark them as being used by
	setting the data present indications within the object.  A
	LFLOWGROUP may indeed be active even though its internal data-
	present indications say there is no data.  This does make some
	sense since there is other useful information in a LFLOWGROUP
	other than the traditional data portion.

	Initialization note:

	The forwarding bus and backwarding bus pointers provided to us
	MUST point to the actual start of the forwarding and
	backwarding bus object structure arrays.  This is especially
	necessary since we scan forwarding buses starting at our
	forwarding-bus-read-index and proceeding backwards for a
	forwarding-span amount of buses.  The backwarding bus pointer
	is provided similarly although we scan that only at the
	target object bus under the pointer.  This could change in
	the future if we want more scan connectivity in the register
	filter units (as it currently is -- I think -- with the ASes).

	Design note:

	We are NOT using the LBUSINT object to manage our buses for
	us.  Due to enormously hostile complaints about a possible
	clock delay in writing to the bus (when it is otherwise
	entirely free) we are going to try to manage our bus
	transactions ourselves.  I do not think that this gives any
	better performance but it may serve as a test-bed for future
	bus ideas.  The down side of not using a LBUSINT is a more
	complicated bus interface !  Reading buses is relatively
	straight forward but requires the allocation and small
	management of the bus read registers.  Writing buses is the
	real problem but we are trying to be cleaver about making
	requests for buses as soon as possible by making the request as
	soon as we decide to shift something into the internal register
	stores.  

	Design note:

	The direction indicator on the 'regfilter_readbuses()'
	subroutine is used to indicate the direction of the bus scan.
	A 'FALSE' value for this indicates that scanning should go
	backwards for forwarding buses.  A 'TRUE' value indicates that
	scanning should proceed forwards, like for backwarding buses.
	This is the same convention used in the ASes so we stay with
	that to avoid confusion.

	Operational note:

	Since we can get many new registers every clock, we need a way
	to get them forwarded given the very limited forwarding
	bandwidth.  We will set a bit associated with each architected
	register to store the fact that the register's Levo FlowGroup
	must be forwarded when forwarding bandwidth becomes available.
	Now that I think about it, we do not have to propagate a stall
	backwards as I was originally thinking.  This forwarding-bit
	seems to serve the purpose of cicumventing the need for
	propagating a stall backwards through the machine.  

	In order to avoid register starvation (whatever you want to call
	it) when our output forwarding bandwidth is totally maxed out
	all of the time, we will also store an incrementing time-stamp
	value indicating how far back in time a certain register update
	operation occurred.  We will use a policy of always forwarding
	the oldest updated values before forwarding any newly updated
	values that occurred in the current clock period.  We will reset
	the age-time of the register when that register FlowGroup is
	actually forwarded rather than when the register gets updated.

	Operational note:

	Alireza threw me a curve ball on Tuesday (00/12/05).  He is now
	forwarding NULLIFY transactions for registers !  When we
	receive a NULLIFY on one of the forwarding buses, we have to
	invalidate the associated register.

	If the register being NULLed was previously valid, we should
	forward a NULLIFY into the future.  If the register was not
	previously valid, can we do nothing ?  I think the answer is
	yes.

	Design note:

	This code CANNOT handle disjoint and out-of-phase execution
	paths.  That is hard for many aspects of the machine but it
	would mean that each register file operates independently for
	each path (something that we are not doing right now).  I
	suppose that an enhancement could be made in the future to
	handle each path independently.  Separate snooping time-tag
	values would have to be maintained for each path and they would
	also have to be properly updated as the machine shifts !

	Design note:

	There is a chance (maybe) that we can get several updates for a
	register in the same clock (on the different buses).  If this
	happens, we try to see if the real data or transaction has
	changed from the previous values.  If there is no change in
	either or both of those values, we cancel the request to
	forward the register.  This sort of thing cannot happen with
	backward requests since there is (currently) only one backwarding
	bus that is being snooped !

	There is also currently no provision for canceling a register
	operation of any kind over more than one clock if the data and
	transaction values do not change.  This can be an optimization
	for the future. :-)

	Operation note:

	When a REGFILTER is at the top of a column AND it is assigned
	the time-tag of '0', it should be configured (dynamically as it is)
	to snoop ALL backwarding buses.  This is necessary so that
	all backwarding requests will be satisfied "at the top" and
	won't be otherwise never satisfied.



**************************************************************************/


#define	REGFILTER_MASTER	1


#include	<sys/types.h>
#include	<climits>
#include	<cstdlib>
#include	<cstring>

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
#include	<cstdio>
#endif

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"bus.h"
#include	"regfilter.h"
#include	"lfgf.h"		/* Levo FlowGroup FIFO */

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	REGFILTER_MAGIC		0x12437498
#define	REGFILTER_MINTT		INT_MIN		/* minimum allowed TT value */
#define	REGFILTER_DISPLAYTT	-9999999



/* external subroutines */

extern int	seqok(uint,uint,uint) ;

#if	F_MASTERDEBUG && F_DEBUG
extern int	ctdeci(char *,int,int) ;
#endif


/* forward references */

static int regfilter_handleshift(REGFILTER *,struct proginfo *,int) ;
static int regfilter_readbuses(REGFILTER *,struct proginfo *,BUS *,int,
				int,int,int,int) ;
static int regfilter_checkgrants(REGFILTER *,struct proginfo *,LSIM *) ;
static int regfilter_lookup(REGFILTER *,struct proginfo *,LSIM *,
				struct levoinfo *) ;
static int regfilter_update(REGFILTER *,struct proginfo *,LSIM *,
				struct levoinfo *) ;
static int regfilter_goingempty(REGFILTER *,struct proginfo *) ;
static int regfilter_requests(REGFILTER *,struct proginfo *) ;
static int regfilter_fifoshifts(REGFILTER *,struct proginfo *) ;
static int regfilter_writebuses(REGFILTER *,struct proginfo *) ;
static int regfilter_checkholds(REGFILTER *,struct proginfo *,
				struct levoinfo *) ;
static int regfilter_holdbuses(REGFILTER *,struct proginfo *,struct levoinfo *,
				BUS *,int,int,int,int) ;
static int regfilter_calcdone(REGFILTER *,struct proginfo *) ;
static int regfilter_calcfifocount(REGFILTER *,struct proginfo *) ;

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
static int mkttbuf(char *,int) ;
#endif






int regfilter_init(rfp,pip,mip,lip,ap)
REGFILTER		*rfp ;
struct proginfo		*pip ;
LSIM			*mip ;
struct levoinfo		*lip ;
REGFILTER_INITARGS	*ap ;
{
	uint	path, addr ;

	int	rs = SR_OK ;
	int	i, j, size ;
	int	n ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if ((ap == NULL) || (pip == NULL) || (mip == NULL) || (lip == NULL))
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: entered\n") ;
#endif

	(void) memset(rfp,0,sizeof(REGFILTER)) ;

	rfp->magic = 0 ;

	rfp->pip = pip ;
	rfp->mip = mip ;
	rfp->lip = lip ;

	rfp->id = ap->id ;		/* our ID */

	rfp->rfbuses = ap->rfbuses ;	/* forward buses */
	rfp->rbbuses = ap->rbbuses ;	/* backward buses */
	rfp->ffsize = ap->ffsize ;	/* forward FIFO size */
	rfp->bfsize = ap->bfsize ;	/* backward FIFO size */
	rfp->fri = ap->fri ;		/* forward bus read index */
	rfp->fwi = ap->fwi ;		/* forward bus write index */
	rfp->bri = ap->bri ;		/* backward bus read index */
	rfp->bwi = ap->bwi ;		/* backward bus write index */
	rfp->fspan = MAX(ap->fspan,1) ;	/* forwarding bus span */
	rfp->bspan = MAX(ap->bspan,1) ;	/* backwarding bus span */

	rfp->c.tt = rfp->n.tt = ap->tt ;	/* our time-tag value */

	rfp->fbid = ap->fbid ;		/* bus master IDs */
	rfp->bbid = ap->bbid ;
	rfp->fpri = ap->fpri ;		/* bus priorities */
	rfp->bpri = ap->bpri ;

	rfp->minfree = ap->minfree ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    int	m = lip->nsg ;

	    eprintf(
	        "regfilter_init: id=%d fbmid=%d bbmid=%d fwi=%d bwi=%d\n",
	        rfp->id,rfp->fbid , rfp->bbid, rfp->fwi, rfp->bwi ) ;

	    j = rfp->fri ;
	    eprintf("regfilter_init: RF buses, fri=%d\n",j) ;
	    for (i = 0 ; i < rfp->fspan ; i += 1) {

	        eprintf("regfilter_init: RFB(%d)=%08lx\n",
	            j,(rfp->rfbuses + j)) ;

	        j = (j + 1 + m) % m ;
	    }

	    j = rfp->bri ;
	    eprintf("regfilter_init: RB buses, bri=%d\n",j) ;
	    for (i = 0 ; i < rfp->bspan ; i += 1) {

	        eprintf("regfilter_init: RBB(%d)=%08lx\n",
	            j,(rfp->rbbuses + j)) ;

	        j = (j - 1 + m) % m ;
	    }
	}
#endif /* F_DEBUG */

/* check some parameters */

	if (rfp->ffsize < REGFILTER_FSIZE)	/* forward FIFO size */
	    rfp->ffsize = REGFILTER_FSIZE ;

	if (rfp->bfsize < REGFILTER_FSIZE)	/* backward FIFO size */
	    rfp->bfsize = REGFILTER_FSIZE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("regfilter_init: ffsize=%d bfsize=%d\n",
	        rfp->ffsize,rfp->bfsize) ;
	}
#endif

	if (rfp->minfree < REGFILTER_MINFREE)
		rfp->minfree = REGFILTER_MINFREE ;


	rfp->f.shift = FALSE ;

/* what is the overflow time-tag value ? */

	rfp->ovtt = lip->nsg * lip->nasrpsg ;


/* allocate some read registers for reading the buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: allocating bus read registers\n") ;
#endif

	n = rfp->fspan + rfp->bspan ;	/* forwarding & backwarding buses */

	size = 2 * n * sizeof(struct regfilter_busreg) ;
	rs = uc_malloc(size,(void **) &rfp->a.brr) ;

	if (rs < 0)
	    goto bad0 ;

	rfp->c.brr = rfp->a.brr + 0 ;
	rfp->n.brr = rfp->a.brr + n ;

/* allocate the extra valid array */

	size = n * (sizeof(char)) ;
	rs = uc_malloc(size,(void **) &rfp->brr_v) ;

	if (rs < 0)
	    goto bad1 ;


/* allocate our machine architected registers */

	rfp->nregs = REGFILTER_NREGS * lip->npaths ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: allocating regs n=%d\n",rfp->nregs) ;
#endif

	size = 2 * rfp->nregs * sizeof(REGFILTER_REG) ;
	rs = uc_malloc(size,(void **) &rfp->a.regs) ;

	if (rs < 0)
	    goto bad2 ;

	rfp->c.regs = rfp->a.regs + 0 ;
	rfp->n.regs = rfp->a.regs + rfp->nregs ;


/* allocate and initialize our FIFO shift-in buffer and FIFOS */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: initing FIFO related stuff\n") ;
#endif

/* first the shift-in holding variables */

	size = (2 * rfp->fspan) * sizeof(struct regfilter_busreg) ;
	rs = uc_malloc(size,(void **) &rfp->fifoshifts) ;

	if (rs < 0)
	    goto bad3 ;

	(void) memset(rfp->fifoshifts,0,size) ;

/* now the actual FIFOs */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: F nregs=%d\n",n) ;
#endif

/* allocate them */

	n = (2 * rfp->fspan) ;

	size = n * sizeof(LFGF) ;
	rs = uc_malloc(size,(void **) &rfp->fifos) ;

	if (rs < 0)
	    goto bad4 ;

/* initialize them */

	for (i = 0 ; i < n ; i += 1) {

	    rs = lfgf_init(rfp->fifos + i,pip,mip,lip, rfp->ffsize) ;

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0) {

	    for (j = 0 ; j < i ; j += 1)
	        lfgf_free(rfp->fifos + j) ;

	} /* end if (error condition) */

	if (rs < 0)
	    goto bad5 ;

/* now the FIFO age registers */

	n = rfp->fspan ;
	size = 2 * n * sizeof(struct regfilter_fifoinfo) ;
	rs = uc_malloc(size,(void **) &rfp->a.fifoinfo) ;

	if (rs < 0)
	    goto bad6 ;

	(void) memset(rfp->a.fifoinfo,0,size) ;

	rfp->c.fifoinfo = rfp->a.fifoinfo + 0 ;
	rfp->n.fifoinfo = rfp->a.fifoinfo + n ;


/* initialize the allocated data structures, if necessary */

/* the machine architected registers */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: initializing ISA registers\n") ;
#endif

	for (path = 0 ; path < lip->npaths ; path += 1) {

	    for (addr = 0 ; addr < REGFILTER_NREGS ; addr += 1) {

	        i = (REGFILTER_NREGS * path) + addr ;

	        memset(rfp->c.regs + i,0,sizeof(REGFILTER_REG)) ;

	        memset(rfp->n.regs + i,0,sizeof(REGFILTER_REG)) ;

	        rfp->c.regs[i].btt = rfp->n.regs[i].btt = 0 ;
	        rfp->c.regs[i].fg.path = rfp->n.regs[i].fg.path = path ;
	        rfp->c.regs[i].fg.addr = rfp->n.regs[i].fg.addr = addr ;
	        rfp->c.regs[i].fg.tt = rfp->n.regs[i].fg.tt = 
	            REGFILTER_MINTT ;

/* mark **invlid** by default ! */

	        rfp->c.regs[i].fg.trans = rfp->n.regs[i].fg.trans = 
	            LFLOWGROUP_TNULLIFY ;

	    } /* end for (addresses) */

	} /* end for (paths) */

/* the bus read registers */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: initializing bus read registers\n") ;
#endif

	n = rfp->fspan + rfp->bspan ;	/* forwarding & backwarding buses */
	for (i = 0 ; i < n ; i += 1) {

	    memset(rfp->c.brr + i,0,sizeof(struct regfilter_busreg)) ;

	    memset(rfp->n.brr + i,0,sizeof(struct regfilter_busreg)) ;

	    rfp->c.brr[i].f_v = rfp->n.brr[i].f_v = FALSE ;

	} /* end for */


/* those little things that we forget in the night ! */

	rfp->c.donecount = rfp->n.donecount = 0 ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: optional checksum checkcalc\n") ;
#endif

#if	F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct regfilter_state),&rfp->c.checksum) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: exiting nromally rs=%d\n",rs) ;
#endif

	rfp->magic = REGFILTER_MAGIC ;
	return rs ;

/* bad things */
bad7:

#ifdef	COMMENT
	free(rfp->a.fifoinfo) ;
#endif

bad6:

	n = (2 * rfp->fspan) ;
	for (i = 0 ; i < n ; i += 1 )
	    (void) lfgf_free(rfp->fifos + i) ;

bad5:
	free(rfp->fifos) ;

bad4:
	free(rfp->fifoshifts) ;

bad3:
	free(rfp->a.regs) ;

bad2:
	(void) free(rfp->brr_v) ;

bad1:
	(void) free(rfp->a.brr) ;

bad0:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_init: exiting bad rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (regfilter_init) */


/* free up this object */
int regfilter_free(rfp)
REGFILTER	*rfp ;
{
	struct proginfo	*pip ;

	int	rs, i ;
	int	n ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_free: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: fifoinfo=%08lx\n",rfp->a.fifoinfo) ;
#endif

	free(rfp->a.fifoinfo) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: 1 fifos\n") ;
#endif

	n = (2 * rfp->fspan) ;
	for (i = 0 ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: i=%d lfgf_free()\n",i) ;
#endif

	    rs = lfgf_free(rfp->fifos + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: lfgf_free() rs=%d\n",rs) ;
#endif

	} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: 2 fifos=%08lx\n",rfp->fifos) ;
#endif

	free(rfp->fifos) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: regs=%08lx\n",rfp->a.regs) ;
#endif

	if (rfp->a.regs != NULL)
	    free(rfp->a.regs) ;

	rfp->c.regs = rfp->n.regs = NULL ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: brr_v=%08lx\n",rfp->brr_v) ;
#endif

	if (rfp->brr_v != NULL)
	    free(rfp->brr_v) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: brr=%08lx\n",rfp->a.brr) ;
#endif

	if (rfp->a.brr != NULL)
	    free(rfp->a.brr) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_free: exiting\n") ;
#endif

	rfp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (regfilter_free) */


/* perform the combinatorial computations */
int regfilter_comb(rfp,phase)
REGFILTER	*rfp ;
int		phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;
	int	i, j ;
	int	n ;
	int	size ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_comb: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	mip = rfp->mip ;
	lip = rfp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_comb: id=%d rftt=%d entered ck=%lld ph=%d\n",
	        rfp->id,rfp->c.tt,mip->clock,phase) ;
#endif

/* do our sub-objects */

	n = 2 * rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

	    rs = lfgf_comb(rfp->fifos + i,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: i=%d lfgf_comb() rs=%d\n", i,rs) ;
#endif

	    if (rs < 0)
	        return rs ;

	} /* end for */


/* do any debugging stuff */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	    LFLOWGROUP	fg ;

	    int	fi ;
	    int	fe ;

	    char	ttbuf[24] ;

	    n = rfp->fspan ;
	    for (i = 0 ; i < n ; i += 1) {

/* the data FIFO */

	        fi = (i * 2) + 0 ;
	        rs = lfgf_count(rfp->fifos + fi) ;

	        if (rs > 0) {

	            eprintf(
			"regfilter_comb: bi=%dD count=%d f_pfd=%d age=%d\n",
	                i,rs,
	                rfp->c.fifoinfo[i].f_fpd,rfp->c.fifoinfo[i].age) ;

	            for (fe = 0 ; 
			(rs = lfgf_enum(rfp->fifos + fi,fe,&fg)) >= 0 ; 
	                fe += 1) {

	                mkttbuf(ttbuf,fg.tt) ;

	                eprintf(
	                    "regfilter_comb: fe=%d FIFO tr=%d p=%d "
				"tt=%s:%d a=%d "
	                    "dp=%d dv=%08x\n",
	                    fe,
	                    fg.trans,fg.path,
	                    ttbuf,fg.seq,
	                    fg.addr,
	                    (fg.dp != 0),
	                    fg.dv) ;

	            } /* end for */

	        } /* end if */

/* the NULLIFY FIFO */

	        fi = (i * 2) + 1 ;
	        rs = lfgf_count(rfp->fifos + fi) ;

	        if (rs > 0) {

	            eprintf(
			"regfilter_comb: bi=%dN count=%d f_pfd=%d age=%d\n",
	                i,rs,
	                rfp->c.fifoinfo[i].f_fpd,rfp->c.fifoinfo[i].age) ;

	            for (fe = 0 ; 
			(rs = lfgf_enum(rfp->fifos + fi,fe,&fg)) >= 0 ; 
	                fe += 1) {

	                mkttbuf(ttbuf,fg.tt) ;

	                eprintf(
	                    "regfilter_comb: fe=%d FIFO tr=%d p=%d "
				"tt=%s:%d a=%d "
	                    "dp=%d dv=%08x\n",
	                    fe,
	                    fg.trans,fg.path,
	                    ttbuf,fg.seq,
	                    fg.addr,
	                    (fg.dp != 0),
	                    fg.dv) ;

	            } /* end for */

	        } /* end if */

	    } /* end for */

	    rs = SR_OK ;
	}
#endif /* F_DEBUG */

/* do our own combinatorial logic */

	switch (phase) {

	case 0:
	    rfp->f.shift = FALSE ;
	    rfp->f.fwdregs = FALSE ;
	    rfp->f.fwdfifo = FALSE ;
	    rfp->f.backward = FALSE ;
	    rfp->f.settt = FALSE ;
	    rfp->f.goingempty = FALSE ;

	    n = 2 * rfp->fspan ;
	    for (i = 0 ; i < n ; i += 1)
	        rfp->fifoshifts[i].f_v = FALSE ;

/* set the default next state for the bus read registers */

	    n = rfp->fspan + rfp->bspan ;	/* all of the buses */
	    for (i = 0 ; i < n ; i += 1)
	        rfp->n.brr[i].f_v = FALSE ;

/* the default is for the next state to be the same as the current state */

/* copy architected register file */

	    size = rfp->nregs * sizeof(REGFILTER_REG) ;
	    (void) memcpy(rfp->n.regs,rfp->c.regs,size) ;

/* copy output FIFO information */

	    size = rfp->fspan * sizeof(struct regfilter_fifoinfo) ;
	    (void) memcpy(rfp->n.fifoinfo,rfp->c.fifoinfo,size) ;


/* print out some defaults */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	    	LFLOWGROUP	*fgp ;

		int	a = 31 ;

	    char	ttbuf[24] ;

			fgp = &rfp->c.regs[a].fg ;
	                mkttbuf(ttbuf,fgp->tt) ;
		
	                eprintf(
	                    "regfilter_comb: existing C tr=%d p=%d "
				"tt=%s:%d a=%d "
	                    "dp=%d dv=%08x\n",
	                    fgp->trans,fgp->path,
	                    ttbuf, fgp->seq,
	                    fgp->addr,
	                    fgp->dp,
	                    fgp->dv) ;

			fgp = &rfp->n.regs[a].fg ;
	                mkttbuf(ttbuf,fgp->tt) ;
		
	                eprintf(
	                    "regfilter_comb: existing N tr=%d p=%d "
				"tt=%s:%d a=%d "
	                    "dp=%d dv=%08x\n",
	                    fgp->trans,fgp->path,
	                    ttbuf, fgp->seq,
	                    fgp->addr,
	                    fgp->dp,
	                    fgp->dv) ;

	    }
#endif /* F_DEBUG */


/* this update stuff must be in phase 0 so that it can request the bus */

/* clear all next-state forwarding requests (used for same-clock indication) */

	    for (i = 0 ; i < rfp->nregs ; i += 1) {

	        rfp->n.regs[i].f.forward = FALSE ;
	        rfp->n.regs[i].f.backward = FALSE ;

	    } /* end for */


/* hack for possible use in other arrangements of phase use */

	    rfp->fifocount = 0 ;


/* do the calculation for commitment-ready */

	    rs = regfilter_calcdone(rfp,pip) ;

	    if (rs < 0)
	        return rs ;


/* process the data from backwarding buses */

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_lookup()\n") ;
#endif

	    rs = regfilter_lookup(rfp,pip,mip,lip) ;

	    if (rs < 0)
	        break ;

/* process the data from forwarding buses */

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_update()\n") ;
#endif

	    rs = regfilter_update(rfp,pip,mip,lip) ;

	    if (rs < 0)
	        break ;

/* make bus requests if necessary */

	    rs = regfilter_goingempty(rfp,pip) ;

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_requests()\n") ;
#endif

	    rs = regfilter_requests(rfp,pip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_requests() rs=%d\n",rs) ;
#endif

	    break ;

/* phase 1 */
	case 1:

/* write to any buses that we are currently master for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_writebuses()\n") ;
#endif

	    rs = regfilter_writebuses(rfp,pip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_writebuses() rs=%d\n",rs) ;
#endif

/* do we need some bus holds ? */

	    rs = regfilter_checkholds(rfp,pip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_checkholds() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

/* do any FIFO shift-ins that we need */

	    rs = regfilter_fifoshifts(rfp,pip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_fifoshifts() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

	    break ;

/* phase 2, check for bus grants, read the buses */
	case 2:

/* check for bus grants */

	    rs = regfilter_checkgrants(rfp,pip,mip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_comb: regfilter_checkgrants() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

/* read buses */

	    {

/* read the forwarding buses */

	        if (rfp->c.tt > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf(
	                    "regfilter_comb: RF regfilter_readbuses() "
	                    "fri=%d fspan=%d\n",
	                    rfp->fri,rfp->fspan) ;
#endif

	            rs = regfilter_readbuses(rfp,pip,rfp->rfbuses,lip->nsg,
	                rfp->fri,rfp->fspan, TRUE,0) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf("regfilter_comb: regfilter_readbuses() rs=%d\n",
	                    rs) ;
#endif

	            if (rs < 0)
	                break ;

	        } /* end if (we are not at the top) */


/* read all of the backwarding buses available to us (our span's worth) */

	        if (rfp->c.tt < rfp->ovtt) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	                eprintf(
	                    "regfilter_comb: RB regfilter_readbuses() "
	                    "bri=%d bspan=%d\n",
	                    rfp->bri,rfp->bspan) ;
	                eprintf("regfilter_comb: f_scanall=%d \n",
	                    rfp->c.f.scanall) ;
	            }
#endif /* F_DEBUG */

	            n = (rfp->c.f.scanall) ? rfp->bspan : 1 ;
	            rs = regfilter_readbuses(rfp,pip,rfp->rbbuses,lip->nsg,
	                rfp->bri,n, FALSE,rfp->fspan) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	                eprintf(
	                    "regfilter_comb: regfilter_readbuses() rs=%d\n",
	                    rs) ;
	            }
#endif

	            if (rs < 0)
	                break ;

	        } /* end if (we were not at the bottom of the machine) */

	    } /* end block (reading buses) */

	    break ;

/* handle machine shifts in this last phase */
	case 3:
	    rs = regfilter_handleshift(rfp,pip,lip->totalrows) ;

	    break ;

	} /* end switch */

/* we're out of here ! */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_comb: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (regfilter_comb) */


/* handle a clock transition */
int regfilter_clock(rfp)
REGFILTER	*rfp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;
	int	n, size ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_clock: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;


/* transition our subobjects */

	n = 2 * rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

	    rs = lfgf_clock(rfp->fifos + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_clock: i=%d lfgf_clock() rs=%d\n", i,rs) ;
#endif

	    if (rs < 0)
	        return rs ;

	} /* end for */

/* transition ourselves */

#if	F_SAFE
	rs = d_checkverify(&rfp->c,sizeof(struct regfilter_state),
	    &rfp->c.checksum) ;

	if (rs < 0) {

	    eprintf("regfilter_clock: REGFILTER=%08lx check bad format\n",
	        rfp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */

/* we are swapping pointers here */

	{
	    struct regfilter_busreg	*brp ;

	    REGFILTER_REG	*rp ;

	    struct regfilter_fifoinfo	*fip ;


	    brp = rfp->c.brr ;
	    rp = rfp->c.regs ;		/* save allocated pointer */
	    fip = rfp->c.fifoinfo ;

	    rfp->c = rfp->n ;		/* copy everything */

	    rfp->n.brr = brp ;
	    rfp->n.regs = rp ;		/* swap in allocated pointer */
	    rfp->n.fifoinfo = fip ;

	} /* end block */


#if	F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct regfilter_state),&rfp->c.checksum) ;
#endif


	return rs ;
}
/* end subroutine (regfilter_clock) */


/* shift the machine (called in phase 1) */
int regfilter_shift(rfp)
REGFILTER	*rfp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;
	int	n ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_shift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;


/* signal our subobjects */

	n = 2 * rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

	    rs = lfgf_shift(rfp->fifos + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_shift: FR lfgf_shift() rs=%d\n", rs) ;
#endif

	if (rs < 0)
		break ;

	} /* end for */


/* signal ourself */

	rfp->f.shift = TRUE ;

	return rs ;
}
/* end subroutine (regfilter_shift) */


/* should we scan all backwarding buses or just our primary one */
int regfilter_scanall(rfp,f_on)
REGFILTER	*rfp ;
int		f_on ;
{
	struct proginfo	*pip ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_scanall: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_scanall: id=%d rftt=%d f=%d\n",
	        rfp->id,rfp->c.tt,f_on) ;
#endif

	rfp->n.f.scanall = ((f_on) ? TRUE : FALSE) ;

	return SR_OK ;
}
/* end subroutine (regfilter_scanall) */


/* allow the caller to initialize a register value */
int regfilter_initval(rfp,path,addr,val)
REGFILTER	*rfp ;
uint		path, addr ;
uint		val ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	ri ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_initval: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	lip = rfp->lip ;

#if	F_SAFE2
	if (path >= lip->npaths)
		return SR_INVALID ;
#endif

	ri = (lip->npaths * path) + addr ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("regfilter_initval: REGFILTER=%08lx npaths=%d\n",
	        rfp,lip->npaths) ;
	    eprintf("regfilter_initval: p=%d a=%d dv=%08x\n",
	        path,addr,val) ;
	}
#endif

	if ((ri < 0) || (ri >= rfp->nregs))
	    return SR_INVALID ;

/* set the normal next state */

	rfp->n.regs[ri].fg.ftt = rfp->c.tt ;
	rfp->n.regs[ri].fg.trans = LFLOWGROUP_TSTORE ;
	rfp->n.regs[ri].fg.path = path ;
	rfp->n.regs[ri].fg.addr = addr ;
	rfp->n.regs[ri].fg.tt = -1 ;
	rfp->n.regs[ri].fg.seq = 0 ;
	rfp->n.regs[ri].fg.dp = LFLOWGROUP_DPALL ;
	rfp->n.regs[ri].fg.dv = val ;

/* set the current state also ? */

	rfp->c.regs[ri].fg = rfp->n.regs[ri].fg ;

#if	F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct regfilter_state),&rfp->c.checksum) ;
#endif


	return SR_OK ;
}
/* end subroutine (regfilter_initval) */


/* allow the caller to set a register value */
int regfilter_setval(rfp,path,addr,val)
REGFILTER	*rfp ;
uint		path, addr ;
uint		val ;
{
	struct levoinfo	*lip ;

	int	ri ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_setval: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	lip = rfp->lip ;

	ri = (lip->npaths * path) + addr ;

	if ((ri < 0) || (ri >= rfp->nregs))
	    return SR_INVALID ;

/* set the normal next state */

	rfp->n.regs[ri].fg.ftt = rfp->c.tt ;
	rfp->n.regs[ri].fg.path = path ;
	rfp->n.regs[ri].fg.addr = addr ;
	rfp->n.regs[ri].fg.trans = LFLOWGROUP_TSTORE ;
	rfp->n.regs[ri].fg.dv = val ;
	rfp->n.regs[ri].fg.dp = LFLOWGROUP_DPALL ;

/* should we set the current state also ? */

#if	F_SETCURVAL
	rfp->c.regs[ri].fg.trans = LFLOWGROUP_TSTORE ;
	rfp->c.regs[ri].fg.dv = val ;
	rfp->c.regs[ri].fg.dp = LFLOWGROUP_DPALL ;

#if	F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct regfilter_state),&rfp->c.checksum) ;
#endif

#endif /* F_SETCURVAL */

	return SR_OK ;
}
/* end subroutine (regfilter_setval) */


/* get a register value */

/* Returns :
	<0	bad something
	0	data returned OK
	1	invalid
	2	no_data_present

*/

int regfilter_getval(rfp,path,addr,vp)
REGFILTER	*rfp ;
uint		path, addr ;
uint		*vp ;
{
	struct levoinfo	*lip ;

	int	rs, ri ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_getval: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	lip = rfp->lip ;

	ri = (lip->npaths * path) + addr ;

	if ((ri < 0) || (ri >= rfp->nregs))
	    return SR_INVALID ;

/* set the value */

	*vp = rfp->c.regs[ri].fg.dv ;

	rs = (rfp->c.regs[ri].fg.trans == LFLOWGROUP_TNULLIFY) ? 1 : 0 ;
	if (rs == 0)
	    rs = (rfp->c.regs[ri].fg.dp == 0) ? 2 : 0 ;

	return rs ;
}
/* end subroutine (regfilter_getval) */


/* get a register value in the form of a flow-group */

/* Returns :
	<0	bad something
	0	data returned OK
	1	invalid
	2	no_data_present

*/

int regfilter_getfg(rfp,path,addr,fgp)
REGFILTER	*rfp ;
uint		path, addr ;
LFLOWGROUP	*fgp ;
{
	struct levoinfo	*lip ;

	int	rs, ri ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_getval: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	lip = rfp->lip ;

	ri = (lip->npaths * path) + addr ;

	if ((ri < 0) || (ri >= rfp->nregs))
	    return SR_INVALID ;

/* set the value */

	if (fgp != NULL)
	    *fgp = rfp->c.regs[ri].fg ;

	rs = (rfp->c.regs[ri].fg.trans == LFLOWGROUP_TNULLIFY) ? 1 : 0 ;
	if (rs == 0)
	    rs = (rfp->c.regs[ri].fg.dp == 0) ? 2 : 0 ;

	return rs ;
}
/* end subroutine (regfilter_getfg) */


/* invalidate a path (or all paths) */
int regfilter_invalidate(rfp,path)
REGFILTER	*rfp ;
uint		path ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i ;
	int	ri ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_invalidate: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	lip = rfp->lip ;

	if ((path != (~ 0)) && (path >= lip->npaths)) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_invalidate: bad path=%d\n",path) ;
#endif

	    return SR_INVALID ;
	}

	if (path == (~ 0)) {

	    for (ri = 0 ; ri < rfp->nregs ; ri += 1) {

	        rfp->n.regs[ri].fg.trans = LFLOWGROUP_TNULLIFY ;
	        rfp->n.regs[ri].fg.tt = REGFILTER_MINTT ;
	        rfp->n.regs[ri].fg.dp = LFLOWGROUP_DPNONE ;
	        rfp->n.regs[ri].fage = 0 ;
	        rfp->n.regs[ri].bage = 0 ;
	        rfp->n.regs[ri].f.forward = FALSE ;
	        rfp->n.regs[ri].f.backward = FALSE ;
	        rfp->n.regs[ri].f.brout = FALSE ;

	    }

	} else {

	    ri = lip->npaths * path ;
	    for (i = 0 ; i < REGFILTER_NREGS ; i += 1) {

	        rfp->n.regs[ri].fg.trans = LFLOWGROUP_TNULLIFY ;
	        rfp->n.regs[ri].fg.tt = REGFILTER_MINTT ;
	        rfp->n.regs[ri].fg.dp = LFLOWGROUP_DPNONE ;
	        rfp->n.regs[ri].fage = 0 ;
	        rfp->n.regs[ri].bage = 0 ;
	        rfp->n.regs[ri].f.forward = FALSE ;
	        rfp->n.regs[ri].f.backward = FALSE ;
	        rfp->n.regs[ri].f.brout = FALSE ;

	        ri += 1 ;

	    } /* end for */

	}

	return rs ;
}
/* end subroutine (regfilter_invalidate) */


/* set the time-tag value for this REGFILTER */
int regfilter_settt(rfp,tt)
REGFILTER	*rfp ;
int		tt ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_settt: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_settt: next tt=%d\n",tt) ;
#endif

#ifdef	COMMENT
	rfp->c.tt = tt ;		/* just set next state ?! */
#endif

	rfp->n.tt = tt ;

	rfp->f.settt = TRUE ;
	return SR_OK ;
}
/* end subroutine (regfilter_settt) */


/* get the current time-tag for this REGFILTER */
int regfilter_gettt(rfp,ttp)
REGFILTER	*rfp ;
int		*ttp ;
{
	struct levoinfo	*lip ;

	int	ri ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_gettt: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (ttp == NULL)
	    return SR_FAULT ;

	*ttp = rfp->c.tt ;
	return SR_OK ;
}
/* end subroutine (regfilter_gettt) */


/* copy a path worth's of registers from some other unit to this one */
int regfilter_pathcopy(rfp,rfp2,path)
REGFILTER	*rfp ;
REGFILTER	*rfp2 ;
int		path ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;
	int	sri, ri ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_pathcopy: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;

	if (rfp2 == NULL)
	    return SR_FAULT ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	lip = rfp->lip ;

#if	F_SAFE2
	if (path >= lip->npaths) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_pathcopy: bad path=%d\n",path) ;
#endif

	    return SR_INVALID ;
	}
#endif /* F_SAFE2 */

	sri = REGFILTER_NREGS * path ;
	for (ri = sri ; ri < (sri + REGFILTER_NREGS) ; ri += 1) {

	    rfp->n.regs[ri] = rfp2->c.regs[ri] ;

	    rfp->n.regs[ri].frseq = 
			(rfp2->c.regs[ri].fg.seq + 1) % lip->rfmod ;

	    rfp->n.regs[ri].f.new = TRUE ;

	} /* end for */

	return rs ;
}
/* end subroutine (regfilter_pathcopy) */


/* update the registers from another file to this one */
int regfilter_merge(rfp,rfp2)
REGFILTER	*rfp ;
REGFILTER	*rfp2 ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;
	int	ri ;
	int	path, addr ;
	int	f_invalid ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_merge: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;

	if (rfp2 == NULL)
	    return SR_FAULT ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	mip = rfp->mip ;
	lip = rfp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("regfilter_merge: id=%d rftt=%d from id=%d\n",
	        rfp->id,rfp->c.tt,
		(rfp2 - (rfp - rfp->id))) ;
	}
#endif

	for (ri = 0 ; ri < rfp->nregs ; ri += 1) {

		int	f ;

#if	F_MASTERDEBUG && F_DEBUG
		int	f_seq = FALSE ;
#endif


	    f_invalid = (rfp->n.regs[ri].fg.dp == 0) ||
	        (rfp->n.regs[ri].fg.trans == LFLOWGROUP_TNULLIFY) ;

		f = f_invalid ||
	        (rfp2->c.regs[ri].fg.tt > rfp->n.regs[ri].fg.tt) ;

		if ((! f) &&
			(rfp2->c.regs[ri].fg.tt == rfp->n.regs[ri].fg.tt)) {

	            f = seqok(rfp2->c.regs[ri].fg.seq,rfp->n.regs[ri].frseq,
			lip->rfmod) ;

#if	F_MASTERDEBUG && F_DEBUG
			f_seq = f ;
	        if (f_seq && (pip->debuglevel >= 5)) {
		eprintf("regfilter_merge: rfmod=%d new seq=%d old frseq=%d\n",
			lip->rfmod,
	            rfp2->c.regs[ri].fg.seq,rfp->n.regs[ri].frseq) ;
		}
#endif

		} /* end if (equal time-tags) */

		if (f) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	 	    char	ttbuf[24] ;


	            path = ri / REGFILTER_NREGS ;
	            addr = ri % REGFILTER_NREGS ;
	                mkttbuf(ttbuf,rfp2->c.regs[ri].fg.tt) ;

		eprintf("regfilter_merge: f_dp=%d f_iv=%d f_tt=%d f_seq=%d\n",
	    		(rfp->n.regs[ri].fg.dp == 0),
			f_invalid,
	        	(rfp2->c.regs[ri].fg.tt > rfp->n.regs[ri].fg.tt),
			f_seq) ;

	                mkttbuf(ttbuf,rfp->n.regs[ri].fg.tt) ;

	            eprintf(
	                "regfilter_merge: exiting p=%d tt=%s:%d a=%d dv=%08x\n",
	                rfp->n.regs[ri].fg.path,
	                ttbuf,rfp->n.regs[ri].fg.seq,
	                rfp->n.regs[ri].fg.addr ,
	                rfp->n.regs[ri].fg.dv) ;

	                mkttbuf(ttbuf,rfp2->c.regs[ri].fg.tt) ;

	            eprintf(
	                "regfilter_merge: new p=%d tt=%s:%d a=%d dv=%08x\n",
	                rfp2->c.regs[ri].fg.path,
	                ttbuf,rfp2->c.regs[ri].fg.seq,
	                rfp2->c.regs[ri].fg.addr ,
	                rfp2->c.regs[ri].fg.dv) ;

	        }
#endif /* F_DEBUG */

	        rfp->n.regs[ri].fg = rfp2->c.regs[ri].fg ;

	        rfp->n.regs[ri].frseq = 
			(rfp2->c.regs[ri].fg.seq + 1) % lip->rfmod ;

	        rfp->n.regs[ri].f.backward = FALSE ;
	        rfp->n.regs[ri].f.forward = TRUE ;

#if	F_MERGEFWD
	        rfp->n.regs[ri].fage += 1 ;
#else
	        rfp->n.regs[ri].fage += rfp2->c.regs[ri].fage ;
#endif

	        rfp->n.regs[ri].bage = 0 ;

		rfp->n.regs[ri].clock = mip->clock ;

	    } /* end if (needed merge) */

	} /* end for (looping over ML registers) */


	return rs ;
}
/* end subroutine (regfilter_udpate) */


/* is this REGFILTER ready to commit ? */

/* Returns :
	<0	error
	0	not ready to commit
	1	ready to commit
*/

int regfilter_readycommit(rfp)
REGFILTER	*rfp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;
	int	count ;
	int	n, i ;
	int	f_forward, f_backward ;
	int	f_ready ;


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != REGFILTER_MAGIC) && (rfp->magic != 0)) {

	    eprintf("regfilter_readycommit: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != REGFILTER_MAGIC)
	    return SR_NOTOPEN ;

#endif /* F_SAFE */

	pip = rfp->pip ;
	lip = rfp->lip ;


/* check the most likely blocking conditions first */

	f_forward = rfp->c.f.frequested || rfp->c.f.fownbus ;
	f_backward = rfp->c.f.brequested || rfp->c.f.bownbus ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf(
	        "regfilter_readycommit: rftt=%d f_forward=%d f_backward=%d\n",
	        rfp->c.tt,f_forward,f_backward) ;

	    if (f_forward)
	        eprintf("regfilter_readycommit: freq=%d fown=%d\n",
	            rfp->c.f.frequested,rfp->c.f.fownbus) ;

	    if (f_backward)
	        eprintf("regfilter_readycommit: breq=%d bown=%d\n",
	            rfp->c.f.brequested,rfp->c.f.bownbus) ;

	}
#endif /* F_EBUG */

	if (f_forward || f_backward)
	    return 0 ;


/* I do not believe the indication above, so we check REGs individually */

	f_ready = TRUE ;
	{
	    int	path = LEVOINFO_PATHML ;
	    int	addr ;


	    for (addr = 0 ; addr < REGFILTER_NREGS ; addr += 1) {

	        i = (REGFILTER_NREGS * path) + addr ;

	        if (rfp->c.regs[i].fage > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf(
	                    "regfilter_readycommit: forward a=%d\n",
	                    addr) ;
#endif

	            f_ready = FALSE ;
	            break ;
	        }

	    } /* end for */

	} /* end block */

	if (! f_ready) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf(
	            "regfilter_readycommit: bad thing\n") ;
#endif

	    return 0 ;
	}


/* check some FIFO butt also ! for safety */

	rs = regfilter_calcfifocount(rfp,pip) ;

	if (rs < 0)
	    return rs ;

	count = rs ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_readycommit: FIFO maxcount=%d\n",
	        count) ;
#endif

	if (count > 0)
	    return 0 ;


#ifdef	COMMENT /* do not check these as they may be irrelavant ! */

/* what about the bus read registers ? */

	if (rfp->c.tt > 0) {

	    f_ready = TRUE ;
	    n = rfp->fspan + rfp->bspan ;
	    for (i = 0 ; i < n ; i += 1) {

	        if (rfp->c.brr[i].f_v) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	                LFLOWGROUP	*fgp ;

	                eprintf(
	                    "regfilter_readycommit: BRR has some at i=%d\n",
	                    i) ;
	                fgp = &rfp->c.brr[i].fg ;
	                eprintf("regfilter_readycommit: rftt=%d a=%d\n",
	                    rfp->c.tt,fgp->addr) ;
	            }
#endif /* F_DEBUG */

	            f_ready = FALSE ;
	            break ;
	        }

	    } /* end for */

	    if (! f_ready)
	        return 0 ;

	} /* end if (we were not at the top of the machine) */

#endif /* COMMENT */

/* done count */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_readycommit: donecount=%d\n",
	        rfp->c.donecount) ;
#endif

	if (rfp->c.donecount > 0)
	    return 0 ;


/* what about the invalid registers if we are at bottom column ? */

#if	F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_readycommit: tentative ready commit\n") ;
#endif

	rs = 1 ;
	f_ready = TRUE ;
	if ((rfp->c.tt > 0) && ((rfp->c.tt % lip->totalrows) == 0)) {

	    uint	path = LEVOINFO_PATHML ;
	    uint	addr ;


	    for (addr = 0 ; addr < REGFILTER_NREGS ; addr += 1) {

	        i = (REGFILTER_NREGS * path) + addr ;

	        if (rfp->c.regs[i].fg.trans != LFLOWGROUP_TSTORE) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf(
	                    "regfilter_readycommit: invalid a=%d\n",
	                    addr) ;
#endif

	            f_ready = FALSE ;
	            break ;
	        }

	    } /* end for */

	} /* end if (bottom of column) */

/* check again because I found it hard to believe you the first time ! */

	if (f_ready &&
		(rfp->c.tt > 0) && ((rfp->c.tt % lip->totalrows) == 0)) {

	    uint	path = LEVOINFO_PATHML ;
	    uint	addr ;


	    for (addr = 0 ; addr < REGFILTER_NREGS ; addr += 1) {

	        i = (REGFILTER_NREGS * path) + addr ;

	        if (rfp->c.regs[i].f.brout) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf(
	                    "regfilter_readycommit: brout a=%d\n",
	                    addr) ;
#endif

	            f_ready = FALSE ;
	            break ;
	        }

	    } /* end for */

	} /* end if (bottom of column) */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_readycommit: exiting rs=%d\n",
	        ((rs >= 0) ? f_ready : rs)) ;
#endif

	return (rs >= 0) ? f_ready : rs ;
}
/* end subroutine (regfilter_readycommit) */



/* PRIVATE SUBROUTINES */



/* read a set of buses into the bus read-registers */
static int regfilter_readbuses(rfp,pip,buses,m,ri,span,f_up,si)
REGFILTER	*rfp ;
struct proginfo	*pip ;
BUS		*buses ;
int		ri ;			/* bus read index */
int		span ;			/* bus read span */
int		f_up ;			/* bus scan direction, up=T down=F */
int		si ;			/* bus-read-register store index */
{
	LFLOWGROUP	*fgp ;

	int	rs = SR_OK ;
	int	i, j ;

#if	F_MASTERDEBUG && F_DEBUG
	char	ttbuf[24] ;
#endif


/* read all read buses and see if there is anything there for us */

	j = ri ;
	for (i = 0 ; i < span ; i += 1) {

	    fgp = &rfp->n.brr[si].fg ;
	    rfp->n.brr[si].f_v = FALSE ;
	    rs = bus_read(buses + j,fgp) ;

	    if (rs > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	            eprintf(
	                "regfilter_readbuses: id=%d rftt=%d V BUS(%d)=%08lx\n",
	                rfp->id,rfp->c.tt,j,(buses + j)) ;

	            mkttbuf(ttbuf,fgp->tt) ;

	            eprintf(
	                "regfilter_readbuses: ftt=%d tr=%d p=%d tt=%s:%d a=%d "
	                "dp=%d dv=%08x\n",
	                fgp->ftt,
	                fgp->trans,fgp->path,
	                ttbuf,
	                fgp->seq,
	                fgp->addr,
	                ((fgp->dp) ? 1 : 0),
	                fgp->dv) ;

	            eprintf(
	                "regfilter_readbuses: brr=%d\n",si) ;
	        }
#endif /* F_DEBUG */

	        rfp->n.brr[si].f_v = TRUE ;

	    } /* end if (we had something on the bus) */

	    si += 1 ;
	    j = (j + ((f_up) ? 1 : -1) + m) % m ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (regfilter_readbuses) */


/* calculate if we are going to be FIFO-empty after this clock */
static int regfilter_goingempty(rfp,pip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
{
	LFLOWGROUP	fg ;

	int	rs = SR_OK, i ;
	int	n, fi ;
	int	f_toomany ;


	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("regfilter_goingempty: f_fownbus=%d\n",
	        rfp->c.f.fownbus) ;
	}
#endif

	f_toomany = FALSE ;
	if (rfp->c.f.fownbus) {

	    f_toomany = TRUE ;

	    n = rfp->fspan ;
	    for (i = 0 ; i < n ; i += 1) {

	        fi = (i * 2) ;
	        rs = lfgf_count(rfp->fifos + fi + 1) ;

	        if (rs < 0)
	            return rs ;

	        if (rs > 1) {

	            f_toomany = TRUE ;
	            break ;

	        } else if (rs == 1) {

	            rs = lfgf_read(rfp->fifos + fi + 0,&fg) ;

	            if (rs < 0)
	                return rs ;

	            if ((fg.dp != 0) && (! rfp->c.fifoinfo[i].f_fpd)) {

	                f_toomany = TRUE ;
	                break ;

	            }

	        }

	    } /* end for */

	    rfp->f.goingempty = (! f_toomany) ;

	} /* end if (we own the bus) */

	return (rs >= 0) ? f_toomany : rs ;
}
/* end subroutine (regfilter_goingempty) */


/* write the output bus */
static int regfilter_writebuses(rfp,pip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
{
	LFLOWGROUP	fg, *fgp ;

	int	rs, i ;
	int	fi ;
	int	maxcount, maxbi ;
	int	f ;

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	char	ttbuf[24] ;
#endif


	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("regfilter_writebuses: id=%d rftt=%d "
		"f_fownbus=%d f_fwdv=%d\n",
	            rfp->id,
	        rfp->c.tt,
	        rfp->c.f.fownbus,rfp->c.fwd.f_v) ;
	    eprintf("regfilter_writebuses: f_bownbus=%d f_bwdv=%d\n",
	        rfp->c.f.bownbus,rfp->c.bwd.f_v) ;
	}
#endif /* F_DEBUG */


/* forwarding bus */

	if (rfp->c.f.fownbus) {

	    int	f_gotone = FALSE ;


/* find what we are supposed to write out to the bus ! */

	    if (rfp->fifocount == 0) {

	        rs = regfilter_calcfifocount(rfp,pip) ;

	        if (rs < 0)
	            return rs ;

	    } /* end if (getting FIFO maxcount) */

	    if (rfp->fifocount > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_writebuses: FIFO\n") ;
#endif

/* were any FIFOs already started ? */

	        for (i = 0 ; i < rfp->fspan ; i += 1) {

	            if (rfp->c.fifoinfo[i].f_fpd)
	                break ;

	        } /* end for */

	        if (i >= rfp->fspan) {

/* find which FIFO we should pay attention to (let's consider FIFO depth) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf(
		"regfilter_writebuses: finding a new FIFO to read\n") ;
#endif

	            maxcount = 0 ;
	            maxbi = 0 ;
	            for (i = 0 ; i < rfp->fspan ; i += 1) {

	                fi = (i * 2) + 1 ;
	                rs = lfgf_count(rfp->fifos + fi) ;

	                if (rs < 0)
	                    return rs ;

	                if (rs > maxcount) {

	                    maxbi = i ;
	                    maxcount = rs ;

	                }

	            } /* end for (looping over buses) */

	            i = maxbi ;

	        } /* end if */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_writebuses: FIFO set i=%d\n",i) ;
#endif

	        fi = (i * 2) ;
	        rs = lfgf_read(rfp->fifos + fi + 0,&fg) ;

	        if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf(
	                    "regfilter_writebuses: bad lfgf_read() rs=%d\n",
	                    rs) ;
#endif

	            return rs ;
	        }

	        if (fg.dp == 0) {

	            rs = lfgf_readout(rfp->fifos + fi + 0,&fg) ;

	            rs = lfgf_readout(rfp->fifos + fi + 1,&fg) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_writebuses: doing FIFO NULL 1\n") ;
#endif

	        } else if (rfp->c.fifoinfo[i].f_fpd) {

	            rs = lfgf_readout(rfp->fifos + fi + 0,&fg) ;

	            rs = lfgf_readout(rfp->fifos + fi + 1,&fg) ;

	            rfp->n.fifoinfo[i].f_fpd = FALSE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_writebuses: doing FIFO NULL 2\n") ;
#endif

	        } else {

	            rfp->n.fifoinfo[i].f_fpd = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_writebuses: doing FIFO DATA\n") ;
#endif

	        }

	        f_gotone = TRUE ;
	        fgp = &fg ;

	    } /* end if (something was in the FIFO) */

	    if (! f_gotone) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_writebuses: doing REG\n") ;
#endif

	        f_gotone = rfp->c.fwd.f_v ;
	        fgp = &rfp->c.fwd.fg ;

	    } /* end if (we got our thing from the FWD register) */

	    if (f_gotone) {

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	        mkttbuf(ttbuf,fgp->tt) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	            eprintf(
	                "regfilter_writebuses: rftt=%d "
			"F writing BUS=%p\n",
	                rfp->c.tt,rfp->rfbuses + rfp->fwi) ;

	            eprintf(
	                "regfilter_writebuses: tr=%d tt=%s:%d a=%d "
	                "dp=%d dv=%08x\n",
	                fgp->trans,
	                ttbuf,
	                fgp->seq,
	                fgp->addr,
	                (fgp->dp != 0),
	                fgp->dv) ;

	        }
#endif /* F_DEBUG */

#if	F_FPRINTF
	        fprintf(stdout,
	            "regfilter_writebuses: id=%d rftt=%d F "
	            "tr=%d p=%d tt=%s:%d a=%d "
	            "dp=%d dv=%08x\n",
	            rfp->id,
	            rfp->c.tt,
	            fgp->trans,
	            fgp->path,
	            ttbuf,
	            fgp->seq,
	            fgp->addr,
	            (fgp->dp != 0),
	            fgp->dv) ;
#endif /* F_PRINTF */

	        rs = bus_writecur(rfp->rfbuses + rfp->fwi,fgp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf(
	                "regfilter_writebuses: F bus_writecur() rs=%d\n",
	                rs) ;
#endif

	    } /* end if (something to write) */

	} /* end if (writing the forwarding bus) */


/* backwarding bus */

	if (rfp->c.f.bownbus && rfp->c.bwd.f_v) {

	    fgp = &rfp->c.bwd.fg ;

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	    mkttbuf(ttbuf,fgp->tt) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	            eprintf(
	                "regfilter_writebuses: rftt=%d "
			"B writing BUS=%p\n",
	                rfp->c.tt,rfp->rbbuses + rfp->bwi) ;

	            eprintf(
	                "regfilter_writebuses: tr=%d tt=%s:%d a=%d "
	                "dp=%d dv=%08x\n",
	                fgp->trans,
	                ttbuf,
	                fgp->seq,
	                fgp->addr,
	                (fgp->dp != 0),
	                fgp->dv) ;

	    }
#endif /* F_DEBUG */

#if	F_FPRINTF
	    fprintf(stdout,
	        "regfilter_writebuses: id=%d rftt=%d B "
	        "tr=%d p=%d tt=%s:%d a=%d "
	        "dp=%d dv=%08x\n",
	        rfp->id,
	        rfp->c.tt,
	        fgp->trans,
	        fgp->path,
	        ttbuf,
	        fgp->seq,
	        fgp->addr,
	        fgp->dp,
	        fgp->dv) ;
#endif /* F_FPRINTF */

	    rs = bus_writecur(rfp->rbbuses + rfp->bwi,&rfp->c.bwd.fg) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_writebuses: B bus_writecur() rs=%d\n",
	            rs) ;
#endif

	} /* end if (writing the backwarding bus) */


	return SR_OK ;
}
/* end subroutine (regfilter_writebuses) */


/* do we have a machine shift */
static int regfilter_handleshift(rfp,pip,totalrows)
REGFILTER	*rfp ;
struct proginfo	*pip ;
int		totalrows ;
{
	int	rs = SR_OK, i ;
	int	n ;
	int	tt_test ;


	if (! rfp->f.shift)
	    return rs ;

	rfp->f.shift = FALSE ;
	tt_test = REGFILTER_MINTT + totalrows ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("regfilter_handleshift: rftt=%d trows=%d\n",
	        rfp->c.tt,totalrows) ;
	}
#endif


/* shift the machine architected registers */

	for (i = 0 ; i < rfp->nregs ; i += 1) {

	    if (rfp->n.regs[i].fg.tt >= tt_test)
	        rfp->n.regs[i].fg.tt -= totalrows ;

	    if (rfp->n.regs[i].btt >= tt_test)
	        rfp->n.regs[i].btt -= totalrows ;

	} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	    int	j, k ;

	    char	ttbuf[24] ;


	    i = 0 ;
	    for (k = 0 ; k < 4 ; k += 1) {
	        eprintf("regfilter_handleshift:") ;
	        for (j = 0 ; j < 8 ; j += 1) {

	            mkttbuf(ttbuf, rfp->n.regs[i].fg.tt) ;

	            eprintf(" %4s", ttbuf) ;

	            i += 1 ;
	        }
	        eprintf("\n") ;
	    }
	}
#endif /* F_DEBUG */

/* shift the bus read-registers */

	n = rfp->fspan + rfp->bspan ;	/* forwarding & backwarding buses */
	for (i = 0 ; i < n ; i += 1) {

	    if (rfp->n.brr[i].f_v) {

	        if (rfp->n.brr[i].fg.ftt >= tt_test)
	            rfp->n.brr[i].fg.ftt -= totalrows ;

	        if (rfp->n.brr[i].fg.tt >= tt_test)
	            rfp->n.brr[i].fg.tt -= totalrows ;

	        if (rfp->n.brr[i].fg.rtt >= tt_test)
	            rfp->n.brr[i].fg.rtt -= totalrows ;

	    }

	} /* end for (looping through the bus-read registers) */


/* shift the bus write-registers */

	if (rfp->n.fwd.f_v) {

	    if (rfp->n.fwd.fg.ftt >= tt_test)
	        rfp->n.fwd.fg.ftt -= totalrows ;

	    if (rfp->n.fwd.fg.tt >= tt_test)
	        rfp->n.fwd.fg.tt -= totalrows ;

	    if (rfp->n.fwd.fg.rtt >= tt_test)
	        rfp->n.fwd.fg.rtt -= totalrows ;

	}

	if (rfp->n.bwd.f_v) {

	    if (rfp->n.bwd.fg.ftt >= tt_test)
	        rfp->n.bwd.fg.ftt -= totalrows ;

	    if (rfp->n.bwd.fg.tt >= tt_test)
	        rfp->n.bwd.fg.tt -= totalrows ;

	    if (rfp->n.bwd.fg.rtt >= tt_test)
	        rfp->n.bwd.fg.rtt -= totalrows ;

	}


/* shift our snooping TT value */

	if (rfp->f.settt) {

	    if (rfp->n.tt >= tt_test)
	        rfp->n.tt = rfp->n.tt - totalrows ;

	} else {

	    if (rfp->c.tt >= tt_test)
	        rfp->n.tt = rfp->c.tt - totalrows ;

	}


	return rs ;
}
/* end subroutine (regfilter_handleshift) */


/* process all backwarding requests */

/****

	If we get a request for a register and we hold an invalid entry
	for that register, then We propagate backwards a request for
	that register.  If we get a request and we have a valid
	register, then we just forward that register value.

****/

static int regfilter_lookup(rfp,pip,mip,lip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
{
	REGFILTER_REG	*rp ;

	LFLOWGROUP	*fgp ;

	int	rs = SR_OK, i ;
	int	ri ;			/* register file index */
	int	n ;
	int	f_invalid ;

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	char	ttbuf[24] ;
#endif


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_lookup: id=%d rftt=%d bri=%d\n",
	        rfp->id,rfp->c.tt,rfp->fspan) ;
#endif

/* make sure that we do not respond to requests if we are not supposed to ! */

	if ((rfp->c.tt < 0) || (rfp->c.tt >= rfp->ovtt))
	    return rs ;

/* process the backward bus data if any */

	n = rfp->fspan + rfp->bspan ;
	for (i = rfp->fspan ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_lookup: BRR i=%d\n",i) ;
#endif

	    fgp = &rfp->c.brr[i].fg ;
	    if (rfp->c.brr[i].f_v && 
	        (fgp->ftt >= rfp->c.tt) && (fgp->tt >= rfp->c.tt)) {

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	        mkttbuf(ttbuf,fgp->tt) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	            eprintf(
	                "regfilter_lookup: id=%d rftt=%d "
			"tr=%d p=%d tt=%s a=%d dp=%d dv=%08x\n",
	                rfp->id,
	                rfp->c.tt,
	                fgp->trans,
	                fgp->path,
	                ttbuf,
	                fgp->addr,
	                fgp->dp,
	                fgp->dv) ;

	        }
#endif /* F_DEBUG */

#if	F_FPRINTF
	        fprintf(stdout,
	            "regfilter_lookup: id=%d rftt=%d tr=%d p=%d tt=%s a=%d "
	            "dp=%d dv=%08x\n",
	            rfp->id,
	            rfp->c.tt,
	            fgp->trans,
	            fgp->path,
	            ttbuf,
	            fgp->addr,
	            fgp->dp,
	            fgp->dv) ;
#endif /* F_PRINTF */

/* do a sanity check on the path */

#if	F_SAFE2
	        if (fgp->path >= lip->npaths) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf("regfilter_lookup: bad path=%d\n",
	                    fgp->path) ;
#endif /* F_DEBUG */

	            rs = SR_INVALID ;
	            break ;

	        } /* end if (path sanity check) */
#endif /* F_SAFE2 */

/* do a sanity check on the register address */

	        if (fgp->addr >= REGFILTER_NREGS) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf("regfilter_lookup: bad addr=%d\n",
	                    fgp->addr) ;
#endif /* F_DEBUG */

	            rs = SR_INVALID ;
	            break ;

	        } /* end if (path sanity check) */

	        ri = (lip->npaths * fgp->path) + fgp->addr ;
	        rp = rfp->n.regs + ri ;

	        f_invalid = (! rfp->c.regs[ri].fg.dp) ||
	            (rfp->c.regs[ri].fg.trans == LFLOWGROUP_TNULLIFY) ;

/* handle this lookup according to whether we have a valid value or not */

	        if (f_invalid) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf("regfilter_lookup: reg=%d invalid\n",ri) ;
#endif

	            if (fgp->tt > rp->btt)
	                rp->btt = fgp->tt ;

	            if (rfp->c.tt > 0) {

	                rp->f.brout = TRUE ;
	                rp->f.backward = TRUE ;
	                rp->bage = rfp->c.regs[ri].bage + 1 ;

	                rfp->f.backward = TRUE ;

	            } /* end if (backward actually needed) */

	        } else {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

			mkttbuf(ttbuf,rp->fg.tt) ;

	                eprintf(
	                    "regfilter_lookup: need FWD existing "
	                    "tt=%s:%d a=%d dv=%08x ck=%lld\n",
				ttbuf, rp->fg.seq,
	                    rp->fg.addr,rp->fg.dv,
				rp->clock) ;
	            }
#endif /* F_DEBUG */

	            rp->f.forward = TRUE ;

	            rp->fage = rfp->c.regs[ri].fage + 1 ;

		    rp->fg.f_resp = TRUE ;

	            rfp->f.fwdregs = TRUE ;

	        } /* end if (we had a valid value or not) */

	    } /* end if (we had some snoopable data on the bus) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_lookup: scanall exit check\n") ;
#endif

	    if ((! rfp->c.f.scanall) || (rfp->c.tt != 0))
	        break ;

	} /* end for (looping through the backward bus data) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_lookup: exiting rs=%d f=%d b=%d\n",
	        rs,rfp->f.fwdregs,rfp->f.backward) ;
#endif

	return rs ;
}
/* end subroutine (regfilter_lookup) */


/* process all read data to see if they should update the register file */

/****

	Note that NULLIFY transactions are handled just like a normal
	transaction.  If it is a change from what we had already,
	we forward it.  

	Note on above statement :  Wrong !!  Ali informs me that
	NULLIFYs are a whole new ball-game and that they work nothing
	like one would imagine !  Everything is off.

****/

static int regfilter_update(rfp,pip,mip,lip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
{
	REGFILTER_REG	*crp, *rp ;

	LFLOWGROUP	*fgp ;

	int	rs = SR_OK, i, j ;
	int	n, ri ;

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	char	ttbuf[24] ;
#endif


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_update: entered id=%d rftt=%d\n",
	        rfp->id,rfp->c.tt) ;
#endif

/* make sure that we do not do any updates if we are not supposed to ! */

	if (rfp->c.tt <= 0)
	    return rs ;

/* perform some space filtering up front with priority decoding */

/* remove the structurally improper transactions */

	n = rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf(
		    "regfilter_update: id=%d rftt=%d P brr=%d\n",
	            rfp->id,rfp->c.tt,i) ;
#endif

	    fgp = &rfp->c.brr[i].fg ;
	    rfp->brr_v[i] = (rfp->c.brr[i].f_v && 
	        (fgp->ftt < rfp->c.tt) && (fgp->tt < rfp->c.tt)) ;


#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	    mkttbuf(ttbuf,fgp->tt) ;

#ifdef	COMMENT
	if (pip->debuglevel >= 5)
		eprintf("regfilter_update: ftt=%d tt=%s:%d\n",
			fgp->ftt,ttbuf,fgp->seq) ;
#endif
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 5) && rfp->brr_v[i]) {

	        eprintf(
	            "regfilter_update: ftt=%d tr=%d p=%d "
			"tt=%s:%d a=%d dp=%d dv=%08x\n",
	            fgp->ftt,
	            fgp->trans,
	            fgp->path,
	            ttbuf,
	            fgp->seq,
	            fgp->addr,
	            fgp->dp,
	            fgp->dv) ;

	    }
#endif /* F_DEBUG */

#if	(F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)) && 0
	    if (rfp->brr_v[i]) {

	        fprintf(stdout,
	            "regfilter_update: id=%d rftt=%d tr=%d p=%d "
	            "tt=%s:%d a=%d dp=%d dv=%08x\n",
	            rfp->id,rfp->c.tt,
	            fgp->trans,
	            fgp->path,
	            ttbuf,
	            fgp->seq,
	            fgp->addr,
	            fgp->dp,
	            fgp->dv) ;

	    }
#endif /* F_FPRINTF */

	} /* end for (removing structurally improper transactions) */

/* perform the sequence checking on like-transactions */

	n = rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

	    if (rfp->brr_v[i]) {

	        LFLOWGROUP	*tfgp ;

	        uint	path, addr ;

		int	tt ;
	        int	besti, maxseq ;


	        fgp = &rfp->c.brr[i].fg ;
	        path = fgp->path ;
	        addr = fgp->addr ;
	        tt = fgp->tt ;

	        besti = i ;
	        maxseq = fgp->seq ;
	        for (j = (i + 1) ; j < n ; j += 1) {

	            if (rfp->brr_v[j]) {

	                tfgp = &rfp->c.brr[j].fg ;
	                if ((tfgp->path == path) &&
	                    (tfgp->addr == addr) &&
	                    (tfgp->tt == tt) &&
	                    (tfgp->seq > maxseq)) {

	                    besti = j ;
	                    maxseq = tfgp->seq ;

	                }

	            } /* end if */

	        } /* end for */

	        for (j = i ; j < n ; j += 1) {

	            if (rfp->brr_v[j]) {

	                tfgp = &rfp->c.brr[j].fg ;
	                if ((tfgp->path == path) &&
	                    (tfgp->addr == addr) &&
	                    (tfgp->tt == tt)) {

	                    if (j != besti) {

	                        rfp->brr_v[j] = FALSE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	                            mkttbuf(ttbuf,tfgp->tt) ;

	                            eprintf(
	                                "regfilter_update: W=%d ftt=%d "
	                                "tr=%d p=%d tt=%s:%d a=%d "
	                                "dp=%d dv=%08x\n",
	                                j,
	                                tfgp->ftt,
	                                tfgp->trans,
	                                tfgp->path,
	                                ttbuf,
	                                tfgp->seq,
	                                tfgp->addr,
	                                tfgp->dp,
	                                tfgp->dv) ;

	                        }
#endif /* F_DEBUG */

	                    } /* end if (one got zapped) */

	                } /* end if */

	            } /* end if */

	        } /* end for */

	    } /* end if */

	} /* end for */

/* process the forwarding bus related data */

	n = rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_update: id=%d rftt=%d brr=%d\n",
	            rfp->id,rfp->c.tt,i) ;
#endif

	    if (rfp->brr_v[i]) {

	        int	f_nullify ;
	        int	f_invalid ;
	        int	f_nodata ;
	        int	f ;


	        fgp = &rfp->c.brr[i].fg ;

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	        mkttbuf(ttbuf,fgp->tt) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	            eprintf(
	                "regfilter_update: p ftt=%d "
	                "tr=%d p=%d tt=%s:%d a=%d "
	                "dp=%d dv=%08x\n",
	                fgp->ftt,
	                fgp->trans,
	                fgp->path,
	                ttbuf,
	                fgp->seq,
	                fgp->addr,
	                ((fgp->dp) ? 1 : 0),
	                fgp->dv) ;

	        }
#endif /* F_DEBUG */

#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)
	        fprintf(stdout,
	            "regfilter_update: id=%d rftt=%d tr=%d p=%d "
	            "tt=%s:%d a=%d "
	            "dp=%d dv=%08x\n",
	            rfp->id,rfp->c.tt,
	            fgp->trans,
	            fgp->path,
	            ttbuf,
	            fgp->seq,
	            fgp->addr,
	            ((fgp->dp) ? 1 : 0),
	            fgp->dv) ;
#endif /* F_FPRINTF */

/* do a sanity check on the path */

#if	F_SAFE2
	        if (fgp->path >= lip->npaths) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf("regfilter_update: bad path=%d\n",
	                    fgp->path) ;
#endif /* F_DEBUG */

	            rs = SR_INVALID ;
	            break ;

	        } /* end if (path sanity check) */
#endif /* F_SAFE2 */

/* do a sanity check on the register address */

	        if (fgp->addr >= REGFILTER_NREGS) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf("regfilter_update: bad addr=%d\n",
	                    fgp->addr) ;
#endif /* F_DEBUG */

	            rs = SR_INVALID ;
	            break ;

	        } /* end if (path sanity check) */

	        ri = (lip->npaths * fgp->path) + fgp->addr ;
	        rp = rfp->n.regs + ri ;

/* calculate some flags for later use */

	        f_invalid = 
	            (rfp->n.regs[ri].fg.trans == LFLOWGROUP_TNULLIFY) ;
	        f_nodata = (rfp->n.regs[ri].fg.dp == 0) ;
	        f_nullify = (fgp->trans == LFLOWGROUP_TNULLIFY) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	        mkttbuf(ttbuf,
	                rfp->n.regs[ri].fg.tt) ;

	            eprintf(
	                "regfilter_update: f_invalid=%d "
	                "f_brout=%d frseq=%d\n",
	                f_invalid,rp->f.brout,rp->frseq) ;

	            eprintf(
	                "regfilter_update: existing tt=%s:%d "
	                "dp=%d dv=%08x\n",
	                ttbuf,
	                rfp->n.regs[ri].fg.seq,
	                (rfp->n.regs[ri].fg.dp != 0),
	                rfp->n.regs[ri].fg.dv) ;
	        }
#endif /* F_DEBUG */

/* go through the various cases */

	        if (f_nullify) {

	            int	f_killed = FALSE ;
	            int	f_greater = FALSE ;
	            int	f_hadfwd = FALSE ;


/* we received a NULLIFY */

	            if (f_invalid) {

/* we currently have invalid data */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf(
	                        "regfilter_update: NULL w/ invalid\n") ;
#endif

	                f = (fgp->tt >= rp->fg.tt) ;

	                if (fgp->tt == rp->fg.tt)
	                    f = seqok(fgp->seq,rp->frseq,lip->rfmod) ;

	                if (f) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                        eprintf(
	                            "regfilter_update: updating TT\n") ;
#endif

	                    rp->fg.tt = fgp->tt ;
	                    rp->fg.seq = fgp->seq ;

	                    rp->frseq = (fgp->seq + 1) % lip->rfmod ;

	                } /* end if */

	            } else {

/* we have valid data, invalidate us only if the TTs match exactly ! */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf(
	                        "regfilter_update: NULL w/ valid"
	                        " old_tt=%d new_tt=%d\n",
	                        rp->fg.tt,fgp->tt) ;
#endif

	                f = TRUE ;
	                f = f && (fgp->tt == rp->fg.tt) ;

	                f = f && seqok(fgp->seq,rp->frseq,lip->rfmod) ;

	                if (f) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                        eprintf(
	                            "regfilter_update: invalidate\n") ;
#endif

	                    rp->fg = *fgp ;
	                    rp->frseq = (fgp->seq + 1) % lip->rfmod ;

	                    f_killed = TRUE ;

/* cancel any forwarding that may have been in progress */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                        eprintf(
	                            "regfilter_update: f_fwd=%d fage=%d\n",
	                            rp->f.forward,rp->fage) ;
#endif

	                    if (rp->f.forward || (rp->fage > 0)) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                            eprintf(
	                                "regfilter_update: cancel FWD\n") ;
#endif

	                        rp->f.forward = FALSE ;
	                        rp->fage = 0 ;
	                        f_hadfwd = TRUE ;	/* special request */

	                    }

	                } else if (fgp->tt > rp->fg.tt) {

#if	F_EXTRABROUT
	                    f_greater = TRUE ;
#else
	                    f_greater = FALSE ;
#endif

	                } /* end if (possibly our current data) */

	            } /* end if (we had invalid or valid data) */

/* forward if necessary */

#if	F_EXTRANULL
	            f = TRUE ;
#else
	            f = (i == 0) ;
#endif

			f = f && (fgp->tt != rfp->c.regs[i].fg.tt) ;

#ifdef	WHATIS
			f = f && (fgp->seq != rfp->c.regs[i].fg.seq) ;
#endif

	            if (f && (rfp->c.tt < rfp->ovtt)) {

	                LFLOWGROUP	tfg ;

	                int	fi ;

			int	f_prenull ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf(
	                        "regfilter_update: need to forward\n") ;
#endif

	                fi = (i * 2) ;

#if	F_PRENULL
			f_prenull = TRUE ;
#else
			f_prenull = FALSE ;
#endif

/* do we have a current (previous) data value that needs forwarding at all ? */

	                crp = &rfp->c.regs[ri] ;
	                if ((! f_invalid) && f_prenull &&
	                    (crp->fage > 0) && (crp->fg.dp != 0)) {

/* shift in the value that needs forwarding first */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                        eprintf(
	                            "regfilter_update: S => FIFO tr=%d p=%d"
	                            " tt=%d a=%d dp=%d dv=%08x\n",
	                            crp->fg.trans,crp->fg.path,
	                            crp->fg.tt, crp->fg.addr,
	                            (crp->fg.dp != 0),crp->fg.dv) ;
#endif

#if	F_SHIFTDELAY
	                    rfp->fifoshifts[fi].fg = crp->fg ;
	                    rfp->fifoshifts[fi].fg.ftt = rfp->c.tt ;
	                    rfp->fifoshifts[fi].f_v = TRUE ;
#else

	                    tfg = crp->fg ;
	                    tfg.ftt = rfp->c.tt ;
	                    rs = lfgf_write(rfp->fifos + fi + 0,&tfg) ;
#endif /* F_SHIFTDELAY */

	                    rp->f.forward = FALSE ;
	                    rp->fage = 0 ;

	                } else {

/* shift in a junk place-holder value */

	                    tfg = rp->fg ;
	                    tfg.ftt = rfp->c.tt ;
	                    tfg.dp = LFLOWGROUP_DPNONE ;

#if	F_SHIFTDELAY
	                    rfp->fifoshifts[fi].fg = tfg ;
	                    rfp->fifoshifts[fi].f_v = TRUE ;
#else

	                    rs = lfgf_write(rfp->fifos + fi + 0,&tfg) ;
#endif /* F_SHIFTDELAY */

	                } /* end if */

/* finally, schedule the forwarding of the NULLIFY */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf(
	                        "regfilter_update: N => FIFO tr=%d p=%d"
	                        " tt=%d a=%d dp=%d dv=%08x\n",
	                        fgp->trans,fgp->path,
	                        fgp->tt, fgp->addr,
	                        (fgp->dp != 0),fgp->dv) ;
#endif

#if	F_SHIFTDELAY
	                rfp->fifoshifts[fi + 1].fg = *fgp ;
	                rfp->fifoshifts[fi + 1].fg.ftt = rfp->c.tt ;

	                rfp->fifoshifts[fi + 1].f_v = TRUE ;
#else

	                fgp->ftt = rfp->c.tt ;
	                rs = lfgf_write(rfp->fifos + fi + 1,fgp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf(
	                        "regfilter_update: lfgf_write() rs=%d\n",
	                        rs) ;
#endif

#endif /* F_SHIFTDELAY */

	                rfp->f.fwdfifo = TRUE ;

	            } /* end if (needed a forward) */

/* do we need a backward ? */

	            if ((f_killed || f_greater) && 
	                (rfp->c.tt > 0) && 
	                (f_hadfwd || ((rfp->c.tt % lip->totalrows) == 0))) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf(
	                        "regfilter_update: need BWD\n") ;
#endif

	                if (fgp->tt > rp->btt)
	                    rp->btt = fgp->tt ;

	                rp->f.backward = TRUE ;
	                rp->f.brout = TRUE ;
	                rp->bage = rfp->c.regs[ri].bage + 1 ;

	                rfp->f.backward = TRUE ;

	            } /* end if (backward actually needed) */

	        } else {

	            f = f_invalid ;
	            if (f_invalid) {

	                if (fgp->tt == rp->fg.tt)
	                    f = seqok(fgp->seq,rp->frseq,lip->rfmod) ;

	            } else {

	                f = (fgp->tt >= rp->fg.tt) ;

#ifdef	COMMENT
	                if (f && (fgp->tt == rp->fg.tt) &&
			    (fgp->seq == rp->fg.seq))
	                    rp->f.brout = FALSE ;
#endif /* COMMENT (what is this ?) */

	                if (fgp->tt == rp->fg.tt) {

	                    f = seqok(fgp->seq,rp->frseq,lip->rfmod) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                        eprintf(
	                            "regfilter_update: extrasnoop f=%d\n",
	                            f) ;
#endif

	                    if ((! f) && (fgp->seq == rp->fg.seq)) {

				f = rp->fg.f_resp ;
				if (! f)
					f = rp->f.new ;

			    }
					
	                }

	            } /* end if */

	            if (f) {

	                int	f_datachange ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	                    eprintf(
	                        "regfilter_update: updating TT\n") ;
	                    eprintf(
	                        "regfilter_update: a=%d dp=%d dv=%08x\n",
	                        fgp->addr,
	                        ((fgp->dp) ? 1 : 0),
	                        fgp->dv) ;
	                }
#endif /* F_DEBUG */

/* update our stored TT value on this register (is this OK ?) */

	                rp->fg.tt = fgp->tt ;
	                rp->fg.seq = fgp->seq ;

	                rp->frseq = (fgp->seq + 1) % lip->rfmod ;

/* do we have a data change ? */

	                f_datachange = FALSE ;
	                if ((! f_invalid) && (fgp->dp != 0)) {

#if	F_EXTRADATA
			f_datachange = TRUE ;
#else
	                    f_datachange = (rp->fg.dp == 0) ;
	                    if (! f_datachange)
	                        f_datachange = (fgp->dv != rp->fg.dv) ;
#endif /* F_EXTRADATA */

	                } /* end if (data change) */

	                if (f_invalid || f_datachange || rp->f.brout ||
				rp->f.new || rp->fg.f_resp) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                        eprintf(
	                            "regfilter_update: updating register"
	                            " f_datachange=%d \n",
	                            f_datachange) ;
#endif

	                    rp->fg = *fgp ;
				rp->clock = mip->clock ;

/* should we forward ? */

	                    if ((rp->f.brout || (i == 0)) && 
	                        (rfp->c.tt < rfp->ovtt)) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                            eprintf(
	                                "regfilter_update: need FWD\n") ;
#endif

				rp->f.new = FALSE ;
	                        rp->f.forward = TRUE ;

	                        rp->fage = rfp->c.regs[ri].fage + 1 ;

	                        rfp->f.fwdregs = TRUE ;

	                    } else {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                            eprintf(
	                                "regfilter_update: cancel FWD\n") ;
#endif

	                        rp->f.forward = FALSE ;

	                        rp->fage = 0 ;

	                        rfp->f.fwdregs = FALSE ;

	                    }

	                } /* end if (value changed) */

	                rp->f.brout = FALSE ;

	            } /* end if (a register TT may have changed) */

	        } /* end if (NULLIFY or regular data) */

	    } /* end if (we had some valid data on the bus) */

	} /* end for (looping through the forwarding buses) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf(
	        "regfilter_update: exiting rs=%d "
	        "f_fwdregs=%d f_fwdfifo=%d\n",
	        rs,rfp->f.fwdregs,rfp->f.fwdfifo) ;
#endif

	return ((rs >= 0) ? (rfp->f.fwdregs || rfp->f.fwdfifo) : rs) ;
}
/* end subroutine (regfilter_update) */


/* make any necessary bus requests */
static int regfilter_requests(rfp,pip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
{
	REGFILTER_REG	*rp ;

	int	rs = SR_OK, i ;
	int	p, a ;
	int	f_forward = FALSE ;
	int	f_backward = FALSE ;
	int	f_datachange, f_newdata, f_curdata, f_requests ;
	int	f_invalid ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf(
	        "regfilter_requests: id=%d rftt=%d f_fwdregs=%d f_fwdfifo=%d "
	        "f_bwd=%d\n",
		rfp->id,
	        rfp->c.tt,rfp->f.fwdregs, rfp->f.fwdfifo,rfp->f.backward) ;
#endif


/* loop through all registers, looking for something !? */

	for (i = 0 ; i < rfp->nregs ; i += 1) {

/* age any registers that need it ! */

	    if (rfp->n.regs[i].fage > 0)
	        rfp->n.regs[i].fage = rfp->c.regs[i].fage + 1 ;

	    if (rfp->n.regs[i].bage > 0)
	        rfp->n.regs[i].bage = rfp->c.regs[i].bage + 1 ;

#if	F_CANCELFWD /* seems inconsistent with NULLIFY requirements */

/* filter false positive forward operations that we may have just created */

	    if (rfp->n.regs[i].f.forward && (rfp->c.regs[i].fage == 0)) {

	        f_newdata = (rfp->n.regs[i].fg.dp != 0) ;
	        f_curdata = (rfp->c.regs[i].fg.dp != 0) ;
	        f_datachange =
	            (rfp->n.regs[i].fg.dv != rfp->c.regs[i].fg.dv) ;
	        f_invalid = 
	            (rfp->c.regs[i].fg.trans == LFLOWGROUP_TNULLIFY) ;

	        if ((! f_invalid) && f_curdata &&
	            ((! f_newdata) || (! f_datachange))) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	                eprintf(
	                    "regfilter_requests: cancelling forward i=%d\n",
	                    i) ;
	                eprintf(
	                    "regfilter_requests: f_newdata=%d \n",
	                    f_newdata) ;
	                eprintf(
	                    "regfilter_requests: f_datachange=%d\n",
	                    f_datachange) ;
	            }
#endif /* F_DEBUG */

	            rfp->n.regs[i].f.forward = FALSE ;
	            rfp->n.regs[i].fage = 0 ;

	        } /* end if (canceled the forward) */

	    } /* end if (forward cancel check) */

#endif /* F_CANCELFWD */

/* check for needed forward */

	    if ((rfp->c.tt < rfp->ovtt) && (rfp->n.regs[i].fage > 0)) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	            p = i / REGFILTER_NREGS ;
	            a = i % REGFILTER_NREGS ;
	            if (p != rfp->c.regs[i].fg.path)
	                eprintf(
	                    "regfilter_requests: i=%d bad path p=%d fg_p=%d\n",
	                    i,p,rfp->c.regs[i].fg.path) ;

	            if (a != rfp->c.regs[i].fg.addr)
	                eprintf(
	                    "regfilter_requests: i=%d bad addr a=%d fg_a=%d\n",
	                    i,a,rfp->c.regs[i].fg.addr) ;

	            eprintf(
	                "regfilter_requests: i=%d age=%d FWD "
	                "tr=%d p=%d tt=%d a=%d dp=%d dv=%08x\n",
	                i,
	                rfp->n.regs[i].fage,
	                rfp->n.regs[i].fg.trans,
	                p,
	                rfp->n.regs[i].fg.tt,
	                a,
	                (rfp->n.regs[i].fg.dp != 0),
	                rfp->n.regs[i].fg.dv) ;
	        }
#endif /* F_DEBUG */

	        f_forward = TRUE ;

	    } /* end if */

/* check for needed backward */

	    if (rfp->n.regs[i].bage > 0) {

	        f_backward = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_requests: B age-request i=%d"
			" a=%d dv=%08x dp=%d bage=%d\n",
			i,
			rfp->n.regs[i].fg.addr,
			rfp->n.regs[i].fg.dv,
			rfp->n.regs[i].fg.dp,
			rfp->n.regs[i].bage) ;
#endif

	    } /* end if (backward request needed) */

	} /* end for (looping through registers) */


/* OK, what about the forwarding FIFO ? */

	if (! f_forward)
	    f_forward = rfp->f.fwdfifo ;

	if ((! f_forward) && (! rfp->f.goingempty) && 
	    (rfp->c.tt < rfp->ovtt)) {

	    if (rfp->fifocount == 0) {

	        rs = regfilter_calcfifocount(rfp,pip) ;

	        if (rs < 0)
	            return rs ;

	    }

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_requests: rftt=%d FIFO maxcount=%d\n",
	            rfp->c.tt,rs) ;
#endif

	    if (rfp->fifocount > 0)
	        f_forward = TRUE ;

	} /* end if (an extra chance to forward something) */


/* do we have to forward anything ? */

	if (f_forward && (! rfp->c.f.frequested)) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf(
	            "regfilter_requests: F bus request bmid=%d pri=%d\n",
	            rfp->fbid,rfp->fpri) ;
#endif

	    rfp->n.f.frequested = TRUE ;
	    rs = bus_request(rfp->rfbuses + rfp->fwi,rfp->fbid,rfp->fpri) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_requests: F bus_request() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        return rs ;

	} /* end if (forwarding) */


/* do we have to backward anything ? */

	if (f_backward && (! rfp->c.f.brequested)) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf(
	            "regfilter_requests: B bus request bmid=%d pri=%d\n",
	            rfp->fbid,rfp->fpri) ;
#endif

	    rfp->n.f.brequested = TRUE ;
	    rs = bus_request(rfp->rbbuses + rfp->bwi,rfp->bbid,rfp->bpri) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_requests: B bus_request() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        return rs ;

	} /* end if (backwarding) */


	f_requests = rfp->n.f.frequested || rfp->n.f.brequested ;

	return ((rs >= 0) ? f_requests : rs) ;
}
/* end subroutine (regfilter_requests) */


/* make any necessary FIFO shift-ins */
static int regfilter_fifoshifts(rfp,pip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
{
	int	rs = SR_OK, i ;
	int	n ;


/* do we need to shift-in any data ? */

	n = 2 * rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

	    if (rfp->fifoshifts[i].f_v) {

	        rfp->fifoshifts[i].fg.ftt = rfp->c.tt ;
	        rs = lfgf_write(rfp->fifos + i,&rfp->fifoshifts[i].fg) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("regfilter_fifoshifts: fi=%d lfgf_write() rs=%d\n",
	                i,rs) ;
#endif

	        if (rs < 0)
	            return rs ;

	        if (rs == 1)
	            rfp->n.fifoinfo[i].age = 1 ;

	    } /* end if */

	} /* end for */

	return rs ;
}
/* end subroutine (regfilter_fifoshifts) */


/* check for bus grants */

/*
	This is also a pretty good place to find the best data value
	to put on the bus if we get a grant.
*/

static int regfilter_checkgrants(rfp,pip,mip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
{
	LFLOWGROUP	nfg ;

	ULONG	clocks ;

	int	rs = SR_OK ;
	int	i, j, maxage ;
	int	ri ;
	int	f_won = FALSE ;


	rs = lsim_getclock(mip,&clocks) ;


/* setup a null LFLOWGROUP in case of a problem */

	lflowgroup_clear(&nfg,rfp->c.tt) ;

	nfg.path = 0 ;
	nfg.trans = LFLOWGROUP_TNULLIFY ;
	nfg.addr = 0 ;
	nfg.dv = 0 ;
	nfg.dp = LFLOWGROUP_DPNONE ;


/* check forwarding bus for grant */

	rfp->n.f.fownbus = FALSE ;
	rfp->n.fwd.f_v = FALSE ;
	if (rfp->n.f.frequested || rfp->c.f.frequested) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf(
	            "regfilter_checkgrants: rftt=%d checking F "
			"nfr=%d cfr=%d\n",
	            rfp->c.tt,rfp->n.f.frequested, rfp->c.f.frequested) ;
#endif

	    rs = bus_grantednext(rfp->rfbuses + rfp->fwi,rfp->fbid) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_checkgrants: bus_grantednext() rs=%d\n",
	            rs) ;
#endif

	    if (rs > 0) {

	        int	f_gotone = FALSE ;


/* we won the bus ! */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("regfilter_checkgrants: F won\n") ;
#endif

	        f_won = TRUE ;

	        rfp->n.f.frequested = FALSE ;
	        rfp->n.f.fownbus = TRUE ;

/* find the data that we want to forward */

	        f_gotone = rfp->f.fwdfifo ;
	        if (! f_gotone) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf("regfilter_checkgrants: checking FIFO\n") ;
#endif

	            if (rfp->fifocount == 0) {

	                rs = regfilter_calcfifocount(rfp,pip) ;

	                if (rs < 0)
	                    return rs ;

	            } /* end if */

	            if (rfp->fifocount > 0) {

	                f_gotone = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf("regfilter_checkgrants: F FIFO =>\n") ;
#endif

	            }

	        } /* end if (extra chance) */

	        rs = SR_OK ;

/* OK, now check the registers for any data forwards */

	        if (! f_gotone) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf("regfilter_checkgrants: checking REGs\n") ;
#endif

	            ri = -1 ;
	            maxage = 0 ;
	            for (i = 0 ; i < rfp->nregs ; i += 1) {

/* check NEXT state here to catch the "forward" register */

	                if (rfp->n.regs[i].fage > maxage) {

	                    ri = i ;
	                    maxage = rfp->n.regs[i].fage ;

	                }

	            } /* end for */

	            if (ri >= 0) {

	                rfp->n.fwd.fg = rfp->n.regs[ri].fg ;
	                rfp->n.fwd.f_v = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf("regfilter_checkgrants: F REG => "
	                        "tr=%d p=%d tt=%d a=%d dp=%d dv=%08x\n",
	                        rfp->n.fwd.fg.trans,
	                        rfp->n.fwd.fg.path,
	                        rfp->n.fwd.fg.tt,
	                        rfp->n.fwd.fg.addr,
	                        rfp->n.fwd.fg.dp,
	                        rfp->n.fwd.fg.dv) ;
#endif

/* turn off indication */

	                rfp->n.regs[ri].fage = 0 ;
	                rfp->n.regs[ri].f.forward = FALSE ;
	                rfp->n.regs[ri].f.new = FALSE ;

	            } else {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                    eprintf("regfilter_checkgrants: F *empty*\n") ;
#endif

	                rfp->n.fwd.fg = nfg ;

			}

/* set our forwarder's TT */

	            rfp->n.fwd.fg.ftt = rfp->c.tt ;

	        } /* end if (register checking) */

	    } /* end if (won bus) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_checkgrants: ck=%lld F "
			"f_won=%d\n",
	            clocks, rfp->n.f.fownbus) ;
#endif

	} /* end if (forwarding bus) */

	if (rs < 0)
	    return rs ;


/* check backwarding bus for grant */

	rfp->n.f.bownbus = FALSE ;
	rfp->n.bwd.f_v = FALSE ;
	if (rfp->n.f.brequested || rfp->c.f.brequested) {

	    rs = bus_grantednext(rfp->rbbuses + rfp->bwi,rfp->bbid) ;

	    if (rs > 0) {

/* we won the bus ! */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("regfilter_checkgrants: B won\n") ;
#endif

	        f_won = TRUE ;

	        rfp->n.f.brequested = FALSE ;
	        rfp->n.f.bownbus = TRUE ;

/* find the data that we want to forward */

	        ri = -1 ;
	        maxage = 0 ;
	        for (i = 0 ; i < rfp->nregs ; i += 1) {

/* check NEXT state here to catch the "forward" register */

	            if (rfp->n.regs[i].bage > maxage) {

	                ri = i ;
	                maxage = rfp->n.regs[i].bage ;

	            }

	        } /* end for */

	        if (ri >= 0) {

	            rfp->n.bwd.fg = rfp->n.regs[ri].fg ;
	            rfp->n.bwd.fg.tt = MAX(rfp->n.regs[ri].btt,rfp->c.tt) ;

	            rfp->n.bwd.fg.trans = LFLOWGROUP_TSTORE ;
	            rfp->n.bwd.fg.dp = LFLOWGROUP_DPNONE ;
#if	(! F_BWDDATA)
	            rfp->n.bwd.fg.dv = 0 ;
#endif

	            rfp->n.bwd.f_v = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	                eprintf(
	                    "regfilter_checkgrants: B REG => "
	                    "a=%d dp=%d dv=%08x\n",
	                    rfp->n.bwd.fg.addr,
	                    rfp->n.bwd.fg.dp,
	                    rfp->n.bwd.fg.dv) ;
#endif

/* turn off indication */

	            rfp->n.regs[ri].bage = 0 ;
	            rfp->n.regs[ri].f.backward = FALSE ;

	        } else
	            rfp->n.bwd.fg = nfg ;

/* set our forwarder's TT */

	        rfp->n.bwd.fg.ftt = rfp->c.tt ;

	    } /* end if (won bus) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_checkgrants: ck=%lld B won=%d\n",
	            clocks, rfp->n.f.bownbus) ;
#endif

	} /* end if (backwarding bus) */

	if (rs < 0)
	    return rs ;


	return ((rs >= 0) ? f_won : rs) ;
}
/* end subroutine (regfilter_checkgrants) */


/* check if we need to propagate some bus holds */
static int regfilter_checkholds(rfp,pip,lip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	BUS	*buses ;

	int	rs = SR_OK, i, j ;
	int	fi ;
	int	ri, m ;
	int	c, cr ;
	int	nhold = 0 ;
	int	maxcount = 0 ;
	int	f_up = TRUE ;


/* check the RFB FIFOs */

	buses = rfp->rfbuses ;
	ri = rfp->fri ;
	m = lip->nsg ;

	j = ri ;
	for (i = 0 ; i < rfp->fspan ; i += 1) {

		fi = (i * 2) + 1 ;
		rs = lfgf_count(rfp->fifos + fi) ;

		if (rs < 0)
			break ;

		c = rs ;
		cr = rfp->ffsize - c ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("regfilter_checkholds: F FIFO=%i minfree=%d\n",
		i,rfp->minfree) ;
	    eprintf("regfilter_checkholds: remaining entries=%d\n",
	        cr) ;
	}
#endif

		if (cr < rfp->minfree) {

	    		rs = bus_hold(buses + j) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_checkholds: BUS=%08lx bus_hold() rs=%d\n",
	            (buses + j),rs) ;
#endif

			if (rs < 0)
				break ;

			nhold += 1 ;

		} /* end if (holding a bus) */

		if (c > maxcount)
			maxcount = c ;

	    j = (j + ((f_up) ? 1 : -1) + m) % m ;

	} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_checkholds: nhold=%d maxcount=%d\n",
	            nhold,maxcount) ;
#endif

	return ((rs >= 0) ? nhold : rs) ;
}
/* end subroutine (regfilter_checkholds) */


/* hold some buses */
static int regfilter_holdbuses(rfp,pip,lip,buses,m,ri,span,f_up)
REGFILTER	*rfp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
BUS		*buses ;
int		m ;			/* bus index modulus */
int		ri ;			/* bus read index */
int		span ;			/* bus read span */
int		f_up ;			/* bus scan direction, FALSE=f TRUE=b */
{
	int	rs = SR_OK ;
	int	i, j ;


/* loop through all of the specified buses and initiate a hold on them */

	return SR_OK ;
}
/* end subroutine (regfilter_holdbuses) */


/* miscellaneous processing for calculating commitment-ready information */
static int regfilter_calcdone(rfp,pip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
{
	int	rs = SR_OK, i ;
	int	count ;
	int	f_forward, f_backward ;
	int	f_ready  ;


/* any forwards or backwards in progress ? */

	f_forward = rfp->c.f.frequested || rfp->c.f.fownbus ;
	f_backward = rfp->c.f.brequested || rfp->c.f.bownbus ;

/* check some FIFO butt also ! for safety */

	count = 0 ;
	if ((! f_forward) && (! f_backward)) {

	    rs = regfilter_calcfifocount(rfp,pip) ;

	    if (rs < 0)
	        return rs ;

	    count = rs ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_calcdone: FIFO maxcount=%d\n",
	            count) ;
#endif

	} /* end if (checking forwarding FIFOs) */

/* put it all together */

	f_ready = (! f_forward) && (! f_backward) && (count == 0) ;

	if (f_ready) {

	    if (rfp->c.donecount > 0)
	        rfp->n.donecount = (rfp->c.donecount - 1) ;

	        else
	        rfp->n.donecount = 0 ;

	} else
	    rfp->n.donecount = REGFILTER_DONECOUNT ;

	if (rs >= 0)
	    rs = (f_ready && (rfp->c.donecount == 0)) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_calcdone: f_ready=%d dc=%d n_dc=%d rs=%d\n",
	        f_ready,rfp->c.donecount,rfp->n.donecount,rs) ;
#endif

	return rs ;
}
/* end subrotuine (regfilter_calcdone) */


/* calculate the maxium FIFO count */
static int regfilter_calcfifocount(rfp,pip)
REGFILTER	*rfp ;
struct proginfo	*pip ;
{
	int	rs = SR_OK, i ;
	int	n, fi ;
	int	maxbi ;
	int	maxcount ;


	maxcount = 0 ;
	maxbi = 0 ;

	n = rfp->fspan ;
	for (i = 0 ; i < n ; i += 1) {

	    fi = (i * 2) + 1 ;
	    rs = lfgf_count(rfp->fifos + fi) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("regfilter_calcfifocount: bi=%d rs=%d\n",
	            i,rs) ;
#endif

	    if (rs < 0)
	        break ;

	    if (rs > maxcount) {

	        maxbi = i ;
	        maxcount = rs ;

	    }

	} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regfilter_calcfifocount: maxbi=%d maxcount=%d\n",
	        maxbi,maxcount) ;
#endif

	rfp->fifocount = maxcount ;	/* save in temporary */
	return (rs >= 0) ? maxcount : rs ;
}
/* end subrotuine (regfilter_calcfifocount) */


#if	F_FPRINTF || (F_MASTERDEBUG && F_DEBUG)

static int mkttbuf(buf,tt)
char	buf[] ;
int	tt ;
{
	int	rs ;


	if (tt < REGFILTER_DISPLAYTT) {

	    buf[0] = '-' ;
	    buf[1] = '\0' ;
	    rs = 2 ;

	} else
	    rs = ctdeci(buf,-1,tt) ;

	return rs ;
}

#endif /* F_DEBUG */



