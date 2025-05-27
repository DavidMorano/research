/* levo */

/* top-level Levo object */


#define	F_DEBUGS	0		/* non-switchable */
#define	F_DEBUG		0		/* switchable */
#define	F_DEBUG2	0		/* switchable */
#define	F_MEMBUSZERO	1		/* zero out the memory bus objects ? */
#define	F_PARAMS	1		/* get parameters */

#define	F_ALL		1		/* turn "ON" most of Levo ? */
#define	F_IW		1		/* instruction window stuff */
#define	F_LMEM		0		/* enable memory subsystem */
#define	F_MBUSES	1
#define	F_IFB		1		/* instruction fetch bus */
#define	F_IRB		1		/* instruction response bus */
#define	F_LIBUS		1		/* handle LIBUS objects yet ? */

#define	F_TESTMOD	0		/* the test module stuff */
#define	F_BUSMON	1		/* bus monitor testing */
#define	F_BUSTEST	1		/* bus write testing */
#define	F_TESTSCHED	0		/* test simulator scheduling */

#define	F_SAFE		1		/* safe mode */
#define	F_SAFE2		1		/* more safe mode */

#define	F_MINHEIGHT	1		/* enforce a minumum AS height */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


	= 01/07/19, Dave Morano

	I added a call to 'proginfo_levoconf()' to record
	the summary configuration string someplace.


	= 01/08/06, Dave Morano

	I added a parameter to 'levo_init()' to handle the slipping
	of instructions at start-up.


*/




/******************************************************************************

	This is the top object for the Levo machine.  Under this object
	all other Levo machine specific objects are eventually
	instantiated.  This object instantiates only the most top
	level objects.  All other objects making up the machine
	are instantiated as subobject from the top most ones.

	Our instantiated subobjects are part of our own object state.
	We will initialize our subobjects our through our own
	object initialization.


******************************************************************************/




#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<netorder.h>

#include	"localmisc.h"
#include	"paramfile.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"		/* simulator information */
#include	"statemips.h"		/* get the reg defs */
#include	"xmlinfo.h"		/* get the reg defs */

#include	"levoinfo.h"		/* levo information */
#include	"bus.h"			/* generic buses */
#include	"iw.h"			/* instruction window */
#include	"lmem.h"		/* memory subsystem */
#include	"lbusint.h"		/* testing */
#include	"busmon.h"		/* testing */
#include	"bustest.h"		/* testing */
#include	"rfbus.h"		/* Registered Flow Bus */

#include	"levo.h"		/* ourselves */



/* local defines */

#define	LEVO_MAGIC	0x65544332
#define	LEVO_MINMODULUS	4

#undef	BUFLEN
#define	BUFLEN		80



/* external subroutines */

extern int	cfdecui(char *,int,uint *) ;
extern int	cfnumui(char *,int,uint *) ;
extern int	ffbsi(uint), flbsi(uint) ;
extern int	getnumbuses(uint) ;


/* local structures */


/* forward references */

int		levo_shift(LEVO *) ;

static int	levo_sanitycheck(LEVO *) ;
static int	levo_testinit(LEVO *) ;
static int	levo_testfree(LEVO *) ;
static int	levo_testcomb(LEVO *,int) ;
static int	levo_testclock(LEVO *) ;
static int	levo_callback(LEVO *,void *) ;
static int	levo_initmbuses(LEVO *,struct proginfo *) ;
static int	levo_xmlconfig(LEVO *,XMLINFO *) ;
static int	levo_xmlout(LEVO *,XMLINFO *) ;

static uint	getnextpow(uint) ;

#ifdef	COMMENT
static int	bus_freemany(BUS *,int) ;
#endif


/* local data */

static char	*const lparams[] = {
	    "nsgrows",		/* number of SG rows  */
	    "nsgcols",		/* number of SG columns */
	    "nasrpsg",		/* number of ASs high per SG */
	    "nrfspan",		/* register forwarding bus SG span */
	    "nrbspan",		/* register backwarding bus SG span */
	    "npfspan",		/* predicate forwarding bus SG span */
	    "nmfspan",		/* memory forwarding bus SG span */
	    "nmbspan",		/* memory backwarding bus SG span */
	    "nprpas",		/* predicate register sets per AS */
	    "ifetchwidth",		/* I-fetch bus width in 32-bit words */
	    "meminter",		/* default memory interleave schedule */
	    "levo:bustrace",	/* bus trace */
	    "levo:mastertrace",	/* bus master trace (LBUSINT only) */
	    "ndeepaths",		/* number of DEE paths to model */
	    "btrbsize",		/* Branch Tracking Buffer size */
	    "nloadbufs",		/* number of Levo Load Buffers */
	    "mfinter",		/* memory forward interleave schedule */
	    "mbinter",		/* memory backward interleave schedule */
	    "mwinter",		/* memory write interleave schedule */
	    "wmfinter",		/* execution window MF interleave schedule */
	    "wmbinter",		/* execution window MW interleave schedule */
	    "rfmod",		/* register forwarding (RF) modulus */
	    "rbmod",		/* register backwarding (RB) modulus */
	    "pbmod",		/* predicate forwarding (PF) modulus */
	    NULL
} ;

#define	LPARAM_NSGROWS		0
#define	LPARAM_NSGCOLS		1
#define	LPARAM_NASRPSG		2
#define	LPARAM_NRFSPAN		3
#define	LPARAM_NRBSPAN		4
#define	LPARAM_NPFSPAN		5
#define	LPARAM_NMFSPAN		6
#define	LPARAM_NMBSPAN		7
#define	LPARAM_NPRPAS		8
#define	LPARAM_IFETCHWIDTH	9
#define	LPARAM_MEMINTER		10
#define	LPARAM_BUSTRACE		11
#define	LPARAM_MASTERTRACE	12
#define	LPARAM_NDEEPATHS	13
#define	LPARAM_BTRBSIZE		14
#define	LPARAM_NLOADBUFS	15
#define	LPARAM_MFINTER		16
#define	LPARAM_MBINTER		17
#define	LPARAM_MWINTER		18
#define	LPARAM_WMFINTER		19
#define	LPARAM_WMBINTER		20
#define	LPARAM_RFMOD		21
#define	LPARAM_RBMOD		22
#define	LPARAM_PFMOD		23








int levo_init(lp,pip,pfp,mip,smp)
LEVO			*lp ;
struct proginfo		*pip ;
PARAMFILE		*pfp ;
LSIM			*mip ;
struct statemips	*smp ;
{
	struct levoinfo	*lip ;

	uint	n, uv ;

	int	rs, i ;
	int	sl, v1, v2 ;
	int	size ;
	int	f_meminter ;
	int	f_mfinter, f_mbinter, f_mwinter ;
	int	f_wmfinter, f_wmbinter ;

