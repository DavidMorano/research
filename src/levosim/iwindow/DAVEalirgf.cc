/* alirgf */

/* Levo Register file*/


#define	F_DEBUGS	0
#define	F_DEBUG		0


/* revision history :

	= 00/07/14, Alireza Khalafi

	This code was started basically from scratch !


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
#include	"mintinfo.h"
#include	"levoinfo.h"
#include	"alirgf.h"
#include	"lmipsregs.h"
#include	"las.h"



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

	for(i=0;i<NUM_OF_REGS;++i) {
		alirgfp->reg[i].tt = INT_MIN ;
		alirgfp->reg[i].d = 0;
	}

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

int
alirgf_commit(alirgfp)
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


int alirgf_read(p, lasp)
ALIRGF	*p;
LAS	*lasp;
{

	struct alirgf_entry *ep ;
	int	tt,i;
	int	s1,s2,d1,d2;


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
	
	return SR_OK ;
}


int alirgf_getval(p,a,vp)
ALIRGF	*p ;
int	a ;
uint	*vp ;
{


	if (p == NULL)
		return SR_FAULT ;

	if ((a < 0) || (a >= LMIPSREG_NREGS))
		return SR_INVALID ;

	*vp = p->reg[a].d ;
	return SR_OK ;
}
/* end subroutine (aliregf_getval) */


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



