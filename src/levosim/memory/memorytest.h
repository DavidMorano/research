/* memory test */

/* Levo memory subsystem tester component */



/* revision history :

	= 01/02/23, Maryam Ashouei
	This code was started.
*/




#include	<usystem.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"mintinfo.h"
#include	"levoinfo.h"
#include	"bus.h"
#include	"lbusint.h"
#include	"lflowgroup.h"



#ifndef	MEMORYTEST_INCLUDE
#define	MEMORYTEST_INCLUDE		1



#define	MEMORYTEST		struct memorytest_head





struct memorytest_head {

	struct proginfo		*pip ;		/* program information */
	struct levoinfo		*lip ;		/* Levo information */
	struct mintinfo		*mip ;		/* MINT information */
	RFBUS			*MFB;
        RFBUS                   *MBB;
        RFBUS                   *MWB;
        int                     interleave;
        int                     interleave_scheduling;
 	int			value ;
} ;








#endif /* BUSTEST_INCLUDE */