	char	buf[BUFLEN + 2] ;
	char	btfname[MAXPATHLEN + 1] ;
	char	mtfname[MAXPATHLEN + 1] ;
	char	*cp ;


	if (lp == NULL)
	    return SR_FAULT ;

#if	F_DEBUGS
	eprintf("levo_init: entered, LEVO=%08lx\n",lp) ;
	eprintf("levo_init: PIP=%08lx\n",pip) ;
#endif

/* local initialization */

	btfname[0] = '\0' ;
	mtfname[0] = '\0' ;


/* object initialization */

	(void) memset(lp,0,sizeof(LEVO)) ;

	lp->magic = 0 ;

	lp->f.exit = FALSE ;

	lp->pip = pip ;
	lp->mip = mip ;
	lip = &lp->info ;


/* 'levoinfo' struct initialization */

#ifdef	OPTIONAL
	(void) memset(lip,0,sizeof(struct levoinfo)) ;
#endif

	lp->info.mip = mip ;
	lp->info.btfp = NULL ;

/* some miscellaneous stuff */

	lip->btrbsize = LEVOINFO_BTRBSIZE ;
	lip->nloadbufs = LEVOINFO_NLOADBUFS ;


/* some of the broken ones (can't currently model) */

	lip->npaths = LEVOINFO_NPATHS ;

/* the rest are just defaults */

	lp->info.nsgrows = LEVOINFO_NSGROWS ;
	lp->info.nsgcols = LEVOINFO_NSGCOLS ;
	lp->info.nrfspan = LEVOINFO_NFSPAN ;
	lp->info.nrbspan = LEVOINFO_NFSPAN ;
	lp->info.npfspan = LEVOINFO_NFSPAN ;
	lp->info.nmfspan = LEVOINFO_NFSPAN ;
	lp->info.nmbspan = LEVOINFO_NFSPAN ;

	lp->info.nasrpsg = LEVOINFO_NASRPSG ;
	lp->info.nprpas = LEVOINFO_NPRPAS ;
	lp->info.ifetchwidth = LEVOINFO_IFETCHWIDTH ;
	lp->info.meminter = LEVOINFO_MEMINTER & (~ 3) ;

	n = getnextpow(LEVOINFO_MODULUS) ;

	lp->info.rfmod = n ;
	lp->info.rbmod = n ;
	lp->info.pfmod = n ;

/* memory interleave schedules */

	f_meminter = FALSE ;

	lp->info.mfinter = lp->info.meminter ;
	lp->info.mbinter = lp->info.meminter ;
	lp->info.mwinter = lp->info.meminter ;

	f_mfinter = f_mbinter = f_mwinter = FALSE ;

/* execution window memory interleave schedules */

	lp->info.wmfinter = lp->info.meminter ;
	lp->info.wmbinter = lp->info.meminter ;

	f_wmfinter = f_wmbinter = FALSE ;

/* other (testing) */

	lp->info.btfp = NULL ;


/* get the height of the machine (column height in sharing groups) */

#if	F_PARAMS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 3)
	    eprintf("levo_init: getting parameters\n") ;
#endif

