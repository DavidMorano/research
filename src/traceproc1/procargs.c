/* procargs */

/* process a program arguments file */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0		/* compile-time debugging */


/* revision history:

	= 1998-09-10, David Morano

	This program was originally written.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine will read (process) a file with program
	arguments in it.


*******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>

#include	<bfile.h>
#include	<field.h>
#include	<vecstr.h>
#include	<localmisc.h>


/* local defines */

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif

#undef	FBUFLEN
#define	FBUFLEN		(4 * MAXPATHLEN)


/* external subroutines */

extern int	bopenroot(bfile *,const char *,const char *,
			char *,const char *,mode_t) ;

extern int	vstrkeycmp(char **,char **) ;

extern char	*strwcpy(char *,const char *,int) ;


/* external variables */


/* local structures */


/* forward references */


/* local variables */

static const uchar	fterms[32] = {
	0x00, 0x00, 0x00, 0x00,
	0x09, 0x00, 0x00, 0x20,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
} ;


/* exported subroutines */


int procargs(programroot,fname,lp)
const char	programroot[] ;
const char	fname[] ;
VECSTR		*lp ;
{
	bfile	efile, *efp = &efile ;

	FIELD	fsb ;

	int	rs ;
	int	len ;
	int	fl ;
	int	n = 0 ;

	const char	*fp ;

	char	linebuf[LINEBUFLEN + 1] ;
	char	fbuf[FBUFLEN + 1] ;


#if	CF_DEBUGS
	debugprintf("procargs: entered fname=%s\n",fname) ;
#endif

	rs = bopenroot(efp,programroot,fname,NULL,"r",0666) ;
	if (rs < 0)
	    goto ret0 ;

#if	CF_DEBUGS
	debugprintf("procargs: opened\n") ;
#endif

	n = 0 ;
	while ((rs = breadline(efp,linebuf,LINEBUFLEN)) > 0) {

	    len = rs ;
	    if (linebuf[len - 1] == '\n')
		len -= 1 ;

#if	CF_DEBUGS
	    debugprintf("procargs: line> %t\n",linebuf,len) ;
#endif

	    if (linebuf[0] == '\0')
		continue ;

	    if ((rs = field_start(&fsb,linebuf,len)) >= 0) {

	        while (rs >= 0) {

	            fp = fbuf ;
	            fl = field_sharg(&fsb,fterms,fbuf,FBUFLEN) ;
	            if (fl < 0)
	                break ;

#if	CF_DEBUGS
	        debugprintf("procargs: 3 field> %t\n",fp,fl) ;
#endif

		    n += 1 ;
	            rs = vecstr_add(lp,fp,fl) ;

	            if (fsb.term == '#')
	                break ;

	        } /* end while (reading fields) */

		field_finish(&fsb);
	    } /* end if */

	} /* end while (reading lines) */

	bclose(efp) ;

ret0:

#if	CF_DEBUGS
	debugprintf("procargs: ret rs=%d\n",rs) ;
#endif

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (procargs) */


