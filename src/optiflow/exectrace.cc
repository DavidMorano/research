/* exectrace SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* create and read an execution trace */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_DEBUGSRS	0
#define	CF_SAFE		0		/* might need this */
#define	CF_FASTCLEAR	1		/* caution: hack could be dangerous */
#define	CF_FASTREAD	1
#define	CF_NOCOMPRESS	0		/* do not compress IAs */
#define	CF_SETBUF	0		/* set larger buffer size */

/* revision history:

	= 2001-06-06, David Morano
	I am starting this out from scratch for the LevoSim simulator.

*/

/* Copyright © 1998 David A-D- Morano.  All rights reserved. */

/*******************************************************************************

	This little object allows for the writing and reading of a
	committed execution trace of a program.  Provision is made
	for the writing of instructions executed as well as register
	written and memory locations written.

*******************************************************************************/

#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctime>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<usystem.h>
#include	<getxusername.h>
#include	<getnodedomain.h>
#include	<bfile.h>
#include	<netorder.h>
#include	<sfx.h>
#include	<cfdec.h>
#include	<strwcpy.h>
#include	<localmisc.h>

#include	"exectrace.h"


/* local defines */

#define	GETDP(type)	(((type) >> EXECTRACE_TBITS) & 15)

#define	ET		EXECTRACE{

/* modes */

#define	EXECTRACE_MREAD		0
#define	EXECTRACE_MWRITE	1
#define	EXECTRACE_MAPPEND	2

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif

#define	BUFSIZE			(8 * 8192)

#define	EXECTRACEDEB		"exectrace.deb"


/* external subroutines */


/* local structures */


/* forward references */

int 		exectrace_read(EXECTRACE *,EXECTRACE_ENTRY *) ;

static int	exectrace_readsub(EXECTRACE *,EXECTRACE_ENTRY *,uint) ;

#if	CF_DEBUGS
extern int	mkhexstr(char *,int,void *,int) ;
#endif
#if	CF_DEBUGS
static int	half(LONG,int) ;
#endif

#ifdef	COMMENT
static void	mkmodestr(char *,int) ;
#endif


/* local data */

#if	CF_DEBUGS

static const char	*typenames[] = {
	"name",
	"date",
	"where",
	"clock",
	"ia",
	"reg",
	"mem",
	"regdump",
	"in",
	"som",
	"rsa",
	"msa",
	"rsv",
	"msv",
	"syscall",
	NULL
} ;

#endif /* CF_DEBUGS */


/* exported variables */


/* exported subroutines */

