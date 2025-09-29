/* sspe */

/* processing element resource allocator */


#define	CF_DEBUGS	0
#define	CF_DEBUG	0		/* switchable debugging */
#define	CF_SAFE		0		/* safe mode */
#define	CF_LPE		0		/* what is this now ? */


/* revision history:

	= 00/02/04, Dave Morano

	Module was originally written for the LEVO simulator LEVOSIM.


	= 03/11/07, Dave Morano

	This code was borrowed from the AS code but is pretty much
	hacked to pieces from that.  Really just the skeleton of
	the AS code was used.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/**************************************************************************

	This code module provides the resource allocation function for
	the ASes.  As AS will call 'sspe_getfu()' to try to get
	(reserve) a functional unit for its execution.  If the return
	value is non-zero positive, then it has the unit.  If the
	return is 0, then no more units were available in the
	requesting clock cycle.  If the return is negative, then it is
	a programming bug because the AS asked for an invalid function
	unit.


**************************************************************************/


#define	SSPE_MASTER	0


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"ss.h"
#include	"xmlinfo.h"

#include	"instrclass.h"
#include	"sscommon.h"
#include	"ssinfo.h"
#include	"machstate.h"
#include	"sspe.h"
#include	"sshiftreg.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	SSPE_MAGIC	0x09189887
#define	SSPE_DISPLAYTT	-9999999

#ifndef	RFTYPE_RELAY
#define	RFTYPE_RELAY	0
#define	RFTYPE_NULLIFY	1
#endif

#define	INSTRDISLEN	100



/* external subroutines */

extern int	ffbsi(uint) ;
extern int	getnumbuses(uint) ;


/* forward references */

static int	sspe_loadcnums(SSPE *,struct ssinfo *) ;
static int	sspe_loaddefs(SSPE *,struct proginfo *,struct ssinfo *) ;
static int	sspe_finish(SSPE *,struct proginfo *,struct ssinfo *) ;
static int	sspe_combstart(SSPE *,struct proginfo *,struct ssinfo *) ;
static int	sspe_combend(SSPE *,struct proginfo *,struct ssinfo *) ;
static int	sspe_furesults(SSPE *,struct proginfo *,struct ssinfo *) ;
static int	sspe_handleshift(SSPE *,struct proginfo *,struct ssinfo *) ;

#ifdef	COMMENT
static int	sspe_xmloutreg(SSPE *,XMLINFO *,struct proginfo *,
			struct sspe_reg *,char *) ;
#endif /* COMMENT */

#if	(CF_MASTERDEBUG && CF_DEBUG) || F_XML
static int	mkttbuf(char *,int) ;
#endif

/* local variables */






/* initialize this SSPE object */

/**** with :

	- object pointer
	- 'proginfo'
	- SS pointer
	- 'ssinfo'
	- arguments

****/

