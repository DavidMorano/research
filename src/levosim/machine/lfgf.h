/* lfgf */

/* Levo FlowGroup FIFO component */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


	= 00/11/22, Dave Morano

	Just a note to say that this code started life as the Register
	Forwarding Unit but became the Register Filter Unit after I
	realized that the masking type operations for register snooping
	was more complicated than need be for our purposes.  When
	actual time-tag values are passed, then it made the situation
	easy to provide more general caching of register values.


*/




#ifndef	LFGF_INCLUDE
#define	LFGF_INCLUDE

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines */

#define	LFGF		struct lfgf_head
#define	LFGF_ARGS	struct lfgf_a



#include	<usystem.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"

#include	"levoinfo.h"
#include	"lflowgroup.h"





struct lfgf_sflags {
	uint	notempty : 1 ;		/* FIFO is not empty */
} ;

struct lfgf_state {
	struct lfgf_sflags	f ;
	LFLOWGROUP		*regs ;		/* all registers */
	int			ri ;		/* read index */
	int			wi ;		/* write index */
	int			count ;		/* count of items */
	uint			checksum ;
} ;

struct lfgf_flags {
	uint	shift : 1 ;
} ;

struct lfgf_allocs {
	LFLOWGROUP		*regs ;		/* allocated registers */
} ;

struct lfgf_head {
	unsigned long		magic ;
	struct lfgf_flags	f ;
	struct proginfo		*pip ;
	LSIM			*mip ;
	struct levoinfo		*lip ;
	struct lfgf_allocs	a ;		/* allocations */
	struct lfgf_state	c, n ;		/* machine state */
	int			nregs ;		/* size of FIFO */
} ;


#if	(! defined(LFGF_MASTER)) || (LFGF_MASTER == 0)

extern int lfgf_init(LFGF *,struct proginfo *,LSIM *,
	struct levoinfo *,int) ;
extern int lfgf_free(LFGF *) ;
extern int lfgf_comb(LFGF *,int) ;
extern int lfgf_clock(LFGF *) ;
extern int lfgf_shift(LFGF *) ;
extern int lfgf_write(LFGF *,LFLOWGROUP *) ;
extern int lfgf_read(LFGF *,LFLOWGROUP *) ;
extern int lfgf_readout(LFGF *,LFLOWGROUP *) ;
extern int lfgf_count(LFGF *) ;
extern int lfgf_enum(LFGF *,int,LFLOWGROUP *) ;

#endif /* LFGF_MASTER */


#endif /* LFGF_INCLUDE */



