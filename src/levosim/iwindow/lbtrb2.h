/* lpe */

/* Levo Branch Targeting Buffer */



/* revision history :

	= 00/07/15, Alireza Khalafi

*/



#ifndef	LBTRB_INCLUDE
#define	LBTRB_INCLUDE

#define		LBTRB_BUFF_SIZE  256


#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"



#define	LBTRB		struct lbtrb_head

struct lbtrb_entry {
	int	valid;
	int	PA;	/* predication address */
	int	TA;	/* Target address */
	int	BA;	/* Branch Address */
};

struct lbtrb_head {
	struct proginfo	*pip ;
	LSIM		*mip ;
	struct levoinfo	*lip ;

	struct lbtrb_entry ** head; 
	int	num_of_entries; /* numebr of entries in the buffer */
	int	empty_slot;	/* index of the free slot in the buffer */

} ;



extern int	lbtrb_free(LBTRB *) ;
extern int	lbtrb_shift(LBTRB *) ;
extern int	lbtrb_clock(LBTRB *) ;
extern int	lbtrb_comb(LBTRB *,int) ;
extern int	lbtrb_insert(LBTRB *, struct lbtrb_entry *);
extern int	lbtrb_lookup(LBTRB *, int TA, int *);
//extern int	lbtrb_lookup(LBTRB *, int PA);

//#endif /* LPE_MASTER */


#endif /* LPE_INCLUDE */



