/* busmon */

/* Levo Bus Monitor */


#define	F_DEBUGS	0
#define	F_DEBUG		0
#define	F_SAFE		0


/* revision history :

	= 00/07/27, Dave Morano

	Module was originally written.


*/



/**************************************************************************

	This object module provides a monitor function to listen
	to a 'BUS' object.


**************************************************************************/


#define	BUSMON_MASTER	0


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
#include	"busmon.h"



/* local defines */


/* external subroutines */


/* forward references */

static int	busmon_busread(BUSMON *) ;
static int	busmon_buswrite(BUSMON *) ;





int busmon_init(rfp,pip,mip,lip,busp)
BUSMON		*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
BUS		*busp ;
{
	int	rs = SR_OK ;


	(void) memset(rfp,0,sizeof(BUSMON)) ;

	rfp->pip = pip ;
	rfp->mip = lip->mip ;
	rfp->lip = lip ;

	rfp->busp = busp ;

	return rs ;
}
/* end subroutine (busmon_init) */


int busmon_free(rfp)
BUSMON	*rfp ;
{


	if (rfp == NULL)
	    return SR_FAULT ;

	return SR_OK ;
}
/* end subroutine (busmon_free) */


/* perform the combinatorial computations */
int busmon_comb(rfp,phase)
BUSMON		*rfp ;
int		phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	rs = SR_OK ;


	pip = rfp->pip ;
	mip = rfp->mip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("busmon_comb: entered ck=%lld ph=%d\n",
	        mip->clock,phase) ;
#endif

	switch (phase) {

	case 0:
	    break ;

	case 1:
	    break ;

	case 2:
	    busmon_busread(rfp) ;

	    break ;

	case 3:
	    break ;

	case 4:
	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (busmon_comb) */


/* handle a clock transition */
int busmon_clock(rfp)
BUSMON	*rfp ;
{
	struct proginfo	*pip ;


	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("busmon_clock: entered\n") ;
#endif

	rfp->c = rfp->n ;		/* copy everything */

	return SR_OK ;
}
/* end subroutine (busmon_clock) */


/* we have a request for a write operation */
int busmon_write(rfp,pri,lfgp)
BUSMON		*rfp ;
int		pri ;
LFLOWGROUP	*lfgp ;
{
	struct proginfo	*pip ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if (rfp->f.request)
	    return SR_INPROGRESS ;

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("busmon_write: request to write v=%d\n",
	        lfgp->dv) ;
#endif

	rfp->n.wreg = *lfgp ;

	rfp->n.pri = pri ;		/* bus priority */
	rfp->f.request = TRUE ;
	return SR_OK ;
}
/* end subroutine (busmon_write) */


/* are we ready for another request ? */
int busmon_ready(rfp)
BUSMON		*rfp ;
{


	if (rfp == NULL)
	    return SR_FAULT ;

	if (rfp->f.request)
	    return SR_INPROGRESS ;

	return SR_OK ;
}
/* end subroutine (busmon_ready) */


/* user wants to read the bus */
int busmon_read(rfp,lfgp)
BUSMON		*rfp ;
LFLOWGROUP	*lfgp ;
{
	int	f_vdp ;


	f_vdp = (rfp->c.rreg.dp) ? 1 : 0 ;
	if (f_vdp)
	    *lfgp = rfp->c.rreg ;

	return f_vdp ;
}
/* end subroutine (busmon_read) */



/* PRIVATE SUBROUTINES */



/* read our incoming bus */
static int busmon_busread(rfp)
BUSMON	*rfp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	LFLOWGROUP	*fgp ;

	int	rs = SR_OK ;
	int	cmid ;


	pip = rfp->pip ;
	mip = rfp->mip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("busmon_busread: entered\n") ;
#endif

/* read the bus and see if there is anything there for us */

	fgp = &rfp->n.rreg ;

	fgp->dv = (int) -1 ;
	fgp->dp = LFLOWGROUP_DPNONE ;
	rs = bus_read(rfp->busp,fgp) ;

	cmid = bus_curmaster(rfp->busp) ;

	{
	    int	f_vdp ;


	    f_vdp = (fgp->dp) ? 1 : 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 0) {

	        eprintf("busmon_busread: ck=%lld mid=%d rs=%d vdp=%d v=%d\n",
	            mip->clock,cmid,rs,f_vdp,fgp->dv) ;

	    }
#endif

	    if (! f_vdp)
	        fgp->dp = LFLOWGROUP_DPNONE ;

	}

	return SR_OK ;
}
/* end subroutine (busmon_busread) */


/* write the output bus */
static int busmon_buswrite(rfp)
BUSMON	*rfp ;
{
	BUS_CONTENT	rd ;

	int	rs, i, j ;
	int	addr ;
	int	totalrows ;
	int	npaths ;



	return SR_OK ;
}
/* end subroutine (busmon_buswrite) */



