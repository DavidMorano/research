#ifndef SGI_FCNTL_H__
#define SGI_FCNTL_H__

/* * flags for F_GETFL, F_SETFL */

#define	SGIFNDELAY	0x04	/* Non-blocking I/O */
#define	SGIFAPPEND	0x08	/* append (writes guaranteed at the end) */
#define	SGIFSYNC	0x10	/* synchronous write option for files */
#define	SGIFDSYNC	0x20	/* synchronous write option for data */
#define	SGIFRSYNC	0x40	/* synchronous data integrity read option */
#define	SGIFNONBLOCK	0x80	/* Non-blocking I/O */
#define	SGIFASYNC	0x1000	/* interrupt-driven I/O for sockets */
#define	SGIFLARGEFILE	0x2000	/* open is large file aware */
#define	SGIFNONBLK		FNONBLOCK
#define	SGIFDIRECT		0x8000

/* * open only modes */

#define	SGIFCREAT	0x0100		/* create if nonexistent */
#define	SGIFTRUNC	0x0200		/* truncate to zero length */
#define	SGIFEXCL	0x0400		/* error if already created */
#define	SGIFNOCTTY	0x0800		/* do not make this tty control term */

#ifdef	COMMENT /* assume these are the same as on Solaris */

/*
 * Flag values accessible to open(2) and fcntl(2)
 * (the first three and O_DIRECT can only be set by open).
 */
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_NDELAY	0x04	/* non-blocking I/O */
#define	O_APPEND	0x08	/* append (writes guaranteed at the end) */
#define	O_SYNC		0x10	/* synchronous write option (POSIX) */
#define	O_DSYNC		0x20	/* synchronous write option for data (POSIX)*/
#define	O_RSYNC		0x40	/* synchronous data integrity read (POSIX) */
#define	O_NONBLOCK	0x80	/* non-blocking I/O (POSIX) */
#define	O_LARGEFILE	0x2000	/* allow large file opens */
#define O_DIRECT	0x8000	/* direct I/O */
/*
 * Flag values accessible only to open(2).
 */
#define	O_CREAT		0x100	/* open with file create (uses third open arg) */
#define	O_TRUNC		0x200	/* open with truncation */
#define	O_EXCL		0x400	/* exclusive open */
#define	O_NOCTTY	0x800	/* do not allocate controlling tty (POSIX) */

#endif /* COMMENT */

/* fcntl(2) requests */
#define	SGIF_DUPFD		0	/* Duplicate fildes */
#define	SGIF_GETFD		1	/* Get fildes flags */
#define	SGIF_SETFD		2	/* Set fildes flags */
#define	SGIF_GETFL		3	/* Get file flags */
#define	SGIF_SETFL		4	/* Set file flags */

#define	SGIF_GETLK		14	/* Get file lock */

#define	SGIF_SETLK		6	/* Set file lock */
#define	SGIF_SETLKW	7	/* Set file lock and wait */

#define	SGIF_CHKFL		8	/* Unused */
#define	SGIF_ALLOCSP	10	/* Reserved */
#define	SGIF_FREESP	11	/* Free file space */

#define	SGIF_SETBSDLK	12	/* Set Berkeley record lock */
#define	SGIF_SETBSDLKW	13	/* Set Berkeley record lock and wait */
#define	SGIF_DIOINFO	30	/* get direct I/O parameters */
#define	SGIF_FSGETXATTR	31	/* get extended file attributes (xFS) */
#define	SGIF_FSSETXATTR	32	/* set extended file attributes (xFS) */
#define	SGIF_GETLK64	33	/* Get 64 bit file lock */
#define	SGIF_SETLK64	34	/* Set 64 bit file lock */
#define	SGIF_SETLKW64	35	/* Set 64 bit file lock and wait */
#define	SGIF_ALLOCSP64	36	/* Alloc 64 bit file space */
#define	SGIF_FREESP64	37	/* Free 64 bit file space */
#define	SGIF_GETBMAP	38	/* Get block map (64 bit only) */
#define	SGIF_FSSETDM	39	/* Set DMI event mask and state (XFS only) */
#define SGIF_RESVSP	40	/* Reserve file space. Allocate space for the 
				 * file without zeroing file data or changing 
				 * file size */
#define SGIF_UNRESVSP	41	/* Remove file space. */
#define SGIF_RESVSP64	42	/* Reserv 64 bit file space */
#define SGIF_UNRESVSP64	43	/* Remove 64 bit file space */
#define	SGIF_GETBMAPA	44	/* Get block map for attributes (64 bit only) */
#define	SGIF_FSGETXATTRA	45	/* get extended file attributes (xFS-A) */

#define SGIF_SETBW         46      /* set bandwidth allocation */
#define SGIF_GETBW         47      /* get bandwidth allocation */
#define SGIF_GETOPS        50      /* get operation table */

#define SGIF_RSETLK	20	/* Remote SETLK for NFS */
#define SGIF_RGETLK	21	/* Remote GETLK for NFS */
#define SGIF_RSETLKW	22	/* Remote SETLKW for NFS */

/* only for sockets */
#define	SGIF_GETOWN	23	/* Get owner (socket emulation) */
#define	SGIF_SETOWN	24	/* Set owner (socket emulation) */


/* File segment locking set data type - information passed to system by user */

typedef struct sgiflock {
	short	l_type;
	short	l_whence;
	int	l_start;
	int	l_len;		/* len == 0 means until end of file */
        int	l_sysid;
        pid_t	l_pid;
	int	l_pad[4];		/* reserve area */
} sgiflock_t;


/* * File segment locking types.  */
#define	SGIF_RDLCK	01	/* Read lock */
#define	SGIF_WRLCK	02	/* Write lock */
#define	SGIF_UNLCK	03	/* Remove lock(s) */

/* * POSIX constants */

#define	SGIFD_CLOEXEC	1	/* close on exec flag */


#endif /* !__FCNTL_H__ */
