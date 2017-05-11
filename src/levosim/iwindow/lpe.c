/* lpe */

/* Levo Processing Element */


#define	F_DEBUGS	0
#define	F_DEBUG		1		/* switchable debug print-outs */
#define	F_SAFE		1
#define	F_SAFE2		1
#define	F_IGNOREFAULTS	1		/* ignore i-execution faults */
#define	F_FPRINTF	1		/* fprintf() ? */
#define	F_SRC5		1		/* we have 5 source input operands ? */


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written.


	= 00/06/04, Alireza Khalafi

	I took over this code at the beginning of Levo IV.


	= 01/03/31, Dave Morano

	I fixed a bad bug that was causing crashes.  The bug was that
	there was a fixed size of the 'waitlist' and 'delvlist'
	arrays.


	= 01/06/22, Dave Morano

	I created the compile-time flag 'F_IGNOREFAULTS' so that the
	rest of the simulator will ignore any instruction faults for
	now.  This was needed because the AS unit does not properly do
	anything useful with the fault indications !  The AS unit needs
	to observe thses in the future.  For now, we just return any
	destination operand on a fault and hope that it is not a
	problem when the speculation of instrcutions (presumably the
	cause of the fault) comes to an end and commitment approaches.


	= 01/09/26, Dave Morano

	Added stuff for an XML output state trace.


*/

/* TO DO 
	reset the tracknumbr at column load time 
*/


/**************************************************************************

	This object module provides the functions for a Levo Processing
	Element.


**************************************************************************/


#define	LPE_MASTER	0


#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"xmlinfo.h"

#include	"levoinfo.h"
#include	"lpe.h"
#include	"opclass.h"		/* instructions classes */
#include	"lexecop.h"		/* instructions operations */
#include	"lexec.h"



/* local defines */

#define	LPE_MAGIC	0x85731895



/* external subroutines */


/* forward references */

static int	lpe_phase2(LPE *,struct proginfo *,struct levoinfo *) ;
static int	lpe_handleshift(LPE *,struct proginfo *,struct levoinfo *) ;
static int	lpe_exec(LPE *,struct proginfo *,struct lpe_waitlist *) ;






int lpe_init(lpep,pip,mip,lip)
LPE		*lpep ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK ;
	int	n, i ;
	int	size ;


	if (lpep == NULL)
	    return SR_FAULT ;

	(void) memset(lpep,0,sizeof(LPE)) ;

	lpep->pip = pip ;
	lpep->mip = mip ;
	lpep->lip = lip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lpe_init: entered a=%08lx\n",lpep) ;
#endif

	lpep -> tracknumber = 0 ;
	lpep -> num_entr_in_dlist = 0 ;
	lpep -> num_entr_in_wlist = 0 ;

	n = (lip->nasrpsg * LEVOINFO_NASWPSG) * 20 + 100 ;

	size = n * sizeof(struct lpe_waitlist) ;
	rs = uc_malloc(size,(void **) &lpep->waitlist) ;

	if (rs < 0)
	    goto bad0 ;

	size = n * sizeof(struct lpe_waitlist) ;
	rs = uc_malloc(size,(void **) &lpep->delvlist) ;

	if (rs < 0)
	    goto bad1 ;

	lpep->nentries = n ;

	for (i = 0 ; i < n ; i += 1) {

	    lpep->delvlist[i].tracknumber = 0 ;
	    lpep->waitlist[i].tracknumber = 0 ;

	}

/* we're out of here ! :-) */

	lpep->magic = LPE_MAGIC ;
	return rs ;

/* bad things */
bad1:
	free(lpep->waitlist) ;

bad0:
	return rs ;
}
/* end subroutine (lpe_init) */


/* free up this object */
int lpe_free(lpep)
LPE	*lpep ;
{
	int	i ;


#if	F_MASTERDEBUG && F_SAFE
	if (lpep == NULL)
	    return SR_FAULT ;

	if (lpep->magic != LPE_MAGIC)
	    return SR_NOTOPEN ;
#endif

#if	F_DEBUGS
	eprintf("lpe_free: entered\n") ;
#endif

	lpep->magic = 0 ;

	if (lpep->waitlist != NULL)
	    free(lpep->waitlist) ;

	if (lpep->delvlist != NULL)
	    free(lpep->delvlist) ;


	return SR_OK ;
}
/* end subroutine (lpe_free) */


