/* statemips */


/* revision history:

	= 2001-08-06, David Morano

	This is needed for skipping instrcutions when using LevoSim.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	STATEMIPS_INCLUDE
#define	STATEMIPS_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>

#include	<localmisc.h>

#include	"lmipsregs.h"


/* architected state of the MIPS machine */

ustatemips {
	ULONG	in ;
	uint	regs[LMIPSREG_NREGS] ;
	uint	ia ;
} ;


#endif /* STATEMIPS_INCLUDE */


