/* main */

/* generic (pretty much) front end program subroutine */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0		/* non-runtime-switchable debugs */
#define	CF_DEBUG	1		/* runtime-switchable debug prints */
#define	CF_PRECISEENV	1		/* only create specified environment */
#define	CF_STDOUT	1		/* use STDOUT? */
#define	CF_NOSIGINT	0		/* ignore SIGINT? */
#define	CF_NOSIGHUP	0		/* ignore SIGHUP? */
#define	CF_NOSIGPIPE	1		/* ignore SIGPIPE? */
#define	CF_SIGDUMP	1		/* dump signal status */


/* revision history :

	= 2000-02-15, David Morano

        This program was originally written. Parts were copied from other
        miscellaneous subroutines.


	= 2001-03-28, David Morano

        I added a magic number to the 'proginfo' structure so that deep
        subroutines can check what is happenning!


*/

/* Copyright © 2000,2001 David A­D­ Morano.  All rights reserved. */

/*****************************************************************************

	This is the outer front end of the LevoSim simulator program.


*****************************************************************************/


#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/utsname.h>
#include	<sys/ucontext.h>
#include	<climits>
#include	<netdb.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<cstdlib>
#include	<cstring>
#include	<cerrno>		/* for signal catching */

#include	<usystem.h>
#include	<ascii.h>
#include	<bitops.h>
#include	<char.h>
#include	<bio.h>
#include	<field.h>
#include	<logfile.h>
#include	<vecstr.h>
#include	<userinfo.h>
#include	<varsub.h>
#include	<storebuf.h>
#include	<mallocstuff.h>
#include	<exitcodes.h>

#include	"schedvar.h"
#include	"paramfile.h"
#include	"getfname.h"

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"configfile.h"
#include	"debugwin.h"

#ifdef	DMALLOC
#include	<dmalloc.h>
#endif


/* local defines */

#define	MAXARGINDEX	100
#define	MAXARGGROUPS	(MAXARGINDEX/8 + 1)
#define	NUMNAMES	12
#define	NAMEBUFLEN	(NUMNAMES * MAXPATHLEN)

#ifndef	BUFLEN
#define	BUFLEN		((8 * 1024) + NAMEBUFLEN)
#endif

#ifndef	VBUFLEN
#define	VBUFLEN		100
#endif

#ifndef	PEBUFLEN
#define	PEBUFLEN	(VBUFLEN + 100)
#endif

#define	SIGDUMPFILE	"sigdumper"


/* external subroutines */

extern int	optmatch(const char **,const char *,int) ;
extern int	vstrkeycmp(char **,char **) ;
extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecll(const char *,int,LONG *) ;
extern int	perm(char *,int,int,int *,int) ;
extern int	permsched(const char **,VECSTR *,char *,int,char *,int) ;
extern int	getpwd(char *,int) ;
extern int	userinfo() ;
extern int	getnodedomain(char *,char *) ;
extern int	bopenroot(bfile *,char *,char *,char *,char *,int) ;
extern int	getfname(char *,char *,int,char *) ;
extern int	varsub_loadvec(), varsub_loadenv() ;
extern int	varsub_subbuf(), varsub_merge() ;
extern int	expander() ;
extern int	procfilepaths(char *,char *,VECSTR *) ;
extern int	procfileenv(char *,char *,VECSTR *) ;
extern int	process(struct proginfo *,const char *,PARAMFILE *,
			VECSTR *,ULONG,ULONG) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strbasename(char *) ;
extern char	*timestr_log(time_t,char *) ;
extern char	*timestr_elapsed(time_t,char *) ;


/* external variables */

extern char	levosim_makedate[] ;


/* local structures */

struct rsstring {
	int	rs ;
	char	*str ;
} ;


/* forward references */

static int	procfile(struct proginfo *,int (*)(),char *,
			SCHEDVAR *,char *, VECSTR *) ;
static int	makedate_get(const char *,char **) ;

static void	int_inter(int,siginfo_t *,void *) ;
static void	int_hanger(int,siginfo_t *,void *) ;
static void	int_piper(int,siginfo_t *,void *) ;


/* local variables */

static const char *argopts[] = {
	"TMPDIR",
	"VERSION",
	"VV",
	"VERBOSE",
	"ROOT",
	"LOGFILE",
	"CONFIG",
	"PARAMTAB",
	"DEBUG",
	"DS",
	"icount",
	"count",
	"et",
	"s",
	"skip",
	NULL
} ;

enum argopts {
	argopt_tmpdir,
	argopt_version,
	argopt_vv,
	argopt_verbose,
	argopt_root,
	argopt_logfile,
	argopt_config,
	argopt_paramtab,
	argopt_debug,
	argopt_ds,
	argopt_icount,
	argopt_count,
	argopt_et,
	argopt_select,
	argopt_skip,
	argopt_overlast
} ;

/* program modes (these must match the corresponding defines) */

static const char *progmodes[] = {
	"levosim",
	"levosize",
	"simplesim",
	NULL
} ;

/* 'conf' for most regular programs */
static const char	*sched1[] = {
	"%p/%e/%n/%n.%f",
	"%p/%e/%n/%f",
	"%p/%e/%n.%f",
	"%p/%n.%f",
	NULL
} ;

/* non-'conf' ETC stuff for all regular programs */
static const char	*sched2[] = {
	"%p/%e/%n/%n.%f",
	"%p/%e/%n/%f",
	"%p/%e/%n.%f",
	"%p/%e/%f",
	"%p/%n.%f",
	NULL
} ;

/* 'conf' and non-'conf' ETC stuff for local searching */
static const char	*sched3[] = {
	"%e/%n/%n.%f",
	"%e/%n/%f",
	"%e/%n.%f",
	"%e/%f",
	"%n.%f",
	"%f",
	NULL
} ;

/* exit strings */

static const struct rsstring	rsstrings[] = {
	{ 1, "program reached normal exit" },
	{ SR_OK, "OK exit" },
	{ SR_NOTFOUND, "no entry found" },
	{ SR_NOMEM, "not enough memory" },
	{ SR_DEADLOCK, "deadlock was detected" },
	{ SR_BADFMT, "bad format detected" },
	{ SR_NOTOPEN, "something was not open or initialized" },
	{ SR_FAULT, "some memory fault or NULL pointer was encountered" },
	{ SR_INVALID, "invalid argument or parameter" },
	{ SR_ILSEQ, "illegal data sequence encountered" },
	{ SR_NOANODE, "no anode -- very bad!" },
	{ SR_NOTSUP, "operation not supported" },
	{ 0, NULL }
} ;


/* global variables */

int	f_exit ;
int	f_pipe ;


/* exported subroutines */


