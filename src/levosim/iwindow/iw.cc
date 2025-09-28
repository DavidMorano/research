/* iw */

/* Levo Instruction Window */


#define	F_DEBUGS	0		/* non-switched debugging */
#define	F_DEBUG		0		/* switched debugging */
#define	F_SAFE		1		/* safe mode */
#define	F_SAFE2		1		/* more safe mode */
#define	F_SAFE3		1
#define	F_SAFEFUNC	0		/* call the sanity function ? */
#define	F_EXTRABUS	0		/* extra bus traffic (PATHID=-1) */
#define	F_EXTRABUSES	0		/* more than one extra traffic ? */
#define	F_INITAS	1		/* init the ASes */
#define	F_INITPE	1		/* init the PEes */
#define	F_DOAS		1		/* do the ASes */
#define	F_LLB		1		/* LLB */
#define	F_LIFETCH	1		/* LIFETCH */
#define	F_ALIRGF	1		/* the special "ALI" register file */
#define	F_ALIRGFSHIFT	0		/* does ALI regfile need shifting ? */
#define	F_EXTRABUSSHIFT	0		/* extra bus shift ? */
#define	F_MANIPULATE	1		/* manipulate the register filters */
#define	F_SHIFTLB	0		/* shift the Levo Load Buffer (LB) ? */
#define	F_LASCLOCK	1		/* clock the ASes ? */
#define	F_LPECLOCK	1		/* clock the PEs ? */
#define	F_RFCLOCK	1		/* clock the REGFILTERs ? */
#define	F_FAKEPFB	1		/* FAKE Register Predicate Buses */
#define	F_DELAYCOMMIT	1		/* delay commitment for Ali */
#define	F_GLOBALBUSID	1		/* do global bus IDs */
#define	F_FPRINTF	1		/* do some fprintf() printing */
#define	F_DETAILS	0		/* show details on print-out */
#define	F_BOTTOMREADY	1		/* need bottom REGFILTER for commit */
#define	F_LIFETCHFUNC	1		/* pass sanity func to LIFETCH */
#define	F_SETTOPRFTT	1		/* set the TT on the top RF unit */
#define	F_LBTRBTARGET	0		/* use LBTRB for target-in ? */
#define	F_IJ		0		/* special handle indirect jumps ? */
#define	F_MAPALLOC	1		/* use mapping for allocation */
#define	F_CLOCKALL	0		/* clock everything ? */
#define	F_CHECKALIRGF	1		/* check against ALIRGF ? */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


	= 00/09/18, Dave Morano

	I added the LBTRB object.


	= 00/09/28, Dave Morano

	Unfortunately much of this code with regard to initialization
	has to change !  The memory buses that are created and
	initialized in the LEVO object no longer are the same buses
	that go to the ASes.  This was expected anyway eventually so it
	may as well happen now.  New memory buses are created and
	initialized within the IW for use by the ASes.  Also all
	register and predicate buses are not entirely local to the IW
	also.  The LEVO object creates memory buses for use between
	the memory subsystem and the i-window.  Those buses are now
	made out of RFBUS objects while the memory buses that go
	to the ASes (here in this code) are still made out of BUS
	objects.


	= 00/12/30, Dave Morano

	Alireza found one problem with mutliple Sharing Groups not
	working.  This code was not calling the '_comb()' or '_clock()'
	methods of all of the LPE object !  Only the first LPE object
	was being handled correctly.  I fixed it simply with a loop to
	get all of them that are configured.


	= 01/02/20, Dave Morano

	I am just marking this version as the latest.  I fixed some
	REGFILTER manipulation issues that had to do with invalidation
	of register values and I put some extra debug prints in this
	version.


*/


/******************************************************************************

	Levo Instruction Window

	This is the top of the i-window for the Levo machine.
	We will get buses that go over to the memory subsystem.
	Buses to memory include :

		- i-fetch request bus
		- i-fetch response bus
		- memory backwarding bus (load requests)
		- memory forwarding bus (load responses)
		- memory write bus (store requests)

	All other buses include :
		- register forwarding buses
		- register backwarding buses
		- predicate forwarding buses
		- all of our internal memory forwarding & backwarding buses



******************************************************************************/


#define	IW_MASTER	1


#include	<sys/types.h>
#include	<sys/mman.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstdlib>
#include	<cstring>
#include	<malloc.h>

#if	F_FPRINTF
#include	<cstdio>
#endif

#include	<usystem.h>
#include	<bio.h>

#include	"localmisc.h"
#include	"paramfile.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"statemips.h"
#include	"lmipsregs.h"
#include	"shiftregl.h"
#include	"xmlinfo.h"

#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"bus.h"			/* forwarding register bus */
#include	"rfbus.h"		/* registered flow bus */
#include	"las.h"
#include	"lpe.h"
#include	"llb.h"
#include	"lifetch.h"
#include	"lbtrb.h"
#include	"lwq.h"
#include	"iw.h"
#include	"alirgf.h"
#include	"opclass.h"
#include	"regfilter.h"
#include	"bustrace.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	IW_MAGIC		0x91818273
#define	IW_EXTRAMASTERS		4
#define	IW_WAITCOUNT		5
#define	IW_RFTYPE		0	/* relay forwarding */
#define	IW_DEADBUILTIN		35	/* maximum instructions execution */
#define	IW_DEADCLOCKSCHECK	4	/* clocks between deadlock checks */
#define	IW_DEADCLOCKS		12	/* clocks assumed to be deadlock ! */
#define	IW_DEADCLOCKSMIN	7	/* minimum dead clocks for now */
#define	IW_MAPALLOCFILE		"/dev/zero"


/* external subroutines */

extern int	getnumbuses(uint) ;


/* forward references */

int		iw_sanitycheck(IW *) ;

