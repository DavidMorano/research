/* lbusint */

/* Levo Bus Interface Helper */


#define	F_DEBUGS	0		/* non-switchable */
#define	F_DEBUG		0		/* switchable printouts */
#define	F_SAFE		1		/* be extra safe, at expense of time */
#define	F_BUSTRACE	1		/* final switch for bus tracing */


/* revision history :

	= 00/06/23, Dave Morano

	Module was originally written.


*/



/**************************************************************************

	This object module provides an interface to a Levo bus
	that is easier to use than the underlying bus interface that
	is more asynchronous.  This interface is more synchronous
	with respect to the Levo machine clock.

	Operating rules :

	+ caller can make a write request in phases 0-3
	+ caller can make read requests in phase 0-3


**************************************************************************/


#define	LBUSINT_MASTER	0


#include	<sys/types.h>
#include	<climits>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<bio.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"bus.h"
#include	"lflowgroup.h"
#include	"lbusint.h"



/* local defines */

#define	LBUSINT_MAGIC		0x96857463
#define	LBUSINT_DONEDELAY	3
#define	LBUSINT_DISPLAYTT	-9999999



/* external subroutines */

extern int	ctdeci(char *,int,int) ;


/* forward references */

static int	lbusint_busread(LBUSINT *) ;
static int	lbusint_handleshift(LBUSINT *) ;






int lbusint_init(rfp,pip,lip,busp,mid,id_type,id_which)
LBUSINT		*rfp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
BUS		*busp ;
int		mid ;
int		id_type, id_which ;
{


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("lbusint_init: state size=%d\n",
	        sizeof(struct lbusint_state)) ;
	}
#endif

	if (rfp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("lbusint_init: LBUSINT=%08lx BUS=%08lx busid=%d\n",
	        rfp,busp,mid) ;
	}
#endif

	(void) memset(rfp,0,sizeof(LBUSINT)) ;

	rfp->magic = 0 ;
	rfp->id_type = id_type ;
	rfp->id_which = id_which ;

	rfp->pip = pip ;
	rfp->lip = lip ;

	rfp->mip = lip->mip ;

	rfp->busp = busp ;
	rfp->mid = mid ;

	rfp->c.count = rfp->n.count = 0 ;

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct lbusint_state),&rfp->c.checksum) ;
#endif

	rfp->magic = LBUSINT_MAGIC ;
	return SR_OK ;
}
/* end subroutine (lbusint_init) */


/* free up this object */
int lbusint_free(rfp)
LBUSINT	*rfp ;
{
	struct proginfo	*pip ;


	if (rfp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_free: LBUSINT=%08lx bad format\n",
	        rfp) ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lbusint_free: entered\n") ;
#endif

	rfp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (lbusint_free) */


/* perform the combinatorial computations */
int lbusint_comb(rfp,phase)
LBUSINT		*rfp ;
int		phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_comb: LBUSINT=%08lx bad format\n",
	        rfp) ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	mip = rfp->mip ;

#if	F_DEBUGS && 0
	eprintf("lbusint_comb: hello pip=%08lx\n",pip) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("lbusint_comb: LBUSINT=%08lx ck=%lld ph=%d\n",
	        rfp,mip->clock,phase) ;
#endif

	switch (phase) {

	case 0:
	    rfp->f.shift = FALSE ;

#ifdef	OPTIONAL /* by default, all data "holds" */
	    rfp->n = rfp->c ;
#endif

	    if (rfp->c.f.request && (! rfp->c.f.requested)) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("lbusint_comb: requesting bus, busid=%d pri=%d\n",
	                rfp->mid,rfp->c.pri) ;
#endif

	        rs = bus_request(rfp->busp,rfp->mid,rfp->c.pri) ;

#if	F_MASTERDEBUG && F_SAFE
	        if ((rs == SR_FAULT) || (rs == SR_BADFMT) || 
	            (rs == SR_NOTOPEN)) {

	            eprintf("lbusint_comb: BUS bad format (%d)\n",rs) ;

	            return rs ;
	        }
#endif /* F_SAFE */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("lbusint_comb: request rs=%d\n",rs) ;
#endif

	        if (pip->debuglevel > 1) {

	            if (rs < SR_INPROGRESS)
	                eprintf("lbusint_comb: bad request rs=%d\n",rs) ;
	        }

	        rfp->n.f.requested = TRUE ;

	    } /* end if */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf("lbusint_comb: ph=%d cdp=%d ndp=%d\n",phase,
	            ((rfp->c.rreg.dp) ? 1 : 0),
	            ((rfp->n.rreg.dp) ? 1 : 0)) ;
	        eprintf("lbusint_comb: c.rreg.dv=\\x%08lx\n",
	            rfp->c.rreg.dv) ;
	    }
