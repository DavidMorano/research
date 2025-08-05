/** ssas */

/* Levo Active Station */


#define	F_DEBUGS	0		/* non-switchable debugging */
#define	F_DEBUG		0		/* switchable debugging */
#define	F_SAFE		1		/* safe mode */
#define	F_OPTIONAL	1		/* do optional stuff (safety) ? */
#define	F_INSTREXEC	1		/* execute instructions */
#define	F_VALIDONLOAD	0		/* operand valid on load */
#define	F_LATECOMMIT	1		/* commit late */
#define	F_FAKELOADSTORE	1		/* do fake load/stores */


/* revision history:

	= 00/02/04, Dave Morano

	Module was originally written for the LEVO simulator LEVOSIM.


	= 03/04/17, Dave Morano

	I wiped most all of the stuff that was here both before and
	after Alireza (Khalafi) took over the code.  There are no
	buses and I completely abandoned the "event packet" scheme
	that Alireza had (it was a huge memory hog also and bore
	no resemblance to any actual hardware implementaion either).

	See below for the new strategy of how this who code executes.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/**************************************************************************

	This module provides the functions for a Levo Active Station.


	Phase 0:

	We do any private initialization for the current clock period.
	No external module will try to give us any stimulus in this
	phase.


	Phase 1:

	+ We may be asked whether we are ready to commit.  The
	commitment decision is based on current state.

	+ We may get loaded with an instruction and optional operands.

	+ All ASes exchange their current operands.


	Phase 2:

	+ We calculate whether we need to re-execute anything.  The
	execution will occur in the next clock if one is needed.


	Phase 3:

	+ We request any operands that were not already satisfied above.


	Phase 4:

	+ We handle any shifting that we might have been directed to do.



**************************************************************************/


#define	SSAS_MASTER	1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"ss.h"
#include	"xmlinfo.h"

#include	"instrclass.h"
#include	"sscommon.h"
#include	"ssinfo.h"
#include	"machstate.h"
#include	"ssas.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	SSAS_MAGIC	0x09089887
#define	SSAS_DISPLAYTT	-9999999

#ifndef	RFTYPE_RELAY
#define	RFTYPE_RELAY	0
#define	RFTYPE_NULLIFY	1
#endif

#define	INSTRDISLEN	100



/* external subroutines */

extern int	ffbsi(uint) ;
extern int	getinterleave(uint,uint) ;
extern int	getnumbuses(uint) ;
extern int	seqok(uint,uint,uint) ;


/* forward references */

static int	ssas_opinit(SSAS *,int) ;
static int	ssas_opchanged(SSAS *,int) ;
static int	ssas_checkexec(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_snoopset(SSAS *,int,struct operand *) ;
static int	ssas_handleshift(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_handlecommit(SSAS *) ;
static int	ssas_checkops(SSAS *,struct proginfo *,SS *,struct ssinfo *) ;
static int	ssas_combs(SSAS *,struct proginfo *,struct ssinfo *) ;

#ifdef	COMMENT
static int	ssas_xmloutreg(SSAS *,XMLINFO *,struct proginfo *,
			struct ssas_reg *,char *) ;
#endif /* COMMENT */

#if	(CF_MASTERDEBUG && F_DEBUG) || F_XML
static int	mkttbuf(char *,int) ;
#endif


/* initialize this SSAS object */

/**** with :

	- object pointer
	- 'proginfo'
	- SS pointer
	- 'ssinfo'
	- our unique AS identification number
	- physical column index
	- physical SG index

****/


int ssas_init(lasp,pip,mip,lip,ap)
SSAS		*lasp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
SSAS_INITARGS	*ap ;
{
	int	rs = SR_OK, i, j, k ;
	int	n, size ;
	int	npred ;


	if ((lasp == NULL) || (ap == NULL))
	    return SR_FAULT ;

	(void) memset(lasp,0,sizeof(SSAS)) ;

	lasp->magic = 0 ;
	(void) memset(&lasp->f,0,sizeof(struct ssas_flags)) ;

	lasp->pip = pip ;
	lasp->mip = mip ;
	lasp->lip = lip ;

/* load up any arguments given us */

	lasp->asid = ap->asid ;		/* our ID */
	lasp->wp = ap->wp ;
	lasp->pep = ap->pep ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("ssas_init: SSAS=%p asid=%d\n",lasp,ap->asid) ;
#endif

	lasp->rftype = MIN(ap->rftype,1) ;	/* register forwarding type */

/* some miscellaneous calculations for later */

	lasp->rfmodmask = (lip->rfmod - 1) ;
	lasp->rbmodmask = (lip->rbmod - 1) ;


/* zero all machine state */

	(void) memset(&lasp->c,0,sizeof(struct ssas_state)) ;

	(void) memset(&lasp->n,0,sizeof(struct ssas_state)) ;


#if	CF_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("ssas_init: ret rs=%d\n",rs) ;
#endif

	lasp->magic = SSAS_MAGIC ;
	return rs ;

/* bad things */
bad0:
	return rs ;
}
/* end subroutine (ssas_init) */


int ssas_stats(lasp,sp)
SSAS		*lasp ;
SSAS_STATS	*sp ;
{
	struct proginfo	*pip ;


	if (lasp == NULL)
		return SR_FAULT ;

#if	CF_MASTERDEBUG && F_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_free: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;

	if (sp == NULL)
		return SR_FAULT ;

	*sp = lasp->s ;
	return 0 ;
}
/* end subroutine (ssas_stats) */


/* free up the object */
int ssas_free(lasp)
SSAS		*lasp ;
{
	struct proginfo		*pip ;

	int	rs = SR_OK ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && F_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_free: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	    eprintf("ssas_free: SSAS(%p) asid=%d\n",
	        lasp,lasp->asid) ;
	}
#endif


	lasp->magic = 0 ;
	return rs ;
}
/* end subroutine (ssas_free) */


/* perform our combinatorial logic */
int ssas_comb(lasp,phase)
SSAS	*lasp ;
int	phase ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	ULONG	clock ;

	int	rs = SR_OK, i ;
	int	rs1 ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && F_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_comb: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	mip = lasp->mip ;
	lip = lasp->lip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_comb: asid=%u ck=%lld:%u tt=%d\n",
	        lasp->asid,mip->ck,phase,lasp->c.tt) ;
