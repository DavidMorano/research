/* bpfifo */

/* branch prediction FIFO */


#define	CF_DEBUGS	0		/* non-switchable */
#define	CF_SAFE		0		/* extra safe mode? */


/* revision history:

	= 2002-06-11, David Morano

	This code was snarfed from the VPFIFO code.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This is a little FIFO code for storing branch prediction
	information for subsequent update.


*******************************************************************************/


#define	BPFIFO_MASTER	0


#include	<envstandards.h>

#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>
#include	<localmisc.h>

#include	"bpfifo.h"


/* local defines */

#define	BPFIFO_MAGIC	0x94732651


/* external subroutines */


/* local structures */


/* exported subroutines */


int bpfifo_init(op,n)
BPFIFO	*op ;
int	n ;
{
	int	rs ;
	int	size ;


	if (op == NULL)
		return SR_FAULT ;

	(void) memset(op,0,sizeof(BPFIFO)) ;

	size = n * sizeof(BPFIFO_ENTRY) ;
	rs = uc_malloc(size,&op->table) ;

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


int bpfifo_free(op)
BPFIFO	*op ;
{


	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != BPFIFO_MAGIC)
		return SR_NOTOPEN ;

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


int bpfifo_add(op,in,ia,row,outcome)
BPFIFO	*op ;
ULONG	in ;
ULONG	ia ;
uint	row ;
uint	outcome ;
{
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


int bpfifo_remove(op,inp,iap,brp,otp)
BPFIFO	*op ;
ULONG	*inp ;
ULONG	*iap ;
uint	*brp ;
uint	*otp ;
{
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


int bpfifo_read(op,inp,iap,brp,otp)
BPFIFO	*op ;
ULONG	*inp ;
ULONG	*iap ;
uint	*brp ;
uint	*otp ;
{
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


