/* libus */

/* bus to handle data transactions (read/write) to and from memory */


#define	F_DEBUGS	0


/* revision history :

	= 00/05/26, Dave Morano

	This object was taken from the previous 'busmemdata' object.


	= 01/05/11, Dave Morano

	I changed this code so that a single register delay is incurred
	between data that is written and data that is read.


*/


/******************************************************************************

	This is the bus that the I-fetch unit reads instruction words
	off of when they come back from I-cache.

	Operating rules :

	+ there are no bus masters !
	+ there is a hold function


******************************************************************************/


#define	LIBUS_MASTER	0


#include	<sys/types.h>
#include	<climits>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"xmlinfo.h"

#include	"lflowgroup.h"
#include	"levoinfo.h"
#include	"libus.h"



/* local defines */

#define	LIBUS_MAGIC	137842746



/* external subroutines */


/* forward references */

static int libus_handleshift(LIBUS *,struct proginfo *,struct levoinfo *) ;
static int libus_clear(LIBUS *) ;






int libus_init(busp,pip,mip,lip,width)
LIBUS		*busp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
int		width ;
{
	int	rs, i ;
	int	size ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (width <= 0)
	    width = 1 ;

	(void) memset(busp,0,sizeof(LIBUS)) ;

	busp->pip = pip ;
	busp->mip = mip ;
	busp->lip = lip ;

	busp->width = width ;


	busp->f.shift = FALSE ;

/* allocate some stuff */

	size = 2 * width * sizeof(LFLOWGROUP) ;
	rs = uc_malloc(size,&busp->a.fgs) ;

	if (rs < 0)
	    goto bad1 ;

	(void) memset(busp->a.fgs,0,size) ;

	busp->c.fgs = busp->a.fgs + 0 ;
	busp->n.fgs = busp->a.fgs + width ;

	for (i = 0 ; i < width ; i += 1) {

	    lflowgroup_clear(busp->c.fgs + i,0) ;

	    lflowgroup_clear(busp->n.fgs + i,0) ;

	} /* end for */

	if (rs < 0)
	    goto bad2 ;


	busp->n.nfgs = busp->c.nfgs = 0 ;


	busp->magic = LIBUS_MAGIC ;
	return rs ;

/* things come here */
bad2:
	free(busp->a.fgs) ;

bad1:
	return rs ;
}
/* end subroutine (libus_init) */