#endif


/* do our own combinatorial work */

	switch (phase) {

	case 0:
	    lasp->f.retire = FALSE ;
	    lasp->f.commit = FALSE ;
	    lasp->f.loaded = FALSE ;
	    lasp->f.shift = FALSE ;
	    lasp->f.needexec = FALSE ;

	    ssas_opinit(lasp,0) ;

	    ssas_opinit(lasp,1) ;

	    break ;

/* we snoop our operands here */
	case 1:
	    break ;

/* execute all our combinational parts */
	case 2:
	    ssas_opchanged(lasp,1) ;		/* read (input) set */

	    if (lasp->c.f.v && (lasp->c.ia != 0)) {

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_comb: ssas_checkexec() ck=%llu:%u tt=%d\n",
	                mip->ck,phase,lasp->c.tt) ;
#endif

		if (! (lasp->n.flags & F_MEM))
	        rs = ssas_checkexec(lasp,pip,lip) ;

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_comb: ssas_checkexec() rs=%d\n",rs) ;
#endif

	    } /* end if (checked if execution needed) */

	    ssas_opchanged(lasp,0) ;	/* write (output) set */

	    break ;

	case 3:
		if (lasp->c.f.v)
	    rs = ssas_checkops(lasp,pip,mip,lip) ;

	    break ;

	case 4:
	    if (lasp->f.shift)
	        rs = ssas_handleshift(lasp,pip,lip) ;

	    break ;

	case 5:
	    ssas_handlecommit(lasp) ;

	    ssas_combs(lasp,pip,lip) ;

#ifdef	COMMENT
	    ssas_clearexports(lasp) ;
#endif

	    break ;

	} /* end switch */

#ifdef	COMMENT
	if (rs < 0)
	    return rs ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_comb: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssas_comb) */


/* preform a clocked state transition */
int ssas_clock(lasp)
SSAS	*lasp ;
{
	struct proginfo		*pip ;

	struct ssinfo		*lip ;

	struct ssas_reg		*regp ;

	int	rs ;
	int	ret ;
	int	i,j,k,ll ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_clock: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;


/* transition ourself */


/* careful here */

	lasp->c = lasp->n ;




/* execute all subobject clock methods */



	return SR_OK ;
}
/* end subroutine (ssas_clock) */


/* is this AS used ? */
int ssas_used(lasp)
SSAS	*lasp ;
{
	int	rs = SR_OK ;


#ifdef	COMMENT

	if (lasp->c.asstate == UNUSED)
	    return INSTRCLASS_UNUSED ;

	switch (lasp->c.opclass) {

	case INSTR_LOAD:
	    return INSTRCLASS_REG ;
	    break ;

	case INSTR_STORE:
	    return INSTRCLASS_STORE ;
	    break ;

	case INSTR_BREL:
	    return INSTRCLASS_CBR ;
	    break ;

/* branch indirect */
	case INSTR_BIND:
	    return INSTRCLASS_JMP ;
	    break ;

	case INSTR_ALU:
	    return INSTRCLASS_REG ;
	    break ;

	case INSTR_FPALU:
	    return INSTRCLASS_REG ;
	    break ;

	case INSTR_SPECIAL:
	    return INSTRCLASS_REG ;
	    break ;

	case INSTR_JREL:
	    return INSTRCLASS_JMP ;
	    break ;

	case INSTR_JIND:
	    return INSTRCLASS_JMP ;
	    break ;

	case INSTR_EXCEP:
	    return INSTRCLASS_CALL ;
	    break ;

	case INSTR_UNUSED:
	    return INSTRCLASS_UNUSED ;
	    break ;

	default:
	    rs = SR_NOTSUP ;

	} /* end switch */

#endif /* COMMENT */

	return rs ;
}
/* end subroutine (ssas_used) */


/* receive the combinatorial SHIFT signal */
int ssas_shift(lasp,amount)
SSAS	*lasp ;
int	amount ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_shift: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_shift: asid=%d c_tt=%d n_tt=%d shift=%d\n",
	        lasp->asid,lasp->c.tt,lasp->n.tt,amount) ;
#endif

/* "do" all of our subobjects */



/* finally "do" us */

	lasp->f.shift = TRUE ;
	lasp->shiftamount = amount ;

bad5:
bad4:
bad3:
bad2:
bad1:
	return (rs >= 0) ? lasp->c.f.v : rs ;
}
/* end subroutine (ssas_shift) */


