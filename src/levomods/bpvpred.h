/* bpvpred HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

#ifndef	BPVPRED_INCLUDE
#define	BPVPRED_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	BPVPRED			struct bpvpred_head
#define	BPVPRED_ENT		struct bpvpred_entry
#define	BPVPRED_OPER		struct bpvpred_operand
#define	BPVPRED_ST		struct bpvpred_stats
#define	BPVPRED_MAGIC		0x23456787
/* more important defines */
#define	BPVPRED_NOPS		5	/* number of operands */
#define	BPVPRED_COUNTBITS	2	/* counter bits */


struct bpvpred_stats {
	ulong		in_lu, in_hit ;
	ulong		in_up, in_update, in_replace ;
	ulong		op_lu, op_hit ;
	ulong		op_up, op_update, op_replace ;
	uint		tablen ;
} ; /* end struct */

struct bpvpred_operand {
	uint		last ;
	uint		stride ;
	uint		counter : BPVPRED_COUNTBITS ;
} ; /* end struct */

struct bpvpred_entry {
	BPVPRED_OPER	ops[BPVPRED_NOPS] ;
	uint		tag ;
	uint		hits ;
	uint		replaces ;
	uint		f_valid : 1 ;
} ; /* end struct */

struct bpvpred_head {
	BPVPRED_ENT	*table ;
	BPVPRED_ST	s ;		/* statistics */
	uint		magval ;
	uint		tablen ;
	uint		tagshift ;
	uint		ncount ;	/* number of states */
	uint		stridemask ;
	int		nops ;
} ; /* end struct */

typedef	BPVPRED		bpvpred ;
typedef	BPVPRED_ENT	bpvpred_ent ;
typedef	BPVPRED_OPER	bpvpred_oper ;
typedef	BPVPRED_ST	bpvpred_st ;

EXTERNC_begin

extern int	bpvpred_start	(bpvpred *,int,int,int) noex ;
extern int	bpvpred_lookup	(bpvpred *,uint,uint *,int) noex ;
extern int	bpvpred_update	(bpvpred *,uint,uint *,int) noex ;
extern int	bpvpred_zerostats(bpvpred *) noex ;
extern int	bpvpred_getstats(bpvpred *,bpvpred_st *) noex ;
extern int	bpvpred_finish	(bpvpred *) noex ;

EXTERNC_end


#endif /* BPVPRED_INCLUDE */


