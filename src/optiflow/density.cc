/* density */

/* this is a density gathering object */


/* revision history:

	= 2002-02-16, Dave Morano

	This was written for some statistics gathering for some software
	evaluation.


*/

/* Copyright © 2002 David Morano.  All rights reserved. */

/******************************************************************************

	This object facilitates maintaining the probability density for
	some random variable.


******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<cstring>

#include	"vsystem.h"

#include	"localmisc.h"
#include	"density.h"



/* local defines */

#define	DENSITY_MAGIC	91827346



/* external subroutines */

extern int	densitystatll(ULONG *,int,double *,double *) ;






int density_init(op,len)
DENSITY		*op ;
uint		len ;
{
	int	rs ;
	int	size ;


	if (len < 1)
	    return SR_INVALID ;

	memset(op,0,sizeof(DENSITY)) ;

	size = (len + 1) * sizeof(ULONG) ;
	rs = uc_malloc(size,&op->a) ;

	if (rs >= 0) {

	    memset(op->a,0,size) ;

	    op->len = len ;
	    op->magic = DENSITY_MAGIC ;

	} /* end if */

	return rs ;
}
/* end subroutine (density_init) */


int density_free(op)
DENSITY		*op ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != DENSITY_MAGIC)
	    return SR_NOTOPEN ;

	if (op->a != NULL)
	    uc_free(op->a) ;

	op->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (density_free) */


int density_update(op,ai)
DENSITY		*op ;
uint		ai ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != DENSITY_MAGIC)
	    return SR_NOTOPEN ;

	if (ai > op->max)
	    op->max = ai ;

	if (ai > (op->len - 1)) {

	    rs = SR_RANGE ;
	    ai = (op->len - 1) ;
	    op->ovf += 1 ;

	} /* end if */

	op->c += 1 ;
	op->a[ai] += 1 ;

	return rs ;
}
/* end subroutine (density_update) */


int density_slot(op,ai,rp)
DENSITY		*op ;
uint		ai ;
ULONG		*rp ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != DENSITY_MAGIC)
	    return SR_NOTOPEN ;

	if (ai > (op->len - 1))
	    return SR_INVALID ;

	if (rp == NULL)
	    return SR_FAULT ;

	*rp = op->a[ai] ;
	return SR_OK ;
}
/* end subroutine (density_slot) */


int density_stats(op,sp)
DENSITY		*op ;
DENSITY_STATS	*sp ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != DENSITY_MAGIC)
	    return SR_NOTOPEN ;

	if (sp == NULL)
	    return SR_FAULT ;

	sp->mean = 0.0 ;
	sp->var = 0.0 ;
	sp->count = op->c ;
	sp->max = op->max ;
	sp->ovf = op->ovf ;
	sp->len = op->len ;
	rs = densitystatll(op->a,(int) op->len,&sp->mean,&sp->var) ;

	if (rs >= 0) {

	    ULONG	sum ;

	    int	i ;


	    sum = 0 ;
	    for (i = 0 ; i < op->len ; i += 1)
	        sum += op->a[i] ;

	    if (sum != op->c)
	        rs = SR_BADFMT ;

	} /* end if */

	return rs ;
}
/* end subroutine (density_stats) */



