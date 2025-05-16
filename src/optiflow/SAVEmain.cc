/* main.c - main line routines */


#define	F_DEBUGS	1		/* non-switchable debug print-outs */
#define	F_DEBUG		1
#define	F_DEBUGFORCE	0		/* force the debuglevel to something */
#define	F_GETEXECNAME	0		/* use 'getexecname(3c)' */


/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 2000-2001 by The Regents of The University of Michigan.
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#define	DAM	1


#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifndef _MSC_VER
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef BFD_LOADER
#include <bfd.h>
#endif /* BFD_LOADER */

#include	<bitops.h>
#include	<field.h>
#include	<bio.h>
#include	<vecstr.h>
#include	<paramfile.h>
#include	<userinfo.h>
#include	<logfile.h>
#include	<exitcodes.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"
#include	"getfname.h"

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "endian.h"
#include "version.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "loader.h"
#include "sim.h"



/* local defines */

#define	MAXARGINDEX	100
#define	MAXARGGROUPS	(MAXARGINDEX/8 + 1)

#ifndef	LINEBUFLEN
#define	LINEBUFLEN	(MAXPATHLEN + 30)
#endif

#ifndef	FNE_STATS
#define	FNE_STATS		".stats"
#endif

#ifndef	FNE_TELL
#define	FNE_TELL		".tell"
#endif

/* default simulator scheduling priority */
#define NICE_DEFAULT_VALUE		0



/* external subroutines */

extern int	sncpy1(char *,int,const char *) ;
extern int	sncpy2(char *,int,const char *,const char *) ;
extern int	sfbasename(const char *,int,char **) ;
extern int	optmatch(const char **,const char *,int) ;
extern int	optmatch2(const char **,const char *,int) ;
extern int	vstrkeycmp(char **,char **) ;
extern int	mkpath1(char *,const char *) ;
extern int	mkpath2(char *,const char *,const char *) ;
extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecui(const char *,int,uint *) ;
extern int	cfdecll(const char *,int,LONG *) ;
extern int	cfdecull(const char *,int,ULONG *) ;
extern int	cfdecmfull(const char *,int,ULONG *) ;
extern int	getfname(char *,char *,int,char *) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnchr(const char *,int,int) ;
extern char	*timestr_logz(time_t,char *) ;


/* external variables */

struct proginfo		pi ;


/* local structures */

struct pav {
	char	*pr ;
	int	ex ;
	int	f_version ;
	int	f_usage ;
} ;


/* forward references */

void		sim_print_stats(FILE *) ;

static int	procargs(struct proginfo *,struct pav *,int,char **) ;
static int	orphan_fn(int, int , char **) ;
static int	procn(struct proginfo *,const char *,int) ;

static void	signal_sim_stats(int) ;
static void	signal_exit_now(int) ;
static void	banner(FILE *, int, char **) ;
static void	exit_now(int) ;
static void	usage(FILE *, int, char **) ;


/* local variables */

static const char *argopts[] = {
	"TMPDIR",
	"VERSION",
	"VERBOSE",
	"ROOT",
	"LOGFILE",
	"CONFIG",
	"PARAMTAB",
	"DEBUG",
	"DS",
	"bn",
	"sp",
	"skip",
	"winstr",
	"ninstr",
	"linstr",
	"et",
	"s",
	"in",
	"ck",
	NULL
} ;

enum argopts {
	argopt_tmpdname,
	argopt_version,
	argopt_verbose,
	argopt_root,
	argopt_logfile,
	argopt_config,
	argopt_paramtab,
	argopt_debug,
	argopt_ds,
	argopt_bn,
	argopt_sp,
	argopt_skip,
	argopt_winstr,
	argopt_ninstr,
	argopt_linstr,
	argopt_et,
	argopt_select,
	argopt_in,
	argopt_ck,
	argopt_overlast
} ;


/* other local and global variables from original simple-scalar */

/* execution instruction counter */
counter_t sim_num_insn = 0;

#if 0 /* not portable... :-( */
/* total simulator (data) memory usage */
unsigned int sim_mem_usage = 0;
#endif

/* execution start/end times */
time_t sim_start_time;
time_t sim_end_time;
int sim_elapsed_time;

/* byte/word swapping required to execute target executable on this host */
int sim_swap_bytes;
int sim_swap_words;

/* exit when this becomes non-zero */
int sim_exit_now = FALSE;

/* longjmp here when simulation is completed */
jmp_buf sim_exit_buf;

/* set to non-zero when simulator should dump statistics */
int sim_dump_stats = FALSE;

/* options database */
struct opt_odb_t *sim_odb;

/* stats database */
USTAT_sdb_t *sim_sdb;

/* EIO interfaces */
char *sim_eio_fname = NULL;
char *sim_chkpt_fname = NULL;
FILE *sim_eio_fd = NULL;

/* redirected program/simulator output file names */
static char *sim_simout = NULL;
static char *sim_progout = NULL;
FILE *sim_progfd = NULL;

