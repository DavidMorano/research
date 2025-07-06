/* main */

/* compare execution traces */
/* last modified %G% version %I% */


#define	CF_DEBUGS	0		/* debug print-outs (non-switchable) */
#define	CF_DEBUG	0		/* debug print-outs switchable */
#define	CF_MIPSDIS	1		/* enable disassembly */


/* revision history:

	= 2000-11-01, David Morano

	This subroutine was originally written.


	= 2001-08-01, David Morano

	I don't know where this subroutine originally came from but
	it's been a long time since 1990 !  I forget where I grabbed
	this from but it is part of the TRACECMP program for Levo
	research now!


*/

/* Copyright © 1998 David Morano.  All rights reserved. */

/*******************************************************************************

	This is the front-end for some of the trace processing suite
	of tools.

	Synopsis:

	$ tracecmp trace1 trace2 


*******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/utsname.h>
#include	<sys/resource.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<cstdlib>
#include	<cstring>
#include	<ctype.h>

#include	<vsystem.h>
#include	<vecstr.h>
#include	<bfile.h>
#include	<keyopt.h>
#include	<paramopt.h>
#include	<ascii.h>
#include	<char.h>
#include	<userinfo.h>
#include	<logfile.h>
#include	<mallocstuff.h>
#include	<exitcodes.h>
#include	<localmisc.h>

#include	"getfname.h"
#include	"exectrace.h"
#include	"config.h"
#include	"defs.h"
#include	"lmapprog.h"
#include	"mipsdis.h"
#include	"filtercalls.h"


/* local defines */


/* external subroutines */

extern int	sfbasename(const char *,int,const char **) ;
extern int	sfdirname(const char *,int,const char **) ;
extern int	matstr(const char **,const char *,int) ;
extern int	matostr(const char **,int,const char *,int) ;
extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecull(const char *,int,ULONG *) ;
extern int	cfnumull(const char *,int,ULONG *) ;

extern int	analyze(struct proginfo *,LMAPPROG *,MIPSDIS *,char *,char *) ;
extern int	dump(struct proginfo *,LMAPPROG *,MIPSDIS *,char *) ;
extern int	icounter(struct proginfo *,LMAPPROG *,MIPSDIS *,char *) ;
extern int	fcounter(struct proginfo *,LMAPPROG *,MIPSDIS *,char *) ;
extern int	tcopy(struct proginfo *,LMAPPROG *,MIPSDIS *,char *,char *) ;
extern int	stats(struct proginfo *,LMAPPROG *,MIPSDIS *,
			KEYOPT *,ULONG,char *) ;

extern char	*strwcpy(char *,const char *,int) ;


/* external variables */

extern const char	makedate[] ;


/* local structures */


/* forward references */

static int	usage(struct proginfo *) ;
static int	havefname(struct proginfo* ,const char *,const char *,char *) ;
static int	makedate_get(const char *,char **) ;

#if	CF_DEBUG || CF_DEBUGS
static int	printkeyops(struct proginfo *,KEYOPT *) ;
#endif


/* local variables */

static const char	*argopts[] = {
	"VERSION",
	"DEBUG",
	"icount",
	"count",
	"jn",
	"jobname",
	"ko",
	NULL
} ;

enum argopts {
	argopt_version,
	argopt_debug,
	argopt_icount,
	argopt_count,
	argopt_jn,
	argopt_jobname,
	argopt_ko,
	argopt_overlast
} ;

static const char	*progmodes[] = {
	"tracecmp",
	"tracedump",
	"analyze",
	"dump",
	"icount",
	"fcount",
	"copy",
	"stats",
	NULL
} ;

enum progmodes {
	progmode_tracecmp,
	progmode_tracedump,
	progmode_analyze,
	progmode_dump,
	progmode_icount,
	progmode_fcount,
	progmode_copy,
	progmode_stats,
	progmode_overlast
} ;

static const char	*dumpopts[] = {
	"in",
	"inum",
	"regs",
	"regvals",
	"mems",
	"memvals",
	"instr",
	"instrdis",
	"dis",
	"eregs",
	"emems",
	"pixie",
	"rsa",
	"rsv",
	"msa",
	"msv",
	"mpv",
	"full",
	"fulldump",
	"noerrno",
	"onemem",
	"syscalls",
	"plusinstr",
	"fcount",
	"ssh",
	"vpred",
	"yags",
	"tourna",
	"gspag",
	"eveight",
	"gskew",
	"regstats",
	"memstats",
	"badias",
	NULL
} ;

enum dumpopts {
	dumpopt_in,
	dumpopt_inum,
	dumpopt_regs,
	dumpopt_regvals,
	dumpopt_mems,
	dumpopt_memvals,
	dumpopt_instr,
	dumpopt_instrdis,
	dumpopt_dis,
	dumpopt_eregs,
	dumpopt_emems,
	dumpopt_pixie,
	dumpopt_rsa,
	dumpopt_rsv,
	dumpopt_msa,
	dumpopt_msv,
	dumpopt_mpv,
	dumpopt_full,
	dumpopt_fulldump,
	dumpopt_noerrno,
	dumpopt_onemem,
	dumpopt_syscalls,
	dumpopt_plusinstr,
	dumpopt_fcount,
	dumpopt_ssh,
	dumpopt_vpred,
	dumpopt_yags,
	dumpopt_tourna,
	dumpopt_gspag,
	dumpopt_eveight,
	dumpopt_gskew,
	dumpopt_regstats,
	dumpopt_memstats,
	dumpopt_badias,
	dumpopt_overlast
} ;


/* exported subroutines */


