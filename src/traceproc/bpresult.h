/* bpresult HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* OptiFlow Branch-Prediction Result */
/* version %I% last-modified %G% */


/* revision history:

	= 1998-11-01, David Morano
	Originally written for Audix Database Processor (DBP) work.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	BPRESULT_INCLUDE
#define	BPRESULT_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>		/* POSIX® */
#include	<time.h>		/* POSIX® */
#include	<stddef.h>		/* CSTD */
#include	<stdlib.h>		/* CSTD */
#include	<clanguage.h>		/* LIBU */
#include	<usysbase.h>		/* LIBU */
#include	<bfile.h>		/* LIBB */


#define	BPRESULT	struct bpresult_head
#define	BPRESULT_MAGIC	0x98326514


struct bpresult_head {
	bfile		rfile ;
	uint		magic ;
} ; /* end struct */

typedef	BPRESULT	bpresult ;

EXTERNC_begin
extern int	bpresult_open(bpresult *,char *) noex ;
extern int	bpresult_write(bpresult *,char *,double, char *,
			int,int,int,int,int) noex ;
extern int	bpresult_close(bpresult *) noex ;
EXTERNC_end


#endif /* BPRESULT_INCLUDE */


