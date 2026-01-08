/* bpresult */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	BPRESULT_INCLUDE
#define	BPRESULT_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>

#include	<bfile.h>
#include	<localmisc.h>


/* object defines */
#define	BPRESULT			struct bpresult_head


struct bpresult_head {
	unsigned long		magic ;
	bfile			rfile ;
} ;


#if	(! defined(BPRESULT_MASTER)) || (BPRESULT_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	bpresult_open(BPRESULT *,char *) ;
extern int	bpresult_write(BPRESULT *,char *,double, char *,
			int,int,int,int,int) ;
extern int	bpresult_close(BPRESULT *) ;

#ifdef	__cplusplus
}
#endif

#endif /* BPRESULT_MASTER */

#endif /* BPRESULT_INCLUDE */


