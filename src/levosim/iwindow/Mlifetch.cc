/* lifetch.c */

/* Levo Instruction Fetch Unit */


#define	F_DEBUGS	0		/* non-switchable */
#define	F_DEBUG		0		/* switchable */
#define	F_LDECODE	1		/* turn on if it can build ! */
#define	F_FPRINTF	1		/* STDOUT fprintf's ? */
#define	F_SAFE		1		/* extra safe mode ? */
#define	F_SAFE2		0		/* extra safe mode ? */
#define	F_AGGRESSIVE	0		/* aggressive mode */
#define	F_TOGGLELOAD	1		/* load signal toggles */
#define	F_SAFEFUNC	1		/* call the safe function */
#define	F_OLDLLB	0		/* use old LLB interface */
#define PERF_FETCH      1		/* perfect fetching */


/* revision history:

   	= 00/07/05, Marcos R. de Alba

	This code was started from scratch !


	= 00/07/14, Dave morano

	I added the standard method calls for integration
	into the whole package.


	= 00/07/17, Dave morano

	+ I added some debugging print-outs.
	+ The results from the instruction file opens were not
	being checked and caused a core dump.
	+ I changed the way that the binary instruction file is read
	in.  I am just reading the straight binary now, not the binary
	and hex combined.
	+ I changed the way that instructions are read into the load buffer.


	= 00/08/16, Dave morano

	+ I added the subroutine call to read the simulated program
	memory.  The old method of loading from some hack binary
	file is no longer needed.


	= 00/09/19, Dave Morano

	+ I fixed some little typo-like bugs and some missing variable
	declarations.


	= 01/03/29, Dave Morano

	I had to add an extra initialization argument in order to
	perform some sanity checking for debugging purposes.


*/


/******************************************************************************

	This is the Levo Instruction Fetch code !



******************************************************************************/



#include	<sys/types.h>
#include	<assert.h>
#include	<cstdio>

#include	<usystem.h>
#include	<bio.h>
#include	<libuc.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"ldecode.h"
#include	"lbtrb.h"
#include	"lifetch.h"			/* ourselves */




/* local defines */

#define	LIFETCH_MAGIC	0x86732543



/* external subroutines */


/* local structures */


/* forward references */

static int	lifetch_fetch(LIFETCH *,struct proginfo *,LSIM *,
			struct levoinfo *) ;
static int	lifetch_req_insts(LIFETCH *,int ,struct lifetch_mem *) ;
static int	lifetch_next_instr(LIFETCH *,LDECODE *) ;
static int      lifetch_unroll_loop(uint,uint,int,int,LDECODE *, 
			LIFETCH *lifp,int *lbindex) ;
static int	lifetch_sanitycheck(LIFETCH *,struct proginfo *,
			struct levoinfo *) ;






int lifetch_init(lifp,pip,mip,lip,ap)
LIFETCH			*lifp ;
struct proginfo		*pip ;
LSIM			*mip ;
struct levoinfo		*lip ;
LIFETCH_INITARGS	*ap ;
{
	uint	exitaddr ;

	int	rs = SR_OK ;
	int	size ;

	char	*cp ;


	if (lifp == NULL)
	    return SR_FAULT ;

	(void) memset(lifp,0,sizeof(LIFETCH)) ;

	lifp->pip = pip ;
	lifp->mip = mip ;
	lifp->lip = lip ;

	lifp->llbp = lip->llbp ;


#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_init: entered\n") ;
#endif


/* debugging stuff ,DAM */

	lifp->rfbuses = ap->rfbuses ;
	lifp->lases = ap->lases ;
	lifp->fetchaddr = ap->startaddr ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_init: LASes=%08lx\n",lifp->lases) ;
#endif

	lifp->sanity.func = ap->sanity.func ;
	lifp->sanity.arg = ap->sanity.arg ;


/* Initialize the Window Size */

	lifp->winsize = lip->totalrows * lip->nsgcols ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_init: totalrows=%d nsgcols=%d winsize=%d\n",
			lip->totalrows,lip->nsgcols,lifp->winsize) ;
