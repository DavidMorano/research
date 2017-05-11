/* defs */



/* revision history:

	= 00/02/15, Dave Morano

	This code was started.


*/



#ifndef	DEFS_INCLUDE
#define	DEFS_INCLUDE	1



#include	<sys/types.h>
#include	<netdb.h>

#include	<vsystem.h>
#include	<logfile.h>
#include	<vecstr.h>

#include	"paramfile.h"
#include	"density.h"
#include	"localmisc.h"
#include	"traceinfo.h"
#include	"xmlinfo.h"
#include	"debugwin.h"
#include	"bpeval.h"
#include	"iflags.h"
#include	"regstats.h"
#include	"memstats.h"
#include	"checker.h"

#include	"ssinfo.h"
#include	"sscommon.h"



/* compilation feature switches */

#ifndef	CF_XML
#define	CF_XML		0		/* turn ON XML stuff */
#endif

#ifndef	CF_MASTERDEBUG
#define	CF_MASTERDEBUG	1
#endif


/* other stuff */

#ifndef	FD_STDIN
#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2
#endif

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	2048
#endif

#ifndef	MAXNAMELEN
#define	MAXNAMELEN	256
#endif

#ifndef	USERNAMELEN
#ifndef	LOGNAME_MAX
#define	USERNAMELEN	LOGNAME_MAX
#else
#define	USERNAMELEN	32
#endif
#endif

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif

#define	USERBUFLEN	4096

#ifndef	LOGIDLEN
#define	LOGIDLEN	14
#endif

#ifndef	INFNAME
#define	INFNAME		"*STDIN*"
#endif

#ifndef	OUTFNAME
#define	OUTFNAME	"*STDOUT*"
#endif

#ifndef	ERRFNAME
#define	ERRFNAME	"*STDERR*"
#endif

#ifndef	DIGBUFLEN
#define	DIGBUFLEN	20
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	80
#endif

#ifndef	DEBUGLEVEL
#define	DEBUGLEVEL(n)	(pip->debuglevel >= (n))
#endif

#define	BUFLEN		(5 * 1024)
#define	VBUFLEN		MAXNAMELEN
#define	JOBNAMELEN	MAXNAMELEN

#ifndef	SGIFSSIZE
#define	SGIFSSIZE	32
#endif

#define	ULONG_UNSET	((ULONG) -1)
#define	ULONG_SIMPOINT	((ULONG) -2)


/* program modes */

#define PROGMODE_LEVOSIM	0
#define	PROGMODE_LEVOSIMSIZE	1
#define	PROGMODE_SIMPLESIM	2


/* file names */

#define	LEVOOUTFNAME	"levo.out"





/* program information */

struct proginfo_select {
	ULONG	start, end ;
	ULONG	n ;
} ;

struct proginfo_flags {
	uint	outfile : 1 ;		/* do we have STDOUT ? */
	uint	errfile : 1 ;		/* do we have STDERR ? */
	uint	log : 1 ;		/* do we have a log file ? */
	uint	quiet : 1 ;		/* are we in quiet mode ? */
	uint	exit : 1 ;		/* exit program */
	uint	params : 1 ;		/* we have passed parameters */
	uint	select : 1 ;		/* selection state */
	uint	past : 1 ;		/* past the selection (if any) */
	uint	exectrace : 1 ;		/* do we have an execution trace ? */
	uint	xmltrace : 1 ;		/* XML state trace */
	uint	statistics : 1 ;	/* do we have a statistics file ? */
	uint	instrdis : 1 ;		/* instruction disassembly available */
	uint	bpeval : 1 ;		/* BP evaluation */
	uint	nomem : 1 ;		/* do not do MEM-OP passing */
	uint	execover : 1 ;		/* do overlapping executions */
	uint	memdatalat : 1 ;	/* do memory latency count in AS */
	uint	perfpred : 1 ;		/* "perfect" (oracle) CF prediction */
	uint	iflags : 1 ;		/* profile instruction flags */
	uint	itype : 1 ;		/* profile instruction types */
	uint	itrace : 1 ;		/* instruction traces */
	uint	regstats : 1 ;
	uint	memstats : 1 ;
	uint	checkonly : 1 ;		/* don't run functional sim */
	uint	exit_target : 1 ;	/* ended due to target program */
	uint	exit_clock : 1 ;	/* ended due to clock expiration */
	uint	exit_instr : 1 ;	/* ended due to instr expiration */
} ;

struct proginfo_tell {
	ULONG	in ;
	ULONG	ck ;
	time_t	synctime ;
} ;

