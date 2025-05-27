/* iflags */

/* track the instruction flags for statistic keeping */


/* revision history:

	= 04/02/15, Dave Morano

	This code was started.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This object performs some rudimentary instruction flag
	accounting.


******************************************************************************/


#include	<sys/types.h>
#include	<cstring>

#include	<bio.h>

#include	"localmisc.h"

#include	"machine.h"
#include	"iflags.h"




extern double	percentll(ULONG,ULONG) ;





int iflags_init(ifp)
struct iflags	*ifp ;
{


	memset(ifp,0,sizeof(struct iflags)) ;

	return 0 ;
}

int iflags_proc(ifp,flags,class)
struct iflags	*ifp ;
int		flags, class ;
{


	if (flags & F_ICOMP)
	    ifp->icomp += 1 ;

	if (flags & F_CTRL)
	    ifp->ctrl += 1 ;

	if (flags & F_UNCOND)
	    ifp->uncond += 1 ;

	if (flags & F_COND)
	    ifp->cond += 1 ;

	if (flags & F_MEM)
	    ifp->mem += 1 ;

	if (flags & F_LOAD)
	    ifp->load += 1 ;

	if (flags & F_STORE)
	    ifp->store += 1 ;

	if (flags & F_DISP)
	    ifp->disp += 1 ;

	if (flags & F_RR)
	    ifp->rr += 1 ;

	if (flags & F_DIRECT)
	    ifp->direct += 1 ;

	if (flags & F_TRAP)
	    ifp->trap += 1 ;

	if (flags & F_LONGLAT)
	    ifp->longlat += 1 ;

	if (flags & F_DIRJMP)
	    ifp->dirjmp += 1 ;

	if (flags & F_INDIRJMP)
	    ifp->indirjmp += 1 ;

	if (flags & F_CALL)
	    ifp->call += 1 ;

	if (flags & F_FPCOND)
	    ifp->fpcond += 1 ;

	if (flags & F_IMM)
	    ifp->imm += 1 ;

	if ((flags & F_CTRL) && (flags & F_DIRJMP))
	    ifp->ctrl_dirjmp += 1 ;

	if ((flags & F_CTRL) && (flags & F_INDIRJMP))
	    ifp->ctrl_indirjmp += 1 ;

	if ((flags & F_CTRL) && (flags & F_CALL))
	    ifp->ctrl_call += 1 ;

	if ((flags & F_CTRL) && (flags & F_FPCOND))
	    ifp->ctrl_fpcond += 1 ;

/* all classes over the limit are placed at one above the limit */

	if ((class < 0) || (class > NUM_FU_CLASSES))
		class = NUM_FU_CLASSES ;

/* update classes (unrestricted) */

	ifp->class[class] += 1 ;

/* update the special control-flow-classes */

	if (flags & F_CTRL) 
		ifp->ctrl_class[class] += 1 ;

	return 0 ;
}