	for (i = 0 ; lparams[i] != NULL ; i += 1) {

	    if ((sl = paramfile_fetch(pfp,lparams[i],NULL,&cp)) >= 0) {

	        switch (i) {

	        case LPARAM_NSGROWS:
	        case LPARAM_NSGCOLS:
	        case LPARAM_NASRPSG:
	        case LPARAM_NRFSPAN:
	        case LPARAM_NRBSPAN:
	        case LPARAM_NPFSPAN:
	        case LPARAM_NMFSPAN:
	        case LPARAM_NMBSPAN:
	        case LPARAM_NPRPAS:
	        case LPARAM_IFETCHWIDTH:
	        case LPARAM_MEMINTER:
	        case LPARAM_NDEEPATHS:
	        case LPARAM_BTRBSIZE:
	        case LPARAM_NLOADBUFS:
	        case LPARAM_MFINTER:
	        case LPARAM_MBINTER:
	        case LPARAM_MWINTER:
	        case LPARAM_WMFINTER:
	        case LPARAM_WMBINTER:
	        case LPARAM_RFMOD:
	        case LPARAM_RBMOD:
	        case LPARAM_PFMOD:
	            if (cfnumui(cp,sl,&uv) >= 0) {

	                switch (i) {

	                case LPARAM_NSGROWS:
	                    lip->nsgrows = uv ;
	                    break ;

	                case LPARAM_NSGCOLS:
	                    lip->nsgcols = uv ;
	                    break ;

	                case LPARAM_NASRPSG:
	                    lip->nasrpsg = uv ;
	                    break ;

	                case LPARAM_NRFSPAN:
	                    lip->nrfspan = uv ;
	                    break ;

	                case LPARAM_NRBSPAN:
	                    lip->nrbspan = uv ;
	                    break ;

	                case LPARAM_NPFSPAN:
	                    lip->npfspan = uv ;
	                    break ;

	                case LPARAM_NMFSPAN:
	                    lip->nmfspan = uv ;
	                    break ;

	                case LPARAM_NMBSPAN:
	                    lip->nmbspan = uv ;
	                    break ;

	                case LPARAM_NPRPAS:
	                    lip->nprpas = uv ;
	                    break ;

	                case LPARAM_IFETCHWIDTH:
	                    lip->ifetchwidth = uv ;
	                    break ;

	                case LPARAM_MEMINTER:
	                    lip->meminter = uv & (~ 3) ;
	                    f_meminter = TRUE ;
	                    break ;

	                case LPARAM_NDEEPATHS:
	                    lip->npaths = uv + 1 ;
	                    break ;

	                case LPARAM_BTRBSIZE:
	                    lip->btrbsize = uv ;
	                    break ;

	                case LPARAM_NLOADBUFS:
	                    lip->nloadbufs = uv ;
	                    break ;

	                case LPARAM_MFINTER:
	                    lip->mfinter = uv & (~ 3) ;
	                    f_mfinter = TRUE ;
	                    break ;

	                case LPARAM_MBINTER:
	                    lip->mbinter = uv & (~ 3) ;
	                    f_mbinter = TRUE ;
	                    break ;

	                case LPARAM_MWINTER:
	                    lip->mwinter = uv & (~ 3) ;
	                    f_mwinter = TRUE ;
	                    break ;

	                case LPARAM_WMFINTER:
	                    lip->wmfinter = uv & (~ 3) ;
	                    f_wmfinter = TRUE ;
	                    break ;

	                case LPARAM_WMBINTER:
	                    lip->wmbinter = uv & (~ 3) ;
	                    f_wmbinter = TRUE ;
	                    break ;

	                case LPARAM_RFMOD:
	                    lip->rfmod = uv ;
	                    break ;

	                case LPARAM_RBMOD:
	                    lip->rbmod = uv ;
	                    break ;

	                case LPARAM_PFMOD:
	                    lip->pfmod = uv ;
	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        case LPARAM_BUSTRACE:
	            strcpy(btfname,cp) ;

	            break ;

	        case LPARAM_MASTERTRACE:
	            strcpy(mtfname,cp) ;

	            break ;

	        } /* end switch (outer) */

	    } /* end if (got a good parameter) */

	} /* end for (getting the geometry-like parameters) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: done getting parameters\n") ;
#endif

#endif /* F_PARAMS */


/* some state initialization */

	lp->c.verifier = lp->n.verifier = 0 ;


/* some checks on the parameters */

	if (lip->npaths > LEVOINFO_NPATHS)
	    lip->npaths = LEVOINFO_NPATHS ;

	if (lp->info.nsgrows < 1)
	    lp->info.nsgrows = 1 ;

	if (lp->info.nsgcols < 1)
	    lp->info.nsgcols = 1 ;

	if (lp->info.nasrpsg < 1)
	    lp->info.nasrpsg = 1 ;

#if	F_MINHEIGHT
	if ((lp->info.nasrpsg * lp->info.nsgrows) < 2)
	    lp->info.nasrpsg = 2 ;
#endif

/* check the various interleave schedules */

	if (f_meminter) {

	    if (! f_mfinter)
	        lip->mfinter = lip->meminter ;

	    if (! f_mbinter)
	        lip->mbinter = lip->meminter ;

	    if (! f_mwinter)
	        lip->mwinter = lip->meminter ;

	    if (! f_wmfinter)
	        lip->wmfinter = lip->meminter ;

	    if (! f_wmbinter)
	        lip->wmbinter = lip->meminter ;

	} /* end if */


	if (lip->rfmod < LEVO_MINMODULUS)
	    lip->rfmod = LEVO_MINMODULUS ;

	if (lip->rbmod < LEVO_MINMODULUS)
	    lip->rbmod = LEVO_MINMODULUS ;

	if (lip->pfmod < LEVO_MINMODULUS)
	    lip->pfmod = LEVO_MINMODULUS ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: rfmod=%d rbmod=%d pfmod=%d\n",
	        lip->rfmod, lip->rbmod, lip->pfmod) ;
#endif


#if	F_ALL

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: doing the all stuff\n") ;
#endif

/* all AS columns in the machine */

	lp->info.totalcols = lp->info.nsgcols * LEVOINFO_NASWPSG ;


/* some observations */

	lp->info.nsg = lp->info.nsgrows * lp->info.nsgcols ;
	lp->info.totalrows = lp->info.nsgrows * lp->info.nasrpsg ;


/* maximum spans */

	n = lp->info.nsg - 1 ;

	if (lp->info.nrfspan > n)
	    lp->info.nrfspan = n ;

	if (lp->info.nrbspan > n)
	    lp->info.nrbspan = n ;

	if (lp->info.npfspan > n)
	    lp->info.npfspan = n ;

/* some spans must be a minimum of 1 (when the machine is larger than one SG) */

	if (lp->info.nsg > 1) {

	    if (lp->info.nrfspan < 1)
	        lp->info.nrfspan = 1 ;

	    if (lp->info.nrbspan < 1)
	        lp->info.nrbspan = 1 ;

	    if (lp->info.npfspan < 1)
	        lp->info.npfspan = 1 ;

	} /* end if (more than one SG in the machine) */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("levo_init: nrfspan=%d nrbspan=%d npfspan=%d\n",
	        lp->info.nrfspan,lp->info.nrbspan,lp->info.npfspan) ;
	    eprintf("levo_init: nmfspan=%d nmbspan=%d\n",
	        lp->info.nmfspan,lp->info.nmbspan) ;
	}
#endif


/* OK, tell somebody about our configuration summary */

	sl = bufprintf(buf,(BUFLEN + 1),"%d-%d-%d-%d-%d",
	    lip->nsgrows,
	    lip->nsgcols,
	    lip->nasrpsg,
	    lip->nrfspan,
	    lip->nrbspan) ;

	if (sl > 0)
	    proginfo_levoconf(pip,buf,sl) ;


/* find the ceiling log base 2 of the number of total AS rows */

	v1 = flbsi(lp->info.totalrows) ;

	v2 = ffbsi(lp->info.totalrows) ;

	lp->info.logrows = (v1 == v2) ? v1 : v1 + 1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: ceiling log base 2 of total AS rows=%d\n",
	        lp->info.logrows) ;
#endif


/* OK, we now create the structures that are at this level ! */


/* the instruction memory buses (fetch request and response) */

	lp->info.ifbp = NULL ;
	lp->info.irbp = NULL ;

/* instruction fetch requests */

#if	F_IFB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: IFB=%08lx\n",
	        &lp->ifb) ;
#endif

	rs = rfbus_init(&lp->ifb,pip,lip,1,1) ;

	if (rs < 0)
	    goto bad0 ;

	lp->info.ifbp = &lp->ifb ;

#endif /* F_IFB */

/* instruction responses */

#if	F_IRB && F_LIBUS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: IRB=%08lx\n",
	        &lp->irb) ;
#endif

	rs = libus_init(&lp->irb,pip,mip,lip,lip->ifetchwidth) ;

	if (rs < 0)
	    goto bad1 ;

	lp->info.irbp = &lp->irb ;

#endif /* F_IRB && F_LIBUS */


/* the default number of buses */

	lip->nmembuses = getnumbuses(lip->meminter) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: nmembuses=%d\n",n) ;
#endif

/* the memory subsystem memory buses */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {
	    eprintf("levo_init: MF interleave=%08x\n",lip->mfinter) ;
	    eprintf("levo_init: MB interleave=%08x\n",lip->mbinter) ;
	}
#endif

	lip->nmfbuses = getnumbuses(lip->mfinter) ;

	lip->nmbbuses = getnumbuses(lip->mbinter) ;

	lip->nmwbuses = getnumbuses(lip->mwinter) ;


/* allocate memory buses */

	lp->info.mfbuses = NULL ;
	lp->info.mbbuses = NULL ;
	lp->info.mwbuses = NULL ;

#if	F_MBUSES

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: allocating %d MFBUSES\n",
	        lip->nmfbuses) ;
#endif

	size = lip->nmfbuses * sizeof(RFBUS) ;
	rs = uc_malloc(size,&lp->info.mfbuses) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: uc_malloc() on MFB=%08lx rs=%d \n",
	        lp->info.mfbuses,rs) ;
#endif

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lp->info.mfbuses,size,"levo_init:mfbuses") ;
#endif

#if	F_MEMBUSZERO
	(void) memset(lp->info.mfbuses,0,size) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: allocating %d MBBUSES\n",
	        lip->nmbbuses) ;
