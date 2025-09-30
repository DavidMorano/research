/* sssim */

/* Levo simulator state sequencing operations */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		0		/* switchable debugging */
#define	F_SAFE		1		/* play it safe ? */
#define	F_MACHINE	1		/* call the LEVO machine methods */
#define	F_MEMSET	0		/* zero out object memory */
#define	F_MAXCLOCKS	1		/* maximum numbers of clocks */
#define	F_TESTMAP	0		/* test program mapping */
#define	F_TESTMAPSHORT	0		/* short test program mapping */


/* revision history:

	= 00/02/15, Dave Morano

	This code was started (for LevoSim).


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This is the object (roughly) for handling simulator specific
	operations such as call-backs.


******************************************************************************/


#define	SSSIM_MASTER	1


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>
#include	<elf.h>

#include	<usystem.h>
#include	<veclist.h>
#include	<vecitem.h>
#include	<hdb.h>
#include	<paramfile.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#ifdef	COMMENT
#include	"lmapprog.h"
#endif

#include	"plainq.h"
#include	"sssim.h"		/* ourselves */



/* local defines */

#define	SSSIM_MAGIC		0x95827364
#define	SSSIM_CLOCKINTERVAL	200



/* external subroutines */


/* local structures */


/* forward references */

int		sssim_callthem(SSSIM *) ;

#ifdef	COMMENT
int		sssim_getsymval(SSSIM *,char *,uint *) ;
#endif /* COMMENT */

#ifdef	COMMENT
static int	sssim_syscalls(SSSIM *) ;
static int	sssim_syscallsfree(SSSIM *) ;
#endif /* COMMENT */

#if	F_DEBUG || F_DEBUGS
static int	sssim_prpq(SSSIM *,ULONG) ;
static int	sssim_finddatamem(SSSIM *,char *,uint *) ;
#endif

#if	defined(COMMENT) && (F_DEBUG || F_DEBUGS)
static int	datasymrank(Elf32_Sym *) ;
#endif


/* local variables */

static const char	*lparams[] = {
	"sim:maxclocks",	/* maximum number of clocks */
	"sim:fastqueues",	/* number of fast queues */
	NULL
} ;

enum lparams {
	lparam_maxclocks,
	lparam_fastqueues,
	lparam_overlast
} ;

static const char	*syscalls[] = {
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






int sssim_init(mip,pip,pfp,maxclocks,simprog,machobj)
SSSIM		*mip ;
struct proginfo	*pip ;
PARAMFILE	*pfp ;
LONG		maxclocks ;
SSSIM_SIMPROG	*simprog ;
SSSIM_MACHOBJ	*machobj ;
{
	ULONG	ulw ;

	int	rs = SR_OK ;
	int	i, j ;
	int	iw, sl ;
	int	size ;

	char	*cp ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4))
	    eprintf("sssim_init: entered\n") ;
#endif

	(void) memset(mip,0,sizeof(SSSIM)) ;

	mip->magic = 0 ;
	mip->pip = pip ;
	mip->nentries = 0 ;
	mip->nq = SSSIM_NQUEUES ;
	mip->cqi = 0 ;
	mip->ck = 0 ;
	mip->ck_count = 0 ;

	mip->f.syscalls = FALSE ;


/* store what we have so far */

/* the program information */


/* the machine object information */

	mip->topobjp = NULL ;
	mip->topcombp = NULL ;
	mip->topclockp = NULL ;
	if (machobj != NULL) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_init: got a machobj, LEVO=%08lx\n",
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

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	            eprintf("sssim_init: %i lparam=%s \n",i,lparams[i]) ;
	            eprintf("sssim_init: %i val=%s\n",i,vbuf) ;
	        }
#endif

	        switch (i) {

	        case lparam_maxclocks:
	            rs = cfnumull(vbuf,vl,&ulw) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	                eprintf("sssim_init: rs=%d maxclocks=%llu\n",rs,ulw) ;
#endif

	            if (rs >= 0) {

	                switch (i) {

	                case lparam_maxclocks:
				if (maxclocks == 0)
	                    		maxclocks = ulw ;

	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        case lparam_fastqueues:
	            rs = cfnumui(vbuf,vl,&iw) ;

	            if (rs >= 0) {

	                switch (i) {

	                case lparam_fastqueues:
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
	    maxclocks = SSSIM_MAXCLOCKS ;

	mip->maxclocks = maxclocks ;

	if (mip->nq < 5)
	    mip->nq = 5 ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	    eprintf("sssim_init: maxclock=%llu\n",mip->maxclocks) ;
	    eprintf("sssim_init: fastqueues=%u\n",mip->nq) ;
	}
#endif /* F_DEBUG */


/* start initializing our scheduling apparatus */

	size = mip->nq * sizeof(veclist) ;
	rs = uc_malloc(size,&mip->qes) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(2) && (rs < 0))
	    eprintf("sssim_init: Qs malloc failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(mip->qes,size,"sssim_init:calloutqueues") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    iw = i ;
	    rs = veclist_init(mip->qes + i,0,VECLIST_PSWAP) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("sssim_init: Qs veclist(%d) failed rs=%d\n",i,rs) ;
#endif

	    if (rs < 0)
	        goto bad1 ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4))
	    eprintf("sssim_init: queue list(%d)=%08lx\n",(mip->qes + i)) ;
#endif

	} /* end for */

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4)) {
	    eprintf("sssim_init: ck=%llu \n",mip->ck) ;
	    eprintf("sssim_init: 1 c0=%08x c1=%08x\n",
	        getarrayint(&mip->ck,0),
	        getarrayint(&mip->ck,1)) ;
	}