/* free up this object */
int libus_free(busp)
LIBUS	*busp ;
{
	int	rs, i ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	if (busp->a.fgs != NULL) {

	    free(busp->a.fgs) ;

	    busp->n.fgs = busp->c.fgs = NULL ;
	    busp->a.fgs = NULL ;

	} /* end if */

	busp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (libus_free) */


/* perform the combinatorial computations */
int libus_comb(busp,phase)
LIBUS	*busp ;
int	phase ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	pip = busp->pip ;
	lip = busp->lip ;

	switch (phase) {

/* requests happen here */
	case 0:
	    (void) libus_clear(busp) ;

	    busp->f.shift = FALSE ;
	    busp->n.f.hold = FALSE ;
	    break ;

	case 1:
	    break ;

/* grants are available here */
	case 2:
	    break ;

	case 3:
	    break ;

	case 4:
	    rs = libus_handleshift(busp,pip,lip) ;

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (libus_comb) */


/* the system clocks us */
int libus_clock(busp)
LIBUS	*busp ;
{
	    LFLOWGROUP	*fgp ;

	int	size ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

/* we copy, not swap */

	    fgp = busp->c.fgs ;
	    busp->c = busp->n ;		/* all opaque */
	    busp->c.fgs = fgp ;

	    size = busp->n.nfgs * sizeof(LFLOWGROUP) ;
	    (void) memcpy(busp->c.fgs,busp->n.fgs,size) ;

	return SR_OK ;
}
/* end subroutine (libus_clock) */


/* hold the bus (called in phase 1 on) */
int libus_hold(busp)
LIBUS	*busp ;
{


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	busp->n.f.hold = TRUE ;
	return SR_OK ;
}
/* end subroutine (libus_hold) */


/* write the CURRENT clock cycle */
int libus_write(busp,dp,n)
LIBUS		*busp ;
LFLOWGROUP	*dp ;
int		n ;
{
	int	rs = SR_OK, i, j ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	if (n > busp->width)
	    return SR_TOOBIG ;

	if (busp->c.f.hold)
	    return SR_BUSY ;

	for (i = 0 ; i < n ; i += 1)
	    busp->n.fgs[i] = dp[i] ;

	for (j = i ; j < busp->width ; j += 1)
	    (void) memset(busp->n.fgs + j,0,sizeof(LFLOWGROUP)) ;

	busp->n.nfgs = n ;
	busp->n.f.dp = TRUE ;
	busp->n.f.busy = TRUE ;

	return rs ;
}
/* end subroutine (libus_write) */


/* read the data off of the bus (if there was any) */
int libus_read(busp,dp,n)
LIBUS		*busp ;
LFLOWGROUP	*dp ;
int		n ;
{
	int	rs = SR_OK, i ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	if ((! busp->c.f.busy) || (! busp->c.f.dp))
	    return 0 ;

	if (n < busp->c.nfgs)
	    return SR_TOOBIG ;

	for (i = 0 ; i < n ; i += 1)
	    dp[i] = busp->c.fgs[i] ;

	rs = n ;
	return rs ;
}
/* end subroutine (libus_read) */


/* shift anything that needs shifting ! */
int libus_shift(busp)
LIBUS	*busp ;
{


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	busp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (libus_shift) */


int libus_xmlinit(busp,xip)
LIBUS	*busp ;
XMLINFO	*xip ;
{
	int	rs, size ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	size = sizeof(struct libus_xml) ;
	rs = uc_malloc(size,&busp->xsp) ;

	if (rs < 0)
		return rs ;

	(void) memset(busp->xsp,0,size) ;

	return SR_OK ;
}
/* end subroutine (libus_xmlinit) */


int libus_xmlout(busp,xip)
LIBUS	*busp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	int	rs, i ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	pip = busp->pip ;

	bprintf(&xip->xmlfile,"<libus>\n") ;

	bprintf(&xip->xmlfile,"<uid>%p</uid>\n",
		busp) ;

	bprintf(&xip->xmlfile,"<hold>%d</hold>\n", busp->c.f.hold) ;

	bprintf(&xip->xmlfile,"<busy>%d</busy>\n", busp->c.f.busy) ;

	bprintf(&xip->xmlfile,"<dp>%d</dp>\n", busp->c.f.dp) ;

	if (busp->c.f.dp) {

		for (i = 0 ; i < busp->c.nfgs ; i += 1) {

	bprintf(&xip->xmlfile,"<datalane>\n") ;

	bprintf(&xip->xmlfile,"<id>%d</id>\n",i) ;

			lflowgroup_xmlout(busp->c.fgs + i,xip) ;

	bprintf(&xip->xmlfile,"</datalane>\n") ;

		} /* end for */

	} /* end if (data present) */

	bprintf(&xip->xmlfile,"</libus>\n") ;


	return SR_OK ;
}
/* end subroutine (libus_xmlout) */


int libus_xmlfree(busp,xip)
LIBUS	*busp ;
XMLINFO	*xip ;
{


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != LIBUS_MAGIC)
	    return SR_NOTOPEN ;

	if (busp->xsp != NULL) {

		free(busp->xsp) ;

		busp->xsp = NULL ;

	} /* end if */


	return SR_OK ;
}
/* end subroutine (libus_xmlfree) */



/* PRIVATE SUBROUTINES */



static int libus_clear(busp)
LIBUS	*busp ;
{


	busp->n.f.dp = FALSE ;
}
/* end subroutine (libus_clear) */


static int libus_handleshift(busp,pip,lip)
LIBUS		*busp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	i ;
	int	totalrows = lip->totalrows ;
	int	tt_test ;


	if (! busp->f.shift)
	    return SR_OK ;

	busp->f.shift = FALSE ;
	tt_test = INT_MIN + totalrows ;
	for (i = 0 ; i < busp->width ; i += 1) {

	    if (busp->n.fgs[i].ftt >= tt_test)
	    	busp->n.fgs[i].ftt -= totalrows ;

	    if (busp->n.fgs[i].tt >= tt_test)
	    	busp->n.fgs[i].tt -= totalrows ;

	    if (busp->n.fgs[i].rtt >= tt_test)
	    	busp->n.fgs[i].rtt -= totalrows ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (libus_handleshift) */



