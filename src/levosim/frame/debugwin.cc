/* debugwin */

/* read in a file of parameters */


#define	CF_DEBUGS	0
#define	CF_SAFE		0


/* revision history :

	- 01/02/05, Dave Morano

	Some code for this subroutine was taken from something
	that did something similar to what we are doing here.
	The rest was originally written for LevoSim.


*/


/******************************************************************************

	This object manages the Debug Schedule data base.  Among the
	subroutines available, there is something that will read in a
	Debug Sched file and put the entries into the database.


******************************************************************************/


#define	DEBUGWIN_MASTER	1


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<netdb.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>

#include	<vsystem.h>
#include	<field.h>
#include	<char.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"debugwin.h"



/* local defines */

#define	LINELEN			200
#undef	BUFLEN
#define	BUFLEN			((20 * MAXPATHLEN) + (10 * MAXHOSTNAMELEN))



/* external subroutines */

extern int	cfnumull(const char *,int,ULONG *) ;
extern int	sfkey(const char *,int,char **) ;
extern int	optmatch(char *const *,const char *,int) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnchr(const char *,int,int) ;


/* externals variables */


/* forward references */

static int	debugwin_procopts(DEBUGWIN *,const char *,int) ;


/* local data */

/* trace options */

static char	*const opts[] = {
	"ck",
	"in",
	NULL
} ;

enum opts {
	opt_ck,
	opt_in,
	opt_overlast
} ;

/* white space, '#', and comma (',') */
static const unsigned char 	oterms[32] = {
	0x00, 0x0B, 0x00, 0x00,
	0x09, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
} ;








int debugwin_init(pfp)
DEBUGWIN	*pfp ;
{
	int	rs = SR_OK ;
	int	rs2 ;


#if	CF_DEBUGS
	eprintf("debugwin_init: entered\n") ;
#endif

	if (pfp == NULL)
	    return SR_FAULT ;

	(void) memset(pfp,0,sizeof(DEBUGWIN)) ;

	return rs ;
}
/* end subroutine (debugwin_init) */


/* set the debugging level */
int debugwin_setlevel(op,val)
DEBUGWIN	*op ;
int		val ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	op->debuglevel = val ;

	return SR_OK ;
}
/* end subroutine (debugwin_setlevel) */


/* set some options */
int debugwin_setopt(op,s,slen)
DEBUGWIN	*op ;
char		s[] ;
int		slen ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	rs = debugwin_procopts(op,s,slen) ;

/* re-compute some things (that may have changed) */

	op->ck_end = op->ck_start + op->ck_n ;
	op->in_end = op->in_start + op->in_n ;

	return rs ;
}
/* end subroutine (debugwin_setopt) */


/* is tracing enabled (for real) ? */
int debugwin_check(op,ck,in)
DEBUGWIN	*op ;
ULONG		ck ;
ULONG		in ;
{
	int	rs ;
	int	f_ie, f_ce ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;
#endif

	if (op->debuglevel <= 0)
		return 0 ;

/* instructions first */

#if	CF_DEBUGS
	    eprintf("debugwin_check: in=%lld start=%lld end=%lld\n",
		in,op->in_start,op->in_end) ;
#endif

	if (in < op->in_start)
		f_ie = FALSE ;

	else if (op->in_n == 0)  
		f_ie = TRUE ;

	else
		f_ie = (in < op->in_end) ;

#if	CF_DEBUGS
	    eprintf("debugwin_check: f_ie=%d\n",f_ie) ;
#endif

/* clocks */

#if	CF_DEBUGS
	    eprintf("debugwin_check: ck=%lld start=%lld end=%lld\n",
		ck,op->ck_start,op->ck_end) ;
#endif

	if (ck < op->ck_start)
		f_ce = FALSE ;

	else if (op->ck_n == 0)  
		f_ce = TRUE ;

	else
		f_ce = (ck < op->ck_end) ;

#if	CF_DEBUGS
	    eprintf("debugwin_check: f_ce=%d\n",f_ce) ;
#endif

/* decide */

	rs = 0 ;
	if (f_ie && f_ce)
		rs = op->debuglevel ;

	return rs ;
}
/* end subroutine (debugwin_check) */