#endif /* F_DEBUG */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_init: plainq_init() FQ=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_init(&mip->fq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("sssim_init: plainq failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_printf("plainq_init") ;
#endif

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4)) {
	    eprintf("sssim_init: ck=%llu \n",mip->ck) ;
	    eprintf("sssim_init: 2 c0=%08x c1=%08x\n",
	        getarrayint(&mip->ck,0),
	        getarrayint(&mip->ck,1)) ;
	}
#endif /* F_DEBUG */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_init: hdb_init()\n") ;
#endif

	rs = hdb_init(&mip->pq,mip->nq,NULL) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("sssim_init: PQ failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad3 ;

#ifdef	MALLOCLOG
	malloclog_printf("hdb_init") ;
#endif

/* map the program that we want to simulate */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_init: progfile=%s\n",simprog->program) ;
#endif

	rs = lmapprog_init(&mip->ourprog,pip,
	    simprog->program,simprog->argv,simprog->envv) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("sssim_init: lmapprog_init() failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad4 ;

#ifdef	MALLOCLOG
	malloclog_printf("lmapprog_init") ;
#endif


/* supposedly, we are fairly well done ! (need to index SYSCALLs later) */

	mip->magic = SSSIM_MAGIC ;


/* if so, do some little tests ! */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	    uint		iaddr, ma ;
	    uint		val ;

	    eprintf("sssim_init: start of simulated program i-memory\n") ;
	    lmapprog_getentry(&mip->ourprog,&iaddr) ;
	    for (i = 0 ; i < 5 ; i += 1) {
	        rs = lmapprog_readint(&mip->ourprog,iaddr,&val) ;
	        eprintf("sssim_init: rs=%d ia=%08x %08x\n",rs,iaddr,val) ;
	        iaddr += 4 ;
	    }

/* read it as instructions */

	    eprintf("sssim_init: reading i-memory as instructions\n") ;
	    lmapprog_getentry(&mip->ourprog,&iaddr) ;
	    for (i = 0 ; i < 5 ; i += 1) {
	        rs = lmapprog_readinstr(&mip->ourprog,iaddr,&val) ;
	        eprintf("sssim_init: rs=%d ia=%08x %08x\n",rs,iaddr,val) ;
	        iaddr += 4 ;
	    }

/* find the start of some data memory */

	    rs = sssim_finddatamem(mip,"fdata",&iaddr) ;

	    eprintf("sssim_init: find data 'fdata' rs=%d\n",rs) ;

		if (rs < 0) {

	    rs = sssim_finddatamem(mip,"_fdata",&iaddr) ;

	    eprintf("sssim_init: find data '_fdata' rs=%d\n",rs) ;

		}

		if (rs < 0) {

	    rs = sssim_finddatamem(mip,".rodata",&iaddr) ;

	    eprintf("sssim_init: find data '.rodata' rs=%d\n",rs) ;

		}

		if (rs < 0) {

	    rs = sssim_finddatamem(mip,".sdata",&iaddr) ;

	    eprintf("sssim_init: find data '.sdata' rs=%d\n",rs) ;

		}

		if (rs >= 0) {

	    eprintf("sssim_init: start of simulated program d-memory\n") ;
	    for (i = 0 ; i < 10 ; i += 1) {
	        rs = lmapprog_readint(&mip->ourprog,iaddr,&val) ;
	        eprintf("sssim_init: rs=%d da=%08x %08x\n",rs,iaddr,val) ;
	        iaddr += 4 ;
	    }

		} /* end if (d-memory) */

/* OK, do some free-lance testing ! */

	        eprintf("sssim_init: free-lance testing\n") ;
	ma = 0x10009500 ;
	for (i = 0 ; i < 72 ; i += 1) {

	        rs = lmapprog_readint(&mip->ourprog,ma,&val) ;

		if (rs < 0)
			break ;

	        eprintf("sssim_init: a=%08x %08x\n",ma,val) ;

		ma += 4 ;

	} /* end for */

	        eprintf("sssim_init: free-lance result i=%d rs=%d\n",i,rs) ;

	}
#endif /* F_DEBUG */


/* test the program mapping */

#if	F_TESTMAP
	if (DEBUGLEVEL(1)) {

	    LMAPPROG		*mpp ;

	    LMAPPROG_SEGMENT	*psp ;

	    uint	maddr ;
	    uint	val1, val2 ;
		uint	max ;

	    int	rs1, rs2 ;
	    int	f_failed = FALSE ;


	    eprintf("sssim_init: simulated memory compare - ") ;
	    mpp = &mip->ourprog ;
	    for (i = 0 ; vecitem_get(&mpp->segments,i,&psp) >= 0 ; i += 1) {

	        maddr = psp->vaddr ;
#if	F_TESTMAPSHORT
		max = MIN((psp->vaddr + psp->vlen),0x10) ;
#else
		max = psp->vaddr + psp->vlen ;
#endif
	        while (maddr < max) {

	            rs1 = lmapprog_readint(mpp,maddr,&val1) ;

	            rs2 = lmapprog_readtest(mpp,maddr,&val2) ;

	            if ((rs1 != rs2) || (val1 != val2)) {

	                f_failed = TRUE ;
	                eprintf("sssim_init: testmap error at addr=%08x\n",
	                    maddr) ;

	            } /* end if */

	            maddr += 4 ;

	        } /* end while */

	    } /* end for (outer) */

	    eprintf("%s\n",
	        (f_failed) ? "failed" : "passed") ;

/* get a symbol value */

	    rs = sssim_getsymval(mip,"main",&maddr) ;

	    eprintf("sssim_init: sssim_getsymval() rs=%d main=%08x\n",
	        rs,maddr) ;

	} /* end block */
#endif /* F_TESTMAP */

/* if we have any symbols, find any SYSCALLs and index them by address */

	rs = sssim_syscalls(mip) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("sssim_init: sssim_syscalls() failed rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad5 ;


/* finish up */

#if	F_DEBUG && 0
	if (DEBUGLEVEL(2)) {
	    eprintf("sssim_init: ck=%llu \n",mip->ck) ;
	    eprintf("sssim_init: 3 c0=%08x c1=%08x\n",
	        getarrayint(&mip->ck,0),
	        getarrayint(&mip->ck,1)) ;
	    eprintf("sssim_init: exiting OK rs=%d\n",rs) ;
	}
#endif /* F_DEBUG */

	return rs ;

/* bad things */
bad5:
	(void) lmapprog_free(&mip->ourprog) ;

bad4:
	(void) hdb_free(&mip->pq) ;

bad3:
	(void) plainq_free(&mip->fq) ;

bad2:
bad1:
	for (j = 0 ; j < i ; j += 1)
	    (void) veclist_free(mip->qes + j) ;

	free(mip->qes) ;

bad0:

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("sssim_init: ret BAD rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sssim_init) */


/* set the machine object information */
int sssim_setmach(mip,machobj)
SSSIM		*mip ;
SSSIM_MACHOBJ	*machobj ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4))
	    eprintf("sssim_setmach: entered\n") ;
#endif


	if (machobj->topobjp == NULL)
	    return SR_FAULT ;

	if (machobj->topcombp == NULL)
	    return SR_FAULT ;

	if (machobj->topclockp == NULL)
	    return SR_FAULT ;

/* the machine object information */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_setmach: got a machobj, LEVO=%08lx\n",
	        machobj->topobjp) ;
