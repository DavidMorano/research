/* itype HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* instruction-type classification */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-01-07, David A-D- Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	ITYPE_INCLUDE
#define	ITYPE_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<bfile.h>

#include	"sscommon.h"
#include	"ssas.h"


/* object instantiation defines */
#define	ITYPE		struct itype_head
#define	ITYPE_MAGIC	0x33221158


struct itype_head {
	ulong	magic ;
	ulong	itype_ins ;
	ulong	itype_class[iclass_overlast] ;
	ulong	itype_cf ;
	ulong	itype_cfcond ;
	ulong	itype_cfdir ;
	ulong	itype_cfind ;
	ulong	itype_cfsub ;
	ulong	itype_mem ;
	ulong	itype_memload, itype_memstore ;
} ; /* end struct */

typedef	ITYPE		itype ;

extern int	itype_init(ITYPE *) ;
extern int	itype_proc(ITYPE *,SSAS *) ;
extern int	itype_writeout(ITYPE *,cchar *) ;
extern int	itype_free(ITYPE *) ;


#endif /* ITYPE_INCLUDE */


