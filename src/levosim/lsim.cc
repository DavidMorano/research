/* lsim SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* Levo simulator state sequencing operations */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable debug print-outs */
#define	CF_DEBUG	0		/* switchable debugging */
#define	CF_SAFE		1		/* play it safe? */
#define	CF_MACHINE	1		/* call the LEVO machine methods */
#define	CF_MEMSET	1		/* zero out object memory */
#define	CF_MAXCLOCKS	1		/* maximum numbers of clocks */
#define	CF_TESTMAP	1		/* test program mapping */
#define	CF_TESTMAPSHORT	1		/* short test program mapping */

/* revision history:

	= 2000-02-15, David Morano
	This code was started (for LevoSim).

*/

/* Copyright © 2000 David A-D- Morano.  All rights reserved. */

/*******************************************************************************

  	Object:
	lsim

	Description:
	This is the object (roughly) for handling simulator specific
	operations such as call-backs.

*******************************************************************************/

#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<elf.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<usystem.h>
#include	<vechand.h>
#include	<vecitem.h>
#include	<hdb.h>
#include	<paramfile.h>
#include	<plainq.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"
#include	"lmapprog.h"
#include	"lsim.h"		/* ourselves */


/* local defines */

#define	LSIM_MAGIC		0x95827364
#define	LSIM_CLOCKINTERVAL	200

#ifndef	VBUFLEN
#define	VBUFLEN			100
#endif


/* external subroutines */


/* local structures */


/* forward references */

int		lsim_callthem(LSIM *) ;
int		lsim_getsymval(LSIM *,char *,uint *) ;

#ifdef	COMMENT
static int	hashfunc() ;
#endif

static int	lsim_syscalls(LSIM *) ;
static int	lsim_syscallsfree(LSIM *) ;

#if	CF_DEBUG || CF_DEBUGS
static int	lsim_prpq(LSIM *,ULONG) ;
static int	lsim_finddatamem(LSIM *,char *,uint *) ;
#endif

#if	CF_DEBUG || CF_DEBUGS
static int	datasymrank(Elf32_Sym *) ;
#endif


/* local variables */

static char	*const lparams[] = {
	"sim:maxclocks",	/* maximum number of clocks */
	"sim:fastqueues",	/* number of fast queues */
	NULL
} ;

#define	LPARAM_MAXCLOCKS	0
#define	LPARAM_FASTQUEUES	1

static char	*const syscalls[] = {
	"open",
	"close",
	"read",
	"write",
	"fcntl",
	"ioctl",
	"brk",
	"sbrk",
	"mmap",
	"munmap",
	"getpid",
	"getppid",
	"getpgrp",
	"getpgid",
	"getuid",
	"geteuid",
	"getgid",
	"getwgid",
	"atexit",
	"_exit",
	"__exit",
	"exit",
	NULL
} ;


/* exported subroutines */


int lsim_init(mip,pip,pfp,maxclocks,simprog,machobj)
LSIM		*mip ;
struct proginfo	*pip ;
PARAMFILE	*pfp ;
LONG		maxclocks ;
LSIM_SIMPROG	*simprog ;
LSIM_MACHOBJ	*machobj ;
{
	ULONG	ulw ;

	int	rs = SR_OK ;
	int	i, j ;
	int	iw, sl, vl ;
	int	size ;

	char	vbuf[VBUFLEN + 1] ;
	char	*cp ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_init: entered\n") ;
#endif

	memset(mip,0,sizeof(LSIM)) ;

	mip->magic = 0 ;
	mip->pip = pip ;
	mip->nentries = 0 ;
	mip->nq = LSIM_NQUEUES ;
	mip->cqi = 0 ;
	mip->clock = 0 ;
	mip->clock_count = 0 ;

	mip->f.syscalls = FALSE ;


/* store what we have so far */

/* the program information */


/* the machine object information */

	mip->topobjp = NULL ;
	mip->topcombp = NULL ;
	mip->topclockp = NULL ;
	if (machobj != NULL) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_init: got a machobj, LEVO=%08lx\n",
	            machobj->topobjp) ;
#endif

	    mip->topobjp = machobj->topobjp ;
	    mip->topcombp = machobj->topcombp ;
	    mip->topclockp = machobj->topclockp ;

	}

/* get some parameters */

	for (i = 0 ; lparams[i] != NULL ; i += 1) {

	    vl = paramfile_fetch(pfp,lparams[i],NULL,vbuf,VBUFLEN) ;

	    if (vl >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4) {
	            debugprintf("lsim_init: %i lparam=%s \n",i,lparams[i]) ;
	            debugprintf("lsim_init: %i val=%s\n",i,vbuf) ;
	        }
#endif

	        switch (i) {

	        case LPARAM_MAXCLOCKS:
	            rs = cfnumull(vbuf,vl,&ulw) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	                debugprintf("lsim_init: rs=%d maxclocks=%llu\n",rs,ulw) ;
#endif

	            if (rs >= 0) {

	                switch (i) {

	                case LPARAM_MAXCLOCKS:
				if (maxclocks == 0)
	                    		maxclocks = ulw ;

	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        case LPARAM_FASTQUEUES:
	            rs = cfnumui(vbuf,vl,&iw) ;

	            if (rs >= 0) {

	                switch (i) {

	                case LPARAM_FASTQUEUES:
	                    mip->nq = iw ;
	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        } /* end swtich */

	    } /* end if */

	} /* end for (getting simulation parameters) */


/* check and store any new parameters */

	if (maxclocks == 0)
	    maxclocks = LSIM_MAXCLOCKS ;

	mip->maxclocks = maxclocks ;

	if (mip->nq < 5)
	    mip->nq = 5 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	    debugprintf("lsim_init: maxclock=%llu\n",mip->maxclocks) ;
	    debugprintf("lsim_init: fastqueues=%u\n",mip->nq) ;
	}
#endif /* CF_DEBUG */


/* start initializing our scheduling apparatus */

	size = mip->nq * sizeof(vechand) ;
	rs = uc_malloc(size,&mip->qes) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lsim_init: Qs malloc failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(mip->qes,size,"lsim_init:calloutqueues") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    iw = i ;
	    rs = vechand_start((mip->qes + i),0,VECHAND_PSWAP) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lsim_init: Qs vechand(%d) failed rs=%d\n",i,rs) ;
#endif

	    if (rs < 0)
	        goto bad1 ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_init: queue list(%d)=%08lx\n",(mip->qes + i)) ;
#endif

	} /* end for */

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4) {
	    debugprintf("lsim_init: ck=%llu \n",mip->clock) ;
	    debugprintf("lsim_init: 1 c0=%08x c1=%08x\n",
	        getarrayint(&mip->clock,0),
	        getarrayint(&mip->clock,1)) ;
	}
