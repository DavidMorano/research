/* sslsq */

/* load-store queue */


#define	CF_DEBUG	0		/* switchable debugging */
#define	CF_SAFE		1		/* safe mode */
#define	CF_OPTIONAL	1		/* do optional stuff (safety) ? */
#define	CF_INSTREXEC	1
#define	CF_SCREWMEM	0		/* memory alignment crap ? */


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

	This module provides the load-store-queue function.
	It also stores pending read requests that are outstanding
	in the memory system hierarchy.

	When a "request" comes in from an AS, we first search the
	store-queue for a matching address.  It it is found, it is
	marked as needing to be exported.  When we are polled by the
	SSIW module, all exported operands are broadcast forwarded and
	the operand is then marked as no longer needing export.

	If there is no match in the store-queue for a requested
	operand, an entry is made in the pending list with the
	information from the request.  There are as many entries in the
	pending list (arranged for at initialization time) as there are
	ASes in the window.  In this way, there can never be a
	situation where there is no free list entry for a requesting
	AS.  The machine does not have the mechanism to ration out
	pending request entries.  Also, a pending request entry does
	not eactly correspond with an actual microarchitectural 
	component.  In a real machine, a request from an AS that
	misses in the store-queue would proceed directly to and
	into the memory hierarchy without any pending list component.

	When the latency on a pending request has been determined, the
	request starts a timer with the latency and it sits in the list
	until the the timer expires.  On expiration, the response is
	marked as needing export and it eventually gets broadcast back
	to the ASes, including any that requested it.


**************************************************************************/


#define	SSLSQ_MASTER	1


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
#include	"sslsq.h"
#include	"memory.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	SSLSQ_MAGIC	0x19089827
#define	SSLSQ_DISPLAYTT	-9999999

#ifndef	RFTYPE_RELAY
#define	RFTYPE_RELAY	0
#define	RFTYPE_NULLIFY	1
#endif

#define	INSTRDISLEN	100

#define	DEFNSLOTS	200
#define	DEFNREADS	200

#define	NREADCLOCKS	2
#define	NWRITECLOCKS	2

#ifndef	MEMWORDSIZE
#define	MEMWORDSIZE	8
#define	MEMWORDADDR(a)	((a) & 15)
#endif



/* external subroutines */

extern int	ffbsi(uint) ;
extern int	getinterleave(uint,uint) ;
extern int	getnumbuses(uint) ;
extern int	seqok(uint,uint,uint) ;


/* forward references */

static int	sslsq_startops(SSLSQ *,int) ;
static int	sslsq_checkexec(SSLSQ *,struct proginfo *,struct ssinfo *) ;
static int	sslsq_snoopset(SSLSQ *,int,struct operand *) ;
static int	sslsq_handleshift(SSLSQ *,struct proginfo *,struct ssinfo *) ;
static int	sslsq_handlecommit(SSLSQ *) ;
static int	sslsq_combs(SSLSQ *,struct proginfo *,struct ssinfo *) ;

#ifdef	COMMENT
static int	sslsq_xmloutreg(SSLSQ *,XMLINFO *,struct proginfo *,
			struct sslsq_slot *,char *) ;
#endif /* COMMENT */

#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_XML
static int	mkttbuf(char *,int) ;
#endif






int sslsq_init(op,pip,mip,lip,ap)
SSLSQ		*op ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
SSLSQ_INITARGS	*ap ;
{
	int	rs = SR_OK, i ;
	int	size ;
	int	n ;


	if (op == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(SSLSQ)) ;

	op->magic = 0 ;

#ifdef	OPTIONAL
	(void) memset(&op->f,0,sizeof(struct sslsq_flags)) ;
#endif

	op->pip = pip ;
	op->mip = mip ;
	op->lip = lip ;

	if (ap != NULL) {

/* register forwarding type */

	    op->rftype = MIN(ap->rftype,1) ;

	    op->nwrites = ap->nwrites ;
	    op->nreads = ap->nreads ;
	}


/* fix up any bad arguments */

	n = (lip->iwsize * 2) ;
	if (op->nwrites < n)
	    op->nwrites = n ;

	n = (lip->iwsize * 2) ;
	if (op->nreads < n)
	    op->nreads = n ;


/* zero all machine state */

#ifdef	OPTIONAL
	(void) memset(&op->c,0,sizeof(struct sslsq_state)) ;

	(void) memset(&op->n,0,sizeof(struct sslsq_state)) ;
#endif


/* allocate data structures */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sslsq_init: nwrites=%u nreads=%u\n",
		op->nwrites,op->nreads) ;
