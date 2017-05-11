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


/* Register Address Offsets */
  
#define OFFSET_0 32
#define OFFSET_1 64


#define BOGUS_OP 0xFFFF

#define LEVO_OPCODE_SPECIAL 0x0  
#define LEVO_OPCODE_REGIMM  0x1
#define LEVO_OPCODE_J       0x2
#define LEVO_OPCODE_JAL     0x3
#define LEVO_OPCODE_BEQ     0x4
#define LEVO_OPCODE_BNE     0x5
#define LEVO_OPCODE_BLEZ    0x6
#define LEVO_OPCODE_BGTZ    0x7
#define LEVO_OPCODE_ADDI    0x8
#define LEVO_OPCODE_ADDIU   0x9
#define LEVO_OPCODE_SLTI    0xa
#define LEVO_OPCODE_SLTIU   0xb
#define LEVO_OPCODE_ANDI    0xc
#define LEVO_OPCODE_ORI     0xd
#define LEVO_OPCODE_XORI    0xe
#define LEVO_OPCODE_LUI     0xf

/* LOAD AND STORE INSTRUCTIONS */
#define LEVO_OPCODE_LB      0x20
#define LEVO_OPCODE_LH      0x21
#define LEVO_OPCODE_LWL     0x22
#define LEVO_OPCODE_LW      0x23
#define LEVO_OPCODE_LBU     0x24
#define LEVO_OPCODE_LHU     0x25
#define LEVO_OPCODE_LWR     0x26
#define LEVO_OPCODE_SB      0x28
#define LEVO_OPCODE_SH      0x29
#define LEVO_OPCODE_SWL     0x2a
#define LEVO_OPCODE_SW      0x2b
#define LEVO_OPCODE_SWR     0x2e

 /* Not valid for R2000/R3000 but cause exception */
#define LEVO_OPCODE_BEQL    0x14
#define LEVO_OPCODE_BNEL    0x15 
#define LEVO_OPCODE_BLEZL   0x16 
#define LEVO_OPCODE_BGTZL   0x17
#define LEVO_OPCODE_LL      0x30  
#define LEVO_OPCODE_LDC1    0x35
#define LEVO_OPCODE_SC      0x38
#define LEVO_OPCODE_SDC1    0X3d

/* COPROCESSOR RELATED INSTRUCTIONS */
#define LEVO_OPCODE_COP0    0x10
#define LEVO_OPCODE_COP1    0x11

/* LOAD AND STORE INSTRUCTIONS COPROCESSOR INVOLVED (FLOATING POINT) */
#define LEVO_OPCODE_LWC1    0x31
#define LEVO_OPCODE_SWC0    0x38
#define LEVO_OPCODE_SWC1    0x39

/* SPECIAL FUNCTIONS */
#define LEVO_SPECIAL_SLL_OPCODE         0x0
#define LEVO_SPECIAL_SRL_OPCODE         0x2
#define LEVO_SPECIAL_SRA_OPCODE         0x3
#define LEVO_SPECIAL_SLLV_OPCODE        0x4
#define LEVO_SPECIAL_SRLV_OPCODE        0x6
#define LEVO_SPECIAL_SRAV_OPCODE        0x7
#define LEVO_SPECIAL_JR_OPCODE          0x8
#define LEVO_SPECIAL_JALR_OPCODE        0x9
#define LEVO_SPECIAL_SYSCALL_OPCODE     0xc
#define LEVO_SPECIAL_BREAK_OPCODE       0xd
#define LEVO_SPECIAL_SYNC               0xf
#define LEVO_SPECIAL_MFHI_OPCODE        0x10
#define LEVO_SPECIAL_MFLO_OPCODE        0x12
#define LEVO_SPECIAL_MTHI_OPCODE        0x11
#define LEVO_SPECIAL_MTLO_OPCODE        0x13
#define LEVO_SPECIAL_MULT_OPCODE        0x18
#define LEVO_SPECIAL_MULTU_OPCODE       0x19
#define LEVO_SPECIAL_DIV_OPCODE         0x1a
#define LEVO_SPECIAL_DIVU_OPCODE        0x1b
#define LEVO_SPECIAL_ADD_OPCODE_OVER_F  0x20
#define LEVO_SPECIAL_ADD_OPCODE_NO_OVER_F 0x21
#define LEVO_SPECIAL_SUB_OPCODE         0x22
#define LEVO_SPECIAL_SUBU_OPCODE        0x23
#define LEVO_SPECIAL_AND_OPCODE         0x24
#define LEVO_SPECIAL_OR_OPCODE          0x25
#define LEVO_SPECIAL_XOR_OPCODE         0x26
#define LEVO_SPECIAL_NOR_OPCODE         0x27
#define LEVO_SPECIAL_SLT_OPCODE         0x2a
#define LEVO_SPECIAL_SLTU_OPCODE        0x2b

