/* main */

/* compare execution traces */
/* last modified %G% version %I% */


#define	CF_DEBUGS	0		/* debug print-outs (non-switchable) */
#define	CF_DEBUG	0		/* debug print-outs switchable */


/* revision history:

	= 2000-11-01, David Morano

	This subroutine was originally written.


*/

/* Copyright © 2000 David Morano.  All rights reserved. */

/**************************************************************************

	This is the front-end for some of the trace processing suite
	of tools.

	Execute as :

	$ findhammocks program1 program2


**************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/utsname.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<cstdlib>
#include	<cstring>
#include	<ctype.h>

#include	<vsystem.h>
#include	<vecstr.h>
#include	<bfile.h>
#include	<paramopt.h>
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


/* external subroutines */

extern int	sncpy2(char *,int,const char *,const char *) ;
extern int	mkpath2(char *,const char *,const char *) ;
extern int	sfbasename(const char *,int,const char **) ;
extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecull(const char *,int,ULONG *) ;
extern int	cfnumull(const char *,int,ULONG *) ;
extern int	optmatch(const char **,char *,int) ;

extern char	*strwcpy(char *,const char *,int) ;


/* external variables */

extern int	process(struct proginfo *,LMAPPROG *,MIPSDIS *,char *) ;


/* local structures */


/* forward references */

static int	usage(struct proginfo *) ;
static int	havefname(struct proginfo* ,const char *,const char *,char *) ;


/* local variables */

static const char	*argopts[] = {
	"VERSION",
	"DEBUG",
	"count",
	"jn",
	"jobname",
	NULL
} ;

enum argopts {
	argopt_version,
	argopt_debug,
	argopt_count,
	argopt_jn,
	argopt_jobname,
	argopt_overlast
} ;

static const char	*progmodes[] = {
	"ssh",
	NULL
} ;

enum progmodes {
	progmode_ssh,
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
	NULL
} ;

enum dumpopts {
	procopt_in,
	procopt_inum,
	procopt_regs,
	procopt_regvals,
	procopt_mems,
	procopt_memvals,
	procopt_instr,
	procopt_instrdis,
	procopt_dis,
	procopt_eregs,
	procopt_emems,
	procopt_pixie,
	procopt_rsa,
	procopt_rsv,
	procopt_msa,
	procopt_msv,
	procopt_mpv,
	procopt_full,
	procopt_fulldump,
	procopt_noerrno,
	procopt_onemem,
	procopt_syscalls,
	procopt_plusinstr,
	procopt_overlast
} ;


/* exported subroutines */