/* track first argument orphan, this is the program to execute */
static int exec_index = -1;

/* dump help information */
static int help_me;

/* random number generator seed */
static int rand_seed;

/* initialize and quit immediately */
static int init_quit;

#ifndef _MSC_VER
/* simulator scheduling priority */
static int nice_priority;
#endif

static int running = FALSE;







int main(argc,argv,envv)
int	argc ;
char	*argv[] ;
char	*envv[] ;
{
	USTAT	sb ;

	struct proginfo	*pip = &pi ;

	struct pav	pav ;

	struct userinfo	u ;

	bfile	errfile, outfile ;

	time_t	daytime ;

	ULONG	maxclocks = 0, skipinstr = 0 ;
	ULONG	ullw ;

	int	rs, rs1, sl, cl ;
	int	ex = EX_INFO ;
	int	i, exit_code;
	int	fd_debug ;

	char	userinfobuf[USERINFO_LEN + 1] ;
	char	tmpfname[MAXPATHLEN + 1] ;
	char	timebuf[TIMEBUFLEN + 1] ;
	char	**envp = envv ;
	char	*pr = NULL ;
	char	*sp, *cp ;
  	char *s;


	if ((cp = getenv(VARDEBUGFD1)) == NULL)
	    cp = getenv(VARDEBUGFD2) ;

	if ((cp != NULL) &&
	    (cfdeci(cp,-1,&fd_debug) >= 0))
	    esetfd(fd_debug) ;


	(void) memset(pip,0,sizeof(struct proginfo)) ;

	proginfo_init(pip,envv,argv[0],VERSION) ;

	proginfo_banner(pip,BANNER) ;

#if	F_DEBUGS
	eprintf("main: 0a pip->pr=%s\n",pip->pr) ;
#endif


/* open STDERR */

	if (bopen(&errfile,BIO_STDERR,"dwca",0666) >= 0) {

	    pip->efp = &errfile ;
	    pip->f.errfile = TRUE ;
	    bcontrol(&errfile,BC_LINEBUF,0) ;

	}


/* some defaults values */

	pip->sinstr = (ULONG) -1 ;
	pip->winstr = (ULONG) -1 ;
	pip->ninstr = (ULONG) -1 ;
	pip->linstr = (ULONG) -1 ;
	pip->minstr = (ULONG) -1 ;
	pip->mclock = (ULONG) -1 ;


/* process the private arguments */

	memset(&pav,0,sizeof(struct pav)) ;

	if ((cp = getenv(VARARGS)) != NULL) {

		vecstr	args ;

		FIELD	fsb ;

		char	argbuf[LINEBUFLEN + 1] ;


		vecstr_init(&args,10,VECSTR_PSORTED) ;

		if (argv[0] != NULL)
			vecstr_add(&args,argv[0],-1) ;

		else
			vecstr_add(&args,SEARCHNAME,-1) ;

		field_init(&fsb,cp,-1) ;

		while (field_sharg(&fsb,NULL,argbuf,LINEBUFLEN) >= 0) {

#if	F_DEBUGS
	eprintf("main: opt=>%w<\n",fsb.fp,fsb.flen) ;
#endif

			vecstr_add(&args,fsb.fp,fsb.flen) ;

		} /* end while */

		field_free(&fsb) ;

/* OK, process the arguments */

		rs = vecstr_count(&args) ;

#if	F_DEBUGS
	eprintf("main: procargs() argc=%d\n",rs) ;
#endif

		ex = procargs(pip,&pav,rs,args.va) ;

#if	F_DEBUGS
	eprintf("main: procargs() ex=%d\n",ex) ;
#endif

		vecstr_free(&args) ;

		ex = pav.ex ;
		pr = pav.pr ;

	} else
		ex = EX_OK ;

	if (ex != 0) {

#if	F_DEBUGS
	eprintf("main: exiting early\n") ;
#endif

		goto exitnow ;
	}

#if	F_DEBUGFORCE
	pip->debuglevel = 2 ;
#endif

#if	F_DEBUGS
	eprintf("main: debuglevel=%d\n",pip->debuglevel) ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(1))
	eprintf("main: debuglevel=%d\n",pip->debuglevel) ;
#endif


#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(1)) {
	eprintf("main: ck=%llu:%llu in=%llu:%llu\n",
		pip->ck.start,pip->ck.n,
		pip->in.start,pip->in.n) ;
	}
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(3))
	eprintf("main: 0 mclock=%lld sinstr=%lld ninstr=%lld\n",
		pip->mclock,
		pip->sinstr,
		pip->ninstr) ;
#endif


/* get program root (mostly just for logging) */

