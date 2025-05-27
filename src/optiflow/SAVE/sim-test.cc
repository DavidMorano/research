/* sim-test */

/* sim-test.c - sample functional simulator implementation */


#define	F_DEBUGS	1		/* debug print-outs */
#define	F_TRACE		0		/* debug trace */
#define	F_FAKE		0		/* use fake until we get real */


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
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <cstdio>

/* some of Dave's libraries */

#include	<bio.h>
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

#include	"paramfile.h"
#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#include	"ss.h"
#include	"ssclocking.h"
#include	"ssinfo.h"
#include	"ssmachstate.h"
#include	"ssdecode.h"
#include	"ssiw.h"
#include	"ssas.h"
#include	"machstate.h"



/* local defines (not Simple-Scalar) */

#define	SETOPI(asp,oi,opname,type)		\
	if (opname != DNA) {			\
	                    \
	    op = &asp->n.iops[oi++] ;	\
	    setop(op,type,(ULONG) opname) ;	\
	                    \
	    if (type == OPERAND_TREG)	\
	        setopreg(op,opname) ;	\
	                    \
	}

#define	SETOPO(asp,oi,opname,type)		\
	if (opname != DNA) {			\
	                    \
	    op = &asp->n.oops[oi++] ;	\
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

int	setregval(struct regs_t *,int,ULONG) ;
int	getregval(struct regs_t *,int,ULONG *) ;

int	sim_exec(struct ss *,uint) ;
int	sim_retire(struct ss *,uint) ;

static int	sim_fast(struct ss *,uint) ;
static int	sim_perf(struct ss *,uint) ;
static int	setas(SSAS *,int,int,int,int,int,int,int) ;
static int	setasreg(SSAS *) ;
static int	setasmem(SSAS *,int,ULONG,int,ULONG,ULONG) ;
static int	setasmemread(SSAS *,ULONG,int,ULONG) ;
static int	setop(OPERAND *,uint,ULONG) ;
static int	setopreg(OPERAND *,int) ;
static int	mkdatapresent(uint,int) ;
static int	g_asdump(struct ss *,SSAS *) ;


/* local variables */

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */

/* simulated registers */
static struct regs_t regs ;

/* simulated memory */
static struct mem_t *mem = NULL ;

/* track number of refs */
static counter_t sim_num_refs = 0 ;

/* maximum number of inst's to execute */
static unsigned int max_insts ;

/* number of instructions to fast forward */
static unsigned int fastfwd_count ;

static struct ss	g ;

static unsigned short dps[] = {
	    0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f,
	    0x00ff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0x0000, 0x0002, 0x0006, 0x000e, 0x001e, 0x003e, 0x007e, 0x00fe,
	    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0x0000, 0x0004, 0x000c, 0x001c, 0x003c, 0x007c, 0x00fc, 0xffff,
	    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0x0000, 0x0008, 0x0018, 0x0038, 0x0078, 0x00f8, 0xffff, 0xffff,
	    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0x0000, 0x0010, 0x0030, 0x0070, 0x00f0, 0xffff, 0xffff, 0xffff,
	    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0x0000, 0x0020, 0x0060, 0x00e0, 0xffff, 0xffff, 0xffff, 0xffff,
	    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0x0000, 0x0040, 0x00c0, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0x0000, 0x0080, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
} ;

static const char	*ssparams[] = {
	"fwsize",
	"dwsize",
	"iwsize",
	"sinstr",
	"ninstr",
	NULL
} ;

enum ssparams {
	ssparam_fwsize,		/* fetch width */
	ssparam_dwsize,		/* dispatch width */
	ssparam_iwsize,		/* issue width */
	ssparam_sinstr,		/* number of instructions to skip */
	ssparam_ninstr,		/* number of instructions to execute */
	ssparam_overlast
} ;






/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{


	opt_reg_header(odb, 
	    "sim-test: This simulator implements a functional simulator.  This\n"
	    "functional simulator is the simplest, most user-friendly simulator in the\n"
	    "simplescalar tool set.  Unlike sim-fast, this functional simulator checks\n"
	    "for all instruction errors, and the implementation is crafted for clarity\n"
	    "rather than speed.\n"
	    ) ;

/* fast forward */
	opt_reg_int(odb, "-fastfwd", 
		"number of insts skipped before timing starts",
	    &fastfwd_count, /* default */0,
	    /* print */TRUE, /* format */NULL) ;

/* instruction limit */
	opt_reg_uint(odb, "-max:inst", 
		"maximum number of inst's to execute",
	    &max_insts, /* default */0,
	    /* print */TRUE, /* format */NULL) ;

}
/* end subroutine (sim_reg_options) */


/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{


/* nada */
}
/* end subroutine (sim_check_options) */


/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{


	stat_reg_counter(sdb, "sim_num_insn",
	    "total number of instructions executed",
	    &sim_num_insn, sim_num_insn, NULL) ;
	stat_reg_counter(sdb, "sim_num_refs",
	    "total number of loads and stores executed",
	    &sim_num_refs, 0, NULL) ;
	stat_reg_int(sdb, "sim_elapsed_time",
	    "total simulation time in seconds",
	    &sim_elapsed_time, 0, NULL) ;
	stat_reg_formula(sdb, "sim_inst_rate",
	    "simulation speed (in insts/sec)",
	    "sim_num_insn / sim_elapsed_time", NULL) ;
	ld_reg_stats(sdb) ;
	mem_reg_stats(mem, sdb) ;
}
/* end subroutine (sim_reg_stats) */


/* initialize the simulator */
void
sim_init(void)
{
	struct proginfo	*pip = &pi ;

	struct ss	*gp = &g ;

	struct ssinfo	*ip = &gp->i ;

	struct ssmachstate	*msp = &g.ms ;

	uint	size ;

	int	rs, i ;
	int	sl, cl ;

	char	*sp, *cp ;


	(void) memset(gp,0,sizeof(struct ss)) ;

	sim_num_refs = 0 ;

/* allocate and initialize register file */
	regs_init(&regs) ;

/* allocate and initialize memory space */
	mem = mem_create("mem") ;
	mem_init(mem) ;


	(void) memset(&g,0,sizeof(struct ss)) ;

	bopen(&g.tfile,"trace","wct",0666) ;


/* initialze the parameters */

	memset(ip,0,sizeof(struct ssinfo)) ;

/* read parameters that we are interested in */

	if (pip->f.params) {

		ULONG	ulv ;

		uint	uv ;


	for (i = 0 ; ssparams[i] != NULL ; i += 1) {

	    sl = paramfile_fetch(&pip->pf,ssparams[i],NULL,&sp) ;

	    if (sl >= 0) {

	        switch (i) {

	        case ssparam_fwsize:
	        case ssparam_dwsize:
	        case ssparam_iwsize:
	            if (cfnumui(sp,sl,&uv) >= 0) {

	                switch (i) {

	                case ssparam_fwsize:
	                    ip->fwsize= uv ;
	                    break ;

	                case ssparam_dwsize:
	                    ip->dwsize = uv ;
	                    break ;

	                case ssparam_iwsize:
	                    ip->iwsize = uv ;
	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        case ssparam_sinstr:
	        case ssparam_ninstr:
	            if (cfnumull(sp,sl,&ulv) >= 0) {

	                switch (i) {

	                case ssparam_sinstr:
	                    ip->sinstr = ulv ;

			break ;

	                case ssparam_ninstr:
	                    ip->ninstr = ulv ;

			break ;

			} /* end switch */

		} /* end if (got a number) */

		break ;

#ifdef	COMMENT
	        case LPARAM_BUSTRACE:
	            strcpy(btfname,cp) ;

	            break ;

	        case LPARAM_MASTERTRACE:
	            strcpy(mtfname,cp) ;

	            break ;
#endif /* COMMENT */

	        } /* end switch (outer) */

	    } /* end if (got a good parameter) */

	} /* end for (getting the geometry-like parameters) */

	} /* end if (reading parameters) */


/* define some default parameters */

	if (ip->iwsize <= 0)
		ip->iwsize = SSINFO_IWSIZE ;

	ip->modulus = SSINFO_MODULUS ;

	ip->rfmod = ip->modulus ;
	ip->rbmod = ip->modulus ;


/* check the parameters */

		if (ip->fwsize <= 1)
			ip->fwsize = 1 ;

		if (ip->dwsize <= 1)
			ip->dwsize = 1 ;

		if (ip->iwsize <= 1)
			ip->iwsize = 1 ;


/* determine checker window size */

	msp->wsize = ip->iwsize + 2 ;

/* allocate the checkers ASes */

	size = msp->wsize * sizeof(SSAS) ;
	rs = uc_malloc(size,&msp->cas) ;

	if (rs < 0)
		pip->f.exit = TRUE ;


}
/* end subroutine (sim_init) */


/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
int argc, char **argv,	/* program arguments */
char **envp)		/* program environment */
{


/* load program text and data, set up environment, memory, and regs */
	ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE) ;

/* initialize the DLite debugger */
	dlite_init(md_reg_obj, dlite_mem_obj, dlite_mstate_obj) ;
}


/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{


/* nothing currently */
}


/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{


/* nada */
}


/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
	struct ss	*gp = &g ;



/* print out some statistics */

	bprintf(&g.tfile,"in=%lld\n",
	    g.s.in) ;

	bprintf(&g.tfile,"memory %lld\n",
	    g.s.mem) ;

	bprintf(&g.tfile,"memory reads %lld\n",
	    g.s.memreads) ;

	bprintf(&g.tfile,"memory writes %lld\n",
	    g.s.memwrites) ;

	bprintf(&g.tfile,"unaligned memory %lld\n",
	    g.s.memunaligned) ;

	bprintf(&g.tfile,"memory multiple %lld\n",
	    g.s.memmulti) ;

	bprintf(&g.tfile,"weird %lld\n",
	    g.s.weird) ;

	bprintf(&g.tfile,"cf %lld\n",g.s.cf) ;

	bprintf(&g.tfile,"cfind %lld\n",g.s.cfind) ;

	bprintf(&g.tfile,"cfsub %lld\n",g.s.cfsub) ;

	bprintf(&g.tfile,"cfcond %lld\n",g.s.cfcond) ;

	bclose(&g.tfile) ;


/* free up the checker ASes */

	if (gp->ms.cas != NULL)
		free(gp->ms.cas) ;



}
/* end subroutine (sim_uninit) */


