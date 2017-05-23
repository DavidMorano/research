/* stats */

/* statistics gathering */


#define	CF_DEBUGS	0		/* non-switchable */
#define	CF_DEBUG	1		/* switchable */
#define	F_SAFE		1		/* extra safe mode ? */
#define	F_SAFE2		1		/* extra safe mode ? */
#define F_PVALUESBEF    0		/* print values before */
#define F_PVALUESAFT    0 		/* print values after */
#define F_CREATETRACE   0		/* create debugging trace data */
#define	F_MEMFAULT	0		/* fault on memory */
#define	F_EXECFAULT	0		/* fault on instruction execution */
#define	F_SSHTESTING	0		/* SSH testing */
#define	F_BREAKVP	0		/* break the value-prediction */
#define	F_LATEUPDATE	1		/* do BP update from trace */


/* revision history:

	= 01/07/10, David Morano

	This code (the old i-fetch code) has been turned into SimpleSim !


	= 01/07/27, Marcos de Alba

	I added another field to the call of lexec (ilptr).


	= 01/08/10, David Morano

	I changed the code so that the ISA registers are passed to us.
	Also, the code will only execute for a maximum number of
	instructions (if specified).  I also changed some variable
	types to be consistent (or more consistent) across the
	whole program.  I also added symbolic TRUE/FALSE for
	boolean variables.


	= 01/08/17, David Morano

	I added the calls to write out trace information.
	I had to add the whole trace facility to LevoSim first.


	= 01/08/29, David Morano

	I added the calls to get some disassembled output text for the
	instrcutions being executed.


	= 01/10/07, David Morano

	I fixed the instructions SH, LWL, and LWR.


	= 01/10/10, David Morano

	I added the code for the LWC1 and SWC1 instructions.


	= 01/11/06, David Morano

	I added the code for the LDC1 and SDC1 instructions.


	= 02/03/14, David Morano

	I took this from the old LevoSim (file 'simplesim.c') and
	hacked it up to be used for gathering statistics from
	traces.


*/

/* Copyright © 1998 David Morano.  All rights reserved. */

/******************************************************************************

	This subroutine performs the function 'fcount' for the
	TRACEPROC program when run in 'fcount' mode.

	This code just gathers statistics from traces now (sigh).
	It used to be a part of LevoSim.



******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	<vsystem.h>
#include	<bfile.h>
#include	<keyopt.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"

#include	"exectrace.h"
#include	"lmapprog.h"
#include	"fcount.h"
#include	"ssh.h"
#include	"vpred.h"
#include	"yags.h"
#include	"bpalpha.h"
#include	"gspag.h"
#include	"eveight.h"
#include	"bpresult.h"

#include	"lflowgroup.h"
#include	"lmipsregs.h"
#include	"opclass.h"
#include	"lexecop.h"
#include	"ldecodeinstrclass.h"
#include	"ldecode.h"
#include	"lexec.h"

#include	"mipsdis.h"
#include	"statemips.h"

#include	"tracedata.h"
#include	"bpfifo.h"



/* local defines */

#define	INSTRDISLEN	100		/* buffer to hold disassembly */
#define	BPSIZE		100		/* instructions in basic block */
#define	BTSIZE		10000		/* instructions in basic block */

#define	DEBUGLEVEL(n)	(pip->debuglevel >= (n))

#define	FE_SGINM	".sginm"
#define	FE_ICOUNTS	".eticounts"
#define	FE_FCOUNTS	".etfcounts"
#define	FE_BPLEN	".etbplen"
#define	FE_BTLEN	".etbtlen"
#define	FE_HTLEN	".ethtlen"
#define	FE_BSTATS	".etstats"
#define	FE_SSH		".ssh"

#define	NSSH		1000		/* expected (?) number SSH */

#define	DEFROWS		1		/* default rows */
#define	DEFDELAY	512		/* default delay */

#define	DEFVP_ROWS	64
#define	DEFVP_DELAY	512
#define	DEFVP_STRIDE	8		/* in bits */
#define	DEFVP_NOPS	2		/* number operands */
#define	DEFVP_ENTRY	64		/* number value prediction entries */

#define	DEFYAGS_ROWS		64
#define	DEFYAGS_DELAY		512
#define	DEFYAGS_CPHT		4096
#define	DEFYAGS_DPHT		1024

#define	DEFBPALPHA_ROWS		64
#define	DEFBPALPHA_DELAY	512
#define	DEFBPALPHA_LBHT		512
#define	DEFBPALPHA_LPHT		1024
#define	DEFBPALPHA_GPHT		4096

#define	DEFGSPAG_ROWS		64
#define	DEFGSPAG_DELAY		512
#define	DEFGSPAG_LBHT		1024
#define	DEFGSPAG_GPHT		4096

#define	DEFEVEIGHT_ROWS		1
#define	DEFEVEIGHT_DELAY	512
#define	DEFEVEIGHT_TLEN		-1



/* external subroutines */

extern int	mkfname2(char *,const char *,const char *) ;
extern int	optmatch3(const char **,const char *,int) ;
extern int	headkeymat(const char *,const char *,int) ;
extern int	fmeanvaral(ULONG *,int,double *,double *) ;
extern int	cfdeci(const char *,int,int *) ;

extern char	*timestr_log(time_t,char *) ;

#if	defined(SOLARIS) && (SOLARIS == 8)
extern double	sqrt(double) ;
#endif


/* local structures */

struct instrinfo {
	ULONG	in ;			/* instruction number */
	uint	ia ;
	uint	instr ;
	uint	opexec ;
	uint	opclass ;
	uint	ta ;
	uint	f_cf : 1 ;		/* control flow */
	uint	f_cbr : 1 ;		/* conditional branch */
	uint	f_fcbr : 1 ;		/* forward conditional branch */
	uint	f_taken : 1 ;		/* only for conditional branches */
	uint	f_nullify : 1 ;		/* state for instruction NULLIFY */
	uint	f_yags : 1 ;
	uint	f_bpalpha : 1 ;
	uint	f_gspag : 1 ;
	uint	f_eveight : 1 ;
} ;

struct ustats {
	ULONG	in ;			/* number of instructions */
	ULONG	cf ;			/* control-flow-change instructions */
	ULONG	cf_ind ;		/* indirect CFC */
	ULONG	br_rel ;		/* relative branches */
	ULONG	br_con ;		/* conditional branches */
	ULONG	br_fwd ;		/* forward conditional branches */
	ULONG	br_ssh ;		/* single-sided simple hammocks */
	ULONG	lexecop[LEXECOP_TOTAL] ;	/* opcodes */
	ULONG	bplen[BPSIZE] ;		/* instructions per BP */
	ULONG	btlen[BTSIZE] ;		/* branch target length */
	ULONG	htlen[BTSIZE] ;		/* SS hammock branch target length */
	ULONG	ia_bad ;		/* bad IA from execution */
	ULONG	vp_inlu ;
	ULONG	vp_inhit ;		/* value-prediction lookup hit */
	ULONG	vp_oplu ;		/* operand lookups */
	ULONG	vp_ophit ;		/* operand lookup hits */
	ULONG	vp_opcor ;		/* correct operand lookups */
	ULONG	vp_allcor ;		/* all operands correct */
	ULONG	yags_correct ;		/* YAGS predicted correctly */
	ULONG	yags_bits ;		/* YAGS bits */
	ULONG	bpalpha_correct ;	/* BPALPHA predicted correctly */
	ULONG	bpalpha_bits ;		/* BPALPHA bits */
	ULONG	gspag_correct ;		/* GSPAG predicted correctly */
	ULONG	gspag_bits ;		/* GSPAG bits */
	ULONG	eveight_correct ;	/* EVEIGHT predicted correctly */
	ULONG	eveight_bits ;		/* EVEIGHT bits */
} ;

struct testing {
	uint	n ;			/* number */
	uint	c_ssh ;
	uint	c_ne ;
	uint	hias[NSSH] ;
} ;

struct params {
	uint	rows ;
	uint	delay ;
	uint	vp_rows ;
	uint	vp_delay ;
	uint	vp_entries ;
	uint	vp_stride ;
	uint	vp_nops ;
	uint	yags_rows ;
	uint	yags_delay ;
	uint	yags_cpht ;
	uint	yags_dpht ;
	uint	bpalpha_rows ;
	uint	bpalpha_delay ;
	uint	bpalpha_lbht ;
	uint	bpalpha_lpht ;
	uint	bpalpha_gpht ;
	uint	gspag_rows ;
	uint	gspag_delay ;
	uint	gspag_lbht ;
	uint	gspag_gpht ;
	uint	eveight_rows ;
	uint	eveight_delay ;
	uint	eveight_tlen ;
} ;

struct vpentry {
	uint	ops[5] ;
	int	n ;
	int	row ;
} ;

struct vpfifo {
	struct vpentry	*table ;
	int		head, tail ;
	int		n ;
} ;


/* forward references */

static int	loadinstr(struct proginfo *,LMAPPROG *,int,uint,uint,
			uint *,uint *) ;
static int	storeinstr(struct proginfo *,LMAPPROG *,int,uint,uint,uint,
			uint *,uint *,uint *,uint *) ;
static int	regload(struct proginfo *,int,uint,uint,uint,uint,
			uint *,uint *) ;
static int	writestats(struct proginfo *, struct ustatemips *,
			VPRED_STATS *, struct ustats *,struct params *) ;
static int	writevpstats(struct proginfo *, struct ustatemips *,
			VPRED_STATS *, struct ustats *,int) ;

static int	hias_check(struct testing *,uint) ;
static int	getstatsopts(struct proginfo *,KEYOPT *,struct params *) ;

#ifdef	COMMENT
static int	checkpsyscall(struct proginfo *,LSIM *,SYSCALLS *,uint,uint) ;
#endif

static int	vpfifo_init(struct vpfifo *,int) ;
static int	vpfifo_free(struct vpfifo *) ;
static int	vpfifo_add(struct vpfifo *,uint *,int,int) ;
static int	vpfifo_remove(struct vpfifo *,uint *,int *) ;

static double	percentll(ULONG,ULONG) ;


/* local variables */

static const char	*keyopts[] = {
	    "rows",
	    "delay",
	    "vpred:rows",
	    "vpred:delay",
	    "vpred:entries",
	    "vpred:stride",
	    "vpred:nops",
	    "yags:rows",
	    "yags:delay",
	    "yags:cpht",
	    "yags:dpht",
	    "bpalpha:rows",
	    "bpalpha:delay",
	    "bpalpha:gpht",
	    "bpalpha:lpht",
	    "bpalpha:lbht",
	    "gspag:rows",
	    "gspag:delay",
	    "gspag:lbht",
	    "gspag:gpht",
	    "eveight:rows",
	    "eveight:delay",
	    "eveight:tlen",
	    NULL
} ;

enum keyopts {
	keyopt_rows,
	keyopt_delay,
	keyopt_vprows,
	keyopt_vpdelay,
	keyopt_vpentries,
	keyopt_vpstride,
	keyopt_vpnops,
	keyopt_yagsrows,
	keyopt_yagsdelay,
	keyopt_yagscpht,
	keyopt_yagsdpht,
	keyopt_bpalpharows,
	keyopt_bpalphadelay,
	keyopt_bpalphagpht,
	keyopt_bpalphalpht,
	keyopt_bpalphalbht,
	keyopt_gspagrows,
	keyopt_gspagdelay,
	keyopt_gspaglbht,
	keyopt_gspaggpht,
	keyopt_eveightrows,
	keyopt_eveightdelay,
	keyopt_eveighttlen,
	keyopt_overlast
} ;

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






