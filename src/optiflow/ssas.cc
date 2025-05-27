/* ssas */

/* Levo Active Station */


#define	CF_DEBUGS	0		/* non-switchable debugging */
#define	CF_DEBUG	0		/* switchable debugging */
#define	CF_DEBUGCOMBEND	0		/* combend() */
#define	CF_DEBUGETIMER	0		/* exectimer */
#define	CF_DEBUGSPECIAL	0		/* special */
#define	CF_SAFE		0		/* safe mode */
#define	CF_INSTREXEC	1		/* execute instructions */
#define	CF_VALIDONLOAD	0		/* operand valid on load */
#define	CF_LATECOMMIT	1		/* commit late */
#define	CF_SIMPLEMEM	1		/* do fake load/stores */
#define	CF_DCACHE	1		/* use D-cache */
#define	CF_EXECTIMEROBJ	0		/* use EXECTIMER object */
#define	CF_ISRES	1		/* track IS residency */


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

	+ All executions are done (initiated by the SSPE module).
	+ We request any operands that were not already satisfied above.


	Phase 4:

	+ we get loaded here !
	+ We handle any shifting that we might have been directed to do.


**************************************************************************/


#define	SSAS_MASTER	1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<bio.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"ss.h"
#include	"xmlinfo.h"

#include	"instrclass.h"
#include	"sscommon.h"
#include	"ssinfo.h"
#include	"checker.h"
#include	"sspe.h"
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
extern int	dcache_latency(struct proginfo *,SS *,
			CHECKER *,ULONG,int) ;


/* forward references */

static int	ssas_opinit(SSAS *,int) ;
static int	ssas_opchanged(SSAS *,struct proginfo *,int) ;
static int	ssas_checkexec(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_execrequest(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_execlocal(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_execresult(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_snoopset(SSAS *,int,struct operand *) ;
static int	ssas_handleshift(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_handlecommit(SSAS *) ;
static int	ssas_checkops(SSAS *,struct proginfo *,SS *,struct ssinfo *) ;
static int	ssas_combstart(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_combend(SSAS *,struct proginfo *,struct ssinfo *) ;
static int	ssas_haveiops(SSAS *,struct proginfo *) ;
static int	ssas_dumpop(SSAS *,bfile *,int,OPERAND *) ;

#if	CF_EXECTIMEROBJ

static int	exectimer_init(SSAS *,struct proginfo *) ;
static int	exectimer_begin(SSAS *,struct proginfo *) ;
static int	exectimer_set(SSAS *,struct proginfo *,int) ;
static int	exectimer_clear(SSAS *,struct proginfo *) ;
static int	exectimer_check(SSAS *,struct proginfo *) ;
static int	exectimer_end(SSAS *,struct proginfo *) ;
static int	exectimer_free(SSAS *,struct proginfo *) ;

#endif /* CF_EXECTIMER */

#ifdef	COMMENT
static int	ssas_xmloutreg(SSAS *,XMLINFO *,struct proginfo *,
struct ssas_reg *,char *) ;
#endif /* COMMENT */

#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_XML
static int	mkttbuf(char *,int) ;
#endif


/* local variables */





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

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssas_init: SSAS=%p asid=%d\n",lasp,ap->asid) ;
#endif

	lasp->rftype = MIN(ap->rftype,1) ;	/* register forwarding type */

/* some miscellaneous calculations for later */

	lasp->rfmodmask = (lip->rfmod - 1) ;
	lasp->rbmodmask = (lip->rbmod - 1) ;


/* zero all machine state */

	(void) memset(&lasp->c,0,sizeof(struct ssas_state)) ;

	(void) memset(&lasp->n,0,sizeof(struct ssas_state)) ;

#if	CF_EXECTIMEROBJ
	exectimer_init(lasp,pip) ;
#endif


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
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

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_stats: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

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

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_free: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	    eprintf("ssas_free: SSAS(%p) asid=%d\n",
	        lasp,lasp->asid) ;
	}
#endif

#if	CF_EXECTIMEROBJ
	exectimer_free(lasp,pip) ;
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

	int	rs = SR_OK, rs1, i ;
	int	f ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_DEBUGSPECIAL
	eprintf("ssas_comb: entered ph=%u\n",phase) ;
#endif

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_comb: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	mip = lasp->mip ;
	lip = lasp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_comb: asid=%u ck=%lld:%u tt=%d\n",
	        lasp->asid,mip->ck,phase,lasp->c.tt) ;
#endif


/* do our own combinatorial work */

	switch (phase) {

	case 0:
	    ssas_combstart(lasp,pip,lip) ;

	    lasp->f.retire = FALSE ;
	    lasp->f.commit = FALSE ;
	    lasp->f.loaded = FALSE ;
	    lasp->f.shift = FALSE ;
	    lasp->f.opchanged = FALSE ;
	    lasp->f.needexec = FALSE ;

#if	CF_EXECTIMEROBJ
	    lasp->f.exectimer = FALSE ;
#endif

	    ssas_opinit(lasp,0) ;

	    ssas_opinit(lasp,1) ;

	    if (lasp->n.exectimer > 0)
		lasp->n.exectimer -= 1 ;

	    if (lasp->n.mincounter > 0) {

		lasp->n.mincounter -= 1 ;
		if ((lasp->n.mincounter == 0) && (lasp->c.flags & F_MEM)) {
			lasp->n.clr.executing = TRUE ;
			lasp->n.set.executed = TRUE ;
		}

	    }

#if	CF_EXECTIMEROBJ
	    exectimer_begin(lasp,pip) ;
#endif

	    if (lasp->c.f.v)
		lasp->s.vcount += 1 ;		/* "valid" count */

	    break ;

/* we snoop our operands here */
	case 1:
	    break ;

	case 2:
	    if (lasp->c.f.v && (lasp->c.ia != 0)) {

	        if (! lasp->n.f.haveiops)
	            ssas_haveiops(lasp,pip) ;

	        ssas_opchanged(lasp,pip,1) ;	/* read (input) set */

	        if (lasp->n.f.haveiops) {

		    f = FALSE ;
		    f = f || 
			    (! lasp->c.f.executed) && (! lasp->n.f.executing) ;
		    f = f || lasp->f.opchanged ;

	            lasp->f.needexec = f ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssas_comb: c.executed=%u opchanged=%u "
	                    "needexec=%u\n",
	                    lasp->c.f.executed,lasp->f.opchanged,
	                    lasp->f.needexec) ;
#endif

	        } /* end if (have input operands) */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_comb: v=%u ia=%016llx\n",
	                lasp->c.f.v,lasp->c.ia) ;
#endif

		if (lasp->f.needexec || lasp->c.f.needexec) {

			lasp->n.f.needexec = TRUE ;
			if (lasp->c.f.localexec) {

	        	if (lasp->n.flags & F_MEM) {

				if ((! lasp->c.f.executed) &&
					(! lasp->c.f.executing)) {

				lasp->n.clr.needexec = TRUE ;
				sspe_fusu(lasp->pep,lasp->c.class) ;

				if (lasp->c.mincounter > 1) {
				lasp->n.set.executing = TRUE ;
				} else {
				lasp->n.clr.executing = TRUE ;
				lasp->n.set.executed = TRUE ;
				}

				lasp->s.nmemexec += 1 ;
				lasp->s.ntexec += 1 ;

				} /* end if (not executing) */

			} else
	                	rs = ssas_execlocal(lasp,pip,lip) ;

			} else
	                	rs = ssas_execrequest(lasp,pip,lip) ;

	        } /* end if (needexec) */

	    } /* end if (valid and loaded) */

	    break ;

/* FUs return results in this phase */
	case 3:
	    break ;

	case 4:
	    if (lasp->c.f.v) {
		
		if (lasp->c.flags & F_MEM) {

		if (lasp->c.mincounter == 0)
	    		ssas_opchanged(lasp,pip,0) ;

		} else
	    		ssas_opchanged(lasp,pip,0) ;

	        rs = ssas_checkops(lasp,pip,mip,lip) ;

	    } /* end if (valid) */

	    break ;

	case 5:
	    if (lasp->f.commit)
	    	ssas_handlecommit(lasp) ;

#if	CF_EXECTIMEROBJ

	    if ((! lasp->f.loaded) && (! lasp->f.retire)) {

	        exectimer_check(lasp,pip) ;

	        if (lasp->n.exectimer == 0)
	            lasp->n.clr.executing = TRUE ;

	    } else
	        exectimer_clear(lasp,pip) ;

#endif /* CF_EXECTIMEROBJ */

	    if (lasp->f.shift)
	        rs = ssas_handleshift(lasp,pip,lip) ;

	    ssas_combend(lasp,pip,lip) ;

#ifdef	COMMENT
	    ssas_clearexports(lasp) ;
#endif

#if	CF_EXECTIMEROBJ
	    exectimer_end(lasp,pip) ;
#endif

	    break ;

	} /* end switch */

#ifdef	COMMENT
	if (rs < 0)
	    return rs ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
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

	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_clock: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

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


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_shift: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
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


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_getia: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;

	if (iap == NULL)
	    return SR_FAULT ;

/* put something here ! */

	*iap = (lasp->c.f.v) ? lasp->c.ia : 0 ;

	rs = lasp->c.f.v ;

	return rs ;
}
/* end subroutine (ssas_getia) */


/* load an AS with a new instruction (called from SSIW) */
int ssas_load(lasp, casp,tt)
SSAS	*lasp ;
SSAS	*casp ;
int	tt ;
{
	struct proginfo	*pip ;

	SS	*mip ;

	SSINFO	*lip ;

	CHECKER	*csp ;

	int	rs = SR_OK, rs1, i, j ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_getia: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	mip = lasp->mip ;
	lip = lasp->lip ;
	csp = &pip->check ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssas_load: SSAS(%p) asid=%d CASP(%p) tt=%d\n",
	        lasp,lasp->asid,casp,tt) ;

	    eprintf("ssas_load: loading from\n") ;

	    ss_asdump(mip,"ssas_load",casp) ;
	}
