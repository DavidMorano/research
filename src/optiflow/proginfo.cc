/* proginfo */

/* program information */


#define	CF_DEBUGS	0		/* non-switchable debug print-outs */
#define	CF_DEBUG	0
#define	CF_SAFE		0


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This group of subroutines help find and set from variables
	for program start-up type functions.


******************************************************************************/


#include	<sys/types.h>
#include	<sys/param.h>
#include	<cstdlib>
#include	<cstring>

#include	<vsystem.h>
#include	<bio.h>
#include	<field.h>
#include	<mallocstuff.h>

#include	"localmisc.h"

#if	defined(P_OPTIFLOW)
#include	"ssconfig.h"
#else
#include	"config.h"
#endif /* P_SSLEVO */

#include	"defs.h"



/* local defines */



/* external subroutines */

extern int	sfdirname(const char *,int,char **) ;
extern int	sfbasename(const char *,int,char **) ;
extern int	sfkey(const char *,int,char **) ;
extern int	getpwd(char *,int) ;
extern int	optmatch(const char **,const char *,int) ;
extern int	cfnumull(const char *,int,ULONG *) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnchr(const char *,int,int) ;
extern char	*timestr_logz(time_t,char *) ;
extern char	*timestr_elapsed(time_t,char *) ;



/* forward references */

int	proginfo_getpwd(struct proginfo *,char *,int) ;

static int	proginfo_procopts(struct proginfo *, const char *,int) ;

static int	setit(void *,const void *) ;

static void	freeit(void *) ;


/* local variables */

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







int proginfo_init(pip,envv,ap,version)
struct proginfo	*pip ;
const char	*envv[] ;
const char	ap[] ;
const char	version[] ;
{
	time_t	daytime = time(NULL) ;

	int	rs ;


	if (pip == NULL)
	    return SR_FAULT ;

	memset(pip,0,sizeof(struct proginfo)) ;

	pip->envv = (char **) envv ;
	pip->version = mallocstr(version) ;

	if (pip->version == NULL)
	    goto bad0 ;

	rs = proginfo_setprog(pip,ap) ;

	if (rs < 0)
	    goto bad1 ;

	proginfo_initextra(pip,daytime) ;

	return rs ;

/* bad stuff */
bad1:
	if (pip->version != NULL)
	    free(pip->version) ;

bad0:
	return rs ;
}
/* end subroutine (proginfo_init) */


int proginfo_free(pip)
struct proginfo	*pip ;
{


	if (pip == NULL)
	    return SR_FAULT ;

	proginfo_freeextra(pip) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_free: version\n") ;
#endif

	freeit(&pip->version) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_free: pwd\n") ;
#endif

	freeit(&pip->pwd) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_free: progdname\n") ;
#endif

	freeit(&pip->progdname) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_free: progname\n") ;
#endif

	freeit(&pip->progname) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_free: pr\n") ;
#endif

	freeit(&pip->pr) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_free: searchname\n") ;
#endif

	freeit(&pip->searchname) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_free: banner\n") ;
#endif

	freeit(&pip->banner) ;


	memset(pip,0,sizeof(struct proginfo)) ;

	return SR_OK ;
}
/* end subroutine (proginfo_free) */


int proginfo_banner(pip,v)
struct proginfo	*pip ;
const char	v[] ;
{
	int	rs ;


	if (pip == NULL)
	    return SR_FAULT ;

	rs = setit(&pip->banner,v) ;

	return rs ;
}
/* end subroutine (proginfo_banner) */


int proginfo_searchname(pip,v)
struct proginfo	*pip ;
const char	v[] ;
{
	int	rs ;


	if (pip == NULL)
	    return SR_FAULT ;

	rs = setit(&pip->searchname,v) ;

	return rs ;
}
/* end subroutine (proginfo_searchname) */


int proginfo_setprog(pip,ap)
struct proginfo	*pip ;
const char	ap[] ;
{
	int	rs = SR_OK ;
	int	dl, bl ;
	int	al ;

	char	*dp, *bp ;


	pip->progdname = NULL ;
	pip->progname = NULL ;
	if (ap == NULL)
	    return SR_FAULT ;

	al = strlen(ap) ;

	if ((dl = sfdirname(ap,al,&dp)) > 0) {

	    freeit(&pip->progdname) ;

	    pip->progdname = mallocstrn(dp,dl) ;

	    if (pip->progdname == NULL)
	        rs = SR_NOMEM ;

	}

	if (rs >= 0) {

	    bl = sfbasename(ap,al,&bp) ;

	    freeit(&pip->progname) ;

	    pip->progname = mallocstrn(bp,bl) ;

	    if (pip->progname == NULL) {

	        rs = SR_NOMEM ;
	        goto bad1 ;
	    }

	}

	return rs ;

/* bad stuff comes here */
bad1:
	freeit(&pip->progdname) ;

bad0:
	return rs ;
}
/* end subroutine (proginfo_setprog) */


