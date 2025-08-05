/* SGI errno */


/* revision history :

	= 01/10/19, Dave Morano

	Names changed to protect the innocent.
	This is odified for use by LevoSim and SimpleSim.


*/


/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef SGIERRNO_H	/* wrapper symbol for kernel use */
#define SGIERRNO_H	/* subject to change without notice */


/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */


/*
 * Error codes
 */

#define	SGIE_PERM	1	/* Operation not permitted		*/
#define	SGIE_NOENT	2	/* No such file or directory		*/
#define	SGIE_SRCH	3	/* No such process			*/
#define	SGIE_INTR	4	/* Interrupted function call		*/
#define	SGIE_IO	5	/* I/O error				*/
#define	SGIE_NXIO	6	/* No such device or address		*/
#define	SGIE_2BIG	7	/* Arg list too long			*/
#define	SGIE_NOEXEC	8	/* Exec format error			*/
#define	SGIE_BADF	9	/* Bad file number			*/
#define	SGIE_CHILD	10	/* No child processes			*/
#define	SGIE_AGAIN	11	/* Resource temporarily unavailable	*/
#define	SGIE_NOMEM	12	/* Not enough space			*/
#define	SGIE_ACCES	13	/* Permission denied			*/
#define	SGIE_FAULT	14	/* Bad address				*/
#define	SGIE_NOTBLK	15	/* Block device required		*/
#define	SGIE_BUSY	16	/* Resource busy			*/
#define	SGIE_EXIST	17	/* File exists				*/
#define	SGIE_XDEV	18	/* Improper link			*/
#define	SGIE_NODEV	19	/* No such device			*/
#define	SGIE_NOTDIR	20	/* Not a directory			*/
#define	SGIE_ISDIR	21	/* Is a directory			*/
#define	SGIE_INVAL	22	/* Invalid argument			*/
#define	SGIE_NFILE	23	/* File table overflow			*/
#define	SGIE_MFILE	24	/* Too many open files			*/
#define	SGIE_NOTTY	25	/* Inappropriate I/O control operation	*/
#define	SGIE_TXTBSY	26	/* Text file busy			*/
#define	SGIE_FBIG	27	/* File too large			*/
#define	SGIE_NOSPC	28	/* No space left on device		*/
#define	SGIE_SPIPE	29	/* Illegal seek				*/
#define	SGIE_ROFS	30	/* Read only file system		*/
#define	SGIE_MLINK	31	/* Too many links			*/
#define	SGIE_PIPE	32	/* Broken pipe				*/
#define	SGIE_DOM	33	/* Domain error				*/
#define	SGIE_RANGE	34	/* Result too large			*/
#define	SGIE_NOMSG	35	/* No message of desired type		*/
#define	SGIE_IDRM	36	/* Identifier removed			*/
#define	SGIE_CHRNG	37	/* Channel number out of range		*/
#define	SGIE_L2NSYNC 38	/* Level 2 not synchronized		*/
#define	SGIE_L3HLT	39	/* Level 3 halted			*/
#define	SGIE_L3RST	40	/* Level 3 reset			*/
#define	SGIE_LNRNG	41	/* Link number out of range		*/
#define	SGIE_UNATCH 42	/* Protocol driver not attached		*/
#define	SGIE_NOCSI	43	/* No CSI structure available		*/
#define	SGIE_L2HLT	44	/* Level 2 halted			*/
#define	SGIE_DEADLK	45	/* Resource deadlock avoided		*/
#define	SGIE_NOLCK	46	/* No locks available			*/
#define	SGIE_CKPT	47	/* POSIX checkpoint/restart error	*/

/* Convergent Error Returns */
#define	SGIE_BADE	50	/* invalid exchange			*/
#define	SGIE_BADR	51	/* invalid request descriptor		*/
#define	SGIE_XFULL	52	/* exchange full			*/
#define	SGIE_NOANO	53	/* no anode				*/
#define	SGIE_BADRQC	54	/* invalid request code			*/
#define	SGIE_BADSLT	55	/* invalid slot				*/
#define	SGIE_DEADLOCK 56	/* file locking deadlock error		*/

#define	SGIE_BFONT	57	/* bad font file fmt			*/

