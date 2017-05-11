/* ss */


/* revision history :

	= 01/08/06, Dave Morano

	This is needed for skipping instrcutions when using LevoSim.


*/



#ifndef	SS_INCLUDE
#define	SS_INCLUDE	1



/* object define */

#define	SS		struct ss



#include	<sys/types.h>

#include	<bio.h>

#include	"localmisc.h"
#include	"defs.h"
#include	"ssas.h"
#include	"regs.h"			/* this is from S-S */
#include	"ssinfo.h"
#include	"ssmachstate.h"




struct ssdatastats {
	ULONG	in ;
	ULONG	mem ;
	ULONG	memmulti ;
	ULONG	memreads, memwrites ;
	ULONG	memunaligned ;
	ULONG	cf ;			/* control flow */
	ULONG	cfind ;			/* control flow (indirect) */
	ULONG	cfsub ;			/* control flow (subroutine) */
	ULONG	cfcond ;		/* control flow (conditional) */
	ULONG	weird ;			/* weird */
} ;

struct ss {
	ULONG			clock ;
	bfile			tfile ;
	struct ssinfo		i ;
	struct ssdatastats	s ;
	struct ssmachstate	ms ;
	int			phase ;
} ;



#endif /* SS_INCLUDE */



