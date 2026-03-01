/* bpalpha HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	BPALPHA_INCLUDE
#define	BPALPHA_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	BPALPHA			struct bpalpha_head
#define	BPALPHA_ST		struct bpalpha_stats
#define	BPALPHA_MAGIC		0x29456781


struct bpalpha_stats {
	uint		gpht ;		/* global length */
	uint		lbht ;		/* local history length */
	uint		lpht ;		/* local pattern length */
	uint		bits ;		/* total-bits */
} ; /* end struct */

struct bpalpha_head {
	uchar		*cpht ;		/* choice PHT */
	uchar		*gpht ;		/* global PHT */
	uint		*lbht ;		/* local BHT */
	uchar		*lpht ;		/* local PHT */
	BPALPHA_ST	s ;
	uint		magval ;
	uint		bhistory ;	/* global branch history */
	uint		lhlen ;		/* local history length */
	uint		lplen ;		/* local pattern length */
	uint		glen ;		/* global length */
	uint		historymask ;
} ; /* end struct */

typedef	BPALPHA		bpalpha ;
typedef	BPALPHA_ST	bpalpha_st ;

EXTERNC_begin

extern int	bpalpha_start		(bpalpha *,int,int,int) noex ;
extern int	bpalpha_lookup		(bpalpha *,uint) noex ;
extern int	bpalpha_confidence	(bpalpha *,uint) noex ;
extern int	bpalpha_update		(bpalpha *,uint,int) noex ;
extern int	bpalpha_zerostats	(bpalpha *) noex ;
extern int	bpalpha_getstats	(bpalpha *,bpalpha_st *) noex ;
extern int	bpalpha_finish		(bpalpha *) noex ;

EXTERNC_end


#endif /* BPALPHA_INCLUDE */