int main(int argc,const char **argv,const char **envv)
{
	bfile		errfile, *efp = &errfile ;
	bfile		logfile ;
	bfile		pidfile ;
	bfile		btfile ;

	USTAT		sb ;

	struct proginfo		g, *pip = &g ;

	struct userinfo		u ;

	CONFIGFILE		cf ;

	PARAMFILE		pfile ;

	vecstr		defines, unsets, exports ;

	SCHEDVAR	svars ;

#ifdef	COMMENT
	varsub		srvsubs ;
#endif
	varsub		vsh_e, vsh_d ;

	time_t	daytime ;

	ULONG	maxclocks = 0, skipinstr = 0 ;
	ULONG	ullw ;

	int	argr, argl, aol, akl, avl ;
	int	maxai, pan, npa, kwi, i, j, k ;
	int	argnum ;
	int	f_optminus, f_optplus, f_optequal ;
	int	f_extra = FALSE ;
	int	ex = EX_INFO ;
	int	rs, len ;
	int	f_version = FALSE ;
	int	f_makedate = FALSE ;
	int	f_usage = FALSE ;
	int	loglen = -1 ;
	int	sl, cl, l2 ;
	int	fd_debug ;
	int	logfile_type = GETFNAME_TYPEUNKNOWN ;
	int	mintfile_type = GETFNAME_TYPEUNKNOWN ;
	int	paramfile_type = GETFNAME_TYPEUNKNOWN ;
	int	debugfile_type = GETFNAME_TYPEUNKNOWN ;
	int	f_programroot = FALSE ;
	int	f_configfile = FALSE ;
	int	f_freeconfigfname = FALSE ;
	int	f_procfileenv = FALSE ;
	int	f_procfilepaths = FALSE ;

	char	*argp, *aop, *akp, *avp ;
	char	argpresent[MAXARGGROUPS] ;
	char	*programroot = NULL ;
	char	buf[BUFLEN + 1] ;
	char	buf2[BUFLEN + 1] ;
	char	userbuf[USERINFO_LEN + 1] ;
	char	nodename[NODENAMELEN + 1], domainname[MAXHOSTNAMELEN + 1] ;
	char	tmpfname[MAXPATHLEN + 1] ;
	char	mintfname[MAXPATHLEN + 1] ;
	char	paramfname[MAXPATHLEN + 1] ;
	char	logfname[MAXPATHLEN + 1] ;
	char	debugfname[MAXPATHLEN + 1] ;
	char	timebuf[TIMEBUFLEN + 1] ;
	char	*configfname = NULL ;
	char	*searchname = SEARCHNAME ;
	char	*progmodestr = NULL ;
	char	*cp ;


	if ((cp = getenv(DEBUGFDVAR1)) == NULL)
	    cp = getenv(DEBUGFDVAR2) ;

	if ((cp != NULL) && (cfdeci(cp,-1,&fd_debug) >= 0))
	    esetfd(fd_debug) ;


/* program-wide initialization */

	f_exit = FALSE ;
	f_pipe = FALSE ;
	(void) memset(pip,0,sizeof(struct proginfo)) ;

	pip->magic = PROGMAGIC ;
	pip->version = VERSION ;
	pip->envv = envv ;
	pip->arg0 = argv[0] ;
	pip->progname = strbasename(argv[0]) ;

#if	CF_DEBUGS
	eprintf("main: progname=%s\n",pip->progname) ;
#endif

/* we want to open up some files so that the first few FD slots are FULL! */

	if (u_fstat(FD_STDIN,&sb) < 0)
	    (void) u_open("/dev/null",O_RDONLY,0666) ;

	pip->f.fd_stdout = TRUE ;
	if (u_fstat(FD_STDOUT,&sb) < 0) {

	    pip->f.fd_stdout = FALSE ;
	    (void) u_open("/dev/null",O_RDONLY,0666) ;

	}

	    pip->f.fd_stderr = FALSE ;
	pip->efp = NULL ;
	if (bopen(&errfile,BIO_STDERR,"dwca",0666) >= 0) {

	pip->f.fd_stderr = TRUE ;
		pip->efp = &errfile ;
	    bcontrol(&errfile,BC_LINEBUF,0) ;

	} else
	    (void) u_open("/dev/null",O_WRONLY,0666) ;


	pip->ofp = NULL ;
	pip->btfp = NULL ;

	pip->pid = getpid() ;

	pip->programroot = NULL ;
	pip->username = NULL ;
	pip->groupname = NULL ;
	pip->tmpdir = NULL ;
	pip->workdir = NULL ;
	pip->logid = NULL ;
	pip->execfname = NULL ;		/* execution file name */
	pip->etfname = NULL ;		/* execution trace file */
	pip->xmlfname = NULL ;		/* XML state flow file */
	pip->statfname = NULL ;		/* statistics output file */


	(void) memset(pip->jobname,0,JOBNAMELEN) ;

	pip->debuglevel = 0 ;
	pip->verboselevel = 0 ;
	pip->ninstr = 0 ;

	pip->f.quiet = FALSE ;
	pip->f.log = FALSE ;
	pip->f.params = FALSE ;


	u_time(&daytime) ;

	proginfo_init(pip,daytime) ;


/* other local initialization */

	tmpfname[0] = '\0' ;
	mintfname[0] = '\0' ;
	paramfname[0] = '\0' ;
	logfname[0] = '\0' ;
	debugfname[0] = '\0' ;


	debugwin_init(&pip->dw) ;	/* debug window thing */


#if	CF_MASTERDEBUG && CF_SIGDUMP
	sigdumper(SIGDUMPFILE,pip->pid,"s0") ;
#endif


/* start parsing the arguments */

	for (i = 0 ; i < MAXARGGROUPS ; i += 1) argpresent[i] = 0 ;

	npa = 0 ;			/* number of positional so far */
	maxai = 0 ;
	i = 0 ;
	argr = argc - 1 ;
	while (argr > 0) {

	    argp = argv[++i] ;
	    argr -= 1 ;
	    argl = strlen(argp) ;

	    f_optminus = (*argp == '-') ;
	    f_optplus = (*argp == '+') ;
	    if ((argl > 0) && (f_optminus || f_optplus)) {

	        if (argl > 1) {

	            if (isdigit(argp[1])) {

	                if (((argl - 1) > 0) && 
	                    (cfdeci(argp + 1,argl - 1,&argnum) < 0))
	                    goto badargval ;

	            } else {

#if	CF_DEBUGS
	                eprintf("main: got an option\n") ;
#endif

	                aop = argp + 1 ;
	                aol = argl - 1 ;
	                akp = aop ;
	                f_optequal = FALSE ;
	                if ((avp = strchr(aop,'=')) != NULL) {

#if	CF_DEBUGS
	                    eprintf("main: got an option key w/ a value\n") ;
#endif

	                    akl = avp - aop ;
	                    avp += 1 ;
	                    avl = aop + aol - avp ;
	                    f_optequal = TRUE ;

#if	CF_DEBUGS
	                    eprintf("main: aol=%d avp=\"%s\" avl=%d\n",
	                        aol,avp,avl) ;

	                    eprintf("main: akl=%d akp=\"%s\"\n",
	                        akl,akp) ;
#endif

	                } else {

	                    akl = aol ;
	                    avl = 0 ;

	                }

#if	CF_DEBUGS
	                eprintf("main: about to check for a key word match\n") ;
#endif

	                if ((kwi = optmatch(argopts,akp,akl)) >= 0) {

#if	CF_DEBUGS
	                    eprintf("main: option keyword, kwi=%d opt=%s\n",
	                        kwi,argopts[kwi]) ;
#endif

	                    switch (kwi) {

	                    case argopt_tmpdir:
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl)
	                                pip->tmpdir = avp ;

	                        } else {

	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                pip->tmpdir = argp ;

	                        }

	                        break ;

/* version */
	                    case argopt_version:
	                        f_version = TRUE ;
	                        break ;

	                    case argopt_vv:
	                        f_version = TRUE ;
	                        f_makedate = TRUE ;
	                        break ;

/* verbose mode */
	                    case argopt_verbose:
	                        pip->verboselevel = 1 ;
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl > 0) {

	                                rs = cfdeci(avp,avl,
	                                    &pip->verboselevel) ;

	                                if (rs < 0)
	                                    goto badargval ;

	                            }

	                        }

	                        break ;

/* program root */
	                    case argopt_root:
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl)
	                                pip->programroot = avp ;

	                        } else {

	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                pip->programroot = argp ;

	                        }

	                        break ;

/* configuration file */
	                    case argopt_config:
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl)
	                                configfname = avp ;

	                        } else {

	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                configfname = argp ;

	                        }

	                        break ;