static int	iw_mapalloc(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_mapfree(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_initmbuses(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_freembuses(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_setnext(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_commitcheck(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_manipulate(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_commitcheckases(IW *,struct proginfo *,struct levoinfo *,
			int) ;
static int	iw_commitchecklwq(IW *,struct proginfo *,struct levoinfo *,
			int) ;
static int	iw_readyregfilters() ;
static int	iw_commitases(IW *,struct proginfo *,struct levoinfo *,int) ;
static int	iw_loadcheck(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_lbready() ;
static int	iw_initregfilters(IW *,struct proginfo *,LSIM *,
				struct levoinfo *,uint *) ;
static int	iw_freeregfilters(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_handleshift() ;
static int	iw_targetin(IW *,struct proginfo *,struct levoinfo *,uint,int) ;
static int	iw_targetinlb(IW *,struct proginfo *,struct levoinfo *,
				uint,int *) ;
static int	iw_getrfunused(IW *,struct proginfo *,struct levoinfo *,int) ;
static int	iw_setrfs(IW *,struct proginfo *,struct levoinfo *,int,int) ;
static int	iw_initregs(IW *,struct proginfo *,struct levoinfo *,
				REGFILTER *,uint *) ;
static int	iw_checkdeadlock(IW *,struct proginfo *,struct levoinfo *) ;
static int	iw_checkexit(IW *,struct proginfo *,struct levoinfo *,
				LAS_COMMITINFO *) ;
static int	iw_flushwindow(IW *,struct proginfo *,struct levoinfo *,int) ;
static int	iw_rfdisable(IW *,struct proginfo *,struct levoinfo *,int) ;
static int	iw_rfgettop(IW *,struct proginfo *,struct levoinfo *,int) ;
static int	iw_rfalloc(IW *,struct proginfo *,struct levoinfo *,int) ;
static int	iw_rffree(IW *,struct proginfo *,struct levoinfo *,int) ;
static int	iw_killpath(IW *,struct proginfo *,struct levoinfo *,
				int,int,int) ;
static int	iw_getpathcol(IW *,struct levoinfo *,int,int) ;
static int	iw_getnextcol(IW *,struct levoinfo *,int,int) ;

static int	getbusid(struct levoinfo *,int) ;
static uint	ceiling(uint,uint) ;

#if	F_FPRINTF || F_DEBUGS || (F_MASTERDEBUG && F_DEBUG)
static int	iw_printregs(IW *,struct proginfo *,struct levoinfo *) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
static int	iw_checkregs(IW *,struct proginfo *,struct levoinfo *,
				REGFILTER *) ;
#endif


/* local variables */

static char	*const iwparams[] = {
	"iw:btrbsize",		/* Levo Branch Tracking Buffer */
	"iw:sqsize",		/* Levo Store Queue queue depth size */
	"iw:wmfinter",		/* EW MF interleave schedule */
	"iw:wmbinter",		/* EW MB interleave schedule */
	"iw:rfbtrace",		/* RFB trace */
	"iw:rbbtrace",		/* RBB trace */
	"iw:pfbtrace",		/* PFB trace */
	"iw:mfbtrace",		/* MFB trace */
	"iw:mbbtrace",		/* MBB trace */
	"iw:waitcount",		/* extra clock wait count for commitment */
	"iw:rftype",		/* register forwarding type */
	"iw:ifetchtrace",	/* i-fetch trace file */
	"iw:deadclocks",	/* clocks assumed to be deadlock ! */
	NULL
} ;

#define	IWPARAM_BTRBSIZE	0
#define	IWPARAM_SQSIZE		1
#define	IWPARAM_WMFINTER	2
#define	IWPARAM_WMBINTER	3
#define	IWPARAM_RFBTRACE	4
#define	IWPARAM_RBBTRACE	5
#define	IWPARAM_PFBTRACE	6
#define	IWPARAM_MFBTRACE	7
#define	IWPARAM_MBBTRACE	8
#define	IWPARAM_WAITCOUNT	9
#define	IWPARAM_RFTYPE		10
#define	IWPARAM_IFETCHTRACE	11
#define	IWPARAM_DEADCLOCKS	12






int iw_init(wp,pip,pfp,mip,lip,ap)
IW		*wp ;
struct proginfo	*pip ;
PARAMFILE	*pfp ;
LSIM		*mip ;
struct levoinfo	*lip ;
IW_INITARGS	*ap ;
{
	struct statemips	*smp ;

	uint	uv ;
	uint	deadclocks = IW_DEADCLOCKS ;
	uint	*regs ;

	int	rs, srs, i, j ;
	int	size ;
	int	nreqs, nreqs2 ;
	int	sl, n, asid ;
	int	iwp_btrbsize = -2 ;
	int	iwp_sqsize = -1 ;
	int	f_bad ;

	char	*rfbtrace = NULL ;
	char	*rbbtrace = NULL ;
	char	*pfbtrace = NULL ;
	char	*mfbtrace = NULL ;
	char	*mbbtrace = NULL ;
	char	*ifetchtrace = NULL ;
	char	*cp ;


	if (wp == NULL)
	    return SR_FAULT ;

	(void) memset(wp,0,sizeof(IW)) ;

	wp->magic = 0 ;
	wp->pip = pip ;
	wp->mip = mip ;
	wp->lip = lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("iw_init: entered\n") ;
	}
#endif

	(void) memset(&wp->sanity,0,sizeof(struct iw_sanitycall)) ;

/* initialize those things from the extra arguments */

	wp->sanity.func = ap->sanity.func ;
	wp->sanity.arg = ap->sanity.arg ;

	wp->smp = smp = ap->smp ;
	regs = smp->regs ;

/* just some miscellaneous stuff */

	wp->c.f.targetout = wp->n.f.targetout = FALSE ;


/* set some defaults */

	wp->wmfinter = lip->wmfinter ;
	wp->wmbinter = lip->wmbinter ;

	wp->waitcount = IW_WAITCOUNT ;
	wp->rftype = IW_RFTYPE ;


/* fetch some IW specific stuff from the parameter files */

	for (i = 0 ; iwparams[i] != NULL ; i += 1) {

	    if ((sl = paramfile_fetch(pfp,iwparams[i],NULL,&cp)) >= 0) {

	        switch (i) {

	        case IWPARAM_BTRBSIZE:
	        case IWPARAM_SQSIZE:
	        case IWPARAM_WMFINTER:
	        case IWPARAM_WMBINTER:
	        case IWPARAM_WAITCOUNT:
	        case IWPARAM_RFTYPE:
	        case IWPARAM_DEADCLOCKS:
	            rs = cfnumui(cp,sl,&uv) ;

	            if (rs >= 0) {

	                switch (i) {

	                case IWPARAM_BTRBSIZE:
	                    iwp_btrbsize = uv ;
	                    break ;

	                case IWPARAM_SQSIZE:
	                    iwp_sqsize = uv ;
	                    break ;

	                case IWPARAM_WMFINTER:
	                    wp->wmfinter = uv & (~ 3) ;
	                    break ;

	                case IWPARAM_WMBINTER:
	                    wp->wmbinter = uv & (~ 3) ;
	                    break ;

	                case IWPARAM_WAITCOUNT:
	                    wp->waitcount = uv ;
	                    break ;

	                case IWPARAM_RFTYPE:
	                    wp->rftype = uv ;
	                    break ;

	        	case IWPARAM_DEADCLOCKS:
	                    deadclocks = uv ;
	                    break ;

	                } /* end switch */

	            } /* end if (valid data) */

	            break ;

	        case IWPARAM_RFBTRACE:
	        case IWPARAM_RBBTRACE:
	        case IWPARAM_PFBTRACE:
	        case IWPARAM_MFBTRACE:
	        case IWPARAM_MBBTRACE:
	        case IWPARAM_IFETCHTRACE:
	            if (sl > 0) {

	                switch (i) {

	                case IWPARAM_RFBTRACE:
	                    rfbtrace = cp ;
	                    break ;

	                case IWPARAM_RBBTRACE:
	                    rbbtrace = cp ;
	                    break ;

	                case IWPARAM_PFBTRACE:
	                    pfbtrace = cp ;
	                    break ;

	                case IWPARAM_MFBTRACE:
	                    mfbtrace = cp ;
	                    break ;

	                case IWPARAM_MBBTRACE:
	                    mbbtrace = cp ;
	                    break ;

	                case IWPARAM_IFETCHTRACE:
	                    ifetchtrace = cp ;
	                    break ;

	                } /* end switch (inner) */

	            } /* end if */

	        } /* end switch (outer) */

	    } /* end if (got a good parameter) */

	} /* end for (parameters) */


/* apply some defaults as necessary */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: 1 btrbsize=%d\n",iwp_btrbsize) ;
#endif

	if (iwp_btrbsize < -1)
	    iwp_btrbsize = LEVOINFO_BTRBSIZE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: 2 btrbsize=%d\n",iwp_btrbsize) ;
#endif

	if (wp->rftype > 1)
		wp->rftype = 1 ;

	if (deadclocks < IW_DEADCLOCKSMIN)
		deadclocks = IW_DEADCLOCKSMIN ;

	deadclocks += (wp->waitcount + IW_DEADBUILTIN) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("iw_init: rfbtrace=%s\n",rfbtrace) ;
	    eprintf("iw_init: waitcount=%d\n",wp->waitcount) ;
	    eprintf("iw_init: rftype=%d\n",wp->rftype) ;
	}
#endif /* F_DEBUG */


/* calculate the number of the MF and MB buses in the execution window */

	wp->nwmfbuses = getnumbuses(wp->wmfinter) ;

	wp->nwmbbuses = getnumbuses(wp->wmbinter) ;


/* fix up some FAKE problems */

#if	F_FAKEPFB
	lip->npfspan = lip->nsg - 1 ;
#endif


/* get some variables that we use like water */

	n = lip->nsgcols ;

	size = 2 * n * sizeof(struct iw_col) ;
	rs = uc_malloc(size,&wp->a.sgcols) ;

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(wp->a.sgcols,size,"iw_init:sgcols") ;
#endif

	wp->c.sgcols = wp->a.sgcols + 0 ;
	wp->n.sgcols = wp->a.sgcols + n ;

	(void) memset(wp->a.sgcols,0,size) ;


	for (i = 0 ; i < n ; i += 1) {

	    wp->c.sgcols[i].rfti = wp->n.sgcols[i].rfti = -1 ;
	    wp->c.sgcols[i].rfbi = wp->n.sgcols[i].rfbi = -1 ;

	} /* end if */


/* make our own structures to track the i-window AS columns */

	n = lip->totalcols ;

	size = 2 * n * sizeof(struct iw_col) ;
	rs = uc_malloc(size,&wp->a.cols) ;

	if (rs < 0)
	    goto bad1 ;

#ifdef	MALLOCLOG
	malloclog_alloc(wp->a.cols,size,"iw_init:cols") ;
#endif

	wp->c.cols = wp->a.cols + 0 ;
	wp->n.cols = wp->a.cols + n ;

	(void) memset(wp->a.cols,0,size) ;


	for (i = 0 ; i < n ; i += 1) {

#ifdef	COMMENT
	    wp->c.cols[i].path = wp->n.cols[i].path = 0 ;
#else
	    wp->c.cols[i].path = wp->n.cols[i].path = -1 ;
#endif
	    wp->c.cols[i].tt = wp->n.cols[i].tt = -1 ;

	} /* end if */


/* create the structure for tracking paths */

	n = LEVOINFO_NPATHS ;

	size = 2 * n * sizeof(struct iw_path) ;
	rs = uc_malloc(size,&wp->a.paths) ;

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_alloc(wp->a.paths,size,"iw_init:paths") ;
#endif

	wp->c.paths = wp->a.paths + 0 ;
	wp->n.paths = wp->a.paths + n ;

	(void) memset(wp->a.paths,0,size) ;


	for (i = 0 ; i < n ; i += 1) {

	    wp->c.paths[i].sgci = wp->n.paths[i].sgci = 0 ;
	    wp->c.paths[i].ci = wp->n.paths[i].ci = 0 ;
	    wp->c.paths[i].ncols = wp->n.paths[i].ncols = 0 ;

	} /* end if */


/* create register forwarding & backwarding & predicate forwarding buses */

	lip->nsgbuses = lip->nsgrows * lip->nsgcols ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: nsgrows=%d nsgcols=%d nsgbuses=%d\n",
	        lip->nsgrows,lip->nsgcols,
	        lip->nsgbuses) ;
#endif

	size = lip->nsgbuses * sizeof(BUS) ;
	rs = uc_malloc(size,&wp->rfbuses) ;

	if (rs < 0)
	    goto bad4 ;

#ifdef	MALLOCLOG
	malloclog_alloc(wp->rfbuses,size,"iw_init:rfbuses") ;
#endif

	size = lip->nsgbuses * sizeof(BUS) ;
	rs = uc_malloc(size,&wp->rbbuses) ;

	if (rs < 0)
	    goto bad5 ;

#ifdef	MALLOCLOG
	malloclog_alloc(wp->rbbuses,size,"iw_init:rbbuses") ;
#endif

	size = lip->nsgbuses * sizeof(BUS) ;
	rs = uc_malloc(size,&wp->pfbuses) ;

	if (rs < 0)
	    goto bad6 ;

#ifdef	MALLOCLOG
	malloclog_alloc(wp->pfbuses,size,"iw_init:pfbuses") ;
#endif

/* calculate number of possible masters for register & predicate buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bus_init()s\n") ;
#endif

/****
	Each AS in a SG is a bus master along with up to two REGFILTER
	units.  There are other readers but they do not write the bus !
****/

/* buses with "spans" */

#if	F_GLOBALBUSID
	{
	    int	na, nb, nc ;

/* per sharing group */

	    na = (lip->nasrpsg * LEVOINFO_NASWPSG) + 2 ;

/* per column */

	    nb = (na * lip->nsgrows) + 1 ;

/* per execution window */

	    nc = nb + 1 ;

	    nreqs = nc ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: old global bus ID rules, nreqs=%d\n",
			nreqs) ;
#endif

	    nreqs = getbusid(lip,-1) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: new global bus ID rules, nreqs=%d\n",
			nreqs) ;
#endif

	} /* end block */

#else

	nreqs = (lip->nasrpsg * LEVOINFO_NASWPSG) + 2 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: using local bus ID rules, nreqs=%d\n",nreqs) ;
#endif

#endif /* F_GLOBALBUSID */

/* buses without "spans" AND without (filter or forwarding units) */

	nreqs2 = (lip->totalrows + 1) * lip->totalcols + 2 ;

/* loop through initializing the in-execution-window buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: execution window buses nreqs=%d\n",
	        nreqs) ;
#endif

	for (i = 0 ; i < lip->nsgbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: i=%d RFB=%08lx\n",
	            i, (wp->rfbuses + i)) ;
#endif

	    rs = bus_init(wp->rfbuses + i,pip,nreqs,1) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: RFB rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: i=%d RBB=%08lx\n",
	            i, (wp->rbbuses + i)) ;
#endif

	    rs = bus_init(wp->rbbuses + i,pip,nreqs,1) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: RBB rs=%d\n",rs) ;
#endif

	    if (rs < 0) {

	        bus_free(wp->rfbuses + i) ;

	        break ;
	    }

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: i=%d PFB=%08lx\n",
	            i, (wp->pfbuses + i)) ;
#endif

	    rs = bus_init(wp->pfbuses + i,pip,nreqs,1) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: PFB rs=%d\n",rs) ;
#endif

	    if (rs < 0) {

	        bus_free(wp->rfbuses + i) ;

	        bus_free(wp->rbbuses + i) ;

	        break ;
	    }

	} /* end for (initializing) */

	if (rs < 0) {

	    for (j = 0 ; j < i ; j += 1) {

	        bus_free(wp->rfbuses + j) ;

	        bus_free(wp->rbbuses + j) ;

	        bus_free(wp->pfbuses + j) ;

	    }

	    goto bad7 ;

	} /* end if (initialization failure cleanup) */


/* do we want to trace any of these buses above ? */

	if (rfbtrace != NULL) {

	    wp->f.rfbtrace = TRUE ;
	    rs = bustrace_init(&wp->rfbtrace,pip,mip,rfbtrace,
	        wp->rfbuses,lip->nsgbuses) ;

	}

	if (rbbtrace != NULL) {

	    wp->f.rbbtrace = TRUE ;
	    rs = bustrace_init(&wp->rbbtrace,pip,mip,rbbtrace,
	        wp->rbbuses,lip->nsgbuses) ;

	}

	if (pfbtrace != NULL) {

	    wp->f.pfbtrace = TRUE ;
	    rs = bustrace_init(&wp->pfbtrace,pip,mip,pfbtrace,
	        wp->pfbuses,lip->nsgbuses) ;

	}


/* create all of the memory forwarding and backwarding buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: iw_initmbuses()\n") ;
#endif

	rs = iw_initmbuses(wp,pip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: iw_initmbuses() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad8 ;


/* what about memory bus traces */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: memory bus traces ?\n") ;
#endif

	if (mfbtrace != NULL) {

	    wp->f.mfbtrace = TRUE ;
	    rs = bustrace_init(&wp->mfbtrace,pip,mip,mfbtrace,
	        wp->mfbuses,wp->nwmfbuses) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: mfbtrace rs=%d\n",rs) ;
#endif

	    if (rs < 0)
		goto bad8a ;

	}

	if (mbbtrace != NULL) {

	    wp->f.mbbtrace = TRUE ;
	    rs = bustrace_init(&wp->mbbtrace,pip,mip,mbbtrace,
	        wp->mbbuses,wp->nwmbbuses) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: mbbtrace rs=%d\n",rs) ;
#endif

	    if (rs < 0)
		goto bad8b ;

	}


/* start in with some of the subcomponets */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: instantiating some subcomponents\n") ;
#endif

	rs = iw_mapalloc(wp,pip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: iw_mapalloc() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		goto bad9d ;



/* OK, now initialize and connect them up */


/* the LPEs */

#if	F_INITPE

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: initing the PEs\n") ;
#endif

	n = lip->nsg ;
	for (i = 0 ; i < n ; i += 1) {

	    rs = lpe_init(wp->lpes + i, pip,mip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: LPE=%08lx lpe_init() rs=%d\n",
	            (wp->lpes + i),rs) ;
#endif

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0) {

	    for (j = 0 ; j < i ; j += 1)
	        lpe_free(wp->lpes + j) ;

	    goto bad11 ;
	}

#endif /* F_INITPE */


#if	F_INITAS

/**** initialize the LASes, with :
	
	- object pointer
	- 'proginfo'
	- 'mintinfo'
	- 'levoinfo'
	- unique identification of this AS
	- number of predicate register sets
	- physical column index
	- physical SG index
	- Levo instruction load buffer (LLB) pointer
	- PE pointer
	- index to local RFB
	- index to local RBB
	- index to local PFB
	- pointer to MFBes (wp->nwmfbuses of them)
	- pointer to MBBes (wp->nwmbbuses of them)
	- bus ID for buses (all of them right now)

****/

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: initializing the ASes\n") ;
#endif

/* let's try and pop them one column at a time ! */

	asid = 0 ;

	{
	    LAS_INITARGS	las_a ;

	    int	busid ;
	    int	sgid ;
	    int	sgid_row, sgid_col ;
	    int	sgid_old = -1 ;


/* initialize the argument for the ASes */

	    (void) memset(&las_a,0,sizeof(LAS_INITARGS)) ;

#if	F_GLOBALBUSID
	    busid = getbusid(lip,0) ;
#endif

#if	F_ALIRGF
	    las_a.alirgf = &wp->alirgf ;
#endif

		las_a.rftype = wp->rftype ;

	    las_a.llbp = &wp->ourload ;
	    las_a.lwqp = &wp->wqueue ;
	    las_a.btrb = &wp->lbtrb ;

	    las_a.rfbuses = wp->rfbuses ;
	    las_a.rbbuses = wp->rbbuses ;
	    las_a.pfbuses = wp->pfbuses ;

	    las_a.mfbuses = wp->mfbuses ;
	    las_a.mbbuses = wp->mbbuses ;

	    las_a.mfinter = wp->wmfinter ;
	    las_a.mbinter = wp->wmbinter ;

/* stuff for the EXTRASAFEty mode */

	    las_a.sanity.func = iw_sanitycheck ;
	    las_a.sanity.arg = wp ;

/* go through the initialization (connection) loops */

	    for (j = 0 ; j < lip->totalcols ; j += 1) {

	        sgid_col = j / LEVOINFO_NASWPSG ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_init: pci=%d sgid_col=%d\n",j,sgid_col) ;
#endif

	        for (i = 0 ; i < lip->totalrows ; i += 1) {

	            sgid_row = i / lip->nasrpsg ;
	            sgid = sgid_row + (sgid_col * lip->nsgrows) ;

/* are we entering a new Sharing Group ? */

	            if (sgid != sgid_old) {

	                sgid_old = sgid ;

	                n = (j % LEVOINFO_NASWPSG) ;
#if	(! F_GLOBALBUSID)
	                busid = lip->nasrpsg * (LEVOINFO_NASWPSG - n) ;
#endif /* (! F_GLOBALBUSID) */

	            } /* end if (new Sharing Group) */

	            busid -= 1 ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1) {
	                eprintf("iw_init: las_init() ASID=%d SGID=%d\n",
	                    asid,sgid) ;
	                eprintf("iw_init: LAS(%d)=%08lx\n",
	                    asid,wp->lases + asid) ;
	                eprintf("iw_init: sgid_row=%d bmid=%d\n",
	                    sgid_row,busid) ;
	            }
#endif /* F_DEBUG */

	            las_a.asid = asid ;		/* AS ID */
	            las_a.psgi = sgid ;		/* physical SG index */
	            las_a.pci = j ;		/* physical column index */
	            las_a.lpes = wp->lpes + sgid ;
	            las_a.busid = busid ;
	            las_a.rfbi = sgid ;
	            las_a.rbbi = sgid ;
	            las_a.pfbi = sgid ;

	            rs = las_init(wp->lases + asid, pip,mip,lip,
	                &las_a) ;

	            if (rs < 0)
	                break ;

	            asid += 1 ;

	        } /* end for */

	        if (rs < 0)
	            break ;

	    } /* end for */

	} /* end block */

	if (rs < 0) {

	    for (j = 0 ; j < asid ; j += 1)
	        las_free(wp->lases + j) ;

	    goto bad12 ;

	} /* end if */

#endif /* F_INITAS */


/* initialize the register filters */

	rs = iw_initregfilters(wp,pip,mip,lip,regs) ;

	if (rs < 0)
	    goto bad13 ;


/* Levo Branch Tracking Buffer */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: LBTRB initialization\n") ;
#endif

	rs = lbtrb_init(&wp->lbtrb,pip,mip,lip,iwp_btrbsize) ;

	if (rs < 0)
	    goto bad14 ;

	lip->lbtrbp = &wp->lbtrb ;


/* Levo Store Queue (be careful to only use the CORRECT buses !) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: LWQ initialization\n") ;
#endif

	{
	    LWQ_INITARGS	lwq_a ;


	    (void) memset(&lwq_a,0,sizeof(LWQ_INITARGS)) ;

/* LEVO object create buses */

	    lwq_a.memwrite = lip->mwbuses ;	/* write buses */
	    lwq_a.memresponse = lip->mfbuses ;	/* load response buses */
	    lwq_a.memrequest = lip->mbbuses ;	/* load request buses */

/* IW created buses */

	    lwq_a.mfwbus = wp->mfbuses ;	/* to i-window */
	    lwq_a.mbwbus = wp->mbbuses ;	/* from i-window */

	    lwq_a.wmfinter = wp->wmfinter ;
	    lwq_a.wmbinter = wp->wmbinter ;

/* other miscellaneous */

	    lwq_a.sqsize = iwp_sqsize ;		/* LWQ FIFO queue size */

#if	F_GLOBALBUSID
	    lwq_a.busid = getbusid(lip,3) - 1 ;
#else
	    lwq_a.busid = ((lip->totalrows + 1) * lip->totalcols) - 1 ;
#endif /* F_GLOBALBUSID */

	    rs = lwq_init(&wp->wqueue,pip,mip,lip,&lwq_a) ;

	} /* end block */

	if (rs < 0)
	    goto bad15 ;


#if	F_ALIRGF

/* register file */

	rs = alirgf_init(&wp->alirgf, pip,mip,lip,regs[LMIPSREG_SP]) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("iw_init: bad alirgf() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad9a ;

#endif /* F_ALIRGF */


/* the instruction load buffer */

	rs = llb_init(&wp->ourload,pip,mip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("iw_init: bad llb_init() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad9b ;

	lip->llbp = &wp->ourload ;


/* instantiate the i-fetch unit */

#if	F_LIFETCH

	{
	    LIFETCH_INITARGS	la ;


	    (void) memset(&la,0,sizeof(LIFETCH_INITARGS)) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5) {
	        eprintf("iw_init: lifetch_init()\n") ;
	        eprintf("iw_init: entry=%08x ifetchtrace=%s\n",
	            wp->smp->ia, ifetchtrace) ;
	}
#endif
	    la.rfbuses = wp->rfbuses ;
	    la.lases = wp->lases ;
	    la.startaddr = wp->smp->ia ;
	    la.ifetchtrace = ifetchtrace ;

#if	F_LIFETCHFUNC

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("iw_init: installing SAFEFUNC for LIFETCH\n") ;
#endif

		la.sanity.func = wp->sanity.func ;
		la.sanity.arg = wp->sanity.arg ;

#endif /* F_LIFETCHFUNC */

	    if (rs >= 0)
	        rs = lifetch_init(&wp->ourfetch ,pip,mip,lip, &la) ;

	} /* end block */

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("iw_init: bad lifetch_init() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad9c ;

#endif /* F_LIFETCH */


/* do some testing stuff */

#if	F_EXTRABUS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: extra bus traffic initialization\n") ;
#endif

	wp->mytests = NULL ;

/* put a BUSTEST on each of the the MFBes */

#if	F_EXTRABUSES
	    wp->ntestbuses = lip->nmfbuses ;
#else
	wp->ntestbuses = 1 ;
#endif

	n = wp->ntestbuses ;
	size = n * sizeof(BUSTEST) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: allocating BUSTESTs n=%d size=%d\n",
		lip->nmfbuses,size) ;
#endif

	rs = uc_malloc(size,&wp->mytests) ;

	if (rs < 0)
		goto bad20 ;

#if	MALLOCLOG
	if (rs >= 0)
	malloclog_alloc(wp->mytests,size,"iw_init:mytests") ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_init: trying extra bus load on %d buses\n",n) ;
#endif

	    (void) memset(wp->mytests,0,size) ;

	    for (i = 0 ; i < n ; i += 1) {

	        rs = bustest_init(wp->mytests + i,pip,mip,lip,
			wp->mfbuses + i,
	            ((lip->totalrows + 1) * lip->totalcols),
	            IW_EXTRAMASTERS,8) ;

	        if ((rs == SR_FAULT) || (rs == SR_BADFMT) || 
	            (rs == SR_NOTOPEN)) {

	            eprintf("iw_init: BUSTEST(%d)=%08lx bad format rs=%d\n",
	                i,wp->mytests + i,rs) ;

	            break ;
	        }

	    } /* end for */

	if (rs < 0) {

		n = i ;
		for (i = 0 ; i < n ; i += 1)
	        	bustest_free(wp->mytests + i) ;

		goto bad21 ;
	}
#endif /* F_EXTRABUS */


/* set some additional variables ("Ali" specials !) */

	wp->c.faddress = wp->n.faddress = -1 ;


/* clean all statistics */

	(void) memset(&wp->s,0,sizeof(struct iw_statistics)) ;


/* maybe start something off */

	rs = lifetch_loadstatus(&wp->ourfetch,1) ;

	if (rs < 0)
		goto bad22 ;


/* make a shift register for deadlock analysis */

	n = ((deadclocks % IW_DEADCLOCKSCHECK) > 0) ? 1 : 0 ;
	n += (deadclocks / IW_DEADCLOCKSCHECK) ;
	rs = shiftregl_init(&wp->ba,n) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: shiftregl_init() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		goto bad23 ;

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&wp->c,sizeof(struct iw_state),&wp->c.checksum) ;
#endif

/* we're out of there */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: exiting OK\n") ;
#endif

	wp->magic = IW_MAGIC ;
	return rs ;

/* bad things have happened */
bad24:

#ifdef	COMMENT
	shiftregl_free(&wp->ba) ;
#endif

bad23:

bad22:
	if (wp->mytests != NULL) {

	for (i = 0 ; i < wp->ntestbuses ; i += 1)
	        	bustest_free(wp->mytests + i) ;

	}

bad21:
	if (wp->mytests != NULL)
		free(wp->mytests) ;

bad20:

#if	F_LIFETCH
	(void) lifetch_free(&wp->ourfetch) ;
#endif

bad9c:
	(void) llb_free(&wp->ourload) ;

bad9b:
	(void) alirgf_free(&wp->alirgf) ;

bad9a:
	lwq_free(&wp->wqueue) ;

bad15:
	lbtrb_free(&wp->lbtrb) ;

bad14:
	iw_freeregfilters(wp,pip,lip) ;

bad13:

#if	F_INITAS
	for (j = 0 ; j < (lip->totalrows * lip->totalcols) ; j += 1)
	    las_free(wp->lases + j) ;
#endif /* F_INITAS */

bad12:

#if	F_INITPE
	for (j = 0 ; j < lip->nsg ; j += 1)
	    lpe_free(wp->lpes + j) ;
#endif /* F_INITPE */

bad11:

#ifdef	COMMENT
	free(wp->lpes) ;
#endif

bad10:

#ifdef	COMMENT
	free(wp->lases) ;
#endif

	iw_mapfree(wp,pip,lip) ;

bad9d:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 9d\n") ;
#endif

	if (wp->f.mbbtrace)
	bustrace_free(&wp->mbbtrace) ;

bad8b:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 8b\n") ;
#endif

	if (wp->f.mfbtrace)
	bustrace_free(&wp->mfbtrace) ;

bad8a:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 8a\n") ;
#endif

	iw_freembuses(wp,pip,lip) ;

bad8:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 8\n") ;
#endif

	if (wp->f.rfbtrace)
	    bustrace_free(&wp->rfbtrace) ;

	if (wp->f.rbbtrace)
	    bustrace_free(&wp->rbbtrace) ;

	if (wp->f.pfbtrace)
	    bustrace_free(&wp->pfbtrace) ;

bad7a:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 7a\n") ;
#endif

	for (i = 0 ; i < lip->nsgbuses ; i += 1) {

	    bus_free(wp->rfbuses + i) ;

	    bus_free(wp->rbbuses + i) ;

	    bus_free(wp->pfbuses + i) ;

	} /* end for */

bad7:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 7\n") ;
#endif

	free(wp->pfbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->pfbuses,"iw_init:pfbuses") ;
#endif

bad6:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 6\n") ;
#endif

	free(wp->rbbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->rbbuses,"iw_init:rbbuses") ;
#endif

bad5:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 5\n") ;
#endif

	free(wp->rfbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->rfbuses,"iw_init:rfbuses") ;
#endif

bad4:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 4\n") ;
#endif

bad3:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 3\n") ;
#endif

	if (wp->a.paths != NULL) {

	    free(wp->a.paths) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.paths,"iw_init:paths") ;
#endif

	}

bad2:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 2\n") ;
#endif

	if (wp->a.cols != NULL) {

	    free(wp->a.cols) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.cols,"iw_init:cols") ;
#endif

	}

bad1:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: bad 1\n") ;
#endif

	if (wp->a.sgcols != NULL) {

	    free(wp->a.sgcols) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.sgcols,"iw_init:sgcols") ;
#endif

	}

bad0:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_init: exiting BAD rs=%d\n",rs) ;
#endif

	wp->magic = IW_MAGIC ;
	return rs ;
}
/* end subroutine (iw_init) */


/* free up this object */
int iw_free(wp)
IW	*wp ;
{
	struct proginfo		*pip ;

	struct statemips	*smp ;

	struct levoinfo		*lip ;

	int	rs, i ;
	int	n ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != IW_MAGIC) && (wp->magic != 0)) {

	    eprintf("iw_free: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != IW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: entered\n") ;
#endif


/* before we go to far, write back the final instruction number */

	smp = wp->smp ;

	smp->in = wp->s.commit_ienabled ;


/* free up statistics stuff */

	shiftregl_free(&wp->ba) ;


/* free up any test stuff */

#if	F_EXTRABUS
	if (wp->mytests != NULL) {

	    n = wp->ntestbuses ;
	    for (i = 0 ; i < n ; i += 1) {

	        rs = bustest_free(wp->mytests + i) ;

#if	F_MASTERDEBUG && F_SAFE2
	        if ((rs == SR_FAULT) || (rs == SR_BADFMT) || 
	            (rs == SR_NOTOPEN)) {

	            eprintf("iw_free: BUSTEST(%d)=%08lx bad format rs=%d\n",
	                i,wp->mytests + i,rs) ;

	            return rs ;
	        }
#endif /* F_SAFE2 */

	    } /* end for */

	    free(wp->mytests) ;

	}
#endif /* F_EXTRABUS */


/* free up machine compoent stuff */

#if	F_LIFETCH
	lifetch_free(&wp->ourfetch) ;
#endif


#if	F_LLB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: llb_free()\n") ;
#endif

	llb_free(&wp->ourload) ;
#endif

	alirgf_free(&wp->alirgf) ;


/* free up the Levo Store Queue */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: lwq_free()\n") ;
#endif

	lwq_free(&wp->wqueue) ;


/* the Levo Branch Tracking Buffer */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: lbtrb_free()\n") ;
#endif

	lbtrb_free(&wp->lbtrb) ;


/* free up the register filters */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: iw_freeregfilters()\n") ;
#endif

	rs = iw_freeregfilters(wp,pip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: iw_freeregfilters() rs=%d\n",rs) ;
#endif


/* free up the LPE and LPASes */

#if	F_INITAS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: freeing ASes\n") ;
#endif

	n = lip->totalrows * lip->totalcols ;
	for (i = 0 ; i < n ; i += 1)
	    las_free(wp->lases + i) ;

#ifdef	COMMENT
	free(wp->lases) ;
#endif

#endif /* F_INITAS */

#if	F_INITPE

	for (i = 0 ; i < lip->nsg ; i += 1)
		lpe_free(wp->lpes + i) ;

#endif /* F_INITPE */


	iw_mapfree(wp,pip,lip) ;


/* memory oriented tracing */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: MFB memory tracing\n") ;
#endif

	if (wp->f.mfbtrace)
	    bustrace_free(&wp->mfbtrace) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: MBB memory tracing\n") ;
#endif

	if (wp->f.mbbtrace)
	    bustrace_free(&wp->mbbtrace) ;

/* free up the execution window memory buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: iw_freembuses()\n") ;
#endif

	iw_freembuses(wp,pip,lip) ;


/* register oriented tracing */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: register tracing\n") ;
#endif

	if (wp->f.rfbtrace)
	    bustrace_free(&wp->rfbtrace) ;

	if (wp->f.rbbtrace)
	    bustrace_free(&wp->rbbtrace) ;

	if (wp->f.pfbtrace)
	    bustrace_free(&wp->pfbtrace) ;

/* free up the register & predicate buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: execution window RF, RB, PF buses\n") ;
#endif

	for (i = 0 ; i < lip->nsgbuses ; i += 1) {

	    bus_free(wp->rfbuses + i) ;

	    bus_free(wp->rbbuses + i) ;

	    bus_free(wp->pfbuses + i) ;

	} /* end for */

	free(wp->pfbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->pfbuses,"iw_free:pfbuses") ;
#endif

	free(wp->rbbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->rbbuses,"iw_free:rbbuses") ;
#endif

	free(wp->rfbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->rfbuses,"iw_free:rfbuses") ;
#endif


/* free up our own structures */

	if (wp->a.paths != NULL) {

	    free(wp->a.paths) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.paths,"iw_free:paths") ;
#endif

	}

	if (wp->a.cols != NULL) {

	    free(wp->a.cols) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.cols,"iw_free:cols") ;
#endif

	}

	if (wp->a.sgcols != NULL) {

	    free(wp->a.sgcols) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.sgcols,"iw_free:sgcols") ;
