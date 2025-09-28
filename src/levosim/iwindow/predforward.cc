/* predforward */

/* Levo Forwarding Register Filter */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		1		/* switchable debug print-outs */
#define	F_SAFE		1		/* want it extra safe ? */


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written.


*/


/**************************************************************************

	This object module provides the function of the Forwarding
	Register Bus Filter hardware component in the Levo machine.



**************************************************************************/


#define	PREDFORWARD_MASTER	1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"bus.h"
#include	"predforward.h"



/* local defines */

#define	PREDFORWARD_MAGIC	0x95847362



/* external subroutines */


/* forward referecens */

static int predforward_busread(PREDFORWARD *) ;
static int predforward_buswrite(PREDFORWARD *) ;






int predforward_init(rfp,gp,mip,lip,rbus,wbus,n)
PREDFORWARD	*rfp ;
struct proginfo	*gp ;
LSIM		*mip ;
struct levoinfo	*lip ;
BUS		*rbus, *wbus ;
int		n ;		/* number of FIFO registers */
{
	int	rs = SR_OK ;
	int	i, size ;


	(void) memset(rfp,0,sizeof(PREDFORWARD)) ;

	rfp->gp = gp ;
	rfp->mip = mip ;
	rfp->lip = lip ;

	rfp->rbus = rbus ;
	rfp->wbus = wbus ;
	rfp->nqentries = n ;

/* allocate our registers */

	rfp->nregs = PREDFORWARD_NREGS * lip->npaths ;
	size = 2 * rfp->nregs * sizeof(PREDFORWARD_REG) ;
	rs = uc_malloc(size,(void *) &rfp->c.regs) ;

	if (rs < 0) 
		goto bad1 ;

	rfp->n.regs = rfp->c.regs + (size >> 1) ;

/* this is not needed depending on the exact update policy */

	for (i = 0 ; i < rfp->nregs ; i += 1) {

		rfp->c.regs[i].tt = -1 ;
		rfp->n.regs[i].tt = -1 ;

	}

bad1:
	return rs ;
}
/* end subroutine (predforward_init) */


int predforward_free(rfp)
PREDFORWARD	*rfp ;
{


	if (rfp->c.regs != NULL)
		free(rfp->c.regs) ;

	rfp->c.regs = NULL ;
	rfp->n.regs = NULL ;
	return SR_OK ;
}


/* handle a clock transition */
int predforward_clock(rfp)
PREDFORWARD	*rfp ;
{
	PREDFORWARD_REG	*rp ;


/* we swap the allocated parts and copy the rest */

	rp = rfp->c.regs ;		/* save allocated pointer */

	rfp->c = rfp->n ;		/* copy everything */

	rfp->n.regs = rp ;		/* swap in allocated pointer */

	return SR_OK ;
}
/* end subroutine (predforward_clock) */


/* perform the combinatorial computations */
int predforward_comb(rfp,phase)
PREDFORWARD	*rfp ;
int		phase ;
{
	int	rs = SR_OK ;
	int	j ;


/* the default is for the next state to be the same as the current state */

	switch (phase) {

	case 0:
	    rfp->f.shift = FALSE ;
	    rfp->f.forward = FALSE ;

	    rs = predforward_buswrite(rfp) ;

	    rs = predforward_lookup(rfp) ;

	    break ;

	case 1:
	    break ;

	case 2:
	    rs = predforward_busread(rfp) ;

	    break ;

	case 3:
	    rs = predforward_handleshift(rfp) ;

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (predforward_comb) */


/* shift the machine (called in phase 1) */
int predforward_shift(rfp)
PREDFORWARD	*rfp ;
{


#if	F_SAFE
	if (rfp == NULL)
	    return SR_FAULT ;

	if ((rfp->magic != PREDFORWARD_MAGIC) && (rfp->magic != 0)) {

		eprintf("predforward_shift: bad format\n") ;

		return SR_BADFMT ;
	}

	if (rfp->magic != PREDFORWARD_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */


	rfp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (predforward_shift) */



/* INTERNAL SUBROUTINES */



/* read our incoming bus */
static int predforward_busread(rfp)
PREDFORWARD	*rfp ;
{
	BUS_CONTENT	rd ;

	int	rs = SR_OK ;


/* read the bus and see if there is anything there for us */

	    rs = bus_read(rfp->rbus,&rd) ;

	    if (rs > 0) {

		int	f_error = FALSE ;


		if ((! f_error) && rd.dp[0].any)
			rfp->n.busdata = rd ;

	    	else
			rfp->n.busdata.dp[0].any = FALSE ;

		} /* end if (something was on the bus) */


	retrun SR_OK ;
}
/* end subroutine (predforward_busread) */


/* write the output bus */
static int predforward_buswrite(rfp)
PREDFORWARD	*rfp ;
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
/* end subroutine (predforward_buswrite) */


/* do we have a machine shift */
static int predforward_handleshift(rfp)
PREDFORWARD	*rfp ;
{
	int	rs, i, j ;
	int	addr ;
	int	totalrows ;
	int	npaths ;


	if (rfp->f.shift) {

	    rfp->f.shift = FALSE ;
	    for (j = 0 ; j < PREDFORWARD_NREGS ; j += 1)
	        rfp->n.regs[j].tt -= totalrows ;

		if (rfp->n.busdata.dp[0])
	        	rfp->n.busdata.tt -= totalrows ;

	} /* end if (machine shift) */

	return SR_OK ;
}
/* end subroutine (predforward_handleshift) */


/* lookup to see if the present forward operation must forward again */
static int predforward_lookup(rfp)
PREDFORWARD	*rfp ;
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
			if ((rfp->c.busdata.tt >= rfp->c.regs[i].tt) &&
				(rfp->c.busdata.dv.l != rfp->c.regs[i].dv.l))
					rfp->f.forward = TRUE ;

		} /* end if */

/* do we have to forward ? */

		if (f_forward) {





	} /* end if (forwarding) */

	return SR_OK ;
}
/* end subroutine (predforward_lookup) */



