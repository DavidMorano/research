/* regstats */

/* maintain register statistics */
/* last modified %G% version %I% */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_SAFE		0


/* revision history:

	= 2002-08-21, David Morano

	This program was originally written.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This object module tracks certain statistics about the
	use of registers.

*******************************************************************************/

#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<assert.h>

#if	defined(IRIX)
#include	<standards.h>
#endif

#include	<vsystem.h>
#include	<mallocstuff.h>
#include	<bfile.h>
#include	<localmisc.h>

#include	"regstats.h"


/* local defines */

#define	REGSTATS_MAGIC		0x98726514
#define	REGSTATS_DEFUSELEN	(8 * 1024)
#define	REGSTATS_DEFLIFELEN	(8 * 1024)
#define	REGSTATS_DEFTABLEN	256	/* number of registers */

#if	defined(IRIX)
#ifndef	CF_ULOCK
#define CF_ULOCK	0	/* Unlock a previously locked region */
#endif
#ifndef	CF_LOCK
#define CF_LOCK	1	/* Lock a region for exclusive use */
#endif
#ifndef	CF_TLOCK
#define CF_TLOCK	2	/* Test and lock a region for exclusive use */
#endif
#ifndef	CF_TEST
#define CF_TEST	3	/* Test a region for other processes locks */
#endif
#endif /* defined(IRIX) */


/* external subroutines */

extern uint	nextpowtwo(uint) ;

extern int	fmeanvarai(uint *,int,double *,double *) ;
extern int	flbsi(uint) ;


/* external variables */


/* local structures */


/* forward references */

static double getdenov(uint *,int) ;


/* global variables */


/* exported subroutines */


int regstats_init(op,lenlife, lenuse,lentab)
REGSTATS	*op ;
int		lenuse ;
int		lenlife ;
int		lentab ;
{
	int	rs ;
	int	max ;
	int	size ;


	if (op == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(REGSTATS)) ;

/* density tables */

	if (lenuse < 0)
	    lenuse = REGSTATS_DEFUSELEN ;

	if (lenlife < 0)
	    lenlife = REGSTATS_DEFLIFELEN ;

	op->lenuse = lenuse ;
	op->lenlife = lenlife ;
	max = MAX(lenuse,lenlife) ;

#if	CF_DEBUGS
	debugprintf("regstats_init: lenlife=%d lenuse=%d max=%d\n",
	    lenlife,lenuse,max) ;
#endif

	size = 3 * max * sizeof(uint) ;
	rs = uc_malloc(size,&op->den) ;

	if (rs < 0)
	    goto bad1 ;

	(void) memset(op->den,0,size) ;

	op->denrint = op->den + (0 * max) ;
	op->denlife = op->den + (1 * max) ;
	op->denuse = op->den + (2 * max) ;

/* tracking tables */

	if (lentab < 0)
	    lentab = REGSTATS_DEFTABLEN ;

	op->lentab = lentab ;
	size = 4 * op->lentab * sizeof(uint) ;
	rs = uc_malloc(size,&op->atrack) ;

	if (rs < 0)
	    goto bad2 ;

	(void) memset(op->atrack,0,size) ;

	op->rtrack = op->atrack + (0 * op->lentab) ;
	op->wtrack = op->atrack + (1 * op->lentab) ;
	op->rread = op->atrack + (2 * op->lentab) ;
	op->rwrite = op->atrack + (3 * op->lentab) ;

	op->magic = REGSTATS_MAGIC ;
	return rs ;

/* bad stuff */
bad2:
	free(op->den) ;

bad1:

bad0:
	return rs ;
}
/* end subroutine (regstats_init) */