int stats(pip,mpp,mdp,kop,skipinstr,tfname)
struct proginfo	*pip ;
LMAPPROG	*mpp ;
MIPSDIS		*mdp ;
KEYOPT		*kop ;
ULONG		skipinstr ;
char		tfname[] ;
{
	LDECODE 		di ;

	EXECTRACE		t ;

	EXECTRACE_ENTRY		e, *ep = &e ;

	SSH			hammocks ;

	SSH_ENTRY		*hep ;

	VPRED			*valuepred ;

	VPRED_STATS		vpstats ;

	YAGS			*bp_yags ;

	BPALPHA			*bp_bpalpha ;

	GSPAG			*bp_gspag ;

	EVEIGHT			*bp_eveight ;

	FCOUNT			funcs ;

	struct ustatemips	mstate, *smp = &mstate  ;
	struct ustatemips	sstate ;

	struct params		p ;

	struct instrinfo	bii, pii, cii ;

	struct vpfifo		vf ;

	BPFIFO			bf_yags, bf_bpalpha, bf_gspag, bf_eveight ;

	struct ustats		scounts ;

	struct testing		other ;

	ULONG	in_bp ;			/* branch path instruction number */
	ULONG	nlast ;			/* last instruction for us */
	ULONG	trecs = 0 ;

	uint	ia, instr ;
	uint	dst1, dst2, dst3 ;
	uint	ma, mv, mv1, mv2 ;	/* memory address & value */
	uint    a, rv, dp ;		/* data present bits (for memory) */
	uint	ia_lastbranch ;		/* IA of last c-branch seen */
	uint	ta_lastbranch ;		/* TA of last c-branch seen */
	uint	pvalues[5] ;		/* predicted source operand values */
	uint	rvalues[5] ;		/* real source operand values */

	int	rs = SR_OK, rs1 ;
	int	i, n, size ;
	int	offset ;		/* constant offset in instructions */
	int	c, nnzero ;
	int	vprow, bprow ;
	int	f_exit = FALSE ;
	int	f_double ;
	int	f_se ;			/* selection enable */
	int	f_ssh = FALSE ;		/* have ss-hammock detection */
	int	f_vpred = FALSE ;	/* have value-prediction */
	int	f_vpredicted ;		/* value-predicted ? */
	int	f_yags = FALSE ;	/* YAGS branch predictor */
	int	f_bpalpha = FALSE ;	/* BPALPHA branch predictor */
	int	f_gspag = FALSE ;	/* GSPAG predictor */
	int	f_eveight = FALSE ;	/* EVEIGHT */
	int	f ;

	char	tmpfname[MAXPATHLEN + 1] ;
	char	sshfname[MAXPATHLEN + 1] ;
	char	instrdis[INSTRDISLEN + 1] ;


	(void) memset(&mstate,0,sizeof(struct ustatemips)) ;


#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("stats: tfname=%s\n",tfname) ;
#endif

	rs = exectrace_open(&t,tfname,"r",0666,NULL) ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("stats: exectrace_open() rs=%d\n",rs) ;
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


/* optional parameters */

	size = sizeof(struct params) ;
	(void) memset(&p,0,size) ;

	getstatsopts(pip,kop,&p) ;

/* general */

#ifdef	COMMENT
	if (p.rows < 1)
	    p.rows = DEFROWS ;

	if (p.delay < 1)
	    p.delay = DEFDELAY ;
#endif /* COMMENT */

/* specific */

	if (p.vp_rows < 1)
	    p.vp_rows = (p.rows > 0) ? p.rows : DEFVP_ROWS ;

	if (p.vp_delay < 1)
	    p.vp_delay = (p.delay > 0) ? p.delay : DEFVP_DELAY ;

	if (p.vp_stride <= 0)
	    p.vp_stride = DEFVP_STRIDE ;

	if (p.vp_nops <= 0)
	    p.vp_nops = DEFVP_NOPS ;

	if (p.vp_entries <= 0)
	    p.vp_entries = DEFVP_ENTRY ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("stats: vp_entries=%d vp_rows=%d\n",
	        p.vp_entries,p.vp_rows) ;
#endif

/* YAGS */

	if (p.yags_rows < 1)
	    p.yags_rows = (p.rows > 0) ? p.rows : DEFYAGS_ROWS ;

	if (p.yags_delay < 1)
	    p.yags_delay = (p.delay > 0) ? p.delay : DEFYAGS_DELAY ;

	if (p.yags_cpht < 1)
	    p.yags_cpht = DEFYAGS_CPHT ;

	if (p.yags_dpht < 1)
	    p.yags_dpht = DEFYAGS_DPHT ;

/* BPALPHA */

	if (p.bpalpha_rows < 1)
	    p.bpalpha_rows = (p.rows > 0) ? p.rows : DEFBPALPHA_ROWS ;

	if (p.bpalpha_delay < 1)
	    p.bpalpha_delay = (p.delay > 0) ? p.delay : DEFBPALPHA_DELAY ;

	if (p.bpalpha_lbht < 1)
	    p.bpalpha_lbht = DEFBPALPHA_LBHT ;

	if (p.bpalpha_lpht < 1)
	    p.bpalpha_lpht = DEFBPALPHA_LPHT ;

	if (p.bpalpha_gpht < 1)
	    p.bpalpha_gpht = DEFBPALPHA_GPHT ;

/* GSPAG */

	if (p.gspag_rows < 1)
	    p.gspag_rows = (p.rows > 0) ? p.rows : DEFGSPAG_ROWS ;

	if (p.gspag_delay < 1)
	    p.gspag_delay = (p.delay > 0) ? p.delay : DEFGSPAG_DELAY ;

	if (p.gspag_lbht < 1)
	    p.gspag_lbht = DEFGSPAG_LBHT ;

	if (p.gspag_gpht < 1)
	    p.gspag_gpht = DEFGSPAG_GPHT ;

/* EVEIGHT */

	if (p.eveight_rows < 1)
	    p.eveight_rows = (p.rows > 0) ? p.rows : DEFEVEIGHT_ROWS ;

	if (p.eveight_delay < 1)
	    p.eveight_delay = (p.delay > 0) ? p.delay : DEFEVEIGHT_DELAY ;

	if (p.eveight_tlen < 1)
	    p.eveight_tlen = DEFEVEIGHT_TLEN ;


/* statistics */

	in_bp = 0 ;
	ia_lastbranch = 0 ;
	ta_lastbranch = 0 ;

	size = sizeof(struct ustats) ;
	(void) memset(&scounts,0,size) ;


/* loop termination */

	nlast = 0 ;
	if (pip->ninstr > 0)
	    nlast = (nlast > 0) ? MIN(pip->ninstr,nlast) : pip->ninstr ;

	if (skipinstr > 0)
	    nlast = (nlast > 0) ? MIN(skipinstr,nlast) : skipinstr ;

	if (pip->in.n > 0)
	    nlast = (nlast > 0) ? MIN(pip->in.end,nlast) : pip->in.end ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    debugprintf("stats: nlast=%lld\n",nlast) ;
#endif


/* value predictor for source operands */

	if (pip->f.opt_vpred) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: opt_vpred\n") ;
#endif

	    size = p.vp_rows * sizeof(VPRED) ;
	    rs = uc_malloc(size,&valuepred) ;

	    if (rs < 0)
	        goto bad1 ;

#ifdef	MALLOCLOG
	    malloclog_alloc(valuepred,rs,"stats:valuepred") ;
#endif

	    f_vpred = TRUE ;
	    for (i = 0 ; i < p.vp_rows ; i += 1) {

	        rs = vpred_init(&valuepred[i],
	            p.vp_entries,p.vp_nops,p.vp_stride) ;

	        f_vpred &= (rs >= 0) ;
	        if (rs < 0)
	            bprintf(pip->efp,
	                "%s: could not initialize the value predictor (%d)\n",
	                pip->progname,rs) ;

	    } /* end for */

	    if (f_vpred)
	        rs = vpfifo_init(&vf,p.vp_delay + 2) ;

	    if ((! f_vpred) || (rs < 0)) {

	        for (i = 0 ; i < p.vp_rows ; i += 1)
	            vpred_free(&valuepred[i]) ;

	        uc_free(valuepred) ;

#ifdef	MALLOCLOG
	        malloclog_free(valuepred,"stats:valuepred") ;
#endif

	        goto bad1 ;
	    }

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: f_vpred=%d\n",f_vpred) ;
#endif

	} /* end if (opt_vpred) */


/* YAGS branch predictor */

	if (pip->f.opt_yags) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: opt_yags\n") ;
#endif

	    size = p.yags_rows * sizeof(YAGS) ;
	    rs = uc_malloc(size,&bp_yags) ;

	    if (rs < 0)
	        goto bad2 ;

#ifdef	MALLOCLOG
	    malloclog_alloc(bp_yags,rs,"stats:bp_yags") ;
#endif

	    f_yags = TRUE ;
	    for (i = 0 ; i < p.yags_rows ; i += 1) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: i=%d yags_init()\n",i) ;
#endif

	        rs = yags_init(&bp_yags[i], p.yags_cpht,p.yags_dpht) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: yags_init() rs=%d\n",rs) ;
#endif

	        f_yags &= (rs >= 0) ;
	        if (rs < 0)
	            bprintf(pip->efp,
	                "%s: could not initialize the YAGS predictor (%d)\n",
	                pip->progname,rs) ;

	    } /* end for */

	    if (f_yags)
	        rs = bpfifo_init(&bf_yags,p.yags_delay + 2) ;

	    if ((! f_yags) || (rs < 0)) {

	        for (i = 0 ; i < p.yags_rows ; i += 1) {
	            yags_free(&bp_yags[i]) ;
		}

	        uc_free(bp_yags) ;

#ifdef	MALLOCLOG
	        malloclog_free(bp_yags,"stats:bp_yags") ;
#endif

	        goto bad2 ;
	    }

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: f_yags=%d\n",f_yags) ;
#endif

	} /* end if (opt_yags) */


/* BPALPHA branch predictor */

	if (pip->f.opt_bpalpha) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: opt_bpalpha\n") ;
#endif

	    size = p.bpalpha_rows * sizeof(BPALPHA) ;
	    rs = uc_malloc(size,&bp_bpalpha) ;

	    if (rs < 0)
	        goto bad2a ;

#ifdef	MALLOCLOG
	    malloclog_alloc(bp_bpalpha,rs,"stats:bp_bpalpha") ;
#endif

	    f_bpalpha = TRUE ;
	    for (i = 0 ; i < p.bpalpha_rows ; i += 1) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: i=%d bpalpha_init()\n",i) ;
#endif

	        rs = bpalpha_init(&bp_bpalpha[i], 
	            p.bpalpha_lbht,p.bpalpha_lpht,p.bpalpha_gpht) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: bpalpha_init() rs=%d\n",rs) ;
#endif

	        f_bpalpha &= (rs >= 0) ;
	        if (rs < 0)
	            bprintf(pip->efp,
	                "%s: could not initialize the BPALPHA predictor (%d)\n",
	                pip->progname,rs) ;

	    } /* end for */

	    if (f_bpalpha) {
	        rs = bpfifo_init(&bf_bpalpha,p.bpalpha_delay + 2) ;
	    }

	    if ((! f_bpalpha) || (rs < 0)) {

	        for (i = 0 ; i < p.bpalpha_rows ; i += 1) {
	            bpalpha_free(&bp_bpalpha[i]) ;
		}

	        uc_free(bp_bpalpha) ;

#ifdef	MALLOCLOG
	        malloclog_free(bp_bpalpha,"stats:bp_bpalpha") ;
#endif

	        goto bad2a ;
	    }

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: f_bpalpha=%d\n",f_bpalpha) ;
#endif

	} /* end if (opt_bpalpha) */


/* GSPAG branch predictor */

	if (pip->f.opt_gspag) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: opt_gspag\n") ;
