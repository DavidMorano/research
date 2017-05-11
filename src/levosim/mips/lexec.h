/* lexec.h */

/* Levo Instruction Execution */



/* revision history:

   = 01/01/23, Marcos R de Alba

   Included the R4000 instructions: CEIL.W, TRUNC, SQRT, ROUND and
   FLOOR Extended the number of source and destination registers to
   handle floating point double precision.

   Removed instructions:
     LDC0 
     CTC0
     

   = 00/06/23, Marcos R de Alba

   Extended MIPS Instructions Set and right implementation of
   instructions.


   = 00/08/09, Marcos de Alba

   Included new definitions to cover compare instructions


   = 00/06/24, David Morano

   Corrected parameters types.  Added the defines for the execution
	exceptions.


	= 02/01/09, Dave Morano

	I added new public structures as part of a redefinition of the
	calling parameters for 'lexec()'.  I also changed the
	declaration of the subroutine to reflect the new parameters.


	= 02/01/30, Dave Morano

	I separated out the LEXECOP defines from this file since
	they are used by the instruction decode routines and
	other things that do not need to reference the LEXEC object
	itself.


*/


/* MIPS R3000 ISA execution interface for use in LEVO simulator */



#ifndef LEXEC_INCLUDE
#define LEXEC_INCLUDE	1



/* include some files that we need */


/* return codes */

#define	LEXEC_ROK	0		/* executed OK */
#define	LEXEC_RUNIMPL	-1		/* unimplemented instruction */
#define	LEXEC_RDIVZERO	-2		/* divide by zero */
#define	LEXEC_REXCEPT	-3		/* caused a machine exception */



/* some structures for passing parameters to the execution code */

/* decoded instruction information */
struct lexec_instr {
	uint	ia ;			/* instructions address */
	uint	instr ;			/* original instruction word */
	uint	constant ;		/* whatever was with the instruction */
	int	opexec ;		/* decoded operation code (LEXECOP) */
} ;

/* execution source input values */
struct lexec_src {
	uint	s1, s2, s3, s4, s5 ;	/* register operands */
} ;

struct lexec_dstflags {
	uint	jump : 1 ;		/* ? */
	uint	taken : 1 ;		/* branch taken */
	uint	nullify : 1 ;		/* nullify the delay slow instr */
} ;

/* execution destination output values */
struct lexec_dst {
	uint	d1, d2, d3 ;		/* register operands */
	uint	ma ;
	uint	ta ;
	uint	delay ;
	struct lexec_dstflags	f ;	/* output flags */
} ;


#if	(! defined(LEXEC_MASTER)) || (LEXEC_MASTER == 0)

/* subroutine declarations */

#ifdef	COMMENT

extern int lexec(int,uint *,uint *,uint *,uint *,
		uint *,uint *,uint *,
		uint *,
		int *,uint *,
		int *, uint *,
		int *,int *) ;

#endif /* COMMENT */

extern int	lexec(struct lexec_instr *,struct lexec_src *,
			struct lexec_dst *) ;

#endif /* (the master) */


#endif /* LEXEC_INCLUDE */



