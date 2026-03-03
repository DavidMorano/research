/* bpfifo HEADER */
/* charset=ISO8859-1 */
/* lang=C20 (conformance reviewed) */

/* branch prediction FIFO */
/* version %I% last-modified %G% */


/* revision history:

	= 1998-11-01, David Morano
	Originally written for Audix Database Processor (DBP) work.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	BPFIFO_INCLUDE
#define	BPFIFO_INCLUDE


#include	<envstandards.h>
#include	<clanguage.h>
#include	<usysbase.h>


#define	BPFIFO		struct bpfifo_head
#define	BPFIFO_ENT	struct bpfifo_entry
#define	BPFIFO_MAGIC	0x94732651


struct bpfifo_entry {
	ulong		in ;
	ulong		ia ;
	uint		row ;
	uint		outcome ;
} ; /* end struct */

struct bpfifo_head {
	BPFIFO_ENT	*table ;
	uint		magic ;
	int		head ;
	int		tail ;
	int		n ;
} ; /* end struct */

typedef	BPFIFO		bpfifo ;
typedef	BPFIFO_ENT	bpfifo_ent ;

EXTERNC_begin

extern int	bpfifo_start(bpfifo *,int) noex ;
extern int	bpfifo_finish(bpfifo *) noex ;
extern int	bpfifo_add(bpfifo *,ulong,ulong,uint,uint) noex ;
extern int	bpfifo_rem(bpfifo *,ulong *,ulong *,uint *,uint *) noex ;
extern int	bpfifo_read(bpfifo *,ulong *,ulong *,uint *,uint *) noex ;

EXTERNC_end


#endif /* BPFIFO_INCLUDE */


