/* branchinfo */

/* branchinfo branch detector */



/* revision history :

	= 00/07/27, Dave Morano

	This code was started.


*/


#ifndef	BRANCHINFO_INCLUDE
#define	BRANCHINFO_INCLUDE	1



#include	<sys/types.h>

#include	<hdb.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"



#ifndef	UINT
#define	UINT	unsigned int
#endif



#define	BRANCHINFO		struct branchinfo_head
#define	BRANCHINFO_ENTRY	struct branchinfo_e




struct branchinfo_ef {
	uint	hammock : 1 ;
} ;

struct branchinfo_e {
	ulong	visited ;		/* number times visited */
	ulong	taken ;			/* number times taken */
	struct branchinfo_ef	f ;
	uint	ia ;
	uint	ta ;
} ;

struct branchinfo_head {
	unsigned long		magic ;
	struct proginfo		*pip ;		/* program information */
	HDB			bi ;
} ;




#if	(! defined(BRANCHINFO_MASTER)) || (BRANCHINFO_MASTER == 0)

extern int	branchinfo_init(BRANCHINFO *,struct proginfo *,int) ;
extern int	branchinfo_hammock(BRANCHINFO *,uint,int) ;
extern int	branchinfo_visit(BRANCHINFO *,uint,uint,int) ;
extern int	branchinfo_free(BRANCHINFO *) ;

#endif /* BRANCHINFO_MASTER */


#endif /* BRANCHINFO_INCLUDE */



