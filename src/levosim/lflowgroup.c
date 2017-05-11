/* lflowgroup */

/* Levo Flow Group */



/* revision history :

	= 00/06/28, Dave Morano

	This was moved here from the old 'bus' object.
	Alireza and I agreed to separate out these different
	functions manybe two weeks ago.


*/


/******************************************************************************

	Levo Flow Group (whatever !)

	This object, the structure along with these subroutines,
	provides a basic unit of data and manipulation of that data
	for much of the Levo i-window.


******************************************************************************/


#include	<sys/types.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lflowgroup.h"




/* local data */

static const uint	dps[] = {
	8,
	12,
	14,
	15
} ;




int lflowgroup_init(lfgp)
LFLOWGROUP	*lfgp ;
{
	int	rs = SR_OK ;


	if (lfgp == NULL)
		return SR_FAULT ;

	(void) memset(lfgp,0,sizeof(LFLOWGROUP)) ;

	return rs ;
}
/* end subroutine (lflowgroup_init) */


int lflowgroup_free(lfgp)
LFLOWGROUP	*lfgp ;
{


	if (lfgp == NULL)
		return SR_FAULT ;

	return SR_OK ;
}
/* end subroutine (lflowgroup_free) */


int lflowgroup_loadbytes(lfgp,bytes,n)
LFLOWGROUP	*lfgp ;
uchar		bytes[] ;
int		n ;
{


	if (lfgp == NULL)
		return SR_FAULT ;

	if (n > sizeof(uint))
		return SR_TOOBIG ;

	(void) memcpy((uchar *) &lfgp->dv,bytes,n) ;

	    lfgp->dp = dps[n] ;
	return SR_OK ;
}
/* end subroutine (lflowgroup_loadbytes) */


/* clear out a FlowGroup */
int lflowgroup_clear(lfgp,ftt)
LFLOWGROUP	*lfgp ;
int		ftt ;		/* forwarder's TT */
{


	if (lfgp == NULL)
		return SR_FAULT ;

	(void) memset(lfgp,0,sizeof(LFLOWGROUP)) ;

	lfgp->ftt = ftt ;
	return SR_OK ;
}
/* end subroutine (lflowgroup_clear) */


/* swap-copy the second into the first */
int lflowgroup_swapcopy(g1p,g2p)
LFLOWGROUP	*g1p, *g2p ;
{
	LFLOWGROUP	tmp ;

	int	rs = SR_OK ;


	if ((g1p == NULL) || (g2p == NULL))
		return SR_FAULT ;

	tmp = *g1p ;
	*g1p = *g2p ;
	*g2p = tmp ;

	return rs ;
}
/* end subroutine (lflowgroup_swapcopy) */


/* set the forwarder's time-tag value */
int lflowgroup_setftt(lfgp,ftt)
LFLOWGROUP	*lfgp ;
int		ftt ;
{


	if (lfgp == NULL)
		return SR_FAULT ;

	    lfgp->ftt = ftt ;
	return SR_OK ;
}
/* end subroutine (lflowgroup_setftt) */


#ifdef	XML

/* XML stuff */
int lflowgroup_xmlinit(lfgp,xip)
LFLOWGROUP	*lfgp ;
XMLINFO		*xip ;
{


	if (lfgp == NULL)
		return SR_FAULT ;

	return SR_OK ;
}
/* end subroutine (lflowgroup_xmlinit) */


#ifdef	COMMENT

	int	ftt ;		/* forwarder's time-tag value */
	int	ott ;		/* original time-tag value */
	int	tt ;		/* time-tag (for snooping) */
	int	rtt ;		/* reset time-tag value */
	uint	seq ;		/* sequence number */
	uint	addr ;		/* data */
	uint	dv ;		/* data value (32 bits) */
	uint	trans : 8 ;	/* type of transaction */
	uint	path : 4 ;	/* data (which path are we ?) */
	uint	dp : 5 ;	/* data present bits (one for each byte) */
	uint	f_resp : 1 ;	/* response bit */

#endif /* COMMENT **/

int lflowgroup_xmlout(lfgp,xip)
LFLOWGROUP	*lfgp ;
XMLINFO		*xip ;
{


	if (lfgp == NULL)
		return SR_FAULT ;

	bprintf(&xip->xmlfile,"<lflowgroup>\n") ;

	bprintf(&xip->xmlfile,"<ftt>%d</ftt>\n",lfgp->ftt) ;

	bprintf(&xip->xmlfile,"<ott>%d</ott>\n",lfgp->ott) ;

	bprintf(&xip->xmlfile,"<tt>%d</tt>\n",lfgp->tt) ;

	bprintf(&xip->xmlfile,"<seq>%d</seq>\n",lfgp->seq) ;

	bprintf(&xip->xmlfile,"<addr>\\x%08x</addr>\n",lfgp->addr) ;

	bprintf(&xip->xmlfile,"<dv>\\x%08x</dv>\n",lfgp->dv) ;

	bprintf(&xip->xmlfile,"<trans>%d</trans>\n",lfgp->trans) ;

	bprintf(&xip->xmlfile,"<path>%d</path>\n",lfgp->path) ;

	bprintf(&xip->xmlfile,"<dp>%d</dp>\n",lfgp->dp) ;


	bprintf(&xip->xmlfile,"</lflowgroup>\n") ;

	return SR_OK ;
}
/* end subroutine (lflowgroup_xmlout) */


int lflowgroup_xmlfree(lfgp,xip)
LFLOWGROUP	*lfgp ;
XMLINFO		*xip ;
{


	if (lfgp == NULL)
		return SR_FAULT ;

	return SR_OK ;
}
/* end subroutine (lflowgroup_xmlfree) */

#endif /* XML */