/* log file */
	                    case argopt_logfile:
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl) {

	                                logfile_type = GETFNAME_TYPELOCAL ;
	                                strwcpy(logfname,avp,avl) ;

	                            }

	                        } else {

	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)  {

	                                logfile_type = GETFNAME_TYPELOCAL ;
	                                strwcpy(logfname,argp,argl) ;

	                            }
	                        }

	                        break ;

/* parameter */
	                    case argopt_paramtab:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)  {

	                            paramfile_type = GETFNAME_TYPELOCAL ;
	                            strwcpy(paramfname,argp,argl) ;

	                        }

	                        break ;

/* debug level */
	                    case argopt_debug:
	                        pip->debuglevel = 1 ;
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl > 0) {

	                                rs = cfdeci(avp,avl, &pip->debuglevel) ;

	                                if (rs < 0)
	                                    goto badargval ;

	                            }
	                        }

	                        break ;

/* debug schedule */
	                    case argopt_ds:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

#ifdef	COMMENT /* old idea */
	                        if (argl)  {

	                            debugfile_type = GETFNAME_TYPELOCAL ;
	                            strwcpy(debugfname,argp,argl) ;

	                        }
#endif /* COMMENT */

	                        if (argl) {

	                            debugwin_setopt(&pip->dw,
	                                argp,argl) ;

	                        }

	                        break ;

	                    case argopt_icount:
	                    case argopt_count:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)  {

	                            if (cfdecull(argp,argl,&ullw) < 0)
	                                goto badargval ;

	                            pip->ninstr = ullw ;

	                        }

	                        break ;

	                    case argopt_et:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)  {

	                            pip->etfname = mallocstr(argp) ;

	                        }

	                        break ;

/* set the selection */
	                    case argopt_select:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

	                            rs = proginfo_setselect(pip,
	                                argp,argl) ;

#if	CF_DEBUGS
	                            eprintf(
					"main: proginfo_setselect() rs=%d\n",
	                                rs) ;
#endif

	                            if (rs < 0)
	                                goto badargval ;

	                        }

	                        break ;


/* skip instructions (fast forward through SimpleSim)
							     */
	                    case argopt_skip:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

	                            if (cfdecull(argp,argl,&ullw) < 0)
	                                goto badargval ;

	                            skipinstr = ullw ;

	                        }

	                        break ;

/* handle all keyword defaults */
	                    default:
	                        bprintf(efp,"%s: option (%s) not supported\n",
	                            pip->progname,akp) ;

	                        goto badarg ;

	                    } /* end switch */

	                } else {

#if	CF_DEBUGS
	                    eprintf("main: got an option key letter\n") ;
#endif

	                    while (akl--) {

#if	CF_DEBUGS
	                        eprintf("main: option key letters\n") ;
#endif

	                        switch (*akp) {

/* debug */
	                        case 'D':
	                            pip->debuglevel = 1 ;
	                            if (f_optequal) {

	                                f_optequal = FALSE ;
	                                if (avl > 0) {

	                                    rs = cfdeci(avp,avl,
	                                        &pip->debuglevel) ;

	                                    if (rs < 0)
	                                        goto badargval ;

	                                }

	                            }

	                            break ;

/* version */
	                        case 'V':
	                            f_version = TRUE ;
	                            break ;

/* configuration file */
	                        case 'C':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                configfname = argp ;

	                            break ;

/* maximum number of clocks */
	                        case 'c':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                if (cfdecull(argp,argl,&ullw) < 0)
	                                    goto badargval ;

	                                maxclocks = ullw ;

	                            }

	                            break ;

/* MINT executable! (stupid thing) */
	                        case 'e':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                mintfile_type = GETFNAME_TYPELOCAL ;
	                                strwcpy(mintfname,argp,argl) ;

	                            }

	                            break ;

/* optional jobname */
	                        case 'j':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                strwcpy(pip->jobname,argp,
	                                    MAX(argl,JOBNAMELEN)) ;

	                            }

	                            break ;

/* program mode */
	                        case 'm':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                progmodestr = argp ;

	                            break ;

/* execute only a certain number of instructi
								    ons */
	                        case 'n':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                if (cfdecull(argp,argl,&ullw) < 0)
	                                    goto badargval ;

	                                pip->ninstr = ullw ;

	                            }

	                            break ;

/* options */
	                        case 'o':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                rs = proginfo_setopt(pip,
	                                    argp,argl) ;

	                                if (rs < 0)
	                                    goto badargval ;

	                            }

	                            break ;

/* parameter file name */
	                        case 'p':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                paramfile_type = GETFNAME_TYPELOCAL ;
	                                strwcpy(paramfname,argp,argl) ;

	                            }

	                            break ;

/* quiet mode */
	                        case 'q':
	                            pip->f.quiet = TRUE ;
	                            break ;

/* select some instructions */
	                        case 's':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                rs = proginfo_setselect(pip,
	                                    argp,argl) ;

#if	CF_DEBUGS
	                                eprintf("main: proginfo_setselect() "
						"rs=%d\n",
	                                    rs) ;
#endif

	                                if (rs < 0)
	                                    goto badargval ;

	                            }

	                            break ;

/* verbose mode */
	                        case 'v':
	                            pip->verboselevel = 1 ;
	                            if (f_optequal) {

	                                f_optequal = FALSE ;
	                                if (avl > 0) {

	                                    rs = cfdeci(avp,avl,
	                                        &pip->verboselevel) ;

	                                    if (rs < 0)
	                                        goto badargval ;

	                                }

	                            }

	                            break ;

	                        case '?':
	                            f_usage = TRUE ;
	                            break ;

	                        default:
	                            f_usage = TRUE ;
	                            ex = EX_USAGE ;
	                            bprintf(efp,"%s: unknown option - %c\n",
	                                pip->progname,*aop) ;

	                        } /* end switch */

	                        akp += 1 ;

	                    } /* end while */

	                } /* end if (individual option key letters) */

	            } /* end if (digits as argument or not) */

	        } else {

/* we have a plus or minux sign character alone on the command line */

	            if (i < MAXARGINDEX) {

	                BSET(argpresent,i) ;
	                maxai = i ;
	                npa += 1 ;	/* increment position count */

	            }

	        } /* end if */

	    } else {

	        if (i < MAXARGINDEX) {

	            BSET(argpresent,i) ;
	            maxai = i ;
	            npa += 1 ;

	        } else {

	            if (! f_extra) {

			ex = EX_USAGE ;
	                f_extra = TRUE ;
	                bprintf(efp,"%s: extra arguments specified\n",
	                    pip->progname) ;

	            }
	        }

	    } /* end if (key letter/word or positional) */

	} /* end while (all command line argument processing) */


