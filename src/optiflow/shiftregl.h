/* shiftregl */

/* Shift Register of Longs */



/* revision history :

	= 01/07/10, Dave Morano

	This code was started.


*/




#ifndef	SHIFTREGL_INCLUDE
#define	SHIFTREGL_INCLUDE

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines */

#define	SHIFTREGL	struct shiftregl_head



#include	<usystem.h>

#include	"misc.h"






struct shiftregl_head {
	unsigned long	magic ;
	ULONG		*regs ;
	int		n ;
	int		i ;		/* next write index */
} ;



#if	(! defined(SHIFTREGL_MASTER)) || (SHIFTREGL_MASTER == 0)

extern int shiftregl_init(SHIFTREGL *,int) ;
extern int shiftregl_free(SHIFTREGL *) ;
extern int shiftregl_shiftin(SHIFTREGL *,ULONG) ;
extern int shiftregl_get(SHIFTREGL *,int,ULONG *) ;

#endif /* SHIFTREGL_MASTER */


#endif /* SHIFTREGL_INCLUDE */





