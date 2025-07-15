/* proginfo */

/* program information */

#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_DEBUGN	0		/* non-switchable debug print-outs */
#define	CF_GETEXECNAME	1		/* use 'getexecname()' */
#define	CF_RMEXT	1		/* remove extension */

/* revision history:

	= 1998-03-17, David Morano
	I enhanced this somewhat from my previous version.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This group of subroutines help find and set from variables
	for program start-up type functions.

*******************************************************************************/

#include	<envstandards.h>	/* MUST be first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<vsystem.h>
#include	<vecstr.h>
#include	<shellunder.h>
#include	<rmx.h>
#include	<hasx.h>
#include	<localmisc.h>

#include	"defs.h"


/* local defines */

#ifndef	NODENAMELEN
#define	NODENAMELEN	257
#endif

#define	NOPROGNAME	"NP"

#ifndef	VAREXECFNAME
#define	VAREXECFNAME	"_EF"
#endif

#ifndef	VARUNDER
#define	VARUNDER	"_"
#endif

#define	NDEBFNAME	"proginfo.deb"


/* external subroutines */

extern int	sncpy1(char *,int,const char *) ;
extern int	mkpath1(char *,const char *) ;
extern int	mkpath2(char *,const char *,const char *) ;
extern int	mkpath2w(char *,const char *,const char *,int) ;
extern int	sfdirname(const char *,int,const char **) ;
extern int	sfbasename(const char *,int,const char **) ;
extern int	matstr(const char **,const char *,int) ;
extern int	getnodename(char *,int) ;
extern int	getpwd(char *,int) ;
extern int	getev(const char **,const char *,int,const char **) ;
extern int	hasprintbad(const char *,int) ;
extern int	hasuc(const char *,int) ;

#if	CF_DEBUGN
extern int	nprintf(const char *,const char *,...) ;
#endif

extern char	*getourenv(const char **,const char *) ;
extern char	*strwcpy(char *,const char *,int) ;
extern char	*strwcpylc(char *,const char *,int) ;
extern char	*strwcpyuc(char *,const char *,int) ;
extern char	*strnchr(const char *,int,int) ;
extern char	*strnrchr(const char *,int,int) ;


/* forward references */

int	proginfo_setentry(struct proginfo *,const char **,const char *,int) ;
int	proginfo_setprogname(struct proginfo *,const char *) ;
int	proginfo_setexecname(struct proginfo *,const char *) ;
int	proginfo_getpwd(struct proginfo *,char *,int) ;
int	proginfo_pwd(struct proginfo *) ;
int	proginfo_progdname(struct proginfo *) ;
int	proginfo_progename(struct proginfo *) ;

static int proginfo_setdefnames(struct proginfo *) ;

#ifdef	COMMENT
static int proginfo_setdefdn(struct proginfo *) ;
static int proginfo_setdefpn(struct proginfo *) ;
#endif


/* local variables */

#if	CF_RMEXT
static const char	*exts[] = {
	"x",
	"s5",
	"s5u",
	"us5",
	"s4",
	"aout",
	"elf",
	"ksh",
	"sh",
	"ksh93",
	"csh",
	"osf",
	NULL
} ;
#endif /* CF_RMEXT */


/* exported subroutines */