/* load the positional arguments */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: checking for positional arguments\n") ;
#endif

	pan = 0 ;
	for (i = 0 ; i <= maxai ; i += 1) {

	    if (BTST(argpresent,i)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1) eprintf(
	            "main: got a positional argument i=%d pan=%d arg=%s\n",
	            i,pan,argv[i]) ;
#endif

	        switch (pan) {

	        case 0:
	            if ((strlen(argv[i]) > 0) && (argv[i][0] != '-'))
	                configfname = argv[i] ;

	            break ;

	        default:
	            ex = EX_USAGE ;
	            f_usage = TRUE ;
	            bprintf(efp,
	                "%s: extra positional arguments ignored\n",
	                pip->progname) ;

	        } /* end switch */

	        pan += 1 ;

	    } /* end if (got a positional argument) */

	} /* end for (loading positional arguments) */



	if (f_version || (pip->debuglevel >= 1))
	    bprintf(efp,"%s: version %s\n",
	        pip->progname,VERSION) ;

	if (f_makedate || (pip->debuglevel >= 1)) {

	    cl = makedate_get(levosim_makedate,&cp) ;

	    bprintf(efp,"%s: makedate %w\n",
	        pip->progname,cp,cl) ;

	}

	if (f_usage)
	    goto usage ;

	if (f_version)
	    goto retearly ;

	if (pip->debuglevel > 0)
	    bprintf(efp,"%s: debuglevel=%d\n",
	        pip->progname,pip->debuglevel) ;


#if	CF_DEBUG
	if (pip->debuglevel >= 2)
	    eprintf("main: debuglevel=%d\n",pip->debuglevel) ;
#endif


/* early things */

	debugwin_setlevel(&pip->dw,pip->debuglevel) ;


/* get our program root */

	if (pip->programroot == NULL) {

	    programroot = getenv(PROGRAMROOTVAR1) ;

	    if (programroot == NULL)
	        programroot = getenv(PROGRAMROOTVAR2) ;

	    if (programroot == NULL)
	        programroot = getenv(PROGRAMROOTVAR3) ;

	    if (programroot == NULL)
	        programroot = PROGRAMROOT ;

	    pip->programroot = programroot ;

	} else {

	    f_programroot = TRUE ;
	    programroot = pip->programroot ;

	}


/* let's get the program mode here (if we do not have one already) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: specified progmode=%s\n",
	        progmodestr) ;
#endif

	if (progmodestr == NULL) {

	    sl = sfbasename(pip->progname,-1,&cp) ;

	    if (sl > 0) {

	        strwcpy(buf,cp,MIN(BUFLEN,MAXPATHLEN)) ;

	        if ((cp = strrchr(buf,'.')) != NULL)
	            *cp = '\0' ;

	        if ((cp = strchr(buf,'-')) != NULL) {

	            strwcpy(buf2,buf,-1) ;

	            cp = strchr(buf2,'-') ;

	            strwcpy(buf,(cp + 1),-1) ;

	            progmodestr = buf ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("main: w/ progname progmode=%s\n",
	                    progmodestr) ;
#endif

	        } else {

	            progmodestr = buf ;
	            if ((cp = getenv(PROGMODEVAR)) != NULL)
	                progmodestr = cp ;

	        }

	    } /* end if */

	} /* end if (program mode was specified) */

	if (progmodestr == NULL)
	    progmodestr = getenv(PROGMODEVAR) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: before default progmode=%s\n",
	        progmodestr) ;
#endif

	if (progmodestr == NULL)
	    progmodestr = "levosim" ;

	for (i = 0 ; progmodes[i] != NULL ; i += 1) {

	    if (strcasecmp(progmodestr,progmodes[i]) == 0)
	        break ;

	} /* end for */

	pip->progmode = (progmodes[i] != NULL) ? i : PROGMODE_LEVOSIM ;

	if (pip->debuglevel > 0)
	    bprintf(pip->efp,"%s: program mode=%s\n",
	        pip->progname, progmodes[pip->progmode]) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: progmode=%s (%d)\n",
	        progmodes[pip->progmode],
	        pip->progmode) ;
#endif


/* get some host/user information */

	rs = userinfo(&u,userbuf,USERINFO_LEN) ;

	pip->nodename = u.nodename ;
	pip->domainname = u.domainname ;
	pip->username = u.username ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {
	    eprintf("main: UI rs=%d\n",rs) ;
	    eprintf("main: UI nodename=%s\n",pip->nodename) ;
	    eprintf("main: UI name=%s\n",u.name) ;
	}
#endif


/* get PWD and store */

	if ((rs = getpwd(tmpfname,BUFLEN)) < 0) {

	    tmpfname[0] = '.' ;
	    tmpfname[1] = '\0' ;
	}

	pip->pwd = mallocstr(tmpfname) ;


/* see if we have a parameter file specified from our environment */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: 0 paramfname=%s\n",paramfname) ;
#endif

	if ((paramfname[0] == '\0') &&
	    ((cp = getenv(PARAMVAR)) != NULL)) {

	    paramfile_type = GETFNAME_TYPELOCAL ;
	    strwcpy(paramfname,cp,MAXPATHLEN) ;

	} /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: 1 paramfname=%s\n",paramfname) ;
#endif


/* set things that take precedence over things in configuration file */



/* prepare to store configuration variable lists */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: general variables\n") ;
#endif

	if ((rs = vecstr_init(&defines,10,VECSTR_PORDERED)) < 0)
	    goto badlistinit ;

	if ((rs = vecstr_init(&unsets,10,VECSTR_PNOHOLES)) < 0) {

	    vecstr_free(&defines) ;

	    goto badlistinit ;
	}

	if ((rs = vecstr_init(&exports,10,VECSTR_PNOHOLES)) < 0) {

	    vecstr_free(&defines) ;

	    vecstr_free(&unsets) ;

	    goto badlistinit ;
	}


/* create the values for the file schedule searching */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: file schedule variables\n") ;
#endif

	schedvar_init(&svars) ;

	schedvar_add(&svars,"p",pip->programroot) ;

	schedvar_add(&svars,"e","etc") ;

	schedvar_add(&svars,"n",SEARCHNAME) ;


/* load up some initial environment that everyone should have! */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: about to do INITFNAME=%s\n",INITFNAME) ;
#endif

	procfileenv(pip->programroot,INITFNAME,&exports) ;


/* find a configuration file if we have one */

	if (configfname == NULL)
	    configfname = getenv(CONFIGVAR) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {

	    eprintf("main: checking for configuration file\n") ;

	    eprintf("main: 0 CF=%s\n",configfname) ;

	}
#endif /* CF_DEBUG */

	rs = SR_NOEXIST ;
	if ((configfname == NULL) || (configfname[0] == '\0')) {

	    configfname = CONFIGFNAME ;

	    if ((sl = permsched(sched1,&svars,configfname,R_OK,
	        tmpfname,MAXPATHLEN)) < 0) {

	        if ((sl = permsched(sched3,&svars,configfname,R_OK,
	            tmpfname,MAXPATHLEN)) > 0)
	            configfname = tmpfname ;

	    } else if (sl > 0)
	        configfname = tmpfname ;

	    rs = sl ;

	} else {

	    sl = getfname(pip->programroot,configfname,
	        GETFNAME_TYPELOCAL,tmpfname) ;

	    if (sl > 0)
	        configfname = tmpfname ;

	} /* end if */

	if (configfname == tmpfname) {

	    f_freeconfigfname = TRUE ;
	    configfname = mallocstr(tmpfname) ;

	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1) {

	    eprintf("main: find rs=%d\n",sl) ;

	    eprintf("main: 1 CF=%s\n",configfname) ;

	}
