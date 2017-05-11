/* llb */

/* Levo Load Buffer */


#define	F_DEBUGS	0
#define	F_DEBUG		0
#define	F_SAFE		1
#define	F_SAFE2		1


/* revision history :

	= 00/07/14, Alireza Khalafi

	This code was started basically from scratch !


	= 01/04/23, Dave Morano

	I fixed bad bad stray-write bug that was occurring with the
	predicate register cells in the row entries (not enough of them).


	= 01/05/09, Dave Morano

	I modified it to be more like a FIFO.  We need to change
	the interface to be more like a FIFO in the future for
	when we actually implement a FIFO here.


	= 01/06/22, Dave Morano

	I changed the way the memory allocations are done at start-up
	to find what is causing a core dump.

	I also fixed a bug with not freeing memory on object destruction.


	= 01/09/26, Dave Morano

	Added stuff for an XML output state trace.


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
#include	"mipsdis.h"

#include	"levoinfo.h"
#include	"instrclass.h"
#include	"llb.h"
#include        "ldecodeinstrclass.h"
#include        "lexec.h"



/* local defines */

#define	LLB_MAGIC	0x84736245
#define	LLB_MAXENTRIES	1

#define	INSTRDISLEN	100



/* external subroutines */


/* forward references */

static int	llb_checkload(LLB *,struct proginfo *,struct levoinfo *) ;
static int	llb_handleshift(LLB *,struct proginfo *,struct levoinfo *) ;

static int	isbadp(void *) ;





/* initialize this object */
int llb_init(llbp,pip,mip,lip)
LLB		*llbp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
{
	int	rs, i, n ;
	int	size ;


	(void) memset(llbp,0,sizeof(LLB)) ;

	llbp->pip = pip ;
	llbp->mip = mip ;
	llbp->lip = lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: entered\n") ;
#endif

	llbp->nentries = lip->totalrows ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: nentries=%d\n",llbp->nentries) ;
#endif

	llbp->npred = lip->nprpas ;
	if (llbp->npred < 1)
	    llbp->npred = 1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: nentries=%d npred=%d\n",
	        llbp->nentries,llbp->npred) ;
#endif

	size = 2 * llbp->nentries * sizeof(struct llb_entry) ;
	rs = uc_malloc(size,(void **) &llbp->a.entry) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: 1 uc_malloc() rs=%d ENTRY=%p\n",
			rs,llbp->a.entry) ;
#endif

	if (rs < 0)
	    goto bad0 ;

	llbp->c.entry = llbp->a.entry + 0 ;
	llbp->n.entry = llbp->a.entry + llbp->nentries ;

	(void) memset(llbp->a.entry,0,size) ;

/* allocate some predicate storage cells */

	    size = llbp->npred * sizeof(int) ;
	for (i = 0 ; i < llbp->nentries ; i += 1) {

/* the A things */

	    llbp->c.entry[i].cpina = (int *) malloc(size) ;

	    if (llbp->c.entry[i].cpina == NULL)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: C.ENTRY[%d].cpina=%p\n",
	    	i,llbp->c.entry[i].cpina) ;
#endif

	    llbp->n.entry[i].cpina = (int *) malloc(size) ;

	    if (llbp->n.entry[i].cpina == NULL) {

	        free(llbp->c.entry[i].cpina) ;

	        break ;
	    }

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: N.ENTRY[%d].cpina=%p\n",
	    	i,llbp->n.entry[i].cpina) ;
#endif

/* the V things */

	    llbp->c.entry[i].cpinv = (int *) malloc(size) ;

	    if (llbp->c.entry[i].cpinv == NULL) {

	        free(llbp->n.entry[i].cpina) ;

	        free(llbp->c.entry[i].cpina) ;

	        break ;
	    }

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: C.ENTRY[%d].cpinv=%p\n",
	    	i,llbp->c.entry[i].cpinv) ;
#endif

	    llbp->n.entry[i].cpinv = (int *) malloc(size) ;

	    if (llbp->n.entry[i].cpinv == NULL) {

	        free(llbp->c.entry[i].cpinv) ;

	        free(llbp->n.entry[i].cpina) ;

	        free(llbp->c.entry[i].cpina) ;

	        break ;
	    }

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: N.ENTRY[%d].cpinv=%p\n",
	    	i,llbp->n.entry[i].cpinv) ;
