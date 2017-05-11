/* config */

/* last modified %G% version %I% */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/




#ifndef	CONFIG_INCLUDE
#define	CONFIG_INCLUDE	1



#define	VERSION		"0"
#define	WHATINFO	"@(#)SIMLEVO "

#define	PROGRAMROOTVAR1	"SIMLEVO_PROGRAMROOT"
#define	PROGRAMROOTVAR2	"LOCAL"
#define	PROGRAMROOTVAR3	"PROGRAMROOT"

#ifndef	PROGRAMROOT
#define	PROGRAMROOT	"/usr/add-on/local"
#endif

#define	CONFIGVAR	"SIMLEVO_CONF"
#define	PARAMVAR	"SIMLEVO_PARAM"
#define	JOBNAMEVAR	"SIMLEVO_JOBNAME"
#define	MODEVAR		"SIMLEVO_MODE"
#define	ETFNAMEVAR	"SIMLEVO_EXECTRACE"

#define	SEARCHNAME	"levosim"

#define	CONFIGFNAME	"conf"
#define	PARAMFNAME	"param"
#define	ENVFNAME	"environ"
#define	PATHSFNAME	"paths"
#define	LOGFNAME	"log/levosim"		/* activity log */

#define	LOGINFNAME	"/etc/default/login"
#define	INITFNAME	"/etc/default/init"

#define	TMPDIR		"/tmp"
#define	WORKDIR		"/tmp"

#define	DEFPATH		"/bin:/usr/openwin/bin:/usr/dt/bin"

#define	LOGLEN		(80*1024)
#define	LOGINTERVAL	(30 * 60)		/* for mark time */

#define	BANNER		"Levo Simulator (SIMLEVO)"

#define	PROG_SIMLEVO	"levosim.s5"		/* must be an ELF file */

#define	PROGMAGIC	0x45261818

#define	DEBUGFDVAR1	"SIMLEVO_DEBUGFD"
#define	DEBUGFDVAR2	"DEBUGFD"



#endif /* CONFIG_INCLUDE */



