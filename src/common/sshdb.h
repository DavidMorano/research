/* sshdb HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

#ifndef	SSHDB_INCLUDE
#define	SSHDB_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>

#include	"levomod.h"


#define	SSHDB			struct sshdb_head
#define	SSHDB_ENT		struct sshdb_recentry
#define	SSHDB_INFO		struct sshdb_information
#define	SSHDB_MAGIC		0x23459287
#define	SSHDB_FILEMAGIC		"HAMMOCKS"
#define	SSHDB_FILEVERSION	0
/* branch types */
#define	SSHDB_BTFWD		1	/* forward */
#define	SSHDB_BTSSHDB		2	/* Simple Single-sided Hammock */


struct sshdb_information {
	time_t		write ;		/* time DB written */
	uint		entries ;	/* total number of entries */
	uint		c_cf ;		/* control flow */
	uint		c_rel ;		/* relative branch */
	uint		c_con ;		/* conditional branch */
	uint		c_fwd ;		/* forward conditional branch */
	uint		c_ssh ;		/* SS-hammock */
} ; /* end struct */

struct sshdb_recentry {
	uint		ia ;		/* instruction address */
	uint		ta ;		/* branch target address */
	uint		domainsz ;	/* domain size (instructions) */
	uint		type ;		/* branch type */
} ; /* end struct */

struct sshdb_head {
	uint		(*recind)[2] ;
	SSHDB_ENT	*rectab ;
	void		*mdata ;
	size_t		msize ;
	uint		magval ;
	uint		filesz ;
	uint		rtlen ;
	uint		rilen ;
} ; /* end struct */

typedef	SSHDB		sshdb ;
typedef	SSHDB_ENT	sshdb_ent ;
typedef	SSHDB_INFO	sshdb_info ;

EXTERNC_begin

extern int	sshdb_start	(sshdb *,char *) noex ;
extern int	sshdb_lookup	(sshdb *,uint,sshdb_ent **) noex ;
extern int	sshdb_getinfo	(sshdb *,sshdb_info *) noex ;
extern int	sshdb_get	(sshdb *,int,sshdb_ent **) noex ;
extern int	sshdb_finish	(sshdb *) noex ;

EXTERNC_end

extern const levomod_obj	sshdb_mod ;


#endif /* SSHDB_INCLUDE */


