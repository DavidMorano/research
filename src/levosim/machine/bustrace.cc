/* bustrace */

/* Levo Bus Monitor */


#define	F_DEBUGS	0
#define	F_DEBUG		0
#define	F_SAFE		0


/* revision history :

	= 00/07/27, Dave Morano

	Module was originally written.


*/



/**************************************************************************

	This object module provides a trace function to listen
	to 'BUS' objects.  Any number of BUSes can be traced
	but they have to be ordered as an array of them.
	If yours multiple buses that you want to trace are not
	in an array, then instantiate more of these objects
	for as many individual buses you have.


**************************************************************************/


#define	BUSTRACE_MASTER	0


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>
#include	<math.h>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"bus.h"
#include	"lflowgroup.h"
#include	"bustrace.h"



/* local defines */

#define	BUSTRACE_MAGIC		0x96427312
#define	BUSTRACE_DISPLAYTT	-9999999



/* external subroutines */

#if	defined(SOLARIS) && (SOLARIS == 8)
extern double	sqrt(double) ;
#endif

extern char	*strwcpy(char *,const char *,int) ;


/* forward references */

static int	bustrace_busread(BUSTRACE *,struct proginfo *,LSIM *) ;





int bustrace_init(rfp,pip,mip,filespec,busp,n)
BUSTRACE	*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
char		filespec[] ;
BUS		*busp ;
int		n ;
{
	int	rs = SR_OK ;
	int	size ;

	char	filename[MAXPATHLEN + 1], *cp ;


	if (rfp == NULL)
	    return SR_FAULT ;

#if	F_DEBUGS
	    eprintf("bustrace_init: filespec=%s\n",
	        filespec) ;
#endif

	if ((filespec == NULL) || (filespec[0] == '\0'))
	    return SR_INVALID ;

	(void) memset(rfp,0,sizeof(BUSTRACE)) ;

	rfp->pip = pip ;
	rfp->mip = mip ;

	rfp->level = 2 ;
	if ((cp = strrchr(filespec,'=')) != NULL) {

	    strwcpy(filename,filespec,(cp - filespec)) ;

	    if (cfdeci((cp + 1),-1,&rfp->level) < 0)
	        rfp->level = 0 ;

	} else
	    strwcpy(filename,filespec,MAXPATHLEN) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("bustrace_init: filename=%s level=%d\n",
	        filename,rfp->level) ;
#endif


	rfp->buses = busp ;
	rfp->nbuses = n ;
	rfp->ss = NULL ;


	if (rfp->level > 0) {

	    rs = bopen(&rfp->tfile,filename,"wct",0666) ;

	    if (rs < 0)
	        goto bad0 ;

	    if (rfp->level > 1) {

	        bprintf(&rfp->tfile,
	            "clock    bus        mid "
			"t p       tt     addr     data\n") ;

	    } /* end if */

	    size = n * sizeof(struct bustrace_stats) ;
	    rs = uc_malloc(size,&rfp->ss) ;

	    if (rs < 0)
		goto bad1 ;

	    memset(rfp->ss,0,size) ;

	} /* end if */

	(void) memset(&rfp->s,0,sizeof(struct bustrace_stats)) ;


	rfp->c.clock = rfp->n.clock = 0 ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("bustrace_init: BUSES=%08lx n=%d\n",
	        rfp->buses,rfp->nbuses) ;
#endif


	rfp->magic = BUSTRACE_MAGIC ;
	return rs ;

/* bad things come here */
bad1:
	bclose(&rfp->tfile) ;

bad0:
	return rs ;
}
/* end subroutine (bustrace_init) */


