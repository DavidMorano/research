/* checker */

/* checker machine state */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */



#include	<sys/types.h>
#include	<string.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"defs.h"
#include	"checker.h"
#include	"regs.h"
#include	"mem.h"



#define	CHECKER_MAGIC	0x77331144



int checker_init(op,memp,regp,sinstr,wsize)
CHECKER		*op ;
struct mem_t	*memp ;
struct regs_t	*regp ;
ULONG		sinstr ;
int		wsize ;
{
	int	rs ;
	int	size ;


	if (op == NULL)
		return SR_FAULT ;

	memset(op,0,sizeof(CHECKER)) ;

	if (wsize < 0)
		wsize = 10000 ;

	op->wsize = wsize ;

/* create and duplicate registers */

	op->regs = *regp ;

/* create and dupplicate memory */

	op->mem = mem_create("msmem") ;

	if (op->mem == NULL) {
		rs = SR_NOMEM ;
		goto bad0 ;
	}

	mem_init(op->mem) ;

	rs = mem_duplicate(op->mem,memp) ;

	if (rs < 0)
		goto bad0 ;

/* ISes */

	size = op->wsize * sizeof(SSAS) ;
	rs = uc_malloc(size,&op->cas) ;

	if (rs < 0)
	    goto bad1 ;

	memset(op->cas,0,size) ;

/* done */

	op->in = sinstr ;
	op->magic = CHECKER_MAGIC ;

ret0:
	return rs ;

/* bad stuff */
bad1:
	if (op->mem != NULL)
		mem_free(op->mem) ;

bad0:
	goto ret0 ;
}
/* end subroutine (checker_init) */


int checker_free(op)
CHECKER		*op ;
{

	if (op == NULL)
		return SR_FAULT ;

	if (op->magic != CHECKER_MAGIC)
		return SR_NOTOPEN ;

/* the ISes */

	if (op->cas != NULL)
		free(op->cas) ;

/* memory */

	if (op->mem != NULL)
		mem_free(op->mem) ;

	op->mem = NULL ;

/* done */

	return SR_OK ;
}
/* end subroutine (checker_free) */



