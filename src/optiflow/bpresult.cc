/* bpresult */

/* write a BP result */
/* last modified %G% version %I% */


#define	CF_DEBUGS	0


/* revision history:

	= 02/06/14, Dave Morano

	This program was originally written.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/**************************************************************************

	This object module facilitates wiring branch-prediction results
	to a file.


**************************************************************************/


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<climits>
#include	<cstdlib>
#include	<cstring>
#include	<ctype.h>

#if	defined(IRIX)
#include	<standards.h>
#endif

#include	<vsystem.h>

#include	"localmisc.h"
#include	"bpresult.h"



/* local defines */

#define	BPRESULT_MAGIC	0x98326514

#define	DEFFILE		"bpdb"
#define	LINELEN		31
#define	TO_LOCK		4		/* seconds */

#if	defined(IRIX)
#ifndef	F_ULOCK
#define F_ULOCK	0	/* Unlock a previously locked region */
#endif
#ifndef	F_LOCK
#define F_LOCK	1	/* Lock a region for exclusive use */
#endif
#ifndef	F_TLOCK
#define F_TLOCK	2	/* Test and lock a region for exclusive use */
#endif
#ifndef	F_TEST
#define F_TEST	3	/* Test a region for other processes locks */
#endif
#endif /* defined(IRIX) */



/* external subroutines */


/* external variables */


/* local structures */


/* forward references */







int bpresult_open(op,fname)
BPRESULT	*op ;
char		fname[] ;
{
	int	rs ;


	if (op == NULL)
		return SR_FAULT ;

	if ((fname == NULL) || (fname[0] == '\0'))
		fname = DEFFILE ;

	rs = bopen(&op->rfile,fname,"wca",0666) ;

	if (rs < 0)
		return rs ;


	op->magic = BPRESULT_MAGIC ;
	return rs ;
}
/* end subroutine (bpresult_open) */


int bpresult_write(op,progname,acc,predname,bits,p1,p2,p3,p4)
BPRESULT	*op ;
char		progname[] ;
double		acc ;
char		predname[] ;
int		bits ;
int		p1, p2, p3, p4 ;
{
	int	rs ;
	int	f_locked ;


	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != BPRESULT_MAGIC)
		return SR_NOTOPEN ;

	if ((progname == NULL) || (predname == NULL))
		return SR_FAULT ;

	rs = bcontrol(&op->rfile,BC_LOCK,TO_LOCK) ;

	f_locked = (rs >= 0) ;

	rs = bprintf(&op->rfile,"%-15s %7.3f %-15s %d %d %d %d %d\n",
		progname,acc,predname,bits,p1,p2,p3,p4) ;

	if (f_locked)
		bcontrol(&op->rfile,BC_UNLOCK,0) ;

	return rs ;
}


int bpresult_close(op)
BPRESULT	*op ;
{
	int	rs ;


	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != BPRESULT_MAGIC)
		return SR_NOTOPEN ;


	rs = bclose(&op->rfile) ;

	return rs ;
}



