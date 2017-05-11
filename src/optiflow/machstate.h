/* machstate */


/* revision history :

	= 01/08/06, Dave Morano

	This is needed for skipping instrcutions when using LevoSim.


*/


/******************************************************************************

	This is the FUNCTIONAL simulator state.


******************************************************************************/


#ifndef	MACHSTATE_INCLUDE
#define	MACHSTATE_INCLUDE	1




#include	<sys/types.h>

#include	"localmisc.h"



/* this is Alpha stuff */

#define	MACHSTATE_NREGS		(64 + 3)	/* Alpha */
#define	MACHSTATE_MAXOPSI	4		/* Alpha, 3 reg + 1 mem */
#define	MACHSTATE_MAXOPSO	3		/* Alpha, 2 reg + 1 mem  */



/* architected state of the ALPHA machine */

struct machstate {
	ULONG	in ;				/* instruction number */
	ULONG	ia ;				/* instruction address */
	/* ULONG	regs[MACHSTATE_NREGS] ; */
} ;



#endif /* MACHSTATE_INCLUDE */



