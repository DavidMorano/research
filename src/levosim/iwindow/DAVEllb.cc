/* llb */

/* Levo Load Buffer */


#define	F_DEBUGS	0
#define	F_DEBUG		1


/* revision history :

	= 00/07/14, Alireza Khalafi

	This code was started basically from scratch !


	= 01/05/09, Dave Morano

	I modified it to be more like a FIFO.  We need to change
	the interface to be more like a FIFO in the future for
	when we actually implement a FIFO here.


*/



/**************************************************************************

	This object module provides the functions for a Levo Load
	Buffer.


**************************************************************************/


#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"levoinfo.h"
#include	"llb.h"
#include        "ldecodeinstrclass.h"
#include        "lexec.h"



/* local defines */

#define	LLB_MAGIC	0x84736245
#define	LLB_MAXENTRIES	1



/* external subroutines */


/* forward references */

static int	llb_checkload(LLB *,struct proginfo *,struct levoinfo *) ;
static int	llb_handleshift(LLB *,struct proginfo *,struct levoinfo *) ;






/* initialize this object */
int llb_init(llbp,pip,mip,lip)
LLB		*llbp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
{
	int	rs, i ;
	int	size ;


	(void) memset(llbp,0,sizeof(LLB)) ;

	llbp->pip = pip ;
	llbp->mip = mip ;
	llbp->lip = lip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("llb_init: entered\n") ;
#endif

	llbp->nentries = lip->totalrows;

	llbp->npred = lip->nprpas;
	if (llbp->npred < 1)
		llbp->npred = 1 ;

	size = llbp->nentries * sizeof(struct llb_entry) ;
	llbp->c.entry = 
		(struct llb_entry *) malloc(size) ;

	llbp->n.entry = 
		(struct llb_entry *) malloc(size) ;

	for (i = 0 ; i < llbp->nentries ; i += 1) {

		size = llbp->npred * sizeof(int) ;
		llbp->c.entry[i].cpina = (int *) malloc(size) ;

		assert(llbp->c.entry[i].cpina);

		llbp->c.entry[i].cpinv = (int *) malloc(size) ;

		assert(llbp->c.entry[i].cpinv);


		size = llbp->npred * sizeof(int) ;
		llbp->n.entry[i].cpina = (int *) malloc(size) ;

		assert(llbp->n.entry[i].cpina);

		llbp->n.entry[i].cpinv = (int *) malloc(size) ;

		assert(llbp->n.entry[i].cpinv);


		llbp->c.entry[i].ncpin = llbp->n.entry[i].ncpin = 0 ;
		llbp->c.entry[i].valid = llbp->n.entry[i].valid = 0 ;

	}  /* end for */

	llbp->c.count = llbp->n.count = 0 ;

	llbp->readyread = 0;
	llbp->load_enable = 0;


#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("llb_init: exited, rows=%d\n",llbp->nentries) ;
#endif

	llbp->magic = LLB_MAGIC ;
	return SR_OK ;
}
/* end subroutine (llb_init) */


/* free up this object */
int llb_free(llbp)
LLB	*llbp ;
{
	struct proginfo	*pip ;


	pip = llbp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("llb_free: entered\n") ;
#endif


	if (llbp->c.entry != NULL)
		free(llbp->c.entry) ;

	if (llbp->n.entry != NULL)
		free(llbp->n.entry) ;


	return SR_OK ;
}
/* end subroutine (llb_free) */


/* do our combinatorial stuff */
int llb_comb(llbp,phase)
LLB	*llbp ;
int	phase ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


	if (llbp == NULL)
		return SR_FAULT ;

	pip = llbp->pip ;
	lip = llbp->lip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("llb_comb: entered\n") ;