/* get the instruction address from this AS */
int ssas_getia(lasp,iap)
SSAS	*lasp ;
ULONG	*iap ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_getia: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (iap == NULL)
	    return SR_FAULT ;

/* put something here ! */

	*iap = (lasp->c.f.v) ? lasp->c.ia : 0 ;

	rs = lasp->c.f.v ;

	return rs ;
}
/* end subroutine (ssas_getia) */


/* old subroutine to load an AS */
int ssas_load(lasp, cri,tt)
SSAS	*lasp ;
int	cri ;
int	tt ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	SSAS	*casp ;

	int	rs = SR_OK, rs1, i, j ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_getia: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	mip = lasp->mip ;
	lip = lasp->lip ;

	casp = mip->ms.cas + cri ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssas_load: SSAS(%p) asid=%d cri=%d tt=%d\n",
	        lasp,lasp->asid,cri,tt) ;

	    eprintf("ssas_load: loading from\n") ;

	    ss_asdump(mip,"ssas_load",casp) ;
	}
#endif /* F_DEBUG */

/* load it up with the new stuff */

	lasp->n.ia = casp->n.ia ;
	lasp->n.instrcode = casp->n.instrcode ;
	lasp->n.op = casp->n.op ;
	lasp->n.class = casp->n.class ;
	lasp->n.flags = casp->n.flags ;

	lasp->n.ta = casp->n.ta ;

/* copy over operands */

	lasp->n.nopsi = casp->n.nopsi ;
	lasp->n.nopso = casp->n.nopso ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	    char	ibuf[100] ;

	    eprintf("ssas_load: asid=%u tt=%d ia=%016llx instr=%08x\n",
	        lasp->asid,tt,lasp->n.ia,lasp->n.instrcode) ;
	    rs1 = md_disassemble(lasp->n.instrcode,lasp->n.ia,ibuf,99) ;

	    eprintf("ssas_load: instr> %w\n",ibuf,((rs1 > 0) ? rs1 : 0)) ;

	    eprintf("ssas_load: nopsi=%u nopso=%u\n",
	        lasp->n.nopsi,lasp->n.nopso) ;
	}
#endif /* F_DEBUG */

/* input operands */

	for (i = 0 ; i < casp->n.nopsi ; i += 1) {

	    lasp->n.iops[i] = casp->n.iops[i] ;
	    lasp->n.iops[i].f.used = TRUE ;
	    lasp->n.iops[i].tt = INT_MIN ;

#if	(! F_VALIDONLOAD)
		if (! pip->f.nomem)
	    lasp->n.iops[i].f.v = FALSE ;
#endif

	} /* end for */

	for (j = i ; j < MACHSTATE_MAXOPSI ; j += 1)
	    memset((lasp->n.iops + i),0,sizeof(struct operand)) ;

/* output operands */

	for (i = 0 ; i < casp->n.nopso ; i += 1) {

	    lasp->n.oops[i] = casp->n.oops[i] ;
	    lasp->n.oops[i].f.used = TRUE ;
	    lasp->n.oops[i].tt = INT_MIN ;

		if (! pip->f.nomem)
	    lasp->n.oops[i].f.v = FALSE ;

	} /* end for */

	for (j = i ; j < MACHSTATE_MAXOPSO ; j += 1)
	    memset((lasp->n.oops + i),0,sizeof(struct operand)) ;

/* other */

	lasp->n.path = 0 ;		/* only one path for now */

/* special */

	lasp->n.tt = tt ;


/* comments */

#ifdef	COMMENT
	lasp->n.ia = ii.ia ;
	lasp->n.instrcode = ii.instrcode ;
	lasp->n.btype = ii.btype ;	/* Marcos-style i-fetch branch type */
	lasp->n.oopexec = ii.opexec ;
	lasp->n.oopclass = ii.opclass ;
#endif

/* state flags */

	lasp->n.f = casp->n.f ;

	lasp->n.f.enabled = TRUE ;
	lasp->n.f.export = FALSE ;

/* if it is LOAD/SOTRE say we executed ! */

#if	F_FAKELOADSTORE
	if (pip->f.nomem) {

		int	f ;


	f = (lasp->n.flags & F_MEM) ;

	if (! f)
	lasp->n.f.executed = FALSE ;

		lasp->s.nexec += 1 ;
	}
#endif /* F_FAKELOADSTORE */

/* continue */

	lasp->n.f.executing = FALSE ;

	lasp->n.nexec = 0 ;

/* regular flags */

#ifdef	COMMENT /* very bad ! */
	memset(&lasp->f,0,sizeof(struct ssas_flags)) ;
#endif

	lasp->f.loaded = TRUE ;


#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssas_load: loaded to\n") ;

	    ss_asdump(mip,"AS",lasp) ;

	}
#endif /* F_DEBUG */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssas_load: input operand summary\n") ;
	    for (i = 0 ; i < lasp->n.nopsi ; i += 1) {

	        eprintf("ssas_load: op=%u a=%016llx used=%u\n",
	            i,
	            lasp->n.iops[i].a,
	            lasp->n.iops[i].f.used) ;

	    }
	}
#endif /* F_DEBUG */

	return rs ;
}
/* end subroutine (ssas_load) */