#endif

	lifp->ifetchwidth = lip->ifetchwidth ;


	if (lsim_getsymval(lifp->mip,"exit",&exitaddr) >= 0)
	    lifp->exitaddr = exitaddr ;

	    else
	    assert(0) ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_init: ifetchwidth=%d\n",lifp->ifetchwidth) ;
#endif


/* some other local initializations */

	(void) memset(&lifp->c,0,sizeof(struct lifetch_state)) ;

	(void) memset(&lifp->n,0,sizeof(struct lifetch_state)) ;

	lifp->c.f.loadit = lifp->n.f.loadit = TRUE ;


/* allocate the space for the fetch buffer */

	size = lip->ifetchwidth * sizeof(unsigned int) ;
	rs = uc_malloc(size,(void **) &lifp->entry) ;

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lifp->entry,size,"lifetch_init:entry") ;
#endif

	size = lifp->winsize * sizeof(unsigned int) ;
	rs = uc_malloc(size,(void **) &lifp->inwindow) ;

	if (rs < 0)
	    goto bad1 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lifp->inwindow,size,"lifetch_init:inwindow") ;
#endif

#if	F_DEBUG
	                if (pip->debuglevel > 4)
	                    eprintf(
				"lifetch_fetch: INWINDOW=%08lx\n",
	                        lifp->inwindow) ;
#endif /* F_DEBUG */

	rs = uc_malloc(size,(void **) &lifp->ilptr) ;

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lifp->ilptr,size,"lifetch_init:ilptr") ;
#endif



	lifp->index = 0 ;

	lifp->tracefile = NULL ;

#if	PERF_FETCH
/* opens the trace file */

	if ((cp = ap->ifetchtrace) == NULL)
		cp = "addr.trace" ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("lifetch_init: ifetchtrace=%s\n",cp) ;
#endif

  lifp->tracefile=fopen(cp,"r");

	if (lifp->tracefile == NULL) {

		rs = SR_NOENT ;

#if	F_DEBUG
	if (pip->debuglevel > 4)
	eprintf("lifetch_init: no ifetch trace file ! rs=%d\n",rs) ;
#endif

		goto bad3 ;
	}

#ifdef	COMMENT
  assert(lifp->tracefile != NULL);
#endif

#endif /* PERF_FETCH */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_init: exiting OK rs=%d\n",rs) ;
#endif

	lifp->magic = LIFETCH_MAGIC ;
	return rs ;

/* bad things */
bad3:
	free(lifp->ilptr) ;

bad2:
	free(lifp->inwindow) ;

#ifdef	MALLOCLOG
	malloclog_free(lifp->inwindow,"lifetch_init:inwindow") ;
#endif

bad1:
	free(lifp->entry) ;

#ifdef	MALLOCLOG
	malloclog_free(lifp->entry,"lifetch_init:entry") ;
#endif

bad0:

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_init: exiting BAD rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (lifetch_init) */


/* free up this object */
int lifetch_free(lifp)
LIFETCH	*lifp ;
{
	struct proginfo	*pip ;


	if (lifp == NULL)
	    return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
		return SR_NOTOPEN ;

	pip = lifp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_free: entered\n") ;
#endif

	if (lifp->entry != NULL) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_free: freeing entry\n") ;
#endif

	    free(lifp->entry) ;

#ifdef	MALLOCLOG
	malloclog_free(lifp->entry,"lifetch_free:entry") ;
#endif

	}

	if (lifp->inwindow != NULL) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_free: freeing iwindow\n") ;
#endif

	    free(lifp->inwindow) ;

#ifdef	MALLOCLOG
	malloclog_free(lifp->inwindow,"lifetch_free:inwindow") ;
#endif

	}

	if (lifp->ilptr != NULL) {

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_free: freeing liptr\n") ;
#endif

	    free(lifp->ilptr) ;

#ifdef	MALLOCLOG
	malloclog_free(lifp->ilptr,"lifetch_free:ilptr") ;
#endif

	}

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_free: exiting OK\n") ;
#endif

	(void) memset(lifp,0,sizeof(LIFETCH)) ;

	return SR_OK ;
}
/* end subroutine (lifetch_free) */


