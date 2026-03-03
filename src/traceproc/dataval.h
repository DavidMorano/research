/* dataval HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */


/* revision history:

	= 1998-11-01, David Morano
	Originally written for Audix Database Processor (DBP) work.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>


#ifndef	DATAVAL
#define	DATAVAL		union dataval


union dataval {
	float		f ;
	int		i ;
	unsigned int	ui ;
} ;


#endif /* DATAVAL */


