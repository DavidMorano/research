/* ssiw */

/* OpTiFlow execution (issue) window */


#define	CF_DEBUGS	0		/* non-switched debugging */
#define	CF_DEBUG	0		/* switched debugging */
#define	CF_SAFE		0		/* safe mode */
#define	CF_SAFE2	0		/* more safe mode */
#define	CF_SAFE3	0		/* extra sanity checks */
#define	CF_FPRINTF	1		/* do some fprintf() printing */
#define	CF_IJ		0		/* special handle indirect jumps? */
#define	CF_XML		0		/* include XML stuff */
#define	CF_STATFILE	1		/* status output file */
#define	CF_RETIRE	1		/* check for retirement */
#define	CF_GOTO		0		/* handle GOTOs out-of-window */
#define	CF_READYLOAD	0		/* check if AS is ready to load */
#define	CF_NOTALLREQ	0		/* doesn't really work (sigh) */
#define	CF_FETCH	1		/* use fetch stage */
#define	CF_ICACHE	1		/* use iCache */
#define	CF_BPRED	1		/* use branch predictor */
#define	CF_SSLSQ	0		/* use SSLSQ */
#define	CF_NDIPATCH	0		/* faster */


/* revision history:

	= 00/02/15, Dave Morano

	This code was started.


	= 00/09/18, Dave Morano

	I added the LBTRB object.


	= 00/09/28, Dave Morano

	Unfortunately much of this code with regard to initialization
	has to change!  The memory buses that are created and
	initialized in the LEVO object no longer are the same buses
	that go to the ASes.  This was expected anyway eventually so it
	may as well happen now.  New memory buses are created and
	initialized within the SSIW for use by the ASes.  Also all
	register and predicate buses are not entirely local to the LT
	also.  The LEVO object creates memory buses for use between
	the memory subsystem and the i-window.  Those buses are now
	made out of RFBUS objects while the memory buses that go
	to the ASes (here in this code) are still made out of BUS
	objects.


	= 00/12/30, Dave Morano

	Alireza found one problem with mutliple Sharing Groups not
	working.  This code was not calling the '_comb()' or '_clock()'
	methods of all of the LPE object!  Only the first LPE object
	was being handled correctly.  I fixed it simply with a loop to
	get all of them that are configured.


	= 01/02/20, Dave Morano

	I am just marking this version as the latest.  I fixed some
	REGFILTER manipulation issues that had to do with invalidation
	of register values and I put some extra debug prints in this
	version.


	= 03/07/02, Dave Morano

	I'm just getting around to putting a comment here.
	I brought this over a few months ago but am just now
	doing detailed hacks to try to make it work in the S-S
	framework.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	Levo Instruction Window

	This is the top of the i-window for the Levo machine.


	Design notes:

	The indication of the shift to all of the machine will occur in
	phase 2.  Is this OK??


******************************************************************************/


#define	SSIW_MASTER	1


#include	<sys/types.h>
#include	<sys/mman.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstdlib>
#include	<cstring>
#include	<math.h>
#include	<assert.h>

#if	CF_FPRINTF
#include	<cstdio>
#endif

#include	<usystem.h>
#include	<bio.h>
#include	<paramfile.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#if	CF_XML
#include	"xmlinfo.h"
#endif

#include	"ss.h"
#include	"ssinfo.h"
#include	"checker.h"
#include	"ssfetch.h"
#include	"ssreg.h"
#include	"sslsq.h"
#include	"ssas.h"
#include	"sspe.h"
#include	"ssiw.h"
#include	"ssnames.h"
#include	"opclass.h"

#include	"regs.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif



/* local defines */

#define	SSIW_MAGIC		0x91818273
#define	SSIW_RFTYPE		0	/* relay forwarding */

#ifndef	FNE_STATS
#define	FNE_STATS		".stats"
#endif
#ifndef	FNE_PESTATS
#define	FNE_PESTATS		".pestats"
#endif

#ifndef	FNE_ISRES
#define	FNE_ISRES		".isres"
#endif



/* external subroutines */

double		percentll(ULONG,ULONG) ;

extern int	mkpath2(char *,const char *,const char *) ;
extern int	mkfnamesuf(char *,const char *,const char *) ;
extern int	densitystati(uint *,int,double *,double *) ;

extern int	setregval(struct regs_t *,int,ULONG) ;
extern int	getregval(struct regs_t *,int,ULONG *) ;
extern int	icache_latency(struct proginfo *,SS *,
			CHECKER *,ULONG) ;

extern int	sim_checkavail(PROGINFO *) ;
extern int	sim_checkexec(PROGINFO *,int) ;
extern int	sim_checkget(PROGINFO *,SSAS **) ;
extern int	sim_checkrelease(PROGINFO *,int) ;


/* forward references */

static int	ssiw_setnext(SSIW *,struct proginfo *,struct ssinfo *) ;
static int	ssiw_broadcast(SSIW *,struct proginfo *,struct ssinfo *) ;
static int	ssiw_checkasload(SSIW *,struct proginfo *,
			SS *,struct ssinfo *) ;
static int	ssiw_checkretire(SSIW *,struct proginfo *,
			SS *,struct ssinfo *) ;
static int	ssiw_checkretireases(SSIW *,struct proginfo *,
			struct ssinfo *) ;
static int	ssiw_retireases(SSIW *,struct proginfo *,
			struct ssinfo *,int) ;
static int	ssiw_handleshift() ;
static int	ssiw_flushwindow(SSIW *,struct proginfo *,
			struct ssinfo *,int) ;
static int	ssiw_asload(SSIW *,struct proginfo *,
			SS *,struct ssinfo *,SSAS *,int,int) ;
static int	ssiw_commit(SSIW *,SSAS *,int) ;
static int	ssiw_audit(SSIW *,struct proginfo *,struct ssinfo *) ;
static int	ssiw_dumpall(SSIW *,const char *) ;
static int	ssiw_dumpases(SSIW *,bfile *) ;

#if	CF_FETCH
static int	ssiw_checkfetch(SSIW *,struct proginfo *,
			SS *,struct ssinfo *) ;
static int	ssiw_checkdispatch(SSIW *,struct proginfo *,
			SS *,struct ssinfo *) ;
#endif


/* local variables */







int ssiw_init(wp,pip,mip,lip,ap)
SSIW		*wp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
SSIW_INITARGS	*ap ;
{
	uint	uv ;

	int	rs, i, j ;
	int	size ;
	int	sl, n, asid ;

	char	*cp ;


	if (wp == NULL)
	    return SR_FAULT ;

	(void) memset(wp,0,sizeof(SSIW)) ;

	wp->magic = 0 ;

	wp->pip = pip ;
	wp->mip = mip ;
	wp->lip = lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_init: entered\n") ;
#endif


/* now do our specific stuff */

#ifdef	COMMENT
	if (ap != NULL) {

	}
#endif /* COMMENT */

/* just some miscellaneous stuff */

	wp->c.f.targetout = wp->n.f.targetout = FALSE ;


/* set some defaults */

	wp->rftype = SSIW_RFTYPE ;


/* allocate everything that we need to */


/* allocate the ASes */

	size = lip->iwsize * sizeof(SSAS) ;
	rs = uc_malloc(size,&wp->ssas) ;

	if (rs < 0)
	    goto bad10 ;

/* initialize the fetch stage */

#if	CF_FETCH
	{
	    SSFETCH_INITARGS	ia ;


	    memset(&ia,0,sizeof(SSFETCH_INITARGS)) ;

	    ia.nstages = lip->fqsize ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
		eprintf("ssiw: fqsize=%d\n",ia.nstages) ;
#endif

	    rs = ssfetch_init(&wp->fetch,pip,mip,lip,&ia) ;

	} /* end block */

	if (rs < 0)
	    goto bad11 ;
#endif /* CF_FETCH */

/* initialze the PE resource accouting module */

	{
	    SSPE_INITARGS	ia ;


	    memset(&ia,0,sizeof(SSPE_INITARGS)) ;

	    ia.wp = wp ;
	    ia.ssas = wp->ssas ;

	    rs = sspe_init(&wp->pe,pip,mip,lip,&ia) ;

	} /* end block */

	if (rs < 0)
	    goto bad11a ;


/* initialize the register file */

	rs = ssreg_init(&wp->reg,pip,mip,lip,NULL) ;

	if (rs < 0)
	    goto bad12 ;


/* initialize the LSQ */

#if	CF_SSLSQ

	rs = sslsq_init(&wp->lsq,pip,mip,lip,NULL) ;

	if (rs < 0)
	    goto bad12a ;

#endif /* CF_SSLSQ */


/**** initialize the ASes, with :
				
		- object pointer
		- 'proginfo'
		- 'mintinfo'
		- 'ssinfo'
		- unique identification of this AS
	
	****/

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_init: initializing the ASes\n") ;
#endif

/* let's try and pop them one column at a time! */

	{
	    SSAS_INITARGS	ssas_a ;


/* initialize the argument for the ASes */

	    (void) memset(&ssas_a,0,sizeof(SSAS_INITARGS)) ;


	    asid = 0 ;
	    for (i = 0 ; i < lip->iwsize ; i += 1) {

	        ssas_a.wp = wp ;
	        ssas_a.pep = &wp->pe ;
	        ssas_a.rftype = wp->rftype ;
	        ssas_a.asid = i ;

	        rs = ssas_init(wp->ssas + i, pip,mip,lip,
	            &ssas_a) ;

	        if (rs < 0)
	            break ;

	    } /* end for */

	} /* end block */

	if (rs < 0) {

	    for (j = 0 ; j < i ; j += 1)
	        ssas_free(wp->ssas + j) ;

	    goto bad13 ;

	} /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	for (i = 0 ; i < lip->iwsize ; i += 1) {

	    rs = ssas_audit(wp->ssas + i) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(2) && (rs < 0))
	        eprintf("ssiw_init: ssas_audit() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

	}

	if (rs < 0)
	    goto bad14 ;

#endif /* CF_SAFE3 */


/* clean all statistics */

	(void) memset(&wp->s,0,sizeof(struct ssiw_statistics)) ;


/* we're out of there */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_init: ret OK\n") ;
#endif

	wp->magic = SSIW_MAGIC ;
	return rs ;

/* bad things have happened */
bad24:

bad23:

bad22:

bad21:

bad20:

bad9c:

bad9b:

bad9a:

bad15:

/* free up the ASes */
bad14:
	for (j = 0 ; j < lip->iwsize ; j += 1)
	    ssas_free(wp->ssas + j) ;

bad13:

#if	CF_SSLSQ
	sslsq_free(&wp->lsq) ;
#endif

bad12a:
	ssreg_free(&wp->reg) ;

bad12:
	sspe_free(&wp->pe) ;

bad11a:

#if	CF_FETCH
	ssfetch_free(&wp->fetch) ;
#endif

bad11:
	if (wp->ssas != NULL)
	    free(wp->ssas) ;

bad10:

bad9d:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 9d\n") ;
#endif

bad8b:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 8b\n") ;
#endif

bad8a:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 8a\n") ;
#endif

bad8:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 8\n") ;
#endif

bad7a:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 7a\n") ;
#endif

bad7:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 7\n") ;
#endif

bad6:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 6\n") ;
#endif

bad5:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 5\n") ;
#endif

bad4:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 4\n") ;
#endif

bad3:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 3\n") ;
#endif

bad2:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 2\n") ;
#endif

bad1:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: bad 1\n") ;
#endif

bad0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_init: ret BAD rs=%d\n",rs) ;
#endif

	wp->magic = SSIW_MAGIC ;
	return rs ;
}
/* end subroutine (ssiw_init) */


