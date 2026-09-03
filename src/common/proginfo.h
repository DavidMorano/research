/* proginfo HEADER */
/* lang=C++20 (conformance reviewed) */
/* charset=ISO8859-1 */

/* program options */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-02-15, David Morano
	This code was started.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	PROGINFO_INCLUDE
#define	PROGINFO_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<limits.h>
#include	<time.h>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<localmisc.h>

#include	"defs.h"


/* object defines */
#define	PROGINFO	struct proginfo_head

EXTERNC_begin

extern int proginfo_start(PROGINFO *,time_t) ;
extern int proginfo_setselect(PROGINFO *,char *,int) ;
extern int proginfo_setopt(PROGINFO *,char *,int) ;
extern int proginfo_levoconf(PROGINFO *,char *,int) ;
extern int proginfo_progress(PROGINFO *,ulong,ulong) ;
extern int proginfo_tellstart(PROGINFO *,ulong,ulong) ;
extern int proginfo_tellcheck(PROGINFO *,ulong) ;
extern int proginfo_selection(PROGINFO *,ulong,ulong) ;
extern int proginfo_dump(PROGINFO *,ulong) ;
extern int proginfo_finish(PROGINFO *) ;

EXTERNC_end


#endif /* PROGINFO_INCLUDE */