/* do out combinatorial stuff */
int lpe_comb(lpep,phase)
LPE	*lpep ;
int	phase ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (lpep == NULL)
	    return SR_FAULT ;

	if (lpep->magic != LPE_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = lpep->pip ;
	lip = lpep->lip ;

	switch (phase) {

	case 2:
	    rs = lpe_phase2(lpep,pip,lip) ;

	    break ;

	case 3:
	    rs = lpe_handleshift(lpep,pip,lip) ;

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (lpe_comb) */


/* clock us */
int lpe_clock(lpep)
LPE	*lpep ;
{
	struct proginfo	*pip ;

	int i ;


#if	F_MASTERDEBUG && F_SAFE
	if (lpep == NULL)
	    return SR_FAULT ;

	if (lpep->magic != LPE_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = lpep->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("lpe_clock: entered\n") ;
#endif

/* do our clock thing */

	for (i = 0 ; i < lpep->num_entr_in_dlist ; i += 1) {

	    if (lpep->delvlist[i].delvtime > 0)
	        lpep->delvlist[i].delvtime -= 1 ;

	} /* end for */

#ifdef	COMMENT
	lpe_print(lpep) ;
#endif

	return SR_OK ;
}
/* end subroutine (lpe_clock) */


/* get shifted */
int lpe_shift(lpep)
LPE	*lpep ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (lpep == NULL)
	    return SR_FAULT ;

	if (lpep->magic != LPE_MAGIC)
	    return SR_NOTOPEN ;
#endif

	lpep->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (lpe_shift) */


/* request an execution */
int lpe_req2exec(lpep, instpack, priority, tracknumberp)
LPE		*lpep ;
struct lpe_data	*instpack ;
int		priority ;
int		*tracknumberp ;
{
	struct proginfo	*pip ;

	struct lpe_waitlist newentry ;


#if	F_MASTERDEBUG && F_SAFE
	if (lpep == NULL)
	    return SR_FAULT ;

	if (lpep->magic != LPE_MAGIC)
	    return SR_NOTOPEN ;

#endif /* F_SAFE */

	pip = lpep->pip ;

	*tracknumberp = -1 ;
	if (lpep->num_entr_in_wlist > (lpep->nentries - 5)) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("lpe_req2exec: entry overflow nentries=%d active=%d\n",
	        lpep->nentries,lpep->num_entr_in_wlist) ;
#endif

	    return SR_OVERFLOW ;
	}

	*tracknumberp = lpep->tracknumber ;
	lpep->tracknumber = (lpep->tracknumber + 1) & INT_MAX ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("lpe_req2exec: tn=%d nentries=%d\n",
	        *tracknumberp,lpep->num_entr_in_wlist) ;
#endif

	newentry.priority = priority ;
	newentry.instpack = *instpack ;
	newentry.tracknumber = *tracknumberp ;
	lpep->waitlist[lpep->num_entr_in_wlist++] = newentry ;

#if	F_MASTERDEBUG && F_SAFE2
	if (lpep->num_entr_in_wlist > lpep->nentries) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 2)
	        eprintf("lpe_req2exec: bad request LPE=%08lx\n",lpep) ;
#endif

	    return SR_NOTSUP ;
	}
#endif /* F_SAFE2 */

	return SR_OK ;
}
/* end subroutine (lpe_req2exec) */


/* is the instruction executed yet ? */
int lpe_isexecuted(lpep, instp, tracknumber)
LPE * lpep ;
struct lpe_data * instp ;
int tracknumber ;
{
	struct proginfo	*pip ;

	int	rs = SR_NOTFOUND ;
	int	i,j ;


#if	F_MASTERDEBUG && F_SAFE
	if (lpep == NULL)
	    return SR_FAULT ;

	if (lpep->magic != LPE_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = lpep->pip ;

/* search for the specified execution request */

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 2)
	                eprintf("lpe_isexecuted: searching for tn=%d\n",
	                    tracknumber) ;
#endif

	for (i = 0 ; i < lpep->num_entr_in_dlist ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 2)
	                eprintf("lpe_isexecuted: dlist> tn=%d\n",
	                    lpep->delvlist[i].tracknumber) ;
#endif

	    if (lpep->delvlist[i].tracknumber == tracknumber) {

	        if ((lpep->delvlist[i]).delvtime <= 0) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 2)
	                eprintf("lpe_isexecuted: execution completed tn=%d\n",
	                    tracknumber) ;
#endif

	            *instp = lpep -> delvlist[i].instpack ;
	            for (j=i; j<lpep->num_entr_in_dlist-1; j += 1)
	                lpep->delvlist[j] = lpep->delvlist[j+1] ;

	            lpep->num_entr_in_dlist -= 1 ;
	            return SR_OK ;

	        } else {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 2)
	                eprintf(
	                    "lpe_isexecuted: execution "
	                    "in progress delvtime=%d\n",
	                    lpep->delvlist[i].delvtime) ;
