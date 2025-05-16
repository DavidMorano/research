/* syscalls */

/* emulate the system calls */


#define	F_DEBUGS	0
#define	F_DEBUG		1
#define	F_SAFE		1


/* revision history :

	= 01/10/18, Dave Morano

	I am starting this out from scratch for the LevoSim simulator.


*/


/******************************************************************************

	This module is an attempt to emulate some of the system calls
	that may be used by the Spec-2000 programs.


******************************************************************************/


#define	SYSCALLS_MASTER	0


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<string.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"syscalls.h"
#include	"traceinfo.h"
#include	"exectrace.h"

#include	"config.h"
#include	"defs.h"
#include	"lsim.h"
#include	"sgierrno.h"
#include	"sgistat.h"
#include	"sgiresource.h"
#include	"sgisignal.h"
#include	"sgiprctl.h"
#include	"sgisystime.h"
#include	"sgisystimes.h"
#include	"sgifcntl.h"



/* local defines */

#define	SYSCALLS_MAGIC		0x86553823
#define	SYSCALLS_RETPID		100		/* PID to return to target */
#define	SYSCALLS_RETUID		100		/* PID to return to target */
#define	SYSCALLS_PAGESIZE	0x4000		/* SGI pagesize */
#define	SYSCALLS_SGIBLKSIZE	0x4000
#define	SYSCALLS_SGIUNIXBLOCK	512		/* all UNIX the same ? */

#ifndef	BUFLEN
#define	BUFLEN		(8 * 1024)
#endif

#define	O_WRFLAGS	(O_WRONLY | O_CREAT | O_TRUNC)

#define	ceiling(a,b)	(((a) + ((b) - 1)) & (~ ((b) - 1)))



/* external subroutines */

extern int	getnodedomain(char *,char *) ;
extern int	getlogname(char *,int) ;
extern int	sfshrink(const char *,int,char **) ;
extern int	cfdeci(const char *,int,int *) ;

extern char	*strwcpy(char *,const char *,int) ;


/* local structures */

struct scentry {
	char	*name ;
	int	(*handler)() ;
} ;

struct indexentry {
	uint	symval ;
	int	ei ;
} ;


/* forward references */

static int	syscalls_indexinit(SYSCALLS *,LSIM *) ;
static int	syscalls_indexfree(SYSCALLS *,LSIM *) ;
static int	syscalls_memread(SYSCALLS *,struct proginfo *,LSIM *,
			uint,char *,int) ;
static int	syscalls_memreadstr(SYSCALLS *,struct proginfo *,LSIM *,
			uint,char *,int) ;
static int	syscalls_memwrite(SYSCALLS *,struct proginfo *,LSIM *,
			uint,char *,int) ;

