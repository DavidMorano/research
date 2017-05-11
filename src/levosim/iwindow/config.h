/* config */

/* last modified %G% version %I% */


/* revision history:

	= 2000-02-15, Dave Morano

	This code was started.


*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */


#ifndef	CONFIG_INCLUDE
#define	CONFIG_INCLUDE	1


#define	VERSION		"0"
#define	WHATINFO	"@(#)LEVOSIM "
#define	BANNER		"Levo Simulator (LEVOSIM)"
#define	VARPRNAME	"LEVO"
#define	SEARCHNAME	"levosim"

#ifndef	PROGRAMROOT
#define	PROGRAMROOT	"/usr/add-on/local"
#endif

#define	PROGRAMROOTVAR1	"LEVOSIM_PROGRAMROOT"
#define	PROGRAMROOTVAR2	VARPRNAME
#define	PROGRAMROOTVAR3	"PROGRAMROOT"

#define	DEBUGFDVAR1	"LEVOSIM_DEBUGFD"
#define	DEBUGFDVAR2	"DEBUGFD"

#define	CONFIGVAR	"LEVOSIM_CONF"
#define	PARAMVAR	"LEVOSIM_PARAM"
#define	JOBNAMEVAR	"LEVOSIM_JOBNAME"
#define	PROGMODEVAR	"LEVOSIM_MODE"
#define	ETFNAMEVAR	"LEVOSIM_EXECTRACE"

#define	TMPDIR		"/tmp"
#define	WORKDIR		"/tmp"

#define	CONFIGFNAME	"conf"
#define	PARAMFNAME	"param"
#define	ENVFNAME	"environ"
#define	PATHSFNAME	"paths"
#define	LOGFNAME	"log/levosim"		/* activity log */

#define	LOGINFNAME	"/etc/default/login"
#define	INITFNAME	"/etc/default/init"

#define	DEFPATH		"/bin:/usr/openwin/bin:/usr/dt/bin"

#define	LOGLEN		(80*1024)
#define	LOGINTERVAL	(30 * 60)		/* for mark time */

#define	PROG_LEVOSIM	"levosim.s5"		/* must be an ELF file */

#define	PROGMAGIC	0x45261818


#endif /* CONFIG_INCLUDE */


