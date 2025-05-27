/* lexec */

/* MIPS R3000 ISA execution interface for use in LEVO simulator */


#define	CF_DEBUGS	0		/* non-switchable debug print-outs */
#define	CF_DEBUGSMAIN	0
#define	CF_EXPLICITSHIFT	0		/* explicit shift */


/* revision history:

	= 00/06/23, Marcos de Alba

	This code was began.

   
	= 00/06/24, David Morano 

	Corrected parameters style.


	= 00/08/25, Marcos R de Alba

	Included unimplemented MIPS Instructions for r2000/r3000


	= 01/01/24, Marcos 

	Agregue las instrucciones SQRT, TRUNC, y otras mas.

   
	= 01/01/31, Marcos

	Me falta modificar todas las instrucciones de punto flotante
	para que funcionen como el IEEE754
	Tambien debo agregar todas las instrucciones siguientes (lo mismo
	aplica para ldecode):

   Instruccion         Tipo (ldecode)
   
   TLT                 SPECIAL
   TLTU                 SPECIAL
   TNE                 SPECIAL
   TEQ                 SPECIAL
   TGE                 SPECIAL
   TGEU                 SPECIAL
   TLTI                REGIMM
   TLTIU                REGIMM
   TNEI                REGIMM
   TGEI                REGIMM
   TGEIU                REGIMM

   Ademas debo corregir las instrucciones que son _U utilizan los
   operandos como unsigned, las que son normal como MULT utilizan
   los operadores como complemento a 2.


	= July 27, 2001 Marcos de Alba

	I added the parameter 'ilptr' to the 'lexec' routine
	to store the return address on JAL (call) instructions 
    

	= 01/08/06, David Morano

	I fixed a problem with the 'f_nullify' for the 'BLTZAL'
	instructions.  I also added the 'TRUE'/'FALSE' constants where
	they are used.  And I also changed the branch conditional
	output to a boolean integer (as in typical C code).


	= 01/10/30, David Morano

	I fixed the JALR instruction.  It wasn't setting the
	target address (at all).

	I also changed the JR instruction to match the reference
	that I have on it.
	

	= 01/11/06, David Morano

	I fixed the NEGS instructions.  It was computing the one's
	compliment rather than the two's complement.

	I recoded some of the simpler double precision floating point
	instructions.  Some were just not coded correctly originally.
	Others that were coded to try and do double precision
	arithmetic just didn't work correctly (as compared with the
	same instructions executing on the SGI box).  The ones recoded
	were ADDD, SUBD, MULD, DIVD, ABSD, NEGD.


	= 01/11/08, David Morano

	The DIVU instruction was bad !  I fixed it.  Whew !

	I changed the MFC[01] and MTC[01] to NOT do a floating
	to integer or integer to floating conversion.


	= 01/11/12, David Morano

	I fixed the CTC1 and CFC1 to not do any int-float or float-int
	conversion !  After all, these are supposed to be moving a
	control-status type register.

	The biggy is that I also had to fix all of the CVT-type
	instructions.  They were all messed up (they were decoded
	wrongly -- some other file than this one had that) but they
	were also being executed incorrectly.

	I also fixed the MOVD instruction.  It was only "moving"
	32-bits.  It should move 64-bits.


	= 02/01/09, David Morano

	I fixed up the various instructions (compare, branch, ?) that
	use the floating status register (FCR31).

	FYI, FCR31 bits :

	0-1	rounding mode
	2-6	"flag"
	7-11	"enable"
	12-16	"cause"
	17	"Unimp"
	18-22	always zero in all current MIPS ISAs !
	23	C (condition) bit (this is the biggy !)
	24	FS (flush to zero) bit
	25-31	FCC[1-7] bits (used in MIPS-4 ISA and above -- I think !)

	I also changed the way that parameters are passed to make it
	clearer what are input and what are outputs !


	= 02/01/10, David Morano

	I fixed up the proper register addresses for the FCR0 and FCR31
	in coprocessor-1 as well as defining some new Levo physical
	registers for the same corresponding registers in
	coprocessor-0.  We are just not doing anything with
	coprocessors 2 or 3 !  We are hardly doing anything for
	coprocessor-0 as it is also.

	I also fixed up the double precision floating point compare
	instructions.  They were not properly doing the double
	floating comparisons correctly.  Also in the subroutine
	'lexec_comp_instr', the final condition flag bit used
	bit-wise AND operators ('&') accidentally rather than the
	proper logical AND operators ('&&').


	= 02/01/18, David Morano

	The decision about whether a floating pointer comparison was
	single precision or double precision was wrong.  It used the
	'FMT_' which was not testing the correct bit in the decoded
	'opexec' value.  It was trying to test a four bit field for
	equality to something (but it missed on even this because it
	compared against a decimal value rather than a hexadecimal
	value as probably intended) when it should have been testing
	bit 5 !


	= 02/01/21, Dave morano

	I enhanced the floating point comparison and branching
	instructions to use the MIPS-4 type condition codes.


	= 02/01/29, David Morano

	I implemented the single and double precisions square root
	(SQRTS and SQRTD) instructions.  They are needed for the
	SpecInt-2000 VPR program !


	= 02/04/18, David Morano

	I changed the DIVS instruction to not check for a floating zero
	before doing the divide.  It now check for straight integer
	zero like the other divide instructions.  This has not shown up
	as a problem (as far as I know), but comparing a float number
	for equality is not a good practice !


	= 02/04/23, David Morano

	The SRA and SRAV instructions were not coded with the correct
	intent but they were actually correct as they were anyway.
	I changed them to be more explicit.



*/


/******************************************************************************

   GENERAL NOTES:
   
   1. NOTES FOR INSTRUCTIONS THAT GENERATE EXCEPTIONS:

   At exception time:

   Status Register (Register 165) has to be saved 
   kernel/user mode bits ( KUo: bit5, IEo:bit4, KUp:bit3
   IEp:bit2, KUc:bit1, IEc:bit0 )
   
   KUo = KUp 
   IEo = IEp
   KUp = KUc
   IEp = IEc
   KUc = 0
   IEc = 0

   At return time (RFE instruction):
   
   KUp = KUo
   IEp = IEo
   KUc = KUp
   IEc = IEp
   
   2. NOTES FOR UNIMPLEMENTED INSTRUCTIONS
      
   All instructions that are not implemented in r2000/r3000
   have the pattern LEXECOP_UNIMPL_...
   

******************************************************************************/


