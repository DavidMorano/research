/* memfilter */

/* Levo Register Filter component */



/* revision history :

	= 00/02/15, Dave Morano
	This code was started.


*/




#ifndef	MEMFILTER_INCLUDE
#define	MEMFILTER_INCLUDE


#include	<sys/types.h>

#include	<vsystem.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"bus.h"



/* object defines */

#define	MEMFILTER	struct memfilter_head
#define	MEMFILTER_REG	struct memfilter_register


/* get the number of ISA registers from LEVOINFO */

#define	MEMFILTER_NREGS		LEVOINFO_NISAREGS




struct memfilter_reg {
	struct dataval	dv ;		/* register data */
	int		tt ;		/* register TT */
} ;

struct memfilter_sflags {
	uint	requested : 1 ;		/* bus has been requested */
	uint	ownbus : 1 ;		/* bus is owned in the current clock */
} ;

struct memfilter_state {
	struct memfilter_sflags	f ;
	struct memfilter_reg	*r ;	/* all registers */
	LFLOWGROUP		busdata ;
} ;

struct memfilter_flags {
	uint	shift : 1 ;
	uint	forward : 1 ;
} ;

struct memfilter_head {
	struct memfilter_flags	f ;
	struct proginfo		*gp ;
	struct mintinfo		*mip ;
	struct levoinfo		*lip ;
	BUS			*rbus ;		/* our read bus */
	BUS			*wbus ;		/* our write bus */
	struct memfilter_state	c, n ;		/* machine state */
	int			nqentries ;	/* number of FIFO entries */
	int			nregs ;		/* number of allocated regs */
	int			errors ;	/* programming errors */
} ;


#if	(! defined(MEMFILTER_MASTER)) || (MEMFILTER_MASTER == 0)


#endif /* MEMFILTER_MASTER */


#endif /* MEMFILTER_INCLUDE */



