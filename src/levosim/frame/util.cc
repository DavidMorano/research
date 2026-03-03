/* util SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* utilities */
/* version %I% last modified %G% */

#define	CF_DEBUGS	0

/* revision history:

	= 1994-09-10, Dave Morano
	This program was originally written.

*/

/* Copyright © 1994 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/*****************************************************************************

  	Description:
	These are support utility subroutines for the REXD daemon
	program.

*****************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<netdb.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctime>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>		/* |getenv(3c)| */
#include	<cstring>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<bfile.h>
#include	<field.h>
#include	<logfile.h>
#include	<userinfo.h>
#include	<bitops.h>
#include	<cfdec.h>
#include	<char.h>
#include	<varsub.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"


/* local defines */


/* external subroutines */


/* externals variables */


/* local structures */


/* forward references */


/* local variables */


/* exported variables */


/* exported subroutines */

int getportnum(cchar *s,int *rp) {
	SERVENT	*sep ;
	int	rs ;

	*rp = -1 ;
	if (isalphalatin(*s)) {
	    sep = getservbyname(s, "tcp") ;
	    if (sep != NULL) {
	        rs = OK ;
	        *rp = (int) ntohs(sep->s_port) ;
	    } else {
	        rs = BAD ;
	    }
	} else  {
	    rs = cfdeci(s,-1,rp) ;
	}
	return rs ;
}
/* end subroutine (getportnum) */

int getarrayint(int *a,int n) {
	return a[n] ;
}