/* COPROCESSOR 0 INSTRUCTIONS */
#define LEVO_OPCODE_COP0_RFE          0x20
#define LEVO_OPCODE_COP0_ERET         0x18
#define LEVO_COP0_MFC0_OPCODE         0x0
#define LEVO_COP0_MTC0_OPCODE         0x4
#define LEVO_OPCODE_COP0_TLBP         0x8
#define LEVO_OPCODE_COP0_TLBR         0x1
#define LEVO_OPCODE_COP0_TLBWI        0x2
#define LEVO_OPCODE_COP0_TLBWR        0x6


/* REGISTER IMMEDIATE INSTRUCTIONS */
#define LEVO_REGIMM_BLTZ_OPCODE      0x0
#define LEVO_REGIMM_BGEZ_OPCODE      0X1
#define LEVO_REGIMM_BLTZL_OPCODE     0x2
#define LEVO_REGIMM_BGEZL_OPCODE     0X3  /* Added 1/11/2001, r4000 */
#define LEVO_REGIMM_BLTZAL_OPCODE    0x10
#define LEVO_REGIMM_BLTZALL_OPCODE   0x12 /* not r2000/r3000 */
#define LEVO_REGIMM_BGEZAL_OPCODE    0x11
#define LEVO_REGIMM_BGEZALL_OPCODE   0x13


/* Definitions for Floating Point Instructions (Coprocessor 1)*/
#define LEVO_COP1_FPS_OPCODE 16
#define LEVO_COP1_FPD_OPCODE 17
#define LEVO_COP1_WORD_OPCODE 20
#define LEVO_COP1_FP_BRANCH  0x8
#define LEVO_COP1_CTC       0x6
#define LEVO_COP1_CFC       0x2
#define LEVO_COP1_MFC       0
#define LEVO_COP1_MTC       0x4

#define LEVO_EXT_ADDFP_OPCODE 0x0
#define LEVO_EXT_SUBFP_OPCODE 0x1
#define LEVO_EXT_MULFP_OPCODE 0x2
#define LEVO_EXT_DIVFP_OPCODE 0x3
#define LEVO_EXT_SQRT         0x4
#define LEVO_EXT_ABS_OPCODE 0x5
#define LEVO_EXT_MVFP_OPCODE 0X6
#define LEVO_EXT_NEGFP_OPCODE 0x7
#define LEVO_EXT_ROUND 0xc
#define LEVO_EXT_TRUNC 0xd
#define LEVO_EXT_CEIL 0xe
#define LEVO_EXT_FLOOR 0xf
#define LEVO_EXT_CVTS_OPCODE 0x20
#define LEVO_EXT_CVTD_OPCODE 0x21
#define LEVO_EXT_CVTW_OPCODE 0x24


/* This comparision instructions do not take a trap */ 
#define LEVO_EXT_CF_OPCODE 0x30
#define LEVO_EXT_CUN_OPCODE 0x31
#define LEVO_EXT_CEQ_OPCODE 0x32
#define LEVO_EXT_CUEQ_OPCODE 0x33
#define LEVO_EXT_COLT_OPCODE 0x34
#define LEVO_EXT_CULT_OPCODE 0x35
#define LEVO_EXT_COLE_OPCODE 0x36
#define LEVO_EXT_CULE_OPCODE 0x37

/* This comparison instructions take an invalid trap if the operands are 
   unordered */
#define LEVO_EXT_CSF_OPCODE 0x38
#define LEVO_EXT_CNGLE_OPCODE 0x39
#define LEVO_EXT_CSEQ_OPCODE 0x3a
#define LEVO_EXT_CNGL_OPCODE 0x3b
#define LEVO_EXT_CLT_OPCODE 0x3c
#define LEVO_EXT_CNGE_OPCODE 0x3d
#define LEVO_EXT_CLE_OPCODE 0x3e
#define LEVO_EXT_CNGT_OPCODE 0x3f

