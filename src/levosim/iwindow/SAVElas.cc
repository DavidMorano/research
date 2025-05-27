/* las */

/* Levo Active Station */


#define	F_DEBUGS	0		/* non-switchable debugging */
#define	F_DEBUG		1		/* switchable debugging */
#define	F_FPRINTF	1		/* print to STDOUT */
#define F_LASDETAILS	1		/* show LAS Bus interface details */
#define F_LASDETAILS2	1		/* show LAS Bus interface details */
#define F_LASDETAILS3	0		/* show LAS Bus interface details */
#define	F_STDIOFLUSH	1		/* flush stdout ? */
#define	F_SAFE		1		/* safe mode */
#define	F_SAFE2		0		/* more safe */
#define	F_SAFEFUNC	0		/* extra-extra safe ? */
#define	F_RBBHACK	1		/* Dave's REG backwarding bus hack */
#define	F_RFBHACK	1		/* Dave's RFB snoop hack */
#define	F_CHECKSUM	0		/* please turn this **ON** ! */
#define	F_SANITYSUBOBJ	0		/* sanity check */
#define	F_SANITYOURSELF	1		/* sanity check */
#define	F_SANITYSTATE	1		/* sanity check */
#define	F_BADBADHACK	1		/* please get rid of this !! */
#define	F_EXTRASNOOP	1		/* use the extra snoop rule ? */
#define PERF_FETCH	1		/* perfect fetching */


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written for the LEVO simulator LEVOSIM.


	= 00/06/15, Alireza Khalafi

	I took over this code for Levo IV.


	= 00/07/17, Dave Morano

	I integrated the code into an initial runnable build package.
	I also added code for handling most all the buses that come
	to as AS.


	= 00/10/03, Dave Morano

	+ I changed the way some buses come into the LAS object.  This
	was necessary because many of the buses coming into the
	i-window are different than before due to the introduction of
	the Levo Memory (Write) Queue object into the machine.  

	+ There was also a bug with the check for failure from some
	'malloc()' calls but as it turned out it was not causing a
	problem on the current OS !  I fixed the 'malloc()' problems
	and also fixed up some broken cleanup code in the case when
	something in initialization failed (again this was not generally
	a real problem at all).


	= 01/01/10, Dave Morano

	I updated the code for doing machine "shifts" !


	= 01/01/16, Dave Morano

	I updated the code by adding the 'las_commitinfo()' subroutine.
	This was needed to privide additional information back to
	the execution window control logic for control flow changes
	that occur that go 'out of the window'.


	= 01/03/13, Dave Morano

	I just made a very small change to snoop one less bus for
	the group of Register Forwarding Buses that have been given
	to us.  This prevents the same register update from reaching
	us through two different paths.  This is now a bad thing
	so we want to avoid it !


	= 01/03/30, Dave Morano

	I instrumented this code in order to find the bad LPE bug that
	was causing our recent crashes.


	= 01/05/14, Dave Morano

	I fixed a stray write bug that had to do with the number
	of cancelling predicates that we were getting from a load.
	The code was overwriting the end of the allocated array
	to hold the predicates.


	- 01/05/15, Dave Morano

	I added some code to prevent transactions from looping around
	the machine which can happen without active prevention when
	bus spans of more than '1' are configured.


	- 01/05/17, Dave Morano

	I added an extra snooping rule to prevent duplicate
	transactions from being snarfed and processed.  These can occur
	when the machine is confired with a register forwarding bus
	span greater than one.


	- 01/05/31, Dave Morano

	I tried to fix up the 'las_commitinfo()' subroutine to
	provide the correct indication of whether the instruction
	was enabled or not.


*/



/**************************************************************************

	This module provides the functions for a Levo Active Station.

	Operational note :

	The register forwarding bus and predicate forwarding bus are
	both wired alike to us and we snoop them proceeding backwards
	from where we are in the bus array.  For a forwarding span
	of N, we snoop :

	+ ourselves (that is, our own bus)
	+ N buses previous to ourselves (modulo the total number of buses)

	This is a total of (N + 1) buses to be snooped for a span of N.

	Backwarding buses are wired differently than forwarding buses
	but it isn't as bad as it might seem from looking at the
	connectivity pattern in the IW object !  The whole machine is
	SG-view centric and so this makes looking at the buses from
	within an SG (like us) easier to understand.  Anyway, the rule
	is that we snoop :

	+ ourselves (of course)
	+ (N - 1) buses that are previous to us

	This is a total of N backwarding buses being snooped for a span
	of N.  As it turns out, in spite of the weirdo bus connectivity
	arrangement of backwarding buses, it is almost similar to the
	old (before we got smarter) arrangement.  Dave Morano actually
	worked this out to this extent to make our life easier !


**************************************************************************/


#define	LAS_MASTER		1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>
#include	<assert.h>
#include	<cstdio>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"lsim.h"
#include	"levoinfo.h"
#include	"lpe.h" 
#include	"las.h"
#include	"bus.h"
#include	"lbusint.h"
#include	"lexec.h"
#include	"ldecodeinstrclass.h"
#include	"lwq.h"
#include	"instrclass.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	LAS_MAGIC	0x09089887
#define	LAS_DISPLAYTT	-9999999



/* external subroutines */

extern int	ffbsi(uint) ;
extern int	getinterleave(uint,uint) ;
extern int	getnumbuses(uint) ;
extern int	seqok(uint,uint,uint) ;


/* forward references */

static int      busintgrp_shift(struct busintgrp *,
			struct proginfo *, struct levoinfo *) ;
static int	busintgrp_init(struct busintgrp *,
			struct proginfo *, struct levoinfo *, 
			BUS *,int,int,int,int,int,int) ;
static int	busintgrp_free(struct busintgrp *) ;
static int      busintgrp_sanitycheck(struct busintgrp *,
			struct proginfo *,struct levoinfo *) ;

static int	RFWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
static int	RBWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
static int	PFWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
static int	MFWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
static int	MBWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
static int	PE_interface(LAS *,struct proginfo *,struct levoinfo *) ;
static int	snoop_RFWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
static int	snoop_RBWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
static int	snoop_PFWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
static int	snoop_MFWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
static int	snoop_MBWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
static int	AS_logic(LAS *,struct proginfo *,struct levoinfo *) ;
static int	las_predicate_logic(LAS *);
static int	las_handleshift(LAS *,struct proginfo *,struct levoinfo *) ;
static int	las_readrfbuses(LAS *,struct proginfo *,struct levoinfo *,
			LBUSINT *,int) ;
static int	las_doshift(LAS *,struct proginfo *,struct levoinfo *,
			int,int,int *) ;

static int fill_packet(struct las_packet *,LFLOWGROUP *) ;
static int send_packet(LAS *,struct las_packet *,las_storage,int) ;
static int read_packet(struct las_packet *,LFLOWGROUP *) ;

#if	F_DEBUG || F_FPRINTF
static int	mkttbuf(char *,int) ;
#endif


/* initialize this LAS object */

/**** with :

	- object pointer
	- 'proginfo'
	- LSIM pointer
	- 'levoinfo'
	- our unique AS identification number
	- physical column index
	- physical SG index
	- PE pointer
	- index to local RFB
	- index to local RBB
	- index to local PFB
	- pointer to MFBs (lip->nmembuses of them)
	- pointer to MBBs (lip->nmembuses of them)
	- pointer to all RFBs, RBBs, PFBs
	- bus ID for buses (all of them right now)

****/


int las_init(lasp,pip,mip,lip,ap)
LAS		*lasp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
LAS_INITARGS	*ap ;
{
	BUS	*busp ;

	int	rs ;
	int	size ;
	int	npred ;
	int	n, maxspan ;
	int	i ;


	if ((lasp == NULL) || (ap == NULL))
		return SR_FAULT ;

	lasp->magic = 0 ;
	(void) memset(&lasp->f,0,sizeof(struct las_flags)) ;

	lasp->pip = pip ;
	lasp->mip = mip ;
	lasp->lip = lip ;

	lasp->asid = ap->asid ;		/* our ID */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_init: LAS=%08lx ASID=%d\n",lasp,ap->asid) ;
#endif

	lasp->rftype = MIN(ap->rftype,1) ;	/* register forwarding type */
	lasp->pci = ap->pci ;		/* physical column index */
	lasp->psgi = ap->psgi ;		/* physical Sharing Group (SG) index */

	lasp->llbp = ap->llbp ;		/* the LLB object */
	lasp->lwqp = ap->lwqp ;		/* the LWQ object */
	lasp->btrb = ap->btrb ;		/* the BTRB object */
	lasp->lpes = ap->lpes ;
	lasp->rgfp = ap->alirgf;	/* register file */

	lasp->rfbi = ap->rfbi ;
	lasp->rbbi = ap->rbbi ;
	lasp->pfbi = ap->pfbi ;

	lasp->buspriority = ap->busid;

	lasp->sanity = ap->sanity ;

/* get the memory interleave bits from 'levoinfo' */

	lasp->mfwb_intleave = ap->mfinter ;		/* bits for MFB */
	lasp->mbwb_intleave = ap->mbinter ;		/* bits for MBB */


/* our physical AS row is here in element 'lasp->asrow' ! */

	lasp->naspcol = lip->totalrows ;
	lasp->asrow = ap->asid % lip->totalrows ;

	lasp->logrows = ffbsi(lip->totalrows) ;

/* our sharing group (SG) row is here */

	lasp->sgrow = ap->psgi % lip->nsgrows ;

/* our sharing group physcal index (not really useful directly) */

	lasp->psgi = ap->psgi ;

/* zero all machine state */

	(void) memset(&lasp->c,0,sizeof(struct las_state)) ;

	(void) memset(&lasp->n,0,sizeof(struct las_state)) ;

	lasp -> pe_tnumb = -1;
	lasp->c.mem.latesttt = lasp->n.mem.latesttt = INT_MIN;
	lasp->c.asstate = lasp->n.asstate = UNUSED;

/* what about predicate register sets that we need */

	lasp->npred = npred = lip->nprpas ;

/* allocate the predicate register resources */

	size = 2 * npred * sizeof(struct las_reg) ;
	rs = uc_malloc(size,(void **) &lasp->c.pregs.cpin) ;

	if (rs < 0)
		goto bad0 ;

	lasp->n.pregs.cpin = lasp->c.pregs.cpin + npred ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf(
		"las_init: npred=%d c_pregs_cpin=%08lx n_pregs_cpin=%08lx\n",
		npred,lasp->c.pregs.cpin,lasp->n.pregs.cpin) ;
#endif

/* continue with other stuff */

	lasp->c.pregs.num_of_cpin_regs = lasp->n.pregs.num_of_cpin_regs = 0;


/* create our 'lbusint's for use with our various buses */

	maxspan = 0 ;

/* register forwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1) {
		eprintf("las_init: RFB(0)=%08lx span=%d\n",ap->rfbuses,
			lip->nrfspan) ;
	}
#endif

#if	F_RFBHACK
	n = MAX(lip->nrfspan,1) ;	/* new way */
#else
	n = lip->nrfspan + 1 ;		/* old way */
#endif

	rs = busintgrp_init(&lasp->rfwbus,pip,lip,
	    ap->rfbuses,FALSE,ap->rfbi,lip->nsg,n,ap->busid,RFWB) ;

	if (rs < 0)
	    goto bad1 ;

	if (n > maxspan)
		maxspan = n ;

/* register backwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
		eprintf("las_init: RBB(0)=%08lx span=%d\n",ap->rbbuses,
			lip->nrbspan) ;
#endif

#if	F_RBBHACK
	n = MAX(lip->nrbspan,1) ;	/* new way */
#else
	n = lip->nrbspan + 1 ;		/* old way */
#endif

	rs = busintgrp_init(&lasp->rbwbus,pip,lip,
	    ap->rbbuses,TRUE,ap->rbbi,lip->nsg,n,ap->busid,RBWB) ;

	if (rs < 0)
	    goto bad2 ;

	if (n > maxspan)
		maxspan = n ;

/* predicate forwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
		eprintf("las_init: PFB(0)=%08lx span=%d\n",ap->pfbuses,
			lip->npfspan) ;
#endif

	n = lip->npfspan + 1 ;
	rs = busintgrp_init(&lasp->pfwbus,pip,lip,
	    ap->pfbuses,FALSE,ap->pfbi,lip->nsg,n,ap->busid,PFWB) ;

	if (rs < 0)
	    goto bad3 ;

	if (n > maxspan)
		maxspan = n ;

/* memory forwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_init: MFB(0)=%08lx\n",ap->mfbuses) ;
#endif

	n = getnumbuses(ap->mfinter) ;

	busp = ap->mfbuses ;
	rs = busintgrp_init(&lasp->mfwbus,pip,lip,busp,TRUE,0,n,n,
		ap->busid,MFWB) ;

	if (rs < 0)
	    goto bad4 ;

/* memory backwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_init: MBB(0)=%08lx\n",ap->mbbuses) ;
#endif

	n = getnumbuses(ap->mbinter) ;

	busp = ap->mbbuses ;
	rs = busintgrp_init(&lasp->mbwbus,pip,lip,busp,TRUE,0,n,n,
		ap->busid,MBWB) ;

	if (rs < 0)
		goto bad4 ;


/* allocate some temporary bus read register buffer */

	size = maxspan * sizeof(struct las_busreg) ;
	rs = uc_malloc(size,(void **) &lasp->busregs) ;

	if (rs < 0)
		goto bad5 ;

	(void) memset(lasp->busregs,0,size) ;


/* checksum */

#if	F_SAFE
	d_checkcalc(&lasp->c,sizeof(struct las_state),&lasp->c.checksum) ;
#endif


#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_init: exiting rs=%d\n",rs) ;
#endif

	lasp->magic = LAS_MAGIC ;
	return rs ;

/* bad things */
bad6:

#ifdef	COMMENT
	if (lasp->busregs != NULL)
		free(lasp->busregs) ;
#endif

bad5:
	busintgrp_free(&lasp->mfwbus) ;

bad4:
	busintgrp_free(&lasp->pfwbus) ;

bad3:
	busintgrp_free(&lasp->rbwbus) ;

bad2:
	busintgrp_free(&lasp->rfwbus) ;

bad1:
	if (lasp->c.pregs.cpin != NULL)
		free(lasp->c.pregs.cpin) ;

bad0:
	return rs ;
}
/* end subroutine (las_init) */