#if	F_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("main: 1 pr=%s\n",pr) ;
#endif

	if (pr == NULL) {

	    pr = getenv(VARPROGRAMROOT1) ;

	    if (pr == NULL)
	        pr = getenv(VARPROGRAMROOT2) ;

	    if (pr == NULL)
	        pr = getenv(VARPROGRAMROOT3) ;

#if	F_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("main: 2 pr=%s\n",pr) ;
#endif

/* try to see if a path was given at invocation */

	    if ((pr == NULL) && (pip->progdname != NULL))
	        proginfo_rootprogdname(pip) ;

#if	F_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("main: 3 pr=%s pip->pr=%s\n",pr,pip->pr) ;
#endif

/* do the special thing */

#if	F_GETEXECNAME && defined(SOLARIS) && (SOLARIS >= 8)
	    if ((pr == NULL) && (pip->pr == NULL) {

	        const char	*pp ;


	        pp = getexecname() ;

	        if (pp != NULL)
	            proginfo_rootexecname(pip,pp) ;

#if	F_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("main: 4 pr=%s pip->pr=%s\n",pr,pip->pr) ;
#endif

	    }
#endif /* SOLARIS */

	    if (pip->pr == NULL) {

	        if (pr == NULL)
	            pr = PROGRAMROOT ;

#if	F_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("main: 5 pr=%s pip->pr=%s\n",pr,pip->pr) ;
#endif

		proginfo_setroot(pip,pr,-1) ;

	    }

	} /* end if (getting a program root) */

#if	F_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("main: 6 pr=%s\n",pip->pr) ;
#endif

	if (pip->debuglevel > 0)
	    bprintf(pip->efp,"%s: pr=%s\n",
	        pip->progname,pip->pr) ;

/* some initialization */

	if (pip->benchname[0] == '\0') {

	    if ((cp = getenv(VARBENCHNAME)) != NULL)
		rs = sncpy1(pip->benchname,MAXNAMELEN, cp) ;

	}

	if (pip->spfname[0] == '\0') {

	    if ((cp = getenv(VARSPFNAME)) != NULL)
		rs = sncpy1(pip->spfname,MAXNAMELEN, cp) ;

	}

/* do we have any parameter file (DAM addition) ? */

#ifdef	DAM
	cp = pip->paramfname ;
	if (cp[0] == '\0') {

		cp = getenv(VARPARAM) ;

		if (cp != NULL)
			strwcpy(pip->paramfname,cp,(MAXPATHLEN - 1)) ;

	}

	if ((cp != NULL) && (cp[0] != '\0')) {

		rs = paramfile_open(&pip->pf,envv,cp) ;

		pip->f.params = (rs >= 0) ;

	} /* end if (parameter file) */
#endif/* DAM */

/* continue with the rest */

	rs = bopen(&outfile,LEVOOUTFNAME,"wct",0666) ;

	if (rs >= 0)
		pip->ofp = &outfile ;

#ifdef	COMMENT
	rs = bprintf(pip->ofp,"I can print something at start\n") ;

#if	F_DEBUGS
	eprintf("main: bprintf() rs=%d \n",rs) ;
#endif

	bflush(pip->ofp) ;
#endif /* COMMENT */

#ifndef _MSC_VER
  /* catch SIGUSR1 and dump intermediate stats */
  signal(SIGUSR1, signal_sim_stats);

  /* catch SIGUSR2 and dump final stats and exit */
  signal(SIGUSR2, signal_exit_now);
#endif /* _MSC_VER */

  /* register an error handler */
  fatal_hook(sim_print_stats);

  /* set up a non-local exit point */
  if ((exit_code = setjmp(sim_exit_buf)) != 0) {

      /* special handling as longjmp cannot pass 0 */

#ifdef	COMMENT

      exit_now(exit_code);

#else /* COMMENT */

#if	F_DEBUGS
	eprintf("main: setjmp() returned\n") ;
#endif

#if	CF_MASTERDEBUG && F_DEBUGS
	if (DEBUGLEVEL(4))
	eprintf("main: setjmp() returned\n") ;
#endif


	if (exit_code == 0)
		exit_code -= 1 ;

	goto exitnow ;

#endif /* COMMENT */

    }

  /* register global options */
  sim_odb = opt_new(orphan_fn);

  opt_reg_flag(sim_odb, "-h", "print help message",
	       &help_me, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_flag(sim_odb, "-v", "verbose operation",
	       &verbose, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_flag(sim_odb, "-vr", "verbose registers operation",
	       &verbose_regs, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_uint(sim_odb, 
	"-trigger:inst", "trigger instruction for verbose operation",
               &trigger_inst, /* default */0,
               /* print */TRUE, /* format */NULL);

#ifdef DEBUG
  opt_reg_flag(sim_odb, "-d", "enable debug message",
	       &debugging, /* default */FALSE, /* !print */FALSE, NULL);
#endif /* DEBUG */

  opt_reg_flag(sim_odb, "-i", "start in Dlite debugger",
	       &dlite_active, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_int(sim_odb, "-seed",
	      "random number generator seed (0 for timer seed)",
	      &rand_seed, /* default */1, /* print */TRUE, NULL);
  opt_reg_flag(sim_odb, "-q", "initialize and terminate immediately",
	       &init_quit, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_string(sim_odb, "-chkpt", "restore EIO trace execution from <fname>",
		 &sim_chkpt_fname, /* default */NULL, /* !print */FALSE, NULL);

  /* stdio redirection options */
  opt_reg_string(sim_odb, "-redir:sim",
		 "redirect simulator output to file (non-interactive only)",
		 &sim_simout,
		 /* default */NULL, /* !print */FALSE, NULL);
  opt_reg_string(sim_odb, "-redir:prog",
		 "redirect simulated program output to file",
		 &sim_progout, /* default */NULL, /* !print */FALSE, NULL);

#ifndef _MSC_VER
  /* scheduling priority option */
  opt_reg_int(sim_odb, "-nice",
	      "simulator scheduling priority", &nice_priority,
	      /* default */NICE_DEFAULT_VALUE, /* print */TRUE, NULL);
#endif

  /* FIXME: add stats intervals and max insts... */

  /* register all simulator-specific options */
  sim_reg_options(sim_odb);

#ifdef	DAM
	if ((cp = getenv(VARSSCONFIG)) != NULL) {

		int	size ;

		char	**newargv ;


		size = (argc + 3) * sizeof(char *) ;
		uc_malloc(size,&newargv) ;

		for (i = 0 ; i < argc ; i += 1) {

			newargv[i] = argv[i] ;

		} /* end for */

		newargv[i++] = "-config" ;
		newargv[i++] = cp ;
		newargv[i] = NULL ;

		argv = newargv ;

	} /* end if (configuration file) */
#endif /* DAM */


  /* parse simulator options */
  exec_index = -1;
  opt_process_options(sim_odb, argc, argv);


  /* redirect I/O? */
  if (sim_simout != NULL)
    {
      /* send simulator non-interactive output (STDERR) to file SIM_SIMOUT */
      fflush(stderr);
      if (!freopen(sim_simout, "w", stderr))
	fatal("unable to redirect simulator output to file `%s'", sim_simout);
    }

  if (sim_progout != NULL)
    {
      /* redirect simulated program output to file SIM_PROGOUT */
      sim_progfd = fopen(sim_progout, "w");
      if (!sim_progfd)
	fatal("unable to redirect program output to file `%s'", sim_progout);
    }

  /* need at least two argv values to run */
  if (argc < 2)
    {
      banner(stderr, argc, argv);
      usage(stderr, argc, argv);
      exit(1);
    }

  /* opening banner */
  banner(stderr, argc, argv);

  if (help_me)
    {
      /* print help message and exit */
      usage(stderr, argc, argv);
      exit(1);
    }

  /* seed the random number generator */
  if (rand_seed == 0)
    {
      /* seed with the timer value, true random */
      mysrand(time((time_t *)NULL));
    }
  else
    {
      /* seed with default or user-specified random number generator seed */
      mysrand(rand_seed);
    }

  /* exec_index is set in orphan_fn() */
  if (exec_index == -1)
    {
      /* executable was not found */
      fprintf(stderr, "error: no executable specified\n");
      usage(stderr, argc, argv);
      exit(1);
    }
  /* else, exec_index points to simulated program arguments */

  /* check simulator-specific options */
  sim_check_options(sim_odb, argc, argv);

#ifndef _MSC_VER
  /* set simulator scheduling priority */
  if (nice(0) < nice_priority)
    {
      if (nice(nice_priority - nice(0)) < 0)
        fatal("could not renice simulator process");
    }
#endif

  /* default architected value... */
  sim_num_insn = 0;

#ifdef BFD_LOADER
  /* initialize the bfd library */
  bfd_init();
#endif /* BFD_LOADER */

  /* initialize the instruction decoder */
  md_init_decoder();

  /* initialize all simulation modules */

#if	F_DEBUGS
	eprintf("main: sim_init()\n") ;
#endif

  rs = sim_init() ;

	if (rs < 0)
	goto ret0 ;

  /* initialize architected state */
  sim_load_prog(argv[exec_index], 
	(argc - exec_index), (argv + exec_index), envp) ;

/* save the target program name for later */

	if ((exec_index > 0) && (argv[exec_index] != NULL)) {

		cl = sfbasename(argv[exec_index],-1,&cp) ;

		if (cl > 0)
			cl = MIN(cl,MAXNAMELEN) ;

		if (cl > 0)
			strwcpy(pip->targetname,cp,cl) ;

	} /* end if (loading target name) */

/* do we have a jobname yet ? */

	if (pip->jobname[0] == '\0') {

		if ((cp = getenv(VARJOBNAME)) != NULL)
			strwcpy(pip->jobname,cp,JOBNAMELEN) ;

		if (pip->jobname[0] == '\0')
			strwcpy(pip->jobname,pip->targetname,JOBNAMELEN) ;

	}

#if	F_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("main: jobname=%s\n",pip->jobname) ;
#endif

	mkfnamesuf(pip->tellfname,pip->jobname,FNE_TELL) ;

/* open the program-root log file and make a log entry */

#if	F_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("main: 0 logfname=%s\n",pip->logfname) ;
#endif

		rs1 = userinfo(&u,userinfobuf,USERINFO_LEN) ;

#if	F_DEBUG
	if (DEBUGLEVEL(2))
	    eprintf("main: userinfo() rs=%d\n",rs1) ;
#endif

		pip->banner = BANNER ;
		pip->pid = u.pid ;
		pip->domainname = u.domainname ;
		pip->nodename = u.nodename ;
		pip->username = u.username ;

		if ((u.gecosname != NULL) && (u.gecosname[0] != '\0'))
			strwcpy(pip->realname,u.gecosname,NODENAMELEN) ;

	daytime = time(NULL) ;

/* can we open a log file ? */

		if (pip->logfname[0] == '\0')
			mkpath2(pip->logfname,pip->pr,LOGFNAME) ;

		rs = logfile_open(&pip->lh,pip->logfname,0666,u.logid) ;

		if (rs >= 0) {

			pip->f.log = TRUE ;
			logfile_printf(&pip->lh,
				"%s %s\n",
				timestr_logz(daytime,timebuf),
				BANNER) ;

	    if (pip->realname[0] != '\0')
	    logfile_printf(&pip->lh,"%s!%s (%s)\n",
	        pip->nodename,pip->username,pip->realname) ;

		else
	    logfile_printf(&pip->lh,"%s!%s\n",
	        pip->nodename,pip->username) ;

			logfile_printf(&pip->lh,
				"jobname=%s pid=%u\n",pip->jobname,pip->pid) ;

			logfile_printf(&pip->lh,
				"target=%s\n",pip->targetname) ;

			logfile_flush(&pip->lh) ;

	} /* end if (log file opened) */

	if (pip->f.log) {

		logfile_printf(&pip->lh,
		"benchname=%s",pip->benchname) ;

		logfile_printf(&pip->lh,
		"sinstr=%llu ninstr=%llu",
		pip->sinstr,pip->ninstr) ;

	}

/* OK, make the statistics file */

	{
		bfile	sfile ;


	mkfnamesuf(pip->statsfname,pip->jobname,FNE_STATS) ;

	rs = bopen(&sfile,pip->statsfname,"wct",0666) ;

	if (rs >= 0) {

		daytime = time(NULL) ;


		bprintf(&sfile,"STATS %s\n",
			timestr_logz(daytime,timebuf)) ;

		bprintf(&sfile,"target= %s\n",pip->targetname) ;

		bprintf(&sfile,"jobname= %s\n",pip->jobname) ;

		bprintf(&sfile,"\n") ;

		bclose(&sfile) ;

	} /* end if (statistics file) */

	} /* end block (statistics file) */


  /* register all simulator stats */
  sim_sdb = stat_new();
  sim_reg_stats(sim_sdb);

#if 0 /* not portable... :-( */
  stat_reg_uint(sim_sdb, "sim_mem_usage",
		"total simulator (data) memory usage",
		&sim_mem_usage, sim_mem_usage, "%11dk");
#endif

  /* record start of execution time, used in rate stats */
  sim_start_time = time((time_t *)NULL);

  /* emit the command line for later reuse */
  fprintf(stderr, "sim: command line: ");
  for (i=0; i < argc; i++)
    fprintf(stderr, "%s ", argv[i]);

  fprintf(stderr, "\n");

  /* output simulation conditions */
  s = ctime(&sim_start_time);
  if (s[strlen(s)-1] == '\n')
    s[strlen(s)-1] = '\0';

  fprintf(stderr, "\nsim: simulation started @ %s, options follow:\n", s);
  opt_print_options(sim_odb, stderr, /* short */TRUE, /* notes */TRUE);
  sim_aux_config(stderr);
  fprintf(stderr, "\n");

  /* omit option dump time from rate stats */
  sim_start_time = time((time_t *)NULL);

#ifdef	COMMENT
  if (init_quit)
    exit_now(0);
#else
	if (init_quit) {

#if	F_DEBUGS
	eprintf("main: init_quit !\n") ;
#endif

		goto exitnow ;
	}
#endif /* COMMENT */


/* do the simulation */

#if	F_DEBUGS
	eprintf("main: sim_main()\n") ;
#endif

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("main: sim_main()\n") ;
#endif

  running = TRUE;
  sim_main();

#if	F_DEBUGS
	eprintf("main: sim_main() returned \n") ;
#endif

  /* simulation finished early */
exitnow:

#if	F_DEBUGS
	eprintf("main: exiting ex=%u\n",ex) ;
#endif

#ifdef	DAM
	if (pip->f.params)
		paramfile_close(&pip->pf) ;
#endif /* DAM */

  exit_now(0);

ret0:
  return ex ;
}
/* end subroutine (main) */


int procsimpoint(pip,ulwp)
struct proginfo	*pip ;
ULONG		*ulwp ;
{
	PARAMFILE		simpoints ;

	PARAMFILE_CURSOR	cur ;

	int	rs = SR_OK ;
	int	vlen ;

	char	vbuf[VBUFLEN + 1] ;
	char	*cp ;


	if (pip->benchname[0] == '\0') {
		rs = SR_NOENT ;
		goto done ;
	}

	cp = pip->spfname ;
	if (cp[0] == '\0')
		cp = SPFNAME ;

	rs = paramfile_open(&simpoints,pip->envv,cp) ;

	if (rs >= 0) {

	    rs = paramfile_fetch(&simpoints,pip->benchname,NULL,
			vbuf,VBUFLEN) ;

	    vlen = rs ;
	    if (rs >= 0)
	        rs = cfdecmfull(vbuf,vlen,ulwp) ;

	    paramfile_close(&simpoints) ;

	} /* end if (opened SIMPOINT DB) */

done:
	return rs ;
}
/* end subroutine (procsimpoint) */



/* LOCAL SUBROUTINES */



static int procargs(pip,pavp,argc,argv)
struct proginfo	*pip ;
struct pav	*pavp ;
int		argc ;
char		*argv[] ;
{
	ULONG	ullw ;

	int	rs = 0, i ;
	int	argr, argl, npa, kwi, ai, ai_max ;
	int	aol, akl, avl ;
	int	argvalue = -1 ;
	int	cl ;
	int	f_makedate = FALSE ;
	int	f_extra = FALSE ;
	int	f_optplus, f_optminus, f_optequal ;
	int	f_n = FALSE ;
	int	f_skip = FALSE ;
	int	f_ninstr = FALSE ;

	char	argpresent[MAXARGGROUPS] ;
	char	*aop, *akp, *avp ;
	char	*argp ;


	pavp->ex = EX_INFO ;

/* start parsing the arguments */

	for (ai = 0 ; ai < MAXARGGROUPS ; ai += 1) 
		argpresent[ai] = 0 ;

	npa = 0 ;			/* number of positional so far */
	ai = 0 ;
	ai_max = 0 ;
	argr = argc - 1 ;
	while ((rs >= 0) && (argr > 0)) {

	    argp = argv[++ai] ;
	    argr -= 1 ;
	    argl = strlen(argp) ;

#if	F_DEBUGS
	eprintf("main/procargs: argp=>%s<\n",argp) ;
#endif

	    f_optminus = (*argp == '-') ;
	    f_optplus = (*argp == '+') ;
	    if ((argl > 1) && (f_optminus || f_optplus)) {

	            if (isdigit(argp[1])) {

	                if ((argl - 1) > 0)
	                    rs = cfdeci((argp + 1),(argl - 1),&argvalue) ;

	            } else {

	                aop = argp + 1 ;
	                aol = argl - 1 ;
	                akp = aop ;
	                f_optequal = FALSE ;
	                if ((avp = strchr(aop,'=')) != NULL) {

	                    akl = avp - aop ;
	                    avp += 1 ;
	                    avl = aop + aol - avp ;
	                    f_optequal = TRUE ;

	                } else {

	                    akl = aol ;
	                    avl = 0 ;

	                }

/* do we have a keyword match or should we assume only key letters ? */

	                if ((kwi = optmatch(argopts,akp,akl)) >= 0) {

	                    switch (kwi) {

	                    case argopt_tmpdname:
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl)
	                                pip->tmpdname = avp ;

	                        } else {

	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                pip->tmpdname = argp ;

	                        }

	                        break ;

/* version */
	                    case argopt_version:
				if (pavp->f_version)
					f_makedate = TRUE ;

	                        pavp->f_version = TRUE ;
	                        break ;

/* verbose mode */
	                    case argopt_verbose:
	                        pip->verboselevel = 1 ;
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl > 0) {

	                                rs = cfdeci(avp,avl,
	                                    &pip->verboselevel) ;

	                            }

	                        }

	                        break ;

/* program root */
	                    case argopt_root:
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl)
	                                pavp->pr = avp ;

	                        } else {

	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                pavp->pr = argp ;

	                        }

	                        break ;

/* log file */
	                    case argopt_logfile:
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl) {

	                                pip->logfile_type = GETFNAME_TYPELOCAL ;
					cl = MIN(MAXPATHLEN,avl) ;
	                                strwcpy(pip->logfname,avp,cl) ;

	                            }

	                        } else {

	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)  {

	                                pip->logfile_type = GETFNAME_TYPELOCAL ;
					cl = MIN(MAXPATHLEN,argl) ;
	                                strwcpy(pip->logfname,argp,cl) ;

	                            }
	                        }

	                        break ;

/* parameter */
	                    case argopt_paramtab:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)  {

	                            pip->paramfile_type = GETFNAME_TYPELOCAL ;
					cl = MIN(MAXPATHLEN,argl) ;
	                            strwcpy(pip->paramfname,argp,cl) ;

	                        }

	                        break ;

/* debug level */
	                    case argopt_debug:
	                        pip->debuglevel = 1 ;
	                        if (f_optequal) {

	                            f_optequal = FALSE ;
	                            if (avl > 0) {

	                                rs = cfdecui(avp,avl, 
						&pip->debuglevel) ;

	                            }
	                        }

	                        break ;

#ifdef	COMMENT

/* debug schedule */
	                    case argopt_ds:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

	                            rs = debugwin_setopt(&pip->dw,
	                                argp,argl) ;

	                        }

	                        break ;

