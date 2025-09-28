/* proginfo */

/* some subroutines to operate on the program information */


#define	CF_DEBUGS	0
#define	CF_DEBUG	1
#define	CF_SAFE		0


/* revision history:

	= 2000-02-15, Dave Morano

	This code was started.


*/


/******************************************************************************

	This module performs some miscellaneous support functions for
	the whole program.  It currently mostly only provides for
	writing progress records to the logfile and checking if the
	target program being executed happens to be within the general
	"selection."


******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>
#include	<cstring>

#include	<usystem.h>
#include	<field.h>

#include	"localmisc.h"

#if	defined(P_SSLEVO)
#include	"ssconfig.h"
#else
#include	"config.h"
#endif /* P_SSLEVO */

#include	"defs.h"



/* local defines */



/* external subroutines */

extern int	sfkey(const char *,int,char **) ;
extern int	matostr(const char **,int,const char *,int) ;
extern int	cfnumull(const char *,int,ULONG *) ;

extern char	*strnchr(const char *,int,int) ;
extern char	*timestr_logz(time_t,char *) ;
extern char	*timestr_elapsed(time_t,char *) ;


/* local structures */


/* forward references */

static int	proginfo_procopts(struct proginfo *, const char *,int) ;


/* local variables */

/* trace options */

static const char	*opts[] = {
	"sck",
	"sin",
	NULL
} ;

enum opts {
	opt_sck,
	opt_sin,
	opt_overlast
} ;

static const char	*selects[] = {
	"ck",
	"in",
	NULL
} ;

enum selects {
	select_ck,
	select_in,
	select_overlast
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






int proginfo_init(pip,daytime)
struct proginfo	*pip ;
time_t		daytime ;
{


	pip->cin = 0 ;

	pip->f.past = FALSE ;

	pip->starttime = daytime ;
	pip->lastlogtime = daytime ;

	return SR_OK ;
}
/* end subroutine (proginfo_init) */


/* check on some stuff */
int proginfo_check(pip,clock)
struct proginfo	*pip ;
ULONG		clock ;
{
	time_t	daytime ;

	char	timebuf[TIMEBUFLEN + 1] ;


/* check on log interval for a mark */

	daytime = time(NULL) ;

	if (pip->f.log &&
		((daytime - pip->lastlogtime) > LOGINTERVAL)) {

	    pip->lastlogtime = daytime ;
	    logfile_printf(&pip->lh,"%s %s!%s\n",
	        timestr_logz(daytime,timebuf),
	        pip->nodename,pip->username) ;

	    if (pip->jobname[0] != '\0') {

	        logfile_printf(&pip->lh,"jobname=%s pid=%d\n",
	            pip->jobname,pip->pid) ;

	    } else {

	        logfile_printf(&pip->lh,"pid=%d\n",
	            pip->pid) ;

	    }

	    logfile_printf(&pip->lh,"elapsed time %s\n",
	        timestr_elapsed((daytime - pip->starttime),timebuf)) ;

	    logfile_printf(&pip->lh,"clock=%lld\n",
	        clock) ;

	} /* end if (log mark update) */




	return SR_OK ;
}
/* end if subroutine (proginfo_check) */


/* write some progress information */
int proginfo_progress(pip,ck,in)
struct proginfo	*pip ;
ULONG		ck, in ;
{
	int	rs = SR_OK ;

	time_t	daytime ;

	char	timebuf[TIMEBUFLEN + 1] ;


	u_time(&daytime) ;

	if (! pip->f.log) {

	if ((daytime - pip->lastlogtime) > LOGINTERVAL)
	    pip->lastlogtime = daytime ;

	    return rs ;
	}

/* check on log interval for a mark */

	if ((daytime - pip->lastlogtime) > LOGINTERVAL) {

	    pip->lastlogtime = daytime ;

	    logfile_printf(&pip->lh,"%s %s!%s\n",
	        timestr_logz(daytime,timebuf),
	        pip->nodename,pip->username) ;

	    if (pip->jobname[0] != '\0') {

	        logfile_printf(&pip->lh,"jobname=%s pid=%d\n",
	            pip->jobname,pip->pid) ;

	    } else {

	        logfile_printf(&pip->lh,"pid=%d\n",
	            pip->pid) ;

	    }

	    logfile_printf(&pip->lh,"elapsed time %s\n",
	        timestr_elapsed((daytime - pip->starttime),timebuf)) ;

	    logfile_printf(&pip->lh,"cl=%lld in=%lld\n",
	        ck,in) ;

	} /* end if (log mark update) */

	return SR_OK ;
}
/* end if subroutine (proginfo_progress) */


/* Levo summary configuration */
int proginfo_levoconf(pip,buf,buflen)
struct proginfo	*pip ;
char		buf[] ;
int		buflen ;
{
	int	rs = SR_OK ;
	int	sl ;

	char	*bp, *cp ;


	if (! pip->f.log)
	    return rs ;

	if (buflen < 0)
	    buflen = strlen(buf) ;

	bp = buf ;
	while ((buflen > 0) && ((cp = strnchr(bp,buflen,'\n')) != NULL)) {

	    logfile_printf(&pip->lh,"lc> %W\n",
	        bp,(cp - bp)) ;

	    cp += 1 ;
	    buflen -= (cp - bp) ;
	    bp = cp ;

	} /* end while */

	if (buflen > 0)
	    rs = logfile_printf(&pip->lh,"lc> %W\n",bp,buflen) ;


	return rs ;
}
/* end if subroutine (proginfo_levoconf) */


int proginfo_free(pip)
struct proginfo	*pip ;
{


