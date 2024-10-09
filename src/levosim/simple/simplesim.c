/* simplesim SUPPORT */
/* lang=C++98 */

/* SimpleSim MIPS ISA simulator */
/* version %I% last-modified %G% */

#define	F_DEBUGS	0		/* non-switchable */
#define	F_DEBUG		1		/* switchable */
#define	F_SAFE		1		/* extra safe mode ? */
#define	F_SAFE2		1		/* extra safe mode ? */
#define F_PVALUESBEF    1		/* print values before */
#define F_PVALUESAFT    1 		/* print values after */
#define F_CREATETRACE   1		/* create debugging trace data */
#define	F_NOSTOP	0		/* stop after IA mismatch */
#define	F_BREAKERROR	0		/* break out on error */

/* revision history:

	= 01/07/10, Dave Morano
	This code (the old i-fetch code) has been turned into SimpleSim !

	= 01/07/27, Marcos de Alba
	I added another field to the call of lexec (ilptr).

	= 01/08/10, Dave Morano
	I changed the code so that the ISA registers are passed to us.
	Also, the code will only execute for a maximum number of
	instructions (if specified).  I also changed some variable
	types to be consistent (or more consistent) across the
	whole program.  I also added symbolic TRUE/FALSE for
	boolean variables.

	= 01/08/17, Dave Morano
	I added the calls to write out trace information.
	I had to add the whole trace facility to LevoSim first.

	= 01/08/29, Dave Morano
	I added the calls to get some disassembled output text for the
	instrcutions being executed.

	= 01/10/07, Dave Morano
	I fixed the instructions SH, LWL, and LWR.

	= 01/10/10, Dave Morano
	I added the code for the LWC1 and SWC1 instructions.

	= 01/11/06, Dave Morano
	I added the code for the LDC1 and SDC1 instructions.

*/

/******************************************************************************

	This is the SimpleSim component of the LevoSim.
	This subroutine is called when the main LevoSim program
	is called in 'simplesim' mode.

******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<cmath>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<vsystem.h>
#include	<bio.h>
#include	<libuc.h>
#include	<mkpathx.h>
#include	<mkfnamex.h>
#include	<strwcpy.h>
#include	<paramfile.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"
#include	"debugwin.h"

#include	"lsim.h"
#include	"statemips.h"
#include	"exectrace.h"
#include	"traceinfo.h"
#include	"fcount.h"

#include	"lflowgroup.h"
#include	"lmipsregs.h"
#include	"opclass.h"
#include	"lexecop.h"
#include	"ldecodeinstrclass.h"
#include	"ldecode.h"
#include	"lexec.h"
#include	"mipsdis.h"


/* local defines */

#define	INSTRDISLEN	100		/* buffer to hold disassembly */
#define	BPSIZE		100		/* instructions in basic block */
#define	BTSIZE		10000		/* instructions in basic block */

#define	DEBUGLEVEL(n)	(pip->debuglevel >= (n))

#define	ICOUNTS		".icounts"
#define	BTLEN		".btlen"
#define	BPLEN		".bplen"
#define	BSTATS		".bstats"
#define	HTLEN		".htlen"



/* external subroutines */

#if	defined(SOLARIS) && (SOLARIS == 8)
extern double	sqrt(double) ;
#endif

extern int	fmeanvaral(ULONG *,int,double *,double *) ;

extern char	*timestr_log(time_t,char *) ;


/* local structures */

struct instrinfo {
	uint	ia ;
	uint	instr ;
	uint	opexec ;
	uint	opclass ;
	uint	ta ;
	int	f_cf : 1 ;
	uint	f_taken : 1 ;
	uint	f_nullify : 1 ;
} ;

struct stats {
	ULONG	in ;			/* number of instructions */
	ULONG	ccf ;			/* conditional control-flow-changes */
	ULONG	lexecop[LEXECOP_TOTAL] ;	/* opcodes */
	ULONG	bplen[BPSIZE] ;		/* instructions per BP */
	ULONG	btlen[BTSIZE] ;		/* branch target length */
	ULONG	htlen[BTSIZE] ;		/* SS hammock branch target length */
	ULONG	sshammock ;		/* single-sided simple hammocks */
} ;


/* forward references */

static int	loadinstr(struct proginfo *,LSIM *,int,uint,uint,
			uint *,uint *) ;
static int	storeinstr(struct proginfo *,LSIM *,int,uint,uint,uint,
			uint *,uint *,uint *,uint *) ;
static int	checkpsyscall(struct proginfo *,LSIM *,SYSCALLS *,uint,uint) ;
static int	regload(struct proginfo *,int,uint,uint,uint,uint,
			uint *,uint *) ;
static int	iscf(LDECODE *) ;
static int	writestats(struct proginfo *,
			struct stats *,struct statemips *) ;


/* local variables */

static const char	*instrclasses[] = {
	"unused",
	"brel",
	"bind",
	"load",
	"store",
	"alu",
	"fpalu",
	"special",
	"jrel",
	"jind",
	"excep",
	"unimpl",
	NULL
} ;