#endif

	    size = p.gspag_rows * sizeof(GSPAG) ;
	    rs = uc_malloc(size,&bp_gspag) ;

	    if (rs < 0)
	        goto bad2b ;

#ifdef	MALLOCLOG
	    malloclog_alloc(bp_gspag,rs,"stats:bp_gspag") ;
#endif

	    f_gspag = TRUE ;
	    for (i = 0 ; i < p.gspag_rows ; i += 1) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: i=%d gspag_init()\n",i) ;
#endif

	        rs = gspag_init(&bp_gspag[i], p.gspag_lbht,p.gspag_gpht) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: gspag_init() rs=%d\n",rs) ;
#endif

	        f_gspag &= (rs >= 0) ;
	        if (rs < 0)
	            bprintf(pip->efp,
	                "%s: could not initialize the GS-PAG predictor (%d)\n",
	                pip->progname,rs) ;

	    } /* end for */

	    if (f_gspag) {
	        rs = bpfifo_init(&bf_gspag,p.gspag_delay + 2) ;
	    }

	    if ((! f_gspag) || (rs < 0)) {

	        for (i = 0 ; i < p.gspag_rows ; i += 1) {
	            gspag_free(&bp_gspag[i]) ;
		}

	        uc_free(bp_gspag) ;

#ifdef	MALLOCLOG
	        malloclog_free(bp_gspag,"stats:bp_gspag") ;
#endif

	        goto bad2b ;
	    }

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: f_gspag=%d\n",f_gspag) ;
#endif

	} /* end if (opt_gspag) */


/* EVEIGHT branch predictor */

	if (pip->f.opt_eveight) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: opt_eveight\n") ;
#endif

	    size = p.eveight_rows * sizeof(EVEIGHT) ;
	    rs = uc_malloc(size,&bp_eveight) ;

	    if (rs < 0)
	        goto bad2c ;

#ifdef	MALLOCLOG
	    malloclog_alloc(bp_eveight,rs,"stats:bp_eveight") ;
#endif

	    f_eveight = TRUE ;
	    for (i = 0 ; i < p.eveight_rows ; i += 1) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: i=%d eveight_init()\n",i) ;
#endif

	        rs = eveight_init(&bp_eveight[i], p.eveight_tlen,-1,-1,-1) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: eveight_init() rs=%d\n",rs) ;
#endif

	        f_eveight &= (rs >= 0) ;
	        if (rs < 0)
	            bprintf(pip->efp,
	                "%s: could not initialize the EVEIGHT predictor (%d)\n",
	                pip->progname,rs) ;

	    } /* end for */

	    if (f_eveight)
	        rs = bpfifo_init(&bf_eveight,p.eveight_delay + 2) ;

	    if ((! f_eveight) || (rs < 0)) {

	        for (i = 0 ; i < p.eveight_rows ; i += 1) {
	            eveight_free(&bp_eveight[i]) ;
		}

	        uc_free(bp_eveight) ;

#ifdef	MALLOCLOG
	        malloclog_free(bp_eveight,"stats:bp_eveight") ;
#endif

	        goto bad2c ;
	    }

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: f_eveight=%d\n",f_eveight) ;
#endif

	} /* end if (opt_eveight) */


/* SS-Hammocks */

	if (pip->f.opt_ssh) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: opt_ssh\n") ;
#endif

	    mkfname2(sshfname,pip->basename,FE_SSH) ;

	    rs = ssh_init(&hammocks,sshfname) ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: ssh_init() rs=%d\n",rs) ;
#endif

	    f_ssh = (rs >= 0) ;
	    if (rs < 0)
	        bprintf(pip->efp,
	            "%s: could not initialize for SSH detection (%d)\n",
	            pip->progname,rs) ;

#if	F_SSHTESTING
	    if (f_ssh) {

	        for (i = 0 ; (i < NSSH) && (ssh_get(&hammocks,i,&hep) >= 0) ; 
	            i += 1) {

	            other.hias[i] = 0 ;
	            if (hep == NULL)
	                continue ;

	            other.hias[i] = hep->ia ;

	        } /* end for */

	        other.n = i ;

	        if (DEBUGLEVEL(4))
	            debugprintf("stats: SSH-n %d\n", other.n) ;

	    } /* end if (SSH testing) */
#endif /* F_SSHTESTING */

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: f_ssh=%d\n",f_ssh) ;
#endif

	} /* end if (opt_ssh) */


/* function profiling */

	if (pip->f.opt_fcount) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: opt_fcount\n") ;
#endif

	    mkfname2(tmpfname,pip->basename,FE_SGINM) ;

	    rs = fcount_init(&funcs,mpp,tmpfname) ;

	    if (rs < 0)
	        goto bad4 ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("stats: f_fcount=%d\n",rs) ;
#endif

	} /* end if (opt_fcount) */


/* other initialization */

	instrdis[0] = '\0' ;


	(void) memset(&bii,0,sizeof(struct instrinfo)) ;

	(void) memset(&pii,0,sizeof(struct instrinfo)) ;

	pii.opclass = OPCLASS_UNUSED ;
	pii.opexec = LEXECOP_NOOP ;


#ifdef	COMMENT
	boutcome = 0 ;
#endif

/* OK, start the looping ! */

	ia = smp->ia ;
	while (! f_exit) {

	    uint	ia_next ;


#ifdef	COMMENT
	    pip->debuglevel = debugwin_check(&pip->dw,0ULL,smp->in) ;
#endif

/* start new (current) instruction sequence */

	    (void) memset(&cii,0,sizeof(struct instrinfo)) ;


/* TRACE: read the trace (get the next instruction) */

	    ia_next = ia ;
	    while (TRUE) {

	        rs = exectrace_read(&t,&e) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if ((pip->debuglevel >= 2) && (rs < 0))
	            debugprintf("stats: exectrace_read() rs=%d\n",rs) ;
#endif

	        if (rs <= 0)
	            break ;

	        trecs += 1 ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(7))
	            debugprintf("stats: trecs=%lld f_ia=%d\n",trecs,e.f.ia) ;
#endif

	        if (e.f.ia)
	            break ;

	    } /* end while (looping through the trace) */

	    if (rs <= 0) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            debugprintf("stats: end-of-trace in=%lld\n",smp->in) ;
#endif

	        goto done ;
	    }

	    if (ia_next != e.ia) {

	        scounts.ia_bad += 1 ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(3))
	            debugprintf("stats: bad IA in=%lld next_ia=%08x e_ia=%08x\n",
	                smp->in,
	                ia_next,e.ia) ;
#endif

	    }

	    ia = e.ia ;

/* TRACE: end of reading the trace */


/* update any branch predictors */

#if	F_LATEUPDATE
	    if (bii.f_cbr) {

	        ULONG	fin ;

	        int	f_outcome ;


	        f_outcome = (bii.ta == ia) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: f_outcome=%d\n",f_outcome) ;
#endif

	        if (f_yags) {

	            if (f_se && LEQUIV(bii.f_yags,f_outcome))
	                scounts.yags_correct += 1 ;

	            fin = bii.in + p.yags_delay ;
	            bprow = bii.in % p.yags_rows ;
	            bpfifo_add(&bf_yags,fin, bii.ia,bprow,f_outcome) ;

	        } /* end if (queue YAGS update) */

	        if (f_bpalpha) {

	            if (f_se && LEQUIV(bii.f_bpalpha,f_outcome))
	                scounts.bpalpha_correct += 1 ;

	            fin = bii.in + p.bpalpha_delay ;
	            bprow = bii.in % p.bpalpha_rows ;
	            bpfifo_add(&bf_bpalpha,fin, bii.ia,bprow,f_outcome) ;

	        } /* end if (queue BPALPHA update) */

	        if (f_gspag) {

	            if (f_se && LEQUIV(bii.f_gspag,f_outcome))
	                scounts.gspag_correct += 1 ;

	            fin = bii.in + p.gspag_delay ;
	            bprow = bii.in % p.gspag_rows ;
	            bpfifo_add(&bf_gspag,fin, bii.ia,bprow,f_outcome) ;

	        } /* end if (queue GSPAG update) */

	        if (f_eveight) {

	            if (f_se && LEQUIV(bii.f_eveight,f_outcome))
	                scounts.eveight_correct += 1 ;

	            fin = bii.in + p.eveight_delay ;
	            bprow = bii.in % p.eveight_rows ;
	            bpfifo_add(&bf_eveight,fin, bii.ia,bprow,f_outcome) ;

	        } /* end if (queue EVEIGHT update) */

	    } /* end if (had a conditional-branch) */
#endif /* F_LATEUPDATE */


/* update the function counter */

	    if (pip->f.opt_fcount)
	        fcount_update(&funcs,ia,0) ;


/* continue like we were (almost) for "real" ! */

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        debugprintf("stats: in=%lld ia=%08x\n",smp->in,ia) ;
#endif

	    rs = lmapprog_readinstr(mpp,ia,&instr) ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6)) {
	        debugprintf("stats: lmapprog_readinstr() rs=%d instr=%08x\n",
	            rs,instr) ;
	    }
#endif

#if	F_MASTERDEBUG && CF_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        debugprintf("stats: lmapprog_readinstr() rs=%d "
	            "ia=%08x instr=%08x\n",
	            rs,ia,instr) ;
#endif

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5)) {

	        rs1 = SR_NOTSUP ;
	        if (pip->f.instrdis)
	            rs1 = mipsdis_getlevo(mdp,ia,instr,
	                instrdis,INSTRDISLEN) ;

	        if (rs1 < 0)
	            strcpy(instrdis,"unknown") ;

	        debugprintf("stats: in=%lld ia=%08x instr=%08x %s\n",
	            smp->in,ia,instr,instrdis) ;
	    }
#endif /* CF_DEBUG */

#if	F_MASTERDEBUG && CF_DEBUG && F_CREATETRACE
	    if (DEBUGLEVEL(5) &&
	        (instrdis[0] == '\0')) {

	        rs1 = SR_NOTSUP ;
	        if (pip->f.instrdis)
	            rs1 = mipsdis_getlevo(mdp,ia,instr,
	                instrdis,INSTRDISLEN) ;

	        if (rs1 < 0)
	            strcpy(instrdis,"unknown") ;

	        debugprintf("simplesim!itrace: %08x %08x %s\n",
	            ia,instr,instrdis) ;

	    }
#endif /* F_CREATETRACE */


	    rs = ldecode_decode(&di,ia,instr) ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if ((pip->debuglevel >= 2) && (rs != 0)) {
	        debugprintf("stats: ldecode_decode() rs=%d\n",rs) ;
	        debugprintf("stats: in=%lld ia=%08x instr=%08x\n",
	            smp->in,ia,instr) ;
	        rs1 = mipsdis_getlevo(mdp,ia,instr,
	            instrdis,INSTRDISLEN) ;
	        if (rs1 >= 0)
	            debugprintf("stats: instr> %s\n",instrdis) ;
	    }
#endif /* CF_DEBUG */

	    if (rs < 0) {

	        bprintf(pip->efp,"%s: bad decode in=%lld rs=%d\n",
	            pip->progname,smp->in,rs) ;

	    }

	    if (rs != 0)
	        break ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("stats: class=%s(%d) opexec=%d\n",
	            instrclasses[di.opclass],di.opclass,di.opexec) ;
#endif


/* statistics */

	    rs = proginfo_selection(pip,0LL,smp->in) ;

	    if (rs < 0) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(2))
	            debugprintf("stats: proginfo_selection() rs=%dd\n",
	                rs) ;
#endif
	        return rs ;
	    }

	    f_se = (rs > 0) ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        debugprintf("stats: in=%lld f_se=%d\n",
	            smp->in,f_se) ;
#endif