#endif /* CF_DEBUG */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_init: plainq_start() FQ=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_start(&mip->fq) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lsim_init: plainq failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_printf("plainq_start") ;
#endif

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4) {
	    debugprintf("lsim_init: ck=%llu \n",mip->clock) ;
	    debugprintf("lsim_init: 2 c0=%08x c1=%08x\n",
	        getarrayint(&mip->clock,0),
	        getarrayint(&mip->clock,1)) ;
	}
#endif /* CF_DEBUG */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_init: hdb_start()\n") ;
#endif

	rs = hdb_start(&mip->pq,mip->nq,NULL,NULL) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lsim_init: PQ failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad3 ;

#ifdef	MALLOCLOG
	malloclog_printf("hdb_start") ;
#endif

/* map the program that we want to simulate */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_init: progfile=%s\n",simprog->program) ;
#endif

	rs = lmapprog_init(&mip->ourprog,pip,
	    simprog->program,simprog->argv,simprog->envv) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lsim_init: lmapprog_init() failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad4 ;

#ifdef	MALLOCLOG
	malloclog_printf("lmapprog_init") ;
#endif


/* supposedly, we are fairly well done ! (need to index SYSCALLs later) */

	mip->magic = LSIM_MAGIC ;


/* if so, do some little tests ! */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	    uint		iaddr, ma ;
	    uint		val ;

	    debugprintf("lsim_init: start of simulated program i-memory\n") ;
	    lmapprog_getentry(&mip->ourprog,&iaddr) ;
	    for (i = 0 ; i < 5 ; i += 1) {
	        rs = lmapprog_readint(&mip->ourprog,iaddr,&val) ;
	        debugprintf("lsim_init: rs=%d ia=%08x %08x\n",rs,iaddr,val) ;
	        iaddr += 4 ;
	    }

/* read it as instructions */

	    debugprintf("lsim_init: reading i-memory as instructions\n") ;
	    lmapprog_getentry(&mip->ourprog,&iaddr) ;
	    for (i = 0 ; i < 5 ; i += 1) {
	        rs = lmapprog_readinstr(&mip->ourprog,iaddr,&val) ;
	        debugprintf("lsim_init: rs=%d ia=%08x %08x\n",rs,iaddr,val) ;
	        iaddr += 4 ;
	    }

/* find the start of some data memory */

	    rs = lsim_finddatamem(mip,"fdata",&iaddr) ;

	    debugprintf("lsim_init: find data 'fdata' rs=%d\n",rs) ;

		if (rs < 0) {

	    rs = lsim_finddatamem(mip,"_fdata",&iaddr) ;

	    debugprintf("lsim_init: find data '_fdata' rs=%d\n",rs) ;

		}

		if (rs < 0) {

	    rs = lsim_finddatamem(mip,".rodata",&iaddr) ;

	    debugprintf("lsim_init: find data '.rodata' rs=%d\n",rs) ;

		}

		if (rs < 0) {

	    rs = lsim_finddatamem(mip,".sdata",&iaddr) ;

	    debugprintf("lsim_init: find data '.sdata' rs=%d\n",rs) ;

		}

		if (rs >= 0) {

	    debugprintf("lsim_init: start of simulated program d-memory\n") ;
	    for (i = 0 ; i < 10 ; i += 1) {
	        rs = lmapprog_readint(&mip->ourprog,iaddr,&val) ;
	        debugprintf("lsim_init: rs=%d da=%08x %08x\n",rs,iaddr,val) ;
	        iaddr += 4 ;
	    }

		} /* end if (d-memory) */

/* OK, do some free-lance testing ! */

	        debugprintf("lsim_init: free-lance testing\n") ;
	ma = 0x10009500 ;
	for (i = 0 ; i < 72 ; i += 1) {

	        rs = lmapprog_readint(&mip->ourprog,ma,&val) ;

		if (rs < 0)
			break ;

	        debugprintf("lsim_init: a=%08x %08x\n",ma,val) ;

		ma += 4 ;

	} /* end for */

	        debugprintf("lsim_init: free-lance result i=%d rs=%d\n",i,rs) ;

	}
#endif /* CF_DEBUG */


/* test the program mapping */

