/* lfgf */

/* Levo FlowGroup FIFO component */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		0		/* switchable debug print-outs */
#define	F_SAFE		1		/* extra safe mode */


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written.


*/


/**************************************************************************

	This object module provides the function of FIFO suitable
	for use in storing Levo flow groups.  Basicially, this means
	that the data is not entirely abstracted but rather that the
	time-tag values within the data that is stored can be
	decremented on a machine shift (an important part of all
	Levo execution window functions).


**************************************************************************/


#define	LFGF_MASTER	1


#include	<sys/types.h>
#include	<climits>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"

#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"lfgf.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	LFGF_MAGIC		0x12437498



/* external subroutines */


/* forward referecens */

static int lfgf_handleshift(LFGF *,struct proginfo *,int) ;






int lfgf_init(rfp,pip,mip,lip,nentries)
LFGF		*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
int		nentries ;
{
	int	rs = SR_OK ;
	int	i ;
	int	size ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if ((pip == NULL) || (mip == NULL) || (lip == NULL))
	    return SR_FAULT ;

	(void) memset(rfp,0,sizeof(LFGF)) ;

	rfp->pip = pip ;
	rfp->mip = mip ;
	rfp->lip = lip ;

	if (nentries < 1)
	    nentries = 1 ;

	rfp->nregs = nentries ;		/* FIFO size */

/* allocate our registers */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lfgf_init: allocating registers n=%d\n",
		rfp->nregs) ;
#endif

	size = 2 * rfp->nregs * sizeof(LFLOWGROUP) ;
	rs = uc_malloc(size,&rfp->a.regs) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lfgf_init: uc_malloc() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    goto bad1 ;

	rfp->c.regs = rfp->a.regs + 0 ;
	rfp->n.regs = rfp->a.regs + rfp->nregs ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("lfgf_init: LFGF=%08lx regs=%08lx\n",rfp,rfp->a.regs) ;
#endif

/* this is not needed depending on the exact update policy */

	rfp->c.ri = rfp->n.ri = 0 ;
	rfp->c.wi = rfp->n.wi = 0 ;
	rfp->c.count = rfp->n.count = 0 ;
	rfp->c.f.notempty = rfp->n.f.notempty = FALSE ;

#ifdef	COMMENT
	for (i = 0 ; i < rfp->nregs ; i += 1) {

	    rfp->c.regs[i].tt = -1 ;
	    rfp->n.regs[i].tt = -1 ;

	} /* end for */
#endif /* COMMENT */


#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct lfgf_state),&rfp->c.checksum) ;
#endif

	rfp->magic = LFGF_MAGIC ;

/* bad things */
bad1:
	return rs ;
}
/* end subroutine (lfgf_init) */


/* free up this object */
int lfgf_free(rfp)
LFGF	*rfp ;
{
	struct proginfo	*pip ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_free: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4) {
	eprintf("lfgf_free: LFGF=%08lx entered\n",rfp) ;
	eprintf("lfgf_free: LFGF=%08lx regs=%08lx\n",rfp,rfp->a.regs) ;
	}
#endif

	if (rfp->a.regs != NULL)
	    free(rfp->a.regs) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("lfgf_free: exiting\n") ;
#endif

	rfp->c.regs = rfp->n.regs = NULL ;

	rfp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (lfgf_free) */


/* perform the combinatorial computations */
int lfgf_comb(rfp,phase)
LFGF	*rfp ;
int	phase ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i ;
	int	size ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_comb: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	lip = rfp->lip ;


	switch (phase) {

	case 0:
	    rfp->f.shift = FALSE ;

/* the default is for the next state to be the same as the current state */

	    break ;

	case 1:
	    break ;

	case 2:
	    break ;

	case 3:
	    rs = lfgf_handleshift(rfp,pip,lip->totalrows) ;

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (lfgf_comb) */


/* handle a clock transition */
int lfgf_clock(rfp)
LFGF	*rfp ;
{
	LFLOWGROUP	*fgp ;

	int	rs = SR_OK ;
	int	size ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_clock: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */


/* transition ourselves */

#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&rfp->c,sizeof(struct lfgf_state),
	    &rfp->c.checksum) ;

	if (rs < 0) {

	    eprintf("lfgf_clock: LFGF=%08lx check bad format\n",
	        rfp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */

/* we swap the allocated parts and copy the rest */

	fgp = rfp->c.regs ;		/* save allocated pointer */
	rfp->c = rfp->n ;		/* copy everything else */
	rfp->n.regs = fgp ;		/* swap in allocated pointer */

/* copy the swapped parts */

	size = rfp->nregs * sizeof(LFLOWGROUP) ;
	(void) memcpy(rfp->n.regs,rfp->c.regs,size) ;

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct lfgf_state),&rfp->c.checksum) ;
#endif


	return rs ;
}
/* end subroutine (lfgf_clock) */