/* check for our favorite instructions ! */

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5)) {

	        if ((di.opexec == LEXECOP_CVTDW) || 
	            (di.opexec == LEXECOP_CVTSW) || 
	            (di.opexec == LEXECOP_CVTWD) || 
	            (di.opexec == LEXECOP_CVTWS) || 
	            (di.opexec == LEXECOP_CVTSD) || 
	            (di.opexec == LEXECOP_CVTDS)) {

	            debugprintf("stats: CVT in=%lld ia=%08x instr=%08x\n",
	                smp->in,ia,instr) ;

	            if (DEBUGLEVEL(6))
	                debugprintf("stats: CVT s1=%d s2=%d d1=%d d2=%d\n",
	                    di.src1, di.src2, di.dst1, di.dst2) ;

	        }
	    }
#endif /* CF_DEBUG */


/* statistics update */

	    {
	        int	oi ;


	        oi = MIN(di.opexec,(LEXECOP_TOTAL - 1)) ;
	        scounts.lexecop[oi] += 1 ;

	    } /* end block (statistics update) */


/* prepare some operand stuff */


/* source operand value prediction */

	    if (f_vpred) {

	        c = 0 ;
	        nnzero = 0 ;
	        if (di.src1 > 0) {
	            c = 1 ;
	            nnzero += 1 ;
	        }

	        if (di.src2 > 0) {
	            c = 2 ;
	            nnzero += 1 ;
	        }

	        if (di.src3 > 0) {
	            c = 3 ;
	            nnzero += 1 ;
	        }

	        if (di.src4 > 0) {
	            c = 4 ;
	            nnzero += 1 ;
	        }

	        if (di.src5 > 0) {
	            c = 5 ;
	            nnzero += 1 ;
	        }

	        if (f_se) {

	            scounts.vp_inlu += 1 ;
	            scounts.vp_oplu += nnzero ;

	        }

	        vprow = smp->in % p.vp_rows ;
	        rs = vpred_lookup(&valuepred[vprow],ia,pvalues,c) ;

	        f_vpredicted = (rs >= 0) ;
	        if (rs >= 0) {

	            c = rs ;
	            if (f_se) {

	                scounts.vp_inhit += 1 ;
	                scounts.vp_ophit += MIN(c,nnzero) ;

	            }

	        } /* end if */

	    } /* end if (f_vpred) */


/* OK, let's pretend we committed and compare against the predicted values */

	    if (f_vpred) {

	        size = 5 * sizeof(uint) ;
	        (void) memset(rvalues,0,size) ;

/* get the values (one way or another) */

	        if (e.f.sreg) {

	            for (i = 0 ; i < e.f.sreg ; i += 1) {

	                a = e.sreg[i].a ;
	                sstate.regs[a] = e.sreg[i].dv ;

	            }

	            if (di.src1 > 0)
	                rvalues[0] = sstate.regs[di.src1] ;

	            if (di.src2 > 0)
	                rvalues[1] = sstate.regs[di.src2] ;

	            if (di.src3 > 0)
	                rvalues[2] = sstate.regs[di.src3] ;

	            if (di.src4 > 0)
	                rvalues[3] = sstate.regs[di.src4] ;

	            if (di.src5 > 0)
	                rvalues[4] = sstate.regs[di.src5] ;

	        } else {

	            if (di.src1 > 0)
	                rvalues[0] = smp->regs[di.src1] ;

	            if (di.src2 > 0)
	                rvalues[1] = smp->regs[di.src2] ;

	            if (di.src3 > 0)
	                rvalues[2] = smp->regs[di.src3] ;

	            if (di.src4 > 0)
	                rvalues[3] = smp->regs[di.src4] ;

	            if (di.src5 > 0)
	                rvalues[4] = smp->regs[di.src5] ;

	        } /* end if */

/* verify the predicted values against the real ones */

	        if (f_vpredicted && f_se) {

	            int	f_allcor = TRUE ;


	            if ((c >= 1) && (di.src1 > 0)) {

	                if (pvalues[0] == rvalues[0])
	                    scounts.vp_opcor += 1 ;

	                else
	                    f_allcor = FALSE ;

	            }

	            if ((c >= 2) && (di.src2 > 0)) {

	                if (pvalues[1] == rvalues[1])
	                    scounts.vp_opcor += 1 ;

	                else
	                    f_allcor = FALSE ;

	            }

	            if ((c >= 3) && (di.src3 > 0)) {

	                if (pvalues[2] == rvalues[2])
	                    scounts.vp_opcor += 1 ;

	                else
	                    f_allcor = FALSE ;

	            }

	            if ((c >= 4) && (di.src4 > 0)) {

	                if (pvalues[3] == rvalues[3])
	                    scounts.vp_opcor += 1 ;

	                else
	                    f_allcor = FALSE ;

	            }

	            if ((c >= 5) && (di.src5 > 0)) {

	                if (pvalues[4] == rvalues[4])
	                    scounts.vp_opcor += 1 ;

	                else
	                    f_allcor = FALSE ;

	            }

	            if (f_allcor)
	                scounts.vp_allcor += 1 ;

	        } /* end if (values were originally predicted) */

/* queue the values for later update */

	        vpfifo_add(&vf,rvalues,c,vprow) ;

/* update the value predictor with the real values */

	        if (smp->in >= p.vp_delay) {

	            c = vpfifo_remove(&vf,rvalues,&vprow) ;

#if	F_BREAKVP
	            rvalues[0] = 23 ;
	            rvalues[1] = 123 ;
#endif /* F_BREAKVP */

	            rs = vpred_update(&valuepred[vprow],ia,rvalues,c) ;

	        } /* end if (updating predictor) */

	    } /* end if (had a value predictor) */


/* determine if this instruction is a load/store or something else */

	    cii.in = smp->in ;
	    cii.ia = ia ;
	    cii.instr = instr ;
	    cii.opclass = di.opclass ;
	    cii.opexec = di.opexec ;


/* do the instruction execution */

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4)) {

	        if (pii.f_nullify)
	            debugprintf("stats: this instructions was nullified\n") ;

	    }
