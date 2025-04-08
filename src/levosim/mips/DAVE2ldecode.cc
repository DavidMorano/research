/* ldecode.c */

/* MIPS instruction decoding subroutine for Levo */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_FPRINTF	1		/* fprintf() */


/* revision history:

	= 00/06/15, Marcos de Alba

	This code was started.


	= 01/08/06, Dave Morano

	I added a 'break' statement where one was supposed to be !


*/


/******************************************************************************

   GENERAL NOTES:
   
   1. NEW CLASSES OF INSTRUCTIONS  
   
   include new type of class instructions
   INSTR_JREL        distinguish jump instructions
   INSTR_JIND     distinguish indirect jump instructions
   using INSTR_BREL for conditional branches
   and   INSTR_BIND for indirect conditional branches
  
   2. NOTES FOR UNIMPLEMENTED INSTRUCTIONS
      
   All instructions that are not implemented in r2000/r3000
   have the pattern LEXECOP_UNIMPL_...


******************************************************************************/



#include	<sys/types.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lexec.h"
#include	"ldecode.h"
#include	"ldecodeinstrclass.h"



/* local defines */

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






int ldecode_decode(LDECODE *current_instr) 
{
	uint instr,ilptr,rcve_opcode,exec_opcode,opclass;

	int	rs = LDECODE_ROK ;


      current_instr->opexec = LEXECOP_UNIMPL ;
      current_instr->opclass = INSTR_UNIMPL ;
  instr = current_instr->instrword;
  ilptr = current_instr->ilptr;
  rcve_opcode = OPCODE(instr);

  exec_opcode = BOGUS_OP;

  opclass = BOGUS_OP;
  current_instr->src1 = RS_(instr);

#if F_DEBUGS && F_FPRINTF
  printf("ldecode: instr:%08lx, RS:%d RT:%d\n",
		instr,current_instr->src1,RT_(instr));
#endif

  current_instr->src2 = RT_(instr);
  current_instr->src3 = (int) -1;
  current_instr->src4 = (int) -1;

  current_instr->dst1 = RD_(instr);
  current_instr->dst2 = (int) -1;
  current_instr->dst3 = (int) -1;

  current_instr->const1 = (int) -1;
  current_instr->const1_valid = FALSE ;
 
  switch (rcve_opcode) {  

	case LEVO_OPCODE_SPECIAL:
    ldecode_special(current_instr);
    break;
    
  case LEVO_OPCODE_ADDI: 
    exec_opcode =  LEXECOP_ADDI;
    opclass = INSTR_ALU;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = IMMEDIATE_(instr);
    break;
    
  case LEVO_OPCODE_ADDIU:  
    exec_opcode = LEXECOP_ADDIU;
    opclass = INSTR_ALU;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = IMMEDIATE_(instr);
    break;
    
  case LEVO_OPCODE_ANDI: 
    exec_opcode = LEXECOP_ANDI;
    opclass = INSTR_ALU;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = ZERO_EXT_(instr);
    break;
    
  case LEVO_OPCODE_ORI:  
    exec_opcode = LEXECOP_ORI;
    opclass = INSTR_ALU;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = ZERO_EXT_(instr);
    break;
    
  case LEVO_OPCODE_XORI:  
    exec_opcode = LEXECOP_XORI;
    opclass = INSTR_ALU;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = ZERO_EXT_(instr);
    break;
    
  case LEVO_OPCODE_LUI:  
    exec_opcode = LEXECOP_LUI;
    opclass = INSTR_ALU;
    current_instr->src1 = (int) -1;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = IMMEDIATE_(instr);
    break;
    
  case LEVO_OPCODE_SLTI:  
    exec_opcode = LEXECOP_SLTI; /* set less than immediate  */
    opclass = INSTR_ALU;
    current_instr->src1 = RS_(instr);
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = IMMEDIATE_(instr);
    break;
    
  case LEVO_OPCODE_SLTIU:  
    exec_opcode = LEXECOP_SLTIU; /* set less than immediate unsigned*/
    opclass = INSTR_ALU;  
    current_instr->src1 = RS_(instr);
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = IMMEDIATE_(instr); 
    break;
       
  case LEVO_OPCODE_BEQ:
    exec_opcode = LEXECOP_BEQ;
    opclass = INSTR_BREL;
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = BTA_(instr,ilptr);
    current_instr->dst1 = (int) -1;
    break;
    
  case LEVO_OPCODE_REGIMM:  
    ldecode_regimm(current_instr);
    break;
    
  case LEVO_OPCODE_BGTZ:
    exec_opcode = LEXECOP_BGTZ;
    opclass = INSTR_BREL;
    current_instr->src2 = RT_(instr);
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = BTA_(instr,ilptr);
    break;
    
  case LEVO_OPCODE_BLEZ:
    exec_opcode = LEXECOP_BLEZ;
    opclass = INSTR_BREL;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = BTA_(instr,ilptr) ;
    break;
    
  case LEVO_OPCODE_BNE:
    exec_opcode = LEXECOP_BNE;
    opclass = INSTR_BREL;
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = BTA_(instr,ilptr) ;
    break;
    
  case LEVO_OPCODE_J:
    exec_opcode = LEXECOP_J;
    opclass = INSTR_JREL;
    current_instr->src1 = (int) -1;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = TARGET_(instr,ilptr); 
    break;
    
  case LEVO_OPCODE_JAL:
    /* NOTE: at execution time register 31 has to store the return address
       $31 = ilptr + 8
    */
    exec_opcode = LEXECOP_JAL;
    opclass = INSTR_JREL;
    current_instr->src1 = (int) -1;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = 31;  /* GPR31 = ilptr + 8 */
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = TARGET_(instr,ilptr); 
    break;
    
  case LEVO_OPCODE_LB:       /* byte is sign extended */
    exec_opcode = LEXECOP_LB;
    opclass = INSTR_LOAD;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = OFFSET_(instr); 
    break;
    
  case LEVO_OPCODE_LBU:  /* byte is zero-extended  */
    exec_opcode = LEXECOP_LBU;
    opclass = INSTR_LOAD;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = OFFSET_(instr); 
    break;
    
  case LEVO_OPCODE_LH:  /* half word is sign extended */
    exec_opcode = LEXECOP_LH;
    opclass = INSTR_LOAD;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = OFFSET_(instr); 
    break;
    
  case LEVO_OPCODE_LHU:  
    exec_opcode = LEXECOP_LHU;
    opclass = INSTR_LOAD;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = OFFSET_(instr); 
    break;
		  
  case LEVO_OPCODE_LW:  
    exec_opcode = LEXECOP_LW;
    opclass = INSTR_LOAD;
    current_instr->src2 = (int) -1;
    current_instr->dst1 = RT_(instr);
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = OFFSET_(instr); 
    break;
    /* Future Implementation of LWL and LWR */
    
  case LEVO_OPCODE_LWL:
      exec_opcode = LEXECOP_LWL; 
      opclass = INSTR_LOAD;
      current_instr->src2 = RT_(instr); /* A hack for AS */
      current_instr->dst1 = RT_(instr);
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = OFFSET_(instr); 
      break;
      
  case LEVO_OPCODE_LWR:
      exec_opcode = LEXECOP_LWR; 
      opclass = INSTR_LOAD;
      current_instr->src2 = RT_(instr); /* A hack for AS */
      current_instr->dst1 = RT_(instr);
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = OFFSET_(instr); 
      break;
    
  case LEVO_OPCODE_SB:
    exec_opcode = LEXECOP_SB;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time  */
    current_instr->src2 = RT_(instr); /* base register */
/* The memory address is computed at exec time */
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;
    
  case LEVO_OPCODE_SH:  
    exec_opcode = LEXECOP_SH;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time  */
    current_instr->src2 = RT_(instr); /* base register */
/* The memory address is computed at exec time */
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;
    
  case LEVO_OPCODE_SW:  
    exec_opcode = LEXECOP_SW;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time  */
    current_instr->src2 = RT_(instr); /* base register */
/* The memory address is computed at exec time */ 
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;
    
  case LEVO_OPCODE_SWL:  
    exec_opcode = LEXECOP_SWL;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time  */
    current_instr->src2 = RT_(instr); /* base register */
/* The memory address is computed at exec time */
    current_instr->dst1 = (int) -1;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;
    
  case LEVO_OPCODE_SWR:  
    exec_opcode = LEXECOP_SWR;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time  */
    current_instr->src2 = RT_(instr); /* base register */
/* The memory address is computed at exec time */ 
    current_instr->dst1 = (int) -1 ;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;
  case LEVO_OPCODE_COP0:
    ldecode_cop0(current_instr);
    break;
  case LEVO_OPCODE_COP1:  
    ldecode_fp_cop1(current_instr);
    break;    
  case LEVO_OPCODE_BEQL:
    /* Invalid Instruction. Generates exception, writes a 10 in 
       ExcCode Field at Cause Register. It also saves the status register */
    exec_opcode = LEXECOP_BEQL;
    opclass = INSTR_BREL;
    current_instr->src1 = RS_(instr); 
    current_instr->src2 = RT_(instr); 
    current_instr->dst1 = (int) -1 ;  
    current_instr->dst2 = (int) -1 ;
    current_instr->const1_valid = TRUE ;   
    current_instr->const1 = BTA_(instr,ilptr);   

    break;
  case LEVO_OPCODE_BNEL:
    /* Invalid Instruction. Generates exception, writes a 10 in 
       ExcCode Field at Cause Register. It also saves the status register */
    exec_opcode = LEXECOP_BNEL;
    opclass = INSTR_BREL;
    current_instr->src1 = RS_(instr); 
    current_instr->src2 = RT_(instr); 
    current_instr->dst1 = (int) -1 ;  
    current_instr->dst2 = (int) -1 ;   
    current_instr->const1_valid = TRUE ;   
    current_instr->const1 = BTA_(instr,ilptr);  
    break;
  case LEVO_OPCODE_BGTZL:
    /* Invalid Instruction. Generates exception, writes a 10 in 
       ExcCode Field at Cause Register. It also saves the status register */
    exec_opcode = LEXECOP_BGTZL;
    opclass = INSTR_BREL;
    current_instr->src1 = RS_(instr);  
    current_instr->src2 = RT_(instr);
    current_instr->dst1 = (int) -1 ;  
    current_instr->dst2 = (int) -1 ;
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = BTA_(instr,ilptr);
    break;
  case LEVO_OPCODE_BLEZL:
    exec_opcode = LEXECOP_BLEZL;
    opclass = INSTR_BREL;
    current_instr->src1 = RS_(instr); 
    current_instr->src2 = RT_(instr); 
    current_instr->dst1 = (int) -1 ;
    current_instr->dst2 = (int) -1 ;
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = BTA_(instr,ilptr);
    break;
  case LEVO_OPCODE_LDC1: /* Load Double Word to Double FP register*/
    exec_opcode = LEXECOP_LDC1;
    opclass = INSTR_LOAD;
    current_instr->src1 = RD_(instr);  /* Address of Base Register */
    current_instr->dst1 = FT_(instr) + OFFSET_1;  /* Address of Dst  Register */
    break;
  case LEVO_OPCODE_LL:
    exec_opcode = LEXECOP_LL;
    opclass = INSTR_LOAD;
    current_instr->src1 = RS_(instr); 
    current_instr->src2 = (int) -1 ; 
    current_instr->dst1 = RT_(instr); 
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = OFFSET_(instr);
    break;
  case LEVO_OPCODE_LWC1:
    exec_opcode = LEXECOP_LWC1;
    opclass = INSTR_LOAD;
    current_instr->src1 = RS_(instr); 
    current_instr->src2 = (int) -1 ;
    current_instr->dst1 = FT_(instr)+ OFFSET_1; /* FP unit 1 target register */
    current_instr->const1_valid = TRUE ;
    current_instr->const1 = OFFSET_(instr); 
    break;
    
  case LEVO_OPCODE_SC:  
    /* Invalid Instruction for R2000/R3000 */
    exec_opcode = LEXECOP_SC;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time  */
    current_instr->src2 = RT_(instr); /* base register */
/* The memory address is computed at exec time */ 
    current_instr->dst1 = (int) -1 ;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;
    
/* Invalid Instruction for R2000/R3000 , Store Double Word From Coprocessor */
  case LEVO_OPCODE_SDC1:
    exec_opcode = LEXECOP_SDC1;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time  */
    current_instr->src2 = RT_(instr);
/* The memory address is computed at exec time */ 
    current_instr->dst1 = (int) -1 ;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;

/* Invalid Instruction for R2000/R3000, Store Double Word From Coprocessor */
  case LEVO_OPCODE_SWC1:
    exec_opcode = LEXECOP_SWC1;
    opclass = INSTR_STORE;
    /* The address of the memory location where to store the
       value is computed at execution time.
       Register FT of FP unit 1 is the source 
    */
    current_instr->src2 = FT_(instr) + OFFSET_1;
/* The memory address is computed at exec time */ 
    current_instr->dst1 = (int) -1 ;
    current_instr->const1_valid = TRUE ; 
    current_instr->const1 = OFFSET_(instr); /* the offset */
    break;

  default:
      current_instr->opexec = LEXECOP_UNIMPL ;
      current_instr->opclass = INSTR_UNIMPL ;
	rs = LDECODE_RUNIMPL ;

#ifdef	COMMENT
    ldecode_panic("ldecode_get_opcode: error unimplemented instruction:",
		(uint) instr);
#endif /* COMMENT */

    break;

  } /* end switch */

  if (exec_opcode != BOGUS_OP) {

      current_instr->opexec = exec_opcode;
    }
  if (opclass != BOGUS_OP)
    {
      current_instr->opclass = opclass;
    }

  return rs ;
}
/* end subroutine (ldecode_decode) */



