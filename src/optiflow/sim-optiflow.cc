/* sim-optiflow SUPPORT */
/* lang=C++98 */

/* sim-test.c - sample functional simulator implementation */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* debug print-outs */
#define	CF_DEBUG	0		/* switchable debug print-outs */
#define	CF_TRACE	0		/* debug trace */
#define	CF_RJMEM	1		/* right justify memory operands */
#define	CF_BPEVAL	0		/* 'bpeval(3d)' */
#define	CF_CACHEWARM	1		/* warm up caches */
#define	CF_BPREDWARM	1		/* warm up bpredictor */
#define	CF_FASTERFAST	1		/* faster fast */
#define	CF_BPRED	1		/* enable branch predictor */
#define	CF_DENSITY	1		/* use 'density(3d)' */
#define	CF_DENSITYUP	1		/* update the density */
#define	CF_CHARSTAT	0		/* extra stats?? */
#define	CF_WRITEDONE	0		/* close off writes for stats? */
#define	CF_ADJINSTR	1		/* adjust instruction points */

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

  	Name:
	sim-optiflow

	Description:
	This is the main code module for implementing the OpTiFlow
	simulator.  This file is loosely modeled after similar files
	that implement simulators such as 'sim-outorder' and
	'sim-mase'.  Most of the gooko (SimpleScalar interfacing)
	subroutines are purposely put into this file to try to keep
	them (gooko subroutines) in the same place.

	Important note:
	Do NOT turn on the "define" option above named CF_WRITEDONE!
	It really messed up the data!

*******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<cmath>
#include	<cassert>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstdio>
#include	<cstring>
#include	<mkpathx.h>
#include	<mkfnamex.h>
#include	<strwcpy.h>
#include	<bio.h>
#include	<sbuf.h>
#include	<keyopt.h>
#include	<paramfile.h>
#include	<field.h>
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
#include "cache.h"
#include "bpred.h"
#include	"iflags.h"
#include	"itype.h"

#ifdef	COMMENT
#include "resource.h"
#include "bitmap.h"
#include "eval.h"
#include "ptrace.h"
#endif /* COMMENT */

/* from 'main' */

#include	"density.h"
#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#include	"ss.h"
#include	"ssinfo.h"
#include	"sscommon.h"
#include	"checker.h"
#include	"ssdecode.h"
#include	"ssiw.h"
#include	"ssas.h"
#include	"ssnames.h"
#include	"bpeval.h"
#include	"bpresult.h"



/* local defines (not Simple-Scalar) */

#define	INSTRDISLEN	100		/* buffer to hold disassembly */
#define	BPSIZE		100		/* instructions in basic block */
#define	BTSIZE		10000		/* instructions in basic block */

#define	FNE_SGINM	".sginm"
#define	FNE_ICOUNTS	".eticounts"
#define	FNE_FCOUNTS	".etfcounts"
#define	FNE_BPLEN	".etbplen"
#define	FNE_BFTLEN	".etbftlen"	/* forward target length */
#define	FNE_BBTLEN	".etbbtlen"	/* backward target length */
#define	FNE_HTLEN	".ethtlen"
#define	FNE_BSTATS	".etstats"
#define	FNE_SSH		".ssh"

#define	FNE_REGRINT	".rrint"
#define	FNE_REGLIFE	".rlife"
#define	FNE_REGUSE	".ruse"
#define	FNE_REGREAD	".rread"
#define	FNE_REGWRITE	".rwrite"

#define	FNE_MEMRINT	".mrint"
#define	FNE_MEMLIFE	".mlife"
#define	FNE_MEMUSE	".muse"
#define	FNE_DUMP	".dump"
#define	FNE_IFLAGS0	".iflags0"
#define	FNE_IFLAGS1	".iflags1"

#define	FNE_ICACHE	".icache"
#define	FNE_DCACHE	".dcache"

#define	FNE_IT1		".it1"
#define	FNE_IT2		".it2"
#define	FNE_IT3		".it3"

#define	NSSH		1000		/* expected (?) number SSH */

#define	DEFROWS		1		/* default rows */
#define	DEFDELAY	512		/* default delay */

#define	MEMLATSCREW	400		/* maximum memlat screw value */

#define	SETOPI(asp,oi,opname,type)		\
	if (opname != DNA) {			\
	                    \
	    opp = &casp->n.opsi[oi++] ;	\
	    setop(opp,type,(ULONG) opname) ;	\
	                    \
	    if (type == OPERAND_TREG)	\
	        setopreg(opp,opname) ;	\
	                    \
	}

#define	SETOPO(asp,oi,opname,type)		\
	if (opname != DNA) {			\
	                    \
	    opp = &casp->n.opso[oi++] ;	\
	    setop(opp,type,(ULONG) opname) ;	\
	                    \
	    if (type == OPERAND_TREG)	\
	        setopreg(opp,opname) ;	\
	                    \
	}

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#if defined (TARGET_PISA)
#define IACOMPRESS(A)							\
  (compress_icache_addrs ? ((((A) - ld_text_base) >> 1) + ld_text_base) : (A))
#define ISCOMPRESS(SZ)							\
  (compress_icache_addrs ? ((SZ) >> 1) : (SZ))
#else /* !TARGET_PISA */
#define IACOMPRESS(A)		(A)
#define ISCOMPRESS(SZ)		(SZ)
#endif /* TARGET_PISA */



/* external subroutines */

extern double	percentll(ULONG,ULONG) ;

extern uint	nextpowtwo(uint) ;

extern int	mkfnamesuf(char *,cchar *,cchar *) ;
extern int	sfsub(cchar *,int,cchar *,char **) ;
extern int	cfdeci(cchar *,int,int *) ;
extern int	cfdecui(cchar *,int,int *) ;
extern int	cfdecull(cchar *,int,ULONG *) ;
extern int	cfdecmfull(cchar *,int,ULONG *) ;
extern int	cfnumui(cchar *,int,uint *) ;
extern int	densitystatll(ULONG *,int,double *,double *) ;
extern int	procsimpoint(struct proginfo *,LONG *) ;

extern char	*strnchr(cchar *,int,int) ;
extern char	*strsimpoint(char *,ULONG) ;


/* external variables */

extern struct proginfo	pi ;

extern int	sim_exit_now ;
extern int	sim_longjmp ;


/* local structures */

struct params {
	uint	rows ;
	uint	delay ;
	uint	f_confidence ;
	uint	vp_rows ;
	uint	vp_delay ;
	uint	vp_entries ;
	uint	vp_stride ;
	uint	vp_nops ;
	uint	yags_rows ;
	uint	yags_delay ;
	uint	yags_cpht ;
	uint	yags_dpht ;
	uint	tourna_rows ;
	uint	tourna_delay ;
	uint	tourna_lbht ;
	uint	tourna_lpht ;
	uint	tourna_gpht ;
	uint	gspag_rows ;
	uint	gspag_delay ;
	uint	gspag_lbht ;
	uint	gspag_gpht ;
	uint	gskew_rows ;
	uint	gskew_delay ;
	uint	gskew_tlen ;
} ;

struct instrinfo {
	ULONG	ia ;
	uint	f_cfcond : 1 ;
	uint	f_taken : 1 ;
} ;

struct stats {
	ULONG	in ;			/* number of instructions */
	ULONG	cf ;			/* control-flow-change instructions */
	ULONG	cf_ind ;		/* indirect CFC */
	ULONG	br_rel ;		/* relative branches */
	ULONG	br_con ;		/* conditional branches */
	ULONG	br_fwd ;		/* forward conditional branches */
	ULONG	br_ssh ;		/* single-sided simple hammocks */
	ULONG	iclass[iclass_overlast + 1] ;	/* i-classes */
	ULONG	bplen[BPSIZE] ;		/* instructions per BP */
	ULONG	bftlen[BTSIZE] ;	/* branch forward target length */
	ULONG	bbtlen[BTSIZE] ;	/* branch backward target length */
	ULONG	htlen[BTSIZE] ;		/* SS hammock branch target length */
	ULONG	ia_bad ;		/* bad IA from execution */
	ULONG	vp_inlu ;
	ULONG	vp_inhit ;		/* value-prediction lookup hit */
	ULONG	vp_oplu ;		/* operand lookups */
	ULONG	vp_ophit ;		/* operand lookup hits */
	ULONG	vp_opcor ;		/* correct operand lookups */
	ULONG	vp_allcor ;		/* all operands correct */
	ULONG	yags_correct ;		/* YAGS predicted correctly */
	ULONG	yags_bits ;		/* YAGS bits */
	ULONG	tourna_correct ;	/* TOURNA predicted correctly */
	ULONG	tourna_bits ;		/* TOURNA bits */
	ULONG	gspag_correct ;		/* GSPAG predicted correctly */
	ULONG	gspag_bits ;		/* GSPAG bits */
	ULONG	gskew_correct ;		/* GSKEW predicted correctly */
	ULONG	gskew_bits ;		/* GSKEW bits */
} ;


/* forward references */

static unsigned int			/* total latency of access */
mem_access_latency(int) ;		/* block size accessed */

static unsigned int			/* latency of block access */
dl1_access_fn(enum mem_cmd ,		/* access cmd, Read or Write */
md_addr_t ,		/* block address to access */
int ,		/* size of block to access */
struct cache_blk_t *,	/* ptr to block in upper level */
tick_t ) ;		/* time of access */

static unsigned int			/* latency of block access */
dl2_access_fn(enum mem_cmd ,		/* access cmd, Read or Write */
md_addr_t ,		/* block address to access */
int ,		/* size of block to access */
struct cache_blk_t *,	/* ptr to block in upper level */
tick_t ) ;		/* time of access */

static unsigned int			/* latency of block access */
il1_access_fn(enum mem_cmd ,		/* access cmd, Read or Write */
md_addr_t ,		/* block address to access */
int ,		/* size of block to access */
struct cache_blk_t *,	/* ptr to block in upper level */
tick_t ) ;		/* time of access */

static unsigned int			/* latency of block access */
il2_access_fn(enum mem_cmd ,		/* access cmd, Read or Write */
md_addr_t ,		/* block address to access */
int ,		/* size of block to access */
struct cache_blk_t *,	/* ptr to block in upper level */
tick_t ) ;		/* time of access */

static unsigned int			/* latency of block access */
itlb_access_fn(enum mem_cmd ,	/* access cmd, Read or Write */
md_addr_t ,		/* block address to access */
int ,		/* size of block to access */
struct cache_blk_t *,	/* ptr to block in upper level */
tick_t ) ;		/* time of access */

static unsigned int	
dtlb_access_fn(enum mem_cmd ,	/* access cmd, Read or Write */
md_addr_t ,	/* block address to access */
int ,		/* size of block to access */
struct cache_blk_t *,	/* ptr to block in upper level */
tick_t ) ;		/* time of access */

int	sim_checkavail(PROGINFO *) ;
int	sim_checkexec(PROGINFO *,int) ;
int	sim_checkrelease(PROGINFO *,int) ;
int	sim_checkreleaser(PROGINFO *,int) ;

int	instr_display(uint,char *,int) ;

static int	sim_fast(PROGINFO *,ULONG,ULONG) ;
static int	sim_faster(PROGINFO *,ULONG,ULONG) ;
static int	sim_checktional(PROGINFO *) ;
static int	sim_functional(PROGINFO *) ;

static int	sim_asdump(PROGINFO *,SSAS *) ;
static int	sim_tracedump(struct proginfo *,cchar *) ;
static int	sim_genstats(struct proginfo *,cchar *) ;
static int	sim_regstats(struct proginfo *,cchar *) ;
static int	sim_memstats(struct proginfo *,cchar *) ;

static int	setas(SSAS *,int,int,int,int,int,int,int) ;
static int	setasreg(SSAS *) ;
static int	setasmem(SSAS *,int,ULONG,int,ULONG,ULONG) ;
static int	setasmemread(SSAS *,ULONG,int,ULONG) ;
static int	setop(OPERAND *,uint,ULONG) ;
static int	setopreg(OPERAND *,int) ;
static int	mkdatapresent(uint,int) ;

static int	peparse(struct proginfo *,cchar *,int,int *,int *) ;
static int	leparse(struct proginfo *,struct ssinfo *,cchar *,int) ;


/* local variables */

/* simulated registers */
static struct regs_t regs ;

/* simulated memory */
static struct mem_t *mem = NULL ;

/* track number of refs */
static counter_t sim_num_refs = 0 ;

/* maximum number of inst's to execute */
#ifdef	COMMENT
static unsigned int max_insts ;
#else
static counter_t max_insts ;
#endif /* COMMENT */

/* number of instructions to fast forward */
#ifdef	COMMENT
static unsigned int fastfwd_count ;
#else
static counter_t fastfwd_count ;
#endif /* COMMENT */

/* speed of front-end of machine relative to execution core */
static int fetch_speed ;

/* optimistic misfetch recovery */
static int mf_compat ;

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
static char *pred_type ;

/* bimodal predictor config (<table_size>) */
static int bimod_nelt = 1 ;
static int bimod_config[1] =
{ /* bimod tbl size */
	    2048 } ;

/* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
static int twolev_nelt = 4 ;
static int twolev_config[4] =
{ /* l1size */
	    1, /* l2size */1024, /* hist */8, /* xor */FALSE} ;

/* combining predictor config (<meta_table_size> */
static int comb_nelt = 1 ;
static int comb_config[1] =
{ /* meta_table_size */
	    1024 } ;

/* return address stack (RAS) size */
static int ras_size = 8 ;

/* BTB predictor config (<num_sets> <associativity>) */
static int btb_nelt = 2 ;
static int btb_config[2] =
{ /* nsets */
	    512, /* assoc */4 } ;

/* perfect prediction enabled */
static int pred_perfect = FALSE ;

/* speculative bpred-update enabled */
static char *bpred_spec_opt ;
static enum {
	spec_ID, spec_WB, spec_CT 

} bpred_spec_update ;

/* level 1 instruction cache, entry level instruction cache */
static struct cache_t *cache_il1 ;

/* level 1 instruction cache */
static struct cache_t *cache_il2 ;

/* level 1 data cache, entry level data cache */
static struct cache_t *cache_dl1 ;

/* level 2 data cache */
static struct cache_t *cache_dl2 ;

/* instruction TLB */
static struct cache_t *itlb ;

/* data TLB */
static struct cache_t *dtlb ;

/* branch predictor */
static struct bpred_t *pred ;

/* perfect memory disambiguation */
int perfect_disambig = FALSE ;

/* l1 data cache config, i.e., {<config>|none} */
static char *cache_dl1_opt ;

/* l1 data cache hit latency (in cycles) */
static int cache_dl1_lat ;

/* l2 data cache config, i.e., {<config>|none} */
static char *cache_dl2_opt ;

/* l2 data cache hit latency (in cycles) */
static int cache_dl2_lat ;

/* l1 instruction cache config, i.e., {<config>|dl1|dl2|none} */
static char *cache_il1_opt ;

/* l1 instruction cache hit latency (in cycles) */
static int cache_il1_lat ;

/* l2 instruction cache config, i.e., {<config>|dl1|dl2|none} */
static char *cache_il2_opt ;

/* l2 instruction cache hit latency (in cycles) */
static int cache_il2_lat ;

/* flush caches on system calls */
static int flush_on_syscalls ;

/* convert 64-bit inst addresses to 32-bit inst equivalents */
static int compress_icache_addrs ;

/* memory access latency (<first_chunk> <inter_chunk>) */
static int mem_nelt = 2 ;
static int mem_lat[2] =
{ /* lat to first chunk */
	    18, /* lat between remaining chunks */2 } ;

/* memory access bus width (in bytes) */
static int mem_bus_width ;

/* instruction TLB config, i.e., {<config>|none} */
static char *itlb_opt ;

/* data TLB config, i.e., {<config>|none} */
static char *dtlb_opt ;

/* inst/data TLB miss latency (in cycles) */
static int tlb_miss_lat ;


/* add-on simulator state */
static SS		g ;

struct stats		charstat ;

/* this is a database for setting "data-present" bits for memory OPs */
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

static cchar	*ssparams[] = {
	"modulus",
	"nbpred",
	"fqsize",
	"iwsize",
	"fwsize",
	"fwidth",
	"dwsize",
	"dwidth",
	"iwidth",
	"rwidth",
	"fbuses",
	"pe_none",
	"pe_ialu",
	"pe_imult",
	"pe_idiv",
	"pe_fadd",
	"pe_fcmp",
	"pe_fcvt",
	"pe_fmult",
	"pe_fdiv",
	"pe_fsqrt",
	"pe_ld",
	"pe_st",
	"sinstr",
	"winstr",
	"ninstr",
	"linstr",
	"nomem",
	"execover",
	"hangcks",
	"flushdiv",
	"memdatalat",
	"perfpred",
	"iflags",
	"itype",
	"itrace",
	"localexec",
	"intervals",		/* instruction range for access intervals */
	"checkonly",
	"regstats",
	"memstats",
	"regdenlen",
	"memdenlen",
	"memelemsize",
	NULL
} ;

enum ssparams {
	ssparam_modulus,	/* sequence number modulus */
	ssparam_nbpred,		/* number of branch predictions */
	ssparam_fqsize,		/* fetch queue size */
	ssparam_iwsize,		/* number of ISes */
	ssparam_fwsize,		/* fetch width */
	ssparam_fwidth,		/* fetch width */
	ssparam_dwsize,		/* dispatch width */
	ssparam_dwidth,		/* dispatch width */
	ssparam_iwidth,		/* FU issue width */
	ssparam_rwidth,		/* FU result width */
	ssparam_fbuses,		/* forwarding buses */
	ssparam_penone,
	ssparam_peialu,
	ssparam_peimult,
	ssparam_peidiv,
	ssparam_pefadd,
	ssparam_pefcmp,
	ssparam_pefcvt,
	ssparam_pefmult,
	ssparam_pefdiv,
	ssparam_pefsqrt,
	ssparam_peld,
	ssparam_pest,
	ssparam_sinstr,		/* number of instructions to skip */
	ssparam_winstr,		/* warm-up instructions */
	ssparam_ninstr,		/* number of instructions to execute */
	ssparam_linstr,		/* minimum instructions between logs */
	ssparam_nomem,
	ssparam_execover,
	ssparam_hangcks,
	ssparam_flushdiv,
	ssparam_memdatalat,
	ssparam_perfpred,
	ssparam_iflags,
	ssparam_itype,
	ssparam_itrace,
	ssparam_localexec,
	ssparam_intervals,
	ssparam_checkonly,
	ssparam_regstats,
	ssparam_memstats,
	ssparam_regdenlen,
	ssparam_memdenlen,
	ssparam_memelemsize,
	ssparam_overlast
} ;

static cchar	*progopts[] = {
	"rows",
	"delay",
	"confidence",
	"vpred:rows",
	"vpred:delay",
	"vpred:entries",
	"vpred:stride",
	"vpred:nops",
	"yags:rows",
	"yags:delay",
	"yags:cpht",
	"yags:dpht",
	"tourna:rows",
	"tourna:delay",
	"tourna:lbht",
	"tourna:lpht",
	"tourna:gpht",
	"gspag:rows",
	"gspag:delay",
	"gspag:lbht",
	"gspag:gpht",
	"gskew:rows",
	"gskew:delay",
	"gskew:tlen",
	NULL
} ;

enum progopts {
	progopt_rows,
	progopt_delay,
	progopt_confidence,
	progopt_vprows,
	progopt_vpdelay,
	progopt_vpentries,
	progopt_vpstride,
	progopt_vpnops,
	progopt_yagsrows,
	progopt_yagsdelay,
	progopt_yagscpht,
	progopt_yagsdpht,
	progopt_tournarows,
	progopt_tournadelay,
	progopt_tournalbht,
	progopt_tournalpht,
	progopt_tournagpht,
	progopt_gspagrows,
	progopt_gspagdelay,
	progopt_gspaglbht,
	progopt_gspaggpht,
	progopt_gskewrows,
	progopt_gskewdelay,
	progopt_gskewtlen,
	progopt_overlast
} ;

/* default function unit latencies */
static const uint	lfu_default[] = {
	1, 1, 7, 12, 4, 4, 3, 4, 12, 18, 1, 1,
} ;







