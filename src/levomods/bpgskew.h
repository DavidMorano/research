/* bpgskew HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	BPGSKEW_INCLUDE
#define	BPGSKEW_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	BPGSKEW			struct bpgskew_head
#define	BPGSKEW_ST		struct bpgskew_stats
#define	BPGSKEW_BA		struct bpgskew_banks
#define	BPGSKEW_MAGIC		0x45678430
/* more important defines */
#define	BPGSKEW_COUNTBITS	2	/* counter bits */


struct bpgskew_stats {
	uint		tlen ;
	uint		bits ;
	uint		lu ;
	uint		use_bim ;
	uint		use_eskew ;
	uint		update_meta ;
	uint		updateup_meta ;
	uint		update_all, update_bim, update_eskew ;
} ; /* end struct */

struct bpgskew_banks {
	uint		bim : 2 ;
	uint		g0 : 2 ;
	uint		g1 : 2 ;
	uint		meta : 2 ;
} ; /* end struct */

struct bpgskew_head {
	BPGSKEW_BA	*table ;
	BPGSKEW_ST	s ;
	uint		magval ;
	uint		bhistory ;	/* global branch history */
	uint		tlen ;
	uint		nhist ;		/* history bits */
	uint		tmask ;
	uint		hmask ;
	int		n ;		/* 'n' from the papers! :-) */
} ; /* end struct */

typedef	BPGSKEW		bpgskew ;
typedef	BPGSKEW_ST	bpgskew_st ;
typedef	BPGSKEW_BA	bpgskew_ba ;

EXTERNC_begin

extern int	bpgskew_start		(bpgskew *,int,int,int,int) noex ;
extern int	bpgskew_lookup		(bpgskew *,uint) noex ;
extern int	bpgskew_confidence	(bpgskew *,uint) noex ;
extern int	bpgskew_update		(bpgskew *,uint,int) noex ;
extern int	bpgskew_zerostats	(bpgskew *) noex ;
extern int	bpgskew_getstats	(bpgskew *,bpgskew_st *) noex ;
extern int	bpgskew_finish		(bpgskew *) noex ;

EXTERNC_end


#endif /* BPGSKEW_INCLUDE */


