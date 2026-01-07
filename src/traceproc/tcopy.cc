/* tcopy */

/* copy a source trace to another */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_DEBUG	1		/* run-time debugging */


/* revision history:

	= 2001-09-01, David Morano

	This subroutine was originally written.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine copies a trace.  It can also apply a filtering
	process during the copy so that instructions belonging to
	specified subroutines can be eliminated from the new trace
	(created by the copy).


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

#include	"tracedata.h"
#include	"filtercalls.h"
#include	"proginfo.h"


/* local defines */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

static int	getmem(uint,EXECTRACE_ENTRY *) ;
static int	copyrecord(struct proginfo *,EXECTRACE *,EXECTRACE_ENTRY *) ;


/* local (module-scope static) data */


/* exported subroutines */


int tcopy(pip,pmp,dp,t1fname,t2fname)
struct proginfo	*pip ;
LMAPPROG	*pmp ;
MIPSDIS		*dp ;
char		*t1fname, *t2fname ;
{
	EXECTRACE	t1, t2 ;
	EXECTRACE_INFO	*eip ;
	EXECTRACE_ENTRY	e1, e2 ;

	TRACEDATA	ti ;

	ULONG	trecs = 0 ;
	ULONG	in_src = 0, in_copied = 0 ;
	ULONG	in_filterstart ;
	ULONG	nfiltered = 0 ;

	uint	ma ;
	uint	ia_last, ia_search ;

	int	rs ;
	int	rs1, rs2 ;
	int	i, j ;
	int	f_starting ;
	int	f_filtering = FALSE ;
	int	f_complained = FALSE ;
	int	f_ia ;
	int	f_dump ;


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("tcopy: f_filter=%d\n",
	        pip->f.filter) ;
#endif


	tracedata_init(&ti) ;

/* open the traces */

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("tcopy: t1=%s t2=%s\n",
	        t1fname,t2fname) ;
#endif

	rs = exectrace_open(&t1,t1fname,"r",0666,NULL) ;

	if (rs < 0) {

#if	CF_DEBUG
	    if (pip->debuglevel >= 1)
	        debugprintf("tcopy: T1 exectrace_open() rs=%d\n",rs) ;
#endif

	    bprintf(pip->efp,
	        "%s: could not open trace file=%s (%d)\n",
	        pip->progname,t1fname,rs) ;

	    bprintf(pip->ofp,
	        "could not open trace file=%s\n",
	        t1fname) ;

	    goto bad0 ;
	}

	rs = exectrace_open(&t2,t2fname,"w",0666,NULL) ;

	if (rs < 0) {

#if	CF_DEBUG
	    if (pip->debuglevel >= 1)
	        debugprintf("tcopy: T2 exectrace_open() rs=%d\n",rs) ;
#endif

	    bprintf(pip->efp,
	        "%s: could not open trace file=%s (%d)\n",
	        pip->progname,t2fname,rs) ;

	    bprintf(pip->ofp,
	        "could not open trace file=%s\n",
	        t2fname) ;

	    goto bad1 ;
	}


/* fast forward to where we should start the copy from */

	ia_last = 0 ;

	f_starting = TRUE ;
	f_dump = FALSE ;
	while (TRUE) {

	    rs = exectrace_read(&t1,&e1) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("tcopy: exectrace_read() rs=%d\n",rs) ;
#endif

	    if (rs <= 0)
	        break ;

	    trecs += 1 ;
	    tracedata_proc(&ti,&e1) ;

	    if (f_ia = e1.f.ia) {

#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	            debugprintf("tcopy: trecs=%lld in=%lld ia=%08x\n",
			trecs,ti.in,e1.ia) ;
#endif

	        ia_last = e1.ia ;
	        f_dump = (proginfo_dump(pip,ti.in) > 0) ;

	    } /* end if (was an instruction) */

/* are we done ? */

	    if (f_dump || pip->f.past)
	        break ;

/* write back any registers */

	    if (e1.f.reg) {

	        int	n ;


	        n = tracedata_recregs(&ti,&e1) ;

	        if (n > 0)
	            tracedata_writeback(&ti,&e1) ;

	    }

/* accummulate all memory writes */

/* NOT IMPLEMENTED */


/* bottom processing */

	    if (e1.f.ia) {

	        ti.in += 1 ;
		in_src += 1 ;

		if (ti.in >= pip->ninstr)
			break ;

	    } /* end if (had instruction address) */

	} /* end while (fast forwarding) */

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("tcopy: done w/ fast forward\n") ;
#endif


