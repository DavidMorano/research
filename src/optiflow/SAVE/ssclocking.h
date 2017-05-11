/* ssclocking */

/* simulator control related (utility) information */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started (for LevoSim).


*/




#ifndef	SSCLOCKING_INCLUDE
#define	SSCLOCKING_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif




/* object defines (whatever) */

#define	SSCLOCKING		struct ssclocking_head
#define	SSCLOCKING_COE		struct ssclocking_coe
#define	SSCLOCKING_MACHOBJ	struct ssclocking_machobj



/* our dependencies */

#include	<hdb.h>
#include	<veclist.h>
#include	<plainq.h>

#include	"paramfile.h"

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"



/* other defines */

#define	SSCLOCKING_MAXCLOCKS		200
#define	SSCLOCKING_NQUEUES		400
#define	SSCLOCKING_MINPHASES		6




struct ssclocking_machobj {
	void	*topobjp ;
	int	(*topcombp)(void *,int) ;
	int	(*topclockp)(void *) ;
} ;

struct ssclocking_mp {
	uint	data ;
	uint	stack ;
} ;

/* a call-out entry (THE TOP TWO ENTRIES MUST BE IN THAT ORDER) */
struct ssclocking_coe {
	LONG	next ;		/* queue operations */
	LONG	prev ;		/* for queue operations */
	int	(*func)() ;
	void	*objp, *argp ;
	ULONG	wake ;
	ULONG	key ;		/* hash key */
	uint	relative ;
	uint	phase ;
} ;

struct ssclocking_flags {
	uint	syscalls : 1 ;
} ;

struct ssclocking_head {
	unsigned long		magic ;
	struct ssclocking_flags	f ;
	struct proginfo	*pip ;
	ULONG		clock ;		/* this is the main simulator clock ! */
	ULONG		maxclocks ;	/* maximum number of clocks */
	HDB		pq ;		/* pending (future) queue */
	PLAINQ		fq ;		/* free queue */
	veclist		*qes ;		/* fast queues */
	void	*topobjp ;
	int	(*topcombp)(void *,int) ;
	int	(*topclockp)(void *) ;
	int	nq ;		/* number of queues */
	int	cqi ;		/* current queue index */
	int	clock_count ;	/* incremental clock counter */
	int	nentries ;	/* number of "fast" queues */
	int	phase ;		/* current clock phase */
} ;



#if	(! defined(SSCLOCKING_MASTER)) && (! defined(SSCLOCKING_MASTER))

extern int ssclocking_init(SSCLOCKING *,struct proginfo *,
			ULONG,SSCLOCKING_MACHOBJ *) ;
extern int ssclocking_setmach(SSCLOCKING *,SSCLOCKING_MACHOBJ *) ;
extern int ssclocking_callout(SSCLOCKING *,int (*)(),void *,void *,uint,int) ;
extern int ssclocking_loop(SSCLOCKING *) ;
extern int ssclocking_getclock(SSCLOCKING *,ULONG *) ;
extern int ssclocking_free(SSCLOCKING *) ;

#endif /* SSCLOCKING_MASTER */


#endif /* SSCLOCKING_INCLUDE */