int exectrace_open(ET *op,cc *filename,cc *accmode,int perm,cc *progname) noex {
	EXECTRACE_ENTRY	e ;

	LONG	starttime ;

	int	rs, rs1 ;
	int	i, sl ;
	int	rtype ;
	int	f_read = FALSE ;
	int	f_write = FALSE ;
	int	f_append = FALSE ;
	int	f_seekable = FALSE ;

	char	nodename[NODENAMELEN + 1] ;
	char	username[USERNAMELEN + 1] ;
	char	linebuf[LINEBUFLEN + 1] ;
	char	*openmode = linebuf ;
	char	*cp ;


	if (op == NULL)
	    return SR_FAULT ;

	if ((filename == NULL) || (accmode == NULL))
	    return SR_FAULT ;

	(void) memset(op,0,sizeof(EXECTRACE)) ;

/* sanity check */

#if	CF_DEBUGS
	for (i = 0 ; typenames[i] != NULL ; i += 1) ;
	if (i != EXECTRACE_ROVERLAST)
	    return SR_BADFMT ;
#endif /* CF_DEBUGS */

/* continue */

	op->magic = 0 ;

/* figure out what mode we are working on */

	cp = openmode ;
	while (accmode[0] != '\0') {

	    switch (accmode[0]) {

	    case 'r':
	        f_read = TRUE ;
	        *cp++ = 'r' ;
	        break ;

	    case 'a':
	        f_append = TRUE ;
	        *cp++ = 'r' ;

/* FALL THROUGH */

	    case 'w':
	        f_write = TRUE ;
	        *cp++ = 'w' ;
	        break ;

	    } /* end switch */

	    accmode += 1 ;

	} /* end while */

	if (f_read && f_write)
	    return SR_INVALID ;

	if (f_write && (! f_append)) {
	    *cp++ = 'c' ;
	    *cp++ = 't' ;
	}

	*cp = '\0' ;

/* store it away for reference */

	op->f.read = (! f_write) ;
	op->f.append = f_append ;

/* do the actual file open */

#if	CF_DEBUGS
	debugprintf("exectrace_open: filename=%s omode=%s progname=%s\n",
	    filename,openmode,progname) ;
#endif

	rs = bopen(&op->tfile,(char *) filename,openmode,perm) ;

#if	CF_DEBUGS
	debugprintf("exectrace_open: bopen() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    goto bad0 ;

	rs = bcontrol(&op->tfile,BC_CLOSEONEXEC,TRUE) ;

	if (rs < 0)
	    goto bad1 ;

/* if it is seekable, try to get the buffer size --->  UP! */

#if	CF_SETBUF
	if (bseek(&op->tfile,0L,SEEK_CUR) >= 0) {

	    f_seekable = TRUE ;
	    bcontrol(&op->tfile,BC_SETBUF,BUFSIZE) ;

	}
#endif /* F_STEBUF */

/* do some things depending on how we are accessing */

	if (f_read || f_append) {

	    int	len ;


#if	CF_DEBUGS
	    debugprintf("exectrace_open: read/append\n") ;
#endif

	    rs = breadline(&op->tfile,linebuf,LINEBUFLEN) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_open: breadline() rs=%d\n",rs) ;
#endif

	    if (rs <= 0) {
	        rs = SR_BADFMT ;
	        goto bad1 ;
	    }

	    len = rs ;
	    linebuf[len] = '\0' ;
	    cp = EXECTRACE_FILEMAGIC ;
	    sl = strlen(cp) ;

	    rs = SR_BADFMT ;
	    if (strncmp(cp,linebuf,sl) != 0) {

#if	CF_DEBUGS
	        debugprintf("exectrace_open: bad (%d) magic=>%s<\n",
			sl,linebuf) ;
#endif

	        goto bad1 ;
	    }

	    sl = sfshrink((linebuf + sl),(len - sl),&cp) ;

	    op->i.version = 0 ;
	    if (sl > 0) {

	        rs = cfdeci((linebuf + sl),(len - sl),&op->i.version) ;

	        if (rs < 0) {

#if	CF_DEBUGS
	            debugprintf("exectrace_open: bad version parse\n") ;
#endif

	            rs = SR_BADFMT ;
	            goto bad1 ;
	        }

#if	CF_DEBUGS
	        debugprintf("exectrace_open: read file version=%d\n",
	            op->i.version) ;
#endif

	        if (op->i.version > EXECTRACE_FILEVERSION) {

#if	CF_DEBUGS
	            debugprintf("exectrace_open: bad version=%d\n",
	                op->i.version) ;
#endif

	            rs = SR_NOTSUP ;
	            goto bad1 ;
	        }

	    } /* end if (optional version was present) */

/* make up some nice test arrays for speed */

	    (void) memset(&op->istypeinfo,0,EXECTRACE_ROVERLAST) ;

	    for (i = 0 ; i < EXECTRACE_RCLOCK ; i += 1)
	        op->istypeinfo[i] = TRUE ;

	    (void) memset(&op->istypesom,0,EXECTRACE_ROVERLAST) ;

	    op->istypesom[EXECTRACE_RIA] = TRUE ;
	    op->istypesom[EXECTRACE_RSYSCALL] = TRUE ;
	    op->istypesom[EXECTRACE_RSOM] = TRUE ;

	    (void) memset(&op->istypeextra,0,EXECTRACE_ROVERLAST) ;

	    op->istypeextra[EXECTRACE_RREG] = TRUE ;
	    op->istypeextra[EXECTRACE_RMEM] = TRUE ;
	    op->istypeextra[EXECTRACE_RIN] = TRUE ;

/* start the process off */

	    rs = bgetc(&op->tfile) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_open: bgetc() rs=%d c=%02x\n",rs,rs) ;
#endif

	    op->r.type = -1 ;
	    if (rs >= 0) {

	        op->r.type = rs ;
	        rtype = op->r.type & EXECTRACE_TMASK ;
	        if (op->istypeinfo[rtype]) {

	            op->magic = EXECTRACE_MAGIC ;
	            rs = exectrace_read(op,&e) ;

#if	CF_DEBUGS
	            debugprintf("exectrace_open: exectrace_read() rs=%d\n",rs) ;
#endif

	        }

	    } else if (rs == SR_EOF) {

#if	CF_DEBUGS
	        debugprintf("exectrace_open: EOF rs=%d\n",rs) ;
#endif

	        rs = SR_OK ;
	        op->f.eof = TRUE ;

	    }

	} /* end if (read or append) */

	if (f_write && (! f_append)) {

/* write the file magic and version */

	    bprintf(&op->tfile,"%s %d\n",
	        EXECTRACE_FILEMAGIC, EXECTRACE_FILEVERSION) ;

/* write a NAME record */

	    if ((progname != NULL) && (progname[0] != '\0')) {

	        bputc(&op->tfile,EXECTRACE_RNAME) ;

#if	CF_DEBUGS
	        debugprintf("exectrace_open: writing progname=%s\n",progname) ;
#endif

	        bprintf(&op->tfile,"%s\n",
	            progname) ;

	    } /* end if (writing a NAME record) */

/* write a WHERE record */

	    rs1 = getnodedomain(nodename,NULL) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_open: getnodedomain() rs=%d\n",rs1) ;
#endif

	    if (rs1 >= 0) {

	        getusername(username,USERNAMELEN,-1) ;

	        bputc(&op->tfile,EXECTRACE_RWHERE) ;

	        bprintf(&op->tfile,"%s!%s\n",
	            nodename,username) ;

	    } /* end if (writing a WHERE record) */

/* write a DATE record */

	    starttime = unixtime(NULL) ;

	    bputc(&op->tfile,EXECTRACE_RDATE) ;

	    netorder_wll(linebuf,starttime) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_open: date=%016llx %08x:%08x\n",
	        starttime,half(starttime,1),half(starttime,0)) ;
#endif

	    rs = bwrite(&op->tfile,linebuf,sizeof(LONG)) ;

	} /* end if (writing but not appending) */

	if (f_append && f_seekable)
	    bseek(&op->tfile,0L,SEEK_END) ;

	if (rs < 0)
	    goto bad1 ;

#if	CF_DEBUGS
	debugprintf("exectrace_open: exiting OK rs=%d\n",rs) ;
#endif

	op->magic = EXECTRACE_MAGIC ;
	return rs ;

/* bad things come here */
bad1:
	bclose(&op->tfile) ;

bad0:

#if	CF_DEBUGS
	debugprintf("exectrace_open: exiting bad rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (exectrace_open) */


/* free up */
int exectrace_close(op)
EXECTRACE	*op ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;

	rs = bclose(&op->tfile) ;

#if	CF_DEBUGS
	debugprintf("exectrace_close: rs=%d\n",rs) ;
#endif

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (exectrace_close) */


/* write out a clock record */
int exectrace_wclock(op,clock)
EXECTRACE	*op ;
ULONG		clock ;
{
	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RCLOCK ;
	netorder_wull(bp,clock) ;

	bp += 8 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	debugprintf("exectrace_wclock: rs=%d\n",rs) ;
#endif

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wclock) */


/* write out an instruction address record */
int exectrace_wia(op,ia)
EXECTRACE	*op ;
uint		ia ;
{
	uint	inc, type ;

	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGS
	debugprintf("exectrace_wia: saved rs=%d\n",op->rs) ;
#endif

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wia: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	type = EXECTRACE_RIA ;
#if	CF_NOCOMPRESS
	inc = -1 ;
#else
	inc = ia - op->ia_last ;
#endif
	if ((inc <= 0) || (inc >= 16)) {

#if	CF_DEBUGS
	    debugprintf("exectrace_wia: whole ia=%08x\n",ia) ;
#endif

	    *bp++ = type ;
	    netorder_wuint(bp,ia) ;

	    bp += 4 ;

	} else
	    *bp++ = type | (inc << 4) ;

	op->ia_last = ia ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	{
	    int	rs1 ;

	    char	hexbuf[100 + 1] ;

	    rs1 = mkhexstr(hexbuf,100,buf,(bp - buf)) ;
	    debugprintf("exectrace_wia: rs=%d %s diff=%d rs1=%d\n",
	        rs,hexbuf,(bp - buf),rs1) ;
	}
#endif /* CF_DEBUGS */

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wia) */


/* write out the start of a major record */
int exectrace_wsom(op,n)
EXECTRACE	*op ;
uint		n ;
{
	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wsom: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RSOM ;
	netorder_wuint(bp,n) ;

	bp += 4 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wsom) */


/* write out a SYSCALL */
int exectrace_wsyscall(op,n)
EXECTRACE	*op ;
uint		n ;
{
	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wsyscall: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RSYSCALL ;
	netorder_wuint(bp,n) ;

	bp += 4 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wsyscall) */


/* write out an instruction number record */
int exectrace_win(op,in)
EXECTRACE	*op ;
ULONG		in ;
{
	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"win: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RIN ;
	netorder_wull(bp,in) ;

	bp += 8 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_win) */


/* write out a register (small address) record */
int exectrace_wreg(op,a,dv,dp)
EXECTRACE	*op ;
uint		a, dv, dp ;
{
	uint	type ;

	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wreg: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	type = EXECTRACE_RREG | (dp << 4) ;
	*bp++ = type ;
	*bp++ = a ;
	netorder_wuint(bp,dv) ;

	bp += 4 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	debugprintf("exectrace_wreg: rs=%d\n",rs) ;
#endif

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wreg) */


/* write out a register source address record */
int exectrace_wrsa(op,a)
EXECTRACE	*op ;
uint		a ;
{
	uint	type ;

	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wrsa: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RRSA ;
	*bp++ = a ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	{
	    char	hexbuf[100 + 1] ;

	    mkhexstr(hexbuf,100,buf,(bp - buf)) ;
	    debugprintf("exectrace_wrsa: rs=%d %s\n",rs,hexbuf) ;
	}
#endif /* CF_DEBUGS */

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wrsa) */


/* write out a register source value */
int exectrace_wrsv(op,a,dv)
EXECTRACE	*op ;
uint		a, dv ;
{
	uint	type ;

	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wrsv: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RRSV ;
	*bp++ = a ;
	netorder_wuint(bp,dv) ;

	bp += 4 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	debugprintf("exectrace_wrsa: rs=%d\n",rs) ;
#endif

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wrsv) */


/* write out a memory address and value record */
int exectrace_wmem(op,a,dv,dp)
EXECTRACE	*op ;
uint		a, dv, dp ;
{
	uint	type ;

	int	rs = SR_OK ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wmem: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	type = EXECTRACE_RMEM | (dp << 4) ;
	*bp++ = type ;
	netorder_wuint(bp,a) ;

	bp += 4 ;
	netorder_wuint(bp,dv) ;

	bp += 4 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	debugprintf("exectrace_wmem: rs=%d\n",rs) ;
#endif

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wmem) */


/* write out a memory source address */
int exectrace_wmsa(op,a)
EXECTRACE	*op ;
uint		a ;
{
	int	rs ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wmsa: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RMSA ;
	netorder_wuint(bp,a) ;

	bp += 4 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	debugprintf("exectrace_wmsa: a=%08x rs=%d\n",a,rs) ;
#endif

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wmsa) */


/* write out a memory address and value record */
int exectrace_wmsv(op,a,dv)
EXECTRACE	*op ;
uint		a, dv ;
{
	uint	type ;

	int	rs ;

	char	buf[24], *bp = buf ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

#if	CF_DEBUGSRS
	if (op->rs < 0)
		nprintf(EXECTRACEDEB,"wmsv: rs=%d\n",rs) ;
#endif

	if (op->rs < 0)
	    return op->rs ;

	*bp++ = EXECTRACE_RMSV ;
	netorder_wuint(bp,a) ;

	bp += 4 ;
	netorder_wuint(bp,dv) ;

	bp += 4 ;
	rs = bwrite(&op->tfile,buf,(bp - buf)) ;

#if	CF_DEBUGS
	{
	    char	hexbuf[100 + 1] ;

	    mkhexstr(hexbuf,100,buf,(bp - buf)) ;
	    debugprintf("exectrace_wmsv: rs=%d %s\n",
	        rs,hexbuf) ;
	}
#endif /* CF_DEBUGS */

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_wmsv) */


/* read a logical trace record */
int exectrace_read(op,ep)
EXECTRACE	*op ;
EXECTRACE_ENTRY	*ep ;
{
	int	rs, len ;
	int	rtype, n ;
	int	f_clock = FALSE ;
	int	f_som = FALSE ;
	int	f_extra = FALSE ;
	int	f_delay ;

#if	CF_FASTCLEAR
	int	*ebp ;
#endif

	uchar	ch ;


#if	CF_DEBUGS
	debugprintf("exectrace_read: entered\n") ;
#endif

#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;

	if (ep == NULL)
	    return SR_FAULT ;
#endif /* CF_SAFE */

#if	CF_DEBUGS
	debugprintf("exectrace_read: current type=%02x(%d)\n",
	    op->r.type, op->r.type) ;
#endif

#if	CF_FASTCLEAR
	ebp = (int *) &ep->f ;
	*ebp++ = 0 ;
	*ebp++ = 0 ;
#else
	(void) memset(&ep->f,0,sizeof(struct exectrace_eflags)) ;
#endif

	if (op->r.type < 0) {

#if	CF_FASTREAD
	    rs = bread(&op->tfile,&ch,1) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_read: bread() rs=%d ch=%02x(%d)\n",
	        rs,ch,ch) ;
#endif

	    op->r.type = ch & 0xFF ;
	    if (rs <= 0) {

	        op->r.type = -1 ;
	        return rs ;
	    }
#else
	    rs = bgetc(&op->tfile) ;

	    op->r.type = rs ;
	    if ((rs < 0) && (rs != SR_EOF))
	        return rs ;

	    if (rs == SR_EOF)
	        return SR_OK ;
#endif /* CF_FASTREAD */

#if	CF_DEBUGS
	    debugprintf("exectrace_read: 1 newly read type=%02x(%d)\n",
	        op->r.type, op->r.type) ;
#endif

	} /* end if (getting the sub-record type) */

#if	CF_DEBUGS
	debugprintf("exectrace_read: about to loop type=%02x\n",op->r.type) ;
#endif

	rtype = op->r.type & EXECTRACE_TMASK ;
	f_delay = op->istypeinfo[rtype] ;


	n = 0 ;
	while (TRUE) {

	    if (f_clock && (rtype == EXECTRACE_RCLOCK))
	        break ;

	    if (f_som && op->istypesom[rtype])
	        break ;

	    if (f_extra && op->istypeextra[rtype])
	        break ;

	    rs = exectrace_readsub(op,ep,rtype) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_read: exectrace_readsub() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

	    n += 1 ;

	    switch (rtype) {

	    case EXECTRACE_RCLOCK:
	        f_clock = TRUE ;
	        break ;

	    case EXECTRACE_RIA:
	    case EXECTRACE_RSYSCALL:
	    case EXECTRACE_RSOM:
	        f_som = TRUE ;
	        f_clock = TRUE ;
	        break ;

	    default:
	        f_som = TRUE ;

	    } /* end switch */

	    f_extra = f_delay ;

/* read the next sub-record type */

#if	CF_FASTREAD
	    rs = bread(&op->tfile,&ch,1) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_read: bread() rs=%d ch=%02x(%d)\n",
	        rs,ch,ch) ;
#endif

	    op->r.type = ch & 0xFF ;
	    if (rs <= 0) {

	        op->r.type = -1 ;
	        rs = SR_OK ;
	        break ;
	    }
#else
	    rs = bgetc(&op->tfile) ;

	    op->r.type = rs ;
	    if ((rs < 0) && (rs != SR_EOF))
	        return rs ;

	    if (rs == SR_EOF) {

	        rs = SR_OK ;
	        break ;
	    }
#endif /* CF_FASTREAD */

#if	CF_DEBUGS
	    debugprintf("exectrace_read: 2 newly read type=%02x(%d)\n",
	        op->r.type, op->r.type) ;
#endif

	    rtype = op->r.type & EXECTRACE_TMASK ;

	} /* end while */

/* update last things (clock) */

	if (! ep->f.clock)
	    ep->clock = op->r.clock ;

#if	CF_DEBUGS
	debugprintf("exectrace_read: rs=%d\n",rs) ;
#endif

	return ((rs >= 0) ? n : rs) ;
}
/* end subroutine (exectrace_read) */


