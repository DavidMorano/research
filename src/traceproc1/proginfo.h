/* proginfo */

/* program options */


/* revision history:

	= 2000-02-15, David Morano

	This code was started.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	PROGINFO_INCLUDE
#define	PROGINFO_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<limits.h>
#include	<time.h>

#include	<localmisc.h>

#include	"defs.h"


/* object defines */

#define	PROGINFO		struct proginfo


#if	(! defined(PROGINFO_MASTER)) || (PROGINFO_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int proginfo_start(PROGINFO *,time_t) ;
extern int proginfo_setselect(PROGINFO *,char *,int) ;
extern int proginfo_setopt(PROGINFO *,char *,int) ;
extern int proginfo_levoconf(PROGINFO *,char *,int) ;
extern int proginfo_progress(PROGINFO *,ULONG,ULONG) ;
extern int proginfo_tellstart(PROGINFO *,ULONG,ULONG) ;
extern int proginfo_tellcheck(PROGINFO *,ULONG) ;
extern int proginfo_selection(PROGINFO *,ULONG,ULONG) ;
extern int proginfo_dump(PROGINFO *,ULONG) ;
extern int proginfo_finish(PROGINFO *) ;

#ifdef	__cplusplus
}
#endif

#endif /* PROGINFO_MASTER */

#endif /* PROGINFO_INCLUDE */


