/* process SUPPORT */
/* lang=C++98 */

/* setup for simulation (w/ MINT and otherwise) */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable */
#define	CF_DEBUG	1		/* switchable debug print-outs */
#define	CF_MACHINE	1		/* create & run the machine ? */
#define	CF_MINT		0		/* so we need MINT ? */

/* revision history :

	= 2000-02-15, Dave Morano
	This subroutine was originally written.  Parts were copied

*/


/*****************************************************************************

  	Name:
	process

	Description:
	This subroutine ('process') is used to setup things before
	calling MINT so that things are findable.

*****************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/resource.h>
#include	<unistd.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<usystem.h>
#include	<vecstr.h>
#include	<field.h>
#include	<mkpathx.h>
#include	<mkfnamex.h>
#include	<strwcpy.h>
#include	<bio.h>
#include	<mallocstuff.h>
#include	<paramfile.h>
#include	<getfname.h">
#include	<timestr.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"
#include	"exectrace.h"		/* execution tracing */
#include	"lsim.h"		/* the simulator */
#include	"fcount.h"

#include	"syscalls.h"
#include	"mipsdis.h"
#include	"statemips.h"
#include	"lmipsregs.h"		/* get the reg defs */
#include	"levo.h"		/* top of machine */
#include	"xmlinfo.h"		/* XML trace information */



/* local defines */

#define	EXECTRACEOPTSLEN	200
#define	XMLTRACEOPTSLEN		200

#ifndef	LINELEN
#define	LINELEN		100
#endif

#ifndef	VBUFLEN
#define	VBUFLEN		100
#endif

#define	FE_SGINM	".sginm"
#define	FE_FCOUNTS	".fcounts"

#ifndef	DEBUGLEVEL
#define	DEBUGLEVEL(n)	(pip->debuglevel >= (n))
#endif



/* external subroutines */

extern int	perm(char *,int,int,int *,int) ;
extern int	simplesim(struct proginfo *,PARAMFILE *,LSIM *,
			struct statemips *,SYSCALLS *,ULONG) ;

extern char	*strbasename(char *) ;


/* local structures */


/* forward references */

static int	findprog(char *,char *,char * const *,char *) ;
static int	process_fcount(struct proginfo *, struct statemips *) ;


/* local variables */

static cchar	*exts[] = {
	"us5",
	"s5",
	"x",
	"elf",
	"s5u",
	"osf",
	NULL
} ;

/* program parameters */
static cchar	*pparams[] = {
	"stdin",			/* target STDIN file */
	"stdout",			/* target STDOUT file */
	"stderr",			/* target STDERR file */
	"prog:maxclocks",		/* maximum number of clocks */
	"itrace",			/* instructions trace */
	"exectrace",			/* execution trace file */
	"exectraceopts",		/* execution trace options */
	"xmltrace",			/* XML state trace file */
	"xmltraceopts",			/* XML state trace options */
	"sctrace",			/* SystemCall Trace file */
	"fcount",			/* Function Counts file */
	"prog:bustrace",		/* bus-trace file */
	"prog:statistics",		/* statistics file */
	"prog:sgifstype",		/* SGI FS name */
	"prog:sgidev",			/* SGI FS device number */
	"prog:sgifsinfo",		/* file for SGI FS information */
	NULL
} ;

enum pparams {
	pparam_stdin,
	pparam_stdout,
	pparam_stderr,
	pparam_maxclocks,
	pparam_itrace,
	pparam_exectrace,
	pparam_exectraceopts,
	pparam_xmltrace,
	pparam_xmltraceopts,
	pparam_sctrace,
	pparam_fcount,
	pparam_bustrace,
	pparam_statistics,
	pparam_sgifstype,
	pparam_sgidev,
	pparam_sgifsinfo,
	pparam_overlast
} ;







