/* regstats HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* maintain register statistics */
/* last modified %G% version %I% */


/* revision history:

	= 1998-11-01, David Morano
	Originally written for Audix Database Processor (DBP) work.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	REGSTATS_INCLUDE
#define	REGSTATS_INCLUDE


#include	<envstandards.h>	/* ordered first to configure */
#include	<clanguage.h>
#include	<usysbase.h>


#define	REGSTATS		struct regstats_head
#define	REGSTATS_ST		struct regstats_stats
#define	REGSTATS_MAGIC		0x98726514


struct regstats_stats {
	double	rint_mean, rint_var, rint_ov ;
	double	life_mean, life_var, life_ov ;
	double	use_mean, use_var, use_ov ;
} ;

struct regstats_head {
	uint	*atrack ;
	uint	*wtrack ;
	uint	*rtrack ;
	uint	*rread, *rwrite ;
	uint	*den ;
	uint	*denrint, *denlife, *denuse ;
	uint	magval ;
	int	lentab, lenuse, lenlife ;
} ;

typedef	REGSTATS	regstats ;
typedef	REGSTATS_ST	regstats_st ;

EXTERNC_begin

extern int	regstats_init(regstats *,int,int,int) noex ;
extern int	regstats_read(regstats *,uint,int,uint,uint) noex ;
extern int	regstats_readupdate(regstats *,uint,int,uint,uint) noex ;
extern int	regstats_write(regstats *,uint,int,uint,uint) noex ;
extern int	regstats_writedone(regstats *,uint,int) noex ;
extern int	regstats_getstats(regstats *,regstats_st *) noex ;
extern int	regstats_storefiles(regstats *,
			char *,char *,char *,char *,char *) noex ;
extern int	regstats_free(regstats *) noex ;

EXTERNC_end


#endif /* REGSTATS_INCLUDE */