int simplesim(pip,pfp,mip,smp,scp,skipinstr)
struct proginfo		*pip ;
PARAMFILE		*pfp ;
LSIM			*mip ;
struct statemips	*smp ;
SYSCALLS		*scp ;
ULONG			skipinstr ;
{
	LDECODE 		di ;

	EXECTRACE		it ;

	EXECTRACE_ENTRY		e ;

	struct stats		scounts ;

	struct instrinfo	pii, cii ;

	ULONG	in_bp ;			/* branch path instruction number */
	ULONG	nlast ;			/* last instruction for us */
	ULONG	trecs = 0 ;
	ULONG	terrs = 0 ;
	ULONG	in = 0 ;

	uint	ia, instr ;
	uint	dst1, dst2, dst3 ;
	uint	ma, mv1, mv2 ;		/* memory address & value */
	uint    dp ;			/* data present bits (for memory) */
	uint	ia_lastbranch ;		/* IA of last c-branch seen */
	uint	ta_lastbranch ;		/* TA of last c-branch seen */
#ifdef	COMMENT
	uint	 ilptr ;
#endif

	int	rs = SR_OK, rs1 ;
	int	n, size ;
	int	offset ;

#ifdef	COMMENT
	int	boutcome, delay ;
	int	src1i, src2i, src3i, src4i, src5i, dst1i, dst2i, dst3i ;
	int	btarget = 0 ;
	int	f_delayslot = FALSE ;
#endif

	int	f_itrace ;
	int	f_exit = FALSE ;
	int	f_double ;
	int	f_te ;			/* trace enable */
	int	f_se ;			/* selection enable */

#ifdef	COMMENT
	int	f_jump, f_nullify ;
#endif

	char	instrdis[INSTRDISLEN + 1] ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("simplesim: entered, pip:%p mip:%p\n",pip,mip) ;
#endif

	f_itrace = FALSE ;
	if ((pip->itfname != NULL) && (pip->itfname[0] != '\0')) {

	    rs = exectrace_open(&it,pip->itfname,"r",0666,NULL) ;

	    if (rs < 0) {

	        bprintf(pip->efp,
	            "%s: could not open trace file=%s (%d)\n",
	            pip->progname,pip->itfname,rs) ;

	        bprintf(pip->ofp,
	            "could not open trace file=%s (%d)\n",
	            pip->itfname,rs) ;

	        goto bad0 ;
	    }

	    f_itrace = TRUE ;
	    if (pip->verboselevel > 0)
		bprintf(pip->ofp,"using i-fetch trace file=%s\n",
			pip->itfname) ;

	} /* end if (i-trace file) */


/* statistics */

	in_bp = 0 ;
	ia_lastbranch = 0 ;
	ta_lastbranch = 0 ;

	size = sizeof(struct stats) ;
	(void) memset(&scounts,0,size) ;


	nlast = 0 ;
	if (pip->ninstr > 0)
	    nlast = (nlast > 0) ? MIN(pip->ninstr,nlast) : pip->ninstr ;

	if (skipinstr > 0)
	    nlast = (nlast > 0) ? MIN(skipinstr,nlast) : skipinstr ;

	instrdis[0] = '\0' ;


	(void) memset(&pii,0,sizeof(struct instrinfo)) ;

	pii.opclass = OPCLASS_UNUSED ;
	pii.opexec = LEXECOP_NOOP ;


#ifdef	COMMENT
	boutcome = 0 ;
#endif

/* OK, start the looping ! */

	ia = smp->ia ;
	while (! f_exit) {

	    pip->debuglevel = debugwin_check(&pip->dw,0ULL,smp->in) ;

/* start new (current) instruction sequence */

	    (void) memset(&cii,0,sizeof(struct instrinfo)) ;


/* optional i-trace intervention */

	if (f_itrace) {

	while (TRUE) {

	    rs = exectrace_read(&it,&e) ;

#if	F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("simplesim: exectrace_read() rs=%d\n",rs) ;
#endif

	    if (rs <= 0)
	        break ;

	    trecs += 1 ;
	    if (e.f.ia) {

		in += 1 ;
		break ;
	    }

	} /* end while (looping through the trace) */

	if (rs <= 0)
		goto done ;

		if (e.ia != ia) {

#if	F_NOSTOP
			terrs += 1 ;
#else
		bprintf(pip->efp,"%s: IA mismatch e.ia=%08x ia=%08x\n",
			pip->progname,e.ia,ia) ;
		eprintf("simplesim: IA mismatch e.ia=%08x ia=%08x\n",
			e.ia,ia) ;
		rs = SR_NOANODE ;
		goto done ;
#endif /* F_NOSTOP */

		}

		ia = e.ia ;

	} /* end if (i-trace) */

/* continue as if everything was (almost) normal ! */


/* update the function counter */

	    if (pip->f.fcount)
	        fcount_update(&pip->fc,ia,0) ;


/* read the actual instruction from program memory */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("simplesim: in=%lld ia=%08x\n",smp->in,ia) ;
#endif

	    rs = lsim_readinstr(mip,ia,&instr) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5) {
	        eprintf("simplesim: lsim_readinstr() rs=%d instr=%08x\n",
	            rs,instr) ;
	    }
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("simplesim: lsim_readinstr() rs=%d "
	            "ia=%08x instr=%08x\n",
	            rs,ia,instr) ;
#endif

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5) {

	        rs1 = SR_NOTSUP ;
	        if (pip->f.instrdis)
	            rs1 = mipsdis_getlevo(&pip->dis,ia,instr,
	                instrdis,INSTRDISLEN) ;

	        if (rs1 < 0)
	            strcpy(instrdis,"unknown") ;

	        eprintf("simplesim: in=%lld ia=%08x instr=%08x %s\n",
	            smp->in,ia,instr,instrdis) ;
	    }
#endif /* F_DEBUG */

#if	F_MASTERDEBUG && F_DEBUG && F_CREATETRACE
	    if (DEBUGLEVEL(5) && (instrdis[0] == '\0')) {

	        rs1 = SR_NOTSUP ;
	        if (pip->f.instrdis)
	            rs1 = mipsdis_getlevo(&pip->dis,ia,instr,
	                instrdis,INSTRDISLEN) ;

	        if (rs1 < 0)
	            strcpy(instrdis,"unknown") ;

	        eprintf("simplesim!itrace: %08x %08x %s\n",
	            ia,instr,instrdis) ;

	    }
#endif /* F_CREATETRACE */

#ifdef	COMMENT
	    di.ilptr = ia ;
	    di.instrword = instr ;
#endif

	    rs = ldecode_decode(&di,ia,instr) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (DEBUGLEVEL(2) && (rs != 0)) {
	        eprintf("simplesim: ldecode_decode() rs=%d\n",rs) ;
	        eprintf("simplesim: in=%lld ia=%08x instr=%08x\n",
	            smp->in,ia,instr) ;
	        rs1 = mipsdis_getlevo(&pip->dis,ia,instr,
	            instrdis,INSTRDISLEN) ;
	        if (rs1 >= 0)
	            eprintf("simplesim: instr> %s\n",instrdis) ;
	    }
#endif /* F_DEBUG */

	    if (rs < 0) {

	        bprintf(pip->efp,"%s: bad decode in=%lld rs=%d\n",
	            pip->progname,smp->in,rs) ;

	    }

	    if (rs != 0)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (DEBUGLEVEL(5)) {
	        eprintf("simplesim: class=%s(%d) opexec=%d\n",
	            instrclasses[di.opclass],di.opclass,di.opexec) ;
	    }
#endif


/* write an instruction trace record */

	    if (f_te = (traceinfo_enabled(&pip->ti,smp->in) > 0)) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 5)
	            eprintf("simplesim: trace in=%lld ia=%08x\n",
	                smp->in,ia) ;
#endif

	        rs = exectrace_wia(&pip->ti.et,ia) ;

		if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(2))
	            eprintf("simplesim: exectrace_wia() rs=%d\n",
	                rs) ;
#endif

		    if (rs == SR_PIPE)
		    bprintf(pip->efp,
			"%s: EOT writing exectrace (%d)\n",
			pip->progname,rs) ;

		    else
		    bprintf(pip->efp,
			"%s: problem writing exectrace (%d)\n",
			pip->progname,rs) ;

	            return rs ;

		} /* end if (problem w/ exectrace) */

	    } /* end if (trace record) */


/* statistics */

	    rs = proginfo_selection(pip,0LL,smp->in) ;

	    if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (DEBUGLEVEL(2))
	            eprintf("simplesim: proginfo_selection() rs=%d\n",
	                rs) ;
#endif
	        return rs ;
	    }

	    f_se = (rs > 0) ;