static int	handle_exit(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_getpid(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_getuid(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_open(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_close(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_read(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_write(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_lseek(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_fcntl(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_ioctl(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_brk(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_sbrk(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_stat(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_fstat(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_getpagesize(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_getrlimit(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_setrlimit(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_signal(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_access(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_getrusage(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_time(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_times(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_prctl(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_syssgi(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;
static int	handle_isatty(SYSCALLS *,struct proginfo *,LSIM *,
			SYSCALLS_ARGREGS *,SYSCALLS_RETREG *) ;


/* local data */

struct scentry	sctab[] = {
	    { "_exit", handle_exit },
	    { "__exit", handle_exit },
	    { "getpid", handle_getpid },
	    { "_getpid", handle_getpid },
	    { "getuid", handle_getuid },
	    { "_getuid", handle_getuid },
	    { "geteuid", handle_getuid },
	    { "_geteuid", handle_getuid },
	    { "getgid", handle_getuid },
	    { "_getgid", handle_getuid },
	    { "getegid", handle_getuid },
	    { "_getegid", handle_getuid },
	    { "open", handle_open },
	    { "_open", handle_open },
	    { "close", handle_close },
	    { "_close", handle_close },
	    { "read", handle_read },
	    { "_read", handle_read },
	    { "write", handle_write },
	    { "_write", handle_write },
	    { "lseek", handle_lseek },
	    { "_lseek", handle_lseek },
	    { "fstat", handle_fstat },
	    { "_fstat", handle_fstat },
	    { "fcntl", handle_fcntl },
	    { "_fcntl", handle_fcntl },
	    { "ioctl", handle_ioctl },
	    { "_ioctl", handle_ioctl },
	    { "brk", handle_brk },
	    { "_brk", handle_brk },
	    { "sbrk", handle_sbrk },
	    { "_sbrk", handle_sbrk },
	    { "stat", handle_stat },
	    { "_stat", handle_stat },
	    { "getpagesize", handle_getpagesize },
	    { "_getpagesize", handle_getpagesize },
	    { "getrlimit", handle_getrlimit },
	    { "_getrlimit", handle_getrlimit },
	    { "setrlimit", handle_setrlimit },
	    { "_setrlimit", handle_setrlimit },
	    { "signal", handle_signal },
	    { "_signal", handle_signal },
	    { "access", handle_access },
	    { "_access", handle_access },
	    { "getrusage", handle_getrusage },
	    { "_getrusage", handle_getrusage },
	    { "time", handle_time },
	    { "_time", handle_time },
	    { "times", handle_times },
	    { "_times", handle_times },
	    { "prctl", handle_prctl },
	    { "_prctl", handle_prctl },
	    { "syssgi", handle_syssgi },
	    { "_syssgi", handle_syssgi },
	    { "isatty", handle_isatty },
	    { NULL, NULL }
} ;





int syscalls_init(op,pip,mip,smp,stdfiles)
SYSCALLS	*op ;
struct proginfo	*pip ;
LSIM		*mip ;
USTATemips	*smp ;
char		*stdfiles[] ;
{
	int	rs, i ;
	int	fd ;
	int	oflags ;


	(void) memset(op,0,sizeof(SYSCALLS)) ;

	op->pip = pip ;
	op->mip = mip ;
	op->smp = smp ;


/* get a fake FS name and device number (get from 'prog' variables) */

	op->sgidev = pip->sgidev ;
	strncpy(op->sgifstype,pip->sgifstype,SYSCALLS_FSSIZE) ;


/* index the system call names */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3) {
	    eprintf("syscalls_init: attempting to handle system calls:\n") ;
	    for (i = 0 ; sctab[i].name != NULL ; i += 1)
	        eprintf("syscalls_init: n> %s\n",sctab[i].name) ;
	}
#endif

	rs = syscalls_indexinit(op,mip) ;

	if (rs < 0)
	    goto ret0 ;

/* find the symbol value for 'errno' */

	rs = lsim_getsymval(mip,"errno",&op->ea) ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("syscalls_init: lsim_getvalue() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    op->ea = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("syscalls_init: ERRNO=%08x\n",op->ea) ;
#endif


/* initialize the FD translations */

	for (i = 0 ; i < SYSCALLS_NFILE ; i += 1)
	    op->fds[i].fd = -1 ;

/* open up any standard files for starters */

	for (i = 0 ; i < 3 ; i += 1) {

	    if ((stdfiles[i] != NULL) && (stdfiles[i] != '\0')) {

	        oflags = (i == 0) ? O_RDONLY : O_WRFLAGS ;
	        oflags |= O_CREAT ;
	        fd = u_open(stdfiles[i],oflags,0666) ;

	        if (fd >= 0) {

			int	nopen ;


	            op->fds[i].fd = fd ;
		    op->s.c_fileopen += 1 ;
		    nopen = op->s.c_fileopen - op->s.c_fileclose ;
		    if (nopen > op->s.maxopen)
			op->s.maxopen = nopen ;

		}

	    }

	} /* end for */

/* get the current "break" address for fun */

	lsim_getbreak(mip,&op->brka) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 2)
	    eprintf("syscalls_init: brka=%08x\n",op->brka) ;
#endif

	op->brkbase = op->brka ;
	op->brkmax = op->brka ;



	op->magic = SYSCALLS_MAGIC ;

ret0:

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("syscalls_init: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (syscalls_init) */


/* free up */
int syscalls_free(op)
SYSCALLS	*op ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	rs = SR_OK ;
	int	i ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != SYSCALLS_MAGIC)
	    return SR_NOTOPEN ;


	pip = op->pip ;
	mip = op->mip ;


/* close any open target files */

	for (i = 0 ; i < SYSCALLS_NFILE ; i += 1) {

	    if (op->fds[i].fd >= 0)
	        u_close(op->fds[i].fd) ;

	}

/* release other stuff */

	rs = syscalls_indexfree(op,mip) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls_free: rs=%d\n",rs) ;
#endif

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (syscalls_free) */


/* is it a SYSCALL ? */
int syscalls_issyscall(op,addr,npp)
SYSCALLS	*op ;
uint		addr ;
const char	**npp ;
{
	struct proginfo		*pip ;

	struct indexentry	*iep ;

	HDB_DATUM	key, value ;

	int	rs ;
	int	f_found ;


#if	F_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != SYSCALLS_MAGIC)
	    return (op->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
#endif /* F_SAFE */


	pip = op->pip ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 6)
	    eprintf("syscalls_issyscall: entered\n") ;
#endif

	key.buf = &addr ;
	key.len = sizeof(uint) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 6)
	    eprintf("syscalls_issyscall: hdb_fetch()\n") ;
#endif

	rs = hdb_fetch(&op->index,key,NULL,&value) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 6) {
	    if (rs == SR_NOTFOUND)
	        eprintf("syscalls_issyscall: hdb_fetch() rs=NOTFOUND\n") ;
	    else
	        eprintf("syscalls_issyscall: hdb_fetch() rs=%d\n",rs) ;
	}
#endif

	if ((rs < 0) && (rs != SR_NOTFOUND))
	    return rs ;

	f_found = (rs >= 0) ;
	rs = SR_OK ;

	if (npp != NULL) {

	    iep = (struct indexentry *) value.buf ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 6)
	        eprintf("syscalls_issyscall: iep=%p \n",iep) ;
#endif

	    *npp = NULL ;
	    if (f_found)
	        *npp = sctab[iep->ei].name ;

	} /* end if (got one) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 6)
	    eprintf("syscalls_issyscall: exiting rs=%d f_found=%d\n",
	        rs,f_found) ;
#endif

	return ((rs >= 0) ? f_found : rs) ;
}
/* end subroutine (syscalls_issyscall) */


/* try to handle a SYSCALL */
int syscalls_handle(op,ia,arp,rrp)
SYSCALLS		*op ;
uint			ia ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	struct proginfo		*pip ;

	LSIM			*mip ;

	struct indexentry	*iep ;

	HDB_DATUM	key, value ;

	int	rs = SR_OK ;
	int	ei ;
	int	f_exit = FALSE ;

	const char	*np ;


#if	F_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != SYSCALLS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = op->pip ;
	mip = op->mip ;

/* return if it isn't a PSYSCALL */

	key.buf = &ia ;
	key.len = sizeof(uint) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 6)
	    eprintf("syscalls_handle: hdb_fetch() \n") ;
#endif

	rs = hdb_fetch(&op->index,key,NULL,&value) ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 6) {
	    if (rs == SR_NOTFOUND)
	        eprintf("syscalls_issyscall: hdb_fetch() rs=NOTFOUND\n") ;
	    else
	        eprintf("syscalls_issyscall: hdb_fetch() rs=%d\n",rs) ;
	}
#endif

	if ((rs < 0) && (rs != SR_NOTFOUND))
	    return rs ;

	iep = (struct indexentry *) value.buf ;
	ei = iep->ei ;
	np = sctab[ei].name ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("syscalls_handle: n(%d)=%s\n",ei,sctab[ei].name) ;
#endif


	if (traceinfo_fastenabled(&pip->ti) &&
	    traceinfo_syscalls(&pip->ti))
	    exectrace_wsyscall(&pip->ti.et,(uint) ei) ;


	(void) memset(rrp,0,sizeof(SYSCALLS_RETREG)) ;

	rrp->n = 0 ;
	rrp->sgierrno = 0 ;
	rrp->sgirc = 0 ;

	if (sctab[ei].handler != NULL) {

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 6)
	        eprintf("syscalls_handle: handler(%s)=%p\n",
	            sctab[ei].name,sctab[ei].handler) ;
#endif

	    rs = (*sctab[ei].handler)(op,pip,mip,arp,rrp) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("syscalls_handle: handler(%s) rs=%d\n",
	            sctab[ei].name,rs) ;
#endif

	} else
	    rs = SR_BADFMT ;

	if (rrp->sgierrno > 0) {

/* write the value of 'errno' back to the target memory */

	    rrp->sgirc = -1 ;

	    rs = lsim_writeint(mip,op->ea,rrp->sgierrno,15) ;

	    if ((rs >= 0) && traceinfo_fastenabled(&pip->ti) &&
	        traceinfo_mems(&pip->ti)) {

	        rs = exectrace_wmem(&pip->ti.et,op->ea,rrp->sgierrno,15) ;

	    }

	} /* end if (errno writeback) */


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls_handle: rs=%d\n",rs) ;
#endif

	f_exit = (strcmp(np,"_exit") == 0) || (strcmp(np,"__exit") == 0) ;

	return (rs >= 0) ? f_exit : rs ;
}
/* end subroutine (syscalls_handle) */


int syscalls_getbrkmax(op,rp,hp)
SYSCALLS	*op ;
uint		*rp, *hp ;
{
	struct proginfo	*pip ;

	LSIM		*mip ;

	int	rs = SR_OK ;


#if	F_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != SYSCALLS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = op->pip ;
	mip = op->mip ;

	if (rp != NULL)
	    *rp = op->brkmax ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("syscalls_getbrkmax: brkbase=%08x brkmax=%08x\n",
		op->brkbase,op->brkmax) ;
#endif

	if (hp != NULL)
		*hp = op->brkmax - op->brkbase ;

	return rs ;
}
/* end subroutine (syscalls_getbrkmax) */


int syscalls_stats(op,sp)
SYSCALLS	*op ;
SYSCALLS_STATS	*sp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


#if	F_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != SYSCALLS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	pip = op->pip ;

	if (sp == NULL)
		return SR_FAULT ;

	*sp = op->s ;

	return rs ;
}
/* end subroutine (syscalls_stats) */



/* PRIVATE SUBROUTINES */



/* find any SYSCALLs and index them by their function address */
static int syscalls_indexinit(op,mip)
SYSCALLS	*op ;
LSIM		*mip ;
{
	struct proginfo	*pip ;

	LSIM_SNCURSOR	cur ;

	HDB_DATUM	key, value ;

	Elf32_Sym	*eep ;

	struct indexentry	*iep ;

	int	rs, i ;


	pip = mip->pip ;

	rs = hdb_init(&op->index,50,NULL,NULL) ;

	if (rs < 0)
	    return rs ;

	for (i = 0 ; sctab[i].name != NULL ; i += 1) {

	    lsim_sncursorinit(mip,&cur) ;

	    while (TRUE) {

	        rs = lsim_fetchsym(mip,sctab[i].name,&cur,&eep) ;

	        if (rs < 0) {

	            rs = SR_OK ;
	            break ;
	        }

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4) {
	            eprintf("syscalls_indexinit: lsim_fetchsym() rs=%d\n",rs) ;
	            eprintf("syscalls_indexinit: BIND=%d TYPE=%d\n",
	                ELF32_ST_BIND(eep->st_info),
	                ELF32_ST_TYPE(eep->st_info)) ;
	        }
#endif /* F_DEBUG */

	        if ((rs >= 0) && 
	            ((ELF32_ST_BIND(eep->st_info) == STB_GLOBAL) ||
	            (ELF32_ST_BIND(eep->st_info) == STB_WEAK)) &&
	            (ELF32_ST_TYPE(eep->st_info) == STT_FUNC)) {

#if	F_MASTERDEBUG && F_DEBUG
	            if (pip->debuglevel >= 4)
	                eprintf("syscalls_indexinit: sym=%s value=%08x\n",
	                    sctab[i].name,eep->st_value) ;
#endif

	            rs = uc_malloc(sizeof(struct indexentry),&iep) ;

	            if (rs < 0)
	                break ;

	            iep->symval = eep->st_value ;
	            iep->ei = i ;

	            key.buf = &eep->st_value ;
	            key.len = sizeof(Elf32_Addr) ;

	            value.buf = iep ;
	            value.len = sizeof(struct indexentry) ;

	            rs = hdb_store(&op->index,key,value) ;

	            if (rs < 0)
	                free(iep) ;

	            break ;

	        } /* end if (we got one) */

	    } /* end while (looping through candidate symbols) */

	    lsim_sncursorfree(mip,&cur) ;

	    if (rs < 0)
	        break ;

	} /* end for (looping through possible SYSCALLs) */

	if (rs < 0)
	    goto bad1 ;

	return SR_OK ;

/* bad things come here */
bad1:
	(void) syscalls_indexfree(op,mip) ;

	return rs ;
}
/* end subroutine (syscalls_indexinit) */


/* free up the data used to hold SYSCALL information */
static int syscalls_indexfree(op,mip)
SYSCALLS	*op ;
LSIM		*mip ;
{
	HDB_CURSOR	cur ;

	HDB_DATUM	key, value ;

	struct indexentry	*iep ;

	int	rs = SR_OK ;


	hdb_cursorinit(&op->index,&cur) ;

	while (hdb_enum(&op->index,&cur,&key,&value) >= 0) {

	    iep = (struct indexentry *) value.buf ;

	    if (iep != NULL)
	        free(iep) ;

	} /* end while */

	hdb_cursorfree(&op->index,&cur) ;

	rs = hdb_free(&op->index) ;

	return rs ;
}
/* end subroutine (syscalls_indexfree) */


/* exit */
static int handle_exit(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	int	rs = SR_BADFMT ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_exit: entered\n") ;
#endif


	return 1 ;
}
/* end subroutine (handle_exit) */


/* getpid */
static int handle_getpid(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	int	rs = SR_BADFMT ;

	int	n ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getpid: entered\n") ;
#endif

	rrp->sgirc = SYSCALLS_RETPID ;
	rrp->sgierrno = 0 ;

	return 0 ;
}
/* end subroutine (handle_getpid) */


static int handle_getuid(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	int	rs = SR_BADFMT ;

	int	ten = 0 ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getuid: entered\n") ;
#endif

	rrp->sgirc = SYSCALLS_RETUID ;
	rrp->sgierrno = 0 ;

	return 0 ;
}
/* end subroutine (handle_getuid) */


/* open */
static int handle_open(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	int	rs = SR_BADFMT ;
	int	i, j, n ;
	int	ten, tfdi, tfd ;

	char	filename[MAXPATHLEN + 1] ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_open: entered\n") ;
#endif

	op->s.c_callopen += 1 ;
	ten = 0 ;

/* find the lowest numbered target FD that is not being used */

	for (tfdi = 0 ; tfdi < SYSCALLS_NFILE ; tfdi += 1) {

	    if (op->fds[tfdi].fd < 0)
	        break ;

	} /* end for */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_open: tfdi=%d\n", tfdi) ;
#endif

	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	rrp->sgirc = tfdi ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3) {
	    eprintf("syscalls/handle_open: a1=%08x\n",arp->a1) ;
	    eprintf("syscalls/handle_open: a2=%08x\n",arp->a2) ;
	    eprintf("syscalls/handle_open: a3=%08x\n",arp->a3) ;
	}
#endif

	if (ten == 0) {

	    if (arp->a1 == 0)
	        ten = SGIE_FAULT ;

	}

	if (ten == 0) {

/* get arguments */

	    rs = syscalls_memreadstr(op,pip,mip,arp->a1,filename,MAXPATHLEN) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 3) {
	        eprintf("syscalls/handle_open: syscalls_memereadstr() rs=%d\n",
			rs) ;
	        eprintf("syscalls/handle_open: ta=%08x\n",arp->a1) ;
	        j = 0 ;
	        for (i = 0 ; i < rs ; i += 1) {
	            if (j == 0)
	                eprintf("syscalls/handle_open: data> ") ;
	            eprintf(" %02x",filename[i]) ;
	            if (j == 7)
	                eprintf("\n") ;

	            j = (j + 1) % 8 ;
	        }
	        if (j != 0)
	            eprintf("\n") ;

	        if (rs >= 0)
	            eprintf("syscalls/handle_open: filename=%s\n",filename) ;
	    }
#endif /* F_DEBUG */

	    if (rs == 0)
	        ten = SGIE_NOENT ;

	}

	if ((rs >= 0) && (ten == 0)) {

	    rs = u_open(filename,arp->a2,arp->a3) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 3)
	        eprintf("syscalls/handle_open: u_open() rs=%d\n",rs) ;
#endif

	    switch (rs) {

	    case SR_NOENT:
	        ten = SGIE_NOENT ;
	        break ;

	    case SR_ACCES:
	        ten = SGIE_ACCES ;
	        break ;

	    case SR_LOOP:
	        ten = SGIE_LOOP ;
	        break ;

	    case SR_NOTDIR:
	        ten = SGIE_NOTDIR ;
	        break ;

	    } /* end switch */

	}

	if ((rs >= 0) && (ten == 0)) {

		int	nopen ;


	    op->fds[tfdi].fd = rs ;
	    op->s.c_fileopen += 1 ;
		    nopen = op->s.c_fileopen - op->s.c_fileclose ;
		    if (nopen > op->s.maxopen)
			op->s.maxopen = nopen ;

	}

/* return */

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_open) */


static int handle_close(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	int	rs = SR_OK ;
	int	rs1 ;
	int	i, j, n ;
	int	tfdi ;
	int	ten ;


	op->s.c_callclose += 1 ;
	ten = 0 ;
	tfdi = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_close: tfd=%d\n",tfdi) ;
#endif

	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	if ((ten == 0) && (op->fds[tfdi].fd < 0))
	    ten = SGIE_BADF ;

	rs1 = u_close(op->fds[tfdi].fd) ;

	if ((ten == 0) && (rs1 < 0)) {

	    switch (rs1) {

	    case SR_BADF:
	        ten = SGIE_BADF ;
	        break ;

	    case SR_SPIPE:
	        ten = SGIE_SPIPE ;
	        break ;

	    case SR_INVAL:
	        ten = SGIE_INVAL ;
	        break ;

	    case SR_OVERFLOW:
	        ten = SGIE_OVERFLOW ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if */

	if ((rs >= 0) && (ten == 0))
		op->s.c_fileclose += 1 ;

	rrp->sgirc = rs1 ;
	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_close) */


static int handle_read(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	uint	ata, ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	tfdi ;
	int	mlen, alen, rlen, tlen ;
	int	ten ;

	char	buf[BUFLEN + 1] ;


	op->s.c_callread += 1 ;
	ten = 0 ;
	tfdi = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_read: tfd=%d\n",tfdi) ;
#endif

	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	if ((ten == 0) && (op->fds[tfdi].fd < 0))
	    ten = SGIE_BADF ;

	ata = (uint) arp->a2 ;
	alen = (int) arp->a3 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_read: ta=%08x len=%d\n",ata,alen) ;
#endif

	if (ten == 0) {

	    tlen = 0 ;
	    ta = ata ;
	    while (tlen < alen) {

	        rlen = MIN((alen - tlen),BUFLEN) ;

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 4)
	            eprintf("syscalls/handle_read: u_read() rlen=%d\n",rlen) ;
#endif

	        rs1 = u_read(op->fds[tfdi].fd,buf,rlen) ;

	        if (rs1 <= 0)
	            break ;

	        mlen = rs1 ;
	        rs = syscalls_memwrite(op,pip,mip,ta,buf,mlen) ;

	        if (rs < 0)
	            break ;

	        tlen += mlen ;
	        ta += mlen ;

	    } /* end while */

	}

	if ((ten == 0) && (rs1 < 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = tlen ;
	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_read) */


static int handle_write(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	uint	ata, ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	tfdi ;
	int	mlen, alen, rlen, tlen ;
	int	ten ;

	char	buf[BUFLEN + 1] ;


	op->s.c_callwrite += 1 ;
	ten = 0 ;
	tfdi = arp->a1 ;
	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	if ((ten == 0) && (op->fds[tfdi].fd < 0))
	    ten = SGIE_BADF ;

	ata = (uint) arp->a2 ;
	alen = (int) arp->a3 ;

	if (ten == 0) {

	    tlen = 0 ;
	    ta = ata ;
	    while (tlen < alen) {

	        mlen = MIN((alen - tlen),BUFLEN) ;
	        rs = syscalls_memread(op,pip,mip,ta,buf,mlen) ;

	        if (rs < 0)
	            break ;

	        rs1 = u_write(op->fds[tfdi].fd,buf,mlen) ;

	        if (rs1 <= 0)
	            break ;

	        tlen += mlen ;
	        ta += mlen ;

	    } /* end while */

	}

	if ((ten == 0) && (rs1 < 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = tlen ;
	if (ten > 0)
	    rrp->sgierrno = ten ;

	return SR_OK ;
}
/* end subroutine (handle_write) */


static int handle_lseek(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	off_t	cur ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	tfdi ;
	int	ten ;


	ten = 0 ;
	tfdi = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_lseek: tfd=%d a2=%08x a3=%d\n",
	        tfdi,arp->a2,arp->a3) ;
#endif

	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	if ((ten == 0) && (op->fds[tfdi].fd < 0))
	    ten = SGIE_BADF ;

	rs1 = u_seek(op->fds[tfdi].fd,(off_t) arp->a2,arp->a3) ;

	if ((ten == 0) && (rs1 < 0)) {

	    switch (rs1) {

	    case SR_BADF:
	        ten = SGIE_BADF ;
	        break ;

	    case SR_SPIPE:
	        ten = SGIE_SPIPE ;
	        break ;

	    case SR_INVAL:
	        ten = SGIE_INVAL ;
	        break ;

	    case SR_OVERFLOW:
	        ten = SGIE_OVERFLOW ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	}

	if ((ten == 0) && (rs1 >= 0)) {

	    rs = u_tell(op->fds[tfdi].fd,&cur) ;

	}

	rrp->sgirc = (int) cur ;
	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_lseek) */


static int handle_fstat(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	USTAT	sb ;

	struct sgistat	sgib ;

	uint	ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	tfdi ;
	int	mlen, alen, rlen, tlen ;
	int	ten ;


	ten = 0 ;
	tfdi = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_fstat: tfdi=%08x sgisize=%d solsize=%d\n",
	        tfdi,
	        sizeof(struct sgistat),
	        sizeof(USTAT)) ;
#endif

	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	if ((ten == 0) && (op->fds[tfdi].fd < 0))
	    ten = SGIE_BADF ;

	ta = (uint) arp->a2 ;

	if (ten == 0)
	    rs1 = u_fstat(op->fds[tfdi].fd,&sb) ;

	if ((rs1 >= 0) && (ten == 0)) {

	    (void) memset(&sgib,0,sizeof(struct sgistat)) ;

/* SGI device number */

		if (op->sgidev != 0)
	    sgib.st_dev = op->sgidev ;

		else
	    sgib.st_dev = sb.st_dev ;

/* everything else */

	    sgib.st_ino = sb.st_ino ;
	    sgib.st_mode = sb.st_mode ;
	    sgib.st_nlink = sb.st_nlink ;
	    sgib.st_uid = sb.st_uid ;
	    sgib.st_gid = sb.st_gid ;
	    sgib.st_rdev = sb.st_rdev ;
	    sgib.st_size = sb.st_size ;

	    sgib.st_atim.tv_sec = sb.st_atim.tv_sec ;
	    sgib.st_atim.tv_nsec = sb.st_atim.tv_nsec ;
	    sgib.st_mtim.tv_sec = sb.st_mtim.tv_sec ;
	    sgib.st_mtim.tv_nsec = sb.st_mtim.tv_nsec ;
	    sgib.st_ctim.tv_sec = sb.st_ctim.tv_sec ;
	    sgib.st_ctim.tv_nsec = sb.st_ctim.tv_nsec ;

	    sgib.st_blksize = SYSCALLS_SGIBLKSIZE ;
	    sgib.st_blocks = sb.st_blocks ;

/* SGI FS typename */

	if (op->sgifstype[0] != '\0')
	    (void) memcpy(sgib.st_fstype,op->sgifstype,SGI_ST_FSTYPSZ) ;

	else
	    (void) memcpy(sgib.st_fstype,sb.st_fstype,SGI_ST_FSTYPSZ) ;

/* write the struct back to the target memory */

	    rs = syscalls_memwrite(op,pip,mip,ta,
	        (char *) &sgib,sizeof(struct sgistat)) ;

	    switch (rs) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_ACCESS:
	        ten = SGIE_ACCES ;
	        break ;

	    } /* end switch */

	}

	rrp->sgirc = ten ;

	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_fstat: sgirc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_fstat) */


static int handle_fcntl(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	struct flock	fl ;

	struct sgiflock	sgifl ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	ten ;
	int	tfdi, tfd, i ;


	ten = 0 ;
	tfdi = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_fcntl: tfd=%d a2=%08x a3=%d\n",
	        tfdi,arp->a2,arp->a3) ;
#endif

	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	if ((ten == 0) && (op->fds[tfdi].fd < 0))
	    ten = SGIE_BADF ;

	tfd = op->fds[tfdi].fd ;
	if (ten == 0) {

	    switch (arp->a2) {

	    case SGIF_SETFD:
	        rs1 = u_fcntl(tfd,F_SETFD,(arp->a3 ? 1 : 0)) ;

	        break ;

	    case SGIF_DUPFD:
	        if ((arp->a3 < 0) || (arp->a3 >= SYSCALLS_NFILE))
	            ten = SGIE_INVAL ;

	        if (ten == 0) {

/* do we have a spare FD slot for the target program ? */

	            for (i = arp->a3 ; i < SYSCALLS_NFILE ; i += 1) {

	                if (op->fds[i].fd < 0)
	                    break ;

	            } /* end for */

	            if (i < SYSCALLS_NFILE) {

	                rs1 = u_fcntl(tfd,F_DUPFD,0) ;

	                if (rs1 >= 0) {

	                    op->fds[i].fd = rs1 ;
	                    rs1 = i ;

	                }

	            } else
	                rs1 = SGIE_MFILE ;

	        } /* end if */

	        break ;

	    case SGIF_FREESP:
	    case SGIF_SETLK:
	    case SGIF_SETLKW:
	        {
	            uint	ta ;

	            int	len, cmd ;


	            ta = arp->a3 ;
	            len = sizeof(struct sgiflock) ;
	            rs = syscalls_memread(op,pip,mip,ta,(char *) &sgifl,len) ;

	            if (rs < 0)
	                break ;

	            (void) memset(&fl,0,sizeof(struct flock)) ;

	            fl.l_type = sgifl.l_type ;
	            fl.l_whence = sgifl.l_whence ;
	            fl.l_start = sgifl.l_start ;
	            fl.l_len = sgifl.l_len ;
	            fl.l_sysid = sgifl.l_sysid ;
	            fl.l_pid = sgifl.l_pid ;

	            switch (arp->a2) {

	            case SGIF_FREESP:
	                cmd = F_FREESP ;
	                break ;

	            case SGIF_SETLK:
	                cmd = F_SETLK ;
	                break ;

	            case SGIF_SETLKW:
	                cmd = F_SETLKW ;
	                break ;

	            } /* end switch */

	            rs1 = u_fcntl(tfd,cmd,&fl) ;

	        } /* end block */

	        break ;

	    default:

#if	F_MASTERDEBUG && F_DEBUG
	        if (pip->debuglevel >= 2)
	            eprintf("syscalls/handle_fcntl: unknown command=%d\n",
	                arp->a3) ;
#endif

	        rs = SR_NOTSUP ;

	    } /* end switch */

	} /* end if */

	if ((rs >= 0) && (rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = rs1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_fcntl: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_fcntl) */


static int handle_ioctl(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	LSIM_MAXPROG	max ;

	struct sgirlimit	rl ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	ten ;
	int	tfdi, tfd ;


	ten = 0 ;
	tfdi = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_ioctl: tfd=%d a2=%08x a3=%d\n",
	        tfdi,arp->a2,arp->a3) ;
#endif

	if (tfdi >= SYSCALLS_NFILE)
	    ten = SGIE_BADF ;

	if ((ten == 0) && (op->fds[tfdi].fd < 0))
	    ten = SGIE_BADF ;



	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_ioctl: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_ioctl) */


static int handle_brk(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	uint	newval ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	ten ;

	char	buf[BUFLEN + 1] ;


	ten = 0 ;
	newval = (uint) arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("syscalls/handle_brk: newval=%08x\n",newval) ;
#endif

	if (newval < op->brkbase)
	    ten = SGIE_INVAL ;

	if (ten == 0)
	    rs1 = lsim_setbreak(mip,newval) ;

	if (rs1 < 0)
	    bprintf(pip->efp,"%s: break value overflow (%d)\n",
	        pip->progname,rs1) ;

	if (rs1 >= 0) {

	    op->brka = newval ;
	    if (newval > op->brkmax)
	        op->brkmax = newval ;

	}

	if (rs1 < 0) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        ten = SGIE_NOTSUP ;
	        rs = rs1 ;

	        bprintf(pip->efp,"%s: handle_brk() rs=%d\n",
	            pip->progname,rs) ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = 0 ;
	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_brk) */


static int handle_sbrk(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	uint	newval, incr ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	mlen, alen, rlen, tlen ;
	int	ten ;

	char	buf[BUFLEN + 1] ;


	ten = 0 ;
	incr = (uint) arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("syscalls/handle_sbrk: incr=%08x\n",incr) ;
#endif

/* return OLD value */

	rrp->sgirc = op->brka ;

/* calculate NEW value */

	if (incr != 0) {

	    newval = op->brka + incr ;
	    rs1 = lsim_setbreak(mip,newval) ;

	    if (rs1 < 0)
	        bprintf(pip->efp,"%s: break value overflow (%d)\n",
	            pip->progname,rs1) ;

	    if (rs1 >= 0) {

/* set NEW value */

	        op->brka = newval ;
	        if (newval > op->brkmax)
	            op->brkmax = newval ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("syscalls/handle_sbrk: new=%08x\n", newval) ;
#endif

	    }
	}

	if (rs1 < 0) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        ten = SGIE_NOTSUP ;
	        rs = rs1 ;

	        bprintf(pip->efp,"%s: handle_sbrk() rs=%d\n",
	            pip->progname,rs) ;

	    } /* end switch */

	} /* end if (errors) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("syscalls/handle_sbrk: brka=%08x sgirc=%08x\n",
		op->brka, rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_sbrk) */


static int handle_stat(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	USTAT	sb ;

	struct sgistat	sgib ;

	uint	ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	mlen, alen, rlen, tlen ;
	int	ten ;

	char	filename[MAXPATHLEN + 1] ;


	ten = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_stat: entered\n") ;
#endif

	if (ten == 0) {

	    if (arp->a1 == 0)
	        ten = SGIE_FAULT ;

	}

	ta = (uint) arp->a2 ;

/* get filename argument */

	if (ten == 0) {

	    rs = syscalls_memreadstr(op,pip,mip,arp->a1,filename,MAXPATHLEN) ;

	    if (rs < 0) {

	        switch (rs) {

	        case SR_FAULT:
	            ten = SGIE_FAULT ;
	            break ;

	        case SR_ACCESS:
	            ten = SGIE_ACCES ;
	            break ;

	        }

	    } else if (rs == 0)
	        ten = SGIE_NOENT ;

	}

	if (ten == 0)
	    rs1 = u_stat(filename,&sb) ;

	if ((rs1 >= 0) && (ten == 0)) {

	    (void) memset(&sgib,0,sizeof(struct sgistat)) ;

/* SGI device number */

		if (op->sgidev != 0)
	    sgib.st_dev = op->sgidev ;

		else
	    sgib.st_dev = sb.st_dev ;

/* everything else */

	    sgib.st_ino = sb.st_ino ;
	    sgib.st_mode = sb.st_mode ;
	    sgib.st_nlink = sb.st_nlink ;
	    sgib.st_uid = sb.st_uid ;
	    sgib.st_gid = sb.st_gid ;
	    sgib.st_rdev = sb.st_rdev ;
	    sgib.st_size = sb.st_size ;

	    sgib.st_atim.tv_sec = sb.st_atim.tv_sec ;
	    sgib.st_atim.tv_nsec = sb.st_atim.tv_nsec ;
	    sgib.st_mtim.tv_sec = sb.st_mtim.tv_sec ;
	    sgib.st_mtim.tv_nsec = sb.st_mtim.tv_nsec ;
	    sgib.st_ctim.tv_sec = sb.st_ctim.tv_sec ;
	    sgib.st_ctim.tv_nsec = sb.st_ctim.tv_nsec ;

	    sgib.st_blksize = SYSCALLS_SGIBLKSIZE ;
	    sgib.st_blocks = sb.st_blocks ;

/* SGI FS typename */

	if (op->sgifstype[0] != '\0')
	    (void) memcpy(sgib.st_fstype,op->sgifstype,SGI_ST_FSTYPSZ) ;

	else
	    (void) memcpy(sgib.st_fstype,sb.st_fstype,SGI_ST_FSTYPSZ) ;

/* write the struct back to the target memory */

	    rs = syscalls_memwrite(op,pip,mip,ta,
	        (char *) &sgib,sizeof(struct sgistat)) ;

	    switch (rs) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_ACCESS:
	        ten = SGIE_ACCES ;
	        break ;

	    } /* end switch */

	}

	rrp->sgirc = ten ;

	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_NOENT:
	        ten = SGIE_NOENT ;
	        break ;

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    } /* end switch */

	} /* end if (errors) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_stat: sgirc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_stat) */


static int handle_getpagesize(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	uint	ps ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	ten ;


	ten = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getpagesize: entered\n") ;
#endif

#ifdef	COMMENT
	rs1 = lsim_getpagesize(mip,&ps) ;

	if (rs1 < 0) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */
#else
	ps = SYSCALLS_PAGESIZE ;
#endif /* COMMENT */

	rrp->sgirc = ps ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getpagesize: ps=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_getpagesize) */


static int handle_getrlimit(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	LSIM_MAXPROG	max ;

	struct sgirlimit	rl ;

	uint	ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	val ;
	int	ten ;


	ten = 0 ;
	ta = arp->a2 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getrlimit: a1=%d a2=%08x\n",
	        arp->a1,arp->a2) ;
#endif

	if (ta == NULL)
	    ten = SGIE_FAULT ;

	(void) memset(&rl,0,sizeof(struct sgirlimit)) ;

	if (ten == 0)
	    rs = lsim_getmax(mip,&max) ;

	if ((rs >= 0) && (ten == 0)) {

	    switch (arp->a1) {

	    case SGIRLIMIT_CPU:
	    case SGIRLIMIT_FSIZE:
	    case SGIRLIMIT_CORE:
	        rl.rlim_cur = SGIRLIM_INFINITY ;
	        rl.rlim_max = SGIRLIM_INFINITY ;
	        break ;

	    case SGIRLIMIT_DATA:
	        rl.rlim_cur = max.data ;
	        rl.rlim_max = max.data ;
	        break ;

	    case SGIRLIMIT_STACK:
	        rl.rlim_cur = max.stack ;
	        rl.rlim_max = max.stack ;
	        break ;

	    case SGIRLIMIT_NOFILE:
	        rl.rlim_cur = SYSCALLS_NFILE ;
	        rl.rlim_max = SYSCALLS_NFILE ;
	        break ;

	    case SGIRLIMIT_VMEM:
	    case SGIRLIMIT_RSS:
	        rl.rlim_cur = SGIRLIM_INFINITY ;
	        rl.rlim_max = SGIRLIM_INFINITY ;
	        break ;

	    default:
	        ten = SGIE_INVAL ;

	    } /* end switch */

	} /* end if */

	if ((rs >= 0) && (ten == 0)) {

	    rs1 = syscalls_memwrite(op,pip,mip,ta,
	        (char *) &rl,sizeof(struct sgirlimit)) ;

	}

	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getrlimit: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_getrlimit) */


static int handle_setrlimit(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	LSIM_MAXPROG	max ;

	struct sgirlimit	rl ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	ten ;


	ten = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_setrlimit: a1=%d\n",arp->a1) ;
#endif




	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_setrlimit: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_setrlimit) */


static int handle_signal(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	LSIM_MAXPROG	max ;

	struct sgirlimit	rl ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	ten ;


	ten = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_setrlimit: entered\n") ;
#endif




	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = (uint) SGISIG_DFL ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_setrlimit: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_signal) */


/* access */
static int handle_access(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	int	rs = SR_BADFMT ;
	int	i, j, n ;
	int	ten, tfdi, tfd ;

	char	filename[MAXPATHLEN + 1] ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_access: entered\n") ;
#endif

	ten = 0 ;
	if (arp->a1 == 0)
	    ten = SGIE_FAULT ;

	if (ten == 0) {

/* get arguments */

	    rs = syscalls_memreadstr(op,pip,mip,arp->a1,filename,MAXPATHLEN) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4) {
	        eprintf("syscalls/handle_access: syscalls_memereadstr() rs=%d\n",rs) ;
	        eprintf("syscalls/handle_access: ta=%08x\n",arp->a1) ;
	        j = 0 ;
	        for (i = 0 ; i < rs ; i += 1) {
	            if (j == 0)
	                eprintf("syscalls/handle_access: data> ") ;
	            eprintf(" %02x",filename[i]) ;
	            if (j == 7)
	                eprintf("\n") ;

	            j = (j + 1) % 8 ;
	        }
	        if (j != 0)
	            eprintf("\n") ;
	    }
#endif /* F_DEBUG */

	    if (rs == 0)
	        ten = SGIE_NOENT ;

	} /* end if */

	if ((rs >= 0) && (ten == 0)) {

	    rs = u_access(filename,arp->a2) ;

#if	F_MASTERDEBUG && F_DEBUG
	    if (pip->debuglevel >= 4)
	        eprintf("syscalls/handle_access: u_access() rs=%d\n",rs) ;
#endif

	    switch (rs) {

	    case SR_NOENT:
	        ten = SGIE_NOENT ;
	        break ;

	    case SR_ACCES:
	        ten = SGIE_ACCES ;
	        break ;

	    case SR_LOOP:
	        ten = SGIE_LOOP ;
	        break ;

	    case SR_NOTDIR:
	        ten = SGIE_NOTDIR ;
	        break ;

	    } /* end switch */

	}

	if ((rs >= 0) && (ten == 0))
	    rrp->sgirc = rs ;

/* return */

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_access) */


static int handle_getrusage(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	LSIM_MAXPROG	max ;

	USTATemips	*smp ;

	struct sgirusage	ru ;

	uint	ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	val ;
	int	ten ;


	smp = op->smp ;

	ten = 0 ;
	ta = arp->a2 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getrusage: a1=%d a2=%08x\n",
	        arp->a1,arp->a2) ;
#endif

	if (ta == NULL)
	    ten = SGIE_FAULT ;

	(void) memset(&ru,0,sizeof(struct sgirusage)) ;

	if (ten == 0)
	    rs = lsim_getmax(mip,&max) ;

	if ((rs >= 0) && (ten == 0)) {

	    switch (arp->a1) {

	    case SGIRUSAGE_SELF:
	        ru.ru_utime.tv_sec = (int) (smp->in / 1000000) ;
	        ru.ru_utime.tv_usec = (int) (smp->in % 1000000) ;

	        ru.ru_stime.tv_sec = (int) 0 ;
	        ru.ru_stime.tv_usec = (int) ((smp->in % 1000000) / 10) ;

	        break ;

	    case SGIRUSAGE_CHILDREN:
	        break ;

	    default:
	        ten = SGIE_INVAL ;

	    } /* end switch */

	} /* end if */

	if ((rs >= 0) && (ten == 0)) {

	    rs1 = syscalls_memwrite(op,pip,mip,ta,
	        (char *) &ru,sizeof(struct sgirusage)) ;

	}

	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_getrusage: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_getrusage) */


static int handle_time(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	USTATemips	*smp ;

	uint	ta ;

	time_t	t ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	val ;
	int	ten ;


	smp = op->smp ;

	ten = 0 ;
	ta = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_time: a1=%08x\n",
	        arp->a1) ;
#endif

	if (ten == 0)
	    rs = u_time(&t) ;

	if ((rs >= 0) && (ten == 0)) {


	} /* end if */

	if ((rs >= 0) && (ten == 0) && (ta != 0)) {

	    val = (int) t ;
	    rs1 = syscalls_memwrite(op,pip,mip,ta,
	        (char *) &val,sizeof(int)) ;

	}

	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = (int) t ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_time: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_time) */


static int handle_times(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	LSIM_MAXPROG	max ;

	USTATemips	*smp ;

	struct sgitms		his ;

	uint	ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	ten ;
	int	clocknow ;


	smp = op->smp ;

	ten = 0 ;
	ta = arp->a1 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_times: a1=%08x\n",
	        arp->a1) ;
#endif

	if (ta == NULL)
	    ten = SGIE_FAULT ;

	(void) memset(&his,0,sizeof(struct sgitms)) ;

	clocknow = smp->in / 1000 ;
	if ((rs >= 0) && (ten == 0)) {

	    his.tms_utime = (int) clocknow ;
	    his.tms_stime = (int) (clocknow / 10) ;

	    his.tms_cutime = 0 ;
	    his.tms_cstime = 0 ;

	} /* end if */

	if ((rs >= 0) && (ten == 0)) {

	    rs1 = syscalls_memwrite(op,pip,mip,ta,
	        (char *) &his,sizeof(struct sgitms)) ;

	}

	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = clocknow ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_times: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_times) */


static int handle_prctl(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	LSIM_MAXPROG	max ;

	struct sgirlimit	rl ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	ten ;


	ten = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("syscalls/handle_prctl: a1=%08x\n",arp->a1) ;
#endif




	if ((rs1 < 0) && (ten == 0)) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    default:
	        rs = rs1 ;

	    } /* end switch */

	} /* end if (errors) */

	rrp->sgirc = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_prctl: rc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

	return rs ;
}
/* end subroutine (handle_prctl) */


static int handle_syssgi(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	uint	ta ;

	int	rs = SR_OK ;
	int	rs1 = 0 ;
	int	i, j, n ;
	int	ten ;


	ten = 0 ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_syssgi: a1=%08x a2=%08x a3=%08x\n",
	        arp->a1,
	        arp->a2,
	        arp->a3) ;
#endif


	rs = SR_NOTSUP ;

	if (rs1 < 0) {

	    switch (rs1) {

	    case SR_FAULT:
	        ten = SGIE_FAULT ;
	        break ;

	    case SR_TOOBIG:
	    case SR_NOMEM:
	        ten = SGIE_NOMEM ;
	        break ;

	    } /* end switch */

	} /* end if (errors) */

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_syssgi: sgirc=%08x\n",rrp->sgirc) ;
#endif

	if (ten > 0)
	    rrp->sgierrno = ten ;

#if	F_MASTERDEBUG && F_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    eprintf("syscalls/handle_syssgi: rs=%d a1=%08x a2=%08x a3=%08x\n",
	        rs, arp->a1, arp->a2, arp->a3) ;
#endif

	return rs ;
}
/* end subroutine (handle_syssgi) */


static int handle_isatty(op,pip,mip,arp,rrp)
SYSCALLS		*op ;
struct proginfo		*pip ;
LSIM			*mip ;
SYSCALLS_ARGREGS	*arp ;
SYSCALLS_RETREG		*rrp ;
{
	uint	ta ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel >= 4)
	    eprintf("syscalls/handle_isatty: a1=%08x \n",
	        arp->a1) ;
#endif

	rrp->sgirc = 0 ;

	return rs ;
}
/* end subroutine (handle_isatty) */



/* read the target program memory (BIG ENDIAN ASSUMED) */
static int syscalls_memread(op,pip,mip,ta,buf,len)
SYSCALLS	*op ;
struct proginfo	*pip ;
LSIM		*mip ;
uint		ta ;
char		buf[] ;
int		len ;
{
	uint	ra, la, dv ;

	int	rs = SR_OK, i, bo, n ;
	int	tlen = 0 ;

	char	*dvp = (char *) &dv ;
	char	*bp = buf ;


	rs = SR_OK ;
	ra = ta ;
	la = ta + len ;
	while (ra < la) {

	    rs = lsim_readint(mip,ra & (~ 3),&dv) ;

	    if (rs < 0)
	        break ;

	    bo = ra & 3 ;
	    n = MIN((4 - bo),(la - ra)) ;
	    (void) memcpy(bp,dvp + bo,n) ;

	    ra += n ;
	    bp += n ;
	    tlen += n ;

	} /* end while */

	return (rs >= 0) ? tlen : rs ;
}
/* end subroutine (syscalls_memread) */


/* read the target program memory (BIG ENDIAN ASSUMED) */
static int syscalls_memreadstr(op,pip,mip,ta,buf,len)
SYSCALLS	*op ;
struct proginfo	*pip ;
LSIM		*mip ;
uint		ta ;
char		buf[] ;
int		len ;
{
	uint	ra, la, dv ;

	int	rs = SR_OK, i, bo, n ;
	int	tlen = 0 ;
	int	mlen ;
	int	f_eos = FALSE ;

	char	*dvp ;
	char	*bp = buf ;


	rs = SR_OK ;
	ra = ta ;
	la = ta + len ;
	while (ra < la) {

	    rs = lsim_readint(mip,ra & (~ 3),&dv) ;

	    if (rs < 0)
	        break ;

	    bo = ra & 3 ;
	    n = MIN((4 - bo),(la - ra)) ;
	    dvp = ((char *) &dv) + bo ;
	    mlen = strnlen(dvp,n) ;

#if	F_DEBUG
	    if (pip->debuglevel >= 3)
	        eprintf("syscalls_memreadstr: n=%d mlen=%d\n",n,mlen) ;
#endif

	    (void) memcpy(bp,dvp,mlen) ;

	    ra += mlen ;
	    bp += mlen ;
	    tlen += mlen ;

	    if (mlen < n)
	        break ;

	} /* end while */

#if	F_DEBUG
	if (pip->debuglevel >= 3)
	    eprintf("syscalls_memreadstr: tlen=%d\n",tlen) ;
#endif

	*bp = '\0' ;
	return (rs >= 0) ? tlen : rs ;
}
/* end subroutine (syscalls_memreadstr) */


/* write the target program memory (BIG ENDIAN ASSUMED) */
static int syscalls_memwrite(op,pip,mip,ta,buf,len)
SYSCALLS	*op ;
struct proginfo	*pip ;
LSIM		*mip ;
uint		ta ;
char		buf[] ;
int		len ;
{
	uint	wa, la, dv ;

	int	rs = SR_OK, i, j, bo, n ;
	int	tlen = 0 ;
	int	dp ;
	int	tn = 0 ;

	char	*dvp = (char *) &dv ;
	char	*bp = buf ;


#if	F_DEBUG
	if (pip->debuglevel >= 5)
	    eprintf("syscalls_memwrite: ta=%08x len=%d\n",ta,len) ;
#endif

	rs = SR_OK ;
	wa = ta ;
	la = ta + len ;
	while (wa < la) {

	    bo = wa & 3 ;
	    n = MIN((4 - bo),(la - wa)) ;
	    (void) memcpy(dvp + bo,bp,n) ;

	    dp = 0 ;
	    i = bo ;
	    j = 0 ;
	    while ((i < 4) && (j < n)) {

	        dp |= (1 << i) ;

	        i += 1 ;
	        j += 1 ;

	    } /* end for */

#if	F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("syscalls_memwrite: lsim_writeint() wa=%08x dv=%08x dp=%d\n",
	            wa,dv,dp) ;
#endif

	    rs = lsim_writeint(mip,wa & (~ 3),dv,dp) ;

#if	F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("syscalls_memwrite: lsim_writeint() rs=%d\n",rs) ;
#endif

	    if (rs < 0)
	        break ;

/* write some trace data */

#if	F_DEBUG
	    if (pip->debuglevel >= 5)
	        eprintf("syscalls_memwrite: mem tracing ?\n") ;
#endif

	    if (traceinfo_fastenabled(&pip->ti) &&
	        traceinfo_mems(&pip->ti)) {

#if	F_DEBUG
	        if (pip->debuglevel >= 5)
	            eprintf("syscalls_memwrite: mem tracing ? YES !\n") ;
#endif

	        rs = exectrace_wmem(&pip->ti.et,wa,dv,dp) ;

	        tn += 1 ;
	        if (tn >= 200) {

	            tn = 0 ;
	            exectrace_wsom(&pip->ti.et,0) ;

	        }

	    } /* end if (writing trace) */

/* finish operation */

	    wa += n ;
	    bp += n ;
	    tlen += n ;

	} /* end while */

	return (rs >= 0) ? tlen : rs ;
}
/* end subroutine (syscalls_memwrite) */