#endif

	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_free: exiting\n") ;
#endif

	wp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (iw_free) */


/* combinatorial logic */
int iw_comb(wp,phase)
IW	*wp ;
int	phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i, j ;
	int	rs1 ;
	int	n ;


#if	F_DEBUGS
	eprintf("iw_comb: special entered IW=%08lx\n",wp) ;
#endif

#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != IW_MAGIC) && (wp->magic != 0)) {

	    eprintf("iw_comb: IW=%08lx bad format\n",wp) ;

	    return SR_BADFMT ;
	}

	if (wp->magic != IW_MAGIC) {

	    eprintf("iw_comb: IW=%08lx not open\n",wp) ;

	    return SR_NOTOPEN ;
	}
#endif /* F_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: entered ck=%lld ph=%d\n",
	        mip->clock,phase) ;
#endif

/* local sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 0 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* pop the i-fetch unit */

#if	F_LIFETCH

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_comb: lifetch_comb()\n") ;
#endif

	rs = lifetch_comb(&wp->ourfetch,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	    eprintf("iw_comb: LIFETCH=%08lx bad format rs=%d\n",
	        &wp->ourfetch,rs) ;

	    return rs ;
	}
#endif /* F_SAFE2 */

	if (rs < 0)
	    return rs ;

/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 1 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */

#endif /* F_LIFETCH */


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* pop the i-load buffer */

#if	F_LLB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_comb: llb_comb()\n") ;
#endif

	rs = llb_comb(&wp->ourload,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	    eprintf("iw_comb: LLB=%08lx bad format rs=%d\n",
	        &wp->ourload,rs) ;

	    return rs ;
	}
#endif /* F_SAFE2 */

	if (rs < 0)
	    return rs ;

/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 2 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */

#endif /* F_LLB */


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* pop the register oriented buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2) {
	    eprintf("iw_comb: doing register & predicate buses, n=%d\n",
	        lip->nsgbuses) ;
	    eprintf("iw_comb: pip=%08lx\n",pip) ;
	}
#endif /* F_DEBUG */

	for (i = 0 ; i < lip->nsgbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: RFB=%08lx (%d)\n",(wp->rfbuses + i),i) ;
#endif

	    rs = bus_comb(wp->rfbuses + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_comb: BUS=%08lx bad format rs=%d\n",
	            wp->rfbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: RBB\n") ;
#endif

	    rs = bus_comb(wp->rbbuses + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_comb: BUS=%08lx bad format rs=%d\n",
	            wp->rbbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: PFB\n") ;
#endif

	    rs = bus_comb(wp->pfbuses + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_comb: BUS=%08lx bad format rs=%d\n",
	            wp->pfbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0)
	    return rs ;


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* register oriented tracing */

	if (wp->f.rfbtrace)
	    bustrace_comb(&wp->rfbtrace,phase) ;

	if (wp->f.rbbtrace)
	    bustrace_comb(&wp->rbbtrace,phase) ;

	if (wp->f.pfbtrace)
	    bustrace_comb(&wp->pfbtrace,phase) ;


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 3 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* pop the memory oriented buses */

/* the execution window memory forwarding buses */

	for (i = 0 ; i < wp->nwmfbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: MFB\n") ;
#endif

	    rs = bus_comb(wp->mfbuses + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_comb: BUS=%08lx bad format rs=%d\n",
	            wp->mfbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

	    if (rs < 0)
	        break ;

	} /* end for (execution window memory forwarding buses) */

	if (rs < 0)
	    return rs ;

/* the execution window memory backwarding buses */

	for (i = 0 ; i < wp->nwmbbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: MBB\n") ;
#endif

	    rs = bus_comb(wp->mbbuses + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_comb: BUS=%08lx bad format rs=%d\n",
	            wp->mbbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

	    if (rs < 0)
	        break ;

	} /* end for (execution window memory backwarding buses) */

	if (rs < 0)
	    return rs ;


/* memory oriented tracing */

	if (wp->f.mfbtrace)
	    bustrace_comb(&wp->mfbtrace,phase) ;

	if (wp->f.mbbtrace)
	    bustrace_comb(&wp->mbbtrace,phase) ;


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 4 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* pop the ASes */

#if	F_DOAS && F_LASCLOCK

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: doing the ASes\n") ;
#endif

#if	F_CLOCKALL
	n = lip->totalrows * lip->totalcols ;
	for (i = 0 ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: doing AS(%d)=%08lx\n",
	            i,(wp->lases + i)) ;
#endif

	    rs = las_comb(wp->lases + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_comb: las_comb() LAS bad format rs=%d\n",rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: las_comb() rs=%d\n", rs) ;
#endif

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && F_SAFE3
	    rs = iw_sanitycheck(wp) ;

	    if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 3)
	            eprintf(
			"iw_comb: bad AS(%d)=%08lx iw_sanitycheck() rs=%d\n",
	                i, (wp->lases + i) ,rs) ;
#endif

	        return rs ;
	    }
#endif /* F_SAFE3 */

	} /* end for */
#else
	n = (lip->npaths > 1) ? 1 : LEVOINFO_NASWPSG ;
	for (j = 0 ; j < lip->totalcols ; j += n) {

		for (i = 0 ; i < lip->totalrows ; i += 1) {

			int	asi ;


			asi = (j * lip->totalrows) + i ;
	    		rs = las_comb(wp->lases + asi,phase) ;

			if (rs < 0)
				break ;

		} /* end for */

		if (rs < 0)
			break ;

	} /* end for */
#endif /* F_CLOCKALL */

	if (rs < 0)
	    return rs ;

#endif /* F_DOAS && F_LASCLOCK */


#if	F_MASTERDEBUG && F_SAFE3
	rs = iw_sanitycheck(wp) ;

	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
			phase,rs) ;
#endif

	    return rs ;
	}
#endif /* F_SAFE3 */


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 5 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* pop the LPE */

#if	F_INITPE

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: doing PEs n=%d\n",lip->nsg) ;
#endif

	for (i = 0 ; i < lip->nsg ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: doing PE=%08lx\n",wp->lpes + i) ;
#endif

	    rs = lpe_comb(wp->lpes + i,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: lpe_comb() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0)
	    return rs ;

#endif /* F_INITPE */


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 6 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* pop the register filter units */

	n = (lip->nsgrows + 1) * lip->nsgcols ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: doing REGFILTERs n=%d\n",n) ;
#endif

	for (i = 0 ; i < n ; i += 1) {

	    rs = lwq_check(&wp->wqueue) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0))
	        eprintf("iw_comb: lwq_check() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf(
		    "iw_comb: doing REGFILTER(%d)=%08lx\n", 
			i,(wp->rfs + i)) ;
#endif

	    rs = regfilter_comb(wp->rfs + i,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_comb: regfilter_comb() REGFILTER=%08lx rs=%d\n",
	            (wp->rfs + i),rs) ;
#endif

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0)
	    return rs ;


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 7 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* pop the LBTRB */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: doing LBTRB\n") ;
#endif

	rs = lbtrb_comb(&wp->lbtrb,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: lbtrb_comb() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 8 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* pop the LWQ */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: checking LWQ\n") ;
#endif

	rs = lwq_check(&wp->wqueue) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: lwq_check() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: doing LWQ\n") ;
#endif

	rs = lwq_comb(&wp->wqueue,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: lwq_comb() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* extra bus load stuff */

#if	F_EXTRABUS


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 9 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_comb: extra bus stuff\n") ;
#endif

	if (wp->mytests != NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("iw_comb: doing mytests\n") ;
#endif

#if	F_EXTRABUSES
	    n = lip->nmfbuses ;
#else
	    n = 1 ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("iw_comb: doing MYTESTs n=%d\n",n) ;
#endif

	    for (i = 0 ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 4)
	            eprintf("iw_comb: doing MYTEST=%08lx (%d)\n",
	                (wp->mytests + i),i) ;
#endif

	        rs = bustest_comb(wp->mytests + i,phase) ;

#if	F_MASTERDEBUG && F_SAFE2
	        if ((rs == SR_FAULT) || (rs == SR_BADFMT) || 
	            (rs == SR_NOTOPEN)) {

	            eprintf("iw_comb: BUSTEST(%d)=%08lx bad format rs=%d\n",
	                i,wp->mytests + i,rs) ;

	            return rs ;
	        }
#endif /* F_SAFE2 */

	    } /* end for */

	}
#endif /* F_EXTRABUS */


	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
		phase,rs) ;
#endif

	if (rs < 0)
	    return rs ;


/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 10 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* do our own combinatorial logic */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf(
		"iw_comb: doing our own combinatorial, ck=%lld ph=%d\n",
	        mip->clock,phase) ;
#endif

	rs = SR_OK ;
	switch (phase) {

	case 0:
	    rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
			phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

#if	F_FPRINTF || F_DEBUGS || (F_MASTERDEBUG && F_DEBUG)

/* print out registers if we just committed in the last clock transition */

	    if (wp->c.f.committed || (mip->clock == 0)) {

	        rs = iw_printregs(wp,pip,lip) ;

	    } /* end if */

#endif /* F_FPRINTF || F_DEBUG || F_DEBUGS (printing registers) */

	    (void) iw_setnext(wp,pip,lip) ;

/* perform the deadlock checks here (doesn't really matter much) */

		if ((mip->clock % IW_DEADCLOCKSCHECK) == 0)
			rs = iw_checkdeadlock(wp,pip,lip) ;

	    break ;

/* the order of the following subroutines is important */
	case 1:
	    rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
			phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

	    rs = iw_commitcheck(wp,pip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_commitcheck() rs=%d\n",
			phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

	    wp->n.f.committed = (rs > 0) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 1) && wp->n.f.exit)
		eprintf("iw_comb: program exit signalled\n") ;
#endif

/* sanity check */

	    rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
			phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

/* check for machine loading */

	    if (! wp->n.f.exit) {

	    rs = iw_loadcheck(wp,pip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_loadcheck() rs=%d\n",phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

	    } /* end if (check for possible issue) */

/* sanity check */

	    rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
			phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

/* if we have decided to shift, signal the fetch unit to reload the LB */

	    rs = SR_OK ;

	    {
	        uint	loadstat ;


#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 2)
	            eprintf("iw_comb: f_shift=%d f_loadlb=%d\n",
	                wp->f.shift,wp->f.loadlb) ;
#endif

	        loadstat = (wp->f.shift || wp->f.loadlb) ? 1 : 0 ;
	        rs = lifetch_loadstatus(&wp->ourfetch,loadstat) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 2)
	            eprintf(
	                "iw_comb: ph=%d loadstat=%04x "
			"lifetch_loadstatus() rs=%d\n",
	                phase,loadstat,rs) ;
#endif

	    } /* end block */

	    if (rs < 0)
	        return rs ;

/* sanity check */

	    rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",
			phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

/* manipulate the register filter units (whatever that means) on a shift */

#if	F_MANIPULATE
	    rs = iw_manipulate(wp,pip,lip) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_manipulate() rs=%d\n",phase,rs) ;
#endif

#endif /* F_MANIPULATE */


	    break ;

	case 2:
	    rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

/* I think that the following has been decided to be done in the LEVO object */

#ifdef	COMMENT
	    if (wp->f.shift) {

	        rs = (*lip->machineshift)() ;

	    }
#endif /* COMMENT */

/****
	The indication of the shift to all of the machine will occur in
	this phase.  Is this OK ??
****/

	    break ;

	case 3:
	    rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",phase,rs) ;
#endif

	    if (rs < 0)
	        break ;

	    rs = iw_handleshift(wp,pip,lip) ;

	    break ;

	case 5:
		rs = wp->c.f.exit ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: simulator exit signalled \n") ;
#endif

		break ;

	} /* end switch */

	if (rs < 0)
	    goto bad ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_comb: about to exit\n") ;
#endif

/* local sanity */

	rs1 = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_comb: ph=%d iw_sanitycheck() rs=%d\n",phase,rs1) ;
#endif

	if (rs1 < 0)
		return rs1 ;

/* global sanity */

#if	F_SAFEFUNC
	if (wp->sanity.func != NULL) {

		rs1 = (*wp->sanity.func)(wp->sanity.arg) ;

		if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_comb: 11 bad sanity global rs=%d\n",
		rs1) ;
#endif

			return rs1 ;
		}
	}
#endif /* F_SAFEFUNC */


/* good things fall through here */
bad:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2) {
	    eprintf("iw_comb: ph=%d exiting rs=%d\n",
		phase,rs) ;
	    if (rs < 0)
	        eprintf("iw_comb: exiting real bad rs=%d\n",rs) ;
	}
#endif /* F_DEBUG */

	return ((rs >= 0) ? wp->c.f.exit : rs) ;
}
/* end subroutine (iw_comb) */


/* handle clocking */
int iw_clock(wp)
IW	*wp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	struct iw_col	*cp ;

	struct iw_path		*pp ;

	int	rs, i, j ;
	int	n, size ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != IW_MAGIC) && (wp->magic != 0)) {

	    eprintf("iw_clock: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != IW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_clock: entered ck=%lld\n",
	        mip->clock) ;
#endif


#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&wp->c,sizeof(struct iw_state),
	    &wp->c.checksum) ;

	if (rs < 0) {

	    eprintf("iw_clock: 1 IW=%08lx check bad format\n",
	        wp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */


/* transition all of our subobjects */


/* pop the i-fetch unit */

#if	F_LIFETCH
	rs = lifetch_clock(&wp->ourfetch) ;

	if (rs < 0)
		return rs ;

#endif


/* pop the i-load buffer */

#if	F_LLB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_clock: llb_clock()\n") ;
#endif

	rs = llb_clock(&wp->ourload) ;

	if (rs < 0)
		return rs ;


/* local sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_clock: iw_sanitycheck() rs=%d\n",rs) ;
#endif

#endif /* F_LLB */

#if	F_ALIRGF
	alirgf_clock(&wp->alirgf) ;
#endif


/* pop the register oriented buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_clock: popping the buses\n") ;
#endif

	for (i = 0 ; i < lip->nsgbuses ; i += 1) {

	    rs = bus_clock(wp->rfbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0)) {

	        eprintf("iw_clock: RFB BUS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

	    rs = bus_clock(wp->rbbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0)) {

	        eprintf("iw_clock: RBB BUS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

	    rs = bus_clock(wp->pfbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0)) {

	        eprintf("iw_clock: PFB BUS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

			if (rs < 0)
				break ;

	} /* end for */

	if (rs < 0)
		return rs ;


/* register oriented tracing */

	if (wp->f.rfbtrace)
	    bustrace_clock(&wp->rfbtrace) ;

	if (wp->f.rbbtrace)
	    bustrace_clock(&wp->rbbtrace) ;

	if (wp->f.pfbtrace)
	    bustrace_clock(&wp->pfbtrace) ;


/* pop the memory oriented buses */

/* the execution window memory forwarding buses */

	for (i = 0 ; i < wp->nwmfbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0))
	        eprintf("iw_clock: MFB\n") ;
#endif

	    rs = bus_clock(wp->mfbuses + i) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_clock: BUS=%08lx bad format rs=%d\n",
	            wp->mfbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

			if (rs < 0)
				break ;

	} /* end for */

	if (rs < 0)
		return rs ;

/* the execution window memory backwarding buses */

	for (i = 0 ; i < wp->nwmbbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0))
	        eprintf("iw_clock: MBB\n") ;
#endif

	    rs = bus_clock(wp->mbbuses + i) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_clock: BUS=%08lx bad format rs=%d\n",
	            wp->mbbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

			if (rs < 0)
				break ;

	} /* end for */

	if (rs < 0)
		return rs ;


/* memory oriented tracing */

#if	F_MASTERDEBUG && F_DEBUG
	if ((wp->f.mfbtrace || wp->f.mbbtrace) &&
		(pip->debuglevel > 4))
	    eprintf("iw_clock: memory bus tracing\n") ;
#endif

	if (wp->f.mfbtrace)
	    bustrace_clock(&wp->mfbtrace) ;

	if (wp->f.mbbtrace)
	    bustrace_clock(&wp->mbbtrace) ;


/* local sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_clock: iw_sanitycheck() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		return rs ;


/* pop the ASes */

#if	F_DOAS && F_LASCLOCK

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_clock: las_clock()\n") ;
#endif

#if	F_CLOCKALL
	n = lip->totalrows * lip->totalcols ;
	for (i = 0 ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3)
	        eprintf("iw_clock: las_clock() for ASID=%d\n",i) ;
#endif

	    rs = las_clock(wp->lases + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0)) {

	        eprintf("iw_clock: LAS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

/* local sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_clock: iw_sanitycheck() rs=%d\n",rs) ;
#endif

	} /* end for (looping through ASes) */
#else
	n = (lip->npaths > 1) ? 1 : LEVOINFO_NASWPSG ;
	for (j = 0 ; j < lip->totalcols ; j += n) {

		for (i = 0 ; i < lip->totalrows ; i += 1) {

			int	asi ;


			asi = (j * lip->totalrows) + i ;
	    		rs = las_clock(wp->lases + asi) ;

			if (rs < 0)
				break ;

		} /* end for */

		if (rs < 0)
			break ;

	} /* end for */
#endif /* F_CLOCKALL */

	if (rs < 0)
		return rs ;

#endif /* F_DOAS && F_LASCLOCK */


/* pop the LPE */

#if	F_INITPE && F_LPECLOCK

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_clock: lpe_clock()\n") ;
#endif

	for (i = 0 ; i < lip->nsg ; i += 1) {

	    rs = lpe_clock(wp->lpes + i) ;

		if (rs < 0)
			break ;

	} /* end for */

	if (rs < 0)
		return rs ;

#endif /* F_INITPE && F_LPECLOCK */


/* pop the register filter units */

#if	F_RFCLOCK

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_clock: REGFILTERs\n") ;
#endif

	n = (lip->nsgrows + 1) * lip->nsgcols ;
	for (i = 0 ; i < n ; i += 1) {

	    rs = regfilter_clock(wp->rfs + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 3) && (rs < 0))
	        eprintf("iw_clock: regfilter_clock() rs=%d\n",rs) ;
#endif

		if (rs < 0)
			break ;

	}

	if (rs < 0)
		return rs ;

#endif /* F_RFCLOCK */


/* pop the LBTRB */

	rs = lbtrb_clock(&wp->lbtrb) ;

	if (rs < 0)
		return rs ;


/* pop the LWQ */

	rs = lwq_clock(&wp->wqueue) ;

	if (rs < 0)
		return rs ;


/* extra testing stuff */

/* extra bus load stuff */

#if	F_EXTRABUS
	if (wp->mytests != NULL) {

#if	F_EXTRABUSES
	    n = lip->nmfbuses ;
#else
	    n = 1 ;
#endif

	    for (i = 0 ; i < n ; i += 1) {

	        rs = bustest_clock(wp->mytests + i) ;

#if	F_MASTERDEBUG && F_SAFE2
	        if ((rs == SR_FAULT) || (rs == SR_BADFMT) || 
	            (rs == SR_NOTOPEN)) {

	            eprintf("iw_clock: BUSTEST(%d)=%08lx bad format rs=%d\n",
	                i,wp->mytests + i,rs) ;

	            return rs ;
	        }
#endif /* F_SAFE2 */

	    } /* end for */

	}
#endif /* F_EXTRABUS */


/* transition ourselves */

#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&wp->c,sizeof(struct iw_state),
	    &wp->c.checksum) ;

	if (rs < 0) {

	    eprintf("iw_clock: 2 IW=%08lx check bad format\n",
	        wp) ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */


	{

/* save old "current state" pointers */

	    cp = wp->c.cols ;
	    pp = wp->c.paths ;

/* set the non-pointer state */

	    wp->c = wp->n ;

/* store the pointers back into the current state */

	    wp->c.cols = cp ;
	    wp->c.paths = pp ;

/* copy the column and path data from the next state to the current state */

	    size = lip->totalcols * sizeof(struct iw_col) ;
	    (void) memcpy(wp->c.cols,wp->n.cols,size) ;

	    size = LEVOINFO_NPATHS * sizeof(struct iw_path) ;
	    (void) memcpy(wp->c.paths,wp->n.paths,size) ;

	} /* end block (clocking ourselves) */

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&wp->c,sizeof(struct iw_state),&wp->c.checksum) ;
#endif