#if	CF_TESTMAP
	if (pip->debuglevel > 1) {

	    LMAPPROG		*mpp ;

	    LMAPPROG_SEGMENT	*psp ;

	    uint	maddr ;
	    uint	val1, val2 ;
		uint	max ;

	    int	rs1, rs2 ;
	    int	f_failed = FALSE ;


	    debugprintf("lsim_init: simulated memory compare - ") ;
	    mpp = &mip->ourprog ;
	    for (i = 0 ; vecitem_get(&mpp->segments,i,&psp) >= 0 ; i += 1) {

	        maddr = psp->vaddr ;
#if	CF_TESTMAPSHORT
		max = MIN((psp->vaddr + psp->vlen),0x10) ;
#else
		max = psp->vaddr + psp->vlen ;
#endif
	        while (maddr < max) {

	            rs1 = lmapprog_readint(mpp,maddr,&val1) ;

	            rs2 = lmapprog_readtest(mpp,maddr,&val2) ;

	            if ((rs1 != rs2) || (val1 != val2)) {

	                f_failed = TRUE ;
	                debugprintf("lsim_init: testmap error at addr=%08x\n",
	                    maddr) ;

	            } /* end if */

	            maddr += 4 ;

	        } /* end while */

	    } /* end for (outer) */

	    debugprintf("%s\n",
	        (f_failed) ? "failed" : "passed") ;

/* get a symbol value */

	    rs = lsim_getsymval(mip,"main",&maddr) ;

	    debugprintf("lsim_init: lsim_getsymval() rs=%d main=%08x\n",
	        rs,maddr) ;

	} /* end block */
#endif /* F_TESTMAP */

/* if we have any symbols, find any SYSCALLs and index them by address */

	rs = lsim_syscalls(mip) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lsim_init: lsim_syscalls() failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad5 ;


/* finish up */

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 2) {
	    debugprintf("lsim_init: ck=%llu \n",mip->clock) ;
	    debugprintf("lsim_init: 3 c0=%08x c1=%08x\n",
	        getarrayint(&mip->clock,0),
	        getarrayint(&mip->clock,1)) ;
	    debugprintf("lsim_init: exiting OK rs=%d\n",rs) ;
	}
#endif /* CF_DEBUG */

	return rs ;

/* bad things */
bad5:
	lmapprog_free(&mip->ourprog) ;

bad4:
	hdb_finish(&mip->pq) ;

bad3:
	plainq_finish(&mip->fq) ;

bad2:
bad1:
	for (j = 0 ; j < i ; j += 1) {
	    vechand_finish(mip->qes + j) ;
	}
	uc_free(mip->qes) ;
	mip->qes = NULL ;

bad0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 2)
	    debugprintf("lsim_init: exiting BAD rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (lsim_init) */


/* set the machine object information */
int lsim_setmach(mip,machobj)
LSIM		*mip ;
LSIM_MACHOBJ	*machobj ;
{
	struct proginfo	*pip ;
	int		rs = SR_OK ;

	if (mip == NULL) return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_setmach: entered\n") ;
#endif

	if (machobj->topobjp == NULL) return SR_FAULT ;

	if (machobj->topcombp == NULL) return SR_FAULT ;

	if (machobj->topclockp == NULL) return SR_FAULT ;

/* the machine object information */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_setmach: got a machobj, LEVO=%08lx\n",
	        machobj->topobjp) ;
#endif

	mip->topobjp = machobj->topobjp ;
	mip->topcombp = machobj->topcombp ;
	mip->topclockp = machobj->topclockp ;

	return SR_OK ;
}
/* end subroutine (lsim_setmach) */


/* get the mapped program object pointer */
int lsim_getpp(mip,mpp)
LSIM		*mip ;
LMAPPROG	**mpp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_getpp: entered\n") ;
#endif

	if (mpp == NULL)
	    return SR_FAULT ;

	*mpp = &mip->ourprog ;

	return SR_OK ;
}
/* end subroutine (lsim_getpp) */


/* free up whatever! */
int lsim_free(mip)
LSIM	*mip ;
{
	struct proginfo	*pip ;
	LSIM_COE	*ep ;
	vechand		*cqp ;
	int		rs = SR_OK ;
	int		i, j ;

	if (mip == NULL) return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_free: entered\n") ;
#endif

/* free up SYSCALLs data structure */

	if (mip->f.syscalls) {
	    lsim_syscallsfree(mip) ;
	}

/* free up the program mapping */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_free: LMAPPROG\n") ;
#endif

	lmapprog_free(&mip->ourprog) ;

/* free up any and all entries that are out there! */

/* free all entries on the free queue */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_free: free Q entries\n") ;
#endif

	while (plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) >= 0) {
	    uc_free(ep) ;
	}

/* free all entries on the future pending queue */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_free: pending Q entries\n") ;
#endif

	{
	    HDB_DATUM	key, value ;
	    HDB_CUR	hcur ;

	    hdb_curbegin(&mip->pq,&hcur) ;

	    while (hdb_enum(&mip->pq,&hcur,&key,&value) >= 0) {
	        if (value.buf != NULL) {
	            uc_free(value.buf) ;
		}
	    } /* end while */

	    hdb_curend(&mip->pq,&hcur) ;

	} /* end block (freeing pending queue) */

/* free all entries on the individual (fast) queues */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_free: fast Q entries\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    cqp = mip->qes + i ;
	    for (j = 0 ; (rs = vechand_get(cqp,j,&ep)) >= 0 ; j += 1) {
	        if (ep != NULL) {
	            uc_free(ep) ;
		}
	    } /* end while */

	} /* end for */

/* free up all queue objects */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_free: fast Qs\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {
	    vechand_finish(mip->qes + i) ;
	} /* end for */

	plainq_finish(&mip->fq) ;

	hdb_finish(&mip->pq) ;

	uc_free(mip->qes) ;
	mip->qes = NULL ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_free: exiting\n") ;
