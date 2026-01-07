/* gskew */

/* this is a GSKEW branch predictor */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_SAFE		1
#define	CF_VOTEREPLACE	0		/* replace by voting among counters */
#define	CF_COUNTREPLACE	1		/* replace by counting all counters */


/* revision history:

	= 2002-05-01, David Morano

	This object module was created for Levo research.  It is a
	value predictor.  This is not coded as hardware.  It is like
	Atom analysis subroutines!


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This object module implements the GSKEW (2Bc-gskew) branch
	predictor.

	Synopsis:

	int gskew_init(op,p1,p2,p3,p4)
	GSKEW	*op ;
	int	p1, p2, p3, p4 ;

	Arguments:

	p1	table length
	p2	number of history bits


*******************************************************************************/


#define	GSKEW_MASTER	0


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/mman.h>		/* Memory Management */
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>
#include	<localmisc.h>

#include	"bpload.h"
#include	"gskew.h"


/* local defines */

#define	GSKEW_MAGIC		0x45678429
#define	GSKEW_DEFLEN		(64 * 1024)
#define	GSKEW_STATES		4
#define	GSKEW_DEFGLEN		4		/* default entries */
#define	GSKEW_DEFLPLEN	4		/* default entries */
#define	GSKEW_DEFLBLEN	4		/* default entries */
#define	GSKEW_LPHSTATES	8		/* LPHT states */
#define	GSKEW_GPHSTATES	4		/* GPHT states */
#define	GSKEW_GCHSTATES	4		/* CPHT states */

#define	MODP2(v,n)	((v) & ((n) - 1))

#define	GETPRED(c)	(((c) >> 1) & 1)
#define	GETPRED2(c)	(((c) >> 1) & 1)
#define	GETPRED3(c)	(((c) >> 2) & 1)

#define	BIT(w,n)	(((w) >> (n)) & 1)

#ifndef	ENDIAN
#if	defined(SOLARIS) && defined(__sparc)
#define	ENDIAN		1
#else
#ifdef	_BIG_ENDIAN
#define	ENDIAN		1
#endif
#ifdef	_LITTLE_ENDIAN
#define	ENDIAN		0
#endif
#ifndef	ENDIAN
#error	"could not determine endianness of this machine"
#endif
#endif
#endif


/* external subroutines */

extern uint	nextpowtwo(uint) ;

extern int	flbsi(uint) ;


/* forward references */

static uint	satcount(uint,uint,int) ;
static uint	h(int,uint), hinv(int,uint) ;
static uint	fi_bim(int,uint,uint) ;
static uint	fi_g0(int,uint,uint) ;
static uint	fi_g1(int,uint,uint) ;
static uint	fi_meta(int,uint,uint) ;


/* global variables */

struct bpload	gskew = {
	sizeof(GSKEW),
	"gskew",
} ;


/* local variables */


/* exported subroutines */


int gskew_init(op,p1,p2,p3,p4)
GSKEW	*op ;
int	p1, p2, p3, p4 ;
{
	int	rs ;
	int	size ;
	int	max ;


	if (op == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(GSKEW)) ;

	max = p1 ;
	if (max < 0)
	    max = GSKEW_DEFLEN ;

	op->tlen = nextpowtwo(max) ;

	op->tmask = op->tlen - 1 ;

	op->n = flbsi(op->tlen) ;

	op->nhist = MAX(p2,1) ;

	op->hmask = (1 << op->nhist) - 1 ;

/* allocate the space */

	size = op->tlen * sizeof(struct gskew_banks) ;
	rs = uc_malloc(size,&op->table) ;

	if (rs < 0)
	    goto bad0 ;

	(void) memset(op->table,0,size) ;


/* we're out of here */

	op->magic = GSKEW_MAGIC ;
	return rs ;

/* bad things come here */
bad0:
	return rs ;
}
/* end subroutine (gskew_init) */