/* stream problems */
#define	SGIE_NOSTR	60	/* Device not a stream			*/
#define	SGIE_NODATA	61	/* no data (for no delay io)		*/
#define	SGIE_TIME	62	/* timer expired			*/
#define	SGIE_NOSR	63	/* out of streams resources		*/

#define	SGIE_NONET	64	/* Machine is not on the network	*/
#define	SGIE_NOPKG	65	/* Package not installed                */
#define	SGIE_REMOTE	66	/* The object is remote			*/
#define	SGIE_NOLINK	67	/* the link has been severed */
#define	SGIE_ADV	68	/* advertise error */
#define	SGIE_SRMNT	69	/* srmount error */

#define	SGIE_COMM	70	/* Communication error on send		*/
#define	SGIE_PROTO	71	/* Protocol error			*/
#define	SGIE_MULTIHOP 74	/* multihop attempted */
#define	SGIE_BADMSG 77	/* Bad message				*/
#define	SGIE_NAMETOOLONG 78	/* Filename too long */
#define	SGIE_OVERFLOW 79	/* value too large to be stored in data type */
#define	SGIE_NOTUNIQ 80	/* given log. name not unique */
#define	SGIE_BADFD	 81	/* f.d. invalid for this operation */
#define	SGIE_REMCHG	 82	/* Remote address changed */

/* shared library problems */
#define	SGIE_LIBACC	83	/* Can't access a needed shared lib.	*/
#define	SGIE_LIBBAD	84	/* Accessing a corrupted shared lib.	*/
#define	SGIE_LIBSCN	85	/* .lib section in a.out corrupted.	*/
#define	SGIE_LIBMAX	86	/* Attempting to link in too many libs.	*/
#define	SGIE_LIBEXEC 87	/* Attempting to exec a shared library.	*/
#define	SGIE_ILSEQ	88	/* Illegal byte sequence. */
#define	SGIE_NOSYS	89	/* Function not implemented		*/
#define	SGIE_LOOP	90	/* Symbolic link loop */
#define	SGIE_RESTART 91	/* Restartable system call */
#define	SGIE_STRPIPE 92	/* if pipe/FIFO, do not sleep in stream head */

#define	SGIE_NOTEMPTY 93	/* Directory not empty */

#define	SGIE_USERS	94	/* Too many users (for UFS) */

/* BSD Networking Software */
	/* argument errors */
#define	SGIE_NOTSOCK	95		/* Socket operation on non-socket */
#define	SGIE_DESTADDRREQ	96		/* Destination address required */
#define	SGIE_MSGSIZE	97		/* Inappropriate message buffer length */
#define	SGIE_PROTOTYPE	98		/* Protocol wrong type for socket */
#define	SGIE_NOPROTOOPT	99		/* Protocol not available */
#define	SGIE_PROTONOSUPPORT	120		/* Protocol not supported */
#define	SGIE_SOCKTNOSUPPORT	121		/* Socket type not supported */
#define	SGIE_OPNOTSUPP	122		/* Operation not supported on socket */
#define	SGIE_PFNOSUPPORT	123		/* Protocol family not supported */
#define	SGIE_AFNOSUPPORT	124		/* Address family not supported by 
					   protocol family */
#define	SGIE_ADDRINUSE	125		/* Address already in use */
#define	SGIE_ADDRNOTAVAIL	126		/* Can't assign requested address */

	/* operational errors */
#define	SGIE_NETDOWN	127		/* Network is down */
#define	SGIE_NETUNREACH	128		/* Network is unreachable */
#define	SGIE_NETRESET	129		/* Network dropped connection because
					   of reset */
#define	SGIE_CONNABORTED	130		/* Software caused connection abort */
#define	SGIE_CONNRESET	131		/* Connection reset by peer */
#define	SGIE_NOBUFS		132	       	/* No buffer space available */
#define	SGIE_ISCONN		133		/* Socket is already connected */
#define	SGIE_NOTCONN	134		/* Socket is not connected */

