/* regstats */

/* maintain register statistics */
/* last modified %G% version %I% */


#define	CF_DEBUGS	0		/* compile-time debug print-outs */
#define	CF_SAFE		1		/* safer operation */
#define	CF_CHECK	1		/* perform some sanity checks */


/* revision history:

	= 02/08/21, Dave Morano

	This program was originally written.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/**************************************************************************

	This object module tracks certain statistics about the
	use of registers.


**************************************************************************/


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<cstdlib>
#include	<cstring>
#include	<ctype.h>
#include	<assert.h>

#if	defined(IRIX)
#include	<standards.h>
#endif

#include	<vsystem.h>
#include	<mallocstuff.h>
#include	<bio.h>

#include	"localmisc.h"
#include	"regstats.h"



/* local defines */

#define	REGSTATS_MAGIC		0x98726514
#define	REGSTATS_LENDEN		(64 * 1024)
#define	REGSTATS_LENTRACK	128

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
extern int	fmeanvaral(ULONG *,int,double *,double *) ;
extern int	flbsi(uint) ;


/* external variables */


/* local structures */


/* forward references */

static int	writearray(const char *,ULONG *,int) ;

static ULONG tabentries(ULONG *,uint) ;

static double getdenov(ULONG *,int) ;


/* global variables */








int regstats_init(op,lentrack,lenden)
REGSTATS	*op ;
int		lentrack ;
int		lenden ;
{
	int	rs, n, i ;
	int	size ;


	if (op == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(REGSTATS)) ;

	op->sumrint = 0.0 ;
	op->sumuse = 0.0 ;
	op->sumlife = 0.0 ;

	if (lenden <= 0)
	    lenden = REGSTATS_LENDEN ;

	if (lentrack < REGSTATS_LENTRACK)
	    lentrack = REGSTATS_LENTRACK ;

#if	CF_DEBUGS
	eprintf("regstats_init: lenden=%d\n",
	    lenden) ;
#endif
	op->lenden = lenden ;
	op->lentrack = lentrack ;

/* density tables */

	n = 3 ;
	size = n * op->lenden * sizeof(ULONG) ;
	rs = uc_malloc(size,&op->den) ;

	if (rs < 0)
	    goto bad1 ;

	(void) memset(op->den,0,size) ;

	i = 0 ;
	op->denrint = op->den + (i++ * op->lenden) ;
	op->denuse = op->den + (i++ * op->lenden) ;
	op->denlife = op->den + (i++ * op->lenden) ;
	if (i != n)
	    goto bad2 ;

/* tracking tables */

	n = 6 ;
	size = n * op->lentrack * sizeof(ULONG) ;
	rs = uc_malloc(size,&op->track) ;

	if (rs < 0)
	    goto bad2 ;

	(void) memset(op->track,0,size) ;

	i = 0 ;
	op->tread = op->track + (i++ * op->lentrack) ;
	op->twrite = op->track + (i++ * op->lentrack) ;
	op->iread = op->track + (i++ * op->lentrack) ;
	op->iwrite = op->track + (i++ * op->lentrack) ;
	op->svalue = op->track + (i++ * op->lentrack) ;
	op->swrite = op->track + (i++ * op->lentrack) ;
	if (i != n)
	    goto bad3 ;

	op->magic = REGSTATS_MAGIC ;

ret0:
	return rs ;

/* bad stuff */
bad3:
	free(op->track) ;

bad2:
	free(op->den) ;

bad1:
bad0:
	goto ret0 ;
}
/* end subroutine (regstats_init) */


/* call this after i-decode but before i-execution */
int regstats_read(op,in,f_se,a,v)
REGSTATS	*op ;
ULONG		in ;
int		f_se ;
ULONG		a, v ;
{
	LONG	diff, diff_read, diff_write ;

	int	rs = SR_OK ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (a >= op->lentrack) {
	    rs = SR_NOENT ;
	    goto ret0 ;
	}

#if	CF_DEBUGS
	eprintf("regstats_read: in=%llu f_se=%u\n",in,f_se) ;
	eprintf("regstats_read: a=%llu tread=%llu twrite=%llu\n",
	    a,op->tread[a],op->twrite[a]) ;
#endif

	if (f_se) {

		op->in = in ;
		if (! op->f.started) {
			op->f.started = TRUE ;
			op->in_start = in ;
		}

	}

	if (f_se && (op->twrite[a] != 0)) {

#if	CF_DEBUGS
	    eprintf("regstats_read: gathering a=%llu\n",a) ;
#endif

/* reguser usage */

	    op->iread[a] += 1 ;		/* individual read */

/* read density */

	    diff_read = in - op->tread[a] ;
	    diff_write = in - op->twrite[a] ;
	    diff = MIN(diff_read,diff_write) ;
	    op->sumrint += diff ;
	    if (diff >= op->lenden)
	        diff = op->lenden - 1 ;

#if	CF_DEBUGS
	    eprintf("regstats_read: a=%llu rint=%llu\n",a,diff) ;
#endif

	    op->denrint[diff] += 1 ;

/* update def-use density */

	    diff = in - op->twrite[a] ;
	    op->sumuse += diff ;
	    if (diff >= op->lenden)
	        diff = op->lenden - 1 ;

#if	CF_DEBUGS
	    eprintf("regstats_read: a=%llu def-use=%llu\n",a,diff) ;
#endif

	    op->denuse[diff] += 1 ;

	} /* end if (statistics selected) */

ret0:
	return rs ;
}
/* end subroutine (regstats_read) */