/* free up this object */
int ssiw_free(wp)
SSIW		*wp ;
{
	struct proginfo		*pip ;

	struct ssinfo		*lip ;

	int	rs, i ;
	int	n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_free: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != SSIW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("ssiw_free: entered\n") ;
#endif



#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("ssiw_free: freeing ASes\n") ;
#endif

	for (i = 0 ; i < lip->iwsize ; i += 1)
	    ssas_free(wp->ssas + i) ;


#if	CF_SSLSQ
	sslsq_free(&wp->lsq) ;
#endif

	ssreg_free(&wp->reg) ;

	sspe_free(&wp->pe) ;

#if	CF_FETCH
	ssfetch_free(&wp->fetch) ;
#endif

/* de-allocate everything that was allocated */

	if (wp->ssas != NULL)
	    free(wp->ssas) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("ssiw_free: ret\n") ;
#endif

	wp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (ssiw_free) */


/* combinatorial logic */
int ssiw_comb(wp,phase)
SSIW		*wp ;
int		phase ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, rs1, i, j ;
	int	ncommit = 0 ;
	int	n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_comb: SSIW(%p) bad format\n",wp) ;

	    return SR_BADFMT ;
	}

	if (wp->magic != SSIW_MAGIC) {

	    eprintf("ssiw_comb: SSIW(%p) not open\n",wp) ;

	    return SR_NOTOPEN ;
	}
#endif /* CF_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_comb: entered ck=%llu:%u f_exit=%u\n",
	        mip->ck,phase,wp->c.f.exit) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssiw_comb: n.f.exit=%u\n", wp->n.f.exit) ;
	}
#endif

#if	CF_FETCH
	if (rs >= 0)
	    rs = ssfetch_comb(&wp->fetch,phase) ;

#endif

/* pop the FU resource allocator */

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	if (DEBUGLEVEL(5) &&
	    (phase > 0) && (phase < 5)) {
		if (rs >= 0) {
	    eprintf("ssiw_comb: audit SSPE\n") ;
	    rs = ssiw_audit(wp,pip,lip) ;

	    if (rs < 0)
	        eprintf("ssiw_comb: audit before SSPE\n") ;
		}
	}
#endif

	if (rs >= 0)
	    rs = sspe_comb(&wp->pe,phase) ;

/* pop the register file */

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	if (DEBUGLEVEL(5) &&
	    (phase > 0) && (phase < 5)) {
		if (rs >= 0) {
	    eprintf("ssiw_comb: audit SSREG\n") ;
	    rs = ssiw_audit(wp,pip,lip) ;

	    if (rs < 0)
	        eprintf("ssiw_comb: audit before SSREG\n") ;
		}
	}
#endif

	if (rs >= 0)
	    rs = ssreg_comb(&wp->reg,phase) ;

/* the LSQ */

#if	CF_SSLSQ

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	if (DEBUGLEVEL(5) &&
	    (phase > 0) && (phase < 5)) {
		if (rs >= 0) {
	    eprintf("ssiw_comb: audit SSLSQ\n") ;
	    rs = ssiw_audit(wp,pip,lip) ;

	    if (rs < 0)
	        eprintf("ssiw_comb: audit before SSLSQ\n") ;
		}
	}
#endif

	if (rs >= 0)
	    rs = sslsq_comb(&wp->lsq,phase) ;

#endif /* CF_SSLSQ */


/* pop the ASes (but do them in the TT order oldest to newest) */

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	if (DEBUGLEVEL(5) &&
	    (phase > 0) && (phase < 5)) {
		if (rs >= 0) {
	    eprintf("ssiw_comb: audit SSAS\n") ;
	    rs = ssiw_audit(wp,pip,lip) ;

	    if (rs < 0)
	        eprintf("ssiw_comb: audit before SSAS\n") ;
		}
	}
#endif

	if (rs >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_comb: doing the ASes\n") ;
#endif

	n = lip->iwsize ;
	j = wp->c.asri ;
	for (i = 0 ; (rs >= 0) && (i < n) ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        eprintf("ssiw_comb: doing AS(%d)=%08lx\n",
	            i,(wp->ssas + i)) ;
#endif

	    rs = ssas_comb((wp->ssas + j),phase) ;

#if	CF_MASTERDEBUG && CF_SAFE2
	    if ((rs == SR_FAULT) || (rs == SR_BADFMT) || (rs == SR_NOTOPEN)) {

	        eprintf("ssiw_comb: ssas_comb() SSAS bad format rs=%d\n",rs) ;

	        return rs ;
	    }
#endif /* CF_SAFE2 */

#if	CF_MASTERDEBUG && CF_DEBUG && 0
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_comb: ssas_comb() rs=%d\n", rs) ;
#endif

	    if (rs < 0)
	        break ;

	    j = (j + 1) % lip->iwsize ;

	} /* end for (looping ofer ASes) */

	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_comb: doing the ASes rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto ret0 ;

/* do our own combinatorial logic */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_comb: combinatorial ck=%llu:%u\n",
	        mip->ck,phase) ;
#endif

	switch (phase) {

	case 0:
	    (void) ssiw_setnext(wp,pip,lip) ;

	    if (wp->n.c_wait > 0)
	        wp->n.c_wait -= 1 ;

	    break ;

/* the order of the following subroutines is important */
	case 1:

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	if (rs >= 0) {
	    rs = ssiw_audit(wp,pip,lip) ;

	    if (rs < 0)
	        eprintf("ssiw_comb: audit before ssiw_checkreture()\n") ;
	}
#endif

		if (wp->c.checkturn > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssiw_comb: sim_checkrelease() checkturn=%u\n",
	                wp->c.checkturn) ;
#endif

			rs = sim_checkrelease(pip,wp->c.checkturn) ;

			wp->n.checkturn = 0 ;
		}

	    if (rs >= 0) {

	        rs = ssiw_checkretire(wp,pip,mip,lip) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssiw_comb: ck=%llu:%u ssiw_checkretire() rs=%d\n",
	                mip->ck,phase,rs) ;
#endif

	    }

	    ncommit = rs ;
	    if (rs < 0)
	        break ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_comb: ninstr=%llu in=%llu ncommit=%u\n",
	            pip->ninstr,mip->in,ncommit) ;
#endif

	    wp->n.f.committed = (ncommit > 0) ;

	    mip->in += ncommit ;
	    if ((pip->minstr > 0) && (mip->in >= pip->minstr))
	        wp->n.f.exit = TRUE ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(2) && wp->n.f.exit) {
	        eprintf("ssiw_comb: program exit signalled\n") ;
	        eprintf("ssiw_comb: minstr=%llu in=%llu\n",
	            pip->minstr,mip->in) ;
	    }
#endif

	    if (! wp->n.f.committed) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(2))
	                eprintf("ssiw_comb: hangcks c_nocommit=%u\n",
	                    wp->n.c_nocommit) ;
#endif

	        wp->n.c_nocommit += 1 ;
	        if (wp->n.c_nocommit >= lip->hangcks) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(2))
	                eprintf("ssiw_comb: hangup c_nocommit=%u\n",
	                    wp->n.c_nocommit) ;
#endif

#ifdef	NDEBUGFNAME
	nprintf(NDEBUGFNAME,"ssiw: ssiw_dumpall()\n") ;
#endif

		ssiw_dumpall(wp,"hang.dump") ;

	            rs = SR_TIMEDOUT ;
	        }

	    } else
	        wp->n.c_nocommit = 0 ;

/* broadcast all operands to ASes (they will snoop) */

	    if (rs >= 0)
	        rs = ssiw_broadcast(wp,pip,lip) ;

	    break ;

	case 2:
	case 3:

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	    rs = ssiw_audit(wp,pip,lip) ;

	    if (rs < 0)
	        eprintf("ssiw_comb: ph3 audit\n") ;
#endif

	    break ;

	case 4:

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	    rs = ssiw_audit(wp,pip,lip) ;

	    if (rs < 0)
	        eprintf("ssiw_comb: ph3 audit\n") ;

	    if (rs < 0)
	        break ;