/* check for our favorite instructions ! */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 6) {

	        if ((di.opexec == LEXECOP_CVTDW) || 
	            (di.opexec == LEXECOP_CVTSW) || 
	            (di.opexec == LEXECOP_CVTWD) || 
	            (di.opexec == LEXECOP_CVTWS) || 
	            (di.opexec == LEXECOP_CVTSD) || 
	            (di.opexec == LEXECOP_CVTDS)) {

	            eprintf("simplesim: CVT in=%lld ia=%08x instr=%08x\n",
	                smp->in,ia,instr) ;

	            if (pip->debuglevel >= 5)
	                eprintf("simplesim: CVT s1=%d s2=%d d1=%d d2=%d\n",
	                    di.src1, di.src2, di.dst1, di.dst2) ;

	        }
	    }
#endif /* F_DEBUG */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4) {

	        if (di.opclass == OPCLASS_BREL)
		eprintf("simplesim: ta=%08x\n",di.ta) ;

	}
#endif /* F_DEBUGS


/* statistics update */

	    {
	        int	oi ;


	        oi = MIN(di.opexec,(LEXECOP_TOTAL - 1)) ;
	        scounts.lexecop[oi] += 1 ;

	    } /* end block (statistics update) */


/* prepare some operand stuff */

#ifdef	COMMENT
	    src1i = src2i = src3i = src4i = src5i = 0 ;
	    dst1i = dst2i = dst3i = 0 ;
#endif

/* determine if this instruction is a load/store or something else */

	    cii.ia = ia ;
		cii.instr = instr ;
	    cii.opclass = di.opclass ;
	    cii.opexec = di.opexec ;

#ifdef	COMMENT
	    cii.f_cf = iscf(&di) ;
#else
	    cii.f_cf = (di.opclass == OPCLASS_BREL) || 
	        (di.opclass == OPCLASS_JREL) ||
	        (di.opclass == OPCLASS_JIND) ;

#endif


#ifdef	COMMENT
/* hack for 'lexec()' */

	    ilptr = ia ;
	    f_jump = f_nullify = FALSE ;
#endif

/* do the instruction execution */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4) {

	        if (pii.f_nullify)
	            eprintf("simplesim: this instructions was nullified\n") ;

	    }
#endif

	    if (! pii.f_nullify) {

	            if (f_te) {

	                if (traceinfo_rsv(&pip->ti)) {

	                    if (di.src1 >= 0)
	                    exectrace_wrsv(&pip->ti.et,di.src1,
	                        smp->regs[di.src1]) ;

	                    if (di.src2 >= 0)
	                        exectrace_wrsv(&pip->ti.et,di.src2,
	                            smp->regs[di.src2]) ;

	                    if (di.src3 >= 0)
	                        exectrace_wrsv(&pip->ti.et,di.src3,
	                            smp->regs[di.src3]) ;

	                    if (di.src4 >= 0)
	                        exectrace_wrsv(&pip->ti.et,di.src4,
	                            smp->regs[di.src4]) ;

	                    if (di.src5 >= 0)
	                        exectrace_wrsv(&pip->ti.et,di.src5,
	                            smp->regs[di.src5]) ;

	                } else if (traceinfo_rsa(&pip->ti)) {

	                    if (di.src1 >= 0)
	                        exectrace_wrsa(&pip->ti.et,di.src1) ;

	                    if (di.src2 >= 0)
	                        exectrace_wrsa(&pip->ti.et,di.src2) ;

	                    if (di.src3 >= 0)
	                        exectrace_wrsa(&pip->ti.et,di.src3) ;

	                    if (di.src4 >= 0)
	                        exectrace_wrsa(&pip->ti.et,di.src4) ;

	                    if (di.src5 >= 0)
	                        exectrace_wrsa(&pip->ti.et,di.src5) ;

	                } /* end if (register source addresses) */

		} /* end if (tracing) */

/* OK, now we execute based on if it is a load/store or not */

	        if (di.opclass == OPCLASS_LOAD) {

/* we handle load/stores ourselves */

#ifdef	COMMENT
	            src1i = di.src1 ;	/* base register */
	            src2i = di.src2 ;
	            dst1i = di.dst1 ; /* Destination register */
	            dst2i = di.dst2 ;
#endif

	            offset = di.const1 ;
	            ma = offset + smp->regs[di.src1] ;

#if	F_MASTERDEBUG && F_DEBUG && F_PVALUESBEF
	            if (pip->debuglevel >= 5) {
	                eprintf("simplesim: load before> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                eprintf("simplesim: ma=%08x\n", ma) ;
	                eprintf("simplesim: src r%d=%08x r%d=%08x\n",
	                    di.src1, smp->regs[di.src1],
	                    di.src2, smp->regs[di.src2]) ;
	                eprintf("simplesim: dst r%d=%08x r%d=%08x\n",
	                    di.dst1,smp->regs[di.dst1],
	                    di.dst2,smp->regs[di.dst2]) ;

	            }
#endif /* F_DEBUG */

	            if (f_te) {

	                if (traceinfo_msv(&pip->ti)) {

	                    rs = lsim_readint(mip,(ma & (~ 3)),&mv1) ;

	                    if (rs >= 0)
	                        exectrace_wmsv(&pip->ti.et,ma,mv1) ;

	                    if (di.dst2 >= 0) {

	                        rs = lsim_readint(mip,((ma + 4) & (~ 3)),&mv2) ;

	                        if (rs >= 0)
	                            exectrace_wmsv(&pip->ti.et,(ma + 4),mv2) ;

	                    }

	                } else if (traceinfo_msa(&pip->ti)) {

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel >= 5)
	                        eprintf("simplesim: exectrace_wmsa()\n") ;
#endif

	                    exectrace_wmsa(&pip->ti.et,ma) ;

	                    if (di.dst2 >= 0)
	                        exectrace_wmsa(&pip->ti.et,(ma + 4)) ;

	                } /* end if (trace options) */

	            } /* end if (tracing enabled) */

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 5)
	                eprintf("simplesim: loadinstr()\n") ;
#endif

	            rs = loadinstr(pip,mip,di.opexec,
	                ma, smp->regs[di.src2],&dst1,&dst2) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 5) {
	                eprintf("simplesim: loadinstr() rs=%d\n",rs) ;
	            }
#endif

#if	F_MASTERDEBUG && F_DEBUG
	            if ((pip->debuglevel >= 2) && (rs < 0))
	                eprintf("simplesim: loadinstr() rs=%d ma=%08x\n",
	                    rs,ma) ;
#endif

	            if (rs < 0)
	                break ;

#if	F_MASTERDEBUG && F_DEBUG && F_PVALUESAFT
	            if (pip->debuglevel >= 5) {
	                eprintf("simplesim: load after> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                eprintf("simplesim: ma=%08x\n",
	                    ma) ;
	                eprintf("simplesim: dst r%d=%08x r%d=%08x\n",
	                    di.dst1,dst1,
	                    di.dst2,dst2) ;
	            }
#endif /* F_DEBUG */


