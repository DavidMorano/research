/* strsimpoint */

/* print out the character string of a SIMPOINT */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */



#include	<sys/types.h>
#include	<sys/param.h>
#include	<climits>

#include	"localmisc.h"
#include	"defs.h"



/* local defines */

#ifndef	DIGBUFLEN
#define	DIGBUFLEN	20
#endif



/* external subroutines */

extern int	sncpy1(char *,int,const char *) ;
extern int	ctdecull(char *,int,ULONG) ;






char *strsimpoint(buf,v)
char		buf[] ;
ULONG		v ;
{
	int	rs = 0 ;

	char	*cp ;


	if (v == ULONG_UNSET)
		sncpy1(buf,DIGBUFLEN,"unset") ;

	else if (v == ULONG_SIMPOINT)
		sncpy1(buf,DIGBUFLEN,"simpoint") ;

	else
		rs = snprintf(buf,DIGBUFLEN,"%16llu",v) ;

	return (rs >= 0) ? buf : NULL ;
}
/* end subroutine (strsimpoint) */



