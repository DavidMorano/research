/* xmlinfo */

/* store and manage (partially) trace information for LevoSim */


#define	CF_DEBUGS	0


/* revision history :

	= 01/06/06, Dave Morano

	I am starting this out from scratch for the LevoSim simulator.


*/


/******************************************************************************

	This little (partial) object sets the trace options for use
	duing a LevoSim execution.


******************************************************************************/


#define	XMLINFO_MASTER	0


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstring>

#include	<usystem.h>
#include	<field.h>
#include	<mallocstuff.h>

#include	"localmisc.h"
#include	"xmlinfo.h"




/* local defines */

#define	XMLINFO_MAGIC		0x86553823
#define	LINELEN			80



/* external subroutines */

extern int	cfnumull(const char *,int,ULONG *) ;
extern int	sfkey(const char *,int,char **) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnchr(const char *,int,int) ;


/* local structures */


/* forward references */

static int	xmlinfo_procopts(XMLINFO *,char *,const char *) ;


/* local data */

/* trace options */

static char	*const traceopts[] = {
	"file",
	"clockindex",
	"index",
	"start",
	"n",
	"sync",
	NULL
} ;

#define	XMLINFO_OFILE		0
#define	XMLINFO_OCLOCKINDEX	1
#define	XMLINFO_OINDEX		2
#define	XMLINFO_OSTART		3
#define	XMLINFO_ON		4
#define	XMLINFO_OSYNC		5


/* white space, '#', and comma (',') */
static const unsigned char 	oterms[32] = {
	0x00, 0x0B, 0x00, 0x00,
	0x09, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
} ;






int xmlinfo_init(op,filename,traceopts)
XMLINFO		*op ;
char		filename[] ;
char const	traceopts[] ;
{
	int	rs = SR_OK ;
	int	sl ;

	char	fnbuf[MAXPATHLEN + 1] ;
	char	*cp ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	eprintf("xmlinfo_init: filename=%s\n",filename) ;
	eprintf("xmlinfo_init: traceopts=%s\n",traceopts) ;
#endif

	(void) memset(op,0,sizeof(XMLINFO)) ;

	if (filename == NULL)
	    return SR_FAULT ;

/* do we have any trace options ? */

	fnbuf[0] = '\0' ;
	if ((traceopts != NULL) && (traceopts[0] != '\0')) {

#if	CF_DEBUGS
	eprintf("xmlinfo_init: xmlinfo_procopts() filename=%s\n",
		filename) ;
#endif

	    rs = xmlinfo_procopts(op,fnbuf,traceopts) ;

#if	CF_DEBUGS
	eprintf("xmlinfo_init: xmlinfo_procopts() rs=%d\n",rs) ;
#endif

		if ((rs >= 0) && (fnbuf[0] != '\0'))
			filename = fnbuf ;

	    if (op->n > 0)
		op->end = op->start + op->n ;

	} /* end if (we had some trace options) */

	if ((filename == NULL) || (filename[0] == '\0'))
	    return SR_INVALID ;


#if	CF_DEBUGS
	eprintf("xmlinfo_init: about to open trace file\n") ;
#endif

	rs = bopen(&op->xmlfile,filename,"wct",0666) ;

	if (rs < 0)
		goto bad0 ;


/* what about a clock index file */

	if ((op->ifname != NULL) && (op->ifname[0] != '\0')) {

#if	CF_DEBUGS
	eprintf("xmlinfo_init: ifname=%s\n",op->ifname) ;
#endif

	rs = bopen(&op->ifile,op->ifname,"wct",0666) ;

	if (rs < 0)
		goto bad1 ;

	} /* end if (clock index file) */


	op->f.sync = TRUE ;
	if (op->start == 0)
	    op->f.enabled = TRUE ;


#if	CF_DEBUGS
	eprintf("xmlinfo_init: exiting rs=%d\n",rs) ;
#endif

	op->magic = XMLINFO_MAGIC ;
	return rs ;

/* bad things come here */
bad2:

bad1:
	bclose(&op->xmlfile) ;

bad0:

#if	CF_DEBUGS
	eprintf("xmlinfo_init: exiting BAD rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (xmlinfo_init) */


/* is tracing enabled (for real) ? */
int xmlinfo_flush(op)
XMLINFO	*op ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != XMLINFO_MAGIC)
	    return SR_NOTOPEN ;


	return rs ;
}
/* end subroutine (xmlinfo_flush) */


/* is tracing on ? */
int xmlinfo_on(op)
XMLINFO	*op ;
{


	if (op == NULL)
	    return SR_FAULT ;

	return (op->magic == XMLINFO_MAGIC) ;
}
/* end subroutine (xmlinfo_on) */