/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{


	opt_reg_header(odb, 
	"sim-test: This simulator implements a functional simulator.  This\n"
	    "functional simulator is designed for researching a Levo-like \n"
	    "microarchitecture.\n"
	    ) ;

/* fast forward */
#ifdef	COMMENT
	opt_reg_int(odb, "-fastfwd", 
	    "number of insts skipped before timing starts",
	    &fastfwd_count, /* default */0,
	    /* print */TRUE, /* format */NULL) ;
#else
	opt_reg_sqword(odb, "-fastfwd", 
	    "number of insts skipped before timing starts",
	    &fastfwd_count, /* default */0,
	    /* print */TRUE, /* format */NULL) ;
#endif /* COMMENT */

/* instruction limit */
#ifdef	COMMENT
	opt_reg_uint(odb, "-max:inst", 
	    "maximum number of inst's to execute",
	    &max_insts, /* default */0,
	    /* print */TRUE, /* format */NULL) ;
#else
	opt_reg_sqword(odb, "-max:inst", 
	    "maximum number of inst's to execute",
	    &max_insts, /* default */0,
	    /* print */TRUE, /* format */NULL) ;
#endif /* COMMENT */

/* ifetch options */

	opt_reg_int(odb, "-fetch:speed",
	    "speed of front-end of machine relative to execution core",
	    &fetch_speed, /* default */1,
	    /* print */TRUE, /* format */NULL) ;

	opt_reg_flag(odb, "-fetch:mf_compat", "optimistic misfetch recovery",
	    &mf_compat, /* default */FALSE,
	    /* print */TRUE, /* format */NULL) ;

/* branch predictor options */

	opt_reg_note(odb,
	    "  Branch predictor configuration examples for 2-level predictor:\n"
	    "    Configurations:   N, M, W, X\n"
	    "      N   # entries in first level (# of shift register(s))\n"
	    "      W   width of shift register(s)\n"
	    "      M   # entries in 2nd level (# of counters, or other FSM)\n"
	    "      X   (yes-1/no-0) xor history and address for 2nd level index\n"
	    "    Sample predictors:\n"
	    "      GAg     : 1, W, 2^W, 0\n"
	    "      GAp     : 1, W, M (M > 2^W), 0\n"
	    "      PAg     : N, W, 2^W, 0\n"
	    "      PAp     : N, W, M (M == 2^(N+W)), 0\n"
	    "      gshare  : 1, W, 2^W, 1\n"
	    "  Predictor `comb' combines a bimodal and a 2-level predictor.\n"
	    ) ;

	opt_reg_string(odb, "-bpred",
	    "branch predictor type {nottaken|taken|perfect|bimod|2lev|comb}",
	    &pred_type, /* default */"bimod",
	    /* print */TRUE, /* format */NULL) ;

	opt_reg_int_list(odb, "-bpred:bimod",
	    "bimodal predictor config (<table size>)",
	    bimod_config, bimod_nelt, &bimod_nelt,
	    /* default */bimod_config,
	    /* print */TRUE, /* format */NULL, /* !accrue */FALSE) ;

	opt_reg_int_list(odb, "-bpred:2lev",
	    "2-level predictor config "
	    "(<l1size> <l2size> <hist_size> <xor>)",
	    twolev_config, twolev_nelt, &twolev_nelt,
	    /* default */twolev_config,
	    /* print */TRUE, /* format */NULL, /* !accrue */FALSE) ;

	opt_reg_int_list(odb, "-bpred:comb",
	    "combining predictor config (<meta_table_size>)",
	    comb_config, comb_nelt, &comb_nelt,
	    /* default */comb_config,
	    /* print */TRUE, /* format */NULL, /* !accrue */FALSE) ;

	opt_reg_int(odb, "-bpred:ras",
	    "return address stack size (0 for no return stack)",
	    &ras_size, /* default */ras_size,
	    /* print */TRUE, /* format */NULL) ;

	opt_reg_int_list(odb, "-bpred:btb",
	    "BTB config (<num_sets> <associativity>)",
	    btb_config, btb_nelt, &btb_nelt,
	    /* default */btb_config,
	    /* print */TRUE, /* format */NULL, /* !accrue */FALSE) ;

	opt_reg_string(odb, "-bpred:spec_update",
	    "speculative predictors update in {ID|WB} (default non-spec)",
	    &bpred_spec_opt, /* default */NULL,
	    /* print */TRUE, /* format */NULL) ;

/* LSQ options */

	opt_reg_flag(odb, "-lsq:perfect",
	    "perfect memory disambiguation",
	    &perfect_disambig, /* default */FALSE, /* print */TRUE, NULL) ;

/* cache options */

	opt_reg_string(odb, "-cache:dl1",
	    "l1 data cache config, i.e., {<config>|none}",
	    &cache_dl1_opt, "dl1:128:32:4:l",
	    /* print */TRUE, NULL) ;

	opt_reg_note(odb,
	    "  The cache config parameter <config> has the following format:\n"
	    "\n"
	    "    <name>:<nsets>:<bsize>:<assoc>:<repl>\n"
	    "\n"
	    "    <name>   - name of the cache being defined\n"
	    "    <nsets>  - number of sets in the cache\n"
	    "    <bsize>  - block size of the cache\n"
	    "    <assoc>  - associativity of the cache\n"
	    "    <repl>   - block replacement strategy, 'l'-LRU, 'f'-FIFO, 'r'-random\n"
	    "\n"
	    "    Examples:   -cache:dl1 dl1:4096:32:1:l\n"
	    "                -dtlb dtlb:128:4096:32:r\n"
	    ) ;

	opt_reg_int(odb, "-cache:dl1lat",
	    "l1 data cache hit latency (in cycles)",
	    &cache_dl1_lat, /* default */1,
	    /* print */TRUE, /* format */NULL) ;

	opt_reg_string(odb, "-cache:dl2",
	    "l2 data cache config, i.e., {<config>|none}",
	    &cache_dl2_opt, "ul2:1024:64:4:l",
	    /* print */TRUE, NULL) ;

	opt_reg_int(odb, "-cache:dl2lat",
	    "l2 data cache hit latency (in cycles)",
	    &cache_dl2_lat, /* default */6,
	    /* print */TRUE, /* format */NULL) ;

	opt_reg_string(odb, "-cache:il1",
	    "l1 inst cache config, i.e., {<config>|dl1|dl2|none}",
	    &cache_il1_opt, "il1:512:32:1:l",
	    /* print */TRUE, NULL) ;

	opt_reg_note(odb,
	    "  Cache levels can be unified by pointing a level of the instruction cache\n"
	    "  hierarchy at the data cache hiearchy using the \"dl1\" and \"dl2\" cache\n"
	    "  configuration arguments.  Most sensible combinations are supported, e.g.,\n"
	    "\n"
	    "    A unified l2 cache (il2 is pointed at dl2):\n"
	    "      -cache:il1 il1:128:64:1:l -cache:il2 dl2\n"
	    "      -cache:dl1 dl1:256:32:1:l -cache:dl2 ul2:1024:64:2:l\n"
	    "\n"
	    "    Or, a fully unified cache hierarchy (il1 pointed at dl1):\n"
	    "      -cache:il1 dl1\n"
	    "      -cache:dl1 ul1:256:32:1:l -cache:dl2 ul2:1024:64:2:l\n"
	    ) ;

	opt_reg_int(odb, "-cache:il1lat",
	    "l1 instruction cache hit latency (in cycles)",
	    &cache_il1_lat, /* default */1,
	    /* print */TRUE, /* format */NULL) ;

	opt_reg_string(odb, "-cache:il2",
	    "l2 instruction cache config, i.e., {<config>|dl2|none}",
	    &cache_il2_opt, "dl2",
	    /* print */TRUE, NULL) ;

	opt_reg_int(odb, "-cache:il2lat",
	    "l2 instruction cache hit latency (in cycles)",
	    &cache_il2_lat, /* default */6,
	    /* print */TRUE, /* format */NULL) ;

	opt_reg_flag(odb, "-cache:flush", "flush caches on system calls",
	    &flush_on_syscalls, /* default */FALSE, /* print */TRUE, NULL) ;

	opt_reg_flag(odb, "-cache:icompress",
	    "convert 64-bit inst addresses to 32-bit inst equivalents",
	    &compress_icache_addrs, /* default */FALSE,
	    /* print */TRUE, NULL) ;

/* mem options */

	opt_reg_int_list(odb, "-mem:lat",
	    "memory access latency (<first_chunk> <inter_chunk>)",
	    mem_lat, mem_nelt, &mem_nelt, mem_lat,
	    /* print */TRUE, /* format */NULL, /* !accrue */FALSE) ;

	opt_reg_int(odb, "-mem:width", "memory access bus width (in bytes)",
	    &mem_bus_width, /* default */8,
	    /* print */TRUE, /* format */NULL) ;

/* TLB options */

	opt_reg_string(odb, "-tlb:itlb",
	    "instruction TLB config, i.e., {<config>|none}",
	    &itlb_opt, "itlb:16:4096:4:l", /* print */TRUE, NULL) ;

	opt_reg_string(odb, "-tlb:dtlb",
	    "data TLB config, i.e., {<config>|none}",
	    &dtlb_opt, "dtlb:32:4096:4:l", /* print */TRUE, NULL) ;

	opt_reg_int(odb, "-tlb:lat",
	    "inst/data TLB miss latency (in cycles)",
	    &tlb_miss_lat, /* default */30,
	    /* print */TRUE, /* format */NULL) ;


}
/* end subroutine (sim_reg_options) */


/* check simulator-specific option values */
void sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
	char name[128], c ;
	int nsets, bsize, assoc ;


#ifdef	COMMENT
	if (fastfwd_count < 0 || fastfwd_count >= INT_MAX)
	    fatal("bad fast forward count: %llu", fastfwd_count) ;
#endif

	if (fetch_speed < 1)
	    fatal("front-end speed must be positive and non-zero") ;

	if (!mystricmp(pred_type, "perfect"))
	{
/* perfect predictor */
	    pred = NULL ;
	    pred_perfect = TRUE ;
	}
	else if (!mystricmp(pred_type, "taken"))
	{
/* static predictor, not taken */
	    pred = bpred_create(BPredTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0) ;
	}
	else if (!mystricmp(pred_type, "nottaken"))
	{
/* static predictor, taken */
	    pred = bpred_create(BPredNotTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0) ;
	}
	else if (!mystricmp(pred_type, "bimod"))
	{
/* bimodal predictor, bpred_create() checks BTB_SIZE */
	    if (bimod_nelt != 1)
	        fatal("bad bimod predictor config (<table_size>)") ;
	    if (btb_nelt != 2)
	        fatal("bad btb config (<num_sets> <associativity>)") ;

/* bimodal predictor, bpred_create() checks BTB_SIZE */
	    pred = bpred_create(BPred2bit,
	        /* bimod table size */bimod_config[0],
	        /* 2lev l1 size */0,
	        /* 2lev l2 size */0,
	        /* meta table size */0,
	        /* history reg size */0,
	        /* history xor address */0,
	        /* btb sets */btb_config[0],
	        /* btb assoc */btb_config[1],
	        /* ret-addr stack size */ras_size) ;
	}
	else if (!mystricmp(pred_type, "2lev"))
	{
/* 2-level adaptive predictor, bpred_create() checks args */
	    if (twolev_nelt != 4)
	        fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)") ;
	    if (btb_nelt != 2)
	        fatal("bad btb config (<num_sets> <associativity>)") ;

	    pred = bpred_create(BPred2Level,
	        /* bimod table size */0,
	        /* 2lev l1 size */twolev_config[0],
	        /* 2lev l2 size */twolev_config[1],
	        /* meta table size */0,
	        /* history reg size */twolev_config[2],
	        /* history xor address */twolev_config[3],
	        /* btb sets */btb_config[0],
	        /* btb assoc */btb_config[1],
	        /* ret-addr stack size */ras_size) ;
	}
	else if (!mystricmp(pred_type, "comb"))
	{
/* combining predictor, bpred_create() checks args */
	    if (twolev_nelt != 4)
	        fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)") ;
	    if (bimod_nelt != 1)
	        fatal("bad bimod predictor config (<table_size>)") ;
	    if (comb_nelt != 1)
	        fatal("bad combining predictor config (<meta_table_size>)") ;
	    if (btb_nelt != 2)
	        fatal("bad btb config (<num_sets> <associativity>)") ;

	    pred = bpred_create(BPredComb,
	        /* bimod table size */bimod_config[0],
	        /* l1 size */twolev_config[0],
	        /* l2 size */twolev_config[1],
	        /* meta table size */comb_config[0],
	        /* history reg size */twolev_config[2],
	        /* history xor address */twolev_config[3],
	        /* btb sets */btb_config[0],
	        /* btb assoc */btb_config[1],
	        /* ret-addr stack size */ras_size) ;
	}
	else
	    fatal("cannot parse predictor type `%s'", pred_type) ;

	if (!bpred_spec_opt)
	    bpred_spec_update = spec_CT ;
	else if (!mystricmp(bpred_spec_opt, "ID"))
	    bpred_spec_update = spec_ID ;
	else if (!mystricmp(bpred_spec_opt, "WB"))
	    bpred_spec_update = spec_WB ;
	    else
	    fatal("bad speculative update stage specifier, use {ID|WB}") ;


/* use a level 1 D-cache? */
	if (!mystricmp(cache_dl1_opt, "none"))
	{
	    cache_dl1 = NULL ;

/* the level 2 D-cache cannot be defined */
	    if (strcmp(cache_dl2_opt, "none"))
	        fatal("the l1 data cache must defined if the l2 cache is defined") ;
	    cache_dl2 = NULL ;
	}
	else /* dl1 is defined */
	{
	    if (sscanf(cache_dl1_opt, "%[^:]:%d:%d:%d:%c",
	        name, &nsets, &bsize, &assoc, &c) != 5)
	        fatal("bad l1 D-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>") ;
	    cache_dl1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
	        /* usize */0, assoc, cache_char2policy(c),
	        dl1_access_fn, /* hit lat */cache_dl1_lat) ;

/* is the level 2 D-cache defined? */
	    if (!mystricmp(cache_dl2_opt, "none"))
	        cache_dl2 = NULL ;
	        else
	    {
	        if (sscanf(cache_dl2_opt, "%[^:]:%d:%d:%d:%c",
	            name, &nsets, &bsize, &assoc, &c) != 5)
	            fatal("bad l2 D-cache parms: "
	                "<name>:<nsets>:<bsize>:<assoc>:<repl>") ;
	        cache_dl2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
	            /* usize */0, assoc, cache_char2policy(c),
	            dl2_access_fn, /* hit lat */cache_dl2_lat) ;
	    }
	}

/* use a level 1 I-cache? */
	if (!mystricmp(cache_il1_opt, "none"))
	{
	    cache_il1 = NULL ;

/* the level 2 I-cache cannot be defined */
	    if (strcmp(cache_il2_opt, "none"))
	        fatal("the l1 inst cache must defined if the l2 cache is defined") ;
	    cache_il2 = NULL ;
	}
	else if (!mystricmp(cache_il1_opt, "dl1"))
	{
	    if (!cache_dl1)
	        fatal("I-cache l1 cannot access D-cache l1 as it's undefined") ;
	    cache_il1 = cache_dl1 ;

/* the level 2 I-cache cannot be defined */
	    if (strcmp(cache_il2_opt, "none"))
	        fatal("the l1 inst cache must defined if the l2 cache is defined") ;
	    cache_il2 = NULL ;
	}
	else if (!mystricmp(cache_il1_opt, "dl2"))
	{
	    if (!cache_dl2)
	        fatal("I-cache l1 cannot access D-cache l2 as it's undefined") ;
	    cache_il1 = cache_dl2 ;

/* the level 2 I-cache cannot be defined */
	    if (strcmp(cache_il2_opt, "none"))
	        fatal("the l1 inst cache must defined if the l2 cache is defined") ;
	    cache_il2 = NULL ;
	}
	else /* il1 is defined */
	{
	    if (sscanf(cache_il1_opt, "%[^:]:%d:%d:%d:%c",
	        name, &nsets, &bsize, &assoc, &c) != 5)
	        fatal("bad l1 I-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>") ;
	    cache_il1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
	        /* usize */0, assoc, cache_char2policy(c),
	        il1_access_fn, /* hit lat */cache_il1_lat) ;

/* is the level 2 D-cache defined? */
	    if (!mystricmp(cache_il2_opt, "none"))
	        cache_il2 = NULL ;
	    else if (!mystricmp(cache_il2_opt, "dl2"))
	    {
	        if (!cache_dl2)
	            fatal("I-cache l2 cannot access D-cache l2 as it's undefined") ;
	        cache_il2 = cache_dl2 ;
	    }
	    else
	    {
	        if (sscanf(cache_il2_opt, "%[^:]:%d:%d:%d:%c",
	            name, &nsets, &bsize, &assoc, &c) != 5)
	            fatal("bad l2 I-cache parms: "
	                "<name>:<nsets>:<bsize>:<assoc>:<repl>") ;
	        cache_il2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
	            /* usize */0, assoc, cache_char2policy(c),
	            il2_access_fn, /* hit lat */cache_il2_lat) ;
	    }
	}

/* use an I-TLB? */
	if (!mystricmp(itlb_opt, "none"))
	    itlb = NULL ;
	    else
	{
	    if (sscanf(itlb_opt, "%[^:]:%d:%d:%d:%c",
	        name, &nsets, &bsize, &assoc, &c) != 5)
	        fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>") ;
	    itlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
	        /* usize */sizeof(md_addr_t), assoc,
	        cache_char2policy(c), itlb_access_fn,
	        /* hit latency */1) ;
	}

/* use a D-TLB? */
	if (!mystricmp(dtlb_opt, "none"))
	    dtlb = NULL ;
	    else
	{
	    if (sscanf(dtlb_opt, "%[^:]:%d:%d:%d:%c",
	        name, &nsets, &bsize, &assoc, &c) != 5)
	        fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>") ;
	    dtlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
	        /* usize */sizeof(md_addr_t), assoc,
	        cache_char2policy(c), dtlb_access_fn,
	        /* hit latency */1) ;
	}

	if (cache_dl1_lat < 1)
	    fatal("l1 data cache latency must be greater than zero") ;

	if (cache_dl2_lat < 1)
	    fatal("l2 data cache latency must be greater than zero") ;

	if (cache_il1_lat < 1)
	    fatal("l1 instruction cache latency must be greater than zero") ;

	if (cache_il2_lat < 1)
	    fatal("l2 instruction cache latency must be greater than zero") ;

	if (mem_nelt != 2)
	    fatal("bad memory access latency (<first_chunk> <inter_chunk>)") ;

	if (mem_lat[0] < 1 || mem_lat[1] < 1)
	    fatal("all memory access latencies must be greater than zero") ;

	if (mem_bus_width < 1 || (mem_bus_width & (mem_bus_width-1)) != 0)
	    fatal("memory bus width must be positive non-zero and a power of two") ;

	if (tlb_miss_lat < 1)
	    fatal("TLB miss latency must be greater than zero") ;

#ifdef	COMMENT

	if (res_ialu < 1)
	    fatal("number of integer ALU's must be greater than zero") ;
	if (res_ialu > MAX_INSTS_PER_CLASS)
	    fatal("number of integer ALU's must be <= MAX_INSTS_PER_CLASS") ;
	fu_config[FU_IALU_INDEX].quantity = res_ialu ;

	if (res_imult < 1)
	    fatal("number of integer multiplier/dividers must be greater than zero") ;
	if (res_imult > MAX_INSTS_PER_CLASS)
	    fatal("number of integer mult/div's must be <= MAX_INSTS_PER_CLASS") ;
	fu_config[FU_IMULT_INDEX].quantity = res_imult ;

	if (res_memport < 1)
	    fatal("number of memory system ports must be greater than zero") ;
	if (res_memport > MAX_INSTS_PER_CLASS)
	    fatal("number of memory system ports must be <= MAX_INSTS_PER_CLASS") ;
	fu_config[FU_MEMPORT_INDEX].quantity = res_memport ;

	if (res_fpalu < 1)
	    fatal("number of floating point ALU's must be greater than zero") ;
	if (res_fpalu > MAX_INSTS_PER_CLASS)
	    fatal("number of floating point ALU's must be <= MAX_INSTS_PER_CLASS") ;
	fu_config[FU_FPALU_INDEX].quantity = res_fpalu ;

	if (res_fpmult < 1)
	    fatal("number of floating point multiplier/dividers must be > zero") ;
	if (res_fpmult > MAX_INSTS_PER_CLASS)
	    fatal("number of FP mult/div's must be <= MAX_INSTS_PER_CLASS") ;
	fu_config[FU_FPMULT_INDEX].quantity = res_fpmult ;

#endif /* COMMENT */

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

/* branch predictor */

	if (pred)
	    bpred_reg_stats(pred, sdb) ;

/* register cache stats */
	if (cache_il1
	    && (cache_il1 != cache_dl1 && cache_il1 != cache_dl2))
	    cache_reg_stats(cache_il1, sdb) ;
	if (cache_il2
	    && (cache_il2 != cache_dl1 && cache_il2 != cache_dl2))
	    cache_reg_stats(cache_il2, sdb) ;
	if (cache_dl1)
	    cache_reg_stats(cache_dl1, sdb) ;
	if (cache_dl2)
	    cache_reg_stats(cache_dl2, sdb) ;
	if (itlb)
	    cache_reg_stats(itlb, sdb) ;
	if (dtlb)
	    cache_reg_stats(dtlb, sdb) ;

/* continue with original stuff */

	ld_reg_stats(sdb) ;
	mem_reg_stats(mem, sdb) ;
}
/* end subroutine (sim_reg_stats) */


