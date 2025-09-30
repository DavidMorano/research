/* traceinfo */

/* store and manage (partially) trace information for LevoSim */


#define	CF_DEBUGS	0
#define	CF_SAFE		1		/* is really needed ! */
#define	CF_STARTIND	0		/* experiment ? */


/* revision history:

	= 01/06/06, Dave Morano

	I am starting this out from scratch for the LevoSim simulator.


*/

/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */

/******************************************************************************

	This little (partial) object sets the trace options for use
	duing a LevoSim execution.


******************************************************************************/


#define	TRACEINFO_MASTER	0


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstring>

#include	<usystem.h>
#include	<field.h>

#include	"localmisc.h"
#include	"exectrace.h"
#include	"traceinfo.h"




/* local defines */

#define	TRACEINFO_MAGIC		0x86553823
#define	LINELEN			80



/* external subroutines */

extern int	cfnumull(const char *,int,ULONG *) ;
extern int	sfkey(const char *,int,char **) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnchr(const char *,int,int) ;


/* local structures */


/* forward references */

static int	traceinfo_procopts(TRACEINFO *,char *,const char *) ;


/* local data */

/* trace options */

static char	*const traceopts[] = {
	"regs",
	"regvals",
	"mems",
	"memvals",
	"clocks",
	"start",
	"in",
	"n",
	"file",
	"rsa",
	"rsv",
	"msa",
	"msv",
	"mpv",
	"syscalls",
	NULL
} ;

enum traceopts {
	traceopt_regs,
	traceopt_regvals,
	traceopt_mems,
	traceopt_memvals,
	traceopt_clocks,
	traceopt_start,
	traceopt_in,
	traceopt_n,
	traceopt_file,
	traceopt_rsa,
	traceopt_rsv,
	traceopt_msa,
	traceopt_msv,
	traceopt_mpv,
	traceopt_syscalls,
	traceopt_overlast
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






int traceinfo_init(op,filename,traceopts,progname)
TRACEINFO	*op ;
char const	filename[] ;
char const	traceopts[] ;
char const	progname[] ;
{
	int	rs = SR_OK ;
	int	sl ;

	char	fnbuf[MAXPATHLEN + 1] ;
	char	*cp ;


	if (op == NULL)
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(TRACEINFO)) ;

	if (filename == NULL)
	    return SR_FAULT ;

	if (filename[0] == '\0')
	    return SR_INVALID ;

/* do we have any trace options ? */

	fnbuf[0] = '\0' ;
	if ((traceopts != NULL) && (traceopts[0] != '\0')) {

	    if ((filename != NULL) && (filename[0] != '\0'))
	        fnbuf[0] = 1 ;

#if	CF_DEBUGS
	eprintf("traceinfo_init: about to traceinfo_procopts()\n") ;
#endif

	    rs = traceinfo_procopts(op,fnbuf,traceopts) ;

#if	CF_DEBUGS
	eprintf("traceinfo_init: traceinfo_procopts() rs=%d\n",rs) ;
#endif

	    if ((rs >= 0) && (fnbuf[0] > 1))
	        filename = fnbuf ;

	    if (op->n > 0)
		op->ein = op->in + op->n ;

	} /* end if (we had some trace options) */

	if ((filename == NULL) || (filename[0] == '\0'))
	    return SR_INVALID ;


#if	CF_DEBUGS
	eprintf("traceinfo_init: about to open trace file\n") ;
#endif

	rs = exectrace_open(&op->et,filename,"w",0666,progname) ;

#if	CF_DEBUGS
	eprintf("traceinfo_init: exectrace_open() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad0 ;


	if (op->in == 0)
	    op->f.enabled = TRUE ;

#if	CF_DEBUGS
	eprintf("traceinfo_init: regs=%d\n",op->f.regs) ;
	eprintf("traceinfo_init: regvals=%d\n",op->f.regvals) ;
	eprintf("traceinfo_init: rsa=%d\n",op->f.rsa) ;
	eprintf("traceinfo_init: rsv=%d\n",op->f.rsv) ;
	eprintf("traceinfo_init: mems=%d\n",op->f.mems) ;
	eprintf("traceinfo_init: memvals=%d\n",op->f.memvals) ;
	eprintf("traceinfo_init: msa=%d\n",op->f.msa) ;
	eprintf("traceinfo_init: msv=%d\n",op->f.msv) ;
	eprintf("traceinfo_init: mpv=%d\n",op->f.mpv) ;
	eprintf("traceinfo_init: clocks=%d\n",op->f.clocks) ;
	eprintf("traceinfo_init: syscalls=%d\n",op->f.syscalls) ;
#endif /* CF_DEBUG */

	op->magic = TRACEINFO_MAGIC ;
	return rs ;

/* bad things come here */
bad1:

bad0:
	return rs ;
}
/* end subroutine (traceinfo_init) */


/* flush any buffered data */
int traceinfo_flush(op)
TRACEINFO	*op ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != TRACEINFO_MAGIC)
	    return SR_NOTOPEN ;

	rs = exectrace_flush(&op->et) ;

	return rs ;
}
/* end subroutine (traceinfo_flush) */