#endif

	switch (phase) {

	case 0:
		llbp->f.invalidate = FALSE ;
		break ;

	case 3:
		rs = llb_checkload(llbp,pip,lip) ;

		if (rs < 0)
			break ;

		rs = llb_handleshift(llbp,pip,lip) ;

		break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (llb_comb) */


/* we just got clocked */
int llb_clock(llbp)
LLB	*llbp ;
{
	struct proginfo	*pip ;

	struct llb_entry	*tce ;

	int	j,i,*ta,*tv;


	if (llbp == NULL)
		return SR_FAULT ;

	pip = llbp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("llb_clock: entered, f_loadenable=%d\n",
		llbp->load_enable) ;
#endif

/* transition some state */

	llbp->c.count = llbp->n.count ;


/* some other stuff */

	if (! llbp->load_enable)	
		return SR_OK;

/* transition the first-order state */

	tce = llbp->c.entry;
	llbp->c = llbp->n;
	llbp->c.entry = tce;

/* transition the rest of the state */

	for (i=0;i<llbp->lip->totalrows; i += 1) {

		ta = llbp->c.entry[i].cpina;
		tv = llbp->c.entry[i].cpinv;
		llbp->c.entry[i] = llbp->n.entry[i];
		llbp->c.entry[i].cpina = ta;
		llbp->c.entry[i].cpinv = tv;

		for (j = 0 ; j < llbp->npred ; j += 1) {

			* (llbp->c.entry[i].cpina + j) = 
				* (llbp->n.entry[i].cpina + j);
			* (llbp->c.entry[i].cpinv + j) = 
				* (llbp->n.entry[i].cpinv + j);

		}

	} /* end for */

	llbp->readyread = 1;
	llbp->load_enable = 0;

	return SR_OK ;
}
/* end subroutine (llb_clock) */


/* combinatorial signal to shift our time-tag values */
int llb_shift(llbp)
LLB	*llbp ;
{


	if (llbp == NULL)
		return SR_FAULT ;

	llbp->f.shift = TRUE ;
	return SR_OK ;
}


/* Use this function to insert entries into the load buffer */
int 
llb_load(llbp, index, faddr, instr,oopexec,oopclass,opexec, opclass, btype,
	src1, src2, src3, src4, dst1, dst2, 
	const1, const1_valid, pina, cpina, ncpin,
	pouta,cpouta,pinv,cpinv,dyntaken)
LLB	*llbp;
int	index;	/* index of the entry in the load buffer */
uint	faddr;
uint	instr ;
int	oopexec, oopclass ;	/* originals */
int	opexec, opclass;
uint	btype ;
int src1, src2, src3, src4; /* address of the source registers */
int dst1, dst2;		    /* address of the destination register */
int const1, const1_valid;
int pina;	/* address of pin regirster */
int *cpina;	/* addresses of cpin registers , NULL if none */
int ncpin;	/* number of cpin registers */
int pouta;	/* address of pout reg (-1 means no output predicate) */
int cpouta;	/* address of cpout reg (-1 means no canceling predicate) */
int pinv;	/* initial predicate value for input predicate register */
int *cpinv;	/* initial predicate values for input canceling predicate */
int dyntaken;  /* loaded instructions follow dynamic path */ 
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	struct llb_entry	*ep ;

	int	i;


	if (llbp == NULL)
		return SR_FAULT ;

	pip = llbp->pip ;
	lip = llbp->lip ;

	if (llbp->load_enable)
		return SR_INVALID ;

	if ((index < 0) || (index >= lip->totalrows))
		return SR_INVALID ;	

	ep=&llbp->n.entry[index];
	ep->valid=1;

/* the original instruction information */

	ep->faddr = faddr;
	ep->instr = instr ;
	ep->btype = btype ;
	ep->oopexec = oopexec ;
	ep->oopclass = oopclass ;

/* sources and destinations */

	ep->src1 = src1;
	ep->src2 = src2;
	ep->src3 = src3;
	ep->src4 = src4;
	ep->dst1 = dst1;
	ep->dst2 = dst2;

/* whatever ? */

	ep->const1 = const1;
	ep->const1_valid = const1_valid;

	if (opclass != INSTR_UNUSED) {

	if (ncpin > llbp->npred)
		return SR_NOTSUP ;

	ep->pina = pina;
	ep->pinv = pinv;
	ep->ncpin = ncpin;

	for (i = 0 ; i < ncpin ; i += 1) {

		ep->cpina[i] = cpina[i];
		ep->cpinv[i] = cpinv[i];

	} /* end for */

	for ( ; i < llbp->npred ; i += 1) {

		ep->cpina[i] = 0 ;
		ep->cpinv[i] = 0 ;

	} /* end for */

	ep->pouta = pouta;
	ep->cpouta = cpouta;

	} else {

	ep->pina = 0 ;
	ep->pinv = 0 ;
	ep->ncpin = 0 ;

	for (i = 0 ; i < llbp->npred ; i += 1) {

		ep->cpina[i] = 0 ;
		ep->cpinv[i] = 0 ;

	} /* end for */

	ep->pouta = pouta;
	ep->cpouta = cpouta;

	} /* end if (slot used or not) */

/* transformed instruction information */

	ep->opexec = opexec;
	ep->opclass = opclass;

	return SR_OK ;
}
/* end subroutine (llb_load) */


