/* regstats */

/* maintain register statistics */
/* last modified %G% version %I% */


#define	F_DEBUGS	0
#define	F_SAFE		0


/* revision history :

	= 02/08/21, Dave Morano

	This program was originally written.


*/



/**************************************************************************

	This object module tracks certain statistics about the
	use of registers.



**************************************************************************/



#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>

#if	defined(IRIX)
#include	<standards.h>
#endif

#include	<vsystem.h>
#include	<mallocstuff.h>
#include	<bio.h>

#include	"misc.h"
#include	"regstats.h"



/* local defines */

#define	REGSTATS_MAGIC		0x98726514
#define	REGSTATS_DEFUSELEN	4096
#define	REGSTATS_DEFLIFELEN	4096
#define	REGSTATS_DEFTABLEN	256	/* number of registers */

#if	defined(IRIX)
#ifndef	F_ULOCK
#define F_ULOCK	0	/* Unlock a previously locked region */
#endif
#ifndef	F_LOCK
#define F_LOCK	1	/* Lock a region for exclusive use */
#endif
#ifndef	F_TLOCK
#define F_TLOCK	2	/* Test and lock a region for exclusive use */
#endif
#ifndef	F_TEST
#define F_TEST	3	/* Test a region for other processes locks */
#endif
#endif /* defined(IRIX) */



/* external subroutines */


/* external variables */


/* local structures */


/* forward references */


/* global variables */








int regstats_init(op,lifelen, uselen,tablen)
REGSTATS	*op ;
int		uselen ;
int		lifelen ;
int		tablen ;
{
	int	rs ;
	int	max ;
	int	size ;


	if (op == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(REGSTATS)) ;

/* density tables */

	if (uselen < 0)
	    uselen = REGSTATS_DEFUSELEN ;

	if (lifelen < 0)
	    lifelen = REGSTATS_DEFLIFELEN ;

	op->uselen = uselen ;
	op->lifelen = lifelen ;
	max = MAX(uselen,lifelen) ;

#if	F_DEBUGS
	eprintf("regstats_init: lifelen=%d uselen=%d max=%d\n",
	    lifelen,uselen,max) ;
#endif

	size = 3 * max * sizeof(uint) ;
	rs = uc_malloc(size,&op->den) ;

	if (rs < 0)
	    goto bad1 ;

	(void) memset(op->den,0,size) ;

	op->readden = op->den + (0 * max) ;
	op->lifeden = op->den + (1 * max) ;
	op->useden = op->den + (2 * max) ;

/* tracking tables */

	if (tablen < 0)
	    tablen = REGSTATS_DEFTABLEN ;

	op->tablen = tablen ;
	size = 4 * op->tablen * sizeof(uint) ;
	rs = uc_malloc(size,&op->atrack) ;

	if (rs < 0)
	    goto bad2 ;

	(void) memset(op->atrack,0,size) ;

	op->rtrack = op->atrack + (0 * op->tablen) ;
	op->wtrack = op->atrack + (1 * op->tablen) ;
	op->rread = op->atrack + (2 * op->tablen) ;
	op->rwrite = op->atrack + (3 * op->tablen) ;

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


int regstats_read(op,in,f_se,a,v)
REGSTATS	*op ;
uint		in ;
int		f_se ;
uint		a, v ;
{
	uint	diff ;


#if	F_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (a >= op->tablen)
	    return SR_NOENT ;

	if (f_se) {

#if	F_DEBUGS
	    eprintf("regstats_read: gathering a=%u\n",a) ;
#endif

/* reguser usage */

	    op->rread[a] += 1 ;

/* read density */

	    diff = in - op->rtrack[a] ;
	    if (diff >= op->uselen)
	        diff = op->uselen - 1 ;

	    op->readden[diff] += 1 ;

	} /* end if (statistics selected) */

/* update read tracking */

#ifdef	COMMENT
	op->rtrack[a] = in ;
#endif

	if (! f_se)
	    return SR_OK ;

	if ((a == 0) || (a == 28))
	    return SR_OK ;

/* update def-use density */

	diff = in - op->wtrack[a] ;
	if (diff >= op->uselen)
	    diff = op->uselen - 1 ;

#if	F_DEBUGS
	    eprintf("regstats_read: a=%u def-use=%u\n",a,diff) ;
#endif

	op->useden[diff] += 1 ;

	return diff ;
}
/* end subroutine (regstats_read) */


int regstats_readupdate(op,in,f_se,a,v)
REGSTATS	*op ;
uint		in ;
int		f_se ;
uint		a, v ;
{
	uint	diff ;


#if	F_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (a >= op->tablen)
	    return SR_NOENT ;

/* update read tracking */

	op->rtrack[a] = in ;

	return 0 ;
}
/* end subroutine (regstats_readupdate) */


int regstats_write(op,in,f_se,a,v)
REGSTATS	*op ;
uint		in ;
int		f_se ;
uint		a, v ;
{
	uint	diff ;


#if	F_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (a >= op->tablen)
	    return SR_NOENT ;

	if (f_se)
	    op->rwrite[a] += 1 ;

#ifdef	OPTIONAL /* will never happen ! */
	if ((a == 0) || (a == 28))
	    return SR_OK ;
#endif

	diff = in - op->wtrack[a] ;
	if (diff >= op->lifelen)
	    diff = op->lifelen - 1 ;

	if (f_se) {

#if	F_DEBUGS
	    eprintf("regstats_write: a=%u diff=%u\n",a,diff) ;
#endif

	    op->lifeden[diff] += 1 ;

	}

	op->wtrack[a] = in ;
	return diff ;
}
/* end subroutine (regstats_write) */


int regstats_free(op)
REGSTATS	*op ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != REGSTATS_MAGIC)
	    return SR_NOTOPEN ;

	if (op->atrack != NULL)
	    free(op->atrack) ;

	if (op->den != NULL)
	    free(op->den) ;

	op->magic = 0 ;
	return rs ;
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

	sp->read_mean = sp->read_var = 0.0 ;
	sp->life_mean = sp->life_var = 0.0 ;
	sp->use_mean = sp->use_var = 0.0 ;