int sspe_init(op,pip,mip,lip,ap)
SSPE		*op ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
SSPE_INITARGS	*ap ;
{
	int	rs = SR_OK, i, j ;
	int	c, n, size ;


#if	CF_SAFE
	if ((op == NULL) || (ap == NULL))
	    return SR_FAULT ;
#endif

	(void) memset(op,0,sizeof(SSPE)) ;

	op->pip = pip ;
	op->mip = mip ;
	op->lip = lip ;

/* load up any arguments given us */

	op->wp = ap->wp ;
	op->ssas = ap->ssas ;

#if	CF_MASTERDEBUG && CF_DEBUG && CF_LPE
	if (DEBUGLEVEL(4))
	    eprintf("sspe_init: lpe.ialu=%u\n",
	        lip->lpe.ialu) ;
#endif

#ifdef	COMMENT
	sspe_loadcnums(op,lip) ;
#endif


/* zero all machine state */

	(void) memset(&op->c,0,sizeof(struct sspe_state)) ;

	(void) memset(&op->n,0,sizeof(struct sspe_state)) ;


	(void) memset(&op->s,0,sizeof(SSPE_INFO)) ;


	for (i = 1 ; i < iclass_overlast ; i += 1)
	    op->c.cnums[i] = lip->nfu[i] ;


/* allocate the structures for the PEs */

	size = iclass_overlast * sizeof(SSHIFTREG *) ;
	rs = uc_malloc(size,&op->sra) ;

	if (rs < 0)
	    goto bad0 ;

	op->nsr = 0 ;
	for (i = 0 ; i < iclass_overlast ; i += 1)
	    op->nsr += lip->nfu[i] ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_init: nsr=%u\n",op->nsr) ;
#endif

	size = op->nsr * sizeof(SSHIFTREG) ;
	rs = uc_malloc(size,&op->ssrs) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_init: uc_malloc() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad1 ;

/* fill the array with pointers to the various class FU arrays */

	n = 0 ;
	for (i = 0 ; i < iclass_overlast ; i += 1) {

	    op->sra[i] = op->ssrs + n ;
	    n += lip->nfu[i] ;

	} /* end for */

/* initialze the shift registers */

	n = 0 ;
	size = sizeof(struct sspe_opv) ;
	for (i = 0 ; i < iclass_overlast ; i += 1) {

	    for (j = 0 ; j < lip->nfu[i] ; j += 1) {

#if	CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sspe_init: sshiftreg_init() size=%u n=%u\n",
	                size,lip->lfu[i]) ;
#endif

	        rs = sshiftreg_init((op->sra[i] + j),size,lip->lfu[i]) ;

	        if (rs < 0)
	            break ;

	        n += 1 ;

	    }

	}

	if (rs < 0) {

	    if (n > 0) {

	        c = 0 ;
	        for (i = 0 ; i < iclass_overlast ; i += 1) {

	            for (j = 0 ; j < lip->nfu[i] ; j += 1) {

	                sshiftreg_free(op->ssrs + i) ;

	                c += 1 ;
	                if (c >= n)
	                    break ;

	            }

	            if (c >= n)
	                break ;

	        }

	    } /* end if */

	    goto bad2 ;
	}


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_init: ret rs=%d\n",rs) ;
#endif

	op->magic = SSPE_MAGIC ;
	return rs ;

/* bad things */
bad2:
	free(op->sra[0]) ;

bad1:
	free(op->sra) ;

bad0:
	return rs ;
}
/* end subroutine (sspe_init) */