#endif

/* initialize them */

	    llbp->c.entry[i].ncpin = llbp->n.entry[i].ncpin = 0 ;
	    llbp->c.entry[i].valid = llbp->n.entry[i].valid = 0 ;

	} /* end for */

	n = i ;
	if (n < llbp->nentries) {

	    for (i = 0 ; i < n ; i += 1) {

	        free(llbp->n.entry[i].cpinv) ;

	        free(llbp->c.entry[i].cpinv) ;

	        free(llbp->n.entry[i].cpina) ;

	        free(llbp->c.entry[i].cpina) ;

	    } /* end for */

	    rs = SR_NOMEM ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	        eprintf("llb_init: bad predicate allocations rs=%d\n",rs) ;
#endif

	    goto bad1 ;

	} /* end if (error in allocations) */

/* initialize other stuff */

	llbp->c.count = llbp->n.count = 0 ;

	llbp->readyread = 0 ;
	llbp->load_enable = 0 ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_init: exiting, rows=%d\n",llbp->nentries) ;
#endif

	llbp->magic = LLB_MAGIC ;
	return SR_OK ;

bad1:
	free(llbp->a.entry) ;

bad0:
	return rs ;
}
/* end subroutine (llb_init) */


/* free up this object */
int llb_free(llbp)
LLB	*llbp ;
{
	struct proginfo	*pip ;

	int	rs, i, n ;


	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;

	pip = llbp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_free: entered\n") ;
#endif

	for (i = 0 ; i < llbp->nentries ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4) {
	    eprintf("llb_init: N.ENTRY[%d].cpinv=%p\n",
	    	i,llbp->n.entry[i].cpinv) ;
	    eprintf("llb_init: C.ENTRY[%d].cpinv=%p\n",
	    	i,llbp->c.entry[i].cpinv) ;
	    eprintf("llb_init: N.ENTRY[%d].cpinv=%p\n",
	    	i,llbp->n.entry[i].cpina) ;
	    eprintf("llb_init: C.ENTRY[%d].cpina=%p\n",
	    	i,llbp->c.entry[i].cpina) ;
	}
#endif

	    free(llbp->n.entry[i].cpinv) ;

	    free(llbp->c.entry[i].cpinv) ;

	    free(llbp->n.entry[i].cpina) ;

	    free(llbp->c.entry[i].cpina) ;

	} /* end for */

	if (llbp->a.entry != NULL)
	    free(llbp->a.entry) ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_free: exiting\n") ;
#endif

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
	int	i ;


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = llbp->pip ;
	lip = llbp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_comb: entered\n") ;
#endif

/* do some tests */

#if	F_MASTEDEBUG && F_SAFE && F_SAFE2
	if (pip->debuglevel >= 2) {

		int	f_bad = FALSE ;


		if (llbp->c.entry == NULL)
			f_bad = TRUE ;

		if (llbp->nentries > 4) {

			eprintf("llb_comb: bad nentries=%d\n",
				llbp->nentries) ;

			return SR_BADFMT ;

		}
	
		for (i = 0 ; i < llbp->nentries ; i += 1) {

		if (isbadp(llbp->c.entry[i].cpina))
			f_bad = TRUE ;
		
		if (isbadp(llbp->n.entry[i].cpina))
			f_bad = TRUE ;
		
		if (isbadp(llbp->c.entry[i].cpinv))
			f_bad = TRUE ;
		
		if (isbadp(llbp->n.entry[i].cpinv))
			f_bad = TRUE ;

		if (f_bad)
			break ;

		} /* end for */

		if (f_bad) {

			eprintf("llb_comb: bad pointer\n") ;
			return SR_BADFMT ;

		}
	}
#endif /* F_SAFE2 */

/* do our normal work */

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

	int	j,i,*ta,*tv ;


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = llbp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_clock: entered, f_loadenable=%d\n",
	        llbp->load_enable) ;