/* free up the object */
int las_free(lasp)
LAS	*lasp ;
{
	struct proginfo		*pip ;

	int	rs = SR_OK ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_free: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 3) {
	eprintf("las_free: ASID=%d LAS=%08lx\n",
		lasp->asid,lasp) ;
	eprintf("las_free: bus-int-groups\n") ;
	}
#endif

	busintgrp_free(&lasp->mbwbus) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: 2 bus-int-groups\n") ;
#endif

	busintgrp_free(&lasp->mfwbus) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: RF bus-int-groups\n") ;
#endif

	busintgrp_free(&lasp->rfwbus) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: RB bus-int-groups\n") ;
#endif

	rs = busintgrp_free(&lasp->rbwbus) ;

#if	F_DEBUG
	if (pip->debuglevel > 3) {
	eprintf("las_free: RB rs=%d\n",rs) ;
	eprintf("las_free: PF bus-int-groups\n") ;
	}
#endif

	busintgrp_free(&lasp->pfwbus) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: cancelling predicates\n") ;
#endif

	if (lasp->c.pregs.cpin != NULL)
		free(lasp->c.pregs.cpin) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: exiting rs=%d\n",rs) ;
#endif

	lasp->magic = 0 ;
	return rs ;
}
/* end subroutine (las_free) */


/* perform our combinatorial logic */
int las_comb(lasp,phase)
LAS	*lasp ;
int	phase ;
{
	struct proginfo		*pip ;

	LSIM			*mip ;

	struct levoinfo		*lip ;

	struct busintgrp	*bigp ;

	int	rs = SR_OK, i ;
	int	rs1 ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_comb: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	mip = lasp->mip ;
	lip = lasp->lip ;		/* need for some number computation */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_comb: entered ck=%lld ph=%d TT=%d\n",
		mip->clock,phase,lasp->c.tt) ;
#endif

#if	F_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 0 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* F_SAFE2 */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 0 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */


/* 'lbusint's for RFBes for a span */

	bigp = &lasp->rfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d RFB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	F_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 1 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* F_SAFE2 */

/* 'lbusint's for RBBes for a span */

	bigp = &lasp->rbwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d RBB-%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	F_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 2 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* F_SAFE2 */

/* 'lbusint's for predicate buses for a span */

	bigp = &lasp->pfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d PFB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	F_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 3 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* F_SAFE2 */

/* 'lbusint's for memory buses */

	bigp = &lasp->mfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d MFB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	F_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 4 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* F_SAFE2 */

	bigp = &lasp->mbwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d MBB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	F_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 5 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* F_SAFE2 */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 5 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */


/* do our own combinatorial work */

	switch (phase) {

/* execute all our combinational parts */
	case 2:
	    rs = AS_logic(lasp,pip,lip) ;

	    break ;

	case 3:
	    rs = las_handleshift(lasp,pip,lip) ;

	    break ;

	} /* end switch */

	if (rs < 0)
		return rs ;

#if	F_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 6 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* F_SAFE2 */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 6 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	return rs ;
}
/* end subroutine (las_comb) */


/* preform a clocked state transition */
int las_clock(lasp)
LAS	*lasp ;
{
	struct proginfo		*pip ;
	struct levoinfo		*lip ;
	struct busintgrp	*bigp ;

	struct las_reg		*regp;

	ULONG	clocks = 0 ;

	int	rs ;
	int	ret;
	int	i,j,k,ll ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_clock: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;


#if	F_SAFE && F_CHECKSUM
	rs = d_checkverify(&lasp->c,sizeof(struct las_state),
	    &lasp->c.checksum) ;

	if (rs < 0) {

	    eprintf("las_clock: LAS=%08lx checksum bad\n",
	        lasp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */


	clocks = 0 ;
	lsim_getclock(lasp->mip,&clocks) ;

if(lasp->asid == 0)
	printf("CLOCK ===== %lld\n",clocks) ;


/* transition ourself */


/* careful here */
/* we are dealing with some pointer that is masquerading as state ! */

	regp = lasp->c.pregs.cpin;

	lasp->c = lasp->n ;

	lasp->c.pregs.cpin = regp;


#ifdef	COMMENT_BADBUG
	for (ll=0; ll <= lasp->n.pregs.num_of_cpin_regs; ll += 1)
		lasp->c.pregs.cpin[ll] = lasp->n.pregs.cpin[ll];
#else

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf(
		"las_clock: npred=%d c_pregs_cpin=%08lx n_pregs_cpin=%08lx\n",
		lasp->npred,lasp->c.pregs.cpin,lasp->n.pregs.cpin) ;
#endif

	for (ll = 0 ; ll < lasp->npred ; ll += 1)
		lasp->c.pregs.cpin[ll] = lasp->n.pregs.cpin[ll] ;

#endif /* COMMENT_BADBUG */


#if	F_SAFE
	d_checkcalc(&lasp->c,sizeof(struct las_state),&lasp->c.checksum) ;
#endif



/* execute all subobject clock methods */


/* 'lbusint's for RFBes for a span */

	bigp = &lasp->rfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1)
	    lbusint_clock(bigp->head + i) ;

/* 'lbusint's for RBBes for a span */

	bigp = &lasp->rbwbus ;
	for (i = 0 ; i < bigp->num ; i += 1)
	    lbusint_clock(bigp->head + i) ;

/* 'lbusint's for predicate buses for a span */

	bigp = &lasp->pfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1)
	    lbusint_clock(bigp->head + i) ;


/* 'lbusint's for memory buses */

	bigp = &lasp->mfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1)
	    lbusint_clock(bigp->head + i) ;

	bigp = &lasp->mbwbus ;
	for (i = 0 ; i < bigp->num ; i += 1)
	    lbusint_clock(bigp->head + i) ;


#if	F_STDIOFLUSH
	fflush(stdout) ;
#endif


	return SR_OK ;
}
/* end subroutine (las_clock) */


/* is this AS used ? */
int las_used(lasp)
LAS * lasp;
{
	int	rs = SR_OK ;


	if (lasp->c.asstate == UNUSED)
		return INSTRCLASS_UNUSED ;

	switch (lasp->c.opclass) {

	case INSTR_LOAD:
		return INSTRCLASS_REG;
		break;

	case INSTR_STORE:
		return INSTRCLASS_STORE;
		break;

	case INSTR_BREL: 
		return INSTRCLASS_CBR;
		break;

/* branch indirect */
	case INSTR_BIND:
		return INSTRCLASS_JMP;
		break;

	case INSTR_SYSCALL:
		return INSTRCLASS_CALL;
		break;

	case INSTR_ALU:
		return INSTRCLASS_REG;
		break;

	case INSTR_FPALU: 
		return INSTRCLASS_REG;
		break;

	case INSTR_SPECIAL:
		return INSTRCLASS_REG;
		break;

	case INSTR_JREL:
		return INSTRCLASS_JMP;
		break;

	case INSTR_JIND:
		return INSTRCLASS_JMP;
		break;

	case INSTR_EXCEP:
		return INSTRCLASS_CALL;
		break;

	case INSTR_UNUSED:
		return INSTRCLASS_UNUSED;
		break;

	default:
		rs = SR_NOTSUP ;

	} /* end switch */

	return rs ;
}
/* end subroutine (las_used) */


/* receive the combinatorial SHIFT signal */
int las_shift(lasp)
LAS	*lasp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_shift: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;


/* "do" all of our subobjects */


/* register forwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_shift: RFB\n") ;
	}
#endif

	rs = busintgrp_shift(&lasp->rfwbus,pip,lip) ;

	if (rs < 0)
	    goto bad1 ;

/* register backwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: RBB\n") ;
#endif

	rs = busintgrp_shift(&lasp->rbwbus,pip,lip) ;

	if (rs < 0)
	    goto bad2 ;

/* predicate forwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: PFB\n") ;
#endif

	rs = busintgrp_shift(&lasp->pfwbus,pip,lip) ;

	if (rs < 0)
	    goto bad3 ;


/* memory forwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: MFB\n") ;
#endif

	rs = busintgrp_shift(&lasp->mfwbus,pip,lip) ;

	if (rs < 0)
	    goto bad4 ;

/* memory backwarding */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: MBB\n") ;
#endif

	rs = busintgrp_shift(&lasp->mbwbus,pip,lip) ;

	if (rs < 0)
		goto bad5 ;


/* finally "do" us */

	lasp->f.shift = TRUE ;

bad5:
bad4:
bad3:
bad2:
bad1:
	return rs ;
}
/* end subroutine (las_shift) */


/* get the instruction address from this AS */
int las_getia(lasp,iap)
LAS	*lasp ;
uint	*iap ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (iap == NULL)
		return SR_FAULT ;

	if (lasp->c.asstate == UNUSED)
		return SR_EMPTY ;

/* put something here ! */

	*iap = lasp->c.ia ;

	rs = las_used(lasp) ;

	return rs ;
}
/* end subroutine (las_getia) */


/* load this AS from the Levo load buffer */
int las_loadfromlb(lasp, llbp, index, path, f_invert)
LAS 	* lasp ;
LLB 	* llbp ;
int	index ;
int	path ;
int	f_invert;
{
	int	rs ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	rs = las_load(lasp,llbp,index,path) ;	

	if (f_invert && (lasp->n.opclass == INSTR_BREL))
	;

	return rs ;
}
/* end subroutine (las_loadfromlb) */


/* set this AS to be unused */
int las_unused(lasp)
LAS * lasp;
{


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	lasp->n.asstate = UNUSED;
	return SR_OK;
}
/* end subroutine (las_unused) */


/* load an AS from another AS */
int las_loadfromas(lasp, lasp2, path, f_invert)
LAS * lasp ;
LAS * lasp2 ;
int	path ;
int	f_invert ;
{


	if (lasp == NULL)
		return SR_FAULT ;


	return SR_NOTSUP ;
}
/* end subroutine (las_loadfromas) */


/* old subroutine to load an AS */
int las_load(lasp, llbp, index, path)
LAS * lasp ;
LLB * llbp ;
int	index ;
int	path ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	LLB_INSTRINFO	ii ;

	struct llb_entry	*en ;

	int	rs = SR_OK, rs1 ;
	int	ret;
	int	*cpins;
	int	*cpinsv;
	int	i;
	int 	n;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_load: LAS=%08lx index=%d path=%d\n",
		lasp,index,path) ;
#endif

#if	F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf(
		"las_load: npred=%d c_pregs_cpin=%08lx n_pregs_cpin=%08lx\n",
		lasp->npred,lasp->c.pregs.cpin,lasp->n.pregs.cpin) ;
#endif

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 0 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */


/*
	if(lasp->c.asstate != UNUSED)
		return 0;
*/

/* Dave Morano, 01/05/14 -- this is not a good programming practice ! */

	(void) memset(&lasp->n,0,sizeof(struct las_state)) ;

	lasp->n.pregs.cpin = lasp->c.pregs.cpin + lasp->npred ;

/* end of bad programming practice ! */


#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 1 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

/* load some easy stuff first */

	(void) llb_instrinfo(llbp,index,&ii) ;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 1a bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	lasp->n.ia = ii.ia ;
	lasp->n.instr = ii.instr ;
	lasp->n.btype = ii.btype ;	/* Marcos-style i-fetch branch type */
	lasp->n.oopexec = ii.opexec ;
	lasp->n.oopclass = ii.opclass ;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 2 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

/* load the hard stuff */

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_load: c_pregs_cpin=%08lx n_pregs_cpin=%08lx\n",
		lasp->c.pregs.cpin,lasp->n.pregs.cpin) ;
#endif


#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 3 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	lasp -> pe_tnumb = -1;
	lasp->n.mem.latesttt = INT_MIN;

	ret = llb_read(llbp, &lasp->n.opexec, &lasp->n.opclass, 
			&lasp->n.faddr, &lasp->n.src1.a, &lasp->n.src2.a, 
			&lasp->n.src3.a, &lasp->n.src4.a, 
			&lasp->n.dst1.a, &lasp->n.dst2.a, 
			&lasp->n.cnst.val, &lasp->n.cnst.valid, 
			&lasp->n.pregs.pin.a, 
			&lasp->n.pregs.pin.val, &cpins, &cpinsv, 
			&lasp->n.pregs.num_of_cpin_regs, 
			&lasp->n.pregs.pout.a, &lasp->n.pregs.cpout.a, index);

#if	F_DEBUG
	if (pip->debuglevel > 4) {
		eprintf("las_load: opexec=%08x oopexec=%08x\n",
			lasp->n.opexec,lasp->n.oopexec) ;
	}
#endif

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 4 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	if(lasp->n.src1.a == 0) lasp->n.src1.val = 0;
	if(lasp->n.src2.a == 0) lasp->n.src2.val = 0;
	if(lasp->n.src3.a == 0) lasp->n.src3.val = 0;
	if(lasp->n.src4.a == 0) lasp->n.src4.val = 0;
	if(lasp->n.dst1.a == 0) lasp->n.dst1.val = 0;
	if(lasp->n.dst2.a == 0) lasp->n.dst2.val = 0;

	if (ret == SR_OK) {

		lasp->n.pI = 1;
		lasp->n.regfile_read = 0;
		lasp->n.asstate = WAIT_NRC;

	} else
		return 0;

	if (lasp->n.opclass == INSTR_UNUSED)
		lasp->n.asstate = UNUSED;

/* sanity check */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 5 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

/* what about the input predicates ? */

	n = lasp->n.pregs.num_of_cpin_regs;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_load: max npred=%d input predicates n=%d\n", 
		lasp->npred,n) ;
