/* memstats */


#ifndef	MEMSTATS_INCLUDE
#define	MEMSTATS_INCLUDE	1


#include	<sys/types.h>
#include	<sys/param.h>
#include	<time.h>

#include	<vecitem.h>
#include	<hdb.h>

#include	"localmisc.h"



/* object defines */

#define	MEMSTATS			struct memstats_head
#define	MEMSTATS_STATS			struct memstats_stats





struct memstats_stats {
	double	rint_amean ;
	double	use_amean ;
	double	life_amean ;
	double	rint1_amean ;
	double	use1_amean ;
	double	life1_amean ;
	double	rint_mean, rint_var, rint_ov, rint_p7, rint_p8, rint_p9 ;
	double	use_mean, use_var, use_ov, use_p7, use_p8, use_p9 ;
	double	life_mean, life_var, life_ov, life_p7, life_p8, life_p9 ;
	double	rint1_mean, rint1_var, rint1_ov, rint1_p7, rint1_p8, rint1_p9 ;
	double	use1_mean, use1_var, use1_ov, use1_p7, use1_p8, use1_p9 ;
	double	life1_mean, life1_var, life1_ov, life1_p7, life1_p8, life1_p9 ;
	ULONG	in_start, ins ;
	ULONG	reads, writes ;
	uint	readvars, writevars, tes ;
	uint	flen, lenden ;
	uint	pages, groups ;
} ;

struct memstats_te {
	ULONG	read, write ;
} ;

/* tracking page entry */
struct memstats_tpe {
	struct memstats_te	*pp ;	/* pointer to page */
	ULONG			page ;	/* page number */
} ;

struct memstats_group {
	caddr_t		pa ;		/* mapped address */
} ;

struct memstats_curgroup {
	caddr_t		pa ;
	int		e ;
} ;

struct memstats_flags {
	int	started : 1 ;
} ;

struct memstats_head {
	unsigned long			magic ;
	struct memstats_curgroup	cg ;	/* current open group */
	vecitem		groups ;
	HDB		tts ;		/* tracking tables */
	ULONG		in_start, in ;
	ULONG		*den ;
	ULONG		*denrint, *denlife, *denuse ;
	ULONG		*denrint1, *denlife1, *denuse1 ;
	ULONG		offmask, pagemask ;
	ULONG		c_read, c_write ;
	ULONG		c_readnew, c_writenew ;
	ULONG		c_page, c_group ;
	double		sumrint, sumuse, sumlife ;
	double		sumrint1, sumuse1, sumlife1 ;
	int		lenden ;
	int		groupsize, pagesize ;
	int		pagebits, offbits ;
	int		npages ;	/* pages per group */
	int		fd ;		/* file descriptor */
	int		flen ;		/* file length */
	int		elemoff ;	/* element size offset */
	struct memstats_flags	f ;
	char		fname[MAXPATHLEN + 1] ;
} ;



#if	(! defined(MEMSTATS_MASTER)) || (MEMSTATS_MASTER == 0)

extern int	memstats_init(MEMSTATS *,char *,int,int,int,int) ;
extern int	memstats_read(MEMSTATS *,ULONG,int,ULONG,ULONG) ;
extern int	memstats_write(MEMSTATS *,ULONG,int,ULONG,ULONG) ;
extern int	memstats_writedone(MEMSTATS *,ULONG,int) ;
extern int	memstats_stats(MEMSTATS *,MEMSTATS_STATS *) ;
extern int	memstats_storefiles(MEMSTATS *,char *,char *,char *) ;
extern int	memstats_free(MEMSTATS *) ;

#endif /* MEMSTATS_MASTER */


#endif /* MEMSTATS_INCLUDE */



