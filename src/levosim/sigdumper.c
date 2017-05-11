/* sigdumper */



#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<netdb.h>

#include	"sigdumpmsg.h"

#include	"localmisc.h"



/* local defines */

#ifndef	MSGBUFLEN
#define	MSGBUFLEN	1024
#endif

#define	O_FLAGS		(O_NDELAY | O_WRONLY)



/* external subroutines */

extern char	*strwcpy(char *,const char *,int) ;







int sigdumper(fname,pid,s)
char	fname[] ;
int	pid ;
char	s[] ;
{
	struct sigdumpmsg_request	m0 ;

	int	rs, ml ;
	int	fd ;

	char	msgbuf[MSGBUFLEN + 1] ;


	rs = u_open(fname,O_FLAGS,0666) ;

	if (rs < 0)
		return rs ;

	fd = rs ;

	m0.tag = 0 ;
	m0.pid = pid ;
	strwcpy(m0.fname,s,(MAXNAMELEN - 1)) ;

	ml = sigdumpmsg_request(msgbuf,MSGBUFLEN,0,&m0) ;

	rs = u_writen(fd,msgbuf,ml) ;

	u_close(fd) ;

	if (rs > 0)
		sleep(1) ;

	return rs ;
}
/* end subroutine (sigdumper) */