/* free up the object */
int sspe_free(op)
SSPE		*op ;
{
	struct proginfo		*pip ;

	int	rs = SR_OK, i ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_free: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;

/* free the objects */

	for (i = 0 ; i < op->nsr ; i += 1)
	    sshiftreg_free(op->ssrs + i) ;

/* free up our memory allocations */

	if (op->sra[0] != NULL)
	    free(op->sra[0]) ;

	free(op->sra) ;

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (sspe_free) */


/* perform our combinatorial logic */
int sspe_comb(op,phase)
SSPE		*op ;
int		phase ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	ULONG	clock ;

	int	rs = SR_OK, i ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_SAFE
	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_comb: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	mip = op->mip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("sspe_comb: entered ck=%llu:%u\n",
	        mip->ck,phase) ;
#endif

/* do our subobjects */

	for (i = 0 ; i < op->nsr ; i += 1) {

	    rs = sshiftreg_comb((op->ssrs + i),phase) ;

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0)
	    goto ret0 ;


/* do our own combinatorial work */

	switch (phase) {

	case 0:
	    op->n.nissue = 0 ;
	    rs = sspe_loaddefs(op,pip,lip) ;

	    if (rs >= 0)
	        rs = sspe_combstart(op,pip,lip) ;

	    break ;

/* all operands are exhanged in this phase */
	case 1:
	    break ;

/* execute all our combinational parts */
	case 2:
	    break ;

	case 3:
	    rs = sspe_furesults(op,pip,lip) ;

	    break ;

	case 4:
	    rs = sspe_finish(op,pip,lip) ;

	    break ;

	case 5:
	    rs = sspe_combend(op,pip,lip) ;

	    if ((rs >= 0) && op->f.shift)
	        rs = sspe_handleshift(op,pip,lip) ;

	    break ;

	} /* end switch */


ret0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("sspe_comb: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sspe_comb) */


/* preform a clocked state transition */
int sspe_clock(op)
SSPE		*op ;
{
	struct proginfo		*pip ;

	struct ssinfo		*lip ;

	int	rs = SR_OK, i ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_clock: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;


/* do our subobjects */

	for (i = 0 ; i < op->nsr ; i += 1) {

	    rs = sshiftreg_clock(op->ssrs + i) ;

	    if (rs < 0)
	        break ;

	}

	if (rs < 0)
	    goto ret0 ;


/* transition ourself */

	op->c = op->n ;


ret0:
	return rs ;
}
/* end subroutine (sspe_clock) */


/* receive the combinatorial SHIFT signal */
int sspe_shift(op,amount)
SSPE		*op ;
int		amount ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_shift: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
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
/* end subroutine (sspe_shift) */


/* request a stastistic update on an FU for a specified class */
int sspe_fusu(op,class)
SSPE		*op ;
int		class ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_fusu: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (DEBUGLEVEL(5))
	    eprintf("sspe_fusu: class=%u\n",class) ;
#endif

	if (class < iclass_overlast) {

	        op->s.cgrants[class] += 1 ;

	} else
	    rs = SR_INVALID ;

	return rs ;
}
/* end subroutine (sspe_fusu) */


/* request an FU for a specified class */
int sspe_fureq(op,class,rp)
SSPE		*op ;
int		class ;
SSPE_FUREQ	*rp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs, fui ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_fureq: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (DEBUGLEVEL(5))
	    eprintf("sspe_fureq: class=%u\n",class) ;
#endif

	if (class < iclass_overlast) {

	    rs = SR_NOENT ;
	    if ((op->n.cnums[class] > 0) &&
		((lip->iwidth <= 0) || (op->n.nissue < lip->iwidth))) {

	        rs = SR_OK ;
	        op->n.cnums[class] -= 1 ;
		op->n.nissue += 1 ;
	        fui = op->n.cnums[class] ;

	        if (rp != NULL) {
	            rp->fui = fui ;
	            rp->ful = lip->lfu[class] ;
	        }

	        op->s.cgrants[class] += 1 ;

	    } else
	        op->s.cwaits[class] += 1 ;

	} else {

		op->s.cinvalids += 1 ;
	    rs = SR_INVALID ;

	}

	return (rs >= 0) ? fui : rs ;
}
/* end subroutine (sspe_fureq) */


/* load up an FU */
int sspe_fuload(op,class,fui,tag,ovp)
SSPE		*op ;
int		class ;
int		fui ;
int		tag ;
SSPE_OPV	*ovp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK ;
	int	ful = 0 ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_fuload: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_fuload: class=%u\n",class) ;
#endif

	if (class < iclass_overlast) {

		ovp->class = class ;
		ovp->fui = fui ;
		ovp->tag = tag ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4)) {
	        eprintf("sspe_fuload: sshiftreg_write() "
		"class=%u fui=%u tag=%u\n",
	            class,fui,tag) ;
	        eprintf("sspe_fuload: ia=%016llx\n",ovp->instrword) ;
	    }
#endif

	    rs = sshiftreg_write((op->sra[class] + fui),ovp) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sspe_fuload: sshiftreg_write() rs=%d\n",rs) ;
#endif

#if	CF_DEBUG
	    if (DEBUGLEVEL(4) && (rs >= 0)) {
	    struct sspe_opv	*ep ;
	        int	rs1 ;

	        rs1 = sshiftreg_gettail((op->sra[class] + fui),&ep) ;

	        eprintf("sspe_fuload: rs=%d class=%u fui=%u tag=%u\n",
	            rs1,ep->class,ep->fui,ep->tag) ;
	        eprintf("sspe_fuload: ia=%016llx\n",ep->instrword) ;

#ifdef	COMMENT
	        rs1 = sshiftreg_audit(op->sra[class] + fui) ;

	        eprintf("sspe_fuload: sshiftreg_audit() rs=%d\n",rs1) ;

	        rs1 = sspe_audit(op) ;

	        eprintf("sspe_fuload: sspe_audit() rs=%d\n",rs1) ;
#endif /* COMMENT */

	    }