#endif

	mip->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (lsim_free) */


/* take call-back requests from other objects */
int lsim_callout(mip,func,objp,argp,nclocks,phase)
LSIM	*mip ;
int	(*func)() ;
void	*objp, *argp ;
uint	nclocks ;
int	phase ;
{
	struct proginfo	*pip ;
	LSIM_COE	*ep ;
	int		rs = SR_OK ;
	int		i ;

#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	    debugprintf("lsim_callout: ent func=%08lx objp=%08lx argp=%08lx\n",
	        func,objp,argp) ;
	    debugprintf("lsim_callout: ck=%llu nclocks=%u\n",
	        mip->clock,nclocks) ;
	}
#endif /* CF_DEBUG */

	if (nclocks == 0)
	    return SR_INVALID ;

/* get a free entry */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callout: plainq_rem() FQa=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callout: plainq_rem() rs=%d\n",rs) ;
#endif

	if (rs == SR_OVERFLOW) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_callout: plainq_rem() underflow\n") ;
#endif

	    rs = uc_malloc(sizeof(LSIM_COE),(void *) &ep) ;
	    if (rs < 0)
	        return rs ;

#ifdef	MALLOCLOG
	malloclog_alloc(ep,sizeof(LSIM_COE),"lsim_callout:coe") ;
#endif

	    mip->nentries += 1 ;

	} /* end if */

/* fill it in */

	ep->wake = mip->clock + nclocks ;
	ep->relative = nclocks ;
	ep->phase = phase ;
	ep->func = func ;
	ep->objp = objp ;
	ep->argp = argp ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callout: wake=%llu\n",ep->wake) ;
#endif

	if (nclocks < mip->nq) {

/* short time in the future */

	    i = (mip->cqi + nclocks) % mip->nq ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_callout: short=%u wake=%llu\n",i,ep->wake) ;
#endif

	    rs = vechand_add(mip->qes + i,ep) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_callout: vechand_add rs=%d\n",rs) ;
#endif

	} else {

	    HDB_DATUM	key, value ;


/* handle requests for longer time into the future */

	    ep->key = ep->wake / ((ULONG) mip->nq) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_callout: long future, key=%llu\n",
	            ep->key) ;
#endif

	    key.buf = (char *) &ep->key ;
	    key.len = sizeof(ULONG) ;

	    value.buf = ep ;
	    value.len = sizeof(LSIM_COE) ;

	    rs = hdb_store(&mip->pq,key,value) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_callout: hdb_store rs=%d\n",rs) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {

	        rs = lsim_prpq(mip,ep->key) ;

	        debugprintf("lsim_callout: pending on key=%llu n=%d\n",
	            ep->key,rs) ;
	    }
#endif /* CF_DEBUG */

	} /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callout: exiting rs=%d\n",
	        rs) ;
#endif

	return rs ;
}
/* end subroutine (lsim_callout) */


/* this is main loop (we ditched MINT since nobody wanted to call it) */
int lsim_loop(mip)
LSIM	*mip ;
{
	struct proginfo	*pip ;

	ULONG	clocklast = 0 ;

	int	rs = SR_OK ;


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;
	if (mip->topobjp == NULL)
	    return SR_FAULT ;

	if (mip->topcombp == NULL)
	    return SR_FAULT ;

	if (mip->topclockp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_loop: LEVO a=%08lx\n",
	        mip->topobjp) ;
#endif

/* we're good, let's go */

	mip->clock = 0 ;

	while (TRUE) {

/* call the stuff for combinatorial processing */

	    rs = lsim_callthem(mip) ;

	    if (rs != 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_loop: exiting from combinatorial rs=%d\n",
	        rs) ;
#endif

	        break ;

	    } /* end if (combinatorial exit) */

/* should we do any maintenance activities? */

	if ((mip->clock - clocklast) > LSIM_CLOCKINTERVAL) {

		clocklast = mip->clock ;
		proginfo_check(pip,mip->clock) ;

	} /* end if (maintenance activities) */

/* call the stuff to transition the clock */

#if	CF_MACHINE
	    rs = (*mip->topclockp)(mip->topobjp) ;

	    if (rs != 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_loop: exiting from clock transition rs=%d\n",
	        rs) ;
#endif

	        break ;
	}
#endif

	    mip->clock += 1 ;

#if	CF_MAXCLOCKS
	    if ((mip->maxclocks > 0) && (mip->clock >= mip->maxclocks)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_loop: exiting from maxclocks\n") ;
#endif

	        break ;
		}
#endif /* F_MAXCLOCKS */

	} /* end while */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_loop: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (lsim_loop) */


/* subroutine that is called on each clock */

/*

	Here we want to make the calls for combinatorial logic
	processing.

	The basic flow of this subroutine is :
	
		for each phase starting from zero
		loop for a minimum number of phases 
		and while there are user call-backs to call
			call the top object in the machine
			call any user call-back subroutines

		if we are done with the current fast queues
			grab entries for next block of call-backs
			and put them in the fast queues

*/

int lsim_callthem(mip)
LSIM	*mip ;
{
	struct proginfo	*pip ;

	vechand	*cqp ;

	LSIM_COE	*ep ;

	int	phase ;
	int	lrs, rs, i, j ;
	int	n = 0 ;
	int	f_exit ;


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callthem: entering ck=%llu comb is LEVO=%08lx\n",
	        mip->clock,mip->topobjp) ;
#endif

