/* regstats */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	REGSTATS_INCLUDE
#define	REGSTATS_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>

#include	<localmisc.h>


/* object defines */

#define	REGSTATS			struct regstats_head
#define	REGSTATS_STATS			struct regstats_stats


struct regstats_stats {
	double	rint_mean, rint_var, rint_ov ;
	double	life_mean, life_var, life_ov ;
	double	use_mean, use_var, use_ov ;
} ;

struct regstats_head {
	unsigned long	magic ;
	uint	*atrack ;
	uint	*wtrack ;
	uint	*rtrack ;
	uint	*rread, *rwrite ;
	uint	*den ;
	uint	*denrint, *denlife, *denuse ;
	int	lentab, lenuse, lenlife ;
} ;


#if	(! defined(REGSTATS_MASTER)) || (REGSTATS_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	regstats_init(REGSTATS *,int,int,int) ;
extern int	regstats_read(REGSTATS *,uint,int,uint,uint) ;
extern int	regstats_readupdate(REGSTATS *,uint,int,uint,uint) ;
extern int	regstats_write(REGSTATS *,uint,int,uint,uint) ;
extern int	regstats_writedone(REGSTATS *,uint,int) ;
extern int	regstats_stats(REGSTATS *,REGSTATS_STATS *) ;
extern int	regstats_storefiles(REGSTATS *,
			char *,char *,char *,char *,char *) ;
extern int	regstats_free(REGSTATS *) ;

#ifdef	__cplusplus
}
#endif

#endif /* REGSTATS_MASTER */

#endif /* REGSTATS_INCLUDE */


