/* convert SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* convert a trace */
/* version %I% last-modified %G% */

#define	CF_DEBUG	0		/* debugging */

/* revision history:

	= 1001-09-01, David Morano
	This subroutine was originally written.

*/

/* Copyright © 2001 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

#include	<envstandards.h>	/* MUST be first to configure */
#include	<sys/types.h>
#include	<cstddef>		/* CSTD */
#include	<cstdlib>		/* CSTD */
#include	<cstring>		/* CSTD */
#include	<clanguage.h>		/* LIBU */
#include	<usysbase.h>		/* LIBU */
#include	<baops.h>		/* LIBU */
#include	<ascii.h>		/* LIBU */
#include	<strx.h>		/* LIBUC */
#include	<cfdec.h>		/* LIBUC */
#include	<cfhex.h>		/* LIBUC */
#include	<cfnum.h>		/* LIBUC */
#include	<char.h>		/* LIBUC */
#include	<localmisc.h>		/* LIBU */
#include	<libdebug.h>		/* LIBDEBUG |DEBUGPRINTF(3debug)| */

#include	"config.h"
#include	"defs.h"
#include	"exectrace.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |lenstr(3u)| */

/* local defines */

#define	MATHX(b)	(((b)[0] == '0') && ((b)[1] == 'x'))
#define	CHARISWE(c)	(((c) == ' ') || ((c) == '\t') || ((c) == '\n'))
#define	BITBYTES	((EXECTRACE_NREG / 8) + 1)

#ifndef	PI
#define	PI		proginfo
#endif


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

local int	matinstr(cchar *,char **,char **) noex ;
local int	getregval(PI *,char *,int,uint *,uint *) noex ;


/* local (module-scope static) data */


/* exported variables */


/* exported subroutines */

int convert(PI *pip,char *t1fname,char *t2fname) noex {
	EXECTRACE		t ;
	bfile		dbxfile ;
	traceinfo	ti{} ;
	ulong	in = 0 ;
	long	badsub = -1 ;
	int	rs, i, j ;
	int	len ;
	int	sl, il ;
	int	opletter ;
	char	linebuf[LINELEN + 1] = {} ;
	char	regs[BITBYTES] = {} ;
	char	*sp, *cp ;
	char	*ip, *lp ;

	/* open the traces */
#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    DEBUGPRINTF("t1=%s t2=%s\n",
	        t1fname,t2fname) ;
#endif

	rs = bopen(&dbxfile,t1fname,"r",0666) ;
	if (rs < 0) return rs ;


	rs = exectrace_open(&t,t2fname,"w",0666,nullptr) ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    DEBUGPRINTF("exectrace_open() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad1 ;


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    DEBUGPRINTF("starting\n") ;
#endif

	/* loop reading them */
	il = 0 ;
	forever {
	    uint	ia, ra, rv ;
	    if (il == 0) {

#if	CF_DEBUG
	        if (pip->debuglevel >= 5)
	            DEBUGPRINTF("need top new line\n") ;
#endif

	        while ((il = matinstr(linebuf,&ip,&lp)) == 0) {

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                DEBUGPRINTF("top new line\n") ;
#endif

	            badsub += 1 ;
	            rs = breadln(&dbxfile,linebuf,LINELEN) ;
	            if (rs <= 0) break ;
	            len = rs ;
	            if (linebuf[len - 1] == '\n') {
	                len -= 1 ;
		    }
	            linebuf[len] = eos ;

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                DEBUGPRINTF("1 line=>%W<\n",linebuf,len) ;
#endif

	        } /* end while */
	        opletter = *lp ;
	    } /* end if (needed another instruction) */

/* we have an instruction address? */

	    rs = cfhexui(ip,il,&ia) ;
	    if (rs < 0) continue ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 5)
	        DEBUGPRINTF("ia=%08x\n",ia) ;
#endif

/* initialize some stuff now that we have an instruction! :-) */

	    memclear(regs,BITBYTES) ;

/* write an instruction-address trace subrecord */

	    rs = exectrace_wia(&t,ia) ;

#if	CF_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        DEBUGPRINTF("exectrace_wia() rs=%d\n",rs) ;
#endif
	    if (rs < 0) break ;

/* get any sub-records */

	    rs = breadln(&dbxfile,linebuf,LINELEN) ;
	    if (rs <= 0) break ;
	    len = rs ;
	    if (linebuf[len - 1] == '\n') {
	        len -= 1 ;
	    }
	    linebuf[len] = eos ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 5)
	        DEBUGPRINTF("2 line=>%W<\n",linebuf,len) ;