/* initialize the simulator */
int sim_init(void)
{
	struct proginfo	*pip = &pi ;

	SSINFO	*lip = &pip->info ;

	CHECKER	*csp = &pip->check ;

	bfile	sfile ;

	ULONG	sinstr, winstr, ninstr, linstr ;

	uint	size ;

	int	rs, rs1, i ;
	int	sl, cl ;

	char	tmpfname[MAXPATHLEN + 1] ;
	char	digbuf[DIGBUFLEN + 1] ;
	char	*sp, *cp ;


	sinstr = ULONG_UNSET ;
	winstr = ULONG_UNSET ;
	ninstr = ULONG_UNSET ;
	linstr = ULONG_UNSET ;


/* initialize the simulator support object */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_init: nclock=%llu\n",pip->mclock) ;
#endif

	rs = ssinfo_init(lip) ;

	if (rs < 0)
	    goto bad0 ;

/* continue */

	sim_num_refs = 0 ;

/* allocate and initialize register file */
	regs_init(&regs) ;

/* allocate and initialize memory space */
	mem = mem_create("mem") ;
	mem_init(mem) ;


/* initialize some statistics stuff */

	memset(&charstat,0,sizeof(struct stats)) ;

	bopen(&g.tfile,"trace","wct",0666) ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_init: cache stats\n") ;
#endif

	pip->s.imemlatlen = MAX(IMEMLATLEN,100) ;
	size = pip->s.imemlatlen * sizeof(ULONG) ;
	rs = uc_malloc(size,&pip->s.imemlat) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_init: i-cache uc_malloc() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad1a ;

	memset(pip->s.imemlat,0,size) ;


	pip->s.dmemlatlen = MAX(DMEMLATLEN,100) ;
	size = pip->s.dmemlatlen * sizeof(ULONG) ;
	rs = uc_malloc(size,&pip->s.dmemlat) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_init: d-cache uc_malloc() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad2 ;

	memset(pip->s.dmemlat,0,size) ;


	density_init(&pip->s.imlat, pip->s.imemlatlen) ;

	density_init(&pip->s.dmlat, pip->s.dmemlatlen) ;


/* initialze the parameters */


/* default latencies of function units */

	for (i = 0 ; i < iclass_overlast ; i += 1) {
	    lip->nfu[i] = 1 ;
	    lip->lfu[i] = lfu_default[i] ;
	} /* end for */


/* set some default options */

	pip->f.memdatalat = TRUE ;


/* read parameters that we are interested in */

	if (pip->f.params) {

	    PARAMFILE_CURSOR	cur ;

	    ULONG	ulv ;

	    uint	uv ;

	    int		vl, n, latency ;

	    char	vbuf[VBUFLEN + 1] ;


	    for (i = 0 ; ssparams[i] != NULL ; i += 1) {

		paramfile_cursorinit(&pip->pf,&cur) ;

		while (TRUE) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_init: fetch ssparam=%s\n",ssparams[i]) ;
#endif

	            rs1 = paramfile_fetch(&pip->pf,ssparams[i],&cur,
			vbuf,VBUFLEN) ;

		    vl = rs1 ;
		    if (rs1 < 0)
			break ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_init: paramfile_fetch() rs=%d\n",vl) ;
#endif

	        if (vl >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (DEBUGLEVEL(4)) {
	                eprintf("sim_init: vl=%d vbuf=%w \n",
	                    vl,vbuf,
				strnlen(((vl >= 0) ? vbuf : ""),MAX(vl,0))) ;
	            }
#endif /* CF_DEBUG */

	            switch (i) {

	            case ssparam_modulus:
	            case ssparam_fwsize:
	            case ssparam_fwidth:
	            case ssparam_nbpred:
	            case ssparam_fqsize:
	            case ssparam_dwsize:
	            case ssparam_dwidth:
	            case ssparam_iwsize:
	            case ssparam_iwidth:
	            case ssparam_rwidth:
	            case ssparam_fbuses:
	            case ssparam_hangcks:
	            case ssparam_flushdiv:
		    case ssparam_regdenlen:
		    case ssparam_memdenlen:
		    case ssparam_memelemsize:
	                if ((vl > 0) &&
	                    (cfnumui(vbuf,vl,&uv) >= 0)) {

	                    switch (i) {

	                    case ssparam_modulus:
	                        lip->modulus = uv ;
	                        break ;

	                    case ssparam_nbpred:
	                        lip->nbpred = uv ;
	                        break ;

	                    case ssparam_fqsize:
	                        lip->fqsize = uv ;
	                        break ;

	                    case ssparam_iwsize:
	                        lip->iwsize = uv ;
	                        break ;

	                    case ssparam_fwsize:
	                    case ssparam_fwidth:
	                        lip->fwidth = uv ;
	                        break ;

	                    case ssparam_dwsize:
	                    case ssparam_dwidth:
	                        lip->dwidth = uv ;
	                        break ;

	                    case ssparam_iwidth:
	                        lip->iwidth = uv ;
	                        break ;

	                    case ssparam_rwidth:
	                        lip->rwidth = uv ;
	                        break ;

	                    case ssparam_fbuses:
	                        lip->fbuses = uv ;
	                        break ;

	                    case ssparam_hangcks:
	                        lip->hangcks = uv ;
	                        break ;

	                    case ssparam_flushdiv:
	                        lip->flushdiv = uv ;
	                        break ;

	                    case ssparam_regdenlen:
	                        lip->regdenlen = uv ;
	                        break ;

	                    case ssparam_memdenlen:
	                        lip->memdenlen = uv ;
	                        break ;

	                    case ssparam_memelemsize:
	                        lip->memelemsize = uv ;
	                        break ;

	                    } /* end switch */

	                } /* end if (got a number) */

	                break ;

	            case ssparam_peialu:
	            case ssparam_peimult:
	            case ssparam_peidiv:
	            case ssparam_pefadd:
	            case ssparam_pefcmp:
	            case ssparam_pefcvt:
	            case ssparam_pefmult:
	            case ssparam_pefdiv:
	            case ssparam_pefsqrt:
	            case ssparam_peld:
	            case ssparam_pest:
	                if ((vl > 0) &&
	                    (peparse(pip,vbuf,vl,&n,&latency) > 0)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	                    if (DEBUGLEVEL(4))
	                        eprintf("sim_test: peparse() "
	                            "uv=%u latency=%d\n",
	                            n,latency) ;
#endif

	                    switch (i) {

	                    case ssparam_penone:
	                        lip->nfu[iclass_none] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_none] = latency ;

	                        break ;

	                    case ssparam_peialu:
	                        lip->nfu[iclass_ialu] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_ialu] = latency ;

	                        break ;

	                    case ssparam_peimult:
	                        lip->nfu[iclass_imult] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_imult] = latency ;

	                        break ;

	                    case ssparam_peidiv:
	                        lip->nfu[iclass_idiv] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_idiv] = latency ;

	                        break ;

	                    case ssparam_pefadd:
	                        lip->nfu[iclass_fadd] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_fadd] = latency ;

	                        break ;

	                    case ssparam_pefcmp:
	                        lip->nfu[iclass_fcmp] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_fcmp] = latency ;

	                        break ;

	                    case ssparam_pefcvt:
	                        lip->nfu[iclass_fcvt] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_fcvt] = latency ;

	                        break ;

	                    case ssparam_pefmult:
	                        lip->nfu[iclass_fmult] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_fmult] = latency ;

	                        break ;

	                    case ssparam_pefdiv:
	                        lip->nfu[iclass_fdiv] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_fdiv] = latency ;

	                        break ;

	                    case ssparam_pefsqrt:
	                        lip->nfu[iclass_fsqrt] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_fsqrt] = latency ;

	                        break ;

	                    case ssparam_peld:
	                        lip->nfu[iclass_ld] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_ld] = latency ;

	                        break ;

	                    case ssparam_pest:
	                        lip->nfu[iclass_st] = MAX(n,1) ;
	                        if (latency >= 0)
	                            lip->lfu[iclass_st] = latency ;

	                        break ;

	                    } /* end switch */

	                } /* end if */

	                break ;

	            case ssparam_localexec:
	                if (vl > 0) 
	                    leparse(pip,lip,vbuf,vl) ;

			break ;

	            case ssparam_sinstr:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sim-optiflow: param sinstr=%w\n",vbuf,vl) ;
#endif

			cl = MIN(MAXPATHLEN,vl) ;
			sp = tmpfname ;
			sl = strwcpylc(tmpfname,vbuf,cl) - sp ;

			if ((sfsub(sp,sl,"simpoint",NULL) >= 0) &&
				(vl == 8)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sim-optiflow: got a SIMPOINT\n") ;
#endif

				sinstr = ULONG_SIMPOINT ;
				break ;
			}

/* else FALL-THROUGH */
	            case ssparam_winstr:
	            case ssparam_ninstr:
	            case ssparam_linstr:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sim-optiflow: ssparam %s=%w\n",
		ssparams[i],
		vbuf,vl) ;
#endif

	                if ((vl > 0) &&
	                    (cfdecull(vbuf,vl,&ulv) >= 0)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("sim-optiflow: v=%llu\n",ulv) ;
#endif

	                    switch (i) {

	                    case ssparam_sinstr:
	                        sinstr = ulv ;
	                        break ;

	                    case ssparam_winstr:
	                        winstr = ulv ;
	                        break ;

	                    case ssparam_ninstr:
	                        ninstr = ulv ;
	                        break ;

	                    case ssparam_linstr:
	                        linstr = ulv ;
	                        break ;

	                    } /* end switch */

	                } /* end if (got a number) */

	                break ;

	            case ssparam_nomem:
	            case ssparam_execover:
	            case ssparam_memdatalat:
	            case ssparam_perfpred:
	            case ssparam_iflags:
	            case ssparam_itype:
	            case ssparam_itrace:
		    case ssparam_regstats:
		    case ssparam_memstats:
		    case ssparam_checkonly:
	                rs1 = SR_INVALID ;
	                if (vl > 0) {

	                    rs1 = cfnumui(vbuf,vl,&uv) ;

	                    if (rs1 >= 0) {

	                        if (uv != 0)
	                            uv = TRUE ;

	                    }
	                }

	                switch (i) {

	                case ssparam_nomem:
	                    pip->f.nomem = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_execover:
	                    pip->f.execover = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_memdatalat:
	                    pip->f.memdatalat = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_perfpred:
	                    pip->f.perfpred = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_iflags:
	                    pip->f.iflags = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_itype:
	                    pip->f.itype = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_itrace:
	                    pip->f.itrace = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_regstats:
	                    pip->f.regstats = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_memstats:
	                    pip->f.memstats = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                case ssparam_checkonly:
	                    pip->f.checkonly = (rs1 >= 0) ? uv : TRUE ;
	                    break ;

	                } /* end switch */

	                break ;

	            } /* end switch (outer) */

	        } /* end if (got a good parameter) */

		} /* end while */

		paramfile_cursorfree(&pip->pf,&cur) ;

	    } /* end for (getting the geometry-like parameters) */

	} /* end if (reading parameters) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_init: check 0\n") ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	    eprintf("sim_init: after 0 ssparam ninstr=%s\n",
		strsimpoint(digbuf,ninstr)) ;
	}
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	    for (i = 0 ; i < iclass_overlast ; i += 1) {

	        eprintf("sim_init: nfu[%i]=%d lfu[%u]=%d\n",
	            i,lip->nfu[i],i,lip->lfu[i]) ;

	    }
	}
#endif /* CF_DEBUG */

/* do we need to find a SIMPOINT */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("sim_init: have SIMPOINT?\n") ;
#endif

	if (sinstr == ULONG_SIMPOINT) {

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("sim_init: proc SIMPOINT\n") ;
#endif

	    rs = procsimpoint(pip,&sinstr) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2)) {
	    eprintf("sim_init: procsimpoint() rs=%d\n",rs) ;
	    eprintf("sim_init: sinstr=%s\n",
		strsimpoint(digbuf,sinstr)) ;
	}
#endif /* CF_DEBUG */

	    if (rs < 0) {

		bprintf(pip->efp,
		    "%s: could not process SIMPOINT (%d)\n",
			pip->progname,rs) ;
			
		goto bad3 ;
	    }

	} /* end if (had a SIMPOINT) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_init: check 1\n") ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4)) {
	    eprintf("sim_init: before ssparam sinstr=%s\n",
		strsimpoint(digbuf,pip->sinstr)) ;
	    eprintf("sim_init: before ssparam ninstr=%s\n",
		strsimpoint(digbuf,pip->ninstr)) ;
	    eprintf("sim_init: after 1 ssparam sinstr=%s\n",
		strsimpoint(digbuf,sinstr)) ;
	    eprintf("sim_init: after 1 ssparam ninstr=%s\n",
		strsimpoint(digbuf,ninstr)) ;
	}
#endif /* CF_DEBUG */


/* define some default parameters */

	if (lip->modulus <= 4)
	    lip->modulus = nextpowtwo(SSINFO_MODULUS) ;

	lip->rfmod = nextpowtwo(lip->modulus) ;

	lip->rbmod = nextpowtwo(lip->modulus) ;

/* check the parameters */

/* fetch width */
#ifdef	COMMRNT /* zero means infinite */
	if (lip->fwidth <= 0)
	    lip->fwidth = SSINFO_FWIDTH ;
#endif

/* fetch Q size */
	if (lip->fqsize <= 1)
	    lip->fqsize = SSINFO_FQSIZE ;

	if ((lip->fwidth > 0) && (lip->fqsize < lip->fwidth))
		lip->fqsize = lip->fwidth ;

/* dispatch width */
#ifdef	COMMENT /* zero means infinite */
	if (lip->dwidth <= 1)
	    lip->dwidth = SSINFO_DWIDTH ;
#endif

/* instruction window size (number if ISes) */
	if (lip->iwsize <= 0)
	    lip->iwsize = SSINFO_IWSIZE ;

/* FU issue width (default is total number of FUs) */
#ifdef	COMMENT /* zero means total number of FUs */
	if (lip->iwidth <= 0)
	    lip->iwidth = lip->nfu[iclass_ialu] ;
#endif

/* FU result width */
#ifdef	COMMENT /* zero means infinite */
	if (lip->rwidth <= 0)
	    lip->rwidth = lip->iwidth ;
#endif

/* forwarding buses */
#ifdef	COMMRNT /* zero means infinite */
	if (lip->fbuses <= 0)
	    lip->fbuses = SSINFO_FBUSES ;
#endif

	if (lip->hangcks <= HANGCKS)
	    lip->hangcks = HANGCKS ;

	if (lip->flushdiv < 1)
	    lip->flushdiv = 1 ;

	if (lip->regdenlen < 1)
		lip->regdenlen = (64 * 1024) ;

	if (lip->memdenlen < 1)
		lip->memdenlen = (256 * 1024) ;

	if (lip->memelemsize < 1)
		lip->memelemsize = 8 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_init: check 2\n") ;
#endif

	if ((pip->sinstr == ULONG_UNSET) && (sinstr != ULONG_UNSET))
	    pip->sinstr = sinstr ;

	if ((pip->winstr == ULONG_UNSET) && (winstr != ULONG_UNSET))
	    pip->winstr = winstr ;

	if ((pip->ninstr == ULONG_UNSET) && (ninstr != ULONG_UNSET))
	    pip->ninstr = ninstr ;

	if ((pip->linstr == ULONG_UNSET) && (linstr != ULONG_UNSET))
	    pip->linstr = linstr ;

	if (pip->sinstr == ULONG_UNSET)
	    pip->sinstr = 0 ;

	if (pip->winstr == ULONG_UNSET)
	    pip->winstr = 0 ;

	if (pip->ninstr == ULONG_UNSET)
	    pip->ninstr = 0 ;

	if (pip->linstr == ULONG_UNSET)
	    pip->linstr = 0 ;

/* adjust the instruction points */

#if	CF_ADJINSTR
	if (pip->winstr > 0) {

		if (pip->winstr > pip->sinstr)
			pip->sinstr = pip->winstr ;

	}
#endif /* CF_ADJINSTR */

/* calculate something */

	pip->minstr = 0 ;
	if (pip->ninstr > 0)
	    pip->minstr = pip->sinstr + pip->ninstr ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_init: check 3\n") ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2)) {
	    eprintf("sim_init: sinstr=%s\n",
		strsimpoint(digbuf,pip->sinstr)) ;
	    eprintf("sim_init: winstr=%s\n",
		strsimpoint(digbuf,pip->winstr)) ;
	    eprintf("sim_init: ninstr=%s\n",
		strsimpoint(digbuf,pip->ninstr)) ;
	    eprintf("sim_init: linstr=%s\n",
		strsimpoint(digbuf,pip->linstr)) ;
	    eprintf("sim_init: minstr=%s\n",
		strsimpoint(digbuf,pip->minstr)) ;
	}
#endif /* CF_DEBUG */


/* do some miscellaneous calculations */

	lip->rfmodmask = (lip->rfmod - 1) ;
	lip->rbmodmask = (lip->rbmod - 1) ;


	assert(lip->nfu[iclass_st] < 10000) ;

/* handle other components for the base machine */

#if	CF_BPEVAL
	{
	    char	loaddname[MAXPATHLEN + 1] ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sim_init: BPEVAL\n") ;
#endif

	    cp = LOADDNAME ;
	    if (cp[0] != '/') {

	        cp = loaddname ;
	        mkfname(loaddname,pip->pr,LOADDNAME) ;

	    }

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sim_init: loaddname=%s\n",cp) ;
#endif

	    rs = bpeval_init(&pip->bp,cp,1,2) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sim_init: bpeval_init() rs=%d\n",rs) ;
#endif

	    pip->f.bpeval = (rs >= 0) ;

	    if (pip->f.bpeval) {

	        rs1 = bpeval_add(&pip->bp,"gspag","gspag",
	            2028,2048,1024,0) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_init: bpeval_add() rs=%d\n",rs1) ;
#endif

	    } /* end if (adding modules) */

	}
#endif /* CF_BPEVAL */

/* register statistsics */

	if (pip->f.regstats) {

	    int	lentrack ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sim_init: regstats\n") ;
#endif

	    lentrack = (sizeof(struct regs_t) + 1) / sizeof(int) ;
	    rs = regstats_init(&pip->rstats,lentrack,lip->regdenlen) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sim_init: regstats_init() rs=%d\n",rs) ;
#endif

	} /* end if (regstats) */

	    if (rs < 0)
	        goto bad4 ;

/* memory stastics */

	if (pip->f.memstats) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sim_init: opt_memstats\n") ;
#endif

	    mkfname(tmpfname,pip->jobname,".memtrack") ;

	    rs = memstats_init(&pip->mstats,tmpfname,-1,-1,
		lip->memelemsize,lip->memdenlen) ;

	    if (rs < 0)
	        goto bad5 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("sim_init: memstats_init() rs=%d\n",rs) ;
#endif

	} /* end if (memstats) */

	    if (rs < 0)
	        goto bad5 ;

/* return */
ret0:
	return rs ;

/* bad stuff comes here */
bad6:
	if (pip->f.memstats)
		memstats_free(&pip->mstats) ;

bad5:
	if (pip->f.regstats)
		regstats_free(&pip->rstats) ;

bad4:
bad3a:
bad3:
	if (pip->s.dmemlat != NULL)
	    free(pip->s.dmemlat) ;

bad2:
	if (pip->s.imemlat != NULL)
	    free(pip->s.imemlat) ;

bad1a:
bad1:
	ssinfo_free(lip) ;

bad0:
	pip->f.exit = TRUE ;
	goto ret0 ;
}
/* end subroutine (sim_init) */


/* load program into simulated state */

/* program to load */
/* program arguments */
/* program environment */

void sim_load_prog(char *fname,int argc, char **argv,char **envp)
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



/* start simulation, program loaded, processor precise state initialized */
int sim_main(void)
{
	struct proginfo	*pip = &pi ;

	SSINFO	*lip = &pip->info ;

	CHECKER	*csp = &pip->check ;

	ITYPE	it ;

	bfile	it1, it2, it3 ;
	bfile	sfile ;

	uint	n ;

	int	rs, rs1, i, j ;
	int	f_sfile ;

	char	tmpfname[MAXPATHLEN + 1] ;
	char	digbuf[DIGBUFLEN + 1] ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4)) {
	        eprintf("sim_init: entered\n") ;
	    eprintf("sim_init: sinstr=%s\n",
		strsimpoint(digbuf,pip->sinstr)) ;
	    eprintf("sim_init: winstr=%s\n",
		strsimpoint(digbuf,pip->winstr)) ;
	    eprintf("sim_init: ninstr=%s\n",
		strsimpoint(digbuf,pip->ninstr)) ;
	    eprintf("sim_init: linstr=%s\n",
		strsimpoint(digbuf,pip->linstr)) ;
	    eprintf("sim_init: minstr=%s\n",
		strsimpoint(digbuf,pip->minstr)) ;
	}
#endif /* CF_DEBUG */

	rs = SR_OK ;

/* set exit mode for simulator when/if it reaches an 'exit(2)' syscall */

	sim_longjmp = FALSE ;

/* print out what we know so far */

	rs1 = SR_NOENT ;
	if (pip->statsfname[0] != '\0')
	    rs1 = bopen(&sfile,pip->statsfname,"wa",0666) ;

	f_sfile = (rs1 >= 0) ;

	if (f_sfile) {

	    bprintf(&sfile,"= configuration information\n") ;

	    bprintf(&sfile,"sinstr=     %s\n",
		strsimpoint(digbuf,pip->sinstr)) ;

	    bprintf(&sfile,"winstr=     %s\n",
		strsimpoint(digbuf,pip->winstr)) ;

	    bprintf(&sfile,"ninstr=     %s\n",
		strsimpoint(digbuf,pip->ninstr)) ;

	    bprintf(&sfile,"checkonly=  %u\n",pip->f.checkonly) ;

	    bprintf(&sfile,"nomem=      %u\n",pip->f.nomem) ;

	    bprintf(&sfile,"execover=   %u\n",pip->f.execover) ;

	    bprintf(&sfile,"regstats=   %u\n",pip->f.regstats) ;

	    bprintf(&sfile,"memstats=   %u\n",pip->f.memstats) ;

	    bprintf(&sfile,"iflags=     %u\n",pip->f.iflags) ;

	    bprintf(&sfile,"memdatalat= %u\n",pip->f.memdatalat) ;

	    bprintf(&sfile,"hangcks=    %u\n",lip->hangcks) ;

	    bprintf(&sfile,"flushdiv=   %u\n",lip->flushdiv) ;

	    bprintf(&sfile,"fwidth=     %u\n",lip->fwidth) ;

	    bprintf(&sfile,"fqsize=     %u\n",lip->fqsize) ;

	    bprintf(&sfile,"dwidth=     %u\n",lip->dwidth) ;

	    bprintf(&sfile,"iwsize=     %u\n",lip->iwsize) ;

	    bprintf(&sfile,"iwidth=     %u\n",lip->iwidth) ;

	    bprintf(&sfile,"rwidth=     %u\n",lip->rwidth) ;

	    bprintf(&sfile,"fbuses=     %u\n",lip->fbuses) ;

	    bprintf(&sfile,"\n") ;

	    bprintf(&sfile,"FU               N         L\n") ;

	    for (i = 0 ; i < iclass_overlast ; i += 1) {

	        if (lip->nfu[i] != INT_MAX)
	            bprintf(&sfile,"%-8s %9u %9u\n",
	                ssfunames[i],lip->nfu[i],lip->lfu[i]) ;

	        else
	            bprintf(&sfile,"%-8s %9s %9u\n",
	                ssfunames[i],"max",lip->lfu[i]) ;

	    } /* end for */

	    bprintf(&sfile,"\n") ;

	    bclose(&sfile) ;

	} /* end if (configuration data) */

/* set up initial default next PC */
	regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t) ;

/* check for DLite debugger entry condition */
	if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
	    dlite_main(regs.regs_PC - sizeof(md_inst_t),
	        regs.regs_PC, sim_num_insn, &regs, mem) ;

/* fast forward the execution */

	fprintf(stderr, "sim: ** starting fastfwd simulationn **\n") ;

/* fast forward (for the most part) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("sim_main: fastfwd_count=%llu\n",fastfwd_count) ;
#endif

	if ((pip->sinstr > 0) || (pip->winstr > 0)) {

	    if (pip->sinstr > 0)
	        fprintf(stderr,"sim: skipping instructions=%llu\n",
		pip->sinstr) ;

	    if (pip->winstr > 0)
	        fprintf(stderr,"sim: warming  instructions=%llu\n",
		pip->winstr) ;

	    rs = sim_faster(pip,pip->sinstr,pip->winstr) ;

#if	CF_BPREDWARM
	    if ((rs >= 0) && (pred != NULL)) {

	        ULONG	ulw ;


	        fprintf(stderr,"sim: prediction stats on warm-up\n") ;

	        fprintf(stderr,"ins=         %10llu (A)\n",pip->winstr) ;
	        ulw = pip->winstr ;
	        fprintf(stderr,"inscf=       %10llu (%6.1f%% CF/A)\n",
	            pip->s.bp_inscf,
	            percentll(pip->s.bp_inscf,ulw)) ;
	        fprintf(stderr,"taken=       %10llu (%6.1f%% T/CF)\n",
	            pip->s.bp_taken,
	            percentll(pip->s.bp_taken,pip->s.bp_inscf)) ;

#ifdef	COMMENT
	        fprintf(stderr,"predgiven=   %10llu (%6.1f%% P/CF)\n",
	            pip->s.bp_predgiven,
	            percentll(pip->s.bp_predgiven,pip->s.bp_inscf)) ;
#endif

	        fprintf(stderr,"predtaken=   %10llu (%6.1f%% PT/CF)\n",
	            pip->s.bp_predtaken,
	            percentll(pip->s.bp_predtaken,pip->s.bp_inscf)) ;

#ifdef	COMMENT
	        fprintf(stderr,"predcorrect= %10llu (%6.1f%% PC/P)\n",
	            pip->s.bp_predcorrect,
	            percentll(pip->s.bp_predcorrect,pip->s.bp_predgiven)) ;
#endif

	        fprintf(stderr,"predcorrect= %10llu (%6.1f%% PC/CF)\n",
	            pip->s.bp_predcorrect,
	            percentll(pip->s.bp_predcorrect,pip->s.bp_inscf)) ;

	        pip->s.bp_inscf = 0 ;
	        pip->s.bp_taken = 0 ;
	        pip->s.bp_predgiven = 0 ;
	        pip->s.bp_predtaken = 0 ;
	        pip->s.bp_predcorrect = 0 ;

	    } /* end if */
