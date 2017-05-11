/* regs.c - architected registers state routines */

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

#include	<sys/types.h>
#include <stdlib.h>
#include <string.h>

#include	"localmisc.h"
#include	"ssdecode.h"

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "loader.h"
#include "regs.h"




/* create a register file */
struct regs_t *
regs_create(void)
{
  struct regs_t *regs;

  regs = calloc(1, sizeof(struct regs_t));
  if (!regs)
    fatal("out of virtual memory");

  return regs;
}

/* initialize architected register state */
void
regs_init(struct regs_t *regs)		/* register file to initialize */
{
	int	size ;


  /* FIXME: assuming all entries should be zero... */
  /* regs->regs_R[MD_SP_INDEX] and regs->regs_PC initialized by loader... */
	size = sizeof(struct regs_t) ;
  	memset(regs, 0, size) ;

}


/* set a value into a register in a register file */
int regs_setval(rp,opname,v)
struct regs_t	*rp ;
int		opname ;
ULONG		v ;
{
	int	ri ;


	if (REG_IS_INT(opname)) {

	    ri = INT_REG_INDEX(opname) ;
	    rp->regs_R[ri] = v ;

	} else if (REG_IS_FP(opname)) {

	    ri = FP_REG_INDEX(opname) ;
	    rp->regs_F.q[ri] = v ;

	} else if (REG_IS_FPCR(opname))
	    rp->regs_C.fpcr = v ;

	else if (REG_IS_UNIQ(opname))
	    rp->regs_C.uniq = v ;

	else if (REG_IS_TMP(opname))
	    rp->regs_C.tmp = v ;

	return 0 ;
}
/* end subroutine (regs_setval) */


/* get a register value for a register out of a register file */
int regs_getval(rp,opname,vp)
struct regs_t	*rp ;
int		opname ;
ULONG		*vp ;
{
	int	ri ;


	if (REG_IS_INT(opname)) {

	    ri = INT_REG_INDEX(opname) ;
	    *vp = (ULONG) rp->regs_R[ri] ;

	} else if (REG_IS_FP(opname)) {

	    ri = FP_REG_INDEX(opname) ;
	    *vp = (ULONG) rp->regs_F.q[ri] ;

	} else if (REG_IS_FPCR(opname))
	    *vp = (ULONG) rp->regs_C.fpcr ;

	else if (REG_IS_UNIQ(opname))
	    *vp = (ULONG) rp->regs_C.uniq ;

	else if (REG_IS_TMP(opname))
	    *vp = (ULONG) rp->regs_C.tmp ;

	else
	    *vp = 0 ;

	return 0 ;
}
/* end subroutine (regs_getval) */