/* LOCAL SUBROUTINES */



int ldecode_special(LDECODE *current_instr) 
{	
  uint instr = current_instr->instrword;
  uint exec_opcode = BOGUS_OP;
  uint opclass = BOGUS_OP;


  current_instr->src1 = RS_(instr);
  current_instr->src2 = RT_(instr);
  current_instr->src3 = (int) -1 ;
  current_instr->src4 = (int) -1 ;
  current_instr->dst1 = RD_(instr);
  current_instr->dst2 = (int) -1 ;
  current_instr->const1 = (int) -1 ;
  current_instr->const1_valid = FALSE ;

  switch (FIELD6_(instr)) {

    case LEVO_SPECIAL_ADD_OPCODE_OVER_F:  
      exec_opcode = LEXECOP_ADD;
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_ADD_OPCODE_NO_OVER_F:  
      exec_opcode = LEXECOP_ADDU;
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_AND_OPCODE:  
      exec_opcode = LEXECOP_AND;
      opclass = INSTR_ALU;
      break;
      /* For division operations:
	 lo = rs/rt
	 hi = rs % rt
      */
    case LEVO_SPECIAL_DIV_OPCODE:  
      exec_opcode = LEXECOP_DIV;
      opclass = INSTR_ALU;
      current_instr->dst1 = LMIPSREG_LO;
      current_instr->dst2 = LMIPSREG_HI;
      break;
    case LEVO_SPECIAL_DIVU_OPCODE:  
      exec_opcode = LEXECOP_DIVU;
      opclass = INSTR_ALU;
      current_instr->dst1 = LMIPSREG_LO;
      current_instr->dst2 = LMIPSREG_HI;
      break;
      /* For multiplication operations (rs * rt):
	 lo will hold the low-order of the product and
	 hi will hold the high-order of the product
      */
    case LEVO_SPECIAL_MULT_OPCODE:  
      exec_opcode = LEXECOP_MULT;
      opclass = INSTR_ALU;
      current_instr->dst1 = LMIPSREG_LO;
      current_instr->dst2 = LMIPSREG_HI;
      break;
    case LEVO_SPECIAL_MULTU_OPCODE:  
      exec_opcode = LEXECOP_MULTU;
      opclass = INSTR_ALU;
      current_instr->dst1 = LMIPSREG_LO;
      current_instr->dst2 = LMIPSREG_HI;
      break;
    case LEVO_SPECIAL_NOR_OPCODE:  
      exec_opcode = LEXECOP_NOR;
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_OR_OPCODE:		
      exec_opcode = LEXECOP_OR;
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_SLL_OPCODE:
      exec_opcode = LEXECOP_SLL;
      opclass = INSTR_ALU;
      current_instr->src1 = RT_(instr);
      current_instr->src2 = (int) -1 ;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = SHAMT_(instr);
      break;
    case LEVO_SPECIAL_SLLV_OPCODE:
      exec_opcode = LEXECOP_SLLV;
      opclass = INSTR_ALU;
      current_instr->src1 = RT_(instr);
      current_instr->src2 = RS_(instr);
      break;
    case LEVO_SPECIAL_SRA_OPCODE:	
      exec_opcode = LEXECOP_SRA;
      opclass = INSTR_ALU;
      current_instr->src1 = RT_(instr);
      current_instr->src2 = (int) -1 ;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = SHAMT_(instr);
      break;
    case LEVO_SPECIAL_SRAV_OPCODE:
      exec_opcode = LEXECOP_SRAV;
      opclass = INSTR_ALU;
      current_instr->src1 = RT_(instr);
      current_instr->src2 = RS_(instr);
      break;
    case LEVO_SPECIAL_SRL_OPCODE:
      exec_opcode = LEXECOP_SRL;
      opclass = INSTR_ALU;
      current_instr->src1 = RT_(instr);
      current_instr->src2 = (int) -1 ;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = SHAMT_(instr);
      break;
    case LEVO_SPECIAL_SRLV_OPCODE:
      exec_opcode = LEXECOP_SRLV;
      opclass = INSTR_ALU;
      current_instr->src1 = RT_(instr);
      current_instr->src2 = RS_(instr);
      break;
    case LEVO_SPECIAL_SUB_OPCODE:  
      exec_opcode = LEXECOP_SUB;
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_SUBU_OPCODE:  
      exec_opcode = LEXECOP_SUBU;
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_XOR_OPCODE:  
      exec_opcode = LEXECOP_XOR;
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_SLT_OPCODE:  
      exec_opcode = LEXECOP_SLT;
    /*  opclass = INSTR_STORE; */
      opclass = INSTR_ALU;
      break;
    case LEVO_SPECIAL_SLTU_OPCODE:
      exec_opcode = LEXECOP_SLTU;
    /*  opclass = INSTR_STORE; */
      opclass = INSTR_ALU;
      break;

    /* Move the HI register to RD */
    case LEVO_SPECIAL_MFHI_OPCODE:

#ifdef	COMMENT
      assert((RS_(instr) == 0) && (RT_(instr) == 0));
#endif

      exec_opcode = LEXECOP_MFHI;
      opclass = INSTR_ALU; /* move from hi */
      current_instr->src1 = LMIPSREG_HI;
      current_instr->src2 = (int) -1 ;
      current_instr->src3 = (int) -1 ;
      current_instr->src4 = (int) -1 ;
      current_instr->dst1 = RD_(instr);
      current_instr->dst2 = (int) -1 ;
      current_instr->const1 = (int) -1 ;
      current_instr->const1_valid = FALSE ;
      break;

   /* Move the LO register to RD */
    case LEVO_SPECIAL_MFLO_OPCODE:

#ifdef	COMMENT
      assert((RS_(instr) == 0) && (RT_(instr) == 0));
#endif

      exec_opcode = LEXECOP_MFLO;
      opclass = INSTR_ALU; /* move from lo */
      current_instr->src1 = LMIPSREG_LO;
      current_instr->src2 = (int) -1 ;
      current_instr->src3 = (int) -1 ;
      current_instr->src4 = (int) -1 ;
      current_instr->dst1 = RD_(instr);
      current_instr->dst2 = (int) -1 ;
      current_instr->const1 = (int) -1 ;
      current_instr->const1_valid = FALSE ;
      break;
	
   /* Move register RS to HI */
    case LEVO_SPECIAL_MTHI_OPCODE:

#ifdef	COMMENT
      assert((RD_(instr) == 0) && (RT_(instr) == 0));
#endif

      exec_opcode = LEXECOP_MTHI;
      opclass = INSTR_ALU; /* move to hi */
      current_instr->src1 = RS_(instr);
      current_instr->src2 = (int) -1 ;
      current_instr->src3 = (int) -1 ;
      current_instr->src4 = (int) -1 ;
      current_instr->dst1 = LMIPSREG_HI;
      current_instr->dst2 = (int) -1 ;
      current_instr->const1 = (int) -1 ;
      current_instr->const1_valid = FALSE ;
      break;

   /* Move register RS to LO */
    case LEVO_SPECIAL_MTLO_OPCODE:

#ifdef	COMMENT
      assert((RD_(instr) == 0) && (RT_(instr) == 0));
#endif

      exec_opcode = LEXECOP_MTLO;
      opclass = INSTR_ALU; /* move to lo */
      current_instr->src1 = RS_(instr);
      current_instr->src2 = (int) -1 ;
      current_instr->src3 = (int) -1 ;
      current_instr->src4 = (int) -1 ;
      current_instr->dst1 = LMIPSREG_LO;
      current_instr->dst2 = (int) -1 ;
      current_instr->const1 = (int) -1 ;
      current_instr->const1_valid = FALSE ;
      break;

    case LEVO_SPECIAL_JALR_OPCODE:
      exec_opcode = LEXECOP_JALR;
      opclass =  INSTR_JIND;
      current_instr->src1 = RS_(instr);  /* Target address stored in RS */
      current_instr->src2 = (int) -1 ;
      current_instr->src3 = (int) -1 ;
      current_instr->src4 = (int) -1 ;
      current_instr->dst1 = RD_(instr); /* Save return address of next instr
					   in this register (by default $31) */
      current_instr->dst2 = (int) -1 ;
      current_instr->const1 = 0;
      current_instr->const1_valid = FALSE ;
      break;
    case LEVO_SPECIAL_JR_OPCODE:
      exec_opcode = LEXECOP_JR;
      opclass = INSTR_JIND;
      current_instr->src1 = RS_(instr);  /* Target address stored in RS */
      current_instr->src2 = RD_(instr);
      current_instr->src3 = (int) -1 ;
      current_instr->src4 = (int) -1 ;
      current_instr->dst1 = (int) -1 ;
      current_instr->dst2 = (int) -1 ;
      current_instr->const1 = 0;
      current_instr->const1_valid = FALSE ;
      break;			
    case LEVO_SPECIAL_SYSCALL_OPCODE:
	exec_opcode = LEXECOP_SYSCALL;
	opclass = INSTR_SPECIAL;
       /* register $v0 contains the number of the system call, see fig 
          A-17 apendix A Computer Organization Patterson/Hennessy spim */
	break;
    case LEVO_SPECIAL_BREAK_OPCODE:
      /* Generates a breakpoint exception
	   bits 6 to 25 contain the cause exception code */  
      exec_opcode = LEXECOP_BREAK;
      opclass = INSTR_SPECIAL;
      current_instr->src1 = 163; /* Address of Cause Register */
      current_instr->src2 = (int) -1 ;
      current_instr->src3 = (int) -1 ;
      current_instr->src4 = (int) -1 ;
      current_instr->dst1 = 163;  /* Address of Cause Register */
      current_instr->dst2 = (int) -1 ;
      current_instr->const1 = 1;
      current_instr->const1_valid = (BREAK_CODE_(instr) != 0) ;
      break;

    case LEVO_SPECIAL_SYNC:
      break;

    }

  if (exec_opcode != BOGUS_OP) {

      current_instr->opexec = exec_opcode;

    }

  if (opclass != BOGUS_OP)
    {
      current_instr->opclass = opclass;
    }

  return LDECODE_ROK;
}
/* end subroutine (ldecode_special) */