#endif /* CF_BPREDWARM */

	} /* end if (fast forwarding) */

	if (rs > 0) {
		pip->f.exit_target = TRUE ;
		goto ret3 ;
	}

	pip->s.ck_start = pip->sinstr ;
	pip->s.in_start = pip->sinstr ;

/* OK, prepare two instruction trace files */

	pip->it1 = NULL ;
	pip->it2 = NULL ;
	pip->it3 = NULL ;
	if (pip->f.itrace) {

	    mkfnamesuf(tmpfname,pip->jobname,FNE_IT1) ;

	    rs1 = bopen(&it1,tmpfname,"wct",0666) ;

            pip->it1 = (rs1 >= 0) ? &it1 : NULL ;


	    mkfnamesuf(tmpfname,pip->jobname,FNE_IT2) ;

	    rs1 = bopen(&it2,tmpfname,"wct",0666) ;

            pip->it2 = (rs1 >= 0) ? &it2 : NULL ;


	    mkfnamesuf(tmpfname,pip->jobname,FNE_IT3) ;

	    rs1 = bopen(&it3,tmpfname,"wct",0666) ;

            pip->it3 = (rs1 >= 0) ? &it3 : NULL ;

	} /* end if */

/* determine checker window size */

	{
	    int	wsize = 10000 ;


	    wsize = ((lip->fqsize + lip->iwsize) * 4) ;
	    rs = checker_init(csp,mem,&regs,pip->sinstr,wsize) ;

	} /* end block */

	if (rs < 0)
		goto ret4 ;

/* instruction type statistics preparation */

	if (pip->f.itype) {

	    pip->itp = &it ;
	    rs = itype_init(&it) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_main: itype_init() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
		goto ret5 ;

	}

/* do performance simulation */

	if (pip->f.checkonly) {

	    fprintf(stderr, "sim: ** starting checking simulation **\n") ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_main: sim_checktional() in=%llu\n", csp->in) ;
#endif

	    rs = sim_checktional(pip) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_main: sim_checktional() rs=%d in=%llu\n", rs,csp->in) ;
#endif

	} else {

	    fprintf(stderr, "sim: ** starting functional simulation **\n") ;

	    rs = sim_functional(pip) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_main: sim_functional() rs=%d in=%llu\n", rs,csp->in) ;
#endif

	} /* end if */

#if	CF_WRITEDONE

	if (pip->f.regstats)
		regstats_writedone(&pip->rstats,csp->in,1) ;

	if (pip->f.memstats)
		memstats_writedone(&pip->mstats,csp->in,1) ;

#endif /* CF_WRITEDONE */

ret6:
	if (pip->f.itype)
		itype_free(&it) ;

ret5:
	checker_free(csp) ;

ret4:
	if (pip->it3 != NULL)
		bclose(pip->it3) ;

	if (pip->it2 != NULL)
		bclose(pip->it2) ;

	if (pip->it1 != NULL)
		bclose(pip->it1) ;

ret3:
	if (pip->f.exit_target)
	    fprintf(stderr, "sim: ** target program exited **\n") ;

	if (pip->f.exit_clock)
	    fprintf(stderr, "sim: ** clock expiration **\n") ;

	if (pip->f.exit_instr)
	    fprintf(stderr, "sim: ** instruction expiration **\n") ;

	return 0 ;
}
/* end subroutine (sim_main) */


/* un-initialize simulator-specific state */
int sim_uninit(void)
{
	struct proginfo	*pip = &pi ;

	SSINFO	*lip = &pip->info ;

	ULONG	ulw ;

	bfile	tmpfile ;

	double	mean, var ;

	uint	max ;

	int	rs, rs1, i, j ;

	char	tmpfname[MAXPATHLEN + 1] ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_uninit: entered\n") ;
#endif

/* print out some statistics */

#ifdef	COMMENT
	sim_tracedump(pip,"trace") ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_uninit: cache performance, statsfname=%s\n",
	        pip->statsfname) ;
#endif

	rs1 = SR_NOENT ;
	if (pip->statsfname[0] != '\0')
	    rs1 = bopen(&tmpfile,pip->statsfname,"wca",0666) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_uninit: statsfile bopen() rs=%d\n",rs1) ;
#endif

	if (rs1 >= 0) {

	    bfile	tfile ;


/* some cache-related AS information */

	    bprintf(&tmpfile,"= front-end information\n") ;

	    bprintf(&tmpfile,"ncneed=        %10llu\n",
	        pip->s.ncneed) ;

	    bprintf(&tmpfile,"ncadv=         %10llu\n",
	        pip->s.ncadv) ;

	    bprintf(&tmpfile,"idispatches=   %10llu\n",
	        pip->s.asloads) ;

	    bprintf(&tmpfile,"\n") ;

	    bprintf(&tmpfile,"= memory related IS information\n") ;

	    bprintf(&tmpfile,"is_memins=     %10llu\n",
	        pip->s.asmemins) ;

	    bprintf(&tmpfile,"is_memops=     %10llu\n",
	        pip->s.asmemops) ;

	    bprintf(&tmpfile,"is_memopreads= %10llu\n",
	        pip->s.asmemopreads) ;

	    bprintf(&tmpfile,"\n") ;

/* cache performance */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3))
	        eprintf("sim_uninit: i-cache density len=%u\n",
	            pip->s.imemlatlen) ;
#endif

	    bprintf(&tmpfile,"= cache performance\n") ;

	    bprintf(&tmpfile,"icache_reads=      %10llu\n",
	        pip->s.imemreads) ;

#if	CF_DENSITY
	{
		DENSITY_STATS	ds ;


		rs1 = density_stats(&pip->s.imlat,&ds) ;

		mean = ds.mean ;
		var = ds.var ;
		max = ds.max ;

	    bprintf(&tmpfile,"icache_density=    %d\n",rs1) ;

	    bprintf(&tmpfile,"icache_count=      %10llu\n",
		ds.count) ;

	    bprintf(&tmpfile,"icachelat_ovfs=    %10llu\n",
	        ds.ovf) ;

	}
#else /* CF_DENSITY */

	    rs1 = densitystatll(pip->s.imemlat,pip->s.imemlatlen,
	        &mean,&var) ;

		max = pip->s.imemlatmax ;

#endif /* CF_DENSITY */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3))
	        eprintf("sim_uninit: i-cache densitystatll() rs=%d\n",rs1) ;
#endif

	    bprintf(&tmpfile,"icachelat_mean=    %6.1f\n",
	        mean) ;

	    bprintf(&tmpfile,"icachelat_stddev=  %6.1f\n",
	        sqrt(var)) ;

	    bprintf(&tmpfile,"icachelat_max=     %4u\n",
	        max) ;

	    bprintf(&tmpfile,"icachelat_screws=  %4u\n",
	        pip->s.imemlatscrew) ;


	    mkfnamesuf(tmpfname,pip->jobname,FNE_ICACHE) ;

	    rs = bopen(&tfile,tmpfname,"wct",0666) ;

	    if (rs >= 0) {

#if	CF_DENSITY

		for (j = 0 ; density_slot(&pip->s.imlat,j,&ulw) >= 0 ; j += 1) {

	            bprintf(&tfile,"%8u %10llu\n",
	                j, ulw) ;

		}

#else /* CF_DENSITY */

	        for (j = 0 ; j < pip->s.imemlatlen ; j += 1) {

	            bprintf(&tfile,"%8u %10llu\n",
	                j, pip->s.imemlat[j]) ;

	        } /* end for */

#endif /* CF_DENSITY */

	        bclose(&tfile) ;

	    } /* end if (i-cache density) */

	    bprintf(&tmpfile,"dcache_reads=      %10llu\n",
	        pip->s.dmemreads) ;

	    bprintf(&tmpfile,"dcache_writes=     %10llu\n",
	        pip->s.dmemwrites) ;

#if	CF_DENSITY
	{
		DENSITY_STATS	ds ;


		rs1 = density_stats(&pip->s.dmlat,&ds) ;

		mean = ds.mean ;
		var = ds.var ;
		max = ds.max ;

	    bprintf(&tmpfile,"dcache_density=    %d\n",rs1) ;

	    bprintf(&tmpfile,"dcache_count=      %10llu\n",
		ds.count) ;

	    bprintf(&tmpfile,"dcachelat_ovfs=    %10llu\n",
	        ds.ovf) ;

	}
#else /* CF_DENSITY */

	    rs1 = densitystatll(pip->s.dmemlat,pip->s.dmemlatlen,
	        &mean,&var) ;

		max = pip->s.dmemlatmax ;

#endif /* CF_DENSITY */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3))
	        eprintf("sim_uninit: d-cache densitystatll() rs=%d\n",rs1) ;
#endif

	    bprintf(&tmpfile,"dcachelat_mean=    %6.1f\n",
	        mean) ;

	    bprintf(&tmpfile,"dcachelat_stddev=  %6.1f\n",
	        sqrt(var)) ;

	    bprintf(&tmpfile,"dcachelat_max=     %4u\n",
	        max) ;

	    bprintf(&tmpfile,"dcachelat_screws=  %4u\n",
	        pip->s.dmemlatscrew) ;

	    bprintf(&tmpfile,"\n") ;


	    mkfnamesuf(tmpfname,pip->jobname,FNE_DCACHE) ;

	    rs = bopen(&tfile,tmpfname,"wct",0666) ;

	    if (rs >= 0) {

#if	CF_DENSITY

		for (j = 0 ; density_slot(&pip->s.dmlat,j,&ulw) >= 0 ; j += 1) {

	            bprintf(&tfile,"%8u %10llu\n",
	                j, ulw) ;

		}

#else /* CF_DENSITY */

	        for (j = 0 ; j < pip->s.dmemlatlen ; j += 1) {

	            bprintf(&tfile,"%8u %10llu\n",
	                j, pip->s.dmemlat[j]) ;

	        } /* end for */

#endif /* CF_DENSITY */

	        bclose(&tfile) ;

	    } /* end if (d-cache density) */


/* branch predictor performance */

	    bprintf(&tmpfile,"= branch predictor performance\n") ;

	    bprintf(&tmpfile,"bp_inscf=       %10llu (%6.1f%% CF/A)\n",
	        pip->s.bp_inscf,
	        percentll(pip->s.bp_inscf,pip->s.ins)) ;

	    bprintf(&tmpfile,"bp_taken=       %10llu (%6.1f%% T/CF)\n",
	        pip->s.bp_taken,
	        percentll(pip->s.bp_taken,pip->s.bp_inscf)) ;

	    bprintf(&tmpfile,"bp_predtaken=   %10llu (%6.1f%% PT/CF)\n",
	        pip->s.bp_predtaken,
	        percentll(pip->s.bp_predtaken,pip->s.bp_inscf)) ;

	    bprintf(&tmpfile,"bp_predcorrect= %10llu (%6.1f%% PC/CF)\n",
	        pip->s.bp_predcorrect,
	        percentll(pip->s.bp_predcorrect,pip->s.bp_inscf)) ;

	    bprintf(&tmpfile,"\n") ;

	    bclose(&tmpfile) ;

	} /* end if (statistics file) */

/* get other specilized statistics */


/* register and memory statistics */

	if (pip->f.regstats || pip->f.memstats) {

	    char	statfname[MAXPATHLEN + 1] ;


	    mkfnamesuf(statfname,pip->jobname,FNE_BSTATS) ;

	    u_unlink(statfname) ;

	    sim_genstats(pip,statfname) ;

	    if (pip->f.regstats)
		sim_regstats(pip,statfname) ;

/* write out the memory statistics */

	    if (pip->f.memstats)
		sim_memstats(pip,statfname) ;

	} /* end if (register and memory statistics) */

/* branch prediction ? */


/* do the predictors that were dynamically loaded! */

#ifdef	COMMENT
	writestats() ;
#endif

/* free up eveything */


/* memory and register statistics */

	if (pip->f.memstats)
		memstats_free(&pip->mstats) ;

	if (pip->f.regstats)
		regstats_free(&pip->rstats) ;

/* breanch prediction */

#if	CF_BPEVAL
	if (pip->f.bpeval)
	    bpeval_free(&pip->bp) ;
#endif /* CF_BPEVAL */

	density_free(&pip->s.dmlat) ;

	density_free(&pip->s.imlat) ;

	if (pip->s.dmemlat != NULL)
	    free(pip->s.dmemlat) ;

	if (pip->s.imemlat != NULL)
	    free(pip->s.imemlat) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3))
	        eprintf("sim_uninit: ssinfo_free()\n") ;
#endif

	ssinfo_free(lip) ;

/* no return value here */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3))
	        eprintf("sim_uninit: ret\n") ;
#endif

	return 0 ;
}
/* end subroutine (sim_uninit) */


/* execute some checker instructions */
int sim_checkexec(pip,ninstr)
PROGINFO	*pip ;
int		ninstr ;
{
	CHECKER	*csp = &pip->check ;

	ITYPE	*itp = (ITYPE *) pip->itp ;

	SSAS	*cas = csp->cas ;
	SSAS	*casp ;

	uint	uiw ;

	int	rs1 ;
	int	n, ne, nc, ic ;
	int	i ;
	int	f_se ;


	ne = csp->wsize - csp->nw ;
	nc = MIN(ninstr,ne) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("sim_checkexec: sin=%llu nin=%llu win=%llu\n",
		pip->sinstr,pip->ninstr,pip->winstr) ;
	    eprintf("sim_checkexec: cin=%llu ninstr=%u nc=%u\n",
		csp->in,ninstr,nc) ;
	}
#endif

	for (ic = 0 ; ic < nc ; ic += 1) {

	    struct instrinfo	ii, bi ;

	    enum md_opcode opc ;

	    md_inst_t	inst ;

	    enum md_fault_type fault ;

	    md_addr_t	addr ;

		OPERAND	*opp ;

	    ULONG	pv, v ;

	    int	flags, iclass ;
	    int	i ;
	    int	is_write ;
	    int	f_outcome ;


/* maintain $r0 semantics */
	    regs.regs_R[MD_REG_ZERO] = 0 ;
#ifdef TARGET_ALPHA
	    regs.regs_F.d[MD_REG_ZERO] = 0.0 ;
#endif /* TARGET_ALPHA */

	    flags = 0 ;
	    iclass = 0 ;

/* load up a checker slot */

	    casp = cas + csp->wi ;
	    memset(casp,0,sizeof(SSAS)) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("sim_checkexec: CAS(%p) id=%u\n",casp,csp->wi) ;
#endif

	    casp->n.in = csp->in ;
	    casp->n.ia = regs.regs_PC ;
	    casp->n.ta = regs.regs_NPC ;

/* get the next instruction to execute */
	    MD_FETCH_INST(inst, mem, regs.regs_PC) ;

	    casp->n.instrword = inst ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("sim_checkexec: ia=%016llx instr=\\x%08x\n",
	            casp->n.ia,casp->n.instrword) ;
#endif

/* set default reference address and access mode */
	    addr = 0 ;
	    is_write = FALSE ;

/* set default fault - none */
	    fault = md_fault_none ;

/* decode the instruction */
	    MD_SET_OPCODE(opc, inst) ;

	    casp->n.op = opc ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("sim_checkexec: cin=%llu wi=%u ia=%016llx op=%u\n",
	            csp->in,csp->wi,casp->n.ia,casp->n.op) ;
#endif

#if	CF_TRACE
	    bprintf(&g.tfile,"cin=%llu wi=%u\n", csp->in,csp->wi) ;
#endif

/* we do not know anything about this instruction yet, so we must execute */

	    ii.ia = casp->n.ia ;

#if	CF_BPEVAL
	    if (bi.f_cfcond) {

	        f_outcome = (bi.ia == ii.ia) ;
	        bpeval_outcome(&pip->bp,csp->in,casp->n.ia,1,
	            f_outcome) ;

	    }
#endif /* CF_BPEVAL */

	if (pip->f.itrace)
		bprintf(pip->it1,"%10llu %16llX %u\n",
	    	csp->in, casp->n.ia,casp->n.op) ;


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
	setasmem(casp,1,addr,1,pv,0LL), pv)

#undef	READ_HALF
#define READ_HALF(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(casp,1,addr,2,pv,0LL), pv)

#undef	READ_WORD
#define READ_WORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(casp,1,addr,4,pv,0LL), pv)

#ifdef HOST_HAS_QWORD
#undef	READ_QWORD
#define READ_QWORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(casp,1,addr,8,pv,0LL), pv)
#endif /* HOST_HAS_QWORD */

#undef	WRITE_BYTE
#define WRITE_BYTE(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(casp,0,addr,1,pv,v),		\
  	MEM_WRITE_BYTE(mem,addr,v))

#undef	WRITE_HALF
#define WRITE_HALF(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(casp,0,addr,2,pv,v),		\
  	MEM_WRITE_HALF(mem,addr,v))

#undef	WRITE_WORD
#define WRITE_WORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(casp,0,addr,4,pv,v),		\
  	MEM_WRITE_WORD(mem,addr,v))

#ifdef HOST_HAS_QWORD
#undef	WRITE_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(casp,0,addr,8,pv,v),		\
  	MEM_WRITE_QWORD(mem,addr,v))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#undef	SYSCALL
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

	    switch (opc) {

#undef	DEFINST
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	    flags = (FLAGS) ;				\
	    iclass = (RES) ;				\
	    setas(casp,RES,flags,O1,O2,I1,I2,I3) ;	\
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

	    setasreg(casp) ;

/* set some extra bits */

	    if (casp->n.f.cf) {

	        casp->n.f.taken = TRUE ;
	        casp->n.ta = regs.regs_NPC ;
	        if (casp->n.f.cfcond) {

	            casp->n.f.taken = 
	                (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;
	            if (! casp->n.f.taken)
	                casp->n.ta = (regs.regs_PC + sizeof(md_inst_t)) ;

	        }

	    } /* end if (control-flow) */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5)) {
	        char	ibuf[100] ;

	        rs1 = md_disassemble(inst,regs.regs_PC,ibuf,99) ;

	        eprintf("sim_checkexec: %w\n",ibuf,rs1) ;

	        rs1 = instr_display(flags,ibuf,99) ;

	        eprintf("sim_checkexec: flags= %w\n",ibuf,rs1) ;

#ifdef	COMMENT
		if (DEBUGLEVEL(6))
	        ss_asdump(ssp,"sim_checkexec") ;
#endif

	    }
#endif /* CF_DEBUG */

/* selection region */

	f_se = (csp->in >= pip->sinstr) && (csp->in < pip->minstr) ;

/* branch predictor crap!! */

#if	CF_BPRED
	    if ((MD_OP_FLAGS(opc) & F_CTRL) && (pred != NULL)) {

	        struct bpred_update_t	update_rec ;

	        md_addr_t	target_PC = regs.regs_NPC ;
	        md_addr_t	pred_PC ;

	        int	stack_idx ;
	        int	f_taken, f_predtaken, f_predcorrect ;


		if (f_se)
	        pip->s.bp_inscf += 1 ;

/* get the next predicted fetch address */
	        pred_PC = bpred_lookup(pred,
	            /* branch addr */regs.regs_PC,
	            /* target */target_PC,
	            /* inst opcode */opc,
	            /* call? */MD_IS_CALL(opc),
	            /* return? */MD_IS_RETURN(opc),
	            /* stash an update ptr */&update_rec,
	            /* stash return stack ptr */&stack_idx) ;

/* valid address returned from branch predictor? */
	        if (! pred_PC)
	        {
/* no predicted taken target, attempt not taken target */
	            pred_PC = regs.regs_PC + sizeof(md_inst_t) ;

	        }

	        f_taken = 
	            (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;
	        f_predtaken = 
	            (pred_PC != (regs.regs_PC + sizeof(md_inst_t))) ;
	        f_predcorrect = 
	            (pred_PC == regs.regs_NPC) ;

	        casp->n.f.pred_taken = f_predtaken ;
	        casp->n.f.pred_correct = f_predcorrect ;

	        bpred_update(pred,
	            /* branch addr */regs.regs_PC,
	            /* resolved branch target */regs.regs_NPC,
	            /* taken? */ (regs.regs_NPC != (regs.regs_PC +
	            sizeof(md_inst_t))),
	            /* pred taken? */ (pred_PC != (regs.regs_PC +
	            sizeof(md_inst_t))),
	            /* correct pred? */ (pred_PC == regs.regs_NPC),
	            /* opcode */opc,
	            /* predictor update pointer */&update_rec) ;

		if (f_se) {

	        pip->s.bp_predgiven += 1 ;
	        if (f_taken)
	            pip->s.bp_taken += 1 ;

	        if (f_predtaken)
	            pip->s.bp_predtaken += 1 ;

	        if (f_predcorrect)
	            pip->s.bp_predcorrect += 1 ;

		}

	    } /* end if (branch predictor crap) */
#endif /* CF_BPRED */


	    if (pip->f.iflags && f_se)
	        iflags_proc(&pip->ifl,flags,iclass) ;

/* some statistics */

	    if (pip->f.itype && f_se)
		itype_proc(itp,casp) ;

/* some other statistics */

#ifdef	COMMENT

	    if (casp->n.f.cfind && (flags & F_INDIRJMP))
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

	    if (casp->n.f.memunalign)
	        g.s.memunaligned += 1 ;

	    if (casp->n.f.nmem > 1)
	        g.s.memmulti += 1 ;

	    if (casp->n.f.cf)
	        g.s.cf += 1 ;

	    if (casp->n.f.cfind)
	        g.s.cfind += 1 ;

	    if (casp->n.f.cfsub)
	        g.s.cfsub += 1 ;

	    if (casp->n.f.cfcond)
	        g.s.cfcond += 1 ;

#ifdef	COMMENT
#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        sim_asdump(pip,casp) ;
#endif
#endif /* COMMENT */

#endif /* COMMENT */

/* register statistics */

	if (pip->f.regstats) {

/* register reads */

		for (i = 0 ; i < casp->n.nopsi ; i += 1) {

			opp = casp->n.opsi + i ;
			if ((! opp->f.used) || (opp->type != OPERAND_TREG))
				continue ;
	    
#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("sim_checkexec: REG-R in=%llu a=%llu v=\\x%16llx\n",
			csp->in,opp->a,opp->v) ;
#endif

			if (opp->a != 31)
			regstats_read(&pip->rstats, csp->in,f_se,
				opp->a,opp->v) ;

		} /* end for */

/* register read updates */

		for (i = 0 ; i < casp->n.nopsi ; i += 1) {

			opp = casp->n.opsi + i ;
			if ((! opp->f.used) || (opp->type != OPERAND_TREG))
				continue ;

			if (opp->a != 31)
			regstats_readupdate(&pip->rstats, csp->in,f_se,
				opp->a,opp->v) ;

		} /* end for */

/* register writes */

		for (i = 0 ; i < casp->n.nopso ; i += 1) {

			opp = casp->n.opso + i ;
			if ((! opp->f.used) || (opp->type != OPERAND_TREG))
				continue ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(5))
	        eprintf("sim_checkexec: REG-W in=%llu a=%llu v=\\x%16llx\n",
			csp->in,opp->a,opp->v) ;
#endif

			if (opp->a != 31)
			regstats_write(&pip->rstats, csp->in,f_se,
				opp->a,opp->v) ;

		} /* end for */

	} /* end if (register statistics) */

