/* lmipsregs */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	LMIPSREGS_INCLUDE
#define	LMIPSREGS_INCLUDE	1


/* Set of architected registers for Levo.
   Includes:
   - 32 General Purpose Registers (R0 - R31)
   - 32 Floating Point Registers per each Coprocessor (Floating
        Point Unit, Mips has four Coprocessor units)
     Floating Point Registers are addressed as follows:
     
     Register      FP Unit      Register Address
     FGR0             0             32 (0x20) 
     .                .             .
     .                .             .
     .                .             .
     FGR31            0             63 (0x3f)
     FGR32            1             64 (0x40)
     .                .             .
     .                .             .
     .                .             .
     FGR63            1             95 (0x5f)
     FGR64            2             96 (0x60)
     .                .             .
     .                .             .
     .                .             .
     FGR95            2             127 (0x7f)
     FGR96            3             128 (0x80)
     .                .             .
     .                .             .
     .                .             .
     FGR127           3             159 (0x9f)

*/

/* General Purpose Registers */
#define LMIPSREG_R0 0
#define LMIPSREG_R1 1
#define LMIPSREG_R2 2
#define LMIPSREG_R3 3
#define LMIPSREG_R4 4
#define LMIPSREG_R5 5
#define LMIPSREG_R6 6
#define LMIPSREG_R7 7
#define LMIPSREG_R8 8
#define LMIPSREG_R9 9
#define LMIPSREG_R10 10
#define LMIPSREG_R11 11
#define LMIPSREG_R12 12
#define LMIPSREG_R13 13
#define LMIPSREG_R14 14
#define LMIPSREG_R15 15
#define LMIPSREG_R16 16
#define LMIPSREG_R17 17
#define LMIPSREG_R18 18
#define LMIPSREG_R19 19
#define LMIPSREG_R20 20
#define LMIPSREG_R21 21
#define LMIPSREG_R22 22
#define LMIPSREG_R23 23
#define LMIPSREG_R24 24
#define LMIPSREG_R25 25
#define LMIPSREG_R26 26
#define LMIPSREG_R27 27
#define LMIPSREG_R28 28
#define LMIPSREG_R29 29
#define LMIPSREG_R30 30
#define LMIPSREG_R31 31

/* Floating Point Registers, FP unit 0 (In Mips: Coprocessor 0 regs) */
#define LMIPSREG_FGR0 32
#define LMIPSREG_FGR1 33
#define LMIPSREG_FGR2 34
#define LMIPSREG_FGR3 35
#define LMIPSREG_FGR4 36
#define LMIPSREG_FGR5 37
#define LMIPSREG_FGR6 38
#define LMIPSREG_FGR7 39
#define LMIPSREG_FGR8 40
#define LMIPSREG_FGR9 41
#define LMIPSREG_FGR10 42
#define LMIPSREG_FGR11 43
#define LMIPSREG_FGR12 44
#define LMIPSREG_FGR13 45
#define LMIPSREG_FGR14 46
#define LMIPSREG_FGR15 47
#define LMIPSREG_FGR16 48
#define LMIPSREG_FGR17 49
#define LMIPSREG_FGR18 50
#define LMIPSREG_FGR19 51
#define LMIPSREG_FGR20 52
#define LMIPSREG_FGR21 53
#define LMIPSREG_FGR22 54
#define LMIPSREG_FGR23 55
#define LMIPSREG_FGR24 56
#define LMIPSREG_FGR25 57
#define LMIPSREG_FGR26 58
#define LMIPSREG_FGR27 59
#define LMIPSREG_FGR28 60
#define LMIPSREG_FGR29 61
#define LMIPSREG_FGR30 62
#define LMIPSREG_FGR31 63

/* Floating Point Registers, FP unit 1 (In Mips: Coprocessor 1 regs) */
#define LMIPSREG_FGR32 64
#define LMIPSREG_FGR33 65
#define LMIPSREG_FGR34 66
#define LMIPSREG_FGR35 67
#define LMIPSREG_FGR36 68
#define LMIPSREG_FGR37 69
#define LMIPSREG_FGR38 70
#define LMIPSREG_FGR39 71
#define LMIPSREG_FGR40 72
#define LMIPSREG_FGR41 73
#define LMIPSREG_FGR42 74
#define LMIPSREG_FGR43 75
#define LMIPSREG_FGR44 76
#define LMIPSREG_FGR45 77
#define LMIPSREG_FGR46 78
#define LMIPSREG_FGR47 79
#define LMIPSREG_FGR48 80
#define LMIPSREG_FGR49 81
#define LMIPSREG_FGR50 82
#define LMIPSREG_FGR51 83
#define LMIPSREG_FGR52 84
#define LMIPSREG_FGR53 85
#define LMIPSREG_FGR54 86
#define LMIPSREG_FGR55 87
#define LMIPSREG_FGR56 88
#define LMIPSREG_FGR57 89
#define LMIPSREG_FGR58 90
#define LMIPSREG_FGR59 91
#define LMIPSREG_FGR60 92
#define LMIPSREG_FGR61 93
#define LMIPSREG_FGR62 94
#define LMIPSREG_FGR63 95

