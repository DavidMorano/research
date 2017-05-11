/* iw */

/* Levo Instruction Window */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/



#ifndef	IW_INCLUDE
#define	IW_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines */

#define	IW		struct iw_head
#define	IW_INITARGS	struct iw_initargs



#include	<sys/types.h>

#include	<bio.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"statemips.h"
#include	"shiftregl.h"

#include	"levoinfo.h"
#include	"bus.h"
#include	"bustrace.h"

#include	"lpe.h"
#include	"las.h"
#include	"llb.h"
#include	"lifetch.h"
#include	"lbtrb.h"
#include	"lwq.h"
#include	"regfilter.h"
#include	"alirgf.h"
#include	"bustest.h"
#include	"bustrace.h"




struct iw_sanitycall {
	int	(*func)() ;
	void	*arg ;
} ;

struct iw_initargs {
	struct iw_sanitycall	sanity ;
	struct statemips	*smp ;
} ;

struct iw_statistics {
	ULONG	issue_col ;		/* columns */
	ULONG	issue_unused ;		/* unused issues */
	ULONG	issue_null ;		/* NULL issues */
	ULONG	issue_instr ;		/* instruction issues */

	ULONG	commit_col ;		/* column commitments */
	ULONG	commit_unused ;		/* unused commits */
	ULONG	commit_null ;		/* NULL commits */
	ULONG	commit_instr ;		/* instruction commits */
	ULONG	commit_ienabled ;	/* instruction enabled commits */
	ULONG	commit_idisabled ;	/* instruction disabled commits */
} ;

struct iw_path {
	int	sgci ;			/* starting SG column index */
	int	ci ;			/* starting physical AS column */
	int	rfi ;			/* starting register filter index */
	int	ncols ;			/* number of columns allocated */
} ;

struct iw_rfusage {
	UINT	a : 1 ;			/* lower index RF unit */
	UINT	b : 1 ;			/* higher index RF unit */
} ;

struct iw_sgcol {
	struct iw_rfusage	rfused ;
	int	rfti, rfbi ;		/* REGFILTER indices */
} ;

struct iw_colflags {
	UINT	branch : 1 ;		/* has at least one branch */
	UINT	used : 1 ;		/* is used at all */
	UINT	kill : 1 ;		/* should be unused */
} ;

struct iw_col {
	struct iw_colflags	f ;
	int	nbranches ;		/* number of branches in column */
	int	path ;			/* path for this column */
	int	tt ;			/* starting TT for this column */
} ;

struct iw_sflags {
	UINT	targetout : 1 ;		/* a GOTO is in progress */
	UINT	committed : 1 ;		/* committed on last clock transition */
	UINT	exit : 1 ;		/* exit on this clock */
} ;

struct iw_allocs {
	struct iw_sgcol		*sgcols ;
	struct iw_col		*cols ;
	struct iw_path		*paths ;
	BUS			*mbuses ;	/* convenience variable */
} ;

struct iw_state {
	struct iw_sgcol		*sgcols ;	/* SG column tracking */
	struct iw_col		*cols ;		/* column tracking */
	struct iw_path		*paths ;	/* path tracking */
	struct iw_sflags	f ;	/* machine state flags */
	int		count ;		/* commit counter */
	UINT		checksum ;	/* debugging */
	UINT		faddress ;	/* ???? an Ali thing */
	UINT		ta ;		/* target address */
} ;

struct iw_flags {
	UINT	commit : 1 ;		/* we did a commit */
	UINT	shift : 1 ;		/* signal a machine "shift" */
	UINT	noload : 1 ;		/* do not load in this clock */
	UINT	loadlb : 1 ;		/* do a new load of LLB */
	UINT	targetout : 1 ;		/* a GOTO happened */
	UINT	rfbtrace : 1 ;
	UINT	rbbtrace : 1 ;
	UINT	pfbtrace : 1 ;
	UINT	mfbtrace : 1 ;
	UINT	mbbtrace : 1 ;
} ;

struct iw_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	LSIM			*mip ;
	struct statemips	*smp ;
	struct levoinfo		*lip ;

	struct iw_sanitycall	sanity ;
	SHIFTREGL		ba ;	/* bus activity */

	struct iw_flags		f ;	/* non-state flags */
	struct iw_allocs	a ;	/* state memory allocations */
	struct iw_state		c, n ;	/* machine state */
	struct iw_statistics	s ;	/* statistics */

	char			*mapalloc ;

	REGFILTER	*rfs ;		/* register filters */
	LIFETCH		ourfetch ;
	LLB		ourload ;
	LBTRB		lbtrb ;		/* Levo Branch Tracking */
	LWQ		wqueue ;	/* Levo Write Queue */
	BUS		*mfbuses ;	/* memory forwarding buses */
	BUS		*mbbuses ;	/* memory backwarding buses */
	BUS		*rfbuses ;	/* register forwarding */
	BUS		*rbbuses ;	/* register backwarding */
	BUS		*pfbuses ;	/* predicate forwarding */
	LAS		*lases ;
	LPE		*lpes ;

	BUSTRACE	rfbtrace ;
	BUSTRACE	rbbtrace ;
	BUSTRACE	pfbtrace ;
	BUSTRACE	mfbtrace ;
	BUSTRACE	mbbtrace ;

	BUSTEST		*mytests ;	/* bus test components ! */
	ALIRGF		alirgf;		/* temporary register file */

	UINT		rftype ;	/* register forwarding type */
	UINT		wmfinter ;	/* EW MF interleave schedule */
	UINT		wmbinter ;	/* EW MB interleave schedule */
	UINT		waitcount ;	/* Alireza commitment wait count */

	size_t		mapsize ;

	int		nwmfbuses ;	/* number of EW MF buses */
	int		nwmbbuses ;	/* number of EW MB buses */
	int		lbi ;		/* LB load index (temporary) */
	int		ntestbuses ;	/* number test buses */
} ;



#if	(! defined(IW_MASTER)) || (IW_MASTER == 0)

extern int	iw_init(IW *,struct proginfo *,PARAMFILE *,
			LSIM *,struct levoinfo *lip,IW_INITARGS *) ;
extern int	iw_free(IW *) ;
extern int	iw_comb(IW *,int) ;
extern int	iw_clock(IW *) ;
extern int	iw_needshift(IW *) ;
extern int	iw_shift(IW *) ;
extern int	iw_checkstate(IW *) ;
extern int	iw_sanitycheck(IW *) ;
extern int	iw_statfile(IW *,bfile *) ;

#endif /* declarations */


#endif /* IW_INCLUDE */