#endif

/* transition some state */

	llbp->c.count = llbp->n.count ;


/* some other stuff */

	if (! llbp->load_enable)
	    return SR_OK ;

/* transition the first-order state */

	tce = llbp->c.entry ;
	llbp->c = llbp->n ;
	llbp->c.entry = tce ;

/* transition the rest of the state */

	for (i=0;i<llbp->lip->totalrows; i += 1) {

	    ta = llbp->c.entry[i].cpina ;
	    tv = llbp->c.entry[i].cpinv ;
	    llbp->c.entry[i] = llbp->n.entry[i] ;
	    llbp->c.entry[i].cpina = ta ;
	    llbp->c.entry[i].cpinv = tv ;

	    for (j = 0 ; j < llbp->npred ; j += 1) {

	        * (llbp->c.entry[i].cpina + j) = 
	            * (llbp->n.entry[i].cpina + j) ;
	        * (llbp->c.entry[i].cpinv + j) = 
	            * (llbp->n.entry[i].cpinv + j) ;

	    }

	} /* end for */

	llbp->readyread = 1 ;
	llbp->load_enable = 0 ;

	return SR_OK ;
}
/* end subroutine (llb_clock) */


/* combinatorial signal to shift our time-tag values */
int llb_shift(llbp)
LLB	*llbp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	llbp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (llb_shift) */


/* Use this function to insert entries into the load buffer */
int llb_load(llbp, index, faddr, 
	instr,oopexec,oopclass,opexec, opclass, btype,
	src1, src2, src3, src4, src5,
	dst1, dst2, dst3,
	const1, const1_valid, 
	pina, cpina, ncpin, pouta,cpouta,pinv,cpinv,dyntaken)
LLB	*llbp ;
int	index;	/* index of the entry in the load buffer */
uint	faddr ;
uint	instr ;
uint	oopexec, oopclass ;	/* originals */
uint	opexec, opclass ;
uint	btype ;
uint	src1, src2, src3, src4, src5 ;
uint	dst1, dst2, dst3 ;
uint const1, const1_valid ;
uint pina;	/* address of pin regirster */
uint *cpina;	/* addresses of cpin registers , NULL if none */
uint ncpin;	/* number of cpin registers */
uint pouta;	/* address of pout reg (-1 means no output predicate) */
uint cpouta;	/* address of cpout reg (-1 means no canceling predicate) */
uint pinv;	/* initial predicate value for input predicate register */
uint *cpinv;	/* initial predicate values for input canceling predicate */
int dyntaken;  /* loaded instructions follow dynamic path */
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	struct llb_entry	*ep ;

	int	i ;


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = llbp->pip ;
	lip = llbp->lip ;

	if (llbp->load_enable)
	    return SR_INVALID ;

	if ((index < 0) || (index >= lip->totalrows))
	    return SR_INVALID ;

	ep=&llbp->n.entry[index] ;
	ep->valid=1 ;

/* the original instruction information */

	ep->faddr = faddr ;
	ep->instr = instr ;
	ep->btype = btype ;
	ep->oopexec = oopexec ;
	ep->oopclass = oopclass ;

/* sources and destinations */

	ep->src1 = src1 ;
	ep->src2 = src2 ;
	ep->src3 = src3 ;
	ep->src4 = src4 ;
	ep->dst1 = dst1 ;
	ep->dst2 = dst2 ;
	ep->dst3 = dst3 ;

/* whatever ? */

	ep->const1 = const1 ;
	ep->const1_valid = const1_valid ;

	if (opclass != INSTR_UNUSED) {

	    if (ncpin > llbp->npred)
	        return SR_NOTSUP ;

	    ep->pina = pina ;
	    ep->pinv = pinv ;
	    ep->ncpin = ncpin ;

	    for (i = 0 ; i < ncpin ; i += 1) {

	        ep->cpina[i] = cpina[i] ;
	        ep->cpinv[i] = cpinv[i] ;

	    } /* end for */

	    for ( ; i < llbp->npred ; i += 1) {

	        ep->cpina[i] = 0 ;
	        ep->cpinv[i] = 0 ;

	    } /* end for */

	    ep->pouta = pouta ;
	    ep->cpouta = cpouta ;

	} else {

	    ep->pina = 0 ;
	    ep->pinv = 0 ;
	    ep->ncpin = 0 ;

	    for (i = 0 ; i < llbp->npred ; i += 1) {

	        ep->cpina[i] = 0 ;
	        ep->cpinv[i] = 0 ;

	    } /* end for */

	    ep->pouta = pouta ;
	    ep->cpouta = cpouta ;

	} /* end if (slot used or not) */