#endif /* COMMENT */

/* benchmark name */
			case argopt_bn:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)
				    rs = sncpy1(pip->benchname,MAXNAMELEN,
					argp) ;

	                        break ;

/* SIMPOINTS filename */
			case argopt_sp:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl)
				    rs = sncpy1(pip->spfname,MAXNAMELEN,
					argp) ;

	                        break ;

/* numbers of instructions (to simulate) */
	                    case argopt_ninstr:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

					f_ninstr = TRUE ;
					rs = procn(pip,argp,argl) ;

	                        }

	                        break ;

	                    case argopt_et:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

	                            pip->etfname = mallocstr(argp) ;

	                        }

	                        break ;

/* set the selection */
	                    case argopt_select:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

	                            rs = proginfo_setselect(pip,
	                                argp,argl) ;

	                        }

	                        break ;

/* skip instructions (fast forward through these) */
	                    case argopt_skip:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

				    if (strcmp(argp,"simpoint") == 0) {

	                                pip->sinstr = VALUESIMPOINT ;

				    } else {

	                                rs = cfdecmfull(argp,argl,&ullw) ;

	                                pip->sinstr = ullw ;

				    }

	                        }

	                        break ;

/* warm-up instructions */
	                    case argopt_winstr:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

	                            rs = cfdecull(argp,argl,&ullw) ;

	                            pip->winstr = ullw ;

	                        }

	                        break ;

