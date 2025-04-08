/* sshiftreg */

/* synchronous shift register */


#define	CF_DEBUGS	0		/* non-switchable */
#define	CF_SAFE		1		/* extra safe mode ? */


/* revision history:

	= 02/06/11, Dave Morano

	This code was snarfed from the VPFIFO code.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This is a synchronous clocked shift register.


******************************************************************************/


#define	SSHIFTREG_MASTER	0


#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>
#include	<bitops.h>

#include	"localmisc.h"
#include	"sshiftreg.h"



/* local defines */

#define	SSHIFTREG_MAGIC	0x94732451



/* external subroutines */


/* local structures */







int sshiftreg_init(op,objsize,n)
SSHIFTREG	*op ;
int		objsize ;
int		n ;
{
	int	rs ;
	int	size ;


	if (op == NULL)
		return SR_FAULT ;

	(void) memset(op,0,sizeof(SSHIFTREG)) ;

#if	CF_DEBUGS
	eprintf("sshiftreg_init: objsize=%u n=%u\n",objsize,n) ;
#endif

	if ((objsize < 0) || (n < 0))
		return SR_INVALID ;

/* for an N-stage shift register, we need (N+1) slots */

	n += 1 ;

	size = (n + 1) * objsize ;
	rs = uc_malloc(size,&op->buf) ;

#if	CF_DEBUGS
	eprintf("sshiftreg_init: uc_malloc() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		goto bad0 ;

#ifdef	MALLOCLOG
	malloclog_alloc(op->buf,rs,"sshiftreg_init:buf") ;
#endif

	size = ((n + 1) / sizeof(char)) + 1 ;
	rs = uc_malloc(size,&op->valid) ;

#if	CF_DEBUGS
	eprintf("sshiftreg_init: uc_malloc() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		goto bad1 ;

	memset(op->valid,0,size) ;

#ifdef	MALLOCLOG
	malloclog_alloc(op->valid,rs,"sshiftreg_init:valid") ;
#endif

	op->nobj = n ;
	op->objsize = objsize ;

	op->c.head = op->n.head = 0 ;
	op->c.tail = op->n.tail = n ;
	op->magic = SSHIFTREG_MAGIC ;

ret0:
	return rs ;

/* bad stuff comes here */
bad1:
	free(op->buf) ;

bad0:
	goto ret0 ;
}


int sshiftreg_free(op)
SSHIFTREG	*op ;
{


	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;


	if (op->valid != NULL) {

	    free(op->valid) ;

#ifdef	MALLOCLOG
	malloclog_free(op->valid,"sshiftreg_free:valid") ;
#endif

	}

	if (op->buf != NULL) {

	    free(op->buf) ;

#ifdef	MALLOCLOG
	malloclog_free(op->buf,"sshiftreg_free:buf") ;
#endif

	}


	op->magic = 0 ;
	return SR_OK ;
}


int sshiftreg_comb(op,ph)
SSHIFTREG	*op ;
int		ph ;
{
	int	rs = SR_OK ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	switch (ph) {

	case 0:

#ifdef	COMMENT
		BCLRB(op->valid,op->c.tail) ;
#endif

		break ;

	case 5:
		if (BTSTB(op->valid,op->c.tail))
			op->n.c += 1 ;

		if (BTSTB(op->valid,op->c.head))
			op->n.c -= 1 ;

		op->n.tail = (op->n.tail + 1) % (op->nobj + 1) ;
		op->n.head = (op->n.head + 1) % (op->nobj + 1) ;

		BCLRB(op->valid,op->c.head) ;

		break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (sshiftreg_comb) */


int sshiftreg_clock(op)
SSHIFTREG	*op ;
{
	int	rs = SR_OK ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	op->c = op->n ;

	return rs ;
}
/* end subroutine (sshiftreg_clock) */


int sshiftreg_writea(op)
SSHIFTREG	*op ;
{
	int	rs = SR_OK ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	rs = (BTSTB(op->valid,op->c.tail)) ? 0 : 1 ;

#if	CF_DEBUGS
	eprintf("sshiftreg_writea: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sshiftreg_writea) */


int sshiftreg_write(op,ep)
SSHIFTREG	*op ;
void		*ep ;
{
	int	rs = SR_OK ;

	char	*bp ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	bp = op->buf + (op->c.tail * op->objsize) ;
	memcpy(bp,ep,op->objsize) ;

#if	CF_DEBUGS
	eprintf("sshiftreg_write: tail=%u ep(%p)\n",
		op->c.tail,bp) ;
#endif

	rs = (! BTSTB(op->valid,op->c.tail)) ;

	BSETB(op->valid,op->c.tail) ;

#if	CF_DEBUGS
	eprintf("sshiftreg_write: ret rs=%d c=%d\n",
		rs,(op->c.c + 1)) ;
#endif

	return (rs >= 0) ? op->c.tail : rs ;
}
/* end subroutine (sshiftreg_write) */


int sshiftreg_read(op,ep)
SSHIFTREG	*op ;
void		*ep ;
{
	int	rs = SR_NOENT ;
	int	f ;

	char	*bp ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	f = BTSTB(op->valid,op->c.head) ;

	if (f) {

	rs = SR_OK ;
	bp = op->buf + (op->c.head * op->objsize) ;
	memcpy(ep,bp,op->objsize) ;

	}

#if	CF_DEBUGS
	eprintf("sshiftreg_read: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (sshiftreg_read) */


int sshiftreg_gethead(op,vp)
SSHIFTREG	*op ;
void		*vp ;
{
	int	f ;

	void	**epp = vp ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGS
	{
		int	i ;
	eprintf("sshiftreg_get: head=%u\n",op->c.head) ;
	for (i = 0 ; i < (op->nobj / sizeof(char)) ; i += 1) {
	eprintf("sshiftreg_get: validw[%u]=%02x\n",i,op->valid[i]) ;
	}
	}
#endif

	*epp = NULL ;
	f = BTSTB(op->valid,op->c.head) ;

#if	CF_DEBUGS
	eprintf("sshiftreg_get: validbit[%u]=%u\n",
		op->c.head,f) ;
#endif

	if (f)
	*epp = op->buf + (op->c.head * op->objsize) ;

#if	CF_DEBUGS
	eprintf("sshiftreg_get: ret f=%u ep(%p)\n",f,(*epp)) ;
#endif

	return f ;
}
/* end subroutine (sshiftreg_gethead) */


int sshiftreg_gettail(op,vp)
SSHIFTREG	*op ;
void		*vp ;
{
	int	f ;

	void	**epp = (void **) vp ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	*epp = NULL ;
	f = BTSTB(op->valid,op->c.tail) ;

	if (f)
	*epp = op->buf + (op->c.tail * op->objsize) ;

	return f ;
}
/* end subroutine (sshiftreg_gettail) */


int sshiftreg_get(op,i,vp)
SSHIFTREG	*op ;
int		i ;
void		*vp ;
{
	int	f ;

	void	**epp = vp ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if ((i < 0) || (i >= (op->nobj + 1)))
		return SR_NOTFOUND ;

	*epp = NULL ;
	f = BTSTB(op->valid,i) ;

	if (f)
	*epp = op->buf + (i * op->objsize) ;

	return f ;
}
/* end subroutine (sshiftreg_get) */


extern int sshiftreg_audit(op)
SSHIFTREG	*op ;
{
	int	rs = SR_OK, i, j, ch ;

	char	*bp ;


#if	CF_SAFE
	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != SSHIFTREG_MAGIC)
		return SR_NOTOPEN ;
#endif /* CF_SAFE */

	for (i = 0 ; i < (op->nobj + 1) ; i += 1) {

		bp = op->buf + (i * op->objsize) ;
		for (j = 0 ; j < op->objsize ; j += 1)
			ch = *bp++ ;

#if	CF_DEBUGS
	eprintf("sshiftreg_audit: valid[%u]=%u\n",i,BTSTB(op->valid,i)) ;
#endif

	} /* end for */

	if (ch == SSHIFTREG_MAGIC)
		rs = 1 ;

	return rs ;
}
/* end subroutine (sshiftreg_audit) */



/* PRIVATE SUBROUTINES */