/* loop through all of the phases */

	lrs = SR_OK ;
	phase = 0 ;
	f_exit = FALSE ;
	while ((! f_exit) || (phase < LSIM_MINPHASES)) {

		mip->phase = phase ;

#if	CF_MACHINE
	    lrs = (*mip->topcombp)(mip->topobjp,phase) ;

	    if (lrs != 0)
	        f_exit = TRUE ;
#else
	    f_exit = TRUE ;
#endif /* F_MACHINE */

/* go through the current queue and call all the ones with the current phase */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	        debugprintf("lsim_callthem: finding callbacks ph=%d\n",
	            phase) ;
	        debugprintf("lsim_callthem: count Q index=%d\n",mip->cqi) ;
	    }
#endif

	    f_exit = TRUE ;
	    cqp = mip->qes + mip->cqi ;
	    for (j = 0 ; (rs = vechand_get(cqp,j,&ep)) >= 0 ; j += 1) {

	        if (ep == NULL) continue ;

	        f_exit = FALSE ;
	        if (ep->phase == phase) {

	            n += 1 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	                debugprintf("lsim_callthem: calling callback\n") ;
	                debugprintf("lsim_callthem: callback=%p objp=%p\n",
	                    ep->func,ep->objp) ;
	            }
#endif

/* call him! */

#if	CF_MACHINE
	            lrs = (*ep->func)(ep->objp,ep->argp,phase) ;

	            if (lrs != 0)
	                f_exit = TRUE ;
#endif /* F_MACHINE */

/* delete him */

	            vechand_del(cqp,j) ;

	            j -= 1 ;	/* compensate for loop counter */

/* but save the memory data structure for quick access later */

	            plainq_ins(&mip->fq,(PLAINQ_ENTRY *) ep) ;

	        } /* end if (got a live one!) */

	        if (f_exit)
	            break ;

	    } /* end for */

	    phase += 1 ;
	    if (lrs != 0)
	        break ;

	} /* end while (looping through clock phases) */

/* the fast queue for this clock period should be empty now! */

	mip->cqi = (mip->cqi + 1) % mip->nq ;

/* if advanced (mip->nq) clocks, then bring forward pending requests */

	mip->clock_count += 1 ;
	if (mip->clock_count == mip->nq) {

	    HDB_DATUM	key, value ;

	    HDB_CUR	cur ;

	    ULONG	clockkey ;

	    uint	ci ;

	    int		f_next ;


	    clockkey = (mip->clock + 1) / ((ULONG) mip->nq) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_callthem: bring forward, ck=%llu key=%llu\n",
	            mip->clock,clockkey) ;
#endif

	    key.buf = (char *) &clockkey ;
	    key.len = sizeof(LONG) ;

/* find all entries with the same key */

	    hdb_curbegin(&mip->pq,&cur) ;

	    f_next = TRUE ;
	    while (rs >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	            debugprintf("lsim_callthem: HDB loop f_next=%d\n",f_next) ;
#endif

	        if (f_next)
	            rs = hdb_fetch(&mip->pq,key,&cur,&value) ;

	        else
	            rs = hdb_getkeyrec(&mip->pq,key,&cur,NULL,&value) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
			if (rs >= 0)
	            debugprintf("lsim_callthem: HDB loop rs=%d\n",rs) ;
			else if (rs == -2)
	            debugprintf("lsim_callthem: HDB loop rs=NOTFOUND\n") ;
			else
	            debugprintf("lsim_callthem: HDB loop rs=**BAD**\n") ;
		}
#endif /* CF_DEBUG */

	        if (rs < 0)
	            break ;

	        ep = (LSIM_COE *) value.buf ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	            debugprintf("lsim_callthem: HDB loop entry=%08lx\n",
	                ep) ;
	            debugprintf("lsim_callthem: HDB loop entry wake=%llu\n",
	                ep->wake) ;
	        }
#endif

	        ci = (uint) (ep->wake - (mip->clock + 1)) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	            debugprintf("lsim_callthem: HDB loop ci=%u\n",ci) ;
#endif

	        i = (mip->cqi + ci) % mip->nq ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
		debugprintf("lsim_callthem: HDB loop adding i=%d QESa=%08lx\n",
	                i,mip->qes) ;
#endif

	        vechand_add(mip->qes + i,ep) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	            debugprintf("lsim_callthem: HDB loop deleting\n") ;
#endif

	        f_next = FALSE ;	/* deletion, do NOT advance cursor */
	        hdb_delcursor(&mip->pq,&cur,1) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	            debugprintf("lsim_callthem: HDB loop pending key=%llu\n",
	                clockkey) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	            rs = lsim_prpq(mip,clockkey) ;
	            debugprintf("lsim_callthem: HDB loop remaining=%d\n",
	                rs) ;
	        }
#endif /* CF_DEBUG */

	    } /* end while */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_callthem: HDB past loop \n") ;
#endif

	    hdb_curend(&mip->pq,&cur) ;

	    mip->clock_count = 0 ;

	} /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callthem: pending Q ckey=%llu\n",
	        2ULL) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {

	    rs = lsim_prpq(mip,2ULL) ;

	    debugprintf("lsim_callthem: pending Q remaining=%d\n",
	        rs) ;
	}
#endif /* CF_DEBUG */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callthem: FQa=%08lx QESa=%08lx\n",
	        &mip->fq,mip->qes) ;
#endif


/* we-re out of here */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_callthem: exiting\n") ;
#endif

	return lrs ;
}
/* end subroutine (lsim_callthem) */