int proginfo_rootprogdname(pip)
struct proginfo	*pip ;
{
	int	rs = SR_NOENT ;
	int	bl, rl ;
	int	f ;

	char	*bp, *rp ;
	char	*cp ;


	if (pip->pr != NULL)
	    return strlen(pip->pr) ;

	if (pip->progdname == NULL)
	    return SR_NOENT ;

	bl = sfbasename(pip->progdname,-1,&bp) ;

	f = ((bl == 3) && (strncmp(bp,"bin",bl) == 0)) ;

	if (! f)
	    f = ((bl == 4) && (strncmp(bp,"sbin",bl) == 0)) ;

	if (f) {

	    freeit(&pip->pr) ;

	    rl = sfdirname(pip->progdname,-1,&rp) ;

	    rs = rl ;
	    if ((rl == 0) || ((rl > 0) && (*rp != '/'))) {

	        int	pl, ml ;

	        char	buf[MAXPATHLEN + 1] ;


	        rs = proginfo_getpwd(pip,buf,MAXPATHLEN) ;

	        pl = rs ;
	        if ((pl < MAXPATHLEN) && (rl > 0))
	            buf[pl++] = '/' ;

	        ml = MIN(rl,(MAXPATHLEN - pl)) ;
	        cp = strwcpy((buf + pl),rp,ml) ;

	        rs = (cp - buf) ;
	        pip->pr = mallocstrn(buf,(cp - buf)) ;

	    } else
	        pip->pr = mallocstrn(rp,rl) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	    eprintf("proginfo_rootprogdname: pr=%s\n",
	        pip->pr) ;
#endif

	    if (pip->pr == NULL)
	        rs = SR_NOMEM ;

	} /* end if (got one) */

	return rs ;
}
/* end subroutine (proginfo_rootprogdname) */


int proginfo_rootexecname(pip,pp)
struct proginfo	*pip ;
const char	*pp ;
{
	int	rs = SR_OK ;
	int	dl, bl, rl, pl ;
	int	ml ;
	int	f ;

	char	buf[MAXPATHLEN + 1] ;
	char	*dp, *bp, *rp ;
	char	*cp ;


	if (pp == NULL)
	    return SR_FAULT ;

	if ((dl = sfdirname(pp,-1,&dp)) > 0) {

	    if (pip->progdname == NULL)
	        pip->progdname = mallocstrn(dp,dl) ;

	    bl = sfbasename(dp,dl,&bp) ;

	    f = ((bl == 3) && (strncmp(bp,"bin",bl) == 0)) ;

	    if (! f)
	        f = ((bl == 4) && (strncmp(bp,"sbin",bl) == 0)) ;

	    if (f) {

	        freeit(&pip->pr) ;

	        rl = sfdirname(dp,dl,&rp) ;

	        if ((rl > 0) && (*rp != '/')) {

	            rs = proginfo_getpwd(pip,buf,MAXPATHLEN) ;

	            pl = rs ;
	            if (pl < MAXPATHLEN)
	                buf[pl++] = '/' ;

	            ml = MIN(rl,(MAXPATHLEN - pl)) ;
	            cp = strwcpy((buf + pl),rp,ml) ;

	            pip->pr = mallocstrn(buf,(cp - buf)) ;

	        } else
	            pip->pr = mallocstrn(rp,rl) ;

	        if (pip->pr == NULL)
	            rs = SR_NOMEM ;

	    }

	    if (pip->progdname == NULL)
	        rs = SR_NOMEM ;

	} /* end if (had a directory) */

	return rs ;
}
/* end subroutine (proginfo_rootexecname) */


int proginfo_getpwd(pip,buf,buflen)
struct proginfo	*pip ;
char		buf[] ;
int		buflen ;
{
	int	rs ;

	char	lbuf[MAXPATHLEN + 1] ;
	char	*bp = lbuf ;
	char	*cp ;


	if (buf != NULL) {

	    if ((buflen >= 0) && (buflen < MAXPATHLEN))
	        return SR_OVERFLOW ;

	    bp = buf ;
	}

	if (pip->pwd != NULL) {

	    if (buf != NULL) {

	        if (buflen >= 0)
	            cp = strwcpy(buf,pip->pwd,MIN(buflen,MAXPATHLEN)) ;

	        else
	            cp = strwcpy(buf,pip->pwd,MAXPATHLEN) ;

	        rs = (cp - buf) ;

	    } else
	        rs = strlen(pip->pwd) ;

	} else {

	    rs = getpwd(bp,MAXPATHLEN) ;

	    if (rs >= 0) {

	        pip->pwd = mallocstrn(bp,rs) ;

	        if (pip->pwd == NULL)
	            rs = SR_NOMEM ;

	    }
	}

	return rs ;
}
/* end subroutine (proginfo_getpwd) */


