/* analyze */

/* analyze two traces */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_DEBUG	0		/* run-time debugging */


/* revision history:

	= 2001-09-01, David Morano

	This subroutine was originally written.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine is really the heart of the whole tool suite.
	This subroutine is used to analyze two traces and to compare
	them (intelligently) to find discrepencies.  The first trace is
	supposed to be the reference trace and the second one is the
	one under investigation.


*******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>

#include	<vsystem.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"
#include	"exectrace.h"
#include	"lmapprog.h"
#include	"mipsdis.h"
#include	"tracedata.h"


/* local defines */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

static int	getreg(uint,EXECTRACE_ENTRY *) ;
static int	getmem(uint,EXECTRACE_ENTRY *) ;


/* local (module-scope static) data */


/* exported subroutines */


int analyze(pip,pmp,dp,t1fname,t2fname)
struct proginfo	*pip ;
LMAPPROG	*pmp ;
MIPSDIS		*dp ;
char		*t1fname, *t2fname ;
{
	EXECTRACE		t1, t2 ;

	EXECTRACE_ENTRY		e1, e2 ;

	struct tracedata	i1, i2 ;

	ULONG	t1recs = 0, t2recs = 0 ;
	ULONG	in_src = 0 ;
	ULONG	in ;

	Elf32_Sym	*sp ;

	uint	ma ;
	uint	a_errno ;

	int	rs ;
	int	rs1, rs2 ;
	int	i, j, n ;
	int	f_mismatch ;


#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("analyze: f_pixie=%d f_noerrno=%d f_onemem=%d\n",
	        pip->f.opt_pixie,pip->f.opt_noerrno,pip->f.opt_onemem) ;
#endif

	if (pip->f.opt_noerrno) {

	    rs = lmapprog_getsym(pmp,"errno",&sp) ;

	    a_errno = 0 ;
	    if (rs >= 0)
	        a_errno = sp->st_value ;

	} /* end if (getting address of 'errno') */


	tracedata_init(&i1) ;

	tracedata_init(&i2) ;

/* open the traces */

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("analyze: t1=%s t2=%s\n",
	        t1fname,t2fname) ;
#endif

	rs = exectrace_open(&t1,t1fname,"r",0666,NULL) ;

	if (rs < 0) {

#if	CF_DEBUG
	    if (pip->debuglevel >= 1)
	        debugprintf("analyze: T1 exectrace_open() rs=%d\n",rs) ;
#endif

	    bprintf(pip->efp,
	        "%s: could not open trace file=%s (%d)\n",
	        pip->progname,t1fname,rs) ;

	    bprintf(pip->ofp,
	        "could not open trace file=%s\n",
	        t1fname) ;

	    goto bad0 ;
	}

	rs = exectrace_open(&t2,t2fname,"r",0666,NULL) ;

	if (rs < 0) {

#if	CF_DEBUG
	    if (pip->debuglevel >= 1)
	        debugprintf("analyze: T2 exectrace_open() rs=%d\n",rs) ;
#endif

	    bprintf(pip->efp,
	        "%s: could not open trace file=%s (%d)\n",
	        pip->progname,t2fname,rs) ;

	    bprintf(pip->ofp,
	        "could not open trace file=%s\n",
	        t2fname) ;

	    goto bad1 ;
	}