int proginfo_start(pip,envv,argz,version)
struct proginfo	*pip ;
const char	*envv[] ;
const char	argz[] ;
const char	version[] ;
{
	int	rs ;
	int	opts ;


	if (pip == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	debugprintf("proginfo_start: argz=%s\n",argz) ;
#endif

	memset(pip,0,sizeof(struct proginfo)) ;

	pip->envv = (const char **) envv ;

	opts = (VECSTR_OCONSERVE | VECSTR_OREUSE | VECSTR_OSWAP) ;
	if ((rs = vecstr_start(&pip->stores,10,opts)) >= 0) {
	    if ((rs = proginfo_pwd(pip)) >= 0) {
	        if ((rs >= 0) && (argz != NULL)) {
	            rs = proginfo_setprogname(pip,argz) ;
		}
		if (rs >= 0) {
	    	    if ((rs = proginfo_setdefnames(pip)) >= 0) {
			if ((rs >= 0) && (version != NULL)) {
	    		    const char	**vpp = &pip->version ;
	    		    rs = proginfo_setentry(pip,vpp,version,-1) ;
			}
		    }
		}
	    }
	    if (rs < 0)
		vecstr_finish(&pip->stores) ;
	} /* end if (vecstr-stores) */

	return rs ;
}
/* end subroutine (proginfo_start) */


int proginfo_finish(pip)
struct proginfo	*pip ;
{
	int	rs ;


	if (pip == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGN
	proginfo_storelists(pip,"freeing") ;
	rs = vecstr_finish(&pip->stores) ;
	nprintf(NDEBFNAME,"proginfo_finish: ret rs=%d\n",rs) ;
#else
	rs = vecstr_finish(&pip->stores) ;
#endif

	return rs ;
}
/* end subroutine (proginfo_finish) */


/* set an entry */
int proginfo_setentry(pip,epp,vp,vl)
struct proginfo	*pip ;
const char	**epp ;
const char	*vp ;
int		vl ;
{
	int	rs = SR_OK ;
	int	oi = -1 ;
	int	len = 0 ;


	if (pip == NULL) return SR_FAULT ;
	if (epp == NULL) return SR_FAULT ;

/* find existing entry for later deletion */

	if (*epp != NULL)
	    oi = vecstr_findaddr(&pip->stores,*epp) ;

/* add the new entry */

	if (vp != NULL) {
	    len = strnlen(vp,vl) ;
	    rs = vecstr_store(&pip->stores,vp,len,epp) ;
	} else
	    *epp = NULL ;

/* delete the old entry if we had one */

	if ((rs >= 0) && (oi >= 0))
	    vecstr_del(&pip->stores,oi) ;

	return (rs >= 0) ? len : rs ;
}
/* end subroutine (proginfo_setentry) */


#if	CF_DEBUGN
int proginfo_storelists(pip,s)
struct proginfo	*pip ;
const char	s[] ;
{
	VECSTR	*vsp = &pip->stores ;
	int	i ;
	const char	*cp ;
	if (s != NULL)
	    nprintf(NDEBFNAME,"proginfo_storelists: s=>%s<\n",s) ;
	nprintf(NDEBFNAME,"proginfo_storelists: vi=%u\n",vsp->i) ;
	for (i = 0 ; vecstr_get(vsp,i,&cp) >= 0 ; i += 1) {
	    if (cp == NULL) continue ;
	    nprintf(NDEBFNAME,"proginfo_storelists: s[%u](%p)\n",i,cp) ;
	    nprintf(NDEBFNAME,"proginfo_storelists: s[%u]=>%s<\n",i,cp) ;
	}
	nprintf(NDEBFNAME,"proginfo_storelists: done\n") ;
	return vsp->i ;
}
/* end subroutine (proginfo_storelists) */
#endif /* CF_DEBUGN */


int proginfo_setversion(pip,v)
struct proginfo	*pip ;
const char	v[] ;
{
	int	rs ;


	if (pip == NULL)
	    return SR_FAULT ;

	if (v == NULL)
	    return SR_FAULT ;

	rs = proginfo_setentry(pip,&pip->version,v,-1) ;

	return rs ;
}
/* end subroutine (proginfo_setversion) */


int proginfo_setbanner(pip,v)
struct proginfo	*pip ;
const char	v[] ;
{
	int	rs ;


	if (pip == NULL)
	    return SR_FAULT ;

	if (v == NULL)
	    return SR_FAULT ;

	rs = proginfo_setentry(pip,&pip->banner,v,-1) ;

	return rs ;
}
/* end subroutine (proginfo_setbanner) */


int proginfo_setsearchname(pip,var,value)
struct proginfo	*pip ;
const char	var[] ;
const char	value[] ;
{
	int	rs = SR_OK ;
	int	cl = -1 ;

	const char	*cp = NULL ;


	if (pip == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGN
	nprintf(NDEBFNAME,"proginfo_setprogname: value=%s\n",value) ;
#endif

	if ((cp == NULL) && (value != NULL)) {
	    cp = value ;
	}

	if ((cp == NULL) && (var != NULL)) {
	    cp = getourenv(pip->envv,var) ;
	    if (hasourbad(cp,-1) || ((cp != NULL) && (cp[0] == '\0')))
	        cp = NULL ;
	}

	if ((cp == NULL) && (pip->progname != NULL)) {
	    const char	*tp ;
	    cp = pip->progname ;
	    if ((tp = strrchr(cp,'.')) != NULL)
	        cl = (tp - cp) ;
	}

	if ((rs >= 0) && (cp != NULL)) {
	    char	searchname[MAXNAMELEN+1] ;
	    int		ml = MAXNAMELEN ;
	    if (hasuc(cp,cl)) {
	        if ((cl > 0) && (cl < ml)) ml = cl ;
	        cl = strwcpylc(searchname,cp,ml) - searchname ;
	        cp = searchname ;
	    }
	    rs = proginfo_setentry(pip,&pip->searchname,cp,cl) ;
	}

	return rs ;
}
/* end subroutine (proginfo_setsearchname) */


/* set program directory and program name (as might be possible) */
int proginfo_setprogname(pip,ap)
struct proginfo	*pip ;
const char	ap[] ;
{
	int	rs = SR_OK ;
	int	al ;
	int	dl, bl ;

	const char	*en = NULL ;
	const char	*dn = NULL ;
	const char	*dp, *bp ;


	if (ap == NULL)
	    return SR_FAULT ;

	en = pip->progename ;
	dn = pip->progdname ;
	al = strlen(ap) ;

#if	CF_DEBUGN
	nprintf(NDEBFNAME,"proginfo_setprogname: argz=%s\n",ap) ;
#endif

	while ((al > 0) && (ap[al-1] == '/'))
	    al -= 1 ;

	bl = sfbasename(ap,al,&bp) ;

/* set a program dirname? */

	if (((en == NULL) || (dn == NULL)) && 
	    ((dl = sfdirname(ap,al,&dp)) > 0)) {
	    int	f_progename = TRUE ;
	    int		f_parent = FALSE ;
	    int		f_pwd = FALSE ;
	    int		f = FALSE ;

#if	CF_DEBUGN
	    nprintf(NDEBFNAME,"proginfo_setprogname: dirname=%t\n",dp,dl) ;
#endif

	    if (dp[0] == '.') {
	        f_progename = FALSE ;

	        f_pwd = (strcmp(dp,".") == 0) ;

	        if (! f_pwd)
	            f_parent = (strcmp(dp,"..") == 0) ;

	        f = f_pwd || f_parent ;

	    } /* end if */

	    if (f) {

	        if (pip->pwd == NULL)
	            rs = proginfo_pwd(pip) ;

	        if (rs >= 0) {

	            if (f_pwd) {
	                dp = pip->pwd ;
	                dl = pip->pwdlen ;
	            } else {
	                dl = sfdirname(pip->pwd,pip->pwdlen,&dp) ;
	            }

	        } /* end if (PWD or parent) */

	    } /* end if */

	    if ((rs >= 0) && (pip->progdname == NULL))
	        rs = proginfo_setentry(pip,&pip->progdname,dp,dl) ;

	    if ((rs >= 0) && (pip->progename == NULL)) {
	        if (f_progename) {
	            rs = proginfo_setentry(pip,&pip->progename,dp,dl) ;
	        } else if ((bp != NULL) && (bl > 0)) {
	            char	ename[MAXPATHLEN+1] ;
	            rs = mkpath2w(ename,pip->progdname,bp,bl) ;
	            if (rs >= 0)
	                rs = proginfo_setentry(pip,&pip->progename,ename,-1) ;
	        }
	    } /* end if */

	} /* end if (have a dirname) */

/* set a program basename? */

	if (rs >= 0) {

	    if ((bp != NULL) && (bl > 0)) {
	        if ((bl = rmext(bp,bl)) == 0) {
	            bp = NOPROGNAME ;
	            bl = -1 ;
	        }
	    }

	    if ((bp != NULL) && (bl > 0)) {
	        if (hasourbad(bp,bl)) {
	            bp = NULL ;
	            bl = 0 ;
	        }
	    }

	    if ((bp != NULL) && (bl > 0) && (bp[0] == '-')) {
	        pip->f.progdash = TRUE ;
	        bp += 1 ;
	        bl -= 1 ;
	    }

#if	CF_DEBUGN
	    nprintf(NDEBFNAME,"proginfo_setprogname: progname=%t\n",bp,bl) ;
#endif

	    if ((bp != NULL) && (bl != 0))
	        rs = proginfo_setentry(pip,&pip->progname,bp,bl) ;

	} /* end if (basename) */

	return rs ;
}
/* end subroutine (proginfo_setprogname) */


/* set program root */
int proginfo_setprogroot(pip,prp,prl)
struct proginfo	*pip ;
const char	prp[] ;
int		prl ;
{
	const int	tlen = MAXPATHLEN ;

	int	rs = SR_OK ;

	char	tbuf[MAXPATHLEN + 1] ;


	if (prp == NULL)
	    return SR_FAULT ;

	if (prl < 0) prl = strlen(prp) ;

	if (prp[0] != '/') {
	    if ((rs = proginfo_pwd(pip)) >= 0) {
	        rs = mkpath2w(tbuf,pip->pwd,prp,prl) ;
	        prl = rs ;
	        prp = tbuf;
	    }
	}

	if (rs >= 0) {
	    const char	**vpp = &pip->pr ;
	    rs = proginfo_setentry(pip,vpp,prp,prl) ;
	}

	return rs ;
}
/* end subroutine (proginfo_setprogroot) */


/* set the program execution filename */
int proginfo_setexecname(pip,enp)
struct proginfo	*pip ;
const char	*enp ;
{
	int	rs = SR_OK ;

	if (enp == NULL)
	    return SR_FAULT ;

	if (pip->progename == NULL) {
	    int	enl = strlen(enp) ;
	    while ((enl > 0) && (enp[enl-1] == '/')) enl -= 1 ;
	    if (enl > 0) {
		const char	**vpp = &pip->progename ;
	        rs = proginfo_setentry(pip,vpp,enp,enl) ;
	    }
	}

	return rs ;
}
/* end subroutine (proginfo_setexecname) */


/* ensure (set) that the current PWD is set */
int proginfo_pwd(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;
	int	pwdlen = 0 ;


	if (pip->pwd == NULL) {
	    char	pwdname[MAXPATHLEN + 1] ;
	    if ((rs = getpwd(pwdname,MAXPATHLEN)) >= 0) {
		const char	**vpp = &pip->pwd ;
	        pwdlen = rs ;
	        pip->pwdlen = pwdlen ;
	        rs = proginfo_setentry(pip,vpp,pwdname,pwdlen) ;
	    }
	} else
	    pwdlen = pip->pwdlen ;

	return (rs >= 0) ? pwdlen : rs ;
}
/* end subroutine (proginfo_pwd) */


int proginfo_progdname(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;


	if (pip->progdname == NULL) {

	    rs = proginfo_progename(pip) ;

#if	CF_DEBUGS
	    debugprintf("proginfo_progdname: progename=%s\n",pip->progename) ;
#endif

	    if ((rs >= 0) && (pip->progename != NULL)) {
	        int		dl ;
	        const char	*dp ;
	        dl = sfdirname(pip->progename,-1,&dp) ;
	        if (dl == 0) {
	            if (pip->pwd == NULL)
	                rs = proginfo_pwd(pip) ;
	            if ((rs >= 0) && (pip->pwd != NULL)) {
	                dp = pip->pwd ;
	                dl = -1 ;
	            }
	        }
	        if ((rs >= 0) && (dl > 0))
	            rs = proginfo_setentry(pip,&pip->progdname,dp,dl) ;
	    } /* end if */

	} else
	    rs = strlen(pip->progdname) ;

	return rs ;
}
/* end subroutine (proginfo_progdname) */


/* Set Default (program) Exec-Name */
int proginfo_progename(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;


#if	CF_DEBUGN
	nprintf(NDEBFNAME,"proginfo_progename: ent\n") ;
#endif

	if (pip->progename == NULL) {
	    SHELLUNDER	su ;
	    const char	*execfname = NULL ;

#if	defined(OSNAME_SunOS) && (OSNAME_SunOS > 0) && CF_GETEXECNAME
	    if ((rs >= 0) && (execfname == NULL)) {
	        execfname = getexecname() ;
#if	CF_DEBUGN
	        nprintf(NDEBFNAME,"proginfo_progename: getexecname() wn=%s\n",
	            execfname) ;
#endif
	    }
#endif /* SOLARIS */

	    if ((rs >= 0) && (execfname == NULL))
	        execfname = getourenv(pip->envv,VAREXECFNAME) ;

	    if ((rs >= 0) && (execfname == NULL)) {
	        const char	*cp = getourenv(pip->envv,VARUNDER) ;
	        if (cp != NULL) {
	            int	rs1 = shellunder(&su,cp) ;
	            if (rs1 > 0) execfname = su.progename ;
	        }
	    }

	    if ((rs >= 0) && (execfname != NULL)) {
	        char	tmpfname[MAXPATHLEN+1] ;
	        if (execfname[0] != '/') {
	            if (pip->pwd == NULL) rs = proginfo_pwd(pip) ;
	            if (rs >= 0) {
	                rs = mkpath2(tmpfname,pip->pwd,execfname) ;
	                execfname = tmpfname ;
	            }
	        }
	        if (rs >= 0)
	            rs = proginfo_setexecname(pip,execfname) ;
	    }

#if	CF_DEBUGN
	    nprintf(NDEBFNAME,"proginfo_progename: mid rs=%d en=%s\n",
	        rs,pip->progename) ;
#endif

	} else
	    rs = strlen(pip->progename) ;

ret0:

#if	CF_DEBUGN
	nprintf(NDEBFNAME,"proginfo_progename: ret rs=%d en=%s\n",
	    rs,pip->progename) ;
#endif

	return rs ;
}
/* end subroutine (proginfo_progename) */


/* ensure (set) nodename */
int proginfo_nodename(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;
	int	nl = 0 ;

	if (pip->nodename == NULL) {
	    const int	nlen = NODENAMELEN ;
	    char	nbuf[NODENAMELEN + 1] ;
	    if ((rs = getnodename(nbuf,nlen)) >= 0) {
		const char	**vpp = &pip->nodename ;
	        nl = rs ;
	        rs = proginfo_setentry(pip,vpp,nbuf,nl) ;
	    }
	} else
	    nl = strlen(pip->nodename) ;

	return (rs >= 0) ? nl : rs ;
}
/* end subroutine (proginfo_nodename) */


int proginfo_getename(pip,rbuf,rlen)
struct proginfo	*pip ;
char		rbuf[] ;
int		rlen ;
{
	int	rs = SR_OK ;


	if (pip == NULL)
	    return SR_FAULT ;

	if (rbuf == NULL)
	    return SR_FAULT ;

	rbuf[0] = '\0' ;
	if (pip->progename == NULL)
	    rs = proginfo_progename(pip) ;

	if (rs >= 0) {
	    rs = SR_NOENT ;
	    if (pip->progename != NULL)
	        rs = sncpy1(rbuf,rlen,pip->progename) ;
	}

	return rs ;
}
/* end subroutine (proginfo_getename) */


/* get the PWD when it was first set */
int proginfo_getpwd(pip,rbuf,rlen)
struct proginfo	*pip ;
char		rbuf[] ;
int		rlen ;
{
	int	rs = SR_OK ;


	if (pip == NULL)
	    return SR_FAULT ;

	if (rbuf == NULL)
	    return SR_FAULT ;

	rbuf[0] = '\0' ;
	if (pip->pwd == NULL)
	    rs = proginfo_pwd(pip) ;

	if (rs >= 0) {
	    if (rlen >= pip->pwdlen) {
	        rs = sncpy1(rbuf,rlen,pip->pwd) ;
	    } else
	        rs = SR_OVERFLOW ;
	}

	return rs ;
}
/* end subroutine (proginfo_getpwd) */


int proginfo_getenv(pip,np,nl,rpp)
struct proginfo	*pip ;
const char	*np ;
int		nl ;
const char	**rpp ;
{
	int	rs = SR_NOTFOUND ;


	if (np == NULL)
	    return SR_FAULT ;

	if (pip->envv != NULL)
	    rs = getev(pip->envv,np,nl,rpp) ;

	return rs ;
}
/* end subroutine (proginfo_getenv) */


/* local subroutines */


static int proginfo_setdefnames(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;

	if (pip->progname == NULL) {
	    rs = proginfo_progename(pip) ;
	    if ((rs >= 0) && (pip->progename != NULL))
	        rs = proginfo_setprogname(pip,pip->progename) ;
	} /* end if */

	return rs ;
}
/* end subroutine (proginfo_setdefnames) */


#ifdef	COMMENT

/* Set Default (program) Directory-Name */
static int proginfo_setdefdn(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;
	int	cl ;

	const char	*cp ;


	if (pip->progdname == NULL) {
	    if ((rs >= 0) && (pip->progename != NULL)) {
	        if ((cl = sfdirname(pip->progename,-1,&cp)) > 0)
	            rs = proginfo_setentry(pip,&pip->progdname,cp,cl) ;
	    } /* end if */
	} /* end if */

	return rs ;
}
/* end subroutine (proginfo_setdefdn) */


/* Set Default Program-Name */
static int proginfo_setdefpn(pip)
struct proginfo	*pip ;
{
	int	rs = SR_OK ;

	if (pip->progname == NULL) {
	    int		cl ;
	    const char	*cp ;
	    if (pip->progename != NULL) {
	        if ((cl = sfbasename(pip->progename,-1,&cp)) > 0) {
	            cl = rmext(cp,cl) ;
	            if (cl == 0) {
	                cp = NOPROGNAME ;
	                cl = -1 ;
	            }
	        }
	        if ((cp != NULL) && (cl != 0))
	            rs = proginfo_setentry(pip,&pip->progdname,cp,cl) ;
	    } /* end if */
	} /* end if */

	return rs ;
}
/* end subroutine (proginfo_setdefpn) */

#endif /* COMMENT */


