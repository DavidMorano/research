/* procenv SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* process an environment file */
/* version %I% last modified %G% */

#define	CF_DEBUGS	0		/* compile-time debugging */

/* revision history:

	= 1994-09-10, David Morano
	This program was originally written.

*/

/* Copyright © 1994 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

  	Description:
	This subroutine will read (process) an environment file and
	put all of the environment variables into the string list
	(supplied).  New environment variables just get added to
	the list.  Old environment variables already on the list
	are deleted when a new definition is encountered.

	Synopsis:
	int procenv(programroot,fname,lp)
	cchar	programroot[] ;
	cchar	fname[] ;
	VECSTR		*lp ;

	Arguments:
	programroot	program root
	fname		filename to process
	lp		resulting list of environment variables

	Returns:
	>=0		number of environment variables
	<0		error (system-error)

*******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<strings.h>
#include	<unistd.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>		/* |getenv(3c)| */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<bfile.h>
#include	<field.h>
#include	<vecstr.h>
#include	<vstrxcmp.h>		/* |vstrkeycmp(3uc)| */
#include	<char.h>
#include	<localmisc.h>


/* local defines */

#define	BUFLEN		(4 * MAXPATHLEN)

#ifndef	LINEBUFLEN
#define	LINEBUFLEN	MAX((2 * MAXPATHLEN),2048)
#endif


/* external subroutines */

extern int	bopenroot(bfile *,cchar *,cchar *,
			char *,cchar *,mode_t) noex ;


/* external variables */


/* forward references */


/* local structures */


/* local variables */

constexpr char		fterms[] = {
	0x00, 0x00, 0x00, 0x00,
	0x09, 0x00, 0x00, 0x20,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
} ;


/* exported variables */


/* exported subroutines */

int procenv(programroot,fname,lp)
cchar	programroot[] ;
cchar	fname[] ;
VECSTR		*lp ;
{
	bfile	efile, *efp = &efile ;
	cint	llen = MAXLINELEN ;
	int	rs, rs1 ;
	int	bl, cl ;
	int	fl ;
	int	n = 0 ;

	cchar	*fp ;
	cchar	*cp ;

	char	lbuf[llen + 1] ;
	char	buf[BUFLEN + 1], *bp ;


	if ((fname == NULL) || (fname[0] == '\0'))
	    return SR_INVALID ;

	if ((rs = bopenroot(efp,programroot,fname,NULL,"r",0666)) >= 0) {

	while ((rs = breadln(efp,lbuf,llen)) > 0) {
	    len = rs ;

	    if (lbuf[len - 1] == '\n') len -= 1 ;
	    lbuf[len] = '\0' ;

	    cp = lbuf ;
	    cl = len ;
	    while ((cl > 0) && CHAR_ISWHITE(*cp)) {
	        cp += 1 ;
	        cl -= 1 ;
	    }

	    if ((cp[0] == '\0') || (cp[0] == '#'))
	        continue ;

	    field fsb ;
	    if ((rs = field_start(&fsb,cp,cl)) >= 0) {

	        fl = field_get(&fsb,fterms,&fp) ;

	        if (fl > 0) {

	            if ((strncasecmp("export",fp,fl) == 0) &&
	                (fl == 6)) {

	                fl = field_get(&fsb,fterms,&fp) ;

	            }

	            bp = buf ;
	            bp = strwcpy(bp,fp,MIN(fl,BUFLEN)) ;

	            if (bp < (buf + BUFLEN - 1))
	                *bp++ = '=' ;

	            bl = buf + BUFLEN - bp ;
		    fp = bp ;
	            fl = field_sharg(&fsb,fterms,bp,bl) ;

	            if (fl > 0) {
	                bp += fl ;
		    } else
	                bp -= 1 ;

	            *bp = '\0' ;
	            rs1 = vecstr_finder(lp,buf,vstrkeycmp,NULL) ;
		    if (rs >= 0)
	                vecstr_del(lp,rs1) ;

		    n += 1 ;
	            rs = vecstr_add(lp,buf,(bp - buf)) ;
	            if (rs < 0)
	                break ;

	        } /* end if (got a keyname) */

	        field_finish(&fsb) ;
	    } /* end if (fields) */

	} /* end while (reading lines) */

	bclose(efp) ;
	} /* end if (file-open) */

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (procenv) */


