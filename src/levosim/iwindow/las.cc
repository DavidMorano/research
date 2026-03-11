/* las SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* Levo Active Station */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable debugging */
#define	CF_DEBUG	1		/* switchable debugging */
#define	CF_FPRINTF	1		/* print to STDOUT */
#define CF_LASDETAILS	1		/* show LAS Bus interface details */
#define CF_LASDETAILS2	1		/* show LAS Bus interface details */
#define CF_LASDETAILS3	0		/* show LAS Bus interface details */
#define	CF_STDIOFLUSH	1		/* flush stdout ? */
#define	CF_SAFE		1		/* safe mode */
#define	CF_SAFE2	0		/* more safe */
#define	CF_SAFEFUNC	0		/* extra-extra safe ? */
#define	CF_RBBHACK	1		/* Dave's REG backwarding bus hack */
#define	CF_RFBHACK	1		/* Dave's RFB snoop hack */
#define	CF_CHECKSUM	0		/* please turn this **ON** ! */
#define	CF_SANITYSUBOBJ	0		/* sanity check */
#define	CF_SANITYOURSELF 1		/* sanity check */
#define	CF_SANITYSTATE	1		/* sanity check */
#define	CF_BADBADHACK	1		/* please get rid of this !! */
#define	CF_EXTRASNOOP	1		/* use the extra snoop rule ? */
#define PERCF_FETCH	1		/* perfect fetching */
#define	CF_NODST3	1		/* no DST3 yet */
#define	CF_XML		1		/* supprt for XML state output */

/* revision history:

	= 2000-02-04, David Morano
	Module was originally written for the LEVO simulator LEVOSIM.

	= 2000-06-15, Alireza Khalafi
	I took over this code for Levo IV.

	= 2000-07-17, David Morano
	I integrated the code into an initial runnable build package.
	I also added code for handling most all the buses that come
	to as AS.

	= 2000-10-03, David Morano
	+ I changed the way some buses come into the LAS object.
	This was necessary because many of the buses coming into
	the i-window are different than before due to the introduction
	of the Levo Memory (Write) Queue object into the machine.
	+ There was also a bug with the check for failure from some
	'malloc()' calls but as it turned out it was not causing a
	problem on the current OS !  I fixed the 'malloc()' problems
	and also fixed up some broken cleanup code in the case when
	something in initialization failed (again this was not
	generally a real problem at all).

	= 2001-01-10, David Morano
	I updated the code for doing machine "shifts" !

	= 2001-01-16, David Morano
	I updated the code by adding the |las_commitinfo()| subroutine.
	This was needed to privide additional information back to
	the execution window control logic for control flow changes
	that occur that go 'out of the window'.

	= 2001-03-13, David Morano
	I just made a very small change to snoop one less bus for
	the group of Register Forwarding Buses that have been given
	to us.  This prevents the same register update from reaching
	us through two different paths.  This is now a bad thing
	so we want to avoid it!

	= 2001-03-30, David Morano
	I instrumented this code in order to find the bad LPE bug
	that was causing our recent crashes.

	= 2001-05-14, David Morano
	I fixed a stray write bug that had to do with the number
	of cancelling predicates that we were getting from a load.
	The code was overwriting the end of the allocated array
	to hold the predicates.

	- 2001-05-15, David Morano
	I added some code to prevent transactions from looping
	around the machine which can happen without active prevention
	when bus spans of more than '1' are configured.

	- 2001-05-17, David Morano
	I added an extra snooping rule to prevent duplicate
	transactions from being snarfed and processed.  These can
	occur when the machine is confired with a register forwarding
	bus span greater than one.

	- 2001-05-31, David Morano
	I tried to fix up the 'las_commitinfo()' subroutine to
	provide the correct indication of whether the instruction
	was enabled or not.

	= 2001-09-26, David Morano
	I added some code to support the XML state trace.

	= 2002-01-17, David Morano
	I added:
	+ path
	+ tt
	+ opclass
	+ opexec

	elements to the XML state output

	I also initialized two previous uninitialized variables
	when as AS gets loaded from the LLB.  These were :

	+ lasp->n.mem.a = (~ 0) ;
	+ lasp->n.old_mem.a = (~ 0) ;

	I do not know if it fixes anything for the better.  Probably
	not but at least they are now in a known state.

*/

/* Copyright © 2000,2001 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/**************************************************************************

  	Name:
	las

	Description:
	This module provides the functions for a Levo Active Station.

	Operational note:
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
	is that we snoop:

	+ ourselves (of course)
	+ (N - 1) buses that are previous to us

	This is a total of N backwarding buses being snooped for a span
	of N.  As it turns out, in spite of the weirdo bus connectivity
	arrangement of backwarding buses, it is almost similar to the
	old (before we got smarter) arrangement.  David Morano actually
	worked this out to this extent to make our life easier!

	I do not know if it fixes anything for the better.  Probably
	not but at least they are now in a known state.

	When an AS first gets loaded with a MIPS load type instructions,
	it tries to make a request on the memory-backwarding-bus with
	a garbage memory address.  The garbage memory address will now
	be fixed at that shown above.  Before it just flapped all over
	the place.  A fix should be made to not make any memory requests
	until after the AS gets its first register source operands
	(enough of them to make a feasible memory address) and then
	make its first memory request with that calculated address.
	The address might still be wrong (welcome to Levo) but at least
	it is better than pure random (which is what was happenning
	previously)!

*/

/* Copyright © 2000,2001,2002 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/*******************************************************************************

  	Name:
	las

	Description:
	This module provides the functions for a Levo Active Station.

	Operational note:

	The register forwarding bus and predicate forwarding bus are
	both wired alike to us and we snoop them proceeding backwards
	from where we are in the bus array.  For a forwarding span
	of N, we snoop:

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
	old (before we got smarter) arrangement.  David Morano actually
	worked this out to this extent to make our life easier !

*******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<assert.h>
#include	<cstddef>
#include	<cstdlib>
#include	<cstdio>
#include	<cstring>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<findbit.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"mipsdis.h"
#include	"xmlinfo.h"

#include	"opclass.h"		/* instruction classes */
#include	"lexecop.h"		/* instruction operations */
#include	"lexec.h"
#include	"ldecodeinstrclass.h"
#include	"instrclass.h"
#include	"levoinfo.h"
#include	"bus.h"
#include	"lbusint.h"
#include	"lpe.h" 
#include	"las.h"
#include	"lwq.h"
#include	"alirgf.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |lenstr(3u)| */

/* local defines */

#define	LAS_MAGIC	0x09089887
#define	LAS_DISPLAYTT	-9999999

#ifndef	RFTYPE_RELAY
#define	RFTYPE_RELAY	0
#define	RFTYPE_NULLIFY	1
#endif

#define	INSTRDISLEN	100


/* external subroutines */

extenr "C" {
    extern int	getinterleave(uint,uint) noex ;
    extern int	getnumbuses(uint) noex ;
    extern int	seqok(uint,uint,uint) noex ;
}


/* forward references */

local int      busintgrp_shift(struct busintgrp *,
			struct proginfo *, struct levoinfo *) ;
local int	busintgrp_init(struct busintgrp *,
			struct proginfo *, struct levoinfo *, 
			BUS *,int,int,int,int,int,int) ;
local int	busintgrp_free(struct busintgrp *) ;
local int      busintgrp_sanitycheck(struct busintgrp *,
			struct proginfo *,struct levoinfo *) ;

local int	RFWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
local int	RBWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
local int	PFWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
local int	MFWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
local int	MBWB_interface(LAS *,struct proginfo *,struct levoinfo *) ;
local int	PE_interface(LAS *,struct proginfo *,struct levoinfo *) ;
local int	snoop_RFWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
local int	snoop_RBWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
local int	snoop_PFWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
local int	snoop_MFWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
local int	snoop_MBWB_interface(LAS *,
			struct proginfo *,struct levoinfo *) ;
local int	AS_logic(LAS *,struct proginfo *,struct levoinfo *) ;
local int	las_predicate_logic(LAS *);
local int	las_handleshift(LAS *,struct proginfo *,struct levoinfo *) ;
local int	las_readrfbuses(LAS *,struct proginfo *,struct levoinfo *,
			LBUSINT *,int) ;
local int	las_doshift(LAS *,struct proginfo *,struct levoinfo *,
			int,int,int *) ;
local int	PE_checkresult(LAS *,struct proginfo *) ;
local int	las_checkdst3(LAS *,struct proginfo *) ;
local int	las_loadalireg(LAS *,struct las_reg *) ;
local int	send_packet(LAS *,struct las_packet *,las_storage,uint,int) ;
local int	las_print_et(LAS *,int,int,int) ;
local int	las_print_asid(LAS *) ;
local int	las_print(LAS *) ;
local int	las_xmloutreg(LAS *,XMLINFO *,struct proginfo *,
			struct las_reg *,char *) ;

local int	fill_packet(struct las_packet *,LFLOWGROUP *) ;
local int	read_packet(struct las_packet *,LFLOWGROUP *) ;
local int	las_print_pred_regs(struct pred_regs *) ;
local int	las_print_register(const char *, struct las_reg) ;

#if	CF_FPRINTF || (CF_MASTERDEBUG && CF_DEBUG) || CF_XML
local int	mkttbuf(char *,int) ;
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


/* exported variables */


/* exported subroutines */

int las_init(lasp,pip,mip,lip,ap)
LAS		*lasp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
LAS_INITARGS	*ap ;
{
	BUS	*busp ;
	int	rs, i, j, k ;
	int	n, size ;
	int	npred ;
	int	maxspan ;

	if ((lasp == nullptr) || (ap == nullptr)) return SR_FAULT ;

	memclear(lasp) ;
	lasp->magic = 0 ;
	lasp->f = {} ;
	lasp->pip = pip ;
	lasp->mip = mip ;
	lasp->lip = lip ;
	lasp->asid = ap->asid ;		/* our ID */

#if	CF_MASTERDEBUG && CF_DEBUG
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

	lasp->c = {} ;
	lasp->n = {} ;
	lasp->c.mem.latesttt = lasp->n.mem.latesttt = INT_MIN ;
	lasp->c.asstate = lasp->n.asstate = UNUSED ;

	lasp->c.pe_tnumb = lasp->n.pe_tnumb = -1 ;

/* some miscellaneous calculations for later */

	lasp->rfmodmask = (lip->rfmod - 1) ;
	lasp->rbmodmask = (lip->rbmod - 1) ;

/* what about predicate register sets that we need */

	lasp->npred = npred = lip->nprpas ;

/* allocate the predicate register resources */

	size = 2 * npred * szof(struct las_reg) ;
	rs = uc_malloc(size,&lasp->c.pregs.cpin) ;

	if (rs < 0)
		goto bad0 ;

	lasp->n.pregs.cpin = lasp->c.pregs.cpin + npred ;

#if	CF_MASTERDEBUG && CF_DEBUG
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

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
		eprintf("las_init: RFB(0)=%08lx span=%d\n",ap->rfbuses,
			lip->nrfspan) ;
	}
#endif

#if	CF_RFBHACK
	n = MAX(lip->nrfspan,1) ;	/* new way */
#else
	n = lip->nrfspan + 1 ;		/* old way */
#endif

	rs = busintgrp_init(&lasp->rfwbus,pip,lip,
	    ap->rfbuses,false,ap->rfbi,lip->nsg,n,ap->busid,RFWB) ;

	if (rs < 0)
	    goto bad1 ;

	if (n > maxspan)
		maxspan = n ;

/* register backwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
		eprintf("las_init: RBB(0)=%08lx span=%d\n",ap->rbbuses,
			lip->nrbspan) ;
#endif

#if	CF_RBBHACK
	n = MAX(lip->nrbspan,1) ;	/* new way */
#else
	n = lip->nrbspan + 1 ;		/* old way */
#endif

	rs = busintgrp_init(&lasp->rbwbus,pip,lip,
	    ap->rbbuses,true,ap->rbbi,lip->nsg,n,ap->busid,RBWB) ;

	if (rs < 0)
	    goto bad2 ;

	if (n > maxspan)
		maxspan = n ;

/* predicate forwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
		eprintf("las_init: PFB(0)=%08lx span=%d\n",ap->pfbuses,
			lip->npfspan) ;
#endif

	n = lip->npfspan + 1 ;
	rs = busintgrp_init(&lasp->pfwbus,pip,lip,
	    ap->pfbuses,false,ap->pfbi,lip->nsg,n,ap->busid,PFWB) ;

	if (rs < 0)
	    goto bad3 ;

	if (n > maxspan)
		maxspan = n ;

/* memory forwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_init: MFB(0)=%08lx\n",ap->mfbuses) ;
#endif

	n = getnumbuses(ap->mfinter) ;

	busp = ap->mfbuses ;
	rs = busintgrp_init(&lasp->mfwbus,pip,lip,busp,true,0,n,n,
		ap->busid,MFWB) ;

	if (rs < 0)
	    goto bad4 ;

/* memory backwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_init: MBB(0)=%08lx\n",ap->mbbuses) ;
#endif

	n = getnumbuses(ap->mbinter) ;

	busp = ap->mbbuses ;
	rs = busintgrp_init(&lasp->mbwbus,pip,lip,busp,true,0,n,n,
		ap->busid,MBWB) ;

	if (rs < 0)
		goto bad4 ;


/* allocate some temporary bus read register buffer */

	size = maxspan * szof(struct las_busreg) ;
	rs = uc_malloc(size,&lasp->busregs) ;

	if (rs < 0)
		goto bad5 ;

	memclear(lasp->busregs,size) ;

/* OK, clear out the event table for starters */

	for (i = 0 ; i < NCOMMANDS ; i += 1) {
	    for (j = 0 ; j < NBUSES ; j += 1) {
		for (k = 0 ; k < NSTORAGES ; k += 1) {
		    lasp->c.event_table[i][j][k].valid = false ;
		}
	    }
	} /* end for */

/* checksum */