int process(pip,targetname,pfp,exportv,maxclocks,skipinstr)
struct proginfo	*pip ;
char		targetname[] ;
PARAMFILE	*pfp ;
vecstr		*exportv ;
ULONG		maxclocks ;
ULONG		skipinstr ;
{
	LSIM	oursim, *mip = &oursim ;

	vecstr	progargs, progenv ;

	struct statemips	sm ;

	FIELD	fsb ;

	SYSCALLS	sc ;

	LEVO	ourmachine ;

	bfile	statfile ;

	ULONG	clocks ;

	time_t	t_start, t_end, t_daytime ;
	time_t	t_elapsed ;

	uint	uiw ;
	uint	addrstack, addrentry ;

	int	rs, rs1, i, sl, vl ;
	int	progargc ;
	int	len, etlen, xtlen ;
	int	f_progenv = FALSE ;
	int	f_progargs = FALSE ;
	int	f_etfname = FALSE ;

	cchar	**progargv, **progenvv ;

	char	exectraceopts[EXECTRACEOPTSLEN + 1] ;
	char	xmltraceopts[XMLTRACEOPTSLEN + 1] ;
	char	targetbuf[JOBNAMELEN + 1] ;
	char	tmpfname[MAXPATHLEN + 1] ;
	char	argbuf[MAXPATHLEN + 1] ;
	char	vbuf[VBUFLEN + 1] ;
	char	*stdfiles[3] ;
	char	*fifname = NULL ;
	char	*sp ;
	char	*cp, *cp2 ;
	char	*op ;


#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("process: entered\n") ;
#endif

	for (i = 0 ; i < 3 ; i += 1)
	    stdfiles[i] = NULL ;

	etlen = 0 ;
	xtlen = 0 ;

	exectraceopts[etlen] = '\0' ;
	xmltraceopts[xtlen] = '\0' ;

/* the (fake) SGI FS information */

	pip->sgidev = 0 ;
	(void) memset(pip->sgifstype,0,SGIFSSIZE) ;


/* read some stuff */

	if ((rs = vecstr_init(&progargs,10,VECSTR_PORDERED)) < 0)
	    goto ret0 ;

	if ((rs = vecstr_init(&progenv,10,VECSTR_PORDERED)) < 0)
	    goto ret0a ;



/* the simulated program executable and its arguments */

	vl = paramfile_fetch(pfp,"program",NULL,vbuf,VBUFLEN) ;

	if (vl > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: program=%w\n",vbuf,vl) ;
#endif

	    pip->execfname = mallocstrn(vbuf,vl) ;

	} else {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: no simulated program file specifed \n") ;
#endif

	    rs = SR_INVALID ;
	    goto ret2 ;
	}


/* do we have arguments in a file ? */

	f_progargs = FALSE ;
	if ((vl = paramfile_fetch(pfp,"argsfile",NULL,vbuf,VBUFLEN)) > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: argsfile=%s\n",vbuf) ;
#endif

	    rs = procargs(pip->programroot,vbuf,&progargs) ;

	    if (rs < 0)
	        bprintf(pip->efp,
	            "%s: error in argument file (%d) -- continuing\n",
	            pip->progname,rs) ;

	    f_progargs = (rs >= 0) ;

	} /* end if (getting arguments file) */


/* the simulated program arguments from the parameter file */

	if ((! f_progargs) &&
	    ((vl = paramfile_fetch(pfp,"args",NULL,vbuf,VBUFLEN)) > 0)) {

	    field_init(&fsb,vbuf,vl) ;

	    progargc = 0 ;
	    while (field_sharg(&fsb,NULL,argbuf,MAXPATHLEN) >= 0) {

	        if (progargc > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel >= 4)
	                eprintf("process: progargs arg=%W\n",argbuf,fsb.flen) ;
#endif

	        } /* end if */

	        vecstr_add(&progargs,argbuf,fsb.flen) ;

	        progargc += 1 ;

	    } /* end while */

	    field_free(&fsb) ;

	} /* end if (target program arguments) */


/* what about a special environment file for those "special DBX" cases ? */

	f_progenv = FALSE ;
	vl = paramfile_fetch(pfp,"env",NULL,vbuf,VBUFLEN) ;

	if (vl > 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: getting special environment, file=%s\n",op) ;
#endif

	    rs = procenv(pip->programroot,vbuf,&progenv) ;

	    if (rs < 0)
	        bprintf(pip->efp,
	            "%s: error in environment file (%d) -- continuing\n",
	            pip->progname,rs) ;

	    f_progenv = (rs >= 0) ;

	} /* end if (getting environment file) */


