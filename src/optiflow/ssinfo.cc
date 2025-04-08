/* ssinfo */


#define	CF_DEBUGS	0


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */


#include	<sys/types.h>
#include	<string.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"ssinfo.h"



/* local defines */

#define	SSINFO_MAGIC	95847351



/* external subroutines */

extern uint	nextpowtwo(uint) ;






int ssinfo_init(lip)
struct ssinfo	*lip ;
{


	if (lip == NULL)
		return SR_FAULT ;

	memset(lip,0,sizeof(struct ssinfo)) ;

	lip->magic = SSINFO_MAGIC ;
	return SR_OK ;
}


int ssinfo_free(lip)
struct ssinfo	*lip ;
{


	if (lip == NULL)
		return SR_FAULT ;

	if (lip->magic != SSINFO_MAGIC)
		return SR_NOTOPEN ;

	if (lip->le.a != NULL) {

	    free(lip->le.a) ;

	    lip->le.a = NULL ;
	}

	lip->magic = 0 ;
	return SR_OK ;
}


int ssinfo_leset(lip,op)
struct ssinfo	*lip ;
int		op ;
{
	int	rs = SR_INVALID, n ;
	int	size ;

	char	*cp ;


	if (lip == NULL)
		return SR_FAULT ;

	if (lip->magic != SSINFO_MAGIC)
		return SR_NOTOPEN ;

#if	CF_DEBUGS
	eprintf("ssinfo_leset: op=%d\n",op) ;
#endif

	if (op >= 0) {

	    rs = SR_OK ;
	    if (op >= lip->le.alen) {

#if	CF_DEBUGS
	eprintf("ssinfo_leset: op too big, old len=%d\n",
		lip->le.alen) ;
#endif

	        n = nextpowtwo(op + 1) ;

#if	CF_DEBUGS
	eprintf("ssinfo_leset: new len=%d\n",n) ;
#endif

	        size = n * sizeof(char) ;
	        if (lip->le.a == NULL) {

#if	CF_DEBUGS
	eprintf("ssinfo_leset: virgin alloc\n") ;
#endif

	            rs = uc_malloc(size,&cp) ;

		    if (rs >= 0)
			memset(cp,0,size) ;

	        } else
	            rs = uc_realloc(lip->le.a,size,&cp) ;

#if	CF_DEBUGS
	eprintf("ssinfo_leset: Xalloc() rs=%d\n",rs) ;
#endif

	        if (rs >= 0) {
	            lip->le.alen = n ;
	            lip->le.a = cp ;
	        }

	    } /* end if (needed extension) */

	    if (rs >= 0)
	        lip->le.a[op] = TRUE ;

	} /* end if (valid argument) */

#if	CF_DEBUGS
	eprintf("ssinfo_leset: ssinfo_leset() rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssinfo_leset) */


int ssinfo_letst(lip,op)
struct ssinfo	*lip ;
int		op ;
{
	int	rs = SR_OK ;


	if (lip == NULL)
		return SR_FAULT ;

	if (lip->magic != SSINFO_MAGIC)
		return SR_NOTOPEN ;

	if ((op <= lip->le.alen) && (lip->le.a != NULL))
	        rs = (int) lip->le.a[op] ;

	return rs ;
}
/* end subroutine (ssinfo_letst) */