int bustrace_free(rfp)
BUSTRACE	*rfp ;
{
	struct proginfo	*pip ;

	ULONG	tmp ;

	int	i ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if (rfp->magic != BUSTRACE_MAGIC)
	    return SR_NOTOPEN ;

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("bustrace_free: entered\n") ;
#endif


	if (rfp->level > 0) {

		ULONG	slots, n, np ;

		double	ftmp, fslots ;
		double	s1, s2 ;
		double	m1, m2 ;
		double	x, fnp, var ;
		double	p ;


/* add up stuff */

		(void) memset(&rfp->s,0,sizeof(struct bustrace_stats)) ;

		for (i = 0 ; i < rfp->nbuses ; i += 1) {

			rfp->ss[i].total = 
			    rfp->ss[i].data_total + rfp->ss[i].nodata_total ;

			rfp->s.total += rfp->ss[i].total ;
			rfp->s.data_total += rfp->ss[i].data_total ;
			rfp->s.nodata_total += rfp->ss[i].nodata_total ;
			rfp->s.stores_data += rfp->ss[i].stores_data ;
			rfp->s.stores_nodata += rfp->ss[i].stores_nodata ;
			rfp->s.nullifys_data += rfp->ss[i].nullifys_data ;
			rfp->s.nullifys_data += rfp->ss[i].nullifys_nodata ;

		} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("bustrace_free: total=%9lld\n",rfp->s.total) ;
#endif


/* print everything out */

	    bprintf(&rfp->tfile,
	        "----------------------------------------\n") ;

	    bprintf(&rfp->tfile,
	        "clocks                  %16lld\n", 
	        (rfp->c.clock + 1)) ;

	    bprintf(&rfp->tfile,
	        "buses                   %16d\n", 
	        rfp->nbuses) ;

	    slots = rfp->nbuses * (rfp->c.clock + 1) ;
	    fslots = ((double) slots) ;
	    ftmp = fslots / (1024.0 * 1024.0) ;
	    bprintf(&rfp->tfile,
	        "bus slots               %16lld (%.3f-Mi)\n", 
		slots,ftmp) ;


	    ftmp = 100.0 * ((double) rfp->s.total) / fslots ;
	    bprintf(&rfp->tfile,
	        "total transactions             %9lld (%5.1f%% utilization)\n",
	        rfp->s.total,ftmp) ;


		s1 = (double) rfp->s.total ;
		s2 = 0.0 ;
		np = 0 ;
		for (i = 0 ; i < rfp->nbuses ; i += 1) {

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("bustrace_free: total[%d]=%lld\n",
			i,rfp->ss[i].total) ;
#endif

			x = (double) i ;
			p = (double) rfp->ss[i].total ;
			np += rfp->ss[i].total ;
			s2 += (p * x * x) ;

		} /* end for */

		fnp = (double) np ;
		m1 = s1 / fnp ;
		m2 = s2 / fnp ;
		var = m2 - (m1 * m1) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4) {
	    eprintf(
		"bustrace_free: s1=%.2f s2=%.2f m1=%.2f m2=%.2f\n",
		s1,s2,m1,m2) ;
	    eprintf(
		"bustrace_free: var=%.2f stddev=%.2f\n",
			var,sqrt(var)) ;
	}
#endif


	    bprintf(&rfp->tfile,
	        "mean transactions per bus      %11.1f (stddev %.1f)\n",
	        m1,sqrt(var)) ;


	    tmp = rfp->s.stores_data + rfp->s.stores_nodata ;
	    bprintf(&rfp->tfile,
	        "STORE transactions (total)     %9lld\n",
	        tmp) ;

	    bprintf(&rfp->tfile,
	        "STORE transactions (data)      %9lld\n",
	        rfp->s.stores_data) ;

	    bprintf(&rfp->tfile,
	        "STORE transactions (no data)   %9lld\n",
	        rfp->s.stores_nodata) ;


	    tmp = rfp->s.nullifys_data + rfp->s.nullifys_nodata ;
	    bprintf(&rfp->tfile,
	        "NULLIFY transactions (total)   %9lld\n",
	        tmp) ;

	    bprintf(&rfp->tfile,
	        "NULLIFY transactions (data)    %9lld\n",
	        rfp->s.nullifys_data) ;

	    bprintf(&rfp->tfile,
	        "NULLIFY transactions (no data) %9lld\n",
	        rfp->s.nullifys_nodata) ;


#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel > 4)
	        eprintf("bustrace_free: closing\n") ;
#endif

	    bclose(&rfp->tfile) ;

	} /* end if (summary or full) */

	if (rfp->ss != NULL)
		free(rfp->ss) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 4)
	    eprintf("bustrace_free: exiting\n") ;
#endif

	rfp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (bustrace_free) */


/* perform the combinatorial computations */
int bustrace_comb(rfp,phase)
BUSTRACE		*rfp ;
int		phase ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	rs = SR_OK ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if (rfp->magic != BUSTRACE_MAGIC)
	    return SR_NOTOPEN ;

	pip = rfp->pip ;
	mip = rfp->mip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("bustrace_comb: entered ck=%lld ph=%d\n",
	        mip->clock,phase) ;
