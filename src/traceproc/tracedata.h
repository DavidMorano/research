/* tracedata */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	TRACEDATA_INCLUDE
#define	TRACEDATA_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>

#include	<localmisc.h>

#include	"exectrace.h"


#define	TRACEDATA		struct tracedata
#define	TRACEDATA_NREGS		200


struct tracedata_flags {
	uint	ia : 1 ;
	uint	gotregs : 1 ;
} ;

struct tracedata {
	ULONG	clock ;
	ULONG	in ;
	uint	reg[TRACEDATA_NREGS] ;
	struct tracedata_flags	f ;
	char	gotreg[TRACEDATA_NREGS] ;
} ;


#if	(! defined(TRACEDATA_MASTER)) || (TRACEDATA_MASTER == 1)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	tracedata_init(struct tracedata *) ;
extern int	tracedata_free(struct tracedata *) ;
extern int	tracedata_proc(struct tracedata *, EXECTRACE_ENTRY *) ;
extern int	tracedata_recregs(struct tracedata *, EXECTRACE_ENTRY *) ;
extern int	tracedata_writeback(struct tracedata *, EXECTRACE_ENTRY *) ;

#ifdef	__cplusplus
}
#endif

#endif /* TRACEDATA_MASTER */

#define		tracedata_next(ip)	((ip)->in += 1)


#endif /* TRACEDATA_INCLUDE */