#endif

	    break ;

	case 1:
	    if (rfp->c.f.owned) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("lbusint_comb: bus_writecur() dv=\\x%08lx\n",
	                rfp->c.wreg.dv) ;
#endif

	        rs = bus_writecur(rfp->busp,&rfp->c.wreg) ;

#if	F_MASTERDEBUG && F_SAFE
	        if ((rs == SR_BADFMT) || (rs == SR_NOTOPEN) || 
	            (rs == SR_FAULT)) {

	            eprintf("lbusint_comb: BUS bad format (%d)\n",rs) ;

	            return rs ;
	        }
#endif /* F_SAFE */

	    } /* end if (bus owned by us) */

	    if (! rfp->c.f.owned) {

	        if (rfp->c.count > 0)
	            rfp->n.count = rfp->c.count - 1 ;

	    } else
	        rfp->n.count = LBUSINT_DONEDELAY ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf("lbusint_comb: ph=%d cdp=%d ndp=%d\n",phase,
	            ((rfp->c.rreg.dp) ? 1 : 0),
	            ((rfp->n.rreg.dp) ? 1 : 0)) ;
	        eprintf("lbusint_comb: c.rreg.dv=\\x%08lx\n",
	            rfp->c.rreg.dv) ;
	    }
#endif

	    break ;

	case 2:
	    rfp->n.f.owned = FALSE ;
	    if (rfp->c.f.request) {

	        rs = bus_grantednext(rfp->busp,rfp->mid) ;

#if	F_MASTERDEBUG && F_SAFE
	        if ((rs == SR_BADFMT) || (rs == SR_NOTOPEN) || 
	            (rs == SR_FAULT)) {

	            eprintf("lbusint_comb: BUS bad format (%d)\n",rs) ;

	            return rs ;
	        }
#endif /* F_SAFE */

	        if (rs > 0) {

	            rfp->n.f.owned = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("lbusint_comb: won bus grant\n") ;
#endif

	        }

	    }

	    rs = lbusint_busread(rfp) ;

#if	F_MASTERDEBUG && F_SAFE
	    if ((rs == SR_BADFMT) || (rs == SR_NOTOPEN) || 
	        (rs == SR_FAULT)) {

	        eprintf("lbusint_comb: LBUSINT bad format (%d)\n",rs) ;

	        return rs ;
	    }
#endif /* F_SAFE */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf("lbusint_comb: ph=%d cdp=%d ndp=%d\n",phase,
	            ((rfp->c.rreg.dp) ? 1 : 0),
	            ((rfp->n.rreg.dp) ? 1 : 0)) ;
	        eprintf("lbusint_comb: c.rreg.dv=\\x%08x\n",
	            rfp->c.rreg.dv) ;
	    }
#endif

	    break ;

	case 3:

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf("lbusint_comb: ph=%d cdp=%d ndp=%d\n",phase,
	            ((rfp->c.rreg.dp) ? 1 : 0),
	            ((rfp->n.rreg.dp) ? 1 : 0)) ;
	        eprintf("lbusint_comb: c.rreg.dv=\\x%08x\n",
	            rfp->c.rreg.dv) ;
	    }
#endif

	    break ;

	case 4:
	    rfp->n.f.request = rfp->f.request ;

	    if (rfp->c.f.request)
	        rfp->n.wreg = rfp->c.wreg ;


	    if (rfp->n.f.owned) {

	        rfp->f.request = FALSE ;

	        rfp->n.f.request = FALSE ;
	        rfp->n.f.requested = FALSE ;

	    }


