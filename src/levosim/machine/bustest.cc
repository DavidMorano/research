/* bustest */

/* Levo Bus Test */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		0		/* switchable debug print-outs */
#define	F_SAFE		1


/* revision history :

	= 00/07/27, Dave Morano

	Module was originally written.


*/



/**************************************************************************

	This object module provides a monitor function to listen
	to a 'BUS' object.


**************************************************************************/


#define	BUSTEST_MASTER	0


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"

#include	"levoinfo.h"
#include	"bus.h"
#include	"lflowgroup.h"
#include	"lbusint.h"
#include	"bustest.h"



/* local defines */

#define	BUSTEST_MAGIC	0x02039485



/* external subroutines */


/* forward references */

static int	bustest_dostuff(BUSTEST *) ;






int bustest_init(rfp,pip,mip,lip,busp,busid,nm,pathid)
BUSTEST		*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
BUS		*busp ;
int		busid, nm ;
int		pathid ;
{
	int	rs = SR_OK ;
	int	nmasters = nm ;
	int	i ;


	(void) memset(rfp,0,sizeof(BUSTEST)) ;

	rfp->magic = 0 ;
	rfp->pip = pip ;
	rfp->mip = lip->mip ;
	rfp->lip = lip ;

	rfp->busp = busp ;
	rfp->pathid = pathid ;

/* initialize our LBUSINTs */

#ifdef	COMMENT
	nmasters = bus_nmasters(busp) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("bustest_init: BUSTEST=%08lx BUS=%08lx masters=%d\n",
	            rfp,busp,nmasters) ;
#endif

	rfp->nbuses = nmasters ;
	if (nmasters > 0) {

	    int	size ;


	    size = rfp->nbuses * sizeof(LBUSINT) ;
	    rs = uc_malloc(size,&rfp->mybuses) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("bustest_init: malloc rs=%d\n",
	            rs) ;
#endif

	    if (rs < 0)
	        goto bad0 ;


	    for (i = 0 ; i < rfp->nbuses ; i += 1) {

	        rs = lbusint_init(rfp->mybuses + i,pip,lip,
	            rfp->busp,busid,0,0) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1) {
	            eprintf("bustest_init: LBUSINT(%d)=%08lx \n",
	                i,(rfp->mybuses + i)) ;
	            eprintf("bustest_init: lbusint_init(%d) rs=%d\n",
	                i,rs) ;
		}
#endif /* F_DEBUG */

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

		eprintf("bustest_init: LBUSINT(%d)=%08lx bad format (%d)\n",
			i,rfp->mybuses + i,rs) ;

		return rs ;
	}
#endif /* F_SAFE */

		busid += 1 ;

	    } /* end for */

	} /* end if (enough masters for a test) */

	rfp->value = 0 ;
	rfp->magic = BUSTEST_MAGIC ;

bad0:
	return rs ;
}
/* end subroutine (bustest_init) */


int bustest_free(rfp)
BUSTEST	*rfp ;
{
	int	i ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != BUSTEST_MAGIC) && (rfp->magic != 0)) {

		eprintf("bustest_free: BUSTEST=%08lx bad magic\n",
			rfp) ;

		return SR_BADFMT ;
	}

	if (rfp->magic != BUSTEST_MAGIC)
		return SR_NOTOPEN ;

	if (rfp->mybuses != NULL) {

	    for (i = 0 ; i < rfp->nbuses ; i += 1)
	        (void) lbusint_free(rfp->mybuses + i) ;

	    free(rfp->mybuses) ;

	} /* end if */

	rfp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (bustest_free) */


/* perform the combinatorial computations */
int bustest_comb(rfp,phase)
BUSTEST		*rfp ;
int		phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	rs = SR_OK ;
	int	i ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != BUSTEST_MAGIC) && (rfp->magic != 0)) {

		eprintf("bustest_comb: BUSTEST=%08lx bad magic\n",
			rfp) ;

		return SR_BADFMT ;
	}

	if (rfp->magic != BUSTEST_MAGIC)
		return SR_NOTOPEN ;

	pip = rfp->pip ;
	mip = rfp->mip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bustest_comb: entered ck=%lld ph=%d\n",
	        mip->clock,phase) ;