int main(argc,argv,envv)
int		argc ;
const char	*argv[] ;
const char	*envv[] ;
{
	struct proginfo	pi, *pip = &pi ;
	ustat	sb ;

	USERINFO	u ;

	PARAMOPT	aparams ;

	LMAPPROG	pm ;

	MIPSDIS		dis ;

	KEYOPT		kopts ;

	FILTERCALLS	callfilter ;

	vecstr		args, exports ;

	bfile		errfile, *efp = &errfile ;
	bfile		outfile ;

	time_t	daytime ;

	int	argr, argl, aol, akl, avl ;
	int	kwi, npa, i ;
	int	ex = EX_INFO ;
	int	unknown ;
	int	rs, sl, cl ;
	int	logfile_type = GETFNAME_TYPEUNKNOWN ;
	int	loglen = -1 ;
	int	fd_debug = -1 ;
	int	f_optplus, f_optminus, f_optequal ;
	int	f_exitargs = FALSE ;
	int	f_programroot = FALSE ;
	int	f_version = FALSE ;
	int	f_makedate = FALSE ;
	int	f_usage = FALSE ;
	int	f ;

	const char	*argp, *aop, *akp, *avp ;
	const char	*argval = NULL ;
	char	buf[BUFLEN + 1] ;
	char	userinfobuf[USERINFO_LEN + 1] ;
	char	tmpfname[MAXPATHLEN + 1] ;
	char	logfname[MAXPATHLEN + 1] ;
	char	progbnamebuf[MAXPATHLEN + 1] ;
	char	progfnamebuf[MAXPATHLEN + 1] ;
	char	progargsbuf[MAXPATHLEN + 1] ;
	char	progenvbuf[MAXPATHLEN + 1] ;
	char	timebuf[TIMEBUFLEN + 1] ;
	const char	*sn = NULL ;
	const char	*programroot = NULL ;
	const char	*progbname = NULL ;	/* base name */
	const char	*progfname = NULL ;
	const char	*argsfname = NULL ;
	const char	*envfname = NULL ;
	const char	*filterfname = NULL ;
	const char	*t1fname = NULL ;
	const char	*t2fname = NULL ;
	const char	*ofname = NULL ;
	const char	*progmodestr = NULL ;
	const char	*instrspec = NULL ;
	const char	*sp, *cp ;


	vecstr_start(&args,10,0) ;

	vecstr_start(&exports,10,0) ;

	if ((cp = getenv(VARDEBUGFD1)) == NULL)
	    cp = getenv(VARDEBUGFD2) ;

	if (cp == NULL)
	    cp = getenv(VARDEBUGFD3) ;

	if ((cp != NULL) &&
	    (cfdeci(cp,-1,&fd_debug) >= 0))
	    debugsetfd(fd_debug) ;


#if	CF_DEBUGS
	debugprintf("main: starting\n") ;
#endif

	(void) memset(pip,0,sizeof(struct proginfo)) ;

	sfbasename(argv[0],-1,&cp) ;
	pip->progname = cp ;

	if ((rs = bopen(efp,BFILE_STDERR,"dwca",0666)) >= 0) {
	    pip->efp = efp ;
	    bcontrol(efp,BC_LINEBUF,0) ;
	}

#if	CF_DEBUGS
	debugprintf("main: STDERR bopen() rs=%d\n",rs) ;
#endif

/* early things to initialize */

	pip->version = VERSION ;
	pip->pr = NULL ;

	pip->ofp = NULL ;

	pip->ninstr = 0 ;

	pip->debuglevel = 0 ;
	pip->verboselevel = 0 ;

	pip->callfilter = &callfilter ;

	daytime = time(NULL) ;

/* this initializes starting time and other things */

	rs = proginfo_start(pip,envv,argv[0],VERSION) ;
	if (rs < 0) {
 	    ex = EX_OSERR ;
	    goto badprogstart ;
	}

	if ((cp = getourenv(envv,VARBANNER)) == NULL) cp = BANNER ;
	proginfo_setbanner(pip,cp) ;

/* program mode */

#ifdef	COMMENT
	pip->progmode = progmode_analyze ;
	sl = sfbasename(argv[0],-1,&sp) ;

	for (i = 0 ; progmodes[i] != NULL ; i += 1) {

	    if (strncmp(sp,progmodes[i],sl) == 0)
	        break ;

	} /* end for */

	if (progmodes[i] != NULL)
	    pip->progmode = i ;
#endif /* COMMENT */


/* initialize other options */

	rs = keyopt_start(&kopts) ;
	pip->open.kopts = (rs >= 0) ;

	if (rs >= 0) {
	    rs = paramopt_start(&aparams) ;
	    pip->open.aparams = (rs >= 0) ;
	}

/* gather the arguments */

	npa = 0 ;			/* number of positional so far */
	i = 0 ;
	argr = argc - 1 ;
	while ((! f_exitargs) && (argr > 0)) {

	    argp = argv[++i] ;
	    argr -= 1 ;
	    argl = strlen(argp) ;

	    f_optminus = (*argp == '-') ;
	    f_optplus = (*argp == '+') ;
	    if ((argl > 0) && (f_optplus || f_optminus)) {

	        if (argl > 1) {

	            if (isdigit(argp[1])) {

	                if (cfdeci(argp + 1,argl - 1,&unknown) < 0)
	                    goto badargval ;

	            } else {

	                aop = argp + 1 ;
	                aol = argl - 1 ;
	                akp = aop ;
	                f_optequal = FALSE ;
	                if ((avp = strchr(aop,'=')) != NULL) {

#if	CF_DEBUGS
	                    debugprintf("main: got option key w/ a value\n") ;
#endif

	                    akl = avp - aop ;
	                    avp += 1 ;
	                    avl = aop + aol - avp ;
	                    f_optequal = TRUE ;
	                } else {
	                    akl = aol ;
	                    avl = 0 ;
	                }

/* do we have a keyword or only key letters? */

	                if ((kwi = matostr(argopts,2,akp,akl)) >= 0) {

	                    switch (kwi) {

/* version */
	                    case argopt_version:
				f_makedate = f_version ;
	                        f_version = TRUE ;
	                        if (f_optequal)
	                            goto badargextra ;

	                        break ;

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

	                    case argopt_icount:
	                    case argopt_count:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)  {

	                            rs = cfnumull(argp,argl, 
	                                &pip->ninstr) ;

	                            if (rs < 0)
	                                goto badargval ;

	                        }

	                        break ;

	                    case argopt_jn:
	                    case argopt_jobname:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)  {

	                            strwcpy(pip->jobname,argp,
	                                MIN(argl,JOBNAMELEN)) ;

	                        }

	                        break ;

	                    case argopt_ko:
	                        if (argr <= 0)
	                            goto badargnum ;

	                        argp = argv[++i] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)  {

	                            rs = keyopt_loads(&kopts,argp,argl) ;
 
	                            if (rs < 0)
					goto badargval ;

	                        }

	                        break ;

/* handle all keyword defaults */
	                    default:
	                        ex = EX_USAGE ;
	                        if (efp != NULL)
	                            bprintf(efp,
	                                "%s: unknown argument keyword \"%s\"\n",
	                                pip->progname,akp) ;

	                        f_exitargs = TRUE ;
	                        f_usage = TRUE ;

	                    } /* end switch */

	                } else {

#if	CF_DEBUGS
	                    debugprintf("main: got an option key letter\n") ;
#endif

	                    while (akl--) {
				int	kc = (*akp & 0xff) ;

	                        switch (kc) {

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

	                        case 'V':
				    f_makedate = f_version ;
	                            f_version = TRUE ;
	                            break ;

/* argument file */
	                        case 'a':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                argsfname = argp ;

	                            break ;

/* environment file */
	                        case 'e':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                envfname = argp ;

	                            break ;

/* filter file */
	                        case 'f':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                filterfname = argp ;

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

/* target program base name */
	                        case 'n':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                progbname = argp ;

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

/* options */
	                        case 'o':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                rs = paramopt_loads(&aparams,DUMPOPTS,
	                                    argp,argl) ;

	                            }

	                            break ;

/* target program file */
	                        case 'p':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                progfname = argp ;

	                            break ;

/* instructions to dump */
	                        case 's':
	                            if (argr <= 0)
	                                goto badargnum ;

	                            argp = argv[++i] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                instrspec = argp ;

	                            break ;

/* verbosity level */
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
	                            ex = EX_USAGE ;
	                            f_usage = TRUE ;
	                            if (efp != NULL)
	                                bprintf(efp,
	                                    "%s: unknown option - %c\n",
	                                    pip->progname,*akp) ;

	                            f_exitargs = TRUE ;
	                            break ;

	                        } /* end switch */

#if	CF_DEBUGS
	                        debugprintf("main: beyond key letters switch\n") ;
#endif

	                        akp += 1 ;

	                    } /* end while */

	                } /* end if (individual option key letters) */

	            } /* end if (digits as argument or not) */

	        } else {

/* we have a plus or minus sign character alone on the command line */

	            npa += 1 ;

	        } /* end if */

	    } else {

	        switch (npa) {

	        case 0:
	            if (argl > 0)
	                t1fname = argp ;

	            break ;

	        case 1:
	            if (argl > 0)
	                t2fname = argp ;

	            break ;

	        default:
	            ex = EX_USAGE ;
	            f_usage = TRUE ;
	            if (efp != NULL)
	                bprintf(efp,
	                    "%s: extra positional arguments specified\n",
	                    pip->progname) ;

	            f_exitargs = TRUE ;

	        } /* end switch */

	        npa += 1 ;

	    } /* end if (key letter/word or positional) */

	} /* end while (all command line argument processing) */