#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf("lbusint_comb: ph=%d cdp=%d ndp=%d\n",phase,
	            ((rfp->c.rreg.dp) ? 1 : 0),
	            ((rfp->n.rreg.dp) ? 1 : 0)) ;
	        eprintf("lbusint_comb: c.rreg.dv=\\x%08x\n",
	            rfp->c.rreg.dv) ;
	    }
#endif

	    rs = lbusint_handleshift(rfp) ;

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (lbusint_comb) */


/* handle a clock transition */
int lbusint_clock(rfp)
LBUSINT	*rfp ;
{
	struct proginfo	*pip ;

	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_clock: LBUSINT=%08lx bad format\n",
	        rfp) ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 4)
	    eprintf("lbusint_clock: entered\n") ;
#endif

#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&rfp->c,sizeof(struct lbusint_state),
	    &rfp->c.checksum) ;

	if (rs < 0) {

	    eprintf("lbusint_clock: checksum bad format\n") ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */

	rfp->c = rfp->n ;		/* copy everything */

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&rfp->c,sizeof(struct lbusint_state),&rfp->c.checksum) ;
#endif

	return SR_OK ;
}
/* end subroutine (lbusint_clock) */


/* shift the machine (called in phase 1) */
int lbusint_shift(rfp)
LBUSINT	*rfp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_shift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	rfp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (lbusint_shift) */


/* are we ready for another request ? */
int lbusint_ready(rfp)
LBUSINT		*rfp ;
{


	if (rfp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_ready: %08lx bad format\n",rfp) ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (rfp->f.request)
	    return SR_INPROGRESS ;

	return SR_OK ;
}
/* end subroutine (lbusint_ready) */


/* we have a request for a write operation */
int lbusint_write(rfp,pri,fgp)
LBUSINT		*rfp ;
int		pri ;
LFLOWGROUP	*fgp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	j ;

#if	F_BUSTRACE || (F_MASTERDEBUG && F_DEBUG)
	char	ttbuf[24] ;
	char	seqbuf[24] ;
#endif


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_write: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;

	if (fgp == NULL)
	    return SR_FAULT ;
#endif /* F_SAFE */

	if (rfp->f.request)
	    return SR_INPROGRESS ;

	pip = rfp->pip ;
	mip = rfp->mip ;
	lip = rfp->lip ;

#if	F_BUSTRACE || (F_MASTERDEBUG && F_DEBUG)

	    if (fgp->tt < LBUSINT_DISPLAYTT)
	        strcpy(ttbuf,"-") ;

	    else
	        ctdeci(ttbuf,-1,fgp->tt) ;

	    if (fgp->seq > 999)
		strcpy(seqbuf,"+") ;

	    else
		ctdeci(seqbuf,-1,fgp->seq) ;

#endif /* F_DEBUG || F_BUSTRACE */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("lbusint_write: LBUSINT=%p mid=%d BUS=%p\n",
		rfp,rfp->mid,rfp->busp) ;
	    eprintf(
	        "lbusint_write: %08llx %08lx W %2d %d %8s:%-3s %08x %08x\n",
	        mip->clock,rfp,
	        fgp->trans,fgp->path,
	        ttbuf, seqbuf,
	        fgp->addr, fgp->dv) ;
	}
#endif /* F_DEBUG */

#if	F_BUSTRACE
	if (lip->btfp != NULL)
	    bprintf(lip->btfp,
	        "%08llx %08lx W %2d %d %8s:%-3s %08x %08x\n",
	        mip->clock,rfp->busp,
	        fgp->trans,
	        fgp->path,
	        ttbuf, seqbuf,
	        fgp->addr, fgp->dv) ;

	if (lip->mtfp != NULL) {

	    bprintf(lip->mtfp,
	        "%08llx %08lx %08lx %5d W %2d %d %8s:%-3s %08x",
	        mip->clock,rfp,rfp->busp,rfp->mid,
	        fgp->trans,
	        fgp->path,
	        ttbuf, seqbuf,
	        fgp->addr) ;

		if (fgp->dp) {

		bputc(lip->mtfp,' ') ;

		for (j = 3 ; j >= 0 ; j -= 1) {

	                    if ((fgp->dp >> j) & 1) {

	                        bprintf(lip->mtfp,"%02x",
	                            ((fgp->dv >> (j * 8)) & 255)) ;

	                    } else
	                        bprintf(lip->mtfp,"--") ;


		} /* end for */

		}

		bputc(lip->mtfp,'\n') ;

	} /* end if (master-trace file) */
