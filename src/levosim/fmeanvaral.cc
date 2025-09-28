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


/******************************************************************************

	Floating Point Mean/Variance Calculator (for Arrays of LONGs)

	This subroutine will calculate the mean and variance for
	an array of 64-bit integer values.  The results are floating
	point values.

	Synopsis :

	int fmeanvaral(a,n,mp,vp)
	ULONG	*a ;
	int	n ;
	double	*mp, *vp ;


	Arguments :

	a	array of LONG (64-bit) integers
	n	number of elements in array
	mp	pointer to double to hold the 'mean' result
	mp	pointer to double to hold the 'variance' result

	Returns :

	>=0	OK
	<0	something bad



******************************************************************************/




#include	<sys/types.h>

#include	<usystem.h>

#include	"localmisc.h"





int fmeanvaral(a,n,mp,vp)
ULONG	a[] ;
int	n ;
double	*mp, *vp ;
{
	double	s1, s2 ;
	double	m1, m2 ;
	double	x, p, fnp ;

	ULONG	np ;

	int	rs, i ;


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
	np = 0 ;
	for (i = 0 ; i < n ; i += 1) {

	    x = (double) i ;
	    p = (double) a[i] ;
	    np += a[i] ;
	    s1 += (p * x) ;
	    s2 += (p * x * x) ;

	} /* end for */

	fnp = (double) np ;
	m1 = s1 / fnp ;
	if (mp != NULL)
	    *mp = m1 ;

	if (vp != NULL) {

	    m2 = s2 / fnp ;
	    *vp = m2 - (m1 * m1) ;

	}

	return (np > 0) ? 0 : SR_RANGE ;
}
/* end subroutine (fmeanvaral) */



