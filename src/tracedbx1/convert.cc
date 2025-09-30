/* convert SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* convert a trace */
/* version %I% last-modified %G% */

#define	CF_DEBUG	0

/* revision history:

	= 1001-09-01, David Morano
	This subroutine was originally written.

*/

/* Copyright © 2001 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

#include	<envstandards.h>	/* MUST be first to configure */
#include	<sys/types.h>
#include	<cstring>
#include	<ctype.h>

#include	<usystem.h>
#include	<baops.h>
#include	<ascii.h>
#include	<strx.h>
#include	<char.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"
#include	"exectrace.h"


/* local defines */

#define	MATHX(b)	(((b)[0] == '0') && ((b)[1] == 'x'))
#define	CHARISWE(c)	(((c) == ' ') || ((c) == '\t') || ((c) == '\n'))
#define	BITBYTES	((EXECTRACE_NREG / 8) + 1)


/* external subroutines */

extern int	cfdeci(cchar *,int,int *) ;
extern int	cfhexui(cchar *,int,uint *) ;
extern int	cfhexi(cchar *,int,int *) ;
extern int	cfnumui(cchar *,int,uint *) ;


/* external variables */


/* local structures */


/* forward references */

static int	matinstr(char *,char **,char **) ;
static int	getregval(struct proginfo *,char *,int,uint *,uint *) ;


/* local (module-scope static) data */






int convert(pip,t1fname,t2fname)
struct proginfo	*pip ;
char		t1fname[] ;
char		t2fname[] ;
{
	bfile		dbxfile ;

	EXECTRACE		t ;

	struct traceinfo	ti ;

	ULONG	in = 0 ;

	LONG	badsub = -1 ;

	int	rs, i, j ;
	int	len ;
	int	sl, il ;
	int	opletter ;

	char	linebuf[LINELEN + 1] ;
	char	regs[BITBYTES] ;
	char	*sp, *cp ;
	char	*ip, *lp ;


	(void) memset(&ti,0,sizeof(struct traceinfo)) ;

/* open the traces */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: t1=%s t2=%s\n",
	        t1fname,t2fname) ;
#endif


	rs = bopen(&dbxfile,t1fname,"r",0666) ;

	if (rs < 0)
	    return rs ;


	rs = exectrace_open(&t,t2fname,"w",0666,NULL) ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: exectrace_open() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad1 ;


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: starting\n") ;
#endif

/* loop reading them */

	linebuf[0] = '0' ;
	il = 0 ;
	while (TRUE) {

	    uint	ia, ra, rv ;


	    if (il == 0) {

#if	CF_DEBUG
	        if (pip->debuglevel >= 5)
	            debugprintf("convert: need top new line\n") ;
#endif

	        while ((il = matinstr(linebuf,&ip,&lp)) == 0) {

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                debugprintf("convert: top new line\n") ;
#endif

	            badsub += 1 ;

	            rs = breadline(&dbxfile,linebuf,LINELEN) ;

	            if (rs <= 0)
	                break ;

	            len = rs ;
	            if (linebuf[len - 1] == '\n')
	                len -= 1 ;

	            linebuf[len] = '\0' ;

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                debugprintf("convert: 1 line=>%W<\n",linebuf,len) ;
#endif

	        } /* end while */

	        opletter = *lp ;

	    } /* end if (needed another instruction) */

/* we have an instruction address ? */

	    rs = cfhexui(ip,il,&ia) ;

	    if (rs < 0)
	        continue ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 5)
	        debugprintf("convert: ia=%08x\n",ia) ;
#endif

/* initialize some stuff now that we have an instruction ! :-) */

	    (void) memset(regs,0,BITBYTES) ;

/* write an instruction-address trace subrecord */

	    rs = exectrace_wia(&t,ia) ;

#if	CF_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        debugprintf("convert: exectrace_wia() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

/* get any sub-records */

	    rs = breadline(&dbxfile,linebuf,LINELEN) ;

	    if (rs <= 0)
	        break ;

	    len = rs ;
	    if (linebuf[len - 1] == '\n')
	        len -= 1 ;

	    linebuf[len] = '\0' ;

#if	CF_DEBUG
	    if (pip->debuglevel >= 5)
	        debugprintf("convert: 2 line=>%W<\n",linebuf,len) ;