#if	CF_DEBUGS
	debugprintf("main: done looping on arguments\n") ;
#endif


#if	CF_MASTERDEBUG && CF_DEBUG
	debugprintf("main: debuglevel=%u\n",pip->debuglevel) ;
#endif


	if (f_version && (efp != NULL)) {

	    bprintf(efp,"%s: version %s\n",
	        pip->progname,VERSION) ;

		if (f_makedate) {

		cl = makedate_get(makedate,&cp) ;

	    bprintf(efp,"%s: makedate %t\n",
	        pip->progname,cp,cl) ;

		}

	} /* end if (version) */

	if (f_usage) {

	    rs = usage(pip) ;

	    goto retearly ;
	}

	if (f_version)
	    goto retearly ;


	if (pip->debuglevel > 0)
	    bprintf(pip->efp,"%s: debuglevel=%u verboselevel=%d\n",
	        pip->progname,
	        pip->debuglevel,
	        pip->verboselevel) ;


/* what mode are we executing in */

	if (progmodestr == NULL) {

	    sl = sfbasename(pip->progname,-1,&cp) ;

	     if (sl > 0) {

		strwcpy(buf,cp,MIN(BUFLEN,MAXPATHLEN)) ;

		if ((cp = strrchr(buf,'.')) != NULL)
			*cp = '\0' ;

		if ((cp = strchr(buf,'-')) != NULL) {

		    strwcpy(tmpfname,buf,-1) ;

		    cp = strchr(tmpfname,'-') ;

		    strwcpy(buf,(cp + 1),-1) ;

		    progmodestr = buf ;

		} else {

		    progmodestr = buf ;
		    if ((cp = getenv(PROGMODEVAR)) != NULL)
			progmodestr = cp ;

		}

	    } /* end if */

	} /* end if (program mode was specified) */

	if (progmodestr == NULL)
		progmodestr = getenv(PROGMODEVAR) ;

	if ((progmodestr == NULL) || (progmodestr[0] == '\0'))
		progmodestr = "dump" ;

	    for (i = 0 ; progmodes[i] != NULL ; i += 1) {

	        if (strcmp(progmodestr,progmodes[i]) == 0)
	            break ;

	    } /* end for */

	    if (progmodes[i] == NULL)
	        goto badprogmode ;

	    pip->progmode = i ;

