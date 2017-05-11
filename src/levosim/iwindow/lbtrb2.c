/* lbrtb */

/* Levo Branch Tracking Buffer */


#define	F_DEBUGS	0


/* revision history :

	= 00/07/04, Alireza Khalafi

	Module was originally written.


*/


/**************************************************************************

	This object module provides the functions for a Levo Branch 
	Tracking Buffer.



**************************************************************************/


#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lbtrb2.h"



/* local defines */



/* external subroutines */


/* forward referecens */






int lbtrb_init(lbtrbp,pip,mip,lip)
LBTRB		*lbtrbp ;
struct proginfo	*pip ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
{
	int	rs ;

	lbtrbp->pip = pip ;
	lbtrbp->mip = mip ;
	lbtrbp->lip = lip ;

	lbtrbp->head = (lbtrb_entry **) malloc(LBTRB_BUFF_SIZE*sizeof(lbtrb_entry *));
	if(lbtrbp->head == NULL)
		return SR_FAILURE;

	for(i=0; i < LBTRB_BUFF_SIZE; ++i) {
		if(!(lbtrb->head[i] = (lbtrb_entry *) malloc(sizeof(lbtrb_entry));
			return SR_FAILURE;
	}

	lbtrb->num_of_entries = 0;
	return SR_OK ;
}


int lbtrb_free(lbtrbp)
LBTRB	*lbtrbp ;
{


	return SR_OK ;
}
/* end subroutine (lpe_free) */


int lbtrb_clock(lbtrbp)
LBTRB	*lbtrbp ;
{
	int i;

}


int lbtrb_comb(lbtrbp,phase)
LBTRB	*lbtrbp ;
int	phase ;
{

}


int lbtrb_shift(lbtrbp)
LBTRB	*lbtrbp ;
{


}

int
lbtrb_assign_predicate(lbtrbp, inst)
LBTRB   *lbtrbp;
lbtrb_inst * inst;   /* needs to be replaced with whatever structure the decoding stage uses */
{

	while (there is a copy of inst address found in btrb)
		if(inst is branch)
			special case
			create predicate and canceling
		else
			remove the branch from buff
			create new predicate
	else if(inst is a branch)
		insert it in the btrb
		create a new predicate and canceling predicate
	else
		assing the current predicate reg address to the inst.


	
	if(inst->type == BRANCH) {
		inst->

	}


}




/* INTERNAL SUBROUTINES */









