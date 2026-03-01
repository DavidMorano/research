/* levomod HEADER */
/* charset=ISO8859-1 */
/* lang=C20 */

/* branch predictor */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

#ifndef	LEVOMOD_INCLUDE
#define	LEVOMOD_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>


#define	LEVOMOD		struct levomod_obj


struct levomod_obj {
	cchar	*objname ;
	int	objsz ;
} ; /* end struct (levomod_obj) */

#ifdef	__cplusplus
namespace levomod {
    extern int flbsi(int) noex ;
} /* end namespace */
#endif /* __cplusplus */


#endif /* LEVOMOD_INCLUDE */