/* Fetch a number of instructions equal to the number of rows every cycle */
int lifetch_comb(lifp,phase)
LIFETCH	*lifp ;
int	phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


	if (lifp == NULL)
		return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
		return SR_NOTOPEN ;

	pip = lifp->pip ;
	mip = lifp->mip ;
	lip = lifp->lip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_comb: entered, ph=%d\n",phase) ;
#endif


	switch (phase) {

	case 0:

#if	(! F_TOGGLELOAD)
	    lifp->n.f.loadit = FALSE ;
#endif

	    break ;

	case 1:
	    break ;

	case 2:
	    rs = lifetch_fetch(lifp,pip,mip,lip) ;

	    break ;

	} /* end switch */


	return rs ;
}
/* end subroutine (lifetch_comb) */


/* do a clock transition */
int lifetch_clock(lifp)
LIFETCH	*lifp ;
{
	struct proginfo	*pip ;


	if (lifp == NULL)
		return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
		return SR_NOTOPEN ;

	pip = lifp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_clock: entered\n") ;
#endif

/* copy next state to become current state */

	lifp->c = lifp->n ;


	return SR_OK ;
}
/* end subroutine (lifetch_clock) */


/* this gets called in phase 1 right now */
int lifetch_loadstatus(lifp,sw)
LIFETCH	*lifp ;
uint	sw ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


	if (lifp == NULL)
		return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
		return SR_NOTOPEN ;

	pip = lifp->pip ;
	mip = lifp->mip ;
	lip = lifp->lip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_loadstatus: loadstat=%04x\n",sw) ;
#endif


/* update our load state */

	lifp->n.f.loadit = ((sw & 1) != 0) ;


	return SR_OK ;
}
/* end subroutine (lifetch_loadstatus) */


/* update our various predictor tables */
int lifetch_bu(lifp,ba,type,bc,ta)
LIFETCH	*lifp ;
uint	ba ;
uint	type ;
int	bc ;
uint	ta ;
{
	int	rs = SR_OK ;


	if (lifp == NULL)
		return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
		return SR_NOTOPEN ;


	return rs ;
}
/* end subroutine (lifetch_bu) */


/* we are told to go to some address */
int lifetch_goto(lifp,ta)
LIFETCH	*lifp ;
uint	ta ;
{
	int	rs = SR_OK ;


	if (lifp == NULL)
		return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
		return SR_NOTOPEN ;

	lifp->btarget = (ta != -1) ? ta : 0 ;

	return rs ;
}
/* end subroutine (lifetch_goto) */



/* PRIVATE SUBROUTINES */



static int lifetch_targetin(lifp,brTA, tt)
LIFETCH *lifp ;
uint brTA ;
int	tt ;
{
	int i ;


	for(i=0;i<lifp->winsize;++i) {
	    if(brTA == lifp->inwindow[i])
	        return 1 ;
	}
	return 0 ;
}


/* fetch some instructions, decode, and load them */
static int lifetch_fetch(lifp,pip,mip,lip)
LIFETCH		*lifp ;
struct proginfo *pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
{
	LDECODE current_instr,current_auxinstr ;
	LDECODE auxbuffer[256] ; /* have to be fixed */

	struct lbtrb_predicates pr ;

	uint	faddr,newfaddr ;
	uint	instr,auxinstr,nrefinstrs ;
	uint   pina,  ncpin, pouta, cpouta, pinv ;
	uint	cpina[100],cpinv[100] ;

	int	rs = SR_OK ;
	int	rs1 = SR_BADFMT ;
	int	auxindex, i, j, k, m, bodysize, newilptr ;
	int	lbindex, f_loopcapture, f_fill_llb ;
	int	f_branch=0 ;

