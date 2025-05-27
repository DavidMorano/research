/* ssreg */

/* architected register file */


#define	CF_DEBUG	0		/* switchable debugging */
#define	CF_SAFE		1		/* safe mode */
#define	CF_OPTIONAL	1		/* do optional stuff (safety) ? */
#define	CF_INSTREXEC	1


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


	Phase 2:

	+ All ASes exchange their current operands.


	Phase 3:

	+ We request any operands that were not already satisfied above.

	+ We calculate whether we need to re-execute anything.
	The execution will occur in the next clock if one is
	needed.


	Phase 4:

	+ We handle any shifting that we might have been directed to do.



**************************************************************************/


#define	SSREG_MASTER	1


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
#include	"checker.h"
#include	"instrclass.h"
#include	"machstate.h"
#include	"ssreg.h"
#include	"regs.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	SSREG_MAGIC	0x09089827
#define	SSREG_DISPLAYTT	-9999999

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

static int	ssreg_startops(SSREG *,int) ;
static int	ssreg_handleshift(SSREG *,struct proginfo *,struct ssinfo *) ;
static int	ssreg_handlecommit(SSREG *) ;
static int	ssreg_combs(SSREG *,struct proginfo *,struct ssinfo *) ;

#ifdef	COMMENT
static int	ssreg_xmloutreg(SSREG *,XMLINFO *,struct proginfo *,
			struct ssreg_reg *,char *) ;
#endif /* COMMENT */

#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_XML
static int	mkttbuf(char *,int) ;
#endif


/* initialize this SSREG object */

/**** with :

	- object pointer
	- 'proginfo'
	- SS pointer
	- 'ssinfo'
	- our unique AS identification number
	- physical column index
	- physical SG index

****/


int ssreg_init(regp,pip,mip,lip,ap)
SSREG		*regp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
SSREG_INITARGS	*ap ;
{
	int	rs = SR_OK, i ;
	int	n, size ;
	int	npred ;
	int	maxspan ;


	if (regp == NULL) 
	    return SR_FAULT ;

	(void) memset(regp,0,sizeof(SSREG)) ;

	regp->magic = 0 ;

#ifdef	OPTIONAL
	(void) memset(&regp->f,0,sizeof(struct ssreg_flags)) ;
#endif

	regp->pip = pip ;
	regp->mip = mip ;
	regp->lip = lip ;

	if (ap != NULL) {

	regp->rftype = MIN(ap->rftype,1) ;	/* register forwarding type */

	}


/* zero all machine state */

#ifdef	OPTIONAL
	(void) memset(&regp->c,0,sizeof(struct ssreg_state)) ;

	(void) memset(&regp->n,0,sizeof(struct ssreg_state)) ;
#endif

	n = MACHSTATE_NREGS ;
	for (i = 0 ; i < n ; i += 1) {

		regp->n.regs[i].fg.type = OPERAND_TREG ;
		regp->n.regs[i].fg.a = i ;
		regp->n.regs[i].fg.tt = -1 ;

		regp->c.regs[i].fg.type = OPERAND_TREG ;
		regp->c.regs[i].fg.a = i ;
		regp->c.regs[i].fg.tt = -1 ;

	} /* end for */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("ssreg_init: ret rs=%d\n",rs) ;
#endif

	regp->magic = SSREG_MAGIC ;
	return rs ;

/* bad things */
bad0:
	return rs ;
}
/* end subroutine (ssreg_init) */


/* free up the object */
int ssreg_free(regp)
SSREG		*regp ;
{
	struct proginfo		*pip ;

	int	rs = SR_OK ;


	if (regp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_free: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = regp->pip ;


	regp->magic = 0 ;
	return rs ;
}
/* end subroutine (ssreg_free) */


/* perform our combinatorial logic */
int ssreg_comb(regp,phase)
SSREG		*regp ;
int		phase ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	ULONG	clock ;

	int	rs = SR_OK, i ;
	int	rs1 ;


	if (regp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_comb: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = regp->pip ;
	mip = regp->mip ;
	lip = regp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssreg_comb: entered ck=%llu:%u \n",
	        mip->ck,phase) ;
#endif


/* do our own combinatorial work */

	switch (phase) {

	case 0:
		regp->f.commit = FALSE ;
		regp->f.shift = FALSE ;

	    ssreg_startops(regp,0) ;

	    break ;

/* all operands are exhanged in this phase (in zero time !) */
	case 1:
	    break ;

	case 2:

	    break ;

	case 3:
	    break ;

	case 4:
	    if (regp->f.shift)
	        rs = ssreg_handleshift(regp,pip,lip) ;

	    break ;

	case 5:

#ifdef	COMMENT
		ssreg_handlecommit(regp) ;
#endif

	    ssreg_combs(regp,pip,lip) ;

	    break ;

	} /* end switch */

	if (rs < 0)
	    return rs ;


	return rs ;
}
/* end subroutine (ssreg_comb) */


