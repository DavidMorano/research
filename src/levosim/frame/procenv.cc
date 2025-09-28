/* procenv */

/* process an environment file */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0


/* revision history :

	= 94/09/10, Dave Morano

	This program was originally written.


*/



/*****************************************************************************

	This subroutine will read (process) an environment file and put
	all of the environment variables into the string list
	(supplied).  New environment variables just get added to the
	list.  Old environment variables already on the list are
	deleted when a new definition is encountered.



*****************************************************************************/




#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>
#include	<bio.h>
#include	<field.h>
#include	<vecstr.h>

#include	"localmisc.h"




/* local defines */

#undef	BUFLEN
#define	BUFLEN		(4 * MAXPATHLEN)
#undef	LINELEN
#define	LINELEN		(2 * MAXPATHLEN)



/* external subroutines */

extern int	vstrkeycmp(char **,char **) ;

extern char	*strwcpy(char *,char *,int) ;


/* externals variables */


/* forward references */


/* local global variabes */


/* local structures */


/* local variables */

static const uchar	fterms[32] = {
	    0x00, 0x00, 0x00, 0x00,
	    0x09, 0x00, 0x00, 0x20,
	    0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00
} ;




int procenv(programroot,fname,lp)
char	programroot[] ;
char	fname[] ;
VECSTR	*lp ;
{
	bfile	efile, *fp = &efile ;

	FIELD	fsb ;

	int	len, n, rs ;

	char	linebuf[LINELEN + 1] ;
	char	buf[BUFLEN + 1], *bp ;


#if	CF_DEBUGS
	eprintf("procenv: entered fname=%s\n",fname) ;
#endif

	if ((fname == NULL) || (fname[0] == '\0'))
	    return SR_INVALID ;

	if ((rs = bopenroot(fp,programroot,fname,NULL,"r",0666)) < 0)
	    return rs ;

#if	CF_DEBUGS
	eprintf("procenv: opened\n") ;
#endif

	n = 0 ;
	while ((len = bgetline(fp,linebuf,LINELEN)) > 0) {

#if	CF_DEBUGS
	    eprintf("procenv: line> %W\n",linebuf,len - 1) ;
#endif

	    fsb.lp = linebuf ;
	    fsb.rlen = (linebuf[len - 1] == '\n') ? (len - 1) : len ;
	    field_get(&fsb,fterms) ;

	    if (fsb.flen <= 0) continue ;

#if	CF_DEBUGS
	    eprintf("procenv: 1 field> %W\n",fsb.fp,fsb.flen) ;
#endif

	    if (strncasecmp(fsb.fp,"export",fsb.flen) == 0)
	        field_get(&fsb,fterms) ;

#if	CF_DEBUGS
	    eprintf("procenv: 2 field> %W\n",fsb.fp,fsb.flen) ;
#endif

	    bp = buf ;
	    bp = strwcpy(bp,fsb.fp,MIN(fsb.flen,BUFLEN)) ;

	    if (bp < (buf + BUFLEN - 1))
	        *bp++ = '=' ;

	    field_sharg(&fsb,fterms,bp,buf + BUFLEN - bp) ;

#if	CF_DEBUGS
	    eprintf("procenv: 3 field> %W\n",fsb.fp,fsb.flen) ;
#endif

	    if (fsb.flen > 0)
	        bp += fsb.flen ;

	    else
	        bp -= 1 ;

	    *bp = '\0' ;

	    if ((rs = vecstr_finder(lp,buf,vstrkeycmp,NULL)) >= 0)
	        vecstr_del(lp,rs) ;

	    rs = vecstr_add(lp,buf,bp - buf) ;

		if (rs >= 0)
	    n += 1 ;

	        if (fsb.term == '#')
	            break ;

	} /* end while */

	bclose(fp) ;

	    rs = len ;

#if	CF_DEBUGS
	eprintf("procenv: exiting rs=%d\n",
	    ((rs >= 0) ? n : rs)) ;
#endif

	return ((rs >= 0) ? n : rs) ;
}
/* end subroutine (procenv) */