	char    address[21];


#if	F_DEBUG
	if (pip->debuglevel > 2) {
	    eprintf("lifetch_fetch: entered\n") ;
	}
#endif

/* local sanity */

#if	F_SAFE2
	rs = lifetch_sanitycheck(lifp,pip,lip) ;

	if (rs < 0) {

#if	F_DEBUG
	    if (pip->debuglevel > 2) {
	        eprintf("lifetch_fetch: 0a sanity rs=%d\n", 
	            rs) ;
	    }
#endif

#if	F_DEBUGS
	    eprintf("lifetch_fetch: 0a sanity rs=%d\n", 
	        rs) ;
#endif

	    return rs ;
	}
#endif /* F_SAFE2 */


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("lifetch_fetch: 0 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("lifetch_fetch: 0 bad sanity global rs=%d\n",
	                rs1) ;
#endif

	        return rs1 ;
	    }
	}
#endif /* F_SAFEFUNC */


/* are we free to load the Levo LB ? */

#if	F_AGGRESSIVE

	if (! lifp->c.f.loadit) {

#if	F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("lifetch_fetch: loadit says not ready\n") ;
#endif

	    return SR_OK ;
	}

#else /* F_AGGRESSIVE */

#if	F_OLDLLB
	if (! llb_is_free(lifp->llbp))
	    return SR_OK ;
#else
	rs = llb_inready(lifp->llbp) ;

#if	F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("lifetch_fetch: llb_inready() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		return rs ;

	if (rs == 0) {

		return SR_OK ;
	}
#endif /* F_OLDLLB */


#endif /* F_AGGRESSIVE */


	if (! lbtrb_isready(lip->lbtrbp)) {

#if	F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("lifetch_fetch: LBTRB says not ready\n") ;
#endif

	    return SR_OK ;
	}


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("lifetch_fetch: 4 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("lifetch_fetch: 4 bad sanity global rs=%d\n",
	                rs1) ;
#endif

	        return rs1 ;
	    }
	}
#endif /* F_SAFEFUNC */


/* we assume that the branch target is not in the window */

	if (lifp->btarget != 0) {

	    faddr = lifp->btarget ;
	    lifp->btarget = 0 ;
/* flush branch tracking buffer */

	} else
	    faddr = lifp->fetchaddr ;

/* eprintf("lifetch_fetch: ifetchwidth: %d\n", lifp->ifetchwidth) ; */

	if (faddr == lifp->exitaddr) {
	    printf("Simulation terminated\n") ;
	}

	for (lbindex = 0 ; lbindex < lip->totalrows ; lbindex += 1) {

#if	PERF_FETCH
	if (lifp->tracefile != NULL) {

		if(!lifp->reuse_faddr){
			if(fgets(address,20,lifp->tracefile) == NULL)
				assert(0);
			sscanf(address,"%X",&faddr);

		} else {

			faddr = lifp->reuse_faddr;
			lifp->reuse_faddr = 0;
		}
		printf("fetch address from trace = %X\n",faddr);
	} /* end if */
#endif /* PERF_FETCH */

	    rs = lsim_readinstr(mip,faddr,&instr) ;


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("lifetch_fetch: 6 sanity global\n") ;
#endif

	    if (lifp->sanity.func != NULL) {

	        rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	        if (rs1 < 0) {

#if	F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("lifetch_fetch: 6 bad sanity global rs=%d\n",
	                    rs1) ;
#endif

	            return rs1 ;
	        }
	    }
#endif /* F_SAFEFUNC */


	    if (rs < 0) {
	        printf("lifetch_fetch: LSIM readinstr returned negative\n") ;
	        current_instr.opclass = INSTR_UNUSED ;
	        current_instr.opexec = LEXECOP_NOOP ;
/* assert(0) ; */
	    } else {

#if	F_FPRINTF
	        printf("lifetch_fetch: lbindex=%4d ia=%08x instr=%08x\n", 
	            lbindex,faddr,instr) ;
#endif

	        current_instr.instrword = instr ;
	        current_instr.ilptr= faddr ;
	        ldecode_decode(&current_instr) ;


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	        if (pip->debuglevel > 4)
	            eprintf("lifetch_fetch: 8 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_DEBUG
	                if (pip->debuglevel > 1)
	                    eprintf(
				"lifetch_fetch: 8 bad sanity global rs=%d\n",
	                        rs1) ;
#endif

	                return rs1 ;
	            }
	        }