/* write out all registers (that we have gotten) */

	if (ti.f.gotregs) {

	    for (i = 0 ; i < TRACEDATA_NREGS ; i += 1) {

	        if (ti.gotreg[i])
	            exectrace_wreg(&t2,i,ti.reg[i],15) ;

	    } /* end for */

	} /* end if (we have some registers) */

/* write out all accumulated memory writes */

	if (pip->f.opt_memvals) {

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("tcopy: writing out accumulated memory\n") ;
#endif

/* NOT IMPLEMENTED (hard to do because of tracking) */


	} /* end if */


/* write out an instruction-number record */

	exectrace_win(&t2,ti.in) ;


/* read the source trace and just copy it to the destination */

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("tcopy: copy to destination\n") ;
#endif

	f_dump = TRUE ;
	while (! pip->f.past) {

	    if (! f_starting) {

	        rs = exectrace_read(&t1,&e1) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	            debugprintf("tcopy: exectrace_read() rs=%d\n",rs) ;
#endif

	        if (rs <= 0)
	            break ;

	        trecs += 1 ;
	        tracedata_proc(&ti,&e1) ;

	        if (f_ia = e1.f.ia) {

#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	                debugprintf("tcopy: trecs=%lld ia=%08x\n",
				trecs,e1.ia) ;
#endif

	            f_dump = (proginfo_dump(pip,ti.in) > 0) ;

	        }

	        if ((! f_dump) || pip->f.past)
	            break ;

	    } else
	        f_starting = FALSE ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("tcopy: in=%lld\n",ti.in) ;
#endif

/* check for a filter call */

	    if (pip->f.filter && (! f_filtering)) {

	        const char	*np ;


	        rs = filtercalls_have(pip->callfilter,e1.ia,&np) ;

	        f_filtering = FALSE ;
	        if (rs > 0) {

	            f_filtering = TRUE ;
			in_filterstart = ti.in ;
	            ia_search = ia_last + 4 ;
	            nfiltered += 1 ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	            debugprintf("tcopy: in=%lld filter call=%s\n",ti.in,np) ;
#endif

	        } /* end if (filtering) */

#if	CF_DEBUG
	        if ((pip->debuglevel >= 5) && f_filtering)
	            debugprintf("tcopy: filter call=%s\n",np) ;
#endif

	    } /* end if (filtering) */


	    if (f_filtering && f_ia && 
		((e1.ia == ia_search) || (e1.ia == (ia_search + 4)))) {

	        f_filtering = FALSE ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	            debugprintf("tcopy: in=%lld no longer filtering\n",ti.in) ;
#endif

	    }

		if ((pip->debuglevel >= 1) &&
			f_filtering && (ti.in > (in_filterstart + 100000))) {

			if (! f_complained) {

				f_complained = TRUE ;
	    bprintf(pip->efp,
	        "%s: more than 100k instruction in a SYSCALL ??? in=%lld\n",
			pip->progname,in_filterstart) ;

			}
		}

	    if (! f_filtering)
	        copyrecord(pip,&t2,&e1) ;


	    if (f_ia) {

	        ia_last = e1.ia ;
	        ti.in += 1 ;
		in_src += 1 ;
		in_copied += 1 ;

		if ((pip->ninstr > 0) && (ti.in >= pip->ninstr))
			break ;

	    } /* end if (had an instruction address) */

	} /* end while (looping through the trace) */

#ifdef	COMMENT
	    if (f_ia)
	        ti.in += 1 ;
