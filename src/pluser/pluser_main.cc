/* main SUPPORT (pluser) */
/* charset=ISO8859-1 */
/* lang=C++98 */

/* expand a certain space character to a tab character */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-05-17, David A-D- Morano
	This subroutine was originally written.

*/

/* Copyright © 200 David A-D- Morano.  All rights reserved. */

#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<cerrno>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstdio>
#include	<cstring>
#include	<new>
#include	<iostream>
#include	<usystem.h>
#include	<stdsio.h>
#include	<localmisc.h>


/* local defines */

#ifndef	LINEBUFLEN
#define	LINEBUFLEN	1024
#endif


/* local namespaces */

using std::cout ;			/* variable */
using std::nothrow ;			/* constant */


/* exported variables */


/* exported subroutines */

int main(int,mainv,mainv) {
    	cnullptr	np{} ;
	cint		llen = LINEBUFLEN ;
	int		rs = SR_NOMEM ;
	int		ex = EXIT_SUCCESS ;
	if (char *lbuf ; (lbuf = new(nothrow) char[llen + 1]) != np) {
	    while ((rs = freadln(stdin,lbuf,llen)) > 0) {
	        int	len = rs ;
		lbuf[len] = '\0' ;
		if (strncmp(lbuf,"+ ",2) == 0) {
		    lbuf[1] = '\t' ;
		}
		cout << lbuf ;
	    } /* end while */
	    delete [] lbuf ;
	} /* end if (new-char) */
	if ((rs < 0) && (ex == EXIT_SUCCESS)) ex = EXIT_FAILURE ;
	return ex ;
}
/* end subroutine (main) */


