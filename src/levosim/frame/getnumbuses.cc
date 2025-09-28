/* getnumbuses */

/* calculate the number of buses from an interleave schedule */


#define	CF_DEBUGS	0		/* non-switchable */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/




/******************************************************************************

	This subroutine calculates the number of buses (or memory banks)
	given an interleave schedule for that set of buses (or memory
	block).


******************************************************************************/




#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>

#include	"localmisc.h"
#include	"defs.h"



/* local defines */



/* external subroutines */

extern int	fbscounti(uint), ffbsi(uint), flbsi(uint) ;


/* local structures */


/* forward references */





/* calculate the number of buses from an interleave schedule */
int getnumbuses(inter)
uint	inter ;
{
	int	n ;


	n = fbscounti(inter) ;

	return (1 << n) ;		/* n = 2 ^ n */
}
/* end subroutine (getnumbuses) */



