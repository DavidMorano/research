/* ss_instrexec */

/* execute a single instruction (in a functional simulation context) */


#define	CF_DEBUGS	0		/* debug print-outs */
#define	CF_DEBUG	0		/* switchable debug print-outs */
#define	CF_RJMEM	1		/* right justify memory operands */
#define	CF_SYSCALLRET	1		/* do explicit syscall return */
#define	CF_CORRECT	1		/* do more precise operand handling */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	Here we execute a single instruction (not easy to do at all
	in the SimpleScalar framework) in the context of the functional
	"performance" simulator.

	The context in which this instruction is executed is an AS
	within the functional simulator.  So this thing is supposed
	to resemble a FU or PE or something similar.


******************************************************************************/


#include	<sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* some of Dave's libraries */

#include	<bio.h>
#include	<paramfile.h>
#include	<mallocstuff.h>

/* simple scalar stuff */

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"

/* from 'main' */

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#include	"ss.h"
#include	"ssinfo.h"
#include	"ssdecode.h"
#include	"ssiw.h"
#include	"ssas.h"



/* local defines (not Simple-Scalar) */

#define	SETOPI(asp,oi,opname,type)		\
	if (opname != DNA) {			\
	                    \
	    op = &asp->n.opsi[oi++] ;	\
	    setop(op,type,(ULONG) opname) ;	\
	                    \
	    if (type == OPERAND_TREG)	\
	        setopreg(op,opname) ;	\
	                    \
	}

#define	SETOPO(asp,oi,opname,type)		\
	if (opname != DNA) {			\
	                    \
	    op = &asp->n.opso[oi++] ;	\
	    setop(op,type,(ULONG) opname) ;	\
	                    \
	    if (type == OPERAND_TREG)	\
	        setopreg(op,opname) ;	\
	                    \
	}



/* external subroutines */

extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecui(const char *,int,int *) ;
extern int	cfdecull(const char *,int,ULONG *) ;


/* external variables */

extern struct proginfo	pi ;


/* local structures */


/* forward references */

static int ssas_getreg(SSAS *,int,ULONG *) ;
static int ssas_setreg(SSAS *,int,ULONG) ;
static int ssas_getmem(SSAS *,ULONG,int,ULONG *) ;
static int ssas_setmem(SSAS *,ULONG,int,ULONG) ;
static int ssas_getop(SSAS *,int,ULONG,ULONG *) ;
static int ssas_setop(SSAS *,int,ULONG,int,ULONG) ;

/* subroutines called by the macros */

static LONG	getgpr(int,SSAS *) ;
static LONG	getfprq(int,SSAS *) ;
static LONG	getreg(int,SSAS *) ;

static double	getfprd(int,SSAS *) ;

static int	setgpr(int,LONG,SSAS *) ;
static int	setfprq(int,LONG,SSAS *) ;
static int	setfprd(int,double,SSAS *) ;
static int	setreg(int,LONG,SSAS *) ;

static int	getmembyte(ULONG,SSAS *) ;
static int	getmemhalf(ULONG,SSAS *) ;
static int	getmemword(ULONG,SSAS *) ;
static LONG	getmemlong(ULONG,SSAS *) ;

static int	setmembyte(ULONG,LONG,SSAS *) ;
static int	setmemhalf(ULONG,LONG,SSAS *) ;
static int	setmemword(ULONG,LONG,SSAS *) ;
static int	setmemlong(ULONG,LONG,SSAS *) ;


/* local variables */

static const char	*optypes[] = {
	    "none",
	    "register",
	    "memory",
	    "predicate",
	    NULL
} ;






int ss_instrexec(pip,asp)
SS		*pip ;
SSAS		*asp ;
{
	enum md_opcode op ;

	md_inst_t	inst ;
	md_addr_t	addr ;

	enum md_fault_type fault ;

	ULONG	pv, v ;

	uint	n, e, i ;

	int	rs = SR_OK, rs1 ;
	int	flags ;
	int	is_write ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_instrexec: entered\n") ;
#endif

/* get the OP code */

	op = asp->c.op ;
	inst = asp->c.instrword ;


/* set default reference address and access mode */
	addr = 0 ;
	is_write = FALSE ;

/* set default fault - none */
	fault = md_fault_none ;

	asp->n.ta = asp->c.ia + sizeof(md_inst_t) ;

/* print some stuff */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    char	fbuf[100] ;

	    rs1 = instr_flags(asp->c.flags,fbuf,99) ;
	    eprintf("ss_instrexec: flags= %w\n",fbuf,rs1) ;
	}
#endif /* CF_DEBUG */


/* execute the instruction */

