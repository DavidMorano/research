/* debug SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* debugging stubs */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* compile-time debugging */

/* revision history:

	= 1998-01-15, David Morano
	This was written to debug the REXEC program.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

  	Group:
	debug

	Description:
	This modeule provides debugging support for the REXEC program.

*******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<csignal>
#include	<stropts.h>
#include	<cstdlib>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>		/* |getenv(3c)| */
#include	<cstring>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<uclibmem.h>
#include	<estrings.h>
#include	<storebuf.h>
#include	<localmisc.h>

#include	"debug.h"

/* local defines */

#define	PRINTBUFLEN	(COLUMNS + 2)
#define	HEXBUFLEN	100


/* external subroutines */

extenr "C" {
    extern int	bufprintf(char *,int,cchar *,...) noex ;
    extern int	nprintf(cchar *,cchar *,...) noex ;
    extern int	debugprintf(cchar *,...) noex ;
    extern int	debugprint(cchar *,int) noex ;
}


/* external variables */


/* local structures */

struct debug_oflags {
	int	m ;
	char	*s ;
} ;


/* forward references */

int		mkhexstr(char *,int,cvoid *,int) noex ;

local int	checkbasebounds(cchar *,int,void *) noex ;


/* external variables */


/* local variables */

constexpr char	hextable[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
} ;


/* exported variables */


/* exported subroutines */

char *d_reventstr(int revents,char *buf,int buflen) noex {

#if	CF_DEBUGS
	debugprintf("d_reventstr: ent\n") ;
#endif

	buf[0] = '\0' ;
	bufprintf(buf,buflen,"%s %s %s %s %s %s %s %s %s",
	    (revents & POLLIN) ? "I " : "  ",
	    (revents & POLLRDNORM) ? "IN" : "  ",
	    (revents & POLLRDBAND) ? "IB" : "  ",
	    (revents & POLLPRI) ? "PR" : "  ",
	    (revents & POLLWRNORM) ? "WN" : "  ",
	    (revents & POLLWRBAND) ? "WB" : "  ",
	    (revents & POLLERR) ? "ER" : "  ",
	    (revents & POLLHUP) ? "HU" : "  ",
	    (revents & POLLNVAL) ? "NV" : "  ") ;

#if	CF_DEBUGS
	debugprintf("d_reventstr: %s\n",buf) ;
#endif

	return buf ;
}
/* end subroutine (d_reventstr) */

/* who is open? */
void d_whoopen(cchar *s) noex {
	int	rs = SR_OK ;

	if (s) {
	    debugprintf("d_whoopen: %s\n",s) ;
	}
	for (int i = 0 ; i < 20 ; i += 1) {
	    if ((rs = u_fcntl(i,F_GETFL,0)) >= 0) {
	        debugprintf("d_whoopen: open on %d accmod=%08x\n",
	            i,(rs & O_ACCMODE)) ;
	    }
	} /* end for */

}
/* end subroutine (d_whoopen) */

/* return a count of the number of open files */
int d_openfiles() noex {
	int	count = 0 ;
	for (int i = 0 ; i < 2048 ; i += 1) {
	    if (ustat sb ; (u_fstat(i,&sb) >= 0)) {
	        count += 1 ;
	    }
	} /* end for */
	return count ;
}
/* end subroutine (d_openfiles) */

int d_ispath(cchar *p) noex {
	if (p == NULL)
	    return FALSE ;

#ifdef	DEBFILE
	nprintf(DEBFILE,"d_ispath: PATH=>%W<\n",
	    p,strnlen(p,30)) ;
#endif

	return ((*p == '/') || (*p == ':')) ;
}
/* end subroutine (d_ispath) */

int gdb() noex {
	return 0 ;
}
/* end subroutine (gdb) */

int mkhexstr(char *dbuf,int dlen,cvoid *vp,int vl) noex {
	int	sl = vl ;
	int	ch ;
	int	j = 0 ; /* used-afterwards */
	const uchar	*sp = (const uchar *) vp ;

	if (sl < 0) sl = strlen(sp) ;

	for (int i = 0 ; (dlen >= 3) && (i < sl) ; i += 1) {
	    ch = sp[i] ;

	    if (i > 0) dbuf[j++] = ' ' ;
	    dbuf[j++] = hextable[(ch>>4)&15] ;
	    dbuf[j++] = hextable[(ch>>0)&15] ;

	    dlen -= ((i > 0) ? 3 : 2) ;
	} /* end for */
	dbuf[j] = '\0' ;

	return j ;
}
/* end subroutine (mkhexstr) */

int mkhexnstr(char *hbuf,int hlen,int maxcols,cchar *sbuf,int slen) noex {
	int	n = 0 ;

	if (maxcols < 0) maxcols = COLUMNS ;

	if (slen < 0) slen = strlen(sbuf) ;
	n = MIN((maxcols / 3),slen) ;
	mkhexstr(hbuf,hlen,sbuf,n) ;

	return n ;
}
/* end subroutine (mkhexnstr) */

