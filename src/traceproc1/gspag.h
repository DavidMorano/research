/* gspag */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	GSPAG_INCLUDE
#define	GSPAG_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>

#include	<localmisc.h>


/* object defines */

#define	GSPAG			struct gspag_head
#define	GSPAG_STATS		struct gspag_stats

/* more important defines */

#define	GSPAG_COUNTBITS	2	/* counter bits */


/* statistics */
struct gspag_stats {
	uint			gpht ;		/* global length */
	uint			lbht ;		/* local history length */
	uint			lpht ;		/* local pattern length */
	uint			bits ;
} ;

struct gspag_head {
	unsigned long		magic ;
	struct gspag_stats	s ;
	uint			*lbht ;		/* local BHT */
	uchar			*gpht ;		/* global PHT */
	uint			bhlen ;		/* BHT length */
	uint			phlen ;		/* GPHT length */
} ;


#if	(! defined(GSPAG_MASTER)) || (GSPAG_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	gspag_init(GSPAG *,int,int) ;
extern int	gspag_lookup(GSPAG *,ULONG) ;
extern int	gspag_confidence(GSPAG *,ULONG) ;
extern int	gspag_update(GSPAG *,ULONG,int) ;
extern int	gspag_zerostats(GSPAG *) ;
extern int	gspag_stats(GSPAG *,GSPAG_STATS *) ;
extern int	gspag_free(GSPAG *) ;

#ifdef	__cplusplus
}
#endif

#endif /* GSPAG_MASTER */

#endif /* GSPAG_INCLUDE */


