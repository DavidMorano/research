/* convert */

/* convert a trace from the Alireza-Pixie output format */


#define	CF_DEBUG	0


/* revision history:

	= 01/09/01, David Morano

	This subroutine was originally written.



*/


/******************************************************************************

	This subroutine converts the output format from Alireza's
	Pixie-reader program into an ET-type trace format.



******************************************************************************/


#include	<sys/types.h>
#include	<string.h>
#include	<ctype.h>

#include	<vsystem.h>
#include	<baops.h>
#include	<char.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"exectrace.h"



/* local defines */

#define	MAT_IA(b)	(((b)[0] == '0') && ((b)[1] == 'x'))
#define	BITBYTES	((EXECTRACE_NREG / 8) + 1)
#define	RL_SIZE		(LINELEN + 2)



/* external subroutines */

extern int	cfdeci(const char *,int,int *) ;
extern int	cfhexi(const char *,int,int *) ;
extern int	cfnumui(const char *,int,uint *) ;


/* external variables */


/* local structures */


/* forward references */


/* local (module-scope static) data */







int convert(pip,t1fname,t2fname,n)
struct proginfo	*pip ;
char		t1fname[] ;
char		t2fname[] ;
uint		n ;
{
	bfile		pixfile ;

	EXECTRACE		t ;

	ULONG	in = 0 ;
	ULONG	nmax = n ;
	ULONG	sourcebytes = 0 ;
	ULONG	ulw ;

#ifdef	COMMENT
	ULONG	badsub = 0 ;
#endif

	uint	ia, ma ;

	int	rs, i, j, iw ;
	int	rs1, rs2 ;
	int	rls[RL_SIZE + 1] ;
	int	size, len ;
	int	linereads ;
	int	type ;			/* type of instruction */

	char	linebuf[LINELEN + 1] ;
	char	regs[BITBYTES] ;
	char	*sp, *cp ;


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: n=%u\n",n) ;
#endif

	linereads = 0 ;
	size = RL_SIZE * sizeof(int) ;
	(void) memset(rls,0,size) ;


/* open the traces */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: t1=%s t2=%s\n",
	        t1fname,t2fname) ;
#endif

	rs1 = bopen(&pixfile,t1fname,"r",0666) ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: bopen() rs=%d\n",rs1) ;
#endif

	rs2 = exectrace_open(&t,t2fname,"w",0666,NULL) ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: exectrace_open() rs=%d\n",rs2) ;
#endif

	if (rs1 < 0) {

	    bprintf(pip->efp,"%s: could not open the Pixie file (%d)\n",
	        pip->progname,rs1) ;

	    exectrace_close(&t) ;

	    rs = rs1 ;
	    goto ret0 ;
	}

	if (rs2 < 0) {

	    bprintf(pip->efp,"%s: could not open the ET file (%d)\n",
	        pip->progname,rs2) ;

	    rs = rs2 ;
	    goto ret1 ;
	}


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: starting\n") ;
#endif

/* loop reading them */

	linebuf[0] = '0' ;
	while ((rs = breadline(&pixfile,linebuf,LINELEN)) > 0) {

	    len = rs ;

	    linereads += 1 ;
	    rls[len] += 1 ;

	    ulw = len ;
	    sourcebytes += ulw ;

	    if (linebuf[len - 1] == '\n')
	        len -= 1 ;

	    linebuf[len] = '\0' ;

/* we have an instruction address ? */

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("convert: line> %s\n",linebuf) ;
#endif

/* get the instruction address */

	    cp = sp = linebuf + 2 ;
	    while (*cp && (! CHAR_ISWHITE(*cp)))
	        cp += 1 ;

	    rs = cfhexi(sp,(cp - sp),(int *) &ia) ;

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("convert: cfhexi() rs=%d ia=%08x\n",rs,ia) ;
#endif

	    if (rs < 0)
	        continue ;

/* write an instruction-address trace subrecord */

	    rs = exectrace_wia(&t,ia) ;

#if	CF_DEBUG
	    if (pip->debuglevel > 1)
	        debugprintf("convert: exectrace_wia() rs=%d\n",rs) ;
#endif

/* is there a memory reference on this instruction ? */

	    sp = cp ;
	    while (CHAR_ISWHITE(*sp))
	        sp += 1 ;

	    type = *sp ;

	    if ((cp = strchr(sp,'<')) != NULL) {

	        if (tolower(type) == 's') {

	            sp = cp + 1 ;
	            if (((cp = strchr(sp,'>')) != NULL) &&
	                (cfnumui(sp,(cp - sp),(uint *) &ma) >= 0)) {

	                rs = exectrace_wmem(&t,ma,0,0) ;

	                if (rs < 0)
	                    break ;

	            } /* end if (got the address) */

	        } else if (tolower(type) == 'l') {

	            sp = cp + 1 ;
	            if (((cp = strchr(sp,'>')) != NULL) &&
	                (cfnumui(sp,(cp - sp),(uint *) &ma) >= 0)) {

	                rs = exectrace_wmsa(&t,ma) ;

	                if (rs < 0)
	                    break ;

	            } /* end if (got the address) */

	        } /* end if (load/store we know about) */

	    } /* end if (memory reference) */

	    in += 1 ;
	    if ((n > 0) && (in >= n))
	        break ;

	} /* end while (trying to read whole records) */


#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: done in=%lld\n",in) ;
#endif

#ifdef	COMMENT
	if (pip->verboselevel > 0) {

	    bprintf(pip->ofp,
	        "unknown sub records encountered %lld\n",badsub) ;

	}
#endif /* COMMENT */

	bprintf(pip->ofp,
	    "instructions processed          %lld\n", in) ;

	if (pip->verboselevel > 0) {

	    bprintf(pip->ofp,
	        "source file bytes read          %lld\n", sourcebytes) ;

	}

	if (pip->verboselevel > 1) {

	    bprintf(pip->ofp,"lines read %d\n",linereads) ;

	    bprintf(pip->ofp,"read lengths\n") ;

	    for (i = 0 ; i < RL_SIZE ; i += 1)
	        bprintf(pip->ofp,"%4d %10d\n",i,rls[i]) ;

	} /* end if */

	if (pip->debuglevel > 0) {

	    bprintf(pip->efp,
	        "%s: instructions processed          %lld\n", 
	        pip->progname,in) ;

	    bprintf(pip->efp,
	        "%s: source file bytes read          %lld\n", 
	        pip->progname,sourcebytes) ;

	}


ret2:
	rs1 = exectrace_close(&t) ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: exectrace_close() rs=%d\n",rs1) ;
#endif

ret1:
	bclose(&pixfile) ;

ret0:

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	    debugprintf("convert: returning rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (convert) */



/* LOCAL SUBROUTINES */