int main(argc,argv,envv)
int	argc ;
char	*argv[] ;
char	*envv[] ;
{
	struct ustat	sb ;
	struct proginfo	pi, *pip = &pi ;

	PARAMOPT	aparams ;

	LMAPPROG	pm ;

	MIPSDIS		dis ;

	USERINFO	u ;

	vecstr		args, exports ;

	bfile		errfile, *efp = &errfile ;
	bfile		outfile ;

	time_t	daytime ;

	int	argr, argl, aol, akl, avl ;
	int	kwi, npa, i ;
	int	f_optplus, f_optminus, f_optequal ;
	int	f_exitargs = FALSE ;
	int	f_programroot = FALSE ;
	int	f_version = FALSE ;
	int	f_usage = FALSE ;
	int	ex = EX_INFO ;
	int	argnum ;
	int	rs, sl ;
	int	logfile_type = GETFNAME_TYPEUNKNOWN ;
	int	loglen = -1 ;
	int	fd_debug = -1 ;
	int	f ;

	const char	*argp, *aop, *akp, *avp ;
	char	buf[BUFLEN + 1] ;
	char	userinfobuf[USERINFO_LEN + 1] ;
	char	tmpfname[MAXPATHLEN + 1] ;
	char	logfname[MAXPATHLEN + 1] ;
	char	progbnamebuf[MAXPATHLEN + 1] ;
	char	progfnamebuf[MAXPATHLEN + 1] ;
	char	progargsbuf[MAXPATHLEN + 1] ;
	char	progenvbuf[MAXPATHLEN + 1] ;
	char	sshfnamebuf[MAXPATHLEN + 1] ;
	char	timebuf[TIMEBUFLEN + 1] ;
	const char	*programroot = NULL ;
	const char	*progbname = NULL ;	/* base name */
	const char	*progfname = NULL ;
	const char	*argsfname = NULL ;
	const char	*envfname = NULL ;
	const char	*sshfname = NULL ;
	const char	*ofname = NULL ;
	const char	*progmodestr = NULL ;
	const char	*instrspec = NULL ;
	const char	*sp, *cp ;


	if ((cp = getenv(VARDEBUGFD1)) == NULL)
	        cp = getenv(VARDEBUGFD2) ;

	if (cp == NULL)
	    cp = getenv(VARDEBUGFD3) ;

	if ((cp != NULL) &&
	    (cfdeci(cp,-1,&fd_debug) >= 0))
	    debugsetfd(fd_debug) ;

	rs = proginfo_start(pip,envv,argv[0],VERSION) ;
	if (rs < 0) {
	    ex = EX_OSERR ;
	    goto badprogstart ;
	}

	proginfo_setbanner(pip,BANNER) ;

	if (bopen(&errfile,BFILE_STDERR,"dwca",0666) >= 0) {
	    pip->efp = &errfile ;
	    bcontrol(&errfile,BC_LINEBUF,0) ;
	}

/* early things to initialize */

	pip->ofp = NULL ;
	pip->ninstr = 0 ;
	pip->debuglevel = 0 ;
	pip->verboselevel = 0 ;

	daytime = time(NULL) ;

/* program mode */

	sl = sfbasename(argv[0],-1,&sp) ;

	pip->progmode = progmode_ssh ;
	for (i = 0 ; progmodes[i] != NULL ; i += 1) {

	    if (strncmp(sp,progmodes[i],sl) == 0)
	        break ;

	} /* end for */

	if (progmodes[i] != NULL)
	    pip->progmode = i ;


	rs = paramopt_start(&aparams) ;

	pip->open.aparams = (rs >= 0) ;

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

	                rs = cfdeci(argp + 1,argl - 1,&argnum) ;

			if (rs < 0)
	                    goto badarg ;

	            } else {

	                aop = argp + 1 ;
	                aol = argl - 1 ;
	                akp = aop ;
	                f_optequal = FALSE ;
	                if ((avp = strchr(aop,'=')) != NULL) {

#if	CF_DEBUGS
	                    debugprintf("main: got an option key w/ a value\n") ;
#endif

	                    akl = avp - aop ;
	                    avp += 1 ;
	                    avl = aop + aol - avp ;
	                    f_optequal = TRUE ;

#if	CF_DEBUGS
	                    debugprint("main: avp=") ;
	                    debugprint(avp) ;
	                    debugprint("\n") ;

	                    debugprint("main: about to 'format'\n") ;

	                    debugprintf("main: aol=%d avp=\"%s\" avl=%d\n",
	                        aol,avp,avl) ;

	                    debugprintf("main: akl=%d akp=\"%s\"\n",
	                        akl,akp) ;
#endif

	                } else {

	                    akl = aol ;
	                    avl = 0 ;

	                }

/* do we have a keyword match or should we assume only key letters ? */

#if	CF_DEBUGS
	                debugprintf("main: about to check for a key word match\n") ;
#endif

	                if ((kwi = optmatch(argopts,akp,akl)) >= 0) {

#if	CF_DEBUGS
	                    debugprintf("main: got an option keyword, kwi=%d\n",
	                        kwi) ;
#endif

	                    switch (kwi) {

/* version */
	                    case argopt_version:
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

#if	CF_DEBUGS
	                        debugprintf("main: option key letters >%c<\n",
	                            *akp) ;
#endif

	                        switch ((int) *akp) {

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

	                            if (argl)
	                                rs = paramopt_loads(&aparams,PO_DUMPOPT,
	                                    argp,argl) ;

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
	                progfname = argp ;

	            break ;

	        case 1:
	            if (argl > 0)
	                sshfname = argp ;

	            break ;

	        default:
	            ex = EX_USAGE ;
	            f_usage = TRUE ;
	            if (efp != NULL)
	                bprintf(efp,
	                    "%s: extra positional arguments ignored\n",
	                    pip->progname) ;

	            f_exitargs = TRUE ;

	        } /* end switch */

	        npa += 1 ;

	    } /* end if (key letter/word or positional) */

	} /* end while (all command line argument processing) */

#if	CF_DEBUGS
	debugprintf("main: done looping on arguments\n") ;
#endif


	if (f_version && (efp != NULL))
	    bprintf(efp,"%s: version %s\n",
	        pip->progname,VERSION) ;

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

	if ((progmodestr != NULL) && (progmodestr[0] != '\0')) {

	    for (i = 0 ; progmodes[i] != NULL ; i += 1) {

	        if (strcmp(progmodestr,progmodes[i]) == 0)
	            break ;

	    } /* end for */

	    if (progmodes[i] == NULL)
	        goto badprogmode ;

	    pip->progmode = i ;

	} /* end if (determining program mode) */


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

	if ((pip->progmode == progmode_ssh) && (npa < 1))
	    goto badargnum ;


/* let's move on */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: programroot=%s\n",pip->programroot) ;
#endif

	if (pip->programroot == NULL) {

	    programroot = getenv(VARPROGRAMROOT1) ;

	    if (programroot == NULL)
	        programroot = getenv(VARPROGRAMROOT2) ;

	    if (programroot == NULL)
	        programroot = getenv(VARPROGRAMROOT3) ;

	    if (programroot == NULL)
	        programroot = PROGRAMROOT ;

	    pip->programroot = programroot ;

	} else {

	    f_programroot = TRUE ;
	    programroot = pip->programroot ;

	}

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: programroot=%s\n",pip->programroot) ;
#endif


/* dumb stuff */

	pip->pid = getpid() ;

	userinfo(&u,userinfobuf,USERINFO_LEN,NULL) ;

	pip->nodename = u.nodename ;

	bufprintf(buf,BUFLEN,"%s%d",pip->nodename,(int) pip->pid) ;

	pip->logid = mallocstrw(buf,LOGIDLEN) ;


	rs = SR_BAD ;
	if (logfname[0] == '\0') {

	    logfile_type = GETFNAME_TYPEROOT ;
	    strcpy(logfname,LOGFNAME) ;

	}

	sl = getfname(pip->programroot,logfname,logfile_type,tmpfname) ;

	if (sl > 0)
	    strwcpy(logfname,tmpfname,sl) ;

	rs = logfile_open(&pip->lh,logfname,0,0666,pip->logid) ;

#if	F_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: 1 logfname=%s rs=%d\n",logfname,rs) ;
#endif

	if (rs >= 0) {

		struct utsname	un ;


#if	F_MASTERDEBUG && CF_DEBUG
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

#if	F_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: we checked its length\n") ;
#endif

#if	F_MASTERDEBUG && CF_DEBUG
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

#if	F_MASTERDEBUG && CF_DEBUG
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

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: ninst=%lld\n",pip->ninstr) ;
#endif

	vecstr_start(&args,10,0) ;

	vecstr_start(&exports,10,0) ;


/* instructions to dump */

	(void) memset(&pip->in,0,sizeof(struct proginfo_select)) ;

	if (instrspec != NULL) {

	    sl = -1 ;
	    if ((cp = strchr(instrspec,':')) != NULL) {

	        if (cfnumull((cp + 1),-1,&pip->in.n) < 0)
			goto badsnum ;

	        sl = cp - instrspec ;

	    } /* end if */

	    if ((sl > 0) && (cfnumull(instrspec,sl,&pip->in.start) < 0))
			goto badsnum ;

	    if (pip->in.n > 0)
	        pip->in.end = pip->in.start + pip->in.n ;

	} /* end if (instructions to dump) */

#if	CF_DEBUG
	if (pip->debuglevel > 1) {
	    debugprintf("main: start=%lld num=%lld end=%lld\n",
		pip->in.start,pip->in.n,pip->in.end) ;
	    debugprintf("main: progbname=%s\n",progbname) ;
	}
#endif


/* option parsing */

	rs = SR_OK ;

	{
	    PARAMOPT_CUR	c ;

	    int	spc = 0 ;


	    paramopt_curbegin(&aparams,&c) ;

	    while (paramopt_enumvalues(&aparams,PO_DUMPOPT,&c,&cp) >= 0) {

	        rs = i = optmatch(dumpopts,cp,-1) ;

		if (i < 0)
			continue ;

	            switch (i) {

	            case procopt_in:
	            case procopt_inum:
	                pip->f.opt_in = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_instr:
	                pip->f.opt_instr = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_instrdis:
	            case procopt_dis:
	                pip->f.opt_instrdis = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_regs:
	                pip->f.opt_regs = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_regvals:
	                pip->f.opt_regs = TRUE ;
	                pip->f.opt_regvals = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_rsa:
	                pip->f.opt_rsa = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_rsv:
	                pip->f.opt_rsa = TRUE ;
	                pip->f.opt_rsv = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_eregs:
	                pip->f.opt_regs = TRUE ;
	                pip->f.opt_eregs = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_mems:
	                pip->f.opt_mems = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_memvals:
	                pip->f.opt_mems = TRUE ;
	                pip->f.opt_memvals = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_msa:
	                pip->f.opt_msa = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_msv:
	                pip->f.opt_msa = TRUE ;
	                pip->f.opt_msv = TRUE ;
	                spc += 1 ;
	                break ;

		    case procopt_mpv:
	                pip->f.opt_mpv = TRUE ;
	                spc += 1 ;
			break ;

	            case procopt_emems:
	                pip->f.opt_mems = TRUE ;
	                pip->f.opt_emems = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_pixie:
	                pip->f.opt_pixie = TRUE ;
	                spc += 1 ;
	                break ;

		    case procopt_full:
		    case procopt_fulldump:
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

	            case procopt_noerrno:
	                pip->f.opt_noerrno = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_onemem:
	                pip->f.opt_onemem = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_syscalls:
	                pip->f.opt_syscalls = TRUE ;
	                spc += 1 ;
	                break ;

	            case procopt_plusinstr:
	                pip->f.opt_plusinstr = TRUE ;
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

		if ((cp = getenv(VARNAME1)) == NULL)
			cp = getenv(VARNAME2) ;

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

#ifdef	COMMENT
	    if (rs < 0) {

	        rs = havefname(pip,"mips",t2fname,progfnamebuf) ;

	        if (rs < 0)
	            rs = havefname(pip,"mips",t1fname,progfnamebuf) ;

	    }
#endif /* COMMENT */

	    if (rs >= 0)
	        progfname = progfnamebuf ;

	} /* end if (finding a program file) */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: proggname=%s\n",progfname) ;
#endif


/* sorry, for this program, if we do not have an executable, we're bad */

		rs = u_stat(progfname,&sb) ;

		if ((rs < 0) || (! S_ISREG(sb.st_mode)))
			goto badprog ;


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


/* create a cache file name if we do not have one already */

	if ((sshfname == NULL) && 
	    (progbname != NULL) && (progbname[0] != '\0')) {

		sshfname = sshfnamebuf ;
		sncpy2(sshfnamebuf,MAXPATHLEN,progbname,".ssh") ;

	} /* end if (cache filename) */


/* store away the base name if we have one */

	pip->basename[0] = '\0' ;
	if (progbname != NULL)
		strwcpy(pip->basename,progbname,MAXNAMELEN) ;


/* jobname ? */

	if ((pip->jobname[0] == '\0') &&
	    (progbname != NULL) && (progbname[0] != '\0')) {

		strwcpy(pip->jobname,progbname, JOBNAMELEN) ;

	}


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

	} /* end if (target arguments) */

	if (argsfname != NULL) {

	    rs = procargs(pip->programroot,argsfname,&args) ;

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

	} /* end if (target program environment) */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: envfname=%s\n",
	        envfname) ;
#endif

	if (envfname != NULL) {

	    rs = procenv(pip->programroot,envfname,&exports) ;

		if ((rs < 0) && f_envfile)
			goto badenvfile ;

#if	CF_DEBUG
	    if (pip->debuglevel > 1) {

	        debugprintf("main: procenv() rs=%d\n",rs) ;

	        for (i = 0 ; rs = vecstr_get(&exports,i,&cp) >= 0 ; i += 1) {

	            if (cp == NULL) continue ;

	            debugprintf("main: e> %s\n",cp) ;

	        }
	    }
#endif /* CF_DEBUG */

	} /* end if (user specified environment) */

	} /* end block (arguments) */



/* map the program file if we have one */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: progfname=%s\n",progfname) ;
#endif

	f = FALSE ;
	f = f || pip->f.opt_instr || pip->f.opt_instrdis | pip->f.opt_msv ;
	f = f || (pip->progmode == progmode_ssh) ;
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

/* sorry, for this program, if you didn't map, we're dead */

	if (rs < 0)
		goto badmap ;


/* should we initialize for disassembly ? */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: disassemble ?\n") ;
#endif

	f = FALSE ;
	f = f || (pip->f.progmap && pip->f.opt_instrdis) ;
#ifdef	COMMENT
	f = f || (pip->progmode == progmode_ssh) ;
#endif

	if ((progfname != NULL) && f) {

	    rs = mipsdis_init(&dis,NULL,progfname) ;

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

	case progmode_ssh:

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("main: finding SSHs \n") ;
#endif

		rs = process(pip,&pm,&dis,sshfname) ;

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

	if (rs < 0)
		bprintf(pip->efp,"%s: result rs=%d\n",
			pip->progname,rs) ;


	bclose(&outfile) ;



	if (pip->f.instrdis)
	    mipsdis_free(&dis) ;


	if (pip->f.progmap)
	    lmapprog_free(&pm) ;


	ex = EX_OK ;
	if (rs == SR_NOENT)
	    ex = EX_NOINPUT ;

	else if (rs < 0)
	    ex = EX_DATAERR ;


done:
ret3:
	if (pip->f.log)
		logfile_close(&pip->lh) ;

retearly:
ret2:
	vecstr_finish(&args) ;

	vecstr_finish(&exports) ;

	if ((pip->debuglevel > 0) && (pip->efp != NULL))
		bprintf(pip->efp,"%s: ex=%d\n",pip->progname,ex) ;

ret1:
	if (pip->open.aparams)
	    paramopt_finish(&aparams) ;

	if (pip->efp != NULL)
	    bclose(pip->efp) ;

	proginfo_finish(pip) ;

badprogstart:
	return ex ;

/* bad stuff */
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
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: a bad (NULL, whatever !) argument was given\n",
	        pip->progname) ;

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

badprog:
	ex = EX_NOINPUT ;
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: could not open the program executable (%d)\n",
	        pip->progname,rs) ;

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

badmap:
	ex = EX_NOINPUT ;
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: could not map the program executable file (%d)\n",
	        pip->progname,rs) ;

	goto done ;

badoutopen:
	ex = EX_CANTCREAT ;
	goto done ;
}
/* end subroutine (main) */



/* LOCAL SUBROUTINES */



/* print out (standard error) some short usage */
static int usage(pip)
struct proginfo	*pip ;
{
	int	rs = SR_NOTOPEN ;


	if (pip->efp == NULL)
	    return rs ;


	rs = SR_OK ;
	bprintf(pip->efp,
	    "%s: USAGE> %s progfile sshfile [-n progname]\n",
	    pip->progname,pip->progname) ;

	return rs ;
}
/* end subroutine (usage) */


/* do we have a file with the new extension ? */
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

	else if ((dp != NULL) && (bnp == NULL))
	    sp = strwcpy(tmpfname,tfname,(dp - tfname)) ;

	else
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



