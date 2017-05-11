/* levo-mfu */


#ifndef	LEVOMFU_INCLUDE
#define	LEVOMFU_INCLUDE		1



#include	<sys/types.h>

#include	"localmisc.h"
#include	"levo-specific.h"



/* objects */

#define	MFU			struct mfu




struct mfu {
	struct operand	cache[LEVO_NMEMS] ;
	struct operand	pcb[LEVO_NMEMS] ;
	int	tt ;
} ;



#endif /* LEVOMFU_INCLUDE */



