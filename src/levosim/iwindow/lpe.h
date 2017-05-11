/* lpe */

/* Levo Processing Element */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


	= 01/09/26, Dave Morano

	Added stuff for an XML output state trace.


*/


#ifndef	LPE_INCLUDE
#define	LPE_INCLUDE



#define	LPE		struct lpe_head



#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"xmlinfo.h"

#include	"levoinfo.h"
#include	"lexec.h"




struct lpe_data {
	int	opexec ;
	uint	src1;
	uint	src2;
	uint	src3 ;
	uint	src4 ;
	uint	src5 ;
	uint	dst1;
	uint	dst2 ;
	uint	dst3 ;
	uint	cnst ;
	int	bcnd ; /* branch condition (boolean) */
	uint	mema ; /* memory address */
	int	delay ;
};

struct lpe_waitlist {
	struct lpe_data	instpack ;
	int		priority ;
	int		delvtime ;
	int		tracknumber ;
};

struct lpe_flags {
	uint	shift : 1 ;
} ;

struct lpe_head {
	unsigned long	magic ;
	struct proginfo	*pip ;
	LSIM		*mip ;
	struct levoinfo	*lip ;
	struct lpe_flags	f ;
	struct lpe_waitlist	*delvlist ;
	struct lpe_waitlist	*waitlist ;
	int	nentries ;		/* maximum entries allowed */
	int	num_entr_in_dlist;
	int	num_entr_in_wlist;
	int	tracknumber;
} ;




#if	(! defined(LPE_MASTER)) || (LPE_MASTER != 0)

extern int	lpe_init(LPE *,struct proginfo *, LSIM *,
				struct levoinfo *) ;
extern int	lpe_free(LPE *) ;
extern int	lpe_comb(LPE *,int) ;
extern int	lpe_clock(LPE *) ;
extern int	lpe_shift(LPE *) ;
extern int	lpe_req2exec(LPE *, struct lpe_data *, int , int * );
extern int	lpe_isexecuted(LPE *, struct lpe_data *, int );
extern int	lpe_xmlinit(LPE *,XMLINFO *) ;
extern int	lpe_xmlout(LPE *,XMLINFO *) ;
extern int	lpe_xmlfree(LPE *,XMLINFO *) ;

#endif /* LPE_MASTER */


#endif /* LPE_INCLUDE */