#if	CF_MASTERDEBUG && CF_SAFE
	d_checkcalc(&lasp->c,szof(struct las_state),&lasp->c.checksum) ;
#endif


#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_init: exiting rs=%d\n",rs) ;
#endif

	lasp->magic = LAS_MAGIC ;
	return rs ;

/* bad things */
bad6:

	if (lasp->busregs) {
		free(lasp->busregs) ;
	}

bad5:
	busintgrp_free(&lasp->mfwbus) ;

bad4:
	busintgrp_free(&lasp->pfwbus) ;

bad3:
	busintgrp_free(&lasp->rbwbus) ;

bad2:
	busintgrp_free(&lasp->rfwbus) ;

bad1:
	if (lasp->c.pregs.cpin) {
		free(lasp->c.pregs.cpin) ;
	}

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

	if (lasp == nullptr) return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_free: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3) {
	eprintf("las_free: ASID=%d LAS=%08lx\n",
		lasp->asid,lasp) ;
	eprintf("las_free: bus-int-groups\n") ;
	}
#endif

	busintgrp_free(&lasp->mbwbus) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: 2 bus-int-groups\n") ;
#endif

	busintgrp_free(&lasp->mfwbus) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: RF bus-int-groups\n") ;
#endif

	busintgrp_free(&lasp->rfwbus) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: RB bus-int-groups\n") ;
#endif

	rs = busintgrp_free(&lasp->rbwbus) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3) {
	eprintf("las_free: RB rs=%d\n",rs) ;
	eprintf("las_free: PF bus-int-groups\n") ;
	}
#endif

	busintgrp_free(&lasp->pfwbus) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_free: cancelling predicates\n") ;
#endif

	if (lasp->c.pregs.cpin) {
		free(lasp->c.pregs.cpin) ;
	}

#if	CF_MASTERDEBUG && CF_DEBUG
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
	LSIM			*mip ;
	struct proginfo		*pip ;
	struct levoinfo		*lip ;
	struct busintgrp	*bigp ;
	ulong	clock ;
	int	rs = SR_OK ;
	int	rs1 ;
	int	i ;

	if (lasp == nullptr) return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_comb: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	mip = lasp->mip ;
	lip = lasp->lip ;		/* need for some number computation */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3)
	eprintf("las_comb: entered ck=%lld ph=%d TT=%d\n",
		mip->clock,phase,lasp->c.tt) ;
#endif

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 0 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 0 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

/* 'lbusint's for RFBes for a span */

	bigp = &lasp->rfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d RFB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 1 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

/* 'lbusint's for RBBes for a span */

	bigp = &lasp->rbwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d RBB-%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 2 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

/* 'lbusint's for predicate buses for a span */

	bigp = &lasp->pfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d PFB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			break ;
		}

	} /* end for */

	if (rs < 0)
		return rs ;

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 3 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

/* 'lbusint's for memory buses */

	bigp = &lasp->mfwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d MFB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 3b bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

	bigp = &lasp->mbwbus ;
	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_comb(bigp->head + i,phase) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: i=%d MBB=%08lx bad rs=%d\n",
		i,(bigp->head + i),rs) ;
#endif

			return rs ;
		}

	}

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 5 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 5 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */


/* one more time ! */

		rs = las_checkdst3(lasp,pip) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 1 checkdst3() rs=%d\n",rs) ;
#endif

		return rs ;
	}


/* do our own combinatorial work */

	switch (phase) {

	case 0:

#if	CF_FPRINTF
	clock = 0 ;
	lsim_getclock(lasp->mip,&clock) ;

	if (lasp->asid == 0) {

	printf("CLOCK ===== %lld\n",clock) ;

#if	CF_STDIOFLUSH
	fflush(stdout) ;
#endif

	}
#endif /* CF_FPRINTF */

/* do something for real */

		rs = las_checkdst3(lasp,pip) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 2 checkdst3() rs=%d\n",rs) ;
#endif

		}

		break ;

/* execute all our combinational parts */
	case 2:
	    rs = AS_logic(lasp,pip,lip) ;

	if (rs < 0)
		break ;

		rs = las_checkdst3(lasp,pip) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 3 checkdst3() rs=%d\n",rs) ;
#endif

		}

	    break ;

	case 3:
	    rs = las_handleshift(lasp,pip,lip) ;

	if (rs < 0)
		break ;

		rs = las_checkdst3(lasp,pip) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 4 checkdst3() rs=%d\n",rs) ;
#endif

		}

	    break ;

	case 5:
		break ;

	} /* end switch */

	if (rs < 0)
		return rs ;

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 6 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;
	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_comb: 6 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */


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
	int	rs ;
	int	ret;
	int	i,j,k,ll ;

#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr) return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_clock: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;


#if	CF_MASTERDEBUG && (CF_SAFE && CF_CHECKSUM)
	rs = d_checkverify(&lasp->c,szof(struct las_state),
	    &lasp->c.checksum) ;

	if (rs < 0) {

	    eprintf("las_clock: LAS=%08lx checksum bad\n",
	        lasp) ;

	    return SR_BADFMT ;
	}
#endif /* CF_SAFE */


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

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf(
		"las_clock: npred=%d c_pregs_cpin=%08lx n_pregs_cpin=%08lx\n",
		lasp->npred,lasp->c.pregs.cpin,lasp->n.pregs.cpin) ;
#endif

	for (ll = 0 ; ll < lasp->npred ; ll += 1)
		lasp->c.pregs.cpin[ll] = lasp->n.pregs.cpin[ll] ;

#endif /* COMMENT_BADBUG */


#if	CF_MASTERDEBUG && CF_SAFE
	d_checkcalc(&lasp->c,szof(struct las_state),&lasp->c.checksum) ;
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
	int	rs = SR_OK ;
	int	i ;

#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr) return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_shift: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;


/* "do" all of our subobjects */


/* register forwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_shift: RFB\n") ;
	}
#endif

	rs = busintgrp_shift(&lasp->rfwbus,pip,lip) ;

	if (rs < 0)
	    goto bad1 ;

/* register backwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: RBB\n") ;
#endif

	rs = busintgrp_shift(&lasp->rbwbus,pip,lip) ;

	if (rs < 0)
	    goto bad2 ;

/* predicate forwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: PFB\n") ;
#endif

	rs = busintgrp_shift(&lasp->pfwbus,pip,lip) ;

	if (rs < 0)
	    goto bad3 ;


/* memory forwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: MFB\n") ;
#endif

	rs = busintgrp_shift(&lasp->mfwbus,pip,lip) ;

	if (rs < 0)
	    goto bad4 ;

/* memory backwarding */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_shift: MBB\n") ;
#endif

	rs = busintgrp_shift(&lasp->mbwbus,pip,lip) ;

	if (rs < 0)
		goto bad5 ;


/* finally "do" us */

	lasp->f.shift = true ;

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

#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr) return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {
		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;
		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (iap == nullptr)
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


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr)
		return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

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


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr)
		return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

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


	if (lasp == nullptr)
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

	int	rs = SR_OK, rs1, i ;
	int	ret;
	int 	n ;
	int	*cpins;
	int	*cpinsv;
	int	f_constvalid ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr)
		return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_load: LAS=%08lx index=%d path=%d\n",
		lasp,index,path) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 4)
	    eprintf(
		"las_load: npred=%d c_pregs_cpin=%08lx n_pregs_cpin=%08lx\n",
		lasp->npred,lasp->c.pregs.cpin,lasp->n.pregs.cpin) ;
#endif

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 0 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

/*
	if (lasp->c.asstate != UNUSED)
		return 0;
*/

/* David Morano, 01/05/14 -- this is not a good programming practice ! */

	lasp->n = {} ;
	lasp->n.pregs.cpin = lasp->c.pregs.cpin + lasp->npred ;

/* end of bad programming practice ! */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 1 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

/* load some easy stuff first */

	(void) llb_instrinfo(llbp,index,&ii) ;

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 1a bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

	lasp->n.ia = ii.ia ;
	lasp->n.instr = ii.instr ;
	lasp->n.btype = ii.btype ;	/* Marcos-style i-fetch branch type */
	lasp->n.oopexec = ii.opexec ;
	lasp->n.oopclass = ii.opclass ;

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 2 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

/* load the hard stuff */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_load: c_pregs_cpin=%08lx n_pregs_cpin=%08lx\n",
		lasp->c.pregs.cpin,lasp->n.pregs.cpin) ;
#endif


#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 3 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

	lasp->n.pe_tnumb = -1 ;

/* 02/01/17 DAM, can't leave it hanging in the wind ! */

	lasp->n.mem.a = (~ 0) ;
	lasp->n.old_mem.a = (~ 0) ;

/* end of change */

	lasp->n.mem.latesttt = INT_MIN ;

	ret = llb_read(llbp, &lasp->n.opexec, &lasp->n.opclass, 
			&lasp->n.faddr, &lasp->n.src1.a, &lasp->n.src2.a, 
			&lasp->n.src3.a, &lasp->n.src4.a, &lasp->n.src5.a,
			&lasp->n.dst1.a, &lasp->n.dst2.a, &lasp->n.dst3.a,
			&lasp->n.cnst.val, 
			&f_constvalid, 
			&lasp->n.pregs.pin.a, 
			&lasp->n.pregs.pin.val, &cpins, &cpinsv, 
			&lasp->n.pregs.num_of_cpin_regs, 
			&lasp->n.pregs.pout.a, &lasp->n.pregs.cpout.a, index);

	lasp->n.cnst.valid = f_constvalid ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 4) {
		eprintf("las_load: opexec=%08x oopexec=%08x\n",
			lasp->n.opexec,lasp->n.oopexec) ;
	}
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 4) {
		if (lasp->n.dst3.a != -1)
		eprintf("las_load: DST3 required for execution\n") ;
	}
#endif

#if	CF_NODST3
	lasp->n.dst3 = {} ;
	lasp->n.dst3.a = -1 ;
#endif

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 4 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

	if (lasp->n.src1.a == 0) lasp->n.src1.val = 0;
	if (lasp->n.src2.a == 0) lasp->n.src2.val = 0;
	if (lasp->n.src3.a == 0) lasp->n.src3.val = 0;
	if (lasp->n.src4.a == 0) lasp->n.src4.val = 0;
	if (lasp->n.src5.a == 0) lasp->n.src5.val = 0;
	if (lasp->n.dst1.a == 0) lasp->n.dst1.val = 0;
	if (lasp->n.dst2.a == 0) lasp->n.dst2.val = 0;
	if (lasp->n.dst3.a == 0) lasp->n.dst3.val = 0;
	if (ret == SR_OK) {
		lasp->n.pI = 1;
		lasp->n.f_regfileread = false ;	/* schedule a read */
		lasp->n.asstate = WAIT_NRC;
	} else {
		return 0 ;
	}

	if (lasp->n.opclass == INSTR_UNUSED) {
		lasp->n.asstate = UNUSED;
	}

/* sanity check */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 5 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

/* what about the input predicates ? */

	n = lasp->n.pregs.num_of_cpin_regs ;

#if	CF_MASTERDEBUG && CF_DEBUG
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

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 6 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

		for (i=0 ; i<lasp->n.pregs.num_of_cpin_regs ; i += 1) {
			lasp->n.pregs.cpin[i].a = cpins[i];
			lasp->n.pregs.cpin[i].val = cpinsv[i];
			lasp->n.pregs.cpin[i].latesttt = INT_MIN;
		} /* end for */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 7 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif

		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

	} else {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_load: we do not have any input predicates n=0\n") ;
#endif

		lasp->n.pregs.cpin[0].a = -1 ;

	}

/* sanity check */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 9 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

/* assign the TT */

	lasp->n.tt = (lip->nsg * lip->nasrpsg) + lasp->asrow ;

/* more stuff */

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
	lasp->n.src5.latesttt = INT_MIN;

	lasp->n.dst1.latesttt = INT_MIN;
	lasp->n.dst2.latesttt = INT_MIN;
	lasp->n.dst3.latesttt = INT_MIN;

	lasp->n.mem.latesttt = INT_MIN;

/* very bad thing to do, messed up the pipelining of loads ! */

#if	CF_BADBADHACK
	lasp -> c.pregs.pout.val = -2 ;
#endif

/* OK */

	lasp -> n.pregs.pout.val = -2 ;

/* very bad thing to do, messed up the pipelining of loads ! */

#if	CF_BADBADHACK
	lasp -> c.pregs.cpout.val = -2 ; 
#endif

/* OK */

	lasp -> n.pregs.cpout.val = -2 ; 

/* sanity check */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_load: 12 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */


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


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr)
		return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

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

#ifdef	COMMENT
	if (lasp->c.re_exec_inst && (lasp->c.pe_tnumb >= 0))
		commit = 0 ;
#else
	if (lasp->c.re_exec_inst)
		commit = 0 ;
#endif

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for instruction execution tn=%d\n",
		lasp->c.pe_tnumb) ;
#endif

	return 0;
	}

#ifdef	COMMENT
	if (lasp->wait4src1 && lasp->wait4src2 )
		commit = 0;
#endif /* COMMENT */

	if (lasp->c.fw_pred_on_pfwb) 
		commit = 0;

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for special PFB\n") ;
#endif

	return 0;
	}

/* check the event table */
	
	for (i = 0 ; i < NCOMMANDS ; i += 1) {

	    for (j = 0 ; j < NBUSES ; j += 1) {

		for (k = 0 ; k < NSTORAGES ; k += 1) {

		    if (lasp->c.event_table[i][j][k].valid) {

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

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for event table %d:%d:%d\n",
		i,j,k) ;
#endif

		return 0 ;
	}

/* check if waiting on results from PE */

	if (lasp->c.pe_tnumb != -1)
		commit = 0 ;

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for PE result\n") ;
#endif

		return 0 ;
	}

/* check if any transaction is still on the output buses */

/* check *only* out output Register Forward bus */

	lbp = lasp->rfwbus.outbus ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: RFB LBUSINT=%08lx\n",lbp) ;
#endif

	    rs = lbusint_done(lbp) ;

	    if (rs < 0)
		goto badbusint ;

	    if (rs < 1)
		commit = 0;

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for RFB LBUSINT=%08lx\n",lbp) ;
#endif

		return 0;
	}

