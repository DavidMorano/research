/* fcounter */

/* count instructions in the trace */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_DEBUG	1		/* run-time debugging */


/* revision history:

	= 2001-09-01, David Morano
	This subroutine was originally written.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine performs the function 'fcount' for the
	TRACEPROC program when run in 'fcount' mode.

*******************************************************************************/

#include	<envstandards.h>
#include	<sys/types.h>
#include	<vsystem.h>
#include	<mkpathx.h>
#include	<strwcpy.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"

#include	"exectrace.h"
#include	"lmapprog.h"
#include	"fcount.h"
#include	"mipsdis.h"
#include	"lmipsregs.h"

#include	"tracedata.h"


/* local defines */

#define	FE_SGINM	".sginm"
#define	FE_FCOUNTS	".etfcounts"


/* external subroutines */

extern int	sncpy2(char *,int,cchar *,cchar *) ;


/* external variables */


/* local structures */


/* forward references */


/* local (module-scope static) data */


/* exported subroutines */


int fcounter(pip,pmp,dp,tfname)
struct proginfo	*pip ;
LMAPPROG	*pmp ;
MIPSDIS		*dp ;
char		tfname[] ;
{
	EXECTRACE		t ;
	EXECTRACE_ENTRY		e ;

	FCOUNT			funcs ;

	ULONG	trecs = 0 ;
	ULONG	in = 0 ;

	int	rs, i ;

	char	tmpfname[MAXPATHLEN + 1] ;


#if	CF_DEBUG
	if (pip->debuglevel >= 2)
	    debugprintf("fcounter: tfname=%s\n",tfname) ;
#endif

	rs = exectrace_open(&t,tfname,"r",0666,NULL) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("fcounter: exectrace_open() rs=%d\n",rs) ;
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


/* initialize the function counter */

	if (pip->f.progmap) {

	        sncpy2(tmpfname,MAXPATHLEN,pip->basename,FE_SGINM) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 4)
	debugprintf("fcounter: fname=%s\n",tmpfname) ;
#endif

		rs = fcount_init(&funcs,pmp,tmpfname) ;

		if (rs < 0)
			goto bad1 ;

	}

/* loop reading them */

	while (TRUE) {

	    rs = exectrace_read(&t,&e) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("fcounter: in=%lld exectrace_read() rs=%d\n",in,rs) ;
#endif

	    if (rs <= 0)
	        break ;

	    trecs += 1 ;
	    if (e.f.ia) {

		in += 1 ;

	if (pip->f.progmap)
		fcount_update(&funcs,e.ia,0) ;

	} /* end if (instruction) */

		if ((pip->ninstr > 0) && (in >= pip->ninstr))
			break ;

	} /* end while (looping through the trace) */


/* write out the function count file */

	if (pip->f.progmap) {

	        bfile	tmpfile ;


	        fcount_done(&funcs) ;

	        sncpy2(tmpfname,MAXPATHLEN,pip->jobname,FE_FCOUNTS) ;

	        if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	            FCOUNT_ENTRY	*ep ;

	            double	fn, fd, fpercent ;

			uint	ins, calls ;


	                bprintf(&tmpfile,"%12lld %7.3f%% %08x %08x %s\n",
	                    in,100.0,0,0,"*total*") ;
	                
			fcount_getother(&funcs,&ins,&calls) ;

	                fn = (double) ins ;
	                fd = (double) in ;
	                fpercent = (in != 0) ? (fn * 100.0 / fd) : 0.0 ;
	                bprintf(&tmpfile,"%12ld %7.3f%% %08x %08x %s\n",
	                    ins,fpercent,0,0,"*other*") ;

	            for (i = 0 ; fcount_get(&funcs,i,&ep) >= 0 ; i += 1) {

#if	F_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(4)) {
	                    debugprintf("stats: i=%d ep=%p\n",i,ep) ;
	                    if (ep != NULL)
	                        debugprintf("stats: ia=%08x ins=%u n=%s\n",
	                            ep->ia,ep->ins,ep->name) ;
	                }
#endif /* CF_DEBUG */

	                if (ep == NULL)
	                    continue ;

	                fn = (double) ep->ins ;
	                fd = (double) in ;
	                fpercent = (in != 0) ? (fn * 100.0 / fd) : 0.0 ;
			bprintf(&tmpfile,"%12ld %7.3f%% %08x %08x %s\n",
	                    ep->ins,fpercent,ep->ia,ep->size,ep->name) ;

	            } /* end for */

	            bclose(&tmpfile) ;

	        } /* end if (function instruction counts file) */

	    } /* end if (function instruction counts) */


	if (pip->verboselevel > 0) {

	    rs = bprintf(pip->ofp,
	        "%lld records processed\n",
	        trecs) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("fcounter: STDOUT bprintf() rs=%d\n",rs) ;
#endif

	    rs = bprintf(pip->ofp,
	        "%lld instructions processed\n",
	        in) ;

	} /* end if (verbose) */

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("fcounter: STDOUT bprintf() rs=%d\n",rs) ;
#endif

bad2:
	if (pip->f.progmap) {
	    fcount_free(&funcs) ;
	}

bad1:
	exectrace_close(&t) ;

bad0:

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("fcounter: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (fcounter) */