#endif /* CF_DEBUG */

		ful = lip->lfu[class] ;

		op->s.cissues[class] += 1 ;

	} else
	    rs = SR_INVALID ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_fuload: ret=%d\n",rs) ;
#endif

	return (rs >= 0) ? ful : rs ;
}
/* end subroutine (sspe_fuload) */


/* try to allocate an FU for a specified class */
int sspe_allocfu(op,class,tag,ovp)
SSPE		*op ;
int		class ;
int		tag ;
SSPE_OPV	*ovp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i, j, rc ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_allocfu: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_allocfu: class=%u\n",class) ;
#endif

	if (class < iclass_overlast) {

	    rc = op->n.cnums[class] ;
	    if (rc > 0) {

	        for (i = 0 ; i < lip->nfu[class] ; i += 1) {

	            rs = sshiftreg_writea(op->sra[class] + i) ;

/* error ? */

	            if (rs < 0)
	                break ;

/* got one */

	            if (rs > 0)
	                break ;

	        } /* end for */

	        if (rs > 0) {

	            op->n.cnums[class] -= 1 ;

/* copy the operands in */

		    ovp->class = class ;
		    ovp->fui = i ;
	            ovp->tag = tag ;

	            rs = sshiftreg_write((op->sra[class] + i),ovp) ;

	            op->s.cgrants[class] += 1 ;

	        } else
	            rs = SR_BADFMT ;

	    } else
	        op->s.cwaits[class] += 1 ;

	} else
	    rs = SR_INVALID ;

	return (rs >= 0) ? rc : rs ;
}
/* end subroutine (sspe_allocfu) */


int sspe_pollresult(op,class,tag,ovp)
SSPE		*op ;
int		class ;
int		tag ;
SSPE_OPV	*ovp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	struct sspe_opv	*ep ;

	int	rs = SR_OK, i, j , rc ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_pollresult: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_pollresult: class=%u\n",class) ;
#endif

	if (class < iclass_overlast) {

	    for (i = 0 ; i < lip->nfu[class] ; i += 1) {

	        rs = sshiftreg_gethead((op->sra[class] + i),&ep) ;

/* error ? */

	        if (rs < 0)
	            break ;

/* got one */

	        if (ep->tag == tag)
	            break ;

	    } /* end for */

	    if (rs > 0) {

	        rc = (i < lip->nfu[class]) ;
	        if (rc) {

		    memcpy(ovp,ep,sizeof(SSPE_OPV)) ;

	        }

	    } else
	        rs = SR_BADFMT ;

	} else
	    rs = SR_INVALID ;

	return (rs >= 0) ? rc : rs ;
}
/* end subroutine (sspe_pollresult) */


/* get (actually reserve) a function unit for this clock */
int sspe_getfu(op,class,lp)
SSPE		*op ;
int		class ;
uint		*lp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, rc ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_shift: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_getfu: class=%u\n",class) ;
#endif

	if (lp != NULL)
	    *lp = 0 ;

	if (class < iclass_overlast) {

	    rc = op->n.cnums[class] ;
	    if (op->n.cnums[class] > 0) {

	        op->n.cnums[class] -= 1 ;
	        op->s.cgrants[class] += 1 ;
#ifdef	COMMENT
	        if (lp != NULL)
	            *lp = op->cdelays[class] ;
#else
	        if (lp != NULL)
	            *lp = lip->lfu[class] ;
#endif /* COMMENT */

	    } else
	        op->s.cwaits[class] += 1 ;

	} else
	    rs = SR_INVALID ;

#ifdef	COMMENT
	op->s.requests += 1 ;
	if ((rs >= 0) && (rc == 0))
	    op->s.twait += 1 ;
#endif /* COMMENT */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_getfu: rs=%d rc=%d\n",rs,rc) ;
#endif

	return (rs >= 0) ? rc : rs ;
}
/* end subroutine (sspe_getfu) */


/* return our commitment information */
int sspe_info(op,ip)
SSPE		*op ;
SSPE_INFO	*ip ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_info: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	if (ip == NULL)
	    return SR_FAULT ;

	(void) memcpy(ip,&op->s,sizeof(SSPE_INFO)) ;


	return rs ;
}
/* end subroutine (sspe_info) */


