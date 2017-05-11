/* ssinfo */

/* general levo information */


/* revision history:

	= 00/02/15, Dave Morano

	This code was started.


*/


/******************************************************************************

	The structure presented in this file contains the information
	about the simulated machine.  This structure does NOT provide
	any information about the checker back-end machine.


******************************************************************************/


#ifndef	SSINFO_INCLUDE
#define	SSINFO_INCLUDE	1



#include	<sys/types.h>
#include	<sys/param.h>

#include	"sscommon.h"

#include	"localmisc.h"
#include	"ssconfig.h"



#define	SSINFO		struct ssinfo


/* default parameters */

#define	SSINFO_FWIDTH		8		/* fetch width */
#define	SSINFO_FQSIZE		256		/* fetch width */
#define	SSINFO_DWIDTH		8		/* decode width */
#define	SSINFO_IWIDTH		8		/* FU issue width */
#define	SSINFO_RWIDTH		8		/* FU result width */
#define	SSINFO_IWSIZE		8		/* issue window size */
#define	SSINFO_FBUSES		8		/* forwarding buses */
#define	SSINFO_MODULUS		(8*1024)	/* default modulus */

/* other things */

#define	SSINFO_NPREDS		(4096 * 16)
#define	SSINFO_MAXPREDTARGETS	4		/* max preds in table */
#define	SSINFO_MAXOPSI		4		/* max input ops */
#define	SSINFO_MAXOPSO		4		/* max output ops */

/* not used but for compatibility */

#define	SSINFO_PATHML		0




struct ssinfo_localexec {
	uchar	*a ;
	int	alen ;
} ;

struct ssinfo {
	ULONG	magic ;
	struct ssinfo_localexec		le ;
	uint	nfu[iclass_overlast] ;	/* # FUs */
	uint	lfu[iclass_overlast] ;	/* latency FUs */
	uint	nbpred ;		/* number of branch predictions */
	uint	fwidth ;		/* fetch width */
	uint	fqsize ;		/* fetch queue size */
	uint	dwidth ;		/* dispatch width */
	uint	iwsize ;		/* issue-window size in ASes */
	uint	iwidth ;		/* FU issue width */
	uint	rwidth ;		/* FU result width (NOT possible) */
	uint	fbuses ;		/* forwarding buses */
	uint	modulus ;		/* default */
	uint	rfmod, rbmod ;		/* FW and BW moduli */
	uint	rfmodmask, rbmodmask ;
	uint	hangcks ;		/* commitment timeout (cks) */
	uint	flushdiv ;		/* for testing (window flush hack) */
	uint	regdenlen ;
	uint	memdenlen ;
	uint	memelemsize ;		/* memory element size (in byyes) */
} ;




extern int	ssinfo_init(struct ssinfo *) ;
extern int	ssinfo_leset(struct ssinfo *,int) ;
extern int	ssinfo_letst(struct ssinfo *,int) ;
extern int	ssinfo_free(struct ssinfo *) ;





#endif /* SSINFO_INCLUDE */