/* next program counter */
#undef	SET_NPC
#define SET_NPC(EXPR)		(asp->n.ta = (EXPR))

/* current program counter */
#undef	CPC
#define CPC			(asp->c.ia)

/* general purpose registers */
#undef	GPR
#define GPR(N)			(getgpr((N),asp))
#undef	SET_GPR
#define SET_GPR(N,EXPR)		(setgpr((N),(EXPR),asp))

/* floating point registers, L->word, F->single-prec, D->double-prec */
#undef	FPR_Q
#define FPR_Q(N)		(getfprq((N),asp))
#undef	SET_FPR_Q
#define SET_FPR_Q(N,EXPR)	(setfprq((N),(EXPR),asp))
#undef	FPR
#define FPR(N)			(getfprd((N),asp))
#undef	SET_FPR
#define SET_FPR(N,EXPR)		(setfprd((N),(EXPR),asp))

/* miscellaneous register accessors */
#undef	FPCR
#define FPCR			(getreg(DFPCR,asp))
#undef	SET_FPCR
#define SET_FPCR(EXPR)		(setreg(DFPCR,(EXPR),asp))
#undef	UNIQ
#define UNIQ			(getreg(DUNIQ,asp))
#undef	SET_UNIQ
#define SET_UNIQ(EXPR)		(setreg(DUNIQ,(EXPR),asp))

/* * configure the execution engine */

/* precise architected memory state accessor macros */

#undef	READ_BYTE
#define READ_BYTE(SRC, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	getmembyte(addr,asp))

#undef	READ_HALF
#define READ_HALF(SRC, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	getmemhalf(addr,asp))

#undef	READ_WORD
#define READ_WORD(SRC, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	getmemword(addr,asp))

#ifdef HOST_HAS_QWORD
#undef	READ_QWORD
#define READ_QWORD(SRC, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	getmemlong(addr,asp))
#endif /* HOST_HAS_QWORD */

#undef	WRITE_BYTE
#define WRITE_BYTE(SRC, DST, FAULT)				\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	setmembyte(addr,v,asp))

#undef	WRITE_HALF
#define WRITE_HALF(SRC, DST, FAULT)				\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	setmemhalf(addr,v,asp))

#undef	WRITE_WORD
#define WRITE_WORD(SRC, DST, FAULT)				\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	setmemword(addr,v,asp))

#ifdef HOST_HAS_QWORD
#undef	WRITE_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)				\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	setmemlong(addr,v,asp))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#undef	SYSCALL
#if	CF_SYSCALLRET
#define SYSCALL(INST)	{ setgpr(0,0LL,asp) ; setgpr(1,0LL,asp) ; }
#define SET_GPR(N,EXPR)		(setgpr((N),(EXPR),asp))
#else
#define SYSCALL(INST)	/* nothing */
#endif

	switch (op) {

#undef	DEFINST
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	    flags = (FLAGS) ;				\
          SYMCAT(OP,_IMPL);						\
          break ;

#undef	DEFLINK
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
          panic("attempted to execute a linking opcode (b)") ;

#undef	CONNECT
#define CONNECT(OP)

#undef	DECLARE_FAULT
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }

#include "machine.def"

	default:
	    panic("attempted to execute a bogus opcode (b)") ;

	} /* end switch */

/* set some destination stuff */

	asp->n.f.taken = asp->n.f.cfcond &&
	    (asp->n.ta != (asp->c.ia + sizeof(md_inst_t))) ;


/* print some stuff */


/* do some other miscellaneous Simple-Scalar things */

#ifdef	COMMENT
	if (fault != md_fault_none)
	    fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC) ;
#endif

#ifdef	COMMENT
	if (MD_OP_FLAGS(op) & F_MEM) {

	    if (MD_OP_FLAGS(op) & F_STORE)
	        is_write = TRUE ;

	}
#endif /* COMMENT */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ss_instrexec: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ss_instrexec) */



/* PRIVATE SUBROUTINES */



/* subroutines for the macros (registers) */

static LONG getgpr(n,asp)
int	n ;
SSAS	*asp ;
{
	LONG	v ;

	int	rs, dn ;


	dn = DGPR(n) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getgpr: n=%u dn=%u\n",n,dn) ;
#endif

	rs = ssas_getreg(asp,dn,(ULONG *) &v) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getgpr: ret v=%016llx\n",v) ;
#endif

	return v ;
}
/* end subroutine (getgpr) */


static int setgpr(n,v,asp)
int	n ;
LONG	v ;
SSAS	*asp ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setgpr: n=%u\n",n) ;
#endif

	rs = ssas_setreg(asp,DGPR(n),(ULONG) v) ;

	return rs ;
}
/* end subroutine (setgpr) */


