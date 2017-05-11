/* alirgf */

/* Alireza Levo Register file */



/* revision history :

	= 00/02/15, Alireza Khalafi 

	This code was started.


*/



#ifndef	ALIRGF_INCLUDE
#define	ALIRGF_INCLUDE	1



/* object defines */

#define	ALIRGF		struct alirgf_head

#define	NUM_OF_REGS	200



#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"levoinfo.h"

/* this forms an object definition loop */
#ifdef	COMMENT
#include	"las.h"
#endif /* COMMENT */



struct alirgf_reg {
	int	d;
	int	a;
	int	tt;
	int	pathid;
} ;

struct alirgf_head {
	struct proginfo	*pip ;
	LSIM		*mip ;
	struct levoinfo	*lip ;
	struct alirgf_reg	reg[NUM_OF_REGS];
} ;



/* public subroutines */

extern int	alirgf_init(ALIRGF *, 
			struct proginfo *,LSIM *, 
			struct levoinfo *,uint) ;
extern int	alirgf_free(ALIRGF *) ;
extern int	alirgf_shift(ALIRGF *) ;
extern int	alirgf_clock(ALIRGF *) ;
extern int	alirgf_comb(ALIRGF *,int) ;
extern int	alirgf_getval(ALIRGF *,uint,uint *) ;
extern int	alirgf_getval2(ALIRGF *,uint,int *,uint *) ;
extern int	alirgf_writereg(ALIRGF *,uint,int,uint) ;

/* sorry, the following two subroutines cause an object definition loop ! */

#ifdef	COMMENT
extern int	alirgf_write(ALIRGF *,LAS *);
extern int	alirgf_read(ALIRGF *,LAS *);
#endif /* COMMENT */


#endif /* ALIRGF_INCLUDE */