/* preform a clocked state transition */
int ssreg_clock(regp)
SSREG		*regp ;
{
	struct proginfo		*pip ;

	struct ssinfo		*lip ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (regp == NULL)
	    return SR_FAULT ;

	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_clock: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = regp->pip ;


/* transition ourself */


/* careful here */

	regp->c = regp->n ;




/* execute all subobject clock methods */



	return SR_OK ;
}
/* end subroutine (ssreg_clock) */


/* receive the combinatorial SHIFT signal */
int ssreg_shift(regp,amount)
SSREG		*regp ;
int		amount ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (regp == NULL)
	    return SR_FAULT ;

	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_shift: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = regp->pip ;
	lip = regp->lip ;


/* "do" all of our subobjects */



/* finally "do" us */

	regp->f.shift = TRUE ;
	regp->shiftamount = amount ;

bad5:
bad4:
bad3:
bad2:
bad1:
	return rs ;
}
/* end subroutine (ssreg_shift) */


/* set an initial register value */
int ssreg_setval(regp,n,v)
SSREG		*regp ;
int		n ;
ULONG		v ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (regp == NULL)
	    return SR_FAULT ;

	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_setval: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = regp->pip ;
	lip = regp->lip ;

	if (n > MACHSTATE_NREGS)
		return SR_INVALID ;

	regp->c.regs[n].fg.pv = v ;
	regp->c.regs[n].fg.v = v ;

	regp->n.regs[n].fg.pv = v ;
	regp->n.regs[n].fg.v = v ;

	return rs ;
}
/* end subroutine (ssreg_setval) */


/* request an operand by associative match (this is like a reverse snoop) */
int ssreg_opmatch(regp,rop)
SSREG		*regp ;
OPERAND		*rop ;
{
	int	rs = SR_OK ;
	int	i, n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_opmatch: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	    if (rop->type != OPERAND_TREG) 
		return SR_INVALID ;

	n = MACHSTATE_NREGS ;
	i = rop->a ;
	if (i > n)
		return SR_INVALID ;
	
	regp->n.regs[i].set.export = TRUE ;

	return SR_OK ;
}
/* end subroutine (rssreg_opmatch) */


/* request an operand */
int ssreg_oprequest(regp,i)
SSREG		*regp ;
int		i ;
{
	int	rs = SR_OK ;
	int	n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_oprequest: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	n = MACHSTATE_NREGS ;
	if (i > n)
		return SR_INVALID ;
	
	regp->n.regs[i].set.export = TRUE ;

	return SR_OK ;
}
/* end subroutine (rssreg_oprequest) */


int ssreg_cursorinit(op,cp)
SSREG		*op ;
SSREG_CURSOR	*cp ;
{


	cp->ei = -1 ;
	return 0 ;
}
/* end subroutine (ssreg_cursorinit) */


int ssreg_cursorfree(op,cp)
SSREG		*op ;
SSREG_CURSOR	*cp ;
{


	cp->ei = -1 ;
	return 0 ;
}
/* end subroutine (ssreg_cursorfree) */


/* is an operand exportable ? */
int ssreg_opexport(regp,cp,opp)
SSREG		*regp ;
SSREG_CURSOR	*cp ;
OPERAND		**opp ;
{
	PROGINFO	*pip ;

	OPERAND	*oop ;

	int	rs = SR_OK, ei ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_opexport: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (cp == NULL)
	    return SR_FAULT ;

	pip = regp->pip ;

	ei = (cp->ei < 0) ? 0 : (cp->ei + 1) ;

	f = FALSE ;
	while (ei < MACHSTATE_NREGS) {

		if (f = regp->c.regs[ei].f.export)
			break ;

		ei += 1 ;

	} /* end while */

	if (ei >= MACHSTATE_NREGS)
		rs = SR_EMPTY ;

	if (rs >= 0) {

		if (f) {

			SS	*mip = regp->mip ;

			CHECKER	*csp = &pip->check ;

			ULONG	v ;

			uint	n ;


		oop = &regp->c.regs[ei].fg ;
		*opp = oop ;
		regp->n.regs[ei].clr.export = TRUE ;

/* grab the value from the checker "committed" register file */

			n = oop->a ;
			regs_getval(&csp->regs,n,&v) ;

			(*opp)->v = v ;
			(*opp)->pv = v ;

		} /* end if (got one to export) */

		cp->ei = ei ;
	}

	return (rs >= 0) ? f : rs ;
}
/* end subroutine (rssreg_opexport) */


