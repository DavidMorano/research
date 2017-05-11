/* traceinfo */


/* revision history :

	= 01/08/06, Dave Morano

	This is needed for skipping instrcutions when using LevoSim.


*/



#ifndef	TRACEINFO_INCLUDE
#define	TRACEINFO_INCLUDE	1



#define	TRACEINFO		struct traceinfo




#include	<sys/types.h>

#include	"localmisc.h"
#include	"exectrace.h"




struct traceinfo_flags {
	uint	enabled : 1 ;		/* master switch */
	uint	regs : 1 ;		/* register addresses */
	uint	regvals : 1 ;		/* register values */
	uint	rsa : 1 ;
	uint	rsv : 1 ;
	uint	mems : 1 ;		/* memory addresses */
	uint	memvals : 1 ;		/* memory values */
	uint	msa : 1 ;
	uint	msv : 1 ;
	uint	mpv : 1 ;		/* memory previous value */
	uint	clocks : 1 ;		/* clocks */
	uint	syscalls : 1 ;		/* system calls */
} ;

struct traceinfo {
	unsigned long	magic ;
	struct traceinfo_flags	f ;
	EXECTRACE	et ;		/* the execution trace */
	ULONG		sin ;		/* starting instruction number */
	ULONG		ein ;		/* end instruction number */
	ULONG		n ;		/* number of instructions */
	ULONG		in ;		/* current instruction number */
} ;



#if	(! defined(TRACEINFO_MASTER)) || (TRACEINFO_MASTER == 0)

extern int traceinfo_init(TRACEINFO *,const char *,const char *,const char *) ;
extern int traceinfo_enabled(TRACEINFO *,ULONG) ;
extern int traceinfo_flush(TRACEINFO *) ;
extern int traceinfo_free(TRACEINFO *) ;

/* some useful macros for inlining purposes */

#define	traceinfo_fastenabled(op)	((op)->f.enabled)
#define	traceinfo_regs(op)		((op)->f.regs)
#define	traceinfo_regvals(op)		(((op)->f.regvals) ? 15 : 0)
#define	traceinfo_rsa(op)		((op)->f.rsa)
#define	traceinfo_rsv(op)		((op)->f.rsv)
#define	traceinfo_mems(op)		((op)->f.mems)
#define	traceinfo_memvals(op)		(((op)->f.memvals) ? 15 : 0)
#define	traceinfo_msa(op)		((op)->f.msa)
#define	traceinfo_msv(op)		((op)->f.msv)
#define	traceinfo_mpv(op)		((op)->f.mpv)
#define	traceinfo_syscalls(op)		((op)->f.syscalls)

#endif /* (! defined(TRACEINFO_MASTER)) || (TRACEINFO_MASTER == 0) */


#endif /* TRACEINFO_INCLUDE */