#endif

	if (n < 0)
		return SR_INVALID ;

	if (n > lasp->npred)
		return SR_NOTSUP ;

	if (n > 0) {

/* we have some input predicates */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 6 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

		for(i=0;i<lasp->n.pregs.num_of_cpin_regs; ++i) {
			lasp->n.pregs.cpin[i].a = cpins[i];
			lasp->n.pregs.cpin[i].val = cpinsv[i];
			lasp->n.pregs.cpin[i].latesttt = INT_MIN;
		}

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 7 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	} else {

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_load: we do not have any input predicates n=0\n") ;
#endif

		lasp->n.pregs.cpin[0].a = -1;

	}

/* sanity check */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 9 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

/* assign the TT */

	lasp->n.tt = (lip->nsg * lip->nasrpsg) + lasp->asrow ;


	lasp->n.re_exec_inst = 1;
	lasp->n.path = path;
	lasp->n.pregs.pin.latesttt = INT_MIN;
	lasp->n.pregs.pout.val = 0;
	lasp->n.pregs.cpout.val = 0;
	lasp->n.mem.a = -1;
	lasp->n.src1.latesttt = INT_MIN;
	lasp->n.src2.latesttt = INT_MIN;
	lasp->n.src3.latesttt = INT_MIN;
	lasp->n.src4.latesttt = INT_MIN;
	lasp->n.dst1.latesttt = INT_MIN;
	lasp->n.dst2.latesttt = INT_MIN;
	lasp->n.mem.latesttt = INT_MIN;

/* very bad thing to do, messed up the pipelining of loads */

#if	F_BADBADHACK
	lasp -> c.pregs.pout.val = -2;
#endif

/* OK */

	lasp -> n.pregs.pout.val = -2;

/* very bad thing to do, messed up the pipelining of loads */

#if	F_BADBADHACK
	lasp -> c.pregs.cpout.val = -2; 
#endif

/* OK */

	lasp -> n.pregs.cpout.val = -2; 

/* sanity check */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 12 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */


	return 0 ;
}
/* end subroutine (las_load) */


/* are we ready to commit an instruction ? */
int las_readycommit(lasp)
LAS * lasp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	LBUSINT		*lbp ;

	int	rs ;

	int	commit = 1 ;
	int	i,j,k;
	int	n, mi ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;
	if (lasp->c.asstate == UNUSED)
		return 1;

/* if we are NOT enabled, then commit immediately */

	if (lasp->c.pI == 0) {

		if (lasp->c.opclass == INSTR_BREL)
			return 2;
		else
			return 1;

	}

/* otherwise, we are enabled and we should wait for other things */

	if (lasp->c.re_exec_inst)
		commit = 0;

	if (!commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for instruction execution\n") ;
#endif

	return 0;
	}

/*
	if(lasp->wait4src1 && lasp->wait4src2 )
		commit = 0;
		*/

	if (lasp->c.fw_pred_on_pfwb) 
		commit = 0;

	if (!commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for PFB\n") ;
#endif

	return 0;
	}

/* check the event table */
	
	for (i = 0 ; i < NCOMMANDS ; i += 1) {

	    for (j = 0 ; j < NBUSES ; j += 1) {

		for (k = 0 ; k < NSTORAGES ; k += 1) {

		    if (lasp->c.event_table[i][j][k].valid != 0) {

			commit = 0 ;
			break ;
		    }

		} /* end for */

		if (! commit)
		    break ;

	    } /* end for */

	    if (! commit)
		break ;

	} /* end for */

	if (! commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for event table %d:%d:%d\n",
		i,j,k) ;
#endif

	return 0;
	}

	/* check if waiting on results from PE */
	if (lasp->pe_tnumb != -1)
		commit = 0;

	if (! commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for PE result\n") ;
#endif

		return 0;
	}

/* check if any transaction is still on the output buses */

/* check *only* out output Register Forward bus */

	lbp = lasp->rfwbus.outbus ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: RFB LBUSINT=%08lx\n",lbp) ;
#endif

	    rs = lbusint_done(lbp) ;

	    if (rs < 0)
		goto badbusint ;

	    if (rs < 1)
		commit = 0;

	if (! commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for RFB\n") ;
#endif

		return 0;
	}

/* check the output Register Backward bus */

	lbp = lasp->rbwbus.outbus ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: RBB LBUSINT=%08lx\n",lbp) ;
#endif

	    rs = lbusint_done(lbp) ;

	    if (rs < 0)
		goto badbusint ;

	    if (rs < 1)
		commit = 0;

	if (! commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for RBB\n") ;
#endif

		return 0;
	}

/* check the output Predicate Forward bus */

	lbp = lasp->pfwbus.outbus ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: PFB LBUSINT=%08lx\n",lbp) ;
#endif

	    rs = lbusint_done(lbp) ;

	    if (rs < 0)
		goto badbusint ;

	    if (rs < 1)
		commit = 0;

	if (! commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for PFB\n") ;
#endif

		return 0;
	}

/* do all of the memory buses (since we may write to them all) */

	n = lip->nmembuses ;
	for (mi = 0 ; commit && (mi < n) ; mi += 1) {

/* Memory Forwarding bus */

		lbp = lasp->mfwbus.outbus + mi ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: MFB LBUSINT=%08lx i=%d\n",
		lbp,mi) ;
#endif

		rs = lbusint_done(lbp) ;

		if (rs < 0)
			goto badbusint ;

		if (rs < 1)
			commit = 0;

	if (! commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for MFB i=%d\n",i) ;
#endif

		return 0;
	}

/* Memory Backwarding bus */

		lbp = lasp->mbwbus.outbus + mi ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: MBB LBUSINT=%08lx i=%d\n",
		lbp,mi) ;
#endif
		rs = lbusint_done(lbp) ;

	if (rs < 0)
		goto badbusint ;

	if (rs < 1)
		commit = 0;

	if (! commit) {

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for MBB i=%d\n",i) ;
#endif

		return 0;
	}

	} /* end for (memory buses) */

	if (! commit) {

		rs = 0 ;
		goto ret ;
	}

	rs = (lasp->c.opclass == INSTR_BREL) ? 2 : 1 ;
	goto ret ;

/* bad stuff comes here */
badbusint:

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: bad LBUSINT=%08lx rs=%d\n",
		lbp,rs) ;
#endif

ret:

#if	F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: exiting asid=%d astt=%d rs=%d\n",
		lasp->asid,lasp->c.tt,rs) ;
#endif

	return rs ;
}
/* end subroutine (las_readycommit) */


/* return our commitment information */
int las_commitinfo(lasp,ip)
LAS		*lasp ;
LAS_COMMITINFO	*ip ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;
	int	i ;
	int	f_enable = FALSE ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (ip == NULL)
		return SR_FAULT ;

	pip = lasp->pip ;
	(void) memset(ip,0,sizeof(struct las_commitinfo)) ;

/* get the instruction class used by Dave in the execution window */

	ip->ia = 0 ;
	ip->iclass = 0 ;
	if (lasp->c.asstate == UNUSED)
		return SR_EMPTY ;

/* current instruction information */

	ip->instr = lasp->c.instr ;		/* original instruction word */
	ip->ia = lasp->c.faddr ;		/* instruction address */
	ip->ta = ip->ia + 4 ;
	ip->opclass = lasp->c.opclass ;		/* regular OPCLASS */
	ip->btype = lasp->c.btype ;		/* Marcos-style branch type */
	ip->iclass = las_used(lasp) ;		/* Dave-style i-class */

/* is this instruction enabled */

/* domain predicate ? */

	ip->f_enabled = (lasp->c.pregs.pin.val) ? 1 : 0 ;

/* branch target predicates */

	if (! ip->f_enabled) {

	    for (i = 0 ; i < lasp->c.pregs.num_of_cpin_regs ; i += 1)
		ip->f_enabled |= ((lasp->c.pregs.cpin[i].val) ? 1 : 0) ;

	} /* end if (determining if we were enabled) */

#if	PERF_FETCH
		ip->f_enabled = lasp->c.pI ;
#endif

/* do we have a control flow change type of instruction ? */

	ip->f_cf = FALSE ;
	if ((lasp->c.opclass == INSTR_BREL) || 
		(lasp->c.opclass == INSTR_JREL) ||
	   	(lasp->c.opclass == INSTR_JIND)) {

		rs = 1 ;
		ip->f_cf = TRUE ;
		ip->f_taken = lasp->c.bcond ; 
		ip->ta = (ip->f_taken) ? lasp->c.cnst.val : (ip->ia + 8) ; 

	} /* end if (control-flow-type instruction) */

/****

	Non-control-flow instructions are supposed to be able to look
	at the 'lasp->c.pI' value also (and alone) for its current
	execution predicate.  Let's test this hypothesis !

****/

#if	F_SAFE && 0
	if (! ip->f_cf) {

	    int	f ;


	    f = (lasp->c.pI) ? 1 : 0 ;
	    if (f != ip->f_enabled) {

		rs = SR_NOANODE ;
		if (pip->debuglevel > 1) {

			eprintf(
			"las_commitinfo: mismatched predicates rs=%d\n",
				rs) ;
			eprintf(
			"las_commitinfo: f_enabled=%d pI=%d\n",
				ip->f_enabled,lasp->c.pI) ;
			eprintf(
			"las_commitinfo: f_cf=%d f_taken=%d ta=%08x\n",
				ip->f_cf,ip->f_taken, ip->ta) ;
		}
	    }

	}
#endif /* F_DEBUG */

	return rs ;
}
/* end subroutine (las_commitinfo) */


/* we are being told to commit our instruction */
int las_commit(lasp,ipval,bdir,btarget,faddr)
LAS	*lasp;
int	*ipval;	/* input predicate value */
int	*bdir;
uint	*btarget;
uint	*faddr;
{
	int	rs = SR_OK ;

	int cpin=0;
	int i;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

/* entered */

	las_print(lasp);

	*ipval = 0;
	*bdir = -1;

	lasp->n.asstate = UNUSED;

	if(lasp->c.asstate == UNUSED) {
		return SR_EMPTY;
	}

	if (lasp->c.opclass == INSTR_BREL || lasp->c.opclass == INSTR_JREL ||
	   lasp->c.opclass == INSTR_JIND ) {

		rs = 1 ;
		*bdir = lasp->c.bcond; 
		*btarget = lasp->c.cnst.val; 
		for(i=0; i<lasp->c.pregs.num_of_cpin_regs; ++i)
			cpin |= lasp->c.pregs.cpin[i].val;

		*ipval = cpin | lasp -> c.pregs.pin.val ;

	} else {
		*ipval = lasp->c.pI ;
	}
		
#if	PERF_FETCH
		*ipval = lasp->c.pI;
#endif


	*faddr = lasp->c.faddr;

	lbtrb_as_commit(lasp->btrb, lasp->c.pregs.pout.a, 
		lasp->c.pregs.pout.val, lasp->c.pregs.cpout.a,
		lasp->c.pregs.cpout.val,lasp->asrow);

	if(lasp->c.pI == 0)
		return rs ;

	alirgf_write(lasp->rgfp,lasp); 

	if (lasp->c.opclass == INSTR_STORE)
		lwq_store(lasp->lwqp, (lasp->c.mem.a & 0xfffffffc),
			lasp->c.mem.val,lasp->c.tt, lasp->asrow,lasp->c.dp);

	lasp->n.regfile_read = 0;
	return rs ;
}
/* end subroutine (las_commit) */


/* check for sanity */
int las_sanitycheck(lasp)
LAS	*lasp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_shift: LAS=%08lx bad magic\n",lasp) ;

		rs = SR_BADFMT ;
		goto bad0 ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;


#if	F_SANITYSTATE && F_CHECKSUM

	rs = d_checkverify(&lasp->c,sizeof(struct las_state),
	    &lasp->c.checksum) ;

	if (rs < 0) {

	    eprintf("las_sanitycheck: LAS=%08lx checksum bad\n",
	        lasp) ;

	    return SR_BADFMT ;
	}

#endif /* F_SANITYSTATE */


/* check all of our subobjects */

#if	F_SANITYSUBOBJ

/* register forwarding */

	rs = busintgrp_sanitycheck(&lasp->rfwbus,pip,lip) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: RFB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad1 ;
	}

/* register backwarding */

	rs = busintgrp_sanitycheck(&lasp->rbwbus,pip,lip) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: RBB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad2 ;
	}

/* predicate forwarding */

	rs = busintgrp_sanitycheck(&lasp->pfwbus,pip,lip) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: PFB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad3 ;
	}

/* memory forwarding */

	rs = busintgrp_sanitycheck(&lasp->mfwbus,pip,lip) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: MFB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad4 ;
	}

/* memory backwarding */

	rs = busintgrp_sanitycheck(&lasp->mbwbus,pip,lip) ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: MBB bad rs=%d\n",rs) ;
	}
#endif

		goto bad5 ;
	}

#endif /* F_SANITYSUBOBJ */


/* check ourself */

#if	F_SANITYOURSELF

/* check the opexec value for corruption */

	rs = (lasp->c.opexec == lasp->c.oopexec) ? SR_OK : SR_BADFMT ;

	if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf(
		"las_sanitycheck: opexec bad opexec=%08x oopexec=%08x rs=%d\n",
			lasp->c.opexec,lasp->c.oopexec,rs) ;
	}
#endif

		goto bad6 ;
	}

#endif /* F_SANITYOURSELF */


bad6:
bad5:
bad4:
bad3:
bad2:
bad1:
bad0:
	return rs ;
}
/* end subroutine (las_sanitycheck) */



/* PRIVATE SUBROUTINES */



/* handle a machine shift operation ! */
static int las_handleshift(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	i, j, k ;
	int	totalrows ;
	int	tt_test ;


	if (! lasp->f.shift)
		return SR_OK ;

	lasp->f.shift = FALSE ;
	totalrows = lip->totalrows ;
	tt_test = INT_MIN + totalrows ;

/* do the most obvious stuff */

	lasp->n.tt -= totalrows ;

/* all of the sources and destinations */

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src1.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src2.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src3.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src4.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst1.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst2.tt) ;

/* do them again ! */

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src1.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src2.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src3.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.src4.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst1.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst2.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.mem.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.cnst.latesttt) ;
	
	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.old_mem.latesttt) ;