/* OK, now get some program options that we need at this point */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("process: getting other program options\n") ;
#endif

	for (i = 0 ; pparams[i] != NULL ; i += 1) {

	    vl = paramfile_fetch(pfp,pparams[i],NULL,vbuf,VBUFLEN) ;

	    if (vl >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("process: fetch=>%s<\n",vbuf) ;
#endif

	        switch (i) {

	        case pparam_maxclocks:
	            if (cfnumui(vbuf,vl,&uiw) >= 0) {

	                if (maxclocks == 0)
	                    maxclocks = uiw ;

	            } /* end if */

	            break ;

	        case pparam_sgidev:
	            if (cfnumui(vbuf,vl,&uiw) >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	                if (pip->debuglevel >= 4)
	                    eprintf("process: sgidev=%08x\n",uiw) ;
#endif

	                pip->sgidev = uiw ;

	            } /* end if */

	            break ;

	        case pparam_stdin:
	        case pparam_stdout:
	        case pparam_stderr:
	        case pparam_itrace:
	        case pparam_exectrace:
	        case pparam_exectraceopts:
	        case pparam_xmltrace:
	        case pparam_xmltraceopts:
	        case pparam_sctrace:
	        case pparam_fcount:
	        case pparam_bustrace:
	        case pparam_statistics:
	        case pparam_sgifstype:
	        case pparam_sgifsinfo:
	            switch (i) {

	            case pparam_itrace:
	                if ((vbuf[0] != '\0') && (pip->itfname == NULL))
	                    pip->itfname = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_bustrace:
	                break ;

	            case pparam_statistics:
	                if ((vbuf[0] != '\0') && (pip->statfname == NULL))
	                    pip->statfname = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_exectrace:
	                if ((vbuf[0] != '\0') && (pip->etfname == NULL)) {

	                    f_etfname = TRUE ;
	                    pip->etfname = mallocstrn(vbuf,vl) ;

	                }

	                break ;

	            case pparam_exectraceopts:
	                if ((vbuf[0] != '\0') &&
	                    ((EXECTRACEOPTSLEN - etlen) > 1)) {

	                    if (etlen > 0)
	                        *(exectraceopts + etlen--) = ',' ;

	                    cp2 = strwcpy((exectraceopts + etlen),vbuf,
	                        (EXECTRACEOPTSLEN - etlen)) ;

	                    etlen = cp2 - exectraceopts ;
	                }

	                break ;

	            case pparam_xmltrace:
	                if ((vbuf[0] != '\0') && (pip->xmlfname == NULL))
	                    pip->xmlfname = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_xmltraceopts:
	                if ((vbuf[0] != '\0') &&
	                    ((XMLTRACEOPTSLEN - xtlen) > 1)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	                    if (pip->debuglevel >= 4)
	                        eprintf("process: XMLTRACEOPTS >%s<\n",vbuf) ;
#endif

	                    if (xtlen > 0)
	                        *(xmltraceopts + xtlen--) = ',' ;

	                    cp2 = strwcpy((xmltraceopts + xtlen),vbuf,
	                        (XMLTRACEOPTSLEN - xtlen)) ;

	                    xtlen = cp2 - xmltraceopts ;
	                }

	                break ;

	            case pparam_sctrace:
	                if ((vbuf[0] != '\0') && (pip->sctfname == NULL))
	                    pip->sctfname = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_fcount:
	                if ((vbuf[0] != '\0') && (pip->fcfname == NULL))
	                    pip->fcfname = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_stdin:
	                if (vbuf[0] != '\0')
	                    stdfiles[0] = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_stdout:
	                if (vbuf[0] != '\0')
	                    stdfiles[1] = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_stderr:
	                if (vbuf[0] != '\0')
	                    stdfiles[2] = mallocstrn(vbuf,vl) ;

	                break ;

	            case pparam_sgifstype:
	                if (vbuf[0] != '\0')
	                    strncpy(pip->sgifstype,vbuf,SGIFSSIZE) ;

	                break ;

	            case pparam_sgifsinfo:
	                if (vbuf[0] != '\0')
	                    fifname = mallocstrn(vbuf,MIN(vl,MAXPATHLEN)) ;

	                break ;

	            } /* end switch (inner) */

	            break ;

	        } /* end switch (outer) */

	    } /* end if (got a parameter) */

	} /* end for (getting program parameters) */