/* free up this gskew object */
int gskew_free(op)
GSKEW	*op ;
{
	int		rs = SR_BADFMT ;

	if (op == NULL) return SR_FAULT ;

	if (op->magic != GSKEW_MAGIC) return SR_NOTOPEN ;

	if (op->table != NULL) {
	    uc_free(op->table) ;
	    op->table = NULL ;
	}

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (gskew_free) */


/* lookup an IA */
int gskew_lookup(op,ia)
GSKEW	*op ;
uint	ia ;
{
	uint	a ;
	uint	v, v1, v2 ;

	int	rs ;
	int	ibim, ig0, ig1, imeta ;
	int	f_meta ;
	int	f_bim, f_g0, f_g1 ;
	int	f_pred ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != GSKEW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	a = ia >> 2 ;
	v = (a << op->nhist) | (op->bhistory & op->hmask) ;

	v1 = v & op->tmask ;
	v2 = (v >> op->n) & op->tmask ;


	imeta = fi_meta(op->n,op->bhistory,a) & op->tmask ;

	ibim = fi_bim(op->n,v1,v2) ;

#if	CF_DEBUGS
	debugprintf("gskew_lookup: ibim=%d ig0=%d ig1=%d imeta=%d\n",
	    ibim,ig0,ig1,imeta) ;
#endif

	f_meta = GETPRED(op->table[imeta].meta) ;

	f_bim = GETPRED(op->table[ibim].bim) ;

/* BIM will be "UP", ESKEW will be "DOWN" */

	if (f_meta) {

	    f_pred = f_bim ;

	} else {

	    int	c ;


	    ig0 = fi_g0(op->n,v1,v2) ;

	    ig1 = fi_g1(op->n,v1,v2) ;

	    f_g0 = GETPRED(op->table[ig0].g0) ;

	    f_g1 = GETPRED(op->table[ig1].g1) ;

	    c = f_bim + f_g0 + f_g1 ;

	    f_pred = (c >= 2) ;

	}

	return f_pred ;
}
/* end subroutine (gskew_lookup) */


#ifdef	COMMENT

/* get confidence */
int gskew_confidence(op,ia)
GSKEW	*op ;
uint	ia ;
{
	uint	a ;
	uint	v, v1, v2 ;

	int	rs ;
	int	ibim, ig0, ig1, imeta ;
	int	f_meta ;
	int	f_bim, f_g0, f_g1 ;
	int	f_pred ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != GSKEW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	v = (a << op->nhist) | (op->bhistory & op->hmask) ;

	v1 = v & op->tmask ;
	v2 = (v >> op->n) & op->tmask ;


	imeta = fi_meta(op->n,op->bhistory,a) & op->tmask ;

	ibim = fi_bim(op->n,v1,v2) ;

	    ig0 = fi_g0(op->n,v1,v2) ;

	    ig1 = fi_g1(op->n,v1,v2) ;




	return pred ;
}
/* end subroutine (gskew_confidence) */

#endif /* COMMENT */


/* update on branch resolution */
int gskew_update(op,ia,f_outcome)
GSKEW	*op ;
uint	ia ;
int	f_outcome ;
{
	uint	ncount ;
	uint	a ;
	uint	v, v1, v2 ;

	int	rs ;
	int	ibim, ig0, ig1, imeta ;
	int	vote ;
	int	f_meta ;
	int	f_bim, f_eskew, f_g0, f_g1 ;
	int	f_agree ;
	int	f_pred ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != GSKEW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	a = ia >> 2 ;
	v = (a << op->nhist) | (op->bhistory & op->hmask) ;

	v1 = v & op->tmask ;
	v2 = (v >> op->n) & op->tmask ;


	imeta = fi_meta(op->n,op->bhistory,a) & op->tmask ;

	ibim = fi_bim(op->n,v1,v2) ;

	    ig0 = fi_g0(op->n,v1,v2) ;

	    ig1 = fi_g1(op->n,v1,v2) ;

#if	CF_DEBUGS
	debugprintf("gskew_update: ibim=%d ig0=%d ig1=%d imeta=%d\n",
	    ibim,ig0,ig1,imeta) ;
#endif

	{

	    f_meta = GETPRED(op->table[imeta].meta) ;

	    f_bim = GETPRED(op->table[ibim].bim) ;

	    f_g0 = GETPRED(op->table[ig0].g0) ;

	    f_g1 = GETPRED(op->table[ig1].g1) ;

	    vote = f_bim + f_g0 + f_g1 ;
	    f_eskew = (vote >= 2) ;

/* BIM will be "UP", ESKEW will be "DOWN" */

	    f_pred = (f_meta) ? f_bim : f_eskew ;

	}

/* do the updating */

	f_agree = LEQUIV(f_bim,f_g0) && LEQUIV(f_bim,f_g1) ;
	if (LEQUIV(f_outcome,f_pred) && (! f_agree)) {

	    if (! LEQUIV(f_bim,f_eskew)) {

/* strengthen META */

	        ncount = 
	            satcount(op->table[imeta].meta,GSKEW_STATES,f_meta) ;

	        op->table[imeta].meta = ncount ;

	    }

/* strengthen others */

	    if (f_meta) {

/* strengthen BIM */

	        ncount = 
	            satcount(op->table[ibim].bim,GSKEW_STATES,f_outcome) ;

	        op->table[ibim].bim = ncount ;


	    } else {

/* strengthen correct tables */

	        if (LEQUIV(f_bim,f_outcome)) {

	            ncount = 
	                satcount(op->table[ibim].bim,
			GSKEW_STATES,f_outcome) ;

	            op->table[ibim].bim = ncount ;

	        }

	        if (LEQUIV(f_g0,f_outcome)) {

	            ncount = 
	                satcount(op->table[ig0].g0,GSKEW_STATES,f_outcome) ;

	            op->table[ig0].g0 = ncount ;

	        }

	        if (LEQUIV(f_g1,f_outcome)) {

	            ncount = 
	                satcount(op->table[ig1].g1,GSKEW_STATES,f_outcome) ;

	            op->table[ig1].g1 = ncount ;

	        }

	    }

	} else {

	    if (! LEQUIV(f_bim,f_eskew)) {

/* strengthen META */

	        ncount = 
	            satcount(op->table[imeta].meta,GSKEW_STATES,f_meta) ;

	        op->table[imeta].meta = ncount ;

/* re-compute */

	        {

	            f_meta = GETPRED(op->table[imeta].meta) ;

	            f_bim = GETPRED(op->table[ibim].bim) ;

	            f_g0 = GETPRED(op->table[ig0].g0) ;

	            f_g1 = GETPRED(op->table[ig1].g1) ;

	            vote = f_bim + f_g0 + f_g1 ;
	            f_eskew = (vote >= 2) ;

/* BIM will be "UP", ESKEW will be "DOWN" */

	            f_pred = (f_meta) ? f_bim : f_eskew ;

	        }

	        if (LEQUIV(f_pred,f_outcome)) {

/* "strengthen participating tables" */

	            ncount = satcount(op->table[ibim].bim,
			GSKEW_STATES,f_outcome) ;

	            op->table[ibim].bim = ncount ;

	            if (! f_meta) {

	                ncount = satcount(op->table[ig0].g0,
				GSKEW_STATES,f_outcome) ;

	                op->table[ig0].g0 = ncount ;

	                ncount = satcount(op->table[ig1].g1,
				GSKEW_STATES,f_outcome) ;

	                op->table[ig1].g1 = ncount ;

	            }

	        } else {

/* update all "banks" */

	            ncount = satcount(op->table[ibim].bim,
				GSKEW_STATES,f_outcome) ;

	            op->table[ibim].bim = ncount ;

	            ncount = 
	                satcount(op->table[ig0].g0,GSKEW_STATES,f_outcome) ;

	            op->table[ig0].g0 = ncount ;

	            ncount = 
	                satcount(op->table[ig1].g1,GSKEW_STATES,f_outcome) ;

	            op->table[ig1].g1 = ncount ;

	        }

	    }

	} /* end if (prediction/misprediction) */

/* update global branch history register */

	op->bhistory = (op->bhistory << 1) | f_outcome ;


	return 0 ;
}
/* end subroutine (gskew_update) */


/* zero out the statistics */
int gskew_zerostats(op)
GSKEW		*op ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != GSKEW_MAGIC)
	    return SR_NOTOPEN ;

	(void) memset(&op->s,0,sizeof(GSKEW_STATS)) ;

	return rs ;
}
/* end subroutine (gskew_zerostats) */


