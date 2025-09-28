/* lsq */

/* Levo Load Buffer */


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
#include	<cstdlib>
#include	<cstring>
#include	<assert.h>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"mintinfo.h"
#include	"levoinfo.h"
#include	"lsq.h"
#include        "ldecodeinstrclass.h"
#include        "lexec.h"



/* local defines */

#define	LSQ_QSIZE	100
#define	LSQ_MAGIC	0x94357234



/* external subroutines */


/* forward references */

static int	lsq_handleshift(LSQ *,struct proginfo *,struct levoinfo *) ;





/* initialize this object */
int lsq_init(lsqp,pip,mip,lip,sqsize)
LSQ		*lsqp ;
struct proginfo	*pip ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
int		sqsize ;
{
	int	rs, i ;
	int	bsize=0;
	int	numpred=0;
	int	size ;


	if (lsqp == NULL)
		return SR_FAULT ;

	lsqp->magic = 0 ;
	lsqp->pip = pip ;
	lsqp->mip = mip ;
	lsqp->lip = lip ;

	lsqp->qsize = (sqsize <= 0) ? LSQ_QSIZE : sqsize ;

	lsqp->num_of_entries = 0;
	lsqp->f.shift = FALSE ;

/* make some dynamic private data for the actual Q storage */

	size = lsqp->qsize * sizeof(struct lsq_entry) ;
	rs = uc_malloc(size,(void **) &lsqp->entry) ;

	if (rs < 0)
		goto bad0 ;

	for (i=0; i < lsqp->qsize; i += 1) {
		lsqp->entry[i].valid=0;
	}

	lsqp->magic = LSQ_MAGIC ;
	return SR_OK ;

bad0:
	return rs ;
}
/* end subroutine (lsq_init) */