/* call this at the start of i-execution for register reads */
int regstats_read(op,in,f_se,a,v)
REGSTATS	*op ;
uint		in ;
int		f_se ;
uint		a, v ;
{
	int	diff, diff_read, diff_write ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (a >= op->lentab)
	    return SR_NOENT ;

#if	CF_DEBUGS
	debugprintf("regstats_read: a=%u rtrack=%u wtrack=%u\n",
	    a,op->rtrack[a],op->wtrack[a]) ;
#endif

	if (f_se) {

#if	CF_DEBUGS
	    debugprintf("regstats_read: gathering a=%u\n",a) ;
#endif

/* reguser usage */

	    op->rread[a] += 1 ;

/* read density */

	    diff_read = in - op->rtrack[a] ;
	    diff_write = in - op->wtrack[a] ;
	    diff = MIN(diff_read,diff_write) ;
	    if (diff >= op->lenuse)
	        diff = op->lenuse - 1 ;

	    op->denrint[diff] += 1 ;

	} /* end if (statistics selected) */

	if (! f_se)
	    return SR_OK ;

	if ((a == 0) || (a == 28))
	    return SR_OK ;

/* update def-use density */

	diff = in - op->wtrack[a] ;
	if (diff >= op->lenuse)
	    diff = op->lenuse - 1 ;

#if	CF_DEBUGS
	debugprintf("regstats_read: a=%u def-use=%u\n",a,diff) ;
#endif

	op->denuse[diff] += 1 ;

	return diff ;
}
/* end subroutine (regstats_read) */


/* call this after i-execution but before you call 'regstats_write()' */
int regstats_readupdate(op,in,f_se,a,v)
REGSTATS	*op ;
uint		in ;
int		f_se ;
uint		a, v ;
{


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (a >= op->lentab)
	    return SR_NOENT ;

/* update read tracking */

	op->rtrack[a] = in ;

	return 0 ;
}
/* end subroutine (regstats_readupdate) */


/* call this when a write to a register occurs */
int regstats_write(op,in,f_se,a,v)
REGSTATS	*op ;
uint		in ;
int		f_se ;
uint		a, v ;
{
	int	diff ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (a >= op->lentab)
	    return SR_NOENT ;

	if (f_se)
	    op->rwrite[a] += 1 ;

#ifdef	OPTIONAL /* will never happen ! */
	if (a == 0)
	    return SR_OK ;
#endif

#if	CF_DEBUGS
	debugprintf("regstats_write: a=%u rtrack=%u wtrack=%u\n",
	    a,op->rtrack[a],op->wtrack[a]) ;
#endif

	diff = (op->rtrack[a] - op->wtrack[a]) ;
	if (diff < 0)
	    diff = 0 ;

	if (f_se && (op->wtrack[a] != 0)) {

	    if (diff >= op->lenlife)
	        diff = op->lenlife - 1 ;

#if	CF_DEBUGS
	    debugprintf("regstats_write: a=%u diff=%u\n",a,diff) ;
#endif

	    op->denlife[diff] += 1 ;

	} /* end if (selected and not first write) */

	op->wtrack[a] = in ;
	return diff ;
}
/* end subroutine (regstats_write) */


/* call this when the simulator is done executing instructions */
int regstats_writedone(op,in,f_se)
REGSTATS	*op ;
uint		in ;
int		f_se ;
{
	int	diff ;
	int	a ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (f_se) {

	    for (a = 0 ; a < op->lentab ; a += 1) {

	        if (op->wtrack[a] > 0) {

	            diff = (op->rtrack[a] - op->wtrack[a]) ;
	            if (diff > 0) {

	                if (diff >= op->lenlife)
	                    diff = op->lenlife - 1 ;

#if	CF_DEBUGS
	                debugprintf("regstats_writedone: a=%u diff=%u\n",a,diff) ;
#endif

	                op->denlife[diff] += 1 ;

	            } /* end if (we had some extra reads) */

	        } /* end if (register was written) */

	    } /* end for */

	} /* end if (selection was active) */

	return 0 ;
}
/* end subroutine (regstats_writedone) */


int regstats_free(op)
REGSTATS	*op ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;

	if (op->atrack != NULL)
	    free(op->atrack) ;

	if (op->den != NULL)
	    free(op->den) ;

	op->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (regstats_free) */