/* read intervals */

	rs = fmeanvarai(op->readden,op->uselen,&mean,&var) ;

#if	F_DEBUGS
	eprintf("regstats_write: read rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    return rs ;

	sp->read_mean = mean ;
	sp->read_var = var ;

/* lifetimes */

	rs = fmeanvarai(op->lifeden,op->lifelen,&mean,&var) ;

#if	F_DEBUGS
	eprintf("regstats_write: write rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    return rs ;

	sp->life_mean = mean ;
	sp->life_var = var ;

/* use-defs */

	rs = fmeanvarai(op->useden,op->uselen,&mean,&var) ;

#if	F_DEBUGS
	eprintf("regstats_write: def-use rs=%d mean=%12.4f var=%12.4f\n",
	    rs, mean,var) ;
#endif

	if (rs < 0)
	    return rs ;

	sp->use_mean = mean ;
	sp->use_var = var ;

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

#if	F_DEBUGS
	eprintf("regstats_storefiles: rintfname=%s\n",rintfname) ;
#endif

	if ((rintfname != NULL) && (rintfname[0] != '\0') &&
	    ((rs = bopen(&outfile,rintfname,"wct",0666)) >= 0)) {

#if	F_DEBUGS
	    eprintf("regstats_storefiles: bopen() rs=%d\n",rs) ;
#endif

	    for (i = 0 ; i < op->uselen ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->readden[i]) ;

	    bclose(&outfile) ;

	} /* end if (life density) */

#if	F_DEBUGS
	eprintf("regstats_storefiles: 2 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* life density */

#if	F_DEBUGS
	eprintf("regstats_storefiles: lifefname=%s\n",lifefname) ;
#endif

	if ((lifefname != NULL) && (lifefname[0] != '\0') &&
	    ((rs = bopen(&outfile,lifefname,"wct",0666)) >= 0)) {

#if	F_DEBUGS
	    eprintf("regstats_storefiles: bopen() rs=%d\n",rs) ;
#endif

	    for (i = 0 ; i < op->lifelen ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->lifeden[i]) ;

	    bclose(&outfile) ;

	} /* end if (life density) */

#if	F_DEBUGS
	eprintf("regstats_storefiles: 2 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* use density */

	if ((usefname != NULL) && (usefname[0] != '\0') &&
	    ((rs = bopen(&outfile,usefname,"wct",0666)) >= 0)) {

	    for (i = 0 ; i < op->uselen ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->useden[i]) ;

	    bclose(&outfile) ;

	} /* end if (use density) */

#if	F_DEBUGS
	eprintf("regstats_storefiles: 1 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* read usage */

	if ((readfname != NULL) && (readfname[0] != '\0') &&
	    ((rs = bopen(&outfile,readfname,"wct",0666)) >= 0)) {

	    for (i = 0 ; i < op->tablen ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->rread[i]) ;

	    bclose(&outfile) ;

	} /* end if (read density) */

#if	F_DEBUGS
	eprintf("regstats_storefiles: 3 rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* write usage */

	if ((writefname != NULL) && (writefname[0] != '\0') &&
	    ((rs = bopen(&outfile,writefname,"wct",0666)) >= 0)) {

	    for (i = 0 ; i < op->tablen ; i += 1)
	        bprintf(&outfile,"%12d %12u\n",i,op->rwrite[i]) ;

	    bclose(&outfile) ;

	} /* end if (write density) */

#if	F_DEBUGS
	eprintf("regstats_storefiles: 4 rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (regstats_files) */



