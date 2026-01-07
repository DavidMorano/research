/* filtercalls */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	FILTERCALLS_INCLUDE
#define	FILTERCALLS_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>

#include	<hdb.h>
#include	<localmisc.h>		/* for 'ULONG' */

#include	"lmapprog.h"


#ifndef	UINT
#define	UINT	unsigned int
#endif

/* object defines */

#define	FILTERCALLS		struct filtercalls_head
#define	FILTERCALLS_ENTRY	struct filtercalls_e


struct filtercalls_e {
	uint	symval ;
	char	*name ;
} ;

struct filtercalls_flags {
	UINT	eof : 1 ;		/* EOF indication on reading */
	UINT	in : 1 ;		/* got a starting instruction number */
	UINT	read : 1 ;		/* reading or writing */
	UINT	append : 1 ;		/* appending */
} ;

struct filtercalls_head {
	unsigned long	magic ;
	LMAPPROG	*mp ;
	struct filtercalls_flags		f ;
	HDB		index ;
} ;


#if	(! defined(FILTERCALLS_MASTER)) || (FILTERCALLS_MASTER == 1)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	filtercalls_init(FILTERCALLS *,LMAPPROG *,const char *) ;
extern int	filtercalls_free(FILTERCALLS *) ;
extern int	filtercalls_have(FILTERCALLS *,UINT,const char **) ;

#ifdef	__cplusplus
}
#endif

#endif /* (! defined(FILTERCALLS_MASTER)) || (FILTERCALLS_MASTER == 0) */

#endif /* FILTERCALLS_INCLUDE */