int ldecode_fp_cop1(LDECODE *current_instr) 
{
	uint ilptr; 
	uint instr = current_instr->instrword;
	uint format = BITS_6_10(instr);	
	uint op = BITS_11_15(instr);

	int	rs = LDECODE_ROK ;


	ilptr = current_instr->ilptr;

	current_instr->opclass = INSTR_FPALU;

	
	switch (format) {

		case 0:
			if(BITS_21_31(instr) == 0){
				/*  LEVO_COP1_MFC: */
				current_instr->opexec = LEXECOP_MFC1;
				current_instr->src1 = FS_(instr) + OFFSET_1 ;
				current_instr->dst1 = RT_(instr);
			}
			break;
		case LEVO_COP1_FPD_OPCODE:
			ldecode_fpd_op(current_instr);
			break;
		case LEVO_COP1_FPS_OPCODE:
			ldecode_fps_op(current_instr);
			break;
		case LEVO_COP1_WORD_OPCODE:
			ldecode_fpw_op(current_instr);
		  break;
		  
/* 

This case analyzes the special BC1F  and BC1T instructions.
NOTE:
In these two instructions the bits 11 to 15 are used to
distinguish between BC1F and BC1T.
The instructions are identical for singled and double precision

*/

		case LEVO_COP1_FP_BRANCH:
			current_instr->opclass = INSTR_BREL;

/* Condition signal 1, 1-bit register 182  */
			current_instr->src1 = 183;  
			current_instr->src2 = (int) -1 ;
			current_instr->src3 = (int) -1 ;
			current_instr->src4 = (int) -1 ;

/* Exception: Cause reg has to be set to 11 */
			current_instr->dst1 = (int) -1 ;
			current_instr->dst2 = (int) -1 ;
			current_instr->const1_valid = TRUE ;
			current_instr->const1 = BTA_(instr,ilptr);

			switch (op) {

				case 0:
					current_instr->opexec = LEXECOP_BC1F;
					break;
				case 1:
					current_instr->opexec = LEXECOP_BC1T;
					break;
				case 2:
					current_instr->opexec = LEXECOP_BC1FL;
					break;
				case 3:
					current_instr->opexec = LEXECOP_BC1TL;
					break;
				default:
					break;

			} /* end switch */

/* 01/08/06, DAM -- added this 'break' */

			break ;

		case LEVO_COP1_CFC:
			current_instr->opexec = LEXECOP_CFC1;
			current_instr->src1 = FD_(instr);
			current_instr->dst1 = RT_(instr) + OFFSET_1;
			break;

		case LEVO_COP1_CTC:
			if ((FS_(instr) == 0) || (FS_(instr) == 31)) {

				current_instr->opexec = LEXECOP_CTC1;
				current_instr->src1 = RT_(instr);
				current_instr->dst1 = FS_(instr) + OFFSET_1; 

			} else {

#if	F_FPRINTF
				printf("\nldecode__fp_cop1: "
				"Invalid register number for this "
				"operation: %#x\n",FS_(instr));
#endif

			}

			break;

		case LEVO_COP1_MTC:
			if (BITS_21_31(instr) == 0) {

				current_instr->opexec = LEXECOP_MTC1;
				current_instr->src1 = RT_(instr);
				current_instr->dst1 = FD_(instr) + OFFSET_1;
				break;

			} else {

				rs = LDECODE_RUNIMPL ;
		
#ifdef	COMMENT
				printf("ldecode_fp_cop1: unimplemented "
				"instruction: 0x%x\n",instr);
#endif /* COMMENT */

			} /* end if */
			
		default:
			rs = LDECODE_RUNIMPL ;
			break;

	} /* end switch */

	return rs ;
}
/* end subroutine (ldecode_fp_cop1) */


