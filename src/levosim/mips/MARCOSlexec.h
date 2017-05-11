/*  lexec.h */

/* Levo Instruction Execution */



/* revision history:
   = 01/01/23
   Marcos R de Alba
   Included the R4000 instructions: CEIL.W, TRUNC, SQRT, ROUND and FLOOR
   Extended the number of source and destination registers
   to handle floating point double precision

   Removed instructions:
     LDC0 
     CTC0
     

   = 00/06/23
   Marcos R de Alba
   Extended MIPS Instructions Set and right implementation of
   instructions.

   = 00/08/09, Marcos de Alba

   Included new definitions to cover compare instructions

   = 00/06/24, David Morano

   Corrected parameters style.


*/

/*
   MIPS R3000 ISA execution interface for use in LEVO simulator 
*/

#ifndef LEXEC_INCLUDE
#define LEXEC_INCLUDE

/* include some files that we need */

#include        "dataval.h"


#define	LEXEC_ROK	0		/* executed OK */
#define	LEXEC_REXCEPT	-1		/* caused a machine exception */
#define	LEXEC_RUNIMPL	-2		/* unimplemented instruction */

#define LEXECOP_NOOP 0
#define LEXECOP_ADD    1  /* Add                        */
#define LEXECOP_ADDU   2  /* Add Unsigned               */
#define LEXECOP_ADDI   3  /* Add Immediate              */
#define LEXECOP_ADDIU  4  /* Add Immediate Unsigned     */
#define LEXECOP_AND    5  /* And                        */
#define LEXECOP_ANDI   6  /* And Immediate              */
#define LEXECOP_DIV    7  /* Division                   */
#define LEXECOP_DIVU   8  /* Division  Unsigned         */
#define LEXECOP_MULT   9  /* Multiply        */
#define LEXECOP_MULTU  10  /* Multiply Unsigned       */
#define LEXECOP_NOR    11  /* Nor */
#define LEXECOP_OR     12  /* Or                        */
#define LEXECOP_ORI    13  /* Or Immediate                        */
#define LEXECOP_SLL    14 /* Shift Left Logical        */
#define LEXECOP_SLLV   15 /* Shift Left Logical        */
#define LEXECOP_SRA    16 /* Shift Register Arithmetic V */
#define LEXECOP_SRAV   17 /* Shift Register Arithmetic V */
#define LEXECOP_SRL    18 /* Shift Right Logical       */
#define LEXECOP_SRLV   19 /* Shift Right Logical       */
#define LEXECOP_SUB    20  /* Subtract                  */
#define LEXECOP_SUBU   22  /* Subtract                  */
#define LEXECOP_XOR    23  /* xor */
#define LEXECOP_XORI   24  /* xor immediate */

/* Constant immediate instructions */
#define LEXECOP_LUI    25 /* Load Upper Immediate  */

/* Comparison instructions */
#define LEXECOP_SLT    26  /* Set on Less Than */
#define LEXECOP_SLTU   27  /* Set on Less Than Unsigned */
#define LEXECOP_SLTI   28  /* Set on Less Than Immediate  */
#define LEXECOP_SLTIU  29  /* Set on Less Than Immediate Unsigned */

/* Branch instructions */
#define LEXECOP_BEQ    30 /* Branch if Equal  */
#define LEXECOP_BGEZ   31 /* Branch if Greater than Equal to Zero */
#define LEXECOP_BGEZAL 32 /* Branch if Greater than Equal to Zero and
Link*/
#define LEXECOP_BGTZ   33 /* Branch on Greater than Zero */

#define LEXECOP_BLEZ   34 /* Branch on Less than or Equal to Zero */
#define LEXECOP_BLTZAL 35 /* Branch on Less than or Equal to Zero and Link */
#define LEXECOP_BLTZ   36 /* Branch on Less than Zero */
#define LEXECOP_BNE    37 /* Branch on not Equal       */

/* Jump Instructions */
#define LEXECOP_J      38 /* Jump */
#define LEXECOP_JAL    39 /* Jump and Link */
#define LEXECOP_JALR   40 /* Jump and Link             */
#define LEXECOP_JR     41 /* Jump Register             */