/* other */

	if (pip->progmode == progmode_tracecmp)
	    pip->progmode = progmode_analyze ;

	if (pip->progmode == progmode_tracedump)
	    pip->progmode = progmode_dump ;

	if (pip->debuglevel > 0)
	    bprintf(pip->efp,"%s: mode=%s\n",
	        pip->progname,progmodes[pip->progmode]) ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: program mode=%s (%d)\n",
	        progmodes[pip->progmode],pip->progmode) ;
#endif


/* check arguments */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: checking arguments\n") ;
#endif

	if ((pip->progmode == progmode_analyze) && (npa < 2))
	    goto badargnum ;

	if ((pip->progmode == progmode_dump) && (npa < 1))
	    goto badargnum ;


/* let's move on */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: programroot=%s\n",pip->pr) ;
#endif

	if (pip->pr == NULL) {

	    programroot = getenv(VARPROGRAMROOT1) ;

	    if (programroot == NULL)
	        programroot = getenv(VARPROGRAMROOT2) ;

	    if (programroot == NULL)
	        programroot = getenv(VARPROGRAMROOT3) ;

	    if (programroot == NULL)
	        programroot = PROGRAMROOT ;

	    pip->pr = programroot ;

	} else {

	    f_programroot = TRUE ;
	    programroot = pip->pr ;

	}

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: programroot=%s\n",pip->pr) ;
#endif


/* dumb stuff */

	pip->pid = getpid() ;

	userinfo(&u,userinfobuf,USERINFO_LEN,NULL) ;

	pip->nodename = u.nodename ;
	pip->domainname = u.domainname ;


	bufprintf(buf,BUFLEN,"%s%d",pip->nodename,(int) pip->pid) ;

	pip->logid = mallocstrw(buf,LOGIDLEN) ;


	rs = SR_BAD ;
	if (logfname[0] == '\0') {

	    logfile_type = GETFNAME_TYPEROOT ;
	    strcpy(logfname,LOGFNAME) ;

	}

	sl = getfname(pip->pr,logfname,logfile_type,tmpfname) ;

	if (sl > 0)
	    strwcpy(logfname,tmpfname,sl) ;

	rs = logfile_open(&pip->lh,logfname,0,0666,pip->logid) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: 1 logfname=%s rs=%d\n",logfname,rs) ;
#endif

	if (rs >= 0) {

	    struct utsname	un ;


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: we opened a logfile\n") ;
#endif

	    pip->f.log = TRUE ;
	    if (pip->debuglevel > 0)
	        bprintf(efp,"%s: logfile=%s\n",pip->progname,logfname) ;

/* we opened it, maintenance this log file if we have to */

	    if (loglen < 0)
	        loglen = LOGSIZE ;

	    logfile_checksize(&pip->lh,loglen) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: we checked its length\n") ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: making log entry\n") ;
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

	    buf[0] = '\0' ;
	    if ((u.name != NULL) && (u.name[0] != '\0'))
	        sprintf(buf,"(%s)",u.name) ;

	    else if ((u.gecosname != NULL) && (u.gecosname[0] != '\0'))
	        sprintf(buf,"(%s)",u.gecosname) ;

	    else if ((u.fullname != NULL) && (u.fullname[0] != '\0'))
	        sprintf(buf,"(%s)",u.fullname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: LF name=%s\n",buf) ;
#endif

	    logfile_printf(&pip->lh,"%s!%s %s\n",
	        u.nodename,u.username,buf) ;

	    if (pip->progmode > 0)
	        logfile_printf(&pip->lh,"program mode=%s\n",
	            progmodes[pip->progmode]) ;

	    logfile_printf(&pip->lh,"pid=%d\n",
	        (int) pip->pid) ;

	} /* end if (we have a log file or not) */


/* some more initialization */

	if ((cp = getenv(VAROPTS)) != NULL) {

		rs = keyopt_loads(&kopts,cp,-1) ;

		if (rs < 0)
			goto badargval ;

	} /* end if (environment options) */


#if	CF_DEBUG
	if (pip->debuglevel >= 5)
		printkeyops(pip,&kopts) ;
#endif


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: ninstr=%lld\n",pip->ninstr) ;
#endif


/* instructions to dump */

	(void) memset(&pip->in,0,sizeof(struct proginfo_select)) ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: instrspec=%s\n",instrspec) ;
#endif

	if (instrspec != NULL) {

	    sl = -1 ;
	    if ((cp = strchr(instrspec,':')) != NULL) {

	        if (cfnumull((cp + 1),-1,&pip->in.n) < 0)
	            goto badsnum ;

	        sl = cp - instrspec ;

	    } /* end if */

		pip->in.start = 0 ;
	    if (sl > 0) {

	    if (cfnumull(instrspec,sl,&pip->in.start) < 0)
	        goto badsnum ;

	    }

	    if (pip->in.n > 0)
	        pip->in.end = pip->in.start + pip->in.n ;

	} /* end if (instructions to dump) */