#endif /* CF_DEBUG */

	pip->s.asloads += 1 ;

/* load the weirdo thing */

	{
		struct bpred_update_t update_rec ;


		update_rec = casp->n.update_rec ;
		lasp->n.update_rec = update_rec ;

	} /* end block */

/* load it up with the new stuff */

	lasp->n.ia = casp->n.ia ;
	lasp->n.ta = casp->n.ta ;
	lasp->n.instrword = casp->n.instrword ;
	lasp->n.op = casp->n.op ;
	lasp->n.class = casp->n.class ;
	lasp->n.flags = casp->n.flags ;

/* copy over operands */

	lasp->n.nopsi = casp->n.nopsi ;
	lasp->n.nopso = casp->n.nopso ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	    char	ibuf[100] ;

	    eprintf("ssas_load: asid=%u tt=%d ia=%016llx instr=%08x\n",
	        lasp->asid,tt,lasp->n.ia,lasp->n.instrword) ;
	    rs1 = md_disassemble(lasp->n.instrword,lasp->n.ia,ibuf,99) ;

	    eprintf("ssas_load: instr> %w\n",ibuf,((rs1 > 0) ? rs1 : 0)) ;

	    eprintf("ssas_load: nopsi=%u nopso=%u\n",
	        lasp->n.nopsi,lasp->n.nopso) ;
	}
#endif /* CF_DEBUG */

/* input operands */

	for (i = 0 ; i < casp->n.nopsi ; i += 1) {

	    lasp->n.opsi[i] = casp->n.opsi[i] ;
	    lasp->n.opsi[i].f.used = TRUE ;
	    lasp->n.opsi[i].tt = INT_MIN ;

#if	(! CF_VALIDONLOAD)
	    if (! pip->f.nomem)
	        lasp->n.opsi[i].f.v = FALSE ;
#endif

	} /* end for */

	for (j = i ; j < MACHSTATE_MAXOPSI ; j += 1)
	    memset((lasp->n.opsi + i),0,sizeof(struct operand)) ;

/* output operands */

	for (i = 0 ; i < casp->n.nopso ; i += 1) {

	    lasp->n.opso[i] = casp->n.opso[i] ;
	    lasp->n.opso[i].f.used = TRUE ;
	    lasp->n.opso[i].tt = INT_MIN ;

#if	CF_SIMPLEMEM
	        lasp->n.opso[i].f.v = FALSE ;
	if (pip->f.nomem && (lasp->n.flags & F_MEM))
	        lasp->n.opso[i].f.v = TRUE ;
#else
	        lasp->n.opso[i].f.v = FALSE ;
#endif

	} /* end for */

	for (j = i ; j < MACHSTATE_MAXOPSO ; j += 1)
	    memset((lasp->n.opso + i),0,sizeof(struct operand)) ;

/* other */

	lasp->n.path = 0 ;		/* only one path for now */

/* special */

	lasp->n.tt = tt ;

/* state flags */

	lasp->n.f = casp->n.f ;

	lasp->n.f.v = TRUE ;
	lasp->n.f.enabled = TRUE ;
	lasp->n.f.export = FALSE ;
	lasp->n.f.executed = FALSE ;
	lasp->n.f.executing = FALSE ;

