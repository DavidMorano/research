/* bpeval */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	BPEVAL_INCLUDE
#define	BPEVAL_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>

#include	<vecitem.h>
#include	<localmisc.h>

#include	"bpfifo.h"


/* object define */

#define	BPEVAL			struct bpeval_head
#define	BPEVAL_STATS		struct bpeval_stats


/* the rest of these are not in the loadable modules */

struct bpeval_calls {
	int	(*init)() ;
	int	(*lookup)() ;
	int	(*confidence)() ;
	int	(*update)() ;
	int	(*stats)() ;
	int	(*free)() ;
} ;

struct bpeval_params {
	int	p1, p2, p3, p4 ;
} ;

struct bpeval_stats {
	struct bpeval_params	p ;
	uint	lookups ;
	uint	corrects ;
	uint	confidence[8] ;
	uint	cc[8] ;
	uint	bits ;
	char	name[MAXNAMELEN + 1] ;
} ;

struct bpeval_ii {
	uint	f_pred : 1 ;
	uint	confidence : 4 ;
} ;

struct bpeval_entry {
	struct bpeval_calls	call ;
	struct bpeval_stats	s ;
	struct bpeval_ii	cii, pii, bii ;
	int			size ;
	void			*dlp ;	/* load handle */
	void			*op ;	/* object pointers */
} ;

struct bpeval_head {
	unsigned long		magic ;
	BPFIFO			fifo ;
	vecitem			bps ;
	char			*dir ;
	int	rows, delay ;
	int	bpsel ;			/* "selected" BP (?) */
} ;


#if	(! defined(BPEVAL_MASTER)) || (BPEVAL_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	bpeval_init(BPEVAL *,char *,int,int) ;
extern int	bpeval_add(BPEVAL *,char *,char *,int,int,int,int) ;
extern int	bpeval_bpsel(BPEVAL *,char *) ;
extern int	bpeval_lookup(BPEVAL *,ULONG,ULONG,int) ;
extern int	bpeval_confidence(BPEVAL *,ULONG,ULONG,int) ;
extern int	bpeval_outcome(BPEVAL *,ULONG,ULONG,int,int) ;
extern int	bpeval_update(BPEVAL *,ULONG,ULONG,int) ;
extern int	bpeval_checkmid(BPEVAL *,ULONG) ;
extern int	bpeval_checkend(BPEVAL *,ULONG) ;
extern int	bpeval_zerostats(BPEVAL *) ;
extern int	bpeval_stats(BPEVAL *,int,BPEVAL_STATS *) ;
extern int	bpeval_free(BPEVAL *) ;

#ifdef	__cplusplus
}
#endif

#endif /* BPEVAL_MASTER */

#endif /* BPEVAL_INCLUDE */