	return SR_OK ;
}
/* end subroutine (proginfo_free) */


/* set the selection (if we get one) */
int proginfo_setselect(op,s,slen)
struct proginfo	*op ;
char		s[] ;
int		slen ;
{
	FIELD	fsb ;

	ULONG	ulw ;

	int	rs = SR_OK ;
	int	fl ;
	int	i, n ;
	int	sl, cl, cl2 ;

	char	*fp ;
	char	*cp, *cp2 ;


	if (op == NULL)
	    return SR_FAULT ;

	n = 0 ;
	rs = field_init(&fsb,s,slen) ;
	if (rs < 0)
	    goto ret0 ;

	while ((fl = field_get(&fsb,oterms,&fp)) >= 0) {

	    if (fl == 0)
	        continue ;

#if	CF_DEBUGS
	    debugprintf("proginfo_setselect: option >%W<\n",
	        fp,fl) ;
#endif

	    sl = sfkey(fp,fl,&cp) ;

	    if (sl >= 0) {

#if	CF_DEBUGS
	        debugprintf("proginfo_setselect: option key >%W<\n",cp,sl) ;
#endif

	        i = matostr(selects,2,cp,sl) ;
	        if (i < 0)
	            continue ;

	        cp2 = strnchr(cp + sl,(fl - sl),'=') ;

	        cl2 = 0 ;
	        if (cp2 != NULL) {

	            cp2 += 1 ;
	            cl2 = fl - (cp2 - fsb.fp) ;

	        }

#if	CF_DEBUGS
	        debugprintf("proginfo_setselect: option match i=%d\n", i) ;
	        debugprintf("proginfo_setselect: cp2=%W\n",cp2,cl2) ;
#endif

	    } else {

	        cp2 = fp ;
	        cl2 = fl ;
	        i = select_in ;

	    }

	    switch (i) {

	    case select_in:
	        if (cp2 != NULL) {

	            int		sl3 ;

	            char	*cp3 ;


	            if ((cp3 = strnchr(cp2,cl2,':')) != NULL) {

	                sl3 = (cp2 + cl2) - (cp3 + 1) ;

	                if ((sl3 > 0) &&
	                    (cfnumull((cp3 + 1),sl3,&ulw) >= 0))
	                    op->in.n = ulw ;

	                cl2 = cp3 - cp2 ;

	            } /* end if (had another value) */

	            if (cfnumull(cp2,cl2,&ulw) >= 0)
	                op->in.start = ulw ;

#if	CF_DEBUGS
	            debugprintf("proginfo_setselect: in_start=%lld in_n=%lld\n",
	                op->in.start,op->in.n) ;
#endif

	        } /* end if */

	        break ;

	    case select_ck:
	        if (cp2 != NULL) {

	            int		sl3 ;

	            char	*cp3 ;


	            if ((cp3 = strnchr(cp2,cl2,':')) != NULL) {

	                sl3 = (cp2 + cl2) - (cp3 + 1) ;

	                if ((sl3 > 0) &&
	                    (cfnumull((cp3 + 1),sl3,&ulw) >= 0))
	                    op->ck.n = ulw ;

	                cl2 = cp3 - cp2 ;

	            } /* end if (had another value) */

	            if (cfnumull(cp2,cl2,&ulw) >= 0)
	                op->ck.start = ulw ;

#if	CF_DEBUGS
	            debugprintf("proginfo_setselect: ck_start=%lld ck_n=%lld\n",
	                op->ck.start,op->ck.n) ;
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
	debugprintf("proginfo_setselect: exiting rs=%d\n",rs) ;
#endif


/* re-compute some things (that may have changed) */

	op->ck.end = op->ck.start + op->ck.n ;
	op->in.end = op->in.start + op->in.n ;

ret0:
	return (rs >= 0) ? n : rs ;
}
/* end subroutine (proginfo_setselect) */


/* set some options */
int proginfo_setopt(op,s,slen)
struct proginfo	*op ;
char		s[] ;
int		slen ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	rs = proginfo_procopts(op,s,slen) ;

/* re-compute some things (that may have changed) */

	op->ck.end = op->ck.start + op->ck.n ;
	op->in.end = op->in.start + op->in.n ;

	return rs ;
}
/* end subroutine (proginfo_setopt) */


/* is tracing enabled (for real) ? */
int proginfo_selection(op,ck,in)
struct proginfo	*op ;
ULONG		ck ;
ULONG		in ;
{
	int	rs ;
	int	f_ie, f_ce ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;
#endif

/* instructions first */

#if	CF_DEBUGS
	debugprintf("proginfo_check: in=%lld start=%lld end=%lld\n",
	    in,op->in.start,op->in.end) ;
#endif

	if (in < op->in.start)
	    f_ie = FALSE ;

	else if (op->in.n == 0)
	    f_ie = TRUE ;

	else
	    f_ie = (in < op->in.end) ;

#if	CF_DEBUGS
	debugprintf("proginfo_check: f_ie=%d\n",f_ie) ;
#endif

/* clocks */

#if	CF_DEBUGS
	debugprintf("proginfo_check: ck=%lld start=%lld end=%lld\n",
	    ck,op->ck.start,op->ck.end) ;
#endif

	if (ck < op->ck.start)
	    f_ce = FALSE ;

	else if (op->ck.n == 0)
	    f_ce = TRUE ;

	else
	    f_ce = (ck < op->ck.end) ;

#if	CF_DEBUGS
	debugprintf("proginfo_check: f_ce=%d\n",f_ce) ;
#endif

	return (f_ie && f_ce) ;
}
/* end subroutine (proginfo_selection) */


#ifdef	COMMENT

/* are we supposed to be dumping (inside the selection region) */
int proginfo_dump(pip,in)
struct proginfo	*pip ;
ULONG		in ;
{


	if (in < pip->in.start)
	    return FALSE ;

	if (pip->in.n == 0)
	    return TRUE ;

	if (in < pip->in.end)
	    return TRUE ;

	pip->f.past = TRUE ;
	return FALSE ;
}
/* end subroutine (proginfo_dump) */

#endif /* COMMENT */


/* is tracing enabled (for real) ? */
int proginfo_dump(pip,in)
struct proginfo	*pip ;
ULONG		in ;
{
	int	r ;
	int	f_old ;


#if	CF_SAFE
	if (pip == NULL)
	    return SR_FAULT ;
#endif

	f_old = pip->f.select ;
	pip->cin = in ;

	if (in < pip->in.start) {

		pip->f.select = FALSE ;

	} else if (pip->in.n == 0) {

		pip->f.select = TRUE ;

	} else {

		pip->f.select = (in < pip->in.end) ;
		pip->f.past = (! pip->f.select) ;

	}

#if	CF_DEBUGS
	    debugprintf("proginfo_dump: %d\n",pip->f.select) ;
#endif

	r = (pip->f.select) ? ((f_old) ? 1 : 2) : 0 ;

	return r ;
}
/* end subroutine (proginfo_dump) */


/* private subroutines */


static int proginfo_procopts(op,s,slen)
struct proginfo	*op ;
const char	s[] ;
int		slen ;
{
	FIELD	fsb ;

	ULONG	ulw ;

	int	rs = SR_OK ;
	int	fl ;
	int	i, n ;
	int	sl, sl2 ;

	char	*fp ;
	char	*cp, *cp2 ;


	n = 0 ;
	rs = field_init(&fsb,s,slen) ;
	if (rs < 0)
	    goto ret0 ;

	while ((fl = field_get(&fsb,oterms,&fp)) >= 0) {

	    if (fl == 0)
	        continue ;

#if	CF_DEBUGS
	    debugprintf("proginfo_procopts: option >%W<\n",
	        fp,fl) ;
#endif

	    if ((sl = sfkey(fp,fl,&cp)) < 0) {
	        cp = fp ;
	        sl = fl ;
	    }

#if	CF_DEBUGS
	    debugprintf("proginfo_procopts: option key >%W<\n",cp,sl) ;
#endif

	    i = matostr(opts,2,cp,sl) ;
	    if (i < 0)
	        continue ;

	    cp2 = strnchr(cp + sl,(fl - sl),'=') ;

	    sl2 = 0 ;
	    if (cp2 != NULL) {

	        cp2 += 1 ;
	        sl2 = fl - (cp2 - fp) ;

	    }

#if	CF_DEBUGS
	    debugprintf("proginfo_procopts: option match i=%d\n", i) ;
	    debugprintf("proginfo_procopts: cp2=%W\n",cp2,sl2) ;
#endif

	    switch (i) {

	    case select_in:
	        if (cp2 != NULL) {

	            int		sl3 ;

	            char	*cp3 ;


	            if ((cp3 = strnchr(cp2,sl2,':')) != NULL) {

	                sl3 = (cp2 + sl2) - (cp3 + 1) ;

	                if ((sl3 > 0) &&
	                    (cfnumull((cp3 + 1),sl3,&ulw) >= 0))
	                    op->in.n = ulw ;

	                sl2 = cp3 - cp2 ;

	            } /* end if (had another value) */

	            if (cfnumull(cp2,sl2,&ulw) >= 0)
	                op->in.start = ulw ;

#if	CF_DEBUGS
	            debugprintf("proginfo_procopts: in_start=%lld in_n=%lld\n",
	                op->in.start,op->in.n) ;
#endif

	        } /* end if */

	        break ;

	    case select_ck:
	        if (cp2 != NULL) {

	            int		sl3 ;

	            char	*cp3 ;


	            if ((cp3 = strnchr(cp2,sl2,':')) != NULL) {

	                sl3 = (cp2 + sl2) - (cp3 + 1) ;

	                if ((sl3 > 0) &&
	                    (cfnumull((cp3 + 1),sl3,&ulw) >= 0))
	                    op->ck.n = ulw ;

	                sl2 = cp3 - cp2 ;

	            } /* end if (had another value) */

	            if (cfnumull(cp2,sl2,&ulw) >= 0)
	                op->ck.start = ulw ;

#if	CF_DEBUGS
	            debugprintf("proginfo_procopts: ck_start=%lld ck_n=%lld\n",
	                op->ck.start,op->ck.n) ;
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
	debugprintf("proginfo_procopts: exiting rs=%d\n",rs) ;
#endif

ret0:
	return (rs >= 0) ? n : rs ;
}
/* end subroutine (proginfo_procopts) */