/* loop reading them */

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("analyze: looping\n") ;
#endif
	in = 0 ;

	f_mismatch = FALSE ;
	while (! f_mismatch) {

	    rs1 = exectrace_read(&t1,&e1) ;

	    rs2 = exectrace_read(&t2,&e2) ;

	    if ((rs1 <= 0) || (rs2 <= 0))
	        break ;

	    t1recs += 1 ;
	    t2recs += 1 ;

/* process a possible dump on trace 1 */

	    tracedata_proc(&i1,&e1) ;

	    tracedata_proc(&i2,&e2) ;


	    while (! e1.f.ia) {

	        tracedata_writeback(&i1,&e1) ;

/* continue with trace 1 */

	        rs1 = exectrace_read(&t1,&e1) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("analyze: rs1=%d\n",rs1) ;
#endif

	        if (rs1 <= 0)
	            break ;

	        t1recs += 1 ;

/* process a possible dump on trace 1 */

	        rs = tracedata_proc(&i1,&e1) ;

	    } /* end while */

	    while (! e2.f.ia) {

	        tracedata_writeback(&i2,&e2) ;

/* continue with trace 1 */

	        rs2 = exectrace_read(&t2,&e2) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("analyze: rs2=%d\n",rs2) ;
#endif

	        if (rs2 <= 0)
	            break ;

	        t2recs += 1 ;

/* process a possible dump on trace 1 */

	        rs = tracedata_proc(&i2,&e2) ;

	    } /* end while */


	    if ((rs1 <= 0) || (rs2 <= 0))
	        break ;


#if	CF_DEBUG
	    if (pip->debuglevel > 2)
	        debugprintf("analyze: in=%lld ia=%08x\n",
	            i2.in,e2.ia) ;
#endif

/* compare instruction addresses */

	    if (e2.ia != e1.ia) {

	        bprintf(pip->ofp,
	            "instruction address mismatch "
			"in=%lld t1ia=%08x t2ia=%08x\n",
	            i2.in,e1.ia,e2.ia) ;

	        f_mismatch = TRUE ;
	        break ;
	    }


/* compare register (if any and values) in trace 2 with any from trace 1 */

	    if (e2.f.reg) {

	        for (i = 0 ; i < e2.f.reg ; i += 1) {

	            j = getreg(e2.reg[i].a,&e1) ;

	            if (j >= 0) {

#if	CF_DEBUG
	                if (pip->debuglevel > 2)
	                    debugprintf("analyze: both traces had register %d\n",
	                        e2.reg[i].a) ;
#endif

	                if (e1.reg[i].dp) {

	                    if (e1.reg[j].dv != e2.reg[i].dv) {

	                        f_mismatch = TRUE ;
	                        bprintf(pip->ofp,
	                            "in=%lld register %d value "
	                            "mistmatch e1=%08x e2=%08x\n",
	                            i2.in, e2.reg[i].a, 
	                            e1.reg[j].dv, e2.reg[i].dv) ;

	                    } /* end if (datamistmatch) */

	                } /* end if (data present) */

	            } else if (pip->f.opt_regs) {

	                f_mismatch = TRUE ;
	                bprintf(pip->ofp,
	                    "in=%lld register %d was not in the source trace\n",
	                    i2.in,e2.reg[i].a) ;

	            } /* end if (register addresses) */

	        } /* end for (looping through registers) */

	    } /* end if (had some registers in trace 2) */


/* compare memory (if any) and values in trace 2 with any from trace 1 */

	    if (e2.f.mem) {

#if	CF_DEBUG
	        if (pip->debuglevel > 2)
	            debugprintf("analyze: t2 has a memory\n") ;
#endif

	        n = (pip->f.opt_onemem) ? 1 : e2.f.mem ;
	        for (i = 0 ; i < n ; i += 1) {

#if	CF_DEBUG
	            if (pip->debuglevel > 2)
	                debugprintf("analyze: MA=%08x\n",
	                    e2.mem[i].a) ;
#endif

	            ma = e2.mem[i].a ;
	            if (pip->f.opt_noerrno && (ma == a_errno))
	                continue ;

	            if (pip->f.opt_pixie)
	                ma &= 0x0FFFFFF ;

	            j = getmem(ma,&e1) ;

	            if (j >= 0) {

#if	CF_DEBUG
	                if (pip->debuglevel > 2)
	                    debugprintf("analyze: both traces had memory %08x\n",
	                        e2.mem[i].a) ;
#endif

	                if (e1.mem[i].dp) {

	                    if (e1.mem[j].dv != e2.mem[i].dv) {

	                        f_mismatch = TRUE ;
	                        bprintf(pip->ofp,
	                            "in=%lld memory %08x value "
	                            "mistmatch e1=%08x e2=%08x\n",
	                            i2.in, e2.mem[i].a, 
	                            e1.mem[j].dv, e2.mem[i].dv) ;

	                    } /* end if (datamistmatch) */

	                } /* end if (data present) */

	            } else if (pip->f.opt_mems) {

	                f_mismatch = TRUE ;
	                bprintf(pip->ofp,
	                    "in=%lld memory %08x was not in source trace\n",
	                    i2.in,e2.mem[i].a) ;

	            } /* end if (memory addresses) */

	        } /* end for (looping through memory writes) */

	    } /* end if (had some memory writes in trace 2) */


	    tracedata_writeback(&i1,&e1) ;

	    tracedata_writeback(&i2,&e2) ;

	    if ((pip->in.n > 0) && (i2.in >= pip->in.end))
	        break ;

	    if (e1.f.ia) {

	        i1.in += 1 ;
		in_src += 1 ;
	    }

	    if (e2.f.ia)
	        i2.in += 1 ;

		if ((pip->ninstr > 0) && (in >= pip->ninstr))
			break ;

	} /* end while (looping through the target trace) */

	if ((pip->in.n > 0) && (i2.in >= pip->in.end)) {

	    if (e1.f.ia) {

	        i1.in += 1 ;
		in_src += 1 ;
	    }

	    if (e2.f.ia)
	        i2.in += 1 ;

	} /* end if (no mismatch) */

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("analyze: rs1=%d rs2=%d\n",rs1,rs2) ;
#endif

	if ((rs1 == 0) && (rs2 > 0)) {

	    bprintf(pip->ofp,
	        "trace1 was short\n") ;

	} else if ((rs2 == 0) && (rs1 > 0)) {

	    bprintf(pip->ofp,
	        "trace2 was short\n") ;

	}


