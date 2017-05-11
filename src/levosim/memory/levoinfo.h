/* levoinfo */

/* general levo information */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


*/




#ifndef	LEVOINFO_INCLUDE
#define	LEVOINFO_INCLUDE	1



/* default Levo machine parameters */

#define	LEVOINFO_NSGROWS	16	/* machine height in groups */
#define	LEVOINFO_NSGCOLS	8	/* machine columns (groups) */
#define	LEVOINFO_NFSPAN		8	/* forwarding span (groups) */
#define	LEVOINFO_NASRPSG	2	/* default AS height per SG */
#define	LEVOINFO_NASWPSG	2	/* AS width per SG */
#define	LEVOINFO_NPEPSG		1	/* PEs per SG */
#define	LEVOINFO_NPRPAS		40	/* number of predicate regs/AS */
#define	LEVOINFO_NPATHS		5	/* total number of execution paths */
#define	LEVOINFO_MEMINTER	0x0c	/* default memory interleave pattern */
#define	LEVOINFO_IFETCHWIDTH	32	/* I-fetch bus with in 32-bit words */
#define	LEVOINFO_BTRBSIZE	2048	/* default Levo Branch Tracking */
#define	LEVOINFO_NLOADBUFS	1	/* number of load buffers */
#define	LEVOINFO_MODULUS	256	/* default modulus for sequencing */



/* some Levo machine-wide defines */

#define	LEVOINFO_PATHML		0
#define	LEVOINFO_PATHDEE1	1
#define	LEVOINFO_PATHDEE2	2
#define	LEVOINFO_PATHDEE3	3
#define	LEVOINFO_PATHDEE4	4
#define	LEVOINFO_PATHDEE5	5
#define	LEVOINFO_PATHDEE6	6
#define	LEVOINFO_PATHDEE7	7
#define	LEVOINFO_PATHDEE8	8


/* derived from above */

#define	LEVOINFO_NASCPSG	LEVOINFO_NASWPSG



#include	<sys/types.h>

#include	<bio.h>

#include	"misc.h"		/* for unsigned int (uint) */
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"

#include	"bus.h"			/* memory data buses */
#include	"rfbus.h"		/* registered flow bus for memory */
#include	"libus.h"		/* Levo I-bus for i-responses */
#include	"llb.h"			/* Levo i-Load Buffer */
#include	"lbtrb.h"		/* Levo Branch Tracking Buffer */




struct levoinfo {
	LSIM	*mip ;

	RFBUS	*mfbuses ;	/* memory forwarding (store) buses */
	RFBUS	*mbbuses ;	/* memory backwarding buses (loads) */
	RFBUS	*mwbuses ;	/* memory commit write buses */

	RFBUS	*ifbp ;		/* Levo Instruction Fetch Bus */
	LIBUS	*irbp ;		/* Levo I-Response Bus */

	uint	nsgrows ;		/* number of SG rows  */
	uint	nsgcols ;		/* number of SG columns */
	uint	nasrpsg ;		/* number of ASs high per SG */
	uint	nrfspan ;		/* register forwarding bus SG span */
	uint	nrbspan ;		/* register backwarding bus SG span */
	uint	npfspan ;		/* predicate forwarding bus SG span */
	uint	nmfspan ;		/* memory forwarding bus SG span */
	uint	nmbspan ;		/* memory backwarding bus SG span */
	uint	rfmod ;		/* register forwarding modulus */
	uint	rbmod ;		/* register backwarding modulus */
	uint	pfmod ;		/* predicate forwarding modulus */
	uint	ifetchwidth ;		/* I-fetch bus width in 32-bit words */
	uint	meminter ;		/* memory interleave */
	uint	mfinter ;		/* MFB interleave schedule */
	uint	mbinter ;		/* MBB interleave schedule */
	uint	mwinter ;		/* MWB interleave schedule */
	uint	wmfinter ;		/* window MFB interleave schedule */
	uint	wmbinter ;		/* window MBB interleave schedule */
	uint	nprpas ;		/* number predicate regs/AS */
	uint	btrbsize ;	/* Levo Branch Tracking Buffer size */
	uint	nloadbufs ;	/* number of Levo Load Buffers */

/* derived numbers */

	int	totalrows ;		/* total AS rows */
	int	totalcols ;		/* total AS columns */
	int	npaths ;		/* number of paths */
	int	nsg ;			/* total number of SGes */
	int	logrows ;		/* log of SG rows */
	int	nsgbuses ;		/* total number of SG buses */
	int	nmembuses ;		/* default number of memory buses */
	int	nmfbuses ;		/* total number MFBes */
	int	nmbbuses ;		/* total number MBBes */
	int	nmwbuses ;		/* total number MWBes */

/* extra things that need to be passed widely */

	LLB	*llbp ;			/* Levo I-Load Buffer */
	LBTRB	*lbtrbp ;		/* Levo Branch Tracking Buffer */

	bfile	*btfp ;			/* bus trace file ? */
	bfile	*mtfp ;			/* master bus trace file ? */

	int	(*machineshift)() ;	/* machine shift ! */
} ;




#endif /* LEVOINFO_INCLUDE */



