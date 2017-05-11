/* ssas */



#ifndef	SSAS_INCLUDE
#define	SSAS_INCLUDE	1



/* objects */

#define	SSAS		struct ssas_head
#define	SSAS_INITARGS	struct ssas_initargs
#define	SSAS_INFO	struct ssas_info
#define	SSAS_STATS	struct ssas_stats
#define	SSAS_MEMOP	struct ssas_memop




#include	<sys/types.h>

#include	<vsystem.h>

#include	"localmisc.h"

#include	"ss.h"
#include	"machstate.h"
#include	"sscommon.h"
#include	"ssinfo.h"
#include	"ssiw.h"
#include	"sspe.h"
#include	"bpred.h"



#ifdef	SSCOMMON_MAXISRESIDENCY
#define	SSAS_NDENVALID		SSCOMMON_MAXISRESIDENCY
#else
#define	SSAS_NDENVALID		500
#endif



/* arguments specific to an AS for initialization */
struct ssas_initargs {
	SSIW	*wp ;
	SSPE	*pep ;
	int	asid ;
	int	rftype ;
} ;

struct ssas_stats {
	ULONG	ntexec ;		/* total executions */
	ULONG	nrexec ;		/* re-executions */
	ULONG	nnorexec ;		/* regular executions */
	ULONG	nmemexec ;		/* memory executions */
	ULONG	nfuiss ;		/* FU issue executions */
	ULONG	nfuret ;		/* FU returns executions */
	ULONG	nfuacc ;		/* FU returns accepted */
	uint	isres[SSAS_NDENVALID + 1] ;
	uint	vcount ;		/* valid clock count */
} ;

struct ssas_oper {
	OPERAND		fg ;
	ULONG		opv ;		/* old previous value */
} ;

struct ssas_sflags {
	uint	v : 1 ;		/* valid (used) */
	uint	pred_taken : 1 ;
	uint	pred_correct : 1 ;
	uint	executed : 1 ;	/* executed at least once */
	uint	executing : 1 ;	/* currently executing */
	uint	ibrpred : 1 ;	/* initial branch prediction */
	uint	export : 1 ;	/* some operand needs to be exported */
	uint	enabled : 1 ;	/* instruction was enabled */
	uint	cf : 1 ;	/* control flow instruction */
	uint	cfdir : 1 ;	/* control flow (direct) */
	uint	cfind : 1 ;	/* control flow (indirect) */
	uint	cfsub : 1 ;	/* control flow (subroutine) */
	uint	cfcond : 1 ;	/* control flow (conditional) */
	uint	taken : 1 ;	/* committed branch outcome T=1 NT=0 */
	uint	mem : 1 ;	/* memory operation */
	uint	memload : 1 ;	/* memory load */
	uint	memstore : 1 ;	/* memory store */
	uint	memunalign : 1 ;
	uint	haveiops : 1 ;	/* have all of our input operands */
	uint	needexec : 1 ;	/* needs a new execution */
	uint	nmem : 2 ;	/* number of memory ops */
	uint	localexec : 1 ;	/* does execution locally */
} ;

struct ssas_state {
	struct bpred_update_t update_rec ;
	ULONG	in ;		/* committed instruction (from checker) */
	ULONG	ia ;		/* instruction address */
	ULONG	ta ;		/* CF target address (if available) */
	struct ssas_sflags	f, set, clr ;
	struct predicate	pred ;
	struct operand		opsi[MACHSTATE_MAXOPSI] ;
	struct operand		opso[MACHSTATE_MAXOPSO] ;
	uint	instrword ;	/* instruction bits */
	uint	op ;		/* instruction operation */
	uint	class ;		/* instruction class */
	uint	flags ;		/* instruction flags */
	uint	nopsi, nopso ;
	uint	nexec ;		/* number times executed */
	uint	exectimer ;	/* execution timer */
	uint	c_valid ;	/* valid counter */
	uint	mincounter ;	/* minimum commit counter */
	int	tt ;		/* this instruction time-tag */
	int	path ;		/* this instruction path */
} ;

