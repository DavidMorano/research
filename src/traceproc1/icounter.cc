/* icount */

/* count instructions in the trace */


#define	CF_DEBUG	1		/* run-time debugging */


/* revision history:

	= 2001-09-01, David Morano

	This subroutine was originally written.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine performs the function 'icount' for the
	TRACEPROC program when run in 'icount' mode.


*******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>

#include	<usystem.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"

#include	"exectrace.h"
#include	"lmapprog.h"
#include	"mipsdis.h"
#include	"lmipsregs.h"

#include	"tracedata.h"


/* local defines */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */


/* local (module-scope static) data */


/* exported subroutines */


int icounter(pip,pmp,dp,tfname)
struct proginfo	*pip ;
LMAPPROG	*pmp ;
MIPSDIS		*dp ;
char		tfname[] ;
{
	EXECTRACE		t ;
	EXECTRACE_ENTRY		e ;

	ULONG	trecs = 0 ;
	ULONG	in = 0 ;

	int	rs ;


	rs = exectrace_open(&t,tfname,"r",0666,NULL) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("icount: exectrace_open() rs=%d\n",rs) ;
#endif

	if (rs < 0) {

	    bprintf(pip->efp,
	        "%s: could not open trace file=%s (%d)\n",
	        pip->progname,tfname,rs) ;

	    bprintf(pip->ofp,
	        "could not open trace file=%s (%d)\n",
	        tfname,rs) ;

	    goto bad0 ;
	}


/* loop reading them */

	while (TRUE) {

	    rs = exectrace_read(&t,&e) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("icount: exectrace_read() rs=%d\n",rs) ;
#endif

	    if (rs <= 0)
	        break ;

	    trecs += 1 ;
	    if (e.f.ia)
		in += 1 ;

	} /* end while (looping through the trace) */


	if (pip->verboselevel > 0) {

	    rs = bprintf(pip->ofp,
	        "%lld records processed\n",
	        trecs) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("icount: STDOUT bprintf() rs=%d\n",rs) ;
#endif

	} /* end if (verbose) */

	    rs = bprintf(pip->ofp,
	        "%lld instructions processed\n",
	        in) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("icount: STDOUT bprintf() rs=%d\n",rs) ;
#endif



bad1:
	exectrace_close(&t) ;

bad0:

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("icount: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (icounter) */


