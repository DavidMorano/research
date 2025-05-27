/* procargs */

/* process a program arguments file */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0


/* revision history:

	= 94/09/10, Dave Morano

	This program was originally written.


*/


/*****************************************************************************

	This subroutine will read (process) a file with program
	arguments in it.

*****************************************************************************/


#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<cstdlib>
#include	<cstring>

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

extern char	*strwcpy(char *,const char *,int) ;


/* externals variables */


/* forward references */


/* local structures */


/* local global variabes */


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






int procargs(programroot,fname,lp)
char	programroot[] ;
char	fname[] ;
VECSTR	*lp ;
{
	bfile	efile, *fp = &efile ;

	FIELD	fsb ;

	int	rs, n, len, flen ;

	char	linebuf[LINELEN + 1] ;
	char	buf[BUFLEN + 1], *bp ;


#if	CF_DEBUGS
	eprintf("procargs: entered fname=%s\n",fname) ;
#endif

	if ((rs = bopenroot(fp,programroot,fname,NULL,"r",0666)) < 0)
	    return rs ;

#if	CF_DEBUGS
	eprintf("procargs: opened\n") ;
#endif

	n = 0 ;
	while ((len = bgetline(fp,linebuf,LINELEN)) > 0) {

#if	CF_DEBUGS
	    eprintf("procargs: line> %W\n",linebuf,len - 1) ;
#endif

	    fsb.lp = linebuf ;
	    fsb.rlen = (linebuf[len - 1] == '\n') ? (len - 1) : len ;

	    if (len <= 0)
	        continue ;

	    while (TRUE) {

	        flen = field_sharg(&fsb,fterms,buf,BUFLEN) ;

	        if (flen < 0)
	            break ;

#if	CF_DEBUGS
	        eprintf("procargs: 3 field> %W\n",fsb.fp,fsb.flen) ;
#endif

	        rs = vecstr_add(lp,buf,flen) ;

		if (rs >= 0)
	        n += 1 ;

	        if (fsb.term == '#')
	            break ;

	    } /* end while (reading fields) */

	} /* end while (reading lines) */

	bclose(fp) ;

	    rs = len ;

#if	CF_DEBUGS
	eprintf("procargs: exiting rs=%d\n",
	    ((rs >= 0) ? n : len)) ;
#endif

	return ((rs >= 0) ? n : rs) ;
}
/* end subroutine (procargs) */



