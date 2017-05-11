/* levo-rfu */



#ifndef	LEVORFU_INCLUDE
#define	LEVORFU_INCLUDE		1



#include	<sys/types.h>

#include	"localmisc.h"
#include	"levo-specific.h"



/* objects */

#define	RFU			struct rfu





struct rfu {
	struct operand	regs[LEVO_NREGS] ;
	int	tt ;
} ;



#endif /* LEVORFU_INCLUDE */



