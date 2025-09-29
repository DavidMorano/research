/* bustest */

/* Levo Bus Monitor component */



/* revision history :

	= 00/07/27, Dave Morano
	This code was started.


*/




#ifndef	BUSTEST_INCLUDE
#define	BUSTEST_INCLUDE		1

#ifndef	UINT
#define	UINT	unsigned int
#endif



#define	BUSTEST		struct bustest_head



#include	<usystem.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"

#include	"levoinfo.h"
#include	"bus.h"
#include	"lbusint.h"
#include	"lflowgroup.h"





struct bustest_sflags {
	uint	request : 1 ;		/* should request it in this cycle */
	uint	requested : 1 ;		/* bus has been requested */
	uint	owned : 1 ;		/* bus is owned in the current clock */
} ;

struct bustest_state {
	struct bustest_sflags	f ;
	LFLOWGROUP		wreg ;	/* write storage register */
	LFLOWGROUP		rreg ;	/* read storage register */
	int			pri ;	/* write priority */
} ;

struct bustest_flags {
	uint	shift : 1 ;		/* do we have a machine shift */
	uint	active : 1 ;		/* are we even active ? */
	uint	request : 1 ;		/* asynchronus request */
} ;

struct bustest_head {
	unsigned long		magic ;
	struct bustest_flags	f ;
	struct proginfo		*pip ;		/* program information */
	LSIM			*mip ;		/* MINT information */
	struct levoinfo		*lip ;		/* Levo information */
	struct bustest_state	c, n ;		/* machine state */
	BUS			*busp ;		/* bus under test */
	LBUSINT			*mybuses ;
	int			nbuses ;
	int			value ;
	int			pathid ;	/* path ID to use */
} ;




#if	(! defined(BUSTEST_MASTER)) || (BUSTEST_MASTER == 0)

extern int	bustest_init(BUSTEST *,struct proginfo *,struct mintinfo *,
				struct levoinfo *,
				BUS *,int,int,int) ;
extern int	bustest_comb(BUSTEST *,int) ;
extern int	bustest_clock(BUSTEST *) ;
extern int	bustest_ready(BUSTEST *) ;
extern int	bustest_read(BUSTEST *,LFLOWGROUP *) ;
extern int	bustest_free(BUSTEST *) ;

#endif /* BUSTEST_MASTER */


#endif /* BUSTEST_INCLUDE */