/* request an operand by associative match (this is like a reverse snoop) */
int ssas_opmatch(lasp,rop)
SSAS		*lasp ;
OPERAND		*rop ;
{
	OPERAND	*setp, *oop ;
	OPERAND	*nsetp ;

	int	rs = SR_OK, i ;
	int	wset ;
	int	n ;
	int	f_gotone = FALSE ;


	if (! lasp->c.f.v)
	    return SR_EMPTY ;

	if (! lasp->c.f.export)
	    return SR_EMPTY ;

	for (wset = 0 ; wset < 2 ; wset += 1) {

	    setp = (wset) ? lasp->c.iops : lasp->c.oops ;
	    nsetp = (wset) ? lasp->n.iops : lasp->n.oops ;
	    n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	    for (i = 0 ; i < n ; i += 1) {

	        oop = setp + i ;
	        if (oop->f.used && oop->f.v &&
	            (oop->type == rop->type) && 
	            (oop->a == rop->a) && 
	            (oop->tt < rop->tt)) {

	            nsetp[i].f.export = TRUE ;
	            f_gotone = TRUE ;
	            break ;

	        } /* end if (need this operand) */

	    } /* end for (operands within a set) */

	    if (f_gotone)
	        break ;

	} /* end for (which set) */

	if (f_gotone)
	    lasp->n.f.export = TRUE ;

	return f_gotone ;
}
/* end subroutine (ssas_opmatch) */


/* get an EXPORTABLE operand by index out of this AS */
int ssas_opexport(lasp,oi,opp)
SSAS		*lasp ;
int		oi ;
OPERAND		**opp ;
{
	OPERAND	*oop ;

	int	f ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_opexport: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

/* if the AS isn't valid => it doesn't have any exportable operands ! */

	if (! lasp->c.f.v)
	    return SR_EMPTY ;

/* if we do not have any exportable operands => we do not have any ! */

	if (! lasp->c.f.export)
	    return SR_EMPTY ;

#ifdef	OPTIONAL

	if (oi >= lasp->c.noops)
	    return SR_EMPTY ;

#else

	if (oi >= MACHSTATE_MAXOPSO)
	    return SR_INVALID ;

#endif /* OPTIONAL */

	oop = lasp->c.oops + oi ;
	f = oop->f.used && oop->f.v && oop->f.export ;

	if (f) {
	    *opp = oop ;
	    lasp->n.oops[oi].clr.export = TRUE ;
	}

	return f ;
}
/* end subroutine (ssas_opexport) */


/* get an operand by index out of this AS */
int ssas_opget(lasp,oi,opp)
SSAS		*lasp ;
int		oi ;
OPERAND		**opp ;
{
	OPERAND	*oop ;

	int	f ;


/* if we are not valid => then we do not have any operands ! */

	if (! lasp->c.f.v)
	    return SR_EMPTY ;

#ifdef	COMMENT
	if (! lasp->c.f.export)
	    return SR_OK ;
#endif

#ifdef	COMMENT
	if (oi >= lasp->c.noops)
	    return SR_EMPTY ;
#endif

	if (oi >= MACHSTATE_MAXOPSO)
	    return SR_INVALID ;

	oop = lasp->c.oops + oi ;
	f = oop->f.used ;

	if (f)
	    *opp = oop ;

	return f ;
}
/* end subroutine (ssas_opget) */


/* snoop an operand that is "out there" */
int ssas_opsnoop(lasp,op)
SSAS		*lasp ;
OPERAND		*op ;
{
	struct proginfo	*pip ;

	int	rs, i ;
	int	c = 0 ;


	pip = lasp->pip ;

	rs = ssas_snoopset(lasp,0,op) ;

	if (rs > 0)
	    c += rs ;

	rs = ssas_snoopset(lasp,1,op) ;

	if (rs > 0)
	    c += rs ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_opsnoop: snooped=%u\n",c) ;
#endif

	return (rs >= 0) ? c : rs ;
}
/* end subroutine (ssas_opsnoop) */


/* are we ready to retire this instruction ? */
int ssas_readyretire(lasp)
SSAS	*lasp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	struct ssas_sflags	*fp ;

	int	rs, f_retire = FALSE ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_readyretire: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

#if	(CF_MASTERDEBUG && F_DEBUG) || F_DEBUGS
	if (DEBUGLEVEL(4)) {
	    fp = &lasp->c.f ;
	    eprintf("ssas_readyretire: asid=%u ia=%16llx tt=%d"
	        " v=%u e=%u ex=%u exg=%u exp=%u\n",
	        lasp->asid,
		lasp->c.ia,lasp->c.tt,fp->v,fp->enabled,
	        fp->executed,fp->executing,fp->export) ;
	}
#endif

/* if we are unused, then we can't very well retire ! */

	if (! lasp->c.f.v)
	    return FALSE ;

/* we are ready to retire if we are not enabled ! */

	if (! lasp->c.f.enabled) {

#if	(CF_MASTERDEBUG && F_DEBUG) || F_DEBUGS
	if (DEBUGLEVEL(4))
	    eprintf("ssas_readyretire: not enabled\n") ;
#endif

	    return TRUE ;
	}

/* we are ready to commit if we have executed at least once */

	if ((! lasp->c.f.executed) || lasp->c.f.executing)
	    return FALSE ;

/* we are also not ready if some output operands needed to be exported */

#if	F_LATECOMMIT
	if (lasp->c.f.export)
	    return FALSE ;
#endif

#if	(CF_MASTERDEBUG && F_DEBUG) || F_DEBUGS
	if (DEBUGLEVEL(4))
	    eprintf("ssas_readyretire: READY\n") ;
#endif

	return TRUE ;
}
/* end subroutine (ssas_readyretire) */