/* start simulation, program loaded, processor precise state initialized */
void sim_main(void)
{
	struct proginfo		*pip = &pi ;

	struct ss		*gp = &g ;

	struct ssinfo		*ip = &g.i ;

	struct ssmachstate	*msp = &gp->ms ;

	uint	n ;

	int	rs, j ;


/* set up initial default next PC */
	regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t) ;

/* check for DLite debugger entry condition */
	if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
	    dlite_main(regs.regs_PC - sizeof(md_inst_t),
	        regs.regs_PC, sim_num_insn, &regs, mem) ;


	msp->in = 0 ;
	msp->wi = 0 ;
	msp->nw = 0 ;

/* fast forward */

	fprintf(stderr, "sim: ** starting fastfwd simulation **\n") ;

/* fast forward (for the most part */

	if (fastfwd_count > 0) {

		n = fastfwd_count ;
		rs = sim_fast(gp,n) ;

	}

/* set the "committed" register file to the leading edge of the checker */

	msp->regs = regs ;

/* open up the checker window */

		n = ip->iwsize ;
		rs = sim_exec(gp,n) ;


/* do performance simulation */

	fprintf(stderr, "sim: ** starting functional simulation **\n") ;

#if	F_FAKE
	n = INT_MAX ;
	rs = sim_perf(gp,n) ;