/* transformed instruction information */

	ep->opexec = opexec ;
	ep->opclass = opclass ;

	if (opclass == INSTR_UNUSED) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	        eprintf("llb_load: row=%d UNUSED\n",
	            index) ;
#endif

	    llbp->s.load_unused += 1 ;

	} else {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	        eprintf("llb_load: row=%d ia=%08x instr=%08x\n",
	            index, faddr, instr) ;
#endif

	    llbp->s.load_instr += 1 ;

	}

	return SR_OK ;
}
/* end subroutine (llb_load) */


/* 

call this function when all the entries are filled with instructions
and the buffer is ready to be loaded into active stations.

*/

int llb_loadenable(llbp)
LLB *llbp ;
{
	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	rs = llb_load_complete(llbp) ;

	return rs ;
}
/* end subroutine (llb_loadenable) */


/* this is similar to a FIFO write */
int llb_load_complete(llbp)
LLB	*llbp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	llbp ->load_enable = 1 ;

	llbp->s.load_col += 1 ;

	llbp->n.count += 1 ;
	return llbp->n.count ;
}
/* end subroutine (llb_load_complete) */


/* this is similar to a FIFO readout */
int llb_read_complete(llbp)
LLB	*llbp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	llbp->readyread = 0 ;
	if (llbp->c.count == 0)
	    return SR_OVERFLOW ;

	llbp->n.count -= 1 ;
	return llbp->n.count ;
}
/* end subroutine (llb_read_complete) */


/* return the current count of entries deep */
int llb_count(llbp)
LLB	*llbp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	return llbp->c.count ;
}
/* end subroutine (llb_count) */


/* is the input ready to accept a new shift-in ? */
int llb_inready(llbp)
LLB	*llbp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	return (llbp->c.count < LLB_MAXENTRIES) ;
}
/* end subroutine (llb_inready) */


/* is the buffer ready to read ? */
int llb_readyread(llbp)
LLB	*llbp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	return llbp->readyread ;
}
/* end subroutine (llb_readyread) */


int llb_read(llbp, opexec, opclass, faddr, src1, src2, src3, src4, src5,
		dst1, dst2, dst3, const1, const1_valid, pin, pinv,
		cpin, cpinv, ncpin, pout, cpout, index)
LLB	*llbp ;
int *opexec, *opclass ;
uint *faddr ;
int *src1, *src2, *src3, *src4, *src5 ;
int *dst1, *dst2, *dst3 ;
int *const1, *const1_valid ;
int *pin ;
int *pinv ;
int **cpin;	/* pointer to an array of cpin register addresses */
int **cpinv;	/* pointer to an array of cpin register values */
int *ncpin ;
int *pout ;
int *cpout ;
int	index;	/* index to the load buffer entry to be read */
{
	struct proginfo		*pip ;

	struct llb_entry	*ep ;

	int	i ;


#if	F_MASTERDEBUG && F_SAFE
	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = llbp->pip ;
	if (! llbp->readyread)
	    return SR_PROTO ;

	assert(index >= 0 && index < llbp->lip->totalrows) ;

	ep = &llbp->c.entry[index] ;

	*src1 = ep->src1 ;
	*src2 = ep->src2 ;
	*src3 = ep->src3 ;
	*src4 = ep->src4 ;
	*src4 = ep->src5 ;

	*dst1 = ep->dst1 ;
	*dst2 = ep->dst2 ;
	*dst3 = ep->dst3 ;

	*const1 = ep->const1 ;
	*const1_valid = ep->const1_valid ;
	*faddr = ep->faddr ;
	*opexec = ep->opexec ;
	*opclass = ep->opclass ;
	*pin = ep->pina ;
	*pinv = ep->pinv ;
	*pout = ep->pouta ;
	*ncpin = ep->ncpin ;
	*cpout = ep->cpouta ;
	*cpin = ep->cpina ;
	*cpinv = ep->cpinv ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("llb_read: row=%d ia=%08x\n",
	        index,ep->faddr) ;
#endif

	return SR_OK ;
}
/* end subroutine (llb_read) */


