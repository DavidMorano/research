/* lsg */

/* Levo Sharing Group */


#define	F_DEBUGS	0


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written for LEVOSIM.


*/


/**************************************************************************

	This module provides the functions for a Levo Sharing Group.



**************************************************************************/


#define	LSG_MASTER	0


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"bus.h"
#include	"las.h"
#include	"lpe.h"
#include	"lsg.h"



/* local defines */



/* external subroutines */


/* forward referecens */






int lsg_init(lsgp,gp,mip,lip,frbuses,pi)
LSG		*lsgp ;
struct proginfo	*gp ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
BUSFR		*frbuses[] ;
int		pi ;			/* physical index */
{
	int	rs, i ;
	int	nas, npe ;
	int	sgrow ;


	lsgp->gp = gp ;
	lsgp->mip = mip ;
	lsgp->lip = lip ;
	lsgp->frbuses = frbuses ;
	lsgp->pi = pi ;


/* what is our sharing group row number ? */

		sgrow = pi % lip->ngrows ;


/* create our PE */

	npe = LEVO_NPEPSG ;
	size = npe * sizeof(LPE) ;
	rs = uc_malloc(size,&lsgp->lpes) ;

	if (rs < 0)
	    return rs ;

	for (i = 0 ; i < npe ; i += 1) {

	lpe_init(&lsgp->lpes[i],gp,mip,lip,frbuses,pi) ;

	} /* end for */


/* create our active stations */

	nas = lip->nspg * LEVO_NSWPSG ;
	size = nas * sizeof(LAS) ;
	rs = uc_malloc(size,&lsgp->lass) ;

	if (rs < 0)
	    return rs ;

	for (i = 0 ; i < nas ; i += 1) {

	las_init(&lsgp->lass[i],gp,mip,lip,frbuses,lsgp->lpes,pi) ;

	} /* end for */




	return SR_OK ;
}
/* end subroutine (lsg_init) */


/* free up this sharing group */
int lsg_free(lsgp)
LSG	*lsgp ;
{
	struct levoinfo	*lip = lsgp->lip ;

	int	rs, i ;
	int	nas, npe = LEVO_NPEPSG ;


#if	F_DEBUGS
	eprintf("lsg_free: entered\n") ;
#endif

	if (lsgp == NULL)
		return SR_FAULT ;

	nas = lip->nspg * LEVO_NSWPSG ;


/* free up the active stations */

	for (i = 0 ; i < nas ; i += 1)
		las_free(&lsgp->lass[i]) ;

	free(lsgp->lass) ;


/* free up the PEs */

	for (i = 0 ; i < npe ; i += 1)
		lpe_free(&lsgp->lpes[i]) ;

	free(lsgp->lpes) ;


#if	F_DEBUGS
	eprintf("lsg_free: exiting\n") ;
#endif

	return SR_OK ;
}
/* end subroutine (lsg_free) */





