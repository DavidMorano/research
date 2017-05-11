/* lifetch.c */

/* Levo Instruction Fetch Unit */


#define	F_DEBUGS	0		/* non-switchable */
#define	F_DEBUG		1		/* switchable */
#define	F_DEBUG2	0		/* switchable */
#define	F_FPRINTF	0		/* STDOUT fprintf's ? */
#define	F_SAFE		1		/* extra safe mode ? */
#define	F_SAFE2		0		/* extra safe mode ? */
#define	F_AGGRESSIVE	0		/* aggressive mode */
#define	F_TOGGLELOAD	1		/* load signal toggles */
#define	F_SAFEFUNC	1		/* call the safe function */
#define	F_OLDLLB	0		/* use old LLB interface */
#define F_PERFFETCH	1		/* perfect fetching */


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


	= 01/09/26, Dave Morano

	Added stuff for an XML output state trace.


*/


/******************************************************************************

	This is the Levo Instruction Fetch code !



******************************************************************************/



#include	<sys/types.h>
#include	<assert.h>
#include	<stdio.h>

#include	<vsystem.h>
#include	<bio.h>
#include	<libuc.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"opclass.h"		/* instructions classes */
#include	"lexecop.h"		/* instructions operations */
#include	"ldecode.h"
#include	"lbtrb.h"
#include	"lifetch.h"		/* ourselves */




/* local defines */

#define	LIFETCH_MAGIC	0x86732543

#define	DEBUGLEVEL(n)	(pip->debuglevel >= (n))



/* external subroutines */

extern int	cfnumui(const char *,int,uint *) ;
extern int	cfhexui(const char *,int,uint *) ;


/* local structures */


/* forward references */

static int	lifetch_fetch(LIFETCH *,struct proginfo *,LSIM *,
			struct levoinfo *) ;
static int	lifetch_req_insts(LIFETCH *,int ,struct lifetch_mem *) ;
static int	lifetch_next_instr(LIFETCH *,LDECODE *) ;
static int	lifetch_sanitycheck(LIFETCH *,struct proginfo *,
			struct levoinfo *) ;

#ifdef	COMMENT

static int      lifetch_unroll_loop(uint,uint,int,int,LDECODE *, 
LIFETCH *lifp,int *lbindex) ;

#endif




int lifetch_init(lifp,pip,mip,lip,ap)
LIFETCH			*lifp ;
struct proginfo		*pip ;
LSIM			*mip ;
struct levoinfo		*lip ;
LIFETCH_INITARGS	*ap ;
{
	uint	exitaddr ;

	uint	ia ;

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


#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_init: entered\n") ;
#endif


/* debugging stuff ,DAM */

	lifp->rfbuses = ap->rfbuses ;
	lifp->lases = ap->lases ;
	ia = ap->startaddr ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_init: entry=%08x LASes=%08lx\n",
		ap->startaddr,lifp->lases) ;
#endif

	lifp->sanity.func = ap->sanity.func ;
	lifp->sanity.arg = ap->sanity.arg ;


/* Initialize the Window Size */

	lifp->winsize = lip->totalrows * lip->nsgcols ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_init: totalrows=%d nsgcols=%d winsize=%d\n",
	        lip->totalrows,lip->nsgcols,lifp->winsize) ;
#endif

	lifp->ifetchwidth = lip->ifetchwidth ;


	lifp->exitaddr = 0 ;
	if (lsim_getsymval(lifp->mip,"exit",&exitaddr) >= 0)
	    lifp->exitaddr = exitaddr ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_init: ifetchwidth=%d\n",lifp->ifetchwidth) ;
#endif


/* some other local initializations */

	(void) memset(&lifp->c,0,sizeof(struct lifetch_state)) ;

	(void) memset(&lifp->n,0,sizeof(struct lifetch_state)) ;

	lifp->c.f.loadit = lifp->n.f.loadit = TRUE ;
	lifp->c.ia = lifp->n.ia = ia ;


/* allocate the space for the fetch buffer */

	size = lip->ifetchwidth * sizeof(unsigned int) ;
	rs = uc_malloc(size,&lifp->entry) ;

	if (rs < 0)
	    goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lifp->entry,size,"lifetch_init:entry") ;