/* check the output Register Backward bus */

	lbp = lasp->rbwbus.outbus ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: RBB LBUSINT=%08lx\n",lbp) ;
#endif

	    rs = lbusint_done(lbp) ;

	    if (rs < 0)
		goto badbusint ;

	    if (rs < 1)
		commit = 0;

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for RBB LBUSINT=%08lx\n",lbp) ;
#endif

		return 0;
	}

/* check the output Predicate Forward bus */

	lbp = lasp->pfwbus.outbus ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: PFB LBUSINT=%08lx\n",lbp) ;
#endif

	    rs = lbusint_done(lbp) ;

	    if (rs < 0)
		goto badbusint ;

	    if (rs < 1)
		commit = 0;

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for PFB LBUSINT=%08lx\n",lbp) ;
#endif

		return 0;
	}

/* do all of the memory buses (since we may write to them all) */

	n = lip->nmembuses ;
	for (mi = 0 ; commit && (mi < n) ; mi += 1) {

/* Memory Forwarding bus */

		lbp = lasp->mfwbus.outbus + mi ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: MFB LBUSINT=%08lx mi=%d\n",
		lbp,mi) ;
#endif

		rs = lbusint_done(lbp) ;

		if (rs < 0)
			goto badbusint ;

		if (rs < 1)
			commit = 0;

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for MFB LBUSINT=%08lx mi=%d\n",
		lbp,mi) ;
#endif

		return 0;
	}

/* Memory Backwarding bus */

		lbp = lasp->mbwbus.outbus + mi ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: MBB LBUSINT=%08lx mi=%d\n",
		lbp,mi) ;
#endif
		rs = lbusint_done(lbp) ;

	if (rs < 0)
		goto badbusint ;

	if (rs < 1)
		commit = 0;

	if (! commit) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: waiting for MBB LBUSINT=%08lx mi=%d\n",
		lbp,mi) ;
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

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 4)
	eprintf("las_readycommit: bad LBUSINT=%08lx rs=%d\n",
		lbp,rs) ;
#endif

ret:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las_readycommit: id=%d exiting astt=%d rs=%d\n",
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
	int	f_enable = false ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr)
		return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (ip == nullptr) return SR_FAULT ;

	pip = lasp->pip ;
	memclear(ip) ;

/* get the instruction class used by David in the execution window */

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
/* domain predicate? */

	ip->f_enabled = !!(lasp->c.pregs.pin.val) ;

/* branch target predicates */

	if (! ip->f_enabled) {

	    for (i = 0 ; i < lasp->c.pregs.num_of_cpin_regs ; i += 1)
		ip->f_enabled |= ((lasp->c.pregs.cpin[i].val) ? 1 : 0) ;

	} /* end if (determining if we were enabled) */

#if	PERCF_FETCH
		ip->f_enabled = lasp->c.pI ;
#endif

/* do we have a control flow change type of instruction ? */

	ip->f_cf = false ;
	if ((lasp->c.opclass == INSTR_BREL) || 
		(lasp->c.opclass == INSTR_JREL) ||
	   	(lasp->c.opclass == INSTR_JIND)) {

		rs = 1 ;
		ip->f_cf = true ;
		ip->f_taken = lasp->c.bcond ; 
		ip->ta = (ip->f_taken) ? lasp->c.cnst.val : (ip->ia + 8) ; 

	} /* end if (control-flow-type instruction) */

/****

	Non-control-flow instructions are supposed to be able to look
	at the 'lasp->c.pI' value also (and alone) for its current
	execution predicate.  Let's test this hypothesis !

****/

#if	CF_MASTERDEBUG && CF_SAFE && 0
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
#endif /* CF_DEBUG */

	return rs ;
}
/* end subroutine (las_commitinfo) */


/* we are being told to commit our instruction */
int las_commit(lasp,ipval,bdir,btarget,iap)
LAS	*lasp;
int	*ipval;	/* input predicate value */
int	*bdir;
uint	*btarget;
uint	*iap ;
{
	int	rs = SR_OK ;

	int cpin=0;
	int i;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr)
		return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_getia: LAS=%08lx bad magic\n",lasp) ;

		return SR_BADFMT ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

/* entered */

#if	CF_FPRINTF
	las_print(lasp);
#endif

	*ipval = 0;
	*bdir = -1;

	lasp->n.asstate = UNUSED;

	if (lasp->c.asstate == UNUSED) {
		return SR_EMPTY;
	}

	if (lasp->c.opclass == INSTR_BREL || lasp->c.opclass == INSTR_JREL ||
	   lasp->c.opclass == INSTR_JIND ) {

		rs = 1 ;
		*bdir = lasp->c.bcond; 
		*btarget = lasp->c.cnst.val; 
		for (i=0; i<lasp->c.pregs.num_of_cpin_regs; i += 1)
			cpin |= lasp->c.pregs.cpin[i].val;

		*ipval = cpin | lasp -> c.pregs.pin.val ;

	} else {
		*ipval = lasp->c.pI ;
	}
		
#if	PERCF_FETCH
		*ipval = lasp->c.pI;
#endif


	*iap = lasp->c.faddr;

	lbtrb_as_commit(lasp->btrb, lasp->c.pregs.pout.a, 
		lasp->c.pregs.pout.val, lasp->c.pregs.cpout.a,
		lasp->c.pregs.cpout.val,lasp->asrow);

	if (lasp->c.pI == 0)
		return rs ;

#ifdef	COMMENT
	alirgf_write(lasp->rgfp,lasp) ; 
#else
	if (lasp->c.dst1.a > 0)
		alirgf_writereg(lasp->rgfp,
			lasp->c.dst1.a, 
			lasp->c.tt,
			lasp->c.dst1.val) ; 

	if (lasp->c.dst2.a > 0)
		alirgf_writereg(lasp->rgfp,
			lasp->c.dst2.a, 
			lasp->c.tt,
			lasp->c.dst2.val) ; 

	if (lasp->c.dst3.a > 0)
		alirgf_writereg(lasp->rgfp,
			lasp->c.dst3.a, 
			lasp->c.tt,
			lasp->c.dst3.val) ; 

#endif /* COMMENT */

	if (lasp->c.opclass == INSTR_STORE)
		lwq_store(lasp->lwqp, (lasp->c.mem.a & 0xfffffffc),
			lasp->c.mem.val,lasp->c.tt, lasp->asrow,lasp->c.dp);

	lasp->n.f_regfileread = false ;
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


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == nullptr)
		return SR_FAULT ;

	if ((lasp->magic != LAS_MAGIC) && (lasp->magic != 0)) {

		eprintf("las_shift: LAS=%08lx bad magic\n",lasp) ;

		rs = SR_BADFMT ;
		goto bad0 ;
	}

	if (lasp->magic != LAS_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;


#if	CF_SANITYSTATE && CF_CHECKSUM

	rs = d_checkverify(&lasp->c,szof(struct las_state),
	    &lasp->c.checksum) ;

	if (rs < 0) {

	    eprintf("las_sanitycheck: LAS=%08lx checksum bad\n",
	        lasp) ;

	    return SR_BADFMT ;
	}

#endif /* CF_SANITYSTATE */


/* check all of our subobjects */

#if	CF_SANITYSUBOBJ

/* register forwarding */

	rs = busintgrp_sanitycheck(&lasp->rfwbus,pip,lip) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: RFB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad1 ;
	}

/* register backwarding */

	rs = busintgrp_sanitycheck(&lasp->rbwbus,pip,lip) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: RBB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad2 ;
	}

/* predicate forwarding */

	rs = busintgrp_sanitycheck(&lasp->pfwbus,pip,lip) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: PFB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad3 ;
	}

/* memory forwarding */

	rs = busintgrp_sanitycheck(&lasp->mfwbus,pip,lip) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: MFB bad rs=%d\n",rs) ;
	}
#endif

	    goto bad4 ;
	}

/* memory backwarding */

	rs = busintgrp_sanitycheck(&lasp->mbwbus,pip,lip) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las_sanitycheck: MBB bad rs=%d\n",rs) ;
	}
#endif

		goto bad5 ;
	}

#endif /* CF_SANITYSUBOBJ */


/* check ourself */

#if	CF_SANITYOURSELF

/* check the opexec value for corruption */

	rs = (lasp->c.opexec == lasp->c.oopexec) ? SR_OK : SR_BADFMT ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf(
		"las_sanitycheck: opexec bad opexec=%08x oopexec=%08x rs=%d\n",
			lasp->c.opexec,lasp->c.oopexec,rs) ;
	}
#endif

		goto bad6 ;
	}

/* check the event table */

	rs = las_checkdst3(lasp,pip);

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf(
		"las_sanitycheck: 5 las_checkdst3() bad rs=%d\n",
			rs) ;
	}
#endif

		goto bad7 ;
	}

#endif /* CF_SANITYOURSELF */



bad7:
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


/* XML stuff */
int las_xmlinit(fp,xip)
LAS	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}


int las_xmlout(lasp,xip)
LAS	*lasp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	int	rs ;

	char	ttbuf[40] ;


	pip = lasp->pip ;

	bprintf(&xip->xmlfile,"<as>\n") ;

	bprintf(&xip->xmlfile,"<uid>%p</uid>\n",lasp) ;

	bprintf(&xip->xmlfile,"<asid>%d</asid>\n",lasp->asid) ;

	bprintf(&xip->xmlfile,"<path>%d</path>\n",lasp->c.path) ;

	        mkttbuf(ttbuf, lasp->c.tt) ;

	bprintf(&xip->xmlfile,"<tt>%s</tt>\n",ttbuf) ;

	bprintf(&xip->xmlfile,"<used>%d</used>\n",
		(lasp->c.opclass != OPCLASS_UNUSED)) ;

		if (lasp->c.opclass != OPCLASS_UNUSED) {

		char	instrdis[INSTRDISLEN + 1] ;


	bprintf(&xip->xmlfile,"<ia>%08x</ia>\n",lasp->c.ia) ;

	bprintf(&xip->xmlfile,"<instr>%08x</instr>\n",lasp->c.instr) ;

	        rs = mipsdis_getlevo(&pip->dis,lasp->c.ia,lasp->c.instr,
				instrdis,INSTRDISLEN) ;

	        if (rs < 0)
	            strcpy(instrdis,"unknown") ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 3)
	        eprintf("las_xmlout: ia=%08x instr=%08x %s\n",
	            lasp->c.ia,lasp->c.instr,instrdis) ;
#endif /* CF_DEBUG */

	bprintf(&xip->xmlfile,"<instrdis>%s</instrdis>\n",
		instrdis) ;

	bprintf(&xip->xmlfile,"<opclass>%d</opclass>\n",lasp->c.opclass) ;

	bprintf(&xip->xmlfile,"<opexec>%d</opexec>\n",lasp->c.opexec) ;

/* what about some register sources ? */

		if (lasp->c.src1.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.src1,"src1") ;

		if (lasp->c.src2.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.src2,"src2") ;

		if (lasp->c.src3.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.src3,"src3") ;

		if (lasp->c.src4.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.src4,"src4") ;

		if (lasp->c.src5.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.src5,"src5") ;

		if (lasp->c.dst1.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.dst1,"dst1") ;

		if (lasp->c.dst2.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.dst2,"dst2") ;

		if (lasp->c.dst3.a != (~ 0))
			las_xmloutreg(lasp,xip,pip,&lasp->c.dst3,"dst3") ;

		} /* end if (we had some instruction) */

	bprintf(&xip->xmlfile,"</as>\n") ;

	return SR_OK ;
}
/* end subroutine (las_xmlout) */


int las_xmloutreg(lasp,xip,pip,rp,name)
LAS		*lasp ;
XMLINFO		*xip ;
struct proginfo	*pip ;
struct las_reg	*rp ;
char		name[] ;
{
	char	ttbuf[40] ;
	char	*typename ;


	typename = (name[0] == 's') ? "srcreg" : "dstreg" ;

	bprintf(&xip->xmlfile,"<%s>\n",typename) ;

	bprintf(&xip->xmlfile,"<name>%s</name>\n",
		name) ;

	bprintf(&xip->xmlfile,"<addr>%08x</addr>\n",
		rp->a) ;

	bprintf(&xip->xmlfile,"<dv>%08x</dv>\n",
		rp->val) ;

	bprintf(&xip->xmlfile,"<path>%d</path>\n",
		rp->pathid) ;

	mkttbuf(ttbuf, rp->tt) ;

	bprintf(&xip->xmlfile,"<tt>%s</tt>\n",ttbuf) ;

	bprintf(&xip->xmlfile,"</%s>\n",typename) ;

	return SR_OK ;
}
/* end subroutine (las_xmloutreg) */


int las_xmlfree(fp,xip)
LAS	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}
/* end subroutine (las_xmlfree) */



/* PRIVATE SUBROUTINES */



/* handle a machine shift operation ! */
local int las_handleshift(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	i, j, k ;
	int	totalrows ;
	int	tt_test ;


	if (! lasp->f.shift)
		return SR_OK ;

	lasp->f.shift = false ;
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
		    	&lasp->n.src5.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst1.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst2.tt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst3.tt) ;

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
		    	&lasp->n.src5.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst1.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst2.latesttt) ;

	las_doshift(lasp,pip,lip,tt_test,totalrows,
		    	&lasp->n.dst3.latesttt) ;

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
local int las_doshift(lasp,pip,lip,tt_test,tr,ap)
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
local int busintgrp_init(bigp,pip,lip,buses,f_fwd,bi,m,n,busid,idtype)
struct busintgrp	*bigp ;
struct proginfo		*pip ;
struct levoinfo		*lip ;
BUS			*buses ;
int			f_fwd ;		/* true for a BACKWARDING bus ! */
int			bi, m, n ;
int			busid ;
int			idtype;
{
	int	rs, i ;
	int	size ;
	int	badi ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	eprintf("las/busintgrp_init: n=%d bi=%d\n",n,bi) ;
	eprintf(
	    "las/busintgrp_init: bi=%d\n",bi) ;
	}