#endif /* F_SAFEFUNC */


	    } /* end if (decoding) */

#if	F_SAFE2
	    rs = lifetch_sanitycheck(lifp,pip,lip) ;

	    if (rs < 0) {

#if	F_DEBUG
	        if (pip->debuglevel > 2) {
	            eprintf("lifetch_fetch: 1 sanity i=%d rs=%d\n", 
	                lbindex,rs) ;
	        }
#endif

#if	F_DEBUGS
	        eprintf("lifetch_fetch: 1 sanity i=%d rs=%d\n", 
	            lbindex,rs) ;
#endif

	        return rs ;
	    }
#endif /* F_SAFE2 */

/* make sure the last instruction in the LB is not a branch or jump!! */

	    if ((lbindex == (lifp->lip->totalrows - 1)) &&
	        (current_instr.opclass == INSTR_JIND ||
	        current_instr.opclass == INSTR_BREL || 
	        current_instr.opclass == INSTR_JREL)) {

/* set AS as unused */
	        current_instr.opclass = INSTR_UNUSED ;
	        faddr -= 4 ;
#if     PERF_FETCH
		lifp->reuse_faddr = faddr+4;
		printf("XXXXXXX fetch address adjusted for trace = %X\n",faddr);
#endif


	    } else {


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	        if (pip->debuglevel > 4)
	            eprintf("lifetch_fetch: 12 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_DEBUG
	                if (pip->debuglevel > 1)
	                    eprintf(
	                        "lifetch_fetch: 12 bad sanity global rs=%d\n",
	                        rs1) ;
#endif

	                return rs1 ;
	            }
	        }
#endif /* F_SAFEFUNC */


	        lbtrb_assign_predicate(lip->lbtrbp, 
	            current_instr.opclass, current_instr.ilptr, 
	            current_instr.const1, 0, &pr) ;

#if	F_SAFE2
	        rs = lifetch_sanitycheck(lifp,pip,lip) ;

	        if (rs < 0) {

#if	F_DEBUG
	            if (pip->debuglevel > 2) {
	                eprintf("lifetch_fetch: 2 sanity i=%d rs=%d\n", 
	                    lbindex,rs) ;
	            }
#endif

#if	F_DEBUGS
	            eprintf("lifetch_fetch: 2 sanity i=%d rs=%d\n", 
	                lbindex,rs) ;
#endif

	            return rs ;
	        }
#endif /* F_SAFE2 */


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	        if (pip->debuglevel > 4)
	            eprintf("lifetch_fetch: 13 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_DEBUG
	                if (pip->debuglevel > 1)
	                    eprintf(
                                "lifetch_fetch: 13 bad sanity global rs=%d\n",
	                        rs1) ;
#endif

	                return rs1 ;
	            }
	        }
#endif /* F_SAFEFUNC */


	        pina = pr.ipaddr ;
	        ncpin = pr.numicp ;
	        for (j = 0 ; j < MIN(ncpin,100) ; j += 1)
	            cpina[j] = pr.icpaddr[j] ;


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	        if (pip->debuglevel > 4)
	            eprintf("lifetch_fetch: 14 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_DEBUG
	                if (pip->debuglevel > 1)
	                    eprintf(
                                "lifetch_fetch: 14 bad sanity global rs=%d\n",
	                        rs1) ;
#endif

	                return rs1 ;
	            }
	        }
#endif /* F_SAFEFUNC */


	        pouta = pr.opaddr ;
	        cpouta = pr.ocpaddr ;