#define	LMIPSREG_FP0	LMIPSREG_FGR32
#define	LMIPSREG_FP1	LMIPSREG_FGR33
#define	LMIPSREG_FP2	LMIPSREG_FGR34
#define	LMIPSREG_FP3	LMIPSREG_FGR35
#define	LMIPSREG_FP4	LMIPSREG_FGR36
#define	LMIPSREG_FP5	LMIPSREG_FGR37
#define	LMIPSREG_FP6	LMIPSREG_FGR38
#define	LMIPSREG_FP7	LMIPSREG_FGR39
#define	LMIPSREG_FP8	LMIPSREG_FGR40
#define	LMIPSREG_FP9	LMIPSREG_FGR41
#define	LMIPSREG_FP10	LMIPSREG_FGR42
#define	LMIPSREG_FP11	LMIPSREG_FGR43
#define	LMIPSREG_FP12	LMIPSREG_FGR44
#define	LMIPSREG_FP13	LMIPSREG_FGR45
#define	LMIPSREG_FP14	LMIPSREG_FGR46
#define	LMIPSREG_FP15	LMIPSREG_FGR47
#define	LMIPSREG_FP16	LMIPSREG_FGR48
#define	LMIPSREG_FP17	LMIPSREG_FGR49
#define	LMIPSREG_FP18	LMIPSREG_FGR50
#define	LMIPSREG_FP19	LMIPSREG_FGR51
#define	LMIPSREG_FP20	LMIPSREG_FGR52
#define	LMIPSREG_FP21	LMIPSREG_FGR53
#define	LMIPSREG_FP22	LMIPSREG_FGR54
#define	LMIPSREG_FP23	LMIPSREG_FGR55
#define	LMIPSREG_FP24	LMIPSREG_FGR56
#define	LMIPSREG_FP25	LMIPSREG_FGR57
#define	LMIPSREG_FP26	LMIPSREG_FGR58
#define	LMIPSREG_FP27	LMIPSREG_FGR59
#define	LMIPSREG_FP28	LMIPSREG_FGR60
#define	LMIPSREG_FP29	LMIPSREG_FGR61
#define	LMIPSREG_FP30	LMIPSREG_FGR62
#define	LMIPSREG_FP31	LMIPSREG_FGR63

/* Multiply and Divide Registers */
#define LMIPSREG_HI 160
#define LMIPSREG_LO 161

/* Program Counter */
#define LMIPSREG_PC 162

/* Exception Handling Registers 
   For details: see page 6-3 Mips architecture by Gerry Kane, 1992
*/
#define LMIPSREG_CAUSE     163 
#define LMIPSREG_EPC       164
#define LMIPSREG_STATUS    165
#define LMIPSREG_CONTEXT   166
#define LMIPSREG_ERROR     167
#define LMIPSREG_BADVADDR  168
#define LMIPSREG_CMP       169
#define LMIPSREG_PRID      170
#define LMIPSREG_CONFIG    171
#define LMIPSREG_LLADR     172
#define LMIPSREG_WATCHLO   173
#define LMIPSREG_WATCHHI   174
#define LMIPSREG_ECC       175
#define LMIPSREG_CACHEERR  176
#define LMIPSREG_TAGLO     177
#define LMIPSREG_TAGHI     178
#define LMIPSREG_ERROREPC  179
#define LMIPSREG_INDEX     180
#define LMIPSREG_RANDOM    181

/* Floating Point Control Registers */

#define LMIPSREG_FCR31	182	/* FP Implementation Revision register */ 

#define LMIPSREG_FCR0      183  /* FP Implementation Revision register */ 

#define LMIPSREG_EXC_REG   184  /* register to store the value of an 
                                   unimplemented instruction when a 
                                   trap occurs and we want to emulate 
                                   that instruction */

/* new coprocessor-0 (coprocessor-A) control registers */

#define	LMIPSREG_CACR0	185		/* coprocessor-0 control register 0 */
#define	LMIPSREG_CACR31	186		/* coprocessor-0 control register 31 */

/* aliases for coprocessor-1 (coprocessor-B) control registers */

#define	LMIPSREG_CBCR0	LMIPSREG_FCR0	/* coprocessor-1 control register 0 */
#define	LMIPSREG_CBCR31	LMIPSREG_FCR31	/* coprocessor-1 control register 31 */

/* total number of MIPS architected registers */

#define	LMIPSREG_NREGS	187	/* number of total ISA registers */

/* alias register names */

#define	LMIPSREG_SP	LMIPSREG_R29

/* Condition Signals for Coprocessors. Used to check branch instructions */

#define LMIPSCOC0  185
#define LMIPSCOC1  186
#define LMIPSCOC2  187
#define LMIPSCOC3  188

/* Exception Vectors */
#define LMIPSVEC_RESET     
#define LMIPSVEC_GRAL      

/* Constants to identify exceptions */
#define LMIPSEXCP_ADDERROR   
#define LMIPSEXCP_BREAK      
#define LMIPSEXCP_BUSERROR   
#define LMIPSEXCP_INT        
#define LMIPSEXCP_OVFLW      
#define LMIPSEXCP_RSRVD      
#define LMIPSEXCP_RESET       
#define LMIPSEXCP_SYSCALL    

/* Constants to identify floating point exceptions */
#define LMIPSFPEXCP_INXCT       /* Inexact exception */
#define LMIPSFPEXCP_0VFLW      
#define LMIPSFPEXCP_UNDFLW    
#define LMIPSFPEXCP_DIVBYZ    /* Division by Zero */
#define LMIPSFPEXCP_INV       /* Invalid  */


#endif /* LMIPSREGS_INCLUDE */


