/* xmlinfo */


/* revision history :

	= 01/08/06, Dave Morano

	This is needed for skipping instrcutions when using LevoSim.


*/



#ifndef	XMLINFO_INCLUDE
#define	XMLINFO_INCLUDE	1



#define	XMLINFO		struct xmlinfo




#include	<sys/types.h>

#include	<bio.h>

#include	"misc.h"




struct xmlinfo_flags {
	uint	enabled : 1 ;		/* master switch */
	uint	sync : 1 ;		/* do a synchronization dump */
} ;

struct xmlinfo {
	unsigned long	magic ;
	struct xmlinfo_flags	f ;
	bfile		xmlfile, ifile ;
	ULONG		start ;		/* starting clock */
	ULONG		end ;		/* ending clock */
	ULONG		n ;		/* number of clocks to dump */
	ULONG		lastsync ;	/* clock number of last SYNC */
	char		*ifname ;
	uint		sync ;		/* number of clocks between syncs */
} ;



#if	(! defined(XMLINFO_MASTER)) || (XMLINFO_MASTER == 0)

extern int xmlinfo_init(XMLINFO *,char *,const char *) ;
extern int xmlinfo_on(XMLINFO *) ;
extern int xmlinfo_enabled(XMLINFO *,ULONG) ;
extern int xmlinfo_flush(XMLINFO *) ;
extern int xmlinfo_free(XMLINFO *) ;


/* some useful macros for inlining purposes */

#define	xmlinfo_fastenabled(op)	((op)->f.enabled)
#define	xmlinfo_sync(op)	((op)->f.sync)


#endif /* (! defined(XMLINFO_MASTER)) || (XMLINFO_MASTER == 0) */


#endif /* XMLINFO_INCLUDE */