/* set where this instruction gets executed (localled or otherwise) */

	lasp->n.f.localexec = lasp->n.f.cf ||
		(lasp->n.class == 0) || lasp->n.f.mem ;

	if (! lasp->n.f.localexec) {

		rs1 = ssinfo_letst(lip,lasp->n.op) ;

		lasp->n.f.localexec = (rs1 > 0) ? TRUE : FALSE ;
	}

/* handle case of data memory access */

#if	CF_SIMPLEMEM
	lasp->n.mincounter = 0 ;
	if (pip->f.nomem && (lasp->n.flags & F_MEM)) {

	    uint	lat = 0 ;


	    pip->s.asmemins += 1 ;

#if	CF_DCACHE
	    {
		SSAS_MEMOP	mo ;

		int	c ;
		int	f_read = (lasp->n.flags & F_LOAD) ;


		c = ssas_getmemop(lasp,&mo) ;

		lat = 0 ;
		if (c > 0) {

		    pip->s.asmemops += 1 ;
		    if (f_read)
		        pip->s.asmemopreads += 1 ;

	            lat = dcache_latency(pip,mip,csp,mo.a,f_read) ;

	        } /* end if (got the OP) */

	        lasp->n.mincounter = (lat > 0) ? lat : 0 ;

	    } /* end block */
#else /* CF_DCACHE */

	    lasp->n.mincounter = 0 ;
	    lasp->n.f.executed = TRUE ;

#endif /* CF_DCACHE */

	} /* end if (memory) */

#else /* CF_SIMPLEMEM */

	lasp->n.mincounter = 0 ;

#endif /* CF_SIMPLEMEM */

	lasp->n.nexec = 0 ;

/* regular flags */

#ifdef	COMMENT /* very bad ! */
	memset(&lasp->f,0,sizeof(struct ssas_flags)) ;
#endif

	lasp->f.loaded = TRUE ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssas_load: loaded to\n") ;

	    ss_asdump(mip,"AS",lasp) ;

	}
#endif /* CF_DEBUG */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssas_load: input operand summary\n") ;
	    for (i = 0 ; i < lasp->n.nopsi ; i += 1) {

	        eprintf("ssas_load: op=%u a=%016llx used=%u\n",
	            i,
	            lasp->n.opsi[i].a,
	            lasp->n.opsi[i].f.used) ;

	    }
	}
#endif /* CF_DEBUG */

	return rs ;
}
/* end subroutine (ssas_load) */


/* request an operand by associative match (this is like a reverse snoop) */
int ssas_opmatch(lasp,rop)
SSAS		*lasp ;
OPERAND		*rop ;
{
	OPERAND	*nop ;

	int	rs = SR_OK, n, i ;
	int	wset ;
	int	f_gotone = FALSE ;


	if (! lasp->c.f.v)
	    return SR_EMPTY ;

	if (! lasp->c.f.export)
	    return SR_EMPTY ;

	for (wset = 0 ; wset < 2 ; wset += 1) {

	    nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	    n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;
	    for (i = 0 ; i < n ; i += 1) {

	        if (nop->f.used && nop->f.v &&
	            (nop->type == rop->type) && 
	            (nop->a == rop->a) && 
	            (nop->tt < rop->tt)) {

	            nop->f.export = TRUE ;
	            f_gotone = TRUE ;
	            break ;

	        } /* end if (need this operand) */

	        nop += 1 ;

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


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_opexport: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

/* if the AS isn't valid => it doesn't have any exportable operands ! */

	if (! lasp->c.f.v)
	    return SR_EMPTY ;

/* if we don't have any exportable operands => we don't have any ! */

	if (! lasp->c.f.export)
	    return SR_EMPTY ;

#ifdef	OPTIONAL

	if (oi >= lasp->c.noops)
	    return SR_EMPTY ;

#else

	if (oi >= MACHSTATE_MAXOPSO)
	    return SR_INVALID ;

#endif /* OPTIONAL */

	oop = lasp->c.opso + oi ;
	f = oop->f.used && oop->f.v && oop->f.export ;

	if (f) {
	    *opp = oop ;
	    lasp->n.opso[oi].clr.export = TRUE ;
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


/* if we aren't valid => then we don't have any operands ! */

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

	oop = lasp->c.opso + oi ;
	f = oop->f.used ;

	if (f)
	    *opp = oop ;

	return f ;
}
/* end subroutine (ssas_opget) */


/* snoop an operand that is "out there" (phase 1) */
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

#if	CF_MASTERDEBUG && CF_DEBUG
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

	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_readyretire: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_DEBUGS
	if (DEBUGLEVEL(4)) {
	    fp = &lasp->c.f ;
	    eprintf("ssas_readyretire: asid=%u ia=%16llx tt=%d"
	        " v=%u en=%u ex=%u exg=%u exp=%u\n",
	        lasp->asid,
	        lasp->c.ia,lasp->c.tt,fp->v,fp->enabled,
	        fp->executed,fp->executing,fp->export) ;
	    eprintf("ssas_readyretire: mincount=%u\n",
		lasp->c.mincounter) ;
	}
#endif

/* if we are unused, then we can't very well retire ! */

	if (! lasp->c.f.v)
	    return FALSE ;

/* we are ready to retire if we are not enabled ! */

	if (! lasp->c.f.enabled) {

#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_DEBUGS
	    if (DEBUGLEVEL(4))
	        eprintf("ssas_readyretire: not enabled\n") ;
#endif

	    return TRUE ;
	}

/* we are ready to commit if we have executed at least once */

#if	CF_SIMPLEMEM
	{
		int	f ;


	f = lasp->c.f.executed ;
	if (lasp->c.flags & F_MEM)
		f = (lasp->c.mincounter == 0) ;

	if ((! f) || lasp->c.f.executing)
	    return FALSE ;

	}
#else /* CF_SIMPLEMEM */

	if ((! lasp->c.f.executed) || lasp->c.f.executing)
	    return FALSE ;

#endif /* CF_SIMPLEMEM */

/* we are also not ready if some output operands needed to be exported */

#if	CF_LATECOMMIT
	if (lasp->c.f.export)
	    return FALSE ;
#endif

#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_DEBUGS
	if (DEBUGLEVEL(4))
	    eprintf("ssas_readyretire: READY\n") ;
#endif

	return TRUE ;
}
/* end subroutine (ssas_readyretire) */


/* are we ready to be loaded? */
int ssas_readyload(lasp)
SSAS	*lasp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_readyload: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	lip = lasp->lip ;

/* if we are unused or about to retire, then yes */

	f = ((! lasp->c.f.v) || lasp->f.retire) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_readyload: asid=%d c_v=%u retire=%u f=%u\n",
	        lasp->asid,lasp->c.f.v,lasp->f.retire,f) ;
#endif /* CF_DEBUG */

	return (rs >= 0) ? f : rs ;
}
/* end subroutine (ssas_readyload) */