/* memory statistics */

	if (pip->f.memstats) {

/* memory reads */

		for (i = 0 ; i < casp->n.nopsi ; i += 1) {

			opp = casp->n.opsi + i ;
			if ((! opp->f.used) || (opp->type != OPERAND_TMEM))
				continue ;

			memstats_read(&pip->mstats, csp->in,f_se,
				opp->a,opp->v) ;

		} /* end for */

/* memory writes */

		for (i = 0 ; i < casp->n.nopso ; i += 1) {

			opp = casp->n.opso + i ;
			if ((! opp->f.used) || (opp->type != OPERAND_TMEM))
				continue ;

			memstats_write(&pip->mstats, csp->in,f_se,
				opp->a,opp->v) ;

		} /* end for */

	} /* end if (memory statistics) */

/* do some other miscellaneous Simple-Scalar things */

	    if (fault != md_fault_none)
	        fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC) ;

	    if (verbose && (sim_num_insn >= trigger_inst)) {

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

	    if (MD_OP_FLAGS(opc) & F_MEM)
	    {
	        sim_num_refs++ ;
	        if (MD_OP_FLAGS(opc) & F_STORE)
	            is_write = TRUE ;
	    }

/* check for DLite debugger entry condition */
	    if (dlite_check_break(regs.regs_NPC,
	        is_write ? ACCESS_WRITE : ACCESS_READ,
	        addr, sim_num_insn, sim_num_insn))

	        dlite_main(regs.regs_PC, 
	            regs.regs_NPC, sim_num_insn, &regs, mem) ;

/* finally, we do some characterization here */

#if	CF_CHARSTAT
	    uiw = casp->n.class ;
	    if (uiw < iclass_overlast)
	        charstat.iclass[uiw] += 1 ;
#endif /* CF_CHARSTAT */

	    ii.f_cfcond = casp->n.f.cfcond ;

#if	CF_BPEVAL
	    bpeval_lookup(&pip->bp,csp->in,
	        casp->n.ia,1) ;

	    bpeval_checkmid(&pip->bp,csp->in) ;

	    bpeval_checkend(&pip->bp,csp->in) ;

#endif /* CF_BPEVAL */


/* go to the next instruction */
	    regs.regs_PC = regs.regs_NPC ;
	    regs.regs_NPC += sizeof(md_inst_t) ;

/* turn the window */

	    bi = ii ;
	    csp->wi = (csp->wi + 1) % csp->wsize ;
	    csp->in += 1 ;		/* checker instruction count */
	    sim_num_insn += 1 ;
	    if (sim_exit_now)
		break ;

	} /* end for (reading and executing instrutions) */

	csp->nw += ic ;
	if (sim_exit_now)
		pip->f.exit_target = TRUE ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    eprintf("sim_checkexec: sin=%llu nin=%llu win=%llu\n",
		pip->sinstr,pip->ninstr,pip->winstr) ;
	    eprintf("sim_checkexec: cin=%llu \n",
		csp->in) ;
	}
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	    eprintf("sim_checkexec: ret ic=%u (n-executed)\n",ic) ;
#endif

	return ic ;
}
/* end subroutine (sim_checkexec) */


/* get a checker IS address */
int sim_checkget(pip,caspp)
struct proginfo	*pip ;
SSAS		**caspp ;
{
	CHECKER	*csp = &pip->check ;

	int	rs = 0 ;
	int	navail ;


	if (caspp == NULL)
		return SR_FAULT ;

	navail = csp->nw - csp->nused ;
	if (navail < 1)
		return SR_DOM ;

	*caspp = csp->cas + csp->ui ;
	csp->ui = (csp->ui + 1) % csp->wsize ;
	csp->nused += 1 ;
	return SR_OK ;
}
/* end subroutine (sim_checkget) */


/* retire (sort-of commit) checker ASes */
int sim_checkreleaser(pip,ninstr)
PROGINFO	*pip ;
int		ninstr ;
{
	CHECKER	*csp = &pip->check ;

	int	rs = SR_OK ;


	if (ninstr == 0)
		goto ret0 ;

	if (ninstr > csp->nw) {
		rs = SR_BADFMT ;
		goto ret0 ;
	}

	csp->ri = (csp->ri + ninstr) % csp->wsize ;
	csp->nw -= ninstr ;

ret0:
	return (rs >= 0) ? ninstr : rs ;
}
/* end subroutine (sim_checkreleaser) */


/* retire (sort-of commit) checker ISes */
int sim_checkrelease(pip,ninstr)
PROGINFO	*pip ;
int		ninstr ;
{
	CHECKER	*csp = &pip->check ;

	SSAS	*casp ;

	int	rs = SR_OK, ri, i, j ;
	int	nc = 0 ;


	if (ninstr < 0)
		return SR_INVALID ;

/* get the number that we can retire starting with requested number */

	if (ninstr == 0)
		goto ret0 ;

	if ((ninstr > csp->nw) || (ninstr > csp->nused)) {
		rs = SR_DOM ;
		goto ret0 ;
	}

/* loop over leading (going out of the window) checker ASes */

	nc = ninstr ;
	ri = csp->ri ;
	for (i = 0 ; i < nc ; i += 1) {

	    OPERAND	*opp ;

	    int		opname ;


	    casp = csp->cas + ri ;

/* loop over output (destination) operands and write them back */

	    for (j = 0 ; j < casp->n.nopso ; j += 1) {

	        opp = &casp->n.opso[j] ;
	        if (opp->f.used) {

	            if (opp->type == OPERAND_TREG) {

	                opname = (int) opp->a ;
	                regs_setval(&csp->regs,opname,opp->v) ;

	            } else if (opp->type == OPERAND_TMEM) {

	                switch (opp->size) {

	                case 1:
	                    MEM_WRITE_BYTE(csp->mem,opp->a,opp->pv) ;

	                    break ;

	                case 2:
	                    MEM_WRITE_HALF(csp->mem,opp->a,opp->pv) ;

	                    break ;

	                case 4:
	                    MEM_WRITE_WORD(csp->mem,opp->a,opp->pv) ;

	                    break ;

	                case 8:
	                    MEM_WRITE_QWORD(csp->mem,opp->a,opp->pv) ;

	                    break ;

	                default:
	                    rs = SR_NOTSUP ;

	                } /* end switch */

	            } /* end if (type of operand) */

	        } /* end if (operand used) */

	    } /* end for (writing operands) */

	    ri = (ri + 1) % csp->wsize ;

	} /* end for (looping over checker ASes) */

	csp->ri = ri ;
	csp->nw -= nc ;
	csp->nused -= nc ;

ret0:
	return (rs >= 0) ? nc : rs ;
}
/* end subroutine (sim_checkrelease) */


/* return the number of checker ISes ready to be gotten */
int sim_checkavail(pip)
PROGINFO	*pip ;
{
	CHECKER	*csp = &pip->check ;

	int	rs = SR_OK ;
	int	navail ;


	navail = (csp->nw - csp->nused) ;
	if (navail < 0)
		rs = SR_BADFMT ;

	return (rs >= 0) ? navail : rs ;
}
/* end subroutine (sim_checkavail) */



/* LOCAL SUBROUTINES */



/* do a fast-forward operation */
static int sim_fast(pip,sinstr,winstr)
PROGINFO	*pip ;
ULONG		sinstr, winstr ;
{
	CHECKER	*csp = &pip->check ;

	SSAS	*cas = csp->cas ;
	SSAS	*casp ;

	tick_t	ck_cache ;

	ULONG	minstr, ic ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(10))
	    eprintf("sim_fast: base CAS(%p) == %p\n",csp->cas,cas) ;
#endif

	minstr = sinstr + winstr ;
	for (ic = 0 ; ic < minstr ; ic += 1) {

	    enum md_opcode opc ;

	    md_inst_t	inst ;

	    enum md_fault_type fault ;

	    md_addr_t	addr ;

	    ULONG	pv, v ;

	    int	wi ;
	    int	flags ;
	    int	is_write ;


	    ck_cache = csp->in ;

/* maintain $r0 semantics */
	    regs.regs_R[MD_REG_ZERO] = 0 ;
#ifdef TARGET_ALPHA
	    regs.regs_F.d[MD_REG_ZERO] = 0.0 ;
#endif /* TARGET_ALPHA */

/* load up a checker slot */

	    wi = csp->wi ;
	    casp = cas + csp->wi ;
	    memset(casp,0,sizeof(SSAS)) ;

	    casp->n.ia = regs.regs_PC ;
	    casp->n.ta = regs.regs_NPC ;

/* get the next instruction to execute */
	    MD_FETCH_INST(inst, mem, regs.regs_PC) ;

/* if we are in warm-up mode, do some warm-up */

#if	CF_CACHEWARM
	    if ((ic >= sinstr) && (cache_il1 != NULL) &&
	        (casp->n.ia != 0)) {

	        cache_access(cache_il1, Read, IACOMPRESS(casp->n.ia),
	            NULL, ISCOMPRESS(sizeof(md_inst_t)), ck_cache,
	            NULL, NULL) ;

	    } /* end if (i-cache warm-up) */
#endif /* CF_CACHEWARM */

	    casp->n.instrword = inst ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(10)) {
	        eprintf("sim_fast: ia=%016llx \n", casp->n.ia) ;
	        eprintf("sim_fast: wi=%u CAS=%p\n",wi,casp) ;
	        eprintf("sim_fast: instr=%08x \n", casp->n.instrword) ;
	    }
#endif /* CF_DEBUG */

/* set default reference address and access mode */
	    addr = 0 ;
	    is_write = FALSE ;

/* set default fault - none */
	    fault = md_fault_none ;

/* decode the instruction */
	    MD_SET_OPCODE(opc, inst) ;

	    casp->n.op = opc ;

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
	setasmem(casp,1,addr,1,pv,0LL), pv)

#undef	READ_HALF
#define READ_HALF(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(casp,1,addr,2,pv,0LL), pv)

#undef	READ_WORD
#define READ_WORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(casp,1,addr,4,pv,0LL), pv)

#ifdef HOST_HAS_QWORD
#undef	READ_QWORD
#define READ_QWORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(casp,1,addr,8,pv,0LL), pv)
#endif /* HOST_HAS_QWORD */

#undef	WRITE_BYTE
#define WRITE_BYTE(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_BYTE(mem,addr)),		\
	setasmem(casp,0,addr,1,pv,v),		\
  	MEM_WRITE_BYTE(mem,addr,v))

#undef	WRITE_HALF
#define WRITE_HALF(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	setasmem(casp,0,addr,2,pv,v),		\
  	MEM_WRITE_HALF(mem,addr,v))

#undef	WRITE_WORD
#define WRITE_WORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	setasmem(casp,0,addr,4,pv,v),		\
  	MEM_WRITE_WORD(mem,addr,v))

#ifdef HOST_HAS_QWORD
#undef	WRITE_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	setasmem(casp,0,addr,8,pv,v),		\
  	MEM_WRITE_QWORD(mem,addr,v))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#undef	SYSCALL
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

	    switch (opc) {

#undef	DEFINST
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	    flags = (FLAGS) ;				\
	    setas(casp,RES,flags,O1,O2,I1,I2,I3) ;	\
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

	    setasreg(casp) ;

	    casp->n.ta = regs.regs_NPC ;
	    casp->n.f.taken = casp->n.f.cfcond &&
	        (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;

/* if we are in warm-up mode, do some warm-up */

#if	CF_CACHEWARM
	    if ((ic >= sinstr) && (flags & F_MEM) &&
	        (cache_dl1 != NULL)) {

	        md_addr_t	aa ;

	        enum mem_cmd cmd ;	/* access type, Read or Write */


	        aa = addr & (~ (sizeof(md_addr_t) - 1)) ;
	        if (aa != 0) {

	            cmd = (flags & F_LOAD) ? Read : Write ;
	            cache_access(cache_dl1, cmd, IACOMPRESS(aa),
	                NULL, ISCOMPRESS(sizeof(md_addr_t)), ck_cache,
	                NULL, NULL) ;

	        }

	    } /* end if (cache warm-up) */
#endif /* CF_CACHEWARM */

/* some statistics */

#ifdef	COMMENT
	    if (casp->n.f.cfind && (flags & F_INDIRJMP))
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

	    if (casp->n.f.memunalign)
	        g.s.memunaligned += 1 ;

	    if (casp->n.f.nmem > 1)
	        g.s.memmulti += 1 ;

	    if (casp->n.f.cf)
	        g.s.cf += 1 ;

	    if (casp->n.f.cfind)
	        g.s.cfind += 1 ;

	    if (casp->n.f.cfsub)
	        g.s.cfsub += 1 ;

	    if (casp->n.f.cfcond)
	        g.s.cfcond += 1 ;

#endif /* COMMENT */

/* do some other miscellaneous Simple-Scalar things */

	    if (fault != md_fault_none)
	        fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC) ;

/* keep an instruction count */
	    sim_num_insn += 1 ;

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

	    if (MD_OP_FLAGS(opc) & F_MEM)
	    {
	        sim_num_refs++ ;
	        if (MD_OP_FLAGS(opc) & F_STORE)
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

/* turn the window */

	    csp->wi = (wi + 1) % csp->wsize ;
	    csp->in += 1 ;
	    if (sim_exit_now)
		break ;

	} /* end for (reading and executing instrutions) */

	return 0 ;
}
/* end subroutine (sim_fast) */


/* do a fast-forward operation -- only faster */
static int sim_faster(pip,sinstr,winstr)
PROGINFO	*pip ;
ULONG		sinstr, winstr ;
{
	IFLAGS	it ;

	tick_t	ck_cache ;

	ULONG	iskip, imax ;
	ULONG	icount ;

	int	rs = 0 ;
	int	f_warming ;


/* check if we skip enough for warming */

	if (winstr > 0) {
		if (winstr > sinstr)
			sinstr = winstr ;
	}

/* other initialization */

	if (pip->f.iflags)
		iflags_init(&it) ;

/* loop */

	iskip = sinstr - winstr ;
	imax = sinstr ;
	for (icount = 0 ; icount < imax ; icount += 1) {

	    enum md_opcode opc ;

	    md_inst_t	inst ;

	    enum md_fault_type fault ;

	    md_addr_t	ia, addr ;

	    ULONG	pv, v ;

	    int		flags, iclass ;
	    int		is_write ;


	    f_warming = (icount >= iskip) ;
	    ck_cache = icount ;

/* maintain $r0 semantics */
	    regs.regs_R[MD_REG_ZERO] = 0 ;
#ifdef TARGET_ALPHA
	    regs.regs_F.d[MD_REG_ZERO] = 0.0 ;
#endif /* TARGET_ALPHA */

	    ia = regs.regs_PC ;

/* get the next instruction to execute */
	    MD_FETCH_INST(inst, mem, regs.regs_PC) ;

/* if we are in warm-up mode, do some warm-up */

#if	CF_CACHEWARM
	    if (f_warming && (cache_il1 != NULL) &&
	        (ia != 0)) {

	        cache_access(cache_il1, Read, IACOMPRESS(ia),
	            NULL, ISCOMPRESS(sizeof(md_inst_t)), ck_cache,
	            NULL, NULL) ;

	    } /* end if (i-cache warm-up) */
#endif /* CF_CACHEWARM */

/* set default reference address and access mode */
	    flags = 0 ;
	    iclass = 0 ;
	    addr = 0 ;
	    is_write = FALSE ;

/* set default fault - none */
	    fault = md_fault_none ;

/* decode the instruction */
	    MD_SET_OPCODE(opc, inst) ;

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
	pv)

#undef	READ_HALF
#define READ_HALF(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_HALF(mem,addr)),		\
	pv)

#undef	READ_WORD
#define READ_WORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_WORD(mem,addr)),		\
	pv)

#ifdef HOST_HAS_QWORD
#undef	READ_QWORD
#define READ_QWORD(SRC, FAULT)						\
	(((FAULT) = md_fault_none),		\
	(addr = (SRC)),				\
	(pv = MEM_READ_QWORD(mem,addr)),	\
	pv)
#endif /* HOST_HAS_QWORD */

#undef	WRITE_BYTE
#define WRITE_BYTE(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
  	MEM_WRITE_BYTE(mem,addr, v))

#undef	WRITE_HALF
#define WRITE_HALF(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
  	MEM_WRITE_HALF(mem,addr,v))

#undef	WRITE_WORD
#define WRITE_WORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
  	MEM_WRITE_WORD(mem,addr,v))

#ifdef HOST_HAS_QWORD
#undef	WRITE_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
	(((FAULT) = md_fault_none),		\
	(addr = (DST)),				\
	(v = (SRC)),				\
  	MEM_WRITE_QWORD(mem,addr,v))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#undef	SYSCALL
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

	    switch (opc) {

#undef	DEFINST
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	    flags = (FLAGS) ;				\
	  iclass = (RES) ;				\
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

/* if we are in warm-up mode, do some warm-up */

#if	CF_BPREDWARM
	    if (f_warming && (MD_OP_FLAGS(opc) & F_CTRL) &&
	        (pred != NULL)) {

	        struct bpred_update_t	update_rec ;

	        md_addr_t	target_PC = regs.regs_NPC ;
	        md_addr_t	pred_PC ;

	        int	stack_idx ;
	        int	f_taken, f_predtaken, f_predcorrect ;


	        pip->s.bp_inscf += 1 ;

/* get the next predicted fetch address */
	        pred_PC = bpred_lookup(pred,
	            /* branch addr */regs.regs_PC,
	            /* target */target_PC,
	            /* inst opcode */opc,
	            /* call? */MD_IS_CALL(opc),
	            /* return? */MD_IS_RETURN(opc),
	            /* stash an update ptr */&update_rec,
	            /* stash return stack ptr */&stack_idx) ;

/* valid address returned from branch predictor? */
	        if (! pred_PC)
	        {
/* no predicted taken target, attempt not taken target */
	            pred_PC = regs.regs_PC + sizeof(md_inst_t) ;

	        }

	        f_taken = 
	            (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;
	        f_predtaken = 
	            (pred_PC != (regs.regs_PC + sizeof(md_inst_t))) ;
	        f_predcorrect = 
	            (pred_PC == regs.regs_NPC) ;

	        bpred_update(pred,
	            /* branch addr */regs.regs_PC,
	            /* resolved branch target */regs.regs_NPC,
	            /* taken? */ (regs.regs_NPC != (regs.regs_PC +
	            sizeof(md_inst_t))),
	            /* pred taken? */ (pred_PC != (regs.regs_PC +
	            sizeof(md_inst_t))),
	            /* correct pred? */ (pred_PC == regs.regs_NPC),
	            /* opcode */opc,
	            /* predictor update pointer */&update_rec) ;

	        pip->s.bp_predgiven += 1 ;
	        if (f_taken)
	            pip->s.bp_taken += 1 ;

	        if (f_predtaken)
	            pip->s.bp_predtaken += 1 ;

	        if (f_predcorrect)
	            pip->s.bp_predcorrect += 1 ;

	    } /* end if (branch predictor crap) */
#endif /* CF_BPREDWARM */

#if	CF_CACHEWARM
	    if (f_warming && (flags & F_MEM) &&
	        (cache_dl1 != NULL)) {

	        md_addr_t	aa ;

	        enum mem_cmd cmd ;	/* access type, Read or Write */


	        aa = addr & (~ (sizeof(md_addr_t) - 1)) ;
	        if (aa != 0) {

	            cmd = (flags & F_LOAD) ? Read : Write ;
	            cache_access(cache_dl1, cmd, IACOMPRESS(aa),
	                NULL, ISCOMPRESS(sizeof(md_addr_t)), ck_cache,
	                NULL, NULL) ;

	        }

	    } /* end if (cache warm-up) */
#endif /* CF_CACHEWARM */

/* instruction type profile */

	    if (pip->f.iflags)
	        iflags_proc(&it,flags,iclass) ;

/* do some other miscellaneous Simple-Scalar things */

	    if (fault != md_fault_none)
	        fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC) ;

/* keep an instruction count */
	    sim_num_insn += 1 ;

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

	    if (MD_OP_FLAGS(opc) & F_MEM)
	    {
	        sim_num_refs++ ;
	        if (MD_OP_FLAGS(opc) & F_STORE)
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

	    if (sim_exit_now)
		break ;

	} /* end for (reading and executing instrutions) */

	if (pip->f.iflags) {

	    char	tmpfname[MAXPATHLEN + 1] ;


	    mkfnamesuf(tmpfname,pip->jobname,FNE_IFLAGS0) ;

	    iflags_free(&it,tmpfname) ;

	}

	if (sim_exit_now)
		rs = 1 ;

	return rs ;
}
/* end subroutine (sim_faster) */


/* perform a "checktional" simulation */
static int sim_checktional(pip)
PROGINFO	*pip ;
{
	CHECKER	*csp = &pip->check ;

	ULONG	ic ;

	int	rs = SR_OK, rs1 ;
	int	nm, nic ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2))
	eprintf("sim_checktional: entered cin=%llu\n",csp->in) ;
#endif

	ic = csp->in ;
	while (ic < pip->minstr) {

		nm = MIN((pip->minstr - ic),csp->wsize) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(5))
	eprintf("sim_checktional: sim_checkexec() n=%u\n",nm) ;
#endif

		rs = sim_checkexec(pip,nm) ;

		nic = rs ;
		if (rs < 0)
			break ;

	    ic += nic ;
	    pip->s.ins += nic ;
	    pip->s.cks += nic ;
	    if ((rs > 0) || sim_exit_now)
		break ;

		rs = sim_checkreleaser(pip,nic) ;

		if (rs < 0)
			break ;

	} /* end for */

	if ((rs == 0) && sim_exit_now)
		rs = 1 ;