#endif

#if	CF_FETCH
	if (rs >= 0)
	    rs = ssiw_checkfetch(wp,pip,mip,lip) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_comb: ssiw_checkfetch() rs=%d\n",rs) ;
#endif

	if (rs >= 0)
	    rs = ssiw_checkdispatch(wp,pip,mip,lip) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_comb: ssiw_checkdispatch() rs=%d\n",rs) ;
#endif

#else /* CF_FETCH */

	if (rs >= 0)
	    rs = ssiw_checkasload(wp,pip,mip,lip) ;

#endif /* CF_FETCH */

	    break ;

	case 5:
	    rs = wp->c.f.exit ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if ((pip->debuglevel > 2) && wp->c.f.exit)
	        eprintf("ssiw_comb: simulator exit signalled rs=%d\n",rs) ;
#endif

	    break ;

	} /* end switch */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_comb: about to ret rs=%d\n",rs) ;
#endif

/* good things fall through here */
ret0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssiw_comb: ret n.f.exit=%u\n", wp->n.f.exit) ;
	}
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2)) {
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_comb: ret rs=%d ck=%llu:%u f_exit=%u\n",
	            rs,mip->ck,phase,wp->c.f.exit) ;
	    if (rs < 0)
	        eprintf("ssiw_comb: ret real bad rs=%d\n",rs) ;
	}
#endif /* CF_DEBUG */

	return (rs >= 0) ? wp->c.f.exit : rs ;
}
/* end subroutine (ssiw_comb) */


/* handle clocking */
int ssiw_clock(wp)
SSIW		*wp ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	struct ssiw_path	*pp ;

	int	rs, i, j ;
	int	n, size ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_clock: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != SSIW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_clock: entered ck=%llu\n",
	        mip->ck) ;
#endif


/* pop the fetch stage */

#if	CF_FETCH
	rs = ssfetch_clock(&wp->fetch) ;

	if (rs < 0)
	    return rs ;
#endif

/* pop the FU resource allocator */

	rs = sspe_clock(&wp->pe) ;

	if (rs < 0)
	    return rs ;


/* pop the register file */

	rs = ssreg_clock(&wp->reg) ;

	if (rs < 0)
	    return rs ;


/* pop the LSQ */

#if	CF_SSLSQ

	rs = sslsq_clock(&wp->lsq) ;

	if (rs < 0)
	    return rs ;

#endif


/* pop the ASes */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_clock: ssas_clock()\n") ;
#endif

	n = lip->iwsize ;
	for (i = 0 ; i < n ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        eprintf("ssiw_clock: ssas_clock() asid=%d\n",i) ;
#endif

	    rs = ssas_clock(wp->ssas + i) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3) && (rs < 0))
	        eprintf("ssiw_clock: SSAS bad rs=%d\n",rs) ;
#endif /* CF_DEBUG */

	    if (rs < 0)
	        break ;

	} /* end for (looping through ASes) */

	if (rs < 0)
	    goto bad ;


/* transition ourselves */


	{


/* set the non-pointer state */

	    wp->c = wp->n ;


	} /* end block (clocking ourselves) */


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_clock: ret \n") ;
#endif

	return SR_OK ;

bad:
	return rs ;
}
/* end subroutine (ssiw_clock) */


/* perform a machine shift at this level */
int ssiw_shift(wp,amount)
SSIW		*wp ;
int		amount ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i ;
	int	n ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_shift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != SSIW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_shift: amount=%d\n",amount) ;
#endif

	wp->shiftamount = amount ;

/* pop the ASes */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_shift: do ASes\n") ;
#endif

	n = lip->iwsize ;
	for (i = 0 ; i < n ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_shift: ssas_shift() asid=%d\n",i) ;
#endif

	    rs = ssas_shift((wp->ssas + i),amount) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (rs < 0)
	        eprintf("ssiw_shift: SSAS bad rs=%d\n",rs) ;
#endif /* CF_DEBUG */

	    if (rs < 0)
	        break ;

	} /* end for */

	if (rs < 0)
	    goto bad ;


/* our we supposed to do anything ourselves? */


/* exit */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_shift: ret rs=%d\n",rs) ;
#endif

bad:
	return rs ;
}
/* end subroutine (ssiw_shift) */


/* someone is asking us if we think that the machine should be shifted */
int ssiw_needshift(wp)
SSIW		*wp ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_needshift: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != SSIW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_needshift: entered\n") ;
#endif


	return ((wp->f.shift) ? 1 : SR_OK) ;
}
/* end subroutine (ssiw_needshift) */


/* set a starting register value */
int ssiw_setreg(wp,n,v)
SSIW		*wp ;
int		n ;
ULONG		v ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_setreg: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != SSIW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(6))
	    eprintf("ssiw_setreg: entered\n") ;
#endif


	rs = ssreg_setval(&wp->reg,n,v) ;


	return rs ;
}
/* end subroutine (ssiw_setreg) */


/* someone (an AS given by ASID) is requesting an operand */
int ssiw_oprequest(wp,rop,asid)
SSIW		*wp ;
OPERAND		*rop ;
int		asid ;
{
	struct proginfo	*pip ;

	struct ssinfo	*lip ;

	SSAS	*lasp ;

	int	rs = SR_OK, rs1, i, j, n ;
	int	f ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_oprequest: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (wp->magic != SSIW_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = wp->pip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_oprequest: entered asid=%u\n",asid) ;
#endif

	rs1 = SR_OK ;
	n = (asid - wp->c.asri + lip->iwsize) % lip->iwsize ;
	j = (asid - 1 + lip->iwsize) % lip->iwsize ;
	for (i = 0 ; i < n ; i += 1) {

	    lasp = wp->ssas + j ;
	    rs1 = ssas_opmatch(lasp,rop) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_oprequest: asid=%d ssas_opmatch() rs=%d\n",
	            j,rs1) ;
#endif

	    if ((rs1 < 0) && (rs1 != SR_EMPTY))
	        break ;

#if	CF_NOTALLREQ
	    if (rs1 > 0)
	        break ;
#endif

	    j = (j - 1 + lip->iwsize) % lip->iwsize ;

	} /* end for */

	if (rs1 != SR_EMPTY)
	    rs = rs1 ;

/* we-re requesting, so ask everyone (appropriate)! */

#if	CF_NOTALLREQ
	f = (rs < 1) ;
#else
	f = TRUE ;
#endif

/* as REG (if appropriate) */

	if (f && (rop->type == OPERAND_TREG)) {

	    n = rop->a ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_oprequest: asking REG a=%u\n",n) ;
#endif

	    rs = ssreg_oprequest(&wp->reg,n) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_oprequest: ssreg_oprequest() rs=%d\n",rs) ;
#endif

	} /* end if (asking register file) */

/* ask LSQ (if appropriate) */

#if	CF_SSLSQ

	if (f && (rop->type == OPERAND_TMEM)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_oprequest: asking LSQ a=%llx\n",rop->a) ;
#endif

	    rs = sslsq_oprequest(&wp->lsq,rop->a,rop->size) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_oprequest: sslsq_oprequest() rs=%d\n",rs) ;
#endif

	} /* end if (asking LSQ) */

#endif /* CF_SSLSQ */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_oprequest: ret rs=%d\n",rs) ;
#endif

	return (rs == SR_EMPTY) ? SR_OK : rs ;
}
/* end subroutine (ssiw_oprequest) */


#if	CF_STATFILE

/* write out any statistics that we may have */
int ssiw_statfile(wp,fp)
SSIW		*wp ;
bfile		*fp ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	ULONG	total ;

	int	rs = SR_OK, i ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if ((wp->magic != SSIW_MAGIC) && (wp->magic != 0)) {

	    eprintf("ssiw_statfile: bad format\n") ;

	    rs = SR_BADFMT ;
	    return rs ;
	}

	if (wp->magic != SSIW_MAGIC) {

	    rs = SR_NOTOPEN ;
	    return rs ;
	}
#endif /* CF_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;


	bprintf(fp,"\n\ninstruction execution window:\n") ;

#ifdef	COMMENT
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

#endif /* COMMENT */


	bprintf(fp,"\nissue:\n") ;

#ifdef	COMMENT
	bprintf(fp,
	    "columns                            %16lld\n",
	    wp->s.issue_col) ;
#endif /* COMMENT */

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

#ifdef	COMMENT
	bprintf(fp,
	    "column retirements                 %16lld\n",
	    wp->s.commit_col) ;
#endif /* COMMENT */

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

	    dclocks = (double) mip->ck ;
	    dinstr = wp->s.commit_ienabled ;
	    cpi = dclocks / dinstr ;

	    bprintf(fp,
	        "clocks                             %16lld\n",
	        mip->ck) ;

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
/* end subroutine (ssiw_statfile) */

#endif /* CF_STATFILE */


int ssiw_printstats(wp,fname)
SSIW		*wp ;
const char	fname[] ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	SSPE_INFO	pestats ;

	bfile		sfile, tfile ;

	ULONG	pe_req, pe_grants, pe_waits, pe_issues ;

	int	rs = SR_OK, rs1, i, j ;
	int	size ;

	char	tmpfname[MAXPATHLEN + 1] ;


	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_printstats: jobname=%s\n",pip->jobname) ;
#endif

	if (pip->jobname[0] == '\0')
	    return 0 ;

/* get PE stats */

	sspe_info(&wp->pe,&pestats) ;

	mkfnamesuf(tmpfname,pip->jobname,FNE_PESTATS) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_printstats: tmpfname=%s\n",tmpfname) ;
#endif

	rs = bopen(&sfile,tmpfname,"wct",0666) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("ssiw_printstats: bopen() rs=%d\n",rs) ;