/* some other initialization for stuff that we didn't get above */

	if (((pip->sgidev == 0) || (pip->sgifstype[0] == '\0')) &&
	    (fifname != NULL)) {

	    bfile	fifile ;


	    if (bopen(&fifile,fifname,"r",0666) >= 0) {

	        char	linebuf[LINELEN + 1] ;


	        len = bgetline(&fifile,linebuf,LINELEN) ;

	        if (pip->sgifstype[0] == '\0') {

	            sl = nextfield(linebuf,len,&sp) ;

	            if (sl > 0)
	                strncpy(pip->sgifstype,sp,SGIFSSIZE) ;

	        }

	        len = bgetline(&fifile,linebuf,LINELEN) ;

	        if (pip->sgidev == 0) {

	            sl = nextfield(linebuf,len,&sp) ;

	            if (cfnumui(sp,sl,&uiw) >= 0)
	                pip->sgidev = uiw ;

	        }

	        bclose(&fifile) ;

	    } /* end if (opened file) */

	} /* end if (needed additional information) */


/* do it, initialization */

	if (targetname == NULL) {

	    targetname = targetbuf ;
	    cp = strbasename(pip->execfname) ;

	    if ((op = strchr(cp,'.')) != NULL)
	        strwcpy(targetbuf,cp,MIN(op - cp,JOBNAMELEN)) ;

	    else
	        strwcpy(targetbuf,cp,JOBNAMELEN) ;

	} /* end if (there was no targetname) */

	if (pip->jobname[0] == '\0')
	    strwcpy(pip->jobname,targetname,JOBNAMELEN) ;

	logfile_printf(&pip->lh,
	    "jobname=%s\n",pip->jobname) ;

	logfile_printf(&pip->lh,
	    "target=%s\n",targetname) ;

	if (DEBUGLEVEL(1)) {

		bprintf(pip->efp,
	    		"%s: jobname=%s\n",
			pip->progname,pip->jobname) ;

		bprintf(pip->efp,
	    		"%s: target=%s\n",
			pip->progname,targetname) ;

	}

/* get program environment */

	if (f_progenv)
	    vecstr_getvec(&progenv,&progenvv) ;

	else
	    vecstr_getvec(exportv,&progenvv) ;


/* get the stuff for 'simlevo' */

	progargc = vecstr_count(&progargs) ;

	vecstr_getvec(&progargs,&progargv) ;


/* pass the information to the simulator */

	{
	    LSIM_SIMPROG	ourprog ;

	    LSIM_MACHOBJ	machmethods ;


	    ourprog.program = pip->execfname ;
	    ourprog.argc = progargc ;
	    ourprog.argv = (char **) progargv ;
	    ourprog.envv = (char **) progenvv ;

	    machmethods.topobjp = &ourmachine ;
	    machmethods.topcombp = (int (*)(void *,int)) levo_comb ;
	    machmethods.topclockp = (int (*)(void *)) levo_clock ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: pip=%08lx lsim_init() \n",
	            pip) ;
#endif

	    rs = lsim_init(mip,pip,pfp,maxclocks,&ourprog,&machmethods) ;

#if	CF_DEBUGS
	    eprintf("process: pip=%08lx lsim_init() rs=%d\n",
	        pip,rs) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: lsim_init() rs=%d\n",rs) ;
#endif

	} /* end block (initialize the simulator support) */

	if (rs < 0)
	    goto ret3 ;


