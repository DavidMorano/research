/* syscalls */



#ifndef	SYSCALLS_INCLUDE
#define	SYSCALLS_INCLUDE

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines */

#define	SYSCALLS		struct syscalls_head
#define	SYSCALLS_ARGREGS	struct syscalls_ar
#define	SYSCALLS_RETREG		struct syscalls_rr
#define	SYSCALLS_ENTRY		struct syscalls_e
#define	SYSCALLS_STATS		struct syscalls_s




#include	<sys/types.h>
#include	<sys/param.h>
#include	<netdb.h>		/* for MAXHOSTNAMELEN */

#include	<hdb.h>

#include	"misc.h"		/* for 'ULONG' */
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"statemips.h"



/* other defines */

#define	SYSCALLS_NFILE		64		/* # files */
#define	SYSCALLS_NRR		4		/* # return regs */

#ifdef	SGIFSSIZE
#define	SYSCALLS_FSSIZE		SGIFSSIZE	/* size of FS array */
#else
#define	SYSCALLS_FSSIZE		16		/* size of FS array */
#endif



struct syscalls_s {
	uint	c_callopen, c_callclose, c_callread, c_callwrite ;
	uint	c_fileopen, c_fileclose ;
	uint	maxopen ;
} ;

struct syscalls_ar {
	uint	a1 ;
	uint	a2 ;
	uint	a3 ;
	uint	a4 ;
	uint	sp ;
} ;

struct syscalls_rr {
	uint	n ;
	uint	a[SYSCALLS_NRR] ;
	uint	dv[SYSCALLS_NRR] ;
	int	sgierrno ;
	int	sgirc ;
} ;

struct syscalls_fd {
	int	fd ;
} ;

struct syscalls_flags {
	uint	eof : 1 ;		/* EOF indication on reading */
} ;

struct syscalls_head {
	unsigned long	magic ;
	struct proginfo	*pip ;
	LSIM		*mip ;
	struct statemips	*smp ;
	struct syscalls_flags		f ;
	struct syscalls_s		s ;
	HDB		index ;		/* index of SYSCALLS */
	struct syscalls_fd	fds[SYSCALLS_NFILE] ;
	uint		sgidev ;	/* SGI device number */
	uint		ea ;		/* 'errno' address */
	uint		brka ;		/* current break address */
	uint		brkbase ;	/* original break address */
	uint		brkmax ;	/* maximum value seen */
	char		sgifstype[SYSCALLS_FSSIZE] ;
} ;

struct syscalls_e {
	ULONG			clock ;
	ULONG			in ;
	uint			ia ;
} ;




#if	(! defined(SYSCALLS_MASTER)) || (SYSCALLS_MASTER == 0)

extern int	syscalls_init(SYSCALLS *,struct proginfo *,LSIM *,
			struct statemips *,char **) ;
extern int	syscalls_issyscall(SYSCALLS *,uint,const char **) ;
extern int	syscalls_handle(SYSCALLS *,uint,
			SYSCALLS_ARGREGS *, SYSCALLS_RETREG *) ;
extern int	ssycalls_getbrkmax(SYSCALLS *,uint *,uint *) ;
extern int	syscalls_stats(SYSCALLS *,SYSCALLS_STATS *) ;
extern int	syscalls_free(SYSCALLS *) ;

#endif /* (! defined(SYSCALLS_MASTER)) || (SYSCALLS_MASTER == 0) */


#endif /* SYSCALLS_INCLUDE */



