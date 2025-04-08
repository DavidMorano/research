/* ss */

/* simulator support */


#define	F_DEBUGS	0		/* debug print-outs */
#define	F_TRACE		0		/* debug trace */
#define	F_FAKE		0		/* use fake until we get real */


/* revision history :

	= 03/04/10, Dave Morano

	This was expanded out from the hacks in 'sim-test'.



*/


/******************************************************************************

	This is functional simulator support.



******************************************************************************/


#include	<sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include	<bio.h>
#include	<mallocstuff.h>

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

#include	"paramfile.h"
#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#include	"ss.h"
#include	"ssclocking.h"
#include	"ssinfo.h"
#include	"ssmachstate.h"
#include	"ssdecode.h"
#include	"ssiw.h"
#include	"ssas.h"
#include	"machstate.h"



/* local defines */



/* external subroutines */

extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecui(const char *,int,int *) ;
extern int	cfdecull(const char *,int,ULONG *) ;


/* external variables */

extern struct proginfo	pi ;


/* local structures */


/* forward references */

extern int	setregval(struct regs_t *,int,ULONG) ;
extern int	getregval(struct regs_t *,int,ULONG *) ;

extern int	sim_exec(struct ss *,uint) ;
extern int	sim_retire(struct ss *,uint) ;


/* local variables */





int ss_expand(mip,n)
SS	*mip ;
int	n ;
{
	int	rs ;


	rs = sim_exec(mip,n) ;

	return rs ;
}
/* end subroutine (ss_expand) */