#endif /* CF_DEBUG */

/* read in the configuration file if we have one */

	if ((rs >= 0) || (perm(configfname,-1,-1,NULL,R_OK) >= 0)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: configuration file \"%s\"\n",
	            configfname) ;
#endif

	    if (pip->debuglevel > 0)
	        bprintf(efp,"%s: configfile=%s\n",
	            pip->progname,configfname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: configfile_init()\n") ;
#endif

	    rs = configfile_init(&cf,configfname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: configfile_init() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        goto badconfig ;

	    f_configfile = TRUE ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: we have a good configuration file\n") ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: varsub_init d\n") ;
#endif

/* we must set this mode to 'VARSUB_MBADNOKEY' so that a miss is noticed */

	    varsub_init(&vsh_d,VARSUB_MBADNOKEY) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: varsub_init e\n") ;
#endif

	    varsub_init(&vsh_e,0) ;

#if	CF_DEBUG && 0
	    if (pip->debuglevel > 1) {
	        eprintf("main: program environment\n") ;

	        for (i = 0 ; envv[i] != NULL ; i += 1)
	            eprintf("main: ENV %d> %s\n",i,envv[i]) ;

	        eprintf("main: varsub_loadenv\n") ;
	    }
#endif /* CF_DEBUG */

	    varsub_loadenv(&vsh_e,envv) ;

#if	CF_DEBUG && CF_VARSUBDUMP
	    if (pip->debuglevel > 1) {

	        eprintf("main: 0 for\n") ;

	        varsub_dumpfd(&vsh_e,-1) ;

	    }
#endif /* CF_DEBUG */


/* program root from configuration file */

	    if ((cf.root != NULL) && (! f_programroot)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: processing CF programroot\n") ;
#endif

	        if (((sl = varsub_subbuf(&vsh_d,&vsh_e,cf.root,
	            -1,buf,BUFLEN)) > 0) &&
	            ((l2 = expander(pip,buf,sl,buf2,BUFLEN)) > 0)) {

	            pip->programroot = mallocstrn(buf2,l2) ;

	        }

	    } /* end if (configuration file program root) */


/* loop through the DEFINEd variables in the configuration file */

	    for (i = 0 ; vecstr_get(&cf.defines,i,&cp) >= 0 ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf( "main: 0 top, cp=%s\n",cp) ;
#endif

	        sl = varsub_subbuf(&vsh_d,&vsh_e,cp,-1,buf,BUFLEN) ;

	        if ((sl > 0) &&
	            ((l2 = expander(pip,buf,sl,buf2,BUFLEN)) > 0)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel > 1) eprintf(
	                "main: 0 about to merge\n") ;
#endif

	            varsub_merge(&vsh_d,&defines,buf2,l2) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel > 1) eprintf(
	                "main: 0 out of merge\n") ;
#endif

	        } /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf( "main: 0 bot\n") ;
#endif

	    } /* end for (defines) */

#if	CF_DEBUG && 0
	    if (pip->debuglevel > 1)
	        eprintf("main: done w/ defines\n") ;
#endif


/* all of the rest of the configuration file stuff */

	    if ((cf.workdir != NULL) && (pip->workdir == NULL)) {

	        if (((sl = varsub_subbuf(&vsh_d,&vsh_e,cf.workdir,
	            -1,buf,BUFLEN)) > 0) &&
	            ((l2 = expander(pip,buf,sl,buf2,BUFLEN)) > 0)) {

	            pip->workdir = mallocstrn(buf2,l2) ;

	        }

	    } /* end if (config file working directory) */


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1) {

	        eprintf("main: so far logfname=%s\n",logfname) ;

	        eprintf("main: about to get config log filename\n") ;
	    }
#endif

	    if ((cf.logfname != NULL) && (logfile_type < 0)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: CF logfilename=%s\n",cf.logfname) ;
#endif

	        if (((sl = varsub_subbuf(&vsh_d,&vsh_e,cf.logfname,
	            -1,buf,BUFLEN)) > 0) &&
	            ((l2 = expander(pip,buf,sl,buf2,BUFLEN)) > 0)) {

	            strwcpy(logfname,buf2,l2) ;

	            if (strchr(logfname,'/') != NULL)
	                logfile_type = GETFNAME_TYPEROOT ;

	        }

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: processed CF logfilename=%s\n",logfname) ;
#endif

	    } /* end if (configuration file log filename) */


	    if ((cf.sendmail != NULL) && (mintfname[0] == '\0')) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: CF processing MINT executable\n") ;
#endif

	        if (((sl = varsub_subbuf(&vsh_d,&vsh_e,cf.sendmail,
	            -1,buf,BUFLEN)) > 0) &&
	            ((l2 = expander(pip,buf,sl,buf2,BUFLEN)) > 0)) {

	            strwcpy(mintfname,buf2,l2) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("main: CF MINTEXEC mintfname=%s\n",
	                    mintfname) ;
#endif

	            mintfile_type = GETFNAME_TYPELOCAL ;
	            if (strchr(mintfname,'/') != NULL)
	                mintfile_type = GETFNAME_TYPELOCAL ;

	        }

	    } /* end if (mintfname) */


	    if ((cf.paramfname != NULL) && (paramfname[0] == '\0')) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: CF processing PARAMTAB \n") ;
#endif

	        if (((sl = varsub_subbuf(&vsh_d,&vsh_e,cf.paramfname,
	            -1,buf,BUFLEN)) > 0) &&
	            ((l2 = expander(pip,buf,sl,buf2,BUFLEN)) > 0)) {

	            strwcpy(paramfname,buf2,l2) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("main: CF PARAMTAB s&e paramfname=%s\n",
	                    paramfname) ;
#endif

	            paramfile_type = GETFNAME_TYPELOCAL ;
	            if (strchr(paramfname,'/') != NULL)
	                paramfile_type = GETFNAME_TYPEROOT ;

	        }

	    } /* end if (paramfile) */


/* what about an 'environ' file ? */

	    if (cf.envfname != NULL) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: 1 envfile=%s\n",cf.envfname) ;
#endif

	        procfileenv(pip->programroot,cf.envfname,&exports) ;

	    } else
	        procfile(pip,procfileenv,pip->programroot,&svars,
	            ENVFNAME,&exports) ;

	    f_procfileenv = TRUE ;


/* "do" any 'paths' file before we process the environment variables */

	    if (cf.pathsfname != NULL) {
	        procfilepaths(pip->programroot,cf.pathsfname,&exports) ;
	    } else
	        procfile(pip,procfilepaths,pip->programroot,&svars,
	            PATHSFNAME,&exports) ;

	    f_procfilepaths = TRUE ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: processing CF loglen\n") ;
#endif

	    if ((cf.loglen >= 0) && (loglen < 0))
	        loglen = cf.loglen ;


/* loop through the UNSETs in the configuration file */

	    for (i = 0 ; vecstr_get(&cf.unsets,i,&cp) >= 0 ; i += 1) {

	        if (cp == NULL) continue ;

	        vecstr_add(&unsets,cp,-1) ;

	        rs = vecstr_finder(&exports,cp,vstrkeycmp,NULL) ;

	        if (rs >= 0)
	            vecstr_del(&exports,rs) ;

	    } /* end for */