/* Load instructions */
#define LEXECOP_LB     42 /* Load Byte */
#define LEXECOP_LBU    43 /* Load Byte Unsigned */
#define LEXECOP_LH     44 /* Load Halfword */
#define LEXECOP_LHU    45 /* Load Halfword Unsigned */

#define LEXECOP_LW     46 /*  Load Word */
#define LEXECOP_LWL    47 /* Load Word Left */
#define LEXECOP_LWR    48 /* Load Word Right */

/* Store Instructions */

#define LEXECOP_SB     49  /* Store Byte */
#define LEXECOP_SH     50  /* Store Halfword */
#define LEXECOP_SW     51 /* Store Word               */
#define LEXECOP_SWL    52 /* Store Word Left */
#define LEXECOP_SWR    53 /* Store Word Right */

/* Data Movement */
#define LEXECOP_MFHI   54 /* Move From Hi */
#define LEXECOP_MFLO   55 /* Move From Lo */
#define LEXECOP_MTHI   56 /* Move To Hi */
#define LEXECOP_MTLO   57 /* Move To Lo */


/* floating point opcode macros */
#define LEXECOP_ABSD   58 /* Absolute Value Double */
#define LEXECOP_ABSS   59 /* Absolute Value Single */
#define LEXECOP_ADDD   60 /* Addition Double */
#define LEXECOP_ADDS   61 /* Addition Single */


/* Definitions for Compare Instructions.
   NOTE:

   The notation for this instructions is used to simplify its manipulation
   on lexec.
   */
/* Do not take trap */

#define LEXECOP_CMPFD     0x80 /* 128 decimal */
#define LEXECOP_CMPFS     0x90 /* 144 " */
#define LEXECOP_CMPUND    0x81 /* 129 " */
#define LEXECOP_CMPUNS    0x91 /* 145 " */
#define LEXECOP_CMPEQD    0x82  /* 130 " */
#define LEXECOP_CMPEQS    0x92 /* 146 " */
#define LEXECOP_CMPUEQD   0x83 /* 131 " */
#define LEXECOP_CMPUEQS   0x93 /* 147 " */
#define LEXECOP_CMPOLTD   0x84 /* 132 " */
#define LEXECOP_CMPOLTS   0x94  /* 148 " */
#define LEXECOP_CMPULTD   0x85  /* 133 " */
#define LEXECOP_CMPULTS   0x95  /* 149 " */
#define LEXECOP_CMPOLED   0x86  /* 134 " */
#define LEXECOP_CMPOLES   0x96  /* 150 " */
#define LEXECOP_CMPULED   0x87  /* 135 " */
#define LEXECOP_CMPULES   0x97 /* 151 " */

/* Do take trap */ 
#define LEXECOP_CMPSFD    0x88 /* 136 " */
#define LEXECOP_CMPSFS    0x98 /* 152 " */
#define LEXECOP_CMPNGLED  0x89 /* 137 " */
#define LEXECOP_CMPNGLES  0x99 /* 153 " */
#define LEXECOP_CMPSEQD   0x8a /* 138 " */
#define LEXECOP_CMPSEQS   0x9a /* 154 " */
#define LEXECOP_CMPNGLD   0x8b /* 139 " */
#define LEXECOP_CMPNGLS   0x9b /* 155 " */
#define LEXECOP_CMPLTD    0x8c /* 140 " */ 
#define LEXECOP_CMPLTS    0x9c /* 156 " */
#define LEXECOP_CMPNGED   0x8d /* 141 " */
#define LEXECOP_CMPNGES   0x9d /* 157 " */
#define LEXECOP_CMPLED    0x8e /* 142 " */
#define LEXECOP_CMPLES    0x9e /* 158 " */ 
#define LEXECOP_CMPNGTD   0x8f /* 143 " */
#define LEXECOP_CMPNGTS   0x9f /* 159 decimal */

#define LEXECOP_CVTDS     62 /* Convert single precision to double */
#define LEXECOP_CVTDW     63 /* Convert fixed point to double precision */
#define LEXECOP_CVTSD     64 /* Convert double to single precision */
#define LEXECOP_CVTSW     65 /* Convert fixed point to single precision */  
#define LEXECOP_CVTWS     66 /* Convert single precision to fixed point */  
#define LEXECOP_CVTWD     67 /* Convert double precision to fixed point*/