#endif

	size = lifp->winsize * sizeof(unsigned int) ;
	rs = uc_malloc(size,&lifp->inwindow) ;

	if (rs < 0)
	    goto bad1 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lifp->inwindow,size,"lifetch_init:inwindow") ;
#endif

	(void) memset(lifp->inwindow,0,size) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_init: INWINDOW=%08lx\n",
	        lifp->inwindow) ;
#endif /* F_DEBUG */

	rs = uc_malloc(size,&lifp->ilptr) ;

	if (rs < 0)
	    goto bad2 ;

#ifdef	MALLOCLOG
	malloclog_alloc(lifp->ilptr,size,"lifetch_init:ilptr") ;
#endif



	lifp->index = 0 ;

	lifp->tracefile = NULL ;

#if	F_PERFFETCH

/* opens the trace file */

	if ((cp = ap->ifetchtrace) == NULL)
	    cp = "addr.trace" ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_init: ifetchtrace=%s\n",cp) ;
#endif

	lifp->tracefile=fopen(cp,"r") ;

	if (lifp->tracefile == NULL) {

	bprintf(pip->efp,"%s: no ifetch trace file when one needed\n",
		pip->progname) ;

	    rs = SR_NOENT ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_init: no ifetch trace file ! rs=%d\n",rs) ;
#endif

	    goto bad3 ;
	}

#ifdef	COMMENT
	assert(lifp->tracefile != NULL) ;
#endif

#endif /* F_PERFFETCH */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_free: entered\n") ;
#endif

	if (lifp->entry != NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_free: freeing entry\n") ;
#endif

	    free(lifp->entry) ;

#ifdef	MALLOCLOG
	    malloclog_free(lifp->entry,"lifetch_free:entry") ;
#endif

	}

	if (lifp->inwindow != NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_free: freeing iwindow\n") ;
#endif

	    free(lifp->inwindow) ;

#ifdef	MALLOCLOG
	    malloclog_free(lifp->inwindow,"lifetch_free:inwindow") ;
#endif

	}

	if (lifp->ilptr != NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_free: freeing liptr\n") ;
#endif

	    free(lifp->ilptr) ;

#ifdef	MALLOCLOG
	    malloclog_free(lifp->ilptr,"lifetch_free:ilptr") ;
#endif

	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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


#if	F_MASTERDEBUG && F_SAFE
	if (lifp == NULL)
	    return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = lifp->pip ;
	mip = lifp->mip ;
	lip = lifp->lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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


#if	F_MASTERDEBUG && F_SAFE
	if (lifp == NULL)
	    return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = lifp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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


#if	F_MASTERDEBUG && F_SAFE
	if (lifp == NULL)
	    return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = lifp->pip ;
	mip = lifp->mip ;
	lip = lifp->lip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel >= 5)
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


#if	F_MASTERDEBUG && F_SAFE
	if (lifp == NULL)
	    return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif


	return rs ;
}
/* end subroutine (lifetch_bu) */


/* we are told to go to some address */
int lifetch_goto(lifp,ta)
LIFETCH	*lifp ;
uint	ta ;
{
	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (lifp == NULL)
	    return SR_FAULT ;

	if (lifp->magic != LIFETCH_MAGIC)
	    return SR_NOTOPEN ;
#endif

	lifp->btarget = (ta != -1) ? ta : 0 ;

	return rs ;
}
/* end subroutine (lifetch_goto) */


/* XML stuff */
int lifetch_xmlinit(fp,xip)
LIFETCH	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}


int lifetch_xmlout(fp,xip)
LIFETCH	*fp ;
XMLINFO	*xip ;
{


	bprintf(&xip->xmlfile,"<lifetch>\n") ;


	bprintf(&xip->xmlfile,"</lifetch>\n") ;

	return SR_OK ;
}


int lifetch_xmlfree(fp,xip)
LIFETCH	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}
/* end subroutine (lifetch_xmlfree) */



/* PRIVATE SUBROUTINES */



#ifdef	COMMENT

static int lifetch_targetin(lifp,brTA, tt)
LIFETCH *lifp ;
uint brTA ;
int	tt ;
{
	int i ;


	for (i = 0 ; i < lifp->winsize ; i += 1) {
	    if (brTA == lifp->inwindow[i])
	        return 1 ;
	}

	return 0 ;
}