#endif

	if (rs >= 0) {

	    ULONG	creqs ;

	    double	n1, n2, d ;


	    for (i = 0 ; i < iclass_overlast ; i += 1) {

	        creqs = pestats.cgrants[i] + pestats.cwaits[i] ;
	        n1 = (double) pestats.cgrants[i] ;
	        n2 = (double) pestats.cwaits[i] ;
	        d = (double) ((creqs != 0) ? creqs : 1) ;

#ifdef	COMMENT
	        bprintf(&sfile,"%-8s %10llu %10llu\n",
	            ssfunames[i],
	            pestats.cgrants[i],
	            pestats.cwaits[i]) ;
#else
	        bprintf(&sfile,"%-8s %10llu %10llu (%6.1f%% %6.1f%%)\n",
	            ssfunames[i],
	            pestats.cgrants[i],
	            pestats.cwaits[i],
	            (n1/d),(n2/d)) ;
#endif /* COMMENT */

	    }

	    bclose(&sfile) ;

	} /* end if (PE raw numbers) */

/* the regular stats */

	rs = SR_NOENT ;
	if (pip->statsfname[0] != '\0')
	    rs = bopen(&sfile,pip->statsfname,"wa",0666) ;

	if (rs >= 0) {

	    SSAS_STATS	s ;

	    SSAS		*lasp ;

	    ULONG	ulw ;
	    ULONG	vcount ;
	    ULONG	cks, ck_end ;
	    ULONG	ins, in_end, in_extra ;
	    ULONG	creqs ;
	    ULONG	ntexec, nrexec, nlexec ;
	    ULONG	nnorexec, nmemexec ;
	    ULONG	nfuiss, nfuret, nfuacc, naexec ;
	    ULONG	nnexec ;

	    double	fn, fd, fp, n1, n2, d ;

	    uint	isres[SSCOMMON_MAXISRESIDENCY + 1] ;


	    size = SSCOMMON_MAXISRESIDENCY * sizeof(uint) ;
	    memset(isres,0,size) ;


	    bprintf(&sfile,"= function unit stastics\n") ;

	    bprintf(&sfile,
	        "NAME     "
	        "     GRANTS                 WAITS        "
	        "         ISSUES\n") ;

	    pe_grants = 0 ;
	    pe_waits = 0 ;
	    pe_issues = 0 ;
	    for (i = 0 ; i < iclass_overlast ; i += 1) {

	        creqs = pestats.cgrants[i] + pestats.cwaits[i] ;
	        n1 = (double) pestats.cgrants[i] ;
	        n1 *= 100.0 ;
	        n2 = (double) pestats.cwaits[i] ;
	        n2 *= 100.0 ;
	        d = (double) ((creqs != 0) ? creqs : 1) ;

	        bprintf(&sfile,
	            "%-8s %11llu (%6.1f%%) %11llu (%6.1f%%)"
	            " %11llu\n",
	            ssfunames[i],
	            pestats.cgrants[i],
	            (n1/d),
	            pestats.cwaits[i],
	            (n2/d),
	            pestats.cissues[i]) ;

	        pe_grants += pestats.cgrants[i] ;
	        pe_waits += pestats.cwaits[i] ;
	        pe_issues += pestats.cissues[i] ;

	    } /* end for (instruction classes) */

	    pe_req = pe_grants + pe_waits ;
	    bprintf(&sfile,"requests=  %16llu\n",pe_req) ;

	    d = (double) ((pe_req != 0) ? pe_req : 1) ;
	    n1 = (double) pe_grants ;
	    n1 *= 100.0 ;
	    bprintf(&sfile,"grants=    %16llu (%6.1f%%)\n",
	        pe_grants,(n1/d)) ;

	    n1 = (double) pe_waits ;
	    n1 *= 100.0 ;
	    bprintf(&sfile,"waits=     %16llu (%6.1f%%)\n",
	        pe_waits,(n1/d)) ;

	    bprintf(&sfile,"issues=    %16llu\n",
	        pe_issues) ;

	    bprintf(&sfile,"invalids=  %16llu\n",
	        pestats.cinvalids) ;

	    bprintf(&sfile,"\n") ;

/* AS stats */

	    bprintf(&sfile,"= issue station stastics\n") ;

	    ss_getinstr(mip,&in_end) ;

	    ins = in_end - mip->in_start ;

	    ntexec = 0 ;
	    nrexec = 0 ;
	    nfuiss = 0 ;
	    nfuret = 0 ;
	    nfuacc = 0 ;
	    vcount = 0 ;
	    nnorexec = 0 ;
	    nmemexec = 0 ;
	    for (i = 0 ; i < lip->iwsize ; i += 1) {

	        lasp = wp->ssas + i ;
	        ssas_stats(lasp,&s) ;

	        ntexec += s.ntexec ;
	        nrexec += s.nrexec ;
	        nfuiss += s.nfuiss ;
	        nfuret += s.nfuret ;
	        nfuacc += s.nfuacc ;
	        vcount += s.vcount ;
	        nnorexec += s.nnorexec ;
	        nmemexec += s.nmemexec ;

	        for (j = 0 ; j < SSCOMMON_MAXISRESIDENCY ; j += 1)
	            isres[j] += s.isres[j] ;

	    } /* end for (ISes) */

	    nnexec = ntexec - nrexec ;
	    ss_getclock(mip,&ck_end) ;

	    bprintf(&sfile,"ins=      %16llu\n",ins) ;

	    cks = ck_end - pip->s.ck_start ;
	    bprintf(&sfile,"cks=      %16llu\n",cks) ;

	    bprintf(&sfile,"iwsize=   %16u\n",lip->iwsize) ;

	    ulw = lip->iwsize * cks ;
	    bprintf(&sfile,"slots=    %16llu (S)\n",ulw) ;

	    bprintf(&sfile,"vcount=   %16llu (%6.1f%% V/S)\n",
	        vcount,percentll(vcount,ulw)) ;

	    bprintf(&sfile,"ntexec=   %16llu (%6.1f%% T/N)\n",
	        ntexec,percentll(ntexec,ins)) ;

	    in_extra = ntexec - ins ;
	    bprintf(&sfile,"neexec=   %16llu (%6.1f%% E/N)\n",
	        in_extra,percentll(in_extra,ins)) ;

	    bprintf(&sfile,"nrexec=   %16llu (%6.1f%% R/T)\n",
	        nrexec,percentll(nrexec,ntexec)) ;

	    nlexec = ntexec - nfuiss ;
	    bprintf(&sfile,"nlexec=   %16llu (%6.1f%% L/T) \n",
	        nlexec,percentll(nlexec,ntexec)) ;

	    bprintf(&sfile,"nnorexec= %16llu (%6.1f%% NOR/T))\n",
	        nnorexec,percentll(nnorexec,ntexec)) ;

	    bprintf(&sfile,"nmemexec= %16llu (%6.1f%% MEM/T)\n",
	        nmemexec,percentll(nmemexec,ntexec)) ;

	    bprintf(&sfile,"nfuiss=   %16llu (%6.1f%% FUISS/T) \n",
	        nfuiss,percentll(nfuiss,ntexec)) ;

	    bprintf(&sfile,"nfuret=   %16llu\n",
	        nfuret) ;

	    bprintf(&sfile,
	        "nfuacc=   %16llu "
	        "(%6.1f%% FUACC/FUISS, %6.1f%% FUACC/T)\n",
	        nfuacc,
	        percentll(nfuacc,nfuiss),
	        percentll(nfuacc,ntexec)) ;

	    naexec = nfuiss - nfuacc ;
	    bprintf(&sfile,
	        "naexec=   %16llu (%6.1f%% A/FUISS, %6.1f%% A/T)\n",
	        naexec,
	        percentll(naexec,nfuiss),
	        percentll(naexec,ntexec)) ;

	    {
	        double	mean, var ;

	        uint	max ;


	        max = 0 ;
	        for (j = 0 ; j < SSCOMMON_MAXISRESIDENCY ; j += 1) {

	            if (isres[j] != 0)
	                max = j ;

	        } /* end for */

	        rs1 = densitystati(isres,SSCOMMON_MAXISRESIDENCY,
	            &mean,&var) ;

	        if (rs1 >= 0) {

	            bprintf(&sfile, "isres_mean=    %7.2f cks\n",mean) ;

	            bprintf(&sfile, "isres_stddev=  %7.2f\n",sqrt(var)) ;

	            bprintf(&sfile, "isres_max=     %4u cks\n",max) ;

	        }

	    } /* end block */

	    bprintf(&sfile,"\n") ;

/* close it up */

	    bclose(&sfile) ;

/* write out the IS residency probability density */

	    mkfnamesuf(tmpfname,pip->jobname,FNE_ISRES) ;

	    rs = bopen(&tfile,tmpfname,"wct",0666) ;

	    if (rs >= 0) {

	        for (j = 0 ; j < SSCOMMON_MAXISRESIDENCY ; j += 1) {

	            bprintf(&tfile,"%8u %8u\n",
	                j,isres[j]) ;

	        } /* end for */

	        bclose(&tfile) ;

	    } /* end if (IS residency probability density) */

	} /* end if (opened statistics file) */


	return rs ;
}
/* end subroutine (ssiw_printstats) */