#endif

	mip->topobjp = machobj->topobjp ;
	mip->topcombp = machobj->topcombp ;
	mip->topclockp = machobj->topclockp ;


	return SR_OK ;
}
/* end subroutine (sssim_setmach) */


#ifdef	COMMENT

/* get the mapped program object pointer */
int sssim_getpp(mip,mpp)
SSSIM		*mip ;
LMAPPROG	**mpp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4))
	    eprintf("sssim_getpp: entered\n") ;
#endif

	if (mpp == NULL)
	    return SR_FAULT ;

	*mpp = &mip->ourprog ;

	return SR_OK ;
}
/* end subroutine (sssim_getpp) */

#endif /* COMMENT */


/* free up whatever ! */
int sssim_free(mip)
SSSIM	*mip ;
{
	struct proginfo	*pip ;

	SSSIM_COE	*ep ;

	veclist		*cqp ;

	int	rs, i, j ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_free: entered\n") ;
#endif

/* free up SYSCALLs data structure */

	if (mip->f.syscalls)
		(void) sssim_syscallsfree(mip) ;

/* free up the program mapping */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_free: LMAPPROG\n") ;
#endif

	lmapprog_free(&mip->ourprog) ;


/* free up any and all entries that are out there ! */

/* free all entries on the free queue */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_free: free Q entries\n") ;
#endif

	while (plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) != PLAINQ_RUNDER)
	    free(ep) ;


/* free all entries on the future pending queue */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_free: pending Q entries\n") ;
#endif

	{
	    HDB_DATUM	key, value ;

	    HDB_CURSOR	hcur ;


	    hdb_cursorinit(&mip->pq,&hcur) ;

	    while (hdb_enum(&mip->pq,&hcur,&key,&value) >= 0) {

	        if (value.buf != NULL)
	            free(value.buf) ;	/* the key was in there also ! */

	    } /* end while */

	    hdb_cursorfree(&mip->pq,&hcur) ;

	} /* end block (freeing pending queue) */


/* free all entries on the individual (fast) queues */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_free: fast Q entries\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    cqp = mip->qes + i ;
	    for (j = 0 ; (rs = veclist_get(cqp,j,&ep)) >= 0 ; j += 1) {

	        if (ep == NULL) continue ;

	        free(ep) ;

	    } /* end while */

	} /* end for */


/* free up all queue objects */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_free: fast Qs\n") ;
#endif

	for (i = 0 ; i < mip->nq ; i += 1) {

	    (void) veclist_free(mip->qes + i) ;

	} /* end for */

	(void) plainq_free(&mip->fq) ;

	(void) hdb_free(&mip->pq) ;

	free(mip->qes) ;

	mip->qes = NULL ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_free: exiting\n") ;
#endif

	mip->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (sssim_free) */