/* write statistics (if we were told to) */

	if (pip->f.itype) {

		ITYPE	*itp = (ITYPE *) pip->itp ;
	

		rs1 = itype_writeout(itp,pip->statsfname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_checktional: itype_writeout() rs=%d\n",rs1) ;
#endif

	} /* end if (i-types) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(2)) {
	eprintf("sim_checktional: cin=%llu\n",csp->in) ;
	eprintf("sim_checktional: ret rs=%d\n",rs) ;
	}
#endif

	return rs ;
}
/* end subroutine (sim_checktional) */


/* perform "functional" simulation */
static int sim_functional(pip)
PROGINFO	*pip ;
{
	CHECKER	*csp = &pip->check ;

	SSINFO	*lip = &pip->info ;

	SS	simplesim, *ssp = &simplesim ;

	SS_MACHOBJ	mobj ;

	SSIW		iwindow ;

	    bfile	sfile ;

	    ULONG	ins, in_end ;
	    ULONG	cks, ck_end ;

	    double	ipc_instrs, ipc_clocks, ipc ;
	    double	ips_elapsed, ips ;

	    time_t	time_start = time(NULL) ;
	    time_t	elapsed ;

	int	rs, rs1 ;
	int	i, j ;
	    int		f_sfile ;

	char	tmpfname[MAXPATHLEN + 1] ;
	    char	timebuf[TIMEBUFLEN + 1] ;


	rs = ss_init(ssp,pip,pip->minstr,NULL) ;

	if (rs < 0)
		goto ret0 ;

/* initialize the clocker */

	    memset(&mobj,0,sizeof(SS_MACHOBJ)) ;

	    mobj.topobjp = &iwindow ;
	    mobj.topcombp = (int (*)(void *,int)) ssiw_comb ;
	    mobj.topclockp = (int (*)(void *)) ssiw_clock ;

	    rs = ss_setmach(ssp,&mobj) ;

	if (rs < 0)
		goto ret0 ;

/* initialize the Levo instruction window (SSIW) */

	{
	    SSIW_INITARGS	iwa ;


	    memset(&iwa,0,sizeof(SS_MACHOBJ)) ;

	    iwa.iwsize = lip->iwsize ;

	    rs = ssiw_init(&iwindow,pip,ssp,lip,&iwa) ;

	} /* end block */

		if (rs < 0)
			goto ret1 ;

	    if (pip->f.iflags)
	        iflags_init(&pip->ifl) ;

/* initialize the functional starting registers */

	    for (j = 0 ; j < MACHSTATE_NREGS ; j += 1) {

	        ULONG	v ;


	        regs_getval(&csp->regs,j,&v) ;

	        ssiw_setreg(&iwindow,j,v) ;

	    } /* end for */

/* set the checker clocks to the same as its instructions */

	csp->ck = csp->in ;

/* set starting instruction count for "tell" progress dumps */

	proginfo_tellstart(pip,csp->ck,csp->in) ;

	    fprintf(stderr,"start instruction count=%llu\n",
	        csp->in) ;

/* advance the checker window */

	rs = sim_checkexec(pip,(lip->fqsize + lip->iwsize)) ;

/* do the main loop */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3)) {
	        eprintf("sim_main: ss_loop() in=%llu\n",csp->in) ;
	        eprintf("sim_main: ss_loop() ninstr=%llu\n",pip->ninstr) ;
	    }
#endif

	    rs = ss_loop(ssp,csp->ck,csp->in,pip->ninstr) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3))
	        eprintf("sim_main: ss_loop() rs=%d\n",rs) ;
#endif

	    if (rs < 0) {

	        bfile	dumpfile ;


	        mkfnamesuf(tmpfname,pip->jobname,FNE_DUMP) ;

	        rs1 = bopen(&dumpfile,tmpfname,"wct",0666) ;

	        if (rs1 >= 0) {

	            bprintf(&dumpfile,"dump rs=%d jobname=%s\n",
	                rs,pip->jobname) ;

	            bprintf(&dumpfile,"ck=%llu:%u\n",
	                ck_end,ssp->phase) ;

	            ssiw_dump(&iwindow,&dumpfile) ;

	            bclose(&dumpfile) ;

	        } /* end if (opened dump file) */

	    } /* end if (a dump was needed) */

/* SS information */

	    if (rs >= 0) {

		SS_INFO	si ;


		ss_info(ssp,&si) ;

		pip->f.exit_target = si.exit_target ;
		pip->f.exit_clock = si.exit_clock ;
		if ((! si.exit_target) && (! si.exit_clock))
			pip->f.exit_instr = TRUE ;

	        ss_getinstr(ssp,&in_end) ;

	        ss_getclock(ssp,&ck_end) ;

	        pip->s.ins = in_end - pip->s.in_start ;
	        pip->s.cks = ck_end - pip->s.ck_start ;

	    } /* end block */

/* calculate some stuff */

	    elapsed = time(NULL) - time_start ;

	    fprintf(stderr,"jobname=%s\n",pip->jobname) ;

	    fprintf(stderr,"mrs=%d\n",rs) ;

	    ins = in_end - pip->s.in_start ;
	    fprintf(stderr,"clocked instruction count CIC=%llu\n",
	        ins) ;

	    fprintf(stderr,"end instruction count     EIC=%llu\n",
	        in_end) ;

	    cks = ck_end - pip->s.ck_start ;
	    fprintf(stderr,"clock count CC=%llu\n",cks) ;

/* instruction type statistics */

	if (pip->f.itype) {

		ITYPE	*itp = (ITYPE *) pip->itp ;
	

		rs1 = itype_writeout(itp,pip->statsfname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("sim_functional: itype_writeout() rs=%d\n",rs1) ;
#endif

	} /* end if (i-types) */

/* simulation results */

	rs1 = SR_NOENT ;
	if (pip->statsfname[0] != '\0')
	    rs1 = bopen(&sfile,pip->statsfname,"wca",0666) ;

	f_sfile = (rs1 >= 0) ;

	if (f_sfile) {

	        bprintf(&sfile,"= simulation results\n") ;

	        bprintf(&sfile,"mrs=%d\n",rs) ;

	        bprintf(&sfile,"clocked instruction count CIC=%llu\n",
	            ins) ;

	        bprintf(&sfile,"end instruction count     EIC=%llu\n",
	            in_end) ;

	        bprintf(&sfile,"clock count CC=%llu\n",
	            cks) ;

	        bprintf(&sfile,"exit_target=%u\n",
			pip->f.exit_target) ;

	        bprintf(&sfile,"exit_clock=%u\n",
			pip->f.exit_clock) ;

	        bprintf(&sfile,"exit_instr=%u\n",
			pip->f.exit_instr) ;

	    } /* end if (writing stats file) */

	    ipc_instrs = (double) ins ;
	    ipc_clocks = (double) cks ;
	    ips_elapsed = (double) (elapsed > 0) ? elapsed : 1 ;

	    ipc = 0.0 ;
	    if (cks > 0)
	        ipc = ipc_instrs / ipc_clocks ;

	    fprintf(stderr,"IPC=%8.3f\n",ipc) ;

	    fprintf(stderr,"elapsed clocked time ECT=%s\n",
	        timestr_elapsed(elapsed,timebuf)) ;

	    ips = ipc_instrs / ips_elapsed ;
	    fprintf(stderr,"sim speed IPS=%16.2f\n",ips) ;

	    fprintf(stderr,"sim speed IPH=%16.2f\n",(ips * 3600.0)) ;

	    if (f_sfile) {

	        bprintf(&sfile,"IPC=%8.3f\n",ipc) ;

	        bprintf(&sfile,"elapsed clocked time ECT=%s\n",
	            timestr_elapsed(elapsed,timebuf)) ;

	        bprintf(&sfile,"sim speed IPS=%16.2f\n",ips) ;

	        bprintf(&sfile,"sim speed IPH=%16.2f\n",(ips * 3600.0)) ;

	        bprintf(&sfile,"\n") ;

	    } /* end if (writing statistics) */

	    if (f_sfile)
	        bclose(&sfile) ;

/* get and print some statistics */

	    ssiw_printstats(&iwindow,NULL) ;

/* free stuff up */
ret3:
	    if (pip->f.iflags) {

	        char	tmpfname[MAXPATHLEN + 1] ;


	        mkfnamesuf(tmpfname,pip->jobname,FNE_IFLAGS1) ;

	        iflags_free(&pip->ifl,tmpfname) ;

	    }

ret2:
	    ssiw_free(&iwindow) ;

ret1:
	ss_free(ssp) ;

ret0:
	return rs ;
} 
/* end subroutine (sim_functional) */


#ifdef	COMMENT

/* dump trace file */
static int sim_tracedump(pip,tfname)
struct proginfo	*pip ;
cchar	tfname[] ;
{
	struct proginfo_stats	*psp = &pip->s ;

	bfile	tfile ;

	int	rs ;
	int	wlen ;


	rs = bopen(&tfile,tfname,"wct",066) ;

	if (rs < 0)
		goto ret0 ;

	wlen = 0 ;
	rs = bprintf(&tfile,"in=%lld\n",
	    psp->in) ;

	wlen += rs ;
	rs = bprintf(&tfile,"memory %lld\n",
	    psp->mem) ;

	wlen += rs ;
	rs = bprintf(&tfile,"memory reads %lld\n",
	    pip->s.memreads) ;

	wlen += rs ;
	rs = bprintf(&tfile,"memory writes %lld\n",
	    psp->memwrites) ;

	wlen += rs ;
	rs = bprintf(&tfile,"unaligned memory %lld\n",
	    psp->memunaligned) ;

	wlen += rs ;
	rs = bprintf(&tfile,"memory multiple %lld\n",
	    psp->memmulti) ;

	wlen += rs ;
	rs = bprintf(&tfile,"weird %lld\n",
	    psp->weird) ;

#ifdef	COMMENT
	wlen += rs ;
	rs = bprintf(&tfile,"cf %lld\n",
		psp->cf) ;

	wlen += rs ;
	rs = bprintf(&tfile,"cfind %lld\n",
		psp->cfind) ;

	wlen += rs ;
	rs = bprintf(&tfile,"cfsub %lld\n",
		psp->cfsub) ;

	wlen += rs ;
	rs = bprintf(&tfile,"cfcond %lld\n",
		psp->cfcond) ;
#endif /* COMMENT */

	wlen += rs ;
	rs = bclose(&tfile) ;

ret0:
	return (rs >= 0) ? wlen : rs ;
}
/* end subroutine (sim_tracedump) */

#endif /* COMMENT */


/* write out general statistics */
static int sim_genstats(pip,statfname)
struct proginfo	*pip ;
cchar	statfname[] ;
{
	bfile	tmpfile, *tfp = &tmpfile ;

	double	sd ;

	time_t	daytime ;

	int	rs, rs1 ;

	char	timebuf[TIMEBUFLEN + 1] ;
	char	digbuf[DIGBUFLEN + 1] ;


	rs = bopen(tfp,statfname,"wca",0666) ;

	if (rs < 0)
		goto ret0 ;

	daytime = time(NULL) ;

	    bprintf(tfp,"ETSTATS %s\n",
		timestr_logz(daytime,timebuf)) ;

	    bprintf(tfp,"jobname=%s\n",
		pip->jobname) ;

	    bprintf(tfp,"time_start=%s\n",
		timestr_logz(pip->ti_start,timebuf)) ;

	    bprintf(tfp,"\n") ;

	    bprintf(tfp,"= configuration information\n") ;

	    bprintf(tfp,"sinstr=     %s\n",
		strsimpoint(digbuf,pip->sinstr)) ;

	    bprintf(tfp,"ninstr=     %s\n",
		strsimpoint(digbuf,pip->ninstr)) ;

		bclose(tfp) ;

ret0:
	return rs ;
}
/* end subroutine (sim_genstats) */


/* write out register statistics */
static int sim_regstats(pip,statfname)
struct proginfo	*pip ;
cchar	statfname[] ;
{
	bfile	tmpfile ;

	int	rs, rs1 ;

	        char	tmp1fname[MAXPATHLEN + 1] ;
	        char	tmp2fname[MAXPATHLEN + 1] ;
	        char	tmp3fname[MAXPATHLEN + 1] ;
	        char	tmp4fname[MAXPATHLEN + 1] ;
	        char	tmp5fname[MAXPATHLEN + 1] ;

	        double	sd ;


	        mkfname(tmp1fname,pip->jobname,FNE_REGRINT) ;

	        mkfname(tmp2fname,pip->jobname,FNE_REGLIFE) ;

	        mkfname(tmp3fname,pip->jobname,FNE_REGUSE) ;

	        mkfname(tmp4fname,pip->jobname,FNE_REGREAD) ;

	        mkfname(tmp5fname,pip->jobname,FNE_REGWRITE) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4)) {
	            eprintf("sim_regstats: tmp1fname=%s\n",tmp1fname) ;
	            eprintf("sim_regstats: tmp2fname=%s\n",tmp2fname) ;
	            eprintf("sim_regstats: tmp3fname=%s\n",tmp3fname) ;
	            eprintf("sim_regstats: tmp4fname=%s\n",tmp4fname) ;
	            eprintf("sim_regstats: tmp5fname=%s\n",tmp5fname) ;
	        }
#endif /* CF_DEBUG */

	        rs = regstats_storefiles(&pip->rstats,
	            tmp1fname,tmp2fname,tmp3fname,tmp4fname,tmp5fname) ;

/* write to the stats file */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_regstats: BSTATS statfname=%s\n",statfname) ;
#endif

		if (rs >= 0)
	        rs = bopen(&tmpfile,statfname,"wca",0666) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_regstats: BSTATS bopen() rs=%d\n",rs) ;
#endif

		if (rs >= 0) {

	            REGSTATS_STATS	st ;


#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_uninit: regstats_stats()\n") ;
#endif

	            rs1 = regstats_stats(&pip->rstats,&st) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_uninit: regstats_stats() rs=%d\n",rs1) ;
#endif

	            bprintf(&tmpfile,"\n") ;

	            bprintf(&tmpfile,"= register statistics\n") ;

/* general */

	            bprintf(&tmpfile,"in_start             %17llu\n",
	                st.in_start) ;

	            bprintf(&tmpfile,"ins                  %17llu\n",
	                st.ins) ;

	            bprintf(&tmpfile,"reads                %17llu\n",
	                st.reads) ;

	            bprintf(&tmpfile,"writes               %17llu\n",
	                st.writes) ;

	            bprintf(&tmpfile,"lenden               %17u\n",
	                st.lenden) ;

/* read-interval */

	            bprintf(&tmpfile,"read-interval amean       %17.4f\n",
	                st.rint_amean) ;

	            bprintf(&tmpfile,"read-interval dmean       %17.4f\n",
	                st.rint_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"read-interval variance    %17.4f\n",
	                st.rint_var) ;
#endif

	            bprintf(&tmpfile,"read-interval stddev      %17.4f\n",
	                sqrt(st.rint_var)) ;

	            bprintf(&tmpfile,"read-interval overflow    %17.4f %%\n",
	                st.rint_ov) ;

/* def-use */

	            bprintf(&tmpfile,"def-use amean             %17.4f\n",
	                st.use_amean) ;

	            bprintf(&tmpfile,"def-use dmean             %17.4f\n",
	                st.use_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"def-use variance          %17.4f\n",
	                st.use_var) ;
#endif

	            bprintf(&tmpfile,"def-use stddev            %17.4f\n",
	                sqrt(st.use_var)) ;

	            bprintf(&tmpfile,"def-use overflow          %17.4f %%\n",
	                st.use_ov) ;

/* useful-lifetime */

	            bprintf(&tmpfile,"lifetime amean            %17.4f\n",
	                st.life_amean) ;

	            bprintf(&tmpfile,"lifetime dmean            %17.4f\n",
	                st.life_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"lifetime variance         %17.4f\n",
	                st.life_var) ;
#endif

	            bprintf(&tmpfile,"lifetime stddev           %17.4f\n",
	                sqrt(st.life_var)) ;

	            bprintf(&tmpfile,"lifetime overflow         %17.4f %%\n",
	                st.life_ov) ;

/* done */
	            bclose(&tmpfile) ;

	        } /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_regstats: ret rs=%d\n",rs) ;
#endif

	return rs ;
} 
/* end subroutine (sim_regstats) */


/* write out memory statistics */
static int sim_memstats(pip,statfname)
struct proginfo	*pip ;
cchar	statfname[] ;
{
	        bfile	tmpfile ;

		int	rs, rs1 ;

	        char	tmp1fname[MAXPATHLEN + 1] ;
	        char	tmp2fname[MAXPATHLEN + 1] ;
	        char	tmp3fname[MAXPATHLEN + 1] ;
	        char	tmp4fname[MAXPATHLEN + 1] ;
	        char	tmp5fname[MAXPATHLEN + 1] ;

	        double	sd ;


	        mkfname(tmp1fname,pip->jobname,FNE_MEMRINT) ;

	        mkfname(tmp2fname,pip->jobname,FNE_MEMLIFE) ;

	        mkfname(tmp3fname,pip->jobname,FNE_MEMUSE) ;

		tmp4fname[0] = '\0' ;
		tmp5fname[0] = '\0' ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4)) {
	            eprintf("sim_memstats: tmp1fname=%s\n",tmp1fname) ;
	            eprintf("sim_memstats: tmp2fname=%s\n",tmp2fname) ;
	            eprintf("sim_memstats: tmp3fname=%s\n",tmp3fname) ;
	            eprintf("sim_memstats: tmp4fname=%s\n",tmp4fname) ;
	            eprintf("sim_memstats: tmp5fname=%s\n",tmp5fname) ;
	        }
#endif /* CF_DEBUG */

	        rs = memstats_storefiles(&pip->mstats,
	            tmp1fname,tmp2fname,tmp3fname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_memstats: memstats_storefiles() rs=%d\n",
			rs) ;
#endif

/* write to the stats file */

		if (rs >= 0)
	        rs = bopen(&tmpfile,statfname,"wca",0666) ;

		if (rs >= 0) {

	            MEMSTATS_STATS	st ;


	            memstats_stats(&pip->mstats,&st) ;

	            bprintf(&tmpfile,"\n") ;

	            bprintf(&tmpfile,"= memory statistics\n") ;

	            bprintf(&tmpfile,"in_start             %17llu\n",
	                st.in_start) ;

	            bprintf(&tmpfile,"ins                  %17llu\n",
	                st.ins) ;

	            bprintf(&tmpfile,"memory reads         %17llu\n",
	                st.reads) ;

	            bprintf(&tmpfile,"memory writes        %17llu\n",
	                st.writes) ;

	            bprintf(&tmpfile,"memory lenden             %12u\n",
	                st.lenden) ;

	            bprintf(&tmpfile,"memory variables          %12u\n",
	                st.tes) ;

	            bprintf(&tmpfile,"memory read variables     %12u\n",
	                st.readvars) ;

	            bprintf(&tmpfile,"memory write variables    %12u\n",
	                st.writevars) ;

	            bprintf(&tmpfile,
	                "tracking pages            %12u (in %u groups)\n",
	                st.pages,st.groups) ;

	            bprintf(&tmpfile,"storage allocated         %12u MiBytes\n",
	                (st.flen / (1024 * 1024))) ;

/* the 2^0 table */

	            bprintf(&tmpfile,"2^0 density (%u entries)\n",st.lenden) ;

/* read-interval */

	            bprintf(&tmpfile,"read-interval amean       %17.4f\n",
	                st.rint_amean) ;

	            bprintf(&tmpfile,"read-interval dmean       %17.4f\n",
	                st.rint_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"read-interval variance    %17.4f\n",
	                st.rint_var) ;
#endif

	            bprintf(&tmpfile,"read-interval stddev      %17.4f\n",
	                sqrt(st.rint_var)) ;

	            bprintf(&tmpfile,"read-interval overflow    %17.4f %%\n",
	                st.rint_ov) ;

	            bprintf(&tmpfile,"read-interval 70%% cov     %17.4f\n",
	                st.rint_p7) ;

	            bprintf(&tmpfile,"read-interval 80%% cov     %17.4f\n",
	                st.rint_p8) ;

	            bprintf(&tmpfile,"read-interval 90%% cov     %17.4f\n",
	                st.rint_p9) ;

/* def-use */

	            bprintf(&tmpfile,"def-use amean             %17.4f\n",
	                st.use_amean) ;

	            bprintf(&tmpfile,"def-use dmean             %17.4f\n",
	                st.use_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"def-use variance          %17.4f\n",
	                st.use_var) ;
#endif

	            bprintf(&tmpfile,"def-use stddev            %17.4f\n",
	                sqrt(st.use_var)) ;

	            bprintf(&tmpfile,"def-use overflow          %17.4f %%\n",
	                st.use_ov) ;

	            bprintf(&tmpfile,"defuse 70%% cov            %17.4f\n",
	                st.use_p7) ;

	            bprintf(&tmpfile,"defuse 80%% cov            %17.4f\n",
	                st.use_p8) ;

	            bprintf(&tmpfile,"defuse 90%% cov            %17.4f\n",
	                st.use_p9) ;

/* useful-lifetime */

	            bprintf(&tmpfile,"lifetime amean            %17.4f\n",
	                st.life_amean) ;

	            bprintf(&tmpfile,"lifetime dmean            %17.4f\n",
	                st.life_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"lifetime variance         %17.4f\n",
	                st.life_var) ;
#endif

	            bprintf(&tmpfile,"lifetime stddev           %17.4f\n",
	                sqrt(st.life_var)) ;

	            bprintf(&tmpfile,"lifetime overflow         %17.4f %%\n",
	                st.life_ov) ;

	            bprintf(&tmpfile,"lifetime 70%% cov          %17.4f\n",
	                st.life_p7) ;

	            bprintf(&tmpfile,"lifetime 80%% cov          %17.4f\n",
	                st.life_p8) ;

	            bprintf(&tmpfile,"lifetime 90%% cov          %17.4f\n",
	                st.life_p9) ;

/* the 2^10 table */

	            bprintf(&tmpfile,"2^10 density (%u entries)\n",st.lenden) ;

/* read-interval */

	            bprintf(&tmpfile,"1k read-interval amean    %17.4f\n",
	                st.rint1_amean) ;

	            bprintf(&tmpfile,"1k read-interval dmean    %17.4f\n",
	                st.rint1_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"1k read-interval variance %17.4f\n",
	                st.rint1_var) ;
#endif

	            bprintf(&tmpfile,"1k read-interval stddev   %17.4f\n",
	                sqrt(st.rint1_var)) ;

	            bprintf(&tmpfile,"1k read-interval overflow %17.4f %%\n",
	                st.rint1_ov) ;

	            bprintf(&tmpfile,"1k read-interval 70%% cov  %17.4f\n",
	                st.rint1_p7) ;

	            bprintf(&tmpfile,"1k read-interval 80%% cov  %17.4f\n",
	                st.rint1_p8) ;

	            bprintf(&tmpfile,"1k read-interval 90%% cov  %17.4f\n",
	                st.rint1_p9) ;

/* def-use */

	            bprintf(&tmpfile,"1k def-use amean          %17.4f\n",
	                st.use1_amean) ;

	            bprintf(&tmpfile,"1k def-use dmean          %17.4f\n",
	                st.use1_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"1k def-use variance       %17.4f\n",
	                st.use1_var) ;
#endif

	            bprintf(&tmpfile,"1k def-use stddev         %17.4f\n",
	                sqrt(st.use1_var)) ;

	            bprintf(&tmpfile,"1k def-use overflow       %17.4f %%\n",
	                st.use1_ov) ;

	            bprintf(&tmpfile,"1k def-use 70%% cov        %17.4f\n",
	                st.use1_p7) ;

	            bprintf(&tmpfile,"1k def-use 80%% cov        %17.4f\n",
	                st.use1_p8) ;

	            bprintf(&tmpfile,"1k def-use 90%% cov        %17.4f\n",
	                st.use1_p9) ;

/* useful-lifetime */

	            bprintf(&tmpfile,"1k lifetime amean         %17.4f\n",
	                st.life1_amean) ;

	            bprintf(&tmpfile,"1k lifetime dmean         %17.4f\n",
	                st.life1_mean) ;

#if	CF_VARIANCE
	            bprintf(&tmpfile,"1k lifetime variance      %17.4f\n",
	                st.life1_var) ;
#endif

	            bprintf(&tmpfile,"1k lifetime stddev        %17.4f\n",
	                sqrt(st.life1_var)) ;

	            bprintf(&tmpfile,"1k lifetime overflow      %17.4f %%\n",
	                st.life1_ov) ;

	            bprintf(&tmpfile,"1k lifetime 70%% cov       %17.4f\n",
	                st.life1_p7) ;

	            bprintf(&tmpfile,"1k lifetime 80%% cov       %17.4f\n",
	                st.life1_p8) ;

	            bprintf(&tmpfile,"1k lifetime 90%% cov       %17.4f\n",
	                st.life1_p9) ;

/* done */

	            bclose(&tmpfile) ;

	        } /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("sim_memstats: ret rs=%d\n", rs) ;
