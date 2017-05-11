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




#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"

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
} ;

struct iw_statistics {
	LONG	colcommits ;		/* column commitments */
	LONG	ucommits ;		/* unused committed */
	LONG	ncommits ;		/* NULL commits */
	LONG	iecommits ;		/* enabled instruction commits */
	LONG	idcommits ;		/* disabled instruction commits */
	LONG	icommits ;		/* total instructions committed */
} ;

struct iw_path {
	int	sgci ;			/* starting SG column index */
	int	ci ;			/* starting physical AS column */
	int	ncols ;			/* number of columns allocated */
} ;

struct iw_sgcolumn {
	int	rfti, rfbi ;		/* REGFILTER indices */
} ;

struct iw_columnflags {
	uint	branch : 1 ;		/* has at least one branch */
	uint	used : 1 ;		/* is used at all */
	uint	kill : 1 ;		/* should be unused */
} ;

struct iw_column {
	struct iw_columnflags	f ;
	int	nbranches ;		/* number of branches in column */
	int	path ;			/* path for this column */
	int	tt ;			/* starting TT for this column */
} ;

struct iw_sflags {
	uint	targetout : 1 ;		/* a GOTO is in progress */
	uint	committed : 1 ;		/* committed on last clock transition */
} ;

struct iw_allocs {
	struct iw_sgcolumn	*sgcols ;
	struct iw_column	*cols ;
	struct iw_path		*paths ;
	BUS			*mbuses ;	/* convenience variable */
} ;

struct iw_state {
	struct iw_sgcolumn	*sgcols ;	/* SG column tracking */
	struct iw_column	*cols ;		/* column tracking */
	struct iw_path		*paths ;	/* path tracking */
	struct iw_sflags	f ;	/* machine state flags */
	int		count ;		/* commit counter */
	uint		checksum ;	/* debugging */
	uint		faddress ;	/* ???? an Ali thing */
	uint		ta ;		/* target address */
} ;

struct iw_flags {
	uint	commit : 1 ;		/* we did a commit */
	uint	shift : 1 ;		/* signal a machine "shift" */
	uint	noload : 1 ;		/* do not load in this clock */
	uint	loadlb : 1 ;		/* do a new load of LLB */
	uint	targetout : 1 ;		/* a GOTO happened */
	uint	rfbtrace : 1 ;
	uint	rbbtrace : 1 ;
	uint	pfbtrace : 1 ;
	uint	mfbtrace : 1 ;
	uint	mbbtrace : 1 ;
} ;

struct iw_head {
	unsigned long	magic ;

	struct proginfo	*pip ;
	LSIM		*mip ;
	struct levoinfo	*lip ;

	struct iw_sanitycall	sanity ;

	struct iw_flags		f ;	/* non-state flags */
	struct iw_allocs	a ;	/* state memory allocations */
	struct iw_state		c, n ;	/* machine state */
	struct iw_statistics	s ;	/* statistics */

	REGFILTER	*rfs ;		/* register filters */
	LIFETCH		ourfetch ;
	LLB		ourload ;
	LBTRB		lbtrb ;		/* Levo Branch Tracking */
	LWQ		wqueue ;	/* Levo Write Queue */
	BUS		*mfbuses ;	/* memory forwarding buses */
	BUS		*mbbuses ;	/* memory backwarding buses */
	BUS		*rfbuses ;	/* register backwarding */
	BUS		*rbbuses ;	/* register forwarding */
	BUS		*pfbuses ;	/* predicate forwarding */
	LAS		*lases ;
	LPE		*lpes ;

	BUSTRACE	rfbtrace ;
	BUSTRACE	rbbtrace ;
	BUSTRACE	pfbtrace ;
	BUSTRACE	mfbtrace ;
	BUSTRACE	mbbtrace ;

	BUSTEST		*mytests ;
	ALIRGF		alirgf;		/* temporary register file */

	uint		rftype ;	/* register forwarding type */
	uint		wmfinter ;	/* EW MF interleave schedule */
	uint		wmbinter ;	/* EW MB interleave schedule */
	uint		waitcount ;	/* Alireza commitment wait count */

	int		nwmfbuses ;	/* number of EW MF buses */
	int		nwmbbuses ;	/* number of EW MB buses */
	int		lbi ;		/* LB load index (temporary) */

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

#endif /* declarations */


#endif /* IW_INCLUDE */