#if	CF_DEBUG
	if (pip->debuglevel > 1) {
	    debugprintf("main: start=%lld num=%lld end=%lld\n",
	        pip->in.start,pip->in.n,pip->in.end) ;
	    debugprintf("main: progbname=%s\n",progbname) ;
	}
#endif /* CF_DEBUG */

	if (pip->debuglevel > 0) {

	    bprintf(pip->efp,"%s: ninstr=%lld\n",
		pip->progname,
	        pip->ninstr) ;

	    bprintf(pip->efp,"%s: start=%lld num=%lld end=%lld\n",
		pip->progname,
	        pip->in.start,pip->in.n,pip->in.end) ;

	} /* end if (debugging information) */


/* option parsing */

	rs = SR_OK ;

	{
	    PARAMOPT_CUR	c ;

	    int	spc = 0 ;


	    paramopt_curbegin(&aparams,&c) ;

	    while (paramopt_curenumval(&aparams,DUMPOPTS,&c,&cp) >= 0) {

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("main: option=%s\n",cp) ;
#endif

	        rs = matostr(dumpopts,2,cp,-1) ;

	        if (rs < 0)
	            break ;

		i = rs ;

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("main: oi=%d\n",i) ;
#endif

	        switch (i) {

	        case dumpopt_in:
	        case dumpopt_inum:

#if	CF_DEBUG
	            if (pip->debuglevel > 1)
	                debugprintf("main: opt_in !\n") ;
#endif

	            pip->f.opt_in = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_instr:
	            pip->f.opt_instr = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_instrdis:
	        case dumpopt_dis:
	            pip->f.opt_instrdis = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_regs:
	            pip->f.opt_regs = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_regvals:
	            pip->f.opt_regs = TRUE ;
	            pip->f.opt_regvals = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_rsa:
	            pip->f.opt_rsa = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_rsv:
	            pip->f.opt_rsa = TRUE ;
	            pip->f.opt_rsv = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_eregs:
	            pip->f.opt_regs = TRUE ;
	            pip->f.opt_eregs = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_mems:
	            pip->f.opt_mems = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_memvals:
	            pip->f.opt_mems = TRUE ;
	            pip->f.opt_memvals = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_msa:
	            pip->f.opt_msa = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_msv:
	            pip->f.opt_msa = TRUE ;
	            pip->f.opt_msv = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_mpv:
	            pip->f.opt_mpv = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_emems:
	            pip->f.opt_mems = TRUE ;
	            pip->f.opt_emems = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_pixie:
	            pip->f.opt_pixie = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_full:
	        case dumpopt_fulldump:
	            pip->f.opt_in = TRUE ;
	            pip->f.opt_instr = TRUE ;
	            pip->f.opt_instrdis = TRUE ;
	            pip->f.opt_regs = TRUE ;
	            pip->f.opt_eregs = TRUE ;
	            pip->f.opt_regvals = TRUE ;
	            pip->f.opt_mems = TRUE ;
	            pip->f.opt_emems = TRUE ;
	            pip->f.opt_memvals = TRUE ;
	            pip->f.opt_rsa = TRUE ;
	            pip->f.opt_rsv = TRUE ;
	            pip->f.opt_msa = TRUE ;
	            pip->f.opt_msv = TRUE ;
	            break ;

	        case dumpopt_noerrno:
	            pip->f.opt_noerrno = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_onemem:
	            pip->f.opt_onemem = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_syscalls:
	            pip->f.opt_syscalls = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_plusinstr:
	            pip->f.opt_plusinstr = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_fcount:
	            pip->f.opt_fcount = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_ssh:
	            pip->f.opt_ssh = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_regstats:
	            pip->f.opt_regstats = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_memstats:
	            pip->f.opt_memstats = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_vpred:
	            pip->f.opt_vpred = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_yags:
	            pip->f.opt_yags = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_tourna:
	            pip->f.opt_tourna = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_gspag:
	            pip->f.opt_gspag = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_eveight:
	            pip->f.opt_eveight = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_gskew:
	            pip->f.opt_gskew = TRUE ;
	            spc += 1 ;
	            break ;

	        case dumpopt_badias:
	            pip->f.opt_badias = TRUE ;
	            spc += 1 ;
	            break ;

	        } /* end switch */

	    } /* end while (looping through options) */

	    paramopt_curend(&aparams,&c) ;

	} /* end block (options processing) */

	if (rs < 0)
	    goto badeopt ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: done w/ dump options\n") ;
#endif



	ex = EX_DATAERR ;


/* do we have a base name ? */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: progbname=%s\n",progbname) ;
#endif

	if ((progbname == NULL) || (progbname[0] == '\0')) {

	    if ((cp = getenv(NAMEVAR1)) == NULL)
	        cp = getenv(NAMEVAR2) ;

	    if (cp != NULL)
	        progbname = cp ;

	} /* end if (program basename) */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: progbname=%s\n",progbname) ;
#endif


/* do we have a program file ? */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: progfname=%s\n",progfname) ;
#endif

	if ((progfname == NULL) || (progfname[0] == '\0')) {

	    rs = SR_NOTFOUND ;
	    if ((progbname != NULL) && (progbname[0] != '\0'))
	        rs = havefname(pip,"mips",progbname,progfnamebuf) ;

	    if (rs < 0) {

	        rs = havefname(pip,"mips",t2fname,progfnamebuf) ;

	        if (rs < 0)
	            rs = havefname(pip,"mips",t1fname,progfnamebuf) ;

	    }

	    if (rs >= 0)
	        progfname = progfnamebuf ;

	} /* end if (finding a program file) */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: proggname=%s\n",progfname) ;