int proginfo_initextra(pip,daytime)
struct proginfo	*pip ;
time_t		daytime ;
{


	pip->f.past = FALSE ;

	pip->ti_start = daytime ;
	pip->ti_lastlog = daytime ;

	return SR_OK ;
}
/* end subroutine (proginfo_initextra) */


int proginfo_tellstart(pip,ck,in)
struct proginfo	*pip ;
ULONG		ck, in ;
{


	if (pip == NULL)
		return SR_FAULT ;

	    pip->s.in_start = in ;
	    pip->s.ck_start = ck ;
	return SR_OK ;
}


/* check on some stuff */
int proginfo_tellcheck(pip,f_done,ck,in)
struct proginfo	*pip ;
int		f_done ;
ULONG		ck, in ;
{
	bfile	tellfile ;

	time_t	daytime ;

	ULONG	ins, cks ;

	uint	secs ;

	int	rs = SR_OK ;
	int	f ;

	char	timebuf[TIMEBUFLEN + 1] ;


/* check on log interval for a mark */

	daytime = time(NULL) ;

	if ((daytime - pip->t.synctime) > SYNCINTERVAL) {

	    pip->t.synctime = daytime ;
	    logfile_flush(&pip->lh) ;

	}

	f = f_done ;
	if (! f)
	    f = ((daytime - pip->ti_lastlog) > LOGINTERVAL) ;

	if ((! f) && (pip->linstr > 0))
	    f = ((in - pip->t.in) > pip->linstr) ;

	if (f) {

	    ins = in - pip->s.in_start ;
	    cks = ck - pip->s.ck_start ;
	    secs = daytime - pip->ti_start ;

/* make some log entries */

	    if (pip->f.log) {

	        logfile_printf(&pip->lh,"%s %s %s",
	            timestr_logz(daytime,timebuf),
	            pip->banner,
	            ((f_done) ? "done" : "update")) ;

	        if (pip->realname[0] != '\0')
	            logfile_printf(&pip->lh,"%s!%s (%s)\n",
	                pip->nodename,pip->username,pip->realname) ;

	        else
	            logfile_printf(&pip->lh,"%s!%s\n",
	                pip->nodename,pip->username) ;

	        logfile_printf(&pip->lh,"jobname=%s pid=%d\n",
	            pip->jobname,pip->pid) ;

	        logfile_printf(&pip->lh,"elapsed time %s\n",
	            timestr_elapsed(secs,timebuf)) ;

	        logfile_printf(&pip->lh,"sinstr=%llu ninstr=%llu\n",
	            pip->sinstr,pip->ninstr) ;

	        logfile_printf(&pip->lh,"ck=%llu in=%llu\n",
	            ck,in) ;

	        logfile_printf(&pip->lh,"cks=%llu ins=%llu\n",
	            cks,ins) ;

	    } /* end if (logging) */

/* write the "tell" file */

#if	CF_DEBUG
	    if (DEBUGLEVEL(2))
	        printf("proginfo_check: tellfname=%s\n",pip->tellfname) ;
#endif

	    rs = bopen(&tellfile,pip->tellfname,"wc",0666) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(2))
	        printf("proginfo_check: bopen() rs=%d\n",rs) ;
#endif

	    if (rs >= 0) {

	        double	fcks, fins, fipc, fsecs, fips ;

	        off_t	offset ;

	        char	timebuf[TIMEBUFLEN + 1] ;


	        bseek(&tellfile,0L,SEEK_SET) ;

	        bprintf(&tellfile,"jobname=%s\n",pip->jobname) ;

	        bprintf(&tellfile,"starttime=%s\n",
	            timestr_logz(pip->ti_start,timebuf)) ;

	        bprintf(&tellfile,"daytime=%s\n",
	            timestr_logz(daytime,timebuf)) ;

	        bprintf(&tellfile,"sinstr=%llu\n",pip->sinstr) ;

	        bprintf(&tellfile,"ninstr=%llu\n",pip->ninstr) ;

	        bprintf(&tellfile,"ck=%llu in=%llu\n",ck,in) ;

	        bprintf(&tellfile,"cks=%llu ins=%llu\n",
	            cks,ins) ;

	        fcks = (double) cks ;
	        fins = (double) ins ;
	        fsecs = (double) secs ;

	        fipc = 0.0 ;
	        if (cks != 0)
	            fipc = fins / fcks ;

	        fips = 0.0 ;
	        if (secs != 0)
	            fips = fins / fsecs ;

	        bprintf(&tellfile,"IPC=%12.2f\n",fipc) ;

	        bprintf(&tellfile,"IPH=%12.2f\n",(fips * 3600.0)) ;

/* done */

	        btell(&tellfile,&offset) ;

	        btruncate(&tellfile,offset) ;

	        bclose(&tellfile) ;

	    } /* end if (writing TELL file) */

	    pip->ti_lastlog = daytime ;
	    pip->t.in = in ;
	    pip->t.ck = ck ;

	} /* end if (log mark update) */

	return rs ;
}
/* end if subroutine (proginfo_tellcheck) */


