/* lbusint */

/* Levo Bus Interface component */



/* revision history :

	= 00/06/23, Dave Morano

	This code was started.


*/



#ifndef	LBUSINT_INCLUDE
#define	LBUSINT_INCLUDE		1

#ifndef	UINT
#define	UINT	unsigned int
#endif




#define	LBUSINT		struct lbusint_head



#include	<vsystem.h>
#include	<bio.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"bus.h"
#include	"lflowgroup.h"





struct lbusint_sflags {
	uint	request : 1 ;		/* should request it in this cycle */
	uint	requested : 1 ;		/* bus has been requested */
	uint	owned : 1 ;		/* bus is owned in the current clock */
	uint	dp_read : 1 ;
} ;

struct lbusint_state {
	struct lbusint_sflags	f ;
	LFLOWGROUP		wreg ;	/* write storage register */
	LFLOWGROUP		rreg ;	/* read storage register */
	int			pri ;	/* write priority */
	int			checksum ;
	int			count ;	/* count for 2 clocks */
} ;

struct lbusint_flags {
	uint	shift : 1 ;		/* do we have a machine shift */
	uint	active : 1 ;		/* are we even active ? */
	uint	request : 1 ;		/* asynchronus request */
} ;

struct lbusint_head {
	unsigned long		magic ;
	struct lbusint_flags	f ;
	struct lbusint_state	c, n ;		/* machine state */
	struct proginfo		*pip ;		/* program information */
	LSIM			*mip ;		/* simulator information */
	struct levoinfo		*lip ;		/* Levo information */
	BUS			*busp ;		/* our write bus */
	int			id_type ;	/* which type of bus */
	int			id_which ;	/* which one */
	int			mid ;		/* bus master ID */
} ;



#ifdef	COMMENT_SYNOPSIS

	int lbusint_init(lbip,pip,lip,busp,id)
	LBUSINT	*lbip ;			/* pointer to us */
	struct proginfo		*pip ;	/* program information */
	struct levoinfo		*lip ;	/* Levo information */
	BUS	*busp ;			/* pointer to bus you want */
	int	id ;			/* ID to use on underlying bus */

	int lbusint_write(lbip,pri,lfgp)
	LBUSINT	*lbip ;			/* pointer to us */
	int	id ;			/* priority to use on underlying bus */
	LFLOWGROUP	*lfgp ;	/* pointer to data for bus */

	int lbusint_read(lbip,lfgp)
	LBUSINT	*lbip ;			/* pointer to us */
	LFLOWGROUP	*lfgp ;	/* pointer to data for bus */

	int lbusint_ready(lbip)
	LBUSINT	*lbip ;			/* pointer to us */

	int lbusint_free(lbip)
	LBUSINT	*lbip ;			/* pointer to us */

#endif /* COMMENT_SYNOPSIS */


#if	(! defined(LBUSINT_MASTER)) || (LBUSINT_MASTER == 0)

extern int	lbusint_init(LBUSINT *,struct proginfo *,
			struct levoinfo *, BUS *,int,int,int) ;
extern int	lbusint_comb(LBUSINT *,int) ;
extern int	lbusint_clock(LBUSINT *) ;
extern int	lbusint_ready(LBUSINT *) ;
extern int	lbusint_write(LBUSINT *,int,LFLOWGROUP *) ;
extern int	lbusint_read(LBUSINT *,LFLOWGROUP *) ;
extern int	lbusint_shift(LBUSINT *) ;
extern int	lbusint_free(LBUSINT *) ;
extern int	lbusint_done(LBUSINT *) ;
extern int	lbusint_check(LBUSINT *) ;

#endif /* LBUSINT_MASTER */


#endif /* LBUSINT_INCLUDE */



