/* procfilepaths SUPPORT (leveosim-frame) */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* process a paths file */
/* version %I% last modified %G% */

#define	CF_DEBUGS	0

/* revision history:

	= 1994-09-10, Dave Morano
	This program was originally written.

*/

/* Copyright © 1994 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/*****************************************************************************

  	Description:
	This subroutine will read (process) a file that has
	directory paths in it.

*****************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>		/* |getenv(3c)| */
#include	<cstring>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<bio.h>
#include	<field.h>
#include	<vecstr.h>
#include	<vstrxcmp.h>		/* |vstrkeycmp(3uc)| */
#include	<char.h>
#include	<localmisc.h>


/* local defines */

#undef	BUFLEN
#define	BUFLEN		(4 * MAXPATHLEN)
#undef	LINELEN
#define	LINELEN		(2 * MAXPATHLEN)
#define	PATHBUFLEN	(30 * MAXPATHLEN)


/* external subroutines */


/* externals variables */


/* local structures */


/* forward references */


/* local variables */

constexpr cpcchar	fterms[] = {
	0x00, 0x00, 0x00, 0x00,
	0x09, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
} ;


/* exported variables */


/* exported subroutines */

int procfilepaths(programroot,fname,lp)
char	programroot[] ;
char	fname[] ;
VECSTR	*lp ;
{
	bfile	file, *fp = &file ;

	FIELD	fsb ;

	int	len, i, rs ;
	int	pathbuflen, psi, pbi ;

	char	linebuf[LINELEN + 1] ;
	char	buf[BUFLEN + 1], *bp ;
	char	pathbuf[PATHBUFLEN + 1], *pp ;

#if	CF_DEBUGS
	char	outname[MAXPATHLEN + 1] ;
#endif


#if	CF_DEBUGS
	    eprintf("procfilepaths: entered=%s\n",fname) ;

	if ((rs = bopenroot(fp,programroot,fname,outname,"r",0666)) < 0) {

	        eprintf("procfilepaths: bopen rs=%d\n",rs) ;

	    return rs ;
	}

	    eprintf("procfilepaths: file=%s\n",outname) ;
#else
	if ((rs = bopenroot(fp,programroot,fname,NULL,"r",0666)) < 0)
	    return rs ;
#endif

#if	CF_DEBUGS
	    eprintf("procfilepaths: opened\n") ;
#endif

/* get the PATH variable as it exists now, if it exists */

	psi = -1 ;
	pbi = 0 ;
	pathbuf[0] = '\0' ;
	if ((rs = vecstr_finder(lp,"PATH",vstrkeycmp,&pp)) >= 0) {

	    psi = rs ;
	    pbi += storebuf_buf(pathbuf,PATHBUFLEN,pbi,pp,-1) ;

#if	CF_DEBUGS
	        eprintf("procfilepaths: existing %s\n",
	            pathbuf) ;
#endif

	}


/* read the file and process any paths that we find */

	i = 0 ;
	while ((len = bgetline(fp,linebuf,LINELEN)) > 0) {

#if	CF_DEBUGS
	        eprintf("procfilepaths: line> %W\n",linebuf,len - 1) ;
#endif

	    fsb.lp = linebuf ;
	    fsb.rlen = (linebuf[len - 1] == '\n') ? (len - 1) : len ;
	    field_sharg(&fsb,fterms,buf,BUFLEN) ;

	    if (fsb.flen < 0) continue ;

	    if ((fsb.flen == 0) && (fsb.term == '#')) continue ;

#if	CF_DEBUGS
	        eprintf("procfilepaths: flen=%d\n",fsb.flen) ;
#endif

#if	CF_DEBUGS
	        eprintf("procfilepaths: 1 field> %W\n",fsb.fp,fsb.flen) ;
#endif

	    while ((fsb.flen > 1) && (fsb.fp[fsb.flen - 1] == '/'))
	        fsb.flen -= 1 ;

	    fsb.fp[fsb.flen] = '\0' ;
	    if (pbi > 0) {

	        if (strnvaluecmp(pathbuf,fsb.fp,fsb.flen) != 0) {

#if	CF_DEBUGS
	                eprintf("procfilepaths: adding %W\n",
				fsb.fp,fsb.flen) ;
#endif

	            i += 1 ;
	            pbi += storebuf_char(pathbuf,PATHBUFLEN,pbi,':') ;

	            pbi += storebuf_buf(pathbuf,PATHBUFLEN,pbi,
				fsb.fp,fsb.flen) ;

	        }

	    } else {

	        pbi += storebuf_buf(pathbuf,PATHBUFLEN,pbi,"PATH=",5) ;

	        i += 1 ;
	        pbi += storebuf_buf(pathbuf,PATHBUFLEN,pbi,fsb.fp,fsb.flen) ;

	    }

	} /* end while */

	bclose(fp) ;

	if (i > 0) {

	    if (psi >= 0)
	        vecstr_del(lp,psi) ;

	    vecstr_add(lp,pathbuf,pbi) ;

	}

#if	CF_DEBUGS
	    eprintf("procfilepaths: exiting rs=%d\n",
	        ((len < 0) ? len : i)) ;
#endif

	return ((len < 0) ? len : i) ;
}
/* end subroutine (procfilepaths) */



