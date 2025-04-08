/* seqok */

/* sequence number check (is it OK?) */


/* revision history:

	= 2001-05-17, David A­D­ Morano
	This is for the transaction window sequence checking function.

*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	Synopsis:

	int seqok(s,e,m)
	uint	s ;
	uint	e ;
	uint	m ;

	Arguments:

	s		new snooped operand sequence number
	e		forwarding-receive sequence on record
	m		receive-forwarding modulus

	Results:

	0		the new sequence number was NOT OK
	1		the new sequence number WAS OK


*******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>
#include	<localmisc.h>


/* exported subroutines */


/* is a sequence number in our receive window ? */
int seqok(uint s,uint e,uint m)
{
	uint		lim ;
	uint		m2 = (m >> 1) ;
	int		f ;

	lim = (e + m2) % m ;
	if (e < lim) {
	    f = ((s >= e) && (s < lim)) ;
	} else {
	    f = ((s < lim) || (s >= e)) ;
	}

	return f ;
}
/* end subroutine (seqok) */


