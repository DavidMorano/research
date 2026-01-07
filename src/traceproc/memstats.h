/* memstats */


/* revision history:

	= 1998-11-01, David Morano

	Originally written for Audix Database Processor (DBP) work.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	MEMSTATS_INCLUDE
#define	MEMSTATS_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<time.h>

#include	<vecitem.h>
#include	<hdb.h>
#include	<localmisc.h>


/* object defines */

#define	MEMSTATS			struct memstats_head
#define	MEMSTATS_STATS			struct memstats_stats


struct memstats_stats {
	double	rint_mean, rint_var, rint_ov, rint_p7, rint_p8, rint_p9 ;
	double	life_mean, life_var, life_ov, life_p7, life_p8, life_p9 ;
	double	use_mean, use_var, use_ov, use_p7, use_p8, use_p9 ;
	double	rint1_mean, rint1_var, rint1_ov, rint1_p7, rint1_p8, rint1_p9 ;
	double	life1_mean, life1_var, life1_ov, life1_p7, life1_p8, life1_p9 ;
	double	use1_mean, use1_var, use1_ov, use1_p7, use1_p8, use1_p9 ;
	uint	flen, lenden ;
	uint	reads, writes ;
	uint	readvars, writevars ;
	uint	tes, pages, groups ;
} ;

struct memstats_te {
	uint	read, write ;
} ;

/* tracking page entry */
struct memstats_tpe {
	struct memstats_te	*pp ;	/* pointer to page */
	uint			page ;	/* page number */
} ;

struct memstats_group {
	caddr_t		pa ;		/* mapped address */
} ;

struct memstats_curgroup {
	caddr_t		pa ;
	int		e ;
} ;

struct memstats_head {
	unsigned long			magic ;
	struct memstats_curgroup	cg ;	/* current open group */
	vecitem		groups ;
	HDB		tts ;		/* tracking tables */
	uint		*den ;
	uint		*denrint, *denlife, *denuse ;
	uint		*denrint1, *denlife1, *denuse1 ;
	uint		offmask, pagemask ;
	uint		c_read, c_write ;
	uint		c_readnew, c_writenew ;
	uint		c_page, c_group ;
	int		lenden ;
	int		groupsize, pagesize ;
	int		pagebits, offbits ;
	int		npages ;	/* pages per group */
	int		fd ;		/* file descriptor */
	int		flen ;		/* file length */
	char		fname[MAXPATHLEN + 1] ;
} ;


#if	(! defined(MEMSTATS_MASTER)) || (MEMSTATS_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int	memstats_init(MEMSTATS *,char *,int,int,int) ;
extern int	memstats_read(MEMSTATS *,uint,int,uint,uint) ;
extern int	memstats_write(MEMSTATS *,uint,int,uint,uint) ;
extern int	memstats_writedone(MEMSTATS *,uint,int) ;
extern int	memstats_stats(MEMSTATS *,MEMSTATS_STATS *) ;
extern int	memstats_storefiles(MEMSTATS *,char *,char *,char *) ;
extern int	memstats_free(MEMSTATS *) ;

#ifdef	__cplusplus
}
#endif

#endif /* MEMSTATS_MASTER */

#endif /* MEMSTATS_INCLUDE */