/* loop through the EXPORTed variables in the configuration file */

	    for (i = 0 ; vecstr_get(&cf.exports,i,&cp) >= 0 ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: 1 about to sub> %s\n",cp) ;
#endif

	        sl = varsub_subbuf(&vsh_d,&vsh_e,cp,-1,buf,BUFLEN) ;

	        if ((sl > 0) &&
	            ((l2 = expander(pip,buf,sl,buf2,BUFLEN)) > 0)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("main: 1 about to merge> %w\n",buf,l2) ;
#endif

	            varsub_merge(NULL,&exports,buf2,l2) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel > 1)
	                eprintf("main: 1 done merging\n") ;
#endif

#if	CF_DEBUG && CF_VARSUBDUMP && 0
	            if (pip->debuglevel > 1) {

	                eprintf("varsub_merge: VSA_D so far \n") ;

	                varsub_dump(&vsh_d,-1) ;

	                eprintf("varsub_merge: VSA_E so far \n") ;

	                varsub_dumpfd(&vsh_e,-1) ;

	            } /* end if (debuglevel) */
#endif /* CF_DEBUG */

	        } /* end if (merging) */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel > 1)
	            eprintf("main: done subbing & merging\n") ;
#endif

	    } /* end for (exports) */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: done w/ exports\n") ;
#endif


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: processing CF freeing data structures\n") ;
#endif


	    varsub_free(&vsh_d) ;

	    varsub_free(&vsh_e) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: processing CF free\n") ;
#endif

	    configfile_free(&cf) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1) {

	        eprintf("main: dumping defines\n") ;

	        for (i = 0 ; vecstr_get(&defines,i,&cp) >= 0 ; i += 1)
	            eprintf("main: define> %s\n",cp) ;

	        eprintf("main: dumping exports\n") ;

	        for (i = 0 ; vecstr_get(&exports,i,&cp) >= 0 ; i += 1)
	            eprintf("main: export> %s\n",cp) ;

	    } /* end if (CF_DEBUG) */
#endif /* CF_DEBUG */


	} /* end if (accessed the configuration file) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: finished with any configfile stuff\n") ;
#endif


	if (pip->programroot == NULL)
	    pip->programroot = programroot ;

	if (pip->debuglevel > 0)
	    bprintf(efp,"%s: programroot=%s\n",
	        pip->progname,pip->programroot) ;


/* load up the new program root in the file search schedule variables */

	schedvar_add(&svars,"p",pip->programroot) ;


/* load up some environment and execution paths if we have not already */

	if (! f_procfileenv)
	    procfile(pip,procfileenv,pip->programroot,&svars,
	        ENVFNAME,&exports) ;


	if (! f_procfilepaths)
	    procfile(pip,procfilepaths,pip->programroot,&svars,
	        PATHSFNAME,&exports) ;


/* check program parameters */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: checking program parameters\n") ;
#endif

	if ((pip->tmpdir == NULL) || (pip->tmpdir[0] == '\0')) {

	    if ((pip->tmpdir = getenv("TMPDIR")) == NULL)
	        pip->tmpdir = TMPDIR ;

	} /* end if (tmpdir) */


/* can we access the working directory ? */

	if (pip->workdir == NULL)
	    pip->workdir = WORKDIR ;

	else if (pip->workdir[0] == '\0')
	    pip->workdir = "." ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: access working directory \"%s\"\n",pip->workdir) ;
#endif

	if ((perm(pip->workdir,-1,-1,NULL,X_OK) < 0) || 
	    (perm(pip->workdir,-1,-1,NULL,R_OK) < 0))
	    goto badworking ;


/* do we have an activity log file ? */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: 0 logfname=%s nodename=%s\n",
	        logfname,pip->nodename) ;
#endif

	i = 0 ;
	i += storebuf_strw(buf,BUFLEN,i,pip->nodename,-1) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: LOGID 1 buf=%s\n",buf) ;
#endif

	i += storebuf_dec(buf,BUFLEN,i,pip->pid) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: LOGID 2 buf=%s\n",buf) ;
#endif

	pip->logid = mallocstrn(buf,MAX(i,LOGIDLEN)) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: logid=%s\n",pip->logid) ;
#endif

	rs = SR_BAD ;
	if (logfname[0] == '\0') {

	    logfile_type = GETFNAME_TYPEROOT ;
	    strcpy(logfname,LOGFNAME) ;

	}

	sl = getfname(pip->programroot,logfname,logfile_type,tmpfname) ;

	if (sl > 0)
	    strwcpy(logfname,tmpfname,sl) ;

	rs = logfile_open(&pip->lh,logfname,0666,pip->logid) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: 1 logfname=%s rs=%d\n",logfname,rs) ;
#endif

	if (rs >= 0) {

	    struct utsname	un ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: we opened a logfile\n") ;
#endif

	    pip->f.log = TRUE ;
	    if (pip->debuglevel > 0)
	        bprintf(efp,"%s: logfile=%s\n",pip->progname,logfname) ;

/* we opened it, maintenance this log file if we have to */

	    if (loglen < 0)
	        loglen = LOGLEN ;

	    logfile_checklen(&pip->lh,loglen) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: we checked its length\n") ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: making log entry\n") ;
#endif

	    logfile_printf(&pip->lh,"%s %s\n",
	        timestr_log(daytime,timebuf),
	        BANNER) ;

	    logfile_printf(&pip->lh,"%-14s %s/%s\n",
	        pip->progname,
	        VERSION,(u.f.sysv_ct) ? "SYSV" : "BSD") ;

	    (void) u_uname(&un) ;

	    logfile_printf(&pip->lh,"ostype=%s os=%s(%s) domain=%s\n",
	        (u.f.sysv_rt ? "SYSV" : "BSD"),
	        un.sysname,un.release,
	        pip->domainname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1) {
	        eprintf("main: name=%s\n",u.name) ;
	        eprintf("main: gecosname=%s\n",u.gecosname) ;
	        eprintf("main: fullname=%s\n",u.fullname) ;
	    }
#endif

	    buf[0] = '\0' ;
	    if ((u.name != NULL) && (u.name[0] != '\0')) {
	        sprintf(buf,"(%s)",u.name) ;

	    } else if ((u.gecosname != NULL) && (u.gecosname[0] != '\0')) {
	        sprintf(buf,"(%s)",u.gecosname) ;

	    } else if ((u.fullname != NULL) && (u.fullname[0] != '\0'))
	        sprintf(buf,"(%s)",u.fullname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: LF name=%s\n",buf) ;
#endif

	    logfile_printf(&pip->lh,"%s!%s %s\n",
	        u.nodename,u.username,buf) ;

	    if (pip->progmode > 0)
	        logfile_printf(&pip->lh,"program mode=%s\n",
	            progmodes[pip->progmode]) ;

	    logfile_printf(&pip->lh,"pid=%d\n",
	        pip->pid) ;

	    logfile_printf(&pip->lh,"pwd=%s\n",
	        pip->pwd) ;

	} /* end if (we have a log file or not) */


	if (f_configfile)
	    logfile_printf(&pip->lh,"configfile=%s\n",configfname) ;


