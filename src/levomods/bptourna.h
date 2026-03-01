/* bptourna HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	BPTOURNA_INCLUDE
#define	BPTOURNA_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	BPTOURNA		struct bptourna_head
#define	BPTOURNA_ST		struct bptourna_stats
#define	BPTOURNA_MAGIC		0x29456782


struct bptourna_stats {
	uint		gpht ;		/* global length */
	uint		lbht ;		/* local history length */
	uint		lpht ;		/* local pattern length */
	uint		bits ;
} ; /* end struct */

struct bptourna_head {
	uchar		*cpht ;		/* choice PHT */
	uchar		*gpht ;		/* global PHT */
	uint		*lbht ;		/* local BHT */
	uchar		*lpht ;		/* local PHT */
	BPTOURNA_ST	s ;
	uint		magval ;
	uint		bhistory ;	/* global branch history */
	uint		lhlen ;		/* local history length */
	uint		lplen ;		/* local pattern length */
	uint		glen ;		/* global length */
	uint		historymask ;
} ; /* end struct */

typedef	BPTOURNA	bptourna ;
typedef	BPTOURNA_ST	bptourna_st ;

EXTERNC_begin

extern int	bptourna_start		(bptourna *,int,int,int) noex ;
extern int	bptourna_lookup		(bptourna *,uint) noex ;
extern int	bptourna_confidence	(bptourna *,uint) noex ;
extern int	bptourna_update		(bptourna *,uint,int) noex ;
extern int	bptourna_zerostats	(bptourna *) noex ;
extern int	bptourna_getstats	(bptourna *,bptourna_st *) noex ;
extern int	bptourna_finish		(bptourna *) noex ;

EXTERNC_end


#endif /* BPTOURNA_INCLUDE */