/* get the statistics about this particular predictor */
int gskew_stats(op,rp)
GSKEW		*op ;
GSKEW_STATS	*rp ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != GSKEW_MAGIC)
	    return SR_NOTOPEN ;

	if (rp == NULL)
	    return SR_FAULT ;

	(void) memcpy(rp,&op->s,sizeof(GSKEW_STATS)) ;

/* fill in the regular stuff */

	rp->tlen = op->tlen ;

/* calculate the bits */

	{
	    uint	bits_bim ;
	    uint	bits_g0 ;
	    uint	bits_g1 ;
	    uint	bits_meta ;
	    uint	bits_history ;



	    bits_bim = op->tlen * 2 ;

	    bits_g0 = op->tlen * 2 ;

	    bits_g1 = op->tlen * 2 ;

	    bits_meta = op->tlen * 2 ;

	    rp->bits = 
	        bits_bim + bits_g0 + bits_g1 + bits_meta + bits_history ;

	}

	return rp->bits ;
}
/* end subroutine (gskew_stats) */



/* INTERNAL SUBROUTINES */



static uint satcount(v,n,f_up)
uint	v ;
uint	n ;
int	f_up ;
{
	uint	r ;


	if (f_up)
	    r = (v == (n - 1)) ? v : (v + 1) ;

	else 
	    r = (v == 0) ? 0 : (v - 1) ;

	return r ;
}
/* end subroutine (satcount) */


/* index function for the BIM */
static uint fi_bim(n,h,a)
int	n ;
uint	h, a ;
{


	return a ;
}
/* end subroutine (fi_bim) */


static uint fi_g0(n,v1,v2)
int	n ;
uint	v1, v2 ;
{


	return h(n,v1) ^ hinv(n,v2) ^ v1 ;
}
/* end subroutine (fi_g0) */


static uint fi_g1(n,v1,v2)
int	n ;
uint	v1, v2 ;
{


	return hinv(n,v1) ^ h(n,v2) ^ v2 ;
}
/* end subroutine (fi_g1) */


static uint fi_meta(n,v1,v2)
int	n ;
uint	v1, v2 ;
{


	return h(n,v1) ^ hinv(n,v2) ^ v2 ;
}
/* end subroutine (fi_meta) */


/* forward H function */
static uint h(n,v)
int	n ;
uint	v ;
{


	return (v >> 1) || ((BIT(v,(n - 1)) ^ (v & 1)) << (n - 1)) ;
}
/* end subroutine (h) */


/* inverse H function */
static uint hinv(n,v)
int	n ;
uint	v ;
{


	return (v << 1) || (BIT(v,(n - 1)) ^ BIT(v,(n - 2))) ;
}
/* end subroutine (hinv) */