/* free up this object */
int lsq_free(lsqp)
LSQ	*lsqp ;
{
	struct proginfo	*pip ;


	if (lsqp == NULL)
		return SR_FAULT ;

	if (lsqp->magic != LSQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lsqp->pip ;

/* free up whatever we allocated */

	if (lsqp->entry != NULL)
		free(lsqp->entry) ;

	lsqp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (lsq_free) */


/* do our combinatorial stuff */
int lsq_comb(lsqp,phase)
LSQ	*lsqp ;
int	phase ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


	if (lsqp == NULL)
		return SR_FAULT ;

	if (lsqp->magic != LSQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lsqp->pip ;
	lip = lsqp->lip ;


	switch (phase) {

	case 1:
	case 2:
		break ;

	case 3:
		rs = lsq_handleshift(lsqp,pip,lip) ;

		break ;

	} /* end switch */


	return rs ;
}
/* end subroutine (lsq_comb) */


/* we just got clocked */
int lsq_clock(lsqp)
LSQ	*lsqp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (lsqp == NULL)
		return SR_FAULT ;

	if (lsqp->magic != LSQ_MAGIC)
		return SR_NOTOPEN ;

	pip = lsqp->pip ;



/*	lsqp->c = lsqp->n;  */


	return rs ;
}
/* end subroutine (lsq_clock) */


/* take a request for a machine shift */
int lsq_shift(lsqp)
LSQ	*lsqp ;
{


	if (lsqp == NULL)
		return SR_FAULT ;

	if (lsqp->magic != LSQ_MAGIC)
		return SR_NOTOPEN ;

	lsqp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (lsq_shift) */


/* Use this function to insert entries into the store queue 
   If another entry with the same address already exists in the
   buffer, its data value would be overwritten by the new data */
int 
lsq_insert(lsqp, addr, data, tt, dp)
LSQ	*lsqp;
uint addr;	/* address */
int data;	/* data */
int tt;		/* time tag */
int dp;		/* data present*/
{
	struct lsq_entry	*ep ;
	int	i,j;
	int	noe;
	int	d1,tt1;


	if(lsqp->num_of_entries >= lsqp->qsize-1) {
		assert(0);
		return SR_FAULT;
	}

	j=lsq_lookup(lsqp,addr,&d1,&tt1);
	if(j>=0)
		ep=&lsqp->entry[j];
	else
		ep=&lsqp->entry[lsqp->num_of_entries++];
	ep->valid=1;
		
	if(dp & LFLOWGROUP_DPBYTE3)
		ep->data = (ep->data & 0x00ffffff) | (data & 0xff000000);
	if(dp & LFLOWGROUP_DPBYTE2)
		ep->data = (ep->data & 0xff00ffff) | (data & 0x00ff0000);
	if(dp & LFLOWGROUP_DPBYTE1)
		ep->data = (ep->data & 0xffff00ff) | (data & 0x0000ff00);
	if(dp & LFLOWGROUP_DPBYTE0)
		ep->data = (ep->data & 0xffffff00) | (data & 0x000000ff);
	/* ep->data = data; /* has to be changed to condsider dp*/
	ep->addr = addr;
	ep->tt = tt;
	return SR_OK ;
}
/* end subroutine (lsq_insert) */


/* returns the index to the entry, -1 if could not find a match */
int lsq_lookup(lsqp, addr, data, tt)
LSQ	*lsqp;
uint addr;
int *data, *tt;
{

	struct lsq_entry *ep ;
	int	i;
	int	found = 0;

	if(lsqp->num_of_entries == 0)
		return -1;

	for(i=0; i<lsqp->num_of_entries; ++i) {
		ep = &lsqp->entry[i];
		if(ep->addr == addr && ep->valid == 1) {
			*data = ep->data;
			*tt = ep->tt;
			found = 1;
			break;
		}
	}

	if(found)
		return i;
	else
		return -1;
}


/* Removes all the entries with the matching address */
int lsq_remove(lsqp, addr)
LSQ	*lsqp;
uint addr;
{

	struct lsq_entry *ep ;
	int	i,j;
	int	found = 0;

	if(lsqp->num_of_entries == 0)
		return SR_FAULT;

	for(i=0; i<lsqp->num_of_entries; ++i) {
		ep = &lsqp->entry[i];
		if(ep->addr == addr && ep->valid == 1) {
			ep->valid = 0;
			--lsqp->num_of_entries;
			found = 1;
		}
	}

	if(!found)
		return SR_FAULT;

	for(i=0; i<lsqp->num_of_entries; ++i) {
		if(lsqp->entry[i].valid == 0)
			for(j=i; j<lsqp->qsize-1; ++j)
				lsqp->entry[j] = lsqp->entry[j+1];

	}
	return SR_OK;
}
/* end subroutine (lsq_remove) */


/* this functions needs to be removed!! */
int lsq_read_next(lsqp, addr, data, tt)
LSQ	*lsqp;
uint *addr;
int *data, *tt;
{

	struct lsq_entry *ep ;
	int	i;

	if(lsqp->num_of_entries == 0)
		return SR_FAULT;

	for(i=0; i<lsqp->num_of_entries; ++i) {
		ep = &lsqp->entry[i];
		if(ep->valid == 1) {
			*data = ep->data;
			*tt = ep->tt;
			*addr = ep->addr;
			return SR_OK;
		}
	}

	assert(0);	

	return 0 ;
}
/* end subroutine (lsq_read_next) */


int	
lsq_num_of_entries(lsqp)
LSQ *lsqp;
{
	return lsqp->num_of_entries;

}


int
lsq_print(LSQ * lsqp) 
{
	int i;

	for(i=0;i<lsqp->num_of_entries; ++i) {
	/*	printf("Mem(%d)=%d,tt=%d\n", lsqp->entry[i].addr, lsqp->entry[i].data, lsqp->entry[i].tt);*/
		printf("Mem(0x%x)=%d\n", lsqp->entry[i].addr, lsqp->entry[i].data);
	}

	return 0 ;
}


/* perform a machine shift if one was called for */
static int lsq_handleshift(lsqp,pip,lip)
LSQ		*lsqp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK, i ;
	int	totalrows ;


	if (! lsqp->f.shift)
		return SR_OK ;

	lsqp->f.shift = FALSE ;

/* is this right to not have any clocked machine state ? */

	totalrows = lip->totalrows ;
	for ( i = 0 ; i < lsqp->qsize ; i += 1) {

		lsqp->entry[i].tt -= totalrows ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (lsq_handleshift) */



