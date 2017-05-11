/* lsg */

/* Levo Sharing Group */



/* revision history :

	= 00/02/15, Dave Morano
	This code was started.


*/





#ifndef	LSG_INCLUDE
#define	LSG_INCLUDE	1


#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"busfr.h"
#include	"lpe.h"
#include	"las.h"



/* object defines */

#define	LSG		struct lsg_head


#define	LSG_DEFAS	1




struct lsg_head {
	struct proginfo	*gp ;		/* startup information */
	struct mintinfo	*mip ;		/* MINT information */
	struct levoinfo	*lip ;		/* Levo information */
	BUSFR	*frbuses ;		/* forwarding buses */
	LAS	*lass ;			/* our active stations */
	LPE	*lpes ;			/* our PEs */
	int	pi ;			/* our physical index */
} ;



typedef struct lsg_head		lsg ;



#ifndef	LSG_MASTER


/* stuff */

#endif /* LSG_MASTER */


#endif /* LSG_INCLUDE */