/* log interval instructions */
	                    case argopt_linstr:
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                        argp = argv[++ai] ;
	                        argr -= 1 ;
	                        argl = strlen(argp) ;

	                        if (argl) {

	                            rs = cfdecull(argp,argl,&ullw) ;

	                            pip->linstr = ullw ;

	                        }

	                        break ;

/* handle all keyword defaults */
	                    default:
				rs = SR_INVALID ;
				pavp->ex = EX_USAGE ;
				pavp->f_usage = TRUE ;
	                        bprintf(pip->efp,
					"%s: option (%s) not supported\n",
	                            pip->progname,akp) ;

	                    } /* end switch */

	                } else {

	                    while (akl--) {

	                        switch (*akp) {

/* debug */
	                        case 'D':
	                            pip->debuglevel = 1 ;
	                            if (f_optequal) {

	                                f_optequal = FALSE ;
	                                if (avl > 0) {

	                                    rs = cfdecui(avp,avl,
	                                        &pip->debuglevel) ;

	                                }

	                            }

	                            break ;

/* version */
	                        case 'V':
	                            pavp->f_version = TRUE ;
	                            break ;

/* maximum number of clocks */
	                        case 'c':
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                rs = cfdecull(argp,argl,&ullw) ;

	                                pip->mclock = ullw ;

	                            }

	                            break ;

/* optional jobname */
	                        case 'j':
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                    cl = MIN(argl,JOBNAMELEN) ;
	                                strwcpy(pip->jobname,argp,cl) ;

	                            }

	                            break ;

#ifdef	COMMENT

/* program mode */
	                        case 'm':
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl)
	                                progmodestr = argp ;

	                            break ;