/* XENIX has 135 - 142 */
#define	SGIE_SHUTDOWN	143		/* Can't send after socket shutdown */
#define	SGIE_TOOMANYREFS	144		/* Too many references: can't splice */
#define	SGIE_TIMEDOUT	145		/* Connection timed out */
#define	SGIE_CONNREFUSED	146		/* Connection refused */
#define	SGIE_HOSTDOWN	147		/* Host is down */
#define	SGIE_HOSTUNREACH	148		/* No route to host */

#define	SGIE_WOULDBLOCK	SGIE_AGAIN

#define	SGIE_ALREADY	149		/* operation already in progress */
#define	SGIE_INPROGRESS	150		/* operation now in progress */
/* SUN Network File System */
#define	SGIE_STALE		151		/* Stale NFS file handle */

/* Pyramid's AIO Compatibility - raw disk async I/O */
#define	SGIE_IORESID	500		/* block not fully transferred */

/* XENIX error numbers */
#define	SGIE_UCLEAN 	135	/* Structure needs cleaning */
#define	SGIE_NOTNAM		137	/* Not a XENIX named type file */
#define	SGIE_NAVAIL		138	/* No XENIX semaphores available */
#define	SGIE_ISNAM		139	/* Is a named type file */
#define	SGIE_REMOTEIO	140	/* Remote I/O error */
#define	SGIE_INIT		141	/* Reserved for future */
#define	SGIE_REMDEV		142	/* Error 142 */
#define	SGIE_CANCELED	158	/* AIO operation canceled */
/* IRIX5 error numbers with no SVR4 equivalents */
/* Note: New IRIX5 error numbers begin at 1000 */
/* Error numbers for ShareII product */
#define	SGIE_NOLIMFILE	1001	/* share database not open */
#define	SGIE_PROCLIM	1002	/* process limit reached */
#define	SGIE_DISJOINT	1003	/* Lnode hierarchy is disjoint */
#define	SGIE_NOLOGIN	1004	/* Login not allowed for user */
#define	SGIE_LOGINLIM	1005	/* Login limit reached */
#define	SGIE_GROUPLOOP	1006	/* Loop in lnode hierarchy */
#define	SGIE_NOATTACH	1007	/* Attach to lnode not allowed */


#define	SGIE_NOTSUP		1008	/* Not supported (POSIX 1003.1b) */
#define	SGIE_NOATTR		1009	/* Attribute not found */
#define	SGIE_DIRCORRUPTED	1010	/* Directory is corrupted */

#define	SGIE_DQUOT		1133	/* cruft this to be __IRIXBASE + IRIX4 value */
#define	SGIE_NFSREMOTE	1135	/* -- ditto -- */


/*
 * Error numbers for REACT/Pro product 
 */
#define	SGIE_CONTROLLER	1300	/* controlling proccess can not be controlled */
#define	SGIE_NOTCONTROLLER	1301	/* process not a frs controlling process */
#define	SGIE_ENQUEUED	1302	/* process is under control of frame scheduler */
#define	SGIE_NOTENQUEUED	1303	/* process is not control of frame scheduler */
#define	SGIE_JOINED		1304	/* process already joined to a frame scheduler */
#define	SGIE_NOTJOINED	1305	/* process has not joined a frame scheduler */
#define	SGIE_NOPROC		1306	/* process not found */
#define	SGIE_MUSTRUN	1307	/* process is set mustrun */
#define	SGIE_NOTSTOPPED	1308	/* not in stopped state */
#define	SGIE_CLOCKCPU	1309	/* operation not allowed on clock cpu */
#define	SGIE_INVALSTATE	1310	/* Invalid state for requested operation */
#define	SGIE_NOEXIST	1311	/* does not exist */
#define	SGIE_ENDOFMINOR	1312	/* end of minor */
#define	SGIE_BUFSIZE	1313	/* Inappropriate buffer length */
#define	SGIE_EMPTY		1314	/* empty major or minor */
#define	SGIE_NOINTRGROUP	1315	/* no available interrupt group */	
#define	SGIE_INVALMODE	1316	/* Invalid mode */
#define	SGIE_CANTEXTENT	1317	/* Non extendable timing source */
#define	SGIE_INVALTIME	1318	/* Time is out of range */
#define	SGIE_DESTROYED	1319	/* Destroyed or being destroyed */


#endif	/* _SVC_ERRNO_H */