/* Floating Point Double Precision Instructions */
int ldecode_fpd_op(LDECODE *current_instr)
{
	uint instr = current_instr->instrword;

	int thetype = -1;
	int	rs = LDECODE_ROK ;


  current_instr->src1 = -1;
  current_instr->src2 = -1;
  current_instr->src3 = -1;
  current_instr->src4 = -1;
  current_instr->dst1 = -1;
  current_instr->dst2 = -1;
  current_instr->const1 = -1;
  current_instr->const1_valid = FALSE ;
  current_instr->opclass = INSTR_FPALU;

  /* Double precision registers (FPRs, 64-bit) are formed by concatenating
     two single floating point registers (FGRs).
     FPR0  = FGR1 (HI)  +  FGR0 (LO)
     FPR2  = FGR3 (HI)  +  FGR2 (LO)
     FPR4  = FGR5 (HI)  +  FGR4 (LO)
     FPR6  = FGR7 (HI)  +  FGR6 (LO)
     FPR8  = FGR9 (HI)  +  FGR8 (LO) 
     FPR10 = FGR11(HI)  +  FGR10(LO)
     FPR12 = FGR13(HI)  +  FGR12(LO)
     FPR14 = FGR15(HI)  +  FGR14(LO)
     FPR16 = FGR17(HI)  +  FGR16(LO)
     FPR18 = FGR19(HI)  +  FGR18(LO)
     FPR20 = FGR21(HI)  +  FGR20(LO)
     FPR22 = FGR23(HI)  +  FGR22(LO)
     FPR24 = FGR25(HI)  +  FGR24(LO)
     FPR26 = FGR27(HI)  +  FGR26(LO)
     FPR28 = FGR29(HI)  +  FGR28(LO) 
     FPR30 = FGR31(HI)  +  FGR30(LO)
  */

  switch (FIELD6_(instr)) { 

	case LEVO_EXT_ABS_OPCODE:

      current_instr->opexec = LEXECOP_ABSD;
      thetype = 1;
      break;
    case LEVO_EXT_ADDFP_OPCODE: 
      current_instr->opexec = LEXECOP_ADDD;
      thetype = 2;
      break;
    case LEVO_EXT_CF_OPCODE:
      current_instr->opexec = LEXECOP_CMPFD;
      thetype = 3;
      break;
    case LEVO_EXT_CUN_OPCODE:
      current_instr->opexec = LEXECOP_CMPUND;
      thetype = 3;
      break;
    case LEVO_EXT_CEQ_OPCODE: 
      current_instr->opexec = LEXECOP_CMPEQD;
      thetype = 3;
      break;
    case LEVO_EXT_CUEQ_OPCODE: 
      current_instr->opexec = LEXECOP_CMPUEQD;
      thetype = 3;
      break;
    case LEVO_EXT_COLT_OPCODE: 
      current_instr->opexec = LEXECOP_CMPOLTD;
      thetype = 3;
      break;
    case LEVO_EXT_CULT_OPCODE: 
      current_instr->opexec = LEXECOP_CMPULTD;
      thetype = 3;
      break;
    case LEVO_EXT_COLE_OPCODE: 
      current_instr->opexec = LEXECOP_CMPOLED;
      thetype = 3;
      break;
    case LEVO_EXT_CULE_OPCODE: 
      current_instr->opexec = LEXECOP_CMPULED;
      thetype = 3;
      break;
    case LEVO_EXT_CSF_OPCODE:
      current_instr->opexec = LEXECOP_CMPSFD;
      thetype = 3;
      break;
    case LEVO_EXT_CNGLE_OPCODE: 
      current_instr->opexec = LEXECOP_CMPNGLED;
      thetype = 3;
      break;
    case LEVO_EXT_CSEQ_OPCODE: 
      current_instr->opexec = LEXECOP_CMPSEQD;
      thetype = 3;
      break;
    case LEVO_EXT_CNGL_OPCODE: 
      current_instr->opexec = LEXECOP_CMPNGLD;
      thetype = 3;
      break;
    case LEVO_EXT_CLT_OPCODE: 
      current_instr->opexec = LEXECOP_CMPLTD;
      thetype = 3;
      break;
    case LEVO_EXT_CNGE_OPCODE: 
      current_instr->opexec = LEXECOP_CMPNGED;
      thetype = 3;
      break;
    case LEVO_EXT_CLE_OPCODE: 
      current_instr->opexec = LEXECOP_CMPLED;
      thetype = 3;
      break;
	case LEVO_EXT_CNGT_OPCODE: 
	  current_instr->opexec = LEXECOP_CMPNGTD;
      thetype = 3;
      break;
	case LEVO_EXT_CVTW_OPCODE: 
	  current_instr->opexec = LEXECOP_CVTDW;
	  thetype = 1;
	  break;
	case LEVO_EXT_CVTS_OPCODE:  
	  current_instr->opexec = LEXECOP_CVTDS;
	  thetype = 1;
	  break;
	case LEVO_EXT_DIVFP_OPCODE: 
	  current_instr->opexec = LEXECOP_DIVD;
	  thetype = 2;
	  break;
	case LEVO_EXT_MVFP_OPCODE: 
      current_instr->opexec = LEXECOP_MOVD;
      thetype = 1;
      break;
    case LEVO_EXT_MULFP_OPCODE: 
      current_instr->opexec = LEXECOP_MULD;
      thetype = 2;
      break;
    case LEVO_EXT_NEGFP_OPCODE: 
      current_instr->opexec = LEXECOP_NEGD;
      thetype = 1;
      break;
    case LEVO_EXT_SUBFP_OPCODE: 
      current_instr->opexec = LEXECOP_SUBD;
      thetype = 2;
      break;

      /* New implemented instructions */
    case LEVO_EXT_CEIL:
      current_instr->opexec = LEXECOP_CEILWD;
      thetype = 1;
      break;
    case LEVO_EXT_FLOOR:
      current_instr->opexec = LEXECOP_FLOORWD;
      thetype = 1;
      break;
    case LEVO_EXT_ROUND:
      current_instr->opexec = LEXECOP_ROUNDD;
      thetype = 1;
      break;
    case LEVO_EXT_SQRT:
      current_instr->opexec = LEXECOP_SQRTD;
      thetype = 2;
      break;
    case LEVO_EXT_TRUNC:
      current_instr->opexec = LEXECOP_TRUNCWD;
      thetype = 1;
      break;
      
    default:
	rs = LDECODE_RUNIMPL ;

#ifdef	COMMENT
      printf("\nldecode_fpd_op: Invalid opcode, unimplemented instruction\n");
#endif /* COMMENT */
      
  } /* end switch */
  
  switch (thetype) {
    
  case 1:
    current_instr->src1 = FS_(instr) + OFFSET_1;
    current_instr->src2 = FS_(instr) + 1 + OFFSET_1;
    current_instr->dst1 = FD_(instr) + OFFSET_1;
    current_instr->dst2 = FD_(instr) + 1 + OFFSET_1;
    break;
  case 2:
    current_instr->src1 = FS_(instr) + OFFSET_1;
    current_instr->src2 = FS_(instr) + 1 + OFFSET_1;
    current_instr->src3 = FT_(instr) + OFFSET_1 ;
    current_instr->src4 = FT_(instr) + 1 + OFFSET_1;
    current_instr->dst1 = FD_(instr) + OFFSET_1;
    current_instr->dst2 = FD_(instr) + 1 + OFFSET_1;
    break;
  case 3:
    current_instr->src1 = FS_(instr) + OFFSET_1;
    current_instr->src2 = FS_(instr) + 1 + OFFSET_1;
    current_instr->src3 = FT_(instr) + OFFSET_1;
    current_instr->src4 = FS_(instr) + 1 + OFFSET_1;
    current_instr->dst1 = 183;   /* Condition Bit for FP unit 1 */
    break;
    
  default:
    rs = LDECODE_RUNIMPL ;
    
#ifdef	COMMENT
    printf("\nldecode_fpd_op: Invalid instruction format, unimplemented instruction\n");
#endif /* COMMENT */
    
  }
  
  return rs ;
}
/* end subroutine (ldecode_fpd_op) */