#else /* F_FAKE */
	{
		struct machstate	ms ;

		SSCLOCKING		clocker ;

		SSCLOCKING_MACHOBJ	mobj ;

		SSIW		iwindow ;

		SSIW_INITARGS	ia ;


	memset(&ms,0,sizeof(struct machstate)) ;

	ms.in = msp->in ;
	ms.ia = msp->regs.regs_PC ;

	for (j = 0 ; j < MACHSTATE_NREGS ; j += 1) {

	        getregval(&msp->regs,j,&ms.regs[j]) ;

	} /* end if */

/* initialize the clocker */

	memset(&mobj,0,sizeof(SSCLOCKING_MACHOBJ)) ;

	mobj.topobjp = &iwindow ;
	mobj.topcombp = (int (*)(void *,int)) ssiw_comb ;
	mobj.topclockp = (int (*)(void *)) ssiw_clock ;

	ssclocking_init(&clocker,pip,0L,&mobj) ;

/* initialize the Levo instruction window (SSIW) */

	memset(&ia,0,sizeof(SSCLOCKING_MACHOBJ)) ;

	ia.smp = &ms ;
	ia.iwsize = ip->iwsize ;

	ssiw_init(&iwindow,pip,gp,ip,&ia) ;


/* do the main loop */

	ssclocking_loop(&clocker) ;

/* free stuff up */

	ssiw_free(&iwindow) ;

	ssclocking_free(&clocker) ;

	} /* end block */
#endif /* F_FAKE */

/* VOIDRETURN */

}
/* end subroutine (sim_main) */



/* LOCAL SUBROUTINES */



/* do a fast-forward operation */
static int sim_fast(gp,ninstr)
struct ss	*gp ;
uint		ninstr ;
{
	struct ssmachstate	*msp = &gp->ms ;

	struct ssinfo		*ip = &gp->i ;

	SSAS	*cas = gp->ms.cas ;
	SSAS	*asp ;

	uint	i ;


	for (i = 0 ; i < ninstr ; i += 1) {

	    enum md_opcode op ;

	    md_inst_t	inst ;

	    enum md_fault_type fault ;

	    md_addr_t	addr ;

	    ULONG	pv, v ;

	    int	wi ;
	    int	flags ;
	    int	is_write ;


/* maintain $r0 semantics */
	    regs.regs_R[MD_REG_ZERO] = 0 ;
#ifdef TARGET_ALPHA
	    regs.regs_F.d[MD_REG_ZERO] = 0.0 ;
#endif /* TARGET_ALPHA */

/* load up a checker slot */

	    wi = msp->wi ;
	    asp = cas + wi ;
	    memset((cas + wi),0,sizeof(SSAS)) ;

	    cas[wi].n.ia = regs.regs_PC ;
	    cas[wi].n.ta = regs.regs_NPC ;

/* get the next instruction to execute */
	    MD_FETCH_INST(inst, mem, regs.regs_PC) ;

	    cas[wi].n.instr = inst ;

/* keep an instruction count */
	    sim_num_insn += 1 ;

/* set default reference address and access mode */
	    addr = 0; 
	    is_write = FALSE ;

/* set default fault - none */
	    fault = md_fault_none ;

/* decode the instruction */
	    MD_SET_OPCODE(op, inst) ;

	    cas[wi].n.op = op ;


/* execute the instruction */

/* next program counter */
#undef	SET_NPC
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#undef	CPC
#define CPC			(regs.regs_PC)

/* general purpose registers */
#undef	GPR
#define GPR(N)			(regs.regs_R[N])
#undef	SET_GPR
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

/* floating point registers, L->word, F->single-prec, D->double-prec */
#undef	FPR_Q
#define FPR_Q(N)		(regs.regs_F.q[N])
#undef	SET_FPR_Q
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#undef	FPR
#define FPR(N)			(regs.regs_F.d[(N)])
#undef	SET_FPR
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#undef	FPCR
#define FPCR			(regs.regs_C.fpcr)
#undef	SET_FPCR
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#undef	UNIQ
#define UNIQ			(regs.regs_C.uniq)
#undef	SET_UNIQ
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

/* * configure the execution engine */

/* precise architected memory state accessor macros */

#undef	READ_BYTE
#define READ_BYTE(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(asp,1,addr,1,pv,0LL), pv)

#undef	READ_HALF
#define READ_HALF(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(asp,1,addr,2,pv,0LL), pv)

#undef	READ_WORD
#define READ_WORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(asp,1,addr,4,pv,0LL), pv)

#ifdef HOST_HAS_QWORD
#undef	READ_QWORD
#define READ_QWORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(asp,1,addr,8,pv,0LL), pv)
#endif /* HOST_HAS_QWORD */

#undef	WRITE_BYTE
#define WRITE_BYTE(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(asp,0,addr,1,pv,v),		\
  	MEM_WRITE_BYTE(mem,addr,v))

#undef	WRITE_HALF
#define WRITE_HALF(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(asp,0,addr,2,pv,v),		\
  	MEM_WRITE_HALF(mem,addr,v))

