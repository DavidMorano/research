/* statemips */


/* revision history :

	= 01/08/06, Dave Morano

	This is needed for skipping instrcutions when using LevoSim.


*/



#ifndef	STATEMIPS_INCLUDE
#define	STATEMIPS_INCLUDE	1




#include	<sys/types.h>

#include	"misc.h"
#include	"lmipsregs.h"




/* architected state of the MIPS machine */

struct statemips {
	ULONG	in ;
	uint	regs[LMIPSREG_NREGS] ;
	uint	ia ;
} ;



#endif /* STATEMIPS_INCLUDE */