int ldecode_fps_op(LDECODE *current_instr)
{
  uint instr = current_instr->instrword;

  
  int thetype = -1;
  int	rs = LDECODE_ROK ;
  current_instr->src1 = -1;
  current_instr->src2 = -1;
  current_instr->src3 = -1;
  current_instr->src4 = -1;
  current_instr->dst1 = -1;
  current_instr->dst2 = -1;
  current_instr->const1 = -1;
  current_instr->const1_valid = FALSE ;
  current_instr->opclass = INSTR_FPALU;
  
  switch (FIELD6_(instr)) {
    
  case LEVO_EXT_ABS_OPCODE:
    current_instr->opexec = LEXECOP_ABSS;
    thetype = 1;
    break;
  case LEVO_EXT_ADDFP_OPCODE: 
    current_instr->opexec = LEXECOP_ADDS;
    thetype = 2;
    break;
  case LEVO_EXT_CF_OPCODE:
    current_instr->opexec = LEXECOP_CMPFS;
    thetype = 3;
    break;
  case LEVO_EXT_CUN_OPCODE:
    current_instr->opexec = LEXECOP_CMPUNS;
    thetype = 3;
    break;
  case LEVO_EXT_CEQ_OPCODE: 
    current_instr->opexec = LEXECOP_CMPEQS;
    thetype = 3;
    break;
  case LEVO_EXT_CUEQ_OPCODE: 
    current_instr->opexec = LEXECOP_CMPUEQS;
    thetype = 3;
    break;
  case LEVO_EXT_COLT_OPCODE: 
    current_instr->opexec = LEXECOP_CMPOLTS;
    thetype = 3;
    break;
  case LEVO_EXT_CULT_OPCODE: 
    current_instr->opexec = LEXECOP_CMPULTS;
    thetype = 3;
    break;
  case LEVO_EXT_COLE_OPCODE: 
    current_instr->opexec = LEXECOP_CMPOLES;
    thetype = 3;
    break;
  case LEVO_EXT_CULE_OPCODE: 
    current_instr->opexec = LEXECOP_CMPULES;
    thetype = 3;
    break;
  case LEVO_EXT_CSF_OPCODE:
    current_instr->opexec = LEXECOP_CMPSFS;
    thetype = 3;
    break;
  case LEVO_EXT_CNGLE_OPCODE: 
    current_instr->opexec = LEXECOP_CMPNGLES;
    thetype = 3;
    break;
  case LEVO_EXT_CSEQ_OPCODE: 
    current_instr->opexec = LEXECOP_CMPSEQS;
    thetype = 3;
    break;
  case LEVO_EXT_CNGL_OPCODE: 
    current_instr->opexec = LEXECOP_CMPNGLS;
    thetype = 3;
    break;
  case LEVO_EXT_CLT_OPCODE: 
    current_instr->opexec = LEXECOP_CMPLTS;
    thetype = 3;
    break;
  case LEVO_EXT_CNGE_OPCODE: 
    current_instr->opexec = LEXECOP_CMPNGES;
    thetype = 3;
    break;
  case LEVO_EXT_CLE_OPCODE: 
    current_instr->opexec = LEXECOP_CMPLES;
    thetype = 3;
    break;
  case LEVO_EXT_CNGT_OPCODE: 
    current_instr->opexec = LEXECOP_CMPNGTS;
    thetype = 3;
    break;
  case LEVO_EXT_CVTD_OPCODE: 
    current_instr->opexec = LEXECOP_CVTSD;
    thetype = 4;
    break;
  case LEVO_EXT_CVTW_OPCODE: 
    current_instr->opexec = LEXECOP_CVTSW;
    thetype = 1;
    break;
  case LEVO_EXT_DIVFP_OPCODE: 
    current_instr->opexec = LEXECOP_DIVS;
    thetype = 2;
    break;
  case LEVO_EXT_MVFP_OPCODE: 
    current_instr->opexec = LEXECOP_MOVS;
    thetype = 1;
    break;
  case LEVO_EXT_MULFP_OPCODE: 
    current_instr->opexec = LEXECOP_MULS;
    thetype = 2;
    break;
  case LEVO_EXT_NEGFP_OPCODE: 
    current_instr->opexec = LEXECOP_NEGS;
    thetype = 1;
    break;
  case LEVO_EXT_SUBFP_OPCODE: 
    current_instr->opexec = LEXECOP_SUBS;
    thetype = 2;
    break;
    
    /* New implemented instructions */
  case LEVO_EXT_CEIL:
    current_instr->opexec = LEXECOP_CEILWS;
    thetype = 1;
    break;
  case LEVO_EXT_FLOOR:
    current_instr->opexec = LEXECOP_FLOORWS;
    thetype = 1;
    break;
  case LEVO_EXT_ROUND:
    current_instr->opexec = LEXECOP_ROUNDS;
    thetype = 1;
    break;
  case LEVO_EXT_SQRT:
    current_instr->opexec = LEXECOP_SQRTS;
    thetype = 2;
    break;
  case LEVO_EXT_TRUNC:
    current_instr->opexec = LEXECOP_TRUNCWS;
    thetype = 1;
    break;
    
  default:
    rs = LDECODE_RUNIMPL ;
    
#ifdef	COMMENT
    printf("\nldecode_fps_op: Invalid opcode, "
		"unimplemented instruction\n");
#endif /* COMMENT */
    
  } /* end switch */
  
  switch (thetype) {
    
  case 1:
    current_instr->src1 = FS_(instr)+ OFFSET_1;
    current_instr->dst1 = FD_(instr)+ OFFSET_1;
    break;
  case 2:
    current_instr->src1 = FS_(instr)+ OFFSET_1;
    current_instr->src2 = FT_(instr)+ OFFSET_1;
    current_instr->dst1 = FD_(instr)+ OFFSET_1;
    break;
  case 3:
    current_instr->src1 = FS_(instr)+ OFFSET_1;
    current_instr->src2 = FT_(instr)+ OFFSET_1;
    current_instr->dst1 = 182;   /* Condition Bit register for single precision  */
    break;
  case 4:
    /* Special case for coversion between FP single and FP double*/
    current_instr->src1 = FS_(instr)+ OFFSET_1;
    current_instr->dst1 = FD_(instr)+ OFFSET_1;
    current_instr->dst2 = FD_(instr)+ OFFSET_1 + 1;
    break;
    
  default:
    rs = LDECODE_RUNIMPL ;
    
#ifdef	COMMENT
    printf("\nldecode_fps_op: Invalid instruction format, "
	   "unimplemented instruction\n");
#endif /* COMMENT */
    
  }
  
  return rs ;
}
/* end subroutine (ldecode_fps_op) */