/* are we ready to be loaded ? */
int ssas_readyload(lasp)
SSAS	*lasp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, f ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_readyload: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

/* if we are unused or about to retire, then yes */

	f = ((! lasp->c.f.v) || lasp->f.retire) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_readyload: asid=%d c_v=%u retire=%u f=%u\n",
	        lasp->asid,lasp->c.f.v,lasp->f.retire,f) ;
#endif /* F_DEBUG */

	return (rs >= 0) ? f : rs ;
}
/* end subroutine (ssas_readyload) */


/* return our commitment information */
int ssas_info(lasp,ip)
SSAS		*lasp ;
SSAS_INFO	*ip ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;
	int	i ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_info: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lasp->pip ;
	if (ip == NULL)
	    return SR_FAULT ;

	(void) memset(ip,0,sizeof(struct ssas_info)) ;

	if (! lasp->c.f.v)
	    return SR_EMPTY ;

/* get the instruction class used by Dave in the execution window */

	ip->ia = lasp->c.ia ;
	ip->ta = lasp->c.ta ;

	ip->instrcode = lasp->c.instrcode ;
	ip->op = lasp->c.op ;
	ip->class = lasp->c.class ;
	ip->flags = lasp->c.flags ;

	ip->f_enabled = lasp->c.f.enabled ;
	ip->f_cf = lasp->c.f.cf ;
	ip->f_cfind = lasp->c.f.cfind ;
	ip->f_cfsub = lasp->c.f.cfsub ;
	ip->f_cfcond = lasp->c.f.cfcond ;
	ip->f_taken = lasp->c.f.taken ;

	rs = lasp->c.f.v && lasp->c.f.enabled ;

	return rs ;
}
/* end subroutine (ssas_info) */


/* we are being told to commit our instruction */
int ssas_retire(lasp)
SSAS	*lasp ;
{
	int	rs = SR_OK ;
	int	i ;


#if	CF_MASTERDEBUG && F_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_retire: SSAS=%08lx bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

/* entered */

	lasp->f.retire = TRUE ;


/* get out */

	lasp->f.commit = lasp->c.f.v && lasp->c.f.enabled ;
	rs = lasp->f.commit ;

	return rs ;
}
/* end subroutine (ssas_retire) */


#ifdef	COMMENT

/* XML stuff */
int ssas_xmlinit(fp,xip)
SSAS	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}


int ssas_xmlout(lasp,xip)
SSAS	*lasp ;
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
	    (lasp->c.opclass != OPCSSASS_UNUSED)) ;

	if (lasp->c.opclass != OPCSSASS_UNUSED) {

	    char	instrdis[INSTRDISLEN + 1] ;


	    bprintf(&xip->xmlfile,"<ia>%08x</ia>\n",lasp->c.ia) ;

	    bprintf(&xip->xmlfile,"<instr>%08x</instr>\n",lasp->c.instrcode) ;

	    rs = mipsdis_getlevo(&pip->dis,lasp->c.ia,lasp->c.instrcode,
	        instrdis,INSTRDISLEN) ;

	    if (rs < 0)
	        strcpy(instrdis,"unknown") ;

#if	CF_MASTERDEBUG && F_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssas_xmlout: ia=%08x instr=%08x %s\n",
	            lasp->c.ia,lasp->c.instrcode,instrdis) ;
#endif /* F_DEBUG */

	    bprintf(&xip->xmlfile,"<instrdis>%s</instrdis>\n",
	        instrdis) ;

	    bprintf(&xip->xmlfile,"<opclass>%d</opclass>\n",lasp->c.opclass) ;

	    bprintf(&xip->xmlfile,"<opexec>%d</opexec>\n",lasp->c.opexec) ;

/* what about some register sources ? */

	    if (lasp->c.src1.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.src1,"src1") ;

	    if (lasp->c.src2.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.src2,"src2") ;

	    if (lasp->c.src3.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.src3,"src3") ;

	    if (lasp->c.src4.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.src4,"src4") ;

	    if (lasp->c.src5.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.src5,"src5") ;

	    if (lasp->c.dst1.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.dst1,"dst1") ;

	    if (lasp->c.dst2.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.dst2,"dst2") ;

	    if (lasp->c.dst3.a != (~ 0))
	        ssas_xmloutreg(lasp,xip,pip,&lasp->c.dst3,"dst3") ;

	} /* end if (we had some instruction) */

	bprintf(&xip->xmlfile,"</as>\n") ;

	return SR_OK ;
}
/* end subroutine (ssas_xmlout) */


int ssas_xmloutreg(lasp,xip,pip,rp,name)
SSAS		*lasp ;
XMLINFO		*xip ;
struct proginfo	*pip ;
struct ssas_reg	*rp ;
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
/* end subroutine (ssas_xmloutreg) */


int ssas_xmlfree(fp,xip)
SSAS	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}
/* end subroutine (ssas_xmlfree) */

#endif /* COMMENT */



/* PRIVATE SUBROUTINES */



/* snoop a set of operands (input or output) */

/*
	wset == 0		output set (write)
	wset == 1		input set (read)

*/

static int ssas_snoopset(lasp,wset,rop)
SSAS		*lasp ;
int		wset ;
OPERAND		*rop ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	struct operand	*setp, *oop ;

	int	rs, i ;
	int	n, c = 0 ;
	int	f_needexec = FALSE ;
	int	f ;


	pip = lasp->pip ;
	lip = lasp->lip ;

	setp = (wset) ? lasp->n.iops : lasp->n.oops ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_snoopset: asid=%u snooping %s operands\n",
	        lasp->asid,((wset) ? "input" : "output")) ;