#endif

	bigp->pip = pip ;
	bigp->head = nullptr ;
	bigp->outbus = nullptr ;
	bigp->f_fwd = f_fwd ;
	bigp->bi = bi ;
	bigp->modulus = m ;
	bigp->num = n ;

	size = bigp->num * szof(LBUSINT) ;
	rs = uc_malloc(size,&bigp->head) ;

	if (rs < 0)
	    goto bad0 ;

	memclear(bigp->head,size) ;

/* we start the bus numbering with our primary bus (if that has meaning) */

	bigp->outbus = bigp->head ;
	for (i = 0 ; i < bigp->num ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
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
local int busintgrp_free(bigp)
struct busintgrp	*bigp ;
{
	int	rs = SR_OK ;
	int	rs1 ;

	for (int i = 0 ; i < bigp->num ; i += 1) {
	    rs1 = lbusint_free(bigp->head + i) ;
		if ((rs1 < 0) && (rs >= 0)) {
			rs = rs1 ;
		}
	}

	free(bigp->head) ;

	bigp->head = nullptr ;
	bigp->outbus = nullptr ;
	return rs ;
}
/* end subroutine (busintgrp_free) */

/* shift a bus interface group */
local int busintgrp_shift(bigp,pip,lip)
struct busintgrp	*bigp ;
struct proginfo		*pip ;
struct levoinfo		*lip ;
{
	int	rs = SR_OK ;

	for (int i = 0 ; i < bigp->num ; i += 1) {
	    rs = lbusint_shift(bigp->head + i) ;
		if (rs < 0) break ;
	} /* end for */

	return rs ;
}
/* end subroutine (busintgrp_shift) */


/* sanitycheck the bus-int-groups */
local int busintgrp_sanitycheck(bigp,pip,lip)
struct busintgrp	*bigp ;
struct proginfo		*pip ;
struct levoinfo		*lip ;
{
	int	rs = SR_OK, i ;


/* check the LBUSINTs for corruption */

	for (i = 0 ; i < bigp->num ; i += 1) {

	    rs = lbusint_check(bigp->head + i) ;

		if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
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


/* do the main part of the AS combinatorial logic */
local int AS_logic(lasp,pip,lip)
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

/* jump-start (read) the registers as desired */

	if (! lasp->c.f_regfileread) {

#ifdef	COMMENT
		if (lasp->asid == 0) {
			alirgf_print(lasp->rgfp);
			lwq_print(lasp->lwqp);
			lwq_print_wq(lasp->lwqp);
		}
#endif /* COMMENT */

	las_loadalireg(lasp,&lasp->n.src1) ;

	las_loadalireg(lasp,&lasp->n.src2) ;

	las_loadalireg(lasp,&lasp->n.src3) ;

	las_loadalireg(lasp,&lasp->n.src4) ;

	las_loadalireg(lasp,&lasp->n.src5) ;

	las_loadalireg(lasp,&lasp->n.dst1) ;

	las_loadalireg(lasp,&lasp->n.dst2) ;

	las_loadalireg(lasp,&lasp->n.dst3) ;


		if (lasp->c.src1.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC1], 
			SRC1,LFLOWGROUP_TSTORE,false) ;

		if (lasp->c.src2.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC2], 
			SRC2,LFLOWGROUP_TSTORE,false) ;

		if (lasp->c.src3.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC3], 
			SRC3,LFLOWGROUP_TSTORE,false) ;

		if (lasp->c.src4.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC4], 
			SRC4,LFLOWGROUP_TSTORE,false) ;

		if (lasp->c.src5.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC5], 
			SRC5,LFLOWGROUP_TSTORE,false) ;


/* extra requests for relay forwarding */

		if (lasp->rftype == RFTYPE_RELAY) {

		if (lasp->c.dst1.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][DST1], 
			DST1,LFLOWGROUP_TSTORE,false) ;

		if (lasp->c.dst2.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][DST2], 
			DST2,LFLOWGROUP_TSTORE,false) ;

		if (lasp->c.dst3.a != -1)
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][DST3], 
			DST3,LFLOWGROUP_TSTORE,false) ;

		} /* end if (relay forwarding) */

		lasp->n.f_regfileread = true ;

		return 0;

	} /* end if (register file read) */


/* do we need to do some PE stuff ? */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	eprintf("las/AS_logic: PE stuff ?\n") ;
#endif

	if (lasp->c.re_exec_inst && (lasp->c.pe_tnumb == -1)) {

		PE_interface(lasp,pip,lip) ;


	} /* end if (we have an execution outstanding) */


	las_predicate_logic(lasp);  


	if (lasp->c.pI == 0)
		goto snoop_lab;


/* start doing something, what is this stuff ? */

	pp = &lasp->c.event_table[EXEC_ALU][PE][DST1];
	if (pp->valid) {

		if (lasp->c.dst1.a > 0)
			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST1],
			DST1,LFLOWGROUP_TSTORE,false) ;

		lasp->n.event_table[EXEC_ALU][PE][DST1].valid = false ;
	}


	pp = &lasp->c.event_table[EXEC_ALU][PE][DST2];
	if (pp->valid) {

		if(lasp->c.dst2.a > 0)
			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST2],
			DST2,LFLOWGROUP_TSTORE,false) ;

		lasp->n.event_table[EXEC_ALU][PE][DST2].valid = false ;
	}


	pp = &lasp->c.event_table[EXEC_ALU][PE][DST3];
	if (pp->valid) {

		if(lasp->c.dst3.a > 0)
			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST3],
			DST3,LFLOWGROUP_TSTORE,false) ;

		lasp->n.event_table[EXEC_ALU][PE][DST3].valid = false ;
	}



	pp = &lasp->c.event_table[RECV_NULLIFY][PE][MEM];
	if (pp->valid) {

		send_packet(lasp, 
			&lasp->n.event_table[SEND_NULLIFY][MFWB][MEM], 
			MEM,LFLOWGROUP_TNULLIFY,false) ;

		lasp->n.event_table[SEND_NULLIFY][MFWB][MEM].a = 
			lasp->c.old_mem.a; 

#if	CF_BADBADHACK
		lasp->c.old_mem.a = lasp->c.mem.a; /* hack, sorry!! */
#endif

		lasp->n.old_mem.a = lasp->c.mem.a;

		lasp->n.event_table[RECV_NULLIFY][PE][MEM].valid = false ;
	}


	pp = &lasp->c.event_table[EXEC_ST][PE][MEM]; 
	if (pp->valid) {

		send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][MFWB][MEM],
			MEM,LFLOWGROUP_TSTORE,false) ;

		lasp->n.event_table[EXEC_ST][PE][MEM].valid = false ;
	}


	pp = &lasp->c.event_table[EXEC_LD][PE][MEM];
	if (pp->valid) {

		lasp->n.mem.latesttt = INT_MIN;
		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][MBWB][MEM],
			MEM,LFLOWGROUP_TSTORE,false) ;

		lasp->n.event_table[EXEC_LD][PE][MEM].valid = false ;
	}

	pp = &lasp->c.event_table[EXEC_BR][PE][BCOND];
	if (pp->valid) {

		if (lasp->c.opexec == LEXECOP_JAL) 
			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST1],
			DST1,LFLOWGROUP_TSTORE,false) ;

		lasp->n.event_table[EXEC_BR][PE][BCOND].valid = false ;
	}


/* src1 */

	pp = &lasp->c.event_table[RECV_NULLIFY][RFWB][SRC1];
	if (pp->valid) {

		send_packet(lasp, 
		&lasp->n.event_table[REQ_VALUE][RBWB][SRC1],
		SRC1,LFLOWGROUP_TSTORE,false) ;

#if	CF_BADBADHACK
		lasp->c.src1.latesttt = INT_MIN;
#endif

		lasp->n.src1.latesttt = INT_MIN;

		lasp->n.event_table[RECV_NULLIFY][RFWB][SRC1].valid = false ;
		lasp->wait4src1 = 1;
	}

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][SRC1];
	if (pp->valid) {

#if	CF_BADBADHACK
		lasp->c.src1.latesttt = pp->tt;
#endif

		lasp->n.src1.latesttt = pp->tt;
		if(pp->d != lasp->c.src1.val) 
			lasp->n.re_exec_inst = 1;

#if	CF_BADBADHACK
		lasp->c.src1.val = pp->d;
#endif

		lasp->n.src1.val = pp->d;
		lasp->wait4src1 = 0;

		lasp->n.event_table[WAITON_VALUE][RFWB][SRC1].valid = false ;
		lasp->n.event_table[RECV_VALUE][RFWB][SRC1].valid = false ;
	}


/* src2 */

	pp = &lasp->c.event_table[RECV_NULLIFY][RFWB][SRC2];
	if (pp->valid) {

		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC2],
			SRC2,LFLOWGROUP_TSTORE,false) ;

#if	CF_BADBADHACK
		lasp->c.src2.latesttt = INT_MIN;
#endif

		lasp->n.src2.latesttt = INT_MIN;

		lasp->wait4src2 = 1;
		lasp->n.event_table[RECV_NULLIFY][RFWB][SRC2].valid = false ;
	}

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][SRC2];
	if (pp->valid) {

#if	CF_BADBADHACK
		lasp->c.src2.latesttt = pp->tt;
#endif

		lasp->n.src2.latesttt = pp->tt ;

		if (pp->d != lasp->c.src2.val) 
			lasp->n.re_exec_inst = 1;

#if	CF_BADBADHACK
		lasp->c.src2.val = pp->d;
#endif

		lasp->n.src2.val = pp->d;
		lasp->wait4src2 = 0;

		lasp->n.event_table[WAITON_VALUE][RFWB][SRC2].valid = false ;

		lasp->n.event_table[RECV_VALUE][RFWB][SRC2].valid = false ;

	} /* end if */


/* src3 */

	pp = &lasp->c.event_table[RECV_NULLIFY][RFWB][SRC3];
	if (pp->valid) {

		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC3],
			SRC3,LFLOWGROUP_TSTORE,false) ;

#if	CF_BADBADHACK
		lasp->c.src3.latesttt = INT_MIN;
#endif

		lasp->n.src3.latesttt = INT_MIN;

		lasp->wait4src3 = 1;
		lasp->n.event_table[RECV_NULLIFY][RFWB][SRC3].valid = false ;
	}

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][SRC3];
	if (pp->valid) {

#if	CF_BADBADHACK
		lasp->c.src3.latesttt = pp->tt;
#endif

		lasp->n.src3.latesttt = pp->tt ;

		if (pp->d != lasp->c.src3.val) 
			lasp->n.re_exec_inst = 1;

#if	CF_BADBADHACK
		lasp->c.src3.val = pp->d;
#endif

		lasp->n.src3.val = pp->d;
		lasp->wait4src3 = 0;

		lasp->n.event_table[WAITON_VALUE][RFWB][SRC3].valid = false ;

		lasp->n.event_table[RECV_VALUE][RFWB][SRC3].valid = false ;

	} /* end if */


/* src4 */

	pp = &lasp->c.event_table[RECV_NULLIFY][RFWB][SRC4];
	if (pp->valid) {

		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC4],
			SRC4,LFLOWGROUP_TSTORE,false) ;

#if	CF_BADBADHACK
		lasp->c.src4.latesttt = INT_MIN;
#endif

		lasp->n.src4.latesttt = INT_MIN;
		lasp->wait4src4 = 1;
		lasp->n.event_table[RECV_NULLIFY][RFWB][SRC4].valid = false ;
	}

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][SRC4];
	if (pp->valid) {

#if	CF_BADBADHACK
		lasp->c.src4.latesttt = pp->tt;
#endif

		lasp->n.src4.latesttt = pp->tt ;


		if (pp->d != lasp->c.src4.val) 
			lasp->n.re_exec_inst = 1;

#if	CF_BADBADHACK
		lasp->c.src4.val = pp->d;
#endif

		lasp->n.src4.val = pp->d;
		lasp->wait4src4 = 0;


		lasp->n.event_table[WAITON_VALUE][RFWB][SRC4].valid = false ;

		lasp->n.event_table[RECV_VALUE][RFWB][SRC4].valid = false ;

	} /* end if */


/* src5 */

	pp = &lasp->c.event_table[RECV_NULLIFY][RFWB][SRC5];
	if (pp->valid) {

		send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][RBWB][SRC5],
			SRC5,LFLOWGROUP_TSTORE,false) ;

#if	CF_BADBADHACK
		lasp->c.src5.latesttt = INT_MIN;
#endif

		lasp->n.src5.latesttt = INT_MIN;
		lasp->wait4src5 = 1;
		lasp->n.event_table[RECV_NULLIFY][RFWB][SRC5].valid = false ;
	}

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][SRC5];
	if (pp->valid) {

#if	CF_BADBADHACK
		lasp->c.src5.latesttt = pp->tt;
#endif

		lasp->n.src5.latesttt = pp->tt ;


		if (pp->d != lasp->c.src5.val) 
			lasp->n.re_exec_inst = 1;

#if	CF_BADBADHACK
		lasp->c.src5.val = pp->d;
#endif

		lasp->n.src5.val = pp->d;
		lasp->wait4src5 = 0;


		lasp->n.event_table[WAITON_VALUE][RFWB][SRC5].valid = false ;

		lasp->n.event_table[RECV_VALUE][RFWB][SRC5].valid = false ;

	} /* end if */