#endif

	switch (phase) {

	case 0:
	    rfp->n.clock = rfp->c.clock + 1 ;
	    break ;

	case 1:
	    break ;

	case 2:
	    bustrace_busread(rfp,pip,mip) ;

	    break ;

	case 3:
	    break ;

	case 4:
	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (bustrace_comb) */


/* handle a clock transition */
int bustrace_clock(rfp)
BUSTRACE	*rfp ;
{
	struct proginfo	*pip ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if (rfp->magic != BUSTRACE_MAGIC)
	    return SR_NOTOPEN ;

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("bustrace_clock: entered\n") ;
#endif

	rfp->c = rfp->n ;		/* copy everything */

	return SR_OK ;
}
/* end subroutine (bustrace_clock) */


/* we have a request for a control operation */
int bustrace_control(rfp,flags)
BUSTRACE	*rfp ;
int		flags ;
{
	struct proginfo	*pip ;


	if (rfp == NULL)
	    return SR_FAULT ;

	if (rfp->magic != BUSTRACE_MAGIC)
	    return SR_NOTOPEN ;

	pip = rfp->pip ;

#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("bustrace_control: entered f=%04x\n",
	        flags) ;
#endif


	rfp->f.active = (flags & 1) ? TRUE : FALSE ;

	return SR_OK ;
}
/* end subroutine (bustrace_control) */



/* PRIVATE SUBROUTINES */



/* read our incoming bus */
static int bustrace_busread(rfp,pip,mip)
BUSTRACE	*rfp ;
struct proginfo	*pip ;
LSIM		*mip ;
{
	LFLOWGROUP	fg ;

	int	rs = SR_OK, i ;
	int	cmid ;

	char	ttbuf[24] ;
	char	seqbuf[24] ;


#if	F_MASTERDEBUG && F_DEBUG && 0
	if (pip->debuglevel > 1)
	    eprintf("bustrace_busread: entered\n") ;
#endif

	for (i = 0 ; i < rfp->nbuses ; i += 1) {

	    if ((rs = bus_read(rfp->buses + i,&fg)) > 0) {

	        cmid = bus_curmaster(rfp->buses + i) ;

	        if (rfp->level > 1) {

	            if (fg.tt < BUSTRACE_DISPLAYTT)
	                strcpy(ttbuf,"-") ;

	            else
	                ctdeci(ttbuf,-1,fg.tt) ;

	            if (fg.seq > 999)
	                strcpy(seqbuf,"+") ;

	            else
	                ctdeci(seqbuf,-1,fg.seq) ;

	        } /* end if (full output) */

#if	F_MASTERDEBUG && F_DEBUG && 0
	        if (pip->debuglevel > 4) {
	            eprintf("bustrace_busread: tt=>%s<\n",ttbuf) ;
	            eprintf("bustrace_busread: seq=%d seqbuf=%s >%-3s<\n",
	                fg.seq,seqbuf,seqbuf) ;
	        }
#endif

	        if (fg.dp) {

	            rfp->ss[i].data_total += 1 ;
	            if (fg.trans == LFLOWGROUP_TSTORE)
	                rfp->ss[i].stores_data += 1 ;

	            else if (fg.trans == LFLOWGROUP_TNULLIFY)
	                rfp->ss[i].nullifys_data += 1 ;

	            if (rfp->level > 1)
	                bprintf(&rfp->tfile,
	                    "%08llx %08lx %5d %d %d %8s:%-3s %08x %08x\n",
	                    rfp->c.clock,rfp->buses + i,
	                    cmid,
	                    fg.trans, fg.path,
	                    ttbuf,seqbuf,
	                    fg.addr,fg.dv) ;

	        } else {

	            rfp->ss[i].nodata_total += 1 ;
	            if (fg.trans == LFLOWGROUP_TSTORE)
	                rfp->ss[i].stores_nodata += 1 ;

	            else if (fg.trans == LFLOWGROUP_TNULLIFY)
	                rfp->ss[i].nullifys_nodata += 1 ;

	            if (rfp->level > 1)
	                bprintf(&rfp->tfile,
	                    "%08llx %08lx %5d %d %d %8s:%-3s %08x\n",
	                    rfp->c.clock,rfp->buses + i,
	                    cmid,
	                    fg.trans, fg.path,
	                    ttbuf,seqbuf,
	                    fg.addr) ;

	        }

	    } /* end if (read something) */

	} /* end for */

	return SR_OK ;
}
/* end subroutine (bustrace_busread) */