/* perforn an execution from the FU operands */
int ssas_fureturn(lasp,ovp)
SSAS		*lasp ;
SSPE_OPV	*ovp ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	OPERAND		opsa[MACHSTATE_MAXOPSI] ;

	struct ssinfo	*lip ;

	int		rs, i ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_fureturn: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	mip = lasp->mip ;
	lip = lasp->lip ;


	lasp->s.nfuret += 1 ;

/* should we even do the execution ? */

	if ((lasp->c.ia == 0) || (ovp->ia != lasp->c.ia))
	    return SR_OK ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_fureturn: exectimer=%u\n",lasp->c.exectimer) ;
#endif /* CF_DEBUG */

	if (lasp->c.exectimer > 0)
	    return SR_OK ;

/* save current input operands */

	for (i = 0 ; i < lasp->c.nopsi ; i += 1)
	    opsa[i] = lasp->c.opsi[i] ;


/* OK, do the execution */

#if	CF_INSTREXEC

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_fureturn: ss_instrexec()\n") ;
#endif /* CF_DEBUG */

	rs = ss_instrexec(mip,lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssas_fureturn: asid=%u tt=%d ss_instrexec() rs=%d\n",
	        lasp->asid,lasp->c.tt,rs) ;
#endif /* CF_DEBUG */

#endif /* CF_INSTREXEC */


/* restore original input operands (special case, no damage) */

	for (i = 0 ; i < lasp->c.nopsi ; i += 1)
	    lasp->c.opsi[i] = opsa[i] ;


/* update state for execution */

	lasp->n.clr.executing = TRUE ;
	lasp->n.set.executed = TRUE ;

	lasp->s.nfuacc += 1 ;		/* come-back execution */


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_fureturn: ret rs=%d\n",rs) ;
#endif /* CF_DEBUG */

	return rs ;
}
/* end subroutine (ssas_fureturn) */


/* return our commitment information */
int ssas_info(lasp,ip)
SSAS		*lasp ;
SSAS_INFO	*ip ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;
	int	i ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_info: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;
	if (ip == NULL)
	    return SR_FAULT ;

	(void) memset(ip,0,sizeof(struct ssas_info)) ;

	if (! lasp->c.f.v)
	    return SR_EMPTY ;

/* get the instruction class used by Dave in the execution window */

	ip->in = lasp->c.in ;
	ip->ia = lasp->c.ia ;
	ip->ta = lasp->c.ta ;

	ip->instrword = lasp->c.instrword ;
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
	struct proginfo	*pip ;

	int	rs = SR_OK ;
	int	i ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (lasp == NULL)
	    return SR_FAULT ;

	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_retire: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;

/* entered */

	lasp->f.retire = TRUE ;


/* log our occupancy clock count */

#if	CF_ISRES
	if (lasp->c.f.v) {

	    uint	n ;


	    n = MIN(lasp->c.c_valid,(SSAS_NDENVALID - 1)) ;
	    lasp->s.isres[n] += 1 ;
	}
#endif /* CF_ISRES */

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

	    bprintf(&xip->xmlfile,"<instr>%08x</instr>\n",lasp->c.instrword) ;

	    rs = mipsdis_getlevo(&pip->dis,lasp->c.ia,lasp->c.instrword,
	        instrdis,INSTRDISLEN) ;

	    if (rs < 0)
	        strcpy(instrdis,"unknown") ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssas_xmlout: ia=%08x instr=%08x %s\n",
	            lasp->c.ia,lasp->c.instrword,instrdis) ;
#endif /* CF_DEBUG */

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


int ssas_audit(lasp)
SSAS	*lasp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	eprintf("ssas_audit: asid=%u\n",lasp->asid) ;
#endif

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_audit: 2 asid=%u\n",lasp->asid) ;
	    eprintf("ssas_audit: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(6))
	    eprintf("ssas_audit: 3 asid=%u\n",lasp->asid) ;
#endif


	{
	    int	n, i, v ;
	    int	f ;

	    uchar	*cp ;


	    cp = (uchar *) lasp ;
	    v = 0 ;
	    n = sizeof(SSAS) / sizeof(uchar) ;
	    for (i = 0 ; i < n ; i += 1)
	        v |= *cp++ ;

	    rs = (v == 0) ? SR_BADFMT : 0 ;

	} /* end block */

#if	CF_MASTERDEBUG && (CF_DEBUG || CF_DEBUGS)
	if (DEBUGLEVEL(2) && (rs < 0))
	    eprintf("ssas_audit: ret rs=%d asid=%u\n",rs,lasp->asid) ;
#endif

	return rs ;
}
/* end subroutine (ssas_audit) */