#endif /* COMMENT */

/* execute only a certain number of instructions */
	                        case 'n':
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

					f_n = TRUE ;
					rs = procn(pip,argp,argl) ;

	                            }

	                            break ;

/* options */
	                        case 'o':
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                rs = proginfo_setopt(pip,
	                                    argp,argl) ;

	                            }

	                            break ;

#ifdef	COMMENT

/* parameter file name */
	                        case 'p':
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                pip->paramfile_type = 
						GETFNAME_TYPELOCAL ;
	                                strwcpy(paramfname,argp,argl) ;

	                            }

	                            break ;

#endif /* COMMENT */

/* quiet mode */
	                        case 'q':
	                            pip->f.quiet = TRUE ;
	                            break ;

/* select some instructions */
	                        case 's':
	                            if (argr <= 0) {
	                                rs = SR_INVALID ;
					break ;
				    }

	                            argp = argv[++ai] ;
	                            argr -= 1 ;
	                            argl = strlen(argp) ;

	                            if (argl) {

	                                rs = proginfo_setselect(pip,
	                                    argp,argl) ;

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

	                                }

	                            }

	                            break ;

	                        case '?':
	                            pavp->f_usage = TRUE ;
	                            break ;

	                        default:
				    rs = SR_INVALID ;
	                            pavp->f_usage = TRUE ;
	                            bprintf(pip->efp,
					"%s: unknown option - %c\n",
	                                pip->progname,*aop) ;

	                        } /* end switch */

	                        akp += 1 ;
				if (rs < 0)
					break ;

	                    } /* end while */

	                } /* end if (individual option key letters) */

	            } /* end if (digits as argument or not) */

	        } else {

/* we have a plus or minux sign character alone on the command line */

	            if (ai < MAXARGINDEX) {

	                BSET(argpresent,ai) ;
	                ai_max = ai ;
	                npa += 1 ;	/* increment position count */

	            }

	        } /* end if */

	} /* end while (all command line argument processing) */

	if (rs < 0)
		goto badarg ;