int sspe_audit(op)
SSPE		*op ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i, j, k ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_audit: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = op->pip ;
	lip = op->lip ;

	{
	    struct sspe_opv	*ep ;

	    int		rs1 ;


	    for (i = 1 ; i < iclass_overlast ; i += 1) {

	        for (j = 0 ; j < lip->nfu[i] ; j += 1) {

	            for (k = 0 ; 
	                (rs1 = sshiftreg_get((op->sra[i] + j),k,&ep)) >= 0 ; 
	                k += 1) {

#if	CF_DEBUGS
	                eprintf("sspe_audit: rs=%d "
	                    "class=%u fui=%u ei=%u ep(%p)\n",
	                    rs1,i,j,k,ep) ;
	                if ((rs1 > 0) && (ep != NULL))
	                    eprintf("sspe_audit: class=%u tag=%u ia=%016llx\n",
	                        ep->class, ep->tag, ep->ia ) ;
#endif /* CF_DEBUGS */

	            }

	            if (rs1 != SR_NOTFOUND)
	                rs = rs1 ;

	            if (rs < 0)
	                break ;

	        }

	        if (rs < 0)
	            break ;

	    }

	} /* end block */

#if	CF_DEBUGS
	eprintf("sspe_audit: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sspe_audit) */



/* PRIVATE SUBROUTINES */



/* get the FU results and call back to the ASes */
static int sspe_furesults(op,pip,lip)
SSPE		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	struct sspe_opv	*ep ;

	int	rs = SR_OK, i, j ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if ((op->magic != SSPE_MAGIC) && (op->magic != 0)) {

	    eprintf("sspe_furesults: SSPE(%p) bad magic\n",op) ;

	    return SR_BADFMT ;
	}

	if (op->magic != SSPE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_furesults: entered\n") ;
#endif

	for (i = 0 ; i < iclass_overlast ; i += 1) {

	    for (j = 0 ; j < lip->nfu[i] ; j += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	        if (DEBUGLEVEL(5))
	            eprintf("sspe_furesults: class=%u fui=%u\n",i,j) ;
#endif

	        rs = sshiftreg_gethead((op->sra[i] + j),&ep) ;

/* error ? */

	        if (rs < 0)
	            break ;

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	        if (DEBUGLEVEL(5))
	            eprintf("sspe_furesults: sshiftreg_gethead() "
			"rs=%d ia=%016llx\n",
	                rs,((rs > 0) ? ep->ia : 0L)) ;
#endif

/* got one ? */

	        if ((rs > 0) && (ep->ia != 0)) {

#if	CF_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("sspe_furesults: class=%u fui=%u "
				"tag=%u ia=%016llx\n",
	                    ep->class,ep->fui,ep->tag,ep->ia) ;
#endif

	            rs = ssas_fureturn((op->ssas + ep->tag),ep) ;

#if	CF_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("sspe_furesults: ssas_fureturn() rs=%d\n",rs) ;
#endif

	            if (rs < 0)
	                break ;

	        } else
	            rs = SR_OK ;

	    } /* end for (FUs within class) */

	    if (rs < 0)
	        break ;

	} /* end for (FU classes) */

	return rs ;
}
/* end subroutine (sspe_furesults) */


static int sspe_loaddefs(op,pip,lip)
SSPE		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{

	int	i ;


#ifdef	COMMENT
	for (i = 1 ; i < iclass_overlast ; i += 1)
	    op->n.cnums[i] = op->cnums[i] ;
#else
	for (i = 1 ; i < iclass_overlast ; i += 1)
	    op->n.cnums[i] = lip->nfu[i] ;
#endif /* COMMENT */


	return SR_OK ;
}
/* end subroutine (sspe_loaddefs) */