#define	LEXEC_MASTER	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<climits>
#include	<math.h>
#include	<ieeefp.h>

#include	"dataval.h"

#include	"lmipsregs.h"
#include	"lexecop.h"
#include	"ldecode.h"
#include	"lexec.h"



/* local defines */

#define COND0_(_opexec) ((_opexec) & 1)
#define COND1_(_opexec) ((_opexec) & 2)
#define COND2_(_opexec) ((_opexec) & 4)
#define COND3_(_opexec) ((_opexec) & 8)

/* I do not know what the following was useful for */
/* 1: double, 0: single */

#ifdef	COMMENT
#define FMT_(_opexec) ((((_opexec) & 0xf0) == 80) ? 1 : 0) 
#endif

/* 02/01/14 DAM, fixed double=0, single=1 */
#define FMT_(_opexec)	(((_opexec) & 0x10) ? 1 : 0) 
#define	BITS21_25(w)	(((w) > 21) & 0x1f)



/* external subroutines */

#if	defined(SOLARIS) && (SOLARIS == 8)
extern double	sqrt(double) ;
#endif


/* local structures */

/* 01/11/06, David Morano, aid for double precision instructions */
struct divar {
	uint	hi, lo ;
} ;

/* 01/11/06, David Morano, aid for double precision instructions */
union dvar {
	struct divar	i ;
	double		d ;
} ;

typedef struct lexec_type_ {
  long long    all[2];
  long long    res[2];
  double       temp[2];
} lexec_type ;


/* forward references */

static int lexec_comp_instr(int,uint,
	DATAVAL *,DATAVAL *,DATAVAL *,DATAVAL *,DATAVAL *,DATAVAL *) ;






int lexec(ip,sp,dp)
struct lexec_instr	*ip ;
struct lexec_src	*sp ;
struct lexec_dst	*dp ;
{

	DATAVAL *src1, *src2, *src3, *src4, *src5 ;
	DATAVAL	*cnst ;				/* input */
	DATAVAL	*dst1, *dst2, *dst3 ;		/* outputs */

#ifdef	COMMENT
	lexec_type aux ;
#endif

	unsigned int ahi, alo, bhi, blo ;
	unsigned int part1, part2, part3, part4 ;
	unsigned int p2lo, p2hi, p3lo, p3hi ;
	unsigned int ms1, ms2, ms3, sum, carry ;
	unsigned int extra ;
	unsigned int	uiw, signbit ;

	int	iw ;
	int	*delay ;			/* output */
	int	f_taken = 0, *bcnd = &f_taken ;	/* output */
	int	fj = 0, *f_jump = &fj ;		/* output */
	int	fn = 0, *f_nullify = &fn ;		/* output */

	int	rs = LEXEC_ROK ;
	int	temp, temp2, conflict = 0 ;


/* decoded instructions information */

	cnst = (DATAVAL *) &ip->constant ;

/* source operands */

	src1 = (DATAVAL *) &sp->s1 ;
	src2 = (DATAVAL *) &sp->s2 ;
	src3 = (DATAVAL *) &sp->s3 ;
	src4 = (DATAVAL *) &sp->s4 ;
	src5 = (DATAVAL *) &sp->s5 ;

/* clear destination operands to zero */

	{
		int	*iwp, *lwp ;

		char	*cp, *lcp ;


		iwp = (int *) dp ;
		lwp = iwp + (sizeof(struct lexec_dst) / sizeof(int)) ;
		while (iwp < lwp)
			*iwp++ = 0 ;

		cp = (char *) iwp ;
		lcp = ((char *) dp) + sizeof(struct lexec_dst) ;
		while (cp < lcp)
			*cp++ = 0 ;

	} /* end block */

	dst1 = (DATAVAL *) &dp->d1 ;
	dst2 = (DATAVAL *) &dp->d2 ;
	dst3 = (DATAVAL *) &dp->d3 ;

	delay = (int *) &dp->delay ;


#if	CF_DEBUGS

#ifdef	COMMENT
	printf("lexec: opexec=%d src1:%u src2:%u src3:%u src4:%u\n",
		ip->opexec,src1->ui,src2->ui,src3->ui,src4->ui) ;
	printf("lexec: cnst:%u bcnd:%i delay:%d\n",
			cnst->ui,*bcnd,delay->ui);
#else
	debugprintf("lexec: opexec=%d \n", ip->opexec) ;
	debugprintf("lexec: s1=%08x s2=%08x s3=%08x s4=%08x s5=%08x\n",
		src1->ui,src2->ui,src3->ui,src4->ui,src5->ui) ;
	debugprintf("lexec: cnst=%08x\n", cnst->ui) ;
#endif

#endif /* CF_DEBUGS */

/* set some defaults */

	*delay = 1 ;			/* default */

/* set a default if all else fails */

	dst1->ui = 0 ;
	dst2->ui = 0 ;
	dst3->ui = 0 ;

/* OK, try and do the best that we can */