#endif

	return rs ;
} 
/* end subroutine (sim_memstats) */


/* set a checker AS from decoded instruction information */
static int setas(casp,res,flags,o1,o2,i1,i2,i3)
SSAS	*casp ;
int	res, flags ;
int	o1, o2 ;
int	i1, i2, i3 ;
{
	OPERAND	*opp ;

	int	oi ;


#if	CF_TRACE
	bprintf(&g.tfile,"i1=%d i2=%d i3=%d o1=%d o2=%d\n",
	    i1,i2,i3,o1,o2) ;
#endif

/* instruction class and instruction flags */

	casp->n.class = ((res >= 0) && (res < iclass_overlast)) ? res : 0 ;
	casp->n.flags = flags ;

/* some miscellaneous stuff that we can determine */

	casp->n.f.cfcond = ((flags & F_COND) || (flags & F_FPCOND)) ? 1 : 0 ;
	casp->n.f.cfdir = (flags & F_DIRJMP) ? 1 : 0 ;
	casp->n.f.cfind = (flags & F_INDIRJMP) ? 1 : 0 ;
	casp->n.f.cfsub = ((flags & F_CALL) || (flags & F_TRAP)) ? 1 : 0 ;
	casp->n.f.cf = ((flags & F_CTRL) || (flags & F_FPCOND) || 
	    casp->n.f.cfsub) ? 1 : 0 ;

	if (casp->n.f.cf)
	    casp->n.class = 0 ;

	casp->n.f.mem = (flags & F_MEM) ? 1 : 0 ;
	casp->n.f.memload = ((flags & F_MEM) && (flags & F_LOAD)) ? 1 : 0 ;
	casp->n.f.memstore = ((flags & F_MEM) && (flags & F_STORE)) ? 1 : 0 ;

	casp->n.f.executed = TRUE ;

/* input operands */

	oi = 0 ;
	SETOPI(asp,oi,DEP_NAME(i1),OPERAND_TREG) ;

	SETOPI(asp,oi,DEP_NAME(i2),OPERAND_TREG) ;

	SETOPI(asp,oi,DEP_NAME(i3),OPERAND_TREG) ;

	casp->n.nopsi = oi ;

/* output operands */

	oi = 0 ;
	SETOPO(asp,oi,DEP_NAME(o1),OPERAND_TREG) ;

	SETOPO(asp,oi,DEP_NAME(o2),OPERAND_TREG) ;

	casp->n.nopso = oi ;

/* ?? */

#ifdef	COMMENT
	casp->n.f.opchanged = (casp->n.nopsi || casp->n.nopso) ? TRUE : FALSE ;
#endif

	return 0 ;
}
/* end subroutine (setas) */


/* set all of the output (destination) register values in an AS */
static int setasreg(casp)
SSAS	*casp ;
{
	OPERAND	*opp ;

	int	i ;
	int	opname ;


	for (i = 0 ; i < casp->n.nopso ; i += 1) {

	    opp = &casp->n.opso[i] ;
	    if (opp->f.used && (opp->type == OPERAND_TREG)) {

	        opname = (int) opp->a ;
	        regs_getval(&regs,opname,&opp->v) ;

	    } /* end if */

	} /* end for */

	return 0 ;
}
/* end subroutine (setasreg) */


/* set a previous memory value in an AS */
static int setasmem(casp,f_read,a,size,pv,v)
SSAS	*casp ;
int	f_read ;	/* pread/nwrite */
ULONG	a ;
int	size ;
ULONG	pv, v ;
{
	OPERAND	*opp ;

	ULONG	justmask ;

	int	rs = SR_OK, i ;


#if	CF_TRACE
	bprintf(&g.tfile,"mem a=%016llx size=%d pv=%016llx\n",
	    a,size,pv) ;
#endif

/* allocate a new input operand */

	if (f_read) {

	    if (casp->n.nopsi < MACHSTATE_MAXOPSI) {

	        i = casp->n.nopsi++ ;
	        opp = &casp->n.opsi[i] ;

	    } else
	        rs = SR_OVERFLOW ;

	} else {

	    if (casp->n.nopso < MACHSTATE_MAXOPSO) {

	        i = casp->n.nopso++ ;
	        opp = &casp->n.opso[i] ;

	    } else
	        rs = SR_OVERFLOW ;

	}

/* load it up */

	if (rs >= 0) {

	    setop(opp,OPERAND_TMEM,a) ;

#if	CF_RJMEM
	    justmask = ~ ((~ 0) << (size * 8)) ;
	    opp->pv = pv & justmask ;
#else
	    opp->pv = pv << (a & 7) ;
#endif

#ifdef	COMMENT /* shouldn't matter (here we leave it zero ?) */
	    opp->opv = opp->pv ;
#endif

	    opp->size = size ;
	    opp->dp = mkdatapresent(((uint) (a & 7)),size) ;

	    if (opp->dp & 0xff00)
	        casp->n.f.memunalign = TRUE ;

	    if (! f_read) {

#if	CF_RJMEM
	        opp->v = v & justmask ;
#else
	        opp->v = v << (a & 7) ;
#endif

	    }

	    casp->n.f.nmem += 1 ;

	} /* end if (no problem) */

	return rs ;
}
/* end subroutine (setasmem) */


/* set a previous memory value in an AS */
static int setasmemread(casp,a,size,pv)
SSAS	*casp ;
ULONG	a ;
int	size ;
ULONG	pv ;
{
	OPERAND	*opp ;

	ULONG	justmask ;

	int	i ;


#if	CF_TRACE
	bprintf(&g.tfile,"mem a=%016llx size=%d pv=%016llx\n",
	    a,size,pv) ;
#endif

/* allocate a new input operand */

	if (casp->n.nopsi >= MACHSTATE_MAXOPSI)
	    return SR_OVERFLOW ;

	i = casp->n.nopsi++ ;
	opp = &casp->n.opsi[i] ;

/* load it up */

	setop(opp,OPERAND_TMEM,a) ;

#if	CF_RJMEM
	justmask = ~ ((~ 0) << (size * 8)) ;
	opp->pv = pv * justmask ;
#else
	opp->pv = pv << (a & 7) ;
#endif

	opp->size = size ;
	opp->dp = mkdatapresent(((uint) (a & 7)),size) ;

	return 0 ;
}
/* end subroutine (setasmemread) */


/* set an operand with its operand address (handles any type) */
static int setop(opp,type,oa)
OPERAND	*opp ;
uint	type ;
ULONG	oa ;
{


	opp->type = type ;

	opp->f.v = TRUE ;
	opp->f.used = TRUE ;
	opp->f.export = FALSE ;

	opp->a = oa ;
	opp->pv = 0 ;
	opp->v = 0 ;

	return 0 ;
}
/* end subroutine (setop) */


/* set the previous value of an operand register */
static int setopreg(opp,opname)
OPERAND	*opp ;
int	opname ;
{
	int	rs ;


	rs = regs_getval(&regs,opname,&opp->pv) ;

	return rs ;
}
/* end subroutine (setopreg) */


static int mkdatapresent(a,size)
uint	a ;
int	size ;
{
	int	i, dp ;


#if	CF_MASTERDEBUG && CF_DEBUGS && 0
	i = a & 7 ;
	eprintf("mkdatapresent: a=%02x size=%u\n",i,size) ;
#endif

	i = ((a & 7) << 4) | (size & 15) ;
	dp = (int) dps[i] ;

#if	CF_MASTERDEBUG && CF_DEBUGS && 0
	eprintf("mkdatapresent: dp=%02x\n",dp) ;
#endif

	return dp ;
}
/* end subroutine (mkdatapresent) */


#ifdef	COMMENT

/* dump a checker AS */
static int sim_asdump(pip,asp)
PROGINFO	*pip ;
SSAS		*asp ;
{
	OPERAND	*opp ;

	int	i ;


	bprintf(&ssp->tfile,"ia=%16llx\n",
	    casp->n.ia) ;

	bprintf(&ssp->tfile,"op=%d class=%d flags=%06x\n",
	    casp->n.op,casp->n.class,casp->n.flags) ;

	bprintf(&ssp->tfile,"nio=%d noo=%d\n",
	    casp->n.nopsi,casp->n.nopso) ;

	for (i = 0 ; i < casp->n.nopsi ; i += 1) {

	    opp = &casp->n.opsi[i] ;
	    bprintf(&ssp->tfile,
	        "IOP i=%d t=%u a=%16llx pv=%016llx\n",
	        i,opp->type,opp->a,opp->pv) ;

	    if (opp->type == OPERAND_TMEM)
	        bprintf(&ssp->tfile,
	            "	size=%d dp=%02x\n",
	            opp->size,opp->dp) ;

	} /* end for */

	for (i = 0 ; i < casp->n.nopso ; i += 1) {

	    opp = &casp->n.opso[i] ;
	    bprintf(&ssp->tfile,
	        "OOP i=%d t=%u a=%16llx pv=%016llx v=%016llx\n",
	        i,opp->type,opp->a,opp->pv,opp->v) ;

	    if (opp->type == OPERAND_TMEM)
	        bprintf(&ssp->tfile,
	            "	size=%d dp=%02x\n",
	            opp->size,opp->dp) ;

	} /* end for */


	return 0 ;
}
/* end subroutine (g_asdump) */

#endif /* COMMENT */


int instr_display(f,buf,buflen)
uint	f ;
char	buf[] ;
int	buflen ;
{
	SBUF	b ;

	int	rs ;


	sbuf_init(&b,buf,buflen) ;

	if ((f & F_ICOMP) || (f & F_FCOMP)) {

	    sbuf_strw(&b," COMP",-1) ;

	    if (f & F_ICOMP)
	        sbuf_strw(&b,"-INT",-1) ;

	    if (f & F_FCOMP)
	        sbuf_strw(&b,"-FLOAT",-1) ;

	}

	if (f & F_CTRL) {

	    sbuf_strw(&b," CONTROL",-1) ;

	    if (f & F_COND)
	        sbuf_strw(&b,"-COND",-1) ;

	    if (f & F_UNCOND)
	        sbuf_strw(&b,"-UNCOND",-1) ;

	}

	if (f & F_MEM) {

	    sbuf_strw(&b," MEMORY",-1) ;

	    if (f & F_LOAD)
	        sbuf_strw(&b,"-LOAD",-1) ;

	    if (f & F_STORE)
	        sbuf_strw(&b,"-STORE",-1) ;

	    if (f & F_DISP)
	        sbuf_strw(&b,"-DISP",-1) ;

	    if (f & F_RR)
	        sbuf_strw(&b,"-RR",-1) ;

	    if (f & F_DIRECT)
	        sbuf_strw(&b,"-DIRECT",-1) ;

	}

	if (f & F_TRAP) {

	    sbuf_strw(&b," TRAP",-1) ;

	}

	if (f & F_LONGLAT) {

	    sbuf_strw(&b," LONGLATENCY",-1) ;

	}

	if (f & F_DIRJMP)
	    sbuf_strw(&b," DIRJMP",-1) ;

	if (f & F_INDIRJMP)
	    sbuf_strw(&b," INDIRJMP",-1) ;

	if (f & F_CALL)
	    sbuf_strw(&b," CALL",-1) ;

	if (f & F_FPCOND)
	    sbuf_strw(&b," FPCOND",-1) ;

	if (f & F_IMM)
	    sbuf_strw(&b," IMM",-1) ;

	rs = sbuf_free(&b) ;

	return rs ;
}
/* end subroutine (instr_display) */


/* get options for this statistics module */
int getprogopts(pip,kop,pp)
struct proginfo	*pip ;
KEYOPT		*kop ;
struct params	*pp ;
{
	KEYOPT_CURSOR	kcur ;

	int	rs, i, oi, val ;
	int	nlen, klen, vlen ;
	int	n ;

	char	*kp, *vp ;
	char	*cp ;


/* process all of the options that we have so far */

	n = 0 ;
	keyopt_cursorinit(kop,&kcur) ;

	while ((rs = keyopt_enumkeys(kop,&kcur,&kp)) >= 0) {

	    KEYOPT_CURSOR	vcur ;

	    int	f_value = FALSE ;


	    klen = rs ;

/* get the first (non-zero length) value for this key */

	    vlen = -1 ;
	    keyopt_cursorinit(kop,&vcur) ;

	    while ((rs = keyopt_enumvalues(kop,kp,&vcur,&vp)) >= 0) {

	        f_value = TRUE ;
	        vlen = rs ;
	        if (vlen > 0)
	            break ;

	    } /* end while */

	    keyopt_cursorfree(kop,&vcur) ;

/* do we support this option ? */

	    if ((oi = optmatch3(progopts,kp,klen)) >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("getprogopts: system valid option, oi=%d\n",
	                oi) ;
#endif

	        switch (oi) {

	        case progopt_rows:
	            pp->rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_delay:
	            pp->delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_confidence:
	            pp->f_confidence = FALSE ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->f_confidence = (val != 0) ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_vprows:
	            pp->vp_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_vpdelay:
	            pp->vp_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_vpentries:
	            pp->vp_entries = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_entries = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_vpnops:
	            pp->vp_nops = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_nops = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_vpstride:
	            pp->vp_stride = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->vp_stride = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_yagsrows:
	            pp->yags_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_yagsdelay:
	            pp->yags_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_yagscpht:
	            pp->yags_cpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_cpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_yagsdpht:
	            pp->yags_dpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->yags_dpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_tournarows:
	            pp->tourna_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->tourna_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_tournadelay:
	            pp->tourna_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->tourna_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_tournalbht:
	            pp->tourna_lbht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->tourna_lbht = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_tournalpht:
	            pp->tourna_lpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->tourna_lpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_tournagpht:
	            pp->tourna_gpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->tourna_gpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_gspagrows:
	            pp->gspag_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_gspagdelay:
	            pp->gspag_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_gspaglbht:
	            pp->gspag_lbht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_lbht = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_gspaggpht:
	            pp->gspag_gpht = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gspag_gpht = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_gskewrows:
	            pp->gskew_rows = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gskew_rows = val ;
	                n += 1 ;

	            }

	            break ;

	        case progopt_gskewdelay:
	            pp->gskew_delay = 0 ;
	            if ((vlen > 0) && (cfdeci(vp,vlen,&val) >= 0)) {

	                pp->gskew_delay = val ;
	                n += 1 ;

	            }

	            break ;

	        } /* end switch */

	    } /* end if (got a match) */

	} /* end while */

	keyopt_cursorfree(kop,&kcur) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(4))
	    eprintf("getprogopts: n=%d\n",n) ;
#endif

	return n ;
}
/* end subroutine (getprogopts) */


#if	CF_BPEVAL