#endif


/* program base name */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: progbname=%s\n",progbname) ;
#endif

	if ((progbname == NULL) || (progbname[0] == '\0')) {

	    if ((progfname != NULL) && (progfname[0] != '\0')) {

	        rs = havefname(pip,"",progfname,progbnamebuf) ;

	        if (rs >= 0)
	            progbname = progbnamebuf ;

	    }
	}


/* store away the base name if we have one */

	pip->basename[0] = '\0' ;
	if (progbname != NULL)
		strwcpy(pip->basename,progbname,MAXNAMELEN) ;


/* jobname ? */

	if ((pip->jobname[0] == '\0') &&
	    (progbname != NULL) && (progbname[0] != '\0')) {

	    strwcpy(pip->jobname,progbname, JOBNAMELEN) ;

	}

	    logfile_printf(&pip->lh,"jobname=%s\n",
	        pip->jobname) ;


/* read any target program arguments */

	{
	    int	f_argsfile = TRUE ;


#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: argsfname=%s\n",argsfname) ;
#endif

	    if (argsfname == NULL) {

	        f_argsfile = FALSE ;
	        rs = SR_NOTFOUND ;
	        if ((progbname != NULL) && (progbname[0] != '\0'))
	            rs = havefname(pip,"args",progbname,progargsbuf) ;

	        if (rs >= 0)
	            argsfname = progargsbuf ;

	    } /* end if (target program arguments) */

	    if (argsfname != NULL) {

	        rs = procargs(pip->pr,argsfname,&args) ;

	        if ((rs < 0) && f_argsfile)
	            goto badargsfile ;

	    }

	    if ((argsfname == NULL) && (progbname != NULL))
	        vecstr_add(&args,progbname,-1) ;

	} /* end block (target program arguments) */


/* read in any environment that was specified */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: user specified environment, envfname=%s\n",
	        envfname) ;
#endif

	{
	    int	f_envfile = TRUE ;


	    if (envfname == NULL) {

	        f_envfile = FALSE ;
	        rs = SR_NOTFOUND ;
	        if ((progbname != NULL) && (progbname[0] != '\0')) {

	            rs = havefname(pip,"env",progbname,progenvbuf) ;

	            if (rs >= 0)
	                envfname = progenvbuf ;

	        }

	    } /* end if (target arguments) */

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: envfname=%s\n",
	            envfname) ;
#endif

	    if (envfname != NULL) {

	        rs = procenv(pip->pr,envfname,&exports) ;

	        if ((rs < 0) && f_envfile)
	            goto badenvfile ;

#if	CF_DEBUG
	        if (pip->debuglevel > 1) {

	            debugprintf("main: procenv() rs=%d\n",rs) ;

	            for (i = 0 ; rs = vecstr_get(&exports,i,&cp) >= 0 ; 
			i += 1) {

	                if (cp == NULL) continue ;

	                debugprintf("main: e> %s\n",cp) ;

	            }
	        }
#endif /* CF_DEBUG */

	    } /* end if (user specified environment) */

	} /* end block (target program environment) */


/* what about filtering some calls out of the trace ? */

	if (filterfname == NULL)
	    filterfname = FILTERFNAME ;

	if (u_access(filterfname,R_OK) >= 0)
	    pip->f.filter = TRUE ;


/* map the program file if we have one */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: progfname=%s\n",progfname) ;
#endif

	f = FALSE ;
	f = f || pip->f.opt_instr || pip->f.opt_instrdis | pip->f.opt_msv ;
	f = f || (pip->f.filter && (pip->progmode == progmode_copy)) ;
	f = f || (pip->progmode == progmode_stats) ;
	f = f || (pip->progmode == progmode_fcount) ;
	if ((progfname != NULL) && f) {

#if	CF_DEBUG
	    if (pip->debuglevel > 1) {
	        debugprintf("main: lmapprog_init() progfname=%s\n",
	            progfname) ;
	        debugprintf("main: lmapprog_init() arg[0]=>%s< env[0]=>%s<\n",
	            args.va[0],exports.va[0]) ;
	    }
#endif

	    rs = lmapprog_init(&pm,pip,progfname,args.va,exports.va) ;

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: lmapprog_init() rs=%d\n",rs) ;
#endif

	    if (rs >= 0)
	        pip->f.progmap = TRUE ;

		if ((rs < 0) && (! pip->f.quiet)) {

	        bprintf(pip->efp,
	            "%s: could not load the program executable (%d)\n",
	            pip->progname,rs) ;

	        bprintf(pip->efp,
	            "%s: program executable=%s\n",
	            pip->progname,progfname) ;

		} /* end if (couldn't initialization program) */

		if (rs < 0) {

			int	f_bad = FALSE ;


		switch (pip->progmode) {

		case progmode_copy:
		case progmode_stats:
		case progmode_fcount:
			f_bad = TRUE ;
			break ;

		} /* end switch */

			if (f_bad)
				goto badprogmap ;

		} /* end if (couldn't map program) */

#if	CF_DEBUG
	    if (pip->debuglevel > 2) {
	        uint	ma, mv ;

	        lmapprog_getstack(&pm,&ma) ;


	        ma = ma - 24 ;
	        for (i = 0 ; i < 40 ; i += 1) {

	            lmapprog_readint(&pm,ma,&mv) ;
	            debugprintf("main: ma=%08x mv=%08x\n",ma,mv) ;

	            ma += 4 ;
	        }
	    }
#endif /* CF_DEBUG */

	} /* end if (trying to map the target executable program) */