#endif /* F_BUSTRACE */

	rfp->n.wreg = *fgp ;
	rfp->n.pri = pri ;		/* bus priority */

	rfp->f.request = TRUE ;
	return 1 ;
}
/* end subroutine (lbusint_write) */


/* user wants to read the bus */
int lbusint_read(rfp,fgp)
LBUSINT		*rfp ;
LFLOWGROUP	*fgp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	f_dvp ;


#if	F_MASTERDEBUG && F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_ready: %08lx bad format\n",rfp) ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;

	if (fgp == NULL)
	    return SR_FAULT ;
#endif /* F_SAFE */

	pip = rfp->pip ;
	mip = rfp->mip ;

	f_dvp = rfp->c.f.dp_read ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf(
	        "lbusint_read: LBUSINT=%08lx BUS=%08lx ck=%08llx dp=%d\n", 
	        rfp,rfp->busp,mip->clock,f_dvp) ;
#endif

	if (f_dvp) {

	    *fgp = rfp->c.rreg ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4) {

	        char	ttbuf[24] ;
		char	seqbuf[24] ;


	        if (fgp->tt < LBUSINT_DISPLAYTT)
	            strcpy(ttbuf,"-") ;

	        else
	            ctdeci(ttbuf,-1,fgp->tt) ;

	    if (fgp->seq > 999)
		strcpy(seqbuf,"+") ;

	    else
		ctdeci(seqbuf,-1,fgp->seq) ;

	        eprintf(
		    "lbusint_read: tr=%d p=%d tt=%8s:%-3s a=%08x dv=%08x\n",
	            fgp->trans,
	            fgp->path,
	            ttbuf,seqbuf,
	            fgp->addr, fgp->dv) ;

	    } /* end if (debug) */
#endif /* F_DEBUG */

	} /* end if (data was present) */

	return ((f_dvp) ? 1 : 0) ;
}
/* end subroutine (lbusint_read) */


/* are all current bus writes finished + two clocks ? */
int lbusint_done(rfp)
LBUSINT	*rfp ;
{
	struct proginfo	*pip ;


	if (rfp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_done: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = rfp->pip ;

/* if we own the bus or if our count is greater than zero, return FALSE */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4)
		eprintf("lbusint_done: f_req=%d count=%d\n",
			rfp->c.f.request,rfp->c.count) ;
#endif

	if (rfp->c.f.request || (rfp->c.count > 0))
	    return FALSE ;

	return TRUE ;
}
/* end subroutine (lbusint_done) */


/* sanity check the object */
int lbusint_check(rfp)
LBUSINT	*rfp ;
{
	int	rs ;


	if (rfp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rfp->magic != LBUSINT_MAGIC) && (rfp->magic != 0)) {

	    eprintf("lbusint_check: %08lx bad format\n",rfp) ;

	    return SR_BADFMT ;
	}

	if (rfp->magic != LBUSINT_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */


#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&rfp->c,sizeof(struct lbusint_state),
	    &rfp->c.checksum) ;

	if (rs < 0) {

	    eprintf("lbusint_check: %08lx check bad format\n",
	        rfp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */


	return SR_OK ;
}
/* end subroutine (lbusint_check) */



/* PRIVATE SUBROUTINES */



/* read our incoming bus */
static int lbusint_busread(rfp)
LBUSINT	*rfp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	LFLOWGROUP	*fgp ;

	int	rs = SR_OK, j ;

#if	F_BUSTRACE || (F_MASTERDEBUG && F_DEBUG)
	char	ttbuf[24] ;
	char	seqbuf[24] ;
#endif


	pip = rfp->pip ;
	mip = rfp->mip ;
	lip = rfp->lip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("lbusint_busread: entered\n") ;
#endif

/* read the bus and see if there is anything there for us */

	fgp = &rfp->n.rreg ;

#if	F_MASTERDEBUG && F_DEBUG
	fgp->dv = (uint) -1 ;
	fgp->dp = LFLOWGROUP_DPNONE ;
#endif

	rfp->n.f.dp_read = FALSE ;
	rs = bus_read(rfp->busp,fgp) ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	    eprintf("lbusint_comb: LBUSINT=%08lx BUS=%08lx bad format (%d)\n",
	        rfp,rfp->busp,rs) ;

	    return rs ;
	}