/* dst1 */

	pp = &lasp->c.event_table[RECV_VALUE][RFWB][DST1];
	if (pp->valid) {

		if (lasp->c.opclass != INSTR_LOAD) {

#if	CF_BADBADHACK
			lasp->c.dst1.latesttt = pp->tt;
#endif

			lasp->n.dst1.latesttt = pp->tt;

#if	CF_BADBADHACK
			lasp->c.dst1.oldd = pp->d;
#endif

			lasp->n.dst1.oldd = pp->d;
		}

		lasp->n.event_table[WAITON_VALUE][RFWB][DST1].valid = false ;
		lasp->n.event_table[RECV_VALUE][RFWB][DST1].valid = false ;
	}


	pp = &lasp->c.event_table[RECV_VALUE][RFWB][DST2];
	if (pp->valid) {

		if (lasp->c.opclass != INSTR_LOAD) {

#if	CF_BADBADHACK
			lasp->c.dst2.latesttt = pp->tt;
#endif

			lasp->n.dst2.latesttt = pp->tt;

#if	CF_BADBADHACK
			lasp->c.dst2.oldd = pp->d;
#endif

			lasp->n.dst2.oldd = pp->d;
		}

		lasp->n.event_table[RECV_VALUE][RFWB][DST2].valid = false ;
		lasp->n.event_table[WAITON_VALUE][RFWB][DST2].valid = false ;
	}


	pp = &lasp->c.event_table[RECV_VALUE][RFWB][DST3];
	if (pp->valid) {

		if (lasp->c.opclass != INSTR_LOAD) {

#if	CF_BADBADHACK
			lasp->c.dst3.latesttt = pp->tt;
#endif

			lasp->n.dst3.latesttt = pp->tt;

#if	CF_BADBADHACK
			lasp->c.dst3.oldd = pp->d;
#endif

			lasp->n.dst3.oldd = pp->d;
		}

		lasp->n.event_table[RECV_VALUE][RFWB][DST3].valid = false ;
		lasp->n.event_table[WAITON_VALUE][RFWB][DST3].valid = false ;
	}


/* receive a request for DST1 */

	pp = &lasp->c.event_table[RECV_REQ_VALUE][RBWB][DST1];

	if (pp->valid) {

		if (lasp->c.dst1.a > 0) {

			send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][RFWB][DST1],
			DST1,LFLOWGROUP_TSTORE,true) ;

		} /* end if */

		lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST1].valid = false ;

	} /* end if (receive a request for DST1) */


/* receive a request for DST2 */

	pp = &lasp->c.event_table[RECV_REQ_VALUE][RBWB][DST2];
	if (pp->valid) {

		if (lasp->c.dst2.a > 0)
			send_packet(lasp, 
				&lasp->n.event_table[SEND_VALUE][RFWB][DST2],
				DST2,LFLOWGROUP_TSTORE,true) ;

		lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST2].valid = false ;

	} /* end if (receive a request for DST2) */


/* receive a request for DST3 */

	pp = &lasp->c.event_table[RECV_REQ_VALUE][RBWB][DST3] ;
	if (pp->valid) {

		if (lasp->c.dst2.a > 0)
			send_packet(lasp, 
				&lasp->n.event_table[SEND_VALUE][RFWB][DST3],
				DST3,LFLOWGROUP_TSTORE,true) ;

		lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST3].valid = false ;

	} /* end if (receive a request for DST3) */


/* receive NULLIFY on MEM */

	pp = &lasp->c.event_table[RECV_NULLIFY][MFWB][MEM];
	if (pp->valid) {

		if (lasp->c.opclass == INSTR_LOAD) {

			send_packet(lasp, 
			&lasp->n.event_table[REQ_VALUE][MBWB][MEM],
			MEM,LFLOWGROUP_TSTORE,false) ;

			/*lasp->n.wait_ack_on_mfwb = LAS_LOAD_WATCHDOG; */

#if	CF_BADBADHACK
			lasp->c.mem.latesttt = INT_MIN;
#endif

			lasp->n.mem.latesttt = INT_MIN;
		}

		lasp->n.event_table[RECV_NULLIFY][MFWB][MEM].valid = false ;

	} /* end if (receive NULLIFY on MEM) */


/* receive value on MEM */

	pp = &lasp->c.event_table[RECV_VALUE][MFWB][MEM];
	if (pp->valid) {

		if (lasp->c.dst1.a > 0)
			send_packet(lasp, 
				&lasp->n.event_table[SEND_VALUE][RFWB][DST1],
				DST1,LFLOWGROUP_TSTORE,false) ;

		lasp->n.event_table[RECV_VALUE][MFWB][MEM].valid = false ;
		lasp->n.event_table[WAITON_VALUE][MFWB][MEM].valid = false ;
	}


/* receive a request for MEM */

	pp = &lasp->c.event_table[RECV_REQ_VALUE][MBWB][MEM];
	if (pp->valid) {

		send_packet(lasp, 
			&lasp->n.event_table[SEND_VALUE][MFWB][MEM],
			MEM,LFLOWGROUP_TSTORE,true) ;

		lasp->n.event_table[RECV_REQ_VALUE][MBWB][MEM].valid = false ;
	}



/* enter here from above */
snoop_lab:


/* possibly check the PE for a result */

	if (lasp->c.pe_tnumb != -1) {

		rs = PE_checkresult(lasp,pip) ;

		if (rs < 0)
			return rs ;

	} /* end (we had an outstanding execution) */


	MFWB_interface(lasp,pip,lip) ;


	MBWB_interface(lasp,pip,lip) ;


	RFWB_interface(lasp,pip,lip) ;


	RBWB_interface(lasp,pip,lip) ;


	PFWB_interface(lasp,pip,lip) ;


/*	las_predicate_logic(lasp);  */


	snoop_RFWB_interface(lasp,pip,lip) ;


	snoop_MFWB_interface(lasp,pip,lip) ;


	snoop_MBWB_interface(lasp,pip,lip) ;


	snoop_RBWB_interface(lasp,pip,lip) ;


	snoop_PFWB_interface(lasp,pip,lip) ;


/*	las_predicate_logic(lasp); */


	return SR_OK ;
}
/* end subroutine (AS_logic) */


local int 
RFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP wr ;

	struct las_packet * pp;

	int ret ;


/* send old destination 1 (DST1) */

	pp = &lasp->c.event_table[SEND_VALUE][RFWB][OLDDST1];
	if (pp->valid) {

	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst1.ftseq ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_VALUE][RFWB][OLDDST1].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent old dst1 on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst1.ftseq = 
			(lasp->c.dst1.ftseq + 1) & lasp->rfmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
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
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_VALUE][RFWB][OLDDST2].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent old dst2 on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst2.ftseq = 
			(lasp->c.dst2.ftseq + 1) & lasp->rfmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
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
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_NULLIFY][RFWB][DST1].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent nullify on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst1.ftseq = 
			(lasp->c.dst1.ftseq + 1) & lasp->rfmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
			printf("unsuccessful sending nulify on RFWB at %d\n", 
				lasp->asid);
#endif

		}
	}

/* send a NULLIFY destination 2 (DST2) */

	pp = &lasp->c.event_table[SEND_NULLIFY][RFWB][DST2];
	if (pp->valid) {
		
	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst2.ftseq ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_NULLIFY][RFWB][DST2].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent nullify on RFWB at %d; reg(%d)\n", 
			lasp->asid,wr.addr);
#endif

		lasp->n.dst2.ftseq = 
			(lasp->c.dst2.ftseq + 1) % lasp->rfmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
			printf("unsuccessful sending nulify on RFWB at %d\n", 
				lasp->asid);
#endif

		}
	}

	if (lasp->c.pI == 0)
		return 0;

/* send value destination 1 (DST1) */

	pp = &lasp->c.event_table[SEND_VALUE][RFWB][DST1];
	if (pp->valid) {

	    pp->d = lasp->c.dst1.val;
	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst1.ftseq ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

		    lasp->n.event_table[SEND_VALUE][RFWB][DST1].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf(
			    "successful write to RFWB at %d; reg(%d)=0x%x\n",
				 lasp->asid,wr.addr,wr.dv);
#endif

		lasp->n.dst1.ftseq = 
			(lasp->c.dst1.ftseq + 1) % lasp->rfmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RFWB at %d\n", 
				lasp->asid);
#endif

		}
	}

/* send value destination 1 (DST1) */

	pp = &lasp->c.event_table[SEND_VALUE][RFWB][DST2];
	if (pp->valid) {

	    pp->d = lasp->c.dst2.val;
	    read_packet(pp, &wr);

		wr.seq = lasp->c.dst2.ftseq ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rfwbus.outbus, lasp->buspriority, &wr) ;

		if (ret >= 0) {

		    lasp->n.event_table[SEND_VALUE][RFWB][DST2].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf(
			    "successful write to RFWB at %d; reg(%d)=0x%x\n",
				lasp->asid,wr.addr,wr.dv);
#endif

		lasp->n.dst2.ftseq = 
			(lasp->c.dst2.ftseq + 1) & lasp->rfmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RFWB at %d\n", 
				lasp->asid);
#endif

		}
	}


	return 0 ;
}
/* end subroutine (RFWB_interface) */


local int 
RBWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP wr ;

	LBUSINT *busintp ;

	struct las_packet * pp;

	int ret ;
	int	mi ;


/* request source 1 (SRC1) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][SRC1];
	if (pp->valid) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: SRC1 was valid\n") ;
#endif

		read_packet(pp, &wr);

		if (wr.addr == 0) {

		    lasp->n.event_table[REQ_VALUE][RBWB][SRC1].valid = false ;
			return 0;
		}

	    wr.seq = lasp->c.src1.btseq ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

		    lasp->n.event_table[REQ_VALUE][RBWB][SRC1].valid= false ;
		    lasp->n.event_table[WAITON_VALUE][RFWB][SRC1].valid = 
			false & 1;

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.src1.btseq = 
			(lasp->c.src1.btseq + 1) & lasp->rbmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif

		}
	}

/* request source 2 (SRC2) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][SRC2];
	if (pp->valid) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: SRC2 was valid\n") ;
#endif
		read_packet(pp, &wr);

		if (wr.addr == 0) {

		    lasp->n.event_table[REQ_VALUE][RBWB][SRC2].valid = false ;
			return 0;
		}

	    wr.seq = lasp->c.src2.btseq ;
	    wr.ott = lasp->c.tt ;	/* originator's time-tag */
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

		    lasp->n.event_table[WAITON_VALUE][RFWB][SRC2].valid = 
			false & 1;
		    lasp->n.event_table[REQ_VALUE][RBWB][SRC2].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.src2.btseq = 
			(lasp->c.src2.btseq + 1) & lasp->rbmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif

		}
	}

/* request destination 1 (DST1) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][DST1];
	if (pp->valid) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: DST1 was valid\n") ;
#endif

		read_packet(pp, &wr);

		if (wr.addr == 0) {

		    lasp->n.event_table[REQ_VALUE][RBWB][DST1].valid = false ;
			return 0;
		}

	    wr.seq = lasp->c.dst1.btseq ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

		    lasp->n.event_table[REQ_VALUE][RBWB][DST1].valid = false ;
		    lasp->n.event_table[WAITON_VALUE][RFWB][DST1].valid = 
			false & 1;

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.dst1.btseq = 
			(lasp->c.dst1.btseq + 1) & lasp->rbmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif

		}
	}

/* request destination 2 (DST2) */

	pp = &lasp->c.event_table[REQ_VALUE][RBWB][DST2];
	if (pp->valid) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	eprintf("las/RBWB_interface: DST2 was valid\n") ;
#endif

		read_packet(pp, &wr);

		if (wr.addr == 0) {

		    lasp->n.event_table[REQ_VALUE][RBWB][DST2].valid = false ;
			return 0;
		}

	    wr.seq = lasp->c.dst2.btseq ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->rbwbus.outbus, lasp->buspriority,&wr) ;

		if (ret >= 0) {

		    lasp->n.event_table[REQ_VALUE][RBWB][DST2].valid = false ;
		    lasp->n.event_table[WAITON_VALUE][RFWB][DST2].valid = 
				false & 1 ;

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to RBWB at %d; reg(%d)\n", 
				lasp->asid,wr.addr);
#endif

		lasp->n.dst2.btseq = 
			(lasp->c.dst2.btseq + 1) & lasp->rbmodmask ;

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to RBWB at %d\n", 
				lasp->asid);
#endif

		}
	}


	return 0 ;
}
/* end subroutine (RBWB_interface) */


local int 
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
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
	    wr.ftt = lasp->c.tt ;
	    ret = lbusint_write(lasp->pfwbus.outbus, lasp->buspriority,&wr) ;

	    if (ret >= 0) {

		lasp->n.fw_pred_on_pfwb = 0 ; /* changed from n to c */

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("successful write to PFWB at %d, pout(%d)=%d\n", 
			lasp->asid, lasp->c.pregs.pout.a,wr.dv);
#endif

	    } else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("unsuccessful write to PFWB at %d\n", lasp->asid);
#endif

	    }
	}

	return 0 ;
}
/* end subroutine (PFWB_interface) */


local int 
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
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
	    mi = getinterleave(lasp->mfwb_intleave,wr.addr) ;

	    busintp = lasp->mfwbus.head + mi ;
	    ret = lbusint_write(busintp,lasp->buspriority, &wr) ;

	    if (ret >= 0) {

		lasp->n.event_table[SEND_NULLIFY][MFWB][MEM].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("successfully sent nullify on MFWB at %d; mem(0x%x)\n", 
			lasp->asid,wr.addr);
#endif

	    } else {

#if	CF_FPRINTF && CF_LASDETAILS
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
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
		mi = getinterleave(lasp->mfwb_intleave,wr.addr) ;

		busintp = lasp->mfwbus.head + mi ;
		ret = lbusint_write(busintp, lasp->buspriority,&wr) ;

		if (ret >= 0) {

		    lasp->n.event_table[SEND_VALUE][MFWB][MEM].valid = false ;

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("successful write to MFWB at %d; "
				"mem(0x%x)=0x%x\n", 
				lasp->asid,wr.addr,wr.dv);
#endif

		} else  {

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
			printf("unsuccessful write to MFWB at %d\n", 
				lasp->asid);
#endif

		}

	}

	return 0 ;
}
/* end subroutine (MFWB_interface) */


