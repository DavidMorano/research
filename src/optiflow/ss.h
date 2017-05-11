/* ss */

/* simulator control related (utility) information */



/* revision history:

	= 00/02/15, Dave Morano

	This code was started (for LevoSim).


*/



#ifndef	SS_INCLUDE
#define	SS_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines (whatever) */

#define	SS		struct ss_head
#define	SS_COE		struct ss_coe
#define	SS_MACHOBJ	struct ss_machobj
#define	SS_INFO		struct ss_info



/* our dependencies */

#include	<sys/types.h>

#include	<bio.h>
#include	<hdb.h>
#include	<veclist.h>
#include	<plainq.h>
#include	<paramfile.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"ssas.h"
#include	"regs.h"			/* this is from S-S */
#include	"ssinfo.h"



/* other defines */

#define	SS_MAXCLOCKS		1000000000000LL
#define	SS_NQUEUES		400
#define	SS_MINPHASES		6






struct ss_datastats {
	ULONG	in ;
	ULONG	mem ;
	ULONG	memmulti ;
	ULONG	memreads, memwrites ;
	ULONG	memunaligned ;
	ULONG	cf ;			/* control flow */
	ULONG	cfind ;			/* control flow (indirect) */
	ULONG	cfsub ;			/* control flow (subroutine) */
	ULONG	cfcond ;		/* control flow (conditional) */
	ULONG	weird ;			/* weird */
} ;

struct ss_info {
	ULONG	ck_start, in_start ;
	ULONG	ck, in ;
	uint	exit_target ;
	uint	exit_clock ;
	int	mrs ;
} ;

struct ss_machobj {
	void	*topobjp ;
	int	(*topcombp)(void *,int) ;
	int	(*topclockp)(void *) ;
} ;

/* a call-out entry (THE TOP TWO ENTRIES MUST BE IN THAT ORDER) */
struct ss_coe {
	LONG	next ;		/* queue operations */
	LONG	prev ;		/* for queue operations */
	int	(*func)() ;
	void	*objp, *argp ;
	ULONG	wake ;
	ULONG	key ;		/* hash key */
	uint	relative ;
	uint	phase ;
} ;

struct ss_flags {
	uint	syscalls : 1 ;
	uint	exit_target : 1 ;
	uint	exit_clock : 1 ;
} ;

struct ss_head {
	unsigned long	magic ;
	ULONG		maxclocks ;	/* maximum number of clocks */
	ULONG		ck, ck_start ;	/* this is the main simulator clock! */
	ULONG		in, in_start ;	/* this is the functional version */
	struct proginfo	*pip ;
	veclist		*qes ;		/* fast queues */
	void		*topobjp ;
	int		(*topcombp)(void *,int) ;
	int		(*topclockp)(void *) ;
	struct ss_flags	f ;
	HDB		pq ;		/* pending (future) queue */
	PLAINQ		fq ;		/* free queue */
	bfile			tfile ;
	int	nq ;		/* number of queues */
	int	cqi ;		/* current queue index */
	int	ck_incr ;	/* incremental clock counter */
	int	nentries ;	/* number of "fast" queues */
	int	phase ;		/* current clock phase */
	int	mrs ;		/* machine return status */
} ;



#if	(! defined(SS_MASTER)) && (! defined(SS_MASTER))

extern int ss_init(SS *,struct proginfo *,
			ULONG,SS_MACHOBJ *) ;
extern int ss_setmach(SS *,SS_MACHOBJ *) ;
extern int ss_callout(SS *,int (*)(),void *,void *,uint,int) ;
extern int ss_loop(SS *,ULONG,ULONG,ULONG) ;
extern int ss_getinstr(SS *,ULONG *) ;
extern int ss_getclock(SS *,ULONG *) ;
extern int ss_info(SS *,SS_INFO *) ;
extern int ss_free(SS *) ;

#endif /* SS_MASTER */


#endif /* SS_INCLUDE */



