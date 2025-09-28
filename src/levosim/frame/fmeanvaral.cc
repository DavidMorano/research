/* fmeanvaral */

/* calculate the mean and variance on some integral numbers */



/* revision history :

	= 95/02/16, Dave Morano

	This was originally written for performing some statistical
	calculations for LPPI computer development (specifically I/O
	hardware buffer sizes).


	= 95/10/17, Dave Morano

	I borrowed this code from a previous subroutine that did
	the same thing except I changed the type of the integral
	variables (in the array) to 'ULONG' (a 64-bit integer).


*/



#include	<sys/types.h>

#include	<usystem.h>

#include	"localmisc.h"





int fmeanvaral(a,n,mp,vp)
ULONG	*a ;
int	n ;
double	*mp, *vp ;
{
	double	s1, s2 ;
	double	m1, m2 ;
	double	x, fn ;

	int	i ;


	if (a == NULL)
	    return SR_FAULT ;

	if (n == 0) {

	    if (mp != NULL)
	        *mp = 0.0 ;

	    if (vp != NULL)
	        *vp = 0.0 ;

	    return SR_OK ;
	}

	s1 = 0.0 ;
	s2 = 0.0 ;
	for (i = 0 ; i < n ; i += 1) {

	    x = (double) a[i] ;
	    s1 += x ;
	    s2 += (x * x) ;

	} /* end for */

	fn = (double) n ;
	m1 = s1 / fn ;
	if (mp != NULL)
	    *mp = m1 ;

	m2 = 0.0 ;
	if (vp != NULL) {

	    m2 = s2 / fn ;
	    *vp = m2 - (m1 * m1) ;

	}

	return SR_OK ;
}
/* end subroutine (fmeanvaral) */