#if	CF_DEBUG
	if ((pip->debuglevel >= 1) && f_mismatch)
	    debugprintf("analyze: mismatch\n") ;
#endif

	if (f_mismatch)
	    bprintf(pip->ofp,
	        "mismatch\n") ;

	if (pip->verboselevel > 0)
	    bprintf(pip->ofp,
	        "%lld records processed\n",
	        t1recs) ;

	bprintf(pip->ofp,
	    "%lld instructions processed\n",
	    in_src) ;


bad2:
	exectrace_close(&t2) ;

bad1:
	exectrace_close(&t1) ;

bad0:

#if	CF_DEBUG
	if (pip->debuglevel > 2)
	    debugprintf("analyze: exiting rs=%d\n",
	        rs) ;
#endif

	return (rs >= 0) ? f_mismatch : rs ;
}
/* end subroutine (analyze) */



/* LOCAL SUBROUTINES */



static int getreg(ra,ep)
uint		ra ;
EXECTRACE_ENTRY	*ep ;
{
	int	i ;


	if (ep->f.reg == 0)
	    return -1 ;

	for (i = 0 ; i < ep->f.reg ; i += 1) {

	    if (ep->reg[i].a == ra)
	        break ;

	} /* end for (looking for register address) */

	return (i < ep->f.reg) ? i : -1 ;
}
/* end subroutine (getreg) */


static int getmem(ma,ep)
uint		ma ;
EXECTRACE_ENTRY	*ep ;
{
	int	i ;


	if (ep->f.mem == 0)
	    return -1 ;

	for (i = 0 ; i < ep->f.mem ; i += 1) {

	    if (ep->mem[i].a == ma)
	        break ;

	} /* end for (looking for register address) */

	return (i < ep->f.mem) ? i : -1 ;
}
/* end subroutine (getmem) */



