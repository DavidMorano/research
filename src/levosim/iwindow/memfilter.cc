/* memfilter */

/* Levo Forwarding Register Filter */


#define	F_DEBUGS	0
#define	F_ERRORS	1


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written.


*/



/**************************************************************************

	This object module provides the function of the Forwarding
	Register Bus Filter hardware component in the Levo machine.


**************************************************************************/


#define	MEMFILTER_MASTER	1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"bus.h"
#include	"memfilter.h"



/* local defines */



/* external subroutines */


/* forward referecens */

static int memfilter_busread(MEMFILTER *) ;
static int memfilter_buswrite(MEMFILTER *) ;





int memfilter_init(rfp,gp,mip,lip,rbus,wbus,n)
MEMFILTER	*rfp ;
struct proginfo	*gp ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
BUS		*rbus, *wbus ;
int		n ;
{
	int	rs = SR_OK ;
	int	i, size ;


	(void) memset(rfp,0,sizeof(MEMFILTER)) ;

	rfp->gp = gp ;
	rfp->mip = mip ;
	rfp->lip = lip ;

	rfp->rbus = rbus ;
	rfp->wbus = wbus ;
	rfp->nqentries = n ;

/* allocate our registers */

	rfp->nregs = MEMFILTER_NREGS * lip->npaths ;
	size = 2 * rfp->nregs * sizeof(MEMFILTER_REG) ;
	rs = uc_malloc(size,&rfp->c.regs) ;

	if (rs < 0) 
		goto bad1 ;

	rfp->n.regs = rfp->c.regs + (size >> 1) ;

/* this is not needed depending on the exact update policy */

	for (i = 0 ; i < rfp->nregs ; i += 1)
		rfp->.c.r[i].tt = -1 ;

bad1:
	return rs ;
}
/* end subroutine (memfilter_init) */


int memfilter_free(rfp)
MEMFILTER	*rfp ;
{


	if (rfp->c.regs != NULL)
		free(rfp->c.regs) ;

	rfp->c.regs = NULL ;
	rfp->n.regs = NULL ;
	return SR_OK ;
}


/* handle a clock transition */
int memfilter_clock(rfp)
MEMFILTER	*rfp ;
{
	MEMFILTER_REG	*rp ;


/* we swap the allocated parts and copy the rest */

	rp = rfp->c.r ;			/* save allocated pointer */

	rfp->c = rfp->n ;		/* copy everything */

	rfp->n.r = rp ;			/* swap in allocated pointer */

	return SR_OK ;
}
/* end subroutine (memfilter_clock) */


/* perform the combinatorial computations */
int memfilter_comb(rfp,phase)
MEMFILTER	*rfp ;
int		phase ;
{
	int	rs = SR_OK ;
	int	j ;


	switch (phase) {

	case 0:
	    rfp->f.shift = FALSE ;
	    rfp->f.forward = FALSE ;

/* the default is for the next state to be the same as the current state */

	for (j = 0 ; j < MEMFILTER_NREGS ; j += 1)
	    rfp->n.r[j] = rfp->c.r[j]  ;

	    rs = memfilter_buswrite(rfp) ;

	    rs = memfilter_lookup(rfp) ;

	    break ;

	case 1:
	    break ;

	case 2:
	    rs = memfilter_busread(rfp) ;

	    break ;

	case 3:
	    rs = memfilter_handleshift(rfp)

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (memfilter_comb) */


/* shift the machine (called in phase 1) */
int memfilter_shift(rfp)
MEMFILTER	*rfp ;
{


	rfp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (memfilter_shift) */



/* INTERNAL SUBROUTINES */



/* read our incoming bus */
static int memfilter_busread(rfp)
MEMFILTER	*rfp ;
{
	BUS_CONTENT	rd ;

	int	rs = SR_OK ;


/* read the bus and see if there is anything there for us */

	    rs = bus_read(rfp->rbus,&rd) ;

	    if (rs > 0) {

		int	f_error = FALSE ;


#if	F_ERRORS
		if ((rd.path < 0) || (rd.path > 200)) {

			rfp->errors += 1 ;
			f_error = TRUE ;

		}

		if ((rd.tt < 0) || (rd.tt > 100000)) {

			rfp->errors += 1 ;
			f_error = TRUE ;

		}

		if ((rd.addr < 0) || (rd.addr > 200)) {

			rfp->errors += 1 ;
			f_error = TRUE ;

		}

#endif /* F_ERRORS */

		if ((! f_error) && rd.dp[0])
			rfp->n.busdata = rd ;

	    	else
			rfp->n.busdata.dp[0] = FALSE ;

		} /* end if (something was on the bus) */


	retrun SR_OK ;
}
/* end subroutine (memfilter_busread) */


/* write the output bus */
static int memfilter_buswrite(rfp)
MEMFILTER	*rfp ;
{
	BUS_CONTENT	rd ;

	int	rs, i, j ;
	int	addr ;
	int	totalrows ;
	int	npaths ;


	totalrows = rfp->lip->totalrows ;
	f_shift = rfp->f.shift ;
	rfp->f.update = TRUE ;


}
/* end subroutine (memfilter_buswrite) */


/* do we have a machine shift */
static int memfilter_handleshift(rfp)
MEMFILTER	*rfp ;
{
	int	rs, i, j ;
	int	addr ;
	int	totalrows ;
	int	npaths ;


	if (rfp->f.shift) {

	    rfp->f.shift = FALSE ;
	    for (j = 0 ; j < MEMFILTER_NREGS ; j += 1)
	        rfp->n.r[j].tt -= totalrows ;

		if (rfp->n.busdata.dp[0])
	        	rfp->n.busdata.tt -= totalrows ;

	} /* end if (machine shift) */

	return SR_OK ;
}
/* end subroutine (memfilter_handleshift) */


/* lookup to see if the present forward operation must forward again */
static int memfilter_lookup(rfp)
MEMFILTER	*rfp ;
{
	int	rs, i, j ;
	int	addr ;
	int	totalrows ;
	int	npaths ;


/* do we have anything off of the bus from the previous clock to process */

		rfp->f.forward = FALSE ;
		if (rfp->c.busdata.dp[0]) {

			npaths = lip->npaths ;
			addr = rfp->c.busdata.addr ;
			i = (rfp->c.busdata.path * npaths) + addr ;
			if ((rfp->c.busdata.tt >= rfp->c.r[i].tt) &&
				(rfp->c.busdata.dv.l != rfp->c.r[i].dv.l))
					rfp->f.forward = TRUE ;

		} /* end if */

/* do we have to forward ? */

		if (f_forward) {





	} /* end if (forwarding) */

	return SR_OK ;
}
/* end subroutine (memfilter_lookup) */