/* get an operand */
int ssreg_opget(regp,i,opp)
SSREG		*regp ;
int		i ;
OPERAND		**opp ;
{
	OPERAND	*oop ;

	int	rs = SR_OK ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_opget: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (i > MACHSTATE_NREGS)
		return SR_INVALID ;

	oop = &regp->c.regs[i].fg ;
	f = regp->c.regs[i].f.export ;
	*opp = oop ;

	return f ;
}
/* end subroutine (ssreg_opget) */


/* merge an operand into our operand-array */
int ssreg_opmerge(regp,op)
SSREG	*regp ;
OPERAND	*op ;
{
	struct proginfo	*pip = regp->pip ;

	struct ssinfo	*lip = regp->lip ;

	int	rs = SR_OK, i ;
	int	f = FALSE ;


#if	CF_MASTERDEBUG && CF_SAFE
	if ((regp->magic != SSREG_MAGIC) && (regp->magic != 0)) {

	    eprintf("ssreg_opmerge: SSREG=%08lx bad magic\n",regp) ;

	    return SR_BADFMT ;
	}

	if (regp->magic != SSREG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (op == NULL)
		return SR_FAULT ;

	if (op->type != OPERAND_TREG)
		return SR_INVALID ;

	i = op->a ;
	if (regp->n.regs[i].fg.tt < op->tt) {

		regp->n.regs[i].fg.tt = op->tt ;
		regp->n.regs[i].fg.pv = op->pv ;
		regp->n.regs[i].fg.v = op->v ;
		regp->n.regs[i].fg.seq = 
			(regp->c.regs[i].fg.seq + 1) & lip->rfmodmask ;

		f = TRUE ;

	}

	return (rs >= 0) ? f : rs ;
}
/* end subroutine (ssreg_opmerge) */


#ifdef	COMMENT

/* XML stuff */
int ssreg_xmlinit(fp,xip)
SSREG		*fp ;
XMLINFO		*xip ;
{



	return SR_OK ;
}


int ssreg_xmlout(regp,xip)
SSREG		*regp ;
XMLINFO		*xip ;
{
	struct proginfo	*pip ;

	int	rs ;

	char	ttbuf[40] ;


	pip = regp->pip ;

	bprintf(&xip->xmlfile,"<as>\n") ;

	bprintf(&xip->xmlfile,"<uid>%p</uid>\n",regp) ;

	bprintf(&xip->xmlfile,"<asid>%d</asid>\n",regp->asid) ;

	bprintf(&xip->xmlfile,"<path>%d</path>\n",regp->c.path) ;

	mkttbuf(ttbuf, regp->c.tt) ;

	bprintf(&xip->xmlfile,"<tt>%s</tt>\n",ttbuf) ;

	bprintf(&xip->xmlfile,"<used>%d</used>\n",
	    (regp->c.opclass != OPCSSREGS_UNUSED)) ;

	if (regp->c.opclass != OPCSSREGS_UNUSED) {

	    char	instrdis[INSTRDISLEN + 1] ;


	    bprintf(&xip->xmlfile,"<ia>%08x</ia>\n",regp->c.ia) ;

	    bprintf(&xip->xmlfile,"<instr>%08x</instr>\n",regp->c.instrcode) ;

	    rs = mipsdis_getlevo(&pip->dis,regp->c.ia,regp->c.instrcode,
	        instrdis,INSTRDISLEN) ;

	    if (rs < 0)
	        strcpy(instrdis,"unknown") ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("ssreg_xmlout: ia=%08x instr=%08x %s\n",
	            regp->c.ia,regp->c.instrcode,instrdis) ;
#endif /* CF_DEBUG */

	    bprintf(&xip->xmlfile,"<instrdis>%s</instrdis>\n",
	        instrdis) ;

	    bprintf(&xip->xmlfile,"<opclass>%d</opclass>\n",regp->c.opclass) ;

	    bprintf(&xip->xmlfile,"<opexec>%d</opexec>\n",regp->c.opexec) ;

/* what about some register sources ? */

	    if (regp->c.src1.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.src1,"src1") ;

	    if (regp->c.src2.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.src2,"src2") ;

	    if (regp->c.src3.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.src3,"src3") ;

	    if (regp->c.src4.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.src4,"src4") ;

	    if (regp->c.src5.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.src5,"src5") ;

	    if (regp->c.dst1.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.dst1,"dst1") ;

	    if (regp->c.dst2.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.dst2,"dst2") ;

	    if (regp->c.dst3.a != (~ 0))
	        ssreg_xmloutreg(regp,xip,pip,&regp->c.dst3,"dst3") ;

	} /* end if (we had some instruction) */

	bprintf(&xip->xmlfile,"</as>\n") ;

	return SR_OK ;
}
/* end subroutine (ssreg_xmlout) */


int ssreg_xmloutreg(regp,xip,pip,rp,name)
SSREG		*regp ;
XMLINFO		*xip ;
struct proginfo	*pip ;
struct ssreg_reg	*rp ;
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
/* end subroutine (ssreg_xmloutreg) */


int ssreg_xmlfree(fp,xip)
SSREG		*fp ;
XMLINFO		*xip ;
{



	return SR_OK ;
}
/* end subroutine (ssreg_xmlfree) */

#endif /* COMMENT */



/* PRIVATE SUBROUTINES */



/* initialize next-state that is associated with the operands */
static int ssreg_startops(regp,wset)
SSREG		*regp ;
int		wset ;
{
	struct ssreg_reg	*rp ;

	OPERAND	*oop ;

	int	rs = SR_OK, i ;


	for (i = 0 ; i < MACHSTATE_NREGS ; i += 1) {

		rp = regp->n.regs + i ;
		memset(&rp->set,0,sizeof(struct ssreg_rflags)) ;

		memset(&rp->clr,0,sizeof(struct ssreg_rflags)) ;

		oop = &rp->fg ;
		memset(&oop->set,0,sizeof(struct operand_flags)) ;

		memset(&oop->clr,0,sizeof(struct operand_flags)) ;

	} /* end for */

	return rs ;
}
/* end subroutine (ssreg_startops) */


/* handle a machine shift operation ! */
static int ssreg_handleshift(regp,pip,lip)
SSREG		*regp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	struct ssreg_reg	*rp ;

	struct operand		*oop ;

	int	i ;
	int	m ;
	int	tt_test ;


	if (! regp->f.shift)
	    return SR_OK ;

	regp->f.shift = FALSE ;
	tt_test = INT_MIN + regp->shiftamount ;


/* all of the sources and destinations */

	m = MACHSTATE_NREGS ;
	for (i = 0 ; i < m ; i += 1) {

		rp = regp->n.regs + i ;
		oop = &rp->fg ;

	        if (oop->tt >= tt_test)
	            oop->tt -= regp->shiftamount ;

	        else
	            oop->tt = INT_MIN ;

	} /* end for */


	return SR_OK ;
}
/* end subroutine (ssreg_handleshift) */


