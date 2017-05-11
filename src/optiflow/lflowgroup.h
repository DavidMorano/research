/* lflowgroup */



#ifndef	LFLOWGROUP_INCLUDE
#define	LFLOWGROUP_INCLUDE	1



/* object defines */

#define	LFLOWGROUP		struct lflowgroup
#define	LFLOWGROUP_DATALANE	struct lflowgroup_datalane



#include	"localmisc.h"
#include	"levoconfig.h"
#include	"defs.h"

#if		F_XML
#include	"xmlinfo.h"
#endif



/* the data present indications (bit num <==> byte address) */

#define	LFLOWGROUP_DPNONE	0
#define	LFLOWGROUP_DPBYTE0	1
#define	LFLOWGROUP_DPBYTE1	2
#define	LFLOWGROUP_DPBYTE2	4
#define	LFLOWGROUP_DPBYTE3	8
#define	LFLOWGROUP_DPBYTE4	0x10
#define	LFLOWGROUP_DPBYTE5	0x20
#define	LFLOWGROUP_DPBYTE6	0x40
#define	LFLOWGROUP_DPBYTE7	0x80
#define	LFLOWGROUP_DPALL	255


/* transaction values for the 'trans' field */

/* for register & memory forwarding store operations */

#define	LFLOWGROUP_TSTORE	0	/* performing a store - data enclosed */
#define	LFLOWGROUP_TNULLIFY	1	/* performing a store nullify */

/* for memory forwarding operations only */

#define	LFLOWGROUP_TLOADFAULT	2	/* faulted reply from previous load */
#define	LFLOWGROUP_TCOMMIT	3	/* performing a committed store */

/* all other bus uses do not use the 'trans' field ! */






/* the various different contents of the bus signals */

struct lflowgroup {
	int	ftt ;		/* forwarder's time-tag value */
	int	ott ;		/* original time-tag value */
	int	tt ;		/* time-tag (for snooping) */
	int	rtt ;		/* reset time-tag value */
	uint	seq ;		/* sequence number */
	uint	addr ;		/* data */
	uint	dv ;		/* data value (32 bits) */
	uint	trans : 8 ;	/* type of transaction */
	uint	path : 4 ;	/* data (which path are we ?) */
	uint	dp : 5 ;	/* data present bits (one for each byte) */
	uint	f_resp : 1 ;	/* response bit */
} ;



extern int lflowgroup_init(LFLOWGROUP *) ;
extern int lflowgroup_free(LFLOWGROUP *) ;
extern int lflowgroup_loadbytes(LFLOWGROUP *,uchar *,int) ;
extern int lflowgroup_swapcopy(LFLOWGROUP *,LFLOWGROUP *) ;
extern int lflowgroup_clear(LFLOWGROUP *,int) ;
extern int lglowgroup_setftt(LFLOWGROUP *,int) ;

#if	F_XML
extern int lflowgroup_xmlinit(LFLOWGROUP *,XMLINFO *) ;
extern int lflowgroup_xmlout(LFLOWGROUP *,XMLINFO *) ;
extern int lflowgroup_xmlfree(LFLOWGROUP *,XMLINFO *) ;
#endif /* XML */



#endif /* LFLOWGROUP_INCLUDE */