#endif

	    if (! pii.f_nullify) {


/* OK, now we execute based on if it is a load/store or not */

	        if (di.opclass == OPCLASS_LOAD) {

/* we handle load/stores ourselves */

	            offset = di.const1 ;

	            ma = offset + smp->regs[di.src1] ;

#if	F_MASTERDEBUG && CF_DEBUG && F_PVALUESBEF
	            if (DEBUGLEVEL(5)) {
	                debugprintf("stats: load before> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                debugprintf("stats: ma=%08x\n", ma) ;
	                debugprintf("stats: src r%d=%08x r%d=%08x\n",
	                    di.src1, smp->regs[di.src1],
	                    di.src2, smp->regs[di.src2]) ;
	                debugprintf("stats: dst r%d=%08x r%d=%08x\n",
	                    di.dst1,smp->regs[di.dst1],
	                    di.dst2,smp->regs[di.dst2]) ;

	            }
#endif /* CF_DEBUG */


#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                debugprintf("stats: loadinstr()\n") ;
#endif

	            rs = loadinstr(pip,mpp,di.opexec,
	                ma, smp->regs[di.src2],&dst1,&dst2) ;

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                debugprintf("stats: loadinstr() rs=%d\n",rs) ;
#endif

#if	F_MASTERDEBUG && CF_DEBUG && F_MEMFAULT
	            if ((pip->debuglevel >= 2) && (rs < 0))
	                debugprintf("stats: loadinstr() rs=%d ma=%08x\n",
	                    rs,ma) ;
#endif

#if	F_MEMFAULT

	            if (rs < 0)
	                break ;

#if	F_MASTERDEBUG && CF_DEBUG && F_PVALUESAFT
	            if (DEBUGLEVEL(5)) {
	                debugprintf("stats: load after> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                debugprintf("stats: ma=%08x\n",
	                    ma) ;
	                debugprintf("stats: dst r%d=%08x r%d=%08x\n",
	                    di.dst1,dst1,
	                    di.dst2,dst2) ;
	            }
#endif /* CF_DEBUG */


#endif /* F_MEMFAULT */

	        } else if (di.opclass == OPCLASS_STORE) {

	            offset = di.const1 ;
	            ma = offset + smp->regs[di.src1] ;

#if	F_MASTERDEBUG && CF_DEBUG && F_PVALUESBEF
	            if (DEBUGLEVEL(5)) {

	                debugprintf("stats: store before> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                debugprintf("stats: <%x> "
	                    "base:[%d]=%x offset:%x src:[%d]=%x\n",
	                    ma,
	                    di.src1 ,smp->regs[di.src1],
	                    offset,
	                    di.src2,smp->regs[di.src2]) ;

	            }
#endif /* CF_DEBUG */


	            rs = storeinstr(pip,mpp,di.opexec,
	                ma,
	                smp->regs[di.src2],
	                smp->regs[di.src3],
	                &ma,&mv,&mv2,&dp) ;

#if	F_MASTERDEBUG && CF_DEBUG && F_MEMFAULT
	            if ((pip->debuglevel >= 2) && (rs < 0))
	                debugprintf("stats: storeinstr() rs=%d\n",rs) ;
#endif

#if	F_MEMFAULT

	            if (rs < 0)
	                break ;

#endif /* F_MEMFAULT */

	            f_double = (rs == 1) ;

#if	F_MASTERDEBUG && CF_DEBUG && F_PVALUESAFT
	            if (DEBUGLEVEL(5)) {

	                debugprintf("stats: store after> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                debugprintf("stats: <%x> "
	                    "base:[%d]=%x src:[%d]=%x\n",
	                    (offset + smp->regs[di.src1]),
	                    di.src1 ,smp->regs[di.src1],
	                    di.src2,smp->regs[di.src2]) ;

	            }
#endif /* CF_DEBUG */


	        } else if (di.opexec != LEXECOP_SYSCALL) {

	            struct lexec_instr	li ;

	            struct lexec_src	ls ;

	            struct lexec_dst	ld ;


/* everything else goes to LEXEC */

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(6))
	                debugprintf("stats: need LEXEC execution\n") ;
#endif

#if	F_MASTERDEBUG && CF_DEBUG && F_PVALUESBEF 
	            if (DEBUGLEVEL(5)) {

	                debugprintf("stats: LEXEC before> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                debugprintf("stats: s1[%d]=%x s2[%d]=%x "
	                    "s3[%d]=%x \n",
	                    di.src1,smp->regs[di.src1],
	                    di.src2,smp->regs[di.src2],
	                    di.src3,smp->regs[di.src3]) ;
	                debugprintf("stats: s4[%d]=%x s5[%d]=%x\n",
	                    di.src4,smp->regs[di.src4],
	                    di.src5,smp->regs[di.src5]) ;

	                debugprintf("stats: const=%08x\n",
	                    di.const1) ;

	                debugprintf("stats: d1[%d]=%x d2[%d]=%x d3[%d]=%x\n",
	                    di.dst1,smp->regs[di.dst1],
	                    di.dst2,smp->regs[di.dst2],
	                    di.dst3,smp->regs[di.dst3]) ;

	            }
#endif /* CF_DEBUG */


	            cii.f_cf = (cii.opclass == OPCLASS_BREL) || 
	                (cii.opclass == OPCLASS_JREL) ||
	                (cii.opclass == OPCLASS_JIND) ;

	            cii.f_cbr = (cii.opclass == OPCLASS_BREL) &&
	                ((cii.instr & 0xffff0000) != 0x10000000) ;


/* OK, make some branch predictions */

	            if (cii.f_cbr) {

	                if (f_yags) {

	                    bprow = smp->in % p.yags_rows ;
	                    rs = yags_lookup(&bp_yags[bprow],ia) ;

	                    cii.f_yags = (rs > 0) ;

#if	F_MASTERDEBUG && CF_DEBUG
	                    if (DEBUGLEVEL(4))
	                        debugprintf("stats: in=%lld yags_lookup() rs=%d\n",
	                            smp->in,rs) ;
#endif

	                } /* end if (YAGS) */

	                if (f_bpalpha) {

	                    bprow = smp->in % p.bpalpha_rows ;
	                    rs = bpalpha_lookup(&bp_bpalpha[bprow],ia) ;

	                    cii.f_bpalpha = (rs > 0) ;

#if	F_MASTERDEBUG && CF_DEBUG
	                    if (DEBUGLEVEL(4))
	                        debugprintf("stats: in=%lld bpalpha_lookup() rs=%d\n",
	                            smp->in,rs) ;
#endif

	                } /* end if (BPALPHA) */

	                if (f_gspag) {

	                    bprow = smp->in % p.gspag_rows ;
	                    rs = gspag_lookup(&bp_gspag[bprow],ia) ;

	                    cii.f_gspag = (rs > 0) ;

#if	F_MASTERDEBUG && CF_DEBUG
	                    if (DEBUGLEVEL(4))
	                        debugprintf("stats: in=%lld gspag_lookup() rs=%d\n",
	                            smp->in,rs) ;
#endif

	                } /* end if (GSPAG) */

	                if (f_eveight) {

	                    bprow = smp->in % p.eveight_rows ;
	                    rs = eveight_lookup(&bp_eveight[bprow],ia) ;

	                    cii.f_eveight = (rs > 0) ;

#if	F_MASTERDEBUG && CF_DEBUG
	                    if (DEBUGLEVEL(4))
	                        debugprintf("stats: in=%lld eveight_lookup() rs=%d\n",
	                            smp->in,rs) ;
#endif

	                } /* end if (EVEIGHT) */

	            } /* end if (a conditional branch) */


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

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(6))
	                debugprintf("stats: lexec() rs=%d\n",rs) ;
#endif


/* check for execution faults like divide-by-zero and illegal */

#if	F_MASTERDEBUG && CF_DEBUG && F_EXECFAULT
	            if ((pip->debuglevel >= 2) && (rs < 0)) {
	                debugprintf("stats: lexec() rs=%d\n",rs) ;
	                debugprintf("stats: in=%lld ia=%08x instr=%08x\n",
	                    smp->in,ia,instr) ;
	                rs1 = mipsdis_getlevo(mdp,ia,instr,
	                    instrdis,INSTRDISLEN) ;
	                if (rs1 >= 0)
	                    debugprintf("stats: instr> %s\n",instrdis) ;
	            }
#endif /* CF_DEBUG */

#if	F_EXECFAULT

	            if (rs < 0) {

	                bprintf(pip->efp,
	                    "%s: bad execution in=%lld rs=%d\n",
	                    pip->progname,smp->in,rs) ;

	            }

	            if (rs < 0)
	                break ;

#endif /* F_EXECFAULT */

/* post-execution processing on control-flow-change instructions */

	            if (cii.f_cf) {

#if	F_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(6))
	                    debugprintf("stats: LEXEC f_taken=%d\n", ld.f.taken) ;
#endif

	                cii.f_nullify = ld.f.nullify ;
	                cii.f_taken = ld.f.taken ;
	                cii.ta = di.const1 ;
	                if (cii.opclass == OPCLASS_JIND) {

	                    cii.f_taken = TRUE ;
	                    cii.ta = ld.ta ;

	                }

#if	F_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(6))
	                    debugprintf("stats: CF f_taken=%d\n",cii.f_taken) ;
#endif

	                if (cii.f_cbr) {

#if	F_MASTERDEBUG && CF_DEBUG
	                    if (DEBUGLEVEL(4))
	                        debugprintf("stats: CB f_taken=%d\n",cii.f_taken) ;
#endif

	                    cii.f_fcbr = (cii.ta > cii.ia) ;

/* store the branch outcome for later BP update */

#if	(! F_LATEUPDATE)
	                    if (f_yags) {

	                        ULONG	fin ;


	                        fin = smp->in + p.yags_delay ;
	                        bprow = smp->in % p.yags_rows ;
	                        bpfifo_add(&bf_yags,fin, ia,bprow,cii.f_taken) ;

	                    }
#endif /* (! F_LATEUPDATE) */

	                } /* end if (conditional branch) */

#if	F_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(6))
	                    debugprintf("stats: CF f_taken=%d\n",
	                        cii.f_taken) ;
#endif


/* statistics */

	                if (f_se) {

	                    scounts.cf += 1 ;
	                    if (di.opclass == OPCLASS_JIND)
	                        scounts.cf_ind += 1 ;

	                    ta_lastbranch = 0 ;
	                    if (cii.opclass == OPCLASS_BREL) {

	                        scounts.br_rel += 1 ;
	                        if ((instr & 0xffff0000) != 0x10000000) {

	                            scounts.br_con += 1 ;
	                            n = abs(cii.ta - cii.ia) / 4 ;

#ifdef	COMMENT
	                            if (! cii.f_fcbr)
	                                n -= 1 ;
#endif /* COMMENT */

	                            if (n >= BTSIZE)
	                                n = BTSIZE - 1 ;

	                            scounts.btlen[n] += 1 ;

/* test for ss-hammocks */

	                            if (cii.f_fcbr) {

	                                f = FALSE ;
	                                scounts.br_fwd += 1 ;
	                                if (f_ssh) {

	                                    rs = ssh_check(&hammocks,ia,&
	                                        hep) ;

	                                    if ((rs >= 0) && 
	                                        (hep->type & SSH_BTSSH)) {

	                                        f = TRUE ;
	                                        scounts.br_ssh += 1 ;
	                                        n = (hep->ta - hep->ia) /
	                                            4 ;

	                                        if (n >= BTSIZE)
	                                            n = BTSIZE -
	                                                1 ;

	                                        scounts.htlen[n] += 1
	                                            ;

#if	F_MASTERDEBUG && CF_DEBUG
	                                        if (DEBUGLEVEL(4))
	                                            debugprintf(
	                                                "stats: SSH in=%lld ia=%08x n=%d\n",
	                                                smp->in,cii.ia,
	                                                n) ;
#endif

	                                    } /* end if (SSH) */

#if	F_SSHTESTING
	                                    rs = hias_check(&other,ia) ;

	                                    if (! LEQUIV((rs >= 0),f)) {

	                                        other.c_ne += 1 ;

	                                        if (DEBUGLEVEL(4))
	                                            debugprintf(
	                                                "stats: SSH-ne in=%lld"
							" ia=%08x rs=%d\n",
	                                                smp->in,ia,rs) ;

	                                    }
#endif /* F_SSHTESTING */

	                                } /* end if (SSH) */

	                            } /* end if (forward c-branch) */

/* other */

	                            ia_lastbranch = cii.ia ;
	                            ta_lastbranch = cii.ta ;

#ifdef	COMMENT
/* how did we do with branch prediction ? */

#endif /* COMMENT */

	                        } /* end if (conditional branch) */

	                    } /* end if (relative branch) */

	                } /* end if (statistics) */

	            } /* end if (control flow instruction) */


#if	F_MASTERDEBUG && CF_DEBUG && F_PVALUESAFT 
	            if (DEBUGLEVEL(5)) {
	                debugprintf("stats: LEXEC after> %08x %08x %s\n",
	                    ia,instr,instrdis) ;

	                debugprintf("stats: d1[%d]=%x d2[%d]=%x d3[%d]=%x\n",
	                    di.dst1,smp->regs[di.dst1],
	                    di.dst2,smp->regs[di.dst2],
	                    di.dst3,smp->regs[di.dst3]) ;

	            }
#endif /* CF_DEBUG */

	        } else {

/* it is a real SYSCALL, we're screwed so just get out ! */

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(2))
	                debugprintf("stats: SYSCALL encountered\n") ;
#endif

	            bprintf(pip->efp,"%s: SYSCALL encountered\n",
	                pip->progname) ;

	            rs = SR_NOTSUP ;
	            break ;

	        } /* end if (who executes it) */


/* register write back (only update non-r0 ones) */

#ifdef	COMMENT /* never write our own execution values back ! */

	        if (di.dst1 > 0)
	            smp->regs[di.dst1] = ld.d1 ;

	        if (di.dst2 > 0)
	            smp->regs[di.dst2] = ld.d2 ;

	        if (di.dst3 > 0)
	            smp->regs[di.dst3] = ld.d3 ;

#endif /* COMMENT */


/* TRCAE: write back the register values from the trace */

	        if (ep->f.reg) {

	            for (i = 0 ; i < ep->f.reg ; i += 1) {

	                if (ep->reg[i].dp) {

	                    a = ep->reg[i].a ;
	                    smp->regs[a] = ep->reg[i].dv ;

	                } /* end if (data present) */

	            } /* end for */

	        } /* end if */

/* TRACE: end of trace thing */


	    } /* end if (instruction was not NULLIFYed) */


/* update any predictors that are scheduled into the future */

	    if (f_yags) {

	        ULONG	fin ;

	        uint	fia, frow, foutcome ;


	        rs = bpfifo_read(&bf_yags,&fin,&fia,&frow,&foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: in=%lld bpfifo_read() rs=%d\n",
	                smp->in,rs) ;
#endif

	        if ((rs >= 0) && (smp->in >= fin)) {

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4)) {
	                debugprintf("stats: in=%lld yags_update() \n",
	                    smp->in) ;
	                debugprintf("stats: frow=%d fia=%08x\n",
	                    frow,fia) ;
	            }
#endif

	            rs = yags_update(bp_yags + frow,fia,foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                debugprintf("stats: in=%lld yags_update() rs=%d\n",
	                    smp->in,rs) ;
#endif

	            bpfifo_remove(&bf_yags,
	                &fin,&fia,&frow,&foutcome) ;

	        } /* end if (did an update) */

	    } /* end if (YAGS) */

	    if (f_bpalpha) {

	        ULONG	fin ;

	        uint	fia, frow, foutcome ;


	        rs = bpfifo_read(&bf_bpalpha,
	            &fin,&fia,&frow,&foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: in=%lld bpfifo_read() rs=%d\n",
	                smp->in,rs) ;
#endif

	        if ((rs >= 0) && (smp->in >= fin)) {

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4)) {
	                debugprintf("stats: in=%lld bpalpha_update() \n",
	                    smp->in) ;
	                debugprintf("stats: frow=%d fia=%08x\n",
	                    frow,fia) ;
	            }
#endif

	            rs = bpalpha_update(bp_bpalpha + frow,fia,foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                debugprintf("stats: in=%lld yags_update() rs=%d\n",
	                    smp->in,rs) ;
#endif

	            bpfifo_remove(&bf_bpalpha,
	                &fin,&fia,&frow,&foutcome) ;

	        } /* end if (did an update) */

	    } /* end if (BPALPHA) */

	    if (f_gspag) {

	        ULONG	fin ;

	        uint	fia, frow, foutcome ;


	        rs = bpfifo_read(&bf_gspag,
	            &fin,&fia,&frow,&foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: in=%lld bpfifo_read() rs=%d\n",
	                smp->in,rs) ;
#endif

	        if ((rs >= 0) && (smp->in >= fin)) {

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4)) {
	                debugprintf("stats: in=%lld gspag_update() \n",
	                    smp->in) ;
	                debugprintf("stats: frow=%d fia=%08x\n",
	                    frow,fia) ;
	            }