int ssas_dump(lasp,fp)
SSAS	*lasp ;
bfile	*fp ;
{
	struct proginfo	*pip ;

	struct operand	*op ;

	int	rs, i ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	eprintf("ssas_dump: asid=%u\n",lasp->asid) ;
#endif

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_dump: 2 asid=%u\n",lasp->asid) ;
	    eprintf("ssas_dump: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;


	bprintf(fp,"asid=%u\n",lasp->asid) ;

	bprintf(fp,"c_valid=%u_%u\n",
	    lasp->c.c_valid, lasp->n.c_valid) ;

	bprintf(fp,"ia=%016llx_%016llx\n",
	    lasp->c.ia, lasp->n.ia) ;

	bprintf(fp,"ta=%016llx_%016llx\n",
	    lasp->c.ta, lasp->n.ta) ;

	bprintf(fp,"op=%u_%u\n",
	    lasp->c.op, lasp->n.op) ;

	bprintf(fp,"class=%u_%u\n",
	    lasp->c.class, lasp->n.class) ;

	bprintf(fp,"tt=%u_%u\n",
	    lasp->c.tt, lasp->n.tt) ;

	bprintf(fp,"nexec=%u_%u\n",
	    lasp->c.nexec, lasp->n.nexec) ;

	bprintf(fp,"exectimer=%u_%u\n",
	    lasp->c.exectimer, lasp->n.exectimer) ;

	bprintf(fp,"f_v=%u_%u\n",
	    lasp->c.f.v, lasp->n.f.v) ;

	bprintf(fp,"f_executed=%u_%u\n",
	    lasp->c.f.executed, lasp->n.f.executed) ;

	bprintf(fp,"f_executing=%u_%u\n",
	    lasp->c.f.executing, lasp->n.f.executing) ;

	bprintf(fp,"f_export=%u_%u\n",
	    lasp->c.f.export, lasp->n.f.export) ;

	bprintf(fp,"f_enabled=%u_%u\n",
	    lasp->c.f.enabled, lasp->n.f.enabled) ;

	bprintf(fp,"f_cf=%u_%u\n",
	    lasp->c.f.cf, lasp->n.f.cf) ;

	bprintf(fp,"f_cfind=%u_%u\n",
	    lasp->c.f.cfind, lasp->n.f.cfind) ;

	bprintf(fp,"f_cfsub=%u_%u\n",
	    lasp->c.f.cfsub, lasp->n.f.cfsub) ;

	bprintf(fp,"f_cfcond=%u_%u\n",
	    lasp->c.f.cfcond, lasp->n.f.cfcond) ;

	bprintf(fp,"f_haveiops=%u_%u\n",
	    lasp->c.f.haveiops, lasp->n.f.haveiops) ;

	bprintf(fp,"f_needexec=%u_%u\n",
	    lasp->c.f.needexec, lasp->n.f.needexec) ;

	bprintf(fp,"f_nmem=%u_%u\n",
	    lasp->c.f.nmem, lasp->n.f.nmem) ;

	bprintf(fp,"current_input_operands\n") ;

	for (i = 0 ; i < MACHSTATE_MAXOPSI ; i += 1) {

	    op = lasp->c.opsi + i ;
	    if (op->f.used) {

	        ssas_dumpop(lasp,fp,i,op) ;

	    }
	}

	bprintf(fp,"next_input_operands\n") ;

	for (i = 0 ; i < MACHSTATE_MAXOPSI ; i += 1) {

	    op = lasp->n.opsi + i ;
	    if (op->f.used) {

	        ssas_dumpop(lasp,fp,i,op) ;

	    }
	}

	bprintf(fp,"current_output_operands\n") ;

	for (i = 0 ; i < MACHSTATE_MAXOPSO ; i += 1) {

	    op = lasp->c.opso + i ;
	    if (op->f.used) {

	        ssas_dumpop(lasp,fp,i,op) ;

	    }
	}

	bprintf(fp,"next_output_operands\n") ;

	for (i = 0 ; i < MACHSTATE_MAXOPSO ; i += 1) {

	    op = lasp->n.opso + i ;
	    if (op->f.used) {

	        ssas_dumpop(lasp,fp,i,op) ;

	    }
	}


	return 0 ;
}
/* end subroutine (ssas_dump) */


/* used during AS load */
int ssas_getmemop(lasp,rp)
SSAS		*lasp ;
SSAS_MEMOP	*rp ;
{
	struct proginfo	*pip ;

	OPERAND		*nop ;

	int	rs = SR_OK, n, i ;
	int	c = 0 ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	eprintf("ssas_getmemop: asid=%u\n",lasp->asid) ;
#endif

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_getmemop: 2 asid=%u\n",lasp->asid) ;
	    eprintf("ssas_getmemop: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = lasp->pip ;

	if (rp == NULL)
		return SR_FAULT ;

	if (lasp->n.flags & F_MEM) {

	    if (lasp->n.flags & F_LOAD) {

	        n = MACHSTATE_MAXOPSI ;
	        for (i = 0 ; i < n ; i += 1) {

	            nop = lasp->n.opsi + i ;
	            if (nop->f.used && (nop->type == OPERAND_TMEM)) {

			rp->f_read = TRUE ;
			rp->f_v = nop->f.v ;

			rp->a = nop->a ;
			rp->v = nop->v ;
			rp->dp = nop->dp ;
			rp->size = nop->size ;
			rp->type = nop->type ;
			c += 1 ;

		    }

		    if (c > 0)
			break ;

		} /* end for */

	    } else if (lasp->n.flags & F_STORE) {

	        n = MACHSTATE_MAXOPSO ;
	        for (i = 0 ; i < n ; i += 1) {

	            nop = lasp->n.opso + i ;
	            if (nop->f.used && (nop->type == OPERAND_TMEM)) {

			rp->f_read = FALSE ;
			rp->f_v = nop->f.v ;

			rp->a = nop->a ;
			rp->v = nop->v ;
			rp->dp = nop->dp ;
			rp->size = nop->size ;
			rp->type = nop->type ;
			c += 1 ;

		    }

		    if (c > 0)
			break ;

	        } /* end for */

	    }

	} /* end if (was a LOAD/STORE type instruction) */

	return (rs >= 0) ? c : rs ;
}
/* end subroutine (ssas_getmemop) */



/* PRIVATE SUBROUTINES */



static int ssas_dumpop(lasp,fp,oi,op)
struct operand	*op ;
SSAS		*lasp ;
int		oi ;
bfile		*fp ;
{


	bprintf(fp,
	    "op=%u f_v=%u f_requested=%u f_changed=%u\n",
	    oi,op->f.v,op->f.requested,op->f.changed) ;

	bprintf(fp,
	    "f_export=%u f_wanted=%u f_nullify=%u\n",
	    op->f.export,op->f.wanted,op->f.nullify) ;

	bprintf(fp, "type=%u trans=%u\n",op->type,op->trans) ;

	bprintf(fp, "tt=%016llx seq=%u\n",op->tt,op->seq) ;

	bprintf(fp, "frseq=%u\n",op->frseq) ;

	bprintf(fp, "a=%016llx\n",op->a) ;

	bprintf(fp, "v=%016llx\n",op->pv) ;

	return 0 ;
}


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

	struct operand	*nop ;

	int	rs, i ;
	int	n, c = 0 ;
	int	f ;


	pip = lasp->pip ;
	lip = lasp->lip ;

	nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_snoopset: asid=%u snooping %s operands\n",
	        lasp->asid,((wset) ? "input" : "output")) ;
#endif

	for (i = 0 ; i < n ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssas_snoopset: op=%u a=%016llx used=%u\n",
	            i,nop->a,nop->f.used) ;
#endif

	    if (nop->f.used) {

	        f = TRUE ;
	        f = f && (rop->type == nop->type) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: type %u\n",f) ;
#endif

	        f = f && (rop->path == nop->path) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: path %u\n",f) ;
#endif

	        f = f && (rop->a == nop->a) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: address %u\n",f) ;
#endif

	        f = f && (rop->tt < lasp->n.tt) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: our tt %u\n",f) ;
#endif

	        if (rop->trans != OPERAND_ONULLIFY)
	            f = f && ((! nop->f.v) || (rop->tt >= nop->tt)) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: last tt %u\n",f) ;
#endif

/* sequence number check */

	        if (f && ((! nop->f.v) || (rop->tt == nop->tt)))
	            f = seqok(rop->seq,nop->frseq,lip->rfmod) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_snoopset: sequence %u\n",f) ;
#endif

	        if (f) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("ssas_snoopset: asid=%u tt=%d "
	                    "snarfed a=%016llx v=%016llx\n",
	                    lasp->asid,lasp->c.tt,rop->a,rop->v) ;
#endif

	            if (rop->trans != OPERAND_ONULLIFY) {

	                nop->tt = rop->tt ;
	                nop->frseq = (rop->seq + 1) & lasp->rfmodmask ;
	                if ((! nop->f.v) || (rop->v != nop->pv)) {

	                    c += 1 ;
	                    nop->pv = rop->v ;
	                    nop->f.v = TRUE ;
	                    nop->set.changed = TRUE ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	                    if (DEBUGLEVEL(4))
	                        eprintf("ssas_snoopset: asid=%u tt=%d "
	                            "snarfed v=%016llx\n",
	                            lasp->asid,lasp->c.tt,rop->v) ;
#endif

	                } /* end if (snarfed operand) */

	            } else {

	                nop->f.v = FALSE ;
	                if (wset == 1)
	                    lasp->n.f.haveiops = FALSE ;


	            } /* end if (NULLIFY or not) */

	        } /* end if (got an operand match) */

	    } /* end if (operand was used) */

	    nop += 1 ;

	} /* end for (looping over resident operands) */

	return c ;
}
/* end subroutine (ssas_snoopset) */