#endif

/* pending writes */

	size = 2 * op->nwrites * sizeof(struct sslsq_slot) ;
	rs = uc_malloc(size,&op->a.writes) ;

	if (rs < 0)
	    goto bad0 ;

	memset(op->a.writes,0,size) ;

	op->n.writes = op->a.writes + 0 ;
	op->c.writes = op->a.writes + op->nwrites ;

/* pending reads */

	size = 2 * op->nreads * sizeof(struct sslsq_slot) ;
	rs = uc_malloc(size,&op->a.reads) ;

	if (rs < 0)
	    goto bad1 ;

	memset(op->a.reads,0,size) ;

	op->n.reads = op->a.reads + 0 ;
	op->c.reads = op->a.reads + op->nreads ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("sslsq_init: ret rs=%d\n",rs) ;
#endif

	op->magic = SSLSQ_MAGIC ;
	return rs ;

/* bad things */
bad1:
	free(op->a.writes) ;

bad0:
	return rs ;
}
/* end subroutine (sslsq_init) */


/* free up the object */
int sslsq_free(op)
SSLSQ		*op ;
{
	struct proginfo		*pip ;

	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_free: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

	if (op->a.reads)
	    free(op->a.reads) ;

	if (op->a.writes)
	    free(op->a.writes) ;


	op->magic = 0 ;
	return rs ;
}
/* end subroutine (sslsq_free) */


/* perform our combinatorial logic */
int sslsq_comb(op,phase)
SSLSQ		*op ;
int		phase ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	ULONG	clock ;

	int	rs = SR_OK, i ;
	int	rs1 ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_comb: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	mip = op->mip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("sslsq_comb: entered ck=%llu:%u \n",
	        mip->ck,phase) ;
#endif


/* do our own combinatorial work */

	switch (phase) {

	case 0:
	    op->f.shift = FALSE ;

	    sslsq_startops(op,0) ;

	    break ;

/* all operands are exhanged in this phase (in zero time !) */
	case 1:
	    break ;

	case 2:

	    break ;

	case 3:
	    break ;

	case 4:
	    if (op->f.shift)
	        rs = sslsq_handleshift(op,pip,lip) ;

	    break ;

	case 5:

#ifdef	COMMENT
	    sslsq_handlecommit(op) ;
#endif

	    sslsq_combs(op,pip,lip) ;

	    break ;

	} /* end switch */

	if (rs < 0)
	    return rs ;


	return rs ;
}
/* end subroutine (sslsq_comb) */


/* preform a clocked state transition */
int sslsq_clock(op)
SSLSQ		*op ;
{
	struct proginfo		*pip ;

	struct ssinfo		*lip ;

	int	rs ;
	int	ret ;
	int	i,j,k,ll ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_clock: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;


/* transition ourself */


/* careful here */

	op->c = op->n ;




/* execute all subobject clock methods */



	return SR_OK ;
}
/* end subroutine (sslsq_clock) */


/* receive the combinatorial SHIFT signal */
int sslsq_shift(op,amount)
SSLSQ		*op ;
int		amount ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_shift: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;


/* "do" all of our subobjects */



/* finally "do" us */

	op->f.shift = TRUE ;
	op->shiftamount = amount ;

bad5:
bad4:
bad3:
bad2:
bad1:
	return rs ;
}
/* end subroutine (sslsq_shift) */


#ifdef	COMMENT

/* set an initial register value */
int sslsq_setval(op,n,v)
SSLSQ		*op ;
int		n ;
ULONG		v ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_setval: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

	if (n > MACHSTATE_NREGS)
	    return SR_INVALID ;

	op->c.regs[n].fg.pv = v ;
	op->c.regs[n].fg.v = v ;

	op->n.regs[n].fg.pv = v ;
	op->n.regs[n].fg.v = v ;

	return rs ;
}
/* end subroutine (sslsq_setval) */