#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_clock: exiting\n") ;
#endif

	return SR_OK ;

bad:
	return rs ;
}
/* end subroutine (iw_clock) */


/* perform a machine shift at this level */
int iw_shift(wp)
IW	*wp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i ;
	int	n ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != IW_MAGIC) && (wp->magic != 0)) {

	    eprintf("iw_shift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != IW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: entered\n") ;
#endif

/* pop the i-fetch unit */

#if	F_LIFETCH && 0
	rs = lifetch_shift(&wp->ourfetch) ;
#endif


/* pop the i-load buffer */

#if	F_LLB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: llb_shift()\n") ;
#endif

#if	F_LLBSHIFT
	rs = llb_shift(&wp->ourload) ;
#endif

#endif /* F_LLB */

#if	F_ALIRGF && F_ALIRGFSHIFT
	rs = alirgf_shift(&wp->alirgf) ;
#endif


/* pop the register oriented buses ('BUS' objects DO NOT shift !) */

#ifdef	COMMENT

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: popping the buses\n") ;
#endif

	for (i = 0 ; i < lip->nsgbuses ; i += 1) {

	    rs = bus_shift(wp->rfbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (rs < 0) {

	        eprintf("iw_shift: RFB BUS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

	    rs = bus_shift(wp->rbbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (rs < 0) {

	        eprintf("iw_shift: RBB BUS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

	    rs = bus_shift(wp->pfbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (rs < 0) {

	        eprintf("iw_shift: PFB BUS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

	} /* end for */


/* pop the memory oriented buses */

/* the execution window memory forwarding buses */

	for (i = 0 ; i < wp->nwmfbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_shift: MFB\n") ;
#endif

	    rs = bus_shift(wp->mfbuses + i) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_shift: BUS=%08lx bad format rs=%d\n",
	            wp->mfbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

	}

/* the execution window memory backwarding buses */

	for (i = 0 ; i < wp->nwmbbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_shift: MBB\n") ;
#endif

	    rs = bus_shift(wp->mbbuses + i) ;

#if	F_MASTERDEBUG && F_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("iw_shift: BUS=%08lx bad format rs=%d\n",
	            wp->mbbuses + i,rs) ;

	        return rs ;
	    }
#endif /* F_SAFE2 */

	} /* end for */

#endif /* COMMENT (the BUS object doesn't "shift") */


/* pop the ASes */

#if	F_DOAS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: las_shift()\n") ;
#endif

	for (i = 0 ; i < (lip->totalrows * lip->totalcols) ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_shift: las_shift() for ASID=%d\n",i) ;
#endif

	    rs = las_shift(wp->lases + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (rs < 0) {

	        eprintf("iw_shift: LAS bad rs=%d\n",rs) ;

	        goto bad ;
	    }
#endif /* F_DEBUG */

	} /* end for */

#endif /* F_DOAS */


/* pop the LPE */

#if	F_INITPE

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: lpe_shift()\n") ;
#endif

	for (i = 0 ; i < lip->nsg ; i += 1)
	    rs = lpe_shift(wp->lpes + i) ;

#endif /* F_INITPE */


/* pop the register filter units */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: REGFILTERs \n") ;
#endif

	n = (lip->nsgrows + 1) * lip->nsgcols ;
	for (i = 0 ; i < n ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_shift: REGFILTER=%08lx i=%d\n",
	            (wp->rfs + i),i) ;
#endif

	    rs = regfilter_shift(wp->rfs + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_shift: regfilter_shift() rs=%d\n",rs) ;
#endif

	} /* end for */


/* pop the LBTRB */

	rs = lbtrb_shift(&wp->lbtrb) ;


/* pop the LWQ */

	rs = lwq_shift(&wp->wqueue) ;


/* extra testing stuff */

/* extra bus load stuff */

#if	F_EXTRABUS && F_EXTRABUSSHIFT
	if (wp->mytests != NULL) {

#if	F_EXTRABUSES
	    n = lip->nmfbuses ;
#else
	    n = 1 ;
#endif

	    for (i = 0 ; i < n ; i += 1) {

	        rs = bustest_shift(wp->mytests + i) ;

#if	F_MASTERDEBUG && F_SAFE2
	        if ((rs == SR_FAULT) || (rs == SR_BADFMT) || 
	            (rs == SR_NOTOPEN)) {

	            eprintf("iw_shift: BUSTEST(%d)=%08lx bad format rs=%d\n",
	                i,wp->mytests + i,rs) ;

	            return rs ;
	        }
#endif /* F_SAFE2 */

	    } /* end for */

	}
#endif /* F_EXTRABUS && F_EXTRABUSSHIFT */


/* our we supposed to do anything ourselves ? */


/* exit */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: exiting rs=%d\n",rs) ;
#endif

bad:
	return rs ;
}
/* end subroutine (iw_shift) */


/* someone is asking us if we think that the machine should be shifted */
int iw_needshift(wp)
IW	*wp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != IW_MAGIC) && (wp->magic != 0)) {

	    eprintf("iw_needshift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != IW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_shift: entered\n") ;
#endif


	return ((wp->f.shift) ? 1 : SR_OK) ;
}
/* end subroutine (iw_needshift) */


/* current state safety check */
int iw_sanitycheck(wp)
IW	*wp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != IW_MAGIC) && (wp->magic != 0)) {

	    eprintf("iw_sanitycheck: bad format\n") ;

	    rs = SR_BADFMT ;
	    goto bad ;
	}

	if (wp->magic != IW_MAGIC) {

	    rs = SR_NOTOPEN ;
	    goto bad ;
	}
#endif /* F_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;


#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&wp->c,sizeof(struct iw_state),
	    &wp->c.checksum) ;

	if (rs < 0) {

	    eprintf("iw_sanitycheck: 1 IW=%08lx check bad format\n",
	        wp) ;

	    rs = SR_BADFMT ;
	    goto bad ;
	}
#endif /* F_SAFE */


#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 4)
	    eprintf("iw_sanitycheck: entered\n") ;
#endif

/* check other stuff also */

	if (pip->magic != PROGMAGIC) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_sanitycheck: bad PROGMAGIC=%08x\n",pip->magic) ;
#endif

	    rs = SR_BADFMT ;
	    goto bad ;
	}

#if	F_MASTERDEBUG && F_SAFE2

/* the register forward buses */

	for (i = 0 ; i < lip->nsg ; i += 1) {

	    rs = bus_sanitycheck(wp->rfbuses + i) ;

	    if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	        eprintf("iw_sanitycheck: i=%d bad BUS=%08lx rs=%d\n",
	            i,(wp->rfbuses + i),rs) ;
#endif

	        goto bad ;
	    }

	} /* end for */

/* the ASes */

	for (i = 0 ; i < (lip->totalrows * lip->totalcols) ; i += 1) {

	    rs = las_sanitycheck(wp->lases + i) ;

	    if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	        eprintf("iw_sanitycheck: bad AS(%d)=%08lx rs=%d\n",
	            i,(wp->lases + i),rs) ;
#endif

	        goto bad ;
	    }

	} /* end for */

#endif /* F_SAFE2 */

	return SR_OK ;

bad:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_sanitycheck: exiting bad ck=%lld ph=%d rs=%d\n",
	        mip->clock,mip->phase,rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_sanitycheck) */


/* write out any statistics that we may have */
int iw_statfile(wp,fp)
IW	*wp ;
bfile	*fp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	ULONG	total ;

	int	rs = SR_OK, i ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != IW_MAGIC) && (wp->magic != 0)) {

	    eprintf("iw_statfile: bad format\n") ;

	    rs = SR_BADFMT ;
	    return rs ;
	}

	if (wp->magic != IW_MAGIC) {

	    rs = SR_NOTOPEN ;
	    return rs ;
	}
#endif /* F_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;


	bprintf(fp,"\n\ninstruction execution window:\n") ;

	{
		LLB_STATS	s ;


		rs = llb_getstats(&wp->ourload,&s) ;

	bprintf(fp,"\nload:\n") ;

	bprintf(fp,
	    "columns                            %16lld\n",
	    s.load_col) ;

	total = s.load_unused + s.load_null + s.load_instr ;
	bprintf(fp,
	    "total loads                        %16lld\n",
	    total) ;

	bprintf(fp,
	    "unused loads                       %16lld\n",
	    s.load_unused) ;

	bprintf(fp,
	    "NULL-predicate loads               %16lld\n",
	    s.load_null) ;

	bprintf(fp,
	    "instruction loads                  %16lld\n",
	    s.load_instr) ;

	} /* end block (load buffer information) */

	bprintf(fp,"\nissue:\n") ;

	bprintf(fp,
	    "columns                            %16lld\n",
	    wp->s.issue_col) ;

	total = wp->s.issue_unused + wp->s.issue_null + wp->s.issue_instr ;
	bprintf(fp,
	    "total issues                       %16lld\n",
	    total) ;

	bprintf(fp,
	    "unused issues                      %16lld\n",
	    wp->s.issue_unused) ;

	bprintf(fp,
	    "NULL-predicate issues              %16lld\n",
	    wp->s.issue_null) ;

	bprintf(fp,
	    "instruction issues                 %16lld\n",
	    wp->s.issue_instr) ;

	bprintf(fp,"\nretirement:\n") ;

	bprintf(fp,
	    "column retirements                 %16lld\n",
	    wp->s.commit_col) ;

	total = wp->s.commit_unused + wp->s.commit_null + wp->s.commit_instr ;
	bprintf(fp,
	    "total retirements                  %16lld\n",
	    total) ;

	bprintf(fp,
	    "unused instruction retirements     %16lld\n",
	    wp->s.commit_unused) ;

	bprintf(fp,
	    "NULL-predicate retirements         %16lld\n",
	    wp->s.commit_null) ;

	bprintf(fp,
	    "instruction retirements            %16lld\n",
	    wp->s.commit_instr) ;

	bprintf(fp,
	    "disabled instruction retirements   %16lld\n",
	    wp->s.commit_idisabled) ;

	bprintf(fp,
	    "enabled instruction retirements    %16lld\n",
	    wp->s.commit_ienabled ) ;

/* commitment and the CPI calculation */

	{
		double	dclocks, dinstr, cpi ;


		bprintf(fp,"\ncommitment:\n") ;

		dclocks = (double) mip->clock ;
		dinstr = wp->s.commit_ienabled ;
		cpi = dclocks / dinstr ;

		bprintf(fp,
	    	"clocks                             %16lld\n",
	    	mip->clock) ;

		bprintf(fp,
	    	"committed instructions             %16lld\n",
	    	wp->s.commit_ienabled ) ;

		if (wp->s.commit_ienabled > 0)
	    	    bprintf(fp,
	    		"CPI                                %20.3f\n",
			cpi) ;

		else
	    	    bprintf(fp,
	    		"CPI                                %16s\n",
			"+") ;

	} /* end block (CPI calculation) */

/* bus information */

	{

	bprintf(fp,"\nbuses:\n") ;



	} /* end block (bus information) */


	return rs ;
}
/* end subroutine (iw_statfile) */


/* XML output stuff */
int iw_xmlinit(wp,xip)
IW	*wp ;
XMLINFO	*xip ;
{
	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if (wp->magic != IW_MAGIC) {

		rs = (wp->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
	    return rs ;
	}
#endif /* F_SAFE */



	return SR_OK ;
}
/* end subroutine (iw_xmlinit) */


int iw_xmlout(wp,xip)
IW	*wp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK, i, j ;
	int	rs1 ;
	int	sgrow, sgcol, sgid, n ;
	int	asrow, ascol, asid ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp->magic != IW_MAGIC) {

		rs = (wp->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
	    return rs ;
	}
#endif /* F_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_xmlout: entered\n") ;
#endif

	bprintf(&xip->xmlfile,"<iw>\n") ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_xmlout: LIFETCH\n") ;
#endif

	rs = lifetch_xmlout(&wp->ourfetch,xip) ;

	if (rs < 0)
	    return rs ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_xmlout: LLB\n") ;
#endif

	rs = llb_xmlout(&wp->ourload,xip) ;

	if (rs < 0)
	    return rs ;

/* loop by sharing groups ! */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_xmlout: sharing groups\n") ;
#endif

		for (sgcol = 0 ; sgcol < lip->nsgcols ; sgcol += 1) {

	for (sgrow = 0 ; sgrow < lip->nsgrows ; sgrow += 1) {

	bprintf(&xip->xmlfile,"<sg>\n") ;

	sgid = (sgcol * lip->nsgrows) + sgrow ;

	bprintf(&xip->xmlfile,"<sgid>%d</sgid>\n",sgid) ;

/* do the ASes in columns */

	for (j = 0 ; j < LEVOINFO_NASWPSG ; j += 1) {

	bprintf(&xip->xmlfile,"<ascol>\n") ;

	bprintf(&xip->xmlfile,"<id>%d</id>\n",j) ;

	for (i = 0 ; i < lip->nasrpsg ; i += 1) {

	bprintf(&xip->xmlfile,"<aselem>\n") ;

	bprintf(&xip->xmlfile,"<id>%d</id>\n",i) ;

	asrow = (sgrow * lip->nasrpsg) + i ;
	bprintf(&xip->xmlfile,"<asrow>%d</asrow>\n",asrow) ;

	ascol = (sgcol * LEVOINFO_NASWPSG) + j ;
	asid = (ascol * lip->totalrows) + asrow ;
	bprintf(&xip->xmlfile,"<asid>%d</asid>\n",asid) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_xmlout: AS asid=%d\n",asid) ;
#endif

		rs = las_xmlout(wp->lases + asid,xip) ;

	bprintf(&xip->xmlfile,"</aselem>\n") ;

	} /* end for (AS rows in a SG) */

	bprintf(&xip->xmlfile,"</ascol>\n") ;

	} /* end for (AS columns in a SG) */

/* do the other things in a sharing group */

		rs = lpe_xmlout(wp->lpes + sgid,xip) ;


	bprintf(&xip->xmlfile,"</sg>\n") ;

	} /* end for (SG rows) */

		} /* end for (SG columns) */

	if (rs < 0)
	    return rs ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_xmlout: the other stuff\n") ;
#endif

/* pop the LWQ */

	rs = lwq_xmlout(&wp->wqueue,xip) ;

	if (rs < 0)
	    return rs ;


	bprintf(&xip->xmlfile,"</iw>\n") ;


/* good things fall through here */
bad:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_xmlout: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_xmlout) */


int iw_xmlfree(wp,xip)
IW	*wp ;
XMLINFO	*xip ;
{
	int	rs ;


#if	F_MASTERDEBUG && F_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if (wp->magic != IW_MAGIC) {


		rs = (wp->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
	    return rs ;
	}

#endif /* F_SAFE */



	return SR_OK ;
}
/* end subroutine (iw_xmlfree) */



/* PRIVATE SUBROUTINES */



/* allocate memory for some major machine components */
static int iw_mapalloc(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	size_t	fmsize ;

	off_t	fbase = 0 ;

	int	rs = SR_OK, i ;
	int	n, size, tsize ;
	int	size_ases, size_lpes ;
	int	prot, flags ;
	int	fd ;


	tsize = 0 ;

/* instantiate all PEs (only one per SG !) */

	wp->lpes = NULL ;
	n = lip->nsgrows * lip->nsgcols ;
	size_lpes = n * sizeof(LPE) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_mapalloc: sizing PEes n=%d size=%d\n",
		n,size_lpes) ;
#endif

	tsize += ceiling(size_lpes,16) ;

/* instantiate the ASes (x2 wide per SG) */

	wp->lases = NULL ;
	n = lip->totalrows * lip->totalcols ;
	size_ases = n * sizeof(LAS) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_mapalloc: sizing ASes n=%d size=%d\n",
		n,size_ases) ;
#endif

	tsize += ceiling(size_ases,16) ;

/* map something */

	wp->mapalloc = NULL ;
	wp->mapsize = tsize ;

#if	F_MAPALLOC

	if ((rs = u_open(IW_MAPALLOCFILE,O_RDWR,0666)) < 0)
		return rs ;

	fd = rs ;
	fbase = 0 ;

	        prot = PROT_READ | PROT_WRITE ;
	        flags = MAP_PRIVATE ;

	        rs = u_mapfile(NULL,wp->mapsize,prot,flags,
	            fd,fbase,&wp->mapalloc) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_mapalloc: u_mapfile() rs=%d p=%08lx\n",
		rs,wp->mapalloc) ;
#endif

	u_close(fd) ;

#else

	rs = uc_malloc(wp->mapsize,&wp->mapalloc) ;

#endif /* F_MAPALLOC */

	if (rs >= 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {

		long	pagesize ;

			char	*p ;


			p = (char *) wp->mapalloc ;
		uc_sysconf(_SC_PAGESIZE,(long *) &pagesize) ;

	    eprintf("iw_mapalloc: access test starting\n") ;

		for (i = 0 ; i < (tsize / pagesize) ; i += 1) {

			*p = 0 ;
			p += pagesize ;

		} /* end for */

	    eprintf("iw_mapalloc: access test done\n") ;

	} /* end if */
#endif /* F_DEBUG */

		size = 0 ;

		wp->lpes = (LPE *) (wp->mapalloc + size) ;
		size += ceiling(size_lpes,16) ;

		wp->lases = (LAS *) (wp->mapalloc + size) ;
		size += ceiling(size_ases,16) ;

	} /* end if */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_mapalloc: rs=%d LAS=%08lx LPE=%08lx\n",
		rs,wp->lases,wp->lpes) ;
#endif

	return rs ;
}
/* end subroutine (iw_mapalloc) */


/* free up the allocated machine component memory */
static int iw_mapfree(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK ;


	wp->lases = NULL ;
	wp->lpes = NULL ;

#if	F_MAPALLOC

	if ((wp->mapalloc != NULL) && (wp->mapsize > 0))
		rs = u_munmap(wp->mapalloc,wp->mapsize) ;

#else

	if ((wp->mapalloc != NULL) && (wp->mapsize > 0))
		free(wp->mapalloc) ;

#endif

	wp->mapalloc = NULL ;
	return rs ;
}
/* end subroutine (iw_mapfree) */


