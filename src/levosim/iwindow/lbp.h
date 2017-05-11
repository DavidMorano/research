/* lbp */

/* Levo Branch Predictor */

#ifndef	LBP_INCLUDE
#define	LBP_INCLUDE	



/* object defines */

#define	LBP		struct lbp_head
#define	LBP_BRANCH	struct lbp_branch



#include	<sys/types.h>

#include <assert.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
 


#define LBP_PHTBOX      struct lbp_phtbox

#define LBP_SIZE      128
#define TAKEN 1
#define NOT_TAKEN 0

/* Branch history and statistics */
struct lbp_branch {
  uint ilptr;
  uint BTA;
  int predict; 
  int taken;
  int times;
  int right_pred;
  int history;
} ;

struct lbp_state {
  struct lbp_branch	*b ;
} ;

struct lbp_flags {
	uint			shift : 1 ;
} ;

struct lbp_head {
  struct lbp_flags	f ;		/* flags */
  struct proginfo	*gp ;
  struct mintinfo	*mip ;
  struct levoinfo	*lip ;
  struct lbp_state	c, n ;	   
  int		        size ;    /* size of BHT */
  uint		        *bhr ;    /* branch history register */  
  uint                  *btable ; /* branch history table */  
  uint                  bhtindx;  /* branch history table index */
  uint                  phtindx;  /* pattern history (table) register index */
  int                   misspred ; 
} ;



extern int lbp_init(LBP *, struct proginfo *, struct mintinfo *, 
			struct levoinfo *, int size);
extern int lbp_free(LBP	*);
extern int lbp_clock(LBP *);
extern int lbp_shift(LBP *lbp);
extern int lbp_newstate(LBP *lbp);
extern int lbp_pred(LBP *, uint, int *, uint);
extern int lbp_update(LBP *, uint, int);


#endif /* LBP_INCLUDE */


