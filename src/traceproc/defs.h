/* INCLUDE FILE defs */


#ifndef	DEFS_INCLUDE
#define	DEFS_INCLUDE	1


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>

#include	<vecstr.h>
#include	<hdb.h>
#include	<logfile.h>
#include	<localmisc.h>

#include	"filtercalls.h"


#ifndef	FD_STDIN
#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2
#endif

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	4096
#endif

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	80
#endif

#define	BUFLEN		8192
#define	JOBNAMELEN	31
#define	LOGIDLEN	15

#define	DUMPOPTS	"dump"

#define	NREGS		200


struct proginfo_select {
	ULONG		start, end ;
	ULONG		n ;
} ;

struct proginfo_flags {
	uint		kopts:1 ;
	uint		aparams:1 ;
	uint		log:1 ;
	uint		quiet:1 ;
	uint		progmap:1 ;
	uint		instrdis:1 ;
	uint		select:1 ;		/* selection state */
	uint		past:1 ;		/* past the selection */
	uint		filter:1 ;
	uint		opt_in:1 ;
	uint		opt_regs:1 ;
	uint		opt_regvals:1 ;
	uint		opt_rsa:1 ;
	uint		opt_rsv:1 ;
	uint		opt_mems:1 ;
	uint		opt_memvals:1 ;
	uint		opt_msa:1 ;
	uint		opt_msv:1 ;
	uint		opt_mpv:1 ;
	uint		opt_instr:1 ;
	uint		opt_instrdis:1 ;
	uint		opt_eregs:1 ;
	uint		opt_emems:1 ;
	uint		opt_pixie:1 ;
	uint		opt_noerrno:1 ;		/* do not compare errno */
	uint		opt_onemem:1 ;		/* one memory location */
	uint		opt_syscalls:1 ;	/* system calls */
	uint		opt_plusinstr:1 ;	/* do the '+' thing */
	uint		opt_fcount:1 ;		/* instruction counts (stats) */
	uint		opt_ssh:1 ;		/* SSH (stats) */
	uint		opt_vpred:1 ;		/* value predictor (stats) */
	uint		opt_yags:1 ;		/* YAGS BP (stats) */
	uint		opt_tourna:1 ;		/* Alpha 21264 BP (stats) */
	uint		opt_gspag:1 ;		/* GS-PAG BP (stats) */
	uint		opt_eveight:1 ;		/* EVEIGHT BP (stats) */
	uint		opt_gskew:1 ;		/* GSKEW BP (stats) */
	uint		opt_regstats:1 ;	/* register statistics */
	uint		opt_memstats:1 ;	/* memory statistics */
	uint		opt_badias:1 ;		/* bad IAs */
} ;

struct proginfo {
	vecstr		stores ;
	const char	**envv ;
	const char	*pwd ;
	const char	*progename ;		/* program exec-name */
	const char	*progdname ;		/* program dir-name */
	const char	*progname ;		/* program name */
	const char	*pr ;			/* program root directory */
	const char	*searchname ;
	const char	*version ;
	const char	*banner ;
	const char	*nodename ;
	const char	*domainname ;
	const char	*username ;		/* ours */
	const char	*groupname ;		/* ours */
	const char	*logid ;
	void		*efp ;
	void		*ofp ;
	ULONG		ninstr ;	/* max number of instructions */
	ULONG		cin ;		/* current instruction */
	FILTERCALLS	*callfilter ;	/* calls to filter out */
	struct proginfo_flags	have, f, changed, final ;
	struct proginfo_flags	open ;
	struct proginfo_select	ck, in ;
	LOGFILE	lh ;
	time_t	starttime ;		/* program start */
	time_t	lastlogtime ;		/* last log operation */
	pid_t	pid ;
	int	pwdlen ;
	int	progmode ;		/* program mode */
	int	debuglevel ;		/* debugging level */
	int	verboselevel ;		/* verbosity level */
	char	basename[MAXNAMELEN + 1] ;	/* base name */
	char	jobname[JOBNAMELEN + 1] ;	/* job name */
} ;


#ifdef	__cplusplus
extern "C" {
#endif

extern int proginfo_start(struct proginfo *,const char **,const char *,
		const char *) ;
extern int proginfo_setentry(struct proginfo *,const char **,const char *,int) ;
extern int proginfo_setversion(struct proginfo *,const char *) ;
extern int proginfo_setbanner(struct proginfo *,const char *) ;
extern int proginfo_setsearchname(struct proginfo *,const char *,const char *) ;
extern int proginfo_setprogname(struct proginfo *,const char *) ;
extern int proginfo_setexecname(struct proginfo *,const char *) ;
extern int proginfo_setprogroot(struct proginfo *,const char *,int) ;
extern int proginfo_pwd(struct proginfo *) ;
extern int proginfo_rootname(struct proginfo *) ;
extern int proginfo_progdname(struct proginfo *) ;
extern int proginfo_progename(struct proginfo *) ;
extern int proginfo_nodename(struct proginfo *) ;
extern int proginfo_getpwd(struct proginfo *,char *,int) ;
extern int proginfo_getename(struct proginfo *,char *,int) ;
extern int proginfo_getenv(struct proginfo *,const char *,int,const char **) ;
extern int proginfo_finish(struct proginfo *) ;

#ifdef	__cplusplus
}
#endif

#endif /* DEFS_INCLUDE */