int iflags_free(ifp,fname)
struct iflags	*ifp ;
const char	fname[] ;
{
	bfile	tfile ;

	ULONG	ins = ifp->icount ;

	int	rs1, i ;


	rs1 = bopen(&tfile,fname,"wct",0666) ;

	if (rs1 >= 0) {

	    bprintf(&tfile,"= instruction type profile\n") ;

	    bprintf(&tfile,"ins=         %10llu\n",
		ins) ;

	    bprintf(&tfile,"icomp=       %10llu (%6.1f%%)\n",
	        ifp->icomp,percentll(ifp->icomp,ins)) ;

	    bprintf(&tfile,"fcomp=       %10llu (%6.1f%%)\n",
	        ifp->fcomp,percentll(ifp->fcomp,ins)) ;

	    bprintf(&tfile,"ctrl=        %10llu (%6.1f%%)\n",
	        ifp->ctrl,percentll(ifp->ctrl,ins)) ;

	    bprintf(&tfile,"uncond=      %10llu (%6.1f%%)\n",
	        ifp->uncond,percentll(ifp->uncond,ins)) ;

	    bprintf(&tfile,"cond=        %10llu (%6.1f%%)\n",
	        ifp->cond,percentll(ifp->cond,ins)) ;

	    bprintf(&tfile,"mem=         %10llu (%6.1f%%)\n",
	        ifp->mem,percentll(ifp->mem,ins)) ;

	    bprintf(&tfile,"load=        %10llu (%6.1f%%)\n",
	        ifp->load,percentll(ifp->load,ins)) ;

	    bprintf(&tfile,"store=       %10llu (%6.1f%%)\n",
	        ifp->store,percentll(ifp->store,ins)) ;

	    bprintf(&tfile,"disp=        %10llu (%6.1f%%)\n",
	        ifp->disp,percentll(ifp->disp,ins)) ;

	    bprintf(&tfile,"rr=          %10llu (%6.1f%%)\n",
	        ifp->rr,percentll(ifp->rr,ins)) ;

	    bprintf(&tfile,"direct=      %10llu (%6.1f%%)\n",
	        ifp->direct,percentll(ifp->direct,ins)) ;

	    bprintf(&tfile,"trap=        %10llu (%6.1f%%)\n",
	        ifp->trap,percentll(ifp->trap,ins)) ;

	    bprintf(&tfile,"longlat=     %10llu (%6.1f%%)\n",
	        ifp->longlat,percentll(ifp->longlat,ins)) ;

	    bprintf(&tfile,"dirjmp=      %10llu (%6.1f%%)\n",
	        ifp->dirjmp,percentll(ifp->dirjmp,ins)) ;

	    bprintf(&tfile,"indirjmp=    %10llu (%6.1f%%)\n",
	        ifp->indirjmp,percentll(ifp->indirjmp,ins)) ;

	    bprintf(&tfile,"call=        %10llu (%6.1f%%)\n",
	        ifp->call,percentll(ifp->call,ins)) ;

	    bprintf(&tfile,"fpcond=      %10llu (%6.1f%%)\n",
	        ifp->fpcond,percentll(ifp->fpcond,ins)) ;

	    bprintf(&tfile,"mm=          %10llu (%6.1f%%)\n",
	        ifp->imm,percentll(ifp->imm,ins)) ;

	    bprintf(&tfile,"c_dirjmp=    %10llu (%6.1f%%)\n",
	        ifp->ctrl_dirjmp,
	        percentll(ifp->ctrl_dirjmp,ins)) ;

	    bprintf(&tfile,"c_indirjmp=  %10llu (%6.1f%%)\n",
	        ifp->ctrl_indirjmp,
	        percentll(ifp->ctrl_indirjmp,ins)) ;

	    bprintf(&tfile,"c_call=      %10llu (%6.1f%%)\n",
	        ifp->ctrl_call,
	        percentll(ifp->ctrl_call,ins)) ;

	    bprintf(&tfile,"c_fpcond=    %10llu (%6.1f%%)\n",
	        ifp->ctrl_fpcond,
	        percentll(ifp->ctrl_fpcond,ins)) ;


	    for (i = 0 ; i < (NUM_FU_CLASSES + 1) ; i += 1) {

	    bprintf(&tfile,"class[%2u]=   %10llu (%6.1f%%)\n",i,
	        ifp->class[i],
	        percentll(ifp->class[i],ins)) ;

	    }

	    for (i = 0 ; i < (NUM_FU_CLASSES + 1) ; i += 1) {

	    bprintf(&tfile,"c_class[%2u]= %10llu (%6.1f%%)\n",i,
	        ifp->ctrl_class[i],
	        percentll(ifp->ctrl_class[i],ins)) ;

	    }

	    bprintf(&tfile,"\n") ;

	    bclose(&tfile) ;

	} /* end if (opened file) */

	return rs1 ;
}
/* end subroutine (iflags_free) */



