/* lloop */

/* Levo Branch Predictor */

#ifndef	LLOOP_INCLUDE
#define	LLOOP_INCLUDE	



/* object defines */

#define	LLOOP		struct lloop_head
#define LLOOP_INFO      struct lloop_info
#define LLOOP_ENTRY     struct lloop_entry



#include	<sys/types.h>
#include <assert.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"



#define LOOPTABLESIZE 256
#define LOOPSTACKSIZE 32
#define TAKEN 1
#define NOT_TAKEN 0

/* Structure to keep track of loop statistics */
struct lloop_info {
  uint id;          /* Target address also known as loop id */
  uint braddr;      /* Address of loop branch */
  uint pilptr;  /* previous ilptr, used in case of misprediction */ 
  char predict; 
  int outcome;
  int ttaken;         /* number of times taken */
  int execs;      /* number of loop executions */ 
  int itns;       /* number of iterations per loop execution */
  int lsize;      /* loop size: ((bradrr - id) >> 2) + 1*/
  int right_pred;
  /* For future use */
  int iiter;      /* number of dynamic instr per loop iteration */
  float avgpred;  /* average prediction per execution*/
  float avgt;     /* average taken */
  float avgitns;  /* avg number of itns */
  float totavgitns;  /* avg number of itns */
  float totavgpred;  /* average prediction per execution*/
  float totavgt;     /* average taken */
  
} ;

struct lloop_entry{
  uint b; /* branch address */
  uint t; /* target address or loop id */
};

struct lloop_state {
  struct lloop_info	*l ;
  struct lloop_entry   *lstack ; /* I don't know, have to ask Dave */
} ;

struct lloop_flags {
	uint			shift : 1 ;
} ;

struct lloop_head {
  struct lloop_flags	f ;		/* flags */
  struct proginfo       *gp ;
  struct mintinfo	*mip ;
  struct levoinfo	*lip ;
  struct lloop_state	c, n ;	   
  int	                size ;         /* size of loop table */
  LLOOP_INFO	        *ltable ;      /* loop statistics table */  
  LLOOP_ENTRY           *lstack;       /* loop stack */    
  uint                  ssize;         /* stack size */
} ;

extern int lloop_newstate(LLOOP *) ;
extern int lloop_pred(LLOOP *, uint, uint);
extern int lloop_init(LLOOP *, struct proginfo *, struct mintinfo *, struct levoinfo *, int);
extern int lloop_free(LLOOP *);
extern int lloop_comb(LLOOP *, int);
extern int lloop_shift(LLOOP *);
extern int lloop_clock(LLOOP *);
extern int lloop_update(LLOOP *, int, uint);
extern int lloop_stack(LLOOP *, uint, uint, int);
extern int lloop_del_loop_fromstack(LLOOP *, uint, uint);

#endif /* LLOOP_INCLUDE */