#endif

	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;

#if	CF_MASTERDEBUG && F_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssas_snoopset: op=%u a=%016llx used=%u\n",
	            i,oop->a,oop->f.used) ;
#endif

	    if (oop->f.used) {

	        f = TRUE ;
	        f = f && (rop->type == oop->type) ;

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: type %u\n",f) ;
#endif

	        f = f && (rop->path == oop->path) ;

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: path %u\n",f) ;
#endif

	        f = f && (rop->a == oop->a) ;

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: address %u\n",f) ;
#endif

	        f = f && (rop->tt < lasp->n.tt) ;	/* next TT ! */

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: our tt %u\n",f) ;
#endif

	        if (rop->trans != OPERAND_TNULLIFY)
	            f = f && ((! oop->f.v) || (rop->tt >= oop->tt)) ;

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: last tt %u\n",f) ;
#endif

/* sequence number check */

	        if (f) {

	            if (rop->tt == oop->tt) {

	                if (seqok(rop->seq,oop->frseq,lip->rfmod))
	                    oop->frseq = (rop->seq + 1) & lasp->rfmodmask ;

	                else
	                    f = FALSE ;

	            } else
	                oop->frseq = (rop->seq + 1) & lasp->rfmodmask ;

	        } /* end if (sequence check) */

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: sequence %u\n",f) ;
#endif

	        if (f) {

#if	CF_MASTERDEBUG && F_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("ssas_snoopset: asid=%u tt=%d "
	                    "snarfed a=%016llx v=%016llx\n",
	                    lasp->asid,lasp->c.tt,rop->a,rop->v) ;
#endif

	            oop->tt = rop->tt ;
	            if ((! oop->f.v) || (rop->v != oop->pv)) {

	                if (! oop->f.v)
	                    oop->set.changed = TRUE ;

	                c += 1 ;
	                oop->pv = rop->v ;
	                oop->f.v = TRUE ;

#if	CF_MASTERDEBUG && F_DEBUG && 0
	                if (DEBUGLEVEL(4))
	                    eprintf("ssas_snoopset: asid=%u tt=%d "
				"snarfed v=%016llx\n",
	                        lasp->asid,lasp->c.tt,rop->v) ;
#endif

#ifdef	COMMENT
	                if (wset == 1) && (rop->v != oop->ov)) {

	                    oop->f.changed = TRUE ;
	                    oop->set.changed = TRUE ;

#if	CF_MASTERDEBUG && F_DEBUG
	                    if (DEBUGLEVEL(4))
	                        eprintf("ssas_snoopset: operand changed\n") ;
#endif

#ifdef	COMMENT
	                    lasp->f.needexec = TRUE ;
	                    f_needexec = TRUE ;
#endif

	                } /* end if (operand changed) */

#endif /* COMMENT (handled elsewhere) */

	            } /* end if (snarfed operand) */

	        } /* end if (got an operand match) */

	    } /* end if (operand was used) */

	} /* end for (looping over resident operands) */

	return c ;
}
/* end subroutine (ssas_snoopset) */


/* initialize next-state that is associated with the operands */
static int ssas_opinit(lasp,wset)
SSAS		*lasp ;
int		wset ;
{
	OPERAND	*setp, *oop ;

	int	rs, i ;
	int	n, c = 0 ;


	setp = (wset) ? lasp->n.iops : lasp->n.oops ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;

/* set default */

	    memset(&oop->set,0,sizeof(struct operand_flags)) ;

	    memset(&oop->clr,0,sizeof(struct operand_flags)) ;

/* now set what we want */

	    oop->clr.changed = TRUE ;	/* gets cleared unless set */

/* "old" value */

	    oop->opv = oop->pv ;
	    oop->ov = oop->v ;

	} /* end for */

	return c ;
}
/* end subroutine (ssas_opinit) */


/* calculate if an operand has changed */
static int ssas_opchanged(lasp,wset)
SSAS		*lasp ;
int		wset ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	OPERAND	*setp, *oop, *cop ;

	ULONG	nv ;

	int	rs, i ;
	int	n, c ;


	pip = lasp->pip ;
	lip = lasp->lip ;

	if (! lasp->c.f.v)
	    return SR_OK ;

/* the "previous" values (for both input and output operands) */

	setp = (wset) ? lasp->n.iops : lasp->n.oops ;
	cop = (wset) ? lasp->c.iops : lasp->c.oops ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	c = 0 ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;

	    if (oop->pv != oop->opv) {

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssas_opchanged: asid=%u iop=%u changed\n",
	                lasp->asid,i) ;
#endif

	        oop->f.changed = TRUE ;
	        oop->set.changed = TRUE ;
	        oop->seq = (cop->seq + 1) & lip->rfmodmask ;
	        c += 1 ;

	    } /* end if (value changed) */

	    cop += 1 ;

	} /* end for */

/* if an input operand changed, then we need a re-execution */

	if ((wset == 1) && (c > 0)) {

		if (! (lasp->n.flags & F_MEM))
	    lasp->f.needexec = TRUE ;

	}

