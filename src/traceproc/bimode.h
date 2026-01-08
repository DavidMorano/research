/* bimode */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	BIMODE_INCLUDE
#define	BIMODE_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>

#include	<localmisc.h>


/* object defines */

#define	BIMODE			struct bimode_head
#define	BIMODE_STATS		struct bimode_stats

/* more important defines */

#define	BIMODE_COUNTBITS	2	/* counter bits */


/* statistics */
struct bimode_stats {
	uint			clen ;		/* choice length */
	uint			dlen ;		/* direction length */
	uint			hlen ;		/* history length */
	uint			bits ;		/* total bits */
} ;

struct bimode_dht {
	uint	dir0 : (BIMODE_COUNTBITS + 1) ;
	uint	dir1 : (BIMODE_COUNTBITS + 1) ;
} ;

struct bimode_head {
	unsigned long		magic ;
	struct bimode_stats	s ;
	struct bimode_dht	*dir ;
	uchar			*choice ;
	uint			bhistory ;	/* global branch history */
	uint			clen ;		/* choice length */
	uint			dlen ;		/* direction length */
	uint			hlen ;		/* history length */
} ;


#if	(! defined(BIMODE_MASTER)) || (BIMODE_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	bimode_init(BIMODE *,int,int,int) ;
extern int	bimode_lookup(BIMODE *,uint) ;
extern int	bimode_confidence(BIMODE *,uint) ;
extern int	bimode_update(BIMODE *,uint,int) ;
extern int	bimode_zerostats(BIMODE *) ;
extern int	bimode_stats(BIMODE *,BIMODE_STATS *) ;
extern int	bimode_free(BIMODE *) ;

#ifdef	__cplusplus
}
#endif

#endif /* BIMODE_MASTER */

#endif /* BIMODE_INCLUDE */


