/* lbrtb */

/* Levo Branch Tracking Buffer */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		0		/* switchable debug print-outs */
#define	F_SAFE		0
#define	F_FPRINTF	0


/* revision history :

	= 00/07/04, Alireza Khalafi

	This module was originally written.


	= 00/09/18, Dave Morano

	+ I added the new argument to the initialization call
	for the size of the branch tracking buffer.
	+ I made the actual tracking buffer dynamically allocated
	according to the given buffer size.
	+ I made the private subroutines static scope.
	+ The code needed to include '<assert.h>'.


*/


/**************************************************************************

	This object module provides the functions for a Levo Branch 
	Tracking Buffer.



**************************************************************************/



#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lbtrb.h"
#include	"ldecodeinstrclass.h"



/* local defines */



/* external subroutines */


/* forward referecens */

static int	lbtrb_lookup() ;
static int	lbtrb_get_cpaddr() ;
static int	lbtrb_remove() ;
static int	lbtrb_insert() ;






int 
lbtrb_init(lbtrbp,pip,mip,lip,bufsize)
LBTRB		*lbtrbp ;
struct proginfo	*pip ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
int		bufsize ;
{
	int	rs ;
	int	i, size ;


	if (lbtrbp == NULL)
	    return SR_FAULT ;

	lbtrbp->pip = pip ;
	lbtrbp->mip = mip ;
	lbtrbp->lip = lip ;

	if (bufsize < 0)
	    bufsize = LBTRB_BUFF_SIZE ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("lbtrb_init: entered btrksize=%d\n",bufsize) ;
#endif

	lbtrbp->entry = NULL ;
	size = bufsize * sizeof(struct lbtrb_entry) ;
	if ((rs = uc_malloc(size,&lbtrbp->entry)) < 0)
	    assert(0); 



/*
	lbtrbp->head = 
		(struct lbtrb_entry **) malloc(LBTRB_BUFF_SIZE*
			sizeof(struct lbtrb_entry *));
		if(lbtrbp->head == NULL)
			return SR_FAULT;
	
		for(i=0; i < LBTRB_BUFF_SIZE; ++i) {
			if(!(lbtrbp->head[i] = (struct lbtrb_entry *) 
				malloc(sizeof(struct lbtrb_entry))));
				return SR_FAULT;
		}
*/

	memset(lbtrbp->entry,0,size) ;

	size = lip->totalrows * sizeof(struct lbtrb_predicates);
	rs = uc_malloc(size, (void **) &lbtrbp->commit_buf);
	assert(rs >= 0);
	memset(lbtrbp->commit_buf,-2,size) ;

	lbtrbp->totalrows = lip->totalrows;
	lbtrbp->num_of_entries = 0 ;
	lbtrbp->bsize = bufsize ;
	lbtrbp->ppas = LBTRB_CPPAS ;
	lbtrbp->commited_paddr = 0;
	lbtrbp->commited_pvalue = 1;


	return SR_OK ;
}
/* end subroutine (lbtrb_init) */


/* free up this object */
int 
lbtrb_free(lbtrbp)
LBTRB	*lbtrbp ;
{
	struct proginfo	*pip ;


	if (lbtrbp == NULL)
	    return SR_FAULT ;

	pip = lbtrbp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("lbtrb_comb: called\n") ;
#endif

	if (lbtrbp->entry != NULL)
	    free(lbtrbp->entry) ;

	lbtrbp->entry = NULL ;
	return SR_OK ;
}
/* end subroutine (lbtrb_free) */


/* do any combinatorial work */
int 
lbtrb_comb(lbtrbp,phase)
LBTRB	*lbtrbp ;
int	phase ;
{
	struct proginfo	*pip ;


	pip = lbtrbp->pip ;

	switch (phase) {

	case 3:
/*		if(lbtrbp->c.f_updatebuff == 1)
			lbtrb_update_buffer(lbtrbp); */
		break;

	}

	return SR_OK ;
}


/* handle the clock transistion */
int 
lbtrb_clock(lbtrbp)
LBTRB	*lbtrbp ;
{
	struct proginfo	*pip ;

	struct mintinfo	*mip ;

	struct levoinfo	*lip ;

	LONG	clock ;

	pip = lbtrbp->pip ;
	mip = lbtrbp->mip ;
	lip = lbtrbp->lip ;

#if	F_DEBUG
	if (pip->debuglevel > 1) {

		lsim_getclock(mip,&clock) ;

	eprintf("lbtrb_clock: called %lld\n",clock) ;
	}
#endif

	lbtrbp->c=lbtrbp->n ;

	return SR_OK ;
}


/* machine shift (maybe not needed ?) */
int 
lbtrb_shift(lbtrbp)
LBTRB	*lbtrbp ;
{


	return SR_OK ;
}

