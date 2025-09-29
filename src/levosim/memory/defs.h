/* defs */



/* revision history:

	= 2000-02-15, Dave Morano

	This code was started.


*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */


#ifndef	DEFS_INCLUDE
#define	DEFS_INCLUDE	1


#include	<sys/types.h>
#include	<netdb.h>

#include	<usystem.h>
#include	<logfile.h>
#include	<vecstr.h>
#include	<bio.h>

#include	"misc.h"
#include	"traceinfo.h"
#include	"fcount.h"
#include	"mipsdis.h"
#include	"syscalls.h"
#include	"xmlinfo.h"
#include	"debugwin.h"



/* compilation feature switches */

#define	XML		1		/* turn ON XML stuff */


/* other stuff */

#define	LINELEN		256		/* size of input line */
#define	BUFLEN		(5 * 1024)
#define	LOGIDLEN	14
#define	JOBNAMELEN	31

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	2048
#endif

#ifndef	MAXNAMELEN
#define	MAXNAMELEN	256
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	40
#endif

#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2

/* program modes */

#define PROGMODE_LEVOSIM	0
#define	PROGMODE_LEVOSIMSIZE	1
#define	PROGMODE_SIMPLESIM	2


/* SGI FS typename size */

#define	SGIFSSIZE		16

#define	PROGINFO	struct proginfo
#define	PROGINFO_FL	struct proginfo_flags

#define	PIVARS		struct pivars

#define	ARGINFO		struct arginfo


struct proginfo_select {
	ULONG	start, end ;
	ULONG	n ;
} ;

struct proginfo_flags {
	uint	fd_stdout:1 ;		/* do we have STDOUT? */
	uint	fd_stderr:1 ;		/* do we have STDERR? */
	uint	log:1 ;			/* do we have a log file? */
	uint	quiet:1 ;		/* are we in quiet mode? */
	uint	params:1 ;		/* we have passed parameters */
	uint	select:1 ;		/* selection state */
	uint	past:1 ;		/* past the selection (if any) */
	uint	exectrace:1 ;		/* do we have an execution trace? */
	uint	xmltrace:1 ;		/* XML state trace */
	uint	sctrace:1 ;		/* system-call trace */
	uint	fcount:1 ;		/* function counts */
	uint	statistics:1 ;		/* do we have a statistics file? */
	uint	instrdis:1 ;		/* instruction disassembly available */
} ;

struct proginfo {
	ULONG		ninstr ;	/* max number of instructions */
	ULONG		cin ;		/* current instruction number */
	unsigned long	magic ;
	struct proginfo_flags	f ;
	struct proginfo_select	ck, in ;
	LOGFILE		lh ;		/* program activity log */
	TRACEINFO	ti ;		/* execution trace information */
	XMLINFO		xi ;		/* XML trace information */
	FCOUNT		fc ;		/* function counts */
	MIPSDIS		dis ;		/* instruction disassembly */
	DEBUGWIN	dw ;		/* debug window thing */
	bfile		*lfp ;		/* system log "Basic" file */
	bfile		*efp ;
	bfile		*ofp ;
	bfile		*btfp ;		/* bus trace */
	bfile		*sfp ;		/* statistics file */
	char	*arg0 ;
	char	**envv ;
	char	*progname ;		/* program name */
	char	*programroot ;		/* program root directory */
	char	*version ;		/* program version string */
	char	*domainname ;
	char	*nodename ;
	char	*username ;		/* ours */
	char	*groupname ;		/* ours */
	char	*pwd ;
	char	*logid ;		/* default program LOGID */
	char	*tmpdir ;		/* temporary directory */
	char	*workdir ;		/* working directory */
	char	*execfname ;		/* execution file name */
	char	*itfname ;		/* instruction trace file */
	char	*etfname ;		/* execution trace file */
	char	*xmlfname ;		/* XML state flow file */
	char	*sctfname ;		/* system-call trace file */
	char	*fcfname ;		/* function-count file */
	char	*statfname ;		/* statistics output file */
	time_t	starttime ;
	time_t	lastlogtime ;		/* some times */
	pid_t	pid ;
	uint	sgidev ;		/* SGI device number */
	int	progmode ;		/* program mode */
	int	debuglevel ;		/* debugging level */
	int	verboselevel ;		/* verbosity level */
	char	jobname[JOBNAMELEN + 1] ;	/* job name */
	char	sgifstype[SGIFSSIZE] ;	/* SGI FS type name (fake) */
} ;


#endif /* DEFS_INCLUDE */