#endif /* COMMENT */


/* request an operand by associative match (this is like a reverse snoop) */
int sslsq_opmatch(op,rop)
SSLSQ		*op ;
OPERAND		*rop ;
{
	int	rs = SR_OK ;
	int	i, n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_opmatch: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (rop->type != OPERAND_TMEM)
	    return SR_INVALID ;

#ifdef	COMMENT
	n = MACHSTATE_NREGS ;
	i = rop->a ;
	if (i > n)
	    return SR_INVALID ;

	op->n.regs[i].set.export = TRUE ;
#endif /* COMMENT */

	return SR_OK ;
}
/* end subroutine (sslsq_opmatch) */


/* request an operand */
int sslsq_oprequest(op,a,size)
SSLSQ		*op ;
ULONG		a ;
uint		size ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;
	int	f = FALSE ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_oprequest: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	f = FALSE ;

/* do we have it in the store-queue part ? */

	for (i = 0 ; i < op->nwrites ; i += 1) {

	    if (op->c.writes[i].f.v && 
		(MEMWORDADDR(op->c.writes[i].fg.a) == MEMWORDADDR(a))) {
	        op->n.writes[i].set.export = TRUE ;
	        f = TRUE ;
	        break ;
	    }

	} /* end for */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_oprequest: after write-search f=%u\n",f) ;
#endif

/* else is it in the pending part ? */

	if (! f) {

	    for (i = 0 ; i < op->nreads ; i += 1) {

	        if (op->c.reads[i].f.v && 
		(MEMWORDADDR(op->c.reads[i].fg.a) == MEMWORDADDR(a))) {
	            f = TRUE ;
	            break ;
	        }

	    } /* end for */

	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_oprequest: after read-search f=%u\n",f) ;
#endif

/* else, make it pending in the pending part */

	if (! f) {

/* find a free pending slot */

	    for (i = 0 ; i < op->nreads ; i += 1) {

	        if (! op->n.reads[i].f.v)
	            break ;

	    } /* end for */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_oprequest: free read-slot i=%u\n",i) ;
#endif

	    if (i < op->nreads) {

	        memset(&op->n.reads[i],0,sizeof(struct sslsq_slot)) ;

	        op->n.reads[i].fg.a = a ;
	        op->n.reads[i].fg.size = size ;
	        op->n.reads[i].fg.tt = -1 ;
	        op->n.reads[i].f.v = TRUE ;
		op->n.reads[i].cc = (NREADCLOCKS - 1) ;

		op->n.nreads += 1 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_oprequest: loaded read-slot\n") ;
#endif

	    } else
	        rs = SR_NOBUFS ;

	} /* end if (entering into pending slot) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_oprequest: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sslsq_oprequest) */


int sslsq_cursorinit(op,cp)
SSLSQ		*op ;
SSLSQ_CURSOR	*cp ;
{


	cp->si = 0 ;
	cp->ei = -1 ;
	return 0 ;
}
/* end subroutine (sslsq_cursorinit) */


int sslsq_cursorfree(op,cp)
SSLSQ		*op ;
SSLSQ_CURSOR	*cp ;
{


	cp->si = 0 ;
	cp->ei = -1 ;
	return 0 ;
}
/* end subroutine (sslsq_cursorfree) */


/* is an operand exportable ? */
int sslsq_opexport(op,cp,opp)
SSLSQ		*op ;
SSLSQ_CURSOR	*cp ;
OPERAND		**opp ;
{
	struct proginfo	*pip ;

	OPERAND	*oop ;
	OPERAND	*fgp ;

	int	rs = SR_OK, si, ei ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_opexport: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_opexport: entered\n") ;
#endif

	if (cp == NULL)
	    return SR_FAULT ;

	si = (cp->si < 0) ? 0 : (cp->si + 1) ;
	ei = (cp->ei < 0) ? 0 : (cp->ei + 1) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	eprintf("sslsq_opexport: nwrites=%d nreads=%d\n",
		op->nwrites,op->nreads) ;
	eprintf("sslsq_opexport: si=%d ei=%d\n",si,ei) ;
	}
#endif

	if (si > 1)
	    return SR_EMPTY ;

/* perform a carry-out as appropriate */

	if ((si == 0) && (ei >= op->nwrites)) {
	    ei = 0 ;
	    si = 1 ;
	}

	if ((si == 1) && (ei >= op->nreads))
	    return SR_EMPTY ;

/* OK, start the search */

	f = FALSE ;

/* check the write slots first */

	if (si == 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_opexport: write slots\n") ;
#endif

	    while (ei < op->nwrites) {

	        if (op->c.writes[ei].f.v && op->c.writes[ei].f.export) {

	            oop = &op->c.writes[ei].fg ;
	            op->n.writes[ei].clr.export = TRUE ;
	            f = TRUE ;
	            break ;

	        }

		ei += 1 ;

	    } /* end while */

	    if (ei >= op->nwrites) {
	        si += 1 ;
	        ei = 0 ;
	    }

	} /* end if (looping through slots) */

/* check the read slots if necessary */

	if ((! f) && (si == 1)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_opexport: read slots ei=%d\n",ei) ;
#endif

	    while (ei < op->nreads) {

	        if (op->c.reads[ei].f.v && op->c.reads[ei].f.export) {

	            oop = &op->c.reads[ei].fg ;
	            op->n.reads[ei].f.export = TRUE ;
	            f = TRUE ;
	            break ;

	        }

		ei += 1 ;

	    } /* end while */

	    if (ei >= op->nreads) {
	        si += 1 ;
	        ei = 0 ;
		rs = SR_EMPTY ;
	    }

	} /* end if (looping through pending reads) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_opexport: after slot checks rs=%d f=%u\n",
		rs,f) ;
#endif

	if (rs >= 0) {

	    if (f) {

		CHECKER	*csp = &pip->check ;

		SS	*mip = op->mip ;

		ULONG	aa, v ;


	        *opp = oop ;
		aa = MEMWORDADDR(oop->a) ;

/* read the value (an 8-byte word) from checker "committed" memory */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	eprintf("sslsq_opexport: memory read oop(%p)\n",oop) ;
	eprintf("sslsq_opexport: mem(%p)\n",csp->mem) ;
	eprintf("sslsq_opexport: a=%16llx\n",oop->a) ;
	}
