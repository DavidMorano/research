/* ali_rgf */

/* Levo Register file */



/* revision history :

	= 00/02/15, Alireza Khalafi 
	This code was started.

*/



#ifndef	ALIRGF_INCLUDE
#define	ALIRGF_INCLUDE	1



/* object defines */

#define	NUM_OF_REGS	200
#define	ALIRGF		struct alirgf_head



#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"levoinfo.h"
#include	"las.h"



struct alirgf_reg {
	int	d;
	int	a;
	int	tt;
	int	pathid;
};

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
extern int	alirgf_getval(ALIRGF *,int,uint *) ;

/* extern int	alirgf_write(ALIRGF *,LAS *);
extern int	alirgf_read(ALIRGF *,LAS *);
*/


#endif /* LSQ_INCLUDE */



