/* ldecode.h */

/* Levo Decode Stage */


/* revision history:

	= 2000-06-15, Marcos de Alba

	This code was started.


	= 2000-08-16, David Morano

	The object defines should be above the include files
	for extra build flexibility.


	= 2002-01-09, David Morano

	I added some fields to the basic decoded-instruction
	record.  One is to handle the floating point status register
	as a source operand.  The others are to make the
	decoded-instruction record an *output* of the decode process
	rather than an in-out thing.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef LDECODE_INCLUDE 
#define LDECODE_INCLUDE 1


/* Object defines */

#define LDECODE		struct ldecode_head


#include	<envstandards.h>

#include	<sys/types.h>

#include	<localmisc.h>


/* return codes */

#define	LDECODE_ROK	0		/* properly decoded */
#define	LDECODE_RUNIMPL	1		/* unimplemented (not decoded) */


struct ldecode_head { 
	uint	ia ;		/* ilptr for this instruction */
	uint	instr ;		/* MIPS instruction opcode*/
	uint	opclass ;	/* Instruction class */
	uint	opexec ;	/* Pseudo executable opcode used by lexec */
	uint	ta ;		/* target address for control-flow-changes */
	uint	const1 ;	/* constant in instruction */
	int	src1 ;
	int	src2 ;
	int	src3 ;
	int	src4 ;
	int	src5 ;
	int	dst1 ;
	int	dst2 ;
	int	dst3 ;
	int const1_valid; /* whether constant valid or not */
	int btype;   /* Branch Direction: BACKWARD: -1 or FORWARD: 1 */
};


/* subroutine declarations */

#ifdef	__cplusplus
extern "C" {
#endif

extern int ldecode_decode(LDECODE *,uint,uint) ;

extern void ldecode_printinstr(LDECODE *current_instr);

#ifdef	__cplusplus
}
#endif

#endif  /* LDECODE_INCLUDE */