/* #define LEVO_EXT_RFE_OPCODE 0x20 */
/*#define LEVO_OPCODE_RFE 0x10 */



#define LEVO_OPCODE_MFCZ 0x1zx
#define LEVO_OPCODE_MCZ 0x1z

/* Macros to obtain the different fields of an instruction */

#define OPCODE(_instr) ((_instr) >> 26 & 0x3f) /*opcode is bits 0-5 of instr */
#define BITS_0_25(_instr) ((_instr) >> 6)
#define BITS_6_10(_instr) ((_instr) >> 21 & 0x1f)
#define BITS_11_15(_instr) ((_instr) >> 16 & 0x1f)
#define BITS_16_20(_instr) ((_instr) >> 11 & 0x1f)
#define BITS_21_25(_instr) ((_instr) >> 6 & 0x1f)
#define BITS_21_31(_instr) ((_instr) & 0x7ff)
#define BITS_26_31(_instr) ((_instr) & 0x3f)

#define BITS_16_25(_instr) ((_instr) >> 6 & 0x3ff)
#define BITS_11_26(_instr) ((_instr) >> 5 & 0xffff)
#define BITS_27_31(_instr) ((_instr) & 0x1f)
#define BITS_28_31(_instr) ((_instr) & 0xf)

#define BIT_6(_instr) ((_instr) >> 25 & 1)
#define BITS_7_25(_instr) ((_instr) >> 6 & 0x7ffff)

#define RS_(_instr) BITS_6_10(_instr)  /*rs is bits 6-10 of instr */
#define RT_(_instr) BITS_11_15(_instr)   /*rt is bits 11-15 of instr */
#define RD_(_instr) BITS_16_20(_instr)   /*rd is bits 16-20 of instr */
#define FIELD5_(_instr) BITS_21_25(_instr)  /*field5 is bits 21-25 of instr  */
#define FIELD6_(_instr) BITS_26_31(_instr)   /*field6 is bits 26-31 of instr */
#define FIELD7_(_instr) BITS_28_31(_instr)  /*field7 is bits 28-31 */
#define COND_(_instr) BITS_27_31(_instr)  /*Condition for comparison instrs */
#define SIGN_IMM(_instr) ((_instr) >> 15 & 1)
#define IMMEDIATE_(_instr) ((SIGN_IMM(_instr) == 0) ? (_instr) & 0xffff : (_instr) & 0xffff | 0xffff0000)
#define SHAMT_(_instr) BITS_21_25(_instr)
#define BTA_(_instr,_ilptr) ((SIGN_IMM(_instr) == 0) ? ((((_instr) & 0xffff) << 2) + 4 + ilptr) : (((((_instr) & 0xffff) << 2) | 0xffff0000) + 4 + ilptr))
#define OFFSET_(_instr) IMMEDIATE_(_instr)
#define TARGET_(_instr,_ilptr)((((_instr) & 0x3ffffff) << 2)|((_ilptr) & 0xf0000000))
#define JUMP_REG_(_instr) BITS_27_31(_instr) 

#define FT_(_instr) BITS_11_15(_instr)
#define FS_(_instr) BITS_16_20(_instr)
#define FD_(_instr) BITS_21_25(_instr)
#define FC_(_instr) ((_instr) >> 4 & 0x3)
#define NEXT_FC_(_instr) ((_instr) & 0xf)
#define BREAK_CODE_(_instr) ((_instr) & 0x3ffffc0)
#define ZERO_EXT_(_imm) (0x0000ffff&_imm)
#define UPPER_IMM_(_imm) (_imm<<16)

/* Macros to handle exceptions */



#define ldecode_panic(_err,_instr)	\
printf("\n%s 0x%x\n",_err,_instr);



struct ldecode_head { 
	uint instrword;   /* MIPS instruction opcode*/
	uint opexec;  /* Pseudo executable opcode used by lexec */
	uint opclass;   /* Instruction class defined in ldecodeinstrclass.h */
	uint ilptr;          /* ilptr for this instruction */
	uint	src1;
	uint	src2;
	uint	src3;
	uint	src4;
	uint	dst1;
	uint	dst2;
	uint	dst3 ;
	uint	const1 ;
	uint const1_valid; /* whether constant valid or not */
	int btype;   /* Branch Direction: BACKWARD: -1 or FORWARD: 1 */
};



void printinstr(LDECODE *current_instr);


#endif  /* LDECODE_INCLUDE */