	switch (ip->opexec) {

	case LEXECOP_NOOP:
	    break ;

/* Integer operations: */
	case LEXECOP_ADD:
	    dst1->i= src1->i + src2->i ;
	    break ;

	case LEXECOP_ADDI:
	    dst1->i = src1->i + cnst->i ;
	    break ;

	case LEXECOP_ADDU:
	    dst1->ui = src1->ui + src2->ui ;
	    break ;

	case LEXECOP_ADDIU:

#if	CF_DEBUGS && 0
		debugprintf("lexec: ADDIU src1=%08x cnst=%08x\n",
			src1->ui,cnst->ui) ;
#endif

	    dst1->ui = src1->ui + cnst->ui ;

#if CF_DEBUGS && 0
		debugprintf("lexec: ADDIU: %x + %x = %x\n",
			src1->ui,cnst->ui,(src1->ui+cnst->ui)) ;
		debugprintf("lexec: dst1=%08x\n", dst1->ui) ;
#endif

	    break ;

	case LEXECOP_AND:
	    dst1->ui = src1->ui & src2->ui ;
	    break ;

	case LEXECOP_ANDI:
	    dst1->ui = src1->ui & cnst->ui & 0xffff ;
	    break ;

	case LEXECOP_DIV:
	    if (src2->ui != 0) {

	        dst1->i = src1->i / src2->i ; /* LO */
	        dst2->i = src1->i % src2->i ; /* HI */

	    } else {

	        dst1->i = -1 ;
	        dst2->i = INT_MAX ;
		rs = LEXEC_RDIVZERO ;

	    }

	    *delay = 35 ;
	    break ;

	case LEXECOP_DIVU:
	    if (src2->ui != 0) {

	        dst1->ui = src1->ui / src2->ui ; /* LO */
	        dst2->ui = src1->ui % src2->ui ; /* HI */

	    } else {

	        dst1->i = -1 ;
	        dst2->i = -1 ;
		rs = LEXEC_RDIVZERO ;

		}

	    *delay = 35 ;
	    break ;

	case LEXECOP_MULT:
	    ahi = (unsigned int) src1->i >> 16 ;
	    alo = src1->i & 0xffff ;
	    bhi = (unsigned int) src2->i >> 16 ;
	    blo = src2->i & 0xffff ;

/* compute the partial products */
	    part4 = ahi * bhi ;
	    part3 = ahi * blo ;
	    part2 = alo * bhi ;
	    part1 = alo * blo ;

	    p2lo = part2 << 16 ;
	    p2hi = part2 >> 16 ;
	    p3lo = part3 << 16 ;
	    p3hi = part3 >> 16 ;

/* compute the carry bit by checking the high-order bits of the addends */
	    carry = 0 ;
	    sum = part1 + p2lo ;
	    ms1 = part1 & 0x80000000 ;
	    ms2 = p2lo & 0x80000000 ;
	    ms3 = sum & 0x80000000 ;
	    if ((ms1 & ms2) || ((ms1 | ms2) & ~ms3))
	        carry++ ;
	    ms1 = p3lo & 0x80000000 ;
	    sum += p3lo ;
	    ms2 = sum & 0x80000000 ;
	    if ((ms1 & ms3) || ((ms1 | ms3) & ~ms2))
	        carry++ ;

	    dst1->i= sum; /* LO */
	    extra = 0 ;
	    if (src1->i < 0)
	        extra = src2->i ;

	    if (src2->i < 0)
	        extra += src2->i ;

	    dst2->i = part4 + p2hi + p3hi + carry - extra; /* HI */
	    *delay = 12 ;
	    break ;

	case LEXECOP_MULTU:
	    ahi = src1->ui >> 16 ;
	    alo = src1->ui & 0xffff ;
	    bhi = src2->ui >> 16 ;
	    blo = src2->ui & 0xffff ;

/* compute the partial products */
	    part4 = ahi * bhi ;
	    part3 = ahi * blo ;
	    part2 = alo * bhi ;
	    part1 = alo * blo ;
	    p2lo = part2 << 16 ;
	    p2hi = part2 >> 16 ;
	    p3lo = part3 << 16 ;
	    p3hi = part3 >> 16 ;

/* compute the carry bit by checking the high-order bits of the addends */
	    carry = 0 ;
	    sum = part1 + p2lo ;
	    ms1 = part1 & 0x80000000 ;
	    ms2 = p2lo & 0x80000000 ;
	    ms3 = sum & 0x80000000 ;
	    if ((ms1 & ms2) || ((ms1 | ms2) & ~ms3))
	        carry++ ;

	    ms1 = p3lo & 0x80000000 ;
	    sum += p3lo ;
	    ms2 = sum & 0x80000000 ;
	    if ((ms1 & ms3) || ((ms1 | ms3) & ~ms2))
	        carry++ ;

	    dst1->ui = sum ;
	    dst2->ui = part4 + p2hi + p3hi + carry ;
	    *delay = 12 ;
	    break ;

	case LEXECOP_NOR:
	    dst1->i = ~ (src1->i | src2->i) ;
	    break ;

	case LEXECOP_OR:

#ifdef	COMMENT
 	ldecode_instr->src1.i = RS_(instr);
			ldecode_instr->dst1.i = RT_(instr);
			ldecode_instr->scr2.i = RD_(instr); from ldecode 
#endif /* COMMENT */

	    dst1->i = src1->i | src2->i ;
	    break ;

/* Immediate value passed at src2 */
	case LEXECOP_ORI:
	    dst1->i = src1->i | (cnst->i & 0xffff) ;
	    break ;

/* shift right LOGICAL */
	case LEXECOP_SRL:
	    dst1->ui = src1->ui >> (cnst->ui & 31) ;
	    break ;

/* shift right ARITHMETIC */
	case LEXECOP_SRA:
		iw = cnst->ui & 31 ;

#if	CF_EXPLICITSHIFT
		signbit = (src1->ui >> 31) & 1 ;
		uiw = (signbit && (iw > 0)) ? ((~ 0) << (32 - iw)) : 0 ;
	    dst1->ui = (src1->ui >> iw) | uiw ;
#else
	    dst1->i = (src1->i >> iw) ;
#endif

	    break ;

/* shift left LOGICAL */
	case LEXECOP_SLLV:
/* shamt value at src2 */
	    dst1->ui = src1->ui << (src2->ui & 0x1f) ;
	    break ;

/* shift right LOGICAL */
	case LEXECOP_SRLV:
	    dst1->ui = src1->ui >> (src2->ui & 31) ;
	    break ;

/* shift right ARITHMETIC */
	case LEXECOP_SRAV:
		iw = src2->ui & 31 ;

#if	CF_EXPLICITSHIFT
		signbit = (src1->ui >> 31) & 1 ;
		uiw = (signbit && (iw > 0)) ? ((~ 0) << (32 - iw)) : 0 ;
	    dst1->ui = (src1->ui >> iw) | uiw ;
#else
	    dst1->i = (src1->i >> iw) ;
#endif

	    break ;

/* shift left LOGICAL */
	case LEXECOP_SLL:
	    dst1->ui = src1->ui << (cnst->ui & 31) ;
	    break ;

	case LEXECOP_SUB:
	    dst1->i = src1->i - src2->i ;
	    break ;

	case LEXECOP_SUBU:
	    dst1->ui = src1->ui - src2->ui ;
	    break ;

	case LEXECOP_XOR:
	    dst1->i = src1->i ^ src2->i ;
	    break ;

	case LEXECOP_XORI:
	    dst1->i=src1->i ^ (cnst->i & 0xffff) ;
	    break ;

	case LEXECOP_SLT:
	    dst1->i = (src1->i < src2->i) ? 1 : 0 ;
	    break ;

	case LEXECOP_SLTU:
	    dst1->i = (src1->ui < src2->ui) ? 1 : 0 ;
	    break ;

	case LEXECOP_SLTI:
	    dst1->i = (src1->i < cnst->i) ? 1 : 0 ;
	    break ;

	case LEXECOP_SLTIU:
	    dst1->i = (src1->ui < cnst->ui) ? 1 : 0 ;
	    break ;

/****

NOTES:

For Levo, backward branch target addresses are converted to forward
target addresses.  It is assumed on execution that the conversion is
done during the decode stage in ldecode.c.  The target addresses are
computed at decode stage and here it is only returned what was the
direction of the branch (taken/ not taken).

****/

/****

 Assumptions for most of the branch instructions:

1. The address of this instruction (ilptr) is given in 'ilptr'
2. The offset value is given in source 4
3. The branch target address is returned in dst1 

****/

/* There is a delay of one instruction when the branch is taken */
/* It is only returned if the branch was taken, 1, or not, 0, */

	case LEXECOP_BEQ:
	    *bcnd = (src1->i == src2->i) ;
	    *delay = 2 ;
	    break ;

	case LEXECOP_BNE:

#if CF_DEBUGS && 0
		debugprintf("lexec: BNE src1->i:%d src2->i:%d\n",src1->i,src2->i);
#endif

	    *bcnd = (src1->i != src2->i) ;
	    *delay = 2 ;
	    break ;

/****

 NOTE:

BNEL is an invalid instruction for R2000/R3000 and it generates
an exception: sets ExcCode field of Cause Register to 10 
(Reserved Instruction Exception).

****/


/****

In exception :

dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
temp = src2->i << 2;
dst2->i = dst2->i || temp;               Saves Status Register 

****/

	case LEXECOP_BEQL:
	    *bcnd = (src1->i == src2->i) ;
        *f_nullify = (! *bcnd) ; 
	    *delay = 2 ;
	    break ;

/****
 In exception
		       dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
		       temp = src2->i << 2;
		       dst2->i = dst2->i || temp;               
Saves Status Register 
****/

	case LEXECOP_BNEL:
	    *bcnd = (src1->i != src2->i) ;
		*f_nullify = (! *bcnd) ;
	    *delay = 2 ;
	    break ;
		
/****
 In exception
		       dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
		       temp = src2->i << 2;
		       dst2->i = dst2->i || temp;               
Saves Status Register 
****/

	case LEXECOP_BGEZL:
	    *bcnd = (src1->i >= src2->i) ;
		*f_nullify = (! *bcnd) ;
	    *delay = 2 ;
	    break ;
		
/**** 
In exception
	dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
		       temp = src2->i << 2;
	dst2->i = dst2->i || temp;               
Saves Status Register 
****/

	case LEXECOP_BLEZL:
	    *bcnd = (src1->i <= src2->i) ;
		*f_nullify = (! *bcnd) ;
	    *delay = 2 ;
	    break ;
		
	case LEXECOP_BGEZ:
	    *bcnd = (src1->i >= 0) ;
	    *delay = 2 ;
	    break ;
		
/* link */
	case LEXECOP_BGEZAL:
	    *bcnd = (src1->i >= 0) ;
	    *delay = 2 ;
		dst1->i = ip->ia + 8 ;
		break;

/* link */
	case LEXECOP_BGEZALL:
	    *bcnd = (src1->i >= 0) ;
	    *delay = 2 ;
		dst1->i = ip->ia + 8;
		*f_nullify = (! *bcnd) ;
	    break ;

/* Instruction is valid only if src2->i == 0 */
	case LEXECOP_BGTZ:
	        *delay = 2 ;
#ifdef	COMMENT
	    if (src2->i == 0) {

	        *bcnd = src1->i > 0 ;
	    }
#else
	        *bcnd = (src1->i > 0) ;
#endif

	    break ;

	case LEXECOP_BLEZ:
	    *bcnd = (src1->i <= 0) ;
	    *delay = 2 ;
	    break ;

	case LEXECOP_BLTZ:
	    *bcnd = (src1->i < 0) ;
	    *delay = 2 ;
	    break ;
		
	case LEXECOP_BLTZL:
	    *bcnd = (src1->i < 0) ;
	    *delay = 2 ;
		*f_nullify = (! *bcnd) ;
	    break ;

/* Not implemented in r2000/r3000 */
	case LEXECOP_BGTZL:
	    *bcnd = (src1->i > 0) ;
	    *delay = 2 ;
		*f_nullify = (! *bcnd) ;
	    break ;

/* This two instructions need to store the address of
		       the instruction after the delay slot in r31
		       r31 = ip->ia + 8 */

/* link */
	case LEXECOP_BLTZAL:
	    *bcnd = (src1->i < 0) ;
	    *delay = 2 ;
		dst1->i = ip->ia + 8 ;
		*f_nullify = (! *bcnd) ;
		break;
		
/* link */
	case LEXECOP_BLTZALL:
	    *bcnd = (src1->i < 0) ;
	    *delay = 2 ;
		dst1->i = ip->ia + 8 ;
		*f_nullify = (! *bcnd) ;
	    break ;

/* Break Instruction causes a breakpoint exception */
	case LEXECOP_BREAK:
	    dst1->i = src1->i | (0x3e & 9) ;
	    break ;

	case LEXECOP_SYNC:
	    *delay = 1 ;
	    break ;

/* ilptr = target address */
	case LEXECOP_J:
	    *bcnd = TRUE ;
	    *delay = 2 ;
	    break ;

/* link */
	case LEXECOP_JAL:
	    *bcnd = TRUE ;
        dst1->i = ip->ia + 8 ;
	    *delay = 2 ;
	    break ;
		
/* link */
	case LEXECOP_JALR:
	    *bcnd = TRUE ;
        dst1->i = ip->ia + 8 ;
	 *f_jump = TRUE ;
	 dp->ta = src1->ui ;
	    *delay = 2 ;
	    break ;
		
    case LEXECOP_JR:

#if CF_DEBUGS && 0
	debugprintf("lexec: LEXECOP_JR (1) src2->i = %d  "
		"bcnd->i=%d  f_jump=%d\n",
			src2->i, *bcnd, *f_jump);
#endif

#ifdef	COMMENT
	if (src2->i == 0) {

	 *bcnd = TRUE ;
	 *f_jump = TRUE ;
	 dp->ta = src1->ui ;
	 *delay = 2 ;
	}
#else
	 *bcnd = TRUE ;
	 *f_jump = TRUE ;
	 dp->ta = src1->ui ;
	 *delay = 2 ;
#endif /* COMMENT */

#ifdef	COMMENT
	debugprintf("lexec:LEXECOP_JR (2) src2->i = %d  "
		"bcnd->i=%d  f_jump=%d\n",
			src2->i, bcnd->i, *f_jump);
#endif

	break ;

	case LEXECOP_LUI:
	    dst1->ui = (cnst->ui & 0xffff) << 16 ;
	    *delay = 1 ;
	    break ;

/* Memory LOAD/STORE instructions */

	case LEXECOP_LB:
	case LEXECOP_LW:
	case LEXECOP_LWL:
	case LEXECOP_LWR:
	case LEXECOP_LBU:
	case LEXECOP_LH:
	case LEXECOP_LHU:

	case LEXECOP_SB:
	case LEXECOP_SH:
	case LEXECOP_SW:
	case LEXECOP_SWR:
	case LEXECOP_SWL:
	    dp->ma = src1->ui + cnst->ui ;
	    *delay = 2 ;
	    break ;

	case LEXECOP_SC:
/* Invalid operation for R2000/R3000 
Conditional Store
If there is no conflict on this memory location (another
instruction has modified the contents of this memory
location after the related LL (load Link) instruction at
this location, then the store is successfull, otherwise
it is ignored.
On success, rt =1, on failure, rt =0

On memory conflict set the conflict flag to 1
*/
	    if (! conflict) {

	        dp->ma = src1->ui + cnst->ui ;
	        *delay = 2 ;
	        src1->ui = 1 ;

	    } else
	        src1->ui = 1 ;

	    break ;

	case LEXECOP_SDC1:

/****

This instruction is invalid for R2000/R3000
Generates exception if any of the three least significant bits 
of the effective addres is non-zero.

****/

	    dp->ma = src1->ui + cnst->ui ;
	    *delay = 2 ;
	    break ;

	case LEXECOP_SWC1:

/****

Generates exception if any of the three least significant bits 
of the effective addres is non-zero.

****/

	    dp->ma = src1->ui + cnst->ui ;
	    *delay = 2 ;
	    break ;

	case LEXECOP_MFHI:

/* ASSUMPTION: 

The AS accesses register HI, gets its value and puts it in
src1->i
dst1->i will hold this value to later be stored in the destination
register that the AS must know.

****/

	    dst1->i = src1->i ;
	    *delay = 3 ;
	    break ;

	case LEXECOP_MFLO:

/* ASSUMPTION: 

The AS accesses register LO, gets its value and puts it in
src1->i
dst1->i will hold this value to later be stored in the destination
register that the AS must know.

****/

	    dst1->i = src1->i ;
	    *delay = 3 ;
	    break ;

	case LEXECOP_MTHI:
/* ASSUMPTION: 
		       
The AS accesses source register, gets its value and puts it in
the special register HI.

****/

	    dst1->i = src1->i ;
	    *delay = 3 ;
	    break ;

	case LEXECOP_MTLO:
/* ASSUMPTION: 
		       
The AS accesses source register, gets its value and puts it in
the special register LO.

****/

	    dst1->i = src1->i ;
	    *delay = 3 ;
	    break ;

	case LEXECOP_MFC0:
	case LEXECOP_MFC1:
	    dst1->i = src1->i ;  /* contents of FPR are loaded into GPR */
	    *delay = 2 ;
	    break ;

	case LEXECOP_MTC0:
	case LEXECOP_MTC1:
	    dst1->i = src1->i;  /* contents of GPR are loaded into FPR */
	    *delay = 2 ;
	    break ;

/*******************************************************/
/* Floating point operations: */
/*******************************************************/

/****

GENERAL ASSUMPTIONS ABOUT FLOATING POINT OPERATIONS:

Every source operand with double precision is formed by the
conjuction of two single precision registers.
Only even registers can be used for double precision. For example
if FR8 is going to be used for double precision then
it would be formed by FGR8 (which will form the FGR8 LOW part)
single and FGR9 single (which would form the HI part)

****/

	case LEXECOP_ABSD:

#ifdef	COMMENT
	    aux.all[0] = (unsigned int) src2->f ;
	    aux.all[0] = (aux.all[0] << 32) | (unsigned int) src1->f ;
	    aux.temp[0] = (long long) aux.all[0] ;
	    if (aux.temp[0] < 0) {
	        aux.res[0] = (long long) (aux.temp[0] * -1) ;
	        dst2->f = (aux.res[0] & 0xffffffff00000000) >> 32 ;
	        dst1->f = aux.res[0] & 0x00000000ffffffff ;

	    } else{

	        aux.res[0] = (long long) aux.temp[0] ;
	        dst2->f = (aux.res[0] & 0xffffffff00000000) >> 32 ;
	        dst1->f = aux.res[0] & 0x00000000ffffffff ;
	    }
#else
	{
			union dvar	s1, d1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

	    		d1.d = s1.d ;
			if (d1.d < 0)
				d1.d = (- s1.d) ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

	} /* end block */
#endif /* COMMENT */

	    break ;

	case LEXECOP_ABSS:
	        dst1->f = src1->f ;
	    if (src1->f < 0)
	        dst1->f = (- src1->f) ;

	    break ;

	case LEXECOP_ADDD:

#if	COMMENT
	    aux.all[0] = (unsigned int) src2->f ;
	    aux.all[0] = (aux.all[0] << 32) | (unsigned int) src1->f ;
	    aux.all[1] = (unsigned int) src4->f ;
	    aux.all[1] = (aux.all[1] << 32) | (unsigned int) src3->f ;
	    aux.temp[0] = (long long) aux.all[0] + (long long) aux.all[1] ;
	    aux.res[0] = (long long) aux.temp[0] ;
	    dst2->f = (aux.res[0] & 0xffffffff00000000) >> 32 ;
	    dst1->f = aux.res[0] & 0x00000000ffffffff ;
#else
	{
			union dvar	s1, s2, d1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

			s2.i.lo = src3->ui ;
			s2.i.hi = src4->ui ;

	    		d1.d = s1.d + s2.d ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

	} /* end block */
#endif /* COMMENT */

	    break ;

	case LEXECOP_ADDS:
	    dst1->f =  src1->f + src2->f ;
	    break ;

/****

GENERAL ASSUMPTIONS FOR COMPARISON INSTRUCTIONS:

1. The AS accesses the floating-point condition flag: 
LMIPSREG_FCR31 71  (FP Control/status register) bit 23

2. The result of the comparison is stored on dst1.i and
AS has to update the condition bit.

****/

/* 02/01/09 DAM, added 5th source operand to all compaisons below ! */

	case LEXECOP_CMPFD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPFD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPFS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPFS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPUND:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPUND, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPUNS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPUNS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPEQD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPEQD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPEQS:

#if	CF_DEBUGS
	debugprintf("lexec: CMP SP (cmpeqs)\n") ;
#endif

	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPEQS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPOLTD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPOLTD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPOLTS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPOLTS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPULTD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPULTD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPULTS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPULTS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPOLED:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPOLED, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPOLES:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPOLES, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPULED:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPULED, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPULES:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPULES, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPSFD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPSFD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPSFS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPSFS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGLED:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGLED, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGLES:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGLES, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPSEQD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPSEQD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPSEQS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPSEQS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGLD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGLD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGLS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGLS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPLTD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPLTD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPLTS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPLTS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGED:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGED, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGES:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGES, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPLED:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPLED, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPLES:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPLES, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGTD:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGTD, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

	case LEXECOP_CMPNGTS:
	*delay = 1 ;
	    lexec_comp_instr(LEXECOP_CMPNGTS, ip->instr,
		src1,src2,src3,src4,src5,dst1) ;

	    break ;

/* 02/01/09 DAM, do these all differently ! */

#ifdef	COMMENT

/****

For the following four branch instructions the
src1 is the condition bit flag, assigned as register 183,
which is set by the compare instructions

****/

	case LEXECOP_BC0F:
	case LEXECOP_BC1F:

/* In exception dst1->i = dst1->i | ( 0x3e & 11); */

	case LEXECOP_BC0FL:
	case LEXECOP_BC1FL:
	    *bcnd = (~ src1->i) ;
	    *delay = 2 ;
	    break ;

	case LEXECOP_BC0T:
	case LEXECOP_BC1T:

/* In exception dst1->i = dst1->i | ( 0x3e & 11); */

	case LEXECOP_BC0TL:
	case LEXECOP_BC1TL:
	    *bcnd = (src1->i != 0) ;
	    *delay = 2 ;
	    break ;

#endif /* COMMENT */

/* 02/01/09 DAM, here are the new versions ! */

	case LEXECOP_BC0F:
	case LEXECOP_BC1F:

/* In exception dst1->i = dst1->i | ( 0x3e & 11); */

	case LEXECOP_BC0FL:
	case LEXECOP_BC1FL:
		{
			int	n, bn ;


			n = (ip->instr >> 18) & 7 ;
		bn = (n != 0) ? (24 + n) : 23 ;
	    *bcnd = ! ((src1->i >> bn) & 1) ;

		} /* end block */

	    *delay = 2 ;
	    break ;

	case LEXECOP_BC0T:
	case LEXECOP_BC1T:

/* In exception dst1->i = dst1->i | ( 0x3e & 11); */

	case LEXECOP_BC0TL:
	case LEXECOP_BC1TL:
		{
			int	n, bn ;


			n = (ip->instr >> 18) & 7 ;
		bn = (n != 0) ? (24 + n) : 23 ;
	    *bcnd = ((src1->i >> bn) & 1) ;

		} /* end block */

	    *delay = 2 ;
	    break ;

/* 02/01/09 DAM, end of new versions */


/* This instruction does not do anything for COP0 */
	case LEXECOP_COP0:
	    dst1->i = src1->i ;
	    break ;

/* fixed 01/11/12, David Morano */
/* If exception: dst2->i = dst2->i | ( 0x3e & 11); */
	case LEXECOP_CFC1:
	    dst1->i = src1->i ;
	    break ;

/* fixed 01/11/12, David Morano */
/* If exception: dst2->i = dst2->i | ( 0x3e & 11); */
	case LEXECOP_CTC1:
	    dst1->i = src1->i ;
	    break ;

/* the actual memory transfer is done *elsewhere* */
	case LEXECOP_LDC1:
/* if exception Cause reserved instruction exception 
		       dst2->i = dst2->i | (0x3e & 10) ; 
		    */
	    dp->ma = src1->ui + cnst->ui ;
	    *delay = 2 ;
	    break ;

/* the actual memory transfer is done *elsewhere* */
	case LEXECOP_LL:
/* This is a non r2000/r3000 */
	    dp->ma = src1->ui + cnst->ui ;
	    *delay = 2 ;
	    break ;

	case LEXECOP_LWC1:
	    dp->ma = src1->ui + cnst->ui ;
	    *delay = 2 ;
	    break ;

/****

 GENERAL ASSUMPTIONS FOR CONVERT INSTRUCTIONS:

1. AS accesses the source register (fs), takes its value and puts
it in src1->
2. The result value: dst1 is returned and has to be stored in 
the fd register.

****/

/****

Aqui todavia debo modificar la correcta conversion.
Por ejemplo al convertir a double dos destinos deben ser
modificados, no solo uno 

****/

/* Convert FP single precision to FP double precision */
/* 01/11/12 DAM, fixed */
	case LEXECOP_CVTDS:

#ifdef	COMMENT
	    dst1->f = src1->f ;
	    dst2->f = 0x0000 ;
#else
		{
			union dvar	d1 ;


	    		d1.d = (double) src1->f ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

	} /* end block */
#endif /* F_COMMENT */

	    break ;

/* Convert fixed point to FP double precision */
/* 01/11/12 DAM, fixed */
	case LEXECOP_CVTDW:

#ifdef	COMMENT
	    dst1->f = (float) src1->i ;
	    dst2->f = 0x0000 ;
#else
		{
			union dvar	d1 ;


	    		d1.d = (double) src1->i ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

	} /* end block */
#endif /* COMMENT */

	    break ;

/* Convert FP double precision to FP single precision */
/* 01/11/12 DAM, fixed */
	case LEXECOP_CVTSD:

#ifdef	COMMENT

/* double aux

aux(LO) = src1->f 
aux(HI) = (src1 + 1)->f
dst1 =  (float) aux

*/

	    dst1->f = (float) src1->f ;
#endif

		{
			union dvar	s1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

	    		dst1->f = (float) s1.d ;

	} /* end block */

	    break ;

/* Convert fixed point to FP single precision */
	case LEXECOP_CVTSW:
	    dst1->f = (float) src1->i ;
	    break ;

/* convert FP double precision to fixed point  */
/* 01/11/12 DAM, fixed */
	case LEXECOP_CVTWD:
/* double aux
				 aux(LO) = src1->f 
				 aux(HI) = (src1 + 1)->f
				 dst1 =  (intt) aux
			    */
#ifdef	COMMENT
	    dst1->i = (int) src1->f ;
#else
		{
			union dvar	s1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

	    		dst1->i = (int) s1.d ;

	} /* end block */
#endif /* COMMENT */

	    break ;

/* convert FP single precision to fixed point  */
	case LEXECOP_CVTWS:
	    dst1->i = (int) src1->f ;
	    break ;

/* 01/11/12 DAM, fixed */
	case LEXECOP_SUBD:
		{
			union dvar	s1, s2, d1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

			s2.i.lo = src3->ui ;
			s2.i.hi = src4->ui ;

	    		d1.d = s1.d - s2.d ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

		} /* end block */

	    break ;

	case LEXECOP_SUBS:
	    dst1->f = src1->f - src2->f ;
	    break ;

	case LEXECOP_MULS:
	    dst1->f =  src1->f * src2->f ;
	    break ;

	case LEXECOP_DIVS:
	    if (src2->ui != 0) {

#if	CF_DEBUGS
	debugprintf("lexec: DIVS s1=%08x s2=%08x\n",src1->ui,src2->ui) ;
#endif

	        dst1->f = src1->f / src2->f ;

#if	CF_DEBUGS
	debugprintf("lexec: DIVS d1=%08x \n",dst1->ui) ;
#endif

	    } else {

		dst1->f = FLT_MAX ;
		rs = LEXEC_RDIVZERO ;

		}

	    *delay = 35 ;
	    break ;

/* 01/11/12 DAM, fixed */
	case LEXECOP_DIVD:
		{
			union dvar	s1, s2, d1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

			s2.i.lo = src3->ui ;
			s2.i.hi = src4->ui ;

		d1.d = 0.0 ;
	    if ((src3->ui != 0) || (src4->ui != 0)) {

			d1.d = s1.d / s2.d ;

	    } else {

			d1.d = DBL_MAX ;
		rs = LEXEC_RDIVZERO ;

		}

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

		} /* end block */

	    *delay = 35 ;
	    break ;

/* Assumptions for FP move and load operations:
1. Only compute memory addresses and values
2. It does not accesses registers.
*/

/* AS must access register fs put ts contents on src1->f
and then later take the value of dst1->f  and put it in fd */

/* 01/11/12 DAM, fixed */
	case LEXECOP_MOVD:
	    dst1->i = src1->i ;
		dst2->i = src2->i ;
	    break ;

/* AS must access register fs put ts contents on src1->f
and then later take the value of dst1->f  and put it in
fd */

	case LEXECOP_MOVS:
	    dst1->f = src1->f ;
	    break ;

/* 01/11/12 DAM, fixed */
	case LEXECOP_MULD:
		{
			union dvar	s1, s2, d1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

			s2.i.lo = src3->ui ;
			s2.i.hi = src4->ui ;

	    		d1.d = s1.d * s2.d ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

		} /* end block */

	    break ;

/* 01/11/12 DAM, fixed */
	case LEXECOP_NEGD:

#ifdef	COMMENT
	    dst1->f = (double) ~ src1->i ;
#else
		{
			union dvar	s1, d1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

	    		d1.d = (- s1.d) ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

		} /* end block */
#endif /* COMMENT */

	    break ;

/* 01/11/12 DAM, fixed */
	case LEXECOP_NEGS:

#ifdef	COMMENT
	    dst1->f = (float) ~ src1->i ;
#else
		dst1->f = (- src1->f) ;
#endif /* COMMENT */

	    break ;

	case LEXECOP_RFE:
	    temp =  src1->i & 0xfffffff0 ;
	    temp2 = (src1->i & 0x3c) >> 2 ;
	    dst1->i = temp | temp2 ;
/* also LLbit = 0 */
	    break ;

/* PASS CONTROL TO EXCEPTION HANDLER */
	case LEXECOP_SYSCALL:
	    rs = LEXEC_RUNIMPL ;
	    break ;

/* This instruction is only implemented in R4000
		       It is used to handle returns from interrupts*/
	case LEXECOP_ERET:
	    if (src1->i & 4) {
/* change the ilptr to ERROREPC and set Status register to: */
	        temp = src1->i & 0xfffffff8 ;
	        temp2 = (src1->i & 0x0000000b) |(src1->i & 0x3) ;
	        dst1->i = temp | temp2 ;

	    } else {

/* change the ilptr to EPC and set Status register to: */
	        temp = src1->i & 0xfffffffc ;
	        temp2 = (src1->i & 0x00000002) | (src1->i & 0x1) ;
	        dst1->i = temp | temp2 ;
	    }

	    break ;

	case LEXECOP_SQRTS:
		dst1->f = (float) sqrt(src1->f) ;

		break ;

	case LEXECOP_SQRTD:
		{
			union dvar	s1, d1 ;


			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

	    		d1.d = sqrt(s1.d) ;

			dst1->ui = d1.i.lo ;
			dst2->ui = d1.i.hi ;

		} /* end block */

		break ;

/* NOT implemented instructions */
	case LEXECOP_CEILWS:

	case LEXECOP_FLOORWS:

	case LEXECOP_ROUNDS:

	case LEXECOP_TRUNCWS:

	case LEXECOP_CEILWD:

	case LEXECOP_FLOORWD:

	case LEXECOP_ROUNDD:

	case LEXECOP_TRUNCWD:

	case LEXECOP_TLBP:

	case LEXECOP_TLBR:

	case LEXECOP_TLBWI:

	case LEXECOP_TLBWR:

	case LEXECOP_UNIMPL:

	default:
	    rs = LEXEC_RUNIMPL ;
	    break ;

	} /* end switch */