/* should we initialize for disassembly ? */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: disassemble ?\n") ;
#endif

	f = FALSE ;
	f = f || (pip->f.progmap && pip->f.opt_instrdis) ;
	f = f || (pip->progmode == progmode_stats) ;
	if ((progfname != NULL) && f) {

#if	CF_MIPSDIS
	    rs = mipsdis_init(&dis,NULL,progfname) ;
#else
		rs = SR_NOTSUP ;
#endif

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: mipsdis_init() rs=%d\n",rs) ;
#endif

	    if (rs >= 0)
	        pip->f.instrdis = TRUE ;

	    else
	        bprintf(pip->efp,
	            "%s: could not initialize for disassembly (%d)\n",
	            pip->progname,rs) ;

	} /* end if (trying to initialize for disassembly) */


/* should we initialize further for call filtering ? */

	if (! pip->f.progmap)
	    pip->f.filter = FALSE ;

	if ((filterfname != NULL) && (filterfname[0] != '\0') &&
	    (pip->progmode == progmode_copy) &&
	    pip->f.filter && 
	    pip->f.progmap) {

	    rs = filtercalls_init(pip->callfilter,&pm,filterfname) ;

	    pip->f.filter = (rs >= 0) ;

	} /* end if (further initialization for call filtering) */


/* open output */

#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("main: output file=%s\n",ofname) ;
#endif

	if ((ofname == NULL) || (ofname[0] == '\0'))
	    rs = bopen(&outfile,BFILE_STDOUT,"dwct",0666) ;

	else
	    rs = bopen(&outfile,ofname,"wct",0666) ;

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("main: bopen() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto badoutopen ;

	pip->ofp = &outfile ;


#if	CF_DEBUG
	if (pip->debuglevel >= 3)
	    debugprintf("main: mode=%s\n",
	        progmodes[pip->progmode]) ;
#endif

	switch (pip->progmode) {

	case progmode_analyze:
	    {

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("main: analyzing t1=%s t2=%s\n",
	                t1fname,t2fname) ;
#endif

	        rs = analyze(pip,&pm,&dis,t1fname,t2fname) ;

	        if (rs > 1)
	            rs = SR_NOANODE ;

	    } /* end block */

	    break ;

	case progmode_tracedump:
	case progmode_dump:
	    {

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("main: dumping\n") ;
#endif

	        rs = dump(pip,&pm,&dis,t1fname) ;

	    } /* end block */

	    break ;

	case progmode_icount:
	    {

	        rs = icounter(pip,&pm,&dis,t1fname) ;


	    } /* end block */

	    break ;

	case progmode_fcount:
	    {

	        rs = fcounter(pip,&pm,&dis,t1fname) ;


	    } /* end block */

	    break ;

	case progmode_copy:
	    {

	        rs = tcopy(pip,&pm,&dis,t1fname,t2fname) ;


	    } /* end block */

	    break ;

	case progmode_stats:
	    {

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("main: stats\n") ;
#endif

	        rs = stats(pip,&pm,&dis,&kopts,0ULL,t1fname) ;


	    } /* end block */

	    break ;

	default:
	    rs = SR_INVALID ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 2)
	        debugprintf("main: unknown program mode=%s\n",
	            progmodes[pip->progmode]) ;
#endif

	} /* end switch (program modes) */

#if	CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("main: result rs=%d\n",rs) ;
#endif

	if (pip->verboselevel > 0) {

		struct rusage	ut ;

		int	rs1 ;


		rs1 = uc_getrusage(RUSAGE_SELF,&ut) ;

		if (rs1 >= 0) {

			char	timebuf[40] ;

			int	elapsed ;


			daytime = time(NULL) ;

			elapsed = daytime - pip->starttime ;
			bprintf(&outfile,"real   time=%s.%06d\n",
				timestr_elapsed(elapsed,timebuf),
				ut.ru_utime.tv_usec) ;
	
			bprintf(&outfile,"user   time=%s.%06d\n",
				timestr_elapsed(ut.ru_utime.tv_sec,timebuf),
				ut.ru_utime.tv_usec) ;
	
			bprintf(&outfile,"system time=%s.%06d\n",
				timestr_elapsed(ut.ru_stime.tv_sec,timebuf),
				ut.ru_stime.tv_usec) ;

		}

	} /* end if (trailing comments) */

	if (rs < 0) {
	    bprintf(pip->efp,"%s: result rs=%d\n",
	        pip->progname,rs) ;
	}

	bclose(&outfile) ;

	ex = EX_OK ;
	if (rs == SR_NOENT) {
	    ex = EX_NOINPUT ;
	} else if (rs < 0) {
	    ex = EX_DATAERR ;
	}

done:
	if (pip->f.filter) {
	    pip->f.filter = FALSE ;
	    filtercalls_free(pip->callfilter) ;
	}

	if (pip->f.instrdis) {
	    pip->f.intrrdis = FALSE ;
	    mipsdis_free(&dis) ;
	}

	if (pip->f.progmap) {
	    pip->f.progmap = FALSE ;
	    lmapprog_free(&pm) ;
	}

ret3:
	if (pip->f.log) {
	    pip->f.log = FALSE ;
	    logfile_close(&pip->lh) ;
	}

	vecstr_finish(&args) ;

	vecstr_finish(&exports) ;

retearly:
ret2:
	if (pip->open.aparams) {
	    pip->open.aparams = FALSE ;
	    paramopt_finish(&aparams) ;
	}

	if (pip->open.kopts) {
	    pip->open.kopts = FALSE ;
	    keyopt_finish(&kopts) ;
	}

