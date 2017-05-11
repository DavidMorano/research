/* ssdecode */




/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 2000-2001 by The Regents of The University of Michigan.
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#ifndef	SSDECODE_INCLUDE
#define	SSDECODE_INCLUDE	1



#define DNA                     (-1)

/* The index to register name macros (such as DGPR) convert an
 * index into the register name.  The register name combines type
 * and register information into a single name.  The type and the
 * dependence name can be extracted using the GET_TYPE and DEP_NAME
 * macros respectively.  The index into the register file using the
 * INT_REG_INDEX and FP_REG_INDEX macros.
 *
 * Here is an example using the PISA instruction set:
 *
 * Floating point register $f10 can hold a single-precision
 * floating point number or a double-precision floating point
 * number.  The single-precision macro DFPR_F(N) returns 42 while
 * the double-precision macro DFPR_D(N) returns 78.  This allows
 * MASE to read the proper type of data when reading the register
 * file.  The dependence name is used to track dependencies
 * between instructions and needs to be the same regardless of
 * the tpye of data stored in $f10.  This is where the DEP_NAME
 * macro comes into play.  You can see that DEP_NAME(78) ==
 * DEP_NAME(42) == 42 so that dependencies are tracked between
 * instructions. */




/* --- ALPHA MACROS --- */


/* Alpha only has double-precision FP values and 64-bit
 * integers.  Thus, the dependence names are the same as
 * the register names. */

/* index to register name macros */
#define DGPR(N)                 (N)
#define DFPR(N)                 ((N)+32)
#define DFPCR                   (64)
#define DUNIQ                   (65)
#define DTMP                    (66)

/* get the type from the register name */
#define GET_TYPE(N)				\
  (N == DNA) ? vt_none :			\
  (N >= 0 && N < 32) ? vt_qword :		\
  (N >= 32 && N < 64) ? vt_dfloat :		\
  (N == DFPCR) ? vt_qword :			\
  (N == DUNIQ) ? vt_qword :			\
  (N == DTMP) ? vt_addr : vt_none

/* convert the register name to a dependence name */
#define DEP_NAME(N)		(N)

/* check if the dependence name corresponds to an FP register */
#define REG_IS_INT(N)		((N) < 32) 
#define REG_IS_FP(N)            (((N) >= 32) && ((N) < 64))
#define	REG_IS_FPCR(N)		((N) == DFPCR)
#define	REG_IS_UNIQ(N)		((N) == DUNIQ)
#define	REG_IS_TMP(N)		((N) == DTMP)

/* dependence name to index macros */
#define INT_REG_INDEX(N)        (N)
#define FP_REG_INDEX(N)         ((N)-32)

/* string for the dependence name */
#define REG_NAME(N) 					\
  (N == DTMP) ? "$tmp" : 				\
  (N == DUNIQ) ? "$uniq" : 				\
  (N == DFPCR) ? "$fpcr" :				\
  (REG_IS_FP(N)) ? 					\
  md_reg_name(rt_fpr, FP_REG_INDEX(N)) :		\
  md_reg_name(rt_gpr, INT_REG_INDEX(N))
 



#endif /* SSDECODE_INCLUDE */