static int sspe_finish(op,pip,lip)
SSPE		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{

	int	rs = SR_OK, i, j ;


#ifdef	COMMENT
	{
	    struct sspe_opv	v ;


	    for (i = 1 ; i < iclass_overlast ; i += 1) {

	        for (j = 0 ; j < op->n.cnums[i] ; j += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4)) {
	                eprintf("sspe_finish: sshiftreg_write() "
	                    "class=%u fui=%u\n", i,j) ;
	            }
#endif

	            v.ia = 0 ;
	            v.class = 0 ;
	            rs = sshiftreg_write((op->sra[i] + j),&v) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("sspe_finish: sshiftreg_write() rs=%d\n",
	                    rs) ;
#endif

	            if (rs < 0)
	                break ;

	        } /* end for */

	        if (rs < 0)
	            break ;

	    } /* end for */

	} /* end block */

#endif /* COMMENT */


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sspe_finish: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sspe_finish) */


#ifdef	COMMENT

static int sspe_loadcnums(op,lip)
SSPE		*op ;
struct ssinfo	*lip ;
{
	struct proginfo	*pip ;

	int	size ;


	pip = op->pip ;

	size = iclass_overlast * sizeof(int) ;
	memset(op->cnums,0,size) ;

	op->cnums[iclass_none] = INT_MAX ;
	op->cnums[iclass_ialu] = lip->npe.ialu ;
	op->cnums[iclass_imult] = lip->npe.imult ;
	op->cnums[iclass_idiv] = lip->npe.idiv ;
	op->cnums[iclass_fadd] = lip->npe.fadd ;
	op->cnums[iclass_fcmp] = lip->npe.fcmp ;
	op->cnums[iclass_fcvt] = lip->npe.fcvt ;
	op->cnums[iclass_fmult] = lip->npe.fmult ;
	op->cnums[iclass_fdiv] = lip->npe.fdiv ;
	op->cnums[iclass_fsqrt] = lip->npe.fsqrt ;
	op->cnums[iclass_ld] = lip->npe.ld ;
	op->cnums[iclass_st] = lip->npe.st ;

	op->cdelays[iclass_none] = 0 ;
	op->cdelays[iclass_ialu] = lip->lpe.ialu ;
	op->cdelays[iclass_imult] = lip->lpe.imult ;
	op->cdelays[iclass_idiv] = lip->lpe.idiv ;
	op->cdelays[iclass_fadd] = lip->lpe.fadd ;
	op->cdelays[iclass_fcmp] = lip->lpe.fcmp ;
	op->cdelays[iclass_fcvt] = lip->lpe.fcvt ;
	op->cdelays[iclass_fmult] = lip->lpe.fmult ;
	op->cdelays[iclass_fdiv] = lip->lpe.fdiv ;
	op->cdelays[iclass_fsqrt] = lip->lpe.fsqrt ;
	op->cdelays[iclass_ld] = lip->lpe.ld ;
	op->cdelays[iclass_st] = lip->lpe.st ;

	return SR_OK ;
}
/* end subroutine (sspe_loadcnums) */

#endif /* COMMENT */


/* handle a machine shift operation ! */
static int sspe_handleshift(op,pip,lip)
SSPE		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	int	rs = SR_OK ;


	if (! op->f.shift)
	    return SR_OK ;

	op->f.shift = FALSE ;


	return rs ;
}
/* end subroutine (sspe_handleshift) */


static int sspe_combstart(op,pip,lip)
SSPE		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{

	int	rs = SR_OK ;





	return rs ;
}
/* end subroutine (sspe_combstart) */


static int sspe_combend(op,pip,lip)
SSPE		*op ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{

	int	rs = SR_OK ;


#if	CF_MASTERDEBUG && CF_DEBUG && 0
	if (DEBUGLEVEL(4))
	    eprintf("sspe_combend: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sspe_combend) */


#if	(CF_MASTERDEBUG && CF_DEBUG) || F_XML

static int mkttbuf(buf,tt)
char	buf[] ;
int	tt ;
{
	int	rs ;


	if (tt < SSPE_DISPLAYTT) {

	    buf[0] = '-' ;
	    buf[1] = '\0' ;
	    rs = 2 ;

	} else
	    rs = ctdeci(buf,-1,tt) ;

	return rs ;
}

#endif /* CF_DEBUG */