#endif /* COMMENT */


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

	uint	ia, instr, auxinstr, nrefinstrs ;
	uint	pina,  ncpin, pouta, cpouta, pinv ;
	uint	cpina[100],cpinv[100] ;

	int	rs = SR_OK ;
	int	rs1 = SR_BADFMT ;
	int	auxindex, i, j, k, m, bodysize ;
	int	lbindex, f_loopcapture, f_fill_llb ;
	int	f_cfc ;
	int	f_exit = FALSE ;

	char    addrbuf[21] ;


#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_fetch: entered\n") ;
#endif

/* local sanity */

#if	F_MASTERDEBUG && F_SAFE2
	rs = lifetch_sanitycheck(lifp,pip,lip) ;

	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 2)
	        eprintf("lifetch_fetch: bad 0a sanity rs=%d\n", 
	            rs) ;
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

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_fetch: 0 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 2)
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_fetch: LBTRB says not ready\n") ;
#endif

	    return SR_OK ;
	}


#if	F_PERFFETCH
	if (lifp->c.f.eof)
		return 1 ;
#endif /* F_PERFFETCH */


/* global sanity */

#if	F_SAFEFUNC

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_fetch: 4 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	            eprintf("lifetch_fetch: 4 bad sanity global rs=%d\n",
	                rs1) ;
#endif

	        return rs1 ;
	    }
	}
#endif /* F_SAFEFUNC */


/* we assume that the branch target is not in the window */

	if (lifp->btarget != 0) {

	    ia = lifp->btarget ;
	    lifp->btarget = 0 ;

/* flush branch tracking buffer */

	} else
	    ia = lifp->c.ia ;

#if	F_FPRINTF
	if (ia == lifp->exitaddr)
	    printf("lifetch_fetch: simulation terminating\n") ;
#endif

	lifp->n.f.cfc = FALSE ;
	f_cfc = lifp->c.f.cfc ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_fetch: c_cfc=%d c_ia=%08x\n", 
			lifp->c.f.cfc,lifp->c.ia) ;
#endif

	for (lbindex = 0 ; lbindex < lip->totalrows ; lbindex += 1) {

#if	F_PERFFETCH
	    if (lifp->tracefile != NULL) {

	        if (f_cfc) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_fetch: PF reuse ia=%08x !\n",
			lifp->c.ia) ;
#endif

			f_cfc = FALSE ;
	            ia = lifp->c.ia ;

	        } else {

	            if (fgets(addrbuf,20,lifp->tracefile) == NULL) {

			f_exit = TRUE ;
	                break ;
		}

#ifdef	COMMENT
	            sscanf(addrbuf,"%X",&ia) ;
#else
		if (isdigit(addrbuf[0]) && isdigit(addrbuf[1]))
	            cfhexui(addrbuf,-1,&ia) ;

		else
	            cfnumui(addrbuf,-1,&ia) ;
#endif

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	        eprintf("lifetch_fetch: PF addrbuf=>%s< ia=%08x\n",
			addrbuf,ia) ;
#endif

	        } /* end if (control-flow-change or not) */

	    } /* end if (perfect fetching) */
#endif /* F_PERFFETCH */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	        eprintf("lifetch_fetch: fetching ia=%08x\n",ia) ;
#endif

	    rs = lsim_readinstr(mip,ia,&instr) ;


	    if (rs < 0) {

#if	F_FPRINTF
	        printf("lifetch_fetch: LSIM readinstr returned negative\n") ;
#endif

	        current_instr.opclass = OPCLASS_UNUSED ;
	        current_instr.opexec = LEXECOP_NOOP ;
		return rs ;

	    } else {

#if	F_FPRINTF
	        printf("lifetch_fetch: lbindex=%4d ia=%08x instr=%08x\n", 
	            lbindex,ia,instr) ;
#endif

#ifdef	COMMENT
	        current_instr.ilptr = ia ;
	        current_instr.instr = instr ;
#endif

	        rs = ldecode_decode(&current_instr,ia,instr) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(3)) {
	            eprintf("lifetch_fetch: ldecode_decode() rs=%d\n", 
	                rs) ;
	            eprintf("lifetch_fetch: s1=%08x s2=%08x s3=%08x\n", 
	    current_instr.src1,
	        current_instr.src2,
	        current_instr.src3) ;
	            eprintf("lifetch_fetch: s4=%08x s5=%08x \n", 
	        current_instr.src4,
	        current_instr.src5) ;
	            eprintf("lifetch_fetch: d1=%08x d2=%08x d3=%08x\n", 
	        current_instr.dst1,
	        current_instr.dst2,
	        current_instr.dst3) ;
	}