/* is tracing enabled (for real) ? */
int traceinfo_enabled(op,in)
TRACEINFO	*op ;
ULONG		in ;
{
	int	r ;
	int	f_old ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != TRACEINFO_MAGIC)
	    return SR_NOTOPEN ;
#endif

#if	CF_STARTIND
	f_old = op->f.enabled ;
#endif

	op->in = in ;
	if (in < op->sin)
		op->f.enabled = FALSE ;

	else if (op->n == 0)  
		op->f.enabled = TRUE ;

	else
		op->f.enabled = (in < op->ein) ;

#if	CF_DEBUGS
	    eprintf("traceinfo_enabled: %d\n",op->f.enabled) ;
#endif

#if	CF_STARTIND
	r = (op->f.enabled) ? ((f_old) ? 2 : 1) : 0 ;
#else
	r = op->f.enabled ;
#endif

	return r ;
}
/* end subroutine (traceinfo_enabled) */


/* free up */
int traceinfo_free(op)
TRACEINFO	*op ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != TRACEINFO_MAGIC)
	    return SR_NOTOPEN ;

	rs = exectrace_close(&op->et) ;

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (traceinfo_free) */



/* PRIVATE SUBROUTINES */



static int traceinfo_procopts(op,filename,opts)
TRACEINFO	*op ;
char		filename[] ;
const char	opts[] ;
{
	FIELD	fsb ;

	ULONG	ulw ;

	int	rs = SR_OK, i, n ;
	int	sl, sl2 ;

	char	*cp, *cp2 ;


	n = 0 ;
	field_init(&fsb,opts,-1) ;

	while (field_get(&fsb,oterms) >= 0) {

	    if (fsb.flen == 0)
	        continue ;

#if	CF_DEBUGS
	    eprintf("traceinfo_procopts: option >%W<\n",
	        fsb.fp,fsb.flen) ;
#endif

		cp = fsb.fp ;
		sl = fsb.flen ;
	    sl2 = 0 ;
	    if ((cp2 = strnchr(cp,sl,'=')) != NULL) {

		sl = cp2 - cp ;
	        cp2 += 1 ;
	        sl2 = fsb.flen - (cp2 - fsb.fp) ;

	    }

#if	CF_DEBUGS
	    eprintf("traceinfo_procopts: option key >%W<\n",cp,sl) ;
#endif

	    i = optmatch3(traceopts,cp,sl) ;

	    if (i < 0)
	        continue ;

#if	CF_DEBUGS
	    eprintf("traceinfo_procopts: option match i=%d\n", i) ;
	    eprintf("traceinfo_procopts: cp2=%W\n",cp2,sl2) ;
#endif

	    switch (i) {

	    case traceopt_start:
	    case traceopt_in:
	        if (cp2 != NULL) {

	            int		sl3 ;

		    char	*cp3 ;


	            if ((cp3 = strnchr(cp2,sl2,':')) != NULL) {

	                sl3 = (cp2 + sl2) - (cp3 + 1) ;

	                if ((sl3 > 0) &&
	                    (cfnumull((cp3 + 1),sl3,&ulw) >= 0))
	                    op->n = ulw ;

	                sl2 = cp3 - cp2 ;

	            } /* end if (had another value) */

	            if (cfnumull(cp2,sl2,&ulw) >= 0)
	                op->sin = ulw ;

#if	CF_DEBUGS
	    eprintf("traceinfo_procopts: sin=%lld n=%lld\n",
		op->sin,op->n) ;
#endif

	        } /* end if */

	        break ;

	    case traceopt_n:
	        if ((cp2 != NULL) &&
	            (cfnumull(cp2,sl2,&ulw) >= 0))
	            op->n = ulw ;

	        break ;

	    case traceopt_regs:
	        op->f.regs = TRUE ;
	        break ;

	    case traceopt_regvals:
	        op->f.regs = TRUE ;
	        op->f.regvals = TRUE ;
	        break ;

	    case traceopt_rsa:
	        op->f.rsa = TRUE ;
	        break ;

	    case traceopt_rsv:
	        op->f.rsv = TRUE ;
	        break ;

	    case traceopt_mems:
	        op->f.mems = TRUE ;
	        break ;

	    case traceopt_memvals:
	        op->f.mems = TRUE ;
	        op->f.memvals = TRUE ;
	        break ;

	    case traceopt_msa:
	        op->f.msa = TRUE ;
	        break ;

	    case traceopt_msv:
	        op->f.msv = TRUE ;
	        break ;

	    case traceopt_mpv:
	        op->f.mpv = TRUE ;
	        break ;

	    case traceopt_syscalls:
	        op->f.syscalls = TRUE ;
	        break ;

	    case traceopt_file:
	        if ((cp2 != NULL) && (filename[0] == '\0'))
	            strwcpy(filename,cp2,sl2) ;

	        break ;

	    } /* end switch */

		n += 1 ;
	    if (fsb.term == '#')
	        break ;

	} /* end while */

	field_free(&fsb) ;

#if	CF_DEBUGS
	    eprintf("traceinfo_procopts: exiting rs=%d\n",rs) ;
#endif

	return (rs >= 0) ? n : rs ;
}
/* end subroutine (traceinfo_procopts) */