/* return some statistics to the user */
int regstats_stats(op,sp)
REGSTATS	*op ;
REGSTATS_STATS	*sp ;
{
	int	rs, c = 0 ;

	double	mean, var ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;

	if (sp == NULL)
	    return SR_FAULT ;

	(void) memset(sp,0,sizeof(REGSTATS_STATS)) ;

	sp->rint_mean = sp->rint_var = sp->rint_ov = 0.0 ;
	sp->life_mean = sp->life_var = sp->life_ov = 0.0 ;
	sp->use_mean = sp->use_var = sp->use_ov = 0.0 ;

/* read intervals */

	rs = fmeanvarai(op->denrint,op->lenuse,&mean,&var) ;

#if	CF_DEBUGS
	debugprintf("regstats_stats: read rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    return rs ;

	sp->rint_mean = mean ;
	sp->rint_var = var ;
	sp->rint_ov = getdenov(op->denrint,op->lenuse) ;

/* lifetimes */

	rs = fmeanvarai(op->denlife,op->lenlife,&mean,&var) ;

#if	CF_DEBUGS
	debugprintf("regstats_stats: life rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    return rs ;

	sp->life_mean = mean ;
	sp->life_var = var ;
	sp->life_ov = getdenov(op->denlife,op->lenlife) ;

/* def-uses */

	rs = fmeanvarai(op->denuse,op->lenuse,&mean,&var) ;

#if	CF_DEBUGS
	debugprintf("regstats_stats: def-use rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    return rs ;

	sp->use_mean = mean ;
	sp->use_var = var ;
	sp->use_ov = getdenov(op->denuse,op->lenuse) ;

	return (rs >= 0) ? c : rs ;
}
/* end subroutine (regstats_stats) */


int regstats_storefiles(op,rintfname,lifefname,usefname,readfname,writefname)
REGSTATS	*op ;
char		rintfname[] ;
char		lifefname[] ;
char		usefname[] ;
char		readfname[] ;
char		writefname[] ;
{
	bfile	outfile ;

	int	rs = SR_OK, i ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;

/* read-interval density */

#if	CF_DEBUGS
	debugprintf("regstats_storefiles: rintfname=%s\n",rintfname) ;
#endif

	if ((rintfname != NULL) && (rintfname[0] != '\0') &&
	    ((rs = bopen(&outfile,rintfname,"wct",0666)) >= 0)) {

#if	CF_DEBUGS
	    debugprintf("regstats_storefiles: bopen() rs=%d\n",rs) ;
#endif

	    for (i = 0 ; i < op->lenuse ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->denrint[i]) ;

	    bclose(&outfile) ;

	} /* end if (life density) */

#if	CF_DEBUGS
	debugprintf("regstats_storefiles: 2 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* life density */

#if	CF_DEBUGS
	debugprintf("regstats_storefiles: lifefname=%s\n",lifefname) ;
#endif

	if ((lifefname != NULL) && (lifefname[0] != '\0') &&
	    ((rs = bopen(&outfile,lifefname,"wct",0666)) >= 0)) {

#if	CF_DEBUGS
	    debugprintf("regstats_storefiles: bopen() rs=%d\n",rs) ;
#endif

	    for (i = 0 ; i < op->lenlife ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->denlife[i]) ;

	    bclose(&outfile) ;

	} /* end if (life density) */

#if	CF_DEBUGS
	debugprintf("regstats_storefiles: 2 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* use density */

	if ((usefname != NULL) && (usefname[0] != '\0') &&
	    ((rs = bopen(&outfile,usefname,"wct",0666)) >= 0)) {

	    for (i = 0 ; i < op->lenuse ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->denuse[i]) ;

	    bclose(&outfile) ;

	} /* end if (use density) */

#if	CF_DEBUGS
	debugprintf("regstats_storefiles: 1 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* read usage */

	if ((readfname != NULL) && (readfname[0] != '\0') &&
	    ((rs = bopen(&outfile,readfname,"wct",0666)) >= 0)) {

	    for (i = 0 ; i < op->lentab ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->rread[i]) ;

	    bclose(&outfile) ;

	} /* end if (read density) */

#if	CF_DEBUGS
	debugprintf("regstats_storefiles: 3 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* write usage */

	if ((writefname != NULL) && (writefname[0] != '\0') &&
	    ((rs = bopen(&outfile,writefname,"wct",0666)) >= 0)) {

	    for (i = 0 ; i < op->lentab ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->rwrite[i]) ;

	    bclose(&outfile) ;

	} /* end if (write density) */

#if	CF_DEBUGS
	debugprintf("regstats_storefiles: 4 rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (regstats_storefiles) */



/* PRIVATE SUBROUTINES */



/* get the overflow percentage on this density */
static double getdenov(density,lenden)
uint	density[] ;
int	lenden ;
{
	uint	c = 0 ;

	int	i ;

	double	ov ;


	for (i = 0 ; i < lenden ; i += 1)
		c += density[i] ;

	ov = 100.0 * ((double) density[lenden - 1]) / ((double) c) ;

	return ov ;
}
/* end subroutine (getdenov) */