/* 

call this function when all the entries are filled with instructions
and the buffer is ready to be loaded into active stations.

*/

int llb_loadenable(llbp)
LLB *llbp;
{
	int	rs ;


	if (llbp == NULL)
		return SR_FAULT ;

	rs = llb_load_complete(llbp);

	return rs ;
}
/* end subroutine (llb_loadenable) */


/* this is similar to a FIFO write */
int
llb_load_complete(llbp)
LLB	*llbp;
{


	llbp ->load_enable = 1;

	llbp->n.count += 1 ;
	return llbp->n.count ;
}


/* this is similar to a FIFO readout */
int
llb_read_complete(llbp)
LLB	*llbp;
{


	if (llbp == NULL)
		return SR_FAULT ;

	llbp->readyread = 0;
	if (llbp->c.count == 0)
		return SR_OVERFLOW ;

	llbp->n.count -= 1 ;
	return llbp->n.count ;
}
/* end subroutine (llb_read_complete) */


/* return the current count of entries deep */
llb_count(llbp)
LLB	*llbp ;
{


	if (llbp == NULL)
		return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
		return SR_NOTOPEN ;

	return llbp->c.count ;
}
/* end subroutine (llb_count) */


/* is the input ready to accept a new shift-in ? */
int llb_inready(llbp)
LLB	*llbp ;
{


	if (llbp == NULL)
		return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
		return SR_NOTOPEN ;

	return (llbp->c.count < LLB_MAXENTRIES) ;
}
/* end subroutine (llb_inready) */


/* is the buffer ready to read ? */
int llb_readyread(llbp)
LLB	*llbp;
{


	if (llbp == NULL)
		return SR_FAULT ;

	return llbp->readyread ;
}
/* end subroutine (llb_readyread) */


int llb_read(llbp, opexec, opclass, faddr, src1, src2, src3, src4, dst1, dst2, const1, const1_valid, pin, pinv, cpin, cpinv, ncpin, pout, cpout, index)
LLB	*llbp;
int *opexec, *opclass;
uint *faddr;
int *src1, *src2, *src3, *src4;
int *dst1, *dst2;
int *const1, *const1_valid;
int *pin;
int *pinv;
int **cpin;	/* pointer to an array of cpin register addresses */
int **cpinv;	/* pointer to an array of cpin register values */
int *ncpin;
int *pout;
int *cpout;
int	index;	/* index to the load buffer entry to be read */
{
	struct llb_entry	*ep ;

	int	i;


	if (llbp == NULL)
		return SR_FAULT ;

	if (! llbp->readyread)
		return SR_PROTO ;

	assert(index >= 0 && index < llbp->lip->totalrows);

	ep = &llbp->c.entry[index];
	*src1 = ep->src1;
	*src2 = ep->src2;
	*src3 = ep->src3;
	*src4 = ep->src4;
	*dst1 = ep->dst1;
	*dst2 = ep->dst2;
	*const1 = ep->const1;
	*const1_valid = ep->const1_valid;
	*faddr = ep->faddr;
	*opexec = ep->opexec;
	*opclass = ep->opclass;
	*pin = ep->pina;
	*pinv = ep->pinv;
	*pout = ep->pouta;
	*ncpin = ep->ncpin;
	*cpout = ep->cpouta;
	*cpin = ep->cpina;
	*cpinv = ep->cpinv;

	return SR_OK;
}
/* end subroutine (llb_read) */


/* return some instruction information */
int llb_instrinfo(llbp,i,iip)
LLB		*llbp ;
int		i ;
LLB_INSTRINFO	*iip ;
{
	struct llb_entry	*ep ;


	if ((llbp == NULL) || (iip == NULL))
		return SR_FAULT ;

	if ((i < 0) || (i >= llbp->nentries))
		return SR_INVALID ;

	ep = llbp->c.entry + i ;

	iip->ia = ep->faddr ;
	iip->instr = ep->instr ;
	iip->btype = ep->btype ;	/* Marcos-syle i-fetch branch type */
	iip->opexec = ep->oopexec ;
	iip->opclass = ep->oopclass ;

	return SR_OK ;
}
/* end subroutine (llb_instrinfo) */


