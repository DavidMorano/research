/* ssconfig */

/* last modified %G% version %I% */



/* revision history:

	= 00/02/15, Dave Morano

	This code was started.


*/



#ifndef	SSCONFIG_INCLUDE
#define	SSCONFIG_INCLUDE	1



#define	VERSION		"0"
#define	WHATINFO	"@(#)OPTIFLOW "
#define	BANNER		"OpTiFlow"

#define	VARPROGRAMROOT1	"OPTIFLOW_PROGRAMROOT"
#define	VARPROGRAMROOT2	"LEVO"
#define	VARPROGRAMROOT3	"PROGRAMROOT"

#define	VARERRFILE	"OPTIFLOW_ERRFILE"
#define	VARARGS		"OPTIFLOW_ARGS"
#define	VARBENCHNAME	"OPTIFLOW_BENCHNAME"
#define	VARJOBNAME	"OPTIFLOW_JOBNAME"

#define	VARCONFIG	"OPTIFLOW_CONF"
#define	VARSSCONFIG	"OPTIFLOW_SSCONF"
#define	VARPARAM	"OPTIFLOW_PARAM"
#define	VARSPFNAME	"OPTIFLOW_SIMPOINTS"
#define	VARETFNAME	"OPTIFLOW_EXECTRACE"

#define	VARDEBUGFD1	"OPTIFLOW_DEBUGFD"
#define	VARDEBUGFD2	"DEBUGFD"

#define	SEARCHNAME	"optiflow"

#ifndef	PROGRAMROOT
#define	PROGRAMROOT	"/usr/add-on/local"
#endif

#define	TMPDNAME	"/tmp"
#define	WORKDNAME	"/tmp"

#define	INITFNAME	"/etc/default/init"
#define	LOGINFNAME	"/etc/default/login"

#define	NDEBUGFNAME	"ndebug"
#define	SSCONFIGFNAME	"conf"
#define	PARAMFNAME	"param"
#define	ENVFNAME	"environ"
#define	PATHSFNAME	"paths"
#define	SPFNAME		"simpoints"
#define	LOADDNAME	"lib/bpeval"
#define	LOGFNAME	"log/optiflow"		/* activity log */

#define	DEFPATH		"/bin:/usr/openwin/bin:/usr/dt/bin"

#define	LOGLEN		(80*1024)
#define	LOGINTERVAL	(30 * 60)		/* for mark time */
#define	SYNCINTERVAL	(4 * 60)		/* for synchronization */
#define	LOGINSTR	1000000

#define	PROG_OPTIFLOW	"optiflow.s5"		/* must be an ELF file */

#define	PROGMAGIC	0x45261818

/* SGI FS typename size */

#define	SGIFSSIZE	16


#define	HANGCKS		500		/* commitment timeout (in CKS) */

/* size of some statistics arrays */
#define	IMEMLATLEN	500
#define	DMEMLATLEN	500


#endif /* SSCONFIG_INCLUDE */



