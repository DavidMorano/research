/* levomod SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* Levo loadable modules */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	This object module was created for Levo research.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

  	Names:
	flbsi

	Description:
	This contains various utility subroutines.

*******************************************************************************/

#include	<envstandards.h>	/* MUST be ordered first to configure */
#include	<cstddef>		/* CSTD */
#include	<cstdlib>		/* CSTD */
#include	<clanguage.h>		/* LIBU */
#include	<usysbase.h>		/* LIBU */
#include	<localmisc.h>		/* LIBU */

#include	"levomod.h"

#pragma		GCC dependency		"mod/findbit.ccm"

import findbit ;

namespace levomod {
    int flbsi(int v) noex {
	uint uv = uint(v) ;
	return flbs(uv) ;
    } /* end subroutine (flbsi) */
} /* end namespace */