/* flush out any pending records that may be buffered */
int exectrace_flush(op)
EXECTRACE	*op ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;

	if (op->rs < 0)
	    return op->rs ;

	rs = bflush(&op->tfile) ;

#if	CF_DEBUGS
	debugprintf("exectrace_flush: rs=%d\n",rs) ;
#endif

	op->rs = rs ;
	return rs ;
}
/* end subroutine (exectrace_flush) */


int exectrace_getinfo(op,ipp)
EXECTRACE	*op ;
EXECTRACE_INFO	**ipp ;
{


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != EXECTRACE_MAGIC)
	    return SR_NOTOPEN ;

	if (op->rs < 0)
	    return op->rs ;

	if (ipp == NULL)
	    return SR_FAULT ;

	*ipp = &op->i ;
	return SR_OK ;
}
/* end subroutine (exectrace_getinfo) */



/* PRIVATE SUBROUTINES */



/* read the next sub-record the trace */
static int exectrace_readsub(op,ep,rtype)
EXECTRACE	*op ;
EXECTRACE_ENTRY	*ep ;
uint		rtype ;
{
	uint	ia ;

	int	rs = SR_OK ;
	int	len, sl ;
	int	noi ;

	char	linebuf[LINEBUFLEN + 1] ;
	char	*cp ;


#if	CF_DEBUGS
	{
	    debugprintf("exectrace_readsub: subrecord type=%02x (%s)\n",
	        op->r.type,
	        ((rtype < EXECTRACE_ROVERLAST) ?
	        typenames[rtype] : "*unknown*")) ;
	}
#endif /* CF_DEBUGS */

	switch (rtype) {

	case EXECTRACE_RNAME:
	case EXECTRACE_RWHERE:
	    rs = breadline(&op->tfile,linebuf,LINEBUFLEN) ;

	    if (rs < 0)
	        break ;

	    if (rs == 0) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    len = rs ;
	    if (linebuf[len - 1] == '\n')
	        len -= 1 ;

	    switch (rtype) {

	    case EXECTRACE_RNAME:
	        sl = EXECTRACE_NAMELEN ;
	        cp = op->i.name ;

#if	CF_DEBUGS
	        debugprintf("exectrace_readsub: name=%W\n",
	            linebuf,MIN(len,sl)) ;
#endif

	        break ;

	    case EXECTRACE_RWHERE:
	        sl = EXECTRACE_WHERELEN ;
	        cp = op->i.where ;

#if	CF_DEBUGS
	        debugprintf("exectrace_readsub: where=%W\n",
	            linebuf,MIN(len,sl)) ;
#endif

	        break ;

	    } /* end switch */

	    strwcpy(cp,linebuf,MIN(sl,len)) ;

	    break ;

	case EXECTRACE_RDATE:
	    rs = bread(&op->tfile,linebuf,sizeof(LONG)) ;

	    if (rs < 0)
	        break ;

	    if (rs == 0) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    netorder_rll(linebuf,&op->i.date) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_readsub: date=%016llx %08x:%08x\n",
	        op->i.date,half(op->i.date,1),half(op->i.date,0)) ;
#endif

	    break ;

	case EXECTRACE_RCLOCK:
	    rs = bread(&op->tfile,linebuf,sizeof(LONG)) ;

	    if (rs < 0)
	        break ;

	    if (rs == 0) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    netorder_rull(linebuf,&op->r.clock) ;

	    if (ep != NULL) {

	        ep->clock = op->r.clock ;
	        ep->f.clock = TRUE ;
	    }

	    break ;

	case EXECTRACE_RIA:
	    if ((op->r.type & 0xF0) == 0) {

	        rs = bread(&op->tfile,linebuf,sizeof(uint)) ;

	        if (rs < 0)
	            break ;

	        if (rs == 0) {

	            rs = SR_BADFMT ;
	            break ;
	        }

	        netorder_ruint(linebuf,&ia) ;

	    } else {

	        uint	inc ;


	        inc = (op->r.type >> 4) & 15 ;
	        ia = op->ia_last + inc ;

	    } /* end if (which variety) */

	    if (ep != NULL) {

	        ep->f.ia = TRUE ;
	        ep->ia = ia ;

#if	CF_DEBUGS
	        debugprintf("exectrace_readsub: ia=%08x\n",ep->ia) ;
	        if (ia == 0x00408935) {
	            offset_t	place ;
	            btell(&op->tfile,&place) ;
	            debugprintf("exectrace_readsub: SPEC "
			"type=%02x place=%08lx\n",
	                op->r.type,place) ;
	            debugprintf("exectrace_readsub: next caddr=%p\n",
	                op->tfile.bp) ;
	        }
#endif /* CF_DEBUGS */

	    } /* end if (filling entry) */

	    op->ia_last = ia ;
	    break ;

	case EXECTRACE_RSOM:
	    rs = bread(&op->tfile,linebuf,sizeof(uint)) ;

	    if (rs < 0)
	        break ;

	    if (rs == 0) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    if (ep != NULL) {

	        ep->f.som = TRUE ;
	        netorder_ruint(linebuf,&ep->som) ;

#if	CF_DEBUGS
	        debugprintf("exectrace_readsub: som=%08x\n",ep->som) ;
#endif

	    }

	    break ;

	case EXECTRACE_RSYSCALL:
	    rs = bread(&op->tfile,linebuf,sizeof(uint)) ;

	    if (rs < 0)
	        break ;

	    if (rs == 0) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    if (ep != NULL) {

	        ep->f.sc = TRUE ;
	        netorder_ruint(linebuf,&ep->sc) ;

#if	CF_DEBUGS
	        debugprintf("exectrace_readsub: sc=%08x\n",ep->sc) ;
#endif

	    }

	    break ;

	case EXECTRACE_RIN:
	    rs = bread(&op->tfile,linebuf,sizeof(ULONG)) ;

	    if (rs < 0)
	        break ;

	    if (rs == 0) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    netorder_rull(linebuf,&op->r.in) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_readsub: in=%lld\n",op->r.in) ;
#endif

	    if (ep != NULL) {

	        ep->in = op->r.in ;
	        ep->f.in = TRUE ;

	    }

	    if (! op->f.in) {

	        op->i.in = op->r.in ;
	        op->f.in = TRUE ;

	    }

	    break ;

	case EXECTRACE_RREG:
	    len = sizeof(char) + sizeof(uint) ;
	    rs = bread(&op->tfile,linebuf,len) ;

#if	CF_DEBUGS
	    debugprintf("exectrace_readsub: bread() len=%d rs=%d\n",len,rs) ;
#endif

	    if (rs < 0)
	        break ;

	    if (rs < len) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    if (ep != NULL) {

	        noi = ep->f.reg ;
	        if (noi < EXECTRACE_NREG) {

	            ep->reg[noi].a = linebuf[0] & 0xff ;
	            ep->reg[noi].dp = EXECTRACE_DPALL ;
	            netorder_ruint(linebuf + 1,&ep->reg[noi].dv) ;

#if	CF_DEBUGS
	            debugprintf("exectrace_readsub: ra=%u rv=%08x dp=%d\n",
	                ep->reg[noi].a,
	                ep->reg[noi].dv,
	                ep->reg[noi].dp) ;
#endif

	            ep->f.reg += 1 ;
	        }

	    }

	    break ;

	case EXECTRACE_RRSA:
	case EXECTRACE_RRSV:
	    switch (rtype) {

	    case EXECTRACE_RRSA:
	        len = sizeof(char) ;
	        break ;

	    case EXECTRACE_RRSV:
	        len = sizeof(char) + sizeof(int) ;
	        break ;

	    } /* end switch */

	    rs = bread(&op->tfile,linebuf,len) ;

	    if (rs < 0)
	        break ;

	    if (rs < len) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    if (ep != NULL) {

	        noi = ep->f.sreg ;
	        if (noi < EXECTRACE_NREG) {

	            ep->sreg[noi].a = linebuf[0] & 0xff ;
	            ep->sreg[noi].dp = 0 ;
	            ep->sreg[noi].dv = 0 ;
	            if (rtype == EXECTRACE_RRSV) {

	                ep->sreg[noi].dp = 15 ;
	                netorder_ruint(linebuf + 1,&ep->sreg[noi].dv) ;

	            }

#if	CF_DEBUGS
	            debugprintf("exectrace_readsub: ra=%u rv=%08x dp=%d\n",
	                ep->sreg[noi].a,
	                ep->sreg[noi].dv,
	                ep->sreg[noi].dp) ;
#endif

	            ep->f.sreg += 1 ;

	        } /* end if (place to store) */

	    } /* end if (have store entry) */

	    break ;

	case EXECTRACE_RMEM:
	    len = 2 * sizeof(uint) ;
	    rs = bread(&op->tfile,linebuf,len) ;

	    if (rs < 0)
	        break ;

	    if (rs < len) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    if (ep != NULL) {

	        noi = ep->f.mem ;
	        if (noi < EXECTRACE_NMEM) {

	            ep->mem[noi].dp = (op->r.type >> EXECTRACE_TBITS) & 15 ;
	            netorder_ruint(linebuf,&ep->mem[noi].a) ;

	            netorder_ruint(linebuf + sizeof(uint),&ep->mem[noi].dv) ;

#if	CF_DEBUGS
	            debugprintf("exectrace_readsub: ma=%08x rv=%08x dp=%d\n",
	                ep->mem[noi].a,
	                ep->mem[noi].dv,
	                ep->mem[noi].dp) ;
#endif

	            ep->f.mem += 1 ;

	        } /* end if (place to store) */

	    } /* end if (have store entry) */

	    break ;

	case EXECTRACE_RMSA:
	case EXECTRACE_RMSV:
	    switch (rtype) {

	    case EXECTRACE_RMSA:
	        len = sizeof(int) ;
	        break ;

	    case EXECTRACE_RMSV:
	        len = sizeof(int) + sizeof(int) ;
	        break ;

	    } /* end switch */

	    rs = bread(&op->tfile,linebuf,len) ;

	    if (rs < 0)
	        break ;

	    if (rs < len) {

	        rs = SR_BADFMT ;
	        break ;
	    }

	    if (ep != NULL) {

	        noi = ep->f.smem ;
	        if (noi < EXECTRACE_NMEM) {

	            ep->smem[noi].dp = GETDP(op->r.type) ;
	            netorder_ruint(linebuf,&ep->smem[noi].a) ;

	            if (rtype == EXECTRACE_RMSV) {

	                ep->smem[noi].dp = 15 ;
	                netorder_ruint(linebuf + sizeof(uint),
	                    &ep->smem[noi].dv) ;

	            }

#if	CF_DEBUGS
	            debugprintf("exectrace_readsub: ma=%08x rv=%08x dp=%d\n",
	                ep->smem[noi].a,
	                ep->smem[noi].dv,
	                ep->smem[noi].dp) ;
#endif

	            ep->f.smem += 1 ;

	        } /* end if (place to store) */

	    } /* end if (have store entry) */

	    break ;

	default:
	    rs = SR_NOTSUP ;

#if	CF_DEBUGS
	    {
	        offset_t	place ;
	        btell(&op->tfile,&place) ;
	        debugprintf("exectrace_readsub: unknown "
			"type=%02x type=%02x place=%08lx\n",
	            op->r.type,rtype,place) ;
	    }
#endif /* CF_DEBUGS */

	} /* end switch */

