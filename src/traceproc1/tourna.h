/* tourna */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	TOURNA_INCLUDE
#define	TOURNA_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>

#include	<localmisc.h>


/* object defines */

#define	TOURNA			struct tourna_head
#define	TOURNA_STATS		struct tourna_stats


/* statistics */
struct tourna_stats {
	uint			gpht ;		/* global length */
	uint			lbht ;		/* local history length */
	uint			lpht ;		/* local pattern length */
	uint			bits ;
} ;

struct tourna_head {
	unsigned long		magic ;
	struct tourna_stats	s ;
	uint			*lbht ;		/* local BHT */
	uchar			*lpht ;		/* local PHT */
	uchar			*cpht ;		/* choice PHT */
	uchar			*gpht ;		/* global PHT */
	uint			bhistory ;	/* global branch history */
	uint			lhlen ;		/* local history length */
	uint			lplen ;		/* local pattern length */
	uint			glen ;		/* global length */
	uint			historymask ;
} ;


#if	(! defined(TOURNA_MASTER)) || (TOURNA_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	tourna_init(TOURNA *,int,int,int) ;
extern int	tourna_lookup(TOURNA *,ULONG) ;
extern int	tourna_confidence(TOURNA *,ULONG) ;
extern int	tourna_update(TOURNA *,ULONG,int) ;
extern int	tourna_zerostats(TOURNA *) ;
extern int	tourna_stats(TOURNA *,TOURNA_STATS *) ;
extern int	tourna_free(TOURNA *) ;

#ifdef	__cplusplus
}
#endif

#endif /* TOURNA_MASTER */

#endif /* TOURNA_INCLUDE */