/* do the stuff in the event tables ? */

	for (i = 0 ; i < NCOMMANDS ; i += 1) {

	    for (j = 0 ; j < NBUSES ; j += 1) {

	        for (k = 0 ; k < NSTORAGES ; k += 1) {

		    las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.event_table[i][j][k].tt) ;

	        } /* end for */

	    } /* end for */

	} /* end for */


	return SR_OK ;
}
/* end subroutine (las_handleshift) */


/* so a shift on one value */
static int las_doshift(lasp,pip,lip,tt_test,tr,ap)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		tt_test ;
int		tr ;
int		*ap ;
{


	if (*ap >= tt_test)
		*ap -= tr ;

	else
	    *ap = INT_MIN ;

}
/* end subroutine (las_doshift) */


/* initialize a bus interface group */
static int busintgrp_init(bigp,pip,lip,buses,f_fwd,bi,m,n,busid,idtype)
struct busintgrp	*bigp ;
struct proginfo		*pip ;
struct levoinfo		*lip ;
BUS			*buses ;
int			f_fwd ;		/* TRUE for a BACKWARDING bus ! */
int			bi, m, n ;
int			busid ;
int			idtype;
{
	int	rs, i ;
	int	size ;
	int	badi ;


#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las/busintgrp_init: n=%d bi=%d\n",n,bi) ;
	eprintf(
	    "las/busintgrp_init: bi=%d\n",bi) ;
	}
#endif

	bigp->pip = pip ;
	bigp->head = NULL ;
	bigp->outbus = NULL ;
	bigp->f_fwd = f_fwd ;
	bigp->bi = bi ;
	bigp->modulas = m ;
	bigp->num = n ;

	size = bigp->num * sizeof(LBUSINT) ;
	rs = uc_malloc(size,(void **) &bigp->head) ;

	if (rs < 0)
	    goto bad0 ;

	(void) memset(bigp->head,0,size) ;

/* we start the bus numbering with our primary bus (if that has meaning) */

	bigp->outbus = bigp->head ;
	for (i = 0 ; i < bigp->num ; i += 1) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/busintgrp_init: i=%d BUS=%08lx bi=%d LBUSINT=%08lx\n",
		i,(buses + bi),bi,bigp->head + i) ;
#endif

	    rs = lbusint_init(bigp->head + i,pip,lip,buses + bi,
			busid,idtype,i) ;

	    if (rs < 0)
	        break ;

/* go forward (for backwarding buses), backward (for forwarding buses) */

	    bi = (bi + ((f_fwd) ? 1 : (-1 + m))) % m ;

	} /* end for */

	if (rs < 0) {

	    badi = i ;
	    goto bad1 ;
	}

	return rs ;

/* bad things */
bad1:
	for (i = 0 ; i < badi ; i += 1)
	    lbusint_free(bigp->head + i) ;

	free(bigp->head) ;

bad0:
	return rs ;
}
/* end subroutine (busintgrp_init) */


/* free up a bus interface group */
static int busintgrp_free(bigp)
struct busintgrp	*bigp ;
{
	int	rs = SR_OK, i ;
	int	rs1 ;


	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs1 = lbusint_free(bigp->head + i) ;

		if ((rs1 < 0) && (rs >= 0))
			rs = rs1 ;

	}

	free(bigp->head) ;

	bigp->head = NULL ;
	bigp->outbus = NULL ;
	return rs ;
}
/* end subroutine (busintgrp_free) */


/* shift a bus interface group */
static int busintgrp_shift(bigp,pip,lip)
struct busintgrp	*bigp ;
struct proginfo		*pip ;
struct levoinfo		*lip ;
{
	int	rs = SR_OK, i ;


	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_shift(bigp->head + i) ;

		if (rs < 0)
			break ;

	} /* end for */

	return rs ;
}
/* end subroutine (busintgrp_shift) */


/* sanitycheck the bus-int-groups */
static int busintgrp_sanitycheck(bigp,pip,lip)
struct busintgrp	*bigp ;
struct proginfo		*pip ;
struct levoinfo		*lip ;
{
	int	rs = SR_OK, i ;


/* check the LBUSINTs for corruption */

	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_check(bigp->head + i) ;

		if (rs < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las/busintgrp_sanitycheck: i=%d LBUSINT=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
	}
#endif

			break ;
		}

	} /* end for */


	return rs ;
}
/* end subroutine (busintgrp_sanitycheck) */


static int 
RFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP wr ;

	struct las_packet * pp;

	uint	rfmodmask ;

	int ret ;


	rfmodmask = (lip->rfmod - 1) ;

/* send old destination 1 (DST1) */

	pp = &lasp->c.event_table[SEND_VALUE][RFWB][OLDDST1];
	if (pp->valid) {

	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst1.ftseq ;
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_VALUE][RFWB][OLDDST1].valid = 0;
#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent old dst1 on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst1.ftseq = (lasp->c.dst1.ftseq + 1) & rfmodmask ;

		} else {
#if	F_LASDETAILS
		las_print_asid(lasp);
			printf("unsuccessful sending old dst1 on RFWB at %d\n", 
				lasp->asid);
#endif
		}
	}

/* send old destination 2 (DST2) */

	pp = &lasp->c.event_table[SEND_VALUE][RFWB][OLDDST2];
	if (pp->valid) {

	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst2.ftseq ;
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_VALUE][RFWB][OLDDST2].valid = 0;
#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent old dst2 on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst2.ftseq = (lasp->c.dst2.ftseq + 1) & rfmodmask ;

		} else {
#if	F_LASDETAILS
		las_print_asid(lasp);
			printf("unsuccessful sending old dst2 on RFWB at %d\n", 
				lasp->asid);
#endif
		}
	}

/* send NULLIFY destination 1 (DST1) */

	pp = &lasp->c.event_table[SEND_NULLIFY][RFWB][DST1];
	if (pp->valid) {

	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst1.ftseq ;
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_NULLIFY][RFWB][DST1].valid = 0;
#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent nullify on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst1.ftseq = (lasp->c.dst1.ftseq + 1) & rfmodmask ;

		} else {
#if	F_LASDETAILS
		las_print_asid(lasp);
			printf("unsuccessful sending nulify on RFWB at %d\n", 
				lasp->asid);
#endif
		}
	}

	pp = &lasp->c.event_table[SEND_NULLIFY][RFWB][DST2];
	if (pp->valid) {
		
	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst2.ftseq ;
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_NULLIFY][RFWB][DST2].valid = 0;
#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent nullify on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst2.ftseq = (lasp->c.dst2.ftseq + 1) % rfmodmask ;

		} else {
#if	F_LASDETAILS
		las_print_asid(lasp);
			printf("unsuccessful sending nulify on RFWB at %d\n", 
				lasp->asid);
#endif
		}
	}

	if(lasp->c.pI == 0)
		return 0;

	pp = &lasp->c.event_table[SEND_VALUE][RFWB][DST1];
	if (pp->valid) {

	    pp->d = lasp->c.dst1.val;
	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst1.ftseq ;
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {
			lasp->n.event_table[SEND_VALUE][RFWB][DST1].valid=0;
#if	F_LASDETAILS
			las_print_asid(lasp);
			printf(
			    "successful write to RFWB at %d; reg(%d)=0x%x\n",
				 lasp->asid,wr.addr,wr.dv);
#endif

		lasp->n.dst1.ftseq = (lasp->c.dst1.ftseq + 1) % rfmodmask ;

		} else {
#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RFWB at %d\n", 
				lasp->asid);
#endif
		}
	}

	pp = &lasp->c.event_table[SEND_VALUE][RFWB][DST2];
	if (pp->valid) {

	    pp->d = lasp->c.dst2.val;
	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst2.ftseq ;
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority, &wr) ;

		if (ret >= 0) {

			lasp->n.event_table[SEND_VALUE][RFWB][DST2].valid=0;
#if	F_LASDETAILS
			las_print_asid(lasp);
			printf(
			    "successful write to RFWB at %d; reg(%d)=0x%x\n",
				lasp->asid,wr.addr,wr.dv);
#endif

		lasp->n.dst2.ftseq = (lasp->c.dst2.ftseq + 1) & rfmodmask ;

		} else {

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RFWB at %d\n", 
				lasp->asid);
#endif

		}
	}


	return 0 ;
}
/* end subroutine (RFWB_interface) */


static int 
RBWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP wr ;

	LBUSINT *busintp ;

	struct las_packet * pp;

	uint	rbmodmask ;

	int ret ;
	int	mi ;


	rbmodmask = (lip->rbmod - 1) ;

/* request source 1 (SRC1) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][SRC1];
	if (pp->valid == 1) {

#if	F_LASDETAILS && F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: SRC1 was valid\n") ;
#endif

		read_packet(pp, &wr);

		if (wr.addr == 0) {
			lasp->n.event_table[REQ_VALUE][RBWB][SRC1].valid=0;
			return 0;
		}

	    wr.seq = lasp->c.src1.btseq ;
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

			lasp->n.event_table[REQ_VALUE][RBWB][SRC1].valid=0;
			lasp->n.event_table[WAITON_VALUE][RFWB][SRC1].valid = 0 & 1;

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.src1.btseq = (lasp->c.src1.btseq + 1) & rbmodmask ;

		} else {

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif
		}
	}

/* request source 2 (SRC2) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][SRC2];
	if(pp->valid == 1) {

#if	F_LASDETAILS && F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: SRC2 was valid\n") ;
#endif
		read_packet(pp, &wr);

		if (wr.addr == 0) {
			lasp->n.event_table[REQ_VALUE][RBWB][SRC2].valid=0;
			return 0;
		}

	    wr.seq = lasp->c.src2.btseq ;
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {
			lasp->n.event_table[WAITON_VALUE][RFWB][SRC2].valid = 0 & 1;
			lasp->n.event_table[REQ_VALUE][RBWB][SRC2].valid = 0;
#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.src2.btseq = (lasp->c.src2.btseq + 1) & rbmodmask ;

		} else {

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif

		}
	}

/* request destination 1 (DST1) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][DST1];
	if (pp->valid == 1) {

#if	F_LASDETAILS && F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: DST1 was valid\n") ;
#endif

		read_packet(pp, &wr);

		if (wr.addr == 0) {
			lasp->n.event_table[REQ_VALUE][RBWB][DST1].valid=0;
			return 0;
		}

	    wr.seq = lasp->c.dst1.btseq ;
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

			lasp->n.event_table[REQ_VALUE][RBWB][DST1].valid=0;
			lasp->n.event_table[WAITON_VALUE][RFWB][DST1].valid = 0 & 1;

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.dst1.btseq = (lasp->c.dst1.btseq + 1) & rbmodmask ;

		} else {

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif
		}
	}

/* request destination 2 (DST2) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][DST2];
	if (pp->valid == 1) {

#if	F_LASDETAILS && F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: DST2 was valid\n") ;
#endif

		read_packet(pp, &wr);

		if (wr.addr == 0) {
			lasp->n.event_table[REQ_VALUE][RBWB][DST2].valid=0;
			return 0;
		}

	    wr.seq = lasp->c.dst2.btseq ;
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

			lasp->n.event_table[REQ_VALUE][RBWB][DST2].valid=0;
			lasp->n.event_table[WAITON_VALUE][RFWB][DST2].valid = 0 & 1;

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.dst2.btseq = (lasp->c.dst2.btseq + 1) & rbmodmask ;

		} else {

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif
		}
	}


	return 0 ;
}
/* end subroutine (RBWB_interface) */


static int 
PFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP wr ;

	int ret ;


	if (lasp->c.fw_pred_on_pfwb == 1) {

	    lflowgroup_clear(&wr,lasp->c.tt) ;

	    wr.trans = 0 ;
	    wr.path = lasp->c.path ;
	    wr.tt =	lasp->c.tt ;
	    wr.addr = lasp->c.pregs.pout.a ;
	    wr.dv = lasp->c.pregs.pout.val & 1;
	    wr.dp = LFLOWGROUP_DPALL ;	/* set data present */

/* bit 2:cpout, bit 3: cpout valid */

	    if (lasp->c.pregs.cpout.a != -1)
	    	wr.dv += (2 * lasp->c.pregs.cpout.val + 4) ; 

	    /* request pfwb bus */
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->pfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp -> n.fw_pred_on_pfwb = 0 ; /* changed form n to c */
#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("successful write to PFWB at %d, pout(%d)=%d\n", 
			lasp->asid, lasp->c.pregs.pout.a,wr.dv);
#endif

	    } else {
#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("unsuccessful write to PFWB at %d\n", lasp->asid);
#endif
	    }
	}

	return 0 ;
}
/* end subroutine (PFWB_interface) */


static int 
MFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP wr ;

	LBUSINT	*busintp ;
	struct las_packet * pp;

	int ret ;
	int	mi ;


	pp = &lasp->c.event_table[SEND_NULLIFY][MFWB][MEM];
	if (pp->valid) {

	    read_packet(pp, &wr);

	    wr.addr &= 0xfffffffc;
		wr.ftt = lasp->c.tt ;

	    mi = getinterleave(lasp->mfwb_intleave,wr.addr) ;

	    busintp = lasp->mfwbus.head + mi ;
	    ret = lbusint_write(busintp,lasp->buspriority, &wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_NULLIFY][MFWB][MEM].valid = 0;
#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent nullify on MFWB at %d; mem(0x%x)\n", 
			lasp->asid,wr.addr);
#endif

	    } else {

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("unsuccessful in sending nulify on MFWB at %d\n", 
			lasp->asid);
#endif

	    }

	} else if (lasp->c.pI == 0)
		return 0;

	pp = &lasp->c.event_table[SEND_VALUE][MFWB][MEM];
	if (pp->valid) {

		pp->d = lasp->c.mem.val;
		pp->a = lasp->c.mem.a;
		read_packet(pp, &wr);

		wr.addr &= 0xfffffffc;
		wr.ftt = lasp->c.tt ;

		mi = getinterleave(lasp->mfwb_intleave,wr.addr) ;

		busintp = lasp->mfwbus.head + mi ;
		ret = lbusint_write(busintp, lasp->buspriority,&wr) ;

		if (ret >= 0) {

			lasp->n.event_table[SEND_VALUE][MFWB][MEM].valid=0;
#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to MFWB at %d; "
				"mem(0x%x)=0x%x\n", 
				lasp->asid,wr.addr,wr.dv);
#endif

		} else  {

#if	F_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to MFWB at %d\n", 
				lasp->asid);