struct ssas_flags {
	uint	loaded : 1 ;	/* it's been loaded */
	uint	retire : 1 ;	/* will be retired */
	uint	commit : 1 ;	/* do a commitment if nothing else */
	uint	shift : 1 ;	/* needs to be shifted */
	uint	opchanged : 1 ;
	uint	needexec : 1 ;	/* needs a new execution */
	uint	exectimer : 1 ;	/* was set */
	uint	wrongpath : 1 ;
} ;

struct ssas_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	SS			*mip ;
	struct ssinfo		*lip ;
	SSIW			*wp ;
	SSPE			*pep ;

	struct ssas_state	c, n ;	/* state */
	struct ssas_flags	f ;
	struct ssas_stats	s ;
	int	asid ;			/* our physical ID */
	int	rftype ;		/* register forwarding type */
	int	npred ;			/* number of input predicate slots */
	int	pci ;			/* physical column index */
	int	psgi ;			/* physical SG index */

	int	sgrow ;			/* our SG row */
	int	asrow ;			/* our AS row in the whole window */
	int	naspcol ;		/* total ASes per column */
	int	logrows ;		/* log base 2 of total AS rows */

	uint	rfmodmask ;		/* RF modulus mask */
	uint	rbmodmask ;		/* RB modulus mask */

	uint	shiftamount ;		/* amount to shift by (on a "shift") */
} ;

/* instruction commitment information */
struct ssas_info {
	ULONG	in ;		/* committed instruction (from checker) */
	ULONG	ia ;		/* current instruction address */
	ULONG	ta ;		/* control flow target address */
	uint	instrword ;	/* the original instruction word */
	uint	op ;		/* instruction operations */
	uint	class ;		/* instruction class */
	uint	flags ;		/* instruction flags */
	uint	f_enabled : 1 ;	/* instruction was enabled */
	uint	f_cf : 1 ;	/* control flow type of instruction */
	uint	f_cfind : 1 ;	/* control flow (indirect) */
	uint	f_cfsub : 1 ;	/* control flow (subroutine) */
	uint	f_cfcond : 1 ;	/* control flow (conditional) */
	uint	f_taken : 1 ;	/* commited branch outcome T=1 NT=0 */
} ;

struct ssas_memop {
	ULONG	a ;
	ULONG	v ;
	uint	dp : 16 ;
	uint	size : 6 ;
	uint	type : 4 ;
	uint	f_read : 1 ;
	uint	f_v : 1 ;
} ;




/* public subroutine definitions here */

#if	(! defined(SSAS_MASTER)) || (SSAS_MASTER == 0)

extern int ssas_init(SSAS *,struct proginfo *,SS *,
		struct ssinfo *, SSAS_INITARGS *) ;
extern int ssas_free(SSAS *) ;
extern int ssas_comb(SSAS *,int) ;
extern int ssas_clock(SSAS *) ;
extern int ssas_shift(SSAS *,int) ;
extern int ssas_readyload(SSAS *) ;
extern int ssas_load(SSAS *,SSAS *,int) ;
extern int ssas_opmatch(SSAS *,OPERAND *) ;
extern int ssas_opexport(SSAS *,int,OPERAND **) ;
extern int ssas_opget(SSAS *,int,OPERAND **) ;
extern int ssas_opsnoop(SSAS *,OPERAND *) ;
extern int ssas_fureturn(SSAS *,SSPE_OPV *) ;
extern int ssas_readyretire(SSAS *) ;
extern int ssas_retire(SSAS *) ;
extern int ssas_info(SSAS *,SSAS_INFO *) ;
extern int ssas_used(SSAS *) ;
extern int ssas_getia(SSAS *,ULONG *) ;
extern int ssas_audit(SSAS *) ;
extern int ssas_dump(SSAS *, bfile *) ;
extern int ssas_getmemop(SSAS *,SSAS_MEMOP *) ;

#ifdef	COMMENT

extern int	ssas_xmlinit(SSAS *,XMLINFO *) ;
extern int	ssas_xmlout(SSAS *,XMLINFO *) ;
extern int	ssas_xmlfree(SSAS *,XMLINFO *) ;

#endif /* COMMENT */

#endif /* SSAS_MASTER */

#endif /* SSAS_INCLUDE */



