/* ldecode.h */

/* Levo Decode Stage */


#define	F_DEBUGS	0


/* revision history:

	= 00/06/15, Marcos de Alba

	This code was started.


	= 00/08/16, Dave Morano

	The object defines should be above the include files
	for extra build flexibility.


 */


#ifndef LDECODE_INCLUDE 
#define LDECODE_INCLUDE 1


/* Object defines */

#define LDECODE		struct ldecode_head



#include	<sys/types.h>

#include	<assert.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"ldecodeinstrclass.h"
#include	"lexec.h"
#include	"lmipsregs.h"



/* return codes */

#define	LDECODE_ROK	0
#define	LDECODE_RUNIMPL	1



struct ldecode_head { 
	uint instrword;   /* MIPS instruction opcode*/
	uint opexec;  /* Pseudo executable opcode used by lexec */
	uint opclass;   /* Instruction class defined in ldecodeinstrclass.h */
	uint ilptr;          /* ilptr for this instruction */
	int	src1;
	int	src2;
	int	src3;
	int	src4;
	int	dst1;
	int	dst2;
	int	dst3 ;
	uint	const1 ;
	uint const1_valid; /* whether constant valid or not */
	int btype;   /* Branch Direction: BACKWARD: -1 or FORWARD: 1 */
};



/* subroutine declarations */

extern int ldecode_decode(LDECODE *) ;

extern void ldecode_printinstr(LDECODE *current_instr);



#endif  /* LDECODE_INCLUDE */