/* initialize next-state that is associated with the operands */
static int ssas_opinit(lasp,wset)
SSAS		*lasp ;
int		wset ;
{
	OPERAND	*nop ;

	int	n, i ;
	int	c = 0 ;


	nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	for (i = 0 ; i < n ; i += 1) {

/* set default */

	    memset(&nop->set,0,sizeof(struct operand_flags)) ;

	    memset(&nop->clr,0,sizeof(struct operand_flags)) ;

/* now set what we want */

	    nop->clr.changed = TRUE ;	/* gets cleared unless set */

/* "old" value */

	    nop->opv = nop->pv ;
	    nop->ov = nop->v ;

	    nop += 1 ;

	} /* end for */

	return c ;
}
/* end subroutine (ssas_opinit) */


/* calculate if an operand has changed */
static int ssas_opchanged(lasp,pip,wset)
SSAS		*lasp ;
struct proginfo	*pip ;
int		wset ;
{

	struct ssinfo	*lip ;

	OPERAND	*nop, *cop ;

	int	n, i ;
	int	c ;


	lip = lasp->lip ;

	if (! lasp->c.f.v)
	    return SR_OK ;

	nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	cop = (wset) ? lasp->c.opsi : lasp->c.opso ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	c = 0 ;
	for (i = 0 ; i < n ; i += 1) {

	    if (nop->pv != nop->opv) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssas_opchanged: asid=%u iop=%u changed\n",
	                lasp->asid,i) ;
#endif

	        nop->f.changed = TRUE ;
	        nop->set.changed = TRUE ;
	        nop->seq = (cop->seq + 1) & lip->rfmodmask ;
	        c += 1 ;

	    } /* end if (value changed) */

	    nop += 1 ;
	    cop += 1 ;

	} /* end for */

/* if an input operand changed, then we need a re-execution */

	if ((wset == 1) && (c > 0)) {

#if	CF_SIMPLEMEM
	    if (! (lasp->n.flags & F_MEM))
	        lasp->f.opchanged = TRUE ;
#else
	        lasp->f.opchanged = TRUE ;
#endif

	}

/* our destination values (only if the output set was specified) */

	if (wset == 0) {

	    nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	    cop = (wset) ? lasp->c.opsi : lasp->c.opso ;
	    n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;

	    c = 0 ;
	    for (i = 0 ; i < n ; i += 1) {

	        if (nop->v != nop->ov) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("ssas_opchanged: asid=%u oop=%u changed\n",
	                    lasp->asid,i) ;
#endif

	            nop->f.export = TRUE ;
	            nop->set.export = TRUE ;
	            nop->seq = (cop->seq + 1) & lip->rfmodmask ;
	            c += 1 ;

	        } /* end if (value changed) */

	        nop += 1 ;
	        cop += 1 ;

	    } /* end for */

/* if an output operand changed, mark it to be exported */

	    if (c > 0)
	        lasp->n.set.export = TRUE ;

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
	OPERAND	ro, *nop ;

	int	rs = SR_OK, i ;
	int	wset ;
	int	n ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkops: entered asid=%u tt=%d\n",
	        lasp->asid,lasp->c.tt) ;
#endif

	for (wset = 0 ; wset < 2 ; wset += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssas_checkops: checking %s ops\n",
	            ((wset) ? "input" : "output")) ;
#endif

	    nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	    n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;
	    for (i = 0 ; i < n ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_checkops: checking op=%u used=%u a=%016llx\n",
	                i,nop->f.used,nop->a) ;
#endif

	        if (nop->f.used && (! nop->f.v) &&
	            (! nop->f.requested)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssas_checkops: we need op=%u\n",i) ;
#endif

/* prepare the flow-group for a request (set our TT) */

	            ro = *nop ;
	            ro.tt = lasp->c.tt ;

/* make the request */

	            rs = ssiw_oprequest(lasp->wp,&ro,lasp->asid) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssas_checkops: ssiw_oprequest() rs=%d\n",rs) ;
#endif

	            if (rs < 0)
	                break ;

	            nop->f.requested = TRUE ;

	        } /* end if (need this operand) */

	        nop += 1 ;

	    } /* end for (operands within a set) */

	} /* end for (which set) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_checkops: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssas_checkops) */


/* check to see if we need to execute (has its own clock cycle) */
static int ssas_execrequest(lasp,pip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	SS	*mip ;

	OPERAND	*oop ;

	SSPE_FUREQ	fuinfo ;

	uint	latency ;

	int	rs = SR_OK, n, i ;
	int	fui ;


	mip = lasp->mip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_execrequest: entered ia=%016llx\n",
	        lasp->c.ia) ;
#endif /* CF_DEBUG */

/* do we even have an instruction ? */

#if	defined(OPTIONAL) || 1
	if ((lasp->c.ia == 0) || lasp->c.f.localexec)
	    return SR_OK ;
#endif

/* do we need execution ? */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_execrequest: c.needexec=%u needexec=%u\n",
	        lasp->c.f.needexec,lasp->f.needexec) ;
