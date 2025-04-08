/* getinterleave */

/* get a memory interleave index from an address and interleave schedule */


#define	CF_DEBUGS	0		/* non-switchable debug print-outs */


/* revision history :

	= 00/07/18, Dave Morano

	This subroutine was written to calculate the proper bus
	to use when interleaved buses are provided.


*/


/******************************************************************************

	This subroutine takes an interleave schedule and an address and
	calculates which interleaved bus (by bus index) should be used
	for for the associated operation given the address.


	Synopsis :

	int getinterleave(interleave,addr)
	uint	interleave, addr ;


	Arguments :

	+ interleave	the interleave schedule
	+ address	the address that is in view


	Returns :

	index		the bus index of the interleaved buses



******************************************************************************/



#include	<sys/types.h>

#include	<bitops.h>

#include	"localmisc.h"



/* external subroutines */

extern int	ffbsi(uint) ;






int getinterleave(interleave,addr)
uint	interleave, addr ;
{
	int	result = 0 ;
	int	i ;
	int	bn ;


	i = 0 ;
	bn = ffbsi(interleave) ;

	while (bn >= 0) {

	    if (BTSTI(&addr,bn))
	        result |= (1 << i) ;

	    BCLRI(&interleave,bn) ;

	    i += 1 ;
	    bn = ffbsi(interleave) ;

	} /* end while */

	return result ;
}
/* end subroutine (getinterleave) */