#endif

		}

	}

	return 0 ;
}
/* end subroutine (MFWB_interface) */


static int 
MBWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP wr ;

	LBUSINT *busintp ;

	struct las_packet * pp;

	int	rs1 ;
	int	ret ;
	int	mi ;


	pp = &lasp->c.event_table[REQ_VALUE][MBWB][MEM];

	if (pp->valid) {

		pp->a = lasp->c.mem.a;
		read_packet(pp, &wr);

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/MBWB_interface: 2 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

		wr.addr &= 0xfffffffc;
		wr.ftt = lasp->c.tt ;

		mi = getinterleave(lasp->mbwb_intleave,wr.addr) ;

		busintp = lasp->mbwbus.head + mi ;
	    ret = lbusint_write(busintp, lasp->buspriority,&wr) ;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/MBWB_interface: 3 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	    if (ret >= 0) {

		lasp->n.event_table[REQ_VALUE][MBWB][MEM].valid = 0;
		lasp->n.event_table[WAITON_VALUE][MFWB][MEM].valid = 0 & 1;

		/*lasp -> n.wait_ack_on_mfwb = LAS_LOAD_WATCHDOG; */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/MBWB_interface: 3 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("successful write to MBWB at %d; mem(0x%x)\n", 
			lasp->asid,wr.addr);
#endif

	    } else {

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("unsuccessful write to MBWB at %d\n", 
			lasp->asid);
#endif

		}

	} /* end if (valid) */

	return 0 ;
}
/* end subroutine (MBWB_interface) */


static int 
PE_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	struct lpe_data ip;
	

	if (lasp->c.re_exec_inst == 1 && lasp->c.pI == 1) {

		ip.opexec = lasp->c.opexec;
		ip.src1 = lasp->c.src1.a ? lasp -> c.src1.val : 0;
		ip.src2 = lasp->c.src2.a ? lasp -> c.src2.val : 0;
		ip.src3 = lasp->c.src3.a ? lasp -> c.src3.val : 0;
		ip.src4 = lasp->c.src4.a ? lasp -> c.src4.val : 0;
		ip.dst1 = lasp->c.dst1.a ? lasp -> c.dst1.val : 0;
		ip.dst2 = lasp->c.dst2.a ? lasp -> c.dst2.val : 0;
		ip.cnst = lasp -> c.cnst.val;
		ip.mema = lasp -> c.mem.a;


		if(lpe_req2exec(lasp->lpes, &ip, lasp->c.tt, &lasp->pe_tnumb) == SR_OK) {
		/*	printf("tnumber at AS %d,PE %d = %d\n",lasp->asid,lasp->lpes, lasp->pe_tnumb); */
			lasp->n.re_exec_inst = 0;
			return 0;
		}
		else
			return 0;
	}

	return 0 ;
}
/* end subroutine (pe_interface) */


static int 
snoop_RFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	rd ;

	LBUSINT 	*busintp ;

	uint	rfmodmask ;

	int	rs ;
	int	j, k ;
	int	f_match ;
	int	lastttsrc1 = INT_MIN;
	int	lastttsrc2 = INT_MIN;
	int	lastttdst1 = INT_MIN;
	int	lastttdst2 = INT_MIN;
	int	sgtt ;


/* get our Sharing Group TT */

	sgtt = +(lasp->c.tt / lip->nasrpsg) * lip->nasrpsg ;
	rfmodmask = lip->rfmod - 1 ;

	busintp = lasp->rfwbus.head ;

/* loop reading all buses that we are supposed to read */

/****

	This step (coming up) also filters out some extraneous
	transactions that would not be caught below but which might
	cause problems with the current AS logic since there is some
	hackery around the setting of the latest time-tag value for an
	operand.  At worst, this extra filtering cannot do any harm
	since it is so weak.

	This filtering removes structurally impossible transactions
	(which were also ignored) previously.  But it also ignores
	extra transactions that have arrived delayed compared with
	other transactions from the same past ordered time (TT).

****/

	rs = las_readrfbuses(lasp,pip,lip,busintp,lasp->rfwbus.num) ;

	if (rs < 0)
		return rs ;

/* loop through filtered snoops */

	for (k = 0 ; k < lasp->rfwbus.num ; k += 1) {

	    if (lasp->busregs[k].f_v) {

		rd = lasp->busregs[k].fg ;

/* source 1 (src1) */

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: SRC1 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.src1.a,
			lasp->c.src1.latesttt,
			lasp->c.src1.tt,lasp->c.src1.frseq) ;
#endif

		f_match = TRUE ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.src1.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.src1.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	F_EXTRASNOOP
		if (lasp->c.src1.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.src1.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.src1.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttsrc1);

#if	F_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.src1.tt) {

			if (seqok(rd.seq,lasp->c.src1.frseq,lip->rfmod))
			    lasp->n.src1.frseq = (rd.seq + 1) & rfmodmask ;

			else
			    f_match = FALSE ;

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.src1.frseq = (rd.seq + 1) & rfmodmask ;

		} /* end if (sequence check) */
#endif /* F_EXTRASNOOP */

		if (f_match) {

#if	F_FPRINTF && F_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d SRC1 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.src1.trans = rd.trans ;
		    lasp->n.src1.tt = rd.tt ;
		    lastttsrc1 = rd.tt;
		    if (rd.trans == LFLOWGROUP_TNULLIFY) {

		    fill_packet(
			&lasp->n.event_table[RECV_NULLIFY][RFWB][SRC1], &rd);

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, Nullify from tt=%d; src1(%d)\n", 
			lasp->asid, rd.tt,lasp->c.src1.a);
#endif

		} else {

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf(
		    "Snoop on RFWB at %d, data from tt=%d; src1(%d)=0x%x\n",
			lasp->asid, rd.tt,lasp->c.src1.a,rd.dv);
#endif
		    fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][SRC1],
					&rd);
			lasp->n.event_table[RECV_NULLIFY][RFWB][SRC1].valid=0;

		/* to fix the problem with multiple span that dave had */
			lasp->n.event_table[REQ_VALUE][RBWB][SRC1].valid = 0;

		}
	    }

/* source 2 (src2) */

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: SRC2 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.src2.a,
			lasp->c.src2.latesttt,
			lasp->c.src2.tt,lasp->c.src2.frseq) ;
#endif

		f_match = TRUE ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.src2.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.src2.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	F_EXTRASNOOP
		if (lasp->c.src2.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.src2.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.src2.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttsrc2);

#if	F_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.src2.tt) {

			if (seqok(rd.seq,lasp->c.src2.frseq,lip->rfmod))
			    lasp->n.src2.frseq = (rd.seq + 1) & rfmodmask ;

			else
			    f_match = FALSE ;

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.src2.frseq = (rd.seq + 1) & rfmodmask ;

		} /* end if (sequence check) */
#endif /* F_EXTRASNOOP */

		if (f_match) {

#if	F_FPRINTF && F_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d SRC2 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.src2.trans = rd.trans ;
		    lasp->n.src2.tt = rd.tt ;
		    lastttsrc2 = rd.tt;
		    if (rd.trans == LFLOWGROUP_TNULLIFY) {

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, Nullify from tt=%d; src2(%d)\n",
			lasp->asid, rd.tt,lasp->c.src2.a);
#endif

		    fill_packet(&lasp->n.event_table[RECV_NULLIFY][RFWB][SRC2],
				&rd);

		} else {

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; src2(%d)=0x%x\n",
			 lasp->asid, rd.tt,lasp->c.src2.a,rd.dv);
#endif

		    fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][SRC2],
			&rd);

			lasp->n.event_table[RECV_NULLIFY][RFWB][SRC2].valid=0;

		/* to fix the problem with multiple span that dave had */
			lasp->n.event_table[REQ_VALUE][RBWB][SRC2].valid = 0;

		}
	    }

/* destination 1 (dst1) */

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: DST1 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.dst1.a,
			lasp->c.dst1.latesttt,
			lasp->c.dst1.tt,lasp->c.dst1.frseq) ;
#endif

		f_match = TRUE ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst1.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst1.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	F_EXTRASNOOP
		if (lasp->c.dst1.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.dst1.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.dst1.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttdst1);

#if	F_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.dst1.tt) {

			if (seqok(rd.seq,lasp->c.dst1.frseq,lip->rfmod))
			    lasp->n.dst1.frseq = (rd.seq + 1) & rfmodmask ;

			else
			    f_match = FALSE ;

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.dst1.frseq = (rd.seq + 1) & rfmodmask ;

		} /* end if (sequence check) */
#endif /* F_EXTRASNOOP */

		if (f_match) {

#if	F_FPRINTF && F_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d DST1 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.dst1.trans = rd.trans ;
		    lasp->n.dst1.tt = rd.tt ;

		}

		if (f_match && (rd.trans != LFLOWGROUP_TNULLIFY)) {

		    lastttdst1 = rd.tt;

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; dst(%d)=0x%x\n",
			lasp->asid, rd.tt,lasp->c.dst1.a,rd.dv);
#endif

		fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][DST1],&rd);

	    }

/* destination 2 (dst2) */

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: DST2 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.dst2.a,
			lasp->c.dst2.latesttt,
			lasp->c.dst2.tt,lasp->c.dst2.frseq) ;
#endif

		f_match = TRUE ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst2.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst1.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	F_EXTRASNOOP
		if (lasp->c.dst2.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.dst2.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.dst2.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttdst2);

#if	F_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.dst2.tt) {

			if (seqok(rd.seq,lasp->c.dst2.frseq,lip->rfmod))
			    lasp->n.dst2.frseq = (rd.seq + 1) & rfmodmask ;

			else
			    f_match = FALSE ;

#if	F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.dst2.frseq = (rd.seq + 1) & rfmodmask ;

		} /* end if (sequence check) */
#endif /* F_EXTRASNOOP */

		if (f_match) {

#if	F_FPRINTF && F_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d DST2 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.dst2.trans = rd.trans ;
		    lasp->n.dst2.tt = rd.tt ;

		}

		if (f_match && (rd.trans != LFLOWGROUP_TNULLIFY)) {

		    lastttdst2 = rd.tt;

#if	F_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; dst(%d)=0x%x\n",
			lasp->asid, rd.tt,lasp->c.dst1.a,rd.dv);
#endif

		fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][DST2],
			&rd);

	        }

	    } /* end if */

/* get out early if we are in the first Sharing Group (now optional) */

	    if (sgtt <= 0)
		break ;

	} /* end for (k) */

	return 0 ;
}
/* end subroutine (snoop_RFWB_interface) */


/* snoop the backwarding bus to catch requests for our output operands */
int snoop_RBWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	rd ;

	LBUSINT		*busp ;
	LBUSINT		*busintp ;

	uint	rbmodmask ;

	int	rs ;
	int	n, k ;
	int	f_match ;
	int	lastttdst1 = INT_MIN;
	int	lastttdst2 = INT_MIN;
	int	sgtt ;
	int	ovtt ;


/* get our Sharing Group TT */

	sgtt = +(lasp->c.tt / lip->nasrpsg) * lip->nasrpsg ;
	ovtt = lip->nsg * lip->nasrpsg ;
	rbmodmask = lip->rbmod - 1 ;

	n = lasp->rbwbus.num ;
	rd.addr = lasp->c.src1.a;

	busintp = lasp->rbwbus.head ;
	for (k = 0 ; k < n ; k += 1) {

	    rs = lbusint_read(busintp,&rd) ;

	    if ((rs > 0) && (rd.ftt > lasp->c.tt)) {

		f_match = TRUE ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst1.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst1.val) ; */
		f_match = f_match && (rd.tt > lasp->c.tt) ;
		f_match = f_match && (rd.tt >= lastttdst1);

		if (f_match) {

		    lastttdst1 = rd.tt;
		    fill_packet(
			&lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST1],&rd);

#if	F_LASDETAILS
			las_print_asid(lasp);
		    printf("Snoop on RBWB at %d, data from tt=%d;reg(%d)\n", 
			lasp->asid, rd.tt,rd.addr);
#endif
		}
		
		f_match = TRUE ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst2.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst2.val) ; */
		f_match = f_match && (rd.tt > lasp->c.tt) ;
	        f_match = f_match && (rd.tt >= lastttdst2);

	        if (f_match) {

		    lastttdst2 = rd.tt;
		    fill_packet(
			&lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST2],&rd);

#if	F_LASDETAILS
			las_print_asid(lasp);
		    printf("Snoop on RBWB at %d, data from tt=%d;reg(%d)\n",
			lasp->asid, rd.tt,rd.addr);
#endif
		}
		
            } /* end if (read something) */

	    busintp += 1 ;

/* get out early if we are the **last** Sharing Group in the e-window */

	    if (sgtt >= (ovtt - lip->nasrpsg))
		break ;

	} /* end for (looping on backwarding buses) */

	return 0 ;
}
/* end subroutine (snoop_RBWB_interface) */


/* snoop the BP bus, save results in the snooped register */
static int 
snoop_PFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	rd ;

	LBUSINT 	*busintp ;

	int	rs ;
	int	l,j, k,lll ;
	int	f_match ;
	int	lastttpin = INT_MIN ;
	int	sgtt ;


