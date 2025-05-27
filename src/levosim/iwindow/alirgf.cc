/* alirgf */

/* Levo Register file */


#define	F_DEBUGS	0
#define	F_DEBUG		1
#define	F_EXTRAZEWRO	1		/* do some extra zeroing */


/* revision history :

	= 00/07/14, Alireza Khalafi

	This code was started basically from scratch !


	= 01/07/20, Dave Morano

	I added the extra zeroing of LAS operands on a read
	operation.


	= 01/07/24, Dave Morano

	I added the extra subroutine to write individual registers.
	Its defition does not cause an object definition loop either
	(like 'alirgf_write()' does).


*/



/**************************************************************************

	This object module provides the functions for a Levo Load
	Buffer.


**************************************************************************/


#include	<sys/types.h>
#include	<climits>
#include	<cstdlib>
#include	<cstring>
#include	<assert.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lmipsregs.h"
#include	"alirgf.h"

#ifdef	COMMENT
#include	"las.h"
#endif



/* local defines */



/* external subroutines */


/* forward references */






/* initialize this object */
int alirgf_init(alirgfp,pip,mip,lip,stackaddr)
ALIRGF		*alirgfp ;
struct proginfo	*pip ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
uint		stackaddr ;
{
	int	rs, i ;
	int	bsize=0;
	int	numpred=0;
	int	size ;


	if (alirgfp == NULL)
		return SR_FAULT ;

	alirgfp->pip = pip ;
	alirgfp->mip = mip ;
	alirgfp->lip = lip ;

	for (i = 0 ; i < NUM_OF_REGS ; i += 1) {

		alirgfp->reg[i].tt = INT_MIN ;
		alirgfp->reg[i].d = 0;

	}

#if	F_DEBUG
	if (pip->debuglevel >= 5)
	eprintf("alirgf_init: initial SP=%d\n",stackaddr) ;
#endif

	/* temporary initailization */
	alirgfp->reg[29].d = stackaddr ;

	return SR_OK ;
}
/* end subroutine (alirgf_init) */


/* free up this object */
int alirgf_free(alirgfp)
ALIRGF	*alirgfp ;
{
	struct proginfo	*pip ;


	if (alirgfp == NULL)
		return SR_FAULT ;

	pip = alirgfp->pip ;

/* free up whatever we allocated */

	return SR_OK ;
}
/* end subroutine (alirgf_free) */


/* do our combinatorial stuff */
int alirgf_comb(alirgfp,phase)
ALIRGF	*alirgfp ;
int	phase ;
{
	struct proginfo	*pip ;


	pip = alirgfp->pip ;

	switch (phase) {

	case 1:
	case 2:
		break ;

	case 3:

		break ;

	} /* end switch */


	return SR_OK ;
}
/* end subroutine (alirgf_comb) */


int alirgf_commit(alirgfp)
ALIRGF	*alirgfp ;
{
	int i;
	for(i=0;i<NUM_OF_REGS;++i)
		alirgfp->reg[i].tt = INT_MIN ;
}


/* we just got clocked */
int alirgf_clock(alirgfp)
ALIRGF	*alirgfp ;
{
	int i;
	struct proginfo	*pip ;

	pip = alirgfp->pip ;



/*	alirgfp->c = alirgfp->n;  */

	return SR_OK ;
}
/* end subroutine (alirgf_clock) */


/* take a request for a machine shift */
int alirgf_shift(alirgfp)
ALIRGF	*alirgfp ;
{


	return SR_OK ;
}
/* end subroutine (alirgf_shift) */


#ifdef	COMMENT

int 
alirgf_write(p, lasp)
ALIRGF	*p;
LAS	*lasp;
{
	struct alirgf_entry	*ep ;
	int	i,j;
	int	s1,s2,d1,d2;


	d1 = lasp->c.dst1.a;
	d2 = lasp->c.dst2.a;
	assert(d1 < NUM_OF_REGS);
	assert(d2 < NUM_OF_REGS);

/*
	if(d1 > 0 && p->reg[d1].tt < lasp->c.tt) {
		p->reg[d1].d = lasp->c.dst1.d;
		p->reg[d1].tt = lasp->c.tt;
	}

	if(d2 > 0 && p->reg[d2].tt < lasp->c.tt) {
		p->reg[d2].d = lasp->c.dst2.d;
		p->reg[d2].tt = lasp->c.tt;
	}
*/

	if(d1 > 0) {
		p->reg[d1].d = lasp->c.dst1.val;
		p->reg[d1].tt = lasp->c.tt;
	}

	if(d2 > 0) {
		p->reg[d2].d = lasp->c.dst2.val;
		p->reg[d2].tt = lasp->c.tt;
	}


	return SR_OK ;
}
/* end subroutine (alirgf_write) */