/* Branch predictor would assign initial predicate values */

	        pinv = (pr.ipval == -1) ? 1 : pr.ipval ;
	        for (j = 0 ; j < MIN(ncpin,100) ; j += 1)
	            cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j] ;


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	        if (pip->debuglevel > 4)
	            eprintf("lifetch_fetch: 16 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_DEBUG
	                if (pip->debuglevel > 1)
	                    eprintf(
				"lifetch_fetch: 16 bad sanity global rs=%d\n",
	                        rs1) ;
#endif

	                return rs1 ;
	            }
	        }
#endif /* F_SAFEFUNC */


	    } /* end if */

#if	F_DEBUG && 0
	                if (pip->debuglevel > 4) {
	                    eprintf(
				"lifetch_fetch: INWINDOW=%08lx\n",
	                        lifp->inwindow) ;
	                    eprintf(
				"lifetch_fetch: lbindex=%d\n",
	                        lbindex) ;
		}
#endif /* F_DEBUG */

	    lifp->inwindow[lbindex] = faddr ;

/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("lifetch_fetch: 16a sanity global\n") ;
#endif

	    if (lifp->sanity.func != NULL) {

	        rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	        if (rs1 < 0) {

#if	F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
				"lifetch_fetch: 16a bad sanity global rs=%d\n",
	                    rs1) ;
#endif

	            return rs1 ;
	        }
	    }
#endif /* F_SAFEFUNC */


	    rs = llb_load(lifp->llbp, lbindex, faddr,
	        current_instr.instrword,
	        current_instr.opexec,	/* original opexec */
	    current_instr.opclass,	/* original opclass */
	    current_instr.opexec,	/* transformed opexec */
	    current_instr.opclass,	/* transformed opclass */
	    current_instr.opclass,	/* branch type */
		current_instr.src1,
	        current_instr.src2,
	        current_instr.src3,
	        current_instr.src4,
	        current_instr.dst1,
	        current_instr.dst2,
	        current_instr.dst3,
	        current_instr.const1,
	        current_instr.const1_valid,
	        pina,
	        cpina,
	        ncpin,
	        pouta,
	        cpouta,
	        pinv,
	        cpinv,
		FALSE) ;

	    if (rs < 0) {
	        printf("Load buffer load returned negative (rs=%d)\n",rs) ;
	        assert(0) ;
	    }

#if	F_SAFE2
	    rs = lifetch_sanitycheck(lifp,pip,lip) ;

	    if (rs < 0) {

#if	F_DEBUG
	        if (pip->debuglevel > 2) {
	            eprintf("lifetch_fetch: 3 sanity i=%d rs=%d\n", 
	                lbindex,rs) ;
	        }
#endif

#if	F_DEBUGS
	        eprintf("lifetch_fetch: 3 sanity i=%d rs=%d\n", 
	            lbindex,rs) ;
#endif

	        return rs ;
	    }
#endif /* F_SAFE2 */


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("lifetch_fetch: 18 sanity global\n") ;
#endif

	    if (lifp->sanity.func != NULL) {

	        rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	        if (rs1 < 0) {

#if	F_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf(
				"lifetch_fetch: 18 bad sanity global rs=%d\n",
	                    rs1) ;
#endif

	            return rs1 ;
	        }
	    }
#endif /* F_SAFEFUNC */



	    faddr += 4 ; /* next fetch address */

	} /* end for (looping reading instructions) */


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("lifetch_fetch: 19 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("lifetch_fetch: 19 bad sanity global rs=%d\n",
	                rs1) ;
#endif

	        return rs1 ;
	    }
	}
#endif /* F_SAFEFUNC */


	rs = llb_load_complete(lifp->llbp) ;

#if	F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("lifetch_fetch: llb_loadcomplete() rs=%d\n",
	                rs) ;
#endif

/* update the fetch address for next time */

	lifp->fetchaddr = faddr ;

	rs = SR_OK ;

/* bad things */
bad:


/* global sanity */

#if	F_SAFEFUNC

#if	F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("lifetch_fetch: 20 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf(
			"lifetch_fetch: 20 bad sanity global rs=%d\n",
	                rs1) ;
#endif

	        return rs1 ;
	    }
	}
#endif /* F_SAFEFUNC */


	return rs ;
}
/* end subroutine (lifetch_fetch) */



