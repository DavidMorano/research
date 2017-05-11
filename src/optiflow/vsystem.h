/* vsystem */

/* last modified %G% version %I% */


/* revision history :

	= 83/10/01, Dave Morano

	This module was originally written.


	= 95/03/21, Dave Morano

	I put the <pthread.h> include file in here.  I am not sure why
	now ! :-)  This may be needed for certain cases when POSIX
	threads need to be used instead of Solaris threads.  We pretty
	much only ever use POSIX threads.  Solaris native threads and
	LWPs have some problems (not bugs but screw-ups) !


*/


#ifndef	VSYSTEM_INCLUDE
#define	VSYSTEM_INCLUDE	1


#include	"syshas.h"		/* has what ? */

#include	<sys/types.h>
#include	<sys/utsname.h>
#include	<sys/resource.h>
#include	<sys/stat.h>
#include	<sys/statvfs.h>
#include	<sys/socket.h>
#include	<sys/uio.h>
#include	<sys/poll.h>
#include	<sys/time.h>
#include	<ucontext.h>
#include	<signal.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<pthread.h>
#include	<signal.h>
#include	<netdb.h>
#include	<errno.h>

#ifndef	PWD_INCLUDE
#define	PWD_INCLUDE	1
#include	<pwd.h>
#endif

#ifndef	GRP_INCLUDE
#define	GRP_INCLUDE	1
#include	<grp.h>
#endif

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
#ifndef	SHADOW_INCLUDE
#define	SHADOW_INCLUDE	1
#include	<shadow.h>
#endif /* SHADOW_INCLUDE */
#endif /* SYSHAS_SHADOW */



/* UNIX system defines */

#ifndef	NODENAMELEN
#if	defined(SYS_NMLN)
#define	NODENAMELEN	SYS_NMLN	/* must be at least 257 for SVR4 ! */
#else
#define	NODENAMELEN	512		/* must be at least 257 for SVR4 ! */
#endif
#endif

#ifndef	MAXNODENAMELEN
#define	MAXNODENAMELEN	NODENAMELEN
#endif

#ifndef	MAXUSERNAMELEN
#define	MAXUSERNAMELEN	32
#endif

#ifndef	USERNAMELEN
#ifdef	MAXUSERNAMELEN
#define	USERNAMELEN	MAXUSERNAMELEN
#else
#define	USERNAMELEN	32
#endif
#endif

#ifndef	LOGNAMELEN
#define	LOGNAMELEN	32
#endif

#ifndef	PASSWORDLEN
#define	PASSWORDLEN	8
#endif


/* the long integer problem ! */

#ifndef	LONG
#if	defined(IRIX) || defined(SOLARIS) || defined(LONGLONG)

#define	LONG	long long

#else

#define	LONG	long

#endif /* SOLARIS */
#endif


#ifndef	ULONG
#define	ULONG	unsigned LONG
#endif


/* some limits !! */

#ifndef	LONG64_MIN
#define	LONG64_MIN	(-9223372036854775807L-1LL)
#endif

#ifndef	LONG64_MAX
#define	LONG64_MAX	9223372036854775807LL
#endif

#ifndef	ULONG64_MAX
#define	ULONG64_MAX	18446744073709551615ULL
#endif




/* extra system flags for 'lockfile(3d)' */

#define	F_RLOCK		10		/* new ! (watch UNIX for changes) */
#define	F_TRLOCK	11		/* new ! (watch UNIX for changes) */
#define	F_RTEST		12		/* new ! (watch UNIX for changes) */
#define	F_WLOCK		F_LOCK
#define	F_TWLOCK	F_TLOCK
#define	F_WTEST		F_TEST
#define	F_UNLOCK	F_ULOCK



/* status return codes (only used when explicit return status is requested */

#define	SR_CREATED	3		/* created object */
#define	SR_TIMEOUT	2		/* operation timed out */

/* operation returns */

#define	SR_OK		0		/* the OK return */