#undef	WRITE_WORD
#define WRITE_WORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(asp,0,addr,4,pv,v),		\
  	MEM_WRITE_WORD(mem,addr,v))

#ifdef HOST_HAS_QWORD
#undef	WRITE_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(asp,0,addr,8,pv,v),		\
  	MEM_WRITE_QWORD(mem,addr,v))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#undef	SYSCALL
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

	    switch (op) {

#undef	DEFINST
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	    flags = (FLAGS) ;				\
	    setas((cas + wi),RES,flags,O1,O2,I1,I2,I3) ;	\
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

/* set the destination register values */

	    setasreg((cas + wi)) ;

	    asp->n.f.taken = asp->n.f.cfcond &&
	        (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;

	    if (asp->n.f.cfind && (flags & F_INDIRJMP))
	        g.s.weird += 1 ;

	    if (flags & F_MEM) {

	        g.s.mem += 1 ;
	        if (flags & F_LOAD) {

	            g.s.memreads += 1 ;
	            if (flags & F_STORE)
	                g.s.weird += 1 ;

	        } else
	            g.s.memwrites += 1 ;

	    } /* end if (memory operation) */

	    if (asp->n.f.memunalign)
	        g.s.memunaligned += 1 ;

	    if (asp->n.f.nmem > 1)
	        g.s.memmulti += 1 ;

	    if (asp->n.f.cf)
	        g.s.cf += 1 ;

	    if (asp->n.f.cfind)
	        g.s.cfind += 1 ;

	    if (asp->n.f.cfsub)
	        g.s.cfsub += 1 ;

	    if (asp->n.f.cfcond)
	        g.s.cfcond += 1 ;

#if	F_TRACE
	    g_asdump(&g,(cas + wi)) ;
#endif


/* do some other miscellaneous Simple-Scalar things */

	    if (fault != md_fault_none)
	        fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC) ;

	    if (verbose && sim_num_insn >= trigger_inst)
	    {
	        myfprintf(stderr, "COMMIT: %10n [xor: 0x%08x] @ 0x%08p: ",
	            sim_num_insn, md_xor_regs(&regs), regs.regs_PC) ;
	        md_print_insn(inst, regs.regs_PC, stderr) ;
	        fprintf(stderr, "\n") ;
	        if (verbose_regs)
	        {
	            md_print_iregs(regs.regs_R, stderr) ;
	            md_print_fpregs(regs.regs_F, stderr) ;
	            md_print_cregs(regs.regs_C, stderr) ;
	            fprintf(stderr, "\n") ;
	        }
/* fflush(stderr); */
	    }

	    if (MD_OP_FLAGS(op) & F_MEM)
	    {
	        sim_num_refs++ ;
	        if (MD_OP_FLAGS(op) & F_STORE)
	            is_write = TRUE ;
	    }

/* check for DLite debugger entry condition */
	    if (dlite_check_break(regs.regs_NPC,
	        is_write ? ACCESS_WRITE : ACCESS_READ,
	        addr, sim_num_insn, sim_num_insn))

	        dlite_main(regs.regs_PC, regs.regs_NPC, 
			sim_num_insn, &regs, mem) ;

/* go to the next instruction */
	    regs.regs_PC = regs.regs_NPC ;
	    regs.regs_NPC += sizeof(md_inst_t) ;

	    gp->ms.in += 1 ;
	    g.s.in += 1 ;

#ifdef	COMMENT

/* finish early? */

	    if (fastfwd_count && (sim_num_insn >= fastfwd_count))
	        break ;

	    if (max_insts && (sim_num_insn >= max_insts))
	        break ;

#endif /* COMMENT */

/* turn the window */

	    msp->wi = (wi + 1) % ip->iwsize ;

	} /* end for (reading and executing instrutions) */

	return 0 ;
}
/* end subroutine (sim_fast) */