/* take call-back requests from other objects */
int sssim_callout(mip,func,objp,argp,nclocks,phase)
SSSIM	*mip ;
int	(*func)() ;
void	*objp, *argp ;
uint	nclocks ;
int	phase ;
{
	struct proginfo	*pip ;

	SSSIM_COE	*ep ;

	int	rs = SR_OK ;
	int	i ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	    eprintf("sssim_callout: entered func=%08lx objp=%08lx argp=%08lx\n",
	        func,objp,argp) ;
	    eprintf("sssim_callout: ck=%llu nclocks=%u\n",
	        mip->ck,nclocks) ;
	}
#endif /* F_DEBUG */

	if (nclocks == 0)
	    return SR_INVALID ;

/* get a free entry */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callout: plainq_rem() FQa=%08lx\n",
	        &mip->fq) ;
#endif

	rs = plainq_rem(&mip->fq,(PLAINQ_ENTRY **) &ep) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callout: plainq_rem() rs=%d\n",rs) ;
#endif

	if (rs == PLAINQ_RUNDER) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_callout: plainq_rem() underflow\n") ;
#endif

	    rs = uc_malloc(sizeof(SSSIM_COE),(void *) &ep) ;

	    if (rs < 0)
	        return rs ;

#ifdef	MALLOCLOG
	malloclog_alloc(ep,sizeof(SSSIM_COE),"sssim_callout:coe") ;
#endif

	    mip->nentries += 1 ;

	} /* end if */

/* fill it in */

	ep->wake = mip->ck + nclocks ;
	ep->relative = nclocks ;
	ep->phase = phase ;
	ep->func = func ;
	ep->objp = objp ;
	ep->argp = argp ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callout: wake=%llu\n",ep->wake) ;
#endif

	if (nclocks < mip->nq) {

/* short time in the future */

	    i = (mip->cqi + nclocks) % mip->nq ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_callout: short=%u wake=%llu\n",i,ep->wake) ;
#endif

	    rs = veclist_add(mip->qes + i,ep) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_callout: veclist_add rs=%d\n",rs) ;
#endif

	} else {

	    HDB_DATUM	key, value ;


/* handle requests for longer time into the future */

	    ep->key = ep->wake / ((ULONG) mip->nq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_callout: long future, key=%llu\n",
	            ep->key) ;
#endif

	    key.buf = (char *) &ep->key ;
	    key.len = sizeof(ULONG) ;

	    value.buf = ep ;
	    value.len = sizeof(SSSIM_COE) ;

	    rs = hdb_store(&mip->pq,key,value) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_callout: hdb_store rs=%d\n",rs) ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

	        rs = sssim_prpq(mip,ep->key) ;

	        eprintf("sssim_callout: pending on key=%llu n=%d\n",
	            ep->key,rs) ;
	    }
#endif /* F_DEBUG */

	} /* end if */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callout: ret rs=%d\n",
	        rs) ;
#endif

	return rs ;
}
/* end subroutine (sssim_callout) */


/* this is main loop (we ditched MINT since nobody wanted to call it) */
int sssim_loop(mip)
SSSIM	*mip ;
{
	struct proginfo	*pip ;

	ULONG	clocklast = 0 ;

	int	rs ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;
	if (mip->topobjp == NULL)
	    return SR_FAULT ;

	if (mip->topcombp == NULL)
	    return SR_FAULT ;

	if (mip->topclockp == NULL)
	    return SR_FAULT ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_loop: LEVO a=%08lx\n",
	        mip->topobjp) ;
#endif

/* we're good, let's go */

	mip->ck = 0 ;

	while (TRUE) {

/* call the stuff for combinatorial processing */

	    rs = sssim_callthem(mip) ;

	    if (rs != 0) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_loop: exiting from combinatorial rs=%d\n",
	        rs) ;
#endif

	        break ;

	    } /* end if (combinatorial exit) */


/* should we do any maintenance activities ? */

	if ((mip->ck - clocklast) > SSSIM_CLOCKINTERVAL) {

		clocklast = mip->ck ;
		proginfo_check(pip,0,mip->ck,mip->in) ;

	} /* end if (maintenance activities) */


/* call the stuff to transition the clock */

#if	F_MACHINE
	    rs = (*mip->topclockp)(mip->topobjp) ;

	    if (rs != 0) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_loop: exiting from clock transition rs=%d\n",
	        rs) ;
#endif

	        break ;
	}
#endif /* F_MACHINE */

	    mip->ck += 1 ;

#if	F_MAXCLOCKS
	    if ((mip->maxclocks > 0) && (mip->ck >= mip->maxclocks)) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_loop: exiting from maxclocks\n") ;
#endif

	        break ;
		}
#endif /* F_MAXCLOCKS */

	} /* end while */

	proginfo_check(pip,1,mip->ck,mip->in) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_loop: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sssim_loop) */


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

int sssim_callthem(mip)
SSSIM	*mip ;
{
	struct proginfo	*pip ;

	veclist	*cqp ;

	SSSIM_COE	*ep ;

	int	phase ;
	int	lrs, rs, i, j ;
	int	n = 0 ;
	int	f_exit ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callthem: entering ck=%llu comb is LEVO=%08lx\n",
	        mip->ck,mip->topobjp) ;
#endif