/* dump the SSIW state */
int ssiw_dump(wp,fp)
SSIW		*wp ;
bfile		*fp ;
{
	SS		*mip ;

	struct ssinfo	*lip ;

	int	rs, i ;


	mip = wp->mip ;
	lip = wp->lip ;

	bprintf(fp,"ta=%016llx_%016llx\n",
	    wp->c.ta,wp->n.ta) ;

	bprintf(fp,"commit_count=%u_%u\n",
	    wp->c.c_commit,wp->n.c_commit) ;

	bprintf(fp,"nasactive=%u_%u\n",
	    wp->c.nasactive,wp->n.nasactive) ;

	bprintf(fp,"asri=%u_%u\n",
	    wp->c.asri,wp->n.asri) ;

	bprintf(fp,"aswi=%u_%u\n",
	    wp->c.aswi,wp->n.aswi) ;

	bprintf(fp,"f_targetout=%u_%u\n",
	    wp->c.f.targetout,wp->n.f.targetout) ;

	bprintf(fp,"f_committed=%u_%u\n",
	    wp->c.f.committed,wp->n.f.committed) ;

	bprintf(fp,"f_exit=%u_%u\n",
	    wp->c.f.exit,wp->n.f.exit) ;

	rs = bprintf(fp,"ASes\n") ;

	if (rs < 0)
		goto ret0 ;

	for (i = 0 ; i < lip->iwsize ; i += 1) {

	    ssas_dump((wp->ssas + i),fp) ;

	}

ret0:
	return rs ;
}
/* end subroutine (ssiw_dump) */


#if	CF_XML

/* XML output stuff */
int ssiw_xmlinit(wp,xip)
SSIW	*wp ;
XMLINFO	*xip ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if (wp->magic != SSIW_MAGIC) {

	    rs = (wp->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
	    return rs ;
	}
#endif /* CF_SAFE */



	return SR_OK ;
}
/* end subroutine (ssiw_xmlinit) */


int ssiw_xmlout(wp,xip)
SSIW	*wp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	SS		*mip ;

	struct ssinfo	*lip ;

	int	rs = SR_OK, i, j ;
	int	rs1 ;
	int	sgrow, sgcol, sgid, n ;
	int	asrow, ascol, asid ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp->magic != SSIW_MAGIC) {

	    rs = (wp->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
	    return rs ;
	}
#endif /* CF_SAFE */

	pip = wp->pip ;
	mip = wp->mip ;
	lip = wp->lip ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_xmlout: entered\n") ;
#endif

	bprintf(&xip->xmlfile,"<lt>\n") ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_xmlout: LIFETCH\n") ;
#endif

	rs = lifetch_xmlout(&wp->ourfetch,xip) ;

	if (rs < 0)
	    return rs ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_xmlout: LLB\n") ;
#endif

	rs = llb_xmlout(&wp->ourload,xip) ;

	if (rs < 0)
	    return rs ;

/* loop by sharing groups ! */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_xmlout: sharing groups\n") ;
#endif

	for (sgcol = 0 ; sgcol < lip->nsgcols ; sgcol += 1) {

	    for (sgrow = 0 ; sgrow < lip->nsgrows ; sgrow += 1) {

	        bprintf(&xip->xmlfile,"<sg>\n") ;

	        sgid = (sgcol * lip->nsgrows) + sgrow ;

	        bprintf(&xip->xmlfile,"<sgid>%d</sgid>\n",sgid) ;

/* do the ASes in columns */

	        for (j = 0 ; j < SSINFO_NASWPSG ; j += 1) {

	            bprintf(&xip->xmlfile,"<ascol>\n") ;

	            bprintf(&xip->xmlfile,"<id>%d</id>\n",j) ;

	            for (i = 0 ; i < lip->nasrpsg ; i += 1) {

	                bprintf(&xip->xmlfile,"<aselem>\n") ;

	                bprintf(&xip->xmlfile,"<id>%d</id>\n",i) ;

	                asrow = (sgrow * lip->nasrpsg) + i ;
	                bprintf(&xip->xmlfile,"<asrow>%d</asrow>\n",asrow) ;

	                ascol = (sgcol * SSINFO_NASWPSG) + j ;
	                asid = (ascol * lip->totalrows) + asrow ;
	                bprintf(&xip->xmlfile,"<asid>%d</asid>\n",asid) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(5))
	                    eprintf("ssiw_xmlout: AS asid=%d\n",asid) ;
#endif

	                rs = ssas_xmlout(wp->ssas + asid,xip) ;

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


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_xmlout: the other stuff\n") ;
#endif

/* pop the LWQ */

	rs = lwq_xmlout(&wp->wqueue,xip) ;

	if (rs < 0)
	    return rs ;


	bprintf(&xip->xmlfile,"</lt>\n") ;


/* good things fall through here */
bad:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_xmlout: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssiw_xmlout) */


int ssiw_xmlfree(wp,xip)
SSIW	*wp ;
XMLINFO	*xip ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_SAFE
	if (wp == NULL)
	    return SR_FAULT ;

	if (wp->magic != SSIW_MAGIC) {


	    rs = (wp->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
	    return rs ;
	}

#endif /* CF_SAFE */



	return SR_OK ;
}
/* end subroutine (ssiw_xmlfree) */

#endif /* CF_XML */



/* PRIVATE SUBROUTINES */



/* set the default next state */
static int ssiw_setnext(wp,pip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	int	i ;


/* by default, the next state is the same as it was in the previous clock */

	wp->f.commit = FALSE ;
	wp->f.shift = FALSE ;
	wp->f.noload = FALSE ;
	wp->f.targetout = FALSE ;


	return SR_OK ;
}
/* end subroutine (ssiw_setnext) */


/* load up (dispatch instructions to) any available ASes */

/*
If any ASes are either empty or are being committed, 
load them up if possible.
*/

static int ssiw_checkasload(wp,pip,mip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
{
	CHECKER	*csp = &pip->check ;

	SSAS	*lasp ;

	int	rs = SR_OK, rs1, i, nc, asid ;
	int	ci, tt ;
	int	nempty, nloadable ;


	nempty = lip->iwsize - wp->n.nasactive ;

	if (nempty < 0)
	    return SR_BADFMT ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssiw_checkasload: iwsize=%d nempty=%d\n",
	        lip->iwsize,nempty) ;
	    eprintf("ssiw_checkasload: nasactive=%d\n",wp->n.nasactive) ;
	}
#endif /* CF_DEBUG */

#if	CF_READYLOAD /* handled elsewhere */

/* are there any ASes free or about to become free */

	asid = wp->n.asri ;
	for (i = 0 ; i < wp->n.nasactive ; i += 1) {

	    lasp = wp->ssas + asid ;
	    rs = ssas_readyload(lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkasload: ssas_readyload() rs=%d\n",rs) ;
#endif

	    if (rs <= 0)
	        break ;

	    asid = (asid + 1) % lip->iwsize ;

	} /* end for */

	nloadable = i ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_checkasload: ASes loadable=%d\n", i) ;
#endif

#endif /* CF_READYLOAD */

	if (nempty == 0)
	    return 0 ;

/* check how many templates in the checker are available */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_checkasload: checker instr in window %d\n",
	        csp->nw) ;
#endif

	if (csp->nw < lip->iwsize) {

	    int	ncneed = (lip->iwsize - csp->nw)  ;


	    if (ncneed > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssiw_checkasload: ncneed=%d sim_checkexec() \n",
	                ncneed) ;
#endif
	        pip->s.ncneed += ncneed ;

	        rs1 = sim_checkexec(pip,ncneed) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssiw_checkasload: sim_checkexec() rs1=%d\n",rs1) ;
#endif

	        if (rs1 < 0)
	            return rs1 ;

	        pip->s.ncadv += rs1 ;

	        if ((rs1 >= 0) && (rs1 < ncneed)) {

	            nempty -= (ncneed - rs1) ;
	        }

	    } /* end if (needed to advance the checker) */

	} /* end if (need more checker slots) */

/* OK, we have some to load up */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkasload: loading ASes c_nactive=%d aswi=%d\n",
	        wp->c.nasactive,wp->n.aswi) ;
#endif

	ci = csp->ri ;
	asid = wp->n.aswi ;
	nc = (lip->dwidth > 0) ? MIN(nempty,lip->dwidth) : nempty ;

	for (i = 0 ; i < nc ; i += 1) {

	    lasp = wp->ssas + asid ;
	    tt = wp->c.nasactive + i ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4)) {
	        eprintf("ssiw_checkasload: loading AS(%p) asid=%u tt=%d\n",
	            lasp,asid,tt) ;
	        eprintf("ssiw_checkasload: from CAS(%p) ci=%u\n",
	            (csp->cas + ci),ci) ;
	    }
#endif /* CF_DEBUG */

	    rs1 = ssiw_asload(wp,pip,mip,lip,lasp,ci,tt) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_checkasload: ssiw_asload() rs1=%d i=%d\n",
	            rs1,i) ;
#endif

	    ci = (ci + 1) % csp->wsize ;
	    asid = (asid + 1) % lip->iwsize ;

	} /* end for */

/* window state update */

	wp->n.aswi = asid ;
	wp->n.nasactive += i ;

/* checker update */

#ifdef	COMMENT
	if (i > 0) {

	    rs1 = sim_checkrelease(pip,i) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(2)) {
	        eprintf("ssiw_checkasload: sim_checkrelease() rs=%d\n",rs1) ;
	        assert(rs1 >= 0) ;
	    }
#endif

	} /* end if (checker update) */
#endif /* COMMENT */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_checkasload: ret rs=%d n.nasactive=%d\n",
	        nempty,wp->n.nasactive) ;
#endif

	return (rs >= 0) ? i : rs ;
}
/* end subroutine (ssiw_checkasload) */


#if	CF_FETCH