#endif

	            return SR_INPROGRESS ;
	        }

	    } /* end if (found it) */

	} /* end for (searching) */


#if	F_MASTERDEBUG && F_SAFE2

/* is it waiting for execution ? */

	if (rs == SR_NOTFOUND) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 2)
	                eprintf("lpe_isexecuted: not in delivery list\n") ;
#endif

	for (i = 0 ; i < lpep->num_entr_in_wlist ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel > 2)
	                eprintf("lpe_isexecuted: wlist> tn=%d\n",
	                    lpep->waitlist[i].tracknumber) ;
#endif

	    if (lpep->waitlist[i].tracknumber == tracknumber)
		break ;

	} /* end for */

	if (i < lpep->num_entr_in_wlist)
		rs = SR_INPROGRESS ;

	} /* end if (not previously found) */

#else

	rs = SR_INPROGRESS ;

#endif /* F_SAFE2 */


#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel > 2) && (rs == SR_NOTFOUND))
	    eprintf("lpe_isexecuted: request not found tn=%d rs=%d\n",
	        tracknumber,rs) ;
#endif

	return rs ;
}
/* end subroutine (lpe_isexecuted) */


/* XML stuff */
int lpe_xmlinit(fp,xip)
LPE	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}


int lpe_xmlout(fp,xip)
LPE	*fp ;
XMLINFO	*xip ;
{


	bprintf(&xip->xmlfile,"<lpe>\n") ;

	bprintf(&xip->xmlfile,"<uid>%p</uid>\n",fp) ;


	bprintf(&xip->xmlfile,"</lpe>\n") ;

	return SR_OK ;
}


int lpe_xmlfree(fp,xip)
LPE	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}
/* end subroutine (lpe_xmlfree) */



/* PRIVATE SUBROUTINES */



/* do our phase 2 combinatorial stuff */
static int lpe_phase2(lpep,pip,lip)
LPE		*lpep ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	struct lpe_waitlist wentry ;

	int	rs = SR_OK ;
	int maxpriority ;
	int index ;
	int i ;


	if (lpep->num_entr_in_wlist == 0)
	    return SR_OK ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("lpe_phase2: got some stuff in waiting list n=%d\n",
	        lpep->num_entr_in_wlist) ;
#endif

/* picks up the instrustion with the lowest priority */

	maxpriority = 900000 ;
	index = 0 ;
	for (i=0; i< lpep->num_entr_in_wlist; i += 1) {

	    if (lpep->waitlist[i].priority < maxpriority) {

	        maxpriority = lpep->waitlist[i].priority ;
	        index = i ;
	    }

	} /* end for */

	wentry = lpep->waitlist[index] ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("lpe_phase2: executing tn=%d\n",
	        wentry.tracknumber) ;
#endif

/* remove the entry from the waiting list */

	for (i = index ; i < (lpep->num_entr_in_wlist - 1) ; i += 1)
	    lpep->waitlist[i] = lpep->waitlist[i+1] ;

	lpep->num_entr_in_wlist -= 1 ;

/* do the actual execution of this entry, pick up the execution delay */

	rs = lpe_exec(lpep,pip,&wentry) ;

#if	(! F_IGNOREFAULTS)
	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("lpe_phase2: lpe_exec() rs=%d\n",rs) ;
#endif

	    rs = SR_INVALID ;
	}
#else
	rs = SR_OK ;
#endif /* (! F_IGNOREFAULTS) */

/* add the entry to the delivery list */

	lpep->delvlist[lpep->num_entr_in_dlist++] = wentry ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	    eprintf("lpe_phase2: exiting rs=%d executed tn=%d\n",
	        rs,wentry.tracknumber) ;
#endif

	return rs ;
}
/* end subroutine (lpe_phase2) */


/* handle doing a shift for us */
static int lpe_handleshift(lpep,pip,lip)
LPE		*lpep ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK ;


	if (! lpep->f.shift)
	    return rs ;

	lpep->f.shift = FALSE ;

/* there do not seem to be any time-tags in the machine state here ! */


	return rs ;
}
/* end subroutine (lpe_handleshift) */