/* our destination values (only if the output set was specified) */

	if (wset == 0) {

	    setp = (wset) ? lasp->n.iops : lasp->n.oops ;
	    cop = (wset) ? lasp->c.iops : lasp->c.oops ;
	    n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	    c = 0 ;
	    for (i = 0 ; i < n ; i += 1) {

	        oop = setp + i ;

	        if (oop->v != oop->ov) {

#if	CF_MASTERDEBUG && F_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("ssas_opchanged: asid=%u oop=%u changed\n",
	                    lasp->asid,i) ;
#endif

	            oop->f.export = TRUE ;
	            oop->set.export = TRUE ;
	            oop->seq = (cop->seq + 1) & lip->rfmodmask ;
	            c += 1 ;

	        } /* end if (value changed) */

	        cop += 1 ;

	    } /* end for */

/* if an output operand changed, mark it to be exported */

	    if (c > 0)
	        lasp->n.f.export = TRUE ;

	} /* end if (destination values) */

	return c ;
}
/* end subroutine (ssas_opchanged) */


/* check if we need to request an operand from program-ordered-past */
static int ssas_checkops(lasp,pip,mip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
{
	OPERAND	ro, *setp, *oop ;

	int	rs = SR_OK, i ;
	int	wset ;
	int	n ;


#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkops: entered asid=%u tt=%d\n",
	        lasp->asid,lasp->c.tt) ;
#endif

	for (wset = 0 ; wset < 2 ; wset += 1) {

#if	CF_MASTERDEBUG && F_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssas_checkops: checking %s ops\n",
	            ((wset) ? "input" : "output")) ;
#endif

	    setp = (wset) ? lasp->n.iops : lasp->n.oops ;
	    n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	    for (i = 0 ; i < n ; i += 1) {

	        oop = setp + i ;

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_checkops: checking op=%u used=%u a=%016llx\n",
	                i,oop->f.used,oop->a) ;
#endif

	        if (setp[i].f.used && (! setp[i].f.v) &&
	            (! setp[i].f.requested)) {

#if	CF_MASTERDEBUG && F_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssas_checkops: we need op=%u\n",i) ;
#endif

/* prepare the flow-group for a request (set our TT) */

	            ro = setp[i] ;
	            ro.tt = lasp->c.tt ;

/* make the request */

	            rs = ssiw_oprequest(lasp->wp,&ro,lasp->asid) ;

#if	CF_MASTERDEBUG && F_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssas_checkops: ssiw_oprequest() rs=%d\n",rs) ;
#endif

	            if (rs < 0)
	                break ;

	            setp[i].f.requested = TRUE ;

	        } /* end if (need this operand) */

	    } /* end for (operands within a set) */

	} /* end for (which set) */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkops: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssas_checkops) */


/* check to see if we need to execute */
static int ssas_checkexec(lasp,pip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	SS	*mip ;

	OPERAND	*oop ;

	uint	latency ;

	int	rs = SR_OK ;
	int	i, n ;


	mip = lasp->mip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkexec: entered ia=%016llx\n",
	        lasp->c.ia) ;
#endif /* F_DEBUG */

/* do we even have an instruction ? */

	if (lasp->c.ia == 0)
	    return SR_OK ;

/* are we currently executing ? */

	if (lasp->c.f.executing) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkexec: executing exectimer=%u\n",
			lasp->n.exectimer) ;
#endif

/* are done executing ? */

		if (lasp->c.exectimer == 1) {

		lasp->n.exectimer = 0 ;
		lasp->n.f.executing = FALSE ;
		lasp->n.f.executed = TRUE ;
		lasp->n.nexec += 1 ;		/* statistics */

		lasp->s.nexec += 1 ;

		} else
			lasp->n.exectimer -= 1 ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkexec: exg=%u e=%u exectimer=%u\n",
			lasp->n.f.executing,
			lasp->n.f.executed,
		lasp->n.exectimer) ;
#endif

		return SR_OK ;

	} /* end if (currently executing) */

/* do we need execution ? */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkexec: needexec=%u c_executed=%u\n",
	        lasp->f.needexec,lasp->c.f.executed) ;
#endif /* F_DEBUG */

	if (lasp->c.f.executed && (! lasp->f.needexec))
	    return SR_OK ;

/* are all of the operands that we need valid ? */

	n = lasp->c.nopsi ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = lasp->c.iops + i ;
	    if (oop->f.used) {

#if	CF_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_checkexec: op=%u v=%u\n",
	                i,oop->f.v) ;
#endif /* F_DEBUG */

	        if (! oop->f.v)
	            break ;

	    }

	} /* end for */

	if (i < n) {

#if	CF_MASTERDEBUG && F_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssas_checkexec: op=%u not valid\n",i) ;
#endif /* F_DEBUG */

	    return SR_OK ;

	}

/* OK, get a FU resource based on our instruction class */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssas_checkexec: class=%u\n",lasp->c.class) ;
#endif /* F_DEBUG */

	rs = sspe_getfu(lasp->pep,lasp->c.class,&latency) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssas_checkexec: sspe_getfu() rs=%d latency=%u\n",
		rs,latency) ;
#endif /* F_DEBUG */

	if (rs <= 0)
	    return SR_OK ;

/* OK, do the execution */

#if	F_INSTREXEC

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkexec: ss_instrexec()\n") ;
#endif /* F_DEBUG */

	rs = ss_instrexec(mip,lasp) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssas_checkexec: asid=%u tt=%d ss_instrexec() rs=%d\n",
	        lasp->asid,lasp->c.tt,rs) ;