/* create and initialize the i-window memory buses */
static int iw_initmbuses(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs, i, j ;
	int	n, size ;
	int	nreqs ;


	wp->a.mbuses = NULL ;
	wp->mfbuses = NULL ;
	wp->mbbuses = NULL ;

/* create the correct number of buses */

	n = wp->nwmfbuses + wp->nwmbbuses ;
	size = n * sizeof(BUS) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_initmbuses: allocating BUSs n=%d size=%d\n",n,size) ;
#endif

	rs = uc_malloc(size,&wp->a.mbuses) ;

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(wp->a.mbuses,size,"iw/iw_initmbuses:mbuses") ;
#endif

	wp->mfbuses = wp->a.mbuses + 0 ;
	wp->mbbuses = wp->a.mbuses + wp->nwmfbuses ;

/* now do the actual initializations */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_initmbuses: MFBs %d\n",wp->nwmfbuses) ;
#endif

#if	F_GLOBALBUSID

	nreqs = getbusid(lip,-1) ;

#else

#if	F_EXTRABUS
	nreqs = ((lip->totalrows + 1) * lip->totalcols) + IW_EXTRAMASTERS ;
#else
	nreqs = (lip->totalrows + 1) * lip->totalcols ;
#endif

#endif /* F_GLOBALBUSID */

	for (i = 0 ; i < wp->nwmfbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_initmbuses: MFB %d\n",i) ;
#endif

	    rs = bus_init(wp->mfbuses + i,pip,nreqs,1) ;

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_initmbuses: MFB error rs=%d\n",rs) ;
#endif

	    for (j = 0 ; j < i ; j += 1)
	        bus_free(wp->mfbuses + i) ;

	    goto bad1 ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_initmbuses: MBBs %d\n",lip->nmbbuses) ;
#endif

	for (i = 0 ; i < wp->nwmbbuses ; i += 1) {

	    rs = bus_init(wp->mbbuses + i,pip,nreqs,1) ;

	    if (rs < 0)
	        break ;

	}

	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_initmbuses: MBB error rs=%d\n",rs) ;
#endif

	    for (j = 0 ; j < i ; j += 1)
	        bus_free(wp->mbbuses + i) ;

	    goto bad2 ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_initmbuses: exiting rs=%d\n",rs) ;
#endif

	return rs ;

/* bad things */
bad2:
	for (j = 0 ; j < wp->nwmfbuses ; j += 1)
	    bus_free(wp->mfbuses + i) ;

bad1:
	if (wp->a.mbuses != NULL) {

	    free(wp->a.mbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.mbuses,"iw/iw_initmbuses:mbuses") ;
#endif

	}

bad0:
	return rs ;
}
/* end subroutine (iw_initmbuses) */


static int iw_freembuses(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs, i ;


	for (i = 0 ; i < wp->nwmfbuses ; i += 1)
	    (void) bus_free(wp->mfbuses + i) ;

	for (i = 0 ; i < wp->nwmbbuses ; i += 1)
	    (void) bus_free(wp->mbbuses + i) ;

	free(wp->a.mbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(wp->a.mbuses,"iw/iw_freembuses:mbuses") ;
#endif

	return SR_OK ;
}
/* end subroutine (iw_freembuses) */


/* set the default next state */
static int iw_setnext(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	struct iw_cols	*cp ;

	struct iw_paths	*pp ;

	int	i ;


/* by default, the next state is the same as it was in the previous clock */

	for (i = 0 ; i < lip->totalcols ; i += 1) {

/* do not have any columns as "should be killed" status */

	    wp->n.cols[i].f.kill = FALSE ;


	} /* end for */


	wp->f.commit = FALSE ;
	wp->f.shift = FALSE ;
	wp->f.noload = FALSE ;
	wp->f.loadlb = FALSE ;
	wp->f.targetout = FALSE ;

	wp->lbi = 0 ;

	return SR_OK ;
}
/* end subroutine (iw_setnext) */


/* check if we are ready to commit a column */
static int iw_commitcheck(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	struct iw_path		*pp ;

	struct iw_col	*cp ;

	int	rs, i, j, k ;
	int	sgci, ci, n ;
	int	tt ;
	int	f_commit, f_exit ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheck: entered, c_ncols=%d\n",
		wp->c.paths[LEVOINFO_PATHML].ncols) ;
#endif

	if (wp->c.paths[LEVOINFO_PATHML].ncols <= 0)
	    return SR_OK ;

/* find the leading column for the ML path */

	ci = wp->c.paths[LEVOINFO_PATHML].ci ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheck: some ML columns loaded, at ci=%d\n",
		ci) ;
#endif


/* check it, returns TRUE if it is ready to commit */

	rs = iw_commitcheckases(wp,pip,lip,ci) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheck: iw_commitcheckases() rs=%d\n",rs) ;
#endif

	if (rs <= 0) {

	    wp->n.count = 0 ;
	    return rs ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheck: the ASes say they are ready to commit\n") ;
#endif


/* OK, is the LWQ ready to take this commitment ? */

	rs = iw_commitchecklwq(wp,pip,lip,ci) ;

	if (rs <= 0)
	    return rs ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheck: the Write Q says its ready to commit\n") ;
#endif


/* check if the REGFILTERs say they are ready */

	rs = iw_readyregfilters(wp,pip,lip,ci) ;

	if (rs <= 0) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitcheck: REGFILTERs say NO ! to commit\n") ;
#endif

	    return rs ;
	}


#if	F_DELAYCOMMIT

/* wait around for some some stuff to settle for Ali's sensibilities :-) */

	if (wp->c.count <= wp->waitcount) {

	    wp->n.count = wp->c.count + 1 ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitcheck: no commitment due to Alireza\n") ;
#endif

	    return SR_OK ;

	} /* end if (Alireza hack) */

#endif /* F_DELAYCOMMIT */


	wp->f.commit = TRUE ;


/* do the commitment with the ASes */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheck: committing the ASes ci=%d\n",ci) ;
#endif

	rs = iw_commitases(wp,pip,lip,ci) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheck: iw_commitases() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		return rs ;

	f_exit = (rs > 0) ;

/* commit the LBTRB */
/* must be called after all ASes have called lbtrb_as_commit */

	rs = lbtrb_commit(&wp->lbtrb) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1) {
	            eprintf("iw_commitcheck: lbtrb_commit() rs=%d\n",
	        	rs) ;
	            eprintf("iw_commitcheck: c_ncols=%d n_ncols=%d\n",
			wp->c.paths[LEVOINFO_PATHML].ncols,
			wp->n.paths[LEVOINFO_PATHML].ncols) ;
	}
#endif /* F_DEBUG */

	if (rs < 0)
		return rs ;


/* kill all other (DEE) paths that have a leading TT same as this column */

	tt = wp->c.cols[ci].tt ;
	j = ci ;
	for (i = 0 ; i < lip->totalcols ; i += 1) {

	    if ((wp->n.cols[j].path > 0) && (wp->n.cols[j].tt == tt)) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_commitcheck: killing path=%d col=%d\n",
	        	wp->c.cols[j].path,j) ;
#endif

	        rs = iw_killpath(wp,pip,lip,wp->c.cols[j].path,j,-1) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_commitcheck: iw_killpath() rs=%d\n",rs) ;
#endif

	        wp->n.paths[wp->c.cols[j].path].ncols = 0 ;
	        wp->n.paths[wp->c.cols[j].path].ci = -1 ;

	    } /* end if (killing off a path) */

	    j = (j + 1) % lip->totalcols ;

	} /* end for */

/* update the tracking information */

/* pop this (ML) path */

	n = wp->n.paths[LEVOINFO_PATHML].ncols - 1 ;
	wp->n.paths[LEVOINFO_PATHML].ncols = n ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitcheck: updated c_ncols=%d n_ncols=%d\n",
			wp->c.paths[LEVOINFO_PATHML].ncols,
			n) ;
#endif

	if (n > 0) {

/* find the next column that has the ML path in it */

	    rs = iw_getnextcol(wp,lip,LEVOINFO_PATHML,ci) ;

	    wp->n.paths[LEVOINFO_PATHML].ci = rs ;

	} else {

		int	nci ;


/* is this even needed ? (yes I think so) */

	    nci = (ci + LEVOINFO_NASWPSG) % lip->totalcols ;
	    wp->n.paths[LEVOINFO_PATHML].ci = nci ;

	}

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitcheck: n_ci=%d\n",
	            wp->n.paths[LEVOINFO_PATHML].ci) ;
#endif

	wp->n.paths[LEVOINFO_PATHML].sgci = 
	    (wp->c.paths[LEVOINFO_PATHML].sgci + 1) % lip->nsgcols ;

/* finally, pop the current ML column that is being committed */

	wp->n.cols[ci].path = -1 ;
	wp->n.cols[ci].f.used = FALSE ;
	wp->n.cols[ci].f.kill = TRUE ;


	return 1 ;
}
/* end subroutine (iw_commitcheck) */


/* check to see if all of the ASes in the specified column can commit */
static int iw_commitcheckases(wp,pip,lip,ci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		ci ;
{
	LAS	*lasp ;

	int	rs = SR_OK, i ;
	int	asid ;
	int	f_commit ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_commitcheckases: entered\n") ;
#endif

	f_commit = TRUE ;
	for (i = (lip->totalrows - 1) ; i >= 0 ; i -= 1) {

	    asid = (lip->totalrows * ci) + i ;
	    lasp = wp->lases + asid ;
	    rs = las_readycommit(lasp) ;

	    if ((rs < 0) && (rs != SR_EMPTY))
	        break ;

	    if (rs <= 0) {

	        f_commit = FALSE ;
	        break ;
	    }

	} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel > 1) && (rs <= 0)) {

	    if (rs < 0)
	        eprintf("iw_commitcheckases: bad LAS=%08lx i=%d rs=%d\n",
	            (wp->lases + (lip->totalrows * ci) + i),i,rs) ;

	    else
	        eprintf(
		    "iw_commitcheckases: AS not ready at row=%d asid=%d\n",
	            i,asid) ;

	}
#endif /* F_DEBUG */

	return ((rs >= 0) ? f_commit : rs) ;
}
/* end subroutine (iw_commitcheckases) */


/* see if the LWQ is ready to take a commitment */
static int iw_commitchecklwq(wp,pip,lip,ci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		ci ;
{
	int	rs = SR_BADFMT, i ;
	int	f_commit ;


	f_commit = TRUE ;

	rs = lwq_readycommit(&wp->wqueue,-1) ;

	if (rs < 0)
	    return rs ;

	f_commit = (rs > 0) ;

	if (f_commit) {

	    for (i = (lip->totalrows - 1) ; i >= 0 ; i -= 1) {

	        rs = las_used(wp->lases + (lip->totalrows * ci) + i) ;

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if (rs == INSTRCLASS_STORE) {

	            rs = lwq_readycommit(&wp->wqueue,i) ;

	            if (rs < 0)
	                break ;

	            if (rs <= 0) {

	                f_commit = FALSE ;
	                break ;
	            }

	        } else
	            rs = SR_OK ;

	    } /* end for */

#if	F_MASTERDEBUG && F_DEBUG
		if ((pip->debuglevel > 3) && (! f_commit))
			eprintf("iw_checkreadylwq: not ready at row=%d\n",
				i) ;
#endif

	} /* end if */

	return ((rs >= 0) ? f_commit : rs) ;
}
/* end subroutine (iw_commitchecklwq) */


/* check to see if all of the REGFILTERs in the specified column can commit */
static int iw_readyregfilters(wp,pip,lip,ci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		ci ;
{
	REGFILTER	*rfp ;

	int	rs, i = 0 ;
	int	sgci ;
	int	rfid ;
	int	f_commit ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_readyregfilters: entered\n") ;
#endif

	f_commit = TRUE ;

/* check the "top" one */

	sgci = ci / LEVOINFO_NASWPSG ;

/* get the current "top" REGFILTER */

#ifdef	COMMENT
	if ((rfid = wp->c.sgcols[sgci].rfti) < 0)
	    rfid = wp->c.sgcols[sgci].rfbi ;
#else
	rfid = wp->c.sgcols[sgci].rfti ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_readyregfilters: rfid=%d\n",
	        rfid) ;
#endif

	if (rfid < 0)
		return SR_INVALID ;

	rfp = wp->rfs + rfid ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_readyregfilters: i=%d REGFILTER(%d)=%08lx\n",
	        i,rfid,rfp) ;
#endif

	rs = regfilter_readycommit(rfp) ;

	if (rs < 0)
	    goto bad ;

	f_commit = (rs > 0) ;


/* check all of the others */

	if (f_commit) {

	    rfid = (lip->nsgrows + 1) * sgci + 0 ;
	    rfp = wp->rfs + rfid ;

	    for (i = 1 ; i < lip->nsgrows ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_readyregfilters: i=%d REGFILTER(%d)=%08lx\n",
	                i,(rfid + 1),(rfp + i)) ;
#endif

	        rs = regfilter_readycommit(rfp + i) ;

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if (rs <= 0) {

	            f_commit = FALSE ;
	            break ;
	        }

	    } /* end for */

	} /* end if (checking more stuff for not-readiness) */

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel > 1) && (rs <= 0)) {

	    if (rs < 0)
	        eprintf(
	            "iw_readyregfilters: bad REGFILTER=%08lx i=%d rs=%d\n",
	            rfp,i,rs) ;

	    else
	        eprintf("iw_readyregfilters: not ready to commit at %d\n",
	            i) ;

	}
#endif /* F_DEBUG */


/* what about the bottom of the column ? */

#if	F_BOTTOMREADY
	if (f_commit) {

#ifdef	COMMENT
	    sgci = (sgci + 1) % lip->nsgcols ;

	    if ((rfid = wp->c.sgcols[sgci].rfbi) < 0)
	        rfid = wp->c.sgcols[sgci].rfti ;
#else
	rfid = wp->c.sgcols[sgci].rfbi ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_readyregfilters: rfid=%d\n",
	            rfid) ;
#endif

		if (rfid < 0)
			return SR_INVALID ;

	    rfp = wp->rfs + rfid ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_readyregfilters: i=%d REGFILTER(%d)=%08lx\n",
	            i,rfid,rfp) ;
#endif

	    rs = regfilter_readycommit(rfp) ;

	    if (rs < 0)
	        goto bad ;

	    f_commit = (rs > 0) ;

	} /* end if */
#endif /* F_BOTTOMREADY */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_readyregfilters: exiting rs=%d\n",
	        ((rs >= 0) ? f_commit : rs)) ;
#endif

	return ((rs >= 0) ? f_commit : rs) ;

bad:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_readyregfilters: bad rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_readyregfilters) */


/* check if we are ready to load a column */
static int iw_loadcheck(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	struct iw_path		*pp ;

	struct iw_col	*cp ;

	LLB	*llbp ;

	LAS	*lasp ;

	int	rs, i, j ;
	int	sgci, ci, n ;
	int	f_free ;
	int	f_mlfound ;
	int	f_havefreecol ;
	int	f_branches ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: entered, c_ncols=%d n_ncols=%d\n",
		wp->c.paths[LEVOINFO_PATHML].ncols,
		wp->n.paths[LEVOINFO_PATHML].ncols) ;
#endif

/* pop out if we are not supposed to load in this clock */

	if (wp->f.noload || (wp->lbi < 0))
	    return SR_OK ;

	llbp = lip->llbp ;

/* do we have a free column for another ML load ? */

/****
	We might have a free column because a machine shift is
	going to happen on the next clock edge !
****/

	f_free = (wp->n.paths[LEVOINFO_PATHML].ncols < lip->nsgcols) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_loadcheck: freecols=%d\n",f_free) ;
#endif

	if (! f_free)
	    return SR_OK ;

/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* is the Levo Load Buffer ready to be read ? (whatever that means) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: is LLB ready to be read ?\n") ;
#endif

	rs = llb_readyread(llbp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("iw_loadcheck: llb_readyread() rs=%d\n",rs) ;
	}
#endif /* F_DEBUG */

	if (rs < 1) {

/* signal to load the LB in the next clock */

	    wp->f.loadlb = TRUE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: f_loadlb=%d\n",
		wp->f.loadlb) ;
#endif

	    return rs ;
	}

/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* do we have something in the Levo i-load buffer to load ? */

	llbp = &wp->ourload ;
	rs = iw_lbready(wp,pip,lip,lip->llbp) ;

	f_branches = (rs == INSTRCLASS_CBR) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("iw_loadcheck: iw_lbready() rs=%d\n",rs) ;
	    eprintf("iw_loadcheck: c_ncols=%d n_ncols=%d\n",
		wp->c.paths[LEVOINFO_PATHML].ncols,
		wp->n.paths[LEVOINFO_PATHML].ncols) ;
	}
#endif /* F_DEBUG */

/* we exit now if there is nothing to load */

	if (rs <= 0) {

/* signal to load the LB in the next clock */

	    wp->f.loadlb = TRUE ;

	    return rs ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: something to load \n") ;
#endif

/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* are we waiting for a target instruction to appear in the LB ? */

	if (wp->c.f.targetout) {

	    int	lbi ;
	    int	f_targetin ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: waiting for ta=%08x\n",
		wp->c.ta) ;
#endif

	    rs = iw_targetinlb(wp,pip,lip,wp->c.ta,&lbi) ;

	    if (rs < 0)
	        return rs ;

	    f_targetin = (rs > 0) ;
	    if (! f_targetin) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: target not in LB -- aborting\n") ;
#endif

	        wp->f.loadlb = TRUE ;
	        return SR_OK ;
	    }

	    wp->n.f.targetout = FALSE ;
	    wp->lbi = lbi ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: targetout f_loadlb=%d\n",
		wp->f.loadlb) ;
#endif

	} /* end if (waiting for target) */


/* get the starting column (we really want SG column) for the ML path */

	sgci = wp->n.paths[LEVOINFO_PATHML].sgci ;
	n = wp->n.paths[LEVOINFO_PATHML].ncols ;
	sgci = (sgci + n) % lip->nsgcols ;


/* search for a free column to put an ML path segment in */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3) {
	    eprintf("iw_loadcheck: column usage> ") ;
	    for (i = 0 ; i < MIN(lip->totalcols,25) ; i += 1) {
	        if (wp->n.cols[i].path >= 0)
	            eprintf(" %d", wp->n.cols[i].path) ;

	        else
	            eprintf(" -") ;
	    }
	    eprintf("\n") ;
	}
#endif /* F_DEBUG */

/****

	Here we search forward each of the SG columns and each of
	the individual AS columns within each SG column.

****/

	f_free = FALSE ;
	f_mlfound = FALSE ;
	for (i = 0 ; i < lip->totalcols ; i += LEVOINFO_NASWPSG) {

	    for (j = 0 ; j < LEVOINFO_NASWPSG ; j += 1) {

	        ci = (sgci * LEVOINFO_NASWPSG) + j ;
	        if (wp->n.cols[ci].path == LEVOINFO_PATHML) {

	            f_mlfound = TRUE ;
	            break ;
	        }

	        if (wp->n.cols[ci].path < 0) {

	            f_free = TRUE ;
	            break ;
	        }

	    } /* end for */

	    if (f_free || f_mlfound)
	        break ;

	    sgci = (sgci + 1) % lip->nsgcols ;

	} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: f_free=%d sgci=%d ci=%d\n",
		f_free,sgci,ci) ;
#endif

/* return if an ML column was found (we wrapped completely around) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: f_mlfound=%d\n",f_mlfound) ;
#endif

/* I think that this is the correct thing to do ?? ! */

	if (f_mlfound)
	    return SR_OK ;

/* return if there are no free columns */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("iw_loadcheck: free columns f_free=%d\n",f_free) ;
	    eprintf("iw_loadcheck: c_ncols=%d n_ncols=%d\n",
		wp->c.paths[LEVOINFO_PATHML].ncols,
		wp->n.paths[LEVOINFO_PATHML].ncols) ;
	}
#endif /* F_DEBUG */

	if (! f_free)
	    return SR_OK ;


/* OK, I think that we are ready to rock ! */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: free ML column at ci=%d\n",ci) ;
#endif

/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* load 'em up ! , we're going down the column found previously */

#if	F_INITAS && F_DOAS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_loadcheck: check ASes for used, lbi=%d\n",
		wp->lbi) ;
#endif

	if (wp->lbi > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_loadcheck: UNUSEing some ASes\n",rs) ;
#endif

	    for (i = 0 ; i < wp->lbi ; i += 1) {

		lasp = wp->lases + (lip->totalrows * ci) + i ;
	        rs = las_unused(lasp) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_loadcheck: las_unused() rs=%d\n",rs) ;
#endif

		wp->s.issue_unused += 1 ;

	    } /* end for */

	} /* end if */

/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* load the ASes */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("iw_loadcheck: loading ASes \n") ;
#endif

	for (i = wp->lbi ; i < lip->totalrows ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf(
		    "iw_loadcheck: i=%d asid=%d faddr=%08x "
			"opexec=%d opclass=%d\n",
			i,((lip->totalrows * ci) + i),
	            wp->ourload.c.entry[i].faddr,
	            wp->ourload.c.entry[i].opexec,
	            wp->ourload.c.entry[i].opclass) ;
	    }
#endif /* F_DEBUG */

/* arguments: lasp, llbp, index, path, f_invert */

		lasp = wp->lases + (lip->totalrows * ci) + i ;
	    rs = las_loadfromlb(lasp,
		&wp->ourload,i,
	        LEVOINFO_PATHML,FALSE) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf(
                    "iw_loadcheck: i=%d las_loadfromlb() rs=%d\n",
			i,rs) ;
	    eprintf("iw_loadcheck: c_ncols=%d n_ncols=%d\n",
		wp->c.paths[LEVOINFO_PATHML].ncols,
		wp->n.paths[LEVOINFO_PATHML].ncols) ;
	}