#endif

	            rs = gspag_update(bp_gspag + frow,fia,foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                debugprintf("stats: in=%lld gspag_update() rs=%d\n",
	                    smp->in,rs) ;
#endif

	            bpfifo_remove(&bf_gspag,
	                &fin,&fia,&frow,&foutcome) ;

	        } /* end if (did an update) */

	    } /* end if (GSPAG) */

	    if (f_eveight) {

	        ULONG	fin ;

	        uint	fia, frow, foutcome ;


	        rs = bpfifo_read(&bf_eveight,
	            &fin,&fia,&frow,&foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("stats: in=%lld bpfifo_read() rs=%d\n",
	                smp->in,rs) ;
#endif

	        if ((rs >= 0) && (smp->in >= fin)) {

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4)) {
	                debugprintf("stats: in=%lld eveight_update() \n",
	                    smp->in) ;
	                debugprintf("stats: frow=%d fia=%08x\n",
	                    frow,fia) ;
	            }
#endif

	            rs = eveight_update(bp_eveight + frow,fia,foutcome) ;

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                debugprintf("stats: in=%lld eveight_update() rs=%d\n",
	                    smp->in,rs) ;
#endif

	            bpfifo_remove(&bf_eveight,
	                &fin,&fia,&frow,&foutcome) ;

	        } /* end if (did an update) */

	    } /* end if (EVEIGHT) */


/* increment the number of instructions executed */

	    smp->in += 1 ;

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4)) {

	        if (((smp->in % 10000000) == 0) && (smp->in > 0)) {

	            if (smp->in > 0)
	                debugprintf("stats: so far executed instructions=%lld\n",
	                    smp->in) ;
	        }
	    }
#endif /* CF_DEBUG */


/* statistics */

	    if (f_se) {

/* all instructions */

	        scounts.in += 1 ;

	    } /* end if (statistics) */


/* did we execute the number of instructions we were supposed to ? */

	    rs = SR_OK ;
	    if ((nlast > 0) && (smp->in >= nlast))
	        break ;


/* make some decisions based on the previous instruction (MIPS-style !) */

	    ia = cii.ia + 4 ;
	    if (pii.f_cf) {

	        int	f_conditional ;


#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            debugprintf("stats: PI was CF\n") ;
#endif

	        f_conditional = ((pii.opclass == OPCLASS_BREL) &&
	            ((pii.instr & 0xffff0000) != 0x10000000)) ;


/* statistics update */

	        if (f_se) {

	            if (pii.opclass == OPCLASS_BREL) {

	                if ((pii.instr & 0xffff0000) != 0x10000000) {

	                    int	nin ;


	                    nin = smp->in - in_bp ;
	                    if (nin >= BPSIZE)
	                        nin = BPSIZE - 1 ;

	                    scounts.bplen[nin] += 1 ;
	                    in_bp = smp->in ;

	                } /* end if (conditional branch) */

	            } /* end if (relative branch) */

	        } /* end block (statistics update) */

/* regular business */

	        if (pii.f_taken) {

	            const char	*np ;


#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                debugprintf("stats: PI CF was taken ! ta=%08x\n",
	                    pii.ta) ;
#endif

/* check if SYSCALL and handle as necessary */

	            np = NULL ;
#ifdef	COMMENT
	            rs = syscalls_issyscall(scp,pii.ta,&np) ;
#else
	            rs = -1 ;
#endif

#if	F_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(6))
	                debugprintf("stats: syscalls_issyscall() rs=%d n=%s\n",
	                    rs,np) ;
#endif

#ifdef	COMMENT
	            if (rs > 0) {

	                SYSCALLS_ARGREGS	ar ;

	                SYSCALLS_RETREG		rr ;


	                ar.a1 = smp->regs[4] ;
	                ar.a2 = smp->regs[5] ;
	                ar.a3 = smp->regs[6] ;
	                ar.a4 = smp->regs[7] ;
	                ar.sp = smp->regs[29] ;
	                rs = syscalls_handle(scp,pii.ta,&ar,&rr) ;

#if	F_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(6)) {
	                    debugprintf("stats: PSYSCALL rs=%d ia=%08x\n",
	                        rs,ia) ;
	                    debugprintf("stats: in=%lld\n",smp->in) ;
	                }
#endif

	                if (rs != 0)
	                    break ;

	                smp->regs[2] = rr.sgirc ;


	                ia = cii.ia + 4 ;	/* go on to next instruction */

	            } else {

	                ia = pii.ta ;		/* not a SYSCALL */

	            } /* end if (PSYSCALL or not) */
#else
	            ia = pii.ta ;		/* not a SYSCALL */
#endif /* COMMENT */

	        } /* end if (control flow change was taken) */

	    } /* end if (control flow change) */

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("stats: next ia=%08x\n",ia) ;
#endif

	    bii = pii ;
	    pii = cii ;

	} /* end while (looping reading instructions) */


/* update the instruction (fetch) address (for possible use later !) */
done:
	smp->ia = ia ;


/* write out any statistics */

	{
	    VPRED_STATS		vps ;

	    YAGS_STATS		bpystat ;

	    BPALPHA_STATS	bpastat ;

	    GSPAG_STATS		stat_gspag ;

	    EVEIGHT_STATS	stat_eveight ;

	    char	tmpfname[MAXPATHLEN + 1] ;


	    if (f_yags) {

	        yags_stats(&bp_yags[0],&bpystat) ;

	        scounts.yags_bits = bpystat.bits ;

	    }

	    if (f_bpalpha) {

	        bpalpha_stats(&bp_bpalpha[0],&bpastat) ;

	        scounts.bpalpha_bits = bpastat.bits ;

	    }

	    if (f_gspag) {

	        gspag_stats(&bp_gspag[0],&stat_gspag) ;

	        scounts.gspag_bits = stat_gspag.bits ;

	    }

	    if (f_eveight) {

	        eveight_stats(&bp_eveight[0],&stat_eveight) ;

	        scounts.eveight_bits = stat_eveight.bits ;

	    }

	    rs1 = writestats(pip,smp,&vps,&scounts,&p) ;


/* value prediction stuff */

	    if (f_vpred) {

	        for (i = 0 ; i < p.vp_rows ; i += 1) {

	            vpred_stats(&valuepred[i],&vps) ;

	            rs1 = writevpstats(pip,smp,&vps,&scounts,i) ;

	        } /* end for */

	    } /* end if (value prediction) */


/* function instruction counts */

	    if (pip->f.opt_fcount) {

	        bfile	tmpfile ;


	        fcount_done(&funcs) ;

	        mkfname2(tmpfname,pip->jobname,FE_FCOUNTS) ;

	        if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	            FCOUNT_ENTRY	*ep ;

	            double	fn, fd, fpercent ;

	            uint	ins, calls ;


	            bprintf(&tmpfile,"%12llu %7.3f%% %08x %08x %s\n",
	                smp->in,100.0,0,0,"*total*") ;

	            fcount_getother(&funcs,&ins,&calls) ;

	            fn = (double) ins ;
	            fd = (double) smp->in ;
	            fpercent = (smp->in != 0) ? (fn * 100.0 / fd) : 0.0 ;
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
#endif

	                if (ep == NULL)
	                    continue ;

	                fn = (double) ep->ins ;
	                fd = (double) smp->in ;
	                fpercent = (smp->in != 0) ? (fn * 100.0 / fd) : 0.0 ;
	                bprintf(&tmpfile,"%12ld %7.3f%% %08x %08x %s\n",
	                    ep->ins,fpercent,ep->ia,ep->size,ep->name) ;

	            } /* end for */

	            bclose(&tmpfile) ;

	        } /* end if (function instruction counts file) */

	    } /* end if (function instruction counts) */


	} /* end block (writing statistics) */


	if (pip->verboselevel > 0) {

	    if (pip->verboselevel > 1)
	        bprintf(pip->ofp,
	            "trace records              %12llu\n",
	            trecs) ;

	    bprintf(pip->ofp,
	        "instructions in statistics %12llu\n",
	        scounts.in) ;

	    bprintf(pip->ofp,
	        "total instructions         %12llu\n",
	        smp->in) ;

	} /* end if (verbosity) */

#if	F_SSHTESTING
	if (DEBUGLEVEL(4)) {
	    debugprintf("stats: SSH          %d\n",
	        other.c_ssh) ;
	    debugprintf("stats: SSH-ne       %d\n",
	        other.c_ne) ;
	}
#endif

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("stats: exiting in=%llu rs=%d\n",smp->in,rs) ;
#endif


bad5:
	if (pip->f.opt_fcount) {
	    fcount_free(&funcs) ;
	}

bad4:
	if (f_ssh) {
	    ssh_free(&hammocks) ;
	}

bad3:
	if (f_eveight) {

	    bpfifo_free(&bf_eveight) ;

	    for (i = 0 ; i < p.eveight_rows ; i += 1) {
	        eveight_free(&bp_eveight[i]) ;
	    }

	    uc_free(bp_eveight) ;

#ifdef	MALLOCLOG
	    malloclog_free(bp_eveight,"stats:bp_eveight") ;
#endif

	}

bad2c:
	if (f_gspag) {

	    bpfifo_free(&bf_gspag) ;

	    for (i = 0 ; i < p.gspag_rows ; i += 1) {
	        gspag_free(&bp_gspag[i]) ;
	    }

	    uc_free(bp_gspag) ;

#ifdef	MALLOCLOG
	    malloclog_free(bp_gspag,"stats:bp_gspag") ;
#endif

	}

bad2b:
	if (f_bpalpha) {

	    bpfifo_free(&bf_bpalpha) ;

	    for (i = 0 ; i < p.bpalpha_rows ; i += 1) {
	        bpalpha_free(&bp_bpalpha[i]) ;
	    }

	    uc_free(bp_bpalpha) ;

#ifdef	MALLOCLOG
	    malloclog_free(bp_bpalpha,"stats:bp_bpalpha") ;
#endif

	}

bad2a:
	if (f_yags) {

	    bpfifo_free(&bf_yags) ;

	    for (i = 0 ; i < p.yags_rows ; i += 1) {
	        yags_free(&bp_yags[i]) ;
	    }

	    uc_free(bp_yags) ;

#ifdef	MALLOCLOG
	    malloclog_free(bp_yags,"stats:bp_yags") ;
#endif

	}

bad2:
	if (f_vpred) {

	    vpfifo_free(&vf) ;

	    for (i = 0 ; i < p.vp_rows ; i += 1) {
	        vpred_free(&valuepred[i]) ;
	    }

	    uc_free(valuepred) ;

#ifdef	MALLOCLOG
	    malloclog_free(valuepred,"stats:valuepred") ;
#endif

	}

bad1:
	exectrace_close(&t) ;

bad0:
	return rs ;
}
/* end subroutine (stats) */


/* local subroutines */


/* execute a load type instruction */
static int loadinstr(pip,mip,opexec,ma,rval,dvp,d2p)
struct proginfo	*pip ;
LMAPPROG	*mip ;
int		opexec ;
uint		ma ;		/* memory address to load from */
uint		rval ;		/* source value */
uint		*dvp ;		/* destination value (resulting register) */
uint		*d2p ;		/* destination 2 */
{
	uint		mval1, mval2 ;
	int		rs ;

/* read the memory loacation */

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("loadinstr: ma=%08x rval=%08x\n",ma,rval) ;
#endif

	rs = lmapprog_readint(mip,(ma & (~ 3)),&mval1) ;

#if	F_MASTERDEBUG && CF_DEBUG && F_MEMFAULT
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("loadinstr: 1 lmapprog_readint() rs=%d ma=%08x\n",
	        rs,(ma & (~ 3))) ;
#endif

	if (rs < 0)
	    return rs ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("loadinstr: 1 lmapprog_readint() rs=%d ma=%08x mv=%08x\n",
	        rs,ma,mval1) ;
#endif

	switch (opexec) {

	case LEXECOP_LDC1:
	    rs = lmapprog_readint(mip,((ma + 4) & (~ 3)),&mval2) ;

#if	F_MASTERDEBUG && CF_DEBUG && F_MEMFAULT
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        debugprintf("loadinstr: 2 lmapprog_readint() rs=%d ma=%08x\n",
	            rs,((ma + 4) & (~ 3))) ;
#endif

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(7))
	        debugprintf("loadinstr: lmapprog_readint() 2 rs=%d mval2=%08x\n",
	            rs,mval2) ;
#endif

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(7))
	        debugprintf("loadinstr: got a LDC1\n") ;
