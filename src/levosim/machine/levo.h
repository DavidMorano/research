/* levo HEDER */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* top Levo machine object */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-02-15, Dave Morano
	This code was started.

	= 2001-08-06, Dave Morano
	Added parameter to 'levo_init()' for skipping instructions.

*/

/* Copyright © 2000,2001 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

#ifndef	LEVO_INCLUDE
#define	LEVO_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<bfile.h>

#include	"lsim.h"
#include	"statemips.h"

#include	"levoinfo.h"
#include	"lmem.h"		/* memory subsytem */
#include	"iw.h"			/* instruction window */
#include	"bus.h"			/* buses */
#include	"rfbus.h"		/* buses */
#include	"libus.h"		/* special cheap wide i-fetch bus */
#include	"busmon.h"		/* testing */
#include	"bustest.h"		/* testing */


#define	LEVO		struct levo_head
#define	LEVO_FL		struct levo_flags
#define	LEVO_MAGIC	0x65544332


struct levo_state {
	int	checksum ;
	int	verifier ;
} ;

struct levo_flags {
	uint	exit : 1 ;		/* exit indication */
} ;

struct levo_head {
	proginfo	*pip ;		/* program information */
	LSIM		*mip ;		/* MINT information */
	levoinfo	info ;		/* Levo information */
	levo_state	c, n ;
	levo_flags	f ;
	IW		win ;
	LMEM		memsys ;
	RFBUS		ifb ;		/* i-fetch requests */
	LIBUS		irb ;		/* i-fetch responses */
	BUS		mybus ;		/* testing */
	BUSMON		testmon ;	/* testing */
	BUSTEST		testbus ;	/* testing */
	bfile		btfile ;	/* testing (bus trace) */
	bfile		mtfile ;	/* testing (master trace) */
	uint		magval ;
} ; /* end struct (levo_head) */

typedef	LEVO		levo ;
typedef	LEVO_FL		levo_fl ;

EXTERNC_begin

extern int	levo_init(levo *,proginfo *,
			paramfile *, LSIM *,statemips *) ;
extern int	levo_free(levo *) ;
extern int	levo_comb(levo *,int) ;
extern int	levo_clock(levo *) ;
extern int	levo_statfile(levo *,bfile *) ;

EXTERNC_end


#endif /* LEVO_INCLUDE */