#endif

	    while ((il = matinstr(linebuf,&ip,&lp)) == 0) {

#if	CF_DEBUG
	        if (pip->debuglevel >= 5)
	            DEBUGPRINTF("possible new subrecord\n") ;
#endif

/* what type of sub-record do we have ? */

	        if (((cp = strchr(linebuf,'=')) != nullptr) &&
	            (cp[-1] != CH_RPAREN)) {

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                DEBUGPRINTF("register reference\n") ;
#endif

/* process this register */

	            rs = getregval(pip,linebuf,-1,&ra,&rv) ;

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                DEBUGPRINTF("getregval() rs=%d\n",rs) ;
#endif

	            if (rs >= 0) {
	                if (! BATST(regs,ra)) {
	                    BASET(regs,ra) ;
#if	CF_DEBUG
	                    if (pip->debuglevel >= 5)
	                        DEBUGPRINTF("ra=%d rv=%08x\n",ra,rv) ;
#endif
	                    rs = exectrace_wrsv(&t,ra,rv) ;
	                }
	            } else {
	                badsub += 1 ;
		    }
#if	CF_DEBUG
	            if (pip->debuglevel > 1)
	                DEBUGPRINTF("exectrace_wreg() rs=%d\n",rs) ;
#endif

	        } else if (MATHX(linebuf)) {
	            uint	ma, mv = 0 ;

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                DEBUGPRINTF("memory reference\n") ;
#endif

/* process this memory reference */

	            sp = cp = linebuf + 2 ;
	            while (*cp && (! CHAR_ISWHITE(*cp))) {
	                cp += 1 ;
		    }

	            sl = (cp - sp) ;
	            if (sl > 8) {

#if	CF_DEBUG
	                if (pip->debuglevel >= 5)
	                    DEBUGPRINTF("corrupted memory reference len=%d\n",sl) ;
#endif

	                sp += (sl - 8) ;
	                sl = 8 ;
	            }

	            rs = cfhexui(sp,(cp - sp),&ma) ;

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                DEBUGPRINTF("cfhexui() rs=%d sl=%d >%W<\n",
	                    rs,sl,sp,(cp - sp)) ;
#endif

	            if (rs >= 0) {
#if	CF_DEBUG
	                if (pip->debuglevel >= 5)
	                    DEBUGPRINTF("ma=%08x \n",ma) ;
#endif

	                if (opletter == 'l') {
	                    exectrace_wmsa(&t,ma) ;
			} else {
	                    exectrace_wmem(&t,ma,mv,0) ;
			}
	            }
	        } /* end if (determining sub-record type) */

/* get the next line */

	        rs = breadln(&dbxfile,linebuf,LINELEN) ;
	        if (rs <= 0) break ;
	        len = rs ;
	        if (linebuf[len - 1] == '\n') {
	            len -= 1 ;
		}
	        linebuf[len] = eos ;

#if	CF_DEBUG
	        if (pip->debuglevel >= 5)
	            DEBUGPRINTF("3 line=>%W<\n",linebuf,len) ;
#endif

	    } /* end while (process non-instruction records) */
	    opletter = *lp ;
	    if (rs <= 0) break ;
	    in += 1 ;
	} /* end while (trying to read whole records) */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    DEBUGPRINTF("done in=%lld\n",in) ;
#endif

	if (pip->verboselevel > 0) {
	    bprintf(pip->ofp,
	        "instructions processed          %lld\n", in) ;
	    bprintf(pip->ofp,
	        "unknown sub records encountered %lld\n",badsub) ;
	}

bad2:
	exectrace_close(&t) ;

bad1:
	bclose(&dbxfile) ;

bad0:

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    DEBUGPRINTF("ret rs=%d\n",rs) ;
#endif

	return rs ;
} /* end subroutine (convert) */


/* local subroutines */

local int matinstr(char *linebuf,char **rpp,char **lpp) noex {
	int	len = 0 ;
	char	*sp ;

	if (! MATHX(linebuf))
	    return 0 ;

	*rpp = sp = linebuf + 2 ;
	while (*sp && (! CHARISWE(*sp))) {
	    sp += 1 ;
	}

	len = sp - *rpp ;
	while (CHARISWE(*sp)) {
	    sp += 1 ;
	}

	if (*sp == '\0')
	    return 0 ;

	if (! isalpha(*sp))
	    return 0 ;

	*lpp = sp ;
	while (*sp && (! CHARISWE(*sp))) {
	    sp += 1 ;
	}
	return len ;
} /* end subroutine (matinstr) */

local int getregval(PI *pip,char *linebuf,int len,uint *rap,uint *rvp) noex {
	int	rs, sl ;
	char	*sp, *cp ;

	if ((cp = strbrk(linebuf,"/=")) == nullptr)
	    return SR_INVALID ;

	sp = linebuf + 1 ;
	sl = cp - sp ;
	rs = cfdecui(sp,sl,rap) ;

	if (rs < 0)
	    return SR_INVALID ;

	if ((cp = strchr((sp + sl),'=')) == nullptr)
	    return SR_INVALID ;

	rs = cfnumui(cp + 1,-1,rvp) ;

	return rs ;
} /* end subroutine (getregval) */