/* get our Sharing Group TT */

	sgtt = +(lasp->c.tt / lip->nasrpsg) * lip->nasrpsg ;

	busintp = lasp->pfwbus.head ;
	for (k = 0 ; k < lasp->pfwbus.num ; k += 1) {

	    rs = lbusint_read(busintp,&rd) ;

	    if ((rs > 0) && (rd.ftt <= lasp->c.tt)) {

			lasp->n.pregs.pin.latesttt = lasp->c.pregs.pin.latesttt;
			f_match = TRUE ;
			f_match = f_match && (rd.path == lasp->c.path) ;
			f_match = f_match && (rd.addr == lasp->c.pregs.pin.a) ;
			/*    f_match = f_match && (rd.d != lasp->c.pregs.pin.val) ; */
			f_match = f_match && (rd.tt < lasp->c.tt) ;
			f_match = f_match && (rd.tt >= lasp->c.pregs.pin.latesttt) ;
		        f_match = f_match && (rd.tt >= lasp->n.pregs.pin.latesttt) ;
		        if (f_match) {
			    lasp->n.pregs.pin.latesttt = rd.tt ;
			    lasp->n.pregs.pin.val = rd.dv & 1 ;
#if	F_LASDETAILS
			    las_print_asid(lasp);
			    printf("Snoop on PFWB at %d, data from tt=%d; pin(%d)=0x%x\n", lasp->asid, rd.tt,lasp->c.pregs.pin.a, rd.dv);
#endif
			}

			for (l=0; l < lasp->c.pregs.num_of_cpin_regs; ++l) {

				lasp->n.pregs.cpin[l].latesttt = lasp->n.pregs.cpin[l].latesttt;
				f_match = TRUE ;
				f_match = f_match && (rd.path == lasp->c.path) ;
				f_match = f_match && (rd.addr == lasp->c.pregs.cpin[l].a) ;
				/*    f_match = f_match && (rd.d != lasp->c.pregs.cpin[l].val) ; */
				f_match = f_match && (rd.tt < lasp->c.tt) ;
				f_match = f_match && (rd.tt >= lasp->c.pregs.cpin[l].latesttt) ;
				f_match = f_match && ((rd.dv & 4) == 4);
			        f_match = f_match && (rd.tt >= lasp->n.pregs.cpin[l].latesttt) ;
			        if (f_match) {
				    lasp->n.pregs.cpin[l].latesttt = rd.tt ;
				    lasp->n.pregs.cpin[l].val = rd.dv & 2 ? 1 : 0;

#if	F_LASDETAILS
				    las_print_asid(lasp);
				    printf("Snoop on PFWB at %d, data from tt=%d; cpin[%d](%d)=0x%x\n", lasp->asid, rd.tt,l,lasp->c.pregs.cpin[l].a, rd.dv);
#endif
				}
		
		        } /* end for */

	  	}
	    ++busintp ;

/* get out early if we are the **first** Sharing Group in the e-window */

		if (sgtt <= 0)
			break ;

	} /* end for (k) */

	return 0 ;
}
/* end subroutine (snoop_PFWB_interface) */


static int 
snoop_MFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	rd ;

	LBUSINT		*busintp ;

	int	j, k ;
	int	mi ;
	int	f_match ;


	if (lasp->c.opclass != INSTR_LOAD && lasp->c.opclass != INSTR_STORE)
	    return 0 ;

	rd.addr = lasp->c.mem.a ;

/* we snoop the correct memory bus based on our address */

	mi = getinterleave(lasp->mfwb_intleave,rd.addr) ;

	busintp = lasp->mfwbus.head + mi ;

#if	F_DEBUG
	if (pip->debuglevel > 2) {
	    eprintf(
	        "las/snoop_MFWB_interface: snooping addr=%08x bi=%d\n",
	        lasp->c.mem.a,mi) ;
	    eprintf(
	        "las/snoop_MFWB_interface: reading LBUSINT=%08lx\n",
	        busintp) ;
	}
#endif

	if (lbusint_read(busintp, &rd) > 0) {

	    f_match = TRUE ;
	    f_match = f_match && (rd.path == lasp->c.path) ;
	    f_match = f_match && (rd.addr == (lasp->c.mem.a & 0xfffffffc)) ;
	    /* f_match = f_match && (rd.d != lasp->c.mem.val) ; */
	    f_match = f_match && (rd.tt < lasp->c.tt) ;
	    f_match = f_match && (rd.tt >= lasp->c.mem.latesttt) ;

	    if (f_match &&
	        (lasp->c.opclass == INSTR_LOAD)) {

#if	F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf(
	            "las/snoop_MFWB_interface: load read dv=%08x\n",rd.dv) ;
#endif

	        if (rd.trans == LFLOWGROUP_TNULLIFY) {

#if	F_LASDETAILS
	            las_print_asid(lasp) ;

	            printf("Received Nulify on MFWB at %d, "
	                "data from tt=%d\n", 
	                lasp->asid, rd.tt) ;
#endif

	            fill_packet(
	                &lasp->n.event_table[RECV_NULLIFY][MFWB][MEM],
	                &rd) ;

	        } else {

			int	boundary ;


#if	F_LASDETAILS
	            las_print_asid(lasp) ;

	            printf("Snoop on MFWB at %d, data from tt=%d;"
	                "mem(0x%x)=0x%x\n",
	                lasp->asid, rd.tt,rd.addr,rd.dv) ;
#endif

	                lasp->n.mem.latesttt = rd.tt ;
	                boundary = lasp->c.mem.a & 3 ;

#if	F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf(
	            "las/snoop_MFWB_interface: a=%08x boundary=%d\n",
			lasp->c.mem.a,boundary) ;
#endif

	                switch (lasp->c.opexec) {

	                    int sign ;


	                case LEXECOP_LW:
	                    lasp->n.dst1.val = rd.dv ;
	                    break ;

	                case LEXECOP_LB:
	                    switch (boundary) {

			    case 0:
	                        lasp->n.dst1.val = ((rd.dv >> 24) & 0xff) ;
	                        sign = (rd.dv & 0x80000000) ? 1 : 0 ;
				break ;

			    case 1:
	                        lasp->n.dst1.val = ((rd.dv >> 16) & 0xff) ;
	                        sign = (rd.dv & 0x00800000) ? 1 : 0 ;
				break ;

			    case 2:
	                        lasp->n.dst1.val = ((rd.dv >> 8) & 0xff) ;
	                        sign = (rd.dv & 0x00008000) ? 1 : 0 ;
				break ;

			    case 3:
	                        lasp->n.dst1.val = ((rd.dv >> 0) & 0xff) ;
	                        sign = (rd.dv & 0x00000080) ? 1 : 0 ;
				break ;

	                    } /* end switch */

	                    lasp->n.dst1.val |= (sign * 0xffffff00) ;
	                    break ;

	                case LEXECOP_LBU:
			    switch (boundary) {

			    case 0:
	                        lasp->n.dst1.val = ((rd.dv >> 24) & 0xff) ;
				break ;

			    case 1:
	                        lasp->n.dst1.val = ((rd.dv >> 16) & 0xff) ;
				break ;

			    case 2:
	                        lasp->n.dst1.val = ((rd.dv >> 8) & 0xff) ;
				break ;

			    case 3:
	                        lasp->n.dst1.val = ((rd.dv >> 0) & 0xff) ;
				break ;

			    } /* end switch */

	                    break ;

	                case LEXECOP_LH:
			    switch (boundary) {

			    case 0:
	                        lasp->n.dst1.val = ((rd.dv >> 16) & 0xffff) ;
	                        sign = (rd.dv & 0x80000000) ? 1 : 0 ;
				break ;

			    case 1:
	                        lasp->n.dst1.val = ((rd.dv >> 8) & 0xffff) ;
	                        sign = (rd.dv & 0x00800000) ? 1 : 0 ;
				break ;

			    case 2:
	                        lasp->n.dst1.val = ((rd.dv >> 0) & 0x0000ffff) ;
	                        sign = (rd.dv & 0x00008000) ? 1 : 0 ;
				break ;

			    default:
	                        ; /* memory exception */

	                    } /* end switch */

	                    lasp->n.dst1.val |= (sign * 0xffff0000) ;
	                    break ;

	                case LEXECOP_LHU:
			    switch (boundary) {

			    case 0:
	                        lasp->n.dst1.val = ((rd.dv >> 16) & 0xffff) ;
				break ;

			    case 1:
	                        lasp->n.dst1.val = ((rd.dv >> 8) & 0xffff) ;
				break ;

			    case 2:
	                        lasp->n.dst1.val = ((rd.dv >> 0) & 0x0000ffff) ;
				break ;

			    default:
	                        ; /* memory exception */

	                    } /* end switch */

	                    break ;

	                case LEXECOP_LWL:
			    switch (boundary) {

			    case 0:
	                        lasp->n.dst1.val = rd.dv ;
				break ;

			    case 1:
	                        lasp->n.dst1.val = 
				    (lasp->c.src2.val & 0x000000ff) |
	                            ((rd.dv & 0x00ffffff) << 8) ;
				break ;

			    case 2:
	                        lasp->n.dst1.val = 
				    (lasp->c.src2.val & 0x0000ffff) |
	                            ((rd.dv & 0x0000ffff) << 16) ;
				break ;

			    case 3:
	                        lasp->n.dst1.val = 
				    (lasp->c.src2.val & 0x00ffffff) |
	                            ((rd.dv & 0x000000ff) << 24) ;
				break ;

			    } /* end switch */

	                    break ;

	                case LEXECOP_LWR:
			    switch (boundary) {

			    case 0:
	                        lasp->n.dst1.val = 
				    (lasp->c.src2.val & 0xffffff00) |
	                            ((rd.dv >> 24) & 0xff) ;
				break ;

			    case 1:
	                        lasp->n.dst1.val = 
				    (lasp->c.src2.val & 0xffff0000) |
	                            ((rd.dv >> 16) & 0xffff) ;
				break ;

			    case 2:
	                        lasp->n.dst1.val = 
				    (lasp->c.src2.val & 0xff000000) |
	                            ((rd.dv >> 8) & 0x00ffffff) ;
				break ;

			    case 3:
	                        lasp->n.dst1.val = rd.dv ;

			    } /* end switch */

	                    break ;

	                default:
	                    assert("illegal opcode") ;
	                    break ;

	                } /* end switch */

#if	F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf(
	            "las/snoop_MFWB_interface: DST dv=%08x\n",
	                        lasp->n.dst1.val) ;
#endif

	            rd.dv = lasp->n.dst1.val ;
	            fill_packet(
	                &lasp->n.event_table[RECV_VALUE][MFWB][MEM], &rd) ;

	        } /* end if (NULLIFY or not) */

	    } /* end if (match) */

	} /* end if (bus had something) */

	return 0 ;
}
/* end subroutine (snoop_MFWB_interface) */


static int 
snoop_MBWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	rd ;

	LBUSINT		*busp ;
	LBUSINT		*busintp ;

	int	j, k ;
	int	mi ;
	int	f_match ;


	lip = lasp->lip ;

	rd.addr = lasp->c.mem.a;

/* only the bus with the right interleaving number needs to be snooped */

	mi = getinterleave(lasp->mbwb_intleave,rd.addr) ;

	busintp = lasp->mbwbus.head + mi ;

	if (lbusint_read(busintp, &rd) > 0) {

	    f_match = TRUE ;
	    f_match = f_match && (rd.path == lasp->c.path) ;
	    f_match = f_match && (rd.addr == (lasp->c.mem.a & 0xfffffffc)) ;
	    /*    f_match = f_match && (rd.d != lasp->c.mem.val) ; */
	    f_match = f_match && (rd.tt > lasp->c.tt) ;

	    if (f_match && (lasp->c.opclass == INSTR_STORE)) {

		fill_packet(
			&lasp->n.event_table[RECV_REQ_VALUE][MBWB][MEM],
			&rd);

#if	F_LASDETAILS
		las_print_asid(lasp);
		    printf(
			"Snoop on MBWB at %d, data from tt=%d; mem(0x%x)\n", 
				lasp->asid, rd.tt, rd.addr);
#endif

		}

	} /* end if */

	return 0 ;
}
/* end subroutine (snoop_MBWB_interface) */