/* loop through all of the phases */

	lrs = SR_OK ;
	phase = 0 ;
	f_exit = FALSE ;
	while ((! f_exit) || (phase < SSSIM_MINPHASES)) {

		mip->phase = phase ;

#if	F_MACHINE
	    lrs = (*mip->topcombp)(mip->topobjp,phase) ;

	    if (lrs != 0)
	        f_exit = TRUE ;
#else
	    f_exit = TRUE ;
#endif /* F_MACHINE */

/* go through the current queue and call all the ones with the current phase */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	        eprintf("sssim_callthem: finding callbacks ph=%d\n",
	            phase) ;
	        eprintf("sssim_callthem: count Q index=%d\n",mip->cqi) ;
	    }
#endif

	    f_exit = TRUE ;
	    cqp = mip->qes + mip->cqi ;
	    for (j = 0 ; (rs = veclist_get(cqp,j,&ep)) >= 0 ; j += 1) {

	        if (ep == NULL) continue ;

	        f_exit = FALSE ;
	        if (ep->phase == phase) {

	            n += 1 ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	                eprintf("sssim_callthem: calling callback\n") ;
	                eprintf("sssim_callthem: callback=%08lx objp=%08lx\n",
	                    ep->func,ep->objp) ;
	            }
#endif

/* call him !! */

#if	F_MACHINE
	            lrs = (*ep->func)(ep->objp,ep->argp,phase) ;

	            if (lrs != 0)
	                f_exit = TRUE ;
#endif /* F_MACHINE */

/* delete him */

	            (void) veclist_del(cqp,j) ;

	            j -= 1 ;	/* compensate for loop counter */

/* but save the memory data structure for quick access later */

	            (void) plainq_ins(&mip->fq,(PLAINQ_ENTRY *) ep) ;

	        } /* end if (got a live one !) */

	        if (f_exit)
	            break ;

	    } /* end for */

	    phase += 1 ;
	    if (lrs != 0)
	        break ;

	} /* end while (looping through clock phases) */

/* the fast queue for this clock period should be empty now ! */

	mip->cqi = (mip->cqi + 1) % mip->nq ;

/* if advanced (mip->nq) clocks, then bring forward pending requests */

	mip->ck_count += 1 ;
	if (mip->ck_count == mip->nq) {

	    HDB_DATUM	key, value ;

	    HDB_CURSOR	cur ;

	    ULONG	clockkey ;

	    uint	ci ;

	    int		f_next ;


	    clockkey = (mip->ck + 1) / ((ULONG) mip->nq) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_callthem: bring forward, ck=%llu key=%llu\n",
	            mip->ck,clockkey) ;
#endif

	    key.buf = (char *) &clockkey ;
	    key.len = sizeof(LONG) ;

/* find all entries with the same key */

	    hdb_cursorinit(&mip->pq,&cur) ;

	    f_next = TRUE ;
	    while (TRUE) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	            eprintf("sssim_callthem: HDB loop f_next=%d\n",f_next) ;
#endif

	        if (f_next)
	            rs = hdb_fetch(&mip->pq,key,&cur,&value) ;

	        else
	            rs = hdb_getkeyrec(&mip->pq,key,&cur,NULL,&value) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
			if (rs >= 0)
	            eprintf("sssim_callthem: HDB loop rs=%d\n",rs) ;

			else if (rs == -2)
	            eprintf("sssim_callthem: HDB loop rs=NOTFOUND\n") ;

			else
	            eprintf("sssim_callthem: HDB loop rs=**BAD**\n") ;

		}
#endif /* F_DEBUG */

	        if (rs < 0)
	            break ;

	        ep = (SSSIM_COE *) value.buf ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {
	            eprintf("sssim_callthem: HDB loop entry=%08lx\n",
	                ep) ;
	            eprintf("sssim_callthem: HDB loop entry wake=%llu\n",
	                ep->wake) ;
	        }
#endif

	        ci = (uint) (ep->wake - (mip->ck + 1)) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	            eprintf("sssim_callthem: HDB loop ci=%u\n",ci) ;
#endif

	        i = (mip->cqi + ci) % mip->nq ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	            eprintf("sssim_callthem: HDB loop adding i=%d QESa=%08lx\n",
	                i,mip->qes) ;
#endif

	        (void) veclist_add(mip->qes + i,ep) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	            eprintf("sssim_callthem: HDB loop deleting\n") ;
#endif

	        (void) hdb_delcursor(&mip->pq,&cur) ;

	        f_next = FALSE ;	/* deletion, do NOT advance cursor */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	            eprintf("sssim_callthem: HDB loop pending key=%llu\n",
	                clockkey) ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

	            rs = sssim_prpq(mip,clockkey) ;

	            eprintf("sssim_callthem: HDB loop pending remaining=%d\n",
	                rs) ;
	        }
#endif /* F_DEBUG */

	    } /* end while */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_callthem: HDB past loop \n") ;
#endif

	    hdb_cursorfree(&mip->pq,&cur) ;

	    mip->ck_count = 0 ;

	} /* end if */


#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callthem: pending Q ckey=%llu\n",
	        2ULL) ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

	    rs = sssim_prpq(mip,2ULL) ;

	    eprintf("sssim_callthem: pending Q remaining=%d\n",
	        rs) ;
	}
#endif /* F_DEBUG */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callthem: FQa=%08lx QESa=%08lx\n",
	        &mip->fq,mip->qes) ;
