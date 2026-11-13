/* recorder SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* IA recorder object */
/* version %I% last-modified %G% */


/* revision history:

	= 1998-04-13, David A-D- Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/*******************************************************************************

  	Description:
	This object module creates a data base of SS-Hammock style
	branches.  This object can form the "lower-half" of a system
	to identify SS-Hammocks and to write the resulting data base
	information out to a file.  This object only manages an
	in-core version of the database.  Another "upper-level"
	component must manage:

*******************************************************************************/

#ifndef	RECORDER_INCLUDE
#define	RECORDER_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>		/* POSIX® */
#include	<stddef.h>		/* CSTD */
#include	<stdlib.h>		/* CSTD */
#include	<clanguage.h>		/* LIBU */
#include	<utypedefs.h>		/* LIBU */
#include	<utypealiases.h>	/* LIBU */
#include	<usysdefs.h>		/* LIBU */


/* object defines */
#define	RECORDER		struct recorder_head
#define	RECORDER_ENT		struct recorder_entry
#define	RECORDER_MAGIC		0x12356734
#define	RECORDER_STARTNUM	100	/* starting number records */
/* branch types */
#define	RECORDER_BTFWD		1	/* forward */
#define	RECORDER_BTSSH		2	/* Simple Single-sided Hammock */


struct recorder_entry {
	uint	ia ;		/* instruction address */
	uint	ta ;		/* branch target address */
	uint	domainsize ;	/* domain size (instructions) */
	uint	type ;		/* branch type */
} ; /* end struct */

struct recorder_head {
	RECORDER_ENT	*rectab ;
	uint		magval ;
	int	i ;		/* current length */
	int	e ;		/* current buffer extent */
	int	c ;		/* count */
} ; /* end struct */

typedef	RECORDER	recorder ;

EXTERNC_begin
extern int	recorder_init		(recorder *,int) noex ;
extern int	recorder_free		(recorder *) noex ;
extern int	recorder_add		(recorder *,uint,uint,uint,uint) noex ;
extern int	recorder_already	(recorder *,uint) noex ;
extern int	recorder_gettab		(recorder *,uint **) noex ;
extern int	recorder_rtlen		(recorder *) noex ;
extern int	recorder_count		(recorder *) noex ;
extern int	recorder_indexlen	(recorder *) noex ;
extern int	recorder_indexsize	(recorder *) noex ;
extern int	recorder_mkindex	(recorder *,uint [][2],int) noex ;
EXTERNC_end


#endif /* RECORDER_INCLUDE */