#define	SR_PERM		(- EPERM)	/* Not super-user	*/ 
#define	SR_NOENT	(- ENOENT)	/* No such file or directory */
#define	SR_SRCH		(- ESRCH)	/* No such process */
#define	SR_INTR		(- EINTR)	/* interrupted system call */
#define	SR_IO		(- EIO)		/* I/O error */
#define	SR_NXIO		(- ENXIO)	/* No such device or address */
#define	SR_2BIG		(- E2BIG)	/* Arg list too long */
#define	SR_NOEXEC	(- ENOEXEC)	/* Exec format error */
#define	SR_BADF		(- EBADF)	/* Bad file number */
#define	SR_CHILD	(- ECHILD)	/* No children */
#define	SR_AGAIN	(- EAGAIN)	/* Resource temporarily unavailable */
#define	SR_NOMEM	(- ENOMEM)	/* Not enough core */
#define	SR_ACCES	(- EACCES)	/* Permission denied */
#define	SR_FAULT	(- EFAULT)	/* Bad address */
#define	SR_NOTBLK	(- ENOTBLK)	/* Block device required */
#define	SR_BUSY		(- EBUSY)	/* Mount device busy */
#define	SR_EXIST	(- EEXIST)	/* File exists */
#define	SR_XDEV		(- EXDEV)	/* Cross-device link */
#define	SR_NODEV	(- ENODEV)	/* No such device */
#define	SR_NOTDIR	(- ENOTDIR)	/* Not a directory */
#define	SR_ISDIR	(- EISDIR)	/* Is a directory */
#define	SR_INVAL	(- EINVAL)	/* Invalid argument */
#define	SR_NFILE	(- ENFILE)	/* File table overflow */
#define	SR_MFILE	(- EMFILE)	/* Too many open files */
#define	SR_NOTTY	(- ENOTTY)	/* Inappropriate ioctl for device */
#define	SR_TXTBSY	(- ETXTBSY)	/* Text file busy */
#define	SR_FBIG		(- EFBIG)	/* File too large */
#define	SR_NOSPC	(- ENOSPC)	/* No space left on device */
#define	SR_SPIPE	(- ESPIPE)	/* Illegal seek */
#define	SR_ROFS		(- EROFS)	/* Read only file system */
#define	SR_MLINK	(- EMLINK)	/* Too many links */
#define	SR_PIPE		(- EPIPE)	/* Broken pipe */
#define	SR_DOM		(- EDOM)	/* Math arg out of domain of func */
#define	SR_RANGE	(- ERANGE)	/* Math result not representable */
#define	SR_NOMSG	(- ENOMSG)	/* No message of desired type */
#define	SR_IDRM		(- EIDRM)	/* Identifier removed */
#define	SR_CHRNG	(- ECHRNG)	/* Channel number out of range */
#define	SR_L2NSYNC	(- EL2NSYNC)	/* Level 2 not synchronized */
#define	SR_L3HLT	(- EL3HLT)	/* Level 3 halted */
#define	SR_L3RST	(- EL3RST)	/* Level 3 reset */
#define	SR_LNRNG	(- ELNRNG)	/* Link number out of range */
#define	SR_UNATCH	(- EUNATCH)	/* Protocol driver not attached */
#define	SR_NOCSI	(- ENOCSI)	/* No CSI structure available */
#define	SR_L2HLT	(- EL2HLT)	/* Level 2 halted */
#define	SR_DEADLK	(- EDEADLK)	/* Deadlock condition */
#define	SR_NOLCK	(- ENOLCK)	/* No record locks available */
#define	SR_CANCELED	(- ECANCELED)	/* Operation canceled */
#define	SR_NOTSUP	(- ENOTSUP)	/* Operation not supported */
#define	SR_DQUOT	(- EDQUOT)	/* Disc quota exceeded */
#define	SR_BADE		(- EBADE)	/* invalid exchange */
#define	SR_BADR		(- EBADR)	/* invalid request descriptor */
#define	SR_XFULL	(- EXFULL)	/* exchange full */
#define	SR_NOANO	(- ENOANO)	/* no anode */
#define	SR_BADRQC	(- EBADRQC)	/* invalid request code */
#ifdef	EBADSLT
#define	SR_BADSLT	(- EBADSLT)	/* invalid slot */
#else
#define	SR_BADSLT	(- ENOENT)	/* invalid slot */
#endif /* EBADSLT */
#define	SR_DEADLOCK	(- EDEADLOCK)	/* file locking deadlock error */
#define	SR_BFONT	(- EBFONT)	/* bad font file fmt */
#define	SR_NOSTR	(- ENOSTR)	/* Device not a stream */
#define	SR_NODATA	(- ENODATA)	/* no data (for no delay io) */
#define	SR_TIME		(- ETIME)	/* timer expired */
#define	SR_NOSR		(- ENOSR)	/* out of streams resources */
#define	SR_NONET	(- ENONET)	/* Machine is not on the network */
#define	SR_NOPKG	(- ENOPKG)	/* Package not installed */
#define	SR_REMOTE	(- EREMOTE)	/* The object is remote */
#define	SR_NOLINK	(- ENOLINK)	/* the link has been severed */
#define	SR_ADV		(- EADV)	/* advertise error */
#define	SR_SRMNT	(- ESRMNT)	/* srmount error */
#define	SR_COMM		(- ECOMM)	/* Communication error on send */
#define	SR_PROTO	(- EPROTO)	/* Protocol error */
#define	SR_MULTIHOP	(- EMULTIHOP)	/* multihop attempted */
#define	SR_BADMSG	(- EBADMSG)	/* trying to read unreadable message */
#define	SR_NAMETOOLONG	(- ENAMETOOLONG)	/* path name is too long */
#define	SR_OVERFLOW	(- EOVERFLOW)	/* value too large to be stored */
#define	SR_NOTUNIQ	(- ENOTUNIQ)	/* given login name not unique */
#define	SR_BADFD	(- EBADFD)	/* FD invalid for this operation */
#define	SR_REMCHG	(- EREMCHG)	/* Remote address changed */
#define	SR_LIBACC	(- ELIBACC)	/* Can't access a needed shared lib */
#define	SR_LIBBAD	(- ELIBBAD)	/* Accessing a corrupted shared lib */
#define	SR_LIBSCN	(- ELIBSCN)	/* .lib section in a.out corrupted */
#define	SR_LIBMAX	(- ELIBMAX)	/* Attempting link in too many libs */
#define	SR_LIBEXEC	(- ELIBEXEC)	/* Attempting to exec shared library */
#define	SR_ILSEQ	(- EILSEQ)	/* Illegal byte sequence */
#define	SR_NOSYS	(- ENOSYS)	/* Unsupported file system operation */
#define	SR_LOOP		(- ELOOP)	/* Symbolic link loop */
#define	SR_RESTART	(- ERESTART)	/* Restartable system call */
#define	SR_STRPIPE	(- ESTRPIPE)	/* if pipe/FIFO, don't sleep */
#define	SR_NOTEMPTY	(- ENOTEMPTY)	/* directory not empty */
#define	SR_USERS	(- EUSERS)	/* Too many users (for UFS) */
#define	SR_NOTSOCK	(- ENOTSOCK)	/* Socket operation on non-socket */
#define	SR_DESTADDRREQ	(- EDESTADDRREQ)	/* dst address required */
#define	SR_MSGSIZE	(- EMSGSIZE)	/* Message too long */
#define	SR_PROTOTYPE	(- EPROTOTYPE)	/* Protocol wrong type for socket */
#define	SR_NOPROTOOPT	(- ENOPROTOOPT)	/* Protocol not available */
#define	SR_PROTONOSUPPORT	(- EPROTONOSUPPORT)	/* proto not sup. */
#define	SR_SOCKTNOSUPPORT	(- ESOCKTNOSUPPORT) /* Socket not supported */
#define	SR_OPNOTSUPP	(- EOPNOTSUPP)	/* Operation not supported on socket */
#define	SR_PFNOSUPPORT	(- EPFNOSUPPORT)	/* proto family not supported */
#define	SR_AFNOSUPPORT	(- EAFNOSUPPORT)	/* AF not supported by */
#define	SR_ADDRINUSE	(- EADDRINUSE)	/* Address already in use */
#define	SR_ADDRNOTAVAIL	(- EADDRNOTAVAIL)	/* Can't assign address */
#define	SR_NETDOWN	(- ENETDOWN)	/* Network is down */
#define	SR_NETUNREACH	(- ENETUNREACH)	/* Network is unreachable */
#define	SR_NETRESET	(- ENETRESET)	/* Network dropped connection reset */
#define	SR_CONNABORTED	(- ECONNABORTED)	/* connection abort */
#define	SR_CONNRESET	(- ECONNRESET)	/* Connection reset by peer */
#define	SR_NOBUFS	(- ENOBUFS)	/* No buffer space available */
#define	SR_ISCONN	(- EISCONN)	/* Socket is already connected */
#define	SR_NOTCONN	(- ENOTCONN)	/* Socket is not connected */
#define	SR_SHUTDOWN	(- ESHUTDOWN)	/* Can't send after socket shutdown */
#define	SR_TOOMANYREFS	(- ETOOMANYREFS)	/* Too many : can't splice */
#define	SR_TIMEDOUT	(- ETIMEDOUT)	/* Connection timed out */
#define	SR_CONNREFUSED	(- ECONNREFUSED)	/* Connection refused */
#define	SR_HOSTDOWN	(- EHOSTDOWN)	/* Host is down */
#define	SR_HOSTUNREACH	(- EHOSTUNREACH)	/* No route to host */
#define	SR_WOULDBLOCK	(- EAGAIN)	/* UNIX synonym for AGAIN */
#define	SR_ALREADY	(- EALREADY)	/* operation already in progress */
#define	SR_INPROGRESS	(- EINPROGRESS)	/* operation now in progress */
#define	SR_STALE	(- ESTALE)	/* Stale NFS file handle */