/* memory backwarding bus operations */
local int MBWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	wr ;

	LBUSINT		*busintp ;

	struct las_packet	*pp ;

	int	rs1 ;
	int	ret ;
	int	mi ;


	pp = &lasp->c.event_table[REQ_VALUE][MBWB][MEM];
	if (pp->valid) {

		pp->a = lasp->c.mem.a;
		read_packet(pp, &wr);

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/MBWB_interface: 2 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

		wr.addr &= 0xfffffffc ;
		wr.ott = lasp->c.tt ;	/* originator's time-tag */
		wr.ftt = lasp->c.tt ;
		mi = getinterleave(lasp->mbwb_intleave,wr.addr) ;

		busintp = lasp->mbwbus.head + mi ;
	    ret = lbusint_write(busintp, lasp->buspriority,&wr) ;

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/MBWB_interface: 3 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

	    if (ret >= 0) {

		lasp->n.event_table[REQ_VALUE][MBWB][MEM].valid = false ;
		lasp->n.event_table[WAITON_VALUE][MFWB][MEM].valid = 
			false & 1;

		/*lasp -> n.wait_ack_on_mfwb = LAS_LOAD_WATCHDOG; */

#if	CF_SAFEFUNC
	if (lasp->sanity.func) {
	rs1 = (*lasp->sanity.func)(lasp->sanity.arg) ;
	if (rs1 < 0) {
#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/MBWB_interface: 3 bad sanityfunc LAS=%08lx\n",lasp) ;
#endif
		return rs1 ;
	}
	}
#endif /* CF_SAFEFUNC */

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("successful write to MBWB at %d; mem(0x%x)\n", 
			lasp->asid,wr.addr);
#endif

	    } else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("unsuccessful write to MBWB at %d\n", 
			lasp->asid);
#endif

		}

	} /* end if (valid) */

	return 0 ;
}
/* end subroutine (MBWB_interface) */


/* interface to send things to a PE */
local int PE_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	struct lpe_data ip;

	int	rs = SR_OK ;


	if ((lasp->c.re_exec_inst == 1) && (lasp->c.pI == 1)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 2)
		eprintf("las/PE_interface: id=%d LPE=%08lx need exe\n",
			lasp->asid,lasp->lpes) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
		if (pip->debuglevel > 2) {
			if (lasp->c.dst3.a != -1)
			eprintf("las/PE_interface: need to exe w/ DST3\n") ;
		}
#endif

		ip.opexec = lasp->c.opexec;

/* we use the rule: if non-zero address -> it is for real */

		ip.src1 = lasp->c.src1.a ? lasp->c.src1.val : 0;
		ip.src2 = lasp->c.src2.a ? lasp->c.src2.val : 0;
		ip.src3 = lasp->c.src3.a ? lasp->c.src3.val : 0;
		ip.src4 = lasp->c.src4.a ? lasp->c.src4.val : 0;
		ip.src5 = lasp->c.src5.a ? lasp->c.src5.val : 0;
		ip.dst1 = lasp->c.dst1.a ? lasp->c.dst1.val : 0;
		ip.dst2 = lasp->c.dst2.a ? lasp->c.dst2.val : 0;
		ip.dst3 = lasp->c.dst3.a ? lasp->c.dst3.val : 0;
		ip.cnst = lasp->c.cnst.val;
		ip.mema = lasp->c.mem.a;

		rs = lpe_req2exec(lasp->lpes, &ip, lasp->c.tt, 
			&lasp->n.pe_tnumb) ;

		if (rs == SR_OK) {

#if	CF_MASTERDEBUG && CF_DEBUG
		if (pip->debuglevel > 2)
			eprintf("las/PE_interface: id=%d LPE=%08lx tn=%d\n",
			lasp->asid,lasp->lpes,lasp->c.pe_tnumb) ;
#endif

			lasp->n.re_exec_inst = 0 ;

		} else {

#if	CF_MASTERDEBUG && CF_DEBUG
		if (pip->debuglevel > 2) {

			eprintf("las/PE_interface: id=%d LPE=%08lx\n",
				lasp->asid,lasp->lpes) ;
			eprintf(
			    "las/PE_interface: could not schedule rs=%d\n",
				rs) ;
		}
#endif

			rs = SR_OK ;

		} /* end if */

	} /* end if (needed an execution) */

	return rs ;
}
/* end subroutine (PE_interface) */


local int snoop_RFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	rd ;

	LBUSINT 	*busintp ;

	int	rs ;
	int	j, k ;
	int	f_match ;
	int	lastttsrc1 = INT_MIN;
	int	lastttsrc2 = INT_MIN;
	int	lastttsrc3 = INT_MIN;
	int	lastttsrc4 = INT_MIN;
	int	lastttsrc5 = INT_MIN;
	int	lastttdst1 = INT_MIN;
	int	lastttdst2 = INT_MIN;
	int	lastttdst3 = INT_MIN;
	int	sgtt ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: id=%d astt=%d path=%d\n",
			lasp->asid,lasp->c.tt,lasp->c.path) ;
#endif

/* get our Sharing Group TT */

	sgtt = +(lasp->c.tt / lip->nasrpsg) * lip->nasrpsg ;

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

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: SRC1 existing a=%d latesttt=%d tt=%d:%d tr=%d\n",
			lasp->c.src1.a,
			lasp->c.src1.latesttt,
			lasp->c.src1.tt,lasp->c.src1.frseq,
			lasp->c.src1.trans) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.src1.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.src1.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.src1.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.src1.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.src1.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttsrc1);

#if	CF_EXTRASNOOP
		if (f_match) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: SRC1 partial match\n") ;
#endif

		    if (rd.tt == lasp->c.src1.tt) {

			if (seqok(rd.seq,lasp->c.src1.frseq,lip->rfmod))
			    lasp->n.src1.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.src1.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
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
			&lasp->n.event_table[RECV_NULLIFY][RFWB][SRC1], 
			&rd);

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, Nullify from tt=%d; src1(%d)\n", 
			lasp->asid, rd.tt,lasp->c.src1.a);
#endif

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf(
		    "Snoop on RFWB at %d, data from tt=%d; src1(%d)=0x%x\n",
			lasp->asid, rd.tt,lasp->c.src1.a,rd.dv);
#endif

		    fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][SRC1],
					&rd);

		    lasp->n.event_table[RECV_NULLIFY][RFWB][SRC1].valid = 
				false ;

		/* to fix the problem with multiple span that dave had */
		    lasp->n.event_table[REQ_VALUE][RBWB][SRC1].valid = 
			false ;

		}
	    }

/* source 2 (src2) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: SRC2 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.src2.a,
			lasp->c.src2.latesttt,
			lasp->c.src2.tt,lasp->c.src2.frseq) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.src2.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.src2.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.src2.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.src2.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.src2.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttsrc2);

#if	CF_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.src2.tt) {

			if (seqok(rd.seq,lasp->c.src2.frseq,lip->rfmod))
			    lasp->n.src2.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.src2.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
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

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, Nullify from tt=%d; src2(%d)\n",
			lasp->asid, rd.tt,lasp->c.src2.a);
#endif

		    fill_packet(&lasp->n.event_table[RECV_NULLIFY][RFWB][SRC2],
				&rd);

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; src2(%d)=0x%x\n",
			 lasp->asid, rd.tt,lasp->c.src2.a,rd.dv);
#endif

		    fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][SRC2],
			&rd);

		    lasp->n.event_table[RECV_NULLIFY][RFWB][SRC2].valid = 
				false ;

		/* to fix the problem with multiple span that dave had */
		    lasp->n.event_table[REQ_VALUE][RBWB][SRC2].valid = false ;

		}
	    }

/* source 3 (src3) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: SRC3 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.src3.a,
			lasp->c.src3.latesttt,
			lasp->c.src3.tt,lasp->c.src3.frseq) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.src3.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.src3.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.src3.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.src3.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.src3.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttsrc3);

#if	CF_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.src3.tt) {

			if (seqok(rd.seq,lasp->c.src3.frseq,lip->rfmod))
			    lasp->n.src3.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.src3.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d SRC3 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.src3.trans = rd.trans ;
		    lasp->n.src3.tt = rd.tt ;
		    lastttsrc3 = rd.tt;
		    if (rd.trans == LFLOWGROUP_TNULLIFY) {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, Nullify from tt=%d; src3(%d)\n",
			lasp->asid, rd.tt,lasp->c.src3.a);
#endif

		    fill_packet(&lasp->n.event_table[RECV_NULLIFY][RFWB][SRC3],
				&rd);

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; src3(%d)=0x%x\n",
			 lasp->asid, rd.tt,lasp->c.src3.a,rd.dv);
#endif

		    fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][SRC3],
			&rd);

		    lasp->n.event_table[RECV_NULLIFY][RFWB][SRC3].valid = 
				false ;

		/* to fix the problem with multiple span that dave had */
		    lasp->n.event_table[REQ_VALUE][RBWB][SRC3].valid = false ;

		}
	    }

/* source 4 (src4) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: SRC4 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.src4.a,
			lasp->c.src4.latesttt,
			lasp->c.src4.tt,lasp->c.src4.frseq) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.src4.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.src4.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.src4.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.src4.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.src4.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttsrc4);

#if	CF_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.src4.tt) {

			if (seqok(rd.seq,lasp->c.src4.frseq,lip->rfmod))
			    lasp->n.src4.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.src4.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d SRC4 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.src4.trans = rd.trans ;
		    lasp->n.src4.tt = rd.tt ;
		    lastttsrc4 = rd.tt;
		    if (rd.trans == LFLOWGROUP_TNULLIFY) {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, Nullify from tt=%d; src4(%d)\n",
			lasp->asid, rd.tt,lasp->c.src4.a);
#endif

		    fill_packet(&lasp->n.event_table[RECV_NULLIFY][RFWB][SRC4],
				&rd);

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; src4(%d)=0x%x\n",
			 lasp->asid, rd.tt,lasp->c.src4.a,rd.dv);
#endif

		    fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][SRC4],
			&rd);

		    lasp->n.event_table[RECV_NULLIFY][RFWB][SRC4].valid = 
				false ;

		/* to fix the problem with multiple span that dave had */
		    lasp->n.event_table[REQ_VALUE][RBWB][SRC4].valid = false ;

		}
	    }

/* source 5 (src5) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: SRC5 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.src5.a,
			lasp->c.src5.latesttt,
			lasp->c.src5.tt,lasp->c.src5.frseq) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.src5.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.src5.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.src5.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.src5.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.src5.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttsrc5);

#if	CF_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.src5.tt) {

			if (seqok(rd.seq,lasp->c.src5.frseq,lip->rfmod))
			    lasp->n.src5.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.src5.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d SRC5 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.src5.trans = rd.trans ;
		    lasp->n.src5.tt = rd.tt ;
		    lastttsrc5 = rd.tt;
		    if (rd.trans == LFLOWGROUP_TNULLIFY) {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, Nullify from tt=%d; src5(%d)\n",
			lasp->asid, rd.tt,lasp->c.src5.a);
#endif

		    fill_packet(&lasp->n.event_table[RECV_NULLIFY][RFWB][SRC5],
				&rd);

		} else {

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; src5(%d)=0x%x\n",
			 lasp->asid, rd.tt,lasp->c.src5.a,rd.dv);
#endif

		    fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][SRC5],
			&rd);

		    lasp->n.event_table[RECV_NULLIFY][RFWB][SRC5].valid = 
				false ;

		/* to fix the problem with multiple span that dave had */
		    lasp->n.event_table[REQ_VALUE][RBWB][SRC5].valid = false ;

		}
	    }

/* destination 1 (dst1) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: DST1 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.dst1.a,
			lasp->c.dst1.latesttt,
			lasp->c.dst1.tt,lasp->c.dst1.frseq) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst1.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst1.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.dst1.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.dst1.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.dst1.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttdst1);

#if	CF_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.dst1.tt) {

			if (seqok(rd.seq,lasp->c.dst1.frseq,lip->rfmod))
			    lasp->n.dst1.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.dst1.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
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

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; dst(%d)=0x%x\n",
			lasp->asid, rd.tt,lasp->c.dst1.a,rd.dv);
#endif

		fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][DST1],&rd);

	    }

/* destination 2 (dst2) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: DST2 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.dst2.a,
			lasp->c.dst2.latesttt,
			lasp->c.dst2.tt,lasp->c.dst2.frseq) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst2.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst2.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.dst2.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.dst2.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.dst2.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttdst2);

#if	CF_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.dst2.tt) {

			if (seqok(rd.seq,lasp->c.dst2.frseq,lip->rfmod))
			    lasp->n.dst2.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.dst2.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
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

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; dst(%d)=0x%x\n",
			lasp->asid, rd.tt,lasp->c.dst1.a,rd.dv);
#endif

		fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][DST2],
			&rd);

	        }

/* destination 3 (dst3) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
                "las_srfb: DST3 existing a=%d latesttt=%d tt=%d:%d\n",
			lasp->c.dst3.a,
			lasp->c.dst3.latesttt,
			lasp->c.dst3.tt,lasp->c.dst3.frseq) ;
#endif

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst3.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst3.val) ; */
		f_match = f_match && (rd.tt < lasp->c.tt) ;

#if	CF_EXTRASNOOP
		if (lasp->c.dst3.trans != LFLOWGROUP_TNULLIFY)
			f_match = f_match && (rd.tt >= lasp->c.dst3.tt) ;
#else
		f_match = f_match && (rd.tt >= lasp->c.dst3.latesttt) ;
#endif

		f_match = f_match && (rd.tt >= lastttdst3);

