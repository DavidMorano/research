/* lsq */

/* Levo Store Queue */



/* revision history :

	= 00/02/15, Alireza Khalafi 
	This code was started.

*/



#ifndef	LSQ_INCLUDE
#define	LSQ_INCLUDE	1



/* object defines */

#define	LSQ		struct lsq_head



#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"





struct lsq_entry {
	int	valid;	/* valid entry */
	uint	addr;	/* memory address */
	int	data;	/* data */
	int	tt;	/* time tag */
} ;

struct lsq_flags {
	uint	shift : 1 ;
} ;
	
struct lsq_state {
	int	whatever ;		/* we are stateless ?? */
} ;

struct lsq_head {
	struct proginfo	*pip ;
	struct mintinfo	*mip ;
	struct levoinfo	*lip ;
	struct lsq_flags	f ;	/* object flags */
	struct lsq_state	c, n ;
	struct lsq_entry * entry;	/* pointer to the queue */
	uint	magic ;
	int	qsize;		/* size of the store queue */
	int	num_of_entries;	/* number of entries in the queue */
} ;



/* public subroutines */

extern int	lsq_init(LSQ *, 
			struct proginfo *, struct mintinfo *, 
			struct levoinfo *,int) ;
extern int	lsq_free(LSQ *) ;
extern int	lsq_shift(LSQ *) ;
extern int	lsq_clock(LSQ *) ;
extern int	lsq_comb(LSQ *,int) ;

extern int	lsq_insert(LSQ *,uint,int,int,int);
extern int	lsq_read(LSQ *,uint *, int *, int *);
extern int	lsq_num_of_entries(LSQ *);


#endif /* LSQ_INCLUDE */