/* shift the machine (called in phase 1) */
int lfgf_shift(rfp)
LFGF	*rfp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_shift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	rfp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (lfgf_shift) */


/* shift in a new value into the FIFO */
int lfgf_write(rfp,fgp)
LFGF		*rfp ;
LFLOWGROUP	*fgp ;
{
	struct proginfo	*pip ;

	int	wi ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_write: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (fgp == NULL)
	    return SR_FAULT ;

	pip = rfp->pip ;

/* is this FIFO already full ? */

	if (rfp->c.count >= rfp->nregs) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
		eprintf("lfgf_write: nregs=%d count=%d \n",
			rfp->nregs,rfp->c.count) ;
#endif

	    return SR_OVERFLOW ;
	}

	wi = rfp->c.wi ;
	rfp->n.regs[wi] = *fgp ;

	wi = (wi + 1) % rfp->nregs ;
	rfp->n.wi = wi ;

	rfp->n.count += 1 ;
	return rfp->n.count ;
}
/* end subroutine (lfgf_write) */


/* read the current value that is on the output of the FIFO */
int lfgf_read(rfp,fgp)
LFGF		*rfp ;
LFLOWGROUP	*fgp ;
{
	int	ri ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_read: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (fgp == NULL)
	    return SR_FAULT ;

/* is this FIFO already empty ? */

	if (rfp->c.count <= 0)
	    return SR_OVERFLOW ;

	ri = rfp->c.ri ;
	*fgp = rfp->c.regs[ri] ;

	return (rfp->c.count - 1) ;
}
/* end subroutine (lfgf_read) */


/* shift out the current value that is on the output of the FIFO */
int lfgf_readout(rfp,fgp)
LFGF		*rfp ;
LFLOWGROUP	*fgp ;
{
	int	ri ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_readout: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (fgp == NULL)
	    return SR_FAULT ;

/* is this FIFO already empty ? */

	if (rfp->c.count <= 0)
	    return SR_OVERFLOW ;

	ri = rfp->c.ri ;
	*fgp = rfp->c.regs[ri] ;

	ri = (ri + 1) % rfp->nregs ;
	rfp->n.ri = ri ;

	rfp->n.count -= 1 ;
	return rfp->n.count ;
}
/* end subroutine (lfgf_readout) */


/* return the count of items in the FIFO */
int lfgf_count(rfp)
LFGF	*rfp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_readout: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return rfp->c.count ;
}
/* end subroutine (lfgf_count) */


/* enumerate the entries */
int lfgf_enum(rfp,index,fgp)
LFGF		*rfp ;
int		index ;
LFLOWGROUP	*fgp ;
{
	int	rs = SR_OK ;
	int	ri ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LFGF_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lfgf_enum: bad format LFGF=%08lx\n",rfp) ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LFGF_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (fgp == NULL)
	    return SR_FAULT ;

/* is this FIFO already empty ? */

	if (rfp->c.count <= 0)
	    return SR_OVERFLOW ;

	if ((index < 0) || (index >= rfp->c.count))
	    return SR_OVERFLOW ;

	ri = (rfp->c.ri + index) % rfp->nregs ;
	*fgp = rfp->c.regs[ri] ;

	rs = rfp->c.count - index ;

	return rs ;
}
/* end subroutine (lfgf_readout) */



/* PRIVATE SUBROUTINES */



static int lfgf_handleshift(rfp,pip,totalrows)
LFGF		*rfp ;
struct proginfo	*pip ;
int		totalrows ;
{
	int	n, i ;
	int	tt_test ;


	if (! rfp->f.shift)
		return SR_OK ;

	rfp->f.shift = FALSE ;
	tt_test = INT_MIN + totalrows ;
		
	i = rfp->n.ri ;
	for (n = 0 ; n < rfp->n.count ; n += 1) {

	    if (rfp->n.regs[i].ftt >= tt_test)
	        rfp->n.regs[i].ftt -= totalrows ;

	    if (rfp->n.regs[i].tt >= tt_test)
	        rfp->n.regs[i].tt -= totalrows ;

	    if (rfp->n.regs[i].rtt >= tt_test)
	        rfp->n.regs[i].rtt -= totalrows ;

	    i = (i + 1) % rfp->nregs ;

	} /* end for */


	return SR_OK ;
}
/* end subroutine (lfgf_handleshift) */