#endif

	    while ((il = matinstr(linebuf,&ip,&lp)) == 0) {

#if	CF_DEBUG
	        if (pip->debuglevel >= 5)
	            debugprintf("convert: possible new subrecord\n") ;
#endif

/* what type of sub-record do we have ? */

	        if (((cp = strchr(linebuf,'=')) != NULL) &&
	            (cp[-1] != CH_RPAREN)) {

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                debugprintf("convert: register reference\n") ;
#endif

/* process this register */

	            rs = getregval(pip,linebuf,-1,&ra,&rv) ;

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                debugprintf("convert: getregval() rs=%d\n",rs) ;
#endif

	            if (rs >= 0) {

	                if (! BATST(regs,ra)) {

	                    BASET(regs,ra) ;

#if	CF_DEBUG
	                    if (pip->debuglevel >= 5)
	                        debugprintf("convert: ra=%d rv=%08x\n",ra,rv) ;
#endif

	                    rs = exectrace_wrsv(&t,ra,rv) ;

	                }

	            } else
	                badsub += 1 ;

#if	CF_DEBUG
	            if (pip->debuglevel > 1)
	                debugprintf("convert: exectrace_wreg() rs=%d\n",rs) ;
#endif

	        } else if (MATHX(linebuf)) {

	            uint	ma, mv = 0 ;


#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                debugprintf("convert: memory reference\n") ;
#endif

/* process this memory reference */

	            sp = cp = linebuf + 2 ;
	            while (*cp && (! CHAR_ISWHITE(*cp)))
	                cp += 1 ;

	            sl = (cp - sp) ;
	            if (sl > 8) {

#if	CF_DEBUG
	                if (pip->debuglevel >= 5)
	                    debugprintf("convert: corrupted memory reference len=%d\n",sl) ;
#endif

	                sp += (sl - 8) ;
	                sl = 8 ;
	            }

	            rs = cfhexui(sp,(cp - sp),&ma) ;

#if	CF_DEBUG
	            if (pip->debuglevel >= 5)
	                debugprintf("convert: cfhexui() rs=%d sl=%d >%W<\n",
	                    rs,sl,sp,(cp - sp)) ;
#endif

	            if (rs >= 0) {

#if	CF_DEBUG
	                if (pip->debuglevel >= 5)
	                    debugprintf("convert: ma=%08x \n",ma) ;
#endif

	                if (opletter == 'l')
	                    exectrace_wmsa(&t,ma) ;

	                else
	                    exectrace_wmem(&t,ma,mv,0) ;

	            }

	        } /* end if (determining sub-record type) */

/* get the next line */

	        rs = breadline(&dbxfile,linebuf,LINELEN) ;

	        if (rs <= 0)
	            break ;

	        len = rs ;
	        if (linebuf[len - 1] == '\n')
	            len -= 1 ;

	        linebuf[len] = '\0' ;

#if	CF_DEBUG
	        if (pip->debuglevel >= 5)
	            debugprintf("convert: 3 line=>%W<\n",linebuf,len) ;
#endif

	    } /* end while (process non-instruction records) */

	    opletter = *lp ;

	    if (rs <= 0)
	        break ;

	    in += 1 ;

	} /* end while (trying to read whole records) */


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: done in=%lld\n",in) ;
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
	    debugprintf("convert: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (convert) */



/* LOCAL SUBROUTINES */



static int matinstr(linebuf,rpp,lpp)
char	linebuf[] ;
char	**rpp ;
char	**lpp ;
{
	int	len ;

	char	*sp ;


	if (! MATHX(linebuf))
	    return 0 ;

	*rpp = sp = linebuf + 2 ;
	while (*sp && (! CHARISWE(*sp)))
	    sp += 1 ;

	len = sp - *rpp ;
	while (CHARISWE(*sp))
	    sp += 1 ;

	if (*sp == '\0')
	    return 0 ;

	if (! isalpha(*sp))
	    return 0 ;

	*lpp = sp ;
	while (*sp && (! CHARISWE(*sp)))
	    sp += 1 ;

	return len ;
}
/* end subroutine (matinstr) */


static int getregval(pip,linebuf,len,rap,rvp)
struct proginfo	*pip ;
char		linebuf[] ;
int		len ;
uint		*rap, *rvp ;
{
	int	rs, sl ;

	char	*sp, *cp ;


	if ((cp = strbrk(linebuf,"/=")) == NULL)
	    return SR_INVALID ;

	sp = linebuf + 1 ;
	sl = cp - sp ;
	rs = cfdecui(sp,sl,rap) ;

	if (rs < 0)
	    return SR_INVALID ;

	if ((cp = strchr((sp + sl),'=')) == NULL)
	    return SR_INVALID ;

	rs = cfnumui(cp + 1,-1,rvp) ;

	return rs ;
}
/* end subroutine (getregval) */