#endif

#if	CF_SCREWMEM
		mem_access(csp->mem,Read,oop->a,&v,8) ;
#else
		v = MEM_READ_QWORD(csp->mem,aa) ;
#endif

		(*opp)->v = v ;
		(*opp)->pv = v ;
		(*opp)->size = MEMWORDSIZE ;
		(*opp)->a = aa ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_opexport: exporting OP a=%16llx\n",oop->a) ;
#endif

	    } /* end if (providing the value) */

	    cp->si = si ;
	    cp->ei = ei ;
	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sslsq_opexport: ret rs=%d f=%u si=%u ei=%u\n",
		rs,f,si,ei) ;
#endif

	return (rs >= 0) ? f : rs ;
}
/* end subroutine (sslsq_opexport) */


#ifdef	COMMENT

/* get an operand */
int sslsq_opget(op,i,opp)
SSLSQ		*op ;
int		i ;
OPERAND		**opp ;
{
	OPERAND	*oop ;

	int	rs = SR_OK ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_opget: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (i > MACHSTATE_NREGS)
	    return SR_INVALID ;

	oop = &op->c.regs[i].fg ;
	f = op->c.regs[i].f.export ;
	*opp = oop ;

	return f ;
}
/* end subroutine (sslsq_opget) */

#endif /* COMMENT */