#endif /* F_DEBUG */

	if (rs < 0)
	    break ;

/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    break ;

		rs = llb_used(&wp->ourload,i) ;

		if ((rs < 0) && (rs != SR_EMPTY))
			break ;

		if (rs == INSTRCLASS_UNUSED)
			wp->s.issue_unused += 1 ;

		else if (rs == INSTRCLASS_NULL)
			wp->s.issue_null += 1 ;

		else if (rs > INSTRCLASS_NULL)
			wp->s.issue_instr += 1 ;

		else if (rs < 0) {

			rs = 0 ;
			wp->s.issue_unused += 1 ;

		}

	} /* end for (looping issuing instructions into execution window) */

	if (rs < 0)
	    return rs ;

#endif /* F_INITAS && F_DOAS */


#if	F_LLB

/* do the LLB thing */

	rs = llb_read_complete(llbp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: llb_readcomplete() rs=%d\n",
		rs) ;
#endif

/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    return rs ;

#endif /* F_LLB */


/* can we track this ? */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("iw_loadcheck: do the tracking\n") ;
	    eprintf("iw_loadcheck: c_ncols=%d n_ncols=%d\n",
		wp->c.paths[LEVOINFO_PATHML].ncols,
		wp->n.paths[LEVOINFO_PATHML].ncols) ;
	}
#endif /* F_DEBUG */

/* increment the number of columns for this path */

	wp->n.paths[LEVOINFO_PATHML].ncols += 1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: updated n_ci=%d n_ncols=%d\n",
		ci,wp->n.paths[LEVOINFO_PATHML].ncols) ;
#endif


/* does this new column have branches in it ? (search LLB for answer) */

#ifdef	COMMENT
	for (i = 0 ; i < lip->totalrows ; i += 1) {

	    rs = llb_used(llbp,i) ;

	} /* end for */
#else
	wp->n.cols[ci].nbranches = ((f_branches) ? 1 : 0) ;
#endif

/* update column tracking information */

	wp->s.issue_col += 1 ;
	wp->n.cols[ci].path = LEVOINFO_PATHML ;

/* this will be "shifted" on the next clock if a shift is going to occur */

	wp->n.cols[ci].tt = (lip->nsg * lip->nasrpsg) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: ci=%d ctt=%d\n",
		ci, wp->n.cols[ci].tt) ;
#endif

/* force a machine shift ! (is this right ?) */

	wp->f.shift = TRUE ;


/* sanity */

	rs = iw_sanitycheck(wp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("iw_loadcheck: iw_sanitycheck() rs=%d\n",
		rs) ;
#endif

	if (rs < 0)
	    return rs ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_loadcheck: exiting \n") ;
#endif

	return 1 ;
}
/* end subroutine (iw_loadcheck) */


/* is the specified LB ready to do a load ? */
static int iw_lbready(wp,pip,lip,lbp)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
LLB		*lbp ;
{
	int	rs = SR_OK, i ;
	int	most = 0 ;


	for (i = 0 ; i < lip->totalrows ; i += 1) {

/* if a value greater than zero is returned, it has something in it ! */

	    rs = llb_used(lbp,i) ;

	    if ((rs < 0) && (rs != SR_EMPTY))
	        break ;

	    if (rs > 0) {

	        most = rs ;
	        if (most == INSTRCLASS_CBR)
	            break ;

	    } else
	        rs = SR_OK ;

	} /* end for */

	return ((rs >= 0) ? most : rs) ;
}
/* end subroutine (iw_lbready) */


/* initialize the REGFILTERs */
static int iw_initregfilters(wp,pip,mip,lip,regs)
IW		*wp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
uint		regs[] ;
{
	REGFILTER_INITARGS	rf_a ;

	uint	uiw ;

	int	rs, i, j, k ;
	int	n ;
	int	rfid, sgid ;
	int	busid ;
	int	size ;
	int	ci, sgci ;
	int	f_bad = FALSE ;


/* are we using the global bus ID allocation rules */

#if	F_GLOBALBUSID
	busid = getbusid(lip,1) ;
#endif

/* create and initialize the REGFILTERs (number of SGs plus an extra row) */

	n = (lip->nsgrows + 1) * lip->nsgcols ;
	size = n * sizeof(REGFILTER) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_initregfilters: allocating REGFILTERs n=%d size=%d\n",
		n,size) ;
#endif

	rs = uc_malloc(size,&wp->rfs) ;

	if (rs < 0)
	    goto bad0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_initregfilters: REGFILTER=%08lx\n",wp->rfs) ;
#endif

	(void) memset(wp->rfs,0,size) ;


/* initialize the arguments for the REGFILTERs */

#ifdef	COMMENT
	struct regfilter_a {
	    BUS	*rfbuses, *rbbuses ;
	    int	fbid, bbid ;		/* bus master IDs ! */
	    int	fpri, bpri ;		/* our bus priorities */
	    int	tt ;			/* our time-tag */
	    int	fri ;			/* forwarding bus read index */
	    int	bri ;			/* backwarding bus read index */
	    int	fwi ;			/* forward bus write index */
	    int	bwi ;			/* backward bus write index */
	    int	fspan ;			/* forwarding bus span */
	    int	bspan ;			/* backwarding bus span */
	} ;
#endif /* COMMENT */

	(void) memset(&rf_a,0,sizeof(REGFILTER_INITARGS)) ;

	rf_a.rfbuses = wp->rfbuses ;
	rf_a.rbbuses = wp->rbbuses ;

	rf_a.fspan = lip->nrfspan ;
	rf_a.bspan = lip->nrbspan ;

	rf_a.ffsize = 12 ;		/* forward FIFO entries */
	rf_a.bfsize = 12 ;


	for (j = 0 ; j < lip->nsgcols ; j += 1) {

	    for (i = 0 ; i < lip->nsgrows ; i += 1) {

	        sgid = (lip->nsgrows * j) + i ;

/* calculate the ID for this RF unit */

	        rfid = (j * (lip->nsgrows + 1)) + i ;
	        rf_a.id = rfid ;

/* the bus indicies */

/* for the RFB scan *up* the indices for a 'nrfspan' starting at 'fri' */

	        rf_a.fri = (sgid - lip->nrfspan + lip->nsg) % lip->nsg ;
	        rf_a.fwi = sgid ;

/* for the RBB scan *down* the indices for a 'nrbspan' starting at 'bri' */

	        rf_a.bri = sgid ;
	        rf_a.bwi = (sgid - lip->nrbspan + lip->nsg) % lip->nsg ;

/* give it an initial time-tag (may not have meaning in multiple paths) */

	        rf_a.tt = (rfid == 0) ? 0 : -1 ;

#if	F_GLOBALBUSID
	        busid -= 1 ;
#else
	        busid = (lip->nasrpsg * LEVOINFO_NASWPSG) ;
#endif /* F_GLOBALBUSID */

	        rf_a.fbid = busid ;
	        rf_a.bbid = busid ;

	        rf_a.fpri = rf_a.bpri = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1) {
	            eprintf("iw_initregfilters: initing "
			"REGFILTER(%d)=%08lx\n",
	                rfid,(wp->rfs + rfid),rfid) ;
	            eprintf(
	                "iw_initregfilters: sgid=%d fbmid=%d bbmid=%d"
	                " fri=%d fwi=%d\n",
	                sgid, 
	                rf_a.fbid, rf_a.bbid,
	                rf_a.fri, rf_a.fwi) ;
	        }
#endif /* F_DEBUG */

	        rs = regfilter_init(wp->rfs + rfid,
	            pip,mip,lip,&rf_a) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf(
                        "iw_initregfilters: rfid=%d regfilter_init() rs=%d\n",
	                rfid,rs) ;
#endif

#ifdef	COMMENT
	        if (rs < 0)
	            goto bad1 ;
#else
	        if (rs < 0)
	            f_bad = TRUE ;
#endif

/* do the extra one at the top of each column */

	        if (i == 0) {

#ifdef	COMMENT
	            rf_a.tt = lip->nsg * lip->nasrpsg ;
#else
	            rf_a.tt = -1 ;
#endif

	            rfid = ((j + 1) * (lip->nsgrows + 1)) - 1 ;
	            rf_a.id = rfid ;

#if	F_GLOBALBUSID
	            busid -= 1 ;
#else
	            busid += 1 ;
#endif

	            rf_a.fbid = busid ;
	            rf_a.bbid = busid ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
	                    "iw_initregfilters: initing E "
				"REGFILTER(%d)=%08lx\n",
	                    rfid,(wp->rfs + rfid)) ;
#endif

	            rs = regfilter_init(wp->rfs + rfid,
	                pip,mip,lip,&rf_a) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
			    "iw_initregfilters: E rfid=%d "
				"regfilter_init() rs=%d\n",
	                    rfid,rs) ;
#endif

	            if (rs < 0)
	                f_bad = TRUE ;

	        } /* end if (extra top-of-column) */

	    } /* end for (inner) */

	} /* end for (outer) */

	if (f_bad)
	    goto bad1 ;


/* initialize some register values */


/* choose the RF unit to be the committed one (for ML path) */

	ci = wp->c.paths[LEVOINFO_PATHML].ci ;
	sgci = ci / LEVOINFO_NASWPSG ;

	rfid = (sgci * (lip->nsgrows + 1)) + 0 ;

	wp->c.sgcols[sgci].rfti = wp->n.sgcols[sgci].rfti = rfid ;
	wp->c.sgcols[sgci].rfused.a = wp->n.sgcols[sgci].rfused.a = TRUE ;

	wp->c.paths[LEVOINFO_PATHML].rfi = wp->n.paths[LEVOINFO_PATHML].rfi =
		rfid ;


/* make it scan all backwarding buses */

	regfilter_scanall(wp->rfs + rfid,TRUE) ;


/* initialize all of the "committed" registers to the architected values */

	rs = iw_initregs(wp,pip,lip,wp->rfs + rfid,regs) ;


#ifdef	COMMENT

/* get the initial stack pointer for the program from the OS */

	for (j = 0 ; j < lip->nsgcols ; j += 1) {

	    for (i = 0 ; i < lip->nsgrows ; i += 1) {

	        rfid = (j * (lip->nsgrows + 1)) + i ;
	        for (k = 0 ; k < lip->npaths ; k += 1) {

	            rs = regfilter_initval(wp->rfs + rfid,k,
	                LMIPSREG_R0,0) ;

	            rs = regfilter_initval(wp->rfs + rfid,k,
	                LMIPSREG_R29,regs[LMIPSREG_SP]) ;

	        } /* end for */

	        if (i == 0) {

	            rfid = ((j + 1) * (lip->nsgrows + 1)) - 1 ;
	            for (k = 0 ; k < lip->npaths ; k += 1) {

	                rs = regfilter_initval(wp->rfs + rfid,k,
	                    LMIPSREG_R0,0) ;

	                rs = regfilter_initval(wp->rfs + rfid,k,
	                    LMIPSREG_R29,regs[LMIPSREG_SP]) ;

	            } /* end for */

	        } /* end if (extra top-of-column) */

	    } /* end for (inner) */

	} /* end for (outer) */

#endif /* COMMENT (not needed with pre-initialization of registers) */


/* get out */
bad0:
	return rs ;

/* bad stuff happens here */
bad1:
	for (i = 0 ; i < ((lip->nsgrows + 1) * lip->nsgcols) ; i += 1)
	    (void) regfilter_free(wp->rfs + i) ;

	goto bad0 ;
}
/* end subroutine (iw_initregfilters) */


/* free up the register filter units */
static int iw_freeregfilters(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK, i ;
	int	nrfs ;


	if (wp->rfs == NULL)
	    return SR_OK ;

	nrfs = (lip->nsgrows + 1) * lip->nsgcols ;
	for (i = 0 ; i < nrfs ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_freeregfilters: i=%d regfilter_free()\n", i) ;
#endif

	    rs = regfilter_free(wp->rfs + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_freeregfilters: i=%d regfilter_free() rs=%d\n",
		i,rs) ;
#endif

	} /* end for */

	return rs ;
}
/* end subroutine (iw_freeregfilters) */


/* handle the consequences of a machine shift within our own object */
static int iw_handleshift(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs, j ;
	int	totalrows ;


	if (! wp->f.shift)
	    return SR_OK ;

	wp->f.shift = FALSE ;

	totalrows = lip->totalrows ;
	for (j = 0 ; j < lip->totalcols ; j += 1) {

	    if (wp->n.cols[j].tt >= 0)
	    	wp->n.cols[j].tt -= totalrows ;

	} /* end for */


	return SR_OK ;
}
/* end subroutine (iw_handleshift) */


/* manipulate the register filters */
static int iw_manipulate(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	REGFILTER	*rf1p, *rf2p ;
	REGFILTER	*rftp, *rfbp ;

	int	rs = SR_OK, i, n ;
	int	nsgcols, ci, sgci ;
	int	rfti, rfbi, rfid ;
	int	tt ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_manipulate: entered\n") ;
#endif

	nsgcols = lip->nsgcols ;


/* handle commitment */

	if (wp->f.commit) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_manipulate: commitment occurring\n") ;
#endif

	    ci = wp->c.paths[LEVOINFO_PATHML].ci ;
	    sgci = ci / LEVOINFO_NASWPSG ;
	    if (lip->nsgcols == 1) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: single column\n") ;
#endif

		rfti = wp->n.sgcols[sgci].rfti ;
		rfbi = wp->n.sgcols[sgci].rfbi ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: rfti=%d rfbi=%d\n",rfti,rfbi) ;
#endif

		if ((rfti < 0) || (rfbi < 0))
			return SR_INVALID ;

	        rftp = wp->rfs + rfti ;
	        rfbp = wp->rfs + rfbi ;

/* OK now do something (update the bottom from the top) */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_merge() \n") ;
#endif

	        rs = regfilter_merge(rfbp,rftp) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_merge() rs=%d\n",
			rs) ;
#endif

	        if (rs < 0)
	            goto bad ;

	    } else {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: multiple column, sgci=%d\n",
	                sgci) ;
#endif

		rfti = wp->n.sgcols[sgci].rfti ;
		rfbi = wp->n.sgcols[sgci].rfbi ;

		if ((rfti < 0) || (rfbi < 0))
			return SR_INVALID ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: rfti=%d rfbi=%d\n",rfti,rfbi) ;
#endif

	        rftp = wp->rfs + rfti ;
	        rfbp = wp->rfs + rfbi ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1) {
	            eprintf(
	                "iw_manipulate: current top REGFILTER(%d)=%08lx\n",
	                (rftp - wp->rfs),rftp) ;
	            eprintf(
	                "iw_manipulate: current bottom REGFILTER(%d)=%08lx\n",
	                (rfbp - wp->rfs),rfbp) ;
		}
#endif /* F_DEBUG */

	        rs = regfilter_merge(rfbp,rftp) ;

	        if (rs < 0)
	            goto bad ;

	    } /* end if (one or more columns) */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 3) {

	            rs = iw_checkregs(wp,pip,lip,rfbp) ;

	            eprintf("iw_manipulate: checkregs rs=%d\n",rs) ;

	        }
#endif /* F_DEBUG */

#ifdef	OPTIONAL
		rs = regfilter_settt(rftp,-1) ;
#endif

/* mark the next-state TOP register filter to scan all backwarding buses */

	    rs = regfilter_scanall(rfbp,TRUE) ;

		iw_rffree(wp,pip,lip,rfti) ;

/* record new committed REGFILTER for the ML path */

		wp->n.paths[LEVOINFO_PATHML].rfi = rfbi ;

/* wipe this column */

	        wp->n.sgcols[sgci].rfti = rfbi ;
	        wp->n.sgcols[sgci].rfbi = -1 ;

	} /* end if (commit) */


/* handle shifting */

	if (wp->f.shift) {

	    ci = wp->n.paths[LEVOINFO_PATHML].ci ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_manipulate: shift occurring ci=%d\n",
			ci) ;
#endif

/* find the SG column that was just loaded (or the "last" one) */

	    sgci = ci / LEVOINFO_NASWPSG ;
	    n = wp->n.paths[LEVOINFO_PATHML].ncols ;
	    sgci = (sgci + (n - 1)) % nsgcols  ;
	    if (lip->nsgcols == 1) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: single column\n") ;
#endif

		rfti = iw_rfgettop(wp,pip,lip,sgci) ;

		rfbi = iw_rfalloc(wp,pip,lip,sgci) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: rfti=%d rfbi=%d\n",rfti,rfbi) ;
#endif

		if ((rfti < 0) || (rfbi < 0))
			return SR_INVALID ;

	        rftp = wp->rfs + rfti ;
	        rfbp = wp->rfs + rfbi ;

/* copy the top values to the new emerging bottom */

	        rs = regfilter_pathcopy(rfbp,rftp,LEVOINFO_PATHML) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_pathcopy() rs=%d\n",rs) ;
#endif

	        if (rs < 0)
	            goto bad ;

/* set the TT value for the new top */

	        tt = lip->nsgcols * lip->totalrows ;

#if	F_SETTOPRFTT
	        rs = regfilter_settt(rftp,tt) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_settt() rs=%d\n",rs) ;
#endif

	        if (rs < 0)
	            goto bad ;

#endif /* F_SETTOPRFTT */

/* setup the intervening REGFILTERs for proper operation */

	        rs = iw_setrfs(wp,pip,lip,sgci,tt) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_setrfs() rs=%d\n",rs) ;
#endif

	        if (rs < 0)
	            goto bad ;

/* set the TT value for the new bottom */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_settt() \n") ;
#endif

	        tt += lip->totalrows ;
	        rs = regfilter_settt(rfbp,tt) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_settt() rs=%d\n",rs) ;
#endif

	        if (rs < 0)
	            goto bad ;

/* update */

	        wp->n.sgcols[sgci].rfti = rfti ;
	        wp->n.sgcols[sgci].rfbi = rfbi ;

	    } else {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: multiple column, sgci=%d\n",
			sgci) ;
#endif

/* get the top in this column and the unused in the next */

		rfti = iw_rfgettop(wp,pip,lip,sgci) ;

		rfbi = iw_rfalloc(wp,pip,lip,sgci) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: rfti=%d rfbi=%d\n",rfti,rfbi) ;
#endif

		if ((rfti < 0) || (rfbi < 0))
			return SR_INVALID ;

	        rftp = wp->rfs + rfti ;
	        rfbp = wp->rfs + rfbi ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: new top REGFILTER(%d)=%08lx\n",
	                (rftp - wp->rfs),rftp) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: new bottom REGFILTER(%d)=%08lx\n",
	                (rfbp - wp->rfs), rfbp) ;
#endif

	        rs = regfilter_pathcopy(rfbp,rftp,LEVOINFO_PATHML) ;

	        if (rs < 0)
	            goto bad ;

	        tt = lip->nsgcols * lip->totalrows ;

#if	F_SETTOPRFTT
	        rs = regfilter_settt(rftp,tt) ;

	        if (rs < 0)
	            goto bad ;
#endif /* F_SETTOPRFTT */

/* setup the intervening REGFILTERs for proper operation */

	        rs = iw_setrfs(wp,pip,lip,sgci,tt) ;

	        if (rs < 0)
	            goto bad ;

	        tt += lip->totalrows ;
	        rs = regfilter_settt(rfbp,tt) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_manipulate: regfilter_settt() rs=%d\n",rs) ;
#endif

	        if (rs < 0)
	            goto bad ;

/* update */

	        wp->n.sgcols[sgci].rfti = rfti ;
	        wp->n.sgcols[sgci].rfbi = rfbi ;

	    } /* end if (one or more columns) */

/* mark the new BOTTOM register filter to NOT scan all backwarding buses */

	    rs = regfilter_scanall(rfbp,FALSE) ;

	} /* end if (shift) */

bad:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_manipulate: exiting iw_manipulate() rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_manipulate) */


/* set time-tag values in the REGFILTERs of the specified SG column */
static int iw_setrfs(wp,pip,lip,sgci,tt)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		sgci ;
int		tt ;
{
	REGFILTER	*rfp ;

	int	rs = SR_OK, i ;
	int	rfi ;
	int	rftt ;


	rfi = (lip->nsgrows + 1) * sgci + 0 ;
	rfp = wp->rfs + rfi ;

	for (i = 1 ; i < lip->nsgrows ; i += 1) {

	    rftt = tt + (lip->nasrpsg * i) ;
	    rs = regfilter_settt(rfp + i,rftt) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 1) && (rs < 0))
	        eprintf("iw_setrfs: regfilter_settt() bad i=%d rs=%d\n",i,rs) ;