/* Fixed Point Precision Instructions */
int ldecode_fpw_op(LDECODE *current_instr)
{
  uint instr = current_instr->instrword;

  
  int thetype = -1;
  current_instr->src1 = FS_(instr) + OFFSET_1;
  current_instr->src2 = -1;
  current_instr->src3 = -1;
  current_instr->src4 = -1;
  current_instr->dst1 = FD_(instr) + OFFSET_1;
  current_instr->dst2 = -1;
  current_instr->const1 = -1;
  current_instr->const1_valid = FALSE ;
  current_instr->opclass = INSTR_FPALU;
  
  switch (FIELD6_(instr)) {

  case LEVO_EXT_CVTS_OPCODE: 
    current_instr->opexec = LEXECOP_CVTWS;
    break;
  case LEVO_EXT_CVTD_OPCODE:  
    current_instr->opexec = LEXECOP_CVTWD;
    current_instr->dst2 =  FD_(instr) + OFFSET_1 + 1;
    break;

  } /* end switch */
  
  return SR_OK ;
}
/* end subroutine (ldecode_fpw_op) */

int ldecode_regimm(LDECODE *current_instr)
{
  uint instr = current_instr->instrword;
  uint opclass = BOGUS_OP;
  uint ext_code = BITS_11_15(instr);
  uint ilptr = current_instr->ilptr;


  current_instr->src1 = RS_(instr);
  current_instr->src2 = (int) -1 ;
  current_instr->src3 = (int) -1 ;
  current_instr->src4 = (int) -1 ;
  current_instr->dst1 = (int) -1 ;
  current_instr->dst2 = (int) -1 ;
  current_instr->const1 = (int) -1 ;
  current_instr->const1_valid = FALSE ;
  current_instr->opexec = BOGUS_OP;
  
  switch(ext_code)
    {
    case LEVO_REGIMM_BGEZ_OPCODE:
      current_instr->opexec = LEXECOP_BGEZ;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      break;

      /* This is an R4000 instruction , Added 1/11/2001, r4000 */
    case LEVO_REGIMM_BGEZL_OPCODE:
      current_instr->opexec = LEXECOP_BGEZL;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      break;
    case LEVO_REGIMM_BGEZAL_OPCODE:
      /* Register 31 must hold the return address at execution time */
      current_instr->opexec = LEXECOP_BGEZAL;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      current_instr->dst1 = 0x1f; 
      /* register $31 must hold the return address = ilptr + 8 */
      break;
    case LEVO_REGIMM_BGEZALL_OPCODE:
      /* Register 31 must hold the return address at execution time */
      current_instr->opexec = LEXECOP_BGEZALL;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      current_instr->dst1 = 0x1f; 
      /* register $31 must hold the return address = ilptr + 8 */
      break;
      
    case LEVO_REGIMM_BLTZAL_OPCODE:
      /* Register 31 must hold the return address at execution time */
      current_instr->opexec = LEXECOP_BLTZAL;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      current_instr->dst1 = 0x1f; /* register $31 must hold the return address */
      break;
    case LEVO_REGIMM_BLTZALL_OPCODE:
      /* Register 31 must hold the return address at execution time */
      current_instr->opexec = LEXECOP_BLTZAL;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      current_instr->dst1 = 0x1f; /* register $31 must hold the return address */
      break;
      
    case LEVO_REGIMM_BLTZ_OPCODE:
      current_instr->opexec = LEXECOP_BLTZ;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      break;
    case LEVO_REGIMM_BLTZL_OPCODE:
      current_instr->opexec = LEXECOP_BLTZL;
      opclass = INSTR_BREL;
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      break;
    }
  
  if (current_instr->opexec != BOGUS_OP)
    {
      current_instr->opexec = current_instr->opexec;
    }

  if (opclass != BOGUS_OP)
    {
      current_instr->opclass = opclass;
    }

  return SR_OK;
}
/* end subroutine (ldecode_regimm) */