struct proginfo_stats {
	DENSITY	imlat, dmlat ;
	ULONG	in_start, ins ;
	ULONG	ck_start, cks ;
	ULONG	*imemlat ;
	ULONG	*dmemlat ;
	ULONG	ncneed ;
	ULONG	ncadv ;
	ULONG	asloads ;
	ULONG	asmemins, asmemops, asmemopreads ;
	ULONG	imemreads ;
	ULONG	dmemreads, dmemwrites ;
	ULONG	bp_inscf, bp_inscfcond, bp_taken ;
	ULONG	bp_predgiven, bp_predtaken, bp_predcorrect ;
	uint	imemlatlen, dmemlatlen ;
	uint	imemlatmax, dmemlatmax ;
	uint	imemlatscrew, dmemlatscrew ;
} ;

struct proginfo {
	ULONG		simpoint ;	/* # instructions up to SIMPOINT */
	ULONG		sinstr ;	/* # instructions to skip */
	ULONG		winstr ;	/* # warm-up instructions */
	ULONG		ninstr ;	/* number of instructions */
	ULONG		minstr ;	/* DERIVED max number of instructions */
	ULONG		linstr ;	/* max instructions between logs */
	ULONG		mclock ;	/* max number of clocks */
	unsigned long	magic ;
	struct proginfo_flags	f ;
	struct proginfo_select	ck, in ;	/* for functional machine */
	struct proginfo_select	intervals ;	/* for access intervals */
	struct proginfo_tell	t ;
	struct proginfo_stats	s ;
	LOGFILE		lh ;		/* program activity log */
	TRACEINFO	ti ;		/* execution trace information */
	XMLINFO		xi ;		/* XML trace information */
	DEBUGWIN	dw ;		/* debug window thing */
	PARAMFILE	pf ;
	BPEVAL		bp ;
	REGSTATS	rstats ;	/* register intervals */
	MEMSTATS	mstats ;	/* memory intervals */
	SSINFO		info ;
	CHECKER		check ;		/* the checker */
	char	**envv ;
	char	*arg0 ;
	char	*pwd ;
	char	*progename ;
	char	*progdname ;		/* program directory name */
	char	*progname ;		/* program name */
	char	*pr ;			/* program root directory */
	char	*version ;		/* program version string */
	char	*banner ;
	char	*searchname ;
	char	*domainname ;
	char	*nodename ;
	char	*username ;		/* ours */
	char	*groupname ;		/* ours */
	char	*logid ;		/* default program LOGID */
	char	*tmpdname ;		/* temporary directory */
	char	*workdname ;		/* working directory */
	char	*execfname ;		/* execution file name */
	char	*itfname ;		/* instruction trace file */
	char	*etfname ;		/* execution trace file */
	char	*xmlfname ;		/* XML state flow file */
	void	*efp ;
	void	*ofp ;
	void	*btfp ;			/* bus trace */
	void	*sfp ;		/* statistics file */
	void	*it1, *it2, *it3 ;
	void	*itp ;			/* instruction type statistics */
	struct iflags	ifl ;
	time_t	ti_current ;		/* current time */
	time_t	ti_start ;		/* program start */
	time_t	ti_lastlog ;		/* last log made */
	pid_t	pid ;
	uint	sgidev ;		/* SGI device number */
	int	pwdlen ;
	int	progmode ;		/* program mode */
	int	debuglevel ;		/* debugging level */
	int	verboselevel ;		/* verbosity level */
	int	paramfile_type ;
	int	logfile_type ;
	char	realname[NODENAMELEN + 1] ;	/* user real name */
	char	benchname[MAXNAMELEN + 1] ;	/* benchmark name */
	char	targetname[MAXNAMELEN + 1] ;	/* target program filename */
	char	jobname[JOBNAMELEN + 1] ;	/* job name */
	char	paramfname[MAXPATHLEN + 1] ;	/* parameter file */
	char	spfname[MAXPATHLEN + 1] ;	/* SIMPOINT file */
	char	logfname[MAXPATHLEN + 1] ;	/* log file */
	char	tellfname[MAXPATHLEN + 1] ;	/* "tell" file */
	char	statsfname[MAXPATHLEN + 1] ;
	char	sgifstype[SGIFSSIZE] ;	/* SGI FS type name (fake) */
} ;



#define	PROGINFO	struct proginfo


#if	(! defined(PROGINFO_MASTER)) || (PROGINFO_MASTER == 0)

extern int proginfo_init(PROGINFO *,const char **,const char *,const char *) ;
extern int proginfo_setselect(PROGINFO *,char *,int) ;
extern int proginfo_setopt(PROGINFO *,char *,int) ;
extern int proginfo_levoconf(PROGINFO *,char *,int) ;
extern int proginfo_progress(PROGINFO *,ULONG,ULONG) ;
extern int proginfo_tellstart(PROGINFO *,ULONG,ULONG) ;
extern int proginfo_tellcheck(PROGINFO *,int,ULONG,ULONG) ;
extern int proginfo_selection(PROGINFO *,ULONG,ULONG) ;
extern int proginfo_dump(PROGINFO *,ULONG) ;
extern int proginfo_free(PROGINFO *) ;

#endif /* PROGINFO_MASTER */


#endif /* DEFS_INCLUDE */



