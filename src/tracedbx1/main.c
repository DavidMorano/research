/* main */

/* compare execution traces */
/* last modified %G% version %I% */


#define	CF_DEBUGS	0		/* debug print-outs (non-switchable) */
#define	CF_DEBUG	0		/* debug print-outs switchable */


/* revision history:

	= 90/11/01, David Morano

	This subroutine was originally written.


	= 01/08/01, David Morano

	I don't know where this subroutine originally came from but
	it's been a long time since 1990 !  I forget where I grabbed
	this from but it is part of the TRACECMP program for Levo
	research now !


	= 01/09/06, David Morano

	OK, Here we go one more time.  I am hacking this subroutine up
	to tbe the front-end for the TRACEDBX program.  This program
	will convert the Alireza-format DBX comparison trace format
	into an EXECTRACE type trace.


*/

/* Copyright © 1998 David Morano.  All rights reserved. */

/**************************************************************************

	Synopsis:

	$ tracedbx program.dbxtrace program.et

	Two arguments, the second is the output and it will be binary!


**************************************************************************/


/* Copyright © 1998 David Morano.  All rights reserved. */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>

#include	<vsystem.h>
#include	<bfile.h>
#include	<vecstr.h>
#include	<paramopt.h>
#include	<exitcodes.h>
#include	<localmisc.h>

#include	"exectrace.h"
#include	"config.h"
#include	"defs.h"


/* external subroutines */

extern int	matostr(char **,int,const char *,int) ;
extern int	sfbasename(const char *,int,const char **) ;
extern int	cfdeci(const char *,int,int *) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strbasename(char *) ;


/* external variables */

extern int	convert(struct proginfo *,char *,char *) ;


/* local structures */


/* forward references */

static int	usage(struct proginfo *) ;
static int	haveprogfname(struct proginfo* ,const char *,char *) ;


/* local variables */

static const char *argopts[] = {
	        "VERSION",
	        NULL
} ;

#define	ARGOPT_VERSION	0

static const char	*progmodes[] = {
	        "analyze",
	        "dump",
	        "tracedump",
	        NULL
} ;

#define	PROGMODE_ANALYZE	0
#define	PROGMODE_DUMP		1
#define	PROGMODE_TRACEDUMP	2

static const char	*dumpopts[] = {
	        "regs",
	        "regvals",
	        "mems",
	        "memvals",
	        "instr",
	        "instrdis",
	        "dis",
	        NULL
} ;

#define	DUMPOPT_REGS		0
#define	DUMPOPT_REGVALS		1
#define	DUMPOPT_MEMS		2
#define	DUMPOPT_MEMVALS		3
#define	DUMPOPT_INSTR		4
#define	DUMPOPT_INSTRDIS	5
#define	DUMPOPT_DIS		6


/* exported subroutines */


int main(argc,argv,envv)
int	argc ;
char	*argv[] ;
char	*envv[] ;
{
	struct ustat	sb ;

	struct proginfo	pi, *pip = &pi ;

	PARAMOPT	aparams ;

	vecstr		args, exports ;

	bfile		errfile, *efp = &errfile ;
	bfile		outfile ;

	int	argr, argl, aol, akl, avl ;
	int	kwi, npa, i ;
	int	rs, sl ;
	int	ex = EX_INFO ;
	int	unknown ;
	int	fd_debug = -1 ;
	int	f_optplus, f_optminus, f_optequal ;
	int	f_exitargs = FALSE ;
	int	f_programroot = FALSE ;
	int	f_version = FALSE ;
	int	f_usage = FALSE ;
	int	f ;

	const char	*argp, *aop, *akp, *avp ;
	char	progfnamebuf[MAXPATHLEN + 1] ;
	const char	*programroot = NULL ;
	const char	*progfname = NULL ;
	const char	*argsfname = NULL ;
	const char	*envfname = NULL ;
	const char	*t1fname = NULL ;
	const char	*t2fname = NULL ;
	const char	*ofname = NULL ;
	const char	*progmodestr = NULL ;
	const char	*sp, *cp ;


	if ((cp = getenv(VARDEBUGFD1)) == NULL)
	    cp = getenv(VARDEBUGFD2) ;

	if ((cp != NULL) &&
	    (cfdeci(cp,-1,&fd_debug) >= 0))
	    debugsetfd(fd_debug) ;


	if (bopen(&errfile,BFILE_STDERR,"dwca",0666) >= 0)
	    pip->efp = &errfile ;
	    bcontrol(&errfile,BC_LINEBUF,0) ;
	}

	(void) memset(pip,0,sizeof(struct proginfo)) ;

	pip->efp = efp ;
	pip->progname = strbasename(argv[0]) ;


/* early things to initialize */

	pip->version = VERSION ;
	pip->programroot = NULL ;

	pip->debuglevel = 0 ;
	pip->verboselevel = 0 ;

/* program mode */

	sl = sfbasename(argv[0],-1,&sp) ;

	pip->progmode = PROGMODE_ANALYZE ;
	for (i = 0 ; progmodes[i] != NULL ; i += 1) {

	    if (strncmp(sp,progmodes[i],sl) == 0)
	        break ;

	} /* end for */

	if (progmodes[i] != NULL)
	    pip->progmode = i ;


	rs = paramopt_start(&aparams) ;

	pip->open.aparams = (rs >= 0) ;

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