#endif /* F_SAFE */

	if (rs > 0) {

	    int	f_error = FALSE ;
	    int	f_dvp ;


	    rfp->n.f.dp_read = TRUE ;

#if	F_BUSTRACE || (F_MASTERDEBUG && F_DEBUG)
	        if (fgp->tt < LBUSINT_DISPLAYTT)
	            strcpy(ttbuf,"-") ;

	        else
	            ctdeci(ttbuf,-1,fgp->tt) ;

	    if (fgp->seq > 999)
		strcpy(seqbuf,"+") ;

	    else
		ctdeci(seqbuf,-1,fgp->seq) ;

#endif /* F_DEBUG || F_BUSTRACE */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        f_dvp = (fgp->dp) ? 1 : 0 ;
	        eprintf("lbusint_busread: read v=\\x%08x vdp=%d\n",
	            fgp->dv,f_dvp) ;
	    }
#endif

#if	F_BUSTRACE
	    if (lip->btfp != NULL)
	        bprintf(lip->btfp,
	            "%08llx %08lx R %2d %d %8s:%-3s %08x %08x\n",
	            mip->clock,rfp->busp,
	            fgp->trans,
	            fgp->path,
	            ttbuf,seqbuf,
	            fgp->addr, fgp->dv) ;

	    if (lip->mtfp != NULL) {

	        bprintf(lip->mtfp,
	            "%08llx %08lx %08lx %5d R %2d %d %8s:%-3s %08x",
	            mip->clock,rfp,rfp->busp,rfp->mid,
	            fgp->trans,
	            fgp->path,
	            ttbuf,seqbuf,
	            fgp->addr) ;

		if (fgp->dp) {

		bputc(lip->mtfp,' ') ;

		for (j = 3 ; j >= 0 ; j -= 1) {

	                    if ((fgp->dp >> j) & 1) {

	                        bprintf(lip->mtfp,"%02x",
	                            ((fgp->dv >> (j * 8)) & 255)) ;

	                    } else
	                        bprintf(lip->mtfp,"--") ;


		} /* end for */

		}

		bputc(lip->mtfp,'\n') ;

	} /* end subroutine (master-trace file) */
#endif /* F_BUSTRACE */

	} else {

	    fgp->dp = LFLOWGROUP_DPNONE ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("lbusint_busread: nothing there\n") ;
#endif

	}

	return SR_OK ;
}
/* end subroutine (lbusint_busread) */


/* do we have a machine shift */
static int lbusint_handleshift(rfp)
LBUSINT	*rfp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;
	int	nrows, tt_test ;


	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
		eprintf("lbusint_handleshift: f_shift=%d\n",
			rfp->f.shift) ;
#endif

	if (! rfp->f.shift)
	    return rs ;

	rfp->f.shift = FALSE ;
	lip = rfp->lip ;
	nrows = lip->totalrows ;
	tt_test = INT_MIN + nrows ;

/* shift the bus-write register */

	if (rfp->n.wreg.ftt >= tt_test)
	    rfp->n.wreg.ftt -= nrows ;

	if (rfp->n.wreg.tt >= tt_test)
	    rfp->n.wreg.tt -= nrows ;

	if (rfp->n.wreg.rtt >= tt_test)
	    rfp->n.wreg.rtt -= nrows ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
		eprintf("lbusint_handleshift: shifted wreg ftt=%d tt=%d\n",
	    		rfp->n.wreg.ftt,
	    		rfp->n.wreg.tt) ;
#endif


/* shift the bus-read register */

	if (rfp->n.f.dp_read) {

	    if (rfp->n.rreg.ftt >= tt_test)
	        rfp->n.rreg.ftt -= nrows ;

	    if (rfp->n.rreg.tt >= tt_test)
	        rfp->n.rreg.tt -= nrows ;

	    if (rfp->n.rreg.rtt >= tt_test)
	        rfp->n.rreg.rtt -= nrows ;

	} /* end if (we had read something off the bus) */


	return rs ;
}
/* end subroutine (lbusint_handleshift) */



