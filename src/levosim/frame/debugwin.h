/* debugwin */

/* debugging */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/



#ifndef	DEBUGWIN_INCLUDE
#define	DEBUGWIN_INCLUDE



#include	<sys/types.h>
#include	<limits.h>
#include	<time.h>

#include	"misc.h"



/* object defines */

#define	DEBUGWIN		struct debugwin_head




struct debugwin_flags {
	uint	init_vsd : 1 ;
	uint	init_vse : 1 ;
} ;

struct debugwin_head {
	LONG	ck_start, ck_n, ck_end ;
	LONG	in_start, in_n, in_end ;
	struct debugwin_flags	f ;
	int	debuglevel ;
} ;




#ifndef	DEBUGWIN_MASTER

extern int debugwin_init(DEBUGWIN *) ;
extern int debugwin_setlevel(DEBUGWIN *,int) ;
extern int debugwin_setopt(DEBUGWIN *,char *,int) ;
extern int debugwin_check(DEBUGWIN *,ULONG,ULONG) ;
extern int debugwin_free(DEBUGWIN *) ;

#endif /* DEBUGWIN_MASTER */


#endif /* DEBUGWIN_INCLUDE */