/* return some instruction information */
int llb_instrinfo(llbp,i,iip)
LLB		*llbp ;
int		i ;
LLB_INSTRINFO	*iip ;
{
	struct proginfo		*pip ;

	struct llb_entry	*ep ;


	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;

	pip = llbp->pip ;
	if (iip == NULL)
	    return SR_FAULT ;

	if ((i < 0) || (i >= llbp->nentries))
	    return SR_INVALID ;

	ep = llbp->c.entry + i ;

	iip->ia = ep->faddr ;
	iip->instr = ep->instr ;
	iip->btype = ep->btype ;	/* Marcos-syle i-fetch branch type */
	iip->opexec = ep->oopexec ;
	iip->opclass = ep->oopclass ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4) {

	    if (iip->opclass == INSTR_UNUSED)
	        eprintf("llb_instrinfo: row=%d UNUSED\n",
	            i) ;

	    else
	        eprintf("llb_instrinfo: row=%d ia=%08x instr=%08x\n",
	            i,iip->ia,iip->instr) ;
	}
#endif

	return SR_OK ;
}
/* end subroutine (llb_instrinfo) */


/* 

Call this function when you have loaded all AS's from load buffer.  It
makes the load buffer ready for a new pack of instructions to be loaded
from decode stage.

*/

int llb_is_free(llbp)
LLB	*llbp ;
{


	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;

	if (llbp->load_enable)
	    return 0 ;

	return 1 ;
}
/* end subroutine (llb_is_free) */


/* get the current instruction address of an entry (if any) */
int llb_getia(llbp, index, address)
LLB	*llbp ;
int	index ;
uint	*address ;
{
	struct llb_entry	*ep ;


	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;

	ep = &llbp->c.entry[index] ;
	if (! ep->valid)
	    return SR_EMPTY ;

	*address = ep->faddr ;
	return (INSTRCLASS_NULL + 1) ;
}
/* end subroutine (llb_getia) */


/* invalidate the load buffer on the NEXT clock if not loaded in this clock */
int llb_invalidate(llbp)
LLB	*llbp ;
{
	int i ;


	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;

	llbp->f.invalidate = TRUE ;
	if (llbp->n.count > 0)
	    llbp->n.count = 0 ;

	return SR_OK ;
}
/* end subroutine (llb_invalidate) */


/* is a row entry used ? */
int llb_used(llbp, index)
LLB	*llbp ;
int	index ;
{
	struct llb_entry	*ep ;

	int	rs = SR_OK, i ;


	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;

	if ((index<0) || (index >=llbp->lip->totalrows))
	    return SR_INVALID ;

#ifdef	COMMENT
	if (llbp->c.entry[index].valid != 0) {

	    switch (llbp->c.entry[index].opclass) {

	    case INSTR_BREL:
	        rs = INSTRCLASS_CBR ; /* conditional branch */
	        break ;

	    default:
	        rs = INSTRCLASS_REG ; /* regular instruction */
	        break ;

	    } /* end switch */

	} else
	    rs = 0 ;
#else
	if (! llbp->c.entry[index].valid)
	    return SR_EMPTY ;

	switch (llbp->c.entry[index].opclass) {

	case INSTR_BREL:
	    rs = INSTRCLASS_CBR ; /* conditional branch */
	    break ;

	default:
	    rs = 2; /* regular instruction */
	    break ;

	} /* end switch */

#endif /* COMMENT */

	return rs ;
}
/* end subroutine (llb_used) */


