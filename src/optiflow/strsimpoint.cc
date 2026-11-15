/* strsimpoint SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* print out the character string of a SIMPOINT */
/* version %I% last-modified %G% */


/* revision history:

	= 1998-04-13, David A-D- Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<climits>
#include	<cstddef>		/* CSTD */
#include	<cstdlib>		/* CSTD */
#include	<clanguage.h>
#include	<csysbase.h>
#include	<localmisc.h>		/* LIBU */


/* local defines */


/* exported variables */


/* external subroutines */

char *strsimpoint(char *buf,ulong v) noex {
	int	rs = 0 ;
	char	*cp ;
	if (v == ulong_UNSET) {
		sncpy1(buf,DIGBUFLEN,"unset") ;
	} else if (v == ulong_SIMPOINT) {
		sncpy1(buf,DIGBUFLEN,"simpoint") ;
	} else {
		rs = snprintf(buf,DIGBUFLEN,"%16llu",v) ;
	}
	return (rs >= 0) ? buf : nullptr ;
} /* end subroutine (strsimpoint) */