#endif


/* we-re out of here */

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_callthem: exiting\n") ;
#endif

	return lrs ;
}
/* end subroutine (sssim_callthem) */


/* return the clock number to the caller */
int sssim_getclock(mip,lp)
SSSIM	*mip ;
ULONG	*lp ;
{


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (lp == NULL)
	    return SR_FAULT ;

	*lp = mip->ck ;
	return SR_OK ;
}
/* end subroutine (sssim_getclock) */


#ifdef	COMMENT

/* read program instruction memory (a single 32-bit integer) */
int sssim_readinstr(mip,vaddr,vp)
SSSIM	*mip ;
uint	vaddr ;
uint	*vp ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_readinstr(&mip->ourprog,vaddr,vp) ;
}
/* end subroutine (sssim_readint) */


/* read program instruction memory (a single 32-bit integer) */
int sssim_readint(mip,vaddr,vp)
SSSIM	*mip ;
uint	vaddr ;
uint	*vp ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_readint(&mip->ourprog,vaddr,vp) ;
}
/* end subroutine (sssim_readinstr) */


/* write an integer (32 bits) to the simulated program memory */
int sssim_writeint(mip,vaddr,data,dp)
SSSIM	*mip ;
uint	vaddr ;
uint	data ;
uint	dp ;
{
	struct proginfo	*pip ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("sssim_writeint: ma=%08x dv=%08x\n",vaddr,data) ;
#endif

	return lmapprog_writeint(&mip->ourprog,vaddr,data,dp) ;
}
/* end subroutine (sssim_writeint) */


/* test for a certain memory access mode */
int sssim_memaccess(mip,vaddr,mode)
SSSIM	*mip ;
uint	vaddr ;
uint	mode ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_memaccess(&mip->ourprog,vaddr,mode) ;
}
/* end subroutine (sssim_memaccess) */


/* get the program starting address */
int sssim_getentry(mip,vp)
SSSIM	*mip ;
uint	*vp ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getentry(&mip->ourprog,vp) ;
}
/* end subroutine (sssim_getentry) */


/* get the program starting address */
int sssim_getstack(mip,vp)
SSSIM	*mip ;
uint	*vp ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getstack(&mip->ourprog,vp) ;
}
/* end subroutine (sssim_getstack) */


/* get the program "break" address */
int sssim_getbreak(mip,vp)
SSSIM	*mip ;
uint	*vp ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getbreak(&mip->ourprog,vp) ;
}
/* end subroutine (sssim_getbreak) */


/* get the target program page size */
int sssim_getpagesize(mip,vp)
SSSIM	*mip ;
uint	*vp ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_getpagesize(&mip->ourprog,vp) ;
}
/* end subroutine (sssim_getpagesize) */


/* get the target program page size */
int sssim_getmax(mip,mp)
SSSIM		*mip ;
SSSIM_MAXPROG	*mp ;
{
	LMAPPROG_MAXPROG	max ;

	int	rs ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	rs = lmapprog_getmax(&mip->ourprog,&max) ;

	if (rs < 0)
		return rs ;

	(void) memset(mp,0,sizeof(SSSIM_MAXPROG)) ;

	mp->stack = max.stack ;
	mp->data = max.data ;
	return SR_OK ;
}
/* end subroutine (sssim_getmax) */


/* set the program "break" address */
int sssim_setbreak(mip,newval)
SSSIM	*mip ;
uint	newval ;
{


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	return lmapprog_setbreak(&mip->ourprog,newval) ;
}
/* end subroutine (sssim_setbreak) */


/* is the given address a pseudo SYSCALL ? */
int sssim_issyscall(mip,addr,npp)
SSSIM		*mip ;
uint		addr ;
const char	**npp ;
{
	SSSIM_SYMBOL	*sep ;

	HDB_DATUM	key, value ;

	int	rs ;


#if	F_SAFE
	if (mip == NULL)
	    return SR_FAULT ;

	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0))
	    return SR_BADFMT ;

	if (mip->magic != SSSIM_MAGIC)
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

		sep = (SSSIM_SYMBOL *) value.buf ;
		*npp = (rs >= 0) ? sep->name : NULL ;

	} /* end if (got one) */

	return ((rs >= 0) ? TRUE : FALSE) ;
}
/* end subroutine (sssim_issyscall) */


/* get and return the value of a symbol in the simulated program */

/* NOTE:
	This will return the first symbol with either WEAK or STRONG
	binding that is also either a function or an object !  If you
	want finer discrimination, code it up yourself using a more
	primitive interface !  
*/

int sssim_getsymval(mip,name,rp)
SSSIM	*mip ;
char	name[] ;
uint	*rp ;
{
	LMAPPROG_SNCURSOR	cur ;

	Elf32_Sym		*eep ;

	struct proginfo		*pip ;

	int	rs ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0)) {

	    eprintf("sssim_getsymval: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (name == NULL)
	    return SR_FAULT ;

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_getsymval: entered name=%s\n",name) ;
#endif

	rs = lmapprog_sncursorinit(&mip->ourprog,&cur) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(2) && (rs < 0)))
	        eprintf("sssim_getsymval: lmapprog_sncursorinit() rs=%d\n",rs) ;
