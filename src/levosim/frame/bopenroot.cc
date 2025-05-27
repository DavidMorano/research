/* bopenroot */

/* open a file name according to rules */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0


/* revision history :

	= 94/09/01, Dave Morano

	This program was originally written.


*/



/*****************************************************************************

	This subroutine will form a file name according to some
	rules.

	The rules are roughly :

	+ attempt to open it directly if it is already rooted
	+ open it if it is already in the root area
	+ attempt to open it as it is if it already exists
	+ attempt to open or create it located in the root area
	+ attempt to open or create it as it is


	Synopsis :

		int bopenroot(fp,programroot,fname,outname,mode,permission)
		bfile	*fp ;
		char	programroot[], fname[], outname[] ;
		char	mode[] ;
		int	permission ;


	Arguments :

	+ fp		pointer to 'bfile' object
	+ programroot	path of program root directory
	+ fname		fname to find and open
	+ outname	user supplied buffer to hold possible resulting name
	+ mode		file open mode
	+ permissions	file permissions to use in the open 

	Returns :

	<0		error (same as 'bopen()')
	>=0		success (same as 'bopen()')

	outname		1) zero length string if no new name was needed
			2) will contain the path of the file that was opened

	

*****************************************************************************/



#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<bio.h>

#include	"localmisc.h"
#include	"outbuf.h"



/* local defines */



/* external subroutines */

extern int	perm(char *,int,int,int *,int) ;

extern char	*strwcpy(char *,char *,int) ;


/* external variables */


/* forward references */


/* local global variabes */


/* local structures */


/* local variables */






int bopenroot(fp,programroot,fname,outname,mode,permission)
bfile	*fp ;
char	programroot[], fname[], outname[] ;
char	mode[] ;
int	permission ;
{
	OUTBUF	ob ;

	int	rs ;
	int	sl ;
	int	imode ;
	int	f_outbuf ;

	char	*mp, *onp = NULL ;


#if	CF_DEBUGS
	eprintf("bopenroot: entered fname=%s\n",fname) ;
#endif

	if (fp == NULL)
	    return SR_FAULT ;

	if ((fname == NULL) || (fname[0] == '\0'))
	    return SR_NOEXIST ;

	f_outbuf = (outname != NULL) ;

	imode = 0 ;
	for (mp = mode ; *mp ; mp += 1) {

	    switch ((int) *mp) {

	    case 'r':
	        imode += R_OK ;
	        break ;

	    case 'w':
	        imode += W_OK ;
	        break ;

	    case 'x':
	        imode += X_OK ;
	        break ;

	    } /* end switch */

	} /* end for */


	if (fname[0] == '/') {

	    if (f_outbuf)
	        outname[0] = '\0' ;

	    rs = bopen(fp,fname,mode,permission) ;

	    return rs ;
	}


	rs = outbuf_init(&ob,outname,-1) ;

#if	CF_DEBUGS
	    eprintf("bopenroot: outbuf_init() rs=%d\n",rs) ;
#endif


	if (programroot != NULL) {

#if	CF_DEBUGS
	    eprintf("bopenroot: about to alloc\n") ;
#endif

	    rs = outbuf_get(&ob,&onp) ;

#if	CF_DEBUGS
	    eprintf("bopenroot: outbuf_get() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        goto done ;

	    sl = bufprintf(onp,MAXPATHLEN,"%s/%s",
	        programroot,fname) ;

#if	CF_DEBUGS
	    eprintf("bopenroot: onp=%W\n",onp,sl) ;
#endif

	    if ((perm(onp,-1,-1,NULL,imode) >= 0) &&
	        ((rs = bopen(fp,onp,mode,permission)) >= 0))
	        goto done ;

#if	CF_DEBUGS
	    eprintf("bopenroot: perm() or bopen() failed rs=%d\n",rs) ;
#endif

	} /* end if (we had a programroot) */

	if ((perm(fname,-1,-1,NULL,imode) >= 0) &&
	    ((rs = bopen(fp,fname,mode,permission)) >= 0)) {

	    if (f_outbuf)
	        outname[0] = '\0' ;

	    goto done ;
	}

	if ((programroot != NULL) &&
	    (strchr(fname,'/') != NULL)) {

	    if ((rs = bopen(fp,onp,mode,permission)) >= 0)
	        goto done ;

	}

	if (f_outbuf)
	    outname[0] = '\0' ;

	rs = bopen(fp,fname,mode,permission) ;

#if	CF_DEBUGS
	    eprintf("bopenroot: final bopen() rs=%d\n",rs) ;
#endif

done:

#if	CF_DEBUGS
	    eprintf("bopenroot: done rs=%d\n",rs) ;
#endif

	outbuf_free(&ob) ;

#if	CF_DEBUGS
	    eprintf("bopenroot: exiting\n") ;
#endif

	return rs ;
}
/* end subroutine (bopenroot) */



