/* lsim SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* simulator control related (utility) information */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-02-15, David Morano
	This code was started (for LevoSim).

*/

/* Copyright © 2000 David A-D- Morano.  All rights reserved. */

#ifndef	LSIM_INCLUDE
#define	LSIM_INCLUDE


#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<elf.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<usystem>
#include	<hdb.h>
#include	<vechand.h>
#include	<plainq.h>
#include	<paramfile.h>
#include	<localmisc.h>

#include	"lmapprog.h"
#include	"config.h"
#include	"defs.h"


#ifndef	UINT
#define	UINT	unsigned int
#endif
/* object defines (whatever) */
#define	LSIM			struct mintinfo
#define	LSIM_SIMPROG		struct lsim_simprog
#define	LSIM_MACHOBJ		struct lsim_machobj
#define	LSIM_COE		struct lsim_coe
#define	LSIM_SYMBOL		struct lsim_sym
#define	LSIM_SNCURSOR		struct lsim_sncursor
#define	LSIM_MAXPROG		struct lsim_mp
#define	MINTINFO		LSIM
/* other defines */
#define	LSIM_MAXCLOCKS	200
#define	LSIM_NQUEUES	400
#define	LSIM_MINPHASES	6


struct lsim_mp {
	uint		data ;
	uint		stack ;
} ;

struct lsim_sym {
	Elf32_Sym	*ep ;
	const char	*name ;
} ;

struct lsim_sncursor {
	LMAPPROG_SNCURSOR	sncur ;
} ;

struct lsim_simprog {
	char		*program ;
	char		**argv ;
	char		**envv ;
	int		argc ;
} ;

struct lsim_machobj {
	void		*topobjp ;
	int		(*topcombp)(void *,int) ;
	int		(*topclockp)(void *) ;
} ;

/* a call-out entry (THE TOP TWO ENTRIES MUST BE IN THAT ORDER) */
struct lsim_coe {
	LONG		next ;		/* queue operations */
	LONG		prev ;		/* for queue operations */
	int		(*func)() ;
	void		*objp, *argp ;
	ULONG		wake ;
	ULONG		key ;		/* hash key */
	uint		relative ;
	uint		phase ;
} ;

struct lsim_flags {
	uint		syscalls : 1 ;
} ;

struct mintinfo {
	unsigned long		magic ;
	struct lsim_flags	f ;
	struct proginfo	*pip ;
	ULONG		clock ;		/* this is the main simulator clock ! */
	HDB		syscalls ;	/* indexed by syscall addresses */
	HDB		pq ;		/* pending (future) queue */
	PLAINQ		fq ;		/* free queue */
	vechand		*qes ;		/* fast queues */
	LMAPPROG	ourprog ;
	LONG		maxclocks ;	/* maximum number of clocks */
	void		*topobjp ;
	int		(*topcombp)(void *,int) ;
	int		(*topclockp)(void *) ;
	int		nq ;		/* number of queues */
	int		cqi ;		/* current queue index */
	int		clock_count ;	/* incremental clock counter */
	int		nentries ;	/* number of "fast" queues */
	int		phase ;		/* current clock phase */
} ;


#if	(! defined(LSIM_MASTER)) && (! defined(LSIM_MASTER))

#ifdef	__cplusplus
extern "C" {
#endif

extern int lsim_init(LSIM *,struct proginfo *,PARAMFILE *,
			LONG,LSIM_SIMPROG *,LSIM_MACHOBJ *) ;
extern int lsim_setmach(LSIM *,LSIM_MACHOBJ *) ;
extern int lsim_getpp(LSIM *,LMAPPROG **) ;
extern int lsim_callout(LSIM *,int (*)(),void *,void *,uint,int) ;
extern int lsim_loop(LSIM *) ;
extern int lsim_getclock(LSIM *,ULONG *) ;
extern int lsim_free(LSIM *) ;
extern int lsim_readinstr(LSIM *,uint,uint *) ;
extern int lsim_readint(LSIM *,uint,uint *) ;
extern int lsim_writeint(LSIM *,uint,uint,uint) ;
extern int lsim_memaccess(LSIM *,uint,uint) ;
extern int lsim_issyscall(LSIM *,uint,const char **) ;
extern int lsim_getsymval(LSIM *,char *,uint *) ;
extern int lsim_getentry(LSIM *,uint *) ;
extern int lsim_getstack(LSIM *,uint *) ;
extern int lsim_getbreak(LSIM *,uint *) ;
extern int lsim_getpagesize(LSIM *,uint *) ;
extern int lsim_getmax(LSIM *,LSIM_MAXPROG *) ;
extern int lsim_setbreak(LSIM *,uint) ;
extern int lsim_sncurbegin(LSIM *,LSIM_SNCURSOR *) ;
extern int lsim_sncurend(LSIM *,LSIM_SNCURSOR *) ;
extern int lsim_fetchsym(LSIM *,char *,LSIM_SNCURSOR *,Elf32_Sym **) ;

#ifdef	__cplusplus
}
#endif

#endif /* LSIM_MASTER */

#endif /* LSIM_INCLUDE */


