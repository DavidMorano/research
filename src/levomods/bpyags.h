/* bpyags HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	BPYAGS_INCLUDE
#define	BPYAGS_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	BPYAGS			struct bpyags_head
#define	BPYAGS_PHT		struct bpyags_phter
#define	BPYAGS_CA		struct bpyags_cache
#define	BPYAGS_ST		struct bpyags_stats
#define	BPYAGS_MAGIC		0x29456783
/* more important defines */
#define	BPYAGS_TAGBITS		8	/* number of tag bits */
#define	BPYAGS_COUNTBITS	2	/* counter bits */
#define	BPYAGS_TAGMASK		((1 << BPYAGS_TAGBITS) - 1)


struct bpyags_stats {
	uint		cpht ;		/* choice length */
	uint		dpht ;		/* cache length */
	uint		bits ;		/* total bits */
} ; /* end struct */

struct bpyags_phter {
	uint		counter : 2 ;
} ; /* end struct */

struct bpyags_cache {
	uint		tag0 : (BPYAGS_TAGBITS + 1) ;	/* IAoff + 1 history */
	uint		tag1 : (BPYAGS_TAGBITS + 1) ;	/* IAoff + 1 history */
	uint		counter0 : 2 ;
	uint		counter1 : 2 ;
	uint		lru : 1 ;	/* least recently used */
} ; /* end struct */

struct bpyags_head {
	BPYAGS_PHT	*choice ;
	BPYAGS_CA	*taken ;
	BPYAGS_CA	*nottaken ;
	BPYAGS_ST	s ;
	uint		magval ;
	uint		bhistory ;	/* global branch history */
	uint		chlen ;		/* choice length */
	uint		calen ;		/* cache length */
} ; /* end struct */

typedef	BPYAGS		bpyags ;
typedef	BPYAGS_PHT	bpyags_pht ;
typedef	BPYAGS_CA	bpyags_ca ;
typedef	BPYAGS_ST	bpyags_st ;

EXTERNC_begin

extern int	bpyags_start(bpyags *,int,int) noex ;
extern int	bpyags_lookup(bpyags *,uint) noex ;
extern int	bpyags_update(bpyags *,uint,int) noex ;
extern int	bpyags_zerostats(bpyags *) noex ;
extern int	bpyags_getstats(bpyags *,bpyags_st *) noex ;
extern int	bpyags_finish(bpyags *) noex ;

EXTERNC_end


#endif /* BPYAGS_INCLUDE */


