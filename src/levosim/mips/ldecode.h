/* ldecode.h */

/* Levo Decode Stage */



/* revision history:

	= 00/06/15, Marcos de Alba

	This code was started.


	= 00/08/16, Dave Morano

	The object defines should be above the include files
	for extra build flexibility.


	= 02/01/09, Dave Morano

	I added some fields to the basic decoded-instruction
	record.  One is to handle the floating point status register
	as a source operand.  The others are to make the
	decoded-instruction record an *output* of the decode process
	rather than an in-out thing.


 */


#ifndef LDECODE_INCLUDE 
#define LDECODE_INCLUDE 1


/* Object defines */

#define LDECODE		struct ldecode_head



#include	<sys/types.h>

#include	"misc.h"



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

extern int ldecode_decode(LDECODE *,uint,uint) ;

extern void ldecode_printinstr(LDECODE *current_instr);



#endif  /* LDECODE_INCLUDE */



