/* procfileenv */

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
	deleted with a new definition is encountered.



*****************************************************************************/




#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>

#include	<bio.h>
#include	<field.h>
#include	<vecstr.h>
#include	<char.h>

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




int procfileenv(programroot,fname,lp)
char	programroot[] ;
char	fname[] ;
VECSTR	*lp ;
{
	bfile	file, *fp = &file ;

	FIELD	fsb ;

	int	len, i, rs ;

	char	linebuf[LINELEN + 1] ;
	char	buf[BUFLEN + 1], *bp ;

#if	CF_DEBUGS
	char	outname[MAXPATHLEN + 1] ;
#endif


#if	CF_DEBUGS
	    eprintf("procfileenv: entered=%s\n",fname) ;

	if ((rs = bopenroot(fp,programroot,fname,outname,"r",0666)) < 0) {

	        eprintf("procfileenv: bopen rs=%d\n",rs) ;

	    return rs ;
	}

	    eprintf("procfileenv: outname=%s\n",outname) ;
#else
	if ((rs = bopenroot(fp,programroot,fname,NULL,"r",0666)) < 0)
	    return rs ;
#endif /* CF_DEBUGS */

#if	CF_DEBUGS
	    eprintf("procfileenv: opened\n") ;
#endif

	i = 0 ;
	while ((len = bgetline(fp,linebuf,LINELEN)) > 0) {

#if	CF_DEBUGS
	        eprintf("procfileenv: line> %W\n",linebuf,len - 1) ;
#endif

	    fsb.lp = linebuf ;
	    fsb.rlen = (linebuf[len - 1] == '\n') ? (len - 1) : len ;
	    field_get(&fsb,fterms) ;

	    if (fsb.flen <= 0) continue ;

#if	CF_DEBUGS
	        eprintf("procfileenv: 1 field> %W\n",fsb.fp,fsb.flen) ;
#endif

	    if (strncasecmp(fsb.fp,"export",fsb.flen) == 0)
	        field_get(&fsb,fterms) ;

#if	CF_DEBUGS
	        eprintf("procfileenv: 2 field> %W\n",fsb.fp,fsb.flen) ;
#endif

	    bp = buf ;
	    bp = strwcpy(bp,fsb.fp,MIN(fsb.flen,BUFLEN)) ;

	    if (bp < (buf + BUFLEN - 1))
	        *bp++ = '=' ;

	    field_sharg(&fsb,fterms,bp,buf + BUFLEN - bp) ;

#if	CF_DEBUGS
	        eprintf("procfileenv: 3 field> %W\n",fsb.fp,fsb.flen) ;
#endif
	    if (fsb.flen > 0)
	        bp += fsb.flen ;

	    else
	        bp -= 1 ;

	    *bp = '\0' ;

	    if ((rs = vecstr_finder(lp,buf,vstrkeycmp,NULL)) >= 0)
	        vecstr_del(lp,rs) ;

	    vecstr_add(lp,buf,bp - buf) ;

	    i += 1 ;

	} /* end while */

	bclose(fp) ;

#if	CF_DEBUGS
	    eprintf("procfileenv: exiting rs=%d\n",
	        ((len < 0) ? len : i)) ;
#endif

	return ((len < 0) ? len : i) ;
}
/* end subroutine (procfileenv) */