#endif

	size = lip->nmbbuses * sizeof(RFBUS) ;
	rs = uc_malloc(size,&lp->info.mbbuses) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: uc_malloc() on MBBes rs=%d a=%08lx\n",
	        rs,lp->info.mbbuses) ;
#endif

	if (rs < 0)
	    goto bad3 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lp->info.mbbuses,size,"levo_init:mbbuses") ;
#endif

#if	F_MEMBUSZERO
	(void) memset(lp->info.mbbuses,0,size) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: allocating %d MWBUSES\n",
	        lip->nmwbuses) ;
#endif

	size = lip->nmwbuses * sizeof(RFBUS) ;
	rs = uc_malloc(size,&lp->info.mwbuses) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: uc_malloc() on MWBes rs=%d a=%08lx\n",
	        rs,lp->info.mwbuses) ;
#endif

	if (rs < 0)
	    goto bad4 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lp->info.mwbuses,size,"levo_init:mwbuses") ;
#endif

#if	F_MEMBUSZERO
	(void) memset(lp->info.mwbuses,0,size) ;
#endif

/* initialize them */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: initializing memory buses, default=%d\n",
	        lip->nmembuses) ;
#endif

	rs = levo_initmbuses(lp,pip) ;

	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("levo_init: levo_initmbuses() rs=%d\n",
	            rs) ;
#endif

	    goto bad5 ;

	} /* end if (bad initialization on memory buses) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: MFB=%08lx\n",
	        lp->info.mfbuses) ;
#endif

#endif /* F_MBUSES */


/* set the SHIFT subroutine into the LEVOINFO structure */

	lp->info.machineshift = &levo_shift ;


/* initialize the instruction window */

#if	F_IW

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: iw_init() i-window IW=%08lx\n",
	        &lp->win) ;
#endif

	{
	    IW_INITARGS	iwa ;


	    (void) memset(&iwa,0,sizeof(IW_INITARGS)) ;

	    iwa.sanity.func = levo_sanitycheck ;
	    iwa.sanity.arg = lp ;
	    iwa.smp = smp ;		/* state mips */

	    rs = iw_init(&lp->win,pip,pfp,mip,&lp->info,&iwa) ;

	} /* end block */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: iw_init() rs=%d\n",rs) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("levo_init: bad iw_init() rs=%d\n", rs) ;
#endif

	if (rs < 0)
	    goto bad6 ;

#endif /* F_IW */


/* initialize the memory */

#if	F_LMEM

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: LMEM=%08lx lmem_init()\n",
	        &lp->memsys) ;
#endif

	{
	    LMEM_ARGS	la ;


	    (void) memset(&la,0,sizeof(LMEM_ARGS)) ;

	    la.interleave = lp->info.meminter ;

	    rs = lmem_init(&lp->memsys,pip,pfp,mip,&lp->info,&la) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("levo_init: lmem_init() rs=%d\n",rs) ;
#endif

	} /* end block */

	if (rs < 0)
	    goto bad7 ;

#endif /* F_LMEM */

#endif /* F_ALL */


/* testing stuff */

#if	F_TESTMOD
	levo_testinit(lp) ;
#endif


/* do we have bus trace files to write to ? */

	if (btfname[0] != '\0') {

	    rs = bopen(&lp->btfile,btfname,"wct",0666) ;

	    if (rs >= 0) {

	        lip->btfp = &lp->btfile ;
	        bprintf(lip->btfp,"BUSTRACE\n") ;

	        bprintf(lip->btfp,
	            "clock    busid        op  tr  "
	            "p       tt      addr     val\n") ;

	    }

	} /* end if (bus trace initialization) */

	if (mtfname[0] != '\0') {

	    rs = bopen(&lp->mtfile,mtfname,"wct",0666) ;

	    if (rs >= 0) {

	        lip->mtfp = &lp->mtfile ;
	        bprintf(lip->mtfp,"MASTERTRACE\n") ;

	        bprintf(lip->mtfp,
	            "clock    masterid bus           op"
	            " tr p       tt     addr     val\n") ;

	    }

	} /* end if (bus trace initialization) */


/* what about some XML stuff ? */

	if (pip->f.xmltrace) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("levo_init: levo_xmlconfig()\n") ;
#endif

		levo_xmlconfig(lp,&pip->xi) ;

	} /* end if */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
			eprintf("levo_init: exiting OK\n") ;
#endif

	lp->magic = LEVO_MAGIC ;
	return SR_OK ;

/* bad thins come here */
bad8:

#if	F_LMEM

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_init: LMEM=%08lx lmem_free()\n",
	        &lp->memsys) ;
#endif

	(void) lmem_free(&lp->memsys) ;

#endif /* F_LMEM */

bad7:

#if	F_IW
	iw_free(&lp->win) ;
#endif

bad6:

bad5:

