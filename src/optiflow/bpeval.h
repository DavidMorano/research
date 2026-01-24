/* bpeval HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */


/* revision history:

	= 1998-11-01, David Morano
	Originally written for Audix Database Processor (DBP) work.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	BPEVAL_INCLUDE
#define	BPEVAL_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<vecitem.h>
#include	<localmisc.h>

#include	<bpfifo.h>


#define	BPEVAL			struct bpeval_head
#define	BPEVAL_STATS		struct bpeval_stats


struct bpeval_calls {
	int	(*init)() ;
	int	(*lookup)() ;
	int	(*confidence)() ;
	int	(*update)() ;
	int	(*stats)() ;
	int	(*free)() ;
} ; /* end struct */

struct bpeval_params {
	int	p1, p2, p3, p4 ;
} ; /* end struct */

struct bpeval_stats {
 	struct bpeval_params	p ;	/* predictor parameters */
 	ulong	lookups ;		/* lookups (in the selection) */
 	ulong	corrects ;		/* number correct (in the selection) */
 	ulong	confidence[8] ;		/* density of confidences */
 	ulong	cc[8] ;			/* density of correct confidences */
 	uint	bits ;			/* memory bits the predictor uses */
	char	name[MAXNAMELEN + 1] ;
} ; /* end struct */

struct bpeval_ii {
	uint	f_pred : 1 ;
	uint	confidence : 4 ;
} ; /* end struct */

struct bpeval_entry {
	struct bpeval_calls	call ;
	struct bpeval_stats	s ;
	struct bpeval_ii	cii, pii, bii ;
	int			size ;
	void			*dlp ;	/* load handle */
	void			*op ;	/* object pointers */
} ; /* end struct */

struct bpeval_head {
	BPFIFO		fifo ;
	vecitem		bps ;
	char		*dir ;
	uint		magic ;
	int		rows, delay ;
	int		bpsel ;		/* "selected" BP (?) */
} ; /* end struct */

EXTERNC_begin

extern int	bpeval_init(BPEVAL *,char *,int,int) noex ;
extern int	bpeval_add(BPEVAL *,char *,char *,int,int,int,int) noex ;
extern int	bpeval_bpsel(BPEVAL *,char *) noex ;
extern int	bpeval_lookup(BPEVAL *,ulong,ulong,int) noex ;
extern int	bpeval_confidence(BPEVAL *,ulong,ulong,int) noex ;
extern int	bpeval_outcome(BPEVAL *,ulong,ulong,int,int) noex ;
extern int	bpeval_update(BPEVAL *,ulong,ulong,int) noex ;
extern int	bpeval_checkmid(BPEVAL *,ulong) noex ;
extern int	bpeval_checkend(BPEVAL *,ulong) noex ;
extern int	bpeval_zerostats(BPEVAL *) noex ;
extern int	bpeval_stats(BPEVAL *,int,BPEVAL_STATS *) noex ;
extern int	bpeval_free(BPEVAL *) ;

EXTERNC_end


#endif /* BPEVAL_INCLUDE */