#ifdef	COMMENT

/* write some progress information */
int proginfo_progress(pip,ck,in)
struct proginfo	*pip ;
ULONG		ck, in ;
{
	int	rs = SR_OK ;

	time_t	daytime ;

	char	timebuf[TIMEBUFLEN + 1] ;


	daytime = time(NULL) ;

	if (! pip->f.log) {

	    if ((daytime - pip->ti_lastlog) > LOGINTERVAL)
	        pip->ti_lastlog = daytime ;

	    return rs ;
	}

/* check on log interval for a mark */

	if ((daytime - pip->ti_lastlog) > LOGINTERVAL) {

	    pip->ti_lastlog = daytime ;

	    logfile_printf(&pip->lh,"%s %s!%s\n",
	        timestr_logz(daytime,timebuf),
	        pip->nodename,pip->username) ;

	    logfile_printf(&pip->lh,"jobname=%s pid=%d\n",
	        pip->jobname,pip->pid) ;

	    logfile_printf(&pip->lh,"elapsed time %s\n",
	        timestr_elapsed((daytime - pip->ti_start),timebuf)) ;

	    logfile_printf(&pip->lh,"ck=%llu in=%llu\n",
	        ck,in) ;

	    logfile_flush(&pip->lh) ;

	} /* end if (log mark update) */

	return SR_OK ;
}
/* end if subroutine (proginfo_progress) */

#endif /* COMMENT */


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

	    logfile_printf(&pip->lh,"lc> %w\n",
	        bp,(cp - bp)) ;

	    cp += 1 ;
	    buflen -= (cp - bp) ;
	    bp = cp ;

	} /* end while */

	if (buflen > 0)
	    rs = logfile_printf(&pip->lh,"lc> %w\n",bp,buflen) ;


	return rs ;
}
/* end if subroutine (proginfo_levoconf) */


