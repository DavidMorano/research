/* fcount */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	FCOUNT_INCLUDE
#define	FCOUNT_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>

#include	<vecitem.h>
#include	<localmisc.h>

#include	"lmapprog.h"


/* object defines */

#define	FCOUNT			struct fcount_head
#define	FCOUNT_ENTRY		struct fcount_entry


struct fcount_entry {
	char	*name ;				/* name of function */
	uint	ia ;				/* instruction address */
	uint	size ;				/* size of function */
	uint	calls ;				/* calls */
	uint	ins ;				/* instructions executed */
} ;

struct fcount_head {
	unsigned long		magic ;
	vecitem			table ;
	uint			in ;		/* instruction count */
	uint			func_i ;	/* index */
	uint			func_ia, func_size ;
	uint			other_calls, other_ins ;
} ;


#if	(! defined(FCOUNT_MASTER)) || (FCOUNT_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	fcount_init(FCOUNT *,LMAPPROG *,const char *) ;
extern int	fcount_update(FCOUNT *,uint,int) ;
extern int	fcount_done(FCOUNT *) ;
extern int	fcount_sort(FCOUNT *,int (*)()) ;
extern int	fcount_get(FCOUNT *,int,FCOUNT_ENTRY **) ;
extern int	fcount_gettotal(FCOUNT *,uint *,uint *) ;
extern int	fcount_getother(FCOUNT *,uint *,uint *) ;
extern int	fcount_free(FCOUNT *) ;

#ifdef	__cplusplus
}
#endif

#endif /* FCOUNT_MASTER */

#endif /* FCOUNT_INCLUDE */


