/* predforward */

/* Levo Register Filter component */



/* revision history :

	= 00/02/15, Dave Morano
	This code was started.


*/




#ifndef	PREDFORWARD_INCLUDE
#define	PREDFORWARD_INCLUDE



/* object defines */

#define	PREDFORWARD	struct predforward_head
#define	PREDFORWARD_REG	struct predforward_reg



#include	<sys/types.h>

#include	<usystem.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"bus.h"




/* get the number of ISA registers from LEVOINFO */

#define	PREDFORWARD_NREGS		LEVOINFO_NISAREGS




struct predforward_reg {
	DATAVAL		dv ;		/* register data */
	int		tt ;		/* register TT */
} ;

struct predforward_sflags {
	uint	requested : 1 ;		/* bus has been requested */
	uint	ownbus : 1 ;		/* bus is owned in the current clock */
} ;

struct predforward_state {
	struct predforward_sflags	f ;
	struct predforward_reg	*regs ;		/* all registers */
	LFLOWGROUP		busdata ;
} ;

struct predforward_flags {
	uint	shift : 1 ;
	uint	forward : 1 ;
} ;

struct predforward_head {
	struct predforward_flags	f ;
	struct proginfo		*gp ;
	struct mintinfo		*mip ;
	struct levoinfo		*lip ;
	BUS			*rbus ;		/* our read bus */
	BUS			*wbus ;		/* our write bus */
	struct predforward_state	c, n ;		/* machine state */
	int			magic ;
	int			nqentries ;	/* number of FIFO entries */
	int			nregs ;		/* number of allocated regs */
	int			errors ;	/* programming errors */
} ;



#if	(! defined(PREDFORWARD_MASTER)) || (PREDFORWARD_MASTER == 0)

extern int predforward_init(PREDFORWARD *,struct proginfo *,struct mintinfo *,
			struct levoinfo *, BUS *, BUS *,int) ;
extern int predforward_free(PREDFORWARD *) ;

#endif /* PREDFORWARD_MASTER */


#endif /* PREDFORWARD_INCLUDE */



