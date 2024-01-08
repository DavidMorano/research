/* main (pluser) */
/* lang=C++98 */


/* expand a certain space character to a tab character */


/* revision history:

	= 2000-05-17, David A.D. Morano
	This subroutine was originally written.

*/

/* Copyright © 200 David A­D­ Morano.  All rights reserved. */

#include	<sys/types.h>
#include	<cerrno>
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


/* exported subroutines */

int main(int,mainv,mainv) {
	cint		llen = LINEBUFLEN ;
	int		rs = SR_NOMEM ;
	int		ex = EXIT_SUCCESS ;
	char		*lbuf ;
	if ((lbuf = new(nothrow) char[llen+1]) != nullptr) {
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


