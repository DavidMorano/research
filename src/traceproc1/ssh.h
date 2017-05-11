/* ssh */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	SSH_INCLUDE
#define	SSH_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>

#include	<localmisc.h>


/* object defines */

#define	SSH			struct ssh_head
#define	SSH_ENTRY		struct ssh_recentry
#define	SSH_INFO		struct ssh_info

#define	SSH_FILEMAGIC	"HAMMOCKS"
#define	SSH_FILEVERSION	0

/* branch types */

#define	SSH_BTFWD	1	/* forward */
#define	SSH_BTSSH	2	/* Simple Single-sided Hammock */


/* some miscellaneous information */
struct ssh_info {
	time_t	write ;		/* time DB written */
	uint	entries ;	/* total number of entries */
	uint	c_cf ;		/* control flow */
	uint	c_rel ;		/* relative branch */
	uint	c_con ;		/* conditional branch */
	uint	c_fwd ;		/* forward conditional branch */
	uint	c_ssh ;		/* SS-hammock */
} ;

struct ssh_recentry {
	uint	ia ;		/* instruction address */
	uint	ta ;		/* branch target address */
	uint	domainsize ;	/* domain size (instructions) */
	uint	type ;		/* branch type */
} ;

struct ssh_head {
	unsigned long		magic ;
	caddr_t			filemap ;
	struct ssh_recentry	*rectab ;
	uint			(*recind)[2] ;
	uint			filesize ;
	uint			rtlen, rilen ;
} ;


#if	(! defined(SSH_MASTER)) || (SSH_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	ssh_init(SSH *,char *) ;
extern int	ssh_free(SSH *) ;
extern int	ssh_lookup(SSH *,uint,SSH_ENTRY **) ;
extern int	ssh_info(SSH *,SSH_INFO *) ;
extern int	ssh_get(SSH *,int,SSH_ENTRY **) ;

#ifdef	__cplusplus
}
#endif

#endif /* SSH_MASTER */

#endif /* SSH_INCLUDE */