/* initialize the instruction disassembly function */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("process: execfname=%s\n",pip->execfname) ;
#endif

	rs = mipsdis_init(&pip->dis,NULL,pip->execfname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("process: mipsdis_init() rs=%d\n",rs) ;
#endif

	pip->f.instrdis = (rs >= 0) ;


/* do some stuff depending on what mode we are executing in */

	if ((pip->progmode == PROGMODE_SIMPLESIM) ||
	    (pip->progmode == PROGMODE_LEVOSIM)) {

	    int	size ;


	   if (pip->jobname[0] != '\0')
	   	bprintf(pip->ofp,"jobname=%s\n",pip->jobname);

	    bprintf(pip->ofp, "target=%s\n", targetname) ;


/* initialize the architected registers */

	    size = sizeof(struct statemips) ;
	    (void) memset(&sm,0,size) ;


	    rs = lsim_getstack(mip,&addrstack) ;

	    rs = lsim_getentry(mip,&addrentry) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4) {
	        eprintf("process: stack=%08x\n",addrstack) ;
	        eprintf("process: entry=%08x\n",addrentry) ;
	    }
#endif


	    sm.regs[LMIPSREG_SP] = addrstack ;	/* stack pointer */
	    sm.ia = addrentry ;			/* instruction address */


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4) {
	        eprintf("process: exectrace filename=%s\n",pip->etfname) ;
	        eprintf("process: exectrace opts=>%s<\n", exectraceopts) ;
	    }
#endif


	    rs = traceinfo_init(&pip->ti,pip->etfname,exectraceopts,
	        pip->progname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: traceinfo_init() rs=%d\n",rs) ;
#endif

	    if (rs >= 0) {

	        pip->f.exectrace = TRUE ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (DEBUGLEVEL(3)) {
	            eprintf("process: traceopts> "
	                "regs=%d regvals=%d "
	                "mems=%d memvals=%d\n",
	                pip->ti.f.regs,pip->ti.f.regvals,
	                pip->ti.f.mems,pip->ti.f.memvals) ;
	            eprintf("process: traceopts> start=%lld n=%lld\n",
	                pip->ti.sin,pip->ti.n) ;
	        }
#endif /* CF_DEBUG */

/* write out to trace any non-zero initial register values */

	        for (i = 0 ; i < LMIPSREG_NREGS ; i += 1) {

	            if (sm.regs[i] > 0)
	                exectrace_wreg(&pip->ti.et,i,sm.regs[i],15) ;

	        } /* end if (writing non-zero register values) */

/* are we tracing later ? */

	        if (pip->ti.sin > 0)
	            exectrace_win(&pip->ti.et,pip->ti.sin) ;

	    } /* end if (tracing enabled) */

/* initialize the SYSCALL handling object */

	    rs = syscalls_init(&sc,pip,mip,&sm,stdfiles) ;

	    if (rs < 0)
	        goto ret3a ;

/* initialize for function-call counting */

	    if ((pip->fcfname != NULL) && (pip->fcfname[0] != '\0')) {

	        LMAPPROG	*mpp ;

		char		targetbuf[MAXNAMELEN + 1] ;


	        lsim_getpp(mip,&mpp) ;

	    cp = strbasename(pip->execfname) ;

	    if ((op = strchr(cp,'.')) != NULL) {
	        strwcpy(targetbuf,cp,MIN(op - cp,(MAXNAMELEN - 1))) ;

	    } else {
	        strwcpy(targetbuf,cp,(MAXNAMELEN - 1)) ;
	    }

	        mkfname(tmpfname,targetbuf,FE_SGINM) ;

	        rs = fcount_init(&pip->fc,mpp,tmpfname) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(2))
	        eprintf("process: fcount_init() rs=%d\n",rs) ;
#endif

	        if (rs < 0) {

	            bprintf(pip->efp,
	                "%s: could not open the FCOUNT file\n",
	                pip->progname) ;

	            goto ret3b ;
	        }

	        pip->f.fcount = TRUE ;
		bprintf(pip->ofp,"function counting is active\n") ;

		if (DEBUGLEVEL(1))
		    bprintf(pip->efp,"%s: function counting is active\n",
			pip->progname) ;

	    } /* end if (function-call counting) */

	} /* end if (initializing for execution) */


/* is this a SimpleSim run ? */

	if ((pip->progmode == PROGMODE_SIMPLESIM) ||
	    ((pip->progmode == PROGMODE_LEVOSIM) && (skipinstr > 0))) {


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(2))
	        eprintf("process: simplesim() \n") ;