#endif /* F_DEBUG */

	if (rs < 0)
	    goto bad0 ;

	while ((rs = lmapprog_fetchsym(&mip->ourprog,name,&cur,&eep)) >= 0) {

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4))
	        eprintf("sssim_getsymval: got one\n") ;
#endif

	    if (((ELF32_ST_BIND(eep->st_info) == STB_GLOBAL) ||
	        (ELF32_ST_BIND(eep->st_info) == STB_WEAK)) &&
	        ((ELF32_ST_TYPE(eep->st_info) == STT_FUNC) ||
	        (ELF32_ST_TYPE(eep->st_info) == STT_OBJECT)))
	        break ;

	} /* end while (looping through all symbols that match) */

	(void) lmapprog_sncursorfree(&mip->ourprog,&cur) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

	        if (rs < 0)
	            eprintf("sssim_getsymval: lmapprog_fetchsym() rs=%d\n",
			rs) ;

	    }
#endif /* F_DEBUG */

	    return rs ;
	}

	if (rp != NULL)
	    *rp = (uint) eep->st_value ;

bad0:
	return rs ;
}
/* end subroutine (sssim_getsymval) */


/* cursor operations for symbol fetching */
int sssim_sncursorinit(mip,cp)
SSSIM		*mip ;
SSSIM_SNCURSOR	*cp ;
{
	struct proginfo	*pip ;

	int	rs ;


	if (mip == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0)) {

		eprintf("sssim_sncursorinit: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (cp == NULL)
		return SR_FAULT ;

	pip = mip->pip ;

	rs = lmapprog_sncursorinit(&mip->ourprog,&cp->sncur) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

		if (rs < 0)
		eprintf("sssim_sncursorinit: lmapprog_sncursorinit() rs=%d\n",
			rs) ;

	}
#endif /* F_SAFE */

	return rs ;
}
/* end subroutine (sssim_sncursorinit) */


int sssim_sncursorfree(mip,cp)
SSSIM		*mip ;
SSSIM_SNCURSOR	*cp ;
{
	struct proginfo	*pip ;

	int	rs ;


	if (mip == NULL)
		return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0)) {

		eprintf("sssim_sncursorfree: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (cp == NULL)
		return SR_FAULT ;

	pip = mip->pip ;

	rs = lmapprog_sncursorfree(&mip->ourprog,&cp->sncur) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

		if (rs < 0)
		eprintf("sssim_sncursorfree: lmapprog_sncursorfree() rs=%d\n",
			rs) ;

	}
#endif /* F_SAFE */

	return rs ;
}
/* end subroutine (sssim_sncursorfree) */


/* fetch symbols by name */
int sssim_fetchsym(mip,name,cp,rpp)
SSSIM		*mip ;
char		name[] ;
SSSIM_SNCURSOR	*cp ;
Elf32_Sym	**rpp ;
{

	struct proginfo	*pip ;

	int	rs ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0)) {

	    eprintf("sssim_fetchsyml: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (name == NULL)
	    return SR_FAULT ;

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_fetchsym: entered name=%s\n",name) ;
#endif


	rs = lmapprog_fetchsym(&mip->ourprog,name,&cp->sncur,rpp) ;

#if	F_DEBUG && 0
	if (DEBUGLEVEL(4))
	        eprintf("sssim_fetchsym: lmapprog_fetchsym() rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sssim_fetchsym) */

#endif /* COMMENT */



/* PRIVATE SUBROUTINES */



#if	F_DEBUG || F_DEBUGS

static int sssim_prpq(mip,ckey)
SSSIM	*mip ;
ULONG	ckey ;
{
	SSSIM_COE	*ep ;

	HDB_DATUM	key, value ;

	HDB_CURSOR	cur ;

	int	rs, n = 0 ;


	key.buf = &ckey ;
	key.len = sizeof(ULONG) ;

	eprintf("sssim_prpq: pending Q for ckey=%llu\n",ckey) ;

	hdb_cursorinit(&mip->pq,&cur) ;

	while ((rs = hdb_fetch(&mip->pq,key,&cur,&value)) >= 0) {

	    ep = (SSSIM_COE *) value.buf ;

	    eprintf("sssim_prpq: ep=%08lx wake=%llu\n",ep,ep->wake) ;

	    n += 1 ;
	}

	hdb_cursorfree(&mip->pq,&cur) ;

	return n ;
}
/* end subroutine (sssim_prpq) */

#endif /* F_DEBUG */


#ifdef	COMMENT

/* find any SYSCALLs and index them by their function address */
static int sssim_syscalls(mip)
SSSIM	*mip ;
{
	struct proginfo	*pip ;

	SSSIM_SYMBOL		*sep ;

	LMAPPROG_SNCURSOR	cur ;

	HDB_DATUM	key, value ;

	LMAPPROG	*mpp ;

	Elf32_Sym	*eep ;

	int	rs, i ;


	pip = mip->pip ;
	mpp = &mip->ourprog ;

	mip->f.syscalls = FALSE ;
	rs = hdb_init(&mip->syscalls,50,NULL) ;

	if (rs < 0)
	    return rs ;

	for (i = 0 ; syscalls[i] != NULL ; i += 1) {

	    lmapprog_sncursorinit(&mip->ourprog,&cur) ;

	    while (TRUE) {

	        rs = lmapprog_fetchsym(mpp,syscalls[i],&cur,&eep) ;

	        if (rs < 0) {

	            rs = SR_OK ;
	            break ;
	        }

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(5)) {
	            eprintf("sssim_syscalls: fetchsym() rs=%d\n",rs) ;
	            eprintf("sssim_syscalls: BIND=%d TYPE=%d\n",
	                ELF32_ST_BIND(eep->st_info),
	                ELF32_ST_TYPE(eep->st_info)) ;
	        }
#endif /* F_DEBUG */

	        if ((rs >= 0) && 
	            ((ELF32_ST_BIND(eep->st_info) == STB_GLOBAL) ||
	            (ELF32_ST_BIND(eep->st_info) == STB_WEAK)) &&
	            (ELF32_ST_TYPE(eep->st_info) == STT_FUNC)) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(6))
	                eprintf("sssim_syscalls: sym=%s value=%08x\n",
	                    syscalls[i],eep->st_value) ;
#endif

		    rs = uc_malloc(sizeof(SSSIM_SYMBOL),&sep) ;

		    if (rs < 0)
			break ;

		    sep->ep = eep ;
		    sep->name = syscalls[i] ;

	            key.buf = &eep->st_value ;
	            key.len = sizeof(Elf32_Addr) ;

	            value.buf = sep ;
	            value.len = sizeof(SSSIM_SYMBOL) ;

	            rs = hdb_store(&mip->syscalls,key,value) ;

		    if (rs < 0)
			free(sep) ;

	            break ;

	        } /* end if (we got one) */

	    } /* end while (looping through candidate symbols) */

	    lmapprog_sncursorfree(&mip->ourprog,&cur) ;

	    if (rs < 0)
	        break ;

	} /* end for (looping through possible SYSCALLs) */

	if (rs < 0)
	    goto bad1 ;

	mip->f.syscalls = TRUE ;	/* mark DB as initialized */
	return SR_OK ;

/* bad things come here */
bad1:
	(void) sssim_syscallsfree(mip) ;

	return rs ;
}
/* end subroutine (sssim_syscalls) */


