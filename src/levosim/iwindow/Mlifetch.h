/* lifetch.h */


/* revision history:

	= 00/07/05, Marcos de Alba
	This code was started from scratch.


 */

#ifndef LIFETCH_INCLUDE
#define	LIFETCH_INCLUDE 1

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines */

#define LIFETCH			struct lifetch_head
#define LIFETCH_INITARGS	struct lifetch_initargs



#include	"config.h"		/* pickup some general stuff */
#include	"defs.h"		/* general code-wide defs */
#include	"lsim.h"		/* this structure */
#include	"levoinfo.h"		/* and this structure also */
#include	"llb.h"			/* Levo i-load buffer */
#include        "ldecode.h"
#include	"bus.h"
#include	"las.h"
#include	"stdio.h"



/* some extra arguments */

struct lifetch_sanitycall {
	int	(*func)() ;
	void	*arg ;
} ;

struct lifetch_initargs {
	struct lifetch_sanitycall	sanity ;
	BUS	*rfbuses ;
	LAS	*lases ;
	char	*ifetchtrace ;		/* i-fetch trace filename */
	uint	startaddr ;
} ;

/* pointers to the temporary test simulated memory */
struct lifetch_mem {
  int *data;
  int *addr;
  int	num ;
};

struct lifetch_sflags {
	uint	loadit : 1 ;
} ;

struct lifetch_state {
	struct lifetch_sflags	f ;
} ;

struct lifetch_head {
	unsigned long	magic ;

	struct proginfo	*pip ;
	LSIM		*mip ;
	struct levoinfo *lip ;

	struct lifetch_sanitycall	sanity ;

	struct lifetch_state	c, n ;
	struct lifetch_mem	simmem ;

	LLB *llbp;
	BUS	*rfbuses ;		/* testing */
	LAS	*lases ;		/* testing */
  uint fetchaddr ;
  uint exitaddr ;
  uint btarget ;	/* target address of the last commited branch */
  int  ifetchwidth ;
  int  *ilptr ;
  int  *entry ;
  uint  *inwindow ;
  int  index; 
  uint winsize;
  FILE	*tracefile;
  uint	reuse_faddr;
};



extern int	lifetch_init(LIFETCH *,struct proginfo *,
			LSIM *,struct levoinfo *, LIFETCH_INITARGS *) ;
extern int	lifetch_free(LIFETCH *) ;
extern int	lifetch_comb(LIFETCH *,int) ;
extern int	lifetch_clock(LIFETCH *) ;
extern int	lifetch_goto(LIFETCH *,uint) ;
extern int	lifetch_loadstatus(LIFETCH *,uint) ;
extern int	lifetch_bu(LIFETCH *,uint,uint,int,uint) ;



#endif /* LIFETCH_INCLUDE */



