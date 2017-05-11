/* levo */

/* top Levo machine object */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


	= 01/08/06, Dave Morano

	Added parameter to 'levo_init()' for skipping instructions.



*/




#ifndef	LEVO_INCLUDE
#define	LEVO_INCLUDE	1



/* object defines */

#define	LEVO	struct levo_head

#ifndef	UINT
#define	UINT	unsigned int
#endif




#include	<bio.h>

#include	"lsim.h"
#include	"statemips.h"

#include	"levoinfo.h"
#include	"lmem.h"		/* memory subsytem */
#include	"iw.h"			/* instruction window */
#include	"bus.h"			/* buses */
#include	"rfbus.h"		/* buses */
#include	"libus.h"		/* special cheap wide i-fetch bus */
#include	"busmon.h"		/* testing */
#include	"bustest.h"		/* testing */




struct levo_state {
	int	checksum ;
	int	verifier ;
} ;

struct levo_flags {
	UINT	exit : 1 ;		/* exit indication */
} ;

struct levo_head {
	unsigned long		magic ;

	struct proginfo		*pip ;		/* program information */
	LSIM			*mip ;		/* MINT information */
	struct levoinfo		info ;		/* Levo information */

	struct levo_state	c, n ;
	struct levo_flags	f ;

	IW			win ;
	LMEM			memsys ;
	RFBUS			ifb ;		/* i-fetch requests */
	LIBUS			irb ;		/* i-fetch responses */

	BUS			mybus ;		/* testing */
	BUSMON			testmon ;	/* testing */
	BUSTEST			testbus ;	/* testing */
	bfile			btfile ;	/* testing (bus trace) */
	bfile			mtfile ;	/* testing (master trace) */
} ;




#if	(! defined(LEVO_MASTER)) || (LEVO_MASTER == 0)

extern int	levo_init(LEVO *,struct proginfo *,
			PARAMFILE *, LSIM *, struct statemips *) ;
extern int	levo_free(LEVO *) ;
extern int	levo_comb(LEVO *,int) ;
extern int	levo_clock(LEVO *) ;
extern int	levo_statfile(LEVO *,bfile *) ;

#endif /* (! defined(LEVO_MASTER)) || (LEVO_MASTER == 0) */


#endif /* LEVO_INCLUDE */