#if	CF_DEBUGS
	debugprintf("exectrace_readsub: rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (exectrace_readsub) */


#ifdef	COMMENT

/* make the mode string for opening the file */
static void mkmodestr(buf,mode)
char	buf[] ;
int	mode ;
{
	int	f_read = FALSE ;
	int	f_write = FALSE ;

	char	*cp = buf ;


	if ((mode & O_RDONLY) == O_RDONLY) {

	    f_read = TRUE ;
	    *cp++ = 'r' ;
	}

	if ((mode & O_WRONLY) == O_WRONLY) {

	    f_write = TRUE ;
	    *cp++ = 'w' ;
	}

	if ((mode & O_RDWR) == O_RDWR) {

	    if (! f_write) {

	        f_write = TRUE ;
	        *cp++ = 'w' ;
	    }

	    if (! f_read) {

	        f_read = TRUE ;
	        *cp++ = 'r' ;
	    }
	}

	if ((mode & O_CREAT) == O_CREAT)
	    *cp++ = 'c' ;

	if ((mode & O_TRUNC) == O_TRUNC)
	    *cp++ = 't' ;

	*cp = '\0' ;
}
/* end subroutine (mkmodestr) */

#endif /* COMMENT */


#ifdef	COMMENT

static int istypeinfo(type)
int	type ;
{


	return ((type >= 0) && (type < EXECTRACE_RCLOCK)) ;
}
/* end subroutine (istypeinfo) */


static int istypeextra(type)
int	type ;
{


	if ((type & EXECTRACE_TMASK) == EXECTRACE_RREG)
	    return TRUE ;

	if ((type & EXECTRACE_TMASK) == EXECTRACE_RMEM)
	    return TRUE ;

	if ((type & EXECTRACE_TMASK) == EXECTRACE_RIN)
	    return TRUE ;

	if ((type & EXECTRACE_TMASK) == EXECTRACE_RREGDUMP)
	    return TRUE ;

	return FALSE ;
}
/* end subroutine (istypeextra) */

#endif /* COMMENT */


#if	CF_DEBUGS

static int half(v,w)
LONG	v ;
int	w ;
{
	LONG	lw ;

	int	iw ;


	if (w == 0)
	    lw = (v & 0xffffffff) ;

	else
	    lw = (v >> 32) & 0xffffffff ;

	iw = (int) lw ;
	return iw ;
}
/* end subroutine (half) */

#endif /* CF_DEBUGS */



