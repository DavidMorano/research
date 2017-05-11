/* bustrace */

/* Levo Bus Monitor component */



/* revision history :

	= 00/07/27, Dave Morano

	This code was started.


*/



#include	<sys/types.h>

#include	<vsystem.h>
#include	<bio.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"bus.h"
#include	"lflowgroup.h"



#ifndef	BUSTRACE_INCLUDE
#define	BUSTRACE_INCLUDE		1

#ifndef	UINT
#define	UINT	unsigned int
#endif



#define	BUSTRACE		struct bustrace_head





struct bustrace_state {
	ULONG	clock ;			/* machine clock for faster access */
} ;

struct bustrace_stats {
	ULONG	total ;
	ULONG	data_total ;
	ULONG	nodata_total ;
	ULONG	stores_data ;
	ULONG	stores_nodata ;
	ULONG	nullifys_data ;
	ULONG	nullifys_nodata ;
} ;

struct bustrace_flags {
	uint	active : 1 ;		/* are we even active ? */
} ;

struct bustrace_head {
	unsigned long		magic ;
	struct proginfo		*pip ;		/* program information */
	LSIM			*mip ;		/* MINT information */
	struct bustrace_flags	f ;
	struct bustrace_stats	s ;		/* collective statistics */
	struct bustrace_stats	*ss ;		/* individual statistics */
	struct bustrace_state	c, n ;		/* state */
	bfile			tfile ;		/* trace file */
	BUS			*buses ;	/* buses to minitor */
	int			nbuses ;	/* number of buses */
	int			level ;		/* trace detail level */
} ;




#if	(! defined(BUSTRACE_MASTER)) || (BUSTRACE_MASTER == 0)

extern int	bustrace_init(BUSTRACE *,struct proginfo *,LSIM *,
				char *,BUS *,int) ;
extern int	bustrace_comb(BUSTRACE *,int) ;
extern int	bustrace_clock(BUSTRACE *) ;
extern int	bustrace_free(BUSTRACE *) ;
extern int	bustrace_control(BUSTRACE *,int) ;

#endif /* BUSTRACE_MASTER */


#endif /* BUSTRACE_INCLUDE */



