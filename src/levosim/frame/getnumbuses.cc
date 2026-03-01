/* getnumbuses SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* calculate the number of buses from an interleave schedule */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable */

/* revision history:

	= 2000-02-15, Dave Morano
	This code was started.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/******************************************************************************

  	Name:
	getnumbuses

	Description:
	This subroutine calculates the number of buses (or memory banks)
	given an interleave schedule for that set of buses (or memory
	block).

******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<cstdlib>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstring>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<localmisc.h>

#include	"defs.h"


/* local defines */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */


/* local variables */


/* exported variables */


/* exported subroutines */

int getnumbuses(int inter) {
	int	n ;
	{
	    n = fbscounti(inter) ;
	}
	return (1 << n) ;		/* n = 2 ^ n */
}
/* end subroutine (getnumbuses) */


