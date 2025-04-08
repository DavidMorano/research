/* debug */

/* debugging stubs */


#define	CF_DEBUGS	0



/* revision history :

	= 96/11/21, Dave Morano

	This program was started by copying from the RSLOW program.


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
#include	<pwd.h>
#include	<grp.h>
#include	<stropts.h>
#include	<poll.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<time.h>
#include	<netdb.h>

#include	<bio.h>
#include	<logfile.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"



/* local defines */



/* external subroutines */


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


#if	CF_DEBUGS
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

#if	CF_DEBUGS
	eprintf("d_reventstr: exiting\n") ;
	eprintf("d_reventstr: 2 exiting> %s\n",buf) ;
#endif

	return buf ;
}
/* end subroutine (d_reventstr) */


/* who is open ? */
void whoopen()
{
	struct stat	sb ;

	int	i ;


#if	CF_DEBUGS
	for (i = 0 ; i < 20 ; i += 1) {

	    if (u_fstat(i,&sb) == 0)
	        eprintf("whoopen: open on %d\n",i) ;

	}
#endif

}
/* end subroutine (whoopen) */


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


/* calculate a checksum on a block of data */
int d_checkcalc(sp,size,cp)
int	*sp ;
int	size ;
int	*cp ;
{
	int	sum ;

	int	*ol = sp + (size / sizeof(int)) ;


	sum = 0 ;
	*cp = 0 ;
	while (sp < ol)
		sum += *sp++ ;

	*cp =  (- sum) ;
	return size ;
}


/* verify a checksum on a block of data */
int d_checkverify(sp,size)
int	*sp ;
int	size ;
{
	int	sum ;

	int	*ol = sp + (size / sizeof(int)) ;


	sum = 0 ;
	while (sp < ol)
		sum += *sp++ ;

	return ((sum == 0) ? 0 : SR_BADFMT) ;
}


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




