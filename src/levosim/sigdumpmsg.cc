/* sigdumpmsg SUPPORT */
/* lang=C++98 */

/* create and parse the internal messages */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0

/* revision history:

	= 1999-07-21, Dave Morano
	This module was originally written.

*/


/******************************************************************************

  	Name:
	sigdumpmsg

	Description:
	This module contains the subroutines to make and parse the 
	SIGDUMPMSG family of messages.

******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/socket.h>
#include	<sys/uio.h>
#include	<arpa/inet.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<usystem.h>
#include	<vecstr.h>
#include	<serialbuf.h>
#include	<localmisc.h>

#include	"sigdumpmsg.h"


/* local defines */

#ifndef	BUFLEN
#define	BUFLEN		1024
#endif


/* external subroutines */


/* external variables */


/* local structures */


/* local references */


/* local variables */


/* exported subroutines */


/* exported variables */

int sigdumpmsg_request(buf,buflen,f,sp)
char			buf[] ;
int			buflen ;
int			f ;
struct sigdumpmsg_request	*sp ;
{
	SERIALBUF	msgbuf ;

	int	rs ;


#if	CF_DEBUGS
	debugprintf("sigdumpmsg_request: buflen=%d\n",buflen) ;
#endif

	serialbuf_init(&msgbuf,(char *) buf,buflen) ;

	if (! f) {

/* write */

	    sp->type = sigdumpmsgtype_request ;

	    serialbuf_wchar(&msgbuf,sp->type) ;

	    serialbuf_wuint(&msgbuf,sp->tag) ;

	    serialbuf_wuint(&msgbuf,sp->pid) ;

	    serialbuf_wstrw(&msgbuf,sp->fname,(MAXNAMELEN - 1)) ;

	} else {

/* read */

	    serialbuf_ruchar(&msgbuf,&sp->type) ;

	    serialbuf_ruint(&msgbuf,&sp->tag) ;

	    serialbuf_ruint(&msgbuf,&sp->pid) ;

	    serialbuf_rstrw(&msgbuf,sp->fname,(MAXNAMELEN - 1)) ;

	} /* end if */

	rs = serialbuf_free(&msgbuf) ;

	return rs ;
}
/* end subroutine (sigdumpmsg_request) */