#endif /* F_DEBUG */

		if (rs < 0)
			return rs ;

	    } /* end if (decoding) */


/* make sure the last instruction in the LB is not a branch or jump!! */

	    if ((lbindex == (lifp->lip->totalrows - 1)) &&
	        ((current_instr.opclass == OPCLASS_JIND) ||
	        (current_instr.opclass == OPCLASS_BREL) || 
	        (current_instr.opclass == OPCLASS_JREL))) {

/* set AS as UNUSED */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	            eprintf("lifetch_fetch: CFC in last row !! ia=%08x\n",
				ia) ;
#endif

		lifp->n.f.cfc = TRUE ;
		lifp->n.ia = ia ;
	        current_instr.opclass = OPCLASS_UNUSED ;
	        ia -= 4 ;

	    } else {

/* global sanity */

#if	F_SAFEFUNC

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	            eprintf("lifetch_fetch: 12 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	                    eprintf(
	                        "lifetch_fetch: 12 bad sanity global rs=%d\n",
	                        rs1) ;
#endif

	                return rs1 ;
	            }
	        }
#endif /* F_SAFEFUNC */


	        lbtrb_assign_predicate(lip->lbtrbp, 
	            current_instr.opclass, ia,
	            current_instr.const1, 0, &pr) ;

#if	F_MASTERDEBUG && F_SAFE2
	        rs = lifetch_sanitycheck(lifp,pip,lip) ;

	        if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	                eprintf("lifetch_fetch: bad 2 sanity i=%d rs=%d\n", 
	                    lbindex,rs) ;
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

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	            eprintf("lifetch_fetch: 13 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
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

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	            eprintf("lifetch_fetch: 14 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
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

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	            eprintf("lifetch_fetch: 16 sanity global\n") ;
#endif

	        if (lifp->sanity.func != NULL) {

	            rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	            if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	                    eprintf(
	                        "lifetch_fetch: 16 bad sanity global rs=%d\n",
	                        rs1) ;
#endif

	                return rs1 ;
	            }
	        }
#endif /* F_SAFEFUNC */

	    } /* end if (control-flow-change or not) */

	    lifp->inwindow[lbindex] = ia ;

/* global sanity */

#if	F_SAFEFUNC

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_fetch: 16a sanity global\n") ;
#endif

	    if (lifp->sanity.func != NULL) {

	        rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	        if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	                eprintf(
	                    "lifetch_fetch: 16a bad sanity global rs=%d\n",
	                    rs1) ;
#endif

	            return rs1 ;
	        }
	    }
#endif /* F_SAFEFUNC */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5) {

	        if (current_instr.opclass != OPCLASS_UNUSED)
	            eprintf("lifetch_fetch: row=%d ia=%08x instr=%08x\n",
	                lbindex,ia,instr) ;

		else
	            eprintf("lifetch_fetch: row=%d UNUSED\n",
	                lbindex) ;

		}
#endif /* F_DEBUG */

	    rs = llb_load(lifp->llbp, lbindex, ia,
	        current_instr.instr ,
	        current_instr.opexec,	/* original opexec */
	    current_instr.opclass,	/* original opclass */
	    current_instr.opexec,	/* transformed opexec */
	    current_instr.opclass,	/* transformed opclass */
	    current_instr.opclass,	/* branch type */
	    current_instr.src1,
	        current_instr.src2,
	        current_instr.src3,
	        current_instr.src4,
	        current_instr.src5,
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

#if	F_MASTERDEBUG && F_DEBUG
		if (pip->debuglevel >= 2)
	        eprintf(
	            "lifetch_fetch: llb_load() rs=%d\n",
	            rs) ;
#endif

	        return rs ;

	    } /* end if (bad load) */

