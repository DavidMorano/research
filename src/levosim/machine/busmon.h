/* busmon */

/* Levo Bus Monitor component */



/* revision history :

	= 00/07/27, Dave Morano
	This code was started.


*/



#ifndef	BUSMON_INCLUDE
#define	BUSMON_INCLUDE		1

#ifndef	UINT
#define	UINT	unsigned int
#endif



#define	BUSMON		struct busmon_head



#include	<usystem.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"bus.h"
#include	"lflowgroup.h"




struct busmon_sflags {
	uint	request : 1 ;		/* should request it in this cycle */
	uint	requested : 1 ;		/* bus has been requested */
	uint	owned : 1 ;		/* bus is owned in the current clock */
} ;

struct busmon_state {
	struct busmon_sflags	f ;
	int			pri ;	/* write priority */
	LFLOWGROUP		wreg ;	/* write storage register */
	LFLOWGROUP		rreg ;	/* read storage register */
} ;

struct busmon_flags {
	uint	shift : 1 ;		/* do we have a machine shift */
	uint	active : 1 ;		/* are we even active ? */
	uint	request : 1 ;		/* asynchronus request */
} ;

struct busmon_head {
	unsigned long		magic ;
	struct busmon_flags	f ;
	struct proginfo		*pip ;		/* program information */
	LSIM			*mip ;		/* MINT information */
	struct levoinfo		*lip ;		/* Levo information */
	struct busmon_state	c, n ;		/* machine state */
	BUS			*busp ;		/* our write bus */
	int			errors ;	/* programming errors */
} ;




#if	(! defined(BUSMON_MASTER)) || (BUSMON_MASTER == 0)

extern int	busmon_init(BUSMON *,struct proginfo *,LSIM *,
				struct levoinfo *, BUS *) ;
extern int	busmon_comb(BUSMON *,int) ;
extern int	busmon_clock(BUSMON *) ;
extern int	busmon_ready(BUSMON *) ;
extern int	busmon_read(BUSMON *,LFLOWGROUP *) ;
extern int	busmon_free(BUSMON *) ;

#endif /* BUSMON_MASTER */


#endif /* BUSMON_INCLUDE */