/* do we have a keyword or only key letters ? */

	                if ((kwi = matostr(argopts,2,akp,akl)) >= 0) {

#if	CF_DEBUGS
	                    debugprintf("main: got an option keyword, kwi=%d\n",
	                        kwi) ;
#endif

	                    switch (kwi) {

/* version */
	                    case ARGOPT_VERSION:
	                        f_version = TRUE ;
	                        if (f_optequal)
	                            goto badargextra ;

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

	                                paramopt_loads(&aparams,DUMPOPTS,
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

	if (pip->debuglevel > 0)
	    bprintf(efp,"%s: debuglevel=%u\n",
	        pip->progname,pip->debuglevel) ;

	if (f_version && (efp != NULL))
	    bprintf(efp,"%s: version %s\n",
	        pip->progname,VERSION) ;

	if (f_usage) {

	    rs = usage(pip) ;

	    goto done ;
	}

	if (f_version)
	    goto done ;

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

	if (pip->progmode == PROGMODE_TRACEDUMP)
	    pip->progmode = PROGMODE_DUMP ;

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

	if ((pip->progmode == PROGMODE_ANALYZE) && (npa < 2))
	    goto badargnum ;

	if ((pip->progmode == PROGMODE_DUMP) && (npa < 1))
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

/* option parsing */

	{
	    PARAMOPT_CUR	c ;

	    int	spc = 0 ;


	    paramopt_nullcursor(&aparams,&c) ;

	    while (paramopt_enumvalues(&aparams,DUMPOPTS,&c,&cp) >= 0) {

#if	CF_DEBUG
	        if (pip->debuglevel > 1)
	            debugprintf("main: option=%s\n",cp) ;
#endif

	        if ((i = matostr(dumpopts,2,cp,-1)) >= 0) {

#if	CF_DEBUG
	            if (pip->debuglevel > 1)
	                debugprintf("main: oi=%d\n",i) ;
#endif

	            switch (i) {

	            case DUMPOPT_REGS:
	                pip->f.opt_regs = TRUE ;
	                spc += 1 ;
	                break ;

	            case DUMPOPT_REGVALS:
	                pip->f.opt_regs = TRUE ;
	                pip->f.opt_regvals = TRUE ;
	                spc += 1 ;
	                break ;

	            case DUMPOPT_MEMS:
	                pip->f.opt_mems = TRUE ;
	                spc += 1 ;
	                break ;

	            case DUMPOPT_MEMVALS:
	                pip->f.opt_mems = TRUE ;
	                pip->f.opt_memvals = TRUE ;
	                spc += 1 ;
	                break ;

	            case DUMPOPT_INSTR:
	                pip->f.opt_instr = TRUE ;
	                spc += 1 ;
	                break ;

	            case DUMPOPT_INSTRDIS:
	            case DUMPOPT_DIS:
	                pip->f.opt_instr = TRUE ;
	                pip->f.opt_instrdis = TRUE ;
	                spc += 1 ;
	                break ;

	            } /* end switch */

	        } else
	            bprintf(efp,"%s: unrecognized option \"%s\"\n",
	                pip->progname,cp) ;

	    } /* end while */

	} /* end block */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("main: dump options\n") ;
#endif

	vecstr_start(&args,10,0) ;

	vecstr(&exports,10,0) ;


	ex = EX_DATAERR ;


	if ((rs = bopen(&outfile,BFILE_STDOUT,"wct",0666)) >= 0)
	    pip->ofp = &outfile ;


	rs = convert(pip,t1fname,t2fname) ;


	if (pip->ofp != NULL)
	    bclose(pip->ofp) ;


	ex = EX_OK ;
	if (rs == SR_NOENT)
	    ex = EX_NOINPUT ;

	else if (rs < 0)
	    ex = EX_DATAERR ;

done:
	vecstr_finish(&args) ;

	vecstr_finish(&exports) ;

retearly:
ret2:
ret1:
	if (pip->open.aparams)
	    paramopt_finish(&aparams) ;

	if (pip->efp != NULL)
	    bclose(pip->efp) ;

ret0:
	proginfo_free(pip) ;

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
	if (pip->efp != NULL)
	    bprintf(pip->efp,
	        "%s: a bad (NULL, whatever !) argument was given\n",
	        pip->progname) ;

	goto done ;

#ifdef	COMMENT

usage:
	ex = EX_USAGE ;
	rs = usage(pip) ;

	goto done ;

#endif /* COMMENT */

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
	    "%s: USAGE> %s dbxtracefile outtrace [-v]\n",
	    pip->progname,pip->progname) ;

	bprintf(pip->efp,
	    "%s: \t[-VD]\n",
	    pip->progname) ;

	return rs ;
}
/* end subroutine (usage) */


static int haveprogfname(pip,tfname,tmpfname)
struct proginfo	*pip ;
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

	if ((sp - tmpfname + 4) < (MAXPATHLEN - 1)) {

	    strcpy(sp,".mips") ;

#if	CF_DEBUGS
	    debugprintf("main/haveprogfname: execfile=%s\n",tmpfname) ;
#endif

	    rs = (sp - tmpfname) + 4 ;

	} /* end if */

	return rs ;
}
/* end subroutine (haveprogfname) */



