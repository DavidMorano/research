/* debug */

/* debugging support */


#define	CF_DEBUGS	1		/* debug print-outs */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	Debugging support subroutines.


******************************************************************************/


#include	<sys/types.h>
#include <stdlib.h>
#include <string.h>

/* some of Dave's libraries */

#include	<bio.h>
#include	<sbuf.h>
#include	<mallocstuff.h>
#include	<paramfile.h>

/* simple scalar stuff */

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"

/* from 'main' */

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#include	"ss.h"
#include	"ssinfo.h"
#include	"ssdecode.h"
#include	"ssiw.h"
#include	"ssas.h"





/* parse out into string form the flags of an instruction */
int instr_flags(f,buf,buflen)
uint	f ;
char	buf[] ;
int	buflen ;
{
	SBUF	b ;

	int	rs ;


	rs = sbuf_init(&b,buf,buflen) ;

	if (rs < 0)
		goto ret0 ;

	if ((f & F_ICOMP) || (f & F_FCOMP)) {

	    sbuf_strw(&b," COMP",-1) ;

	    if (f & F_ICOMP)
	        sbuf_strw(&b,"-INT",-1) ;

	    if (f & F_FCOMP)
	        sbuf_strw(&b,"-FLOAT",-1) ;

	}

	if (f & F_CTRL) {

	    sbuf_strw(&b," CONTROL",-1) ;

	    if (f & F_COND)
	        sbuf_strw(&b,"-COND",-1) ;

	    if (f & F_UNCOND)
	        sbuf_strw(&b,"-UNCOND",-1) ;

	}

	if (f & F_MEM) {

	    sbuf_strw(&b," MEMORY",-1) ;

	    if (f & F_LOAD)
	        sbuf_strw(&b,"-LOAD",-1) ;

	    if (f & F_STORE)
	        sbuf_strw(&b,"-STORE",-1) ;

	    if (f & F_DISP)
	        sbuf_strw(&b,"-DISP",-1) ;

	    if (f & F_RR)
	        sbuf_strw(&b,"-RR",-1) ;

	    if (f & F_DIRECT)
	        sbuf_strw(&b,"-DIRECT",-1) ;

	}

	if (f & F_TRAP) {

	    sbuf_strw(&b," TRAP",-1) ;

	}

	if (f & F_LONGLAT) {

	    sbuf_strw(&b," LONGLATENCY",-1) ;

	}

	if (f & F_DIRJMP)
	    sbuf_strw(&b," DIRJMP",-1) ;

	if (f & F_INDIRJMP)
	    sbuf_strw(&b," INDIRJMP",-1) ;

	if (f & F_CALL)
	    sbuf_strw(&b," CALL",-1) ;

	if (f & F_FPCOND)
	    sbuf_strw(&b," FPCOND",-1) ;

	if (f & F_IMM)
	    sbuf_strw(&b," IMM",-1) ;

	rs = sbuf_free(&b) ;

ret0:
	return rs ;
}
/* end subroutine (instr_display) */


/* dump an AS */
int ss_asdump(mip,name,asp)
SS		*mip ;
const char	name[] ;
SSAS		*asp ;
{
	OPERAND	*op ;

	int	rs1, i ;

	char	ibuf[100] ;


	eprintf("ss_asdump: %s ia=%016llx\n",
	    name,asp->n.ia) ;

		rs1 = md_disassemble(asp->n.instrword,asp->n.ia,ibuf,99) ;

	eprintf("ss_asdump: instr> %w\n",ibuf,((rs1 > 0) ? rs1 : 0)) ;

	eprintf("ss_asdump: op=%d class=%d flags=%06x\n",
	    asp->n.op,asp->n.class,asp->n.flags) ;

	eprintf("ss_asdump: nio=%d noo=%d\n",
	    asp->n.nopsi,asp->n.nopso) ;

	for (i = 0 ; i < asp->n.nopsi ; i += 1) {

	    op = &asp->n.opsi[i] ;
	    eprintf("ss_asdump: IOP i=%d t=%u a=%016llx pv=%016llx\n",
	        i,op->type,op->a,op->pv) ;

	    if (op->type == OPERAND_TMEM)
	        eprintf("ss_asdump: 	size=%d dp=%02x\n",
	            op->size,op->dp) ;

	} /* end for */

	for (i = 0 ; i < asp->n.nopso ; i += 1) {

	    op = &asp->n.opso[i] ;
	    eprintf("ss_asdump: OOP i=%d t=%u a=%016llx pv=%016llx v=%016llx\n",
	        i,op->type,op->a,op->pv,op->v) ;

	    if (op->type == OPERAND_TMEM)
	        eprintf("ss_asdump: 	size=%d dp=%02x\n",
	            op->size,op->dp) ;

	} /* end for */


	return 0 ;
}
/* end subroutine (ss_asdump) */