/* set the selection (if we get one) */
int proginfo_setselect(op,s,slen)
struct proginfo	*op ;
char		s[] ;
int		slen ;
{
	FIELD	fsb ;

	ULONG	ulw ;

	int	rs = SR_OK, i, n ;
	int	sl, cl, cl2 ;

	char	*cp, *cp2 ;


	if (op == NULL)
	    return SR_FAULT ;


	n = 0 ;
	rs = field_init(&fsb,s,slen) ;

	if (rs < 0)
	    goto ret0 ;

	while (field_get(&fsb,oterms) >= 0) {

	    if (fsb.flen == 0)
	        continue ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	    eprintf("proginfo_setselect: option >%w<\n",
	        fsb.fp,fsb.flen) ;
#endif

	    sl = sfkey(fsb.fp,fsb.flen,&cp) ;

	    if (sl >= 0) {

#if	CF_MASTERDEBUG && CF_DEBUGS
	        eprintf("proginfo_setselect: option key >%w<\n",cp,sl) ;
#endif

	        i = optmatch(selects,cp,sl) ;

	        if (i < 0)
	            continue ;

	        cp2 = strnchr((cp + sl),(fsb.flen - sl),'=') ;

	        cl2 = 0 ;
	        if (cp2 != NULL) {

	            cp2 += 1 ;
	            cl2 = fsb.flen - (cp2 - fsb.fp) ;

	        }

#if	CF_MASTERDEBUG && CF_DEBUGS
	        eprintf("proginfo_setselect: option match i=%d\n", i) ;
	        eprintf("proginfo_setselect: cp2=%w\n",cp2,cl2) ;
#endif

	    } else {

	        cp2 = fsb.fp ;
	        cl2 = fsb.flen ;
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

#if	CF_MASTERDEBUG && CF_DEBUGS
	            eprintf("proginfo_setselect: in_start=%lld in_n=%lld\n",
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

#if	CF_MASTERDEBUG && CF_DEBUGS
	            eprintf("proginfo_setselect: ck_start=%lld ck_n=%lld\n",
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

/* re-compute some things (that may have changed) */

	op->ck.end = op->ck.start + op->ck.n ;
	op->in.end = op->in.start + op->in.n ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_setselect: ret rs=%d\n",rs) ;
#endif

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

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_selection: in=%lld start=%lld end=%lld\n",
	    in,op->in.start,op->in.end) ;
#endif

	if (in < op->in.start)
	    f_ie = FALSE ;

	else if (op->in.n == 0)
	    f_ie = TRUE ;

	else
	    f_ie = (in < op->in.end) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_selection: f_ie=%d\n",f_ie) ;
#endif

/* clocks */

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_selection: ck=%lld start=%lld end=%lld\n",
	    ck,op->ck.start,op->ck.end) ;
#endif

	if (ck < op->ck.start)
	    f_ce = FALSE ;

	else if (op->ck.n == 0)
	    f_ce = TRUE ;

	else
	    f_ce = (ck < op->ck.end) ;

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_selection: f_ce=%d\n",f_ce) ;
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

	if (in < pip->in.start) {

	    pip->f.select = FALSE ;

	} else if (pip->in.n == 0) {

	    pip->f.select = TRUE ;

	} else {

	    pip->f.select = (in < pip->in.end) ;
	    pip->f.past = (! pip->f.select) ;

	}

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_dump: %d\n",pip->f.select) ;
#endif

	r = (pip->f.select) ? ((f_old) ? 1 : 2) : 
	0 ;

	return r ;
}
/* end subroutine (proginfo_dump) */


int proginfo_freeextra(pip)
struct proginfo	*pip ;
{


	return SR_OK ;
}
/* end subroutine (proginfo_freeextra) */


int proginfo_setroot(pip,pr,prlen)
struct proginfo	*pip ;
const char	pr[] ;
int		prlen ;
{
	int	rs = SR_OK ;


	if ((pr != NULL) && (pr[0] != '\0')) {

	    if (pip->pr != NULL)
	        free(pip->pr) ;

	    pip->pr = mallocstrn(pr,prlen) ;

	    if (pip->pr == NULL)
	        rs = SR_NOMEM ;

	}

	return rs ;
}
/* end subroutine (proginfo_setroot) */



/* LOCAL SUBROUTINES */



static int proginfo_procopts(op,s,slen)
struct proginfo	*op ;
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

#if	CF_MASTERDEBUG && CF_DEBUGS
	    eprintf("proginfo_procopts: option >%w<\n",
	        fsb.fp,fsb.flen) ;
#endif

	    if ((sl = sfkey(fsb.fp,fsb.flen,&cp)) < 0) {

	        cp = fsb.fp ;
	        sl = fsb.flen ;
	    }

#if	CF_MASTERDEBUG && CF_DEBUGS
	    eprintf("proginfo_procopts: option key >%w<\n",cp,sl) ;
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

#if	CF_MASTERDEBUG && CF_DEBUGS
	    eprintf("proginfo_procopts: option match i=%d\n", i) ;
	    eprintf("proginfo_procopts: cp2=%w\n",cp2,sl2) ;
#endif

	    switch (i) {

	    case opt_sin:
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

#if	CF_MASTERDEBUG && CF_DEBUGS
	            eprintf("proginfo_procopts: in_start=%lld in_n=%lld\n",
	                op->in.start,op->in.n) ;
#endif

	        } /* end if */

	        break ;

	    case opt_sck:
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

#if	CF_MASTERDEBUG && CF_DEBUGS
	            eprintf("proginfo_procopts: ck_start=%lld ck_n=%lld\n",
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

#if	CF_MASTERDEBUG && CF_DEBUGS
	eprintf("proginfo_procopts: exiting rs=%d\n",rs) ;
#endif

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (proginfo_procopts) */


static int setit(ap,vp)
void		*ap ;
const void	*vp ;
{
	caddr_t	*p = (caddr_t *) ap ;


	if (vp == NULL)
	    return SR_OK ;

	if (*p != NULL)
	    free(*p) ;

	*p = mallocstr(vp) ;

	return (*p != NULL) ? SR_OK : SR_FAULT ;
}
/* end subroutine (setit) */


/* free up a malloc'ed thing */
static void freeit(ap)
void		*ap ;
{
	caddr_t	*p = (caddr_t *) ap ;


	if (*p != NULL) {

	    free(*p) ;

	    *p = NULL ;
	}
}
/* end subroutine (freeit) */