#ifdef	COMMENT
	if (f_makedate) {

	}
#endif

#if	F_DEBUGS
	eprintf("procargs: ret ex=%u\n",pavp->ex) ;
#endif

	pavp->ex = EX_OK ;

ret0:
	return pavp->ex ;

/* bad stuff */
badarg:
	pavp->ex = EX_USAGE ;
	bprintf(pip->efp,"%s: bad argument was specified (%d)\n",
		pip->progname,rs) ;

	goto ret0 ;

}
/* end subroutine (procargs) */


/* stats signal handler */
static void
signal_sim_stats(int sigtype)
{
  sim_dump_stats = TRUE;
}


/* exit signal handler */
static void
signal_exit_now(int sigtype)
{


  sim_exit_now = TRUE;
}


static int
orphan_fn(int i, int argc, char **argv)
{
  exec_index = i;
  return /* done */FALSE;
}


static void
banner(FILE *fd, int argc, char **argv)
{
  char *s;


  fprintf(fd,
	  "%s: SimpleScalar/%s Tool Set version %d.%d of %s.\n"
          "Copyright (C) 2000-2001 by The Regents of The University of Michigan.\n"
          "Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.\n"
	  "This version of SimpleScalar is licensed for academic non-commercial use only.\n"
	  "\n",
	  ((s = strrchr(argv[0], '/')) ? s+1 : argv[0]),
	  VER_TARGET, VER_MAJOR, VER_MINOR, VER_UPDATE);
}


