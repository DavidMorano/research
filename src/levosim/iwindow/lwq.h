/* lwq */

/* Levo Write Queue */



/* revision history :

	= 00/02/15, Alireza Khalafi 

	This code was started.


	= 01/09/26, Dave Morano

	Added stuff for an XML output state trace.


*/



#ifndef	LWQ_INCLUDE
#define	LWQ_INCLUDE	1



/* object defines */

#define	LWQ		struct lwq_head
#define	LWQ_INITARGS	struct lwq_initargs




#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"xmlinfo.h"

#include	"levoinfo.h"
#include	"lsq.h"
#include	"lbusint.h"
#include	"rfbus.h"



/* defines */

#define	LWQ_MAXSERVICABLE	4
#define LATESTFORWARDSSIZE	100



struct lwq_initargs {
	RFBUS   *memrequest ;           /* load request buses to memory */
	RFBUS   *memresponse ;		/* load response buses from memory */
	RFBUS   *memwrite ;		/* write buses to memory */
	BUS 	*mbwbus;		/* backwarding buses from i-window */
	BUS 	*mfwbus ;		/* forwarding buses to i-window */
	uint	wmfinter, wmbinter ;	/* execution window interleaves */
	int	sqsize ;		/* StoreQueue size, depth of Qs */
	int	busid ;
	int	wqsize;			/* size of write queue */
} ;

struct lwq_wqentry {
	char		full;	/* flag: the slot is occupied */
	char		ready;	/* flag: the memory data value is valid */
	char		serv;	/* flag: the memory request is being serviced */
	char		wait;	/* flag: do not service it in this clock*/
	int		index;	/* index of the slot */
	LFLOWGROUP	d;	/* pointer to the queue entries */
};

struct lwq_waitqueue {
	int	ql;		/* pointer to the next entry to be read */
	int	qs;		/* pointer to the next empty slot */
	int	qr;		/* pointer to the next ready slot */
	char	f_qfull;	/* flag: queue is full */
	char	f_qempty;	/* flag: queue is empty */
	int	qsize;		/* size of the queue */
	struct lwq_wqentry *en;	/* pointer to the entries in the queue */
};

struct lwq_entry {
	int	valid;	/* valid entry */
	uint	addr;	/* memory address */
	int	data;	/* data */
	int	tt;	/* time tag */
	int	dp;	/* data present*/
	int	path;	/* path id */
};

struct lwq_state {
	struct lwq_entry	*as_fifo; /* data read from AS bus */
};

struct lwq_flags {
	uint	shift : 1 ;
} ;

struct lwq_head {
	unsigned long		magic ;
	struct lwq_flags	f ;
	struct proginfo	*pip ;
	LSIM		*mip ;
	struct levoinfo	*lip ;

	struct lwq_waitqueue wq;

	LBUSINT	*mbwbus;	/* execution window bus managers */
	LBUSINT	*mfwbus;	/* execution window bus managers */

	RFBUS   *memrequest ;	/* load request buses to memory */
	RFBUS   *memresponse ;	/* load response buses from memory */
	RFBUS   *memwrite ;	/* write buses to memory */

	struct lwq_state c,n;

	LSQ	*lsqes ;
	LSQ	lsq ;

	uint	wmfinter, wmbinter ;	/* execution window interleaves */
	uint	nwmfbuses, nwmbbuses ;	/* execution window buses */

	int	nmb ;		/* number of memory buses */
	int	norows;		/* number of rows */
	int	buspriority;

/*	int	numForwards;
	long	latestForwards[100]; */
} ;



/* public subroutines */

extern int	lwq_init(LWQ *,struct proginfo *,LSIM *, 
			struct levoinfo *,LWQ_INITARGS *) ;
extern int	lwq_free(LWQ *) ;
extern int	lwq_shift(LWQ *) ;
extern int	lwq_clock(LWQ *) ;
extern int	lwq_comb(LWQ *,int) ;

extern int	lwq_store(LWQ *,uint,int,int,int,int);
extern int	lwq_num_of_entries(LWQ *);



#endif /* LWQ_INCLUDE */