#if	CF_EXTRASNOOP
		if (f_match) {

		    if (rd.tt == lasp->c.dst3.tt) {

			if (seqok(rd.seq,lasp->c.dst3.frseq,lip->rfmod))
			    lasp->n.dst3.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

			else
			    f_match = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("las_srfb: extrasnoop=%d\n",f_match) ;
#endif

		    } else
			lasp->n.dst3.frseq = 
				(rd.seq + 1) & lasp->rfmodmask ;

		} /* end if (sequence check) */
#endif /* CF_EXTRASNOOP */

		if (f_match) {

#if	CF_FPRINTF && CF_LASDETAILS2
		fprintf(stdout,
		     "las_srfb: id=%d astt=%d DST3 tr=%d p=%d tt=%d:%d "
			"a=%d dp=%d dv=%08x\n",
			lasp->asid,lasp->c.tt,
			rd.trans,rd.path,
			rd.tt,rd.seq,
			rd.addr,rd.dp,rd.dv) ;
#endif

		    lasp->n.dst3.trans = rd.trans ;
		    lasp->n.dst3.tt = rd.tt ;

		}

		if (f_match && (rd.trans != LFLOWGROUP_TNULLIFY)) {

		    lastttdst3 = rd.tt;

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);
		printf("Snoop on RFWB at %d, data from tt=%d; dst(%d)=0x%x\n",
			lasp->asid, rd.tt,lasp->c.dst1.a,rd.dv);
#endif

		fill_packet(&lasp->n.event_table[RECV_VALUE][RFWB][DST3],
			&rd);

	        }


	    } /* end if (got a valid bus value) */

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

	int	rs ;
	int	n, k ;
	int	f_match ;
	int	lastttdst1 = INT_MIN;
	int	lastttdst2 = INT_MIN;
	int	lastttdst3 = INT_MIN;
	int	sgtt ;
	int	ovtt ;


/* get our Sharing Group TT */

	sgtt = +(lasp->c.tt / lip->nasrpsg) * lip->nasrpsg ;
	ovtt = lip->nsg * lip->nasrpsg ;

	n = lasp->rbwbus.num ;
	rd.addr = lasp->c.src1.a;

	busintp = lasp->rbwbus.head ;
	for (k = 0 ; k < n ; k += 1) {

	    rs = lbusint_read(busintp,&rd) ;

	    if ((rs > 0) && (rd.ftt > lasp->c.tt)) {

/* destination 1 */

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst1.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst1.val) ; */
		f_match = f_match && (rd.tt > lasp->c.tt) ;
		f_match = f_match && (rd.tt >= lastttdst1);

		if (f_match) {

		    lastttdst1 = rd.tt;
		    fill_packet(
			&lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST1],&rd);

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
		    printf("Snoop on RBWB at %d, data from tt=%d;reg(%d)\n", 
			lasp->asid, rd.tt,rd.addr);
#endif

		} /* end if (match DST1) */
		
/* destination 2 */

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst2.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst2.val) ; */
		f_match = f_match && (rd.tt > lasp->c.tt) ;
	        f_match = f_match && (rd.tt >= lastttdst2);

	        if (f_match) {

		    lastttdst2 = rd.tt;
		    fill_packet(
			&lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST2],&rd);

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
		    printf("Snoop on RBWB at %d, data from tt=%d;reg(%d)\n",
			lasp->asid, rd.tt,rd.addr);
#endif

		} /* end if (match DST2) */

/* destination 3 */

		f_match = true ;
		f_match = f_match && (rd.path == lasp->c.path) ;
		f_match = f_match && (rd.addr == lasp->c.dst3.a) ;
		/*    f_match = f_match && (rd.d != lasp->c.dst3.val) ; */
		f_match = f_match && (rd.tt > lasp->c.tt) ;
	        f_match = f_match && (rd.tt >= lastttdst3);

	        if (f_match) {

		    lastttdst3 = rd.tt;
		    fill_packet(
			&lasp->n.event_table[RECV_REQ_VALUE][RBWB][DST3],&rd);

#if	CF_FPRINTF && CF_LASDETAILS
			las_print_asid(lasp);
		    printf("Snoop on RBWB at %d, data from tt=%d;reg(%d)\n",
			lasp->asid, rd.tt,rd.addr);
#endif

		} /* end if (match DST3) */

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
local int 
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
			f_match = true ;
			f_match = f_match && (rd.path == lasp->c.path) ;
			f_match = f_match && (rd.addr == lasp->c.pregs.pin.a) ;
			/*    f_match = f_match && (rd.d != lasp->c.pregs.pin.val) ; */
			f_match = f_match && (rd.tt < lasp->c.tt) ;
			f_match = f_match && (rd.tt >= lasp->c.pregs.pin.latesttt) ;
		        f_match = f_match && (rd.tt >= lasp->n.pregs.pin.latesttt) ;
		        if (f_match) {
			    lasp->n.pregs.pin.latesttt = rd.tt ;
			    lasp->n.pregs.pin.val = rd.dv & 1 ;

#if	CF_FPRINTF && CF_LASDETAILS
			    las_print_asid(lasp);
			    printf("Snoop on PFWB at %d, "
				"data from tt=%d; pin(%d)=0x%x\n", 
				lasp->asid, rd.tt,
					lasp->c.pregs.pin.a, rd.dv);
#endif

			}

			for (l=0; l < lasp->c.pregs.num_of_cpin_regs; ++l) {

				lasp->n.pregs.cpin[l].latesttt = lasp->n.pregs.cpin[l].latesttt;
				f_match = true ;
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

#if	CF_FPRINTF && CF_LASDETAILS
				    las_print_asid(lasp);
				    printf("Snoop on PFWB at %d, "
				"data from tt=%d; cpin[%d](%d)=0x%x\n", 
				lasp->asid, rd.tt,l,
				lasp->c.pregs.cpin[l].a, rd.dv);
#endif

				}
		
		        } /* end for */

	  	}

	    busintp += 1 ;

/* get out early if we are the **first** Sharing Group in the e-window */

		if (sgtt <= 0)
			break ;

	} /* end for (k) */

	return 0 ;
}
/* end subroutine (snoop_PFWB_interface) */


/* snoop the memory forwarding buses */
local int snoop_MFWB_interface(lasp,pip,lip)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LFLOWGROUP	rd ;

	LBUSINT		*busintp ;

	int	j, k ;
	int	mi ;
	int	f_match ;


	if ((lasp->c.opclass != INSTR_LOAD) && 
		(lasp->c.opclass != INSTR_STORE))
	    return 0 ;

	rd.addr = lasp->c.mem.a ;

/* we snoop the correct memory bus based on our address */

	mi = getinterleave(lasp->mfwb_intleave,rd.addr) ;

	busintp = lasp->mfwbus.head + mi ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2) {
	    eprintf(
	        "las/snoop_MFWB_interface: snooping addr=%08x bi=%d\n",
	        lasp->c.mem.a,mi) ;
	    eprintf(
	        "las/snoop_MFWB_interface: reading LBUSINT=%08lx\n",
	        busintp) ;
	}
#endif /* CF_DEBUG */

	if (lbusint_read(busintp, &rd) > 0) {

	    f_match = true ;
	    f_match = f_match && (rd.path == lasp->c.path) ;
	    f_match = f_match && (rd.addr == (lasp->c.mem.a & 0xfffffffc)) ;
/* f_match = f_match && (rd.d != lasp->c.mem.val) ; */
	    f_match = f_match && (rd.tt < lasp->c.tt) ;
	    f_match = f_match && (rd.tt >= lasp->c.mem.latesttt) ;

	    if (f_match &&
	        (lasp->c.opclass == INSTR_LOAD)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 2)
	            eprintf(
	                "las/snoop_MFWB_interface: load read dv=%08x\n",rd.dv) ;
#endif

	        if (rd.trans == LFLOWGROUP_TNULLIFY) {

#if	CF_FPRINTF && CF_LASDETAILS
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


#if	CF_FPRINTF && CF_LASDETAILS
	            las_print_asid(lasp) ;

	            printf("Snoop on MFWB at %d, data from tt=%d;"
	                "mem(0x%x)=0x%x\n",
	                lasp->asid, rd.tt,rd.addr,rd.dv) ;
#endif

	            lasp->n.mem.latesttt = rd.tt ;
	            boundary = lasp->c.mem.a & 3 ;

#if	CF_MASTERDEBUG && CF_DEBUG
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

	            case LEXECOP_LWC1:
	                lasp->n.dst1.val = rd.dv ;
	                break ;

	            default:
	                assert("illegal opcode") ;
	                break ;

	            } /* end switch */

#if	CF_MASTERDEBUG && CF_DEBUG
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


local int snoop_MBWB_interface(lasp,pip,lip)
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

	    f_match = true ;
	    f_match = f_match && (rd.path == lasp->c.path) ;
	    f_match = f_match && (rd.addr == (lasp->c.mem.a & 0xfffffffc)) ;
	    /*    f_match = f_match && (rd.d != lasp->c.mem.val) ; */
	    f_match = f_match && (rd.tt > lasp->c.tt) ;

	    if (f_match && (lasp->c.opclass == INSTR_STORE)) {

		fill_packet(
			&lasp->n.event_table[RECV_REQ_VALUE][MBWB][MEM],
			&rd);

#if	CF_FPRINTF && CF_LASDETAILS
		las_print_asid(lasp);

		    fprintf(stdout,
			"Snoop on MBWB at %d, data from tt=%d; mem(0x%x)\n", 
				lasp->asid, rd.tt, rd.addr);
#endif

		}

	} /* end if */

	return 0 ;
}
/* end subroutine (snoop_MBWB_interface) */


/* check for results coming back from a PE */
local int PE_checkresult(lasp,pip)
struct proginfo	*pip ;
LAS		*lasp ;
{
	struct lpe_data ip ;

	LFLOWGROUP	rd ;

	int	rs = SR_OK ;
	int	ret, boundry ;


	if (lasp->c.pe_tnumb == -1)
		return rs ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
		eprintf("las/PE_checkresult: LPE=%08lx checking tn=%d\n",
			lasp->lpes,lasp->c.pe_tnumb) ;
#endif

	ret = lpe_isexecuted(lasp->lpes, &ip, lasp->c.pe_tnumb) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2) {

		if (ret == SR_INPROGRESS)
		eprintf("las/PE_checkresult: lpe_isexecuted() INPROGRESS\n") ;

		else
		eprintf("las/PE_checkresult: lpe_isexecuted() rs=%d\n",
			ret) ;
	}
#endif /* CF_DEBUG */

	if (ret == SR_OK) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
		eprintf("las/PE_checkresult: executed tn=%d\n",
			lasp->lpes,lasp->c.pe_tnumb) ;