int debugprinthex(char *ids,int maxcols,cchar *sp,int sl) noex {
	int	rs ;
	int	idlen = 0 ;
	int	wlen = 0 ;

	char	printbuf[PRINTBUFLEN + 1] ;

	if (ids != NULL) idlen = strlen(ids) ;

	if (maxcols < 0) maxcols = COLUMNS ;

	if (idlen > 0) maxcols -= (idlen + 1) ;

	rs = mkhexnstr(printbuf,PRINTBUFLEN,maxcols,sp,sl) ;

	if (rs >= 0) {
	    if (idlen > 0) {
	        rs = debugprintf("%t %s\n",ids,idlen,printbuf) ;
	    } else {
	        rs = debugprintf("%s\n",printbuf) ;
	    }
	    wlen = rs ;
	} /* end if (ok) */

	return (rs >= 0) ? wlen : rs ;
}
/* end subroutine (debugprinthex) */

int debugprinthexblock(cchar *ids,int maxcols,cvoid *vp,int vl) noex {
	int	rs = SR_OK ;
	int	idlen = 0 ;
	int	n ;
	int	pbl ;
	int	cslen ;
	int	cols ;
	int	len ;
	int	sl = vl ;
	int	wlen = 0 ;

	cchar	*sp = (cchar *) vp ;

	char	printbuf[PRINTBUFLEN + 1] ;
	char	*pbp ;


	if (ids != NULL) idlen = strlen(ids) ;

	if (maxcols < 0) maxcols = COLUMNS ;

	if (sl < 0) sl = strlen(sp) ;

	i = 0 ;
	while ((rs >= 0) && (sl > 0)) {

	    pbp = printbuf ;
	    pbl = PRINTBUFLEN ;
	    cols = maxcols ;

	    if (ids != NULL) {
	        if ((idlen+2) < pbl) {
	            int i = strwcpy(pbp,ids,idlen) - pbp ;
		    pbp[i++] = ':' ;
		    pbp[i++] = ' ' ;
		    pbp += i ;
		    pbl -= i ;
		    cols -= i ;
		} else {
		    rs = SR_OVERFLOW ;
		}
	    } /* end if (non-null) */

	    if (rs >= 0) {
	        n = (cols / 3) ;
	            cslen = MIN(n,sl) ;
	            rs = mkhexstr(pbp,pbl,sp,cslen) ;
	            sp += cslen ;
	            sl -= cslen ;
	    }

	    if (rs >= 0) {
	        len = debugprint(printbuf,-1) ;
	        wlen += len ;
	    }

	} /* end while */

	return (rs >= 0) ? wlen : rs ;
}
/* end subroutine (debugprinthexblock) */

int hexblock(cchar *ids,cchar *ap,int n) noex {
	int	sl ;
	char	hexbuf[HEXBUFLEN + 3] ;

	if (ids != NULL)
	    debugprint(ids,-1) ;

	for (int i = 0 ; i < n ; i += 1) {

	    sl = mkhexstr(hexbuf,HEXBUFLEN,ap,4) ;

	    hexbuf[sl++] = '\n' ;
	    hexbuf[sl] = '\0' ;

	    ap += 4 ;

	    debugprint(hexbuf,-1) ;

	} /* end for */

	return n ;
}
/* end subroutine (hexblock) */

/* audit a HOSTENT structure */
int heaudit(hostent *hep,cchar *buf,int buflen) noex {
	int	rs = SR_OK ;
	char	**cpp ;

	if (hep == NULL)
	    return SR_FAULT ;

	if (buf == NULL)
	    return SR_FAULT ;

	if (buflen < 0)
	    return SR_INVALID ;

	if (rs >= 0) {
	    rs = checkbasebounds(buf,buflen,hep->h_name) ;
	}

	if (rs >= 0) {
	    cpp = hep->h_aliases ;
	    if (cpp != NULL) {
	        rs = checkbasebounds(buf,buflen,cpp) ;
	        if (rs >= 0) {
	            for (int i = 0 ; cpp[i] != NULL ; i += 1) {
	                rs = checkbasebounds(buf,buflen,(cpp + i)) ;
	                if (rs >= 0) {
	                    rs = checkbasebounds(buf,buflen,cpp[i]) ;
			}
	                if (rs < 0) break ;
	            } /* end for */
	        }
	    }
	}

	if (rs >= 0) {
	    cpp = hep->h_aliases ;
	    if (cpp != NULL) {
	        rs = checkbasebounds(buf,buflen,cpp) ;

	        if (rs >= 0) {
	            for (int i = 0 ; cpp[i] != NULL ; i += 1) {
	                rs = checkbasebounds(buf,buflen,(cpp + i)) ;
	                if (rs >= 0) {
	                    rs = checkbasebounds(buf,buflen,cpp[i]) ;
			}
	                if (rs < 0) break ;
	            } /* end for */
	        }
	    }
	}

	return rs ;
}
/* end subroutine (heaudit) */

char *stroflags(char *buf,int oflags) noex {
	int	rs = snflagsopen(buf,TIMEBUFLEN,oflags) ;
	return (rs >= 0) ? buf : NULL ;
}
/* end subroutine (stroflags) */


/* local subroutines */

local int checkbasebounds(cchar *bbuf,int blen,void *vp) noex {
	int	rs = SR_OK ;
	cchar	*tp = (cchar *) vp ;

	if ((rs >= 0) && (tp < bbuf))
	    rs = SR_BADFMT ;

	if ((rs >= 0) && (tp >= (bbuf + blen)))
	    rs = SR_BADFMT ;

	return rs ;
}
/* end subroutine (checkbasebounds) */


