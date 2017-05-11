/* regstats */


#ifndef	REGSTATS_INCLUDE
#define	REGSTATS_INCLUDE	1


#include	<sys/types.h>
#include	<time.h>

#include	"localmisc.h"



/* object defines */

#define	REGSTATS			struct regstats_head
#define	REGSTATS_STATS			struct regstats_stats




struct regstats_stats {
	double	rint_amean, rint_mean, rint_var, rint_ov ;
	double	use_amean, use_mean, use_var, use_ov ;
	double	life_amean, life_mean, life_var, life_ov ;
	ULONG	in_start, ins ;
	ULONG	reads, writes ;
	uint	lenden, lentrack ;
} ;

struct regstats_flags {
	int	started : 1 ;
} ;

struct regstats_head {
	ULONG	magic ;
	ULONG	in_start, in ;
	ULONG	*track ;		/* tracking allocation point */
	ULONG	*tread, *twrite ;	/* tracking */
	ULONG	*iread, *iwrite ;	/* individual (per-register) counts */
	ULONG	*den ;			/* denisity allocation point */
	ULONG	*denrint, *denlife, *denuse ;
	ULONG	*svalue, *swrite ;	/* silent writes */
	ULONG	c_read, c_write ;
	double	sumrint, sumuse, sumlife ;
	uint	lenden, lentrack ;
	struct regstats_flags	f ;
} ;



#if	(! defined(REGSTATS_MASTER)) || (REGSTATS_MASTER == 0)

extern int	regstats_init(REGSTATS *,int,int) ;
extern int	regstats_read(REGSTATS *,ULONG,int,ULONG,ULONG) ;
extern int	regstats_readupdate(REGSTATS *,ULONG,int,ULONG,ULONG) ;
extern int	regstats_write(REGSTATS *,ULONG,int,ULONG,ULONG) ;
extern int	regstats_writedone(REGSTATS *,ULONG,int) ;
extern int	regstats_stats(REGSTATS *,REGSTATS_STATS *) ;
extern int	regstats_storefiles(REGSTATS *,
			char *,char *,char *,char *,char *) ;
extern int	regstats_free(REGSTATS *) ;

#endif /* REGSTATS_MASTER */


#endif /* REGSTATS_INCLUDE */