#endif

	    break ;

	} /* end switch */

/* do whatever the load instruction is supposed to do */

	rs = regload(pip,opexec,ma,mval1,mval2,rval,dvp,d2p) ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("loadinstr: regload() rs=%d dreg=%08x\n",rs,*dvp) ;
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

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("regload: rval=%08x ma=%08x mv=%08x boundary=%d\n",
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

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(7))
	        debugprintf("regload: LBU ma=%08x mv=%08x rv=%08x\n",
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

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(7))
	        debugprintf("regload: LWL\n") ;
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

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(7))
	        debugprintf("regload: LWR\n") ;
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

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("regload: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (regload) */


/* do the store instructions */
static int storeinstr(pip,mip,opexec,vaddr,dv1,dv2,map,mv1p,mv2p,dpp)
struct proginfo	*pip ;
LMAPPROG	*mip ;
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

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("storeinstr: ma=%08x mv=%08x boundary=%d\n",
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

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(7))
	        debugprintf("storeinstr: got a SDC1\n") ;
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

#if	F_MASTERDEBUG && CF_DEBUG && F_MEMFAULT
	if ((pip->debuglevel >= 2) & ( rs < 0))
	    debugprintf("storeinstr: LEXEC-type deocde failure rs=%d opexec=%d\n",
	        rs,opexec) ;
#endif

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("storeinstr: lmapprog_writeint() ma=%08x mv=%08x\n",
	        vaddr,mv1) ;
#endif

	if (rs >= 0) {

	    *map = vaddr ;
	    *mv1p = mv1 ;
	    rs = lmapprog_writeint(mip,(vaddr & (~ 3)),mv1,*dpp) ;

#if	F_MASTERDEBUG && CF_DEBUG && F_MEMFAULT
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        debugprintf("storeinstr: 1 lmapprog_writeint() rs=%d ta=%08x\n",
	            rs,(vaddr & (~ 3))) ;
#endif

	    if ((rs >= 0) && f_double) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(7)) {
	            debugprintf("storeinstr: 2 lmapprog_writeint() rs=%d\n",rs) ;
	            debugprintf("storeinstr: ma=%08x mv=%08x\n",
	                (vaddr + 4),mv2) ;
	        }
#endif

	        *mv2p = mv2 ;
	        rs = lmapprog_writeint(mip,((vaddr + 4) & (~ 3)),mv2,*dpp) ;

#if	F_MASTERDEBUG && CF_DEBUG && F_MEMFAULT
	        if ((pip->debuglevel >= 2) && (rs < 0))
	            debugprintf("storeinstr: 2 lmapprog_writeint() rs=%d ta=%08x\n",
	                rs,((vaddr + 4) & (~ 3))) ;
#endif

	    }

	} /* end if */

	return (rs >= 0) ? f_double : rs ;
}
/* end subroutine (storeinstr) */


#ifdef	COMMENT

/* check for a Pseudo System Call (PSYSCALL) */
static int checkpsyscall(pip,mip,scp,ia,ta)
struct proginfo	*pip ;
LMAPPROG	*mip ;
SYSCALLS	*scp ;
uint		ia, ta ;
{
	int	rs ;

	const char	*np ;



#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("checkpsyscall: syscalls_issyscall() \n") ;
#endif

	rs = syscalls_issyscall(scp,ta,&np) ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(7))
	    debugprintf("checkpsyscall: syscalls_issyscall() rs=%d n=%s\n",
	        rs,np) ;
#endif

	rs = lmapprog_issyscall(mip,ta,&np) ;

	if (rs > 0) {

#if	F_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(7))
	        debugprintf("checkpsyscall: PSYSCALL ia=%08x ta=%08x syscall=%s\n",
	            ia,ta,np) ;
#endif

	    if ((strcmp(np,"exit") != 0) && (strcmp(np,"_exit") != 0))
	        rs = SR_NOTSUP ;

	} /* end if */

	return rs ;
}
/* end subroutine (PSYSCALL check) */

#endif /* COMMENT */


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
static int writestats(pip,smp,vsp,scp,pp)
struct proginfo		*pip ;
struct ustats		*scp ;
VPRED_STATS		*vsp ;
struct ustatemips	*smp ;
struct params		*pp ;
{
	BPRESULT	results ;

	bfile	tmpfile ;

	double	v1, v2, ave ;
	double	acc ;

	time_t	daytime ;

	int	rs, i ;

	char	tmpfname[MAXPATHLEN + 1] ;
	char	timebuf1[TIMEBUFLEN + 1], timebuf2[TIMEBUFLEN + 1] ;


/* write out the instruction (operations) frequencies */

	mkfname2(tmpfname,pip->jobname,FE_ICOUNTS) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < LEXECOP_TOTAL ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", i,scp->lexecop[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened i-count file) */

/* write out the branch-path lengths */

	mkfname2(tmpfname,pip->jobname,FE_BPLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BPSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", 
	            i,scp->bplen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened bplen file) */

/* write out the branch-target lengths */

	mkfname2(tmpfname,pip->jobname,FE_BTLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BTSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", 
	            i,scp->btlen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened btlen file) */

/* write out SS hammock branch-target lengths */

	mkfname2(tmpfname,pip->jobname,FE_HTLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BTSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", 
	            i,scp->htlen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened htlen file) */

/* write out other branch path information */

	mkfname2(tmpfname,pip->jobname,FE_BSTATS) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    double	percent, mean, var, stddev ;


	    u_time(&daytime) ;

	    bprintf(&tmpfile,"SimpleSim statistics %s\n",
	        timestr_log(daytime,tmpfname)) ;

	    bprintf(&tmpfile,"statistics for job=%s logid=%s\n",
	        pip->jobname,pip->logid) ;

	    bprintf(&tmpfile,"started %s (elapsed time %s)\n",
	        timestr_log(pip->starttime,timebuf1),
	        timestr_elapsed((daytime - pip->starttime),timebuf2)) ;

	    bputc(&tmpfile,'\n') ;


	    bprintf(&tmpfile,
	        "total instructions    %12llu\n",
	        smp->in) ;

	    bprintf(&tmpfile,"included instructions %12llu (%llu:%llu)\n",
	        scp->in,pip->in.start,scp->in) ;

	    bprintf(&tmpfile,"CF instructions       %12llu (%7.3f%%)\n",
	        scp->cf,
	        percentll(scp->cf,smp->in)) ;

	    bprintf(&tmpfile,"CF-IND instructions   %12llu (%7.3f%%)\n",
	        scp->cf_ind,
	        percentll(scp->cf_ind,smp->in)) ;

	    bprintf(&tmpfile,"relative branches     %12llu (%7.3f%%)\n",
	        scp->br_rel,
	        percentll(scp->br_rel,smp->in)) ;

	    bprintf(&tmpfile,"CF-UNC                %12llu (%7.3f%%)\n",
	        (scp->cf - scp->br_con),
	        percentll((scp->cf - scp->br_con) ,smp->in)) ;

	    bprintf(&tmpfile,"branch paths          %12llu (%7.3f%%)\n",
	        scp->br_con,
	        percentll(scp->br_con,smp->in)) ;

	    bprintf(&tmpfile,"forward c-branches    %12llu (%7.3f%%)\n",
	        scp->br_fwd,
	        percentll(scp->br_fwd,smp->in)) ;

	    if (pip->f.opt_ssh)
	        bprintf(&tmpfile,"branch SS-hammocks    %12llu (%7.3f%%)\n",
	            scp->br_ssh,
	            percentll(scp->br_ssh,smp->in)) ;

/* branch path statistics */

	    v1 = scp->br_con ;		/* conditional control-flow */
	    v2 = scp->in ;		/* total instructions (considered) */
	    ave = (scp->br_con > 0) ? (v2 / v1) : 0.0 ;
	    bprintf(&tmpfile,"instructions per BP %17.4f\n",
	        ave) ;

	    bprintf(&tmpfile,"= branch path statistics\n") ;

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

	    bprintf(&tmpfile,"= branch target-distance statistics\n") ;

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

/* branch target statistics */

	    bprintf(&tmpfile,"= hammock branch target-distance statistics\n") ;

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

/* forward conditional branches */

	    {
	        double	fa, fb ;


	        fa = scp->br_fwd ;
	        fb = scp->br_con ;
	        percent = (fa / fb) * 100.0 ;

	    } /* end block */

	    bprintf(&tmpfile,
	        "forward c-branches    %12llu (%6.2f %% of conditional)\n",
	        scp->br_fwd,percent) ;

/* ss-hammocks */

	    {
	        double	fa, fb ;


	        fa = scp->br_ssh ;
	        fb = scp->br_con ;
	        percent = (fa / fb) * 100.0 ;

	    } /* end block */

	    bprintf(&tmpfile,
	        "SS hammocks           %12llu (%6.2f %% of conditional)\n",
	        scp->br_ssh,percent) ;

/* branch prediction results */

	    rs = bpresult_open(&results,NULL) ;

/* YAGS */

	    if (pip->f.opt_yags) {

	        bprintf(&tmpfile,
	            "YAGS rows             %12d\n",
	            pp->yags_rows) ;

	        bprintf(&tmpfile,
	            "YAGS delay            %12d\n",
	            pp->yags_delay) ;

	        bprintf(&tmpfile,
	            "YAGS CPHT             %12d\n",
	            pp->yags_cpht) ;

	        bprintf(&tmpfile,
	            "YAGS DPHT             %12d\n",
	            pp->yags_dpht) ;

	        bprintf(&tmpfile,
	            "YAGS bits             %12llu\n",
	            scp->yags_bits) ;

	        acc = percentll(scp->yags_correct,scp->br_con) ;

	        bprintf(&tmpfile,
	            "YAGS accuracy         %12llu (%7.3f%%)\n",
	            scp->yags_correct,
	            acc) ;

	        bpresult_write(&results,pip->basename,acc, "yags",
			(int) scp->yags_bits,
			pp->yags_cpht,pp->yags_dpht,0,0) ;

	    } /* end if (YAGS) */

/* BPALPHA */

	    if (pip->f.opt_bpalpha) {

	        bprintf(&tmpfile,
	            "BPALPHA rows          %12d\n",
	            pp->bpalpha_rows) ;

	        bprintf(&tmpfile,
	            "BPALPHA delay         %12d\n",
	            pp->bpalpha_delay) ;

	        bprintf(&tmpfile,
	            "BPALPHA LBHT          %12d\n",
	            pp->bpalpha_lbht) ;

	        bprintf(&tmpfile,
	            "BPALPHA LPHT          %12d\n",
	            pp->bpalpha_lpht) ;

	        bprintf(&tmpfile,
	            "BPALPHA GPHT          %12d\n",
	            pp->bpalpha_gpht) ;

	        bprintf(&tmpfile,
	            "BPALPHA bits          %12llu\n",
	            scp->bpalpha_bits) ;

	        acc = percentll(scp->bpalpha_correct,scp->br_con) ;

	        bprintf(&tmpfile,
	            "BPALPHA accuracy      %12llu (%7.3f%%)\n",
	            scp->bpalpha_correct,
	            acc) ;

	        bpresult_write(&results,pip->basename,acc, "bpalpha",
			(int) scp->bpalpha_bits,
			pp->bpalpha_lbht,pp->bpalpha_lpht,pp->bpalpha_gpht,0) ;

	    } /* end if (BPALPHA) */

/* GSPAG */