#endif

	    if (rs < 0)
	        break ;

/* invalidate all registers on all paths in this RF unit */

	    rs = regfilter_invalidate(rfp + i,(~ 0)) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if ((pip->debuglevel > 1) && (rs < 0))
	        eprintf("iw_setrfs: regfilter_invalidate() bad i=%d rs=%d\n",
			i,rs) ;
#endif

	    if (rs < 0)
	        break ;

	} /* end for (looping through SGs in this row) */

	return rs ;
}
/* end subroutine (iw_setrfs) */


#if	F_FPRINTF || F_DEBUGS || (F_MASTERDEBUG && F_DEBUG)

/* print out some registers */
static int iw_printregs(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	REGFILTER	*rfp, *rf0p, *rf1p ;

	LFLOWGROUP	fg ;

	uint	val ;

	int	rs = SR_OK, i ;
	int	sgci ;
	int	rfid ;
	int	path = 0 ;
	int	ncols ;


	ncols = wp->c.paths[LEVOINFO_PATHML].ncols ;
	sgci = wp->c.paths[LEVOINFO_PATHML].sgci ;
	rfid = wp->c.paths[LEVOINFO_PATHML].rfi ;

	if (rfid < 0)
		return SR_INVALID ;

	rfp = wp->rfs + rfid ;

#if	F_FPRINTF
	printf("committed registers sgci=%d c_ncols=%d rfid=%d rftt=%d\n",
	    sgci,ncols,rfid,rfp->c.tt) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_printregs: sgci=%d c_ncols=%d rfid=%d rftt=%d\n",
	        sgci,ncols,rfid,rfp->c.tt) ;
#endif

/* loop through some of the lower addresses for path=0 */

	for (i = 0 ; i < 40 ; i += 1) {

	    rs = regfilter_getfg(rfp,path,i,&fg) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 3) {

	        if (rs < 0)
	            eprintf("iw_printregs: reg(%3d)=**bad rs=%d**\n",
	                i,rs) ;

	        else if (rs == 0) {

	            eprintf("iw_printregs: reg(%3d)=%08lx tt=%d\n",
	                i,fg.dv,fg.tt) ;

	        } else if (rs == 1)
	            eprintf("iw_printregs: reg(%3d)=*invalid*\n",
	                i) ;

	        else if (rs == 2)
	            eprintf("iw_printregs: reg(%3d)=*nodata*\n",
	                i) ;

	    }
#endif /* F_DEBUG */

#if	F_FPRINTF
	    if (rs < 0)
	        fprintf(stdout,"reg(%3d)=**bad rs=%d**\n",
	            i,rs) ;

	    else if (rs == 0)
	        fprintf(stdout,"reg(%3d)=%08lx tt=%d\n",
	            i,fg.dv,fg.tt) ;

	    else if (rs == 1)
	        fprintf(stdout,"reg(%3d)=*invalid*\n",
	            i) ;

	    else if (rs == 2)
	        fprintf(stdout,"reg(%3d)=*nodata*\n",
	            i) ;

		fflush(stdout) ;

#endif /* F_FPRINTF */

	    if (rs < 0)
	        break ;

#if	F_CHECKALIRGF
	if ((pip->debuglevel > 2) && (rs == 0)) {

		uint	val ;

		int	att ;


		alirgf_getval2(&wp->alirgf,i,&att,&val) ;

		if (fg.dv != val) {

		eprintf(
		    "iw_printregs: mismatch w/ ALIRGF a=%d\n",
			i) ;

		eprintf(
		    "iw_printregs: RFU tt=%d dv=%08x\n",
			fg.tt,fg.dv) ;

		eprintf(
		    "iw_printregs: ALIRGF tt=%d val=%08x\n",
			att,val) ;

			rs = SR_ILSEQ ;
			break ;

		} /* end if */

	} /* end if */
#endif /* F_CHECKALIRGF */

	} /* end for */

	return rs ;
}
/* end subroutine (iw_printregs) */

#endif /* F_FPRINTF || F_DEBUG || F_DEBUGS */


/* initialize the committed registers (where ever they are ?) */
static int iw_initregs(wp,pip,lip,rfp,regs)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
REGFILTER	*rfp ;
uint		regs[] ;
{
	int	rs = SR_OK ;
	int	ra ;


	for (ra = 0 ; ra < LMIPSREG_NREGS ; ra += 1) {

	    rs = regfilter_initval(rfp,LEVOINFO_PATHML, ra,regs[ra]) ;

	    if (rs < 0)
		break ;

	} /* end for */

	return rs ;
}
/* end subroutine (iw_initregs) */


#if	F_MASTERDEBUG && F_DEBUG

static int iw_checkregs(wp,pip,lip,rfp)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
REGFILTER	*rfp ;
{
	uint	val ;

	int	rs = SR_OK, i ;
	int	path = 0, ra ;


	for (i = 0 ; i < 40 ; i += 1) {

	    rs = regfilter_getval(rfp,path,i,&val) ;

	    if (rs < 0)
	        eprintf("iw_checkregs: reg(%d)=**bad rs=%d**\n",
	            i,rs) ;

	    else if (rs == 1)
	        eprintf("iw_checkregs: reg(%d)=*invalid*\n",
	            i) ;

	    else if (rs == 2)
	        eprintf("iw_checkregs: reg(%d)=*nodata*\n",
	            i) ;

	} /* end for */

	return rs ;
}
/* end subroutine (iw_checkregs) */

#endif /* F_DEBUG */


/* get the unused RF for this SG column */
static int iw_getrfunused(wp,pip,lip,sgci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		sgci ;
{
	int	rfti, rfbi ;
	int	rf0i, rf1i ;
	int	rs ;


	rf0i = (lip->nsgrows + 1) * sgci + 0 ;
	rf1i = (lip->nsgrows + 1) * sgci + lip->nsgrows ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_getrfunused: sgci=%d rf0i=%d rf1i=%d\n",
	        sgci,rf0i,rf1i) ;
#endif

	rfti = wp->n.sgcols[sgci].rfti ;
	rfbi = wp->n.sgcols[sgci].rfbi ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_getrfunused: rfti=%d rfbi=%d\n",
	        rfti,rfbi) ;
#endif

	if (rfti < 0) {

	    rs = (rfbi == rf0i) ? rf1i : rf0i ;

	} else if (rfbi < 0) {

	    rs = (rfti == rf0i) ? rf1i : rf0i ;

	} else 
	    rs = -1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("iw_getrfunused: rfui=%d\n",
	        rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_getrfunused) */


/* commit the ASes (not an easy job) */
static int iw_commitases(wp,pip,lip,ci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	LAS	*lasp ;

	LAS_COMMITINFO	asi ;

	uint	ta ;
	uint	btarget, faddr ;

	int	rs, i, n ;
	int	tt ;
	int	ipval, bdir ;
	int	f_goto = FALSE ;
	int	f_ij = FALSE ;		/* indirect jump */
	int	f_cfchange = FALSE ;
	int	f_exit = FALSE ;


	ta = 0 ;
	tt = wp->c.cols[ci].tt ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4)
		eprintf("iw_commitases: ci=%d ctt=%d\n",
			ci,tt) ;
#endif

	for (i = 0 ; (i < lip->totalrows) && (! f_ij) ; i += 1) {

	    lasp = wp->lases + (lip->totalrows * ci) + i ;
	    rs = las_commit(lasp,
	        &ipval,&bdir,&btarget,&faddr) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1) {

	        if (rs == SR_EMPTY)
	            eprintf(
	                "iw_commitases: row=%d las_commit() rs=EMPTY\n",
	                i) ;

	        else
	            eprintf(
	                "iw_commitases: row=%d las_commit() rs=%d\n",
	                i,rs) ;

	    }
#endif /* F_DEBUG */

	    if ((rs < 0) && (rs != SR_EMPTY))
	        break ;

#if	F_FPRINTF
		if (ipval)
	                printf("Inst. Commited\n") ;
#endif

/* do we have a loaded AS ? */

	    if (rs >= 0) {

	        rs = las_commitinfo(lasp, &asi) ;

/* is the instruction a real one or one of the pseudo instructions ? */

	        if ((rs >= 0) && (asi.iclass > INSTRCLASS_NULL)) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
	                    "iw_commitases: ia=%08x instr=%08x "
				"f_ep=%d ipval=%d\n",
	                    asi.ia,asi.instr,asi.f_enabled,ipval) ;
#endif

/* OK, we have an instruction, but was it enabled at the time ? */

	                f_cfchange = (rs == 1) ;
		    wp->s.commit_instr += 1 ;
	            if (asi.f_enabled) {

#if	F_FPRINTF && F_DETAILS
	                printf(
	                    "iw_commitases: in=%lld ia=%08x instr=%08x\n",
	                    wp->s.commit_ienabled,asi.ia,asi.instr) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	                if (pip->debuglevel > 1)
	                    eprintf(
	                        "iw_commitases: in=%lld ia=%08x instr=%08x\n",
	                        wp->s.commit_ienabled,asi.ia,asi.instr) ;
#endif

/* was it a control-flow-change instruction ? */

	                if (f_cfchange) {

	                    int	f_targetin1 ;
	                    int	f_targetin2 ;


#if	F_FPRINTF && F_DETAILS
	                    printf(
	                        "iw_commitases: CFC f_taken=%d ta=%08x\n",
	                        asi.f_taken,asi.ta) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel > 1)
	                        eprintf(
	                            "iw_commitases: CFC f_taken=%d ta=%08x\n",
	                            asi.f_taken,asi.ta) ;
#endif

/* send signal back to i-fetch about the committed state */

	                    rs = lifetch_bu(&wp->ourfetch,
	                        asi.ia,asi.btype,asi.f_taken,asi.ta) ;

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel > 1)
	                        eprintf(
	                            "iw_commitases: lifetch_bu() rs=%d\n",
	                            rs) ;
#endif

/* is this an indirect jump ? */

	                    if (asi.opclass == OPCLASS_JIND) {

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel > 1)
	                        eprintf(
	                            "iw_commitases: indirect jump "
					"ia=%08x ta=%08x\n",
	                            asi.ia,asi.ta) ;
#endif

#if	F_IJ
	                        f_ij = TRUE ;
#endif

	                    } /* end if (indirect jump) */

#if	F_LBTRBTARGET

/* is it in the execution window according to LBTRB ? */

	                    rs = lbtrb_targetin(lip->lbtrbp,
	                        btarget,tt) ;

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel > 1)
	                        eprintf(
	                            "iw_commitases: lbtrb_targetin() "
	                            "rs=%d\n",
	                            rs) ;
#endif

	                    f_targetin1 = (rs >= 1) ;

#else

	                    f_targetin1 = TRUE ;

#endif /* F_LBTRBTARGET */

/* is the target within the execution window ? */

#if	F_MASTERDEBUG && F_DEBUG
			if (pip->debuglevel > 1)
	    			eprintf("iw_commitases: looking "
				"tt=%d ta=%08x\n",
					tt,asi.ta) ;
#endif

	                    rs = iw_targetin(wp,pip,lip,asi.ta,tt) ;

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel > 1)
	                        eprintf(
	                            "iw_commitases: iw_targetin() "
	                            "rs=%d\n",
	                            rs) ;
#endif

	                    if (rs < 0)
	                        break ;

	                    f_targetin2 = (rs >= 1) ;


/* do something if the target was NOT in the execution window */

/****

			If we have not seen a GOTO yet, and we have a
			target out of the window by either target
			buffer thing (LBTRB or Dave 's IEWBTTB), then
			mark the fact that we have an out-of-window
			jump and that it will need a GOTO operation
			later.

****/

	                    if ((! f_goto) && 
	                        (((! f_targetin1) && f_ij) || 
	                        (! f_targetin2))) {

	                        f_goto = TRUE ;
	                        ta = asi.ta ;

#if	F_FPRINTF && F_DETAILS
	                            fprintf(stdout,
	                                "iw_commitases: mark GOTO "
	                                "ia=%08x ta=%08x\n",
	                                asi.ia,asi.ta) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	                        if (pip->debuglevel > 1)
	                            eprintf(
	                                "iw_commitases: mark GOTO "
	                                "ia=%08x ta=%08x\n",
	                                asi.ia,asi.ta) ;
#endif

	                    } /* end if (marking a GOTO) */

	                } /* end if (control flow change) */

			if (! f_exit) {

			rs = iw_checkexit(wp,pip,lip,&asi) ;

	                    if (rs < 0)
	                        break ;

			f_exit = rs ;

			} /* end if (checking for program exit) */

	                wp->s.commit_ienabled += 1 ;	/* enabled commits */

	            } else
	                wp->s.commit_idisabled += 1 ;	/* disabled commits */

	        } else {

	            if ((rs < 0) || (asi.iclass == INSTRCLASS_UNUSED))
	                wp->s.commit_unused += 1 ;	/* unused commits */

	            else
	                wp->s.commit_null += 1 ;	/* NULL commits */

	        }

	    } else
	        wp->s.commit_unused += 1 ;		/* unused commits */

	    tt += 1 ;

	} /* end for (looping committing ASes) */

/* handle bad stuff first */

	if ((rs < 0) && (rs != SR_EMPTY))
	    return rs ;

/* handle indirect jumps special for now (a nightmare that must end !) */

	rs = SR_OK ;
	if (f_ij) {

	    uint	ta2 ;

	    int		f_targetin = FALSE ;
	    int		f_nextcol = FALSE ;


	    ta = asi.ta ;
	    if (i < lip->totalrows) {

/* commit the delay slot AS */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf(
			"iw_commitases: IJ committing DS\n") ;
#endif

	        lasp = wp->lases + (lip->totalrows * ci) + i ;
	        rs = las_commit(lasp,
	            &ipval,&bdir,&btarget,&faddr) ;

	        if ((rs < 0) && (rs != SR_EMPTY))
	            return rs ;

#if	F_FPRINTF
	        if (ipval)
	            printf("Inst. Commited\n") ;
#endif

	        rs = las_commitinfo(lasp, &asi) ;

#ifdef	OPTIONAL
	        if ((rs < 0) && (rs != SR_EMPTY))
	            return rs ;
#endif

	        if ((rs >= 0) && (asi.iclass > INSTRCLASS_UNUSED)) {

	                f_cfchange = (rs == 1) ;
	            if (asi.iclass > INSTRCLASS_NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
	                    "iw_commitases: IJ ia=%08x instr=%08x f_ep=%d\n",
	                    asi.ia,asi.instr,asi.f_enabled) ;
#endif

		    wp->s.commit_instr += 1 ;
	                if (asi.f_enabled) {

#if	F_FPRINTF && F_DETAILS
	                printf(
	                    "iw_commitases: in=%lld ia=%08x instr=%08x\n",
	                    wp->s.commit_ienabled,asi.ia,asi.instr) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	                if (pip->debuglevel > 1)
	                    eprintf(
	                        "iw_commitases: IJ "
				"in=%lld ia=%08x instr=%08x\n",
	                        wp->s.commit_ienabled,asi.ia,asi.instr) ;
#endif

			if (! f_exit) {

			rs = iw_checkexit(wp,pip,lip,&asi) ;

	                    if (rs < 0)
	                        return rs ;

			f_exit = rs ;

			} /* end if (checking for program exit) */

	                    wp->s.commit_ienabled += 1 ;

	                } else
	                    wp->s.commit_idisabled += 1 ;

	            } else
	                wp->s.commit_null += 1 ;

	        } else
	            wp->s.commit_unused += 1 ;

	        i += 1 ;
	        if (i < lip->totalrows) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1) {
	                eprintf(
	                    "iw_commitases: column instructions remaining\n") ;
	                eprintf(
	                    "iw_commitases: AS row=%d\n",i) ;
	            }
#endif

/* is the target the next instruction in this column ? */

	            lasp = wp->lases + (lip->totalrows * ci) + i ;
	            rs = las_getia(lasp,&ta2) ;

#ifdef	OPTIONAL
	            if ((rs < 0) && (rs != SR_EMPTY))
	                return rs ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
	                    "iw_commitases: IJ ta=%08x ta2=%08x\n",ta,ta2) ;
#endif

	            if ((rs > INSTRCLASS_NULL) && (ta == ta2)) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
	                    "iw_commitases: target in this column\n") ;
#endif

			f_targetin = TRUE ;
	                for ( ; i < lip->totalrows ; i += 1) {

	                    lasp = wp->lases + (lip->totalrows * ci) + i ;
	                    rs = las_commit(lasp,
	                        &ipval,&bdir,&btarget,&faddr) ;

	                    if ((rs < 0) && (rs != SR_EMPTY))
	                        break ;

#if	F_FPRINTF
	                    if (ipval)
	                        printf("Inst. Commited\n") ;
#endif

	                    rs = las_commitinfo(lasp, &asi) ;

#ifdef	OPTIONAL
	                    if ((rs < 0) && (rs != SR_EMPTY))
	                        return rs ;
#endif

	                    if ((rs >= 0) && 
				(asi.iclass > INSTRCLASS_UNUSED)) {

	                	f_cfchange = (rs == 1) ;
	                        if (asi.iclass > INSTRCLASS_NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("iw_commitases: ia=%08x "
				"instr=%08x f_ep=%d\n",
	                    asi.ia,asi.instr,asi.f_enabled) ;
#endif

		    			wp->s.commit_instr += 1 ;
	                            if (asi.f_enabled) {

#if	F_FPRINTF && F_DETAILS
	                        printf("iw_commitases: in=%lld "
					"ia=%08x instr=%08x",
	                            wp->s.commit_ienabled,asi.ia,asi.instr) ;
#endif

			if (! f_exit) {

					rs = iw_checkexit(wp,pip,lip,&asi) ;

	                    if (rs < 0)
	                        break ;

			f_exit = rs ;

					} /* end if (checking for exit) */

	                                wp->s.commit_ienabled += 1 ;

	                            } else
	                                wp->s.commit_idisabled += 1 ;

	                        } else
	                            wp->s.commit_null += 1 ;

	                    } else
	                        wp->s.commit_unused += 1 ;

	                } /* end for (committing remaining ASes) */

	            } else {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
	                    "iw_commitases: target *NOT* in this column\n") ;
#endif

	                f_goto = TRUE ;
	                for ( ; i < lip->totalrows ; i += 1) {

	                    lasp = wp->lases + (lip->totalrows * ci) + i ;
	                    rs = las_unused(lasp) ;

#if	F_MASTERDEBUG && F_DEBUG
	                    if (pip->debuglevel > 1)
	                        eprintf("iw_commitases: las_unused() rs=%d\n",
	                            rs) ;
#endif

				if (rs < 0)
					break ;

	                        wp->s.commit_unused += 1 ;

	                } /* end for */

	            } /* end if (target was not next) */

	        } else
	            f_nextcol = TRUE ;

	        if (rs < 0)
	            return rs ;

	    } /* end if (committing the delay slot) */

/* do we have to check the next column for the target ? */

	    if (((! f_targetin) && (i >= lip->totalrows)) || f_nextcol) {

	        int	ci2 ;


#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf(
	                "iw_commitases: next column check ta=%08x\n",ta) ;
#endif

	        ci2 = iw_getnextcol(wp,lip,LEVOINFO_PATHML,ci) ;

	        if (ci2 >= 0) {

	            i = 0 ;
	            lasp = wp->lases + (lip->totalrows * ci2) + i ;
	            rs = las_getia(lasp,&ta2) ;

	            if ((rs < 0) && (rs != SR_EMPTY))
	                return rs ;

/* if the target is not here, convert to a GOTO ! */

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
	                    "iw_commitases: next column ta=%08x ta2=%08x\n",
	                    ta,ta2) ;
#endif

	            if ((rs <= INSTRCLASS_NULL) || (ta != ta2))
	                f_goto = TRUE ;

	        } else
	            f_goto = TRUE ;

	    } /* end if (checking next column for target) */

	} /* end if (indirect jumps) */


