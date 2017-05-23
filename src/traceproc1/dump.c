/* dump */

/* dump a trace */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_DEBUG	1		/* run-time debugging */


/* revision history:

	= 2001-09-01, David Morano

	This subroutine was originally written.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine reads the input trace and writes a text
	representation of the trace data to the output.  Only elements
	that have been specified at invocation to be output will be
	processed and output.


*******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>

#include	<vsystem.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"

#include	"exectrace.h"
#include	"lmapprog.h"
#include	"mipsdis.h"
#include	"lmipsregs.h"

#include	"tracedata.h"
#include	"proginfo.h"


/* local defines */

#define	DISBUFLEN	100


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

static int	memwriteback(struct proginfo *,LMAPPROG *,EXECTRACE_ENTRY *) ;


/* local (module-scope static) data */


/* exported subroutines */


int dump(pip,pmp,dp,tfname)
struct proginfo	*pip ;
LMAPPROG	*pmp ;
MIPSDIS		*dp ;
char		tfname[] ;
{
	EXECTRACE		t ;
	EXECTRACE_ENTRY		e ;

	TRACEDATA	ti ;

	ULONG	trecs = 0 ;
	ULONG	in ;

	uint	ia_last = 0 ;

	int	rs, i, j ;
	int	rs1, rs2 ;
	int	f_dump ;
	int	f_switch ;

	char	disbuf[DISBUFLEN + 1] ;


/* open the traces */

#if	CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("dump: tracefile=%s f_instr=%d f_instrdis=%d\n",
	        tfname,pip->f.opt_instr,pip->f.opt_instrdis) ;
#endif

	rs = exectrace_open(&t,tfname,"r",0666,NULL) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("dump: exectrace_open() rs=%d\n",rs) ;
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

	tracedata_init(&ti) ;

	in = 0 ;

	f_dump = FALSE ;
	f_switch = FALSE ;
	while (TRUE) {

	    uint	instr ;

	    int		f_ia ;
	    int		f_needtab ;


	    rs = exectrace_read(&t,&e) ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 4) {
	        debugprintf("dump: exectrace_read() rs=%d\n",rs) ;
	        debugprintf("dump: f_ia=%d\n",e.f.ia) ;
	    }
#endif

	    if (rs <= 0)
	        break ;

	    trecs += 1 ;
	    tracedata_proc(&ti,&e) ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("dump: trecs=%lld f_ia=%d ti.in=%lld\n",
			trecs,e.f.ia,ti.in) ;
#endif

	    if (f_ia = e.f.ia) {

	        rs = proginfo_dump(pip,ti.in) ;

		f_dump = (rs > 0) ? rs : 0 ;
		f_switch = (rs == 2) ;
	        if (pip->f.past)
	            break ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("dump: rs=%d f_dump=%d f_switch=%d f_past=%d\n",
			rs,f_dump,f_switch,pip->f.past) ;
#endif

	    } /* end if (got an instruction address) */