#define	SR_BAD		-1000		/* general bad */



/* our favorite aliases */

#define	SR_NOENTRY	SR_NOENT	/* no entry */
#define	SR_NOTOPEN	SR_BADF		/* not open */
#define	SR_WRONLY	SR_BADF		/* not readable (historical) */
#define	SR_RDONLY	SR_BADF		/* not writable (historical) */
#define	SR_NOTSEEK	SR_SPIPE	/* not seekable */
#define	SR_ACCESS	SR_ACCES	/* permission denied */
#define	SR_INVALID	SR_INVAL	/* invalid argument */
#define	SR_EXISTS	SR_EXIST	/* object already exists */
#define	SR_LOCKED	SR_AGAIN	/* object is already locked */
#define	SR_INUSE	SR_ADDRINUSE	/* already in use */
#define	SR_LOCKLOST	SR_NXIO		/* a lock was lost */
#define	SR_HANGUP	SR_NXIO		/* hangup on device (not writable) */
#define	SR_TOOBIG	SR_2BIG		/* arguments too big */
#define	SR_BADFMT	SR_BFONT	/* invalid format */
#define	SR_FULL		SR_XFULL	/* object is full */
#define	SR_EMPTY	SR_NOENT	/* object is empty */
#define	SR_EOF		SR_NOENT	/* end-of-file */
#define	SR_NOEXIST	SR_NOENT	/* object does not exist */
#define	SR_NOTFOUND	SR_NOENT	/* not found */
#define	SR_BADREQUEST	SR_BADRQC	/* bad request code */
#define	SR_NOTCONNECTED	SR_NOTCONN	/* Socket is not connected */
#define	SR_OPEN		SR_ALREADY	/* already open */
#define	SR_OUT		SR_INPROGRESS	/* operation in progress */
#define	SR_NOTAVAIL	SR_ADDRNOTAVAIL	/* not available */
#define	SR_BADSLOT	SR_BADSLT	/* invalid slot */
#define	SR_SEARCH	SR_SRCH		/* No such process */
#define	SR_NOANODE	SR_NOANO	/* no anode ! */



