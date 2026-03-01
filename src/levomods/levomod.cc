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
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<localmisc.h>

#include	"levomod.h"

#pragma		GCC dependency		"mod/flbs.ccm"

import flbs ;

namespace levomod {
    int flbsi(int v) noex {
	uint uv = uint(v) ;
	return flbs(uv) ;
    }
} /* end namespace */