/* execute the thing */
int lpe_exec(lpep,pip,wentry)
LPE			*lpep ;
struct proginfo		*pip ;
struct lpe_waitlist	*wentry ;
{
	struct lexec_instr	li ;

	struct lexec_src	ls ;

	struct lexec_dst	ld ;

	struct lpe_data	*d ;

	uint	ia ;

	int	rs = SR_OK ;
	int	f_earlyexit = TRUE ;	/* leave early all of the time ! */
	int	f_nullify, f_jump ;


	d = &wentry -> instpack ;

#ifdef	COMMENT
	rs = lexec(d->opexec, 
	    &d->src1, &d->src2, &d->src3, &d->src4, 
	    &d->dst1, &d->dst2, &d->dst3,
	    &d->cnst, &d->bcnd, &d->mema, 
	    &d->delay,&ia,&f_jump,&f_nullify) ;
#endif /* COMMENT */

/* decoded instruction information */

	li.ia = ia ;
	li.opexec = d->opexec ;
	li.constant = d->cnst ;

/* source operands */

	ls.s1 = d->src1 ;
	ls.s2 = d->src2 ;
	ls.s3 = d->src3 ;
	ls.s4 = d->src4 ;

#if	F_SRC5
	ls.s5 = d->src5 ;
#endif

/* do it */

	rs = lexec(&li,&ls,&ld) ;

/* get the results (destination operands) */

	d->dst1 = ld.d1 ;
	d->dst2 = ld.d2 ;
	d->dst3 = ld.d3 ;

	d->bcnd = ld.f.taken ;
	d->delay = ld.delay ;


	if (rs < 0) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("lpe_exec: BAD lexec() rs=%d\n",rs) ;
#endif

	    rs = SR_INVALID ;
	    f_earlyexit = TRUE ;
	}

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2) {
	    eprintf("lpe_exec: op=%d lexec() rs=%d\n",
	        d->opexec,rs) ;
	    eprintf("lpe_exec: src1=%08x src2=%08x "
	        "cnst=%08x dst1=%08x\n",
	        d->src1,d->src2,d->cnst,d->dst1) ;
	}
#endif

	wentry->delvtime = d->delay ;
	if (f_earlyexit)
	    return rs ;

#if	F_MASTERDEBUG && F_FPRINTF
	printf("lexec is called from PE\n") ;
#endif

	if (d->opexec == LEXECOP_ADD) {

	    d -> dst1 = d->src1 + d->src2 ;

	} else if(d->opexec == LEXECOP_SUB) {

	    d->dst1 = d->src1 - d->src2 ;

	} else if(d->opexec == LEXECOP_LW) {

#ifdef	COMMENT
	    d->dst1 = d->src1 + d->src2 ;
#endif /* COMMENT */

	    d->mema = d->src1 + d->src2 ;

	} else if(d->opexec == LEXECOP_SW) {

#ifdef	COMMENT
	    d->dst1 = d->src1 + d->src2 ;
#endif

	    d->mema = d->src1 + d->src2 ;
	}

	wentry -> delvtime = 2 ;

	return rs ;
}
/* end subroutine (lpe_exec) */


/* print out some stuff */
static int lpe_print(LPE * lpep)
{
	int i ;


#if	F_MASTERDEBUG && F_FPRINTF

	printf("num of entr in delivery list = %d\n", lpep->num_entr_in_dlist) ;
	printf("num of entr in wait list = %d\n", lpep->num_entr_in_wlist) ;
	printf("tracknumber = %d\n", lpep->tracknumber) ;
	printf("wait list ------------\n") ;
	for(i=0;i<lpep->num_entr_in_wlist; ++i)
	    lpe_print_list(&lpep->waitlist[i]) ;
	printf("delivery list ------------\n") ;
	for(i=0;i<lpep->num_entr_in_dlist; ++i)
	    lpe_print_list(&lpep->delvlist[i]) ;

#endif /* F_FPRINTF */

	return 0 ;
}


int lpe_print_list(struct lpe_waitlist * wlp) 
{

	struct lpe_data * lp ;
	lp = &wlp->instpack ;

#if	F_MASTERDEBUG && F_FPRINTF

	printf("priority=%d, delvtime=%d, tracknumber=%d\n",
	    wlp->priority, wlp->delvtime, wlp->tracknumber) ;
	printf("opexec=%d, src1=%d, src2=%d, dst1=%d, mem=%u\n", 
	    lp->opexec, lp->src1, lp->src2, lp->dst1, lp->mema) ;

#endif /* F_FPRINTF */

	return 0 ;
}