/* handle jumps out of the window */

	if (f_goto) {

	    int	lbi ;


#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitases: processing GOTO\n") ;
#endif

	    wp->f.targetout = TRUE ;

/* is the target in the LB ? (almost never is !) */

	    rs = iw_targetinlb(wp,pip,lip,ta,&lbi) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitases: iw_targetinlb() rs=%d\n",rs) ;
#endif

	    if (rs > 0) {

/* target is in the LB */

	        wp->lbi = lbi ;

	    } else {

/* target is NOT in the LB */

	        wp->lbi = -1 ;
	        wp->f.noload = TRUE ;
	        wp->f.loadlb = TRUE ;

	        wp->n.f.targetout = TRUE ;
	        wp->n.ta = ta ;

/* invalidate the LB */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 2)
	            eprintf("iw_commitases: invalidating LB\n") ;
#endif

	        rs = llb_read_complete(&wp->ourload) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 2)
	            eprintf("iw_commitases: llb_read_complete() rs=%d\n",
	                rs) ;
#endif

/* do the actual GOTO to the i-fetch unit */

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_commitases: GOTO ta=%08x\n",ta) ;
#endif

	        rs = lifetch_goto(&wp->ourfetch,ta) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_commitases: lifetch_goto() rs=%d\n",rs) ;
#endif

	    } /* end if (target in or out of LB) */

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitases: GOTO f_loadlb=%d\n",
	            wp->f.loadlb) ;
#endif

/* OK, flush everything except the column that we just committed */

	    rs = iw_flushwindow(wp,pip,lip,ci) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitases: iw_flushwindow() rs=%d\n",rs) ;
#endif


/* flush BTRB, we need to build a new set of predicators */

	    rs = lbtrb_flush(&wp->lbtrb) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("iw_commitases: lbtrb_flush() rs=%d\n",rs) ;
#endif


	} /* end if (doing a GOTO) */

/* update some statistics */

	wp->s.commit_col += 1 ;		/* column commitments */


/* update the "Ali" register file somehow */

#if	F_ALIRGF
	alirgf_commit(&wp->alirgf) ;
#endif

#if	F_FPRINTF
	alirgf_print(&wp->alirgf) ;
	lwq_print(&wp->wqueue) ;
	lwq_print_wq(&wp->wqueue) ;
#endif

	wp->n.count = 0 ;

	return ((rs >= 0) ? f_exit : rs) ;
}
/* end subroutine (iw_commitases) */


/* is the target instruction already in the window ? */
static int iw_targetin(wp,pip,lip,ta,tt)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
uint	ta ;
int	tt ;
{
	LAS	*lasp ;

	uint	ia ;

	int	rs = SR_OK, i, j ;
	int	ci, n, ncols ;
	int	itt ;
	int	f_found ;


	ci = wp->c.paths[LEVOINFO_PATHML].ci ;
	ncols = wp->c.paths[LEVOINFO_PATHML].ncols ;
	n = 0 ;

	f_found = FALSE ;

#if	F_MASTERDEBUG && F_DEBUG
		if (pip->debuglevel > 4)
		eprintf("iw_targetin: c_ci=%d c_ncols=%d tt=%d ta=%08lx\n",
			ci,ncols,tt,ta) ;
#endif

/* for each column up to the number of columns in this path */

	for (j = 0 ; (j < lip->totalcols) && (n < ncols) ; j += 1) {

	    if (wp->c.cols[ci].path == LEVOINFO_PATHML) {

/* search down this column */

	        itt = wp->c.cols[ci].tt ;
	        for (i = 0 ; i < lip->totalrows ; i += 1) {

/* only look at ASes that have TT later than the control flow change */

	            if (itt > tt) {

	                lasp = wp->lases + (lip->totalrows * ci) + i,
	                rs = las_getia(lasp, &ia) ;

/* pure error condition */

	                if ((rs < 0) && (rs != SR_EMPTY))
	                    break ;

/* real test */

#if	F_MASTERDEBUG && F_DEBUG
		if (pip->debuglevel > 4) {
		char	rsbuf[24] ;

		if (rs == SR_EMPTY)
			strcpy(rsbuf,"EMPTY") ;

		else
			ctdeci(rsbuf,23,rs) ;

			eprintf("iw_targetin: ci=%d row=%d "
				"rs=%s itt=%d ia=%08x\n",
			ci,i,rsbuf,itt,ia) ;
		}
#endif /* F_DEBUG */

	                if ((rs > INSTRCLASS_NULL) && (ia == ta)) {

	                    f_found = TRUE ;
	                    break ;

	                } else
	                    rs = SR_OK ;

	            } /* end if (TT OK) */

	            itt += 1 ;

	        } /* end for */

/* get out if pure error */

	        if (rs < 0)
	            break ;

	        if (f_found)
	            break ;

	        n += 1 ;

	    } /* end if */

	    ci = (ci + 1) % lip->totalcols ;

	} /* end for */

	return ((rs >= 0) ? f_found : rs) ;
}
/* end subroutine (iw_targetin) */


/* is the target address (outside of e-window) in the LB ? */
static int iw_targetinlb(wp,pip,lip,ta,ip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
uint		ta ;
int		*ip ;
{
	uint	ta2 ;

	int	rs = SR_OK, i ;
	int	f_targetin = FALSE ;


	for (i = 0 ; i < lip->totalrows ; i += 1) {

	    rs = llb_getia(lip->llbp,i,&ta2) ;

	    if ((rs < 0) && (rs != SR_EMPTY))
	        break ;

#if	F_MASTERDEBUG && F_DEBUG
		if (pip->debuglevel > 4)
		eprintf("iw_targetinlb: row=%d rs=%d ia=%08x\n",
			i,rs,ta2) ;
#endif

	    if ((rs != SR_EMPTY) && (ta2 == ta)) {

#if	F_MASTERDEBUG && F_DEBUG
		if (pip->debuglevel > 4)
		eprintf("iw_targetinlb: row=%d ia=%08x found it\n",
			i,ta2) ;
#endif

	        *ip = i ;
	        f_targetin = TRUE ;
	        break ;
	    }

	    rs = SR_OK ;

	} /* end for */

	return ((rs >= 0) ? f_targetin : rs) ;
}
/* end subroutine (iw_targetinlb) */


/* check for possible deadlock in the execution window */
int iw_checkdeadlock(wp,pip,lip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	BUS_STATS	bs ;

	ULONG	val, total = 0 ;

	int	rs, i ;


/* get data on all RF & RB & PF buses */

	for (i = 0 ; i < lip->nsg ; i += 1) {

		rs = bus_getstats(wp->rfbuses + i,&bs) ;

		if (rs < 0)
			break ;

		total += bs.used ;
		total += bs.unused ;

		rs = bus_getstats(wp->rbbuses + i,&bs) ;

		if (rs < 0)
			break ;

		total += bs.used ;
		total += bs.unused ;

		rs = bus_getstats(wp->pfbuses + i,&bs) ;

		if (rs < 0)
			break ;

		total += bs.used ;
		total += bs.unused ;

	} /* end for */

	if (rs < 0)
		return rs ;

/* get data on the MF bus */

	for (i = 0 ; i < wp->nwmfbuses ; i += 1) {

		rs = bus_getstats(wp->mfbuses + i,&bs) ;

		if (rs < 0)
			break ;

		total += bs.used ;
		total += bs.unused ;

	} /* end for */

	if (rs < 0)
		return rs ;

	for (i = 0 ; i < wp->nwmbbuses ; i += 1) {

		rs = bus_getstats(wp->mbbuses + i,&bs) ;

		if (rs < 0)
			break ;

		total += bs.used ;
		total += bs.unused ;

	} /* end for */

	if (rs < 0)
		return rs ;

/* check for the actual deadlock condition */

	for (i = 0 ; (rs = shiftregl_get(&wp->ba,i,&val)) >= 0 ; i += 1) {

		if (val != total)
			break ;

	} /* end for */

	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
		if (pip->debuglevel > 4)
		eprintf("iw_checkdeadlock: yep, got one !\n") ;
#endif

		return SR_DEADLOCK ;
	}

	rs = shiftregl_shiftin(&wp->ba,total) ;

#if	F_MASTERDEBUG && F_DEBUG
		if (pip->debuglevel > 4)
		eprintf("iw_checkdeadlock: shiftregl_shiftin() rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_checkdeadlock) */


/* check for program exit */
static int iw_checkexit(wp,pip,lip,ip)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
LAS_COMMITINFO	*ip ;
{
	LSIM	*mip ;

	int	rs = SR_OK, j, k ;
	int	path, nmlcols ;
	int	sgci ;

	const char	*np ;


	if (! ip->f_cf)
		return FALSE ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) 
	eprintf("iw_checkexit: ia=%08x ta=%08x\n",ip->ia,ip->ta) ;
#endif

	mip = wp->mip ;
	rs = lsim_issyscall(mip,ip->ta,&np) ;

	if (rs > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("iw_checkexit: PSYSCALL ia=%08x ta=%08x syscall=%s\n",
		ip->ia,ip->ta,np) ;
#endif

		if ((strcmp(np,"exit") != 0) && (strcmp(np,"_exit") != 0))
			rs = SR_NOTSUP ;

	} /* end if */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1) 
	eprintf("iw_checkexit: rs=%d\n",rs) ;
#endif

	if (! wp->n.f.exit)
		wp->n.f.exit = (rs > 0) ;

	return ((rs > 0) ? TRUE : rs) ;
}
/* end subroutine (iw_checkexit) */


/* flush the entire execution window except the specified column ! */
static int iw_flushwindow(wp,pip,lip,ci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		ci ;
{
	int	rs = SR_OK, j, k ;
	int	path, nmlcols ;
	int	sgci ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_flushwindow: after column=%d\n",ci) ;
#endif

	sgci = ci / LEVOINFO_NASWPSG ;

/* flush the AS columns first */

	nmlcols = 0 ;
	j = (ci + 1) % lip->totalcols ;
	for (k = 0 ; k < (lip->totalcols - 1) ; k += 1) {

	    if ((path = wp->c.cols[j].path) >= 0) {

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("iw_flushwindow: live path=%d column=%d\n",
			path,j) ;
#endif

	        if (path == 0) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
                            "iw_flushwindow: killing ML path col=%d\n",
				j) ;
#endif

	            rs = iw_killpath(wp,pip,lip,path,j,(lip->nsgcols - 1)) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("iw_flushwindow: iw_killpath() rs=%d\n",rs) ;
#endif

	            wp->n.cols[j].f.kill = TRUE ;
	            wp->n.cols[j].f.used = FALSE ;
	            wp->n.cols[j].path = -1 ;
	            nmlcols += 1 ;

		} else {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
			    "iw_flushwindow: killing DEE path col=%d\n",
				j) ;
#endif

	            rs = iw_killpath(wp,pip,lip,path,j,-1) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("iw_flushwindow: iw_killpath() rs=%d\n",rs) ;
#endif

	            wp->n.paths[path].ncols = 0 ;
	            wp->n.paths[path].ci = -1 ;

	        } /* end if (ML or DEE) */

	    } /* end if (killing off a path) */

	    j = (j + 1) % lip->totalcols ;

	} /* end for */

	wp->n.paths[LEVOINFO_PATHML].ncols -= nmlcols ;

/* flush the RF units that were at the bottoms of those SG columns */

	sgci = (sgci + 1) % lip->nsgcols ;
	for (k = 0 ; k < (lip->nsgcols - 1) ; k += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	   if (pip->debuglevel > 1)
	      eprintf("iw_flushwindow: rfdisable check sgci=%d\n",sgci) ;
#endif

	    if (wp->n.sgcols[sgci].rfbi >= 0) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("iw_flushwindow: rfdisable sgci=%d\n",sgci) ;
#endif

		rs = iw_rfdisable(wp,pip,lip,sgci) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("iw_flushwindow: iw_rfdisabled() rs=%d\n",rs) ;
#endif

		if (rs < 0)
			break ;

	    } /* end if (had a live one) */

	    sgci = (sgci + 1) % lip->nsgcols ;

	} /* end for (looping over SG columns) */

/* we-re done */

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
                            "iw_flushwindow: rs=%d killed mlcols=%d\n",
				rs,nmlcols) ;
#endif

	return ((rs >= 0) ? nmlcols : rs) ;
}
/* end subroutine (iw_flushwindow) */


/* disable all the RF units after the top one of the specified SG column */
static int iw_rfdisable(wp,pip,lip,sgci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		sgci ;
{
	REGFILTER	*rfp ;

	int	rs = SR_OK, i ;
	int	nsgci ;
	int	rfi, rfti, rfbi ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("iw_rfdisable: sgci=%d\n",sgci) ;
#endif

	rfbi = wp->n.sgcols[sgci].rfbi ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel > 4) && (rfbi < 0))
	    eprintf(
		"iw_rfdisable: this SG col has no other active RF units\n") ;
#endif

	if (rfbi < 0)
		return SR_OK ;

/* first "do" the ones in this column (after the first one) */

	rfi = (lip->nsgrows + 1) * sgci + 0 ;
	rfp = wp->rfs + rfi ;
	for (i = 1 ; i < lip->nsgrows ; i += 1) {

		rs = regfilter_settt(rfp + i,-1) ;

		if (rs < 0)
			break ;

	} /* end for */

	if (rs < 0)
		return rs ;

/* disable the bottom RF unit for this column */

	rs = regfilter_settt(rfp + rfbi,-1) ;

	if (rs < 0)
		return rs ;

/* free up the bottom RF unit in this column */

	wp->n.sgcols[sgci].rfbi = -1 ;
	rs = iw_rffree(wp,pip,lip,rfbi) ;


	return rs ;
}
/* end subroutine (iw_rfdisable) */


/* kill off all columns that have the given path starting at 'ci' */
static int iw_killpath(wp,pip,lip,path,ci,max)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		path ;
int		ci ;
int		max ;
{
	int	rs = SR_OK, i, j, k ;
	int	n ;


	if (wp->n.cols[ci].path != path)
	    return SR_INVALID ;

	if (max < 0)
		max = lip->totalcols ;

	n = 0 ;
	j = ci ;
	for (k = 0 ; (k < lip->totalcols) && (n < max) ; k += 1) {

	    if (wp->c.cols[j].path == path) {

	        wp->n.cols[j].f.kill = TRUE ;
	        wp->n.cols[j].f.used = FALSE ;
	        wp->n.cols[j].path = -1 ;

/* kill off all ASes in this column */

	        for (i = 0 ; i < lip->totalrows ; i += 1) {

	            rs = las_unused(wp->lases + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("iw_killpath: las_unused() rs=%d\n",rs) ;
#endif

	        } /* end for */

		n += 1 ;

	    } /* end if (killed this path) */

	    j = (j + 1) % lip->totalcols ;

	} /* end for */

	return ((rs >= 0) ? n : rs) ;
}
/* end subroutine (iw_killpath) */


/* find the physical column with the given path starting at 'ci' (if any) */
static int iw_getpathcol(wp,lip,path,ci)
IW		*wp ;
struct levoinfo	*lip ;
int		path ;
int		ci ;
{
	int	i, j ;


	j = ci ;
	for (i = 0 ; i < lip->totalcols ; i += 1) {

	    if (wp->c.cols[j].path == path)
	        break ;

	    j = (j + 1) % lip->totalcols ;

	} /* end for */

	return ((i < lip->totalcols) ? j : -1) ;
}
/* end subroutine (iw_getpathcol) */


/* find the next physical column for this path (if any) */

/*

	path	path to look within
	ci	current column index to start looking from (start w/ next)

*/

static int iw_getnextcol(wp,lip,path,ci)
IW		*wp ;
struct levoinfo	*lip ;
int		path ;
int		ci ;
{
	int	i, j ;


	j = (ci + 1) % lip->totalcols ;
	for (i = 0 ; i < (lip->totalcols - 1) ; i += 1) {

	    if (wp->c.cols[j].path == path)
	        break ;

	    j = (j + 1) % lip->totalcols ;

	} /* end for */

	return ((i < lip->totalcols) ? j : -1) ;
}
/* end subroutine (iw_getnextcol) */


/* find the top RF unit for this column */
int iw_rfgettop(wp,pip,lip,sgci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		sgci ;
{
	int	rs ;
	int	rfi ;


	rfi = sgci * (lip->nsgrows + 1) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("iw_rfgettop: sgci=%d rfi=%d a=%d b=%d\n",
		sgci,rfi,
	wp->n.sgcols[sgci].rfused.a,
	wp->n.sgcols[sgci].rfused.b) ;
#endif

	if (wp->n.sgcols[sgci].rfused.a)
		rs = rfi ;

	else if (wp->n.sgcols[sgci].rfused.b)
		rs = rfi + lip->nsgrows ;

	else
		rs = SR_INVALID ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("iw_rfgettop: rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_rfgettop) */


/* allocate a RF unit for this column */
int iw_rfalloc(wp,pip,lip,sgci)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		sgci ;
{
	int	rs ;
	int	nsgci, rfi ;


	nsgci = (sgci + 1) % lip->nsgcols ;
	rfi = nsgci * (lip->nsgrows + 1) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("iw_rfalloc: sgci=%d nsgci=%d rfi=%d a=%d b=%d\n",
		sgci,nsgci,rfi,
	wp->n.sgcols[nsgci].rfused.a,
	wp->n.sgcols[nsgci].rfused.b) ;
#endif

	if (! wp->n.sgcols[nsgci].rfused.a) {

		wp->n.sgcols[nsgci].rfused.a = TRUE ;
		rs = rfi ;

	} else if (! wp->n.sgcols[nsgci].rfused.b) {

		wp->n.sgcols[nsgci].rfused.b = TRUE ;
		rs = rfi + lip->nsgrows ;

	} else
		rs = SR_INVALID ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("iw_rfalloc: rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (iw_rfalloc) */


/* free up a RF unit */
int iw_rffree(wp,pip,lip,rfi)
IW		*wp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		rfi ;
{
	int	rs = SR_OK ;
	int	sgci ;
	int	maxrfi, r ;


#if	F_MASTERDEBUG && F_SAFE3
	maxrfi = (lip->nsgrows + 1) * lip->nsgcols ;
	if ((rfi < 0) || (rfi >= maxrfi))
		return SR_INVALID ;
#endif

	sgci = rfi / (lip->nsgrows + 1) ;

#if	F_MASTERDEBUG && F_SAFE3
	r = rfi % (lip->nsgrows + 1) ;
	if ((r != 0) && (r != lip->nsgrows))
		return SR_INVALID ;
#endif

#if	F_MASTERDEBUG && F_SAFE3
	if (rfi == (sgci * (lip->nsgrows + 1)))
		wp->n.sgcols[sgci].rfused.a = FALSE ;

	else if (rfi == ((sgci * (lip->nsgrows + 1)) + lip->nsgrows))
		wp->n.sgcols[sgci].rfused.b = FALSE ;

	else
		rs = SR_NOANODE ;
#else
	if (rfi == (sgci * (lip->nsgrows + 1)))
		wp->n.sgcols[sgci].rfused.a = FALSE ;

	else
		wp->n.sgcols[sgci].rfused.b = FALSE ;
#endif /* F_SAFE3 */

	return rs ;
}
/* end subroutine (iw_rffree) */


/* return a bus ID according to the "global" allocation rules ! */
static int getbusid(lip,which)
struct levoinfo	*lip ;
int		which ;
{
	int	rs = -1 ;


	if (which < 0)
	    which = -1 ;

	switch (which) {

/* total number */
	case -1:
	    rs = lip->totalrows * lip->totalcols ;
	    rs += (lip->nsg * 2) ;
	    rs += (lip->nsgcols * 1) ;
	    rs += 1 ;
	    break ;

/* Active Stations */
	case 0:
	    rs = lip->totalrows * lip->totalcols ;
	    break ;

/* register filters or predicate forwards or memory filters */
	case 1:
	    rs = lip->totalrows * lip->totalcols ;
	    rs += (lip->nsg * 2) ;
	    break ;

/* Levo Memory Multiplexors */
	case 2:
	    rs = lip->totalrows * lip->totalcols ;
	    rs += (lip->nsg * 2) ;
	    rs += (lip->nsgcols * 1) ;
	    break ;

/* Levo Write Queue */
	case 3:
	    rs = lip->totalrows * lip->totalcols ;
	    rs += (lip->nsg * 2) ;
	    rs += (lip->nsgcols * 1) ;
	    rs += 1 ;
	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (getbusid) */


static uint ceiling(v,a)
uint	v, a ;
{


	return ((v + (a - 1)) & (~ (a - 1))) ;
}
/* end subroutine (ceiling) */