/* return the clock number to the caller */
int lsim_getclock(mip,lp)
LSIM	*mip ;
ULONG	*lp ;
{


	if (mip == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (lp == NULL)
	    return SR_FAULT ;

	*lp = mip->clock ;
	return SR_OK ;
}
/* end subroutine (lsim_getclock) */


/* read program instruction memory (a single 32-bit integer) */
int lsim_readinstr(mip,vaddr,vp)
LSIM	*mip ;
uint	vaddr ;
uint	*vp ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_readinstr(&mip->ourprog,vaddr,vp) ;
}
/* end subroutine (lsim_readint) */


/* read program instruction memory (a single 32-bit integer) */
int lsim_readint(mip,vaddr,vp)
LSIM	*mip ;
uint	vaddr ;
uint	*vp ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_readint(&mip->ourprog,vaddr,vp) ;
}
/* end subroutine (lsim_readinstr) */


/* write an integer (32 bits) to the simulated program memory */
int lsim_writeint(mip,vaddr,data,dp)
LSIM	*mip ;
uint	vaddr ;
uint	data ;
uint	dp ;
{
	struct proginfo	*pip ;


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lsim_writeint: ma=%08x dv=%08x\n",vaddr,data) ;
#endif

	return lmapprog_writeint(&mip->ourprog,vaddr,data,dp) ;
}
/* end subroutine (lsim_writeint) */


/* test for a certain memory access mode */
int lsim_memaccess(mip,vaddr,mode)
LSIM	*mip ;
uint	vaddr ;
uint	mode ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_memaccess(&mip->ourprog,vaddr,mode) ;
}
/* end subroutine (lsim_memaccess) */


/* get the program starting address */
int lsim_getentry(mip,vp)
LSIM	*mip ;
uint	*vp ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getentry(&mip->ourprog,vp) ;
}
/* end subroutine (lsim_getentry) */


/* get the program starting address */
int lsim_getstack(mip,vp)
LSIM	*mip ;
uint	*vp ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getstack(&mip->ourprog,vp) ;
}
/* end subroutine (lsim_getstack) */


/* get the program "break" address */
int lsim_getbreak(mip,vp)
LSIM	*mip ;
uint	*vp ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getbreak(&mip->ourprog,vp) ;
}
/* end subroutine (lsim_getbreak) */


/* get the target program page size */
int lsim_getpagesize(mip,vp)
LSIM	*mip ;
uint	*vp ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getpagesize(&mip->ourprog,vp) ;
}
/* end subroutine (lsim_getpagesize) */


/* get the target program page size */
int lsim_getmax(mip,mp)
LSIM		*mip ;
LSIM_MAXPROG	*mp ;
{
	LMAPPROG_MAXPROG	max ;

	int	rs ;


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	rs = lmapprog_getmax(&mip->ourprog,&max) ;
	if (rs < 0)
	    goto ret0 ;

	memset(mp,0,sizeof(LSIM_MAXPROG)) ;

	mp->stack = max.stack ;
	mp->data = max.data ;

ret0:
	return rs ;
}
/* end subroutine (lsim_getmax) */


/* set the program "break" address */
int lsim_setbreak(mip,newval)
LSIM	*mip ;
uint	newval ;
{


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_setbreak(&mip->ourprog,newval) ;
}
/* end subroutine (lsim_setbreak) */


/* is the given address a pseudo SYSCALL? */
int lsim_issyscall(mip,addr,npp)
LSIM		*mip ;
uint		addr ;
const char	**npp ;
{
	LSIM_SYMBOL	*sep ;

	HDB_DATUM	key, value ;

	int	rs ;


#if	CF_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (! mip->f.syscalls)
	    return SR_NOTSUP ;

	key.buf = &addr ;
	key.len = sizeof(uint) ;

	rs = hdb_fetch(&mip->syscalls,key,NULL,&value) ;

	if ((rs < 0) && (rs != SR_NOTFOUND))
		return rs ;

	if (npp != NULL) {

		sep = (LSIM_SYMBOL *) value.buf ;
		*npp = (rs >= 0) ? sep->name : NULL ;

	} /* end if (got one) */

	return ((rs >= 0) ? TRUE : FALSE) ;
}
/* end subroutine (lsim_issyscall) */


/* get and return the value of a symbol in the simulated program */

/****

 NOTE:

	This will return the first symbol with either WEAK or STRONG
	binding that is also either a function or an object!  If you
	want finer discrimination, code it up yourself using a more
	primitive interface!  

****/

int lsim_getsymval(mip,name,rp)
LSIM	*mip ;
char	name[] ;
uint	*rp ;
{
	LMAPPROG_SNCURSOR	cur ;

	Elf32_Sym		*eep ;

	struct proginfo		*pip ;

	int	rs ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0)) {

	    debugprintf("lsim_getsymval: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (name == NULL)
	    return SR_FAULT ;

	pip = mip->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_getsymval: entered name=%s\n",name) ;
#endif

	rs = lmapprog_sncurbegin(&mip->ourprog,&cur) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lsim_getsymval: lmapprog_sncurbegin() rs=%d\n",
		rs) ;
#endif /* CF_DEBUG */

	if (rs < 0)
	    goto bad0 ;

	while ((rs = lmapprog_fetchsym(&mip->ourprog,name,&cur,&eep)) >= 0) {

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_getsymval: got one\n") ;
#endif

	    if (((ELF32_ST_BIND(eep->st_info) == STB_GLOBAL) ||
	        (ELF32_ST_BIND(eep->st_info) == STB_WEAK)) &&
	        ((ELF32_ST_TYPE(eep->st_info) == STT_FUNC) ||
	        (ELF32_ST_TYPE(eep->st_info) == STT_OBJECT)))
	        break ;

	} /* end while (looping through all symbols that match) */

	lmapprog_sncurend(&mip->ourprog,&cur) ;
	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {

	        if (rs < 0)
	            debugprintf("lsim_getsymval: lmapprog_fetchsym() rs=%d\n",
			rs) ;

	    }
#endif /* CF_DEBUG */

	    return rs ;
	}

	if (rp != NULL)
	    *rp = (uint) eep->st_value ;

