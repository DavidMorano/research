/* ssnames */

/* various names */


#define	CF_DEBUG	0		/* switched debugging */
#define	CF_SAFE		1		/* safe mode */


/* revision history:

	= 00/02/15, Dave Morano

	This code was started.


	= 00/09/18, Dave Morano

	I added the LBTRB object.


	= 00/09/28, Dave Morano

	Unfortunately much of this code with regard to initialization
	has to change !  The memory buses that are created and
	initialized in the LEVO object no longer are the same buses
	that go to the ASes.  This was expected anyway eventually so it
	may as well happen now.  New memory buses are created and
	initialized within the SSIW for use by the ASes.  Also all
	register and predicate buses are not entirely local to the LT
	also.  The LEVO object creates memory buses for use between
	the memory subsystem and the i-window.  Those buses are now
	made out of RFBUS objects while the memory buses that go
	to the ASes (here in this code) are still made out of BUS
	objects.


	= 00/12/30, Dave Morano

	Alireza found one problem with mutliple Sharing Groups not
	working.  This code was not calling the '_comb()' or '_clock()'
	methods of all of the LPE object !  Only the first LPE object
	was being handled correctly.  I fixed it simply with a loop to
	get all of them that are configured.


	= 01/02/20, Dave Morano

	I am just marking this version as the latest.  I fixed some
	REGFILTER manipulation issues that had to do with invalidation
	of register values and I put some extra debug prints in this
	version.


	= 03/07/02, Dave Morano

	I'm just getting around to putting a comment here.
	I brought this over a few months ago but am just now
	doing detailed hacks to try to make it work in the S-S
	framework.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	Levo Instruction Window

	This is the top of the i-window for the Levo machine.


	Design notes:

	The indication of the shift to all of the machine will occur in
	phase 2.  Is this OK ??


******************************************************************************/


#define	SSNAMES_MASTER	1


#include	<sys/types.h>
#include	<sys/mman.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<bio.h>
#include	<paramfile.h>

#include	"localmisc.h"
#include	"ssconfig.h"
#include	"defs.h"

#include	"ss.h"
#include	"ssinfo.h"
#include	"ssreg.h"
#include	"sslsq.h"
#include	"ssas.h"
#include	"sspe.h"
#include	"ssiw.h"
#include	"opclass.h"

#include	"regs.h"



/* local defines */



/* external subroutines */

extern int	mkpath2(char *,const char *,const char *) ;
extern int	mkfnamesuf(char *,const char *,const char *) ;


/* forward references */


/* local variables */

const char	*ssfunames[] = {
	"null",
	"ialu",
	"imult",
	"idiv",
	"fadd",
	"fcmp",
	"fcvt",
	"fmult",
	"fdiv",
	"fsqrt",
	"ld",
	"st",
	NULL
} ;