static int lifetch_unroll_loop(faddr,instr,from, to, auxbuffer, lifp,lbindex)
uint	faddr, instr ;
int	from, to ;
LDECODE *auxbuffer ;
LIFETCH	*lifp ;
int	*lbindex ;
{
	int i,rs,k ;


	i=lifp->index ;
	for (k=0; k < to;k++) {

#ifdef	COMMENT

	    rs = llb_load(lifp->llbp, *lbindex, 
	        faddr,
	        instr,
	        auxbuffer[i].opexec,	/* original */
	    auxbuffer[i].opclass,	/* original */
	    auxbuffer[i].opexec,	/* transformed */
	    auxbuffer[i].opclass,	/* transformed */
	    auxbuffer[i].opclass,	/* branch type */
	    auxbuffer[i].src1, auxbuffer[i].src2,
	        auxbuffer[i].src3,auxbuffer[i].src4,
	        auxbuffer[i].dst1,auxbuffer[i].dst2,
	        auxbuffer[i].const1,
	        auxbuffer[i].const1_valid,
	        auxbuffer[i].pina,
	        auxbuffer[i].cpina,
	        auxbuffer[i].ncpin,
	        auxbuffer[i].pouta,
	        auxbuffer[i].cpouta) ;

/*    printinstr(&auxbuffer[i]) ; */

	    if (rs<0)
	        printf("lifetch_unroll_loop: error loading llb ") ;

#endif /* COMMENT */

	    *lbindex+=1 ;
	    i+=1 ;
	}

	return 0 ;
}
/* end subroutine (lifetch_unroll_loop) */




#ifdef	COMMENT

/* get some instructions into the fetch buffer */
static int lifetch_req_insts(lifp,addr,memp)
LIFETCH	*lifp ;
int	addr ;
struct lifetch_mem	*memp ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int i,n ;


	pip = lifp->pip ;
	lip = lifp->lip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lifetch_reqinsts: entered, addr=%08x\n",
	        addr) ;
#endif

	for (i = 0 ; i < lifp->ifetchwidth ; i += 1) {

	    int	mi ;


	    mi = (addr >> 2) + i ;
	    if (mi >= memp->num)
	        return SR_FAULT ;

	    lifp->entry[i] = memp->data[mi] ;
	    lifp->ilptr[i] = memp->addr[mi] ;

	}


	return SR_OK ;
}
/* end subroutine (lifetch_req_insts) */


static int lifetch_next_instr(LIFETCH *lifp,LDECODE *current_instr)
{
	int index ;

	int	rs = SR_FAULT ;


	index = lifp->index ;
	if (index < lifp->simmem.num) {

	    current_instr->instrword = lifp->entry[index] ;
	    current_instr->ilptr = lifp->ilptr[index] ;
	    lifp->index += 1 ;

	}

	return rs ;
}
/* end subroutine (lifetch_next_instr) */

#endif /* COMMENT */


static int lifetch_sanitycheck(lifp,pip,lip)
LIFETCH		*lifp ;
struct proginfo *pip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK, i ;


/* check the RF buses */

	for (i = 0 ; i < lip->nsg ; i += 1) {

	    rs = bus_sanitycheck(lifp->rfbuses + i) ;

	    if (rs < 0)
	        break ;

	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3) {
	    if (rs < 0)
	        eprintf("lifetch_sanitycheck: bad RFBUS i=%d rs=%d\n",
	            i,rs) ;
	}
#endif

	if (rs < 0)
	    return rs ;

/* check the ASes */

	for (i = 0 ; i < (lip->totalrows * lip->totalcols) ; i += 1) {

	    rs = las_sanitycheck(lifp->lases + i) ;

	    if (rs < 0)
	        break ;

	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3) {
	    if (rs < 0)
	        eprintf("lifetch_sanitycheck: bad AS=%08lx i=%d rs=%d\n",
	            i,(lifp->lases + i),rs) ;
	}
#endif

	return rs ;
}
/* end subroutine (lifetch_sanitycheck) */