/* call this after i-execution but before you call 'regstats_write()' */
int regstats_readupdate(op,in,f_se,a,v)
REGSTATS	*op ;
ULONG		in ;
int		f_se ;
ULONG		a, v ;
{
	int	rs = SR_OK ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (a >= op->lentrack) {
	    rs = SR_NOENT ;
	    goto ret0 ;
	}

#if	CF_DEBUGS
	eprintf("regstats_readupdate: in=%llu\n",in) ;
#endif

#if	CF_CHECK
	if (f_se)
	    op->c_read += 1 ;
#endif

/* update read address tracking */

	if (f_se)
	    op->tread[a] = in ;

ret0:
	return rs ;
}
/* end subroutine (regstats_readupdate) */


/* call this when a write to a register occurs */
int regstats_write(op,in,f_se,a,v)
REGSTATS	*op ;
ULONG		in ;
int		f_se ;
ULONG		a, v ;
{
	LONG	diff ;

	int	rs = SR_OK ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (a >= op->lentrack) {
	    rs = SR_NOENT ;
	    goto ret0 ;
	}

#if	CF_DEBUGS
	eprintf("regstats_write: in=%llu\n",in) ;
#endif

	if (f_se) {

#if	CF_CHECK
	    op->c_write += 1 ;
#endif

	    op->iwrite[a] += 1 ;	/* individual write */

	    if (v == op->svalue[a])
	        op->swrite[a] += 1 ;

	    else
	        op->svalue[a] = v ;

	}

#if	CF_DEBUGS
	eprintf("regstats_write: a=%llu tread=%llu twrite=%llu\n",
	    a,op->tread[a],op->twrite[a]) ;
#endif

	if (f_se && (op->twrite[a] != 0)) {

	    diff = (op->tread[a] - op->twrite[a]) ;
	    if (diff < 0)
	        diff = 0 ;

	    op->sumlife += diff ;
	    if (diff >= op->lenden)
	        diff = op->lenden - 1 ;

#if	CF_DEBUGS
	    eprintf("regstats_write: a=%llu life=%llu\n",a,diff) ;
#endif

	    op->denlife[diff] += 1 ;

	} /* end if (selected and not first write) */

/* update write address tracking */

	if (f_se)
	    op->twrite[a] = in ;

ret0:
	return rs ;
}
/* end subroutine (regstats_write) */


/* call this when the simulator is past the sample window */
int regstats_writedone(op,in,f_se)
REGSTATS	*op ;
ULONG		in ;
int		f_se ;
{
	LONG	diff ;

	int	i ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGS
	eprintf("regstats_writedone: in=%llu\n",in) ;
#endif

	if (f_se) {

	    for (i = 0 ; i < op->lentrack ; i += 1) {

	        if (op->twrite[i] > 0) {

	            diff = (op->tread[i] - op->twrite[i]) ;
	            if (diff > 0) {

	                op->sumlife += diff ;
	                if (diff >= op->lenden)
	                    diff = op->lenden - 1 ;

#if	CF_DEBUGS
	                eprintf("regstats_writedone: i=%u diff=%llu\n",
	                    i,diff) ;
#endif

	                op->denlife[diff] += 1 ;

	            } /* end if (we had some extra reads) */

	        } /* end if (register was written) */

	    } /* end for */

	} /* end if (selection was active) */

	return SR_OK ;
}
/* end subroutine (regstats_writedone) */


int regstats_free(op)
REGSTATS	*op ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;

	if (op->track != NULL)
	    free(op->track) ;

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
	ULONG	tes ;

	int	rs, c = 0 ;

	double	mean, var ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;

	if (sp == NULL)
	    return SR_FAULT ;

	(void) memset(sp,0,sizeof(REGSTATS_STATS)) ;

/* instructions */

	if (op->f.started) {
	    sp->in_start = op->in_start ;
	    sp->ins = op->in - op->in_start ;
	}