/* register write back (only update non-r0 ones) */

	            if (di.dst1 > 0)
	                smp->regs[di.dst1] = dst1 ;

	            if (di.dst2 > 0)
	                smp->regs[di.dst2] = dst2 ;

	        } else if (di.opclass == OPCLASS_STORE) {

#ifdef	COMMENT
	            src1i = di.src1 ;
	            src2i = di.src2 ;
	            src3i = di.src3 ;
#endif

	            offset = di.const1 ;
	            ma = offset + smp->regs[di.src1] ;

#if	F_MASTERDEBUG && F_DEBUG && F_PVALUESBEF
	            if (pip->debuglevel >= 5) {

	                eprintf("simplesim: store before> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                eprintf("simplesim: <%x> "
	                    "base:[%d]=%x offset:%x src:[%d]=%x\n",
	                    ma,
	                    di.src1 ,smp->regs[di.src1],
	                    offset,
	                    di.src2,smp->regs[di.src2]) ;

	            }
#endif /* F_DEBUG */

	            if (f_te) {

	                if (traceinfo_mpv(&pip->ti) &&
	                    (di.src1 >= 0)) {

	                    rs = lsim_readint(mip,(ma & (~ 3)),&mv1) ;

	                    if (rs >= 0)
	                        exectrace_wmsv(&pip->ti.et,ma,mv1) ;

				if (di.dst2 > 0) {

	                    rs = lsim_readint(mip,((ma + 4) & (~ 3)),&mv2) ;

	                    if (rs >= 0)
	                        exectrace_wmsv(&pip->ti.et,(ma + 4),mv2) ;

				}

	                } /* end if (memory previous value) */

	            } /* end if (tracing enabled) */

	            rs = storeinstr(pip,mip,di.opexec,
	                ma,
	                smp->regs[di.src2],
	                smp->regs[di.src3],
	                &ma,&mv1,&mv2,&dp) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if ((pip->debuglevel >= 2) && (rs < 0))
	                eprintf("simplesim: storeinstr() rs=%d\n",rs) ;
#endif

	            if (rs < 0)
	                break ;

	            f_double = (rs == 1) ;

#if	F_MASTERDEBUG && F_DEBUG && F_PVALUESAFT
	            if (pip->debuglevel >= 5) {

	                eprintf("simplesim: store after> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                eprintf("simplesim: <%x> "
	                    "base:[%d]=%x src:[%d]=%x\n",
	                    (offset + smp->regs[di.src1]),
	                    di.src1 ,smp->regs[di.src1],
	                    di.src2,smp->regs[di.src2]) ;

	            }
#endif /* F_DEBUG */

/* should we write some memory trace records ? */

	            if (f_te &&
	                traceinfo_mems(&pip->ti)) {

	                int	rdp ;


	                rdp = (traceinfo_memvals(&pip->ti)) ? dp : 0 ;

	                rs = exectrace_wmem(&pip->ti.et,ma,mv1,rdp) ;

#if	F_MASTERDEBUG && F_DEBUG
	                if ((pip->debuglevel >= 2) && (rs < 0))
	                    eprintf(
	                        "simplesim: exectrace_wmem() rs=%d ma=%08x\n",
	                        rs,ma) ;
#endif

	                if (f_double)
	                    rs = exectrace_wmem(&pip->ti.et,(ma + 4),mv2,rdp) ;

	            } /* end if (memory-store trace records) */

	        } else if (di.opexec != LEXECOP_SYSCALL) {

	            struct lexec_instr	li ;

	            struct lexec_src	ls ;

	            struct lexec_dst	ld ;


/* everything else goes to LEXEC */

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 5) {
	                eprintf("simplesim: need LEXEC execution\n") ;
	            }
#endif

#ifdef	COMMENT
	            if (di.src1 >= 0)
	                src1i = di.src1 ;

	            if (di.src2 >= 0)
	                src2i = di.src2 ;

	            if (di.src3 >= 0)
	                src3i = di.src3 ;

	            if (di.src4 >= 0)
	                src4i = di.src4 ;

	            if (di.src5 >= 0)
	                src5i = di.src5 ;

	            if (di.dst1 >= 0)
	                dst1i = di.dst1 ;

	            if (di.dst2 >= 0)
	                dst2i = di.dst2 ;

	            if (di.dst3 >= 0)
	                dst3i = di.dst3 ;
#endif /* COMMENT */

#if	F_MASTERDEBUG && F_DEBUG && F_PVALUESBEF 
	            if (pip->debuglevel >= 5) {

	                eprintf("simplesim: LEXEC before> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                eprintf("simplesim: s1[%d]=%x s2[%d]=%x "
	                    "s3[%d]=%x \n",
	                    di.src1,smp->regs[di.src1],
	                    di.src2,smp->regs[di.src2],
	                    di.src3,smp->regs[di.src3]) ;
	                eprintf("simplesim: s4[%d]=%x s5[%d]=%x\n",
	                    di.src4,smp->regs[di.src4],
	                    di.src5,smp->regs[di.src5]) ;

	                eprintf("simplesim: const=%08x\n",
	                    di.const1) ;

	                eprintf("simplesim: d1[%d]=%x d2[%d]=%x d3[%d]=%x\n",
	                    di.dst1,smp->regs[di.dst1],
	                    di.dst2,smp->regs[di.dst2],
	                    di.dst3,smp->regs[di.dst3]) ;

	            }
#endif /* F_DEBUG */


#ifdef	COMMENT
	            rs = lexec(di.opexec,
	                &smp->regs[src1i],
	                &smp->regs[src2i],
	                &smp->regs[src3i],
	                &smp->regs[src4i],
	                &dst1,
	                &dst2,
	                &dst3,
	                &di.const1,
	                &boutcome,
	                &ma,
	                &delay,
	                &ilptr,
	                &f_jump,
	                &f_nullify) ;
#endif /* COMMENT */

/* decoded instruction information */

	            li.ia = ia ;
	            li.instr = instr ;
	            li.opexec = di.opexec ;
	            li.constant = di.const1 ;

/* source operands */

	            ls.s1 = smp->regs[di.src1] ;
	            ls.s2 = smp->regs[di.src2] ;
	            ls.s3 = smp->regs[di.src3] ;
	            ls.s4 = smp->regs[di.src4] ;
	            ls.s5 = smp->regs[di.src5] ;


	            rs = lexec(&li,&ls,&ld) ;


/* check for execution faults like divide-by-zero and illegal */

#if	F_MASTERDEBUG && F_DEBUG
	            if ((pip->debuglevel >= 2) && (rs < 0)) {
	                eprintf("simplesim: lexec() rs=%d\n",rs) ;
	                eprintf("simplesim: in=%lld ia=%08x instr=%08x\n",
	                    smp->in,ia,instr) ;
	                rs1 = mipsdis_getlevo(&pip->dis,ia,instr,
	                    instrdis,INSTRDISLEN) ;
	                if (rs1 >= 0)
	                    eprintf("simplesim: instr> %s\n",instrdis) ;
	            }
#endif /* F_DEBUG */

	            if (rs < 0) {

	                bprintf(pip->efp,
			    "%s: bad execution in=%lld ia=%08x op=%d rs=%d\n",
	                    pip->progname,smp->in, li.ia, li.opexec, rs) ;

	            }