static int
PE_checkresult(lasp)
LAS * lasp;
{
	struct lpe_data		ip;

	LFLOWGROUP rd;

	int ret;
	int boundry;


    if (lasp->pe_tnumb != -1) {

	ret = lpe_isexecuted(lasp->lpes, &ip, lasp->pe_tnumb);
	if (ret == SR_OK) {
	/*	printf("lpe %d executed:%d\n",lasp->lpes,lasp->pe_tnumb); */
		lasp->pe_tnumb = -1;
		if(lasp->c.dst1.a == 0)
			lasp->n.dst1.val = 0;
		else
			lasp->n.dst1.val = ip.dst1 ;
		if(lasp->c.dst2.a == 0)
			lasp->n.dst2.val = 0;
		else
			lasp->n.dst2.val = ip.dst2 ;
		lasp->n.mem.a = ip.mema ;
		lasp->n.bcond = ip.bcnd ;

		rd.tt = lasp->c.tt;
		rd.path = lasp->c.path;
		
		if(lasp->c.opclass == INSTR_ALU) {
			rd.dv = ip.dst1 ;
			rd.addr = lasp->c.dst1.a;
			fill_packet(&lasp->n.event_table[EXEC_ALU][PE][DST1], &rd);
			if(lasp->c.dst2.a != -1) {
				rd.dv = ip.dst2 ;
				rd.addr = lasp->c.dst2.a;
				fill_packet(&lasp->n.event_table[EXEC_ALU][PE][DST2], &rd);
			}

		} else if(lasp->c.opclass == INSTR_STORE) {

			if ((lasp->c.mem.a != -1) && (lasp->c.old_mem.a != ip.mema)) {

			/* do not send nulify for the first execution */
				/* rd.addr = ip.mema ; */
				fill_packet(&lasp->n.event_table[RECV_NULLIFY][PE][MEM], &rd);
				/*lasp->n.send_nulify_on_mfwb = 1; */
			}
			rd.addr = ip.mema ;
			boundry = rd.addr % 4;
			switch(lasp->c.opexec) {
				case LEXECOP_SW:
					lasp->n.mem.val = lasp->c.src2.val;
					lasp->n.dp = LFLOWGROUP_DPALL;
					break;
				case LEXECOP_SB:

					if(boundry == 0) {
						lasp->n.dp = LFLOWGROUP_DPBYTE3;	
						lasp->n.mem.val = (lasp->c.src2.val & 0xff) << 24;
					} 
					else if(boundry == 1) {
						lasp->n.dp = LFLOWGROUP_DPBYTE2;	
						lasp->n.mem.val = (lasp->c.src2.val & 0xff) << 16;
					} 
					else if(boundry == 2) {
						lasp->n.dp = LFLOWGROUP_DPBYTE1;	
						lasp->n.mem.val = (lasp->c.src2.val & 0xff) << 8;
					} 
					else {
						lasp->n.dp = LFLOWGROUP_DPBYTE0;	
						lasp->n.mem.val = (lasp->c.src2.val & 0xff);
					} 
					break;
					
				case LEXECOP_SH:
					if(boundry == 0) {
						lasp->n.dp = LFLOWGROUP_DPBYTE3 | LFLOWGROUP_DPBYTE3;
						lasp->n.mem.val = (lasp->c.src2.val & 0xffff) << 16;
					} 
					else if(boundry == 1) {
						lasp->n.dp = LFLOWGROUP_DPBYTE2 | LFLOWGROUP_DPBYTE3;
						lasp->n.mem.val = (lasp->c.src2.val & 0xffff) << 8;
					} 
					else if(boundry == 2) {
						lasp->n.dp = LFLOWGROUP_DPBYTE1 | LFLOWGROUP_DPBYTE3;
						lasp->n.mem.val = (lasp->c.src2.val & 0xffff);
					} 
					else {
						/* mosalligned mem access */	
					}
					break;
					
				case LEXECOP_SWL:
					lasp->n.mem.val = (lasp->c.src2.val) >> (8*boundry);
					if(boundry == 0) {
						lasp->n.dp = LFLOWGROUP_DPALL;
					} 
					else if(boundry == 1) {
						lasp->n.dp = LFLOWGROUP_DPBYTE2 | LFLOWGROUP_DPBYTE1 | LFLOWGROUP_DPBYTE0;
					} 
					else if(boundry == 2) {
						lasp->n.dp = LFLOWGROUP_DPBYTE1 | LFLOWGROUP_DPBYTE0;
					} 
					else {
						lasp->n.dp = LFLOWGROUP_DPBYTE0;	
					} 
					break;

				case LEXECOP_SWR:
					lasp->n.mem.val = lasp->c.src2.val << ((3-boundry)*8);
						lasp->n.dp = LFLOWGROUP_DPBYTE3 | LFLOWGROUP_DPBYTE1 | LFLOWGROUP_DPBYTE0;
					if(boundry == 0) {
						lasp->n.dp = LFLOWGROUP_DPBYTE3;
					} 
					else if(boundry == 1) {
						lasp->n.dp = LFLOWGROUP_DPBYTE3 | LFLOWGROUP_DPBYTE2;
					} 
					else if(boundry == 2) {
						lasp->n.dp = LFLOWGROUP_DPBYTE3 | LFLOWGROUP_DPBYTE2 | LFLOWGROUP_DPBYTE1;
					} 
					else {
						lasp->n.dp = LFLOWGROUP_DPALL;	
					} 
					break;

				default:
					assert("unknown OP!");
					break;
			}
			rd.dv = lasp->n.mem.val;
			rd.dp = lasp->n.dp;
			fill_packet(&lasp->n.event_table[EXEC_ST][PE][MEM], &rd);

		} else if(lasp->c.opclass == INSTR_LOAD) {

			rd.addr = ip.mema ;
			fill_packet(&lasp->n.event_table[EXEC_LD][PE][MEM], &rd);
			/*lasp->n.fw_mem_on_mbwb = 1; */

		} else if(lasp->c.opclass == INSTR_BREL || lasp->c.opclass == INSTR_JREL ||

			lasp->c.opclass == INSTR_JIND) {
			rd.dv = ip.bcnd ;
			fill_packet(&lasp->n.event_table[EXEC_BR][PE][BCOND], &rd);
			if(lasp->c.opexec == LEXECOP_JAL) {
				lasp->n.dst1.val = lasp->c.faddr + 8;
			}
			if(lasp->c.opexec == LEXECOP_JR) {
				lasp->n.cnst.val = lasp->c.src1.val;
			}
		}

	} else if(ret == SR_INPROGRESS)
		;	

    }	

	return 0 ;
}
/* end subroutine (PE_checkresult) */


/* do the main part of the AS combinatorial logic */
static int AS_logic(lasp,pip,lip)
LAS 		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	struct las_packet * pp, * pp2;

	int	rs = SR_OK ;
	int	rs1 ;
	int boundry;


	if (lasp->c.asstate == UNUSED)
		return 0;

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 0 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	if (lasp->c.regfile_read == 0) {


		if(lasp->asid == 0) {
		/*	alirgf_print(lasp->rgfp);
			lwq_print(lasp->lwqp);
			lwq_print_wq(lasp->lwqp); */
		}
		alirgf_read(lasp->rgfp,lasp); 
		if(lasp->c.src1.a != -1)
		send_packet(lasp, &lasp->n.event_table[REQ_VALUE][RBWB][SRC1], SRC1,LFLOWGROUP_TSTORE);
		if(lasp->c.src2.a != -1)
		send_packet(lasp, &lasp->n.event_table[REQ_VALUE][RBWB][SRC2], SRC2,LFLOWGROUP_TSTORE);

/* extra requests for relay forwarding */

		if (lasp->rftype == 0) {

		if(lasp->c.dst1.a != -1)
		send_packet(lasp, &lasp->n.event_table[REQ_VALUE][RBWB][DST1], DST1,LFLOWGROUP_TSTORE);
		if(lasp->c.dst2.a != -1)
		send_packet(lasp, &lasp->n.event_table[REQ_VALUE][RBWB][DST2], DST2,LFLOWGROUP_TSTORE);

		} /* end if (relay forwarding) */

		lasp->n.regfile_read = 1;
		return 0;
	}

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 1 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	if (lasp->c.re_exec_inst && lasp->pe_tnumb == -1) {
		PE_interface(lasp,pip,lip) ;
	}

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 2 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	las_predicate_logic(lasp);  

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 3 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	if(lasp->c.pI == 0)
		goto snoop_lab;

	pp = &lasp->c.event_table[EXEC_ALU][PE][DST1];
	if(pp->valid) {
		if(lasp->c.dst1.a > 0)
			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST1],
			DST1,LFLOWGROUP_TSTORE);
		lasp->n.event_table[EXEC_ALU][PE][DST1].valid=0;
	}

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 4 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[EXEC_ALU][PE][DST2];
	if(pp->valid) {
		if(lasp->c.dst2.a > 0)
			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST2],
			DST2,LFLOWGROUP_TSTORE);
		lasp->n.event_table[EXEC_ALU][PE][DST2].valid=0;
	}

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 4a bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_NULLIFY][PE][MEM];
	if(pp->valid) {

		send_packet(lasp, &lasp->n.event_table[SEND_NULLIFY][MFWB][MEM], MEM,LFLOWGROUP_TNULLIFY);
		lasp->n.event_table[SEND_NULLIFY][MFWB][MEM].a = lasp->c.old_mem.a; 

#if	F_BADBADHACK
		lasp->c.old_mem.a = lasp->c.mem.a; /* hack, sorry!! */
#endif

		lasp->n.old_mem.a = lasp->c.mem.a;

		lasp->n.event_table[RECV_NULLIFY][PE][MEM].valid=0;
	}

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 4b bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[EXEC_ST][PE][MEM]; 
	if(pp->valid) {
		send_packet(lasp, &lasp->n.event_table[SEND_VALUE][MFWB][MEM],
		MEM,LFLOWGROUP_TSTORE);
		lasp->n.event_table[EXEC_ST][PE][MEM].valid=0 ;
	}

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 5 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[EXEC_LD][PE][MEM];
	if(pp->valid) {
		lasp->n.mem.latesttt = INT_MIN;
		send_packet(lasp, &lasp->n.event_table[REQ_VALUE][MBWB][MEM],
		MEM,LFLOWGROUP_TSTORE);
		lasp->n.event_table[EXEC_LD][PE][MEM].valid=0;
	}

	pp = &lasp->c.event_table[EXEC_BR][PE][BCOND];
	if(pp->valid) {
		if(lasp->c.opexec == LEXECOP_JAL) 
			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST1],
			DST1,LFLOWGROUP_TSTORE);
		lasp->n.event_table[EXEC_BR][PE][BCOND].valid=0;
	}

	pp = &lasp->c.event_table[RECV_NULLIFY][RFWB][SRC1];
	if(pp->valid) {
		send_packet(lasp, 
		&lasp->n.event_table[REQ_VALUE][RBWB][SRC1],
		SRC1,LFLOWGROUP_TSTORE);

#if	F_BADBADHACK
		lasp->c.src1.latesttt = INT_MIN;
#endif

		lasp->n.src1.latesttt = INT_MIN;
		lasp->n.event_table[RECV_NULLIFY][RFWB][SRC1].valid=0;
		lasp->wait4src1 = 1;
	}

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][SRC1];
	if(pp->valid) {

#if	F_BADBADHACK
		lasp->c.src1.latesttt = pp->tt;
#endif

		lasp->n.src1.latesttt = pp->tt;
		if(pp->d != lasp->c.src1.val) 
			lasp->n.re_exec_inst = 1;

#if	F_BADBADHACK
		lasp->c.src1.val = pp->d;
#endif

		lasp->n.src1.val = pp->d;
		lasp->wait4src1 = 0;
		lasp->n.event_table[WAITON_VALUE][RFWB][SRC1].valid = 0;
		lasp->n.event_table[RECV_VALUE][RFWB][SRC1].valid=0;
	}

#if	F_SAFEFUNC && 0
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 11 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_NULLIFY][RFWB][SRC2];
	if(pp->valid) {
		send_packet(lasp, 
		&lasp->n.event_table[REQ_VALUE][RBWB][SRC2],
		SRC2,LFLOWGROUP_TSTORE);

#if	F_BADBADHACK
		lasp->c.src2.latesttt = INT_MIN;
#endif

		lasp->n.src2.latesttt = INT_MIN;
		lasp->wait4src2 = 1;
		lasp->n.event_table[RECV_NULLIFY][RFWB][SRC2].valid=0;
	}

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 12 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][SRC2];
	if (pp->valid) {

#if	F_BADBADHACK
		lasp->c.src2.latesttt = pp->tt;
#endif

		lasp->n.src2.latesttt = pp->tt;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 12a bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

		if (pp->d != lasp->c.src2.val) 
			lasp->n.re_exec_inst = 1;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 12b bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

#if	F_BADBADHACK
		lasp->c.src2.val = pp->d;
#endif

		lasp->n.src2.val = pp->d;
		lasp->wait4src2 = 0;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 12c bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

		lasp->n.event_table[WAITON_VALUE][RFWB][SRC2].valid = 0;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 12d bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

		lasp->n.event_table[RECV_VALUE][RFWB][SRC2].valid=0;

	} /* end if */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 13 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][DST1];
	if(pp->valid) {
		if(lasp->c.opclass != INSTR_LOAD) {

#if	F_BADBADHACK
			lasp->c.dst1.latesttt = pp->tt;
#endif

			lasp->n.dst1.latesttt = pp->tt;

#if	F_BADBADHACK
			lasp->c.dst1.oldd = pp->d;
#endif

			lasp->n.dst1.oldd = pp->d;
		}
		lasp->n.event_table[WAITON_VALUE][RFWB][DST1].valid = 0;
		lasp->n.event_table[RECV_VALUE][RFWB][DST1].valid=0;
	}

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][DST2];
	if(pp->valid) {
		if(lasp->c.opclass != INSTR_LOAD) {

#if	F_BADBADHACK
			lasp->c.dst2.latesttt = pp->tt;
#endif

			lasp->n.dst2.latesttt = pp->tt;

#if	F_BADBADHACK
			lasp->c.dst2.oldd = pp->d;
#endif

			lasp->n.dst2.oldd = pp->d;
		}
		lasp->n.event_table[RECV_VALUE][RFWB][DST2].valid=0;
		lasp->n.event_table[WAITON_VALUE][RFWB][DST2].valid = 0;
	}

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 15 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_REQ_VALUE][RBWB][DST1];

	if (pp->valid) {

		if (lasp->c.dst1.a > 0) {

			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST1],
			DST1,LFLOWGROUP_TSTORE);

		} /* end if */

		lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST1].valid=0;

	} /* end if */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 16 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_REQ_VALUE][RBWB][DST2];
	if(pp->valid) {
		if(lasp->c.dst2.a > 0)
			send_packet(lasp, &lasp->n.event_table[SEND_VALUE][RFWB][DST2],DST2,LFLOWGROUP_TSTORE);
		lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST2].valid=0;
	}

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 17 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_NULLIFY][MFWB][MEM];
	if(pp->valid) {
		if(lasp->c.opclass == INSTR_LOAD) {
			send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][MBWB][MEM],
			MEM,LFLOWGROUP_TSTORE);

			/*lasp->n.wait_ack_on_mfwb = LAS_LOAD_WATCHDOG; */

#if	F_BADBADHACK
			lasp->c.mem.latesttt = INT_MIN;
#endif

			lasp->n.mem.latesttt = INT_MIN;
		}
		lasp->n.event_table[RECV_NULLIFY][MFWB][MEM].valid=0;
	}

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 18 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_VALUE][MFWB][MEM];
	if(pp->valid) {
		if(lasp->c.dst1.a > 0)
			send_packet(lasp, &lasp->n.event_table[SEND_VALUE][RFWB][DST1],DST1,LFLOWGROUP_TSTORE);
		lasp->n.event_table[RECV_VALUE][MFWB][MEM].valid=0;
		lasp->n.event_table[WAITON_VALUE][MFWB][MEM].valid = 0;
	}

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 20 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	pp = &lasp->c.event_table[RECV_REQ_VALUE][MBWB][MEM];
	if(pp->valid) {
		send_packet(lasp, 
		&lasp->n.event_table[SEND_VALUE][MFWB][MEM],
		MEM,LFLOWGROUP_TSTORE);
		lasp->n.event_table[RECV_REQ_VALUE][MBWB][MEM].valid=0;
	}

