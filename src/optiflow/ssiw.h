/* ssiw */

/* Levo Machine Elements */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/



#ifndef	SSIW_INCLUDE
#define	SSIW_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines */

#define	SSIW		struct ssiw_head
#define	SSIW_INITARGS	struct ssiw_initargs



#include	<sys/types.h>

#include	<bio.h>

#include	"paramfile.h"
#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"ss.h"
#include	"ssinfo.h"
#include	"checker.h"
#include	"ssfetch.h"
#include	"ssreg.h"
#include	"sslsq.h"
#include	"ssas.h"
#include	"sspe.h"





struct ssiw_initargs {
	int		iwsize ;
} ;

struct ssiw_statistics {
	ULONG	issue_unused ;		/* unused issues */
	ULONG	issue_null ;		/* NULL issues */
	ULONG	issue_instr ;		/* instruction issues */
	ULONG	commit_unused ;		/* unused commits */
	ULONG	commit_null ;		/* NULL commits */
	ULONG	commit_instr ;		/* instruction commits */
	ULONG	commit_ienabled ;	/* instruction enabled commits */
	ULONG	commit_idisabled ;	/* instruction disabled commits */
} ;

struct ssiw_allocs {
	int	n ;
} ;

struct ssiw_sflags {
	UINT	targetout : 1 ;		/* a GOTO is in progress */
	UINT	committed : 1 ;		/* committed on last clock transition */
	UINT	exit : 1 ;		/* exit on this clock */
} ;

struct ssiw_state {
	struct ssiw_sflags	f ;	/* machine state flags */
	ULONG		ta ;		/* target address */
	UINT		checksum ;	/* debugging */
	uint		c_wait ;	/* stall wait counter */
	int		aswi, asri ;	/* ASID indices */
	int		nasactive ;	/* ASes active */
	int		c_commit ;	/* commit counter (?) */
	int		c_nocommit ;	/* no-commit clock counter */
	int		checkturn ;	/* checker read index */
} ;

struct ssiw_flags {
	UINT	commit : 1 ;		/* we did a commit */
	UINT	shift : 1 ;		/* signal a machine "shift" */
	UINT	noload : 1 ;		/* do not load in this clock */
	UINT	loadlb : 1 ;		/* do a new load of LLB */
	UINT	targetout : 1 ;		/* a GOTO happened */
} ;

struct ssiw_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	SS			*mip ;

	SSINFO			*lip ;

	struct ssiw_flags	f ;	/* non-state flags */
	struct ssiw_allocs	a ;	/* state memory allocations */
	struct ssiw_state	c, n ;	/* machine state */
	struct ssiw_statistics	s ;	/* statistics */

	SSFETCH		fetch ;
	SSPE		pe ;		/* PE accouting module */
	SSREG		reg ;		/* register file */
	SSLSQ		lsq ;		/* LSQ */
	SSAS		*ssas ;		/* the ASes */

	uint		rftype ;	/* register forwarding type */
	uint		shiftamount ;
} ;



#if	(! defined(SSIW_MASTER)) || (SSIW_MASTER == 0)

extern int	ssiw_init(SSIW *,struct proginfo *,SS *,
			struct ssinfo *,SSIW_INITARGS *) ;
extern int	ssiw_free(SSIW *) ;
extern int	ssiw_comb(SSIW *,int) ;
extern int	ssiw_clock(SSIW *) ;
extern int	ssiw_needshift(SSIW *) ;
extern int	ssiw_shift(SSIW *,int) ;
extern int	ssiw_checkstate(SSIW *) ;
extern int	ssiw_sanitycheck(SSIW *) ;
extern int	ssiw_statfile(SSIW *,bfile *) ;
extern int	ssiw_oprequest(SSIW *,OPERAND *,int) ;
extern int	ssiw_printstats(SSIW *,const char *) ;
extern int	ssiw_dump(SSIW *,bfile *) ;

#endif /* declarations */


#endif /* SSIW_INCLUDE */