/* return statistics */
int llb_getstats(llbp,sp)
LLB		*llbp ;
LLB_STATS	*sp ;
{
	struct levoinfo	*lip ;

	ULONG	load_total ;
	ULONG	load_sofar ;
	ULONG	load_extra ;


	if (llbp == NULL)
	    return SR_FAULT ;

	if (llbp->magic != LLB_MAGIC)
	    return SR_NOTOPEN ;

	lip = llbp->lip ;

	load_total = llbp->s.load_col * lip->totalrows ;
	load_sofar = llbp->s.load_unused + llbp->s.load_instr ;
	load_extra = load_total - load_sofar ;

	*sp = llbp->s ;
	sp->load_unused += load_extra ;

	return SR_OK ;
}
/* end subroutine (llb_getstats) */


/* XML stuff */
int llb_xmlinit(fp,xip)
LLB	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}


int llb_xmlout(fp,xip)
LLB	*fp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs, i ;


	pip = fp->pip ;
	lip = fp->lip ;

	bprintf(&xip->xmlfile,"<llb>\n") ;

	for (i = 0 ; i < lip->totalrows ; i += 1) {

	bprintf(&xip->xmlfile,"<entry>\n") ;

	bprintf(&xip->xmlfile,"<row>%d</row>\n",i) ;


	bprintf(&xip->xmlfile,"<valid>%d</valid>\n",fp->c.entry[i].valid) ;

	bprintf(&xip->xmlfile,"<used>%d</used>\n",
		(fp->c.entry[i].opclass != OPCLASS_UNUSED)) ;

	if (fp->c.entry[i].opclass != OPCLASS_UNUSED) {

		uint	ia, instr ;


		ia = fp->c.entry[i].faddr ;
		instr = fp->c.entry[i].instr ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel >= 4)
	        eprintf("llb_xmlout: ia=%08x instr=%08x\n",ia,instr) ;
#endif /* F_DEBUG */

	bprintf(&xip->xmlfile,"<ia>%08x</ia>\n",
		ia) ;

	bprintf(&xip->xmlfile,"<instr>%08x</instr>\n",
		instr) ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel >= 4)
	        eprintf("llb_xmlout: f_instrdis=%d\n",pip->f.instrdis) ;
#endif /* F_DEBUG */

		if (pip->f.instrdis) {

		char	instrdis[INSTRDISLEN + 1] ;


	        rs = mipsdis_getlevo(&pip->dis,ia,instr,
				instrdis,INSTRDISLEN) ;

	        if (rs < 0)
	            strcpy(instrdis,"unknown") ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	        eprintf("llb_xmlout: ia=%08x instr=%08x %s\n",
	            ia,instr,instrdis) ;
#endif /* F_DEBUG */

	bprintf(&xip->xmlfile,"<instrdis>%s</instrdis>\n",
		instrdis) ;

		} /* end if (instruction disassembly) */

	} /* end if (valid entry) */

	bprintf(&xip->xmlfile,"</entry>\n") ;

	} /* end for (looping through entries -- rows) */

	bprintf(&xip->xmlfile,"</llb>\n") ;

	return SR_OK ;
}


int llb_xmlfree(fp,xip)
LLB	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}
/* end subroutine (llb_xmlfree) */



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


/* print out some stuff */
int llb_print(llbp)
LLB	*llbp ;
{
	int i ;

	printf("load buffer content\n") ;
	for(i=0;i<llbp->lip->totalrows; ++i) {
	    printf("src1=%d, src2=%d, dst1=%d\n", 
	        llbp->c.entry[i].src1, 
	        llbp->c.entry[i].src2, 
	        llbp->c.entry[i].dst1) ;
	}

	return i ;
}
/* end subroutine (llb_print) */


#ifdef COMMENT

int llb_dummyload(LLB * llbp) {

	int cp2[10],cp4[10] ;
	int cp[100][10] ;


	llb_load_complete(llbp) ;
}

#endif /* COMMENT */


#if	F_MASTEDEBUG && F_SAFE && F_SAFE2
static int isbadp(p)
void	*p ;
{
	int	i = (int) p ;


	i &= (~ (1 << 31)) ;
	return ((p == NULL) || (i < (8 * 1024)) ;
}
#endif /* F_SAFE2 */



