/* lexec */

/* MIPS R3000 ISA execution interface for use in LEVO simulator */


#define	F_DEBUG		0
#define	F_DEBUGMAIN	0
#define	F_FUNNYDOUNLE	0


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

*/


/* include some files that we need */

#include <ieeefp.h>

#include	"config.h"
#include	"defs.h"
#include	"dataval.h"

#include    "lmipsregs.h"
/* #include    "lbp.h"*/
#include    "ldecode.h"
#include	"lexec.h"

/* 
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
   
*/


int lexec(opcode, src1,src2,src3,src4,dst1,dst2,cnst,bcnd,mema,delay)
int opcode;
DATAVAL *src1, *src2, *src3, *src4,*dst1, *dst2, *cnst, *bcnd, *mema;
int *delay;
{
  int EXCEPTION;             /* EXCEPTION FLAG */  

	int	rs = LEXEC_ROK ;
	int	temp,temp2,conflict=0;              
	/* int aux; */
  unsigned int ahi, alo, bhi, blo;
  unsigned int part1, part2, part3, part4;
  unsigned int p2lo, p2hi, p3lo, p3hi;
  unsigned int ms1, ms2, ms3, sum, carry;
  unsigned int extra;
  lexec_type aux;

/* set a default if all else fails */

	dst1->ui = 0 ;
	dst2->ui = 0 ;

/* OK, try and do the best that we can */

  *delay=1;
  switch (opcode) {

	case LEXECOP_NOOP:
		break ;

    /* Integer operations: */
  case LEXECOP_ADD:
    dst1->i= src1->i + src2->i; 
    break;
  case LEXECOP_ADDI:
    dst1->i = src1->i + cnst->i;
    break;
  case LEXECOP_ADDU:
    dst1->ui = src1->ui + src2->ui;
    break; 
  case LEXECOP_ADDIU:
    dst1->i=src1->ui + cnst->ui;
    break;
  case LEXECOP_AND:   
    dst1->i=src1->i & src2->i;
    break;
  case LEXECOP_ANDI:   
    dst1->i=src1->i & cnst->i & 0xffff;
    break;
  case LEXECOP_DIV:
    if(src2->i!=0){
      dst1->i=src1->i / src2->i; /* LO */
      dst2->i=src1->i % src2->i; /* HI */
    }
    else
      {
	rs = LEXEC_REXCEPT;
      }
    *delay =35;
    break;
  case LEXECOP_DIVU:
    if(src2->ui!=0){
      dst1->ui=src1->ui / cnst->ui; /* LO */
      dst2->ui=src1->ui % cnst->ui; /* HI */
    }
    else
      {
	rs = LEXEC_REXCEPT;
      }
    *delay =35;
    break;
  case LEXECOP_MULT:
    ahi = (unsigned int) src1->i >> 16;
    alo = src1->i & 0xffff;
    bhi = (unsigned int) src2->i >> 16;
    blo = src2->i & 0xffff;
    
    /* compute the partial products */
    part4 = ahi * bhi;
    part3 = ahi * blo;
    part2 = alo * bhi;
    part1 = alo * blo;
    
    p2lo = part2 << 16;
    p2hi = part2 >> 16;
    p3lo = part3 << 16;
    p3hi = part3 >> 16;
    
    /* compute the carry bit by checking the high-order bits of the addends */
    carry = 0;
    sum = part1 + p2lo;
    ms1 = part1 & 0x80000000;
    ms2 = p2lo & 0x80000000;
    ms3 = sum & 0x80000000;
    if ((ms1 & ms2) || ((ms1 | ms2) & ~ms3))
      carry++;
    ms1 = p3lo & 0x80000000;
    sum += p3lo;
    ms2 = sum & 0x80000000;
    if ((ms1 & ms3) || ((ms1 | ms3) & ~ms2))
      carry++;
    dst1->i= sum; /* LO */
    extra = 0;
    if (src1->i < 0)
      extra = src2->i;
    if (src2->i < 0)
      extra += src2->i;
    
    dst2->i = part4 + p2hi + p3hi + carry - extra; /* HI */
    *delay =12;
    break;
  case LEXECOP_MULTU:
    ahi = src1->ui >> 16;
    alo = src1->ui & 0xffff;
    bhi = src2->ui >> 16;
    blo = src2->ui & 0xffff;
    
    /* compute the partial products */
    part4 = ahi * bhi;
    part3 = ahi * blo;
    part2 = alo * bhi;
    part1 = alo * blo;
    p2lo = part2 << 16;
    p2hi = part2 >> 16;
    p3lo = part3 << 16;
    p3hi = part3 >> 16;
    
    /* compute the carry bit by checking the high-order bits of the addends */
    carry = 0;
    sum = part1 + p2lo;
    ms1 = part1 & 0x80000000;
    ms2 = p2lo & 0x80000000;
    ms3 = sum & 0x80000000;
    if ((ms1 & ms2) || ((ms1 | ms2) & ~ms3))
      carry++;
    ms1 = p3lo & 0x80000000;
    sum += p3lo;
    ms2 = sum & 0x80000000;
    if ((ms1 & ms3) || ((ms1 | ms3) & ~ms2))
      carry++;
    dst1->ui=sum;
    dst2->ui=part4 + p2hi + p3hi + carry; 
    *delay =12;  
    break;
  case LEXECOP_NOR:
    dst1->i= ~(src1->i | src2->i);
    break;
  case LEXECOP_OR:
    /* 	ldecode_instr->src1.i = RS_(instr);
	ldecode_instr->dst1.i = RT_(instr);
	ldecode_instr->scr2.i = RD_(instr); from ldecode  */
    dst1->i=src1->i | src2->i;
    break;
    /* Immediate value passed at src2 */
  case LEXECOP_ORI:
    dst1->i=src1->i | (cnst->i & 0xffff);
    break;
  case LEXECOP_SLLV:
    /* shamt value at src2 */
    dst1->i=src1->ui << (src2->i & 0x1f);
    break;
  case LEXECOP_SLL:
    dst1->i=src1->ui << cnst->i;
    break;
  case LEXECOP_SRA:
    dst1->i=src1->i >> cnst->i;
    dst1->i |= src1->i & 0x80000000;
    break;
  case LEXECOP_SRAV:
    dst1->i=src1->i >> src2->i;
    dst1->i |= src1->i & 0x80000000;
    break;
  case LEXECOP_SRL:
    dst1->i=src1->ui >> cnst->ui;
    break;
  case LEXECOP_SRLV:
    dst1->i=src1->ui >> src2->ui;
    break;
  case LEXECOP_SUB:
    dst1->i=src1->i - src2->i;
    break;
  case LEXECOP_SUBU:
    dst1->i=src1->ui - src2->ui;
    break;
  case LEXECOP_XOR:
    dst1->i=src1->i ^ src2->i;
    break;
  case LEXECOP_XORI:
    dst1->i=src1->i ^ (cnst->i & 0xffff);
    break;
  case LEXECOP_SLT:
    if (src1->i < src2->i){
      dst1->i=1;
        }
    else{
      dst1->i=0;
    }     
    break;
  case LEXECOP_SLTU:
    if (src1->ui < src2->ui){
      dst1->ui =1;
    }
    else{
      dst1->ui = 0;
    }     
    break;
  case LEXECOP_SLTI:
    if (src1->i < cnst->i){
      dst1->i=1;
    }
    else{
      dst1->i=0;
    }     
    break;    
  case LEXECOP_SLTIU:
    if (src1->ui < cnst->ui){
      dst1->ui=1;
    }
    else{
      dst1->ui=0;
    }     
    break;    
    
    /*
      NOTES:
      For Levo, backward branch target addresses are converted 
      to forward target addresses.
      It is assumed on execution that the conversion is done during
      the decode stage in ldecode.c.
      The target addresses are computed at decode stage and here it
      is only returned what was the direction of the branch (taken/ not taken).
    */  
    /* Assumptions for most of the branch instructions:
                   1. The address of this instruction (ilptr) is given in src3
                   2. The offset value is given in source 4
		   3. The branch target address is returned in dst1 
    */

  case LEXECOP_BEQ:            
    /* There is a delay of one instruction when the branch is taken */
    /* It is only returned if the branch was taken, 1, or not, 0, */
    bcnd->i = src1->i == src2->i;  
    *delay = 2;
    break;
  case LEXECOP_BNE:  
    bcnd->i = src1->i != src2->i;  
    *delay = 2;
    break;
    
    /* NOTE:
       BNEL is an invalid instruction for R2000/R3000 and it generates
       an exception: sets ExcCode field of Cause Register to 10 
       (Reserved Instruction Exception).
    */
  case LEXECOP_BEQL:  
    /* In exception
       dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
       temp = src2->i << 2;
       dst2->i = dst2->i || temp;               Saves Status Register */
    bcnd->i = src1->i == src2->i;  
    *delay = 2;   
    break;
    
  case LEXECOP_BNEL:    
    /* In exception
       dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
       temp = src2->i << 2;
       dst2->i = dst2->i || temp;               Saves Status Register */
    bcnd->i = src1->i != src2->i;   
    *delay = 2;  
    break;
  case LEXECOP_BGEZL:     
    /* In exception
       dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
       temp = src2->i << 2;
       dst2->i = dst2->i || temp;               Saves Status Register */
    bcnd->i = src1->i >= src2->i;    
    *delay = 2; 
    break;
  case LEXECOP_BLEZL:     
    /* In exception
       dst1->i = src1->i | ( 0x3e & cnst);   Sets ExcCode 
       temp = src2->i << 2;
       dst2->i = dst2->i || temp;               Saves Status Register */
    bcnd->i = src1->i <= src2->i;   
    *delay = 2;  
    break;
  case LEXECOP_BGEZ: 
    bcnd->i = src1->i >= 0;  
    *delay = 2;
    break;
  case LEXECOP_BGEZAL:
  case LEXECOP_BGEZALL:
    bcnd->i = src1->i >= 0;
    /*  
	register 31 must hold the address of the instruction after the 
	delay slot
	dst1->i = ilptr + 8 */
    *delay = 2;
    break;
  case LEXECOP_BGTZ:
    /* Instruction is valid only if src2->i == 0 */
    if(src2->i == 0){
      bcnd->i = src1->i > 0;  
      *delay = 2;
    }
    break;
  case LEXECOP_BLEZ:
    bcnd->i = src1->i <= 0;
    *delay = 2;
    break;
  case LEXECOP_BLTZ:
  case LEXECOP_BLTZL:
    bcnd->i = src1->i < 0;
    *delay = 2;
    break;
    
  case LEXECOP_BGTZL:
    /* Not implemented in r2000/r3000 */
    bcnd->i = src1->i < 0;
    *delay = 2;
    break;
    
    /* This two instructions need to store the address of
       the instruction after the delay slot in r31
       r31 = ilptr + 8 */
  case LEXECOP_BLTZAL:
  case LEXECOP_BLTZALL:
    bcnd->i = src1->i < 0;
    *delay = 2;
    break;

    /* Break Instruction causes a breakpoint exception */
  case LEXECOP_BREAK: 
    dst1->i = src1->i | ( 0x3e & 9);
    break;
    
  case LEXECOP_SYNC: 
    *delay = 1;
    break;
  case LEXECOP_J:
    /* 
      ilptr = target address 
    */
    bcnd->i = 1;
    *delay = 2;
    break;
  case LEXECOP_JAL: 
    bcnd->i = 1;
    /*
      Ali,
 
      NOTE:
           FOR jal & jalr  the destination register (ra = $31) must hold
	   the address of the next instruction
	Remember jal is used to make subroutine calls and the
        return address is stored in $31 (ra)
      
     actions:
     
     1.  ilptr = target address = const1
      
     2.  dst1->i = ilptr + 4 = current_instr->ilptr + 4;
    */
    *delay = 2;
    break;
  case LEXECOP_JALR: 
    /*
      ilptr = (RS) 
      and
      (defaul dst1->i = GPR31)
      dst1->i = ilptr + 4;
    */
    bcnd->i = 1;
    *delay = 2;
    break;
  case LEXECOP_JR: 
    /*
      if (src2->i == 0)
      ilptr = (RS); 
      
    */
    bcnd->i = 1;
    *delay = 2;
    break;

  case LEXECOP_LUI:
	  dst1->i = (cnst->i & 0xffff) << 16;
      *delay = 1;

    break;
    
    /* Memory LOAD instructions */
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
    mema->ui = src1->ui + cnst->ui;
    *delay = 2;
    break;
   
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
    if(!conflict){
      mema->ui = src1->ui + cnst->ui;
      *delay = 2;
      src1->ui = 1;
    }
    else
      src1->ui = 1;
    break;

  case LEXECOP_SDC1:
    /* 
       This instruction is invalid for R2000/R3000
       Generates exception if any of the three least significant bits 
       of the effective addres is non-zero 
    */
    mema->ui = src1->ui + cnst->ui;
    *delay = 2;
    break;
  case LEXECOP_SWC1:
    /* 
       Generates exception if any of the three least significant bits 
       of the effective addres is non-zero 
    */
    mema->ui = src1->ui + cnst->ui;
    *delay = 2;
    break;

  case LEXECOP_MFHI:
    /* ASSUMPTION: 
       
       The AS accesses register HI, gets its value and puts it in
       src1->i
       dst1->i will hold this value to later be stored in the destination
       reGister that the AS must know.
    */
    dst1->i = src1->i;
    break;
  case LEXECOP_MFLO:
    /* ASSUMPTION: 
       
       The AS accesses register LO, gets its value and puts it in
       src1->i
       dst1->i will hold this value to later be stored in the destination
       register that the AS must know.
    */
    dst1->i = src1->i;
    break;
  case LEXECOP_MTHI:
    /* ASSUMPTION: 
       
       The AS accesses source register, gets its value and puts it in
       the special register HI.
    */
    dst1->i = src1->i;
    *delay = 3;
    break;
  case LEXECOP_MTLO:
    /* ASSUMPTION: 
       
       The AS accesses source register, gets its value and puts it in
       the special register LO.
    */
    dst1->i = src1->i;
    *delay = 3;
    break;

  case LEXECOP_MFC0:
    dst1->i = src1->f;  /* contents of FPR are loaded into GPR */ 
    *delay =2;
    break;
  case LEXECOP_MTC0:
  case LEXECOP_MTC1:
    dst1->f = src1->i;  /* contents of GPR are loaded into FPR */ 
    *delay =2;
    break;
  case LEXECOP_MFC1:
    dst1->i = src1->f;  /* contents of FPR are loaded into GPR */ 
    *delay =2;
    break;
    /*******************************************************/	    
    /* Floating point operations: */ 
    /*******************************************************/	     
    /* GENERAL ASSUMPTIONS ABOUT FLOATING POINT OPERATIONS:
       Every source operand with double precision is formed by the
       conjuction of two single precision registers.
       Only even registers can be used for double precision. For example
       if FR8 is going to be used for double precision then
       it would be formed by FGR8 (which will fomr the FGR8 LOW part)
       single and FGR9 single (which would conform the HI part)
    */
  case LEXECOP_ABSD:
    aux.all[0] = (unsigned int) src2->f;
    aux.all[0] = (aux.all[0] << 32) | (unsigned int) src1->f;
    aux.temp[0] = (long long) aux.all[0];
    if (aux.temp[0] < 0){
      aux.res[0] = (long long) (aux.temp[0] * -1);
      dst2->f = (aux.res[0] & 0xffffffff00000000) >> 32;
      dst1->f = aux.res[0] & 0x00000000ffffffff;
    }
    else{
      aux.res[0] = (long long) aux.temp[0];
      dst2->f = (aux.res[0] & 0xffffffff00000000) >> 32;
      dst1->f = aux.res[0] & 0x00000000ffffffff;
    }
    break;
  case LEXECOP_ABSS:
    if(src1->f < 0)
      dst1->f = src1->f * -1;
    else
      dst1->f = src1->f;
    break;
  case LEXECOP_ADDD:
    aux.all[0] = (unsigned int) src2->f;
    aux.all[0] = (aux.all[0] << 32) | (unsigned int) src1->f;
    aux.all[1] = (unsigned int) src4->f;
    aux.all[1] = (aux.all[1] << 32) | (unsigned int) src3->f;
    aux.temp[0] = (long long) aux.all[0] + (long long) aux.all[1];
    aux.res[0] = (long long) aux.temp[0];
    dst2->f = (aux.res[0] & 0xffffffff00000000) >> 32;
    dst1->f = aux.res[0] & 0x00000000ffffffff;
    break;
  case LEXECOP_ADDS:
    dst1->f =  src1->f + src2->f;
    break;
    /************************************************************************ 
       GENERAL ASSUMPTIONS FOR COMPARISON INSTRUCTIONS:
       1. The AS accesses the floating-point condition flag: 
       LMIPSREG_FCR31 71  (FP Control/status register) bit 23
       2. The result of the comparison is stored on dst1.i and
       AS has to update the condition bit.
    */
    case LEXECOP_CMPFD:
      lexec_comp_instr(LEXECOP_CMPFD, src1,src2,src3,src4,dst1,dst2,delay);
      break;
      
    case LEXECOP_CMPFS:
      lexec_comp_instr(LEXECOP_CMPFS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPUND:
      lexec_comp_instr(LEXECOP_CMPUND, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPUNS:
      lexec_comp_instr(LEXECOP_CMPUNS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    
    case LEXECOP_CMPEQD:
      lexec_comp_instr(LEXECOP_CMPEQD, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPEQS:
      lexec_comp_instr(LEXECOP_CMPEQS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPOLTD:
      lexec_comp_instr(LEXECOP_CMPOLTD, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPOLTS:
      lexec_comp_instr(LEXECOP_CMPOLTS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPULTD:
      lexec_comp_instr(LEXECOP_CMPULTD, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPULTS:
      lexec_comp_instr(LEXECOP_CMPULTS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPOLED:
      lexec_comp_instr(LEXECOP_CMPOLED, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPOLES:
      lexec_comp_instr(LEXECOP_CMPOLES, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPULED:
      lexec_comp_instr(LEXECOP_CMPULED, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPULES:
      lexec_comp_instr(LEXECOP_CMPULES, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPSFD:
      lexec_comp_instr(LEXECOP_CMPSFD, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPSFS:
      lexec_comp_instr(LEXECOP_CMPSFS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPNGLED:
      lexec_comp_instr(LEXECOP_CMPNGLED, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPNGLES:
      lexec_comp_instr(LEXECOP_CMPNGLES, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    
    case LEXECOP_CMPSEQD:
      lexec_comp_instr(LEXECOP_CMPSEQD, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPSEQS:
      lexec_comp_instr(LEXECOP_CMPSEQS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPNGLD:
      lexec_comp_instr(LEXECOP_CMPNGLD, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPNGLS:
      lexec_comp_instr(LEXECOP_CMPNGLS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPLTD:
      lexec_comp_instr(LEXECOP_CMPLTD, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPLTS:
      lexec_comp_instr(LEXECOP_CMPLTS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
    
    case LEXECOP_CMPNGED:
      lexec_comp_instr(LEXECOP_CMPNGED, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPNGES:
      lexec_comp_instr(LEXECOP_CMPNGES, src1,src2,src3,src4,dst1,dst2,delay);
      break;
        
    case LEXECOP_CMPLED:
      lexec_comp_instr(LEXECOP_CMPLED, src1,src2,src3,src4,dst1,dst2,delay);
      break;

    case LEXECOP_CMPLES:
      lexec_comp_instr(LEXECOP_CMPLES, src1,src2,src3,src4,dst1,dst2,delay);
      break;
      
  case LEXECOP_CMPNGTD:
      lexec_comp_instr(LEXECOP_CMPNGTD, src1,src2,src3,src4,dst1,dst2,delay);
      break;
      
  case LEXECOP_CMPNGTS:
      lexec_comp_instr(LEXECOP_CMPNGTS, src1,src2,src3,src4,dst1,dst2,delay);
      break;
      
      /* For the following four branch instructions the
	 src1 is the condition bit flag, assigned as register 183,
	 which is set by the compare instructions
      */
  case LEXECOP_BC0F:
  case LEXECOP_BC1F:  
    /* In exception 
       dst1->i = dst1->i | ( 0x3e & 11); 
    */
  case LEXECOP_BC0FL: 
  case LEXECOP_BC1FL: 
    bcnd->i = ~(src1->i);
   
    *delay = 2;
    break;
    
  case LEXECOP_BC0T:
  case LEXECOP_BC1T:
    /* In exception 
       dst1->i = dst1->i | ( 0x3e & 11); 
    */
  case LEXECOP_BC0TL:
  case LEXECOP_BC1TL:
    bcnd->i = src1->i;
    *delay = 2;
    break;
    
  case LEXECOP_COP0:
    /* This instruction does not do anything for COP0 */
    break;
  case LEXECOP_CFC1:
    dst1->i = src1->f;
    /* If exception:
       dst2->i = dst2->i | ( 0x3e & 11); */
    break;
  case LEXECOP_CTC1:
    dst1->f = src1->i;
    /* If exception:
       dst2->i = dst2->i | ( 0x3e & 11); */
    break;
  case LEXECOP_LDC1:
    /* if exception Cause reserved instruction exception 
       dst2->i = dst2->i | ( 0x3e & 10); 
    */
    mema->ui = src1->ui + cnst->ui;
    *delay = 2;
    break;
  case LEXECOP_LL:
    /* This is a non r2000/r3000 */
    mema->ui = src1->ui + cnst->ui;
    *delay = 2;
    break;
  case LEXECOP_LWC1:
    mema->ui = src1->ui + cnst->ui;
    *delay = 2;
    break;

    
    /* GENERAL ASSUMPTIONS FOR CONVERT INSTRUCTIONS:
	 1. AS accesses the source register (fs), takes its value and puts
	 it in src1->
	 2. The result value: dst1 is returned and has to be stored in 
	 the fd register.
    */
	/* Aqui todavia debo modificar la correcta conversion.
	   Por ejemplo al convertir a double dos destinos deben ser
	   modificados, no solo uno */
  case LEXECOP_CVTDS: 
    /* Convert FP single precision to FP double precision*/
    dst1->f = src1->f; 
    dst2->f = 0x0000;
    break;
  case LEXECOP_CVTDW: 
    /* Convert fixed point to FP double precision */
    dst1->f = (float) src1->i;
	dst2->f = 0x0000;
    break;
  case LEXECOP_CVTSD:
    /* Convert FP double precision to FP single precision */
	  /* double aux
		 aux(LO) = src1->f 
		 aux(HI) = (src1 + 1)->f
		 dst1 =  (float) aux
	    */
    dst1->f = (float) src1->f;
    break;
  case LEXECOP_CVTSW:
    /* Convert fixed point to FP single precision */
    dst1->f = (float) src1->i;
    break;
  case LEXECOP_CVTWD:
    /* convert FP double precision to fixed point  */
	   /* double aux
		 aux(LO) = src1->f 
		 aux(HI) = (src1 + 1)->f
		 dst1 =  (intt) aux
	    */
    dst1->i = (int) src1->f;
    break;
  case LEXECOP_CVTWS:
    /* convert FP single precision to fixed point  */
    dst1->i = (int) src1->f;
    break;
  case LEXECOP_SUBD:
    dst1->f = src1->f - src2->f;
    break;
  case LEXECOP_SUBS:
    dst1->f = src1->f - src2->f;
    break;
  case LEXECOP_MULS:
    dst1->f =  src1->f * src2->f;
    break;
  case LEXECOP_DIVS:
    if (src2->f != 0){
      dst1->f = src1->f / src2->f;
    }
    else{
      printf("\nlexec: Floating Point division by zero LMIPSFPEXCP_DIVBYZ");
    }
    break;
  case LEXECOP_DIVD:
    if (src2->f != 0){
      dst1->f = src1->f / src2->f;
    }
    else{
      printf("\nlexec: Floating Point division by zero LMIPSFPEXCP_DIVBYZ");
    }
    break;
    
    /* Assumptions for FP move and load operations:
       1. Only compute memory addresses and values
       2. It does not accesses registers.
    */
  case LEXECOP_MOVD:
      /* AS must access register fs put ts contents on src1->f
	 and then later take the value of dst1->f  and put it in
	 fd */
    dst1->f = src1->f;
    break;
  case LEXECOP_MOVS:     
      /* AS must access register fs put ts contents on src1->f
	 and then later take the value of dst1->f  and put it in
	 fd */
    dst1->f = src1->f;
    break;
  case LEXECOP_MULD:
    dst1->f = src1->f * src2->f;
    break;
  case LEXECOP_NEGD:
    dst1->f = (double)~src1->i;
    break;
  case LEXECOP_NEGS:
    dst1->f = (float)~src1->i;
    break;
  case LEXECOP_RFE:
    temp =  src1->i & 0xfffffff0;
    temp2 = (src1->i & 0x3c) >> 2;
    dst1->i = temp | temp2;
    /* also LLbit = 0 */
    break;
  case LEXECOP_SYSCALL:
    /* PASS CONTROL TO EXCEPTION HANDLER */
    break;
  case LEXECOP_ERET:
    /* This instruction is only implemented in R4000
       It is used to handle returns from interrupts*/
    if(src1->i & 4){
      /* change the ilptr to ERROREPC and set Status register to: */
      temp = src1->i & 0xfffffff8;
      temp2 = (src1->i & 0x0000000b) |(src1->i & 0x3);
      dst1->i = temp | temp2;
    }
    else{
      /* change the ilptr to EPC and set Status register to: */
      temp = src1->i & 0xfffffffc;
      temp2 = (src1->i & 0x00000002) |(src1->i & 0x1);
      dst1->i = temp | temp2;
    }
    break;
    /* New implemented instructions */
  case LEXECOP_CEILWS:
    break;
  case LEXECOP_FLOORWS:
    break;
  case LEXECOP_ROUNDS:
    break;
  case LEXECOP_SQRTS:
    break;
  case LEXECOP_TRUNCWS:
    break;
  case LEXECOP_CEILWD:
    break;
  case LEXECOP_FLOORWD:
    break;
  case LEXECOP_ROUNDD:
    break;
  case LEXECOP_SQRTD:
    break;
  case LEXECOP_TRUNCWD:
    break;
  case LEXECOP_TLBP:
    break;
  case LEXECOP_TLBR:
    break;
  case LEXECOP_TLBWI:
    break;
  case LEXECOP_TLBWR:
    break;
  default:
    rs = LEXEC_RUNIMPL ;
    assert(0);
    break;
  } /* end switch */
  rs = LEXEC_ROK ;
  return rs ;
}

/* Floating point Comparison Instructions Routine */
int lexec_comp_instr(opexec, src1,src2,src3,src4,dst1,dst2,delay)
int opexec;
DATAVAL *src1, *src2, *src3, *src4,*dst1, *dst2;
int *delay;
{
  unsigned int less, equal,unordered, condition;
  lexec_type aux;  
  /* All comparison instructions take one clock cycle to execute */
  *delay = 1;

  /* Distinguish between Simple and Double Precision 
    1: Double Precision
    0: Single Precision */

  /* Single Precision */
  if(FMT_(opexec)){
    if((isnanf(src1->f))||(isnanf(src2->f))){
      less = 0;
      equal = 0;
      unordered = 1;
      if(COND3_(opexec)){
	printf("\nlexec_comp_instr: Invalid Operation Exception\n");
	exit(1);
      }
    }
    else{
      if(src1->f < src2->f)
	less = 1;
      else{
	if(src1->f == src2->f){
	  less = 0;
	  equal = 1;
	}
	else{
	  less = 0;
	  equal = 0;
	}
      }
    }
    condition = ((COND2_(opexec) & less)||
		 (COND1_(opexec) & equal) ||
		 (COND0_(opexec) & unordered));
    /* Set the  Flag Control Status Register bit 23 with the result of
       the condition, thus later this value can be checked by the BCzF
       and BCzT instructions */
    dst1->i = condition;
    
  }
  /* Double Precision */
  else{
    
    if((isnanf(src1->f))||(isnanf(src2->f))||(isnanf(src2->f))||(isnanf(src3->f))){
      less = 0;
      equal = 0;
      unordered = 1;
      if(COND3_(opexec)){
	printf("\nlexec_comp_instr: Invalid Operation Exception\n");
	exit(1);
      }
    }
    else{
      aux.all[0] = (unsigned int) src2->f;
      aux.all[0] = (aux.all[0] << 32) | (unsigned int) src1->f;
      aux.all[1] = (unsigned int) src4->f;
      aux.all[1] = (aux.all[1] << 32) | (unsigned int) src3->f;
      aux.temp[0] = (long long) aux.all[0];
      aux.temp[1] = (long long) aux.all[1];
      if(aux.temp[0] < aux.temp[1]){
	less = 1;
      }
      else{
	if(aux.temp[0] == aux.temp[1]){
	  less = 0;
	  equal = 1;
	}
	else{
	  less = 0;
	  equal = 0;
	}
      }    
    }
    condition = ((COND2_(opexec) & less)||
		 (COND1_(opexec) & equal) ||
		 (COND0_(opexec) & unordered));
    /* Set the  Flag Control Status Register bit 23 with the result of
       the condition, thus later this value can be checked by the BC1F
       and BC1T instructions */
    dst1->i = condition;
  }
  /* set fcr 31 bit 23 */
  return SR_OK;
}

#if	F_DEBUGMAIN

main(){
  /* Test for integer operation */
  int s1,s2,s3,s4,dest1,dest2;
  int opcode;
  /* add rd,rs,rt   <->  add dst1 s1,s2 */
  opcode = 0;
  s1.i = 3;
  s2.i = 4;
  s3.i = 0;
  s4.i = 0;
  dst1 = 0;
  dst2 = 0;
  lexec(opcode,s1,s2,s3,s4,dst1,dst2);
  return;
}

#endif /* F_DEBUGMAIN */




