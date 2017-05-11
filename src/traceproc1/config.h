/* config */


#define	VERSION		"0"
#define	WHATINFO	"@(#)TRACEPROC "
#define	SEARCHNAME	"traceproc"
#define	BANNER		"Trace Processor (TraceProc)"
#define	VARPRNAME	"LEVO"

#ifndef	PROGRAMROOT
#define	PROGRAMROOT	"/usr/add-on/local"
#endif

#define	VARPROGRAMROOT1	"TRACEPROC_PROGRAMROOT"
#define	VARPROGRAMROOT2	VARPRNAME
#define	VARPROGRAMROOT3	"PROGRAMROOT"

#define	VARBANNER	"TRACEPROC_BANNER"
#define	VARSEARCHNAME	"TRACEPROC_NAME"
#define	PROGMODEVAR	"TRACEPROC_MODE"
#define	VAROPTS		"TRACEPROC_OPTS"
#define	NAMEVAR1	"TRACEPROC_NAME"
#define	NAMEVAR2	"TRACEDUMP_NAME"

#define	VARDEBUGFD1	"TRACEPROC_DEBUGFD"
#define	VARDEBUGFD2	"DEBUGFD"
#define	VARDEBUGFD3	"ERROR_FD"

#define	LOGFNAME	"log/traceproc"
#define	FILTERFNAME	"filtercalls"
#define	ENVFNAME	"env"

#define	LOGSIZE		(80*1024)
#define	LOGINTERVAL	(30 * 60)		/* for mark time */