#if	F_MBUSES && F_ALL
	free(lip->mwbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(lip->mwbuses,"levo_init:mwbuses") ;
#endif

#endif /* F_MBUSES */

bad4:

#if	F_MBUSES && F_ALL
	free(lip->mbbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(lip->mbbuses,"levo_init:mbbuses") ;
#endif

#endif

bad3:

#if	F_MBUSES && F_ALL
	free(lip->mfbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(lip->mfbuses,"levo_init:mfbuses") ;
#endif

#endif

bad2:

#if	F_IRB && F_LIBUS
	libus_free(&lp->irb) ;
#endif

bad1:

#if	F_IFB && F_ALL
	rfbus_free(&lp->ifb) ;
#endif

bad0:

#if	F_DEBUGS
	eprintf("levo_init: exiting rs=%d\n",rs) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	eprintf("levo_init: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (levo_init) */


int levo_free(lp)
LEVO	*lp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs, i ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;

	if ((lp->magic != LEVO_MAGIC) && (lp->magic != 0)) {

	    eprintf("levo_free: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (lp->magic != LEVO_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lp->pip ;
	lip = &lp->info ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("levo_free: entered\n") ;
#endif


/* free up any of that testing stuff */

	if (lip->mtfp != NULL)
	    (void) bclose(lip->mtfp) ;

	if (lip->btfp != NULL)
	    (void) bclose(lip->btfp) ;


#if	F_TESTMOD
	levo_testfree(lp) ;
#endif


#if	F_ALL


#if	F_IW
	iw_free(&lp->win) ;
#endif

#if	F_LMEM

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("levo_free: LMEM=%08lx lmem_free()\n",
	        &lp->memsys) ;
#endif

	(void) lmem_free(&lp->memsys) ;

#endif /* F_LMEM */

/* instruction response bus */

#if	F_IRB && F_LIBUS
	libus_free(&lp->irb) ;
#endif

/* instruction fetch request bus */

#if	F_IFB
	rfbus_free(&lp->ifb) ;
#endif


/* memory data buses */

#if	F_MBUSES

	for (i = 0 ; i < lp->info.nmwbuses ; i += 1)
	    rfbus_free(lp->info.mwbuses + i) ;

	free(lp->info.mwbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(lip->mwbuses,"levo_free:mwbuses") ;
#endif

	for (i = 0 ; i < lp->info.nmbbuses ; i += 1)
	    rfbus_free(lp->info.mbbuses + i) ;

	free(lp->info.mbbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(lip->mbbuses,"levo_free:mbbuses") ;
#endif

	for (i = 0 ; i < lp->info.nmfbuses ; i += 1)
	    rfbus_free(lp->info.mfbuses + i) ;

	free(lp->info.mfbuses) ;

#ifdef	MALLOCLOG
	malloclog_free(lip->mfbuses,"levo_free:mfbuses") ;
#endif

#endif /* F_MBUSES */


#endif /* F_ALL */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("levo_free: exiting\n") ;
#endif

	lp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (levo_free) */


/* perform the combinatorial computations for this object */
int levo_comb(lp,phase)
LEVO	*lp ;
int	phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	rs = SR_OK, rs1, i ;
	int	f_exit = FALSE ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;

	if ((lp->magic != LEVO_MAGIC) && (lp->magic != 0)) {

	    eprintf("levo_comb: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (lp->magic != LEVO_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lp->pip ;
	mip = lp->mip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: entered ck=%llu ph=%d\n",
	        mip->clock,phase) ;
#endif /* F_DEBUG */



#if	F_ALL

#if	F_MASTERDEBUG && F_SAFE2

/* check LEVO sanity */

	rs = levo_sanitycheck(lp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0)) {
	    eprintf("levo_comb: 0 bad sanity rs=%d\n",rs) ;
	    return rs ;
	}
#endif

#endif /* F_SAFE2 */


/* pop all buses */

#if	F_MBUSES

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: memory buses\n") ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: MF\n") ;
#endif

	for (i = 0 ; i < lp->info.nmfbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("levo_comb: RFBUS=%08lx i=%d\n",
	            lp->info.mfbuses + i,i) ;
#endif

	    rs = rfbus_comb(lp->info.mfbuses + i,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("levo_comb: rfbus_comb() rs=%d\n",rs) ;
#endif

	} /* end for */

#if	F_MASTERDEBUG && F_SAFE2

/* check LEVO sanity */

	rs = levo_sanitycheck(lp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0)) {
	    eprintf("levo_comb: 1 bad sanity rs=%d\n",rs) ;
	    return rs ;
	}
#endif

#endif /* F_SAFE2 */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: MB\n") ;
#endif

	for (i = 0 ; i < lp->info.nmbbuses ; i += 1) {

	    rs = rfbus_comb(lp->info.mbbuses + i,phase) ;

	    if (rs < 0)
	        return rs ;

	} /* end for */

#if	F_MASTERDEBUG && F_SAFE2

/* check LEVO sanity */

	rs = levo_sanitycheck(lp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0)) {
	    eprintf("levo_comb: 2 bad sanity rs=%d\n",rs) ;
	    return rs ;
	}
#endif

#endif /* F_SAFE2 */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: MW\n") ;
#endif

	for (i = 0 ; i < lp->info.nmwbuses ; i += 1) {

	    rs = rfbus_comb(lp->info.mwbuses + i,phase) ;

	    if (rs < 0)
	        return rs ;

	}

#if	F_MASTERDEBUG && F_SAFE2

/* check LEVO sanity */

	rs = levo_sanitycheck(lp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0)) {
	    eprintf("levo_comb: 3 bad sanity rs=%d\n",rs) ;
	    return rs ;
	}
#endif

#endif /* F_SAFE2 */

#endif /* F_MBUSES */


/* pop our other subobjects */

#if	F_IW

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: IW\n") ;
#endif

	rs = iw_comb(&lp->win,phase) ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_BADFMT) || (rs == SR_NOTOPEN) || (rs == SR_FAULT)) {

	    eprintf("levo_comb: IW bad something (rs=%d)\n",rs) ;

	    return rs ;
	}
#endif /* F_SAFE */

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("levo_comb: iw_comb() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* schedule an exit ? */

	if (! lp->f.exit)
	    lp->f.exit = (rs > 0) ;

#endif /* F_IW */


#if	F_LMEM

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: LMEM=%08lx\n",
	        &lp->memsys) ;
#endif

	rs = lmem_comb(&lp->memsys,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: lmem_comb() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

#endif /* F_LMEM */

/* instruction fetch request bus */

#if	F_IFB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: IFB\n") ;
#endif

	rs = rfbus_comb(&lp->ifb,phase) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: IFB rfbus_comb() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

#endif /* F_IFB */

/* instruction response bus */

#if	F_IRB && F_LIBUS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: IRB\n") ;
#endif

	rs = libus_comb(&lp->irb,phase) ;

	if (rs < 0)
	    return rs ;

#endif


#endif /* F_ALL */


#if	F_TESTMOD
	levo_testcomb(lp,phase) ;
#endif


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: own activity ph=%d\n",phase) ;
#endif

/* do our own stuff */

	switch (phase) {

	case 2:

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("levo_comb: does the machine need to shift ?\n") ;
#endif

	    rs = iw_needshift(&lp->win) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("levo_comb: iw_needshift() rs=%d\n",rs) ;
#endif

	    if (rs > 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("levo_comb: signalling for a machine shift !\n") ;
#endif

	        rs = levo_shift(lp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("levo_comb: levo_shift() rs=%d\n",rs) ;
#endif

	    } /* end if (signalling the shifting of the machine) */

	    break ;

	case 3:
	    rs1 = xmlinfo_enabled(&pip->xi,mip->clock) ;

	    if (rs1 > 0) {

/* write the index file */

	        if (pip->xi.ifname != NULL) {

	            off_t	xo ;

	            int	ckback ;

	            char	ientry[8] ;


	            btell(&pip->xi.xmlfile,&xo) ;

	            netorder_wint(ientry,xo) ;

	            ckback = mip->clock - pip->xi.lastsync ;
	            netorder_wshort(ientry + 4,ckback) ;

	            bwrite(&pip->xi.ifile,ientry,6) ;

	        } /* end if (index file) */

/* write out the XML stuff */

	        bprintf(&pip->xi.xmlfile,"<clock>\n") ;

	        bprintf(&pip->xi.xmlfile,"<value>%lld</value>\n",
	            mip->clock) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("levo_comb: levo_xmlout() ck=%lld\n",
			mip->clock) ;
#endif

	        rs1 = levo_xmlout(lp,&pip->xi) ;

	        bprintf(&pip->xi.xmlfile,"</clock>\n") ;

	    } /* end if (XML trace output enabled) */

	    break ;

/* private stuff */
	case 4:
	    f_exit = lp->f.exit ;
	    break ;

	} /* end switch */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_comb: exiting rs=%d\n",rs) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs >= 0) && f_exit)
	    eprintf("levo_comb: simulator exit signalled\n") ;
#endif

	return ((rs >= 0) ? f_exit : rs) ;
}
/* end subroutine (levo_comb) */


/* handle a machine clock transition */
int levo_clock(lp)
LEVO	*lp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;

	if ((lp->magic != LEVO_MAGIC) && (lp->magic != 0)) {

	    eprintf("levo_clock: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (lp->magic != LEVO_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: entering LEVO=%08lx\n",lp) ;
#endif


#if	F_ALL

/* pop all buses */

#if	F_MBUSES

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: buses\n") ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: MFB=%08lx\n",
	        lp->info.mfbuses) ;
#endif

	for (i = 0 ; i < lp->info.nmfbuses ; i += 1)
	    rfbus_clock(lp->info.mfbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: bus MB\n") ;
#endif

	for (i = 0 ; i < lp->info.nmbbuses ; i += 1)
	    rfbus_clock(lp->info.mbbuses + i) ;

	for (i = 0 ; i < lp->info.nmwbuses ; i += 1)
	    rfbus_clock(lp->info.mwbuses + i) ;

#endif /* F_MBUSES */


/* pop our subobjects */

#if	F_IW

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: IW \n") ;
#endif

	rs = iw_clock(&lp->win) ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_BADFMT) || (rs == SR_NOTOPEN) || (rs == SR_FAULT)) {

	    eprintf("levo_clock: IW bad format (%d)\n",rs) ;

	    return rs ;
	}
#endif /* F_SAFE */

#endif /* F_IW */

#if	F_LMEM

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: LMEM=%08lx lmem_clock()\n",
	        &lp->memsys) ;
#endif

	rs = lmem_clock(&lp->memsys) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: lmem_clock() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

#endif /* F_LMEM */

/* instruction fetch request bus */

#if	F_IFB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: IFB bus\n") ;
#endif

	rfbus_clock(&lp->ifb) ;
#endif

/* instruction response bus */

#if	F_IRB && F_LIBUS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: LIBUS\n") ;
#endif

	libus_clock(&lp->irb) ;
#endif

#endif /* F_ALL */

/* do our own clock stuff */

	lp->c = lp->n ;


/* do some test stuff */

#if	F_TESTMOD
	levo_testclock(lp) ;
#endif


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_clock: exiting\n") ;
#endif

	return SR_OK ;
}
/* end subroutine (levo_clock) */


/* signal a shift of the machine ! */
int levo_shift(lp)
LEVO	*lp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;

	if ((lp->magic != LEVO_MAGIC) && (lp->magic != 0)) {

	    eprintf("levo_shift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (lp->magic != LEVO_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: entering LEVO=%p\n",lp) ;
#endif


#if	F_ALL

/* pop all buses */

#if	F_MBUSES

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: buses\n") ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: MF\n") ;
#endif

	for (i = 0 ; i < lp->info.nmfbuses ; i += 1)
	    rs = rfbus_shift(lp->info.mfbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: bus MB\n") ;
#endif

	for (i = 0 ; i < lp->info.nmbbuses ; i += 1)
	    rs = rfbus_shift(lp->info.mbbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: bus MW\n") ;
#endif

	for (i = 0 ; i < lp->info.nmwbuses ; i += 1)
	    rs = rfbus_shift(lp->info.mwbuses + i) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: final bus rfbus_shift() rs=%d\n",rs) ;
#endif

#endif /* F_MBUSES */


/* pop our subobjects */

#if	F_IW

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: IW \n") ;
#endif

	rs = iw_shift(&lp->win) ;

#if	F_MASTERDEBUG && F_SAFE
	if ((rs == SR_BADFMT) || (rs == SR_NOTOPEN) || (rs == SR_FAULT)) {

	    eprintf("levo_shift: IW bad format (%d)\n",rs) ;

	    return rs ;
	}
#endif /* F_SAFE */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: iw_shift() rs=%d\n",rs) ;
#endif

#endif /* F_IW */

#if	F_LMEM

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: LMEM=%08lx\n",
	        &lp->memsys) ;
#endif

	rs = lmem_shift(&lp->memsys) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: lmem_shift() rs=%d\n",rs) ;
#endif

#endif /* F_LMEM */


/* instruction fetch request bus */

#if	F_IFB

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: IFB bus\n") ;
#endif

	rs = rfbus_shift(&lp->ifb) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: IFB rfbus_shift() rs=%d\n",rs) ;
#endif

#endif /* F_IFB */

/* instruction response bus */

#if	F_IRB && F_LIBUS

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: LIBUS\n") ;
#endif

	rs = libus_shift(&lp->irb) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: IRB libus_shift() rs=%d\n",rs) ;
#endif

#endif /* F_IRB && F_LIBUS */


#endif /* F_ALL */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_shift: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (levo_shift) */


int levo_statfile(lp,fp)
LEVO	*lp ;
bfile	*fp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;

	if ((lp->magic != LEVO_MAGIC) && (lp->magic != 0)) {

	    eprintf("levo_statistics: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (lp->magic != LEVO_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = lp->pip ;
	lip = &lp->info ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_statfile: entered\n") ;
#endif


/* dump the machine configuration */

	bprintf(fp,"\n\nmachine configuration :\n") ;

	bprintf(fp,
	    "SG rows                            %16d\n",
	    lip->nsgrows) ;

	bprintf(fp,
	    "SG columns                         %16d\n",
	    lip->nsgcols) ;

	bprintf(fp,
	    "AS rows per SG                     %16d\n",
	    lip->nasrpsg) ;

	bprintf(fp,
	    "register forwarding bus span       %16d\n",
	    lip->nrfspan) ;

	bprintf(fp,
	    "register backwarding bus span      %16d\n",
	    lip->nrbspan) ;

	bprintf(fp,
	    "predicate forwarding bus span      %16d\n",
	    lip->npfspan) ;

	bprintf(fp,
	    "width of instruction fetch bus     %16d\n",
	    lip->ifetchwidth) ;

	bprintf(fp,
	    "memory interleave schedule               \\x%08x\n",
	    lip->meminter) ;

	bprintf(fp,
	    "number of interleaved memory buses %16d\n",
	    lip->nmembuses) ;


/* gather statistics from the buses that go through us */



/* have certain subobjects write out their statistics */

	rs = iw_statfile(&lp->win,fp) ;

	if (rs < 0)
	    return rs ;


#ifdef	COMMENT
	rs = lmem_statfile(&lp->memsys,fp) ;

	if (rs < 0)
	    return rs ;

#endif /* COMMENT */


	return rs ;
}
/* end subroutine (levo_statistics) */



/* PRIVATE SUBROUTINES */



/* XML machine configuration */
static int levo_xmlconfig(lp,xip)
LEVO	*lp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	struct mintinfo	*mip ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;
#endif /* F_SAFE */

	pip = lp->pip ;
	mip = lp->mip ;

	lip = &lp->info ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_xmlconfig: entered XIP=%p\n",xip) ;
#endif

/* opener */

	bprintf(&xip->xmlfile,"<configuration>\n") ;

	bprintf(&xip->xmlfile,"<nsgrows>%d</nsgrows>\n",
		lip->nsgrows) ;

	bprintf(&xip->xmlfile,"<nsgcols>%d</nsgcols>\n",
		lip->nsgcols) ;

	bprintf(&xip->xmlfile,"<nasrpsg>%d</nasrpsg>\n",
		lip->nasrpsg) ;

	bprintf(&xip->xmlfile,"<nrfspan>%d</nrfspan>\n",
		lip->nrfspan) ;

	bprintf(&xip->xmlfile,"<nrbspan>%d</nrbspan>\n",
		lip->nrbspan) ;

	bprintf(&xip->xmlfile,"<npfspan>%d</npfspan>\n",
		lip->npfspan) ;

	bprintf(&xip->xmlfile,"<nmfspan>%d</nmfspan>\n",
		lip->nmfspan) ;

	bprintf(&xip->xmlfile,"<nmbspan>%d</nmbspan>\n",
		lip->nmbspan) ;

	bprintf(&xip->xmlfile,"<ifetchwidth>%d</ifetchwidth>\n",
		lip->ifetchwidth) ;

	bprintf(&xip->xmlfile,"<mfinter>%d</mfinter>\n",
		lip->mfinter) ;

	bprintf(&xip->xmlfile,"<mbinter>%d</mbinter>\n",
		lip->mbinter) ;

	bprintf(&xip->xmlfile,"<mwinter>%d</mwinter>\n",
		lip->mwinter) ;

	bprintf(&xip->xmlfile,"<wmfinter>%d</wmfinter>\n",
		lip->wmfinter) ;

	bprintf(&xip->xmlfile,"<wmbinter>%d</wmbinter>\n",
		lip->wmbinter) ;

	bprintf(&xip->xmlfile,"<nprpas>%d</nprpas>\n",
		lip->nprpas) ;

	bprintf(&xip->xmlfile,"<btrbsize>%d</btrbsize>\n",
		lip->btrbsize) ;

	bprintf(&pip->xi.xmlfile,"</configuration>\n") ;

	return SR_OK ;
}
/* end subroutine (levo_xmlconf) */


/* XML state trace */
static int levo_xmlout(lp,xip)
LEVO	*lp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	struct mintinfo	*mip ;

	int	rs = SR_OK, i ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;

	if (lp->magic != LEVO_MAGIC) {

	    rs = (lp->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
	    return rs ;
	}
#endif /* F_SAFE */


	pip = lp->pip ;
	mip = lp->mip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_xmlout: entered XIP=%p\n",xip) ;
#endif

/* opener */

	bprintf(&pip->xi.xmlfile,"<levo>\n") ;

/* middle */

	bprintf(&pip->xi.xmlfile,"<busgroup>\n") ;

	bprintf(&pip->xi.xmlfile,"<name>%s</name>\n",
	    "memory forwarding") ;

	for (i = 0 ; i < lp->info.nmfbuses ; i += 1) {

	    rs = rfbus_xmlout(lp->info.mfbuses + i,xip) ;

	    if (rs < 0)
	        return rs ;

	} /* end for */

	bprintf(&pip->xi.xmlfile,"</busgroup>\n") ;

	bprintf(&pip->xi.xmlfile,"<busgroup>\n") ;

	bprintf(&pip->xi.xmlfile,"<name>%s</name>\n",
	    "memory backwarding") ;

	for (i = 0 ; i < lp->info.nmbbuses ; i += 1) {

	    rs = rfbus_xmlout(lp->info.mbbuses + i,xip) ;

	    if (rs < 0)
	        return rs ;

	} /* end for */

	bprintf(&pip->xi.xmlfile,"</busgroup>\n") ;

	bprintf(&pip->xi.xmlfile,"<busgroup>\n") ;

	bprintf(&pip->xi.xmlfile,"<name>%s</name>\n",
	    "memory write") ;

	for (i = 0 ; i < lp->info.nmwbuses ; i += 1) {

	    rs = rfbus_xmlout(lp->info.mwbuses + i,xip) ;

	    if (rs < 0)
	        return rs ;

	}

	bprintf(&pip->xi.xmlfile,"</busgroup>\n") ;


	bprintf(&pip->xi.xmlfile,"<busname>\n") ;

	bprintf(&pip->xi.xmlfile,"<name>%s</name>\n",
	    "instruction request") ;

	rs = rfbus_xmlout(&lp->ifb,xip) ;

	if (rs < 0)
	    return rs ;

	bprintf(&pip->xi.xmlfile,"</busname>\n") ;

	bprintf(&pip->xi.xmlfile,"<busname>\n") ;

	bprintf(&pip->xi.xmlfile,"<name>%s</name>\n",
	    "instruction response") ;

	rs = libus_xmlout(&lp->irb,xip) ;

	if (rs < 0)
	    return rs ;

	bprintf(&pip->xi.xmlfile,"</busname>\n") ;


/* the hot stuff */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_xmlout: IW and LMEM, XIP=%p\n",xip) ;
#endif

	rs = iw_xmlout(&lp->win,xip) ;


	rs = lmem_xmlout(&lp->memsys,xip) ;


/* closer */

	bprintf(&pip->xi.xmlfile,"</levo>\n") ;


	return rs ;
}
/* end subroutine (levo_xmlout) */


/* do some sanity checking */
static int levo_sanitycheck(lp)
LEVO	*lp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;


#if	F_MASTERDEBUG && F_SAFE
	if (lp == NULL)
	    return SR_FAULT ;

	if ((lp->magic != LEVO_MAGIC) && (lp->magic != 0)) {

	    eprintf("levo_sanitycheck: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (lp->magic != LEVO_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */


	pip = lp->pip ;


/* check IW sanity */

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	    eprintf("levo_sanitycheck: iw_sanitycheck()\n") ;
#endif

	rs = iw_sanitycheck(&lp->win) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0)) {
	    eprintf("levo_sanitycheck: bad sanity IW rs=%d\n",rs) ;
	    return rs ;
	}
#endif


#if	F_LMEM

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	    eprintf("levo_sanitycheck: lmem_sanitycheck()\n") ;
#endif

	rs = lmem_sanitycheck(&lp->memsys) ;

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if ((pip->debuglevel >= 2) && (rs < 0)) {
	    eprintf("levo_sanitycheck: bad sanity LMEM rs=%d\n",rs) ;
	}
#endif

#endif /* F_LMEM */


	return rs ;
}
/* end subroutine (levo_sanitycheck) */


static int levo_testinit(lp)
LEVO		*lp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs ;
	int	n, bi ;


	pip = lp->pip ;
	lip = &lp->info ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_testinit: our test BUS=%p\n",&lp->mybus) ;
#endif


	rs = bus_init(&lp->mybus,pip,4,1) ;

	if (rs < 0)
	    goto bad2 ;

#if	F_BUSMON
	busmon_init(&lp->testmon,pip,lp->mip,lip,&lp->mybus) ;
#endif

#if	F_BUSTEST
	n = bus_nmasters(&lp->mybus) ;

	bustest_init(&lp->testbus,pip,lp->mip,lip,&lp->mybus,0,n,9) ;
#endif

/* we're out of here */

	return SR_OK ;

/* bad things */
bad2:

bad1:

#if	F_DEBUGS
	eprintf("levo_testinit: bad init rs=%d\n",rs) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_testinit: badinit 2 rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (levo_testinit) */


static int levo_testfree(lp)
LEVO	*lp ;
{


#if	F_BUSTEST
	bustest_free(&lp->testbus) ;
#endif

#if	F_BUSMON
	busmon_free(&lp->testmon) ;
#endif

	bus_free(&lp->mybus) ;


	return SR_OK ;
}


static int levo_testcomb(lp,phase)
LEVO	*lp ;
int	phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	LFLOWGROUP	rd, wr ;

	ULONG	clocks ;

	int	i ;
	int	rs ;


	pip = lp->pip ;
	mip = lp->mip ;
	lip = &lp->info ;


	lsim_getclock(mip,&clocks) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_testcomb: entering ck=%llu ph=%d\n",
	        clocks,phase) ;