int 
lbtrb_flush(lbtrbp)
LBTRB	*lbtrbp ;
{
	int i;


	lbtrbp->inc_curr_pred = 0;
	lbtrbp->curr_pred = 0;

	lbtrbp->num_of_entries = 0 ;
	lbtrbp->commited_paddr = 0;
	lbtrbp->commited_pvalue = 1;

	for(i=0; i<lbtrbp->bsize; ++i) {
	    lbtrbp->entry[i].valid = 0;
        }

	return SR_OK ;
}

int
lbtrb_isready(lbtrbp)
LBTRB	*lbtrbp;
{

	return 1;

}

/* do the real work ! */
int
lbtrb_assign_predicate(lbtrbp, insttype, instaddr, instTA, bpred, pr)
LBTRB   *lbtrbp ;
int insttype;   /* needs to be replaced with whatever structure the decoding stage uses */
uint instaddr ;
uint instTA ;
int  bpred;	/* branch prediction, 1=taken, 0=Not Taken, -1 non-branch */
struct lbtrb_predicates *pr;
{
	int	i,index ;
	int	cpindex ;
	int	branchdelay = 0;

	/* handles the case where a branch is inside the delay slot of another branch (not working !!) */
	if(lbtrbp->inc_curr_pred && (insttype == INSTR_JIND ||
	   insttype == INSTR_BREL || insttype == INSTR_JREL)) {
		++lbtrbp->curr_pred;
		lbtrbp->inc_curr_pred = 0;
	}

	pr->ipaddr = lbtrbp->curr_pred ;
	if(pr->ipaddr == lbtrbp->commited_paddr)
		pr->ipval = lbtrbp->commited_pvalue;
	else
		pr->ipval = -1; /* Branch predictor would assign the best value */

	/* handles the delay slot issue */
	if(lbtrbp->inc_curr_pred == 1) {
		++lbtrbp->curr_pred;
		lbtrbp->inc_curr_pred = 0;
		branchdelay = 1; /* we are handling a branch dealy instruction */

		/*  canceling predicates of the inst. in the branch delay 
			slot is the same as once for branch*/
		index = lbtrb_lookup(lbtrbp, instaddr-4) ;
	}
	else
		index = lbtrb_lookup(lbtrbp, instaddr) ;


/* Assigns the input canceling predicates */

	if (index != -1) {
	    lbtrb_get_cpaddr(lbtrbp, index, &pr->icpaddr, &pr->icpval, &pr->numicp) ;
	    /* assign the canceling predicates to the inst. in the branch delay slot */
	    if (!(insttype == INSTR_BREL || insttype == INSTR_JREL || insttype == INSTR_JIND)) 
		    lbtrb_remove(lbtrbp, index) ;
	}
	else {
		pr->icpaddr[0] = -1;
		pr->numicp = 0;
	}

	/* Assigns output predicates */

	if(branchdelay) {
	    pr->opaddr = -1 ;
	    pr->ocpaddr = -1 ;
        } else if (insttype == INSTR_BREL || insttype == INSTR_JREL
	    || insttype == INSTR_JIND) { 
	    if (lbtrb_insert(lbtrbp, instTA, 1+lbtrbp->curr_pred) < 0)
	        assert(0); /* BTRB full */

	    /* assigns the current predictor address to the next instruction (delay slot)
	       increament the predicate address after that instruction */ 
	    lbtrbp->inc_curr_pred = 1;

	    pr->ocpaddr = lbtrbp->curr_pred + 1;
	    pr->opaddr = lbtrbp->curr_pred + 1;

	} else if (pr->numicp>=1) {
	    pr->opaddr = ++lbtrbp->curr_pred ;
	    pr->ocpaddr = -1 ;

	} else {

	    pr->opaddr = -1 ;
	    pr->ocpaddr = -1 ;
	}

	return 0 ;
}
/* end subroutine (lbtrb_assign_predicate) */

/* this function needs to be fixed for the case that a lower address is 
actually  belogings to a later time */

int
lbtrb_later_in_time(paddr1, paddr2)
int	paddr1;
int	paddr2;
{

	if(paddr1 > paddr2)
		return 1;
	else
		return 0;

}


int
lbtrb_commit(lbtrbp)
LBTRB *lbtrbp;
{


	lbtrb_update_buffer(lbtrbp);

/*	if(lbtrbp->lip->nsgcols == 1)
		lbtrb_flush(lbtrbp);*/


	return SR_OK ;
}


int
lbtrb_as_commit(lbtrbp, opaddr, opval, opcaddr, opcval, row)
LBTRB *lbtrbp;
int opaddr;
int opval;
int opcaddr;
int opcval;
int row;	/* row index of the AS */
{
	struct lbtrb_predicates *cb;

	assert(row < lbtrbp->totalrows);
	cb = lbtrbp->commit_buf;
	cb[row].opaddr = opaddr;
	cb[row].opval = opval;
	cb[row].ocpaddr = opcaddr;
	cb[row].ocpval = opcval;
	if(lbtrb_later_in_time(opaddr, lbtrbp->commited_paddr)) {
		lbtrbp->commited_paddr = opaddr;
		lbtrbp->commited_pvalue = opval;
	}

}
/* end subroutine (lbtrb_as_commit) */