/* execute some checker instructions */
int sim_exec(gp,ninstr)
struct ss	*gp ;
uint		ninstr ;
{
	struct ssmachstate	*msp = &gp->ms ;

	struct ssinfo		*ip = &gp->i ;

	SSAS	*cas = gp->ms.cas ;
	SSAS	*asp ;

	uint	n, e, i ;


/* get number that we can execute starting with requested number */

	e = ip->iwsize - msp->nw ;
	n = MIN(ninstr,e) ;

	for (i = 0 ; i < n ; i += 1) {

	    enum md_opcode op ;

	    md_inst_t	inst ;

	    enum md_fault_type fault ;

	    md_addr_t	addr ;

	    ULONG	pv, v ;

	    int	wi ;
	    int	flags ;
	    int	is_write ;


/* maintain $r0 semantics */
	    regs.regs_R[MD_REG_ZERO] = 0 ;
#ifdef TARGET_ALPHA
	    regs.regs_F.d[MD_REG_ZERO] = 0.0 ;
#endif /* TARGET_ALPHA */

/* load up a checker slot */

	    wi = msp->wi ;
	    asp = cas + wi ;
	    memset((cas + wi),0,sizeof(SSAS)) ;

	    cas[wi].n.ia = regs.regs_PC ;
	    cas[wi].n.ta = regs.regs_NPC ;

/* get the next instruction to execute */
	    MD_FETCH_INST(inst, mem, regs.regs_PC) ;

	    cas[wi].n.instr = inst ;

/* keep an instruction count */
	    sim_num_insn += 1 ;

/* set default reference address and access mode */
	    addr = 0; 
	    is_write = FALSE ;

/* set default fault - none */
	    fault = md_fault_none ;

/* decode the instruction */
	    MD_SET_OPCODE(op, inst) ;

	    cas[wi].n.op = op ;


/* execute the instruction */

/* next program counter */
#undef	SET_NPC
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#undef	CPC
#define CPC			(regs.regs_PC)

/* general purpose registers */
#undef	GPR
#define GPR(N)			(regs.regs_R[N])
#undef	SET_GPR
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

/* floating point registers, L->word, F->single-prec, D->double-prec */
#undef	FPR_Q
#define FPR_Q(N)		(regs.regs_F.q[N])
#undef	SET_FPR_Q
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#undef	FPR
#define FPR(N)			(regs.regs_F.d[(N)])
#undef	SET_FPR
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#undef	FPCR
#define FPCR			(regs.regs_C.fpcr)
#undef	SET_FPCR
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#undef	UNIQ
#define UNIQ			(regs.regs_C.uniq)
#undef	SET_UNIQ
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

/* * configure the execution engine */

/* precise architected memory state accessor macros */

#undef	READ_BYTE
#define READ_BYTE(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(asp,1,addr,1,pv,0LL), pv)

#undef	READ_HALF
#define READ_HALF(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(asp,1,addr,2,pv,0LL), pv)

#undef	READ_WORD
#define READ_WORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(asp,1,addr,4,pv,0LL), pv)

#ifdef HOST_HAS_QWORD
#undef	READ_QWORD
#define READ_QWORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(asp,1,addr,8,pv,0LL), pv)
#endif /* HOST_HAS_QWORD */

#undef	WRITE_BYTE
#define WRITE_BYTE(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(asp,0,addr,1,pv,v),		\
  	MEM_WRITE_BYTE(mem,addr,v))

#undef	WRITE_HALF
#define WRITE_HALF(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(asp,0,addr,2,pv,v),		\
  	MEM_WRITE_HALF(mem,addr,v))

#undef	WRITE_WORD
#define WRITE_WORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(asp,0,addr,4,pv,v),		\
  	MEM_WRITE_WORD(mem,addr,v))

#ifdef HOST_HAS_QWORD
#undef	WRITE_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(asp,0,addr,8,pv,v),		\
  	MEM_WRITE_QWORD(mem,addr,v))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#undef	SYSCALL
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

	    switch (op) {

#undef	DEFINST
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	    flags = (FLAGS) ;				\
	    setas((cas + wi),RES,flags,O1,O2,I1,I2,I3) ;	\
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

/* set the destination register values */

	    setasreg((cas + wi)) ;

	    asp->n.f.taken = asp->n.f.cfcond &&
	        (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;

	    if (asp->n.f.cfind && (flags & F_INDIRJMP))
	        g.s.weird += 1 ;

	    if (flags & F_MEM) {

	        g.s.mem += 1 ;
	        if (flags & F_LOAD) {

	            g.s.memreads += 1 ;
	            if (flags & F_STORE)
	                g.s.weird += 1 ;

	        } else
	            g.s.memwrites += 1 ;

	    } /* end if (memory operation) */

	    if (asp->n.f.memunalign)
	        g.s.memunaligned += 1 ;

	    if (asp->n.f.nmem > 1)
	        g.s.memmulti += 1 ;

	    if (asp->n.f.cf)
	        g.s.cf += 1 ;

	    if (asp->n.f.cfind)
	        g.s.cfind += 1 ;

	    if (asp->n.f.cfsub)
	        g.s.cfsub += 1 ;

	    if (asp->n.f.cfcond)
	        g.s.cfcond += 1 ;

#if	F_TRACE
	    g_asdump(&g,(cas + wi)) ;
#endif


/* do some other miscellaneous Simple-Scalar things */

	    if (fault != md_fault_none)
	        fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC) ;

	    if (verbose && sim_num_insn >= trigger_inst)
	    {
	        myfprintf(stderr, "COMMIT: %10n [xor: 0x%08x] @ 0x%08p: ",
	            sim_num_insn, md_xor_regs(&regs), regs.regs_PC) ;
	        md_print_insn(inst, regs.regs_PC, stderr) ;
	        fprintf(stderr, "\n") ;
	        if (verbose_regs)
	        {
	            md_print_iregs(regs.regs_R, stderr) ;
	            md_print_fpregs(regs.regs_F, stderr) ;
	            md_print_cregs(regs.regs_C, stderr) ;
	            fprintf(stderr, "\n") ;
	        }
/* fflush(stderr); */
	    }

	    if (MD_OP_FLAGS(op) & F_MEM)
	    {
	        sim_num_refs++ ;
	        if (MD_OP_FLAGS(op) & F_STORE)
	            is_write = TRUE ;
	    }

/* check for DLite debugger entry condition */
	    if (dlite_check_break(regs.regs_NPC,
	        is_write ? ACCESS_WRITE : ACCESS_READ,
	        addr, sim_num_insn, sim_num_insn))

	        dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem) ;