/* merge an operand into our operand-array */
int sslsq_opmerge(op,cop)
SSLSQ	*op ;
OPERAND	*cop ;
{
	struct proginfo	*pip = op->pip ;

	struct ssinfo	*lip = op->lip ;

	int	rs = SR_OK, i ;
	int	f = FALSE ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSLSQ_MAGIC) && (op->magic != 0)) {

	    eprintf("sslsq_opmerge: SSLSQ=%08lx bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSLSQ_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (cop == NULL)
	    return SR_FAULT ;

	if (cop->type != OPERAND_TMEM)
	    return SR_INVALID ;

#ifdef	COMMENT /* infinite write queue for now */
	i = op->a ;
	if (op->n.regs[i].fg.tt < op->tt) {

	    op->n.regs[i].fg.tt = op->tt ;
	    op->n.regs[i].fg.pv = op->pv ;
	    op->n.regs[i].fg.v = op->v ;
	    op->n.regs[i].fg.seq = 
	        (op->c.regs[i].fg.seq + 1) & lip->rfmodmask ;

	    f = TRUE ;

	}

#else /* COMMENT */

/* write it to memory */


#endif /* COMMENT */

	return (rs >= 0) ? f : rs ;
}
/* end subroutine (sslsq_opmerge) */


#ifdef	COMMENT

/* XML stuff */
int sslsq_xmlinit(fp,xip)
SSLSQ		*fp ;
XMLINFO		*xip ;
{



	return SR_OK ;
}


int sslsq_xmlout(op,xip)
SSLSQ		*op ;
XMLINFO		*xip ;
{
	struct proginfo	*pip ;

	int	rs ;

	char	ttbuf[40] ;


	pip = op->pip ;

	bprintf(&xip->xmlfile,"<as>\n") ;

	bprintf(&xip->xmlfile,"<uid>%p</uid>\n",op) ;

	bprintf(&xip->xmlfile,"<asid>%d</asid>\n",op->asid) ;

	bprintf(&xip->xmlfile,"<path>%d</path>\n",op->c.path) ;

	mkttbuf(ttbuf, op->c.tt) ;

	bprintf(&xip->xmlfile,"<tt>%s</tt>\n",ttbuf) ;

	bprintf(&xip->xmlfile,"<used>%d</used>\n",
	    (op->c.opclass != OPCSSLSQS_UNUSED)) ;

	if (op->c.opclass != OPCSSLSQS_UNUSED) {

	    char	instrdis[INSTRDISLEN + 1] ;


	    bprintf(&xip->xmlfile,"<ia>%08x</ia>\n",op->c.ia) ;

	    bprintf(&xip->xmlfile,"<instr>%08x</instr>\n",op->c.instrcode) ;

	    rs = mipsdis_getlevo(&pip->dis,op->c.ia,op->c.instrcode,
	        instrdis,INSTRDISLEN) ;

	    if (rs < 0)
	        strcpy(instrdis,"unknown") ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("sslsq_xmlout: ia=%08x instr=%08x %s\n",
	            op->c.ia,op->c.instrcode,instrdis) ;
#endif /* CF_DEBUG */

	    bprintf(&xip->xmlfile,"<instrdis>%s</instrdis>\n",
	        instrdis) ;

	    bprintf(&xip->xmlfile,"<opclass>%d</opclass>\n",op->c.opclass) ;

	    bprintf(&xip->xmlfile,"<opexec>%d</opexec>\n",op->c.opexec) ;

/* what about some register sources ? */

	    if (op->c.src1.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.src1,"src1") ;

	    if (op->c.src2.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.src2,"src2") ;

	    if (op->c.src3.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.src3,"src3") ;

	    if (op->c.src4.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.src4,"src4") ;

	    if (op->c.src5.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.src5,"src5") ;

	    if (op->c.dst1.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.dst1,"dst1") ;

	    if (op->c.dst2.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.dst2,"dst2") ;

	    if (op->c.dst3.a != (~ 0))
	        sslsq_xmloutreg(op,xip,pip,&op->c.dst3,"dst3") ;

	} /* end if (we had some instruction) */

	bprintf(&xip->xmlfile,"</as>\n") ;

	return SR_OK ;
}
/* end subroutine (sslsq_xmlout) */


int sslsq_xmloutreg(op,xip,pip,rp,name)
SSLSQ		*op ;
XMLINFO		*xip ;
struct proginfo	*pip ;
struct sslsq_slot	*rp ;
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
/* end subroutine (sslsq_xmloutreg) */


int sslsq_xmlfree(fp,xip)
SSLSQ		*fp ;
XMLINFO		*xip ;
{



	return SR_OK ;
}
/* end subroutine (sslsq_xmlfree) */

#endif /* COMMENT */



/* PRIVATE SUBROUTINES */



/* initialize next-state that is associated with the operands */
static int sslsq_startops(op,wset)
SSLSQ		*op ;
int		wset ;
{
	struct sslsq_slot	*wsp ;

	struct sslsq_slot	*rsp ;

	OPERAND	*oop ;

	int	rs = SR_OK, i ;


/* do the write slots */

	for (i = 0 ; i < op->nwrites ; i += 1) {

	    wsp = op->n.writes + i ;
	    memset(&wsp->set,0,sizeof(struct sslsq_rflags)) ;

	    memset(&wsp->clr,0,sizeof(struct sslsq_rflags)) ;

	    oop = &wsp->fg ;
	    memset(&oop->set,0,sizeof(struct operand_flags)) ;

	    memset(&oop->clr,0,sizeof(struct operand_flags)) ;

	} /* end for */

/* do the read slots */

	for (i = 0 ; i < op->nreads ; i += 1) {

	    rsp = op->n.reads + i ;
	    memset(&rsp->set,0,sizeof(struct sslsq_rflags)) ;

	    memset(&rsp->clr,0,sizeof(struct sslsq_rflags)) ;

	    oop = &rsp->fg ;
	    memset(&oop->set,0,sizeof(struct operand_flags)) ;

	    memset(&oop->clr,0,sizeof(struct operand_flags)) ;

	} /* end for */

	return rs ;
}
/* end subroutine (sslsq_startops) */


/* handle a machine shift operation ! */
static int sslsq_handleshift(op,pip,lip)
SSLSQ		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	struct sslsq_slot	*sp ;

	struct operand		*oop ;

	int	n, i ;
	int	tt_test ;


	if (! op->f.shift)
	    return SR_OK ;

	op->f.shift = FALSE ;
	tt_test = INT_MIN + op->shiftamount ;


/* write slots */

	for (i = 0 ; i < op->nwrites ; i += 1) {

	    sp = op->n.writes + i ;
	    oop = &sp->fg ;

	    if (oop->tt >= tt_test)
	        oop->tt -= op->shiftamount ;

	    else
	        oop->tt = INT_MIN ;

	} /* end for */

/* read slots */

	for (i = 0 ; i < op->nreads ; i += 1) {

	    sp = op->n.reads + i ;
	    oop = &sp->fg ;

	    if (oop->tt >= tt_test)
	        oop->tt -= op->shiftamount ;

	    else
	        oop->tt = INT_MIN ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (sslsq_handleshift) */


static int sslsq_combs(op,pip,lip)
SSLSQ		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	struct sslsq_slot	*wsp ;

	struct sslsq_slot	*rsp ;

	OPERAND	*oop ;

	int	n, i ;


/* write slots */

	for (i = 0 ; i < op->nwrites ; i += 1) {

	    wsp = op->n.writes + i ;

/* any counter */

		if (wsp->cc == 1)
			wsp->f.v = FALSE ;

		if (wsp->cc != 0) {

			wsp->cc -= 1 ;

			if (wsp->set.export)
				wsp->f.export = TRUE ;

			else if (wsp->clr.export)
				wsp->f.export = FALSE ;

		}

	} /* end for */

/* read slots */

	for (i = 0 ; i < op->nreads ; i += 1) {

	    rsp = op->n.reads + i ;

/* any counter */

		if (rsp->cc == 0) {

			rsp->f.v = FALSE ;
			rsp->f.export = FALSE ;

		} else if (rsp->cc == 1)
			rsp->f.export = TRUE ;

		if (rsp->cc != 0)
			rsp->cc -= 1 ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (sslsq_combs) */


#if	(CF_MASTERDEBUG && CF_DEBUG) || CF_XML

static int mkttbuf(buf,tt)
char	buf[] ;
int	tt ;
{
	int	rs ;


	if (tt < SSLSQ_DISPLAYTT) {

	    buf[0] = '-' ;
	    buf[1] = '\0' ;
	    rs = 2 ;

	} else
	    rs = ctdeci(buf,-1,tt) ;

	return rs ;
}

#endif /* CF_DEBUG */