static int ssreg_combs(regp,pip,lip)
SSREG		*regp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	struct ssreg_reg	*rp ;

	OPERAND	*oop ;

	int	m, i ;


	m = MACHSTATE_NREGS ;
	for (i = 0 ; i < m ; i += 1) {

		rp = regp->n.regs + i ;
		if (rp->set.export)
			rp->f.export = TRUE ;

		else if (rp->clr.export)
			rp->f.export = FALSE ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        eprintf("ssreg_combs: a=%u s_export=%u c_export=%u\n",
		i,rp->set.export,rp->clr.export) ;
#endif /* CF_DEBUG */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5) && rp->f.export)
	        eprintf("ssreg_combs: exportable a=%u\n",i) ;
#endif /* CF_DEBUG */

#ifdef	COMMENT
		oop = &regp->n.regs[i].fg ;

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

#endif /* COMMENT */

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssreg_combs) */


#ifdef	COMMENT

static int ssreg_handlecommit(regp)
SSREG		*regp ;
{
	struct operand	*setp, *oop ;

	int	rs, n, i ;
	int	f ;


	if (regp->f.loaded)
		regp->n.f.v = TRUE ;

#if	OPTIONAL
	if ((! regp->f.loaded) && regp->f.commit)
		regp->n.f.v = FALSE ;
#endif

	if (regp->f.loaded || (! regp->f.commit))
		return SR_OK ;

		regp->n.f.v = FALSE ;

/* clear the used bits in any operands */

	n = MACHSTATE_MAXOPSI ;
	setp = regp->n.iops ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;
	    if (oop->f.used) 
		oop->f.used = FALSE ;

	} /* end for */


	n = MACHSTATE_MAXOPSO ;
	setp = regp->n.oops ;
	for (i = 0 ; i < n ; i += 1) {

	    oop = setp + i ;
	    if (oop->f.used)
		oop->f.used = FALSE ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (ssreg_handlecommit) */

#endif /* COMMENT */


#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_XML

static int mkttbuf(buf,tt)
char	buf[] ;
int	tt ;
{
	int	rs ;


	if (tt < SSREG_DISPLAYTT) {

	    buf[0] = '-' ;
	    buf[1] = '\0' ;
	    rs = 2 ;

	} else
	    rs = ctdeci(buf,-1,tt) ;

	return rs ;
}

#endif /* CF_DEBUG */