/* subroutines for the macros */
static LONG getfprq(n,asp)
int	n ;
SSAS	*asp ;
{
	LONG	v ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getfprq: n=%u\n",n) ;
#endif

	rs = ssas_getreg(asp,DFPR(n),(ULONG *) &v) ;

	return v ;
}
/* end subroutine (getfprq) */


static int setfprq(n,v,asp)
int	n ;
LONG	v ;
SSAS	*asp ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setfprq: n=%u\n",n) ;
#endif

	rs = ssas_setreg(asp,DFPR(n),(ULONG) v) ;

	return rs ;
}
/* end subroutine (setfprq) */


static double getfprd(n,asp)
int	n ;
SSAS	*asp ;
{
	double	v ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getfprd: n=%u\n",n) ;
#endif

	rs = ssas_getreg(asp,DFPR(n),(ULONG *) &v) ;

	return v ;
}
/* end subroutine (getfprd) */


static int setfprd(n,v,asp)
int	n ;
double 	v ;
SSAS	*asp ;
{
	ULONG	vv ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setfprd: n=%u\n",n) ;
#endif

	memcpy(&vv,&v,sizeof(ULONG)) ;

	rs = ssas_setreg(asp,DFPR(n),vv) ;

	return rs ;
}
/* end subroutine (setfprd) */


static LONG getreg(n,asp)
int	n ;
SSAS	*asp ;
{
	LONG	v ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getreg: n=%u\n",n) ;
#endif

	rs = ssas_getreg(asp,n,(ULONG *) &v) ;

	return v ;
}
/* end subroutine (getreg) */


static int setreg(n,v,asp)
int	n ;
LONG	v ;
SSAS	*asp ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setreg: n=%u\n",n) ;
#endif

	rs = ssas_setreg(asp,n,(ULONG) v) ;

	return rs ;
}
/* end subroutine (setreg) */


/* subroutines for the macros (memory) */

static int getmembyte(a,asp)
ULONG	a ;
SSAS	*asp ;
{
	LONG	v ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getmembyte: a=%016llx\n",a) ;
#endif

	rs = ssas_getmem(asp,a,1,(ULONG *) &v) ;

	return ((int) v) ;
}
/* end subroutine (getmembyte) */


static int getmemhalf(a,asp)
ULONG	a ;
SSAS	*asp ;
{
	LONG	v ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getmemhalf: a=%016llx\n",a) ;
#endif

	rs = ssas_getmem(asp,a,2,(ULONG *) &v) ;

	return ((int) v) ;
}
/* end subroutine (getmemhalf) */


static int getmemword(a,asp)
ULONG	a ;
SSAS	*asp ;
{
	LONG	v ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getmemword: a=%016llx\n",a) ;
#endif

	rs = ssas_getmem(asp,a,4,(ULONG *) &v) ;

	return ((int) v) ;
}
/* end subroutine (getmemword) */


static LONG getmemlong(a,asp)
ULONG	a ;
SSAS	*asp ;
{
	LONG	v ;

	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/getmemlong: a=%016llx\n",a) ;
#endif

	rs = ssas_getmem(asp,a,8,(ULONG *) &v) ;

	return v ;
}
/* end subroutine (getmemlong) */


static int setmembyte(a,v,asp)
ULONG	a ;
LONG	v ;
SSAS	*asp ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setmembyte: a=%016llx\n",a) ;
#endif

	rs = ssas_setmem(asp,a,1,(ULONG) v) ;

	return rs ;
}
/* end subroutine (setmembyte) */


static int setmemhalf(a,v,asp)
ULONG	a ;
LONG	v ;
SSAS	*asp ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setmemhalf: a=%016llx\n",a) ;
#endif

	rs = ssas_setmem(asp,a,2,(ULONG) v) ;

	return rs ;
}
/* end subroutine (setmemhalf) */


static int setmemword(a,v,asp)
ULONG	a ;
LONG	v ;
SSAS	*asp ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setmemword: a=%016llx\n",a) ;
#endif

	rs = ssas_setmem(asp,a,4,(ULONG) v) ;

	return rs ;
}
/* end subroutine (setmemword) */


static int setmemlong(a,v,asp)
ULONG	a ;
LONG	v ;
SSAS	*asp ;
{
	int	rs ;


#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("ss_instrexec/setmemlong: a=%016llx\n",a) ;
#endif

	rs = ssas_setmem(asp,a,8,(ULONG) v) ;

	return rs ;
}
/* end subroutine (setmemlong) */