/* read out the register values that we want */
int alirgf_read(p, lasp)
ALIRGF	*p;
LAS	*lasp;
{
	struct proginfo	*pip ;

	struct alirgf_entry *ep ;

	int	tt,i;
	int	s1,s2,d1,d2;


	pip = p->pip ;

#if	F_EXTRAZERO

	lasp->n.src1.val = 0 ;
	lasp->n.src2.val = 0 ;
	lasp->n.src3.val = 0 ;
	lasp->n.src4.val = 0 ;
	lasp->n.dst1.val = 0 ;
	lasp->n.dst2.val = 0 ;
	lasp->n.dst3.val = 0 ;

#endif /* F_EXTRAZERO */

	tt = lasp->c.tt;

	s1 = lasp->c.src1.a;
	s2 = lasp->c.src2.a;
	d1 = lasp->c.dst1.a;
	d2 = lasp->c.dst2.a;


	if(s1 > 0) lasp->n.src1.val = p->reg[s1].d;
	else if(s1 == 0) lasp->n.src1.val = 0;

	if(s2 > 0) lasp->n.src2.val = p->reg[s2].d;
	else if(s2 == 0) lasp->n.src2.val = 0;

	if(d1 > 0) lasp->n.dst1.val = p->reg[d1].d;
	else if(d1 == 0) lasp->n.dst1.val = 0;

	if(d2 > 0) lasp->n.dst2.val = p->reg[d2].d;
	else if(d2 == 0) lasp->n.dst2.val = 0;
	
#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 2)
	eprintf("alirgf_read: dst1=%08x\n",lasp->n.dst1.val) ;
#endif

	return SR_OK ;
}
/* end subroutine (alirgf_read) */

#endif /* COMMENT */


/* retrieve a single value */
int alirgf_getval(p,a,vp)
ALIRGF	*p ;
uint	a ;
uint	*vp ;
{
	struct proginfo	*pip ;


	if (p == NULL)
		return SR_FAULT ;

	pip = p->pip ;
	if (a >= LMIPSREG_NREGS)
		return SR_INVALID ;

	if (vp == NULL)
		return SR_FAULT ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	eprintf("alirgf_getval: a=%d dv=%08x\n",
		a,p->reg[a].d) ;
#endif

	*vp = p->reg[a].d ;
	return SR_OK ;
}
/* end subroutine (aliregf_getval) */


/* retrieve a register (value and time-tag) */
int alirgf_getval2(p,a,tp,vp)
ALIRGF	*p ;
uint	a ;
int	*tp ;
uint	*vp ;
{
	struct proginfo	*pip ;


	if (p == NULL)
		return SR_FAULT ;

	pip = p->pip ;
	if (a >= LMIPSREG_NREGS)
		return SR_INVALID ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	eprintf("alirgf_getval2: tt=%d a=%d dv=%08x\n",
		p->reg[a].tt,a,p->reg[a].d) ;
#endif

	if (tp != NULL)
		*tp = p->reg[a].tt ;

	if (vp != NULL)
		*vp = p->reg[a].d ;

	return SR_OK ;
}
/* end subroutine (aliregf_getval2) */


/* write a register */
int alirgf_writereg(p,a,tt,dv)
ALIRGF	*p;
uint	a ;
int	tt ;
uint	dv ;
{
	struct proginfo	*pip ;


	if (p == NULL)
		return SR_FAULT ;

	pip = p->pip ;
	if (a >= LMIPSREG_NREGS)
		return SR_INVALID ;

	if (a > 0) {

		p->reg[a].d = dv ;
		p->reg[a].tt = tt ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	eprintf("alirgf_writereg: tt=%d a=%d dv=%08x\n",
		tt,a,dv) ;
#endif

	} /* end if */

	return SR_OK ;
}
/* end subroutine (alirgf_write) */


int 
alirgf_print(p)
ALIRGF	*p;
{
	int i;

	for(i=0;i<NUM_OF_REGS;++i) {
		if(p->reg[i].d != 0) {
			printf("Reg[%d] = 0x%x\n", i,p->reg[i].d) ;

		}
	}
}