#if	F_BREAKERROR
	            if (rs < 0)
	                break ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 5) {
	                eprintf("simplesim: lexec() rs=%d\n",rs) ;
	            }
#endif

/* post-execution processing on control-flow-change instructions */

	            if (cii.f_cf) {

	                cii.f_nullify = ld.f.nullify ;
	                cii.f_taken = ld.f.taken ;
	                cii.ta = di.const1 ;
	                if (cii.opclass == OPCLASS_JIND) {

	                    cii.f_taken = TRUE ;
	                    cii.ta = ld.ta ;

	                }

#if	F_MASTERDEBUG && F_DEBUG
	                if (pip->debuglevel >= 5)
	                    eprintf("simplesim: f_cf f_taken=%d\n",
	                        cii.f_taken) ;
#endif

/* statistics (for control flow) */

	                if (f_se) {

	                    ta_lastbranch = 0 ;
	                    if (cii.opclass == OPCLASS_BREL) {

	                	int	nin ;
	                        int	f_forward ;


/* conditional control-flow change */

	                	scounts.ccf += 1 ;

/* branch domain size */

	                        f_forward = (cii.ta > cii.ia) ;
	                        n = abs(cii.ta - cii.ia) / 4 ;

	                        if (! f_forward)
	                            n -= 1 ;

	                        if (n >= BTSIZE)
	                            n = BTSIZE - 1 ;

	                        scounts.btlen[n] += 1 ;

	                        ia_lastbranch = cii.ia ;
	                        ta_lastbranch = cii.ta ;

/* branch path length */

	                	nin = smp->in - in_bp ;
	                	if (nin >= BPSIZE)
				    nin = BPSIZE - 1 ;
	
	                	scounts.bplen[nin] += 1 ;
	                	in_bp = smp->in ;

	                    } /* end if (conditional branch) */

	                } /* end if (statistics) */

	            } /* end if (control flow change) */


/* register write back (only update non-r0 ones) */

#ifdef	COMMENT
	            if (di.dst1 > 0)
	                smp->regs[dst1i] = dst1 ;

	            if (di.dst2 > 0)
	                smp->regs[dst2i] = dst2 ;

	            if (di.dst3 > 0)
	                smp->regs[dst3i] = dst3 ;
#endif /* COMMENT */

	            if (di.dst1 > 0)
	                smp->regs[di.dst1] = ld.d1 ;

	            if (di.dst2 > 0)
	                smp->regs[di.dst2] = ld.d2 ;

	            if (di.dst3 > 0)
	                smp->regs[di.dst3] = ld.d3 ;


#if	F_MASTERDEBUG && F_DEBUG && F_PVALUESAFT 
	            if (pip->debuglevel >= 5) {
	                eprintf("simplesim: LEXEC after> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

#ifdef	COMMENT
	                eprintf("simplesim: const=%x ma=%x ilptr=%x bcond=%d\n",
	                    di.const1, ma,ilptr,boutcome) ;
#endif

	                eprintf("simplesim: d1[%d]=%x d2[%d]=%x d3[%d]=%x\n",
	                    di.dst1,smp->regs[di.dst1],
	                    di.dst2,smp->regs[di.dst2],
	                    di.dst3,smp->regs[di.dst3]) ;

#ifdef	COMMENT
	                eprintf("simplesim: f_jump=%d f_nullify=%d\n",
	                    f_jump,f_nullify) ;
#endif

	            }
#endif /* F_DEBUG */

	        } else {

/* it is a real SYSCALL, we're screwed so just get out ! */

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 2)
	                eprintf("simplesim: SYSCALL encountered\n") ;
#endif

	            bprintf(pip->efp,"%s: SYSCALL encountered\n",
	                pip->progname) ;

	            rs = SR_NOTSUP ;
	            break ;

	        } /* end if (who executes it) */


/* should we write some register trace records ? */

	        if (f_te &&
	            traceinfo_regs(&pip->ti)) {

	            int	rdp ;


#ifdef	COMMENT
	            rdp = traceinfo_regvals(&pip->ti) ;
#else
	            rdp = 15 ;
#endif

	            if (di.dst1 > 0)
	                exectrace_wreg(&pip->ti.et,di.dst1,
	                    smp->regs[di.dst1],rdp) ;

	            if (di.dst2 > 0)
	                exectrace_wreg(&pip->ti.et,di.dst2,
	                    smp->regs[di.dst2],rdp) ;

	            if (di.dst3 > 0)
	                exectrace_wreg(&pip->ti.et,di.dst3,
	                    smp->regs[di.dst3],rdp) ;

	        } /* end if (register trace records) */


	    } /* end if (instruction was not NULLIFYed) */


/* increment the number of instructions executed */

	    smp->in += 1 ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (((smp->in % 1000000) == 0) &&
	        (pip->debuglevel >= 3)) {

	        if (smp->in > 0)
	            eprintf("simplesim: executed instructions=%lld\n",
	                smp->in) ;
	    }
#endif /* F_DEBUG */


/* statistics */

	    if (f_se) {

	            int	f_forward ;


/* all instructions */

	        scounts.in += 1 ;

#ifdef	COMMENT

/* SS hammock conditional branches */

	        if ((ta_lastbranch == ia) && 
	            (f_forward = (ta_lastbranch > ia_lastbranch))) {

	            scounts.sshammock += 1 ;

	            n = abs(ta_lastbranch - ia_lastbranch) / 4 ;

	            if (! f_forward)
	                n -= 1 ;

	            if (n >= BTSIZE)
	                n = BTSIZE - 1 ;

	            scounts.htlen[n] += 1 ;

	        } /* end if (SS hammock) */
#endif /* COMMENT */

	    } /* end if (statistics) */


/* did we execute the number of instructions we were supposed to ? */

	    if ((nlast > 0) && (smp->in >= nlast))
	        break ;


/* make some decisions based on the previous instruction (MIPS-style !) */

	    ia = cii.ia + 4 ;
	    if (pii.f_cf) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 5)
	            eprintf("simplesim: PI was CF\n") ;
#endif

/* regular business */

	        if (pii.f_taken) {

	            const char	*np ;


#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 5)
	                eprintf("simplesim: PI CF was taken ! ta=%08x\n",
	                    pii.ta) ;
#endif

/* check if SYSCALL and handle as necessary */

	            np = NULL ;
	            rs = syscalls_issyscall(scp,pii.ta,&np) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 5)
	                eprintf("simplesim: syscalls_issyscall() rs=%d n=%s\n",
	                    rs,np) ;