#endif /* CF_DEBUG */

	if ((! lasp->c.f.needexec) && (! lasp->f.needexec))
	    return SR_OK ;

	if (! lasp->c.f.haveiops)
	    return SR_OK ;

	if ((! pip->f.execover) && lasp->c.f.executing)
	    return SR_OK ;

/* OK, get a FU resource based on our instruction class */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssas_execrequest: sspe_fureq() class=%u\n",
	        lasp->c.class) ;
#endif /* CF_DEBUG */

	rs = sspe_fureq(lasp->pep,lasp->c.class,&fuinfo) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssas_execrequest: sspe_fureq() rs=%d ful=%u\n",
	        rs,fuinfo.ful) ;
#endif /* CF_DEBUG */

	if (rs >= 0) {

	    SSPE_OPV	v ;


	    fui = rs ;		/* it returns an FU index ? */

/* prepare the information to load to the FU */

	    v.ia = lasp->c.ia ;
	    v.instrword = lasp->c.instrword ;
	    v.nopsi = lasp->c.nopsi ;
	    for (i = 0 ; i < lasp->c.nopsi ; i += 1)
		lasp->c.opsi[i] = lasp->c.opsi[i] ;

/* load everything to the FU */

	    rs = sspe_fuload(lasp->pep,lasp->c.class,fui,lasp->asid,&v) ;

	    if (rs >= 0) {

	    lasp->n.clr.needexec = TRUE ;
	    lasp->n.set.executing = TRUE ;

	    lasp->f.needexec = FALSE ;

	    lasp->n.exectimer = fuinfo.ful ;

#if	CF_EXECTIMEROBJ
	    exectimer_set(lasp,pip,fuinfo.ful) ;
#endif

	    lasp->s.ntexec += 1 ;
	    lasp->s.nfuiss += 1 ;
	    if (lasp->c.f.executed)
	        lasp->s.nrexec += 1 ;

	    rs = 1 ;

	    } /* end if (successful FU load) */

	} else if (rs == SR_NOENT)
	    rs = SR_OK ;

	return rs ;
}
/* end subroutine (ssas_execrequest) */