/* free up */
int debugwin_free(pfp)
DEBUGWIN	*pfp ;
{


	if (pfp == NULL)
	    return SR_FAULT ;

	return SR_OK ;
}
/* end subroutine (debugwin_free) */



/* PRIVATE SUBROUTINES */



static int debugwin_procopts(op,s,slen)
DEBUGWIN	*op ;
const char	s[] ;
int		slen ;
{
	FIELD	fsb ;

	ULONG	ulw ;

	int	rs = SR_OK, i, n ;
	int	sl, sl2 ;

	char	*cp, *cp2 ;


	n = 0 ;
	field_init(&fsb,s,slen) ;

	while (field_get(&fsb,oterms) >= 0) {

	    if (fsb.flen == 0)
	        continue ;

#if	CF_DEBUGS
	    eprintf("debugwin_procopts: option >%W<\n",
	        fsb.fp,fsb.flen) ;
#endif

	            if ((sl = sfkey(fsb.fp,fsb.flen,&cp)) < 0) {

				cp = fsb.fp ;
				sl = fsb.flen ;
			}

#if	CF_DEBUGS
	    eprintf("debugwin_procopts: option key >%W<\n",cp,sl) ;
#endif

	    i = optmatch(opts,cp,sl) ;

	    if (i < 0)
	        continue ;

	    cp2 = strnchr(cp + sl,(fsb.flen - sl),'=') ;

	    sl2 = 0 ;
	    if (cp2 != NULL) {

	        cp2 += 1 ;
	        sl2 = fsb.flen - (cp2 - fsb.fp) ;

	    }

#if	CF_DEBUGS
	    eprintf("debugwin_procopts: option match i=%d\n", i) ;
	    eprintf("debugwin_procopts: cp2=%W\n",cp2,sl2) ;
#endif

	    switch (i) {

	    case opt_in:
	        if (cp2 != NULL) {

	            int		sl3 ;

		    char	*cp3 ;


	            if ((cp3 = strnchr(cp2,sl2,':')) != NULL) {

	                sl3 = (cp2 + sl2) - (cp3 + 1) ;

	                if ((sl3 > 0) &&
	                    (cfnumull((cp3 + 1),sl3,&ulw) >= 0))
	                    op->in_n = ulw ;

	                sl2 = cp3 - cp2 ;

	            } /* end if (had another value) */

	            if (cfnumull(cp2,sl2,&ulw) >= 0)
	                op->in_start = ulw ;

#if	CF_DEBUGS
	    eprintf("debugwin_procopts: in_start=%lld in_n=%lld\n",
		op->in_start,op->in_n) ;
#endif

	        } /* end if */

	        break ;

	    case opt_ck:
	        if (cp2 != NULL) {

	            int		sl3 ;

		    char	*cp3 ;


	            if ((cp3 = strnchr(cp2,sl2,':')) != NULL) {

	                sl3 = (cp2 + sl2) - (cp3 + 1) ;

	                if ((sl3 > 0) &&
	                    (cfnumull((cp3 + 1),sl3,&ulw) >= 0))
	                    op->ck_n = ulw ;

	                sl2 = cp3 - cp2 ;

	            } /* end if (had another value) */

	            if (cfnumull(cp2,sl2,&ulw) >= 0)
	                op->ck_start = ulw ;

#if	CF_DEBUGS
	    eprintf("debugwin_procopts: ck_start=%lld ck_n=%lld\n",
		op->ck_start,op->ck_n) ;
#endif

		} /* end if */

		break ;

	    } /* end switch */

		n += 1 ;
	    if (fsb.term == '#')
	        break ;

	} /* end while */

	field_free(&fsb) ;

#if	CF_DEBUGS
	    eprintf("debugwin_procopts: exiting rs=%d\n",rs) ;
#endif

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (debugwin_procopts) */