static int ssiw_checkfetch(wp,pip,mip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
{
	CHECKER	*csp = &pip->check ;

	SSAS	*casp ;

	ULONG	ia ;

	int	rs = SR_OK, rs1, i, n ;
	int	nc ;
	int	nfavail, ncavail, nfwant ;
	int	nfetched ;
	int	lat ;


	nfetched = 0 ;
	rs = ssfetch_nfetch(&wp->fetch) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	    eprintf("ssiw_checkfetch: ssfetch_nfetch() rs=%d\n", rs) ;
	}
#endif /* CF_DEBUG */

	nfavail = rs ;
	if (rs < 0)
	    goto ret0 ;

	if (nfavail == 0)
	    goto ret0 ;

/* check how many templates in the checker are available */

	rs = sim_checkavail(pip) ;

	ncavail = rs ;
	if (rs < 0)
		goto ret0 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkfetch: sim_checkavail() rs=%d\n",ncavail) ;
#endif

	if ((ncavail >= 0) && (nfavail > ncavail)) {

	    int		ncneed = nfavail - ncavail ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkfetch: sim_checkexec() ncneed=%d\n",
	            ncneed) ;
#endif

	    pip->s.ncneed += ncneed ;
	    rs = sim_checkexec(pip,ncneed) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkfetch: sim_checkexec() rs1=%d\n",rs) ;
#endif

	    if (rs >= 0) {

	    ncavail += rs ;
	    pip->s.ncadv += rs ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkfetch: new ncavail=%u\n",ncavail) ;
#endif

		}

	} /* end if (need more checker slots) */

	if (rs >= 0)
		nfwant = MIN(nfavail,ncavail) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkfetch: nfwant=%d\n",nfwant) ;
#endif

/* OK, we have some to load up */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
		if (rs >= 0) {
	    eprintf("ssiw_checkfetch: load-FQ c_nactive=%d nfwant=%u\n",
	        wp->c.nasactive,nfwant) ;
	    eprintf("ssiw_checkfetch: waitcout=%u\n",
	        wp->c.c_wait) ;
		}
	}
#endif

	if ((rs >= 0) && (wp->c.c_wait == 0)) {

	    nc = (lip->fwidth > 0) ? MIN(nfwant,lip->fwidth) : nfwant ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkfetch: fetch-loop nc=%u \n",
	            nc) ;
#endif

	    for (i = 0 ; i < nc ; i += 1) {

		rs = sim_checkget(pip,&casp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkfetch: sim_checkget() "
		"rs=%d CASP=%p\n",rs,casp) ;
#endif

		if (rs < 0)
			break ;

/* get the cache latency for this instruction fetch */

#if	CF_ICACHE
	        ia = casp->n.ia ;
	        lat = icache_latency(pip,mip,csp,ia) ;

	        if (lat < 0)
	            lat = 0 ;
#else
	        lat = 1 ;
#endif /* CF_ICACHE */

/* write the entry into the fetch queue */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4)) {
	            eprintf("ssiw_checkfetch: fetching in=%llu\n",casp->n.in) ;
	            eprintf("ssiw_checkfetch: lat=%u\n",lat) ;
		}
#endif

	        rs = ssfetch_fetch(&wp->fetch,casp,lat) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssiw_checkfetch: ssfetch_fetch() rs=%d\n",
	                rs) ;
#endif

	        if (rs < 0)
	            break ;

	        if (pip->f.itrace)
	            bprintf(pip->it2,"%10llu %16llX %u\n",
	                casp->n.in, casp->n.ia,casp->n.op) ;

/* update */

	        nfetched += 1 ;

/* should we stall after this this fetch? */

#if	CF_BPRED
	        if ((! pip->f.perfpred) &&
	            casp->n.f.cfcond && (! casp->n.f.pred_correct)) {

	            wp->n.c_wait = (wp->c.nasactive / lip->flushdiv) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4))
	                eprintf("ssiw_checkfetch: stalling.c_wait=%u\n",
	                    wp->n.c_wait) ;
#endif

	            break ;
	        }
#endif /* CF_BPRED */

	    } /* end for (fetching instructions) */

#ifdef	COMMENT

/* update checker */

	    if ((rs >= 0) && (nfetched > 0)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssiw_checkfetch: sim_checkrelease() nf=%u\n",
	                nfetched) ;
#endif

	        rs1 = sim_checkrelease(pip,nfetched) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(2)) {
	            if ((rs1 < 0) || DEBUGLEVEL(4))
	                eprintf("ssiw_checkfetch: sim_checkrelease() "
				"rs=%d\n",rs1) ;
	            assert(rs1 >= 0) ;
	        }
#endif

	    } /* end if (checker update) */

#endif /* COMMENT */

	} /* end if (not in a stall) */

ret0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkfetch: ret rs=%d nfetched=%u\n",
	        rs,nfetched) ;
#endif

	return (rs >= 0) ? nfetched : rs ;
}
/* end subroutine (ssiw_checkfetch) */


/* check for possible instruction dispatch (fetch => ISes) */
static int ssiw_checkdispatch(wp,pip,mip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
{
	CHECKER	*csp = &pip->check ;

	SSAS	*lasp ;

	int	rs = 0, rs1, i, asid ;
	int	tt ;
	int	nempty, nloadable ;
	int	ndispatchable ;
	int	nloaded = 0 ;
	int	n = 0 ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: entered\n") ;
#endif /* CF_DEBUG */

	nempty = lip->iwsize - wp->n.nasactive ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: ISes nempty=%d\n",nempty) ;
#endif /* CF_DEBUG */

#if	CF_READYLOAD

/* are there any ASes free or about to become free */

	asid = wp->n.asri ;
	for (i = 0 ; i < wp->n.nasactive ; i += 1) {

	    lasp = wp->ssas + asid ;
	    rs = ssas_readyload(lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkdispatch: ssas_readyload() rs=%d\n",rs) ;
#endif

	    if (rs <= 0)
	        break ;

	    asid = (asid + 1) % lip->iwsize ;

	} /* end for */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: ISes becoming nempty=%d\n",i) ;
#endif

#else
	i = 0 ;
#endif /* CF_READYLOAD */

	nloadable = nempty + i ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: ISes loadable=%d\n",nloadable) ;
#endif

	if (nloadable == 0)
		goto ret0 ;

/* how many are ready to be dispatched? */

#if	CF_NDISPATCH
	rs = ssfetch_ndispatch(&wp->fetch) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: ssfetch_ndispatch() rs=%d\n",
	        rs) ;
#endif

#else /* CF_NDISPATCH */
	{
		SSFETCH_INFO	fi ;


		rs = ssfetch_info(&wp->fetch,&fi) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: ssfetch_ndispatch() "
		"rs=%d n=%d wc=%d\n",
	        rs,fi.n,fi.waitcount) ;
#endif

	}
#endif /* COMMENT */

	ndispatchable = rs ;
	if (rs < 0)
		goto ret0 ;

	nloaded = 0 ;
	if ((ndispatchable > 0) && (nloadable > 0)) {

	    n = (lip->dwidth > 0) ? lip->dwidth : INT_MAX ;
	    if (n > ndispatchable)
		n = ndispatchable ;

	    if (n > nloadable)
		n = nloadable ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: loop nload=%d\n",n) ;
#endif

	    asid = wp->n.aswi ;
	    for (i = 0 ; i < n ; i += 1) {

	        SSAS	*dasp ;


	        rs = ssfetch_dispatch(&wp->fetch,i,&dasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssiw_checkdispatch: ssfetch_dispatch() "
				"rs=%d DASP=%p\n", rs,dasp) ;
#endif

	        if (rs < 0)
	            break ;

	        lasp = wp->ssas + asid ;
	        tt = wp->n.nasactive + i ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4)) {
	            eprintf("ssiw_checkdispatch: loading AS(%p) "
			"asid=%u tt=%d\n",
	                lasp,asid,tt) ;
	        }
#endif /* CF_DEBUG */

	        if (pip->f.itrace)
	            bprintf(pip->it3,"%10llu %16llX %u\n",
	                dasp->n.in,dasp->n.ia,dasp->n.op) ;

/* load it */

	        rs = ssas_load(lasp,dasp,tt) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssiw_checkdispatch: ssas_load() rs=%d i=%d\n",
	                rs,i) ;
#endif

	        if (rs < 0)
	            break ;

		nloaded += 1 ;
	        asid = (asid + 1) % lip->iwsize ;

	    } /* end for (dispatching instructions to ASes) */

/* update the FQ state */

	if (rs >= 0)
	        rs = ssfetch_dispatchu(&wp->fetch,nloaded) ;

/* update IW state */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("ssiw_checkdispatch: nloaded=%d\n",nloaded) ;
#endif

	    if (rs >= 0) {
	        wp->n.aswi = asid ;
	        wp->n.nasactive += nloaded ;
	    }

	} /* end if (had some to dispatch) */

ret0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkdispatch: ret rs=%d nloaded=%d\n",
	        rs,nloaded) ;
#endif

	return (rs >= 0) ? i : rs ;
}
/* end subroutine (ssiw_checkdispatch) */

#endif /* CF_FETCH */


/* load an AS */
static int ssiw_asload(wp,pip,mip,lip,lasp,ci,tt)
SSIW		*wp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
SSAS		*lasp ;
int		ci, tt ;
{
	OPERAND	*op ;

	CHECKER	*csp = &pip->check ;

	SSAS	*casp ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_asload: asid=%u ci=%d tt=%d\n",
	        lasp->asid,ci,tt) ;
#endif

	casp = csp->cas + ci ;
	rs = ssas_load(lasp,casp,tt) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_asload: ssas_load() rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssiw_asload) */


