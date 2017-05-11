/* recorder */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	RECORDER_INCLUDE
#define	RECORDER_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>

#include	<localmisc.h>


/* object defines */

#define	RECORDER			struct recorder_head
#define	RECORDER_STARTNUM	100	/* starting number records */

/* branch types */

#define	RECORDER_BTFWD		1	/* forward */
#define	RECORDER_BTSSH		2	/* Simple Single-sided Hammock */


struct recorder_entry {
	uint	ia ;		/* instruction address */
	uint	ta ;		/* branch target address */
	uint	domainsize ;	/* domain size (instructions) */
	uint	type ;		/* branch type */
} ;

struct recorder_head {
	unsigned long		magic ;
	struct recorder_entry	*rectab ;
	int	i ;		/* current length */
	int	e ;		/* current buffer extent */
	int	c ;		/* count */
} ;


#if	(! defined(RECORDER_MASTER)) || (RECORDER_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	recorder_init(RECORDER *,int) ;
extern int	recorder_free(RECORDER *) ;
extern int	recorder_add(RECORDER *,uint,uint,uint,uint) ;
extern int	recorder_already(RECORDER *,uint) ;
extern int	recorder_gettab(RECORDER *,uint **) ;
extern int	recorder_rtlen(RECORDER *) ;
extern int	recorder_count(RECORDER *) ;
extern int	recorder_indexlen(RECORDER *) ;
extern int	recorder_indexsize(RECORDER *) ;
extern int	recorder_mkindex(RECORDER *,uint [][2],int) ;

#ifdef	__cplusplus
}
#endif

#endif /* RECORDER_MASTER */

#endif /* RECORDER_INCLUDE */