/* types */

#ifndef	TYPEDEF_LONG64
#define	TYPEDEF_LONG64	1

typedef LONG	long64 ;

#endif /* TYPEDEF_LONG64 */


#ifndef	TYPEDEF_ULONG64
#define	TYPEDEF_ULONG64	1

typedef ULONG	ulong64 ;

#endif /* TYPEDEF_ULONG64 */


/* our favorite structures */

#define	VSESEM		struct vsesem
#define	CSCSB		struct vscsb



/* completion status block */

struct vsesem {
	int	magic ;
	int	esem ;
} ;

struct vscsb {
	int	cs ;
	int	len ;
} ;



#define		fc_mask		0x000F
#define		fm_mask		0xFFF0

#define		FC_MASK		0x000F
#define		FM_MASK		0xFFF0


/* function codes for issuing commands */

#define		fc_all		0
#define		fc_read		1
#define		fc_write	2
#define		fc_control	3
#define		fc_status	4
#define		fc_cancel	5
#define		fc_open		6
#define		fc_close	7

#define		FC_ALL		0
#define		FC_READ		1
#define		FC_WRITE	2
#define		FC_CONTROL	3
#define		FC_STATUS	4
#define		FC_CANCEL	5
#define		FC_OPEN		6
#define		FC_CLOSE	7


