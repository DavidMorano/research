/* sfifo SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* synchronous FIFO */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable */
#define	CF_SAFE		1		/* extra safe mode ? */

/* revision history:

	= 2002-06-11, Dave Morano
	This code was snarfed from the BPFIFO code.

*/

/* Copyright © 2002 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

  	Object:
	sfifo

	Description:
	This object implements a synchronous clocked FIFO.

******************************************************************************/

#include	<envstandards.h>	/* must be ordered first to configure */
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<vsystem.h>
#include	<bitops.h>
#include	<localmisc.h>

#include	"sfifo.h"


/* local defines */


/* local namespaces */


/* local typedefs */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */


/* local variables */


/* exported variables */


/* external subroutines */

int sfifo_init(sfifo *op,int objsize,int n) noex {
	int	rs ;
	int	sz ;


	if (op == NULL)
		return SR_FAULT ;

	(void) memset(op,0,szof(SFIFO)) ;

	sz = (n + 1) * objsize ;
	rs = uc_malloc(sz,&op->buf) ;

	if (rs < 0)
		goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(op->buf,rs,"sfifo_init:buf") ;
#endif

	sz = (n + 1) / szof(char) ;
	rs = uc_malloc(sz,&op->valid) ;

	if (rs < 0)
		goto bad1 ;

#ifdef	MALLOCLOG
	malloclog_alloc(op->valid,rs,"sfifo_init:valid") ;
#endif

	op->nobj = n ;
	op->magic = SFIFO_MAGIC ;

ret0:
	return rs ;

/* bad stuff comes here */
bad1:
	free(op->buf) ;

bad0:
	goto ret0 ;
}


int sfifo_free(sfifo *op) noex {
	if (op == NULL) return SR_FAULT ;
	if (op->magic != SFIFO_MAGIC) return SR_NOTOPEN ;

	if (op->buf != NULL) {

	    free(op->buf) ;

#ifdef	MALLOCLOG
	malloclog_free(op->buf,"sfifo_free:buf") ;
#endif

	}

	if (op->valid != NULL) {

	    free(op->valid) ;

#ifdef	MALLOCLOG
	malloclog_free(op->valid,"sfifo_free:valid") ;
#endif

	}

	op->magic = 0 ;
	return SR_OK ;
}


int sfifo_comb(sfifo *op,int ph) noex {
	int	rs = SR_OK ;

#if	CF_SAFE
	if (op == NULL) return SR_FAULT ;
	if (op->magic != SFIFO_MAGIC) return SR_NOTOPEN ;
#endif /* CF_SAFE */

	switch (ph) {

	case 0:
		op->fl.ins = 0 ;
		op->fl.rem = 0 ;
		break ;

	case 5:
		if (op->fl.ins) {
			op->n.c += 1 ;
			op->n.tail = (op->n.tail + 1) % (op->nobj + 1) ;
		}

		if (op->fl.rem > 0) {
			op->n.c -= 1 ;
			op->n.head = (op->n.head + 1) % (op->nobj + 1) ;
			BCLRB(op->valid,op->c.head) ;
		}

		break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (sfifo_comb) */

int sfifo_clock(sfifo *op) noex {
	int	rs = SR_OK ;

#if	CF_SAFE
	if (op == NULL) return SR_FAULT ;
	if (op->magic != SFIFO_MAGIC) return SR_NOTOPEN ;
#endif /* CF_SAFE */

	op->c = op->n ;

	return rs ;
}
/* end subroutine (sfifo_clock) */

int sfifo_ins(sfifo *op,void *ep) noex {
	int	rs = SR_OK ;
	char	*bp ;

#if	CF_SAFE
	if (op == NULL) return SR_FAULT ;
	if (op->magic != SFIFO_MAGIC) return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (op->c.c >= op->nobj)
		return SR_OVERFLOW ;

	bp = op->buf + (op->c.tail * op->objsize) ;
	memcpy(bp,ep,op->objsize) ;

	BSETB(op->valid,op->c.tail) ;

	op->n.tail = (op->c.tail + 1) % (op->nobj + 1) ;

	op->fl.ins = 1 ;

#if	CF_DEBUGS
	eprintf("sfifo_ins: ret\n") ;
#endif

	return (op->c.c + 1) ;
}

int sfifo_rem(sfifo *op,void *ep) noex {
	int	rs = SR_OK ;
	char	*bp ;

#if	CF_SAFE
	if (op == NULL) return SR_FAULT ;
	if (op->magic != SFIFO_MAGIC) return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (op->c.c == 0)
		return SR_OVERFLOW ;

	bp = op->buf + (op->c.head * op->objsize) ;
	memcpy(ep,bp,op->objsize) ;

	op->n.head = (op->c.head + 1) % (op->nobj + 1) ;

	op->fl.rem = 1 ;
	if (op->c.c == 1)
		rs = 0 ;

#if	CF_DEBUGS
	eprintf("sfifo_rem: ret\n") ;
#endif

	return (op->c.c - 1) ;
}

int sfifo_read(sfifo *op,void *ep) noex {
	int	rs = SR_OK ;
	char	*bp ;

#if	CF_SAFE
	if (op == NULL) return SR_FAULT ;
	if (op->magic != SFIFO_MAGIC) return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (op->c.c == 0)
		return SR_OVERFLOW ;

	bp = op->buf + (op->c.head * op->objsize) ;
	memcpy(ep,bp,op->objsize) ;

#if	CF_DEBUGS
	eprintf("sfifo_read: ret\n") ;
#endif

	return op->c.c ;
}


