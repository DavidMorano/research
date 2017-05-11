/* levo-pfu */


#ifndef	LEVOPFU_INCLUDE
#define	LEVOPFU_INCLUDE		1



#include	<sys/types.h>

#include	"localmisc.h"
#include	"levo-specific.h"



/* objects */

#define	PFU			struct pfu




struct pfu {
	struct operand	preds[LEVO_NPREDS] ;
	int	tt ;
} ;



#endif /* LEVOPFU_INCLUDE */



