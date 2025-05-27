/* isvalidtextaddr */

/* is a memory addresss a valid text address */


#define	CF_DEBUGS	0		/* debug print-outs */
#define	CF_DEBUG	0		/* switchable debug print-outs */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This subroutine checks to see if a memory address is a valid
	text address.


******************************************************************************/


#include	<sys/types.h>
#include <cstdlib>
#include <cstring>
#include	<math.h>
#include	<assert.h>

/* simple scalar stuff */

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "cache.h"
#include "bpred.h"



/* local defines */




int isvalidtextaddr(a)
ULONG	a ;
{
	int	f ;


	f = (ld_text_base <= a) ;
	f = f && (a < (ld_text_base+ld_text_size)) ;
	f = f && (! (a & (sizeof(md_inst_t)-1))) ;

	return f ;
}
/* end subroutine (isvalidtextaddr) */