#endif


	for (i = 0 ; i < rfp->nbuses ; i += 1) {

	    rs = lbusint_comb(rfp->mybuses + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

		eprintf("bustest_comb: LBUSINT(%d)=%08lx bad format (%d)\n",
			i,rfp->mybuses + i,rs) ;

		return rs ;
	}
#endif /* F_SAFE */

	} /* end for */


	rs = SR_OK ;
	switch (phase) {

	case 0:
	    break ;

	case 1:
	    break ;

	case 2:
	    rs = bustest_dostuff(rfp) ;

	    break ;

	case 3:
	    break ;

	case 4:
	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (bustest_comb) */


/* handle a clock transition */
int bustest_clock(rfp)
BUSTEST	*rfp ;
{
	struct proginfo	*pip ;

	int	rs, i ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != BUSTEST_MAGIC) && (rfp->magic != 0)) {

		eprintf("bustest_clock: BUSTEST=%08lx bad magic\n",
			rfp) ;

		return SR_BADFMT ;
	}

	if (rfp->magic != BUSTEST_MAGIC)
		return SR_NOTOPEN ;

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bustest_clock: entered\n") ;
#endif

	for (i = 0 ; i < rfp->nbuses ; i += 1) {

	    rs = lbusint_clock(rfp->mybuses + i) ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

		eprintf("bustest_clock: LBUSINT(%d)=%08lx bad format (%d)\n",
			i,rfp->mybuses + i,rs) ;

		return rs ;
	}
#endif /* F_SAFE */

	} /* end for */


/* do our own stuff */

	rfp->c = rfp->n ;		/* copy everything */


	return SR_OK ;
}
/* end subroutine (bustest_clock) */



/* PRIVATE SUBROUTINES */



/* do stuff */
static int bustest_dostuff(rfp)
BUSTEST	*rfp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	LFLOWGROUP	wr, rd ;

	int	rs = SR_OK ;
	int	i ;
	int	f_vdp ;


	pip = rfp->pip ;
	mip = rfp->mip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("bustest_dostuff: ck=%lld\n",mip->clock) ;
#endif


/* write all buses that are ready ! */

	for (i = 0 ; i < rfp->nbuses ; i += 1) {

	    if ((rs = lbusint_ready(rfp->mybuses + i)) >= 0) {

		lflowgroup_init(&wr) ;

		wr.path = rfp->pathid ;
		wr.tt = 0 ;
		wr.addr = 0 ;
	        wr.dv = rfp->value ;
	        wr.dp = LFLOWGROUP_DPALL ;
	        rs = lbusint_write(rfp->mybuses + i,0,&wr) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("bustest_dostuff: LBUSINT(%d)=%08lx wv=%d rs=%d\n",
	                i,rfp->mybuses + i,rfp->value,rs) ;
#endif

		if (rs >= 0)
	        rfp->value += 1 ;

	    } else {

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

		eprintf("bustest_dostuff: LBUSINT(%d)=%08lx bad format (%d)\n",
			i,rfp->mybuses + i,rs) ;

		return rs ;
	}
#endif /* F_SAFE */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("bustest_dostuff: LBUSINT(%d)=%08lx not ready\n",
	                i,rfp->mybuses + i) ;
#endif

	        wr.dp = LFLOWGROUP_DPNONE ;

	    }

	} /* end for */


/* read all buses that have something */

	for (i = 0 ; i < rfp->nbuses ; i += 1) {

	    rd.dv = (int) -1 ;
	    rd.dp = LFLOWGROUP_DPNONE ;
	    rs = lbusint_read(rfp->mybuses + i,&rd) ;

	    f_vdp = (rd.dp) ? 1 : 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf(
	            "bustest_dostuff: LBUSINT(%d)=%08lx rs=%d vdp=%d rv=%d\n",
	            i,rfp->mybuses + i,rs,f_vdp,rd.dv) ;
#endif

	} /* end for */


	return SR_OK ;
}
/* end subroutine (bustest_dostuff) */