/* get a register value */
static int ssas_getreg(asp,a,vp)
SSAS	*asp ;
int	a ;
ULONG	*vp ;
{
	struct proginfo	*pip ;

	ULONG	aa = a ;

	int	rs ;


	pip = asp->pip ;
	rs = ssas_getop(asp,OPERAND_TREG,aa,vp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_getreg: ssas_getop() rs=%d a=%016llx pv=%016llx\n",
	        rs,aa,*vp) ;
#endif

	return rs ;
}
/* end subroutine (ssas_getreg) */


/* set a destination register value in an AS */
static int ssas_setreg(asp,a,v)
SSAS	*asp ;
int	a ;
ULONG	v ;
{
	struct proginfo	*pip ;

	OPERAND	*op ;

	ULONG	aa = a ;

	int	rs, i ;


	pip = asp->pip ;
	rs = ssas_setop(asp,OPERAND_TREG,a,8,v) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_setreg: ssas_setop() rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssas_setreg) */


/* get a memory value */
static int ssas_getmem(asp,a,size,vp)
SSAS	*asp ;
ULONG	a ;
int	size ;
ULONG	*vp ;
{
	int	rs ;


	rs = ssas_getop(asp,OPERAND_TMEM,a,vp) ;

	return rs ;
}
/* end subroutine (ssas_getmem) */


/* set a memory value */
static int ssas_setmem(asp,a,size,v)
SSAS	*asp ;
ULONG	a ;
int	size ;
ULONG	v ;
{
	int	rs ;


	rs = ssas_setop(asp,OPERAND_TMEM,a,size,v) ;

	return rs ;
}
/* end subroutine (ssas_setmem) */


/* get an operand by name */
static int ssas_getop(asp,type,a,vp)
SSAS	*asp ;
int	type ;
ULONG	a ;
ULONG	*vp ;
{
	OPERAND	*op ;

	int	i ;


	*vp = 0 ;
	for (i = 0 ; i < asp->c.nopsi ; i += 1) {

	    op = &asp->c.opsi[i] ;
	    if (op->f.used && (op->type == type) &&
	        (op->a == a)) {

	        *vp = op->pv ;
	        break ;

	    } /* end if */

	} /* end for */

	return (i < asp->c.nopsi) ? 0 : SR_NOTFOUND ;
}
/* end subroutine (ssas_getop) */


/* set an output operand by name */
static int ssas_setop(asp,type,a,size,v)
SSAS	*asp ;
int	type ;
ULONG	a ;
int	size ;
ULONG	v ;
{
	struct proginfo	*pip ;

	OPERAND	*op ;

	ULONG	nv, justmask ;

	int	rs = SR_NOTFOUND ;
	int	i, f ;


	pip = asp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("ssas_setop: entered type=%s(%u) a=%016llx size=%u\n",
	        optypes[type],type,a,size) ;
	    eprintf("ssas_setop: nopso=%u\n",asp->n.nopso) ;
	}
#endif

	justmask = ~ ((~ 0) << (size * 8)) ;

#if	CF_RJMEM
	v = v & justmask ;
#else
	v = v << (a & 15) ;
#endif

	for (i = 0 ; i < asp->n.nopso ; i += 1) {

	    op = &asp->n.opso[i] ;
	    if (op->f.used && (op->type == type)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(5))
	            eprintf("ssas_setop: existing i=%u a=%016llx\n",
			i,op->a) ;
#endif

	        if (op->a == a) {

	            f = (type == OPERAND_TREG) || (size == op->size) ;
	            rs = (f) ? 0 : SR_INVALID ;

	            op->f.export = (v != op->v) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssas_setop: op=%u f.export=%u\n",
				i,op->f.export) ;
#endif

	            break ;

	        } /* end if (address match) */

	    } /* end if (type match) */

	} /* end for */

/* if we are a memory operand AND we didn't find it, then it was a change */

	if ((type == OPERAND_TMEM) && (rs == SR_NOTFOUND)) {

/* find the first memory operand and "NULLIFY" it */

	    for (i = 0 ; i < asp->n.nopso ; i += 1) {

	        op = &asp->n.opso[i] ;
	        if (op->f.used && (op->type == type)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(5))
	                eprintf("ssas_setop: doing a memory wipe\n") ;
#endif

	            op->f.export = TRUE ;	/* output changed */
	            op->f.nullify = TRUE ;	/* NULLIFY needed */
	            op->na = op->a ;	/* set NULLIFY address */
	            op->a = a ;
	            rs = SR_OK ;
	            break ;
	        }

	    } /* end for */

	} /* end if (handling memory changes) */

	if (rs == SR_OK) {

	    op->f.v = TRUE ;	/* valid */
	    op->v = v ;

	} /* end if (got a match) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("ssas_setop: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (ssas_setop) */



