/* seqok */


/* revision history :

	= 01/05/17, Dave Morano

	This is for the transaction window sequence checking
	function.


*/




#include	<sys/types.h>

#include	"localmisc.h"




/* is a sequence number in our receive window ? */
int seqok(s,e,m)
uint	s, e, m ;
{
	uint	lim ;
	uint	m2 = (m >> 1) ;

	int	f ;


	lim = (e + m2) % m ;
	if (e < lim)
		f = ((s >= e) && (s < lim)) ;

	else
		f = ((s < lim) || (s >= e)) ;

	return f ;
}
/* end subroutine (seqok) */