/* sanity check */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 26 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */


/* enter here from above */
snoop_lab:

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 25 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */



/* possibly check the PE for a result */

	if (lasp->pe_tnumb != -1)
		PE_checkresult(lasp);

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 26 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */


	MFWB_interface(lasp,pip,lip) ;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 26a bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	MBWB_interface(lasp,pip,lip) ;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 27 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	RFWB_interface(lasp,pip,lip) ;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 28 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	RBWB_interface(lasp,pip,lip) ;

	PFWB_interface(lasp,pip,lip) ;

/*	las_predicate_logic(lasp);  */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 30 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	snoop_RFWB_interface(lasp,pip,lip) ;

	snoop_MFWB_interface(lasp,pip,lip) ;

	snoop_MBWB_interface(lasp,pip,lip) ;

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 32 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */

	snoop_RBWB_interface(lasp,pip,lip) ;

	snoop_PFWB_interface(lasp,pip,lip) ;

/*	las_predicate_logic(lasp); */

#if	F_SAFEFUNC
	if (lasp->sanity.func != NULL) {

	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/AS_logic: 33 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* F_SAFEFUNC */


	return SR_OK ;
}
/* end subroutine (AS_logic) */


/* this funciton calculates the predicate and canceling predicate values
   using the value of the valid predicate registers.  It also assignes values to
   the output predicate and canceling predicates.  
*/
int
las_predicate_logic(lasp)
LAS	*lasp;
{
	int p;
	int cpin=0;
	int i;


#if	PERF_FETCH

lasp -> c.pregs.pin.val = lasp -> n.pregs.pin.val = 1; 
lasp->c.pI = lasp->n.pI = 1;

/* satisfy picky compilers */

if (lasp->c.pI)
	return 1;

#endif

	for(i=0; i<lasp->c.pregs.num_of_cpin_regs; ++i)
		cpin |= lasp->c.pregs.cpin[i].val;
	p = cpin | lasp -> c.pregs.pin.val ;

	if(lasp->c.opclass == INSTR_JREL
	   || lasp->c.opclass == INSTR_JIND) {
		lasp ->n.pregs.pout.val = 0;
		lasp ->n.pregs.cpout.val = p; 
		lasp -> n.pI = p;
	}
	else if(lasp->c.opclass == INSTR_BREL) {
		lasp ->n.pregs.pout.val = lasp->c.bcond ? 0 : p;
		lasp ->n.pregs.cpout.val = lasp->c.bcond ? p : 0 ; 
		lasp -> n.pI = 1;
	} else {
		lasp ->n.pregs.pout.val = p;
		lasp ->n.pI = p;
	}
	
	if(p==1 && lasp->c.pI ==0)
		lasp->n.re_exec_inst = 1;

	if(p==0 && lasp->c.pI==1) {
		if(lasp->c.opclass == INSTR_STORE) {
			send_packet(lasp, &lasp->n.event_table[SEND_NULLIFY][MFWB][MEM],MEM,LFLOWGROUP_TNULLIFY);

		} else {

			if(lasp->c.dst1.a > 0) {

/* relay forwarding */

			if (lasp->rftype == 0)
				send_packet(lasp, &lasp->n.event_table[SEND_VALUE][RFWB][OLDDST1],OLDDST1,LFLOWGROUP_TSTORE);

/* NULLIFY forwarding */

			else if (lasp->rftype == 1)
				send_packet(lasp, &lasp->n.event_table[SEND_NULLIFY][RFWB][DST1],DST1,LFLOWGROUP_TNULLIFY);


			}

			if(lasp->c.dst2.a > 0) {

/* relay forwarding */

			if (lasp->rftype == 0)
				send_packet(lasp, &lasp->n.event_table[SEND_VALUE][RFWB][OLDDST2],OLDDST2,LFLOWGROUP_TSTORE);

/* NULLIFY forwarding */

			else if (lasp->rftype == 1)
				send_packet(lasp, &lasp->n.event_table[SEND_NULLIFY][RFWB][DST2],DST2,LFLOWGROUP_TNULLIFY);

			}
		}
	}	

	if(lasp -> c.pregs.pout.a != -1)
		if((lasp -> n.pregs.pout.val != lasp -> c.pregs.pout.val) || (lasp -> n.pregs.cpout.val != lasp -> c.pregs.cpout.val )) {
			if(lasp -> n.fw_pred_on_pfwb == 1) {
				lasp -> c.pregs.pout.val = lasp -> n.pregs.pout.val; 
				lasp -> c.pregs.cpout.val = lasp -> n.pregs.cpout.val;
			}
			else
				lasp -> n.fw_pred_on_pfwb = 1;
		}
	lasp->pred = lasp->n.pI;
/*
	if(lasp->n.pI == 0) {
		if(lasp->c.dst1.a != -1)
			lasp->n.fw_dst1_on_rfwb = 1;
		if(lasp->c.dst2.a != -1)
			lasp->n.fw_dst2_on_rfwb = 1;
	}
*/
}
/* end subroutine (las_predicate_logic) */


/* check the bus read registers */
static int las_readrfbuses(lasp,pip,lip,busintp,n)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
LBUSINT		*busintp ;
int		n ;
{

	LFLOWGROUP	*fgp, *tfgp ;

	int	rs, i, j ;

#if	F_DEBUG || F_FPRINTF
	char	ttbuf[40] ;
#endif


	for (i = 0 ; i < n ; i += 1) {

	    fgp = &lasp->busregs[i].fg ;
	    rs = lbusint_read(busintp + i,fgp) ;

	    lasp->busregs[i].f_v = ((rs > 0) && 
		(fgp->ftt <= lasp->c.tt) && (fgp->tt < lasp->c.tt)) ;

#if	F_FPRINTF && F_LASDETAILS3
		if (lasp->busregs[i].f_v)
		fprintf(stdout,
		     "las_readrfbuses: id=%d astt=%d "
			"tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			fgp->trans,fgp->path,
			fgp->tt,fgp->seq,
			fgp->addr,fgp->dp,fgp->dv) ;
#endif

#if	F_DEBUG
	if ((pip->debuglevel > 4) && (lasp->busregs[i].f_v)) {
		eprintf("las_readrfbuses: id=%d astt=%d "
			"tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			fgp->trans,fgp->path,
			fgp->tt,fgp->seq,
			fgp->addr,fgp->dp,fgp->dv) ;
	}
#endif

	} /* end for */

#if	F_EXTRASNOOP

/* perform the sequence checking on like-transactions */

	for (i = 0 ; i < n ; i += 1) {

	    if (lasp->busregs[i].f_v) {

	        LFLOWGROUP	*tfgp ;

	        int	path, addr, tt ;
	        int	besti, maxseq ;


	        fgp = &lasp->busregs[i].fg ;
	        path = fgp->path ;
	        addr = fgp->addr ;
	        tt = fgp->tt ;

	        besti = i ;
	        maxseq = fgp->seq ;
	        for (j = (i + 1) ; j < n ; j += 1) {

	    	    if (lasp->busregs[j].f_v) {

	            	tfgp = &lasp->busregs[j].fg ;
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

	    	    if (lasp->busregs[j].f_v) {

	            	tfgp = &lasp->busregs[j].fg ;
			if ((tfgp->path == path) &&
	                	(tfgp->addr == addr) &&
	                	(tfgp->tt == tt)) {

	            if (j != besti) {

	                lasp->busregs[j].f_v = FALSE ;

#if	F_DEBUG
	        if (pip->debuglevel > 1) {

	        mkttbuf(ttbuf,tfgp->tt) ;

	            eprintf(
	                "las_readrfbuses: W=%d ftt=%d "
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

			}

			} /* end if */

		    } /* end if */

	        } /* end for */

	    } /* end if */

	} /* end for */

#endif /* F_EXTRASNOOP */

	return SR_OK ;
}
/* end subroutine (las_readrfbuses) */


/* fill up a packet */
static int
fill_packet(pp, rd)
struct las_packet * pp;
LFLOWGROUP * rd;
{

	pp->valid = 1;
	pp->trans = rd->trans;
	pp->a = rd->addr;
	pp->d = rd->dv;
	pp->tt = rd->tt;
	pp->pathid = rd->path;
	pp->dp = rd->dp;
	return 0 ;
}
/* end subroutine (fill_packet) */


static int
send_packet(lasp, pp, stor, trans)
LAS * lasp;
struct las_packet * pp;
las_storage stor;
int trans;
{


	pp->valid = 1;
	pp->tt = lasp->c.tt;
	pp->pathid = lasp->c.path;
	pp->trans = trans;
	pp->dp = lasp->c.dp;
	switch(stor) {

	case SRC1:
		pp->a = lasp->c.src1.a;
		pp->d = lasp->c.src1.val;
		break;
	case SRC2:
		pp->a = lasp->c.src2.a;
		pp->d = lasp->c.src2.val;
		break;
	case SRC3:
		pp->a = lasp->c.src3.a;
		pp->d = lasp->c.src3.val;
		break;
	case SRC4:
		pp->a = lasp->c.src4.a;
		pp->d = lasp->c.src4.val;
		break;
	case DST1:
		pp->a = lasp->c.dst1.a;
		pp->d = lasp->c.dst1.val;
		break;
	case DST2:
		pp->a = lasp->c.dst2.a;
		pp->d = lasp->c.dst2.val;
		break;
	case OLDDST1:
		pp->a = lasp->c.dst1.a;
		pp->d = lasp->c.dst1.oldd;
		break;
	case OLDDST2:
		pp->a = lasp->c.dst2.a;
		pp->d = lasp->c.dst2.oldd;
		break;
	case MEM:
		pp->a = lasp->c.mem.a;
		pp->d = lasp->c.mem.val;
		break;
	default:
		assert(0);

	}

	return 0 ;
}
/* end subroutine (send_packet) */


static int
read_packet(pp, wr)
struct las_packet * pp;
LFLOWGROUP * wr;
{


	if (! pp->valid ) 
		return 1;	

	wr->trans = pp->trans;
	wr->addr = pp->a; 
	wr->dv = pp->d ;
	wr->tt = pp->tt ;
	wr->path = pp->pathid ;
	wr->dp = LFLOWGROUP_DPALL ;	/* set data present */
	return 0 ;
}
/* end subroutine (read_packet) */


#if	F_DEBUG || F_FPRINTF

static int mkttbuf(buf,tt)
char	buf[] ;
int	tt ;
{
	int	rs ;


	if (tt < LAS_DISPLAYTT) {

	    buf[0] = '-' ;
	    buf[1] = '\0' ;
	    rs = 2 ;

	} else
	    rs = ctdeci(buf,-1,tt) ;

	return rs ;
}

#endif /* F_DEBUG */


int
las_print(lasp) 
LAS  *lasp;
{

/*printf("==========================\n"); */
printf("AS ID=%i, TT=%d,",lasp->asid,lasp->c.tt);
printf("FA=0x%x,",lasp->c.faddr);
/*printf(" TT=%i",lasp->c.tt); */
 las_print_register("s1",lasp->c.src1);
 las_print_register("s2",lasp->c.src2);
 las_print_register("dt",lasp->c.dst1);
 las_print_register("mem",lasp->c.mem);
 las_print_register("cnst",lasp->c.cnst);
printf(" OP=%i",lasp->c.opexec); 
printf("\n");
las_print_pred_regs(&lasp->c.pregs);
/*printf("Number of input predicates=%i\n",lasp->npred); */
/*printf("Physical column index=%i\n",lasp->pci); */
/*printf("Physical SG index=%i\n",lasp->psgi); */
/*printf("our SG row=%i\n",lasp->sgrow); */
/*printf("our AS row=%i\n",lasp->asrow);  */
/*printf(" total AS per column=%i\n",lasp->naspcol);  */
/*printf(" log base 2 of total AS rows=%i\n",lasp->logrows);  */

/*printf(" path ID=%i\n",lasp->c.path);  */
/*printf(" pseudo opcode class of instruct=%i\n",lasp->c.opclass); */
/*printf(" interleaving bits for MFB=%i\n",lasp->mfwb_intleave);  */
/*printf(" interleaving bits for MBB=%i\n",lasp->mbwb_intleave);  */

/*printf(" rfbi=%i\n",lasp->rfbi); */
/*printf(" rbbi=%i\n",lasp->rbbi); */
/*printf(" pfbi=%i\n",lasp->pfbi); */
/*printf(" buspriority=%i\n",lasp->buspriority); */


}


int
las_print_register(name,reg)
const char * name;
struct las_reg  reg;
{

	if(reg.a == -1)
		return;
	if(*name == 's' || *name =='d')
		printf(" ,%s(%d)=0x%x",name,reg.a,reg.val);
	else
		printf(" ,%s(0x%x)=0x%x",name,reg.a,reg.val);
}



int
las_print_pred_regs(struct pred_regs *prp) {

printf("pin(%d)=%d, cpin(%d)=%d, ",
	prp->pin.a, prp->pin.val, prp->cpin[0].a, prp->cpin[0].val);
printf("pout(%d)=%d, cpout(%d)=%d\n",
	prp->pout.a, prp->pout.val, prp->cpout.a, prp->cpout.val);

}

int
las_print_asid(lasp)
LAS *lasp;
{
	ULONG clocks;
	lsim_getclock(lasp->mip,&clocks) ;
	printf("C%lld,ASID%d,TT%d,FA0x%x=>",
		clocks,lasp->asid,lasp->c.tt,lasp->c.faddr);
}

int
las_print_et(lasp, i, j, k) 
LAS * lasp;
int i,j,k;
{
	struct las_packet * pp;
	
	/*for(i=0; i< sizeof(las_command); ++i)
		for(j=0; j< sizeof(las_command); ++j)
			for(k=0; k< sizeof(las_command); ++k) { */

	pp = &lasp->c.event_table[i][j][k];
	printf("event table valid bit = %2d\n", pp->valid);


}


void 
gdb(void) {
	printf("in gdb\n");
}