/* perforn a local execution */
static int ssas_execlocal(lasp,pip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	SS		*mip ;

	int		rs, i ;


	if (lasp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((lasp->magic != SSAS_MAGIC) && (lasp->magic != 0)) {

	    eprintf("ssas_execlocal: SSAS(%p) bad magic\n",lasp) ;

	    return SR_BADFMT ;
	}

	if (lasp->magic != SSAS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	mip = lasp->mip ;


	if (! lasp->c.f.localexec)
	    return SR_INVALID ;


/* OK, do the execution (for FU accounting) */

	sspe_fusu(lasp->pep,lasp->c.class) ;

#if	CF_INSTREXEC

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_execlocal: ss_instrexec()\n") ;
#endif /* CF_DEBUG */

	rs = ss_instrexec(mip,lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssas_execlocal: asid=%u tt=%d ss_instrexec() rs=%d\n",
	        lasp->asid,lasp->c.tt,rs) ;
#endif /* CF_DEBUG */

#endif /* CF_INSTREXEC */


/* update state for execution */

	lasp->f.needexec = FALSE ;

	lasp->n.clr.needexec = TRUE ;
	lasp->n.clr.executing = TRUE ;
	lasp->n.set.executed = TRUE ;

	lasp->s.nnorexec += 1 ;
	lasp->s.ntexec += 1 ;
	if (lasp->c.f.executed)
	    lasp->s.nrexec += 1 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_execlocal: ret rs=%d\n",rs) ;
#endif /* CF_DEBUG */

	return rs ;
}
/* end subroutine (ssas_execlocal) */


/* check if we have all input operands needed (set next-state) */
static int ssas_haveiops(lasp,pip)
SSAS		*lasp ;
struct proginfo	*pip ;
{
	OPERAND	*nop ;

	int	rs, n, i ;


/* are all of the operands that we need valid ? */

	nop = lasp->n.opsi ;
	n = lasp->n.nopsi ;
	for (i = 0 ; i < n ; i += 1) {

	    if (nop->f.used && (! nop->f.v))
	        break ;

	    nop += 1 ;

	} /* end for */

	if (i >= n)
	    lasp->n.f.haveiops = TRUE ;

	return rs ;
}
/* end subroutine (ssas_haveiops) */


/* clean something to do with exported stuff (?) */
static int ssas_clearexports(lasp)
SSAS		*lasp ;
{
	struct ssinfo	*lip ;

	struct operand	*nop ;

	int	rs, i ;
	int	wset = 0 ;
	int	n, c = 0 ;
	int	f ;


	lip = lasp->lip ;

	nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;
	for (i = 0 ; i < n ; i += 1) {

	    if (nop->f.v) {

	        nop->f.export = FALSE ;

	    }

	    nop += 1 ;

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
	struct operand	*nop ;

	int	i, j, k ;
	int	n ;
	int	tt_test ;


	if (! lasp->f.shift)
	    return SR_OK ;

	lasp->f.shift = FALSE ;
	tt_test = INT_MIN + lasp->shiftamount ;


/* do the most obvious stuff */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_handleshift: asid=%d shift=%d c_tt=%d n_tt=%d\n",
	        lasp->asid,lasp->shiftamount,lasp->c.tt,lasp->n.tt) ;
#endif /* CF_DEBUG */

	lasp->n.tt -= lasp->shiftamount ;


/* all of the sources and destinations */

	nop = lasp->n.opsi ;
	n = MACHSTATE_MAXOPSI ;
	for (i = 0 ; i < n ; i += 1) {

	    if (nop->f.used) {

	        if (nop->tt >= tt_test)
	            nop->tt -= lasp->shiftamount ;

	        else
	            nop->tt = INT_MIN ;

	    }

	    nop += 1 ;

	} /* end for */

	n = MACHSTATE_MAXOPSO ;
	nop = lasp->n.opso ;
	for (i = 0 ; i < n ; i += 1) {

	    if (nop->f.used) {

	        if (nop->tt >= tt_test)
	            nop->tt -= lasp->shiftamount ;

	        else
	            nop->tt = INT_MIN ;

	    }

	    nop += 1 ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssas_handleshift) */


static int ssas_combstart(lasp,pip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	int	rs = SR_OK ;


	memset(&lasp->n.set,0,sizeof(struct ssas_flags)) ;

	memset(&lasp->n.clr,0,sizeof(struct ssas_flags)) ;

	return rs ;
}
/* end subroutine (ssas_combstart) */


static int ssas_combend(lasp,pip,lip)
SSAS		*lasp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	OPERAND	*nop ;

	int	wset, n, i, j ;
	int	f_active ;


	lasp->n.f.export = FALSE ;

	if (lasp->f.loaded)
		lasp->n.f.v = TRUE ;

	else if (lasp->f.retire)
		lasp->n.f.v = FALSE ;

/* else, leave it alone so that it holds over */


	if (lasp->n.f.v) {

		if (! lasp->f.loaded) {

#if	CF_MASTERDEBUG && CF_DEBUG && CF_DEBUGCOMBEND
	    if (DEBUGLEVEL(4)) {
	        eprintf("ssas_combend: "
	            "set.needexec=%u set.executing=%u set.executed=%u\n",
	            lasp->n.set.needexec,
	            lasp->n.set.executing,
	            lasp->n.set.executed) ;
	        eprintf("ssas_combend: "
	            "clr.needexec=%u clr.executing=%u clr.executed=%u\n",
	            lasp->n.clr.needexec,
	            lasp->n.clr.executing,
	            lasp->n.clr.executed) ;
	    }
#endif

	    for (wset = 0 ; wset < 2 ; wset += 1) {

	        nop = (wset) ? lasp->n.opsi : lasp->n.opso ;
	        n = (wset) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO ;
	        for (j = 0 ; j < n ; j += 1) {

	            if (nop->set.v) nop->f.v = TRUE ;
	            else if (nop->clr.v) nop->f.v = FALSE ;

	            if (nop->set.requested) nop->f.requested = TRUE ;
	            else if (nop->clr.requested) nop->f.requested = FALSE ;

	            if (nop->set.changed) nop->f.changed = TRUE ;
	            else if (nop->clr.changed) nop->f.changed = FALSE ;

	            if (nop->set.export) nop->f.export = TRUE ;
	            else if (nop->clr.export) nop->f.export = FALSE ;

	            if (nop->set.nullify) nop->f.nullify = TRUE ;
	            else if (nop->clr.nullify) nop->f.nullify = FALSE ;

	            if (nop->set.fault) nop->f.fault = TRUE ;
	            else if (nop->clr.fault) nop->f.fault = FALSE ;

/* consequences */

	            if ((wset == 0) && nop->f.export)
	                lasp->n.f.export = TRUE ;

	            nop += 1 ;

	        } /* end for */

	    } /* end for */

/* do other state flags */

	    if (lasp->n.set.needexec)
	        lasp->n.f.needexec = TRUE ;

	    else if (lasp->n.clr.needexec)
	        lasp->n.f.needexec = FALSE ;


	    if (lasp->n.set.executing)
	        lasp->n.f.executing = TRUE ;

	    else if (lasp->n.clr.executing)
	        lasp->n.f.executing = FALSE ;


	    if (lasp->n.set.executed)
	        lasp->n.f.executed = TRUE ;

	    else if (lasp->n.clr.executed)
	        lasp->n.f.executed = FALSE ;


	    if (lasp->c.f.v)
	        lasp->n.c_valid = lasp->c.c_valid + 1 ;

	} else {

	    lasp->n.c_valid = 1 ;

	}

	} else if (lasp->f.retire) {

/* retiring w/o being re-loaded */

	    lasp->n.mincounter = 0 ;

	} /* end if (active or not) */


#if	CF_MASTERDEBUG && CF_DEBUG && CF_DEBUGCOMBEND
	if (DEBUGLEVEL(4))
	    eprintf("ssas_combend: "
	        "n.needexec=%u n.executing=%u n.executed=%u\n",
	        lasp->n.f.needexec,
	        lasp->n.f.executing,
	        lasp->n.f.executed) ;
#endif


/* done ? */

	return SR_OK ;
}
/* end subroutine (ssas_combend) */


static int ssas_handlecommit(lasp)
SSAS	*lasp ;
{
	struct operand	*nop ;

	int	rs, n, i ;
	int	f ;


	if (lasp->f.loaded || (! lasp->f.commit))
	    return SR_OK ;

/* clear the used bits in any operands */

	n = MACHSTATE_MAXOPSI ;
	nop = lasp->n.opsi ;
	for (i = 0 ; i < n ; i += 1) {

	    if (nop->f.used)
	        nop->f.used = FALSE ;

	    nop += 1 ;

	} /* end for */

	n = MACHSTATE_MAXOPSO ;
	nop = lasp->n.opso ;
	for (i = 0 ; i < n ; i += 1) {

	    if (nop->f.used)
	        nop->f.used = FALSE ;

	    nop += 1 ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssas_handlecommit) */


#if	CF_EXECTIMEROBJ

static int exectimer_init(op,pip)
SSAS		*op ;
struct proginfo	*pip ;
{


	op->n.exectimer = 0 ;
	return 0 ;
}


static int exectimer_begin(op,pip)
SSAS		*op ;
struct proginfo	*pip ;
{


	op->f.exectimer = 0 ;
	return 0 ;
}


static int exectimer_set(op,pip,v)
SSAS		*op ;
struct proginfo	*pip ;
int		v ;
{


#if	CF_MASTERDEBUG && CF_DEBUG && CF_DEBUGETIMER
	if (DEBUGLEVEL(4))
	    eprintf("ssas/exectimer_set: v=%d\n",
	        v) ;
#endif

	op->n.exectimer = v ;
	op->f.exectimer = TRUE ;

	return 0 ;
}


static int exectimer_clear(op,pip)
SSAS		*op ;
struct proginfo	*pip ;
{


	op->n.exectimer = 0 ;
	op->f.exectimer = FALSE ;
	return 0 ;
}


static int exectimer_check(op,pip)
SSAS		*op ;
struct proginfo	*pip ;
{


#if	CF_MASTERDEBUG && CF_DEBUG && CF_DEBUGETIMER
	if (DEBUGLEVEL(4))
	    eprintf("ssas/exectimer_check: f_exectimer=%u exectimer=%u\n",
	        op->f.exectimer,op->n.exectimer) ;
#endif

	if ((! op->f.exectimer) && (op->n.exectimer > 0))
	    op->n.exectimer -= 1 ;

	return 0 ;
}


static int exectimer_end(op,pip)
SSAS		*op ;
struct proginfo	*pip ;
{


	return 0 ;
}


static int exectimer_free(op,pip)
SSAS		*op ;
struct proginfo	*pip ;
{


	op->n.exectimer = 0 ;
	op->f.exectimer = FALSE ;
	return 0 ;
}

#endif /* CF_EXECTIMEROBJ */

#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_XML

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

#endif /* CF_DEBUG */