ret1:
	if (pip->efp != NULL) {
	    bclose(pip->efp) ;
	    pip->efp = NULL ;
	}

ret0:
	proginfo_finish(pip) ;

badprogstart:

#ifdef	DMALLOC
	dmalloc_shutdown() ;
#endif

	return ex ;

badargval:
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: bad argument value was specified\n",
	        pip->progname) ;

	goto badarg ;

badargnum:
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: not enough arguments given\n",
	        pip->progname) ;

	goto badarg ;

badargextra:
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: no value associated with this option key\n",
	        pip->progname) ;

	goto badarg ;

badprogmode:
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: unknown program mode specified (%s)\n",
	        pip->progname,progmodestr) ;

	goto badarg ;

badarg:
	ex = EX_USAGE ;

	goto retearly ;

#ifdef	COMMENT

usage:
	ex = EX_USAGE ;
	rs = usage(pip) ;

	goto retearly ;

#endif /* COMMENT */

badsnum:
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: a bad number was specified for selection\n",
	        pip->progname) ;

	goto done ;

badeopt:
	bprintf(efp,"%s: unrecognized option \"%s\"\n",
	    pip->progname,cp) ;

	goto done ;

badargsfile:
	ex = EX_NOINPUT ;
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: could not open the argument file \n",
	        pip->progname) ;

	goto done ;

badenvfile:
	ex = EX_NOINPUT ;
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: could not open the environment file \n",
	        pip->progname) ;

	goto done ;

badoutopen:
	ex = EX_CANTCREAT ;
	goto done ;

badprogmap:
	ex = EX_NOINPUT ;
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: specified program mode requires a program executable\n",
	        pip->progname) ;

	goto ret3 ;

}
/* end subroutine (main) */


/* local subroutines */


/* print out (standard error) some short usage */
static int usage(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;
	int	wlen = 0 ;

	const char	*pn = pip->progname ;
	const char	*fmt ;


	if (pip->efp != NULL) {
	    fmt = "%s: USAGE> %s <trace1> <trace2> [-e <envfile>]\n",
	    rs = bprintf(pip->efp,fmt,pn,pn) ;
	    wlen += rs ;
	    
	    fmt = "%s:  [-a argsfile] [-o <dumpopt(s)>]\n" ;
	    rs = bprintf(pip->efp,fmt,pn) ;
	    wlen += rs ;
	}

	return (rs >= 0) ? wlen : rs ;
}
/* end subroutine (usage) */


static int havefname(pip,ext,tfname,tmpfname)
struct proginfo	*pip ;
const char	ext[] ;
const char	tfname[] ;
char		tmpfname[] ;
{
	int	rs, sl ;

	char	*bnp, *dp ;
	char	*sp ;


	rs = SR_NOENT ;
	if ((tfname == NULL) || (tfname[0] == '\0'))
	    return rs ;

	bnp = strrchr(tfname,'/') ;

	dp = strrchr(tfname,'.') ;

	if ((dp != NULL) && (bnp != NULL) && (dp > bnp))
	    sp = strwcpy(tmpfname,tfname,(dp - tfname)) ;

	else if ((dp != NULL) && (bnp == NULL)) {
	    sp = strwcpy(tmpfname,tfname,(dp - tfname)) ;
	} else
	    sp = strwcpy(tmpfname,tfname,-1) ;

	if (ext[0] != '\0') {

	    sl = strlen(ext) ;

	    if ((sp + sl + 1 - tmpfname) < (MAXPATHLEN - 1)) {

	        if (ext[0] != '.')
	            strcpy(sp++,".") ;

	        sp = strwcpy(sp,ext,sl) ;

	    }

#if	CF_DEBUGS
	    debugprintf("main/havefname: execfile=%s\n",tmpfname) ;
#endif

	} /* end if (have an extension) */

	rs = sp - tmpfname ;
	return rs ;
}
/* end subroutine (havefname) */


#if	CF_DEBUG || CF_DEBUGS

/* print all options */
static int printkeyops(pip,kop)
struct proginfo	*pip ;
KEYOPT		*kop ;
{
	KEYOPT_CUR	kcur ;

	int	rs = SR_OK ;
	int	i, oi, val ;
	int	nlen, klen, vlen ;
	int	n ;

	const char	*kp, *vp ;
	const char	*cp ;


/* process all of the options that we have so far */

	n = 0 ;
	keyopt_curbegin(kop,&kcur) ;

	while ((rs = keyopt_enumkeys(kop,&kcur,&kp)) >= 0) {
	    KEYOPT_CUR	vcur ;
	    int	f_value = FALSE ;

	    klen = rs ;

	    vlen = -1 ;
	    keyopt_curbegin(kop,&vcur) ;

	    while ((rs = keyopt_enumvalues(kop,kp,&vcur,&vp)) >= 0) {

	        f_value = TRUE ;
	        vlen = rs ;

#if	CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("main/printkeyops: key=>%t< f_value=%d "
			"vlen=%d vp=%s\n",
			kp,klen,f_value,vlen,vp) ;
#endif

	    } /* end while */

	    keyopt_curend(kop,&vcur) ;

	} /* end while */

	keyopt_curend(kop,&kcur) ;

#if	CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("main/printkeyopts: n=%d\n",n) ;
#endif

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (printkeyopts) */

#endif /* CF_DEBUG || CF_DEBUGS */


/* get the date out of the ID string */
static int makedate_get(md,rpp)
const char	md[] ;
char		**rpp ;
{
	const char	*sp ;
	const char	*cp ;


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


