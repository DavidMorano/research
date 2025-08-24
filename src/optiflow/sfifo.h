/* sfifo HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* synchronous FIFO */
/* version %I% last-modified %G% */


/* Copyright © 2002 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

  	Object:
	sfifo

	Description:
	This object implements a synchronous clocked FIFO.

******************************************************************************/

#ifndef	SFIFO_INCLUDE
#define	SFIFO_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<utypedefs.h>
#include	<utypealiases.h>
#include	<usysdefs.h>
#include	<usysrets.h>
#include	<usyscalls.h>


#define	SFIFO_MAGIC	0x94732451
#define	SFIFO		struct sfifo_head
#define	SFIFO_ST	struct sfifo_state
#define	SFIFO_FL	struct sfifo_flags


struct sfifo_state {
	int		head ;
	int		tail ;
	int		c ;
} ;

struct sfifo_flags {
	uint		ins:1 ;
	uint		rem:1 ;
} ;

struct sfifo_head {
	SFIFO_ST	c, n ;
	char		*buf ;
	char		*valid ;
	SFIFO_FL	fl ;
	uint		magic ;
	int		objsize ;
	int		nobj ;
} ;

typedef	SFIFO		sfifo ;
typedef	SFIFO_ST	sfifo_st ;
typedef	SFIFO_FL	sfifo_fl ;

EXTERNC_begin

extern int	sfifo_init(sfifo *,int,int) noex ;
extern int	sfifo_free(sfifo *) noex ;
extern int	sfifo_ins(sfifo *,void *) noex ;
extern int	sfifo_rem(sfifo *,void *) noex ;
extern int	sfifo_read(sfifo *,void *) noex ;
extern int	sfifo_clock(sfifo *) noex ;
extern int	sfifo_comb(sfifo *,int) noex ;

EXTERNC_end


#endif /* SFIFO_INCLUDE */


