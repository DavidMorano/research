/* libus */

/* the bus for receiving fetched instruction words */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/



#ifndef	LIBUS_INCLUDE
#define	LIBUS_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif




#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lflowgroup.h"



/* object defines */

#define	LIBUS		struct libus_head




/* synchronous state flags */

struct libus_sflags {
	uint	busy : 1 ;	/* bus busy */
	uint	hold : 1 ;	/* bus hold (hold data for another clock) */
	uint	dp : 1 ;	/* data present */
} ;

struct libus_state {
	struct libus_sflags	f ;
	LFLOWGROUP		*fgs ;
	uint			nfgs ;	/* number of LFGs written */
} ;

struct libus_xml {
	struct libus_state	s ;
} ;

struct libus_flags {
	uint	shift : 1 ;
} ;

struct libus_alloc {
	LFLOWGROUP		*fgs ;
} ;

struct libus_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	LSIM			*mip ;
	struct levoinfo		*lip ;

	struct libus_flags	f ;
	struct libus_state	n, c ;
	struct libus_alloc	a ;
	struct libus_xml	*xsp ;
	int			width ;
} ;



#if	(! defined(LIBUS_MASTER)) || (LIBUS_MASTER == 0)

extern int libus_init(LIBUS *,struct proginfo *,LSIM *,
				struct levoinfo *,int) ;
extern int libus_free(LIBUS *) ;
extern int libus_clock(LIBUS *) ;
extern int libus_comb(LIBUS *,int) ;
extern int libus_write(LIBUS *,LFLOWGROUP *,int) ;
extern int libus_read(LIBUS *,LFLOWGROUP *,int) ;
extern int libus_xmlinit(LIBUS *,XMLINFO *) ;
extern int libus_xmlout(LIBUS *,XMLINFO *) ;
extern int libus_xmlfree(LIBUS *,XMLINFO *) ;

#endif /* LIBUS_MASTER */


#endif /* LIBUS_INCLUDE */