#if	CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("dump: f_dump=%d \n",f_dump) ;
#endif

	    if (f_dump && 
	        (f_ia || (pip->f.opt_syscalls && (e.f.sc || e.f.som)))) {

#if	CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("dump: ia=%08x in=%lld\n",e.ia,ti.in) ;
#endif

/* do the dump */

	        if (pip->f.opt_in)
	            bprintf(pip->ofp,"%12lld ", ti.in) ;

		f_needtab = FALSE ;
	        if (f_ia) {

		    if (pip->f.opt_plusinstr) {

#if	CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("dump: plusinstr ia=%08x ia_last=%08x\n",
				e.ia,ia_last) ;
#endif

			if ((e.ia == (ia_last + 4)) && (! f_switch)) {

		    		f_needtab = TRUE ;
	                	bprintf(pip->ofp,"+") ;

			} else
	                	bprintf(pip->ofp,"%08x",e.ia) ;

		        ia_last = e.ia ;

		    } else
	                bprintf(pip->ofp,"%08x",e.ia) ;

		} else if (e.f.sc || e.f.som)
	                bprintf(pip->ofp, " ________") ;

	        if (pip->f.opt_instr && pip->f.progmap) {

		    if (f_needtab) {

			f_needtab = FALSE ;
			bputc(pip->ofp,'\t') ;

		    }

	            if (f_ia) {

	                rs = lmapprog_readint(pmp,e.ia,&instr) ;

	                if (rs >= 0)
	                    bprintf(pip->ofp, " %08x",instr) ;

	                else
	                    bprintf(pip->ofp, " ********") ;

	            } else
	                bprintf(pip->ofp, " ________") ;

	        } /* end if (writing the instruction word) */

	        if (pip->f.opt_instrdis && pip->f.instrdis) {

		    if (f_needtab) {

			f_needtab = FALSE ;
			bputc(pip->ofp,'\t') ;

		    }

	            if (f_ia) {

	                rs = mipsdis_getlevo(dp,e.ia,instr,disbuf,DISBUFLEN) ;

	                if (rs < 0)
	                    strcpy(disbuf,"unknown") ;

	                bprintf(pip->ofp, " %s",disbuf) ;

	            } else if (e.f.sc)
	                bprintf(pip->ofp, " psyscall %d",e.sc) ;

	            else if (e.f.som)
	                bprintf(pip->ofp, " som %d",e.som) ;

	            else
	                bprintf(pip->ofp, " ??") ;

	        } /* end if (writing the disasembly) */

	        bputc(pip->ofp,'\n') ;

/* source registers ? */

	        if (pip->f.opt_rsa || pip->f.opt_rsv) {

	            EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	            if (pip->debuglevel >= 4)
	                debugprintf("dump: sreg=%d\n",e.f.sreg) ;
#endif

	            for (i = 0 ; i < e.f.sreg ; i += 1) {

	                uint	rv ;


	                op = e.sreg + i ;
	                bprintf(pip->ofp,"\t%%r%-3d =>",op->a) ;

	                if (pip->f.opt_rsv) {

	                    rv = (op->dp) ? op->dv : ti.reg[op->a] ;
	                    bprintf(pip->ofp," %08x",rv) ;

	                }

	                bputc(pip->ofp,'\n') ;

	            } /* end for */

	        } /* end if (source register information) */

/* source memory addresses */

	        if (pip->f.opt_msa || pip->f.opt_msv) {

	            EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	            if ((pip->debuglevel >= 3) && (e.f.smem > 0))
	                debugprintf("dump: in=%lld smem=%d\n",
	                    ti.in,e.f.smem) ;
#endif

	            for (i = 0 ; i < e.f.smem ; i += 1) {

	                op = e.smem + i ;
	                bprintf(pip->ofp,"\t(%08x) =>",op->a) ;

#if	CF_DEBUG
	                if (pip->debuglevel >= 3)
	                    debugprintf("dump: s ma=%08x mdp=%d\n",op->a,op->dp) ;
#endif

	                if (pip->f.opt_msv) {

	                    uint	mv ;


	                    rs1 = 0 ;
	                    mv = op->dv ;
	                    if (! op->dp)
	                        rs1 = lmapprog_readint(pmp,op->a & (~ 3),&mv) ;

#if	CF_DEBUG
	                    if (pip->debuglevel >= 3)
	                        debugprintf("dump: s rs1=%d mv=%08x \n",
	                            rs1,mv) ;
#endif

	                    if (rs1 >= 0)
	                        bprintf(pip->ofp," %08x",mv) ;

	                    else
	                        bprintf(pip->ofp," ********",mv) ;

	                } /* end if (memory source values requested) */

	                bputc(pip->ofp,'\n') ;

	            } /* end for */

	        } /* end if (source memory information) */

