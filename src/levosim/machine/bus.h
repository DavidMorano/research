/* bus */

/* a general bus for all known purposes ! (just deal with it) */


/* revision history :

	= 00/02/15, Dave Morano
	This code was started.


*/



#ifndef	BUS_INCLUDE
#define	BUS_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif



#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lflowgroup.h"



/* object defines */

#define	BUS		struct bus_head
#define	BUS_STATS	struct bus_statistics
#define	BUS_SFLAGS	struct bus_sflags
#define	BUS_INFO	struct bus_info
#define	BUS_CONTENT	LFLOWGROUP
#define	BUS_DATAVAL	unsigned int




struct bus_statistics {
	ULONG	idle ;		/* idle clocks (not requested) */
	ULONG	used ;		/* used clocks w/ data */
	ULONG	unused ; 	/* unused clocks (granted bus not used) */
	ULONG	holds ;		/* bus holds */
	ULONG	stalls ;	/* bus stalls */
	ULONG	conflicts ;	/* bus contention conflicts */
} ;

struct bus_sflags {
	uint	busy : 1 ;	/* bus busy */
	uint	hold : 1 ;	/* bus hold (stop new writes) */
} ;

struct bus_info {
	struct bus_sflags	f ;
	int	cmid ;		/* current master ID */
	int	nreqs ;		/* number of requests for this clock */
} ;

struct bus_state {
	struct bus_sflags	f ;
	struct bus_statistics	s ;	/* statistics */
	int	checksum ;
	int	mp ;		/* master position */
	int	cmid ;		/* current bus master ID */
	int	nreqs ;		/* number of requests for this clock */
} ;

struct bus_flags {
	uint	written : 1 ;	/* was bus written */
	uint	dp : 1 ;	/* data present on bus */
} ;

struct bus_head {
	unsigned long		magic ;
	struct bus_flags	f ;
	struct bus_state	n, c ;
	LFLOWGROUP		content ;	/* flow-group on bus */
	ULONG			clock ;
	struct proginfo		*pip ;
	int		*requests ; 	/* bus requests */
	int		nmasters ;
	int		width ;		/* bus width in units of 'dataval' */
} ;



#if	(! defined(BUS_MASTER)) || (BUS_MASTER == 0)

extern int bus_init(BUS *,struct proginfo *,int,int) ;
extern int bus_free(BUS *) ;
extern int bus_clock(BUS *) ;
extern int bus_comb(BUS *,int) ;
extern int bus_request(BUS *,int,int) ;
extern int bus_grantednext(BUS *,int) ;
extern int bus_hold(BUS *) ;
extern int bus_writecur(BUS *,BUS_CONTENT *) ;
extern int bus_writenext(BUS *,BUS_CONTENT *) ;
extern int bus_read(BUS *,BUS_CONTENT *) ;
extern int bus_bread(BUS *,BUS_CONTENT *) ;
extern int bus_getstats(BUS *,BUS_STATS *) ;
extern int bus_getinfo(BUS *,BUS_INFO *) ;

#endif /* BUS_MASTER */


#endif /* BUS_INCLUDE */