#endif

	            if (rs > 0) {

	                SYSCALLS_ARGREGS	ar ;

	                SYSCALLS_RETREG		rr ;


	                ar.a1 = smp->regs[4] ;
	                ar.a2 = smp->regs[5] ;
	                ar.a3 = smp->regs[6] ;
	                ar.a4 = smp->regs[7] ;
	                ar.sp = smp->regs[29] ;
	                rs = syscalls_handle(scp,pii.ta,&ar,&rr) ;

#if	F_MASTERDEBUG && F_DEBUG
	                if (pip->debuglevel >= 3) {
	                    eprintf("simplesim: PSYSCALL rs=%d ia=%08x\n",
	                        rs,ia) ;
	                    eprintf("simplesim: in=%lld\n",smp->in) ;
	                }
#endif

	                if (rs != 0)
	                    break ;

	                smp->regs[2] = rr.sgirc ;


	                if (f_te &&
	                    traceinfo_regs(&pip->ti)) {

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel >= 3)
	                        eprintf("simplesim: PSYSCALL r2=%08x\n",
	                            smp->regs[2]) ;
#endif

	                    exectrace_wreg(&pip->ti.et,2,smp->regs[2],15) ;

	                }


#ifdef	COMMENT

/* OK, we are done executing, was it a Pseudo System Call ? */

	                rs = checkpsyscall(pip,mip,scp,pii.ia,pii.ta) ;

	                if (rs != 0) {

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel >= 5)
	                        eprintf("simplesim: PSYSCALL rs=%d ia=%08x\n",
	                            rs,ia) ;
#endif

	                    break ;

	                } /* end if (we encountered a PSYSCALL) */

#endif /* COMMENT */

	                ia = cii.ia + 4 ;	/* go on to next instruction */

	            } else {

	                ia = pii.ta ;		/* not a SYSCALL */

	            } /* end if (PSYSCALL or not) */

	        } /* end if (control flow change was taken) */

	    } /* end if (control flow change) */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("simplesim: next ia=%08x\n",ia) ;
#endif

	    pii = cii ;

	} /* end while (looping reading instructions) */

#if	F_MASTERDEBUG && F_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("simplesim: out of loop\n") ;
#endif


/* update the instruction (fetch) address (for possible use later !) */
done:
	smp->ia = ia ;


/* write out any statistics */

	rs1 = writestats(pip,&scounts,smp) ;


	(void) traceinfo_flush(&pip->ti) ;


bad1:
	if (f_itrace) {

		if ((pip->debuglevel > 0) || (terrs > 0))
		bprintf(pip->efp,"%s: i-fetch errors %lld\n",
			pip->progname,terrs) ;

		exectrace_close(&it) ;

	} /* end if (i-fetch trace) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("simplesim: exiting rs=%d\n",rs) ;
#endif

bad0:
	return rs ;
}
/* end subroutine (simplesim) */



/* LOCAL SUBROUTINES */



/* execute a load type instruction */
static int loadinstr(pip,mip,opexec,ma,rval,dvp,d2p)
struct proginfo	*pip ;
LSIM		*mip ;
int		opexec ;
uint		ma ;		/* memory address to load from */
uint		rval ;		/* source value */
uint		*dvp ;		/* destination value (resulting register) */
uint		*d2p ;		/* destination 2 */
{
	uint	mval1, mval2 ;

	int	rs ;


/* read the memory loacation */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("loadinstr: ma=%08x rval=%08x\n",ma,rval) ;
#endif

	rs = lsim_readint(mip,(ma & (~ 3)),&mval1) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("loadinstr: 1 lsim_readint() rs=%d ma=%08x\n",
	        rs,(ma & (~ 3))) ;
#endif

	if (rs < 0)
	    return rs ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("loadinstr: 1 lsim_readint() rs=%d ma=%08x mv=%08x\n",
	        rs,ma,mval1) ;
#endif

	switch (opexec) {

	case LEXECOP_LDC1:
	    rs = lsim_readint(mip,((ma + 4) & (~ 3)),&mval2) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("loadinstr: 2 lsim_readint() rs=%d ma=%08x\n",
	            rs,((ma + 4) & (~ 3))) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("loadinstr: lsim_readint() 2 rs=%d mval2=%08x\n",
	            rs,mval2) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("loadinstr: got a LDC1\n") ;
#endif

	    break ;

	} /* end switch */

/* do whatever the load instruction is supposed to do */

	rs = regload(pip,opexec,ma,mval1,mval2,rval,dvp,d2p) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("loadinstr: regload() rs=%d dreg=%08x\n",rs,*dvp) ;
#endif

	return rs ;
}
/* end subroutine (loadinstr) */


/* perform the register-load function */
static int regload(pip,opexec,ma,mval,mval2,rval,dvp,d2p)
struct proginfo	*pip ;
int		opexec ;
uint		ma ;		/* memory address */
uint		mval ;		/* memory value */
uint		mval2 ;
uint		rval ;		/* starting register value */
uint		*dvp ;		/* destination value (resulting register) */
uint		*d2p ;
{
	int	rs = SR_OK ;
	int	boundary ;


	boundary = ma & 3 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regload: rval=%08x ma=%08x mv=%08x boundary=%d\n",
	        rval,ma,mval,boundary) ;
