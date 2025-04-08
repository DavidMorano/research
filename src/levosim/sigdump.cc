

#include	<sys/types.h>
#include	<sys/param.h>
#include	<netdb.h>





int sigdump(pid,s)
int	pid ;
char	s[] ;
{
	int	rs ;

	char	cmd[MAXHOSTNAMELEN + 1] ;


	sprintf(cmd,"psig %d > %s.sig",pid,s) ;

	rs = system(cmd) ;

	return rs ;
}



