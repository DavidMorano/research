/* bpgspag HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	BPGSPAG_INCLUDE
#define	BPGSPAG_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	BPGSPAG			struct bpgspag_head
#define	BPGSPAG_ST		struct bpgspag_stats
#define	BPGSPAG_MAGIC		0x29656731
/* more important defines */
#define	BPGSPAG_COUNTBITS	2	/* counter bits */


struct bpgspag_stats {
	uint		gpht ;		/* global length */
	uint		lbht ;		/* local history length */
	uint		lpht ;		/* local pattern length */
	uint		bits ;
} ; /* end struct */

struct bpgspag_head {
	uint		*lbht ;		/* local BHT */
	uchar		*gpht ;		/* global PHT */
	BPGSPAG_ST	s ;
	uint		magval ;
	uint		bhlen ;		/* BHT length */
	uint		phlen ;		/* GPHT length */
} ; /* end struct */

typedef	BPGSPAG		bpgspag ;
typedef	BPGSPAG_ST	bpgspag_st ;

EXTERNC_begin

extern int	bpgspag_start(bpgspag *,int,int) noex ;
extern int	bpgspag_lookup(bpgspag *,uint) noex ;
extern int	bpgspag_confidence(bpgspag *,uint) noex ;
extern int	bpgspag_update(bpgspag *,uint,int) noex ;
extern int	bpgspag_zerostats(bpgspag *) noex ;
extern int	bpgspag_getstats(bpgspag *,bpgspag_st *) noex ;
extern int	bpgspag_finish(bpgspag *) noex ;

EXTERNC_end


#endif /* BPGSPAG_INCLUDE */


