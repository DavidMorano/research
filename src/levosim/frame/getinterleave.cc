/* getinterleave SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* get a memory interleave index from an address and interleave schedule */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable debug print-outs */

/* revision history:

	= 2000-07-18, Dave Morano
	This subroutine was written to calculate the proper bus
	to use when interleaved buses are provided.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/******************************************************************************

  	Name:

	Description:
	This subroutine takes an interleave schedule and an address
	and calculates which interleaved bus (by bus index) should
	be used for for the associated operation given the address.

	Synopsis:
	int getinterleave(uint interleave,uint addr)

	Arguments:
	+ interleave	the interleave schedule
	+ address	the address that is in view

	Returns:
	index		the bus index of the interleaved buses

******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<cstddef>
#include	<cstdlib>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<bitops.h>
#include	<findbit.h>
#include	<localmisc.h>


/* local defines */


/* external subroutines */


/* exported variables */


/* exported subroutines */

int getinterleave(uint interleave,uint addr) noex {
	int	result = 0 ;
	for (int i = 0, bn = ffbs(interleave) ; bn >= 0 ; ) {
	    if (BTSTI(&addr,bn)) {
	        result |= (1 << i) ;
	    }
	    BCLRI(&interleave,bn) ;
	    i += 1 ;
	    bn = ffbsi(interleave) ;
	} /* end while */
	return result ;
}
/* end subroutine (getinterleave) */