/* load any specified branch predictor modules */
int loadbps(pip,kop,bpsp)
struct proginfo	*pip ;
KEYOPT		*kop ;
BPEVAL		*bpsp ;
{
	KEYOPT_CURSOR	vcur ;

	int	rs ;
	int	sl, vl ;
	int	n ;

	char	name[MAXNAMELEN + 1] ;
	char	*vp ;
	char	*sp, *cp ;


	n = 0 ;
	keyopt_cursorinit(kop,&vcur) ;

	while ((vl = keyopt_enumvalues(kop,"bpload",&vcur,&vp)) >= 0) {

	    int	p1, p2, p3, p4 ;


	    if (vp == NULL)
	        continue ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("loadbps: vl=%d vp=>%s<\n",vl,vp) ;
#endif

	    rs = SR_INVALID ;
	    if (vp[0] == '\0')
	        break ;

	    p1 = p2 = p3 = p4 = -1 ;

	    sp = vp ;
	    sl = -1 ;
	    if ((cp = strchr(sp,':')) != NULL)
	        sl = cp - vp ;

	    if (sl == 0)
	        break ;

	    strwcpy(name,sp,MIN(sl,(MAXNAMELEN - 1))) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("loadbps: name=%s\n",name) ;
#endif

	    if (cp != NULL) {

	        sp = cp + 1 ;
	        sl = -1 ;
	        if ((cp = strchr(sp,':')) != NULL)
	            sl = cp - sp ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("loadbps: p1s=%w\n",sp,sl) ;
#endif

	        cfdeci(sp,sl,&p1) ;

	    }

	    if (cp != NULL) {

	        sp = cp + 1 ;
	        sl = -1 ;
	        if ((cp = strchr(sp,':')) != NULL)
	            sl = cp - sp ;

	        cfdeci(sp,sl,&p2) ;

	    }

	    if (cp != NULL) {

	        sp = cp + 1 ;
	        sl = -1 ;
	        if ((cp = strchr(sp,':')) != NULL)
	            sl = cp - sp ;

	        cfdeci(sp,sl,&p3) ;

	    }

	    if (cp != NULL) {

	        sp = cp + 1 ;
	        sl = -1 ;
	        if ((cp = strchr(sp,':')) != NULL)
	            sl = cp - sp ;

	        cfdeci(sp,sl,&p4) ;

	    }

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("loadbps: p1=%d p2=%d p3=%d p4=%d\n",p1,p2,p3,p4) ;
#endif

	    rs = bpeval_add(bpsp,name,NULL,p1,p2,p3,p4) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(4))
	        eprintf("loadbps: bpeval_add() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

	    n += 1 ;

	} /* end while */

	keyopt_cursorfree(kop,&vcur) ;

/* do we have a particular one selected ? */

	if ((vl = keyopt_enumvalues(kop,"bpsel",NULL,&vp)) >= 0) {

	    if (vp != NULL)
	        bpeval_bpsel(bpsp,vp) ;

	} /* end if */

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (loadbps) */

#endif /* CF_BPEVAL */


/* write out the statistics */
static int writestats(pip,scp,bpsp,pp)
struct proginfo		*pip ;
struct stats		*scp ;
BPEVAL			*bpsp ;
struct params		*pp ;
{
	BPRESULT	results ;

	bfile	tmpfile ;

	double	v1, v2, ave ;
	double	acc ;

	time_t	daytime ;

	int	rs, i, j, k ;

	char	tmpfname[MAXPATHLEN + 1] ;
	char	timebuf1[TIMEBUFLEN + 1], timebuf2[TIMEBUFLEN + 1] ;


/* write out the instruction (operations) frequencies */

	mkfnamesuf(tmpfname,pip->jobname,FNE_ICOUNTS) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < iclass_overlast ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", i,scp->iclass[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened i-count file) */

/* write out the branch-path lengths */

	mkfnamesuf(tmpfname,pip->jobname,FNE_BPLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BPSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", 
	            i,scp->bplen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened bplen file) */

/* write out the branch-target lengths */

	mkfnamesuf(tmpfname,pip->jobname,FNE_BFTLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BTSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", 
	            i,scp->bftlen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened bftlen file) */

	mkfnamesuf(tmpfname,pip->jobname,FNE_BBTLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BTSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", 
	            i,scp->bbtlen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened bbtlen file) */

/* write out SS hammock branch-target lengths */

	mkfnamesuf(tmpfname,pip->jobname,FNE_HTLEN) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    for (i = 0 ; i < BTSIZE ; i += 1)
	        bprintf(&tmpfile,"%4d %12llu\n", 
	            i,scp->htlen[i]) ;

	    bclose(&tmpfile) ;

	} /* end if (opened htlen file) */

/* write out general stuff */

	mkfnamesuf(tmpfname,pip->jobname,FNE_BSTATS) ;

	if ((rs = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	    struct rusage	ut ;

	    ULONG	llw ;

	    int	rs1 ;

	    double	percent, mean, var, stddev ;


	    daytime = time(NULL) ;

	    bprintf(&tmpfile,"SimpleSim statistics %s\n",
	        timestr_log(daytime,tmpfname)) ;

	    bprintf(&tmpfile,"statistics for job=%s logid=%s\n",
	        pip->jobname,pip->logid) ;

	    bprintf(&tmpfile,"started %s (elapsed time %s)\n",
	        timestr_log(pip->ti_start,timebuf1),
	        timestr_elapsed((daytime - pip->ti_start),timebuf2)) ;

	    rs1 = uc_getrusage(RUSAGE_SELF,&ut) ;

	    if (rs1 >= 0) {

	        char	timebuf[TIMEBUFLEN + 1] ;

	        int	elapsed ;


	        daytime = time(NULL) ;

	        elapsed = daytime - pip->ti_start ;
	        bprintf(&tmpfile,"real   time=%s.%06d\n",
	            timestr_elapsed(elapsed,timebuf),
	            ut.ru_utime.tv_usec) ;

	        bprintf(&tmpfile,"user   time=%s.%06d\n",
	            timestr_elapsed(ut.ru_utime.tv_sec,timebuf),
	            ut.ru_utime.tv_usec) ;

	        bprintf(&tmpfile,"system time=%s.%06d\n",
	            timestr_elapsed(ut.ru_stime.tv_sec,timebuf),
	            ut.ru_stime.tv_usec) ;

	    } /* end if (getrusage) */

	    bputc(&tmpfile,'\n') ;


	    bprintf(&tmpfile,
	        "total instructions    %12llu\n",
	        scp->in) ;

	    bprintf(&tmpfile,"included instructions %12llu (%llu:%llu)\n",
	        scp->in,pip->in.start,scp->in) ;

	    bprintf(&tmpfile,"CF instructions       %12llu (%7.3f%%)\n",
	        scp->cf,
	        percentll(scp->cf,scp->in)) ;

	    bprintf(&tmpfile,"CF-IND instructions   %12llu (%7.3f%%)\n",
	        scp->cf_ind,
	        percentll(scp->cf_ind,scp->in)) ;

	    bprintf(&tmpfile,"relative branches     %12llu (%7.3f%%)\n",
	        scp->br_rel,
	        percentll(scp->br_rel,scp->in)) ;

	    bprintf(&tmpfile,"CF-UNC                %12llu (%7.3f%%)\n",
	        (scp->cf - scp->br_con),
	        percentll((scp->cf - scp->br_con) ,scp->in)) ;

	    bprintf(&tmpfile,"conditional branches  %12llu (%7.3f%%)\n",
	        scp->br_con,
	        percentll(scp->br_con,scp->in)) ;

	    bprintf(&tmpfile,"forward c-branches    %12llu (%7.3f%%)\n",
	        scp->br_fwd,
	        percentll(scp->br_fwd,scp->in)) ;

	    llw = scp->br_con - scp->br_fwd ;
	    percent = percentll(llw,scp->in) ;

	    bprintf(&tmpfile,"backward c-branches   %12llu (%7.3f%%)\n",
	        llw,percent) ;

#ifdef	COMMENT
	    if (pip->f.opt_ssh)
	        bprintf(&tmpfile,"branch SS-hammocks    %12llu (%7.3f%%)\n",
	            scp->br_ssh,
	            percentll(scp->br_ssh,scp->in)) ;
#endif /* COMMENT */

/* branch path statistics */

	    v1 = scp->br_con ;		/* conditional control-flow */
	    v2 = scp->in ;		/* total instructions (considered) */
	    ave = (scp->br_con > 0) ? (v2 / v1) : 0.0 ;
	    bprintf(&tmpfile,"instructions per BP %17.4f\n",
	        ave) ;

	    bprintf(&tmpfile,"= branch path statistics\n") ;

	    rs = fmeanvaral(scp->bplen,BPSIZE,&mean,&var) ;

	    if (rs >= 0) {

	        bprintf(&tmpfile,"instructions per branch path\n") ;

	        bprintf(&tmpfile,"IPBP mean           %17.4f\n",
	            mean) ;

#if	CF_VARIANCE
	        bprintf(&tmpfile,"IPBP variance       %17.4f\n",
	            var) ;
#endif

	        stddev = sqrt(var) ;

	        bprintf(&tmpfile,"IPBP stddev         %17.4f\n",
	            stddev) ;

	    } else
	        bprintf(&tmpfile,"*NA*\n") ;

/* branch target statistics */

	    bprintf(&tmpfile,"= branch target-distance statistics\n") ;

/* forward */

	    rs = fmeanvaral(scp->bftlen,BTSIZE,&mean,&var) ;

	    if (rs >= 0) {

	        bprintf(&tmpfile,"instructions per forward branch target\n") ;

	        bprintf(&tmpfile,"IPFBT mean          %17.4f\n",
	            mean) ;

#if	CF_VARIANCE
	        bprintf(&tmpfile,"IPFBT variance      %17.4f\n",
	            var) ;
#endif

	        stddev = sqrt(var) ;

	        bprintf(&tmpfile,"IPFBT stddev        %17.4f\n",
	            stddev) ;

	    } else
	        bprintf(&tmpfile,"*NA*\n") ;

/* backward */

	    rs = fmeanvaral(scp->bbtlen,BTSIZE,&mean,&var) ;

	    if (rs >= 0) {

	        bprintf(&tmpfile,"instructions per backward branch target\n") ;

	        bprintf(&tmpfile,"IPBBT mean          %17.4f\n",
	            mean) ;

#if	CF_VARIANCE
	        bprintf(&tmpfile,"IPBBT variance      %17.4f\n",
	            var) ;
#endif

	        stddev = sqrt(var) ;

	        bprintf(&tmpfile,"IPBBT stddev        %17.4f\n",
	            stddev) ;

	    } else
	        bprintf(&tmpfile,"*NA*\n") ;

/* SS-Hammock branch target statistics */

	    bprintf(&tmpfile,
	        "= hammock branch target-distance statistics\n") ;

	    rs = fmeanvaral(scp->htlen,BTSIZE,&mean,&var) ;

	    if (rs >= 0) {

	        bprintf(&tmpfile,"IPHT mean           %17.4f\n",
	            mean) ;

#if	CF_VARIANCE
	        bprintf(&tmpfile,"IPHT variance       %17.4f\n",
	            var) ;
#endif

	        stddev = sqrt(var) ;

	        bprintf(&tmpfile,"IPHT stddev         %17.4f\n",
	            stddev) ;

	    } else
	        bprintf(&tmpfile,"*NA*\n") ;

/* forward conditional branches */

	    percent = percentll(scp->br_fwd,scp->br_con) ;

	    bprintf(&tmpfile,
	        "forward c-branches    %12llu (%6.2f %% of conditional)\n",
	        scp->br_fwd,percent) ;

	    llw = scp->br_con - scp->br_fwd ;
	    percent = percentll(llw,scp->br_con) ;

	    bprintf(&tmpfile,
	        "backward c-branches   %12llu (%6.2f %% of conditional)\n",
	        llw,percent) ;

/* ss-hammocks */

	    percent = percentll(scp->br_ssh,scp->br_con) ;

	    bprintf(&tmpfile,
	        "SS hammocks           %12llu (%6.2f%% of conditional)\n",
	        scp->br_ssh,percent) ;


/* other stuff */

#ifdef	COMMENT
	    if (pip->f.opt_badias) {

	        bprintf(&tmpfile,
	            "= debugging\n") ;

	        bprintf(&tmpfile,
	            "bad IAs               %12llu (%7.3f%%)\n",
	            scp->ia_bad,
	            percentll(scp->ia_bad,scp->in)) ;

	    }
#endif /* COMMENT */


/* branch prediction results */

#if	CF_BPEVAL

	    rs = bpresult_open(&results,NULL) ;

/* do the predictors that were dynamically loaded ! */

	    {
	        BPEVAL_STATS	es ;


#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(4))
	            eprintf("stats/writestats: dynamic preds\n") ;
#endif

	        bprintf(&tmpfile,"= dynamically loaded branch predictors :\n") ;

	        bprintf(&tmpfile,
	            "rows                  %12d\n",
	            pp->rows) ;

	        bprintf(&tmpfile,
	            "delay                 %12d\n",
	            pp->delay) ;

	        for (i = 0 ; bpeval_stats(bpsp,i,&es) >= 0 ; i += 1) {

	            bprintf(&tmpfile,
	                "%-8s params           %8d %8d %8d %8d\n",
	                es.name,
	                es.p.p1,es.p.p2,es.p.p3,es.p.p4) ;

	            bprintf(&tmpfile,
	                "%-8s bits         %12u\n",
	                es.name,es.bits) ;

	            bprintf(&tmpfile,
	                "%-8s lookups      %12u\n",
	                es.name,
	                es.lookups) ;

	            acc = percentll((ULONG) es.corrects,scp->br_con) ;

	            bprintf(&tmpfile,
	                "%-8s accuracy     %12u (%7.3f%%)\n",
	                es.name,
	                es.corrects, acc) ;

/* prediction confidences */

	            for (k = 0 ; k < 2 ; k += 1) {

	                bprintf(&tmpfile,
	                    "%-8s confidences %d-%d  ",
	                    es.name,(k * 4),((k * 4) + 3)) ;

	                for (j = (k * 4) ; j < ((k + 1) * 4) ; j += 1) {

	                    acc = percentll((ULONG) es.confidence[j],
	                        scp->br_con) ;

	                    bprintf(&tmpfile,"%7.3f%%",acc) ;

	                }

	                bprintf(&tmpfile,"\n") ;

	            } /* end for */

/* correct confidences */

	            for (k = 0 ; k < 2 ; k += 1) {

	                bprintf(&tmpfile,
	                    "%-8s correct-conf %d-%d ",
	                    es.name,(k * 4),((k * 4) + 3)) ;

	                for (j = (k * 4) ; j < ((k + 1) * 4) ; j += 1) {

	                    acc = percentll((ULONG) es.cc[j],scp->br_con) ;

	                    bprintf(&tmpfile,"%7.3f%%",acc) ;

	                }

	                bprintf(&tmpfile,"\n") ;

	            } /* end for */

/* write out the report for this predictor */

	            acc = percentll((ULONG) es.corrects,scp->br_con) ;

	            bpresult_write(&results,pip->targetname,acc, es.name,
	                (int) es.bits,
	                es.p.p1,es.p.p2,es.p.p3,es.p.p4) ;

	        } /* end for (looping through loaded predictors) */

	    } /* end block (dynamically loaded predictors) */

/* close out the branch predictor results */

	    bpresult_close(&results) ;

#endif /* CF_BPEVAL */


/* done */

	    bclose(&tmpfile) ;

	} /* end if (opened bstats file) */


	return SR_OK ;
}
/* end subroutine (writestats) */


/* memory access latency, assumed to not cross a page boundary */
static unsigned int			/* total latency of access */
mem_access_latency(int blk_sz)		/* block size accessed */
{
	int chunks = (blk_sz + (mem_bus_width - 1)) / mem_bus_width ;

	assert(chunks > 0) ;

	return (/* first chunk latency */mem_lat[0] +
	    (/* remainder chunk latency */mem_lat[1] * (chunks - 1))) ;
}


/* * cache miss handlers */


/* l1 data cache, l1 block miss handler function */
static unsigned int			/* latency of block access */
dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
md_addr_t baddr,		/* block address to access */
int bsize,		/* size of block to access */
struct cache_blk_t *blk,	/* ptr to block in upper level */
tick_t now)		/* time of access */
{
	unsigned int lat ;

	if (cache_dl2)
	{
/* access next level of data cache hierarchy */
	    lat = cache_access(cache_dl2, cmd, baddr, NULL, bsize,
	        /* now */now, /* pudata */NULL, /* repl addr */NULL) ;
	    if (cmd == Read)
	        return lat ;
	        else
	    {
/* FIXME: unlimited write buffers */
	        return 0 ;
	    }
	}
	else
	{
/* access main memory */
	    if (cmd == Read)
	        return mem_access_latency(bsize) ;
	        else
	    {
/* FIXME: unlimited write buffers */
	        return 0 ;
	    }
	}
}
/* end subroutine (dl1_access_fn) */


/* l2 data cache block miss handler function */
static unsigned int			/* latency of block access */
dl2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
md_addr_t baddr,		/* block address to access */
int bsize,		/* size of block to access */
struct cache_blk_t *blk,	/* ptr to block in upper level */
tick_t now)		/* time of access */
{
/* this is a miss to the lowest level, so access main memory */
	if (cmd == Read)
	    return mem_access_latency(bsize) ;
	    else
	{
/* FIXME: unlimited write buffers */
	    return 0 ;
	}
}
/* end subroutine (dl2_access_fn) */


/* l1 inst cache l1 block miss handler function */
static unsigned int			/* latency of block access */
il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
md_addr_t baddr,		/* block address to access */
int bsize,		/* size of block to access */
struct cache_blk_t *blk,	/* ptr to block in upper level */
tick_t now)		/* time of access */
{
	unsigned int lat ;

	if (cache_il2)
	{
/* access next level of inst cache hierarchy */
	    lat = cache_access(cache_il2, cmd, baddr, NULL, bsize,
	        /* now */now, /* pudata */NULL, /* repl addr */NULL) ;
	    if (cmd == Read)
	        return lat ;
	        else
	        panic("writes to instruction memory not supported") ;
	}
	else
	{
/* access main memory */
	    if (cmd == Read)
	        return mem_access_latency(bsize) ;
	        else
	        panic("writes to instruction memory not supported") ;
	}
}
/* end subroutine (il1_access_fn) */


/* l2 inst cache block miss handler function */
static unsigned int			/* latency of block access */
il2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
md_addr_t baddr,		/* block address to access */
int bsize,		/* size of block to access */
struct cache_blk_t *blk,	/* ptr to block in upper level */
tick_t now)		/* time of access */
{


/* this is a miss to the lowest level, so access main memory */
	if (cmd == Read)
	    return mem_access_latency(bsize) ;
	    else
	    panic("writes to instruction memory not supported") ;
}
/* end subroutine (il2_access_fn) */


/* * TLB miss handlers */

/* inst cache block miss handler function */
static unsigned int			/* latency of block access */
itlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
md_addr_t baddr,		/* block address to access */
int bsize,		/* size of block to access */
struct cache_blk_t *blk,	/* ptr to block in upper level */
tick_t now)		/* time of access */
{
	md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data ;

/* no real memory access, however, should have user data space attached */
	assert(phy_page_ptr) ;

/* fake translation, for now... */
	*phy_page_ptr = 0 ;

/* return tlb miss latency */
	return tlb_miss_lat ;
}
/* end subroutine (itlb_access_fn) */


/* data cache block miss handler function */
static unsigned int			/* latency of block access */
dtlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
md_addr_t baddr,	/* block address to access */
int bsize,		/* size of block to access */
struct cache_blk_t *blk,	/* ptr to block in upper level */
tick_t now)		/* time of access */
{
	md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data ;

/* no real memory access, however, should have user data space attached */
	assert(phy_page_ptr) ;

/* fake translation, for now... */
	*phy_page_ptr = 0 ;

/* return tlb miss latency */
	return tlb_miss_lat ;
}
/* end subroutine (dtlb_access_fn) */


/* parse the PE numbers and latencies */
static int peparse(pip,sp,sl,np,lp)
struct proginfo	*pip ;
cchar	*sp ;
int		sl ;
int		*np ;
int		*lp ;
{
	FIELD	fsb ;

	int	rs = SR_OK, i ;
	int	c = 0 ;


	if (sl < 0)
	    sl = strlen(sp) ;

	*np = -1 ;
	*lp = -1 ;
	field_init(&fsb,sp,sl) ;

/* there are no comments within these values so we do not need to check */

	for (i = 0 ; field_get(&fsb,NULL) > 0 ; i += 1) {

	    if ((fsb.flen > 0) && (strcmp(fsb.fp,"-") != 0)) {

	        switch (i) {

	        case 0:
	            c += 1 ;
	            rs = cfnumui(fsb.fp,fsb.flen,np) ;

	            break ;

	        case 1:
	            c += 1 ;
	            rs = cfnumi(fsb.fp,fsb.flen,lp) ;

	            break ;

	        } /* end switch */

	    } /* end if (got a non-zero field) */

	    if (rs < 0)
	        break ;

	} /* end for */

	field_free(&fsb) ;

	return (rs >= 0) ? c : rs ;
}
/* end subroutine (peparse) */


/* parse the Local-Exec operation (OP) codes */
static int leparse(pip,lip,sp,sl)
struct proginfo	*pip ;
struct ssinfo	*lip ;
cchar	*sp ;
int		sl ;
{
	FIELD	fsb ;

	int	rs = SR_OK, i ;
	int	op ;
	int	c = 0 ;


	if (sl < 0)
	    sl = strlen(sp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	eprintf("leparse: sp=>%w<\n",sp,sl) ;
#endif

	field_init(&fsb,sp,sl) ;

/* there are no comments within these values so we do not need to check */

	while (field_get(&fsb,NULL) >= 0) {

		if (fsb.flen == 0) continue ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	eprintf("leparse: op=>%w<\n",fsb.fp,fsb.flen);
#endif

		rs = cfdeci(fsb.fp,fsb.flen,&op) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	eprintf("leparse: cfdeci() rs=%d \n",rs) ;
#endif

		if (rs < 0)
			break ;

		if (op < 0) {
			rs = SR_INVALID ;
			break ;
		}

		rs = ssinfo_leset(lip,op) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	eprintf("leparse: ssinfo_leset() rs=%d \n",rs) ;
#endif

		if (rs < 0)
			break ;

		c += 1 ;

	} /* end while */

	field_free(&fsb) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	eprintf("leparse: ret rs=%d c=%u\n",rs,c) ;
#endif

	return (rs >= 0) ? c : rs ;
}
/* end subroutine (leparse) */


int icache_latency(pip,ssp,csp,ia)
struct proginfo	*pip ;
SS		*ssp ;
CHECKER		*csp ;
ULONG		ia ;
{
	md_addr_t	aa = ia & (~ (sizeof(md_inst_t) - 1)) ;

	int	lat, i ;


	pip->s.imemreads += 1 ;

	lat = cache_il1_lat ;
	if ((cache_il1 != NULL) && (ia != 0)) {

	    tick_t	ck_cache = ssp->ck ;


	    lat = cache_access(cache_il1, Read, IACOMPRESS(aa),
	        NULL, ISCOMPRESS(sizeof(md_inst_t)), ck_cache,
	        NULL, NULL) ;

	} /* end if (had i-cache) */

	if ((lat < 0) || (lat > MEMLATSCREW)) {
	    lat = (lat >= 0) ? MEMLATSCREW : 0 ;
	    pip->s.imemlatscrew += 1 ;
	}

/* update the density for i-cache latencies */

#if	CF_DENSITY

#if	CF_DENSITYUP
		density_update(&pip->s.imlat,lat) ;
#endif

#else /* CF_DENSITY */

	if (pip->s.imemlat != NULL) {

	    if (lat > pip->s.imemlatmax)
	        pip->s.imemlatmax = lat ;

	    i = (lat >= 0) ? lat : 0 ;
	    if (i > (pip->s.imemlatlen - 1))
	        i = (pip->s.imemlatlen - 1) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        eprintf("icache_latency: lat=%u\n",i) ;
#endif

	    pip->s.imemlat[i] += 1 ;

	} /* end if (had i-cache density) */

#endif /* CF_DENSITY */

	return lat ;
}
/* end subroutine (icache_latency) */


int dcache_latency(pip,ssp,csp,da,f_read)
struct proginfo	*pip ;
SS		*ssp ;
CHECKER		*csp ;
ULONG		da ;
int		f_read ;
{
	md_addr_t	aa = da & (~ (sizeof(md_addr_t) - 1)) ;

	int	lat, i ;


	if (f_read)
	    pip->s.dmemreads += 1 ;

	else
	    pip->s.dmemwrites += 1 ;


	lat = cache_dl1_lat ;
	if ((cache_dl1 != NULL) && (aa != 0)) {

	    tick_t	ck_cache = ssp->ck ;


	    enum mem_cmd cmd ;		/* access type, Read or Write */


	    cmd = (f_read) ? Read : Write ;
	    lat = cache_access(cache_dl1, cmd, IACOMPRESS(aa),
	        NULL, ISCOMPRESS(sizeof(md_addr_t)), ck_cache,
	        NULL, NULL) ;

	} /* end if (had d-cache) */

	if ((lat < 0) || (lat > MEMLATSCREW)) {
	    lat = (lat >= 0) ? MEMLATSCREW : 0 ;
	    pip->s.dmemlatscrew += 1 ;
	}

	if (! f_read)
	    lat = 0 ;

/* update the density for d-cache read latencies */

#if	CF_DENSITY

#if	CF_DENSITYUP
	if (f_read)
		density_update(&pip->s.dmlat,lat) ;
#endif

#else /* CF_DENSITY */

	if (f_read && (pip->s.dmemlat != NULL)) {

	    if (lat > pip->s.dmemlatmax)
	        pip->s.dmemlatmax = lat ;

	    i = (lat >= 0) ? lat : 0 ;
	    if (i > (pip->s.dmemlatlen - 1))
	        i = (pip->s.dmemlatlen - 1) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(6))
	        eprintf("dcache_latency: lat=%u\n",i) ;
#endif

	    pip->s.dmemlat[i] += 1 ;

	} /* end if (latency density update) */

#endif /* CF_DENSITY */

	return lat ;
}
/* end subroutine (dcache_latency) */


#ifdef	COMMENT

int bpredictor_lookup(pip,ssp,msp,asp)
struct proginfo	*pip ;
SS		*ssp ;
CHECKERR	*csp ;
SSAS		*asp ;
{
	md_addr_t	branch_PC ;
	md_addr_t	target_PC ;
	md_addr_t	pred_PC ;

	struct bpred_update_t	*urp ;

	enum md_opcode	opc ;

	int	stack_idx ;
	int	f_taken, f_predtaken, f_predcorrect ;


	opc = casp->n.op ;
	urp = &casp->n.update_rec ;
	branch_PC = casp->n.ia ;
	target_PC = casp->n.ta ;

	pred_PC = bpred_lookup(pred,
	    /* branch addr */ branch_PC,
	    /* target */target_PC,
	    /* inst opcode */opc,
	    /* call? */MD_IS_CALL(opc),
	    /* return? */MD_IS_RETURN(opc),
	    /* stash an update ptr */ urp,
	    /* stash return stack ptr */&stack_idx) ;

/* valid address returned from branch predictor? */
	if (! pred_PC)
	{
/* no predicted taken target, attempt not taken target */
	    pred_PC = regs.regs_PC + sizeof(md_inst_t) ;

	}

	f_taken = 
	    (regs.regs_NPC != (regs.regs_PC + sizeof(md_inst_t))) ;
	f_predtaken = 
	    (pred_PC != (regs.regs_PC + sizeof(md_inst_t))) ;
	f_predcorrect = 
	    (pred_PC == regs.regs_NPC) ;

	return f_taken ;
}
/* end subroutine */


int bpredictor_update(pip,msp,asp)
struct proginfo	*pip ;
CHECKER		*csp ;
SSAS		*asp ;
{
	md_addr_t	branch_PC ;
	md_addr_t	target_PC ;
	md_addr_t	pred_PC ;

	struct bpred_update_t	*urp ;

	enum md_opcode	opc ;

	int	stack_idx ;
	int	f_taken, f_predtaken, f_predcorrect ;


	bpred_update(pred,
	    /* branch addr */regs.regs_PC,
	    /* resolved branch target */regs.regs_NPC,
	    /* taken? */ (regs.regs_NPC != (regs.regs_PC +
	    sizeof(md_inst_t))),
	    /* pred taken? */ (pred_PC != (regs.regs_PC +
	    sizeof(md_inst_t))),
	    /* correct pred? */ (pred_PC == regs.regs_NPC),
	    /* opcode */opc,
	    /* predictor update pointer */&update_rec) ;

	pip->s.bp_predgiven += 1 ;
	if (f_taken)
	    pip->s.bp_taken += 1 ;

	if (f_predtaken)
	    pip->s.bp_predtaken += 1 ;

	if (f_predcorrect)
	    pip->s.bp_predcorrect += 1 ;

	return 0 ;
}
/* end subroutine (bpredictor_update) */

#endif /* COMMENT */