/* read modifiers */

#define		fm_echo		0x0000
#define		fm_filter	0x0000
#define		fm_noecho	0x0010		/* no echo on input */
#define		fm_notecho	0x0020		/* no terminator echo */
#define		fm_nofilter	0x0040		/* no filter on input */
#define		fm_rawin	0x0080		/* raw input (no control) */
#define		fm_noblock	0x0100		/* don't start if no data */
#define		fm_timed	0x0200		/* timed read */
#define		fm_exact	0x0400		/* exact transfer length */
#define		fm_again	0x0800		/* return AGAIN if no data */

#define		FM_NONE		0x0000		/* no options specified */
#define		FM_ECHO		0x0000
#define		FM_FILTER	0x0000
#define		FM_NOECHO	0x0010		/* no echo on input */
#define		FM_NOTECHO	0x0020		/* no terminator echo */
#define		FM_NOFILTER	0x0040		/* no filter on input */
#define		FM_RAWIN	0x0080		/* raw input (no control) */
#define		FM_NOBLOCK	0x0100		/* don't start if no data */
#define		FM_TIMED	0x0200		/* timed read */
#define		FM_EXACT	0x0400		/* exact transfer length */
#define		FM_AGAIN	0x0800		/* return AGAIN if no data */


/* write modifiers */

#define		fm_cco		0x1000		/* cancel output cancel */
#define		fm_rawout	0x2000		/* raw output */

#define		FM_CCO		0x1000		/* cancel output cancel */
#define		FM_RAWOUT	0x2000		/* raw output */


/* control modifiers */

#define		fm_setmode	0x0010		/* set terminal mode */
#define		fm_getmode	0x0020		/* retrieve current mode */
#define		fm_int		0x0030		/* set interrupt attention */
#define		fm_kill		0x0040		/* set control Y attention */
#define		fm_reestablish	0x0080
#define		fm_readatt	0x0100
#define		fm_writeatt	0x0200

#define		FM_SETMODE	0x0010		/* set terminal mode */
#define		FM_GETMODE	0x0020		/* retrieve current mode */
#define		FM_INT		0x0030		/* set interrupt attention */
#define		FM_KILL		0x0040		/* set control Y attention */
#define		FM_REESTABLISH	0x0080
#define		FM_READATT	0x0100
#define		FM_WRITEATT	0x0200


/* status modifiers */


/* cancel modifiers */

#define		fm_all		0x0010		/* cancel all requests w/ FC */

#define		FM_ALL		0x0010		/* cancel all requests w/ FC */



/* external subroutines */

#if	(! defined(LIBU_MASTER)) || (LIBU_MASTER == 0)

extern int	u_uname(struct utsname *) ;
extern int	u_time(time_t *) ;
extern int	u_unixtime(LONG *) ;
extern int	u_getloadavg(unsigned int *,int) ;