/* go to the next instruction */
	    regs.regs_PC = regs.regs_NPC ;
	    regs.regs_NPC += sizeof(md_inst_t) ;

	    gp->ms.in += 1 ;
	    g.s.in += 1 ;

/* turn the window */

	    msp->wi = (wi + 1) % ip->iwsize ;

/* finish early? */

	    if (max_insts && (sim_num_insn >= max_insts))
	        break ;

	} /* end for (reading and executing instrutions) */

	msp->nw += n ;

	return 0 ;
}
/* end subroutine (sim_exec) */


int sim_retire(gp,ninstr)
struct ss	*gp ;
uint		ninstr ;
{
	struct ssmachstate	*msp = &gp->ms ;

	struct ssinfo		*ip = &gp->i ;

	SSAS	*cas = gp->ms.cas ;
	SSAS	*asp ;

	uint	n, i ;

	int	ri, j ;


/* get number that we can retire starting with requested number */

	n = MIN(ninstr, msp->nw) ;

/* get the current checker AS read index */

	ri = (msp->wi - msp->nw + ip->iwsize) % ip->iwsize ;

/* loop over leaving (going out of the window) checker ASes */

	for (i = 0 ; i < n ; i += 1) {

		OPERAND	*op ;

		int	opname ;


		asp = cas + ri ;

/* loop over output (destination) operands and write them back */

	for (j = 0 ; j < asp->n.nopso ; j += 1) {

	    op = &asp->n.oops[j] ;
	    if (op->f_used && (op->type == OPERAND_TREG)) {

	        opname = (int) op->a ;
	        setregval(&msp->regs,opname,op->v) ;

	    } /* end if */

	} /* end for (writing operands) */

		ri = (ri + 1) % ip->iwsize ;

	} /* end for (looping over checker ASes) */

	msp->nw -= n ;

	return 0 ;
}
/* end subroutine (sim_retire) */


/* do the "performance" simulation */
static int sim_perf(gp,n)
struct ss	*gp ;
uint		n ;
{
	struct ssmachstate	*msp = &gp->ms ;

	struct ssinfo		*ip = &gp->i ;

	SSAS	*cas = gp->ms.cas ;
	SSAS	*asp ;

	uint	i ;


	for (i = 0 ; i < n ; i += 1) {

	    enum md_opcode op ;

	    md_inst_t	inst ;

	    enum md_fault_type fault ;

	    md_addr_t	addr ;

	    ULONG	pv, v ;

	    int	wi ;
	    int	flags ;
	    int	is_write ;


/* maintain $r0 semantics */
	    regs.regs_R[MD_REG_ZERO] = 0 ;
#ifdef TARGET_ALPHA
	    regs.regs_F.d[MD_REG_ZERO] = 0.0 ;
#endif /* TARGET_ALPHA */

/* load up a checker slot */

	    wi = msp->wi ;
	    asp = cas + wi ;
	    memset((cas + wi),0,sizeof(SSAS)) ;

	    cas[wi].n.ia = regs.regs_PC ;
	    cas[wi].n.ta = regs.regs_NPC ;

/* get the next instruction to execute */
	    MD_FETCH_INST(inst, mem, regs.regs_PC) ;

	    cas[wi].n.instr = inst ;

/* keep an instruction count */
	    sim_num_insn += 1 ;

/* set default reference address and access mode */
	    addr = 0; 
	    is_write = FALSE ;

/* set default fault - none */
	    fault = md_fault_none ;

/* decode the instruction */
	    MD_SET_OPCODE(op, inst) ;

	    cas[wi].n.op = op ;


/* execute the instruction */

/* next program counter */
#undef	SET_NPC
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#undef	CPC
#define CPC			(regs.regs_PC)

/* general purpose registers */
#undef	GPR
#define GPR(N)			(regs.regs_R[N])
#undef	SET_GPR
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

/* floating point registers, L->word, F->single-prec, D->double-prec */
#undef	FPR_Q
#define FPR_Q(N)		(regs.regs_F.q[N])
#undef	SET_FPR_Q
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#undef	FPR
#define FPR(N)			(regs.regs_F.d[(N)])
#undef	SET_FPR
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#undef	FPCR
#define FPCR			(regs.regs_C.fpcr)
#undef	SET_FPCR
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#undef	UNIQ
#define UNIQ			(regs.regs_C.uniq)
#undef	SET_UNIQ
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

/* * configure the execution engine */

/* precise architected memory state accessor macros */

#undef	READ_BYTE
#define READ_BYTE(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(asp,1,addr,1,pv,0LL), pv)

#undef	READ_HALF
#define READ_HALF(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(asp,1,addr,2,pv,0LL), pv)

#undef	READ_WORD
#define READ_WORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(asp,1,addr,4,pv,0LL), pv)

#ifdef HOST_HAS_QWORD
#undef	READ_QWORD
#define READ_QWORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(asp,1,addr,8,pv,0LL), pv)
#endif /* HOST_HAS_QWORD */

#undef	WRITE_BYTE
#define WRITE_BYTE(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(asp,0,addr,1,pv,v),		\
  	MEM_WRITE_BYTE(mem,addr,v))

#undef	WRITE_HALF
#define WRITE_HALF(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(asp,0,addr,2,pv,v),		\
  	MEM_WRITE_HALF(mem,addr,v))