/* find and open the simulator parameter file */

	rs = SR_NOEXIST ;
	if (paramfname[0] == '\0') {

	    paramfile_type = GETFNAME_TYPEROOT ;
	    strcpy(paramfname,PARAMFNAME) ;

	    if ((sl = permsched(sched2,&svars,paramfname,R_OK,
	        tmpfname,MAXPATHLEN)) < 0) {

	        if ((sl = permsched(sched3,&svars,paramfname,R_OK,
	            tmpfname,MAXPATHLEN)) > 0)
	            strcpy(paramfname,tmpfname) ;

	    } else if (sl > 0)
	        strcpy(paramfname,tmpfname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: 2 paramfname=%s\n",paramfname) ;
#endif

	    rs = sl ;

	} else {

	    if (paramfile_type < 0)
	        paramfile_type = GETFNAME_TYPELOCAL ;

	    sl = getfname(pip->programroot,paramfname,paramfile_type,tmpfname) ;

	    if (sl > 0)
	        strwcpy(paramfname,tmpfname,sl) ;

	} /* end if */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: 3 paramfname=%s\n",paramfname) ;
#endif

	if ((rs >= 0) || (perm(paramfname,-1,-1,NULL,R_OK) >= 0)) {

	    if (pip->debuglevel > 0)
	        bprintf(efp,"%s: paramfname=%s\n",pip->progname,paramfname) ;

	    logfile_printf(&pip->lh,"paramfile=%s\n",paramfname) ;

	    rs = paramfile_open(&pfile,(const char **) envv,paramfname) ;

	    if (rs < 0) {

	        logfile_printf(&pip->lh,"bad (%d) parameter file\n",rs) ;

	        bprintf(efp,"%s: paramfile=%s\n",
	            pip->progname,paramfname) ;

	        bprintf(efp,"%s: bad (%d) parameter file\n",
	            pip->progname,rs) ;

	    } else
	        pip->f.params = TRUE ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("main: we have a parameter file=%s\n",paramfname) ;
#endif

	    if (pip->f.params) {

	        paramfile_setdefines(&pfile,&defines) ;

	    } /* end if (we have param file) */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1) {

		PARAMFILE_ENTRY		pe ;

		int	vl ;

		char	pebuf[PEBUFLEN + 1] ;
		char	vbuf[VBUFLEN + 1] ;


	        if (pip->f.params) {

	            PARAMFILE_CURSOR	cur ;


/* enumeration of raw entries */

	            eprintf("main: parameter enum \n") ;

	            paramfile_cursorinit(&pfile,&cur) ;

	            for (i = 0 ; 
			paramfile_curenum(&pfile,&cur,&pe,pebuf,PEBUFLEN) >= 0 ; 
	                i += 1) {

	                if (pe.key != NULL)
	                    eprintf("main: P> key=%s value=>%s<\n",
	                        pe.key, pe.orig) ;

	            } /* end for */

	            paramfile_cursorfree(&pfile,&cur) ;


	            vl = paramfile_fetch(&pfile,"pcs",NULL,vbuf,VBUFLEN) ;

	            if (vl >= 0)
	                eprintf("main: PF pcs=%w\n",vbuf,vl) ;

	            vl = paramfile_fetch(&pfile,"dv",NULL,vbuf,VBUFLEN) ;

	            if (vl >= 0)
	                eprintf("main: PF dv=%w\n",vbuf,vl) ;

	        } else
	            eprintf("main: no parameter file\n") ;

	    }
#endif /* CF_DEBUG */

	} /* end if (accessing a 'paramfile' file) */


/* set an environment variable for the program run mode */

#ifdef	COMMENT
	if ((rs = vecstr_finder(&exports,"RUNMODE",vstrkeycmp,&cp)) >= 0)
	    vecstr_del(&exports,rs) ;

	vecstr_add(&exports,"RUNMODE=tcpmux",-1) ;
#endif /* COMMENT */

#if	(! CF_PRECISEENV)

	if (vecstr_finder(&exports,"HZ",vstrkeycmp,NULL) < 0) {

	    sl = bufprintf(tmpfname,MAXPATHLEN,"HZ=%d",CLK_TCK) ;

	    vecstr_add(&exports,tmpfname,sl) ;

	}


	if (vecstr_finder(&exports,"PATH",vstrkeycmp,NULL) < 0) {

	    sl = bufprintf(tmpfname,MAXPATHLEN,"PATH=%s",DEFPATH) ;

	    vecstr_add(&exports,tmpfname,sl) ;

	}

#endif /* (! CF_PRECISEENV) */


/* clean up some stuff we will no longer need */

	schedvar_free(&svars) ;


/* OK, initialize all remaining stuff */

	if (pip->jobname[0] == '\0') {

	    cp = getenv(JOBNAMEVAR) ;

	    if ((cp != NULL) && (cp[0] != '\0'))
	        strwcpy(pip->jobname,cp,JOBNAMELEN) ;

	} /* end if (finding a jobname) */

	if (pip->etfname == NULL) {

	    cp = getenv(ETFNAMEVAR) ;

	    if ((cp != NULL) && (cp[0] != '\0'))
	        pip->etfname = mallocstr(cp) ;

	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main: etfname=%s\n",pip->etfname) ;
#endif


/* we are done initializing */



/* do it */

	{
	    struct sigaction	sigs ;

	    sigset_t	signalmask ;

	    bfile	outfile, *ofp = &outfile ;

	    char	*targetname = NULL ;


	    rs = vecstr_finder(&defines,"PROGNAME",vstrkeycmp,&cp) ;

	    if (rs >= 0) {

	        if ((targetname = strchr(cp,'=')) != NULL)
	            targetname += 1 ;

	    }


#if	CF_MASTERDEBUG && CF_SIGDUMP
	    sigdumper(SIGDUMPFILE,pip->pid,"s2") ;
#endif

/* pop some signals that we do not want */

#if	CF_NOSIGINT

	    (void) memset(&sigs,0,sizeof(struct sigaction)) ;

	    (void) uc_sigemptyset(&signalmask) ;

	    sigs.sa_handler = NULL ;
	    sigs.sa_sigaction = int_inter ;
	    sigs.sa_mask = signalmask ;
	    sigs.sa_flags = SA_SIGINFO ;
	    u_sigaction(SIGINT,&sigs,NULL) ;

#endif /* CF_NOSIGINT */

#if	CF_NOSIGHUP

	    (void) memset(&sigs,0,sizeof(struct sigaction)) ;

	    (void) uc_sigemptyset(&signalmask) ;

	    sigs.sa_handler = NULL ;
	    sigs.sa_sigaction = int_hanger ;
	    sigs.sa_mask = signalmask ;
	    sigs.sa_flags = SA_SIGINFO ;
	    u_sigaction(SIGHUP,&sigs,NULL) ;

#endif /* CF_NOSIGHUP */

#if	CF_NOSIGPIPE

	    (void) memset(&sigs,0,sizeof(struct sigaction)) ;

	    (void) uc_sigemptyset(&signalmask) ;

	    (void) uc_sigaddset(&signalmask,SIGPIPE) ;

	    sigs.sa_handler = NULL ;
	    sigs.sa_sigaction = int_piper ;
	    sigs.sa_mask = signalmask ;
	    sigs.sa_flags = SA_SIGINFO ;
	    u_sigaction(SIGPIPE,&sigs,NULL) ;

#endif /* CF_NOSIGPIPE */

#if	CF_MASTERDEBUG && CF_SIGDUMP
	    sigdumper(SIGDUMPFILE,pip->pid,"s3") ;
#endif

/* continue */

	    pip->ofp = NULL ;
#if	CF_STDOUT
	    rs = bopen(ofp,BIO_STDOUT,"dwcta",0666) ;

	    if (rs >= 0)
	        pip->ofp = ofp ;
#endif


	    rs = process(pip,targetname,&pfile,&exports,
	        maxclocks,skipinstr) ;


/* is the an English language translation available ? */

	    cp = NULL ;
	    for (i = 0 ; rsstrings[i].str != NULL ; i += 1) {

	        if (rsstrings[i].rs == rs) {

	            cp = rsstrings[i].str ;
	            break ;
	        }

	    } /* end for */

	    if (cp == NULL)
	        cp = "_" ;

	    u_time(&daytime) ;

	    logfile_printf(&pip->lh,
	        "%s exiting> (%d)\n", 
	        timestr_log(daytime,timebuf), rs) ;

	    logfile_printf(&pip->lh, "| %s\n", cp) ;

	    logfile_printf(&pip->lh,
	        "total elapsed time %s\n",
	        timestr_elapsed((daytime - pip->starttime),timebuf)) ;

#if	CF_STDOUT
	    if (pip->ofp != NULL) {

	        bprintf(ofp, "exiting> %s (%d)\n",cp,rs) ;

	        bprintf(ofp,"process result=%d\n",rs) ;

	        bprintf(ofp,"elapsed time %s\n",
	            timestr_elapsed((daytime - pip->starttime),timebuf)) ;

	        bclose(ofp) ;

	    }
#endif /* CF_STDOUT */

#if	CF_MASTERDEBUG && CF_SIGDUMP
	    sigdumper(SIGDUMPFILE,pip->pid,"s9") ;
#endif

	} /* end block */


	ex = (rs >= 0) ? EX_OK : EX_DATAERR ;


	if (pip->etfname != NULL)
	    free(pip->etfname) ;


/* return ? */
ret4:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 0)
	    bprintf(efp,"%s: program finishing\n",
	        pip->progname) ;
