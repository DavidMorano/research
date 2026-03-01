/* bpeveight HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	BPEVEIGHT_INCLUDE
#define	BPEVEIGHT_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	BPEVEIGHT		struct bpeveight_head
#define	BPEVEIGHT_ST		struct bpeveight_stats
#define	BPEVEIGHT_BA		struct bpeveight_banks
#define	BPEVEIGHT_MAGIC		0x45678429
/* more important defines */
#define	BPEVEIGHT_COUNTBITS	2	/* counter bits */


struct bpeveight_stats {
	uint		tlen ;
	uint		bits ;
} ; /* end struct */

struct bpeveight_banks {
	uint		bim : 2 ;
	uint		g0 : 2 ;
	uint		g1 : 2 ;
	uint		meta : 2 ;
} ; /* end struct */

struct bpeveight_head {
	BPEVEIGHT_BA	*table ;
	BPEVEIGHT_ST	s ;
	uint		magval ;
	uint		bhistory ;	/* global branch history */
	uint		tlen ;
	uint		tmask ;
} ; /* end struct */

typedef	BPEVEIGHT	bpeveight ;
typedef	BPEVEIGHT_ST	bpeveight_st ;
typedef	BPEVEIGHT_BA	bpeveight_ba ;

EXTERNC_begin

extern int	bpeveight_start		(bpeveight *,int,int,int,int) noex ;
extern int	bpeveight_lookup	(bpeveight *,uint) noex ;
extern int	bpeveight_confidence	(bpeveight *,uint) noex ;
extern int	bpeveight_update	(bpeveight *,uint,int) noex ;
extern int	bpeveight_zerostats	(bpeveight *) noex ;
extern int	bpeveight_getstats	(bpeveight *,bpeveight_st *) noex ;
extern int	bpeveight_finish	(bpeveight *) noex ;

EXTERNC_end


#endif /* BPEVEIGHT_INCLUDE */


