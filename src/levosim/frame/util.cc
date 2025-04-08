/* util */

/* utilities */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0


/* revision history :

	= 94/09/10, Dave Morano

	This program was originally written.


*/



/*****************************************************************************

	These are support utility subroutines for the REXD daemon program.


*****************************************************************************/



#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<netdb.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<time.h>
#include	<string.h>
#include	<ctype.h>

#include	<bio.h>
#include	<field.h>
#include	<logfile.h>
#include	<userinfo.h>
#include	<bitops.h>
#include	<char.h>
#include	<varsub.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"




/* local defines */



/* external subroutines */

extern int	cfdeci(char *,int,int *) ;
extern int	substring() ;
extern int	getpwd() ;
extern int	userinfo() ;
extern int	username(char *) ;
extern int	getnodedomain(char *,char *) ;
extern int	group_getgid() ;

extern char	*strbasename() ;
extern char	*timestr_log() ;


/* externals variables */


/* local structures */


/* forward references */


/* local global variabes */


/* local variables */





int getportnum(s,rp)
char	s[] ;
int	*rp ;
{
	struct servent	*sep ;

	int	rs ;


	*rp = -1 ;
	if (isalpha(*s)) {

	    sep = getservbyname(s, "tcp") ;

	    if (sep != NULL) {

	        rs = OK ;
	        *rp = (int) ntohs(sep->s_port) ;

	    } else
	        rs = BAD ;

	} else 
	    rs = cfdeci(s,-1,rp) ;

	return rs ;
}
/* end subroutine (getportnum) */


int getarrayint(a,n)
int	a[] ;
int	n ;
{


	return a[n] ;
}



