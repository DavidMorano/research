/* lifetch.h */

/* revision history:

   = 07/05/2000, Marcos de Alba
   This code was started from scratch.

 */

#ifndef LIFETCH_INCLUDE
#define	LIFETCH_INCLUDE 1

/* object defines */

#define LIFETCH		    struct lifetch_head
#define LIFETCH_BUFFER      struct lifetch_buffer
#define LIFETCH_INITARGS    struct lifetch_initargs



#include	"config.h"		
#include	"defs.h"		
#include        "lsim.h"
#include	"levoinfo.h"		
#include	"llb.h"			/* Levo i-load buffer */
#include        "ldecode.h"             /* Levo decode unit */
#include        "misc.h"                /* Some definitions */
#include	"lbtrb.h"               /* Levo Branch Tracking Buffer */
#include        "las.h"                 /* Levo Active Station */
#include        "bus.h"
#include        <stdio.h>

/* Fetch Unit Objects */
#include        "lbp.h"                 /* Levo Branch Predcitor */
#include        "lloop.h"               /* Levo Loop Predictor */
#include        "ltcache.h"             /* Levo Indirect Branch Predictor */
#include        "lcallstack.h"          /* Levo Call/Return Stack */


/* For now assign constant value sizes for the predictors */

#define NUMBENTRIES 32      /* number of entries in lifetch_buffer */
#define NUMLBUFFERS 32       /* maximum number of load buffers */
#define BACKWARD    12
#define FORWARD     13
#define RETURN_TYPE 14

/* some extra arguments */

struct lifetch_sanitycall {
	int	(*func)() ;
	void	*arg ;
} ;

struct lifetch_initargs {
	struct lifetch_sanitycall	sanity ;
	BUS	*rfbuses;
	LAS	*lases;
	char	*ifetchtrace ;		/* i-fetch trace filename */
	uint	startaddr ;
};


struct lifetch_sflags{
  uint loadit : 1;
};

struct lifetch_state{
  struct lifetch_sflags f;
};

struct lifetch_head {
  struct proginfo	*pip ;
  LSIM		*mip ;
  struct levoinfo *lip ;
  struct lifetch_sanitycall	sanity ;
  struct lifetch_state c, n;
  BUS   *rfbuses;
  LAS   *lases;
  uint  magic;
  uint  *inwindow;
  LDECODE             *buffer;
  LLOOP               loops;
  LBP                 lbpred;
  LLB                 *llbp;
  LTCACHE             ltcache;
  LCALLSTACK          lstack;
  uint                lbstatus[NUMLBUFFERS];
  uint                fetchaddr ;
  int                 ifetchwidth ;
  uint                nextinstr ;
  uint                btarget; 
  int                 index; 
  uint                winsize;
  int                 f_br_updated;
  uint                exitaddr;
  uint                f_loaddelayslot;
  int                 f_loadllb;       /* Do we load the Load Buffer? */
  uint                lastreqaddr;     /* address of last requested instruction */
  uint                lastrdaddr;      /* address of last instruction read */
  int                 f_lastpos;       /* Branch instruction loaded in last position of column */
  uint                addrtrace[BUFSIZ];      /* List of perfect fetch addresses */
  uint                traceidx;        /* index to current address in addrtrace*/
  int                 instrsread;               
  int                 finished;
  int                 fsize;
  FILE                *tracefile;
};

extern int	lifetch_init(LIFETCH *,struct proginfo *, LSIM *,
			struct levoinfo *, LIFETCH_INITARGS *) ;
extern int	lifetch_free(LIFETCH *) ;
extern int	lifetch_comb(LIFETCH *,int) ;
extern int	lifetch_clock(LIFETCH *) ;
extern int      lifetch_unroll_loop(LIFETCH *,int,int,int,int, uint *);
extern int      lifetch_cv_bbr_to_fbr(LDECODE*,int,int);
extern int      lifetch_bu(LIFETCH *, uint, uint, int, uint);
extern int      lifetch_loadstatus(LIFETCH *,int);
extern int      lifetch_condbranch(LIFETCH *, uint *, struct levoinfo *);
extern int   	lifetch_jump(LIFETCH *, uint *, struct levoinfo *);
extern int     	lifetch_indjump(LIFETCH *, uint *, struct levoinfo *);
extern int     	lifetch_nonbranch(LIFETCH *, uint *, struct levoinfo *);

#endif /* LIFETCH_INCLUDE */