/* 

Call this function when you have loaded all AS's from load buffer.  It
makes the load buffer ready for a new pack of instructions to be loaded
from decode stage.

*/

int llb_is_free(llbp)
LLB	*llbp;
{


	if (llbp == NULL)
		return SR_FAULT ;

	if (llbp->load_enable) 
		return 0;

	return 1;
}
/* end subroutine (llb_is_free) */


/* get the current instruction address of an entry (if any) */
int llb_getia(llbp, index, address)
LLB	*llbp;
int	index;
uint	*address;
{
	struct llb_entry	*ep ;


	if (llbp == NULL)
		return SR_FAULT ;

	ep = &llbp->c.entry[index];
	if (! ep->valid) 
		return SR_EMPTY;

	*address = ep->faddr ;
	return 1 ;
}
/* end subroutine (llb_getia) */


/* invalidate the load buffer on the NEXT clock if not loaded in this clock */
int llb_invalidate(llbp)
LLB	*llbp;
{
	int i;


	if (llbp == NULL)
		return SR_FAULT ;

	llbp->f.invalidate = TRUE ;
	if (llbp->n.count > 0)
		llbp->n.count = 0 ;

	return SR_OK ;
}
/* end subroutine (llb_invalidate) */


/* is a row entry used ? */
int llb_used(llbp, index)
LLB	*llbp;
int	index;
{
	struct llb_entry	*ep ;

	int	rs = SR_OK, i;


	if ((index<0) || (index >=llbp->lip->totalrows))
		return SR_INVALID ;

#ifdef	COMMENT
	if (llbp->c.entry[index].valid != 0) {

	switch (llbp->c.entry[index].opclass) {

		case INSTR_BREL:
			rs = 3; /* conditional branch */
			break ;

		default:
			rs = 2; /* regular instruction */
			break ;

	} /* end switch */

	} else
		rs = 0 ;
#else
	if (! llbp->c.entry[index].valid)
		return SR_EMPTY ;

	switch (llbp->c.entry[index].opclass) {

		case INSTR_BREL:
			rs = 3; /* conditional branch */
			break ;

		default:
			rs = 2; /* regular instruction */
			break ;

	} /* end switch */

#endif /* COMMENT */

	return rs ;
}
/* end subroutine (llb_used) */



/* PRIVATE SUBROUTINES */



/* handle the combinatorial logic of a machine shift */
static int llb_checkload(llbp,pip,lip)
LLB		*llbp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs, i ;


	if (llbp->load_enable)
		return SR_OK ;

	if (llbp->f.invalidate) {

	for (i = 0 ; i < lip->totalrows ; i += 1)
		llbp->n.entry[i].valid = FALSE ;

	} else {

/* keep the same stuff as is current for next clock also */

	for (i = 0 ; i < lip->totalrows ; i += 1)
		llbp->n.entry[i] = llbp->c.entry[i] ;

	}

	return SR_OK ;
}
/* end subroutine (iw_checkload) */


/* handle the combinatorial logic of a machine shift */
static int llb_handleshift(llbp,pip,lip)
LLB		*llbp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs, i ;
	int	totalrows ;


#ifndef	COMMENT

	if (! llbp->f.shift)
		return SR_OK ;

	llbp->f.shift = FALSE ;

	totalrows = lip->totalrows ;
	for (i = 0 ; i < totalrows ; i += 1) {

/* NOTHING TO SHIFT !!! (for now) */

	} /* end for */

#endif /* COMMENT */


	return SR_OK ;
}
/* end subroutine (llb_handleshift) */



int
llb_print(LLB * llbp) {

	int i;

	printf("load buffer content\n");
	for(i=0;i<llbp->lip->totalrows; ++i) {
		printf("src1=%d, src2=%d, dst1=%d\n", llbp->c.entry[i].src1, llbp->c.entry[i].src2, llbp->c.entry[i].dst1);
	}

}

#ifdef COMMENT
int llb_dummyload(LLB * llbp) {

	int cp2[10],cp4[10];
	int cp[100][10];


llb_load_complete(llbp);
}
#endif