#endif

/* do our subobjects */

	bus_comb(&lp->mybus,phase) ;

#if	F_BUSMON
	busmon_comb(&lp->testmon,phase) ;
#endif

#if	F_BUSTEST
	bustest_comb(&lp->testbus,phase) ;
#endif


/* now do our own stuff */

#if	F_TESTSCHED

	switch (phase) {

	case 0:
	    {
	        ulong	val ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("levo_testcomb: lsim_callout\n") ;
#endif

	        val = (long) clocks ;
	        rs = lsim_callout(mip,levo_callback,lp,((void *) val),12,3) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("levo_testcomb: lsim_callout rs=%d\n",rs) ;
#endif

	    } /* end block */

	    break ;

	} /* end switch */

#endif /* F_TESTSCHED */


#if	F_TESTSCHED

	switch (phase) {

	case 3:
	    {
	        ULONG	clocks ;

	        lsim_getclock(mip,&clocks) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("levo_testcomb: callback ck=%llu\n",
	                clocks) ;
#endif

	    } /* end block */

	    break ;

	} /* end switch */

#endif /* F_TESTSCHED */


	return SR_OK ;
}
/* end subroutine (levo_testcomb) */


static int levo_testclock(lp)
LEVO	*lp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	pip = lp->pip ;


	bus_clock(&lp->mybus) ;

