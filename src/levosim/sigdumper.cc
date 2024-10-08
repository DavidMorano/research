/* sigdumper SUPPORT */
/* lang=C++98 */


#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<netdb.h>
#include	<strwcpy.h>
#include	<localmisc.h>

#include	"sigdumpmsg.h"


/* local defines */

#ifndef	MSGBUFLEN
#define	MSGBUFLEN	1024
#endif

#define	O_FLAGS		(O_NDELAY | O_WRONLY)


/* external subroutines */


/* external subroutines */


/* exported variables */


/* exported subroutines */

int sigdumper(cchar *fname,pid_t pid,char *s) noex {
	struct sigdumpmsg_request	m0{} ;
	int		rs ;
	int		rs1 ;
	int		wl = 0 ;
	if ((rs = u_open(fname,O_FLAGS,0666)) >= 0) {
	    cint	fd = rs ;
	    char	msgbuf[MSGBUFLEN + 1] ;
	    m0.tag = 0 ;
	    m0.pid = pid ;
	    strwcpy(m0.fname,s,(MAXNAMELEN - 1)) ;
	    ml = sigdumpmsg_request(msgbuf,MSGBUFLEN,0,&m0) ;
	    {
	        if ((rs = u_writen(fd,msgbuf,ml)) >= 0) {
	            wl = rs ;
		    sleep(1) ;
		}
	    }
	    rs1 = u_close(fd) ;
	    if (rs >= 0) rs = rs1 ;
	} /* end if (open0close) */
	return rs ;
}
/* end subroutine (sigdumper) */


