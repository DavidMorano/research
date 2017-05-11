/* sssim */

/* simulator control related (utility) information */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started (for LevoSim).


*/




#ifndef	SSSIM_INCLUDE
#define	SSSIM_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif




/* object defines (whatever) */

#define	SSSIM			struct sssim_head
#define	SSSIM_SIMPROG		struct sssim_simprog
#define	SSSIM_MACHOBJ		struct sssim_machobj
#define	SSSIM_COE		struct sssim_coe
#define	SSSIM_SYMBOL		struct sssim_sym
#define	SSSIM_SNCURSOR		struct sssim_sncursor
#define	SSSIM_MAXPROG		struct sssim_mp
#define	MINTINFO		SSSIM



/* our dependencies */

#include	<elf.h>

#include	<hdb.h>
#include	<veclist.h>

#include	"plainq.h"
#include	"paramfile.h"

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"



/* other defines */

#define	SSSIM_MAXCLOCKS		200
#define	SSSIM_NQUEUES		400
#define	SSSIM_MINPHASES		6




struct sssim_mp {
	uint	data ;
	uint	stack ;
} ;

struct sssim_sym {
	Elf32_Sym	*ep ;
	const char	*name ;
} ;

#ifdef	COMMENT

struct sssim_sncursor {
	LMAPPROG_SNCURSOR	sncur ;
} ;

#endif

struct sssim_simprog {
	char	*program ;
	char	**argv ;
	char	**envv ;
	int	argc ;
} ;

struct sssim_machobj {
	void	*topobjp ;
	int	(*topcombp)(void *,int) ;
	int	(*topclockp)(void *) ;
} ;

/* a call-out entry (THE TOP TWO ENTRIES MUST BE IN THAT ORDER) */
struct sssim_coe {
	LONG	next ;		/* queue operations */
	LONG	prev ;		/* for queue operations */
	int	(*func)() ;
	void	*objp, *argp ;
	ULONG	wake ;
	ULONG	key ;		/* hash key */
	uint	relative ;
	uint	phase ;
} ;

struct sssim_flags {
	uint	syscalls : 1 ;
} ;

struct sssim_head {
	unsigned long		magic ;

	struct sssim_flags	f ;
	struct proginfo	*pip ;
	ULONG		clock ;		/* this is the main simulator clock ! */
	HDB		syscalls ;	/* indexed by syscall addresses */
	HDB		pq ;		/* pending (future) queue */
	PLAINQ		fq ;		/* free queue */
	veclist		*qes ;		/* fast queues */

#ifdef	COMMENT
	LMAPPROG	ourprog ;
#endif

	LONG	maxclocks ;	/* maximum number of clocks */
	void	*topobjp ;
	int	(*topcombp)(void *,int) ;
	int	(*topclockp)(void *) ;
	int	nq ;		/* number of queues */
	int	cqi ;		/* current queue index */
	int	clock_count ;	/* incremental clock counter */
	int	nentries ;	/* number of "fast" queues */
	int	phase ;		/* current clock phase */
} ;



#if	(! defined(SSSIM_MASTER)) && (! defined(SSSIM_MASTER))

extern int sssim_init(SSSIM *,struct proginfo *,PARAMFILE *,
			LONG,SSSIM_SIMPROG *,SSSIM_MACHOBJ *) ;
extern int sssim_setmach(SSSIM *,SSSIM_MACHOBJ *) ;

#ifdef	COMMENT
extern int sssim_getpp(SSSIM *,LMAPPROG **) ;
#endif

extern int sssim_callout(SSSIM *,int (*)(),void *,void *,uint,int) ;
extern int sssim_loop(SSSIM *) ;
extern int sssim_getclock(SSSIM *,ULONG *) ;
extern int sssim_free(SSSIM *) ;
extern int sssim_readinstr(SSSIM *,uint,uint *) ;
extern int sssim_readint(SSSIM *,uint,uint *) ;
extern int sssim_writeint(SSSIM *,uint,uint,uint) ;
extern int sssim_memaccess(SSSIM *,uint,uint) ;
extern int sssim_issyscall(SSSIM *,uint,const char **) ;
extern int sssim_getsymval(SSSIM *,char *,uint *) ;
extern int sssim_getentry(SSSIM *,uint *) ;
extern int sssim_getstack(SSSIM *,uint *) ;
extern int sssim_getbreak(SSSIM *,uint *) ;
extern int sssim_getpagesize(SSSIM *,uint *) ;
extern int sssim_getmax(SSSIM *,SSSIM_MAXPROG *) ;
extern int sssim_setbreak(SSSIM *,uint) ;

#ifdef	COMMENT
extern int sssim_sncursorinit(SSSIM *,SSSIM_SNCURSOR *) ;
extern int sssim_sncursorfree(SSSIM *,SSSIM_SNCURSOR *) ;
extern int sssim_fetchsym(SSSIM *,char *,SSSIM_SNCURSOR *,Elf32_Sym **) ;
#endif

#endif /* SSSIM_MASTER */


#endif /* SSSIM_INCLUDE */