#if	F_BUSMON
	busmon_clock(&lp->testmon) ;
#endif

#if	F_BUSTEST

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_testclock: bustest_clock()\n") ;
#endif

	bustest_clock(&lp->testbus) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_testclock: lbusint_clock rs=%d\n",
	        rs) ;
#endif

	return SR_OK ;
}
/* end subroutine (levo_testclock) */


/* testing of simulator scheduling */
static int levo_callback(lp,ap)
LEVO	*lp ;
void	*ap ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	ULONG	clocks ;


	pip = lp->pip ;
	mip = lp->mip ;

	lsim_getclock(mip,&clocks) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_callback: arg=%08lx ck=%llu ph=%d\n",
	        ap,clocks,-1) ;
#endif

	return SR_OK ;
}
/* end subroutine (levo_callback) */


/* initialize the memory buses */
static int levo_initmbuses(lp,pip)
LEVO		*lp ;
struct proginfo	*pip ;
{
	struct levoinfo	*lip ;

	int	rs, i, j ;


	lip = &lp->info ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_initmbuses: entered\n") ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_initmbuses: MF\n") ;
#endif

	for (i = 0 ; i < lip->nmfbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("levo_initmbuses: MF RFBUS=%08lx i=%d\n",
	            lp->info.mfbuses + i,i) ;
#endif

	    rs = rfbus_init(lp->info.mfbuses + i,pip,lip,1,1) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("levo_initmbuses: rfbus_init() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0) {

	    for (j = 0 ; j < i ; j += 1)
	        rfbus_free(&lp->info.mfbuses[i]) ;

	    goto bad0 ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_initmbuses: MB\n") ;
#endif

	for (i = 0 ; i < lip->nmbbuses ; i += 1) {

	    rs = rfbus_init(&lp->info.mbbuses[i],pip,lip,1,1) ;

	    if (rs < 0)
	        break ;

	}

	if (rs < 0) {

	    for (j = 0 ; j < i ; j += 1)
	        rfbus_free(&lp->info.mbbuses[i]) ;

	    goto bad1 ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("levo_initmbuses: MW\n") ;
#endif

	for (i = 0 ; i < lip->nmwbuses ; i += 1) {

	    rs = rfbus_init(&lp->info.mwbuses[i],pip,lip,1,1) ;

	    if (rs < 0)
	        break ;

	}

	if (rs < 0) {

	    for (j = 0 ; j < i ; j += 1)
	        rfbus_free(&lp->info.mwbuses[i]) ;

	    goto bad2 ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("levo_initmbuses: initialized %d buses of each type\n",
	        lip->nmfbuses) ;
#endif

	return rs ;

/* bad things */
bad2:
	for (j = 0 ; j < lip->nmfbuses ; j += 1)
	    rfbus_free(&lp->info.mbbuses[i]) ;

bad1:
	for (j = 0 ; j < lip->nmfbuses ; j += 1)
	    rfbus_free(&lp->info.mfbuses[i]) ;

bad0:

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	eprintf("levo_initmbuses: exiting bad rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (levo_initmbuses) */


/* calculate the next power of 2 */
uint getnextpow(n)
uint	n ;
{
	uint	mask ;

	int	lb ;


	lb = flbsi(n) ;

	if (lb < 1)
	    return 1 ;

	mask = (1 << lb) - 1 ;
	if ((n & mask) && (lb < 31))
	    lb += 1 ;

	return (1 << lb) ;
}
/* end subroutine (getnextpow) */


#ifndef	COMMENT

static int bus_freemany(busp,n)
BUS	*busp ;
int	n ;
{
	int	i ;


	for (i = 0 ; i < n ; i += 1)
	    bus_free(busp + i) ;

	return SR_OK ;
}

#endif /* COMMENT */



