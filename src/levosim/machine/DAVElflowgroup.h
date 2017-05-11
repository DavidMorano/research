/* lflowgroup */



#ifndef	LFLOWGROUP_INCLUDE
#define	LFLOWGROUP_INCLUDE	1



/* object defines */

#define	LFLOWGROUP		struct lflowgroup
#define	LFLOWGROUP_DATALANE	struct lflowgroup_datalane



#include	"config.h"
#include	"defs.h"




/* the data present indications */

#define	LFLOWGROUP_DPNONE	0
#define	LFLOWGROUP_DPALL	15
#define	LFLOWGROUP_DPBYTE0	8
#define	LFLOWGROUP_DPBYTE1	4
#define	LFLOWGROUP_DPBYTE2	2
#define	LFLOWGROUP_DPBYTE3	1


/* transaction values for the 'trans' field */

/* for register & memory forwarding store operations */

#define	LFLOWGROUP_TSTORE	0	/* performing a store - data enclosed */
#define	LFLOWGROUP_TNULLIFY	1	/* performing a store nullify */

/* for memory forwarding operations only */

#define	LFLOWGROUP_TLOADOK	2	/* good reply from previous load */
#define	LFLOWGROUP_TLOADFAULT	3	/* faulted reply from previous load */
#define	LFLOWGROUP_TCOMMIT	4	/* performing a committed store */

/* all other bus uses do not use the 'trans' field ! */






/* the various different contents of the bus signals */

struct lflowgroup {
	int	path ;		/* data (which path are we ?) */
	int	tt ;		/* time-tag */
	int	rtt ;		/* reset time-tag value */
	uint	addr ;		/* data */
	int	trans ;		/* type of transaction */
	int	dvtype ;	/* type of data value */
	uint	dv ;		/* data value (32 bits) */
	uint	dp ;		/* data present bits (one for each byte) */
} ;



extern int lflowgroup_init(LFLOWGROUP *) ;
extern int lflowgroup_free(LFLOWGROUP *) ;
extern int lflowgroup_loadbytes(LFLOWGROUP *,uchar *,int) ;
extern int lflowgroup_swapcopy(LFLOWGROUP *,LFLOWGROUP *) ;
extern int lflowgroup_clear(LFLOWGROUP *) ;



#endif /* LFLOWGROUP_INCLUDE */



