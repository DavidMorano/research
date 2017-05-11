/* las */

/* Levo Active Station */


/* revision history :

	= 00/02/04, Dave Morano

	Module was originally written for the LEVO simulator LEVOSIM.


	- 01/09/26, Dave Morano

	I added some code to support the XML state trace.


*/



#ifndef	LAS_INCLUDE
#define	LAS_INCLUDE	1



/* exported object defines */

#define	LAS		struct las_head
#define	LAS_INITARGS	struct las_initargs
#define	LAS_COMMITINFO	struct las_commitinfo



#include	<sys/types.h>
#include	<limits.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"xmlinfo.h"

#include	"levoinfo.h"
#include	"bus.h"
#include	"lpe.h"
#include	"lbusint.h"
#include 	"llb.h"
#include 	"lbtrb.h"
#include 	"lwq.h"
#include 	"alirgf.h" 
#include 	"instrclass.h" 



/* cycles load inst. would wait before reissuing a new data request */
#define LAS_LOAD_WATCHDOG   15  

typedef enum {
	IDLE=0,
	UNUSED,
	WAIT_NRC,
	WAIT_RC
} las_logic_fsm;


/* the enumerations for the decision table thingy */

typedef enum {
	SEND_VALUE=0, 
	SEND_NULLIFY, 
	RECV_NULLIFY, 
	REQ_VALUE, 
	RECV_REQ_VALUE, 
	RECV_VALUE, 
	EXEC_ALU, 
	EXEC_ST, 
	EXEC_LD, 
	EXEC_BR, 
	REEXEC,
	WAITON_VALUE
} las_command;

#define NCOMMANDS 12

typedef enum {
	MBWB=0, 
	MFWB, 
	RBWB, 
	RFWB, 
	PFWB, 
	PE
} las_bus_interface;

#define NBUSES 6

typedef enum {
	DUMMY = 0, 
	SRC1, 
	SRC2, 
	SRC3, 
	SRC4, 
	SRC5, 
	DST1, 
	DST2, 
	DST3, 
	MEM, 
	BCOND, 
	OLDDST1, 
	OLDDST2, 
	OLDDST3, 
	OLDMEMD, 
	OLDMEMA
} las_storage ;

#define NSTORAGES	(OLDMEMA + 1)




struct las_sanitycall {
	int	(*func)() ;
	void	*arg ;
} ;

/* arguments specific to an AS for initialization */
struct las_initargs {
	struct las_sanitycall	sanity ;
	LWQ		*lwqp ;
	LLB		*llbp ;
	LBTRB		*btrb ;
	ALIRGF		*alirgf;
	LPE		*lpes ;
	BUS		*mfbuses, *mbbuses ;
	BUS		*rfbuses, *rbbuses, *pfbuses ;
	uint		mfinter, mbinter ;
	int		asid ;
	int		rftype ;
	int		pci, psgi ;
	int		rfbi, rbbi, pfbi ;
	int		busid ;
} ;

/* instruction commitment information */
struct las_commitinfo {
	uint	ia ;		/* current instruction address */
	uint	instr ;		/* the original instruction word */
	uint	iclass ;	/* Dave's instruction class */
	uint	opclass ;	/* regular OPCLASS */
	uint	btype ;		/* control flow change type (Marcos) */
	uint	ta ;		/* control flow target address */
	uint	f_taken : 1 ;	/* commited branch outcome T=1 NT=0 */
	uint	f_enabled : 1 ;	/* instruction was enabled */
	uint	f_cf : 1 ;	/* is a control type of instruction */
} ;

/* a single temporary bus read register buffer */
struct las_busreg {
	LFLOWGROUP	fg ;		/* the data */
	int		f_v : 1 ;	/* valid bit */
} ;

/* operand register */
struct las_reg {
	uint	val ;		/* data value */
	uint	oldd ;		/* old data value */
	uint	a ;		/* address */
	int	tt ;		/* snooping time tag */
	int	latesttt ; 	/* time tag mask */
	uint	ftseq, frseq ;	/* forwarding sequence numbers */
	uint	btseq, brseq ;	/* backwarding sequence numbers */
	uint	trans : 8 ;	/* transaction type */
	uint	pathid : 4 ;
	uint	valid : 1 ;	/* register valid (presently unused ?) */
};

struct pred_regs {
	int	num_of_cpin_regs;
	struct las_reg pin;	/* input predicate regsiter */
	struct las_reg *cpin;	/* input canceling predicate registers */
	struct las_reg pout;
	struct las_reg cpout;
};

struct busintgrp {
	struct proginfo	*pip ;		/* debugging */
	int		f_fwd ;
	int		bi, modulus, num ;
	LBUSINT		*head ;
	LBUSINT		*outbus ;
};


struct las_packet {
/*	char	name[10]; */

#ifdef	COMMENT /* never used */
	las_command	command ;
	las_storage	storage ;
#endif

	int	d;
	int	a;
	int	tt;
	uint	pathid : 4 ;
	uint	trans : 8 ;
	uint	dp : 5 ;
	uint	f_resp : 1 ;
	uint	valid : 1 ;
} ;


