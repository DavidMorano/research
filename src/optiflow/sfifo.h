/* sfifo */

/* synchronous FIFO */


#ifndef	SFIFO_INCLUDE
#define	SFIFO_INCLUDE		1


#include	<sys/types.h>

#include	"localmisc.h"		/* for { uint, ULONG } */



#define	SFIFO		struct sfifo_head



struct sfifo_state {
	int	head, tail, c ;
} ;

struct sfifo_flags {
	uint	ins : 1 ;
	uint	rem : 1 ;
} ;

struct sfifo_head {
	unsigned long	magic ;
	struct sfifo_state	c, n ;
	struct sfifo_flags	f ;
	char		*buf ;
	char		*valid ;
	int		objsize, nobj ;
} ;




#if	(! defined(SFIFO_MASTER)) || (SFIFO_MASTER == 0)

extern int	sfifo_init(SFIFO *,int,int) ;
extern int	sfifo_free(SFIFO *) ;
extern int	sfifo_ins(SFIFO *,void *) ;
extern int	sfifo_rem(SFIFO *,void *) ;
extern int	sfifo_read(SFIFO *,void *) ;
extern int	sfifo_clock(SFIFO *) ;
extern int	sfifo_comb(SFIFO *,int) ;

#endif /* SFIFO */


#endif /* SFIFO */