bad0:
	return rs ;
}
/* end subroutine (lsim_getsymval) */


/* cursor operations for symbol fetching */
int lsim_sncurbegin(mip,cp)
LSIM		*mip ;
LSIM_SNCURSOR	*cp ;
{
	struct proginfo	*pip ;

	int	rs ;


	if (mip == NULL)
		return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0)) {

		debugprintf("lsim_sncurbegin: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (cp == NULL)
		return SR_FAULT ;

	pip = mip->pip ;

	rs = lmapprog_sncurbegin(&mip->ourprog,&cp->sncur) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	    if (rs < 0)
		debugprintf("lsim_sncurbegin: "
			"lmapprog_sncurbegin() rs=%d\n",
			rs) ;
	}
#endif /* F_SAFE */

	return rs ;
}
/* end subroutine (lsim_sncurbegin) */


int lsim_sncurend(mip,cp)
LSIM		*mip ;
LSIM_SNCURSOR	*cp ;
{
	struct proginfo	*pip ;

	int	rs ;


	if (mip == NULL)
		return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0)) {

		debugprintf("lsim_sncurend: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (cp == NULL)
		return SR_FAULT ;

	pip = mip->pip ;

	rs = lmapprog_sncurend(&mip->ourprog,&cp->sncur) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {

	    if (rs < 0)
		debugprintf("lsim_sncurend: "
			"lmapprog_sncurend() rs=%d\n",
			rs) ;

	}
#endif /* F_SAFE */

	return rs ;
}
/* end subroutine (lsim_sncurend) */


/* fetch symbols by name */
int lsim_fetchsym(mip,name,cp,rpp)
LSIM		*mip ;
char		name[] ;
LSIM_SNCURSOR	*cp ;
Elf32_Sym	**rpp ;
{

	struct proginfo	*pip ;

	int	rs ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0)) {

	    debugprintf("lsim_fetchsyml: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != LSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (name == NULL)
	    return SR_FAULT ;

	pip = mip->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_fetchsym: entered name=%s\n",name) ;
#endif


	rs = lmapprog_fetchsym(&mip->ourprog,name,&cp->sncur,rpp) ;

#if	CF_DEBUG && 0
	    if (pip->debuglevel > 2)
	        debugprintf("lsim_fetchsym: lmapprog_fetchsym() rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (lsim_fetchsym) */


/* private subroutines */


#if	CF_DEBUG || CF_DEBUGS

static int lsim_prpq(mip,ckey)
LSIM	*mip ;
ULONG	ckey ;
{
	LSIM_COE	*ep ;

	HDB_DATUM	key, value ;

	HDB_CUR	cur ;

	int	rs ;
	int	n = 0 ;


	key.buf = &ckey ;
	key.len = sizeof(ULONG) ;

	debugprintf("lsim_prpq: pending Q for ckey=%llu\n",ckey) ;

	hdb_curbegin(&mip->pq,&cur) ;

	while ((rs = hdb_fetch(&mip->pq,key,&cur,&value)) >= 0) {

	    ep = (LSIM_COE *) value.buf ;

	    debugprintf("lsim_prpq: ep=%08lx wake=%llu\n",ep,ep->wake) ;

	    n += 1 ;
	}

	hdb_curend(&mip->pq,&cur) ;

	return n ;
}
/* end subroutine (lsim_prpq) */

#endif /* CF_DEBUG */


/* find any SYSCALLs and index them by their function address */
static int lsim_syscalls(mip)
LSIM	*mip ;
{
	struct proginfo	*pip ;

	LSIM_SYMBOL		*sep ;

	LMAPPROG_SNCURSOR	cur ;

	HDB_DATUM	key, value ;

	LMAPPROG	*mpp ;

	Elf32_Sym	*eep ;

	int	rs ;
	int	rs1 ;
	int	i ;


	pip = mip->pip ;
	mpp = &mip->ourprog ;

	mip->f.syscalls = FALSE ;
	rs = hdb_start(&mip->syscalls,50,NULL,NULL) ;
	if (rs < 0)
	    goto ret0 ;

	for (i = 0 ; syscalls[i] != NULL ; i += 1) {

	    lmapprog_sncurbegin(&mip->ourprog,&cur) ;

	    while (TRUE) {

	        rs1 = lmapprog_fetchsym(mpp,syscalls[i],&cur,&eep) ;
	        if (rs1 < 0)
	            break ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 6) {
	            debugprintf("lsim_syscalls: fetchsym() rs=%d\n",rs) ;
	            debugprintf("lsim_syscalls: BIND=%d TYPE=%d\n",
	                ELF32_ST_BIND(eep->st_info),
	                ELF32_ST_TYPE(eep->st_info)) ;
	        }
#endif /* CF_DEBUG */

	            if (((ELF32_ST_BIND(eep->st_info) == STB_GLOBAL) ||
	            (ELF32_ST_BIND(eep->st_info) == STB_WEAK)) &&
	            (ELF32_ST_TYPE(eep->st_info) == STT_FUNC)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 6)
	                debugprintf("lsim_syscalls: sym=%s value=%08x\n",
	                    syscalls[i],eep->st_value) ;
#endif

		    rs = uc_malloc(sizeof(LSIM_SYMBOL),&sep) ;
		    if (rs < 0)
			break ;

		    sep->ep = eep ;
		    sep->name = syscalls[i] ;

	            key.buf = &eep->st_value ;
	            key.len = sizeof(Elf32_Addr) ;

	            value.buf = sep ;
	            value.len = sizeof(LSIM_SYMBOL) ;

	            rs = hdb_store(&mip->syscalls,key,value) ;
		    if (rs < 0)
			uc_free(sep) ;

	            break ;

	        } /* end if (we got one) */

	    } /* end while (looping through candidate symbols) */

	    lmapprog_sncurend(&mip->ourprog,&cur) ;

	    if (rs < 0)
	        break ;

	} /* end for (looping through possible SYSCALLs) */

	if (rs < 0)
	    goto bad1 ;

	mip->f.syscalls = TRUE ;	/* mark DB as initialized */

ret0:
	return rs ;

/* bad things come here */
bad1:
	lsim_syscallsfree(mip) ;

bad0:
	goto ret0 ;
}
/* end subroutine (lsim_syscalls) */


/* free up the data used to hold SYSCALL information */
static int lsim_syscallsfree(mip)
LSIM	*mip ;
{
	HDB_CUR		cur ;
	HDB_DATUM	key, value ;
	LSIM_SYMBOL	*sep ;
	int		rs ;

	    hdb_curbegin(&mip->syscalls,&cur) ;

	    while (hdb_enum(&mip->syscalls,&cur,&key,&value) >= 0) {

	        sep = (LSIM_SYMBOL *) value.buf ;

		if (sep != NULL) {
		    uc_free(sep) ;
		}

	    } /* end while */

	    hdb_curend(&mip->syscalls,&cur) ;

	rs = hdb_finish(&mip->syscalls) ;

	mip->f.syscalls = FALSE ;
	return rs ;
}
/* end subroutine (lsim_syscallsfree) */


#if	CF_DEBUG || CF_DEBUGS

/* find a symbol that might indicate data memory */

struct lsim_symrank {
	int	type, bind, rank ;
} ;

static struct lsim_symrank	ranks[] = {
	{ STT_SECTION, STB_GLOBAL, 10 },
	{ STT_SECTION, STB_LOCAL, 9 },
	{ STT_SECTION, -1, 8 },
	{ STT_OBJECT, STB_GLOBAL, 7 },
	{ STT_OBJECT, STB_WEAK, 6 },
	{ STT_OBJECT, STB_LOCAL, 5 },
	{ STT_OBJECT, -1, 4 },
	{ -1, -1, 0 },
	{ -1, -1, -1 },
} ;

static int lsim_finddatamem(mip,name,rp)
LSIM	*mip ;
char	name[] ;
uint	*rp ;
{
	LMAPPROG_SNCURSOR	cur ;
	Elf32_Sym	*best, *sep ;
	struct proginfo	*pip ;
	int		rankbest = -1 ;
	int		rankcur ;
	int		rs ;

	if (mip == NULL) return SR_FAULT ;

#if	CF_SAFE
	if ((mip->magic != LSIM_MAGIC) && (mip->magic != 0)) {
	    debugprintf("lsim_finddatamem: bad format\n") ;
	    return SR_BADFMT ;
	}
	if (mip->magic != LSIM_MAGIC) return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (name == NULL) return SR_FAULT ;

	pip = mip->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lsim_finddatamem: entered name=%s\n",name) ;
#endif

	rs = lmapprog_sncurbegin(&mip->ourprog,&cur) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {

	    if (rs < 0)
	        debugprintf("lsim_finddatamem: lmapprog_sncurbegin() rs=%d\n",
			rs) ;

	}
#endif /* CF_DEBUG */

	if (rs < 0)
	    goto bad0 ;

	while ((rs = lmapprog_fetchsym(&mip->ourprog,name,&cur,&sep)) >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	        debugprintf("lsim_finddatamem: got one, val=%08x\n",
			sep->st_value) ;
#endif

		rankcur = datasymrank(sep) ;

		if (rankcur > rankbest) {

			best = sep ;
			rankbest = rankcur ;
		}

	} /* end while */

	lmapprog_sncurend(&mip->ourprog,&cur) ;

	if ((rs < 0) && (rs != SR_NOTFOUND)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {

	        if (rs < 0)
	            debugprintf("lsim_finddatamem: lmapprog_fetchsym() rs=%d\n",
			rs) ;

	    }
#endif /* CF_DEBUG */

	    return rs ;
	}

	if (rp != NULL)
	    *rp = (rankbest >= 0) ? ((uint) best->st_value) : 0 ;

	return (rankbest < 0) ? SR_NOTFOUND : rankbest ;

bad0:
	return rs ;
}
/* end subroutine (lsim_finddatamem) */


/* find the symbol's ranking suitable for data memory */
static int datasymrank(sep)
Elf32_Sym	*sep ;
{
	int	i ;


	for (i = 0 ; ranks[i].rank >= 0 ; i += 1) {

		if ((ranks[i].type >= 0) &&
			(ELF32_ST_TYPE(sep->st_info) != ranks[i].type))
			continue ;

		if ((ranks[i].bind >= 0) &&
			(ELF32_ST_BIND(sep->st_info) != ranks[i].bind))
			continue ;

		break ;

	} /* end for */

	return (ranks[i].rank < 0) ? 0 : ranks[i].rank ;
}
/* end subroutine (datasymrank) */

#endif /* CF_DEBUG || CF_DEBUGS */