#undef	WRITE_WORD
#define WRITE_WORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(asp,0,addr,4,pv,v),		\
  	MEM_WRITE_WORD(mem,addr,v))

#ifdef HOST_HAS_QWORD
#undef	WRITE_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(asp,0,addr,8,pv,v),		\
  	MEM_WRITE_QWORD(mem,addr,v))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#undef	SYSCALL
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

	    switch (op) {

#undef	DEFINST
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	    flags = (FLAGS) ;				\
	    setas((cas + wi),RES,flags,O1,O2,I1,I2,I3) ;	\
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

/* set the destination register values */

	    setasreg((cas + wi)) ;

	    asp->n.f.taken = asp->n.f.cfcond &&
	        (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;

	    if (asp->n.f.cfind && (flags & F_INDIRJMP))
	        g.s.weird += 1 ;

	    if (flags & F_MEM) {

	        g.s.mem += 1 ;
	        if (flags & F_LOAD) {

	            g.s.memreads += 1 ;
	            if (flags & F_STORE)
	                g.s.weird += 1 ;

	        } else
	            g.s.memwrites += 1 ;

	    } /* end if (memory operation) */

	    if (asp->n.f.memunalign)
	        g.s.memunaligned += 1 ;

	    if (asp->n.f.nmem > 1)
	        g.s.memmulti += 1 ;

	    if (asp->n.f.cf)
	        g.s.cf += 1 ;

	    if (asp->n.f.cfind)
	        g.s.cfind += 1 ;

	    if (asp->n.f.cfsub)
	        g.s.cfsub += 1 ;

	    if (asp->n.f.cfcond)
	        g.s.cfcond += 1 ;

#if	F_TRACE
	    g_asdump(&g,(cas + wi)) ;
#endif


/* do some other miscellaneous Simple-Scalar things */

	    if (fault != md_fault_none)
	        fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC) ;

	    if (verbose && sim_num_insn >= trigger_inst)
	    {
	        myfprintf(stderr, "COMMIT: %10n [xor: 0x%08x] @ 0x%08p: ",
	            sim_num_insn, md_xor_regs(&regs), regs.regs_PC) ;
	        md_print_insn(inst, regs.regs_PC, stderr) ;
	        fprintf(stderr, "\n") ;
	        if (verbose_regs)
	        {
	            md_print_iregs(regs.regs_R, stderr) ;
	            md_print_fpregs(regs.regs_F, stderr) ;
	            md_print_cregs(regs.regs_C, stderr) ;
	            fprintf(stderr, "\n") ;
	        }
/* fflush(stderr); */
	    }

	    if (MD_OP_FLAGS(op) & F_MEM)
	    {
	        sim_num_refs++ ;
	        if (MD_OP_FLAGS(op) & F_STORE)
	            is_write = TRUE ;
	    }

/* check for DLite debugger entry condition */
	    if (dlite_check_break(regs.regs_NPC,
	        is_write ? ACCESS_WRITE : ACCESS_READ,
	        addr, sim_num_insn, sim_num_insn))

	        dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem) ;

/* go to the next instruction */
	    regs.regs_PC = regs.regs_NPC ;
	    regs.regs_NPC += sizeof(md_inst_t) ;

	    gp->ms.in += 1 ;
	    g.s.in += 1 ;

/* turn the window */

	    msp->wi = (wi + 1) % ip->iwsize ;

/* finish early? */

	    if (max_insts && (sim_num_insn >= max_insts))
	        break ;

	} /* end for (reading and executing instrutions) */

	return 0 ;
}
/* end subroutine (sim_perf) */


/* set a checker AS from decoded instruction information */
static int setas(asp,res,flags,o1,o2,i1,i2,i3)
SSAS	*asp ;
int	res, flags ;
int	o1, o2 ;
int	i1, i2, i3 ;
{
	OPERAND		*op ;

	int		oi ;


#if	F_TRACE
	bprintf(&g.tfile,"i1=%d i2=%d i3=%d o1=%d o2=%d\n",
	    i1,i2,i3,o1,o2) ;
#endif

/* instruction class and instruction flags */

	asp->n.class = res ;
	asp->n.flags = flags ;

/* input operands */

	oi = 0 ;
	SETOPI(asp,oi,DEP_NAME(i1),OPERAND_TREG) ;

	SETOPI(asp,oi,DEP_NAME(i2),OPERAND_TREG) ;

	SETOPI(asp,oi,DEP_NAME(i3),OPERAND_TREG) ;

	asp->n.nopsi = oi ;

/* output operands */

	oi = 0 ;
	SETOPO(asp,oi,DEP_NAME(o1),OPERAND_TREG) ;

	SETOPO(asp,oi,DEP_NAME(o2),OPERAND_TREG) ;

	asp->n.nopso = oi ;

/* some miscellaneous stuff that we can determine */

	asp->n.f.cf = (flags & F_CTRL) || (flags & F_FPCOND) ;
	asp->n.f.cfcond = (flags & F_COND) || (flags & F_FPCOND) ;
	asp->n.f.cfind = (flags & F_INDIRJMP) ;
	asp->n.f.cfsub = (flags & F_CALL) || (flags & F_TRAP) ;

	return 0 ;
}
/* end subroutine (setas) */