#endif

	switch (opexec) {

	    int sign ;


	case LEXECOP_LW:
	    *dvp = mval ;
	    break ;

	case LEXECOP_LB:
	    switch (boundary) {

	    case 0:
	        *dvp = ((mval >> 24) & 0xff) ;
	        sign = (mval & 0x80000000) ? 1 : 0 ;
	        break ;

	    case 1:
	        *dvp = ((mval >> 16) & 0xff) ;
	        sign = (mval & 0x00800000) ? 1 : 0 ;
	        break ;

	    case 2:
	        *dvp = ((mval >> 8) & 0xff) ;
	        sign = (mval & 0x00008000) ? 1 : 0 ;
	        break ;

	    case 3:
	        *dvp = ((mval >> 0) & 0xff) ;
	        sign = (mval & 0x00000080) ? 1 : 0 ;
	        break ;

	    } /* end switch */

	    *dvp |= (sign * 0xffffff00) ;
	    break ;

	case LEXECOP_LBU:
	    switch (boundary) {

	    case 0:
	        *dvp = ((mval >> 24) & 0xff) ;
	        break ;

	    case 1:
	        *dvp = ((mval >> 16) & 0xff) ;
	        break ;

	    case 2:
	        *dvp = ((mval >> 8) & 0xff) ;
	        break ;

	    case 3:
	        *dvp = ((mval >> 0) & 0xff) ;
	        break ;

	    } /* end switch */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("regload: LBU ma=%08x mv=%08x rv=%08x\n",
	            ma,mval,*dvp) ;
#endif

	    break ;

	case LEXECOP_LH:
	    switch (boundary) {

	    case 0:
	        *dvp = ((mval >> 16) & 0xffff) ;
	        sign = (mval & 0x80000000) ? 1 : 0 ;
	        break ;

	    case 1:
	        *dvp = ((mval >> 8) & 0xffff) ;
	        sign = (mval & 0x00800000) ? 1 : 0 ;
	        break ;

	    case 2:
	        *dvp = ((mval >> 0) & 0x0000ffff) ;
	        sign = (mval & 0x00008000) ? 1 : 0 ;
	        break ;

	    default:
	        rs = SR_INVALID ;

	    } /* end switch */

	    *dvp |= (sign * 0xffff0000) ;
	    break ;

	case LEXECOP_LHU:
	    switch (boundary) {

	    case 0:
	        *dvp = ((mval >> 16) & 0xffff) ;
	        break ;

	    case 1:
	        *dvp = ((mval >> 8) & 0xffff) ;
	        break ;

	    case 2:
	        *dvp = ((mval >> 0) & 0xffff) ;
	        break ;

	    default:
	        rs = SR_INVALID ;

	    } /* end switch */

	    break ;

	case LEXECOP_LWL:

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("regload: LWL\n") ;
#endif

	    switch (boundary) {

	    case 0:
	        *dvp = mval ;
	        break ;

	    case 1:
	        *dvp = 
	            (rval & 0x000000ff) |
	            ((mval & 0x00ffffff) << 8) ;
	        break ;

	    case 2:
	        *dvp = 
	            (rval & 0x0000ffff) |
	            ((mval & 0x0000ffff) << 16) ;
	        break ;

	    case 3:
	        *dvp = 
	            (rval & 0x00ffffff) |
	            ((mval & 0x000000ff) << 24) ;
	        break ;

	    } /* end switch */

	    break ;

	case LEXECOP_LWR:

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("regload: LWR\n") ;
#endif

	    switch (boundary) {

	    case 0:
	        *dvp = 
	            (rval & 0xffffff00) |
	            ((mval >> 24) & 0xff) ;
	        break ;

	    case 1:
	        *dvp = 
	            (rval & 0xffff0000) |
	            ((mval >> 16) & 0xffff) ;
	        break ;

	    case 2:
	        *dvp = 
	            (rval & 0xff000000) |
	            ((mval >> 8) & 0x00ffffff) ;
	        break ;

	    case 3:
	        *dvp = mval ;
	        break ;

	    } /* end switch */

	    break ;

	case LEXECOP_LWC1:
	    *dvp = mval ;
	    break ;

	case LEXECOP_LDC1:
	    *dvp = mval ;
	    *d2p = mval2 ;
	    break ;

	default:
	    rs = SR_INVALID ;
	    break ;

	} /* end switch */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("regload: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (regload) */


/* do the store instructions */
static int storeinstr(pip,mip,opexec,vaddr,dv1,dv2,map,mv1p,mv2p,dpp)
struct proginfo	*pip ;
LSIM		*mip ;
int		opexec ;
uint		vaddr ;
uint    	dv1, dv2 ;
uint    	*map ;		/* returned memory address value */
uint    	*mv1p, *mv2p ;	/* returned memory data value */
uint    	*dpp ;		/* returned memory data present */
{
	uint	mv1, mv2 ;

	int	rs = SR_OK ;
	int	boundary ;
	int	f_double = FALSE ;


	boundary = vaddr & 3 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("storeinstr: ma=%08x mv=%08x boundary=%d\n",
	        vaddr,dv1,boundary) ;
#endif

	switch (opexec) {

	case LEXECOP_SW:
	    mv1 = dv1 ;
	    *dpp = LFLOWGROUP_DPALL ;
	    break ;

	case LEXECOP_SB:
	    if (boundary == 0) {

	        *dpp = LFLOWGROUP_DPBYTE0 ;
	        mv1 = (dv1 & 0xff) << 24 ;

	    } else if (boundary == 1) {

	        *dpp = LFLOWGROUP_DPBYTE1 ;
	        mv1 = (dv1 & 0xff) << 16 ;

	    } else if (boundary == 2) {

	        *dpp = LFLOWGROUP_DPBYTE2 ;
	        mv1 = (dv1 & 0xff) << 8 ;

	    } else {

	        *dpp = LFLOWGROUP_DPBYTE3 ;
	        mv1 = (dv1 & 0xff) ;

	    }

	    break ;

	case LEXECOP_SH:
	    if (boundary == 0) {

	        *dpp = LFLOWGROUP_DPBYTE0 | LFLOWGROUP_DPBYTE1 ;
	        mv1 = (dv1 & 0xffff) << 16 ;

	    } else if (boundary == 1) {

	        *dpp = LFLOWGROUP_DPBYTE1 | LFLOWGROUP_DPBYTE2 ;
	        mv1 = (dv1 & 0xffff) << 8 ;

	    } else if (boundary == 2) {

	        *dpp = LFLOWGROUP_DPBYTE2 | LFLOWGROUP_DPBYTE3 ;
	        mv1 = (dv1 & 0xffff) ;

	    } else {

/* misalligned mem access */

	        rs = LEXEC_REXCEPT ;

	    }

	    break ;

	case LEXECOP_SWL:
	    mv1 = dv1 >> (8 * boundary) ;
	    if (boundary == 0) {

	        *dpp = LFLOWGROUP_DPALL ;

	    } else if (boundary == 1) {

	        *dpp = LFLOWGROUP_DPBYTE1 | 
	            LFLOWGROUP_DPBYTE2 | 
	            LFLOWGROUP_DPBYTE3 ;

	    } else if (boundary == 2) {

	        *dpp = LFLOWGROUP_DPBYTE2 | LFLOWGROUP_DPBYTE3 ;

	    } else {

	        *dpp = LFLOWGROUP_DPBYTE3 ;

	    }

	    break ;

	case LEXECOP_SWR:
	    mv1 = dv1 << ((3 - boundary) * 8) ;
	    if (boundary == 0) {

	        *dpp = LFLOWGROUP_DPBYTE0 ;

	    } else if (boundary == 1) {

	        *dpp = LFLOWGROUP_DPBYTE0 | LFLOWGROUP_DPBYTE1 ;

	    } else if (boundary == 2) {

	        *dpp = LFLOWGROUP_DPBYTE0 | 
	            LFLOWGROUP_DPBYTE1 | 
	            LFLOWGROUP_DPBYTE2 ;

	    } else {

	        *dpp = LFLOWGROUP_DPALL ;

	    }

	    break ;

	case LEXECOP_SWC1:
	    mv1 = dv1 ;
	    *dpp = LFLOWGROUP_DPALL ;
	    break ;

	case LEXECOP_SDC1:

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("storeinstr: got a SDC1\n") ;
#endif

	    f_double = TRUE ;
	    mv1 = dv1 ;
	    mv2 = dv2 ;
	    *dpp = LFLOWGROUP_DPALL ;
	    break ;

	default:
	    rs = LEXEC_REXCEPT ;
	    break ;

	} /* end switch */

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) & ( rs < 0))
	    eprintf(
	        "storeinstr: LEXEC-type deocde failure rs=%d opexec=%d\n",
	        rs,opexec) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("storeinstr: lsim_writeint() ma=%08x mv=%08x\n",
	        vaddr,mv1) ;
