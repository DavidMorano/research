/* vpred */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	VPRED_INCLUDE
#define	VPRED_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>

#include	<localmisc.h>


/* object defines */

#define	VPRED			struct vpred_head
#define	VPRED_ENTRY		struct vpred_entry
#define	VPRED_STATS		struct vpred_stats

/* more important defines */

#define	VPRED_NOPS		5	/* number of operands */
#define	VPRED_COUNTBITS		2	/* counter bits */


/* statistics */
struct vpred_stats {
	ULONG	in_lu, in_hit ;
	ULONG	in_up, in_update, in_replace ;
	ULONG	op_lu, op_hit ;
	ULONG	op_up, op_update, op_replace ;
	uint	tablen ;
} ;

struct vpred_operand {
	int	last ;
	int	stride ;
	uint	counter : VPRED_COUNTBITS ;
} ;

struct vpred_entry {
	struct vpred_operand	ops[VPRED_NOPS] ;
	uint	tag ;
	uint	hits, replaces ;
	uint	f_valid : 1 ;
} ;

struct vpred_head {
	unsigned long		magic ;
	struct vpred_entry	*table ;
	struct vpred_stats	s ;		/* statistics */
	uint			tablen ;
	uint			tagshift ;
	uint			ncount ;	/* number of states */
	uint			stridemask ;
	int			nops ;
} ;


#if	(! defined(VPRED_MASTER)) || (VPRED_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	vpred_init(VPRED *,int,int,int) ;
extern int	vpred_lookup(VPRED *,uint,uint *,int) ;
extern int	vpred_update(VPRED *,uint,uint *,int) ;
extern int	vpred_zerostats(VPRED *) ;
extern int	vpred_stats(VPRED *,VPRED_STATS *) ;
extern int	vpred_free(VPRED *) ;

#ifdef	__cplusplus
}
#endif

#endif /* VPRED_MASTER */

#endif /* VPRED_INCLUDE */


