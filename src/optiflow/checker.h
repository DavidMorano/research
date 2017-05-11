/* checker */

/* checker machine state */


/* revision history:

	= 01/08/06, Dave Morano

	This is needed for skipping instrcutions when using LevoSim.


*/


/******************************************************************************

	This is the CHECKER simulator state.


******************************************************************************/


#ifndef	CHECKER_INCLUDE
#define	CHECKER_INCLUDE	1


/* object instantiation defines */
#define	CHECKER		struct checker


#include	<sys/types.h>

#include	"localmisc.h"
#include	"ssas.h"
#include	"memory.h"			/* this is from S-S */
#include	"regs.h"			/* this is from S-S */



/* this is Alpha stuff */




/* checker simulator version of machine state for ALPHA ISA */

struct checker {
	ULONG		magic ;
	ULONG		ck ;
	ULONG		in ;		/* checker instruction number */
	SSAS		*cas ;		/* checker ISes */
	struct mem_t	*mem ;		/* checker "committed" memory */
	struct regs_t	regs ;		/* checker "committed" register file */
	int		wsize ;		/* checker window size */
	int		ri ;		/* read index for checker ISes */
	int		ui ;		/* use index for checker ISes */
	int		wi ;		/* write index for checker ISes */
	int		nw ;		/* instructions in checker window */
	int		nused ;		/* number used by functional sim */
} ;


extern int	checker_init(CHECKER *,struct mem_t *,struct regs_t *,
			ULONG,int) ;
extern int	checker_free(CHECKER *) ;


#endif /* CHECKER_INCLUDE */