	    if (pip->f.opt_gspag) {

	        bprintf(&tmpfile,
	            "GSPAG rows            %12d\n",
	            pp->gspag_rows) ;

	        bprintf(&tmpfile,
	            "GSPAG delay           %12d\n",
	            pp->gspag_delay) ;

	        bprintf(&tmpfile,
	            "GSPAG LBHT            %12d\n",
	            pp->gspag_lbht) ;

	        bprintf(&tmpfile,
	            "GSPAG GPHT            %12d\n",
	            pp->gspag_gpht) ;

	        bprintf(&tmpfile,
	            "GSPAG bits            %12llu\n",
	            scp->gspag_bits) ;

	        acc = percentll(scp->gspag_correct,scp->br_con) ;

	        bprintf(&tmpfile,
	            "GSPAG accuracy        %12llu (%7.3f%%)\n",
	            scp->gspag_correct,
	            acc) ;

	        bpresult_write(&results,pip->basename,acc, "gspag",
			(int) scp->gspag_bits,
			pp->gspag_lbht,pp->gspag_gpht,0,0) ;

	    } /* end if (GSPAG) */

/* EVEIGHT */

	    if (pip->f.opt_eveight) {

	        bprintf(&tmpfile,
	            "EVEIGHT rows          %12d\n",
	            pp->eveight_rows) ;

	        bprintf(&tmpfile,
	            "EVEIGHT delay         %12d\n",
	            pp->eveight_delay) ;

	        bprintf(&tmpfile,
	            "EVEIGHT tlen          %12d\n",
	            pp->eveight_tlen) ;

	        bprintf(&tmpfile,
	            "EVEIGHT bits          %12llu\n",
	            scp->eveight_bits) ;

	        acc = percentll(scp->eveight_correct,scp->br_con) ;

	        bprintf(&tmpfile,
	            "EVEIGHT accuracy      %12llu (%7.3f%%)\n",
	            scp->eveight_correct,
	            acc) ;

	        bpresult_write(&results,pip->basename,acc, "eveight",
			(int) scp->eveight_bits,
			pp->eveight_tlen,-1,0,0) ;

	    } /* end if (EVEIGHT) */

/* close out the branch predictor results */

	    bpresult_close(&results) ;

/* other stuff */

	    bprintf(&tmpfile,
	        "bad IAs               %12llu (%7.3f%%)\n",
	        scp->ia_bad,
	        percentll(scp->ia_bad,smp->in)) ;

/* value prediction stuff */

	    bprintf(&tmpfile,"= value prediction statistics (external)\n") ;

	    bprintf(&tmpfile,
	        "VP instruction rows   %12d\n",
	        pp->vp_rows) ;

	    bprintf(&tmpfile,
	        "VP instruction delay  %12d\n",
	        pp->vp_delay) ;

	    bprintf(&tmpfile,
	        "VP entries            %12d\n",
	        pp->vp_entries) ;

	    bprintf(&tmpfile,
	        "VP nops               %12d\n",
	        pp->vp_nops) ;

	    bprintf(&tmpfile,
	        "VP stride bits        %12d\n",
	        pp->vp_stride) ;

	    bprintf(&tmpfile,
	        "instruction lookups   %12llu\n",
	        scp->vp_inlu) ;

	    bprintf(&tmpfile,
	        "instruction LU hits   %12llu "
	        "(%6.2f %% of instructions)\n",
	        scp->vp_inhit,
	        percentll(scp->vp_inhit,scp->vp_inlu)) ;

	    bprintf(&tmpfile,
	        "both (2) ops correct  %12llu "
	        "(%6.2f %% of instructions)\n",
	        scp->vp_allcor,
	        percentll(scp->vp_allcor,scp->vp_inlu)) ;

	    bprintf(&tmpfile,
	        "operand lookups       %12llu\n",
	        scp->vp_oplu) ;

	    bprintf(&tmpfile,
	        "operand hits          %12llu "
	        "(%6.2f %% of operand lookups)\n",
	        scp->vp_ophit,
	        percentll(scp->vp_ophit,scp->vp_oplu)) ;

	    bprintf(&tmpfile,
	        "correct operand hits  %12llu "
	        "(%6.2f %% of operand lookups)\n",
	        scp->vp_opcor,
	        percentll(scp->vp_opcor,scp->vp_oplu)) ;

	    bprintf(&tmpfile,
	        "both (2) ops correct  %12llu "
	        "(%6.2f %% of operand lookups)\n",
	        scp->vp_allcor,
	        percentll(scp->vp_allcor,scp->vp_oplu)) ;

	    bprintf(&tmpfile,
	        "correct operand hits  %12llu "
	        "(%6.2f %% of all operand hits)\n",
	        scp->vp_opcor,
	        percentll(scp->vp_opcor,scp->vp_ophit)) ;


/* done */

	    bclose(&tmpfile) ;

	} /* end if (opened bstats file) */


	return SR_OK ;
}
/* end subroutine (writestats) */


/* write out value prediction statistics */
static int writevpstats(pip,smp,vsp,scp,vprow)
struct proginfo		*pip ;
struct ustats		*scp ;
VPRED_STATS		*vsp ;
struct ustatemips	*smp ;
int			vprow ;
{
	bfile	tmpfile ;

	double	v1, v2 ;

	int	rs, i ;

	char	tmpfname[MAXPATHLEN + 1] ;


	mkfname2(tmpfname,pip->jobname,FE_BSTATS) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wa",0666)) >= 0) {

	    double	percent, mean, var, stddev ;


	    bprintf(&tmpfile,
	        "= value prediction statistics (internal VPROW=%d)\n",vprow) ;

	    bprintf(&tmpfile,
	        "table entries           %12d\n",
	        vsp->tablen) ;

	    bprintf(&tmpfile,
	        "entry lookups           %12lld\n",
	        vsp->in_lu) ;

	    bprintf(&tmpfile,
	        "entry hits              %12lld\n",
	        vsp->in_hit) ;

	    bprintf(&tmpfile,
	        "entry updates           %12lld\n",
	        vsp->in_update) ;

	    bprintf(&tmpfile,
	        "entry replacements      %12lld\n",
	        vsp->in_replace) ;

	    bprintf(&tmpfile,
	        "operand lookups         %12lld\n",
	        vsp->op_lu) ;


/* done */

	    bclose(&tmpfile) ;

	} /* end if (opened bstats file) */


	return SR_OK ;
}
/* end subroutine (writevpstats) */


/* check (the hard way) for a SSH */
static int hias_check(op,ia)
struct testing	*op ;
uint		ia ;
{
	int	rs ;
	int	i ;


	rs = SR_NOENT ;
	for (i = 0 ; i < op->n ; i += 1) {

	    if (ia == op->hias[i])
	        break ;

	} /* end for */

	if (i < op->n) {

	    rs = i ;
	    op->c_ssh += 1 ;

	}

	return rs ;
}
/* end subroutine (hias_check) */


static double	percentll(in,id)
ULONG	in, id ;
{
	double	dn, dd ;
	double	dnp ;


	if (id == 0)
	    return 0.0 ;

	dn = (double) in ;
	dd = (double) id ;
	dnp = dn * 100.0 ;
	return (dnp / dd) ;
}
/* end subroutine (percentll) */


/* get options for this statistics module */
int getstatsopts(pip,kop,pp)
struct proginfo	*pip ;
KEYOPT		*kop ;
struct params	*pp ;
{
	KEYOPT_CURSOR	kcur ;

	int	rs, i, oi, val ;
	int	nlen, klen, vlen ;
	int	n ;

	char	*kp, *vp ;
	char	*cp ;


/* process all of the options that we have so far */

	n = 0 ;
	keyopt_curbegin(kop,&kcur) ;

	while ((rs = keyopt_enumkeys(kop,&kcur,&kp)) >= 0) {

	    KEYOPT_CURSOR	vcur ;

	    int	f_value = FALSE ;


	    klen = rs ;

/* get the first (non-zero length) value for this key */

	    vlen = -1 ;
	    keyopt_curbegin(kop,&vcur) ;

	    while ((rs = keyopt_enumvalues(kop,kp,&vcur,&vp)) >= 0) {

	        f_value = TRUE ;
	        vlen = rs ;
	        if (vlen > 0)
	            break ;

	    } /* end while */

	    keyopt_curend(kop,&vcur) ;

/* do we support this option ? */

	    if ((oi = optmatch3(keyopts,kp,klen)) >= 0) {

#if	F_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("getstatsopts: system valid option, oi=%d\n",
	                oi) ;
#endif

	        switch (oi) {

	        case keyopt_rows:
	            pp->rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_delay:
	            pp->delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_vprows:
	            pp->vp_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_vpdelay:
	            pp->vp_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_vpentries:
	            pp->vp_entries = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_entries = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_vpnops:
	            pp->vp_nops = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_nops = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_vpstride:
	            pp->vp_stride = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_stride = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_yagsrows:
	            pp->yags_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_yagsdelay:
	            pp->yags_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_yagscpht:
	            pp->yags_cpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_cpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_yagsdpht:
	            pp->yags_dpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_dpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_bpalpharows:
	            pp->bpalpha_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->bpalpha_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_bpalphadelay:
	            pp->bpalpha_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->bpalpha_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_bpalphalbht:
	            pp->bpalpha_lbht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->bpalpha_lbht = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_bpalphalpht:
	            pp->bpalpha_lpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->bpalpha_lpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_bpalphagpht:
	            pp->bpalpha_gpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->bpalpha_gpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_gspagrows:
	            pp->gspag_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_gspagdelay:
	            pp->gspag_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_gspaglbht:
	            pp->gspag_lbht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_lbht = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_gspaggpht:
	            pp->gspag_gpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_gpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_eveightrows:
	            pp->eveight_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->eveight_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case keyopt_eveightdelay:
	            pp->eveight_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->eveight_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        } /* end switch */

	    } /* end if (got a match) */

	} /* end while */

	keyopt_curend(kop,&kcur) ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("getstatsopts: n=%d\n",n) ;
#endif

	return n ;
}
/* end subroutine (getstatsopts) */


/* VPFIFO */
static int vpfifo_init(op,n)
struct vpfifo	*op ;
int		n ;
{
	int	rs ;
	int	size ;


	size = n * sizeof(struct vpentry) ;
	rs = uc_malloc(size,&op->table) ;

#ifdef	MALLOCLOG
	malloclog_alloc(op->table,rs,"stats/vpfifo_init:op_table") ;
#endif

	op->head = 0 ;
	op->tail = 0 ;
	op->n = n ;
	return rs ;
}
/* end subroutine (vpfifo_init) */


static int vpfifo_free(op)
struct vpfifo	*op ;
{

	if (op->table != NULL) {
	    uc_free(op->table) ;
#ifdef	MALLOCLOG
	    malloclog_free(op->table,"stats:op_table") ;
#endif
	    op->table = NULL ;
	}

	return SR_OK ;
}
/* end subroutine (vpfifo_free) */


static int vpfifo_add(op,values,n,row)
struct vpfifo	*op ;
uint		values[] ;
int		n ;
int		row ;
{
	int		i ;
	int		size ;

	i = op->tail ;
	size = n * sizeof(uint) ;
	op->table[i].n = n ;
	op->table[i].row = row ;
	(void) memcpy(op->table[i].ops,values,size) ;

	op->tail = (op->tail + 1) % op->n ;
	return SR_OK ;
}
/* end subroutine (vpfifo_add) */


static int vpfifo_remove(op,values,rp)
struct vpfifo	*op ;
uint		values[] ;
int		*rp ;
{
	int		i, n ;
	int		size ;

	i = op->head ;
	*rp = op->table[i].row ;
	n = op->table[i].n ;
	size = n * sizeof(uint) ;
	(void) memcpy(values,op->table[i].ops,size) ;

	op->head = (op->head + 1) % op->n ;
	return n ;
}
/* end subroutine (vpfifo_remove) */