#endif

	tracedata_free(&ti) ;

	if (pip->verboselevel > 0)
	    bprintf(pip->ofp,
	        "%lld records processed\n",
	        trecs) ;

	    bprintf(pip->ofp,
	        "%lld source instructions\n",
	        in_src) ;

	    bprintf(pip->ofp,
	        "%lld instructions copied\n",
	        in_copied) ;

	if (nfiltered > 0)
	    bprintf(pip->ofp,
	        "%lld calls filtered out\n",
	        nfiltered) ;

/* we're out of here */
bad1:
	exectrace_close(&t1) ;

	exectrace_close(&t2) ;

bad0:

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("tcopy: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (tcopy) */



/* LOCAL SUBROUTINES */



static int copyrecord(pip,tp,ep)
struct proginfo	*pip ;
EXECTRACE	*tp ;
EXECTRACE_ENTRY	*ep ;
{
	int	i ;
	int	f ;


/* mandatory (?) stuff */

	if (ep->f.clock) {

	    exectrace_wclock(tp,ep->clock) ;

	}

	f = FALSE ;
	if (ep->f.ia) {

	    f = TRUE ;
	    exectrace_wia(tp,ep->ia) ;

	} /* end if (instruction number) */

	if (ep->f.sc) {

	    f = TRUE ;
	    exectrace_wsyscall(tp,ep->sc) ;

	}

	if (ep->f.som) {

	    f = TRUE ;
	    exectrace_wsom(tp,ep->som) ;

	}

/* user-specified optional (?) stuff */

	if (pip->f.opt_in && ep->f.in) {

	    exectrace_win(tp,ep->in) ;

	} /* end if (instruction number) */

/* source registers ? */

	if (pip->f.opt_rsa || pip->f.opt_rsv) {

	    EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("tcopy: sreg=%d\n",ep->f.sreg) ;
#endif

	    for (i = 0 ; i < ep->f.sreg ; i += 1) {

	        uint	rv ;


	        op = ep->sreg + i ;
	        if (pip->f.opt_rsv && op->dp)
	            exectrace_wrsv(tp,op->a,op->dv) ;

	        else
	            exectrace_wrsa(tp,op->a) ;

	    } /* end for */

	} /* end if (source register information) */

/* source memory addresses */

	if (pip->f.opt_msa || pip->f.opt_msv) {

	    EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("tcopy: smem=%d\n",ep->f.smem) ;
#endif

	    for (i = 0 ; i < ep->f.smem ; i += 1) {

	        op = ep->smem + i ;
	        if (pip->f.opt_msv && op->dp)
	            exectrace_wmsv(tp,op->a,op->dv) ;

	        else
	            exectrace_wmsa(tp,op->a) ;

	    } /* end for */

	} /* end if (source memory information) */

/* what about some registers */

	if (pip->f.opt_regs || pip->f.opt_regvals) {

	    EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("tcopy: reg=%d\n",ep->f.reg) ;
#endif

	    for (i = 0 ; i < ep->f.reg ; i += 1) {

	        op = ep->reg + i ;
	        if (pip->f.opt_regvals && op->dp)
	            exectrace_wreg(tp,op->a,op->dv,op->dp) ;

	        else
	            exectrace_wreg(tp,op->a,op->dv,0) ;

	    } /* end for */

	} /* end if (register information) */

/* what about memory */

	if (pip->f.opt_mems || pip->f.opt_memvals) {

	    EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("tcopy: mem=%d\n",ep->f.mem) ;
#endif

	    for (i = 0 ; i < ep->f.mem ; i += 1) {

	        op = ep->mem + i ;
	        if (pip->f.opt_memvals && op->dp)
	            exectrace_wmem(tp,op->a,op->dv,op->dp) ;

	        else
	            exectrace_wmem(tp,op->a,op->dv,0) ;

	    } /* end for */

	} /* end if (memory information) */

	return f ;
}
/* end subroutine (copyrecord) */


