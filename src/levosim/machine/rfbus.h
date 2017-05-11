/* rfbus */

/* bus to read/write to memory */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/



#ifndef	RFBUS_INCLUDE
#define	RFBUS_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif



#define	RFBUS		struct rfbus_head



#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lflowgroup.h"
#include	"levoinfo.h"



/* synchronous state flags */

struct rfbus_sflags {
	uint	busy : 1 ;	/* bus busy */
	uint	hold : 1 ;	/* bus hold (hold data for another clock) */
} ;

/* the general clock state for this bus */

struct rfbus_state {
	struct rfbus_sflags	f ;
	char			*dps ;		/* spread in time not space */
	LFLOWGROUP		*fgroups ;	/* spread in time not space */
	uint			checksum ;
} ;

struct rfbus_xml {
	struct rfbus_state	s ;
} ;

struct rfbus_alloc {
	char			*dps ;		/* spread in time not space */
	LFLOWGROUP		*fgroups ;	/* spread in time not space */
} ;

struct rfbus_flags {
	uint	shift : 1 ;
	uint	wrote : 1 ;
} ;

struct rfbus_head {
	unsigned long		magic ;
	struct proginfo		*pip ;
	struct levoinfo		*lip ;
	struct rfbus_flags	f ;
	struct rfbus_state	n, c ;
	struct rfbus_alloc	a ;
	struct rfbus_xml	*xsp ;
	int			delay ;
	int			nregs ;
} ;



#if	(! defined(RFBUS_MASTER)) || (RFBUS_MASTER == 0)

extern int rfbus_init(RFBUS *,struct proginfo *,struct levoinfo *,int,int) ;
extern int rfbus_free(RFBUS *) ;
extern int rfbus_clock(RFBUS *) ;
extern int rfbus_comb(RFBUS *,int) ;
extern int rfbus_shift(RFBUS *) ;
extern int rfbus_hold(RFBUS *) ;
extern int rfbus_write(RFBUS *,LFLOWGROUP *) ;
extern int rfbus_read(RFBUS *,LFLOWGROUP *) ;
extern int rfbus_xmlinit(RFBUS *,XMLINFO *) ;
extern int rfbus_xmlout(RFBUS *,XMLINFO *) ;
extern int rfbus_xmlfree(RFBUS *,XMLINFO *) ;

#endif /* RFBUS_MASTER */


#endif /* RFBUS_INCLUDE */