#endif

	if (f_freeconfigfname && (configfname != NULL))
	    free(configfname) ;

	if (pip->f.log)
	    logfile_close(&pip->lh) ;

ret2:

#ifdef	OPTIONAL
	vecstr_free(&defines) ;

	vecstr_free(&exports) ;
#endif /* OPTIONAL */

retearly:
ret1:
	debugwin_free(&pip->dw) ;

	proginfo_free(pip) ;

	bclose(efp) ;

#ifdef	DMALLOC
	dmalloc_shutdown() ;
#endif

ret0:
	return ex ;


/* USAGE> levosim [-C conf] [-p port] [-V?] */
usage:
	bprintf(efp,
	    "%s: USAGE> %s [conf|-] [-C conf] [-p param] [-j jobname]\n",
	    pip->progname,pip->progname) ;

	bprintf(efp,
	    "%s: \t [-?] [-D[=n]]\n",
	    pip->progname) ;

	goto retearly ;

badargnum:
	bprintf(efp,"%s: not enough arguments specified\n",
	    pip->progname) ;

	goto badarg ;

badargextra:
	bprintf(efp,"%s: no value associated with this option key\n",
	    pip->progname) ;

	goto badarg ;

badargval:
	bprintf(efp,"%s: bad argument value was specified\n",
	    pip->progname) ;

	goto badarg ;

badarg:
	ex = EX_USAGE ;
	goto retearly ;

badlistinit:
	bprintf(efp,"%s: could not initialize list structures (rs %d)\n",
	    pip->progname,rs) ;

	goto ret1 ;

badconfig:
	schedvar_free(&svars) ;

	bprintf(efp,
	    "%s: configfile=%s\n",
	    pip->progname,configfname) ;

	bprintf(efp,
	    "%s: error (rs %d) in configuration file starting at line %d\n",
	    pip->progname,rs,cf.badline) ;

	goto badret ;

badworking:
	schedvar_free(&svars) ;

	bprintf(efp,"%s: could not access the working directory \"%s\"\n",
	    pip->progname,pip->workdir) ;

	goto badret ;

/* error types of returns */
badret:
	ex = EX_DATAERR ;
	goto ret4 ;

}
/* end subroutine (main) */


/* local subroutines */


static int procfile(pip,func,pr,svp,fname,elp)
struct proginfo	*pip ;
int		(*func)(char *,char *,VECSTR *) ;
char		pr[] ;
SCHEDVAR	*svp ;
char		fname[] ;
VECSTR		*elp ;
{
	int	sl ;

	char	tmpfname[MAXPATHLEN + 1] ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main/procfile: 1 fname=%s\n",fname) ;
#endif

	if ((sl = permsched(sched2,svp,fname,R_OK,
	    tmpfname,MAXPATHLEN)) < 0) {

	    if ((sl = permsched(sched3,svp,fname,R_OK,
	        tmpfname,MAXPATHLEN)) > 0)
	        fname = tmpfname ;

	} else if (sl > 0)
	    fname = tmpfname ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("main/procfile: 2 fname=%s\n",fname) ;
#endif

	if (sl >= 0)
	    return (*func)(pr,fname,elp) ;

	return SR_NOEXIST ;
}
/* end subroutine (procfile) */


/* get the date out of the ID string */
static int makedate_get(md,rpp)
const char	md[] ;
char		**rpp ;
{
	int	rs ;

	char	*sp ;
	char	*cp ;


	if (rpp != NULL)
	    *rpp = NULL ;

	if ((cp = strchr(md,CH_RPAREN)) == NULL)
	    return SR_NOENT ;

	while (CHAR_ISWHITE(*cp))
	    cp += 1 ;

	if (! isdigit(*cp)) {

	    while (*cp && (! CHAR_ISWHITE(*cp)))
	        cp += 1 ;

	    while (CHAR_ISWHITE(*cp))
	        cp += 1 ;

	} /* end if (skip over the name) */

	sp = cp ;
	if (rpp != NULL)
	    *rpp = cp ;

	while (*cp && (! CHAR_ISWHITE(*cp)))
	    cp += 1 ;

	return (cp - sp) ;
}
/* end subroutine (makedate_get) */


static void int_inter(sn,sip,vp)
int		sn ;
siginfo_t	*sip ;
void		*vp ;
{
	ucontext_t	*ucp = vp ;

	time_t	daytime = time(NULL) ;

	int	oen = errno ;

	char	timebuf[TIMEBUFLEN + 1] ;


	f_pipe = TRUE ;

	nprintf("here.deb","%s SIGINT\n",
	    timestr_log(daytime,timebuf)) ;

	errno = oen ;
	return ;
}
/* end subroutine (int_inter) */


static void int_hanger(sn,sip,vp)
int		sn ;
siginfo_t	*sip ;
void		*vp ;
{
	ucontext_t	*ucp = vp ;

	time_t	daytime = time(NULL) ;

	int	oen = errno ;

	char	timebuf[TIMEBUFLEN + 1] ;


	f_pipe = TRUE ;

	nprintf("here.deb","%s SIGHUP\n",
	    timestr_log(daytime,timebuf)) ;

	errno = oen ;
	return ;
}
/* end subroutine (int_hanger) */


static void int_piper(sn,sip,vp)
int		sn ;
siginfo_t	*sip ;
void		*vp ;
{
	ucontext_t	*ucp = vp ;

	time_t	daytime = time(NULL) ;

	int	oen = errno ;

	char	timebuf[TIMEBUFLEN + 1] ;


	f_pipe = TRUE ;

	nprintf("here.deb","%s SIGPIPE\n",
	    timestr_log(daytime,timebuf)) ;

	errno = oen ;
	return ;
}
/* end subroutine (int_piper) */