/* what about some registers */

	        if (pip->f.opt_regs || pip->f.opt_regvals) {

	            EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	            if (pip->debuglevel >= 4)
	                debugprintf("dump: reg=%d\n",e.f.reg) ;
#endif

	            for (i = 0 ; i < e.f.reg ; i += 1) {

	                op = e.reg + i ;
	                bprintf(pip->ofp,"\t%%r%-3d <=",op->a) ;

	                if (pip->f.opt_regvals && op->dp)
	                    bprintf(pip->ofp," %08x",op->dv) ;

	                bputc(pip->ofp,'\n') ;

	            } /* end for */

	        } /* end if (register information) */

/* what about memory */

	        if (pip->f.opt_mems || pip->f.opt_memvals) {

	            EXECTRACE_OPERAND	*op ;


#if	CF_DEBUG
	            if (pip->debuglevel >= 4)
	                debugprintf("dump: mem=%d\n",e.f.mem) ;
#endif

	            for (i = 0 ; i < e.f.mem ; i += 1) {

	                op = e.mem + i ;

/* previous values ? */

	                if (pip->f.opt_mpv) {

	                    uint	pdv ;


	                    rs1 = lmapprog_readint(pmp,(op->a & (~ 3)),&pdv) ;

	                    if (rs1 >= 0)
	                        bprintf(pip->ofp,"\t(%08x) == %08x\n",
	                            (op->a & (~ 3)),pdv) ;

	                    else
	                        bprintf(pip->ofp,"\t(%08x) == ********\n",
	                            (op->a & (~ 3))) ;

	                } /* end if (previous memory value) */

/* the regular function */

	                bprintf(pip->ofp,"\t(%08x) <=",op->a) ;

#if	CF_DEBUG
	                if (pip->debuglevel >= 4)
	                    debugprintf("dump: ma=%08x mdp=%d\n",op->a,op->dp) ;
#endif

	                if (pip->f.opt_memvals && op->dp) {

	                    int	shift = 24 ;


	                    bputc(pip->ofp,' ') ;

	                    for (j = 0 ; j < 4 ; j += 1) {

	                        if ((op->dp >> j) & 1) {

	                            bprintf(pip->ofp,"%02x",
	                                ((op->dv >> shift) & 255)) ;

	                        } else
	                            bprintf(pip->ofp,"--") ;

	                        shift -= 8 ;

	                    } /* end for */

	                } /* end if */

	                bputc(pip->ofp,'\n') ;

	            } /* end for */

	        } /* end if (memory information) */

	    if (f_ia)
		in += 1 ;

		if ((pip->ninstr > 0) && (in >= pip->ninstr))
			break ;

	    } /* end if (we wanted to dump it) */

/* write back any registers */

	    tracedata_writeback(&ti,&e) ;

/* write back to the program address space if necessary */

	    if (pip->f.opt_msv && e.f.mem)
	        memwriteback(pip,pmp,&e) ;

	    if (f_ia)
	        ti.in += 1 ;

	} /* end while (looping through the trace) */

	tracedata_free(&ti) ;

	if (pip->verboselevel > 0) {
	    bprintf(pip->ofp,
	        "%lld records processed\n", trecs) ;
	}

bad1:
	exectrace_close(&t) ;

bad0:

#if	CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("dump: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (dump) */



/* LOCAL SUBROUTINES */



static int memwriteback(pip,pmp,ep)
struct proginfo	*pip ;
LMAPPROG	*pmp ;
EXECTRACE_ENTRY	*ep ;
{
	EXECTRACE_OPERAND	*op ;

	int	rs, i ;


	for (i = 0 ; i < ep->f.mem ; i += 1) {

	    op = ep->mem + i ;
	    rs = SR_EMPTY ;
	    if (op->dp)
	        rs = lmapprog_writeint(pmp,(op->a & (~ 3)),op->dv,op->dp) ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 3)
	        debugprintf("memwriteback: rs=%d ma=%08x mdp=%d mv=%08x\n",
	            rs,op->a,op->dp,op->dv) ;
#endif

	} /* end for */

	return ep->f.mem ;
}
/* end subroutine (memwriteback) */


