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
#include	<utypedefs.h>
#include	<utypealiases.h>
#include	<usysdefs.h>
#include	<usysrets.h>
#include	<usyscalls.h>


#define	BPFIFO		struct bpfifo_head
#define	BPFIFO_ENT	struct bpfifo_entry
#define	BPFIFO_MAGIC	0x94732651


struct bpfifo_entry {
	ulong		in ;
	ulong		ia ;
	uint		row ;
	uint		outcome ;
} ;

struct bpfifo_head {
	BPFIFO_ENT	*table ;
	uint		magic ;
	int		head ;
	int		tail ;
	int		n ;
} ;

typedef	BPFIFO		bpfifo ;
typedef	BPFIFO_ENT	bpfifo_ent ;

EXTERNC_begin

extern int	bpfifo_init(BPFIFO *,int) noex ;
extern int	bpfifo_free(BPFIFO *) noex ;
extern int	bpfifo_add(BPFIFO *,ulong,ulong,uint,uint) noex ;
extern int	bpfifo_rem(BPFIFO *,ulong *,ulong *,uint *,uint *) noex ;
extern int	bpfifo_read(BPFIFO *,ulong *,ulong *,uint *,uint *) noex ;

EXTERNC_end


#endif /* BPFIFO */