/* broadcast operands */
static int ssiw_broadcast(wp,pip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	OPERAND	*op ;

	SSAS	*lases ;

	SSLSQ_CURSOR	lsqcur ;

	SSREG_CURSOR	regcur ;

	int	rs, rs1, rs2 ;
	int	i, j, k ;
	int	asri, assi ;
	int	n ;
	int	m = 0 ;
	int	c = 0 ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_broadcast: entered\n") ;
#endif

	lases = wp->ssas ;
	n = wp->c.nasactive ;

/* look through the ASes for exportable operands first */

	asri = wp->c.asri ;
	for (i = 0 ; i < (n - 1) ; i += 1) {

	    rs = SR_OK ;
	    for (j = 0 ; (j < MACHSTATE_MAXOPSO) && (rs >= 0) ; j += 1) {

	        rs1 = ssas_opexport((lases + asri),j,&op) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4)) {
	            eprintf("ssiw_broadcast: asid=%u op=%u "
	                "ssas_opexport() rs=%d\n",
	                asri,j,rs1) ;
	        }
#endif

	        if (rs1 < 0)		/* SR_EMPTY */
	            break ;

	        if (rs1 > 0) {

	            m = n - i ;
	            assi = (asri + 1) % lip->iwsize ;
	            for (k = 0 ; k < m ; k += 1) {

	                rs2 = ssas_opsnoop((lases + assi),op) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(4))
	                    eprintf("ssiw_broadcast: asid=%u "
	                        "ssas_opsnoop() rs=%d\n",
	                        assi,rs2) ;
#endif

	                if (rs2 > 0)
	                    c += rs2 ;

	                assi = (assi + 1) % lip->iwsize ;

	            } /* end for (snooping ASes) */

	        } /* end if (got one) */

	        m += 1 ;

	    } /* end for (operands in each AS) */

	    asri = (asri + 1) % lip->iwsize ;
	    if ((lip->fbuses > 0) && (m >= lip->fbuses))
	        break ;

	} /* end for (ASes) */

/* now go through the register file for exportable operands */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_broadcast: register file stuff\n") ;
#endif

	ssreg_cursorinit(&wp->reg,&regcur) ;

	while ((rs1 = ssreg_opexport(&wp->reg,&regcur,&op)) >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        eprintf("ssiw_broadcast: j=%u ssreg_opexport() rs=%d \n",
	            j,rs1) ;
#endif

	    if (rs1 > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(6))
	            eprintf("ssiw_broadcast: exportable a=%u tt=%d\n",
	                j,op->tt) ;
#endif

	        assi = wp->c.asri ;
	        for (k = 0 ; k < n ; k += 1) {

	            rs2 = ssas_opsnoop((lases + assi),op) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf(
	                    "ssiw_broadcast: asid=%u ssas_opsnoop() rs=%d\n",
	                    assi,rs2) ;
#endif

	            if (rs2 > 0)
	                c += rs2 ;

	            assi = (assi + 1) % lip->iwsize ;

	        } /* end for */

	        m += 1 ;

	    } /* end if (got one) */

	    if ((lip->fbuses > 0) && (m >= (lip->fbuses + 2)))
	        break ;

	} /* end while (getting exports from register file) */

	ssreg_cursorfree(&wp->reg,&regcur) ;

/* now go through the LSQ for exportable operands */

#if	CF_SSLSQ

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_broadcast: LSQ stuff\n") ;
#endif

	sslsq_cursorinit(&wp->lsq,&lsqcur) ;

	while ((rs1 = sslsq_opexport(&wp->lsq,&lsqcur,&op)) >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        eprintf("ssiw_broadcast: sslsq_opexport() rs=%d\n",
	            rs1) ;
#endif

	    if (rs1 > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(6))
	            eprintf("ssiw_broadcast: exportable a=%llx tt=%d\n",
	                op->a,op->tt) ;
#endif

	        assi = wp->c.asri ;
	        for (k = 0 ; k < n ; k += 1) {

	            rs2 = ssas_opsnoop((lases + assi),op) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf(
	                    "ssiw_broadcast: asid=%u ssas_opsnoop() rs=%d\n",
	                    assi,rs2) ;
#endif

	            if (rs2 > 0)
	                c += rs2 ;

	            assi = (assi + 1) % lip->iwsize ;

	        } /* end for */

	    } /* end if (got one) */

	} /* end for (LSQ) */

	sslsq_cursorfree(&wp->lsq,&lsqcur) ;

#endif /* CF_SSLSQ */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_broadcast: ret c=%d\n",c) ;
#endif

	return c ;
}
/* end subroutine (ssiw_broadcast) */


/* check if we are ready to retire some ASes */
static int ssiw_checkretire(wp,pip,mip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
SS		*mip ;
struct ssinfo	*lip ;
{
	int	rs = SR_OK ;
	int	n ;
	int	ncommit = 0 ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretire: entered nasactive=%d\n",
	        wp->c.nasactive) ;
#endif

	if (wp->c.nasactive <= 0)
	    goto ret0 ;

/* check them, returns number of ASes ready to commit */

	rs = ssiw_checkretireases(wp,pip,lip) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretire: ssiw_checkretireases() rs=%d\n",rs) ;
#endif

	n = rs ;
	if (rs <= 0) {

	    wp->f.commit = FALSE ;
	    wp->n.c_commit = 0 ;
	    goto ret0 ;
	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretire: ASes say they are ready to retire\n") ;
#endif


	wp->f.commit = TRUE ;

/* do the commitment with the ASes */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretire: committing the ASes n=%d\n",n) ;
#endif

#if	CF_RETIRE
	rs = ssiw_retireases(wp,pip,lip,n) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretire: ssiw_retireases() rs=%d\n",rs) ;
#endif

	ncommit = rs ;

/* get out */
ret0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretire: ret rs=%d ncommit=%d\n",
	        rs,ncommit) ;
#endif

	return (rs >= 0) ? ncommit : rs ;
}
/* end subroutine (ssiw_checkretire) */


/* check to see how many ASes can retire */
static int ssiw_checkretireases(wp,pip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	SSAS	*lasp ;

	SSAS_INFO	asi ;

	ULONG	ta ;

	int	rs = SR_OK, rs1, i ;
	int	asid ;
	int	f_blowoff = FALSE ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretireases: entered nasactive=%d\n",
	        wp->n.nasactive) ;
#endif

	asid = wp->n.asri ;
	for (i = 0 ; i < wp->n.nasactive ; i += 1) {

	    lasp = wp->ssas + asid ;
	    if (f_blowoff) {

	        rs = ssas_getia(lasp,&asi.ia) ;

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if (rs > 0) {

	            if (asi.ia == ta)
	                f_blowoff = FALSE ;

	        } /* end if (valid instruction) */

	    } /* end if (blowoff mode) */

	    if (! f_blowoff) {

	        rs = ssas_readyretire(lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf(
	                "ssiw_checkretireases: asid=%u ssas_readyretire() "
				"rs=%d\n",
	                asid,rs) ;
#endif

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if (rs <= 0)
	            break ;

	        rs = ssas_info(lasp,&asi) ;

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if ((rs >= 0) &&
	            asi.f_enabled && asi.f_cf && asi.f_taken) {

	            ta = asi.ta ;
	            f_blowoff = TRUE ;

	        } /* end if (GOTO) */

	    } /* end if (not in blowoff mode) */

	    asid = (asid + 1) % lip->iwsize ;

	} /* end for (looping over active ISes) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_checkretireases: ret rs=%d i=%d\n",rs,i) ;
#endif

	return (rs >= 0) ? i : rs ;
}
/* end subroutine (ssiw_checkretireases) */


#if	CF_RETIRE

/* retire the ASes (not an easy job) */
static int ssiw_retireases(wp,pip,lip,n)
SSIW		*wp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
int		n ;
{
	SSAS		*lasp ;

	SSAS_INFO	asi ;

	ULONG		ta ;

	int	rs, rs1, i ;
	int	asid ;
	int	tt ;
	int	ncommit = 0 ;
	int	nretire = 0 ;
	int	f_goto = FALSE ;
	int	f_ij = FALSE ;		/* indirect jump */
	int	f_oow ;			/* out-of-window */
	int	f_cfchange = FALSE ;
	int	f_blowoff = FALSE ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_retireases: n=%d ctt=%d\n",
	        n,tt) ;
#endif

/**** "blow-off" mode:

	The "blowoff" mode is when a prior IS has a control-flow-change
	and therefore all subsequent ISes (including the one
	being examined at the moment need to be forcefully retired.
	Obsiously, we do not start in "blow-off" mode, but enter it as
	we examine ISes.

****/

	asid = wp->n.asri ;
	for (i = 0 ; i < wp->n.nasactive ; i += 1) {

	    lasp = wp->ssas + asid ;
	    if (f_blowoff) {

/* in "blowoff" mode, we have a TA in the 'ta' variable */

	        rs = ssas_getia(lasp,&asi.ia) ;

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if (rs > 0) {

	            if (asi.ia == ta)
	                f_blowoff = FALSE ;

	        } /* end if (valid instruction) */

	    } /* end if (blowoff mode) */

	    if (f_blowoff) {

/* signal the retirement to the IS */

		nretire += 1 ;
	        rs = ssas_retire(lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	        rs1 = ssiw_audit(wp,pip,lip) ;

	        if (rs1 < 0) {
	            eprintf("ssiw_retireases: after ssas_retire() 1\n") ;
	            rs = rs1 ;
	        }
#endif

	    } /* end if (blow-off) */

	    if (! f_blowoff) {

	        rs = ssas_readyretire(lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	        rs1 = ssiw_audit(wp,pip,lip) ;

	        if (rs1 < 0) {
	            eprintf("ssiw_retireases: after ssas_readyretire() 2\n") ;
	            rs = rs1 ;
	        }
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("ssiw_retireases: ssas_readyretire() rs=%d "
	                "asid=%d\n",rs,asid) ;
#endif

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if (rs <= 0)
	            break ;

/* signal retirement */

		nretire += 1 ;
	        rs = ssas_retire(lasp) ;

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	        rs1 = ssiw_audit(wp,pip,lip) ;

	        if (rs1 < 0) {
	            eprintf("ssiw_retireases: after ssas_retire() 2\n") ;
	            rs = rs1 ;
	        }
#endif

/* commit operands to permanent storage */

	        if (rs > 0) {

	            rs = ssiw_commit(wp,lasp,asid) ;

	            ncommit += 1 ;

#if	CF_MASTERDEBUG && CF_DEBUG && CF_SAFE3
	            rs = ssiw_audit(wp,pip,lip) ;

	            if (rs < 0)
	                eprintf("ssiw_retireases: before ssiw_info()\n") ;
#endif

	        } /* end if (committing) */

/* check for GOTOs */

	        if (rs >= 0)
	            rs = ssas_info(lasp,&asi) ;

	        if ((rs < 0) && (rs != SR_EMPTY))
	            break ;

	        if ((rs >= 0) &&
	            asi.f_enabled && asi.f_cf && asi.f_taken) {

	            ta = asi.ta ;
	            f_blowoff = TRUE ;

	        } /* end if (GOTO) */

	    } /* end if (not in blowoff mode) */

	    asid = (asid + 1) % lip->iwsize ;

	} /* end for (looping through active ASes) */

/* handle bad stuff first */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2) && (rs < 0) && (rs != SR_EMPTY))
	    eprintf("ssiw_retireases: bad stuff rs=%d\n",rs) ;
#endif

	if ((rs < 0) && (rs != SR_EMPTY))
	    return rs ;

/* number of active ASes reduced */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("ssiw_retireases: reducing n.nasactive=%d i=%d "
	        "ncommit=%d\n",
	        wp->n.nasactive,i,ncommit) ;
#endif

	wp->n.asri = (wp->n.asri + i) % lip->iwsize ;
	wp->n.nasactive -= i ;

/* check */

	if ((rs >= 0) && (i != nretire))
		bprintf(pip->efp,"%s: SSIW retirement bug\n",
		pip->progname) ;

/* signal a shift of the machine by the same amount */

	ssiw_shift(wp,nretire) ;


#if	CF_GOTO /* handle GOTOs for Levo-like (multi-column) machines */

/* handle jumps out of the window */

	rs = ssiw_targetin(wp,pip,lip,asi.ta,tt) ;

	if (f_goto) {

	    int	lbi ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_retireases: processing GOTO\n") ;
#endif

/* OK, flush everything except the column that we just committed */

	    rs = ssiw_flushwindow(wp,pip,lip,ci) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_retireases: ssiw_flushwindow() rs=%d\n",rs) ;
#endif

	} /* end if (doing a GOTO) */

/* update some statistics */

	wp->s.commit_col += 1 ;		/* column commitments */

#endif /* CF_GOTO */

	wp->n.c_commit = 0 ;

	if ((rs >= 0) && (nretire > 0)) {

		wp->n.checkturn = nretire ;

	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_retireases: ret rs=%d ncommit=%d\n",
	        rs,ncommit) ;
#endif

	return (rs >= 0) ? ncommit : rs ;
}
/* end subroutine (ssiw_retireases) */