	dp->f.jump = (fj != 0) ;
	dp->f.taken = (f_taken != 0) ;
	dp->f.nullify = (fn != 0) ;

#if	CF_DEBUGS
		debugprintf("lexec: f_taken=%d\n",dp->f.taken) ;
#endif

	return rs ;
}
/* end subroutine (lexec) */



/* LOCAL SUBROUTINES */



/* Floating point Comparison Instructions Routine */
static int lexec_comp_instr(opexec, instr,src1,src2,src3,src4,src5,dst1)
int	opexec ;
uint	instr ;
DATAVAL *src1, *src2, *src3, *src4, *src5 ;
DATAVAL	*dst1 ;
{
	union dvar	s1, s2, d1 ;

#ifdef	COMMENT
	lexec_type aux ;
#endif

	uint	fcr31 ;

	int	m, bn ;
	int	less, equal,unordered, condition ;


#if	CF_DEBUGS
	{
		uint	tf ;

		char	*ct = "unknown" ;


		tf = BITS21_25(instr) ;
		if (tf == 16)
			ct = "single" ;

		if (tf == 17)
			ct = "double" ;

	debugprintf("lexec/cmp: CMP %s(%d) instr=%08x\n",ct,tf,instr) ;
	}
#endif /* CF_DEBUGS */

	m = (instr >> 8) & 7 ;

/* Distinguish between Simple and Double Precision 
	    1: Double Precision
	    0: Single Precision */

	less = equal = unordered = 0 ;

	if (FMT_(opexec)) {

		fcr31 = src3->ui ;

#if	CF_DEBUGS
	debugprintf("lexec/cmp: CMP SP FCR=%08x\n",src3->ui) ;
#endif

/* Single Precision */

	    if (isnanf(src1->f) || isnanf(src2->f)) {

#if	CF_DEBUGS
	debugprintf("lexec/cmp: CMP SP unordered\n") ;
#endif

	        less = 0 ;
	        equal = 0 ;
	        unordered = 1 ;

#ifdef	COMMENT
	        if (COND3_(opexec)) {
	            printf(
			"lexec_comp_instr: Invalid Operation Exception\n") ;
	            exit(1) ;
	        }
#endif /* COMMENT */

	    } else {

	        if (src1->f < src2->f) {

#if	CF_DEBUGS
	debugprintf("lexec/cmp: CMP SP less\n") ;
#endif

	            less = 1 ;

	        } else {

			less = 0 ;
	            equal = (src1->f == src2->f) ;

#if	CF_DEBUGS
	debugprintf("lexec/cmp: CMP SP %s\n",(equal) ? "equal" : "greater") ;
#endif

	        }

	    }

#ifdef	COMMENT
	    condition = ((COND2_(opexec) & less) ||
	        (COND1_(opexec) & equal) ||
	        (COND0_(opexec) & unordered)) ;
#endif

	} else {

/* Double Precision */

		fcr31 = src5->ui ;

#if	CF_DEBUGS
	debugprintf("lexec/cmp: CMP DP FCR=%08x\n",src5->ui) ;
#endif

			s1.i.lo = src1->ui ;
			s1.i.hi = src2->ui ;

			s2.i.lo = src3->ui ;
			s2.i.hi = src4->ui ;

	    if (isnand(s1.d) || isnand(s2.d)) {

	        less = 0 ;
	        equal = 0 ;
	        unordered = 1 ;

#ifdef	COMMENT
	        if (COND3_(opexec)) {
	            printf("lexec_comp_instr: Invalid Operation Exception\n") ;
	            exit(1) ;
	        }
#endif /* COMMENT */

	    } else {

#ifdef	COMMENT
	        aux.all[0] = (unsigned int) src2->f ;
	        aux.all[0] = (aux.all[0] << 32) | (unsigned int) src1->f ;
	        aux.all[1] = (unsigned int) src4->f ;
	        aux.all[1] = (aux.all[1] << 32) | (unsigned int) src3->f ;
	        aux.temp[0] = (long long) aux.all[0] ;
	        aux.temp[1] = (long long) aux.all[1] ;
	        if (aux.temp[0] < aux.temp[1]) {
	            less = 1 ;

	        } else {

	            if (aux.temp[0] == aux.temp[1]) {
	                less = 0 ;
	                equal = 1 ;

	            } else {
	                less = 0 ;
	                equal = 0 ;
	            }
	        }
#endif /* COMMENT */

	        if (s1.d < s2.d) {

	            less = 1 ;

	        } else {

			less = 0 ;
	            equal = (s1.d == s2.d) ;

	        }


	    } /* end if */

#ifdef	COMMENT
	    condition = ((COND2_(opexec) & less) ||
	        (COND1_(opexec) & equal) ||
	        (COND0_(opexec) & unordered)) ;
#endif

/****

Set the Flag Control Status Register bit 23 with the result of
the condition, thus later this value can be checked by the BC1F
and BC1T instructions 

****/

	} /* end if */

	    condition = ((COND2_(opexec) && less) ||
	        (COND1_(opexec) && equal) ||
	        (COND0_(opexec) && unordered)) ;

	bn = (m != 0) ? (m + 24) : 23 ;
	dst1->ui = fcr31 & (~ (1 << bn)) ;
	dst1->ui |= (condition << bn) ;

#if	CF_DEBUGS
	debugprintf("lexec/cmp: CMP done FCR=%08x\n",dst1->ui) ;
#endif

	return 0 ;
}
/* end subroutine (lexec_comp_instr) */


#if	CF_DEBUGSMAIN

main(){
/* Test for integer operation */
	int s1,s2,s3,s4,dest1,dest2 ;
	int opcode ;
/* add rd,rs,rt   <->  add dst1 s1,s2 */
	opcode = 0 ;
	s1.i = 3 ;
	s2.i = 4 ;
	s3.i = 0 ;
	s4.i = 0 ;
	dst1 = 0 ;
	dst2 = 0 ;
	lexec(opcode,s1,s2,s3,s4,dst1,dst2) ;
	return ;
}

#endif /* CF_DEBUGSMAIN */