#endif

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 0 bad sanity LAS=%08lx\n",lasp) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

		lasp->n.pe_tnumb = -1;
		if (lasp->c.dst1.a == 0)
			lasp->n.dst1.val = 0;

		else
			lasp->n.dst1.val = ip.dst1 ;

		if (lasp->c.dst2.a == 0)
			lasp->n.dst2.val = 0;

		else
			lasp->n.dst2.val = ip.dst2 ;

		if (lasp->c.dst3.a == 0)
			lasp->n.dst3.val = 0;

		else {

			lasp->n.dst3.val = ip.dst3 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
		eprintf("las/PE_checkresult: DST3 updated value\n") ;
#endif

		}

		lasp->n.mem.a = ip.mema ;
		lasp->n.bcond = ip.bcnd ;

		rd.tt = lasp->c.tt;
		rd.path = lasp->c.path;
		
		if (lasp->c.opclass == INSTR_ALU) {

			rd.dv = ip.dst1 ;
			rd.addr = lasp->c.dst1.a;
			fill_packet(&lasp->n.event_table[EXEC_ALU][PE][DST1], 
					&rd);

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 1 bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

			if (lasp->c.dst2.a != -1) {

				rd.dv = ip.dst2 ;
				rd.addr = lasp->c.dst2.a;
				fill_packet(
				&lasp->n.event_table[EXEC_ALU][PE][DST2], 
					&rd);

			} /* end if (destination 2) */

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 2 bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

			if (lasp->c.dst3.a != -1) {

				rd.dv = ip.dst3 ;
				rd.addr = lasp->c.dst3.a;
				fill_packet(
				&lasp->n.event_table[EXEC_ALU][PE][DST3], 
					&rd);

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 2)
		eprintf("las/PE_checkresult: filled packet on DST3\n") ;
#endif

			} /* end if (destination 3) */

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 3a bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

		} else if (lasp->c.opclass == INSTR_STORE) {

			if ((lasp->c.mem.a != -1) && 
				(lasp->c.old_mem.a != ip.mema)) {

			/* do not send nulify for the first execution */

				/* rd.addr = ip.mema ; */
				fill_packet(
				&lasp->n.event_table[RECV_NULLIFY][PE][MEM], 
					&rd);

				/*lasp->n.send_nulify_on_mfwb = 1; */

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 4 bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

			} /* end if (memory thing) */

			rd.addr = ip.mema ;
			boundry = rd.addr & 3 ;
			switch (lasp->c.opexec) {

				case LEXECOP_SW:
					lasp->n.mem.val = lasp->c.src2.val;
					lasp->n.dp = LFLOWGROUP_DPALL;
					break;

				case LEXECOP_SB:
					if (boundry == 0) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE0 ;	
						lasp->n.mem.val = 
						(lasp->c.src2.val & 0xff) << 24;

					} else if (boundry == 1) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE1 ;	
						lasp->n.mem.val = 
						(lasp->c.src2.val & 0xff) << 16;

					} else if (boundry == 2) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE2 ;	
						lasp->n.mem.val = 
						(lasp->c.src2.val & 0xff) << 8;

					} else {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE3 ;	
						lasp->n.mem.val = 
						(lasp->c.src2.val & 0xff);

					} 

					break;
					
				case LEXECOP_SH:
					if (boundry == 0) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE0 | 
						LFLOWGROUP_DPBYTE1 ;
						lasp->n.mem.val = 
					(lasp->c.src2.val & 0xffff) << 16;

					} else if (boundry == 1) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE1 | 
						LFLOWGROUP_DPBYTE2 ;
						lasp->n.mem.val = 
					(lasp->c.src2.val & 0xffff) << 8;

					} else if (boundry == 2) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE2 | 
						LFLOWGROUP_DPBYTE3 ;
						lasp->n.mem.val = 
						(lasp->c.src2.val & 0xffff);

					} else {

						/* misalligned mem access */	

					}

					break;
					
				case LEXECOP_SWL:
					lasp->n.mem.val = 
					(lasp->c.src2.val) >> (8*boundry);

					if (boundry == 0) {

						lasp->n.dp = LFLOWGROUP_DPALL;

					} else if (boundry == 1) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE1 | 
						LFLOWGROUP_DPBYTE2 | 
						LFLOWGROUP_DPBYTE3 ;

					} else if (boundry == 2) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE2 | 
						LFLOWGROUP_DPBYTE3 ;

					} else {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE3 ;	

					} 

					break;

				case LEXECOP_SWR:
					lasp->n.mem.val = 
					lasp->c.src2.val << ((3-boundry)*8);

					if (boundry == 0) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE0 ;

					} else if (boundry == 1) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE0 | 
						LFLOWGROUP_DPBYTE1 ;

					} else if (boundry == 2) {

						lasp->n.dp = 
						LFLOWGROUP_DPBYTE0 | 
						LFLOWGROUP_DPBYTE1 | 
						LFLOWGROUP_DPBYTE2 ;

					} else {

						lasp->n.dp = LFLOWGROUP_DPALL;	

					} 

					break;

	case LEXECOP_SWC1:
		lasp->n.mem.val = lasp->c.src2.val ;
	    lasp->n.dp = LFLOWGROUP_DPALL ;
		break ;

				default:
					assert("unknown OP!");
					break;

			} /* end switch (opexec) */

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 10 bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

			rd.dv = lasp->n.mem.val;
			rd.dp = lasp->n.dp;
			fill_packet(&lasp->n.event_table[EXEC_ST][PE][MEM], 
				&rd);

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 11 bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

		} else if (lasp->c.opclass == INSTR_LOAD) {

			rd.addr = ip.mema ;
			fill_packet(&lasp->n.event_table[EXEC_LD][PE][MEM], 
				&rd);
			/*lasp->n.fw_mem_on_mbwb = 1; */

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 12 bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

		} else if ((lasp->c.opclass == INSTR_BREL) || 
			(lasp->c.opclass == INSTR_JREL) ||
			(lasp->c.opclass == INSTR_JIND)) {

			rd.dv = ip.bcnd ;
			fill_packet(&lasp->n.event_table[EXEC_BR][PE][BCOND], 
				&rd);

			if (lasp->c.opexec == LEXECOP_JAL) {
				lasp->n.dst1.val = lasp->c.faddr + 8;

			} 

			if (lasp->c.opexec == LEXECOP_JR) {
				lasp->n.cnst.val = lasp->c.src1.val;
			}

#if	CF_MASTERDEBUG && CF_SAFE2
	rs = las_sanitycheck(lasp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: 13 bad sanity LAS=%08lx rs=%d\n",
		lasp,rs) ;
#endif

		return rs ;
	}
#endif /* CF_SAFE2 */

		} /* end if (control-flow-change) */

	} else if (ret == SR_INPROGRESS) {

		rs = SR_OK ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: execution in progress tn=%d\n",
			lasp->c.pe_tnumb) ;
#endif

	} else {

		rs = ret ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las/PE_checkresult: execution bad rs=%d\n",
			rs) ;
#endif

	}

	return rs ;
}
/* end subroutine (PE_checkresult) */


/****

This funciton calculates the predicate and canceling predicate values
   using the value of the valid predicate registers.  
It also assignes values to
   the output predicate and canceling predicates.  

****/

int las_predicate_logic(lasp)
LAS	*lasp;
{
	int p;
	int cpin=0;
	int i;


#if	PERCF_FETCH

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
		lasp ->n.pregs.pout.val = lasp->c.bcond ? 0 : p ;
		lasp ->n.pregs.cpout.val = lasp->c.bcond ? p : 0 ; 
		lasp -> n.pI = 1;
	} else {
		lasp ->n.pregs.pout.val = p;
		lasp ->n.pI = p;
	}
	
	if (p==1 && lasp->c.pI ==0)
		lasp->n.re_exec_inst = 1;

	if (p==0 && lasp->c.pI==1) {

		if(lasp->c.opclass == INSTR_STORE) {

			send_packet(lasp, 
				&lasp->n.event_table[SEND_NULLIFY][MFWB][MEM],
				MEM,LFLOWGROUP_TNULLIFY,false) ;

		} else {

			if (lasp->c.dst1.a > 0) {

/* relay forwarding */

			if (lasp->rftype == RFTYPE_RELAY)
				send_packet(lasp, 
				&lasp->n.event_table[SEND_VALUE][RFWB][OLDDST1],
				OLDDST1,LFLOWGROUP_TSTORE,false) ;

/* NULLIFY forwarding */

			else if (lasp->rftype == RFTYPE_NULLIFY)
				send_packet(lasp, 
				&lasp->n.event_table[SEND_NULLIFY][RFWB][DST1],
				DST1,LFLOWGROUP_TNULLIFY,false) ;


			}

			if(lasp->c.dst2.a > 0) {

/* relay forwarding */

			if (lasp->rftype == RFTYPE_RELAY)
				send_packet(lasp, 
				&lasp->n.event_table[SEND_VALUE][RFWB][OLDDST2],
				OLDDST2,LFLOWGROUP_TSTORE,false) ;

/* NULLIFY forwarding */

			else if (lasp->rftype == RFTYPE_NULLIFY)
				send_packet(lasp, 
				&lasp->n.event_table[SEND_NULLIFY][RFWB][DST2],
				DST2,LFLOWGROUP_TNULLIFY,false) ;

			}
		}
	}	

	if (lasp -> c.pregs.pout.a != -1) {

	if ((lasp -> n.pregs.pout.val != lasp -> c.pregs.pout.val) || 
		(lasp -> n.pregs.cpout.val != lasp -> c.pregs.cpout.val )) {

			if (lasp -> n.fw_pred_on_pfwb == 1) {

			lasp->c.pregs.pout.val = lasp->n.pregs.pout.val; 
			lasp->c.pregs.cpout.val = lasp->n.pregs.cpout.val;

			} else
				lasp -> n.fw_pred_on_pfwb = 1;

		}

	} /* end if */

	lasp->pred = lasp->n.pI;
/*
	if(lasp->n.pI == 0) {
		if(lasp->c.dst1.a != -1)
			lasp->n.fw_dst1_on_rfwb = 1;
		if(lasp->c.dst2.a != -1)
			lasp->n.fw_dst2_on_rfwb = 1;
	}
*/

	return 0 ;
}
/* end subroutine (las_predicate_logic) */


/* check the bus read registers */
local int las_readrfbuses(lasp,pip,lip,busintp,n)
LAS		*lasp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
LBUSINT		*busintp ;
int		n ;
{

	LFLOWGROUP	*fgp, *tfgp ;

	int	rs, i, j ;

#if	CF_FPRINTF || (CF_MASTERDEBUG && CF_DEBUG)
	char	ttbuf[40] ;
#endif


	for (i = 0 ; i < n ; i += 1) {

	    fgp = &lasp->busregs[i].fg ;
	    rs = lbusint_read(busintp + i,fgp) ;

	    lasp->busregs[i].f_v = ((rs > 0) && 
		(fgp->ftt <= lasp->c.tt) && (fgp->tt < lasp->c.tt)) ;

#if	CF_FPRINTF && CF_LASDETAILS3
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

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel > 4) && lasp->busregs[i].f_v) {
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

#if	CF_EXTRASNOOP

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

	                lasp->busregs[j].f_v = false ;

#if	CF_MASTERDEBUG && CF_DEBUG
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
#endif /* CF_DEBUG */

			}

			} /* end if */

		    } /* end if */

	        } /* end for */

	    } /* end if */

	} /* end for */

#endif /* CF_EXTRASNOOP */

	return SR_OK ;
}
/* end subroutine (las_readrfbuses) */


/* fill up a packet */
local int
fill_packet(pp, rd)
struct las_packet * pp;
LFLOWGROUP * rd;
{


	pp->valid = true ;
	pp->trans = rd->trans;
	pp->a = rd->addr;
	pp->d = rd->dv;
	pp->tt = rd->tt;
	pp->pathid = rd->path;
	pp->dp = rd->dp;
	pp->f_resp = rd->f_resp ;

	return 0 ;
}
/* end subroutine (fill_packet) */


/* send a packet (to where ? :-) */
local int send_packet(lasp, pp, stor, trans,f_resp)
LAS	*lasp ;
struct las_packet * pp;
las_storage stor;
uint	trans;
int	f_resp ;
{


	pp->valid = true ;
	pp->tt = lasp->c.tt;
	pp->pathid = lasp->c.path;
	pp->trans = trans;
	pp->dp = lasp->c.dp;
	pp->f_resp = f_resp ;

	switch (stor) {

	case SRC1:
		pp->a = lasp->c.src1.a;
		pp->d = lasp->c.src1.val;
		break;

	case SRC2:
		pp->a = lasp->c.src2.a;
		pp->d = lasp->c.src2.val;
		break;

	case SRC3:
		pp->a = lasp->c.src3.a ;
		pp->d = lasp->c.src3.val ;
		break ;

	case SRC4:
		pp->a = lasp->c.src4.a ;
		pp->d = lasp->c.src4.val ;
		break ;

	case SRC5:
		pp->a = lasp->c.src5.a ;
		pp->d = lasp->c.src5.val ;
		break ;

	case DST1:
		pp->a = lasp->c.dst1.a;
		pp->d = lasp->c.dst1.val;
		break;

	case DST2:
		pp->a = lasp->c.dst2.a ;
		pp->d = lasp->c.dst2.val ;
		break ;

	case DST3:
		pp->a = lasp->c.dst3.a ;
		pp->d = lasp->c.dst3.val ;
		break ;

	case OLDDST1:
		pp->a = lasp->c.dst1.a ;
		pp->d = lasp->c.dst1.oldd ;
		break ;

	case OLDDST2:
		pp->a = lasp->c.dst2.a ;
		pp->d = lasp->c.dst2.oldd ;
		break ;

	case OLDDST3:
		pp->a = lasp->c.dst3.a ;
		pp->d = lasp->c.dst3.oldd ;
		break ;

	case MEM:
		pp->a = lasp->c.mem.a ;
		pp->d = lasp->c.mem.val ;
		break ;

	default:
		assert(0);

	} /* end switch */

	return 0 ;
}
/* end subroutine (send_packet) */


local int
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
	wr->f_resp = pp->f_resp ;

	return 0 ;
}
/* end subroutine (read_packet) */


#if	CF_FPRINTF || (CF_MASTERDEBUG && CF_DEBUG) || CF_XML

local int mkttbuf(buf,tt)
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

#endif /* CF_DEBUG */


local int
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


local int
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


local int
las_print_pred_regs(struct pred_regs *prp) 
{

printf("pin(%d)=%d, cpin(%d)=%d, ",
	prp->pin.a, prp->pin.val, prp->cpin[0].a, prp->cpin[0].val);
printf("pout(%d)=%d, cpout(%d)=%d\n",
	prp->pout.a, prp->pout.val, prp->cpout.a, prp->cpout.val);

}


local int
las_print_asid(lasp)
LAS *lasp;
{
	ulong clock ;
	lsim_getclock(lasp->mip,&clock) ;
	printf("C%lld,ASID%d,TT%d,FA0x%x=>",
		clock,lasp->asid,lasp->c.tt,lasp->c.faddr);
	return 0 ;
}


local int
las_print_et(lasp, i, j, k) 
LAS * lasp;
int i,j,k;
{
	struct las_packet * pp;

#ifdef	COMMENT
	/*for(i=0; i< szof(las_command); ++i)
		for(j=0; j< szof(las_command); ++j)
			for(k=0; k< szof(las_command); ++k) { */
#endif /* COMMENT */

	pp = &lasp->c.event_table[i][j][k];
	printf("event table valid=%d\n", pp->valid);

	return 0 ;
}


void 
gdb(void) {
	printf("in gdb\n");
}


local int las_checkdst3(lasp,pip)
LAS		*lasp ;
struct proginfo	*pip ;
{
	int	rs ;
	int	i, j ;


	rs = SR_OK ;
	for (i = 0 ; i < NCOMMANDS ; i += 1) {

	    for (j = 0 ; j < NBUSES ; j += 1) {

		if (lasp->c.event_table[i][j][DST3].valid) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_checkdst3: current state is non-zero !\n") ;
#endif

			rs = SR_BADFMT ;
			break ;
		}

		if (lasp->n.event_table[i][j][DST3].valid) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("las_checkdst3: next state is non-zero !\n") ;
#endif

			rs = SR_BADFMT ;
			break ;
		}

	    }

	    if (rs < 0)
		break ;

	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	if (rs < 0)
		eprintf("las_checkdst3: non-zero !\n") ;
	}
#endif

	return rs ;
}
/* end subroutine (las_checkdst3) */


local int las_loadalireg(lasp,rp)
LAS		*lasp ;
struct las_reg	*rp ;
{
	struct proginfo	*pip ;


	pip = lasp->pip ;

	rp->val = 0 ;
	if ((rp->a > 0) && (rp->a != (~ 0))) {

		alirgf_getval(lasp->rgfp,rp->a,&rp->val) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
		eprintf("las_loadalireg: asid=%d a=%08x dv=%08x\n",
			lasp->asid,rp->a,rp->val) ;
#endif

	}

	return 0 ;
}
/* end subroutine (las_loadalireg) */


