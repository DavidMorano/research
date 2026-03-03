/* memstats HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* maintain register statistics */
/* last modified %G% version %I% */


/* revision history:

	= 2002-08-21, David Morano
	This program was originally written.

*/

/* Copyright © 2002-2007 David A­D­ Morano.  All rights reserved. */

/**************************************************************************

  	Name:
	memstats

	Description:
	This object module tracks certain statistics about the
	use of memory.

**************************************************************************/

#ifndef	MEMSTATS_INCLUDE
#define	MEMSTATS_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<time.h>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<hdb.h>


/* object defines */
#define	MEMSTATS		struct memstats_head
#define	MEMSTATS_STATS		struct memstats_stats
#define	MEMSTATS_MAGIC		0x93726514


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
	ulong	in_start, ins ;
	ulong	reads, writes ;
	uint	readvars, writevars, tes ;
	uint	flen, lenden ;
	uint	pages, groups ;
} ; /* end struct */

struct memstats_te {
	ulong	read, write ;
} ; /* end struct */

/* tracking page entry */
struct memstats_tpe {
	struct memstats_te	*pp ;	/* pointer to page */
	ulong			page ;	/* page number */
} ; /* end struct */

struct memstats_group {
	caddr_t		pa ;		/* mapped address */
} ; /* end struct */

struct memstats_curgroup {
	caddr_t		pa ;
	int		e ;
} ; /* end struct */

struct memstats_flags {
	int	started : 1 ;
} ; /* end struct */

struct memstats_head {
	memstats_curgroup	cg ;	/* current open group */
	vecitem		groups ;
	hdb		tts ;		/* tracking tables */
	ulong		in_start, in ;
	ulong		*den ;
	ulong		*denrint, *denlife, *denuse ;
	ulong		*denrint1, *denlife1, *denuse1 ;
	ulong		offmask, pagemask ;
	ulong		c_read, c_write ;
	ulong		c_readnew, c_writenew ;
	ulong		c_page, c_group ;
	double		sumrint, sumuse, sumlife ;
	double		sumrint1, sumuse1, sumlife1 ;
	uint		magval ;
	int		lenden ;
	int		groupsize, pagesize ;
	int		pagebits, offbits ;
	int		npages ;	/* pages per group */
	int		fd ;		/* file descriptor */
	int		flen ;		/* file length */
	int		elemoff ;	/* element size offset */
	struct memstats_flags	f ;
	char		fname[MAXPATHLEN + 1] ;
} ; /* end struct */

typedef	MEMSTATS	memstats ;
typedef	MEMSTATS_STATS	memstats_st ;

EXTERNC_begin

extern int	memstats_init(memstats *,char *,int,int,int,int) noex ;
extern int	memstats_read(memstats *,ulong,int,ulong,ulong) noex ;
extern int	memstats_write(memstats *,ulong,int,ulong,ulong) noex ;
extern int	memstats_writedone(memstats *,ulong,int) noex ;
extern int	memstats_getstats(memstats *,memstats_st *) noex ;
extern int	memstats_storefiles(memstats *,char *,char *,char *) noex ;
extern int	memstats_free(memstats *) noex ;

EXTERNC_end


#endif /* MEMSTATS_INCLUDE */


