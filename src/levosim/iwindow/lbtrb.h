/* lbtrb */

/* Levo Branch Targeting Buffer */



/* revision history :

	= 00/07/15, Alireza Khalafi
	This was originally written.

	= 00/09/18, Dave Morano
	I made the actual entry buffer dynamically allocated.


*/



#ifndef	LBTRB_INCLUDE
#define	LBTRB_INCLUDE


#define	LBTRB		struct lbtrb_head


#define		LBTRB_BUFF_SIZE  2560
#define		LBTRB_CPPAS  10   /* canceling predicates per AS */


#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"



struct lbtrb_predicates {
	int	ipaddr;		/* input predicate address */
	int	ipval;		/* input predicate value */
	int	icpaddr[LBTRB_CPPAS];	/* pointer to an array of canceling predicate addreses */
	int	icpval[LBTRB_CPPAS];	/* pointer to an array of canceling predicate values */
	int	numicp;		/* number of input canceling predicates */
	int	opaddr;		/* output predicate address */
	int	ocpaddr;	/* output canceling predicate address */
	int	opval;		/* input predicate value -1,0,1 (-1 not yet commited) */
	int	ocpval;		/* output canceling predicate values */
};

struct lbtrb_entry {
	int	valid;
	int	ncp;	/* number of canceling predicates for this address */
	int	cpa[LBTRB_CPPAS];
	int	cpv[LBTRB_CPPAS];
	uint	TA;	/* Target address */
};

struct lbtrb_state {
	int f_updatebuff;
	int f_ready;
};

struct lbtrb_head {
	struct proginfo	*pip ;
	struct mintinfo	*mip ;
	struct levoinfo	*lip ;

	struct lbtrb_state c,n;

	struct lbtrb_entry 	** head; 	/* currently unused */
	struct lbtrb_entry	*entry ;	/* dynamically allocated */
	struct lbtrb_predicates *commit_buf;	/* commit buffer */
	int	num_of_entries; /* numebr of entries in the buffer */
	int	empty_slot;	/* index of the free slot in the buffer */
	int	curr_pred;	/* address of the current predictor */
	int	inc_curr_pred;	/* increment the current predictor */
	int	bsize;		/* total number of entries in the buffer */
	int	totalrows;	/* total number of rows in the instruction window*/
	int	ppas;		/* number of canceling predicates per AS */
	int	commited_paddr; /* address of the last commited predicate */
	int	commited_pvalue;
} ;

#if	(! defined(LBTRB_MASTER)) || (LBTRB_MASTER != 0)

extern int	lbtrb_init(LBTRB *,struct proginfo *,struct mintinfo *,
					struct levoinfo *,int) ;
extern int	lbtrb_free(LBTRB *) ;
extern int	lbtrb_shift(LBTRB *) ;
extern int	lbtrb_flush(LBTRB *) ;
extern int	lbtrb_clock(LBTRB *) ;
extern int	lbtrb_comb(LBTRB *,int) ;
extern int	lbtrb_targetin(LBTRB *,uint,int) ;
extern int 	lbtrb_assign_predicate(LBTRB *, int, uint, uint, int,
			struct lbtrb_predicates *);
		

#endif /* LPE_MASTER */


#endif /* LPE_INCLUDE */



