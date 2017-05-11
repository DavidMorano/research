/* sscheckstate */

/* checker machine state */


/* revision history:

	= 01/08/06, Dave Morano

	This is needed for skipping instrcutions when using LevoSim.


*/


/******************************************************************************

	This is the CHECKER simulator state.


******************************************************************************/


#ifndef	SSMACHSTATE_INCLUDE
#define	SSMACHSTATE_INCLUDE	1


#include	<sys/types.h>

#include	"localmisc.h"
#include	"ssas.h"
#include	"regs.h"			/* this is from S-S */



/* this is Alpha stuff */




/* checker simulator version of machine state for ALPHA ISA */

struct sscheckstate {
	ULONG		in ;		/* leading instruction number */
	SSAS		*cas ;		/* checker ASes */
	struct mem_t	*mem ;		/* checker "committed" memory */
	struct regs_t	regs ;		/* checker "committed" register file */
	int		wsize ;		/* checker window size */
	int		ri ;		/* read index for checker ASes */
	int		wi ;		/* write index for checker ASes */
	int		nw ;		/* instructions in checker window */
	int		nused ;		/* number used by functional sim */
} ;



#endif /* SSMACHSTATE_INCLUDE */