#endif /* F_DEBUG */

#endif /* F_INSTREXEC */

	if (lasp->c.f.executed)
		lasp->s.nrexec += 1 ;

/* do some things that are AS specific */

	if (latency > 1) {

		lasp->n.f.executing = TRUE ;
		lasp->n.exectimer = (latency - 1) ;

	} else {

		lasp->n.f.executed = TRUE ;

#ifdef	COMMENT
		lasp->n.nexec += 1 ;		/* statistics */
#endif

	}

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkexec: ret rs=%d\n",rs) ;
#endif /* F_DEBUG */

	return rs ;
}
/* end subroutine (ssas_checkexec) */


/* clean something to do with exported stuff (?) */
static int ssas_clearexports(lasp)
SSAS		*lasp ;
{
	struct ssinfo	*lip ;

	struct operand	*setp, *oop ;

	int	rs, i ;
	int	wset = 0 ;
	int	n, c = 0 ;
	int	f ;


	lip = lasp->lip ;

	setp = (wset) ? lasp->n.iops : lasp->n.oops ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;
	    if (oop->f.v) {

	        oop->f.export = FALSE ;

	    }

	} /* end for (looping over resident operands) */

	return c ;
}
/* end subroutine (ssas_clearexports) */


/* handle a machine shift operation ! */
static int ssas_handleshift(lasp,pip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	struct operand	*setp, *oop ;

	int	i, j, k ;
	int	n ;
	int	tt_test ;


	if (! lasp->f.shift)
	    return SR_OK ;

	lasp->f.shift = FALSE ;
	tt_test = INT_MIN + lasp->shiftamount ;


/* do the most obvious stuff */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_handleshift: asid=%d shift=%d c_tt=%d n_tt=%d\n",
	        lasp->asid,lasp->shiftamount,lasp->c.tt,lasp->n.tt) ;
#endif /* F_DEBUG */

	lasp->n.tt -= lasp->shiftamount ;


/* all of the sources and destinations */

	n = MACHSTATE_MAXOPSI ;
	setp = lasp->n.iops ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;
	    if (oop->f.used) {

	        if (oop->tt >= tt_test)
	            oop->tt -= lasp->shiftamount ;

	        else
	            oop->tt = INT_MIN ;

	    }

	} /* end for */

	n = MACHSTATE_MAXOPSO ;
	setp = lasp->n.oops ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;
	    if (oop->f.used) {

	        if (oop->tt >= tt_test)
	            oop->tt -= lasp->shiftamount ;

	        else
	            oop->tt = INT_MIN ;

	    }

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssas_handleshift) */


static int ssas_combs(lasp,pip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	OPERAND	*setp, *oop ;

	int	wset, n, i, j ;


	lasp->n.f.export = FALSE ;
	for (wset = 0 ; wset < 2 ; wset += 1) {

	    setp = (wset) ? lasp->n.iops : lasp->n.oops ;
	    n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	    for (j = 0 ; j < n ; j += 1) {

	        oop = setp + j ;

	        if (oop->set.v) oop->f.v = TRUE ;
	        else if (oop->clr.v) oop->f.v = FALSE ;

	        if (oop->set.requested) oop->f.requested = TRUE ;
	        else if (oop->clr.requested) oop->f.requested = FALSE ;

	        if (oop->set.changed) oop->f.changed = TRUE ;
	        else if (oop->clr.changed) oop->f.changed = FALSE ;

	        if (oop->set.export) oop->f.export = TRUE ;
	        else if (oop->clr.export) oop->f.export = FALSE ;

	        if (oop->set.nullify) oop->f.nullify = TRUE ;
	        else if (oop->clr.nullify) oop->f.nullify = FALSE ;

	        if (oop->set.fault) oop->f.fault = TRUE ;
	        else if (oop->clr.fault) oop->f.fault = FALSE ;

/* consequences */

	        if ((wset == 0) && oop->f.export)
	            lasp->n.f.export = TRUE ;

	    } /* end for */

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssas_combs) */


static int ssas_handlecommit(lasp)
SSAS	*lasp ;
{
	struct operand	*setp, *oop ;

	int	rs, n, i ;
	int	f ;


	if (lasp->f.loaded)
	    lasp->n.f.v = TRUE ;

#if	OPTIONAL
	if ((! lasp->f.loaded) && lasp->f.commit)
	    lasp->n.f.v = FALSE ;
#endif

	if (lasp->f.loaded || (! lasp->f.commit))
	    return SR_OK ;

	lasp->n.f.v = FALSE ;

/* clear the used bits in any operands */

	n = MACHSTATE_MAXOPSI ;
	setp = lasp->n.iops ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;
	    if (oop->f.used)
	        oop->f.used = FALSE ;

	} /* end for */

	n = MACHSTATE_MAXOPSO ;
	setp = lasp->n.oops ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;
	    if (oop->f.used)
	        oop->f.used = FALSE ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssas_handlecommit) */


#if	(CF_MASTERDEBUG && F_DEBUG) || F_XML

static int mkttbuf(buf,tt)
char	buf[] ;
int	tt ;
{
	int	rs ;


	if (tt < SSAS_DISPLAYTT) {

	    buf[0] = '-' ;
	    buf[1] = '\0' ;
	    rs = 2 ;

	} else
	    rs = ctdeci(buf,-1,tt) ;

	return rs ;
}

#endif /* F_DEBUG */