int ldecode_cop0(LDECODE *current_instr)
{
  uint instr = current_instr->instrword;
  uint COPSUBOP = BITS_6_10(instr);  /* Coprocessor Sub Operation */
  uint branchtype = BITS_11_15(instr);
  uint funct = BITS_26_31(instr);
  uint ilptr = current_instr->ilptr;


  current_instr->src1 = (int) -1 ;
  current_instr->src2 = (int) -1 ;
  current_instr->src3 = (int) -1 ;
  current_instr->src4 = (int) -1 ;
  current_instr->dst1 = (int) -1 ;
  current_instr->dst2 = (int) -1 ;
  current_instr->const1 = (int) -1 ;
  current_instr->const1_valid = FALSE ;
  current_instr->opexec = BOGUS_OP;

  if (BIT_6(instr) == 1) {

    if ((BITS_7_25(instr) == 0)) {


      current_instr->opclass = INSTR_SPECIAL;

      switch (funct) {

	/* RFE instruction */
      case LEVO_OPCODE_COP0_RFE:
	current_instr->opexec = LEXECOP_RFE;
	current_instr->src1 = 165; /* Address of the Status Register */
	current_instr->dst1 = 165;
	break;
      case LEVO_OPCODE_COP0_ERET:
	current_instr->opexec = LEXECOP_ERET;
	current_instr->src1 = 165; /* Address of the Status Register */
	current_instr->dst1 = 165;
	break;
      case LEVO_OPCODE_COP0_TLBP:
	current_instr->opexec = LEXECOP_TLBP;
	break;
      case LEVO_OPCODE_COP0_TLBR:
	current_instr->opexec = LEXECOP_TLBR;
	break;
      case LEVO_OPCODE_COP0_TLBWI:
	current_instr->opexec = LEXECOP_TLBWI;
	break;
      case LEVO_OPCODE_COP0_TLBWR:
	current_instr->opexec = LEXECOP_TLBWR;
	break;
      default:
	break;

      } /* end switch */

    }

  } else{

    current_instr->opclass = INSTR_FPALU; 
    /* Branch on Flag Bit Instructions */
    switch (COPSUBOP) {

      /* MFC0 */
    case 0:
      current_instr->opexec = LEXECOP_MFC0;
      current_instr->src1 = RD_(instr) + OFFSET_0;
      current_instr->dst1 = RT_(instr);
      break;
    case 4:
      current_instr->opexec = LEXECOP_MTC0;
      current_instr->src1 = RT_(instr);
      current_instr->dst1 = RD_(instr) + OFFSET_0;
      break;
    case 8:
/* Condition signal 1, 1-bit register 182  */
      current_instr->src1 = 183;  
/* Cause register has to be set to 11 */
      current_instr->dst1 = (int) -1 ;  
      current_instr->const1_valid = TRUE ;
      current_instr->const1 = BTA_(instr,ilptr);
      current_instr->opclass = INSTR_BREL;

      switch (branchtype) {

      case 0:
	/* If condition of a compare instruction is not true,
	   then jump to computed target address.
	   Causes a unusable exception.
	*/
	current_instr->opexec = LEXECOP_BC0F;
	break;
      case 1:
	current_instr->opexec = LEXECOP_BC0T;
	break;
      case 2: 
	current_instr->opexec = LEXECOP_BC0FL;
	break;
      case 3: 
	current_instr->opexec = LEXECOP_BC0TL;
	break;
      default:
	printf("\nldecode_cop0: Unimplemented instruction: 0x%x",instr);
	break;

      } /* end switch (branchtype) */
      break;

    default:
      /* Coprocessor Operation. Nothing to do in COP0 */
      current_instr->opexec = LEXECOP_NOCOP0;
      current_instr->opclass = INSTR_FPALU;
      break;

    } /* end switch (COP0SUBOP) */

  } /* end if */

  return SR_OK ;
}
/* end subroutine (ldecode_cop0) */
 

void ldecode_printinstr(LDECODE *current_instr)
{


  eprintf("instruction: 0x%08x\n"
          "opexec: %d\n"
          "opclass: %d\n"
          "ilptr: 0x%08x\n"
          "src1: %d\n"
          "src2: %d\n"
          "src3: %d\n"
          "src4: %d\n"
          "dst1: %d\n"
          "dst2: %d\n"
          "const1: %d=0x%08x\n"
          "const1_valid: %d\n",
		   current_instr->instrword,
		   current_instr->opexec,
		   current_instr->opclass,  
		   current_instr->ilptr,         
		   current_instr->src1,
		   current_instr->src2,
		   current_instr->src3,
		   current_instr->src4,
		   current_instr->dst1,
		   current_instr->dst2,
		   current_instr->const1,current_instr->const1,           
		   current_instr->const1_valid);

}
/* end subroutine (printinstr) */