/* free up the data used to hold SYSCALL information */
static int sssim_syscallsfree(mip)
SSSIM	*mip ;
{
	HDB_CURSOR	cur ;

	HDB_DATUM	key, value ;

	SSSIM_SYMBOL	*sep ;

	int	rs = SR_OK ;


	    hdb_cursorinit(&mip->syscalls,&cur) ;

	    while (hdb_enum(&mip->syscalls,&cur,&key,&value) >= 0) {

	        sep = (SSSIM_SYMBOL *) value.buf ;

		if (sep != NULL)
	        	free(sep) ;

	    } /* end while */

	    hdb_cursorfree(&mip->syscalls,&cur) ;

	rs = hdb_free(&mip->syscalls) ;

	mip->f.syscalls = FALSE ;
	return rs ;
}
/* end subroutine (sssim_syscallsfree) */


#if	F_DEBUG || F_DEBUGS

/* find a symbol that might indicate data memory */

struct sssim_symrank {
	int	type, bind, rank ;
} ;

static struct sssim_symrank	ranks[] = {
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

static int sssim_finddatamem(mip,name,rp)
SSSIM	*mip ;
char	name[] ;
uint	*rp ;
{
	LMAPPROG_SNCURSOR	cur ;

	Elf32_Sym	*best, *sep ;

	struct proginfo	*pip ;

	int	rankbest = -1 ;
	int	rankcur ;
	int	rs ;


	if (mip == NULL)
	    return SR_FAULT ;

#if	F_SAFE
	if ((mip->magic != SSSIM_MAGIC) && (mip->magic != 0)) {

	    eprintf("sssim_finddatamem: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mip->magic != SSSIM_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (name == NULL)
		return SR_FAULT ;

	pip = mip->pip ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sssim_finddatamem: entered name=%s\n",name) ;
#endif

	rs = lmapprog_sncursorinit(&mip->ourprog,&cur) ;

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

	    if (rs < 0)
	        eprintf("sssim_finddatamem: lmapprog_sncursorinit() rs=%d\n",
			rs) ;

	}
#endif /* F_DEBUG */

	if (rs < 0)
	    goto bad0 ;

	while ((rs = lmapprog_fetchsym(&mip->ourprog,name,&cur,&sep)) >= 0) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	        eprintf("sssim_finddatamem: got one, val=%08x\n",
			sep->st_value) ;
#endif

		rankcur = datasymrank(sep) ;

		if (rankcur > rankbest) {

			best = sep ;
			rankbest = rankcur ;
		}

	} /* end while */

	(void) lmapprog_sncursorfree(&mip->ourprog,&cur) ;

	if ((rs < 0) && (rs != SR_NOTFOUND)) {

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4)) {

	        if (rs < 0)
	            eprintf("sssim_finddatamem: lmapprog_fetchsym() rs=%d\n",
			rs) ;

	    }
#endif /* F_DEBUG */

	    return rs ;
	}

	if (rp != NULL)
	    *rp = (rankbest >= 0) ? ((uint) best->st_value) : 0 ;

	return (rankbest < 0) ? SR_NOTFOUND : rankbest ;

bad0:
	return rs ;
}
/* end subroutine (sssim_finddatamem) */


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

#endif /* F_DEBUG || F_DEBUGS */

#endif /* COMMENT */