/* the tough data */

	sp->lenden = op->lenden ;
	sp->lentrack = op->lentrack ;

	sp->rint_mean = sp->rint_var = sp->rint_ov = 0.0 ;
	sp->life_mean = sp->life_var = sp->life_ov = 0.0 ;
	sp->use_mean = sp->use_var = sp->use_ov = 0.0 ;

	sp->reads = op->c_read ;
	sp->writes = op->c_write ;

/* read intervals */

	rs = fmeanvaral(op->denrint,op->lenden,&mean,&var) ;

#if	CF_DEBUGS
	eprintf("regstats_stats: read rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    goto ret0 ;

	if ((tes = tabentries(op->denrint,op->lenden)) > 0)
	    sp->rint_amean = op->sumrint / tes ;

	sp->rint_mean = mean ;
	sp->rint_var = var ;
	sp->rint_ov = getdenov(op->denrint,op->lenden) ;

/* def-uses */

	rs = fmeanvaral(op->denuse,op->lenden,&mean,&var) ;

#if	CF_DEBUGS
	eprintf("regstats_stats: def-use rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    goto ret0 ;

	if ((tes = tabentries(op->denuse,op->lenden)) > 0)
	    sp->use_amean = op->sumuse / tes ;

	sp->use_mean = mean ;
	sp->use_var = var ;
	sp->use_ov = getdenov(op->denuse,op->lenden) ;

/* lifetimes */

	rs = fmeanvaral(op->denlife,op->lenden,&mean,&var) ;

#if	CF_DEBUGS
	eprintf("regstats_stats: life rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    goto ret0 ;

	if ((tes = tabentries(op->denlife,op->lenden)) > 0)
	    sp->life_amean = op->sumlife / tes ;

	sp->life_mean = mean ;
	sp->life_var = var ;
	sp->life_ov = getdenov(op->denlife,op->lenden) ;

/* done */
ret0:
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
	eprintf("regstats_storefiles: rintfname=%s\n",rintfname) ;
#endif

	if ((rs >= 0) && (rintfname != NULL) && (rintfname[0] != '\0'))
	    rs = writearray(rintfname,op->denrint,op->lenden) ;

/* life density */

#if	CF_DEBUGS
	eprintf("regstats_storefiles: lifefname=%s\n",lifefname) ;
#endif

	if ((rs >= 0) && (lifefname != NULL) && (lifefname[0] != '\0'))
	    rs = writearray(lifefname,op->denlife,op->lenden) ;

/* use density */

	if ((rs >= 0) && (usefname != NULL) && (usefname[0] != '\0'))
	    rs = writearray(usefname,op->denuse,op->lenden) ;

/* read usage */

	if ((rs >= 0) && (readfname != NULL) && (readfname[0] != '\0'))
	    rs = writearray(readfname,op->iread,op->lentrack) ;

/* write usage */

	if ((rs >= 0) && (writefname != NULL) && (writefname[0] != '\0'))
	    rs = writearray(writefname,op->iwrite,op->lentrack) ;

ret0:

#if	CF_DEBUGS
	eprintf("regstats_storefiles: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (regstats_storefiles) */



/* PRIVATE SUBROUTINES */



static int writearray(fname,a,alen)
const char	fname[] ;
ULONG		a[] ;
int		alen ;
{
	bfile	outfile ;

	int	rs ;
	int	i ;


	if (fname == NULL)
		return SR_FAULT ;

	if (a == NULL)
		return SR_FAULT ;

	rs = bopen(&outfile,fname,"wct",0666) ;

	if (rs >= 0) {

	    for (i = 0 ; i < alen ; i += 1)
	        bprintf(&outfile,"%12d %12llu\n",i,a[i]) ;

	    bclose(&outfile) ;

	} /* end if */

	return rs ;
}
/* end subroutine (writearray) */


/* get the total number of entries in a density table */
static ULONG tabentries(tab,lentab)
ULONG	tab[] ;
uint	lentab ;
{
	ULONG	sum ;

	uint	i ;


	sum = 0 ;
	for (i = 0 ; i < lentab ; i += 1)
	    sum += tab[i] ;

	return sum ;
}
/* end subroutine (tabentries) */


/* get the overflow percentage on this density */
static double getdenov(density,lenden)
ULONG	density[] ;
int	lenden ;
{
	ULONG	c = 0 ;

	int	i ;

	double	ov ;


	for (i = 0 ; i < lenden ; i += 1)
	    c += density[i] ;

	ov = 100.0 * ((double) density[lenden - 1]) / ((double) c) ;

	return ov ;
}
/* end subroutine (getdenov) */



