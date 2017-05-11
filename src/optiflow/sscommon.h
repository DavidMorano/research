/* sscommon */


#ifndef	SSCOMMON_INCLUDE
#define	SSCOMMON_INCLUDE		1



#include	<sys/types.h>

#include	"localmisc.h"

#include	"sscommon.h"



#define	SSCOMMON_MAXPREDTARGETS		4
#define	SSCOMMON_MAXISRESIDENCY		500



/* levo operand types */

#define	OPERAND_TNONE		0
#define	OPERAND_TREG		1
#define	OPERAND_TMEM		2
#define	OPERAND_TPRED		3


/* objects */

#define	PRED			struct predicate

#define	OPERAND			struct operand

/* operand operation types */

#define	OPERAND_OSTORE		0
#define	OPERAND_ONULLIFY	1
#define	OPERAND_OFAULT		2
#define	OPERAND_OCOMMIT		3




/* dynamic predicate related stuff (DAM) */
struct predicate_region {
	int	tt ;
	int	pr ;			/* region predicate value */
	uint	f_v : 1 ;		/* valid */
} ;

struct predicate_target {
	int	tt ;
	uint	f_v : 1 ;		/* this entry valid */
} ;

/* here is predicate structure for use by an AS (DAM) */
struct predicate {
	struct predicate_target	btp[SSCOMMON_MAXPREDTARGETS] ;
	struct predicate_region	r ;
	int	tt ;			/* last branch target invalidation */
	int	pathid ;
	uint	f_ov : 1 ;		/* branch target table overflowed */
} ;


/* operand related stuff */
struct operand_flags {
	uint	used : 1 ;		/* operand in use */
	uint	v : 1 ;			/* operand valid */
	uint	requested : 1 ;		/* operand requested (initiator) */
	uint	changed : 1 ;		/* value changed */
	uint	export : 1 ;		/* needs to be exported (output) */
	uint	wanted : 1 ;		/* someone wants it (output) */
	uint	nullify : 1 ;		/* NULLIFY address is valid */
	uint	fault : 1 ;		/* memory fault occurred */
} ;

/* here is an operand structure (DAM) */
struct operand {
	ULONG	na ;			/* NULLIFY address of operand */
	ULONG	a ;			/* address of operand */
	ULONG	v ;			/* new value of operand */
	ULONG	pv ;			/* previous value of operand */
	ULONG	ov ;			/* old value */
	ULONG	opv ;			/* old previous value */
	struct operand_flags		f, set, clr ;
	int	ftt ;			/* forwarder's TT */
	int	tt ;			/* TT of operand */
	int	seq ;			/* sequence number */
	int	frseq ;			/* forward-receive sequence */
	uint	dp : 16 ;		/* data present bits (memory) */
	uint	size : 6 ;		/* operand size in bytes (memory) */
	uint	path : 5 ;		/* path-ID of where it was from */
	uint	type : 4 ;		/* type of operand */
	uint	trans : 4 ;		/* transaction type */
} ;


/* instruction classes */
enum iclass {
	iclass_none,
	iclass_ialu,
	iclass_imult,
	iclass_idiv,
	iclass_fadd,
	iclass_fcmp,
	iclass_fcvt,
	iclass_fmult,
	iclass_fdiv,
	iclass_fsqrt,
	iclass_ld,
	iclass_st,
	iclass_overlast
} ;



#endif /* SSCOMMON_INCLUDE */



