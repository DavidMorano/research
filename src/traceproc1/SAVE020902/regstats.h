/* regstats */


#ifndef	REGSTATS_INCLUDE
#define	REGSTATS_INCLUDE	1


#include	<sys/types.h>
#include	<time.h>

#include	"misc.h"



/* object defines */

#define	REGSTATS			struct regstats_head
#define	REGSTATS_STATS			struct regstats_stats




struct regstats_stats {
	double	read_mean, read_var ;
	double	life_mean, life_var ;
	double	use_mean, use_var ;
} ;

struct regstats_head {
	unsigned long	magic ;
	uint	*atrack ;
	uint	*wtrack ;
	uint	*rtrack ;
	uint	*rread, *rwrite ;
	uint	*den ;
	uint	*readden, *lifeden, *useden ;
	int	tablen, uselen, lifelen ;
} ;



#if	(! defined(REGSTATS_MASTER)) || (REGSTATS_MASTER == 0)

extern int	regstats_init(REGSTATS *,int,int,int) ;
extern int	regstats_read(REGSTATS *,uint,int,uint,uint) ;
extern int	regstats_readupdate(REGSTATS *,uint,int,uint,uint) ;
extern int	regstats_write(REGSTATS *,uint,int,uint,uint) ;
extern int	regstats_stats(REGSTATS *,REGSTATS_STATS *) ;
extern int	regstats_storefiles(REGSTATS *,
			char *,char *,char *,char *,char *) ;
extern int	regstats_free(REGSTATS *) ;

#endif /* REGSTATS_MASTER */


#endif /* REGSTATS_INCLUDE */



