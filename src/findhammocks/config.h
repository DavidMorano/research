/* config */


#define	VERSION		"0"
#define	WHATINFO	"@(#)findhammocks "
#define	BANNER		"Find Hammocks"

#define	VARPROGRAMROOT1	"FINDHAMMOCKS_PROGRAMROOT"
#define	VARPROGRAMROOT2	"LEVO"
#define	VARPROGRAMROOT3	"PROGRAMROOT"

#define	VARNAME1	"FINDHAMMOCKS_NAME"
#define	VARNAME2	"TRACEDUMP_NAME"

#define	VARDEBUGFD1	"FINDHAMMOCKS_DEBUGFD"
#define	VARDEBUGFD2	"DEBUGFD"
#define	VARDEBUGFD3	"ERROR_FD"

#ifndef	PROGRAMROOT
#define	PROGRAMROOT	"/usr/add-on/local"
#endif

#define	SEARCHNAME	"findhammocks"

#define	TMPDNAME	"/tmp"

#define	ENVFNAME	"env"
#define	LOGFNAME	"log/findhammocks"

#define	LOGSIZE		(80*1024)
#define	LOGINTERVAL	(30 * 60)		/* for mark time */