struct las_state {
	struct las_reg	src1 ;
	struct las_reg	src2 ;
	struct las_reg	src3 ;
	struct las_reg	src4 ;
	struct las_reg	src5 ;
	struct las_reg	dst1 ;
	struct las_reg	dst2 ;
	struct las_reg	dst3 ;
	struct las_reg	cnst;
	struct las_reg	mem ;
	struct las_reg	old_mem ;	/* old memory address and value */
	struct pred_regs	pregs;
	struct las_packet	event_table[NCOMMANDS][NBUSES][NSTORAGES];
	las_logic_fsm	asstate;
	uint	faddr;			/* fetch address */
	uint	instrword ;		/* original MIPS instruction word */
	uint	opclass ;		/* pseudo opcode class of instruction */
	uint	opexec ;		/* pseudo execution opcode */
	uint	pI : 1 ;		/* instruction execution condition */
	uint	bcond : 1 ;		/* branch condition */
	uint	dp : 5 ;		/* data present bits store instrs */
	uint	path : 4 ;		/* path ID */
	int	f_regfileread : 1 ;	/* read the REG file ? */
	int	pe_tnumb;		/* PE track number */
	int	tt ;			/* our time tag */

/* commands */
	char	re_exec_inst;
	char	fw_pred_on_pfwb;

/* some stuff for Dave */
	uint	checksum ;
	uint	ia ;			/* instruction address */
	uint	instr ;			/* original instruction */
	uint	btype ;			/* Marcos-style i-fetch branch type */
	uint	oopexec, oopclass ;	/* original information */
} ;

struct las_flags {
	uint	shift : 1 ;
} ;

struct las_head {
	unsigned long		magic ;
	struct las_sanitycall	sanity ;
	struct las_flags	f ;
	struct las_state	c, n ;	/* AS state (current and next) */

	struct busintgrp rfwbus ;	/* register forwarding buses */
	struct busintgrp rbwbus ;	/* register backwarding buses */
	struct busintgrp pfwbus ;	/* predicate forwarding buses */
	struct busintgrp mbwbus ;	/* memory backward buses */
	struct busintgrp mfwbus ;	/* memory forward buses */

	struct proginfo		*pip ;
	LSIM			*mip;
	struct levoinfo		*lip ;
	struct las_busreg	*busregs ;

	LLB	*llbp ;			/* the Levo i-load buffer */
	LWQ	*lwqp ;			/* the Levo Store Queue */
	LBTRB	*btrb ;			/* the Levo branch tracking buffer */
	LPE	*lpes ;			/* our PE */
	ALIRGF	*rgfp ; 		/* temporary register file */

	int	asid ;			/* our ID */
	int	rftype ;		/* register forwarding type */
	int	npred ;			/* number of input predicate slots */
	int	pci ;			/* physical column index */
	int	psgi ;			/* physical SG index */

/* some miscellaneous values */
	int	sgrow ;			/* our SG row */
	int	asrow ;			/* our AS row in the whole window */
	int	naspcol ;		/* total ASes per column */
	int	logrows ;		/* log base 2 of total AS rows */

	uint	rfmodmask ;		/* RF modulus mask */
	uint	rbmodmask ;		/* RB modulus mask */
	uint	mfwb_intleave;		/* interleaving bits for MFB */
	uint	mbwb_intleave;		/* interleaving bits for MBB */

	int	rfbi ;			/* register-forwarding bus index */
	int	rbbi ;			/* register-backwarding bus index */
	int	pfbi ;			/* predicate-forwarding bus index */	
	int	buspriority;

/* state machine inputs */
	int	pred;			/* predicate of the AS */
	int	wait4src1;		/* what is this ? */
	int	wait4src2;
	int	wait4src3 ;
	int	wait4src4 ;
	int	wait4src5 ;
} ;



/* public subroutine definitions here */

#if	(! defined(LAS_MASTER)) || (LAS_MASTER == 0)

extern int las_init(LAS *,struct proginfo *,LSIM *,
		struct levoinfo *, LAS_INITARGS *) ;
extern int las_free(LAS *) ;
extern int las_comb(LAS *,int) ;
extern int las_clock(LAS *) ;
extern int las_shift(LAS *) ;
extern int las_readycommit(LAS *) ;
extern int las_commit(LAS *,int *,int *,uint *,uint *) ;
extern int las_commitinfo(LAS *,LAS_COMMITINFO *) ;
extern int las_load(LAS *, LLB *, int, int) ;
extern int las_loadfromlb(LAS *, LLB *, int, int, int) ;
extern int las_loadfromas(LAS *, LAS *, int, int) ;
extern int las_unused(LAS *) ;
extern int las_used(LAS *) ;
extern int las_getia(LAS *,uint *) ;
extern int	las_xmlinit(LAS *,XMLINFO *) ;
extern int	las_xmlout(LAS *,XMLINFO *) ;
extern int	las_xmlfree(LAS *,XMLINFO *) ;

extern int	alirgf_write(ALIRGF *,LAS *);
extern int	alirgf_read(ALIRGF *,LAS *);
extern void	gdb(void);

#endif /* LAS_MASTER */


#endif /* LAS_INCLUDE */