extern int	u_getpgid(pid_t) ;
extern int	u_getpid() ;
extern int	u_getppid() ;
extern int	u_getpgrp() ;
extern int	u_getsid() ;
extern int	u_getuid() ;
extern int	u_geteuid() ;
extern int	u_getgid() ;
extern int	u_getegid() ;
extern int	u_setuid(uid_t) ;
extern int	u_seteuid(uid_t) ;
extern int	u_setreuid(uid_t,uid_t) ;
extern int	u_setgid(gid_t) ;
extern int	u_setegid(gid_t) ;
extern int	u_setregid(gid_t,gid_t) ;
extern int	u_getgroups(int,gid_t *) ;
extern int	u_setgroups(int,const gid_t *) ;

extern int	u_sigaction(int,struct sigaction *,struct sigaction *) ;
extern int	u_fork() ;
extern int	u_vfork() ;
extern int	u_getcontext(ucontext_t *) ;
extern int	u_setcontext(const ucontext_t *) ;
extern int	u_execve(const char *, char *const *, char *const *) ;
extern int	u_execv(const char *,char *const *) ;
extern int	u_execvp(const char *,char *const *) ;
extern int	u_exit(int) ;
extern int	u_kill(pid_t,int) ;
extern int	u_waitpid(pid_t,int *,int) ;
extern int	u_getrlimit(int,struct rlimit *) ;
extern int	u_setrlimit(int,const struct rlimit *) ;

extern int	u_creat(const char *,int) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	u_creat64(const char *,int) ;
#endif

extern int	u_open(const char *,int,int) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	u_open64(const char *,int,int) ;
#endif

extern int	u_dup(int) ;
extern int	u_dup2(int,int) ;
extern int	u_pipe(int *) ;
extern int	u_socket(int,int,int) ;
extern int	u_socketpair(int,int,int,int *) ;
extern int	u_accept(int,void *,int *) ;
extern int	u_stat(const char *,struct stat *) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	u_stat64(const char *,struct stat64 *) ;
#endif

extern int	u_lstat(const char *,struct stat *) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	u_lstat64(const char *,struct stat64 *) ;
#endif

extern int	u_poll(struct pollfd *,int,int) ;
extern int	u_read(int,void *,int) ;
extern int	u_write(int,const void *,int) ;
extern int	u_fstat(int,struct stat *) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	u_fstat64(int,struct stat64 *) ;
#endif

extern int	u_fcntl() ;
extern int	u_fchmod(int,int) ;
extern int	u_seek(int,off_t,int) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	u_seek64(int,off64_t,int) ;
#endif

extern int	u_oseek(int,off_t,int,off_t *) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	u_oseek64(int,off64_t,int,off64_t *) ;
#endif

extern int	u_bind(int,void *,int) ;
extern int	u_getsockname(int,void *,int *) ;
extern int	u_getpeername(int,void *,int *) ;
extern int	u_listen(int,int) ;
extern int	u_send(int,const void *,int,int) ;
extern int	u_sendto(int,const void *,int,int,void *,int) ;
extern int	u_sendmsg(int,struct msghdr *,int) ;
extern int	u_recv(int,void *,int,int) ;
extern int	u_recvfrom(int,void *,int,int,void *,int *) ;
extern int	u_recvmsg(int,struct msghdr *,int) ;
extern int	u_close(int) ;

extern int	u_access(const char *,int) ;
extern int	u_statvfs(const char *,struct statvfs *) ;
extern int	u_fstatvfs(int,struct statvfs *) ;
extern int	u_unlink(const char *) ;
extern int	u_rmdir(const char *) ;
extern int	u_chown(const char *,uid_t,gid_t) ;
extern int	u_lchown(const char *,uid_t,gid_t) ;
extern int	u_fchown(int,uid_t,gid_t) ;

extern int	u_mapfile(caddr_t,size_t,int,int,int,off_t,caddr_t *) ;
extern int	u_munmap(caddr_t,size_t) ;

#endif /* LIBU_MASTER */


#if	(! defined(LIBUC_MASTER)) || (LIBUC_MASTER == 0)