#define LEXECOP_DIVS      68  /* Divide single-precision FP numbers      */   
#define LEXECOP_DIVD      69 /* Divide double-precision FP numbers      */
#define LEXECOP_MOVD      70 /* Move FP Double */
#define LEXECOP_MOVS      71 /* Move FP Single */
#define LEXECOP_MULD      72 /* Multiply double-precision FP numbers    */
#define LEXECOP_MULS      73 /* Multiply double-precision FP numbers    */
#define LEXECOP_NEGD      74 /* Negate Double */
#define LEXECOP_NEGS      75 /* Negate Single */
#define LEXECOP_SWF       76 /* Store FP Double */
#define LEXECOP_SWS       77 /* Store FP Single */
#define LEXECOP_SUBD      78 /* Substract FP Double */
#define LEXECOP_SUBS      79 /* Substract FP Single */

/* Instructions to check the compare flag in the FP Control Status Register */
#define LEXECOP_BC0T      80 /* Branch on FP True                       */
#define LEXECOP_BC0F      81 /* Branch on FP False   */
#define LEXECOP_BC1T      82
#define LEXECOP_BC1F      83

/* Exception and Interrupt Instructions */
#define LEXECOP_RFE       84 /* Return From Exception */
#define LEXECOP_SYSCALL   85 /* System Call */
#define LEXECOP_BREAK     86

#define LEXECOP_BNEL      87
#define LEXECOP_BEQL      88
#define LEXECOP_BGEZL     89
#define LEXECOP_BLEZL     90

#define LEXECOP_CFC1      91

#define LEXECOP_COP0      92
#define LEXECOP_CTC1      94

#define LEXECOP_LDC1      96
#define LEXECOP_LL        97
#define LEXECOP_LWC1      98
#define LEXECOP_SWC1      99

#define LEXECOP_SYNC      100

/* Data movement between general purpose 
   registers and Floating point registers */

#define LEXECOP_MFC0   101
#define LEXECOP_SC     102
#define LEXECOP_SDC1   103

/* Branch on Coprocessor Instructions */
#define LEXECOP_BC0FL   104
#define LEXECOP_BC1FL   105
#define LEXECOP_BC0TL   106
#define LEXECOP_BC1TL   107

#define LEXECOP_BGEZALL 108
#define LEXECOP_BGTZL   109
#define LEXECOP_BLTZALL 110
#define LEXECOP_BLTZL   111

#define LEXECOP_RET   112   /* Valid only for R4000 */

#define LEXECOP_MTC0   113

#define LEXECOP_ERET  114
#define LEXECOP_TLBP   115
#define LEXECOP_TLBR   116
#define LEXECOP_TLBWI  117
#define LEXECOP_TLBWR  118
#define LEXECOP_MTC1   119
#define LEXECOP_MFC1   120
#define LEXECOP_NOCOP0	121

#define	LEXECOP_MAX	122

/* New implemented instructions */
#define LEXECOP_CEILWD    160    
#define LEXECOP_CEILWS    161
#define LEXECOP_FLOORWD   162
#define LEXECOP_FLOORWS   163
#define LEXECOP_ROUNDD    164
#define LEXECOP_ROUNDS    165
#define LEXECOP_SQRTD     166
#define LEXECOP_SQRTS     167
#define LEXECOP_TRUNCWD   168
#define LEXECOP_TRUNCWS   169


/* Additional Macros for Comparison Instructions */

#define COND0_(_opexec) ((_opexec) & 1)
#define COND1_(_opexec) ((_opexec) & 2)
#define COND2_(_opexec) ((_opexec) & 4)
#define COND3_(_opexec) ((_opexec) & 8)
#define FMT_(_opexec) ((((_opexec) & 0xf0) == 80) ? 1 : 0) /* 1: double, 0: single */


  /* Variables for Floating Point Double Precision Conversion */

typedef struct lexec_type_{
  long long    all[2];
  double       temp[2];
  long long    res[2];
}lexec_type;

#endif /* LEXEC_INCLUDE */