#endif

	if (rs >= 0) {

	    *map = vaddr ;
	    *mv1p = mv1 ;
	    rs = lsim_writeint(mip,(vaddr & (~ 3)),mv1,*dpp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("storeinstr: 1 lsim_writeint() rs=%d ta=%08x\n",
	            rs,(vaddr & (~ 3))) ;
#endif

	    if ((rs >= 0) && f_double) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 5)
	            eprintf("storeinstr: 2 lsim_writeint() ma=%08x mv=%08x\n",
	                (vaddr + 4),mv2) ;
#endif

	        *mv2p = mv2 ;
	        rs = lsim_writeint(mip,((vaddr + 4) & (~ 3)),mv2,*dpp) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if ((pip->debuglevel >= 2) && (rs < 0))
	            eprintf("storeinstr: 2 lsim_writeint() rs=%d ta=%08x\n",
	                rs,((vaddr + 4) & (~ 3))) ;
#endif

	    }

	} /* end if */

	return (rs >= 0) ? f_double : rs ;
}
/* end subroutine (storeinstr) */


/* check for a Pseudo System Call (PSYSCALL) */
static int checkpsyscall(pip,mip,scp,ia,ta)
struct proginfo	*pip ;
LSIM		*mip ;
SYSCALLS	*scp ;
uint		ia, ta ;
{
	int	rs ;

	const char	*np ;



#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("checkpsyscall: syscalls_issyscall() \n") ;
#endif

	rs = syscalls_issyscall(scp,ta,&np) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("checkpsyscall: syscalls_issyscall() rs=%d n=%s\n",
	        rs,np) ;
#endif

	rs = lsim_issyscall(mip,ta,&np) ;

	if (rs > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("checkpsyscall: PSYSCALL ia=%08x ta=%08x syscall=%s\n",
	            ia,ta,np) ;
#endif

	    if ((strcmp(np,"exit") != 0) && (strcmp(np,"_exit") != 0))
	        rs = SR_NOTSUP ;

	} /* end if */

	return rs ;
}
/* end subroutine (PSYSCALL check) */


/* is the instruction (in view) a control-flow instruction ? */
static int iscf(dip)
LDECODE	*dip ;
{
	int	f ;


	f = (dip->opclass == OPCLASS_BREL) || 
	    (dip->opclass == OPCLASS_JREL) ||
	    (dip->opclass == OPCLASS_JIND) ;

	return f ;
}
/* end subroutine (iscf) */


/* write out the statistics */
static int writestats(pip,scp,smp)
struct proginfo		*pip ;
struct stats		*scp ;
struct statemips	*smp ;
{
	bfile	tmpfile ;

	double	v1, v2, ave ;

	time_t	daytime ;

	int	rs, i ;

	char	tmpfname[MAXPATHLEN + 1] ;


/* write out the instruction (operations) frequencies */

	mkfname(tmpfname,pip->jobname,ICOUNTS) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < LEXECOP_TOTAL ; i += 1)
	        bprintf(&tmpfile,"%4d %12lld\n", i,scp->lexecop[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened i-count file) */

/* write out the branch-path lengths */

	mkfname(tmpfname,pip->jobname,BPLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BPSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12lld\n", 
	            i,scp->bplen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened bplen file) */

/* write out the branch-target lengths */

	mkfname(tmpfname,pip->jobname,BTLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BTSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12lld\n", 
	            i,scp->btlen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened bplen file) */

#ifdef	COMMENT

/* write out SS hammock branch-target lengths */

	mkfname(tmpfname,pip->jobname,HTLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BTSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12lld\n", 
	            i,scp->htlen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened bplen file) */

#endif /* COMMENT */

/* write out other branch path information */

	mkfname(tmpfname,pip->jobname,BSTATS) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    double	percent, mean, var, stddev ;


	    u_time(&daytime) ;

	    bprintf(&tmpfile,"SimpleSim statistics %s\n",
	        timestr_log(daytime,tmpfname)) ;

	    bprintf(&tmpfile,"statistics for job=%s logid=%s\n\n",
	        pip->jobname,pip->logid) ;


	    bprintf(&tmpfile,"total instructions    %12lld\n",
	        smp->in) ;

	    bprintf(&tmpfile,"included instructions %12lld:%12lld\n",
	        pip->in.start,scp->in) ;

	    bprintf(&tmpfile,"branch paths          %12lld\n",
	        scp->ccf) ;

/* branch path statistics */

	    v1 = scp->ccf ;		/* control-flow instructions */
	    v2 = scp->in ;		/* total instructions (considered) */
	    ave = (scp->ccf > 0) ? (v2 / v1) : 0.0 ;
	    bprintf(&tmpfile,"instructions per BP %17.4f\n",
	        ave) ;

	    bprintf(&tmpfile,"branch path statistics\n") ;

	    rs = fmeanvaral(scp->bplen,BPSIZE,&mean,&var) ;

	    if (rs >= 0) {

	        bprintf(&tmpfile,"IPBP mean           %17.4f\n",
	            mean) ;

	        bprintf(&tmpfile,"IPBP variance       %17.4f\n",
	            var) ;

	        stddev = sqrt(var) ;

	        bprintf(&tmpfile,"IPBP stddev         %17.4f\n",
	            stddev) ;

	    } else
	        bprintf(&tmpfile,"*NA*\n") ;

/* branch target statistics */

	    bprintf(&tmpfile,"branch target-distance statistics\n") ;

	    rs = fmeanvaral(scp->btlen,BTSIZE,&mean,&var) ;

	    if (rs >= 0) {

	        bprintf(&tmpfile,"IPBT mean           %17.4f\n",
	            mean) ;

	        bprintf(&tmpfile,"IPBT variance       %17.4f\n",
	            var) ;

	        stddev = sqrt(var) ;

	        bprintf(&tmpfile,"IPBT stddev         %17.4f\n",
	            stddev) ;

	    } else
	        bprintf(&tmpfile,"*NA*\n") ;

#ifdef	COMMENT

/* hammock target statistics */

	    bprintf(&tmpfile,"hammock branch target-distance statistics\n") ;

	    rs = fmeanvaral(scp->htlen,BTSIZE,&mean,&var) ;

	    if (rs >= 0) {

	        bprintf(&tmpfile,"IPHT mean           %17.4f\n",
	            mean) ;

	        bprintf(&tmpfile,"IPHT variance       %17.4f\n",
	            var) ;

	        stddev = sqrt(var) ;

	        bprintf(&tmpfile,"IPHT stddev         %17.4f\n",
	            stddev) ;

	    } else
	        bprintf(&tmpfile,"*NA*\n") ;

#endif /* COMMENT */

/* other */

#ifdef	COMMENT
	    {
	        double	fa, fb ;


	        fa = scp->sshammock ;
	        fb = scp->ccf ;
	        percent = (fa / fb) * 100.0 ;

	    } /* end block */

	    bprintf(&tmpfile,
	        "SS hammocks           %12lld (%5.2f%% of c-branches)\n",
	        scp->sshammock,percent) ;

#endif /* COMMENT */

/* done */

	    bclose(&tmpfile) ;

	} /* end if (opened bstats file) */


	return SR_OK ;
}
/* end subroutine (writestats) */