extern int	uc_sysconf(int,long *) ;
extern int	uc_swapcontext(ucontext_t *,const ucontext_t *) ;
extern int	uc_usaexec(const char *,const char **,const char **) ;

extern int	uc_open(const char *,int,int) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	uc_open64(const char *,int,int) ;
#endif

extern int	uc_opensocket(const char *,int,int) ;
extern int	uc_openproto(const char *,int,int) ;
extern int	uc_truncate(const char *,off_t) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	uc_truncate64(const char *,off64_t) ;
#endif

extern int	uc_accepte(int,void *,int *,int) ;

extern int	uc_copy(int,int,int) ;
extern int	uc_copyfile(int,int) ;
extern int	uc_writen(int,const char *,int) ;
extern int	uc_ftruncate(int,off_t) ;

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
extern int	uc_ftruncate64(int,off64_t) ;
#endif

extern int	uc_reade(int,char *,int,int,int) ;
extern int	uc_recve(int,char *,int,int,int,int) ;
extern int	uc_recvfrome(int,char *,int,int,void *,int *,int,int) ;
extern int	uc_recvmsge(int,struct msghdr *,int,int,int) ;

extern int	uc_remove(const char *) ;

extern int	uc_gettimeofday(struct timeval *,void *) ;

extern int	uc_getprotobyname(const char *,struct protoent *,char *,int) ;
extern int	uc_getprotobynumber(int,struct protoent *,char *,int) ;
extern int	uc_gethostbyname(const char *,struct hostent *,char *,int) ;
extern int	uc_gethostbyaddr(void *,int,int,struct hostent *,char *,int) ;
extern int	uc_getservbyname(const char *,const char *,
			struct servent *,char *,int) ;

extern int	uc_valloc(int,void *) ;
extern int	uc_malloc(int,void *) ;
extern int	uc_realloc(void *,int,void *) ;
extern int	uc_mallocstrn(const char *,int,char **) ;

extern int	uc_moveup(int,int) ;

extern int	uc_sigemptyset(sigset_t *) ;
extern int	uc_sigfillset(sigset_t *) ;
extern int	uc_sigaddset(sigset_t *,int) ;
extern int	uc_sigdelset(sigset_t *,int) ;
extern int	uc_sigismember(sigset_t *,int) ;

extern int	uc_mktime(struct tm *,time_t *) ;
extern int	uc_gmtime(time_t *,const struct tm *) ;
extern int	uc_localtime(time_t *,const struct tm *) ;

#if	defined(PWD_INCLUDE) && (PWD_INCLUDE != 0)
extern int	uc_getpwnam(const char *,struct passwd *,char *,int) ;
extern int	uc_getpwuid(uid_t,struct passwd *,char *,int) ;
extern int	uc_getpwent(struct passwd *,char *,int) ;
#endif /* PWD_INCLUDE */

#if	defined(GRP_INCLUDE) && (GRP_INCLUDE != 0)
extern int	uc_getgrnam(const char *,struct group *,char *,int) ;
extern int	uc_getgruid(int,struct group *,char *,int) ;
#endif /* GRP_INCLUDE */

#if	defined(SYSHAS_SHADOW) && (SYSHAS_SHADOW != 0)
#if	defined(SHADOW_INCLUDE) && (SHADOW_INCLUDE != 0)
extern int	uc_getspnam(const char *,struct spwd *,char *,int) ;
#endif /* SHADOW_INCLUDE */
#endif /* SYSHAS_SHADOW */

#endif /* LIBUC_MASTER */


#if	(! defined(LIBUT_MASTER)) || (LIBUT_MASTER == 0)

extern int	ut_alloc(int,int,int,void **) ;
extern int	ut_free(void *,int) ;

#endif /* LIBUT_MASTER */


#if	(! defined(VSYSTEM_MASTER)) || (VSYSTEM_MASTER == 0)

extern int	vs_intenable(sigset_t *) ;
extern int	vs_intdisable(sigset_t *) ;

#endif /* (! defined(VSYSTEM_MASTER)) || (VSYSTEM_MASTER == 0) */


#endif /* VSYSTEM_INCLUDE */