static void
usage(FILE *fd, int argc, char **argv)
{


  fprintf(fd, "Usage: %s {-options} executable {arguments}\n", argv[0]);
  opt_print_help(sim_odb, fd);
}


/* print all simulator stats */
void
sim_print_stats(FILE *fd)		/* output stream */
{
#if 0 /* not portable... :-( */
  extern char etext, *sbrk(int);
#endif

  if (!running)
    return;

  /* get stats time */
  sim_end_time = time((time_t *)NULL);
  sim_elapsed_time = MAX(sim_end_time - sim_start_time, 1);

#if 0 /* not portable... :-( */
  /* compute simulator memory usage */
  sim_mem_usage = (sbrk(0) - &etext) / 1024;
#endif

  /* print simulation stats */
  fprintf(fd, "\nsim: ** simulation statistics **\n");
  stat_print_stats(sim_sdb, fd);
  sim_aux_stats(fd);
  fprintf(fd, "\n");
}


/* print stats, uninitialize simulator components, and exit w/ exitcode */
static void
exit_now(int exit_code)
{
	struct proginfo	*pip = &pi ;


#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(4))
	eprintf("exit_now: entered, exitcode=%u\n",exit_code) ;
#endif

  /* print simulation stats */
  sim_print_stats(stderr);

#if	CF_MASTERDEBUG && F_DEBUG
	if (DEBUGLEVEL(3))
	eprintf("exit_now: sim_uninit() \n") ;
#endif

  /* un-initialize the simulator */
  sim_uninit();

	if (pip->ofp != NULL)
		bclose(pip->ofp) ;

	if (pip->f.log)
		logfile_close(&pip->lh) ;

	proginfo_free(pip) ;

	if (pip->efp != NULL)
		bclose(pip->efp) ;

  /* all done! */
  exit(exit_code);

}
/* end subroutine (exit_now) */


static int procn(pip,a,alen)
struct proginfo	*pip ;
const char	a[] ;
int		alen ;
{
	ULONG	ulw ;

	int	rs = SR_OK, sl, cl ;

	char	*tp, *sp, *cp ;


	if ((a == NULL) || 
		(alen <= 0))
		return SR_OK ;

	if (alen < 0)
		alen = strlen(a) ;

	sp = (char *) a ;
	sl = alen ;

	if ((tp = strnchr(sp,sl,':')) != NULL) {

		cp = sp ;
		cl = tp - sp ;
		if ((strncmp(cp,"simpoint",cl) == 0) && (cl == 8)) {

			pip->sinstr = VALUESIMPOINT ;

		} else {

			rs = cfdecmfull(sp,cl,&ulw) ;

			pip->sinstr = ulw ;

		}

		sp += (cl + 1) ;
		sl -= (cl + 1) ;

	} /* end if (had another value) */

	if (rs >= 0) {

		rs = cfdecmfull(sp,sl,&ulw) ;

		pip->ninstr = ulw ;

	}

	return rs ;
}
/* end subroutine (procn) */



