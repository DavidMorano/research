/* INCLUDE FILE defs */



#ifndef	DEFS_INCLUDE
#define	DEFS_INCLUDE	1



#include	<sys/types.h>
#include	<sys/param.h>

#include	<vecstr.h>
#include	<hdb.h>
#include	<logfile.h>

#include	"localmisc.h"



/* feature switches */


/* other stuff */

#ifndef	FD_STDIN
#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	80
#endif

#ifndef	DEBUGLEVEL
#define	DEBUGLEVEL(n)	(pip->debuglevel >= (n))
#endif

#define	LINELEN		256		/* size of input line */

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	4096
#endif

#define	BUFLEN		8192
#define	JOBNAMELEN	31
#define	LOGIDLEN	15

#define	PO_DUMPOPT	"dump"

#define	NREGS		200




struct proginfo_select {
	ULONG	start, end ;
	ULONG	n ;
} ;

struct proginfo_flags {
	uint	aparams : 1 ;
	uint	log : 1 ;
	uint	quiet : 1 ;
	uint	progmap : 1 ;
	uint	instrdis : 1 ;
	uint	past : 1 ;		/* past the selection (if any) */
	uint	select : 1 ;
	uint	opt_in : 1 ;
	uint	opt_regs : 1 ;
	uint	opt_regvals : 1 ;
	uint	opt_rsa : 1 ;
	uint	opt_rsv : 1 ;
	uint	opt_mems : 1 ;
	uint	opt_memvals : 1 ;
	uint	opt_msa : 1 ;
	uint	opt_msv : 1 ;
	uint	opt_mpv : 1 ;
	uint	opt_instr : 1 ;
	uint	opt_instrdis : 1 ;
	uint	opt_eregs : 1 ;
	uint	opt_emems : 1 ;
	uint	opt_pixie : 1 ;
	uint	opt_noerrno : 1 ;	/* do not compare errno */
	uint	opt_onemem : 1 ;	/* only compare one memory location */
	uint	opt_syscalls : 1 ;	/* system calls */
	uint	opt_plusinstr : 1 ;	/* do the '+' thing */
} ;

struct proginfo {
	ULONG		ninstr ;	/* max number of instructions */
	ULONG		cin ;		/* current instruction */
	struct proginfo_flags	f ;
	struct proginfo_flags	open ;
	struct proginfo_select	ck, in ;
	LOGFILE	lh ;
	vecstr	stores ;
	char	**envv ;
	char	*pwd ;
	char	*progname ;		/* program name */
	char	*progename ;
	char	*progdname ;
	char	*pr ;		/* program root directory */
	char	*searchname ;
	char	*version ;
	char	*banner ;
	char	*domainname ;
	char	*nodename ;
	char	*username ;		/* ours */
	char	*groupname ;		/* ours */
	char	*logid ;
	void	*efp ;
	void	*ofp ;
	time_t	starttime, lastlogtime ;
	pid_t	pid ;
	int	pwdlen ;
	int	progmode ;		/* program mode */
	int	debuglevel ;		/* debugging level */
	int	verboselevel ;		/* verbosity level */
	char	basename[JOBNAMELEN + 1] ;	/* job name */
	char	jobname[JOBNAMELEN + 1] ;	/* job name */
} ;



#endif /* DEFS_INCLUDE */



