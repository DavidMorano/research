/* percentll */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */


#include	<sys/types.h>

#include	"localmisc.h"




double percentll(in,id)
ULONG	in, id ;
{
	double	fn, fd, fp ;


	fn = in ;
	fd = id ;

	fp = 0.0 ;
	if (id)
		fp = (fn / fd) * 100.0 ;

	return fp ;
}
/* end subroutine (percentll) */



