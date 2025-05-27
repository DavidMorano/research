/* expander */

/* expand out some per program parameters */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0


/* revision history :

	= 91/09/01, Dave Morano

	This subroutine was originally written.


*/



/*****************************************************************************

	This subroutine expands out some per program (the daemon
	program) parameters into the configuration strings.



*****************************************************************************/



#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<netdb.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstring>
#include	<ctype.h>

#include	<vsystem.h>
#include	<bio.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"




/*
#	The following substitutions are made on command strings :

#		%V	Directory Watcher version string
#		%R	program root directory
#		%N	machine node name
#		%D	machine INET domain name
#		%H	machine INET host name
#		%U	invoking username
#		%G	invoking groupname
#
*/


/* external subroutines */

extern int	strnlen(char *,int) ;





int expander(app,buf,len,rbuf,rlen)
struct proginfo		*app ;
char			rbuf[], buf[] ;
int			rlen, len ;
{
	int	elen = 0, sl ;
	int	rs ;

	char	hostname[MAXHOSTNAMELEN + 1] ;
	char	*rbp = rbuf ;
	char	*bp = buf ;
	char	*cp ;


#if	CF_DEBUGS
	eprintf("expnader: entered, buflen=%d rbuflen=%d\n",
	    len,rlen) ;
#endif

	len = strnlen(buf,len) ;

	if (len == 0) 
		return 0 ;

#if	CF_DEBUGS
	eprintf("expnader: buf=\"%W\"\n",buf,len) ;
#endif

	rlen -= 1 ;			/* reserve for zero terminator */
	while ((len > 0) && (elen < rlen)) {

	    switch ((int) *bp) {

	    case '%':
	        bp += 1 ;
	        len -= 1 ;
	        if (len == 0)
			return elen ;

	        switch ((int) *bp) {

	        case 'N':
	            cp = app->nodename ;
	            sl = strlen(cp) ;

	            break ;

	        case 'D':
	            cp = app->domainname ;
	            sl = strlen(cp) ;

	            break ;

/* invoking groupname */
	        case 'G':
	            sl = 0 ;
	            if ((cp = app->groupname) != NULL)
	                sl = strlen(cp) ;

	            break ;

	        case 'H':
	            cp = hostname ;
	            sl = bufprintf(hostname,MAXHOSTNAMELEN,"%s.%s",
	                app->nodename,app->domainname) ;

	            break ;

/* handle the expansion of our program root */
	        case 'R':
#if	CF_DEBUGS
	            eprintf("expnader: programroot=%s\n",app->programroot) ;
#endif
	            sl = 0 ;
	            if ((cp = app->programroot) != NULL)
	                sl = strlen(cp) ;

	            break ;

/* invoking username */
	        case 'U':
	            sl = 0 ;
	            if ((cp = app->username) != NULL)
	                sl = strlen(cp) ;

	            break ;

	        case 'V':
	            cp = app->version ;
	            sl = strlen(cp) ;

	            break ;

	        default:
	            cp = bp ;
	            sl = 1 ;

	        } /* end switch */

	        bp += 1 ;
	        len -= 1 ;

	        if ((elen + sl) > rlen)
	            goto bad ;

#if	CF_DEBUGS
	        eprintf("expnader: copying over, sl=%d\n",sl) ;
#endif

	        if (sl > 0)
	            strncpy(rbp,cp,sl) ;

#if	CF_DEBUGS
	        eprintf("expnader: copied\n") ;
#endif

	        rbp += sl ;
	        elen += sl ;
	        break ;

	    default:
	        *rbp++ = *bp++ ;
	        elen += 1 ;
	        len -= 1 ;

	    } /* end switch */

	} /* end while */

	rs = (elen >= rlen) ? SR_NOMEM : elen ;

/* we are out of here */
ret:
	rbuf[elen] = '\0' ;

#if	CF_DEBUGS
	eprintf("expander: existing, rs=%d\n",rs) ;
#endif

	return rs ;

/* bad things come here */
bad:

#if	CF_DEBUGS
	eprintf("expnader: we're bad\n") ;
#endif

	rs = SR_NOMEM ;
	goto ret ;
}
/* end subroutine (expander) */