#endif /* CF_RETIRE */


/* commit operands back to machine-permanent storage */
static int ssiw_commit(wp,lasp,asid)
SSIW		*wp ;
SSAS		*lasp ;
int		asid ;
{
	struct proginfo	*pip = wp->pip ;

	OPERAND	*op ;

	int	rs, rs1, i, j ;
	int	f_used = FALSE ;


	rs = SR_OK ;
	for (j = 0 ; (j < MACHSTATE_MAXOPSO) && (rs >= 0) ; j += 1) {

	    rs1 = ssas_opget(lasp,j,&op) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4)) {
	        eprintf("ssiw_commit: asid=%u op=%u\n", asid,j) ;
	        eprintf("ssiw_commit: ssas_opget() rs=%d \n",rs) ;
	    }
#endif

	    if (rs1 < 0)		/* SR_EMPTY */
	        break ;

	    if (rs1 > 0) {

	        f_used = TRUE ;
	        switch (op->type) {

	        case OPERAND_TREG:
	            ssreg_opmerge(&wp->reg,op) ;

	            break ;

	        case OPERAND_TMEM:

#if	CF_SSLSQ
	            sslsq_opmerge(&wp->lsq,op) ;
#endif

	            break ;

	        default:
	            break ;

	        } /* end switch */

	    } /* end if (got one) */

	} /* end for (operands in the AS) */

	return (rs >= 0) ? f_used : rs ;
}
/* end subroutine (ssiw_commit) */


/* handle the consequences of a machine shift within our own object */
static int ssiw_handleshift(wp,pip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	int	rs, j ;
	int	totalrows ;


	if (! wp->f.shift)
	    return SR_OK ;

	wp->f.shift = FALSE ;



	return SR_OK ;
}
/* end subroutine (ssiw_handleshift) */


#ifdef	COMMENT /* this is only for a Levo-solumn-like machine */

/* flush the entire execution window except the specified column! */
static int ssiw_flushwindow(wp,pip,lip,ci)
SSIW		*wp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
int		ci ;
{
	int	rs = SR_OK, j, k ;
	int	path, nmlcols ;
	int	sgci ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_flushwindow: after column=%d\n",ci) ;
#endif

	sgci = ci / SSINFO_NASWPSG ;

/* flush the AS columns first */

	nmlcols = 0 ;
	j = (ci + 1) % lip->totalcols ;
	for (k = 0 ; k < (lip->totalcols - 1) ; k += 1) {

	    if ((path = wp->c.cols[j].path) >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssiw_flushwindow: live path=%d column=%d\n",
	                path,j) ;
#endif

	        if (path == 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf(
	                    "ssiw_flushwindow: killing ML path col=%d\n",
	                    j) ;
#endif

	            rs = ssiw_killpath(wp,pip,lip,path,j,(lip->nsgcols - 1)) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssiw_flushwindow: ssiw_killpath() rs=%d\n",
	                    rs) ;
#endif

	            wp->n.cols[j].f.kill = TRUE ;
	            wp->n.cols[j].f.used = FALSE ;
	            wp->n.cols[j].path = -1 ;
	            nmlcols += 1 ;

	        } else {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf(
	                    "ssiw_flushwindow: killing DEE path col=%d\n",
	                    j) ;
#endif

	            rs = ssiw_killpath(wp,pip,lip,path,j,-1) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssiw_flushwindow: ssiw_killpath() rs=%d\n",
	                    rs) ;
#endif

	            wp->n.paths[path].ncols = 0 ;
	            wp->n.paths[path].ci = -1 ;

	        } /* end if (ML or DEE) */

	    } /* end if (killing off a path) */

	    j = (j + 1) % lip->totalcols ;

	} /* end for */

	wp->n.paths[SSINFO_PATHML].ncols -= nmlcols ;

/* flush the RF units that were at the bottoms of those SG columns */

	sgci = (sgci + 1) % lip->nsgcols ;
	for (k = 0 ; k < (lip->nsgcols - 1) ; k += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("ssiw_flushwindow: rfdisable check sgci=%d\n",sgci) ;
#endif

	    if (wp->n.sgcols[sgci].rfbi >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssiw_flushwindow: rfdisable sgci=%d\n",sgci) ;
#endif

	        rs = ssiw_rfdisable(wp,pip,lip,sgci) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssiw_flushwindow: ssiw_rfdisabled() rs=%d\n",
	                rs) ;
#endif

	        if (rs < 0)
	            break ;

	    } /* end if (had a live one) */

	    sgci = (sgci + 1) % lip->nsgcols ;

	} /* end for (looping over SG columns) */

/* we-re done */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssiw_flushwindow: rs=%d killed mlcols=%d\n",
	        rs,nmlcols) ;
#endif

	return ((rs >= 0) ? nmlcols : rs) ;
}
/* end subroutine (ssiw_flushwindow) */

#endif /* COMMENT */


static int ssiw_audit(wp,pip,lip)
SSIW		*wp ;
struct proginfo	*pip ;
struct ssinfo	*lip ;
{
	int	rs, n, i ;


#if	CF_MASTERDEBUG && (CF_DEBUG || CF_DEBUGS)
	if (DEBUGLEVEL(6))
	    eprintf("ssiw_audit: entered\n") ;
#endif


/* ASes */

	n = lip->iwsize ;
	for (i = 0 ; i < n ; i += 1) {

	    rs = ssas_audit(wp->ssas + i) ;

#if	CF_MASTERDEBUG && (CF_DEBUG || CF_DEBUGS)
	    if (DEBUGLEVEL(2) && (rs < 0))
	        eprintf("ssiw_audit: i=%u ssas_audit() rs=%d\n",i,rs) ;
#endif

	    if (rs < 0)
	        break ;

	} /* end for */


/* PEs */

	if (rs >= 0)
	    rs = sspe_audit(&wp->pe) ;

#if	CF_MASTERDEBUG && (CF_DEBUG || CF_DEBUGS)
	if (DEBUGLEVEL(2) && (rs < 0))
	    eprintf("ssiw_audit: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssiw_audit) */


static int ssiw_dumpases(op,fp)
SSIW		*op ;
bfile		*fp ;
{
	SS		*mip ;

	struct ssinfo	*lip ;

	int	rs = 0, i ;


	mip = op->mip ;
	lip = op->lip ;


	return rs ;
}
/* end subroutine (ssiw_dumpases) */


static int ssiw_dumpall(op,fname)
SSIW		*op ;
const char	fname[] ;
{
	SS		*mip ;

	struct ssinfo	*lip ;

	bfile	dumpfile, *dfp = &dumpfile ;

	int	rs = 0, i ;


	mip = op->mip ;
	lip = op->lip ;

	if (fname == NULL)
		return SR_FAULT ;

	if (fname[0] == '\0')
		return SR_INVALID ;

	rs = bopen(dfp,fname,"wct",0666) ;

	if (rs < 0)
		goto ret0 ;

	ssiw_dump(op,dfp) ;

	ssiw_dumpases(op,dfp) ;

	bclose(dfp) ;

ret0:
	return rs ;
}
/* end subroutine (ssiw_dumpall) */