/* set all of the destination register values in an AS */
static int setasreg(asp)
SSAS	*asp ;
{
	OPERAND	*op ;

	int	i ;
	int	opname ;


	for (i = 0 ; i < asp->n.nopso ; i += 1) {

	    op = &asp->n.oops[i] ;
	    if (op->f_used && (op->type == OPERAND_TREG)) {

	        opname = (int) op->a ;
	        getregval(&regs,opname,&op->v) ;

	    } /* end if */

	} /* end for */

	return 0 ;
}
/* end subroutine (setasreg) */


/* set a previous memory value in an AS */
static int setasmem(asp,f,a,size,pv,v)
SSAS	*asp ;
int	f ;
ULONG	a ;
int	size ;
ULONG	pv, v ;
{
	OPERAND	*op ;

	int	i ;


#if	F_TRACE
	bprintf(&g.tfile,"mem a=%016llx size=%d pv=%016llx\n",
	    a,size,pv) ;
#endif

/* allocate a new input operand */

	if (asp->n.nopsi >= ((f) ? MACHSTATE_MAXOPSI : MACHSTATE_MAXOPSO))
	    return SR_OVERFLOW ;

	if (f) {

	    i = asp->n.nopsi++ ;
	    op = &asp->n.iops[i] ;

	} else {

	    i = asp->n.nopso++ ;
	    op = &asp->n.oops[i] ;

	}

/* load it up */

	setop(op,OPERAND_TMEM,a) ;

	op->pv = pv << (a & 15) ;
	op->size = size ;
	op->dp = mkdatapresent(((uint) (a & 15)),size) ;

	if (op->dp & 0xff00)
	    asp->n.f.memunalign = TRUE ;

	if (! f)
	    op->v = v << (a & 15) ;

	asp->n.f.nmem += 1 ;
	return 0 ;
}
/* end subroutine (setasmem) */


/* set a previous memory value in an AS */
static int setasmemread(asp,a,size,pv)
SSAS	*asp ;
ULONG	a ;
int	size ;
ULONG	pv ;
{
	OPERAND	*op ;

	int	i ;


#if	F_TRACE
	bprintf(&g.tfile,"mem a=%016llx size=%d pv=%016llx\n",
	    a,size,pv) ;
#endif

/* allocate a new input operand */

	if (asp->n.nopsi >= MACHSTATE_MAXOPSI)
	    return SR_OVERFLOW ;

	i = asp->n.nopsi++ ;
	op = &asp->n.iops[i] ;

/* load it up */

	setop(op,OPERAND_TMEM,a) ;

	op->pv = pv << (a & 15) ;
	op->size = size ;
	op->dp = mkdatapresent(((uint) (a & 15)),size) ;

	return 0 ;
}
/* end subroutine (setasmemread) */


static int setop(op,type,oa)
OPERAND	*op ;
uint	type ;
ULONG	oa ;
{


	op->type = type ;

	op->f_v = TRUE ;
	op->f_used = TRUE ;
	op->f_export = FALSE ;

	op->a = oa ;
	op->pv = 0 ;
	op->v = 0 ;

	return 0 ;
}
/* end subroutine (setop) */


/* set the previous value of an operand register */
static int setopreg(op,opname)
OPERAND	*op ;
int	opname ;
{
	int	rs ;


	rs = getregval(&regs,opname,&op->pv) ;

	return rs ;
}
/* end subroutine (setopreg) */


/* set a value into a register in a register file */
int setregval(rp,opname,v)
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
/* end subroutine (setregval) */


/* get a register value for a register out of a register file */
int getregval(rp,opname,vp)
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
/* end subroutine (getregval) */


static int mkdatapresent(a,size)
uint	a ;
int	size ;
{
	int	i, dp ;


	i = ((a & 7) << 4) | (size & 15) ;
	dp = (int) dps[i] ;
	return dp ;
}
/* end subroutine (mkdatapresent) */


static int g_asdump(gp,asp)
struct ss	*gp ;
SSAS		*asp ;
{
	OPERAND	*op ;

	int	i ;


	bprintf(&gp->tfile,"ia=%16llx\n",
	    asp->n.ia) ;

	bprintf(&gp->tfile,"op=%d class=%d flags=%06x\n",
	    asp->n.op,asp->n.class,asp->n.flags) ;

	bprintf(&gp->tfile,"nio=%d noo=%d\n",
	    asp->n.nopsi,asp->n.nopso) ;

	for (i = 0 ; i < asp->n.nopsi ; i += 1) {

	    op = &asp->n.iops[i] ;
	    bprintf(&gp->tfile,
	        "IOP i=%d t=%u a=%16llx pv=%016llx\n",
	        i,op->type,op->a,op->pv) ;

	    if (op->type == OPERAND_TMEM)
	        bprintf(&gp->tfile,
	            "	size=%d dp=%02x\n",
	            op->size,op->dp) ;

	} /* end for */

	for (i = 0 ; i < asp->n.nopso ; i += 1) {

	    op = &asp->n.oops[i] ;
	    bprintf(&gp->tfile,
	        "OOP i=%d t=%u a=%16llx pv=%016llx v=%016llx\n",
	        i,op->type,op->a,op->pv,op->v) ;

	    if (op->type == OPERAND_TMEM)
	        bprintf(&gp->tfile,
	            "	size=%d dp=%02x\n",
	            op->size,op->dp) ;

	} /* end for */


	return 0 ;
}
/* end subroutine (g_asdump) */