#endif

	    rs = simplesim(pip,pfp,mip,&sm,&sc,skipinstr) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("process: simplesim() rs=%d\n",rs) ;
#endif


	} /* end if (SimpleSim run) */


/* is this a LevoSim run ? */

	if ((pip->progmode == PROGMODE_LEVOSIM) && (rs >= 0)) {

/* XML output wanted ? */

	    if ((pip->xmlfname != NULL) && (pip->xmlfname[0] != '\0')) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("process: xmltraceopts=%s\n",xmltraceopts) ;
#endif

	        rs = xmlinfo_init(&pip->xi,pip->xmlfname,xmltraceopts) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if ((pip->debuglevel >= 2) && (rs < 0))
	            eprintf("process: xmlinfo_init() rs=%d\n",rs) ;
#endif

	        if (rs >= 0)
	            pip->f.xmltrace = TRUE ;

	    } /* end if (XML tracing) */


#if	CF_MACHINE

/* create the machine */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("process: about levo_init() LEVO a=%08lx\n",
	            &ourmachine) ;
#endif

	    rs = levo_init(&ourmachine,pip,pfp,mip,&sm) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        eprintf("process: levo_init() rs=%d\n",rs) ;
#endif

	    if (rs >= 0) {

/* loop */

	        u_time(&t_start) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("process: lsim_loop()\n") ;
#endif

	        rs = lsim_loop(mip) ;

#if	CF_DEBUG
	        if ((pip->debuglevel >= 2) && (rs1 < 0))
	            eprintf("process: lsim_loop() rs=%d\n",rs1) ;
#endif


	        u_time(&t_end) ;

	        t_elapsed = t_end - t_start ;


/* free-up and get out */

	        lsim_getclock(mip,&clocks) ;

	        if ((pip->statfname != NULL) && (pip->statfname[0] != '\0') &&
	            (bopen(&statfile,pip->statfname,"wct",0666) >= 0)) {

	            time_t	daytime ;

	            double	dt, dc, clockrate ;

	            char	timebuf[40] ;


	            pip->sfp = &statfile ;
	            pip->f.statistics = TRUE ;


	            u_time(&daytime) ;


	            bprintf(&statfile,
	                "LevoSim execution results %s\n\n",
	                timestr_log(daytime,timebuf)) ;

	            if (pip->jobname[0] != '\0')
	                bprintf(&statfile,
	                    "jobname                          %s\n",
	                    pip->jobname) ;

	            bprintf(&statfile,
	                "logid                            %s\n",
	                pip->logid) ;

	            bprintf(&statfile,
	                "target program name              %s\n",
	                targetname) ;

	            bprintf(&statfile,
	                "target program file              %s\n",
	                pip->execfname) ;

	            bprintf(&statfile,
	                "clocks                           %lld\n",clocks) ;

	            bprintf(&statfile,
	                "elapsed time                     %s\n",
	                timestr_elapsed(t_elapsed,timebuf)) ;

	            dc = (double) clocks ;
	            dt = (double) t_elapsed ;

	            clockrate = 0.0 ;
	            if (clocks != 0) {

	                dt = dt / 60.0 ;
	                clockrate = dc / dt ;

	            }

	            bprintf(&statfile,
	                "simulation rate                  %8.1f "
	                "clocks/minute\n",
	                clockrate) ;


	            rs1 = levo_statfile(&ourmachine,&statfile) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if ((pip->debuglevel >= 2) && (rs1 < 0))
	                eprintf("process: levo_statfile() rs=%d\n",rs1) ;
#endif

	            pip->f.statistics = FALSE ;
	            pip->sfp = NULL ;

	            bclose(&statfile) ;

	        } /* end if (statistics file) */


	        if (pip->ofp != NULL) {

	            struct rusage	a ;


	            bseek(pip->ofp,0L,SEEK_END) ;

	            if (clocks > 0)
	                bprintf(pip->ofp,"clocks %lld\n",clocks) ;

	            uc_getrusage(RUSAGE_SELF,&a) ;

	            bprintf(pip->ofp,"rss %ld\n",a.ru_maxrss) ;

	        } /* end if (output file) */


	        levo_free(&ourmachine) ;

	    } /* end if */