/* is tracing enabled (for real) ? */
int xmlinfo_enabled(op,ck)
XMLINFO	*op ;
ULONG	ck ;
{
	int	f_oldenable ;
	int	f_change ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != XMLINFO_MAGIC)
	    return SR_NOTOPEN ;

	f_oldenable = op->f.enabled ;
	if (ck < op->start)
		op->f.enabled = FALSE ;

	else if (op->n == 0)  
		op->f.enabled = TRUE ;

	else
		op->f.enabled = (ck < op->end) ;

#if	CF_DEBUGS
	    eprintf("xmlinfo_enabled: %d\n",op->f.enabled) ;
#endif

	f_change = ! LEQUIV(f_oldenable,op->f.enabled) ;

	if (f_change)
		op->f.sync = TRUE ;

	else
		op->f.sync = (op->sync) ? ((ck % op->sync) == 0) : FALSE ;

	if (op->f.sync)
		op->lastsync = ck ;

	return op->f.enabled ;
}
/* end subroutine (xmlinfo_enabled) */


/* free up */
int xmlinfo_free(op)
XMLINFO	*op ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != XMLINFO_MAGIC)
	    return SR_NOTOPEN ;

/* close all files */

	bclose(&op->xmlfile) ;

	if (op->ifname != NULL)
		bclose(&op->ifile) ;


/* free up */

	if (op->ifname != NULL)
		free(op->ifname) ;


	op->magic = 0 ;
	return rs ;
}
/* end subroutine (xmlinfo_free) */



/* PRIVATE SUBROUTINES */



static int xmlinfo_procopts(op,filename,opts)
XMLINFO		*op ;
char		filename[] ;
const char	opts[] ;
{
	FIELD	fsb ;

	ULONG	ulw ;

	int	rs = SR_OK, i ;
	int	sl, sl2 ;

	char	*sp ;
	char	*cp, *cp2 ;


#if	CF_DEBUGS
	    eprintf("xmlinfo_procopts: filename=%s\n",filename) ;
	    eprintf("xmlinfo_procopts: opts=>%s<\n",opts) ;
#endif

	rs = field_init(&fsb,opts,-1) ;

#if	CF_DEBUGS
	    eprintf("xmlinfo_procopts: field_init() rs=%d\n", rs) ;
#endif

	while (field_get(&fsb,oterms) >= 0) {

	    if (fsb.flen == 0)
	        continue ;

#if	CF_DEBUGS
	    eprintf("xmlinfo_procopts: option >%W<\n",
	        fsb.fp,fsb.flen) ;
#endif

	            if ((sl = sfkey(fsb.fp,fsb.flen,&sp)) < 0) {

				sp = fsb.fp ;
				sl = fsb.flen ;
			}

#if	CF_DEBUGS
	    eprintf("xmlinfo_procopts: option key >%W<\n",sp,sl) ;
#endif

	    i = optmatch3(traceopts,sp,sl) ;

	    if (i < 0)
	        continue ;

	    cp2 = strnchr(sp + sl,(fsb.flen - sl),'=') ;

	    sl2 = 0 ;
	    if (cp2 != NULL) {

	        cp2 += 1 ;
	        sl2 = fsb.flen - (cp2 - fsb.fp) ;

	    }

#if	CF_DEBUGS
	    eprintf("xmlinfo_procopts: option match i=%d\n", i) ;
	    eprintf("xmlinfo_procopts: cp2=%W\n",cp2,sl2) ;
#endif

	    switch (i) {

	    case XMLINFO_OSTART:
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
	                op->start = ulw ;

	        } /* end if */

	        break ;

	    case XMLINFO_ON:
	        if ((cp2 != NULL) &&
	            (cfnumull(cp2,sl2,&ulw) >= 0))
	            op->n = ulw ;

	        break ;

	    case XMLINFO_OSYNC:
	        if ((cp2 != NULL) &&
	            (cfnumull(cp2,sl2,&ulw) >= 0))
	            op->sync = ulw ;

	        break ;

	    case XMLINFO_OFILE:
	        if ((cp2 != NULL) && (filename[0] == '\0'))
	            strwcpy(filename,cp2,sl2) ;

	        break ;

	    case XMLINFO_OCLOCKINDEX:
	    case XMLINFO_OINDEX:
	        if ((cp2 != NULL) && (op->ifname == NULL))
	            op->ifname = mallocstrn(cp2,sl2) ;

	        break ;

	    } /* end switch */

	    if (fsb.term == '#')
	        break ;

	} /* end while */

	field_free(&fsb) ;


	return rs ;
}
/* end subroutine (xmlinfo_procopts) */



