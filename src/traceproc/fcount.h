/* fcount HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */


/* revision history:

	= 1998-11-01, David Morano
	Originally written for Audix Database Processor (DBP) work.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	FCOUNT_INCLUDE
#define	FCOUNT_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<vecitem.h>

#include	"lmapprog.h"


#define	FCOUNT		struct fcount_head
#define	FCOUNT_ENT	struct fcount_entry
#define	FCOUNT_MAGIC	0x23456787
#define	FCOUNT_DEFFUNCS	300		/* default entries */


struct fcount_entry {
	char		*name ;		/* name of function */
	uint		ia ;		/* instruction address */
	uint		size ;		/* size of function */
	uint		calls ;		/* calls */
	uint		ins ;		/* instructions executed */
} ; /* end struct */

struct fcount_head {
	vecitem		table ;
	uint		magic ;
	uint		in ;		/* instruction count */
	uint		func_i ;	/* index */
	uint		func_ia, func_size ;
	uint		other_calls, other_ins ;
} ; /* end struct */

typedef	FCOUNT		fcount ;
typedef	FCOUNT_ENT	fcount_ent ;

EXTERNC_begin

extern int	fcount_init(fcount *,lmapprog *,cchar *) noex ;
extern int	fcount_update(fcount *,uint,int) noex ;
extern int	fcount_done(fcount *) noex ;
extern int	fcount_sort(fcount *,int (*)()) noex ;
extern int	fcount_get(fcount *,int,fcount_ent **) noex ;
extern int	fcount_gettotal(fcount *,uint *,uint *) noex ;
extern int	fcount_getother(fcount *,uint *,uint *) noex ;
extern int	fcount_free(fcount *) noex ;

EXTERNC_end


#endif /* FCOUNT_INCLUDE */