#endif /* CF_MACHINE */

/* finish off the XML trace if any */

	    if (pip->f.xmltrace) {

	        xmlinfo_free(&pip->xi) ;

	    } /* end if (XML trace) */


	} /* end if (LevoSim run) */



	if ((pip->progmode == PROGMODE_SIMPLESIM) ||
	    (pip->progmode == PROGMODE_LEVOSIM)) {


#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 2)
	        eprintf("process: instructions executed %lld\n",
	            sm.in) ;
#endif

	    if (pip->ofp != NULL) {

	        bprintf(pip->ofp,"instructions executed %lld\n",
	            sm.in) ;

	    }

	    if (pip->debuglevel > 0)
	        bprintf(pip->efp,"%s: instructions executed %lld\n",
	            pip->progname,sm.in) ;


		rs1 = process_fcount(pip,&sm) ;


	} /* end if (LevoSim or SimpleSim) */


/* close out SYSCALL handling */

	{
	    SYSCALLS_STATS	s ;

	    uint	brkmax, heapmax ;


	    rs1 = syscalls_getbrkmax(&sc,&brkmax,&heapmax) ;

	    if (rs1 >= 0) {

	        if (pip->ofp != NULL) {

	            bprintf(pip->ofp,"maximum break address=%08x\n",
	                brkmax) ;

	            bprintf(pip->ofp,"maximum heap size=%08x\n",
	                heapmax) ;

	        }

	        if (pip->debuglevel > 0) {

	            bprintf(pip->efp,"%s: maximum break address=%08x\n",
	                pip->progname,brkmax) ;

	            bprintf(pip->efp,"%s: maximum heap size=%08x\n",
	                pip->progname,heapmax) ;

	        }

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 2) {

	            eprintf("process: maximum break address=%08x\n",
	                brkmax) ;

	            eprintf("process: maximum heap size=%08x\n",
	                heapmax) ;

	        }
#endif /* CF_DEBUG */

	    } /* end if */

	    if (pip->ofp != NULL) {

	        rs1 = syscalls_stats(&sc,&s) ;

	        if (rs1 >= 0) {

	            bprintf(pip->ofp,"SC-open    %10u\n",
	                s.c_callopen) ;

	            bprintf(pip->ofp,"SC-read    %10u\n",
	                s.c_callread) ;

	            bprintf(pip->ofp,"SC-write   %10u\n",
	                s.c_callwrite) ;

	            bprintf(pip->ofp,"SC-close   %10u\n",
	                s.c_callclose) ;

	            bprintf(pip->ofp,"fileopens  %10u\n",
	                s.c_fileopen) ;

	            bprintf(pip->ofp,"filecloses %10u\n",
	                s.c_fileclose) ;

	            bprintf(pip->ofp,"maxopen    %10u\n",
	                s.maxopen) ;

	        }

	    } /* end if */

	} /* end block */


ret4:
	if (pip->f.fcount) {

	    rs1 = fcount_free(&pip->fc) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(2))
	        eprintf("process: fcount_free() rs=%d\n",rs1) ;
#endif

	}

ret3b:
	syscalls_free(&sc) ;


/* close out any traces that we may have */

	if (pip->f.exectrace) {

	    rs1 = traceinfo_free(&pip->ti) ;

	    if ((pip->debuglevel > 0) && (rs1 < 0)) {

		if (rs1 == SR_PIPE)
	        bprintf(pip->efp,
	            "%s: EOT on output trace (%d)\n",
	            pip->progname,rs1) ;

		else
	        bprintf(pip->efp,
	            "%s: failed on output trace (%d)\n",
	            pip->progname,rs1) ;

	    }

	} /* end if (exectrace) */


	if (pip->f.instrdis)
	    mipsdis_free(&pip->dis) ;


/* we're out of here ! */
ret3a:
	(void) lsim_free(mip) ;

ret3:


