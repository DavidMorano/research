/* bpfifo */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	BPFIFO_INCLUDE
#define	BPFIFO_INCLUDE		1


#include	<envstandards.h>

#include	<sys/types.h>

#include	<localmisc.h>		/* for { uint, ULONG } */


#define	BPFIFO		struct bpfifo_head
#define	BPFIFO_ENTRY	struct bpfifo_entry


struct bpfifo_entry {
	ULONG		in ;
	ULONG		ia ;
	uint		row ;
	uint		outcome ;
} ;

struct bpfifo_head {
	unsigned long	magic ;
	BPFIFO_ENTRY	*table ;
	int		head, tail ;
	int		n ;
} ;


#if	(! defined(BPFIFO_MASTER)) || (BPFIFO_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	bpfifo_init(BPFIFO *,int) ;
extern int	bpfifo_free(BPFIFO *) ;
extern int	bpfifo_add(BPFIFO *,ULONG,ULONG,uint,uint) ;
extern int	bpfifo_remove(BPFIFO *,ULONG *,ULONG *,uint *,uint *) ;
extern int	bpfifo_read(BPFIFO *,ULONG *,ULONG *,uint *,uint *) ;

#ifdef	__cplusplus
}
#endif

#endif /* BPFIFO */

#endif /* BPFIFO */


