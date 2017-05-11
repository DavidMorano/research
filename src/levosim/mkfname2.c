/* mkfname2 */

/* make a file name from two parts */



/* revision history :

	= 01/12/03, Dave Morano

	This was made specifically for the HDB UUCP modified code.


*/



#include	<sys/types.h>
#include	<sys/param.h>
#include	<string.h>

#include	<vsystem.h>




int mkfname2(ofname,p1,p2)
char	ofname[] ;
char	p1[], p2[] ;
{
	int	rlen ;
	int	ml ;

	char	*bp ;


	bp = ofname ;
	rlen = MAXPATHLEN ;

/* one */

	ml = strlcpy(bp,p1,rlen) ;

	if (ml >= rlen)
	    return SR_OVERFLOW ;

	bp += ml ;
	rlen -= ml ;

/* two */

	ml = strlcpy(bp,p2,rlen) ;

	if (ml >= rlen)
	    return SR_OVERFLOW ;

	bp += ml ;
	return (bp - ofname) ;
}
/* end subroutine (mkfname2) */



