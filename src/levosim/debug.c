/* debug */

/* debugging stubs */


#define	F_DEBUGS	0


/* revision history :

	- 96/11/21, Dave Morano

	This program was started by copying from the RSLOW program.


	= 98/01/15, Dave Morano

	I modified added the little diddy to make a HEX string
	out of the contents of a binary buffer.


*/



/**************************************************************************

	This modeule provides debugging support for the REXEC program.



**************************************************************************/



#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<sys/param.h>
#include	<sys/utsname.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<netdb.h>
#include	<stropts.h>
#include	<poll.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<pwd.h>
#include	<grp.h>

#include	<bio.h>
#include	<logfile.h>

#include	"localmisc.h"



/* local defines */

#ifndef	XDEBFILE
#define	XDEBFILE		"/var/tmp/pcspoll.deb"
#endif



/* external subroutines */

extern int	strnlen(const char *,int) ;


/* forward subroutines */


/* external variables */


/* local variables */

static const char	hextable[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
} ;






char *d_reventstr(revents,buf,buflen)
int	revents ;
char	buf[] ;
int	buflen ;
{


#if	F_DEBUGS
	eprintf("d_reventstr: entered\n") ;
#endif

	buf[0] = '\0' ;
	bufprintf(buf,buflen,"%s %s %s %s %s %s %s %s %s",
	    (revents & POLLIN) ? "I " : "  ",
	    (revents & POLLRDNORM) ? "IN" : "  ",
	    (revents & POLLRDBAND) ? "IB" : "  ",
	    (revents & POLLPRI) ? "PR" : "  ",
	    (revents & POLLWRNORM) ? "WN" : "  ",
	    (revents & POLLWRBAND) ? "WB" : "  ",
	    (revents & POLLERR) ? "ER" : "  ",
	    (revents & POLLHUP) ? "HU" : "  ",
	    (revents & POLLNVAL) ? "NV" : "  ") ;

#if	F_DEBUGS
	eprintf("d_reventstr: %s\n",buf) ;
#endif

	return buf ;
}
/* end subroutine (d_reventstr) */


/* who is open ? */
void d_whoopen(s)
char	s[] ;
{
	int	rs, i ;


	if (s != NULL)
		eprintf("d_whoopen: %s\n",s) ;

	for (i = 0 ; i < 20 ; i += 1) {

	    if ((rs = u_fcntl(i,F_GETFL,0)) >= 0)
	        eprintf("d_whoopen: open on %d accmod=%08x\n",
			i,rs & O_ACCMODE) ;

	}

}
/* end subroutine (d_whoopen) */


/* return a count of the number of open files */
int d_openfiles()
{
	struct stat	sb ;

	int	i ;
	int	count = 0 ;


	for (i = 0 ; i < 2048 ; i += 1) {

		if (u_fstat(i,&sb) >= 0)
			count += 1 ;

	} /* end for */

	return count ;
}
/* end subroutine (d_openfiles) */


int d_ispath(p)
const char	*p ;
{


	if (p == NULL)
		return FALSE ;

#ifdef	DEBFILE
	nprintf(DEBFILE,"d_ispath: PATH=>%W<\n",
		p,strnlen(p,30)) ;
#endif

	return ((*p == '/') || (*p == ':')) ;
}
/* end subroutine (d_ispath) */


int mkhexstr(hexbuf,p,plen)
char		hexbuf[] ;
const void	*p ;
int		plen ;
{
	int	i, j ;

	char	*b = (char *) p ;


	if (plen < 0)
		plen = strlen(p) ;

	j = 0 ;
	hexbuf[j] = '\0' ;
	for (i = 0 ; i < plen ; i += 1) {

		hexbuf[j++] = ' ' ;
		hexbuf[j++] = hextable[(b[i] >> 4) & 15] ;
		hexbuf[j++] = hextable[b[i] & 15] ;

	} /* end for */

	hexbuf[j] = '\0' ;
	return j ;
}
/* end subroutine (mkhexstr) */