#if	F_MASTERDEBUG && F_SAFE2
	    rs = lifetch_sanitycheck(lifp,pip,lip) ;

	    if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 2)
	            eprintf("lifetch_fetch: 3 sanity i=%d rs=%d\n", 
	                lbindex,rs) ;
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

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	        eprintf("lifetch_fetch: 18 sanity global\n") ;
#endif

	    if (lifp->sanity.func != NULL) {

	        rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	        if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	                eprintf(
	                    "lifetch_fetch: 18 bad sanity global rs=%d\n",
	                    rs1) ;
#endif

	            return rs1 ;
	        }
	    }
#endif /* F_SAFEFUNC */


	    ia += 4 ; /* next fetch address */

	} /* end for (looping reading instructions) */


/* load up and unused rows */

	for ( ; lbindex < lip->totalrows ; lbindex += 1) {

	    lifp->inwindow[lbindex] = ia ;

		current_instr.opclass = OPCLASS_UNUSED ;
		current_instr.opexec = LEXECOP_NOOP ;

	    rs = llb_load(lifp->llbp, lbindex, ia,
	        current_instr.instr,
	        current_instr.opexec,	/* original opexec */
	    current_instr.opclass,	/* original opclass */
	    current_instr.opexec,	/* transformed opexec */
	    current_instr.opclass,	/* transformed opclass */
	    current_instr.opclass,	/* branch type */
	    current_instr.src1,
	        current_instr.src2,
	        current_instr.src3,
	        current_instr.src4,
	        current_instr.src5,
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

	} /* end if (loading unused rows) */


/* global sanity */

#if	F_SAFEFUNC

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_fetch: 19 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	            eprintf("lifetch_fetch: 19 bad sanity global rs=%d\n",
	                rs1) ;
#endif

	        return rs1 ;
	    }
	}
#endif /* F_SAFEFUNC */


	rs = llb_load_complete(lifp->llbp) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_fetch: llb_loadcomplete() rs=%d\n",
	        rs) ;
#endif

	lifp->n.f.eof = f_exit ;

	rs = SR_OK ;

/* bad things */
bad:


/* global sanity */

#if	F_SAFEFUNC

#if	F_MASTERDEBUG && F_DEBUG && F_DEBUG2
	if (pip->debuglevel >= 5)
	    eprintf("lifetch_fetch: 20 sanity global\n") ;
#endif

	if (lifp->sanity.func != NULL) {

	    rs1 = (*lifp->sanity.func)(lifp->sanity.arg) ;

	    if (rs1 < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
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


#ifdef	COMMENT

/* unroll loops */
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

	    rs = llb_load(lifp->llbp, *lbindex, 
	        faddr,
	        instr,
	        auxbuffer[i].opexec,	/* original */
	    auxbuffer[i].opclass,	/* original */
	    auxbuffer[i].opexec,	/* transformed */
	    auxbuffer[i].opclass,	/* transformed */
	    auxbuffer[i].opclass,	/* branch type */
	    auxbuffer[i].src1, 
		auxbuffer[i].src2,
	        auxbuffer[i].src3,
		auxbuffer[i].src4,
		auxbuffer[i].src5,
	        auxbuffer[i].dst1,
		auxbuffer[i].dst2,
		auxbuffer[i].dst3,
	        auxbuffer[i].const1,
	        auxbuffer[i].const1_valid,
	        auxbuffer[i].pina,
	        auxbuffer[i].cpina,
	        auxbuffer[i].ncpin,
	        auxbuffer[i].pouta,
	        auxbuffer[i].cpouta) ;

/*    ldecode_printinstr(&auxbuffer[i]) ; */

#if	F_FPRINTF
	    if (rs<0)
	        printf("lifetch_unroll_loop: error loading llb ") ;
#endif

	    *lbindex+=1 ;
	    i+=1 ;
	}

	return 0 ;
}
/* end subroutine (lifetch_unroll_loop) */


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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
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

	    current_instr->instr = lifp->entry[index] ;
	    current_instr->ia = lifp->ilptr[index] ;
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2) {
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

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2) {
	    if (rs < 0)
	        eprintf("lifetch_sanitycheck: bad AS=%08lx i=%d rs=%d\n",
	            i,(lifp->lases + i),rs) ;
	}
#endif /* F_DEBUG */

	return rs ;
}
/* end subroutine (lifetch_sanitycheck) */



