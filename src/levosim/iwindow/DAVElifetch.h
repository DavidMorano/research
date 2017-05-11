/* lifetch.h */


/* revision history:

	= 00/07/05, Marcos de Alba

	This code was started from scratch.


	= 00/08/01, Dave Morano

	Oh, many changes.


	= 01/09/26, Dave Morano

	Added stuff for an XML output state trace.


 */


#ifndef LIFETCH_INCLUDE
#define	LIFETCH_INCLUDE 1

#ifndef	UINT
#define	UINT	unsigned int
#endif



/* object defines */

#define LIFETCH			struct lifetch_head
#define LIFETCH_INITARGS	struct lifetch_initargs



#include	<sys/types.h>
#include	<stdio.h>

#include	"misc.h"		/* pickup some general stuff */
#include	"config.h"		/* pickup some general stuff */
#include	"defs.h"		/* general code-wide defs */
#include	"lsim.h"		/* this structure */
#include	"xmlinfo.h"		/* this structure */

#include	"levoinfo.h"		/* and this structure also */
#include	"llb.h"			/* Levo i-load buffer */
#include        "ldecode.h"
#include	"bus.h"
#include	"las.h"



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
	int *data ;
	int *addr ;
	int	num ;
} ;

struct lifetch_sflags {
	uint	loadit : 1 ;
	uint	cfc : 1 ;
} ;

struct lifetch_state {
	struct lifetch_sflags	f ;
	uint			ia ;
} ;

struct lifetch_head {
	unsigned long	magic ;
	struct lifetch_sanitycall	sanity ;
	struct lifetch_state	c, n ;
	struct lifetch_mem	simmem ;
	struct proginfo	*pip ;
	LSIM		*mip ;
	struct levoinfo *lip ;
	LLB *llbp ;
	BUS	*rfbuses ;		/* testing */
	LAS	*lases ;		/* testing */
	FILE	*tracefile ;
	int  *ilptr ;
	int  *entry ;
	uint  *inwindow ;

	uint fetchaddr ;
	uint exitaddr ;
	uint btarget ;	/* target address of the last commited branch */
	int  ifetchwidth ;
	int  index ;
	uint winsize ;
} ;



#if	(! defined(LIFETCH_MASTER)) || (LIFETCH_MASTER == 0)

extern int	lifetch_init(LIFETCH *,struct proginfo *,
			LSIM *,struct levoinfo *, LIFETCH_INITARGS *) ;
extern int	lifetch_free(LIFETCH *) ;
extern int	lifetch_comb(LIFETCH *,int) ;
extern int	lifetch_clock(LIFETCH *) ;
extern int	lifetch_goto(LIFETCH *,uint) ;
extern int	lifetch_loadstatus(LIFETCH *,uint) ;
extern int	lifetch_bu(LIFETCH *,uint,uint,int,uint) ;
extern int	lifetch_xmlinit(LIFETCH *,XMLINFO *) ;
extern int	lifetch_xmlout(LIFETCH *,XMLINFO *) ;
extern int	lifetch_xmlfree(LIFETCH *,XMLINFO *) ;

#endif /* LIFETCH_MASTER */


#endif /* LIFETCH_INCLUDE */