/* is a branch target within the current instruction execution window ? */
int lbtrb_targetin(op,ta,tt)
LBTRB	*op ;
uint	ta ;
int	tt ;
{


	if (op == NULL)
		return SR_FAULT ;


	return 0 ;
}
/* end subroutine (lbtrb_targetin) */



/* PRIVATE SUBROUTINES */



/* Updates the cpval with the commited value of the canceling predicate. */

int
lbtrb_cb_find(lbtrbp,cpaddr, cpval)
LBTRB *lbtrbp;
int	cpaddr;
int 	*cpval;
{

	int i ;
	struct lbtrb_predicates *cb;

	cb = lbtrbp->commit_buf;
	for(i=0; i<lbtrbp->totalrows; ++i) {
		if(cb[i].ocpaddr == cpaddr) {
			*cpval = cb[i].ocpval;
			return  1;
		}
	}

	return -1;


}

int
lbtrb_update_buffer(lbtrbp)
LBTRB *lbtrbp;
{
	int i,j ;
	struct lbtrb_predicates *cb;


	cb = lbtrbp->commit_buf;

	for(i=0; i<lbtrbp->bsize; ++i) {

	    if(lbtrbp->entry[i].valid == 1) {
		    for(j=0;j<lbtrbp->entry[i].ncp; ++j) {
			lbtrb_cb_find(lbtrbp,lbtrbp->entry[i].cpa[j],&(lbtrbp->entry[i].cpv[j]));
		    }
	    }

	}

#if	F_FPRINTF
lbtrb_print(lbtrbp);
#endif

	return -1 ;
}



static int 
lbtrb_lookup(lbtrbp, addr)
LBTRB * lbtrbp ;
uint addr ;
{
	int i ;


	for(i=0; i<lbtrbp->bsize; ++i) {

	    if(lbtrbp->entry[i].TA == addr && lbtrbp->entry[i].valid == 1)
	        return i ;

	}
	return -1 ;
}


static int 
lbtrb_get_cpaddr(lbtrbp, index, cpaddr, cpval, ncp)
LBTRB * lbtrbp ;
int	index ;
int	*cpaddr ;
int	*cpval ;
int	*ncp ;
{
	int i ;

	if ((index < 0) || (index > lbtrbp->bsize))
		return SR_INVALID ;

	*ncp = lbtrbp->entry[index].ncp ;
	if(*ncp > 0) {
	    for(i=0; i<*ncp; ++i) {
	        cpaddr[i] = lbtrbp->entry[index].cpa[i] ;
	        cpval[i] = lbtrbp->entry[index].cpv[i] ;
	    }
	}
	else
	    assert(0) ;

	return SR_OK ;
}


static int 
lbtrb_remove(lbtrbp, index)
LBTRB * lbtrbp ;
int	index ;
{


	lbtrbp->entry[index].valid = 0 ;
	return SR_OK ;
}


static int 
lbtrb_insert(lbtrbp, instTA, predaddr)
LBTRB * lbtrbp ;
uint	instTA ;
int	predaddr ;
{

	int	index,i ;


	index = lbtrb_lookup(lbtrbp, instTA) ;
	if (index != -1) {

	    if (lbtrbp->entry[index].ncp < lbtrbp->ppas) {
	        lbtrbp->entry[index].cpa[(lbtrbp->entry[index].ncp)] = predaddr ;
	        lbtrbp->entry[index].cpv[(lbtrbp->entry[index].ncp)++] = -1 ;
	    }	

	    return SR_OK ;

	} else { 

	    for(i=0; i<lbtrbp->bsize; ++i)
	        if(lbtrbp->entry[i].valid == 0) {
	            lbtrbp->entry[i].valid = 1 ;
	            lbtrbp->entry[i].TA = instTA ;
	            lbtrbp->entry[i].ncp = 1 ;
	            lbtrbp->entry[i].cpa[0]=predaddr ;
	            lbtrbp->entry[i].cpv[0]=-1 ;
	            return SR_OK ;
	        }
	    assert(0); /* buffer full */

	}

	return SR_OK ;
}
/* end subroutine (lbtrb_insert) */


int
lbtrb_print(lbtrbp)
LBTRB	*lbtrbp;
{

	int j,i;


	printf("LBTRB buffer:\n");
	printf("committed ipa=%d, ipv=%d\n",
		lbtrbp->commited_paddr,lbtrbp->commited_pvalue);

	for(i=0; i<lbtrbp->bsize; ++i) {
	        if(lbtrbp->entry[i].valid == 1) {
	            printf("TA=%d",lbtrbp->entry[i].TA); 
		    for(j=0;j<lbtrbp->entry[i].ncp;++j) {
			    printf(",cp(%d)=",lbtrbp->entry[i].cpa[j]);
			    printf("%d ",lbtrbp->entry[i].cpv[j]);
		    }
	            printf(",ncp=%d\n",lbtrbp->entry[i].ncp);
	        }
	}

	return 0 ;
}