/* free up any allocated stuff */

	if (pip->sctfname != NULL)
	    free(pip->sctfname) ;

	if (pip->fcfname != NULL)
	    free(pip->fcfname) ;

	if (pip->statfname != NULL)
	    free(pip->statfname) ;

	if (pip->xmlfname != NULL)
	    free(pip->xmlfname) ;

	if ((pip->etfname != NULL) && f_etfname) {

	    free(pip->etfname) ;

	    pip->etfname = NULL ;
	}

	if (pip->execfname != NULL)
	    free(pip->execfname) ;


/* we're out of here */
ret2:
	if (fifname != NULL)
	    free(fifname) ;

ret1:
	vecstr_free(&progenv) ;

ret0a:
	vecstr_free(&progargs) ;

/* try to translate the exit code to a real value */
ret0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs != 0))
	    eprintf("process: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (process) */



/* LOCAL SUBROUTINES */



/* find the executable */
static int findprog(pr,name,exts,result)
char	pr[] ;
char	name[] ;
char	*const exts[] ;
char	result[] ;
{
	int	i, bnlen, sl ;

	char	*bn, *cp ;


	if ((pr == NULL) || (name == NULL))
	    return SR_FAULT ;

	if ((exts == NULL) || (result == NULL))
	    return SR_FAULT ;

	bn = name ;
	bnlen = -1 ;
	if ((cp = strrchr(name,'.')) != NULL)
	    bnlen = cp - name ;

	else
	    bnlen = strlen(name) ;

	for (i = 0 ; exts[i] != NULL ; i += 1) {

	    sl = bufprintf(result,MAXPATHLEN,"%s/bin/%W.%s",
	        pr,bn,bnlen,exts[i]) ;

	    if (perm(result,-1,-1,NULL,X_OK | R_OK) >= 0)
	        break ;

	} /* end for */

	if (exts[i] == NULL)
	    return SR_NOTFOUND ;

	return sl ;
}
/* end subroutine (findprog) */


static int process_fcount(pip,smp)
struct proginfo		*pip ;
struct statemips	*smp ;
{
	char	tmpfname[MAXPATHLEN + 1] ;

	int	rs = SR_OK, i ;
	int	rs1 ;


/* function instruction counts */

	    if (pip->f.fcount) {
	        bfile	tmpfile ;

	        rs = fcount_done(&pip->fc) ;

	        mkfname(tmpfname,pip->jobname,FE_FCOUNTS) ;

	        if ((rs1 = bopen(&tmpfile,tmpfname,"wct",0666)) >= 0) {

	            FCOUNT_ENTRY	*ep ;

	            double	fn, fd, fpercent ;

	            uint	ins, calls ;


	            bprintf(&tmpfile,"%12llu %7.3f%% %08x %08x %s\n",
	                smp->in,100.0,0,0,"*total*") ;

	            fcount_getother(&pip->fc,&ins,&calls) ;

	            fn = (double) ins ;
	            fd = (double) smp->in ;
	            fpercent = (smp->in != 0) ? (fn * 100.0 / fd) : 0.0 ;
	            bprintf(&tmpfile,"%12ld %7.3f%% %08x %08x %s\n",
	                ins,fpercent,0,0,"*other*") ;

	            for (i = 0 ; fcount_get(&pip->fc,i,&ep) >= 0 ; i += 1) {

#if	CF_MASTERDEBUG && CF_DEBUG
	                if (DEBUGLEVEL(4)) {
	                    eprintf("stats: i=%d ep=%p\n",i,ep) ;
	                    if (ep != NULL)
	                        eprintf("stats: ia=%08x ins=%u n=%s\n",
	                            ep->ia,ep->ins,ep->name) ;
	                }
#endif

	                if (ep == NULL)
	                    continue ;

	                fn = (double) ep->ins ;
	                fd = (double) smp->in ;
	                fpercent = (smp->in != 0) ? (fn * 100.0 / fd) : 0.0 ;
	                bprintf(&tmpfile,"%12ld %7.3f%% %08x %08x %s\n",
	                    ep->ins,fpercent,ep->ia,ep->size,ep->name) ;

	            } /* end for */

	            bclose(&tmpfile) ;

	        } /* end if (function instruction counts file) */

	    } /* end if (function instruction counts) */

	return rs ;
}
/* end subroutine (process_fcount) */



