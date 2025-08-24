/* bpfifo SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* branch prediction FIFO */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable */
#define	CF_SAFE		0		/* extra safe mode? */

/* revision history:

	= 1998-11-01, David Morano
	This code was snarfed from the VPFIFO code.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	Object:
	cpfifo

	Description:
	This is a little FIFO code for storing branch prediction
	information for subsequent update.

*******************************************************************************/

#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<vsystem.h>
#include	<localmisc.h>

#include	"bpfifo.h"


/* local defines */


/* local namespaces */


/* local typedefs */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */


/* local variables */


/* exported variables */


/* exported subroutines */

int bpfifo_init(bpfido *op,int n) noex {
	int	rs ;
	int	sz ;

	if (op == NULL)
		return SR_FAULT ;

	(void) memset(op,0,szof(BPFIFO)) ;

	sz = n * szof(BPFIFO_ENTRY) ;
	rs = uc_malloc(sz,&op->table) ;

	if (rs < 0)
		goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(op->table,rs,"bpfifo_init:table") ;
#endif

	op->head = 0 ;
	op->tail = 0 ;
	op->n = n ;
	op->magic = BPFIFO_MAGIC ;

bad0:
	return rs ;
}


int bpfifo_free(bpfifo *op) noex {
	if (op == NULL) return SR_FAULT ;
	if (op->magic != BPFIFO_MAGIC) return SR_NOTOPEN ;

	if (op->table != NULL) {
	    uc_free(op->table) ;
#ifdef	MALLOCLOG
	    malloclog_free(op->table,"bpfifo_free:table") ;
#endif
	    op->table = NULL ;
	}

	op->magic = 0 ;
	return SR_OK ;
}


int bpfifo_add(bpfifo *op,ulong in,ulong ia,int row,int outcome) noex {
	int	i ;

#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != BPFIFO_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

#if	CF_DEBUGS
	debugprintf("bpfifo_add: in=%llu ia=%08x row=%u outcome=%u\n",
		in,ia,row,outcome) ;
#endif

	if (((op->tail + 1) % op->n) == op->head)
		return SR_OVERFLOW ;

	i = op->tail ;
	op->table[i].in = in ;
	op->table[i].ia = ia ;
	op->table[i].row = row ;
	op->table[i].outcome = outcome ;

	op->tail = (op->tail + 1) % op->n ;

#if	CF_DEBUGS
	debugprintf("bpfifo_add: returning\n") ;
#endif

	return SR_OK ;
}


int bpfifo_rem(bpfifo *op,ulong *inp,ulong *iap,uint *brp,uint *otp) noex {
	int	i ;

#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != BPFIFO_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (op->head == op->tail)
		return SR_NOENT ;

	i = op->head ;
	*inp = op->table[i].in ;
	*iap = op->table[i].ia ;
	*brp = op->table[i].row ;
	*otp = op->table[i].outcome ;

	op->head = (op->head + 1) % op->n ;
	return SR_OK ;
}


int bpfifo_read(bpfifo *op,ulong *inp,ulong *iap,uint *brp,uint *otp) noex {
	int	i ;

#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != BPFIFO_MAGIC)
		return SR_NOTOPEN ;
#endif /* F_SAFE */

#if	CF_DEBUGS
	debugprintf("bpfifo_read: entered\n") ;
#endif

	if (op->head == op->tail)
		return SR_NOENT ;

	i = op->head ;
	*inp = op->table[i].in ;
	*iap = op->table[i].ia ;
	*brp = op->table[i].row ;
	*otp = op->table[i].outcome ;

#if	CF_DEBUGS
	debugprintf("bpfifo_read: in=%llu ia=%08x row=%u outcome=%u\n",
		*inp,*iap,*brp,*otp) ;
#endif

	return SR_OK ;
}
/* end subroutine (bpfifo_read) */


