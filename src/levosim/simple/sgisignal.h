/* sgisignal */

/* Copyright (C) 1989-1992 Silicon Graphics, Inc. All rights reserved.  */
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef SGI_SYS_SIGNAL_H
#define	SGI_SYS_SIGNAL_H


/* ANSI C Notes:
 *
 * - THE IDENTIFIERS APPEARING OUTSIDE OF #ifdef's IN THIS
 *   standard header ARE SPECIFIED BY ANSI!  CONFORMANCE WILL BE ALTERED
 *   IF ANY NEW IDENTIFIERS ARE ADDED TO THIS AREA UNLESS THEY ARE IN ANSI's
 *   RESERVED NAMESPACE. (i.e., unless they are prefixed by __[a-z] or
 *   _[A-Z].  For external objects, identifiers with the prefix _[a-z]
 *   are also reserved.)
 *
 * - Section 4.7 indicates that identifiers beginning with SIG or SIG_
 *   followed by an upper-case letter are added to the reserved namespace
 *   when including <signal.h>.
 * POSIX Notes:
 *	Alas, POSIX permits SIG_ but not SIG
 */

/*
 * Signal Numbers.
 */
#define	SGISIGHUP	1	/* hangup */
#define	SGISIGINT	2	/* interrupt (rubout) */
#define	SGISIGQUIT	3	/* quit (ASCII FS) */
#define	SGISIGILL	4	/* illegal instruction (not reset when caught)*/
#define	SGISIGTRAP	5	/* trace trap (not reset when caught) */
#define	SGISIGIOT	6	/* IOT instruction */
#define	SGISIGABRT 6	/* used by abort, replace SIGIOT in the  future */
#define	SGISIGEMT	7	/* EMT instruction */
#define	SGISIGFPE	8	/* floating point exception */
#define	SGISIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SGISIGBUS	10	/* bus error */
#define	SGISIGSEGV	11	/* segmentation violation */
#define	SGISIGSYS	12	/* bad argument to system call */
#define	SGISIGPIPE	13	/* write on a pipe with no one to read it */
#define	SGISIGALRM	14	/* alarm clock */
#define	SGISIGTERM	15	/* software termination signal from kill */
#define	SGISIGUSR1	16	/* user defined signal 1 */
#define	SGISIGUSR2	17	/* user defined signal 2 */
#define	SGISIGCLD	18	/* death of a child */
#define	SGISIGCHLD 18	/* 4.3BSD's/POSIX name */
#define	SGISIGPWR	19	/* power-fail restart */
#define	SGISIGWINCH 20	/* window size changes */
#define	SGISIGURG	21	/* urgent condition on IO channel */
#define	SGISIGPOLL 22	/* pollable event occurred */
#define	SGISIGIO	22	/* input/output possible signal */
#define	SGISIGSTOP	23	/* sendable stop signal not from tty */
#define	SGISIGTSTP	24	/* stop signal from tty */
#define	SGISIGCONT	25	/* continue a stopped process */
#define	SGISIGTTIN	26	/* to readers pgrp upon background tty read */
#define	SGISIGTTOU	27	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SGISIGVTALRM 28	/* virtual time alarm */
#define	SGISIGPROF	29	/* profiling alarm */
#define	SGISIGXCPU	30	/* Cpu time limit exceeded */
#define	SGISIGXFSZ	31	/* Filesize limit exceeded */
#define	SGISIG32	32	/* Reserved for kernel usage */
#define	SGISIGCKPT	33	/* Checkpoint warning */
#define	SGISIGRESTART 34	/* Restart warning  */

/* * Signals reserved for Posix 1003.1c.  */
#define	SGISIGPTINTR	47
#define	SGISIGPTRESCHED	48

/* * Posix 1003.1b signals */
#define	SGISIGRTMIN	49	/* Posix 1003.1b signals */

#define	SGISIGRTMAX	64	/* Posix 1003.1b signals */


#define	SGISIG_PF	((void) (*)(int))

#define	SGISIG_ERR		-1
#define	SGISIG_DFL		0
#define	SGISIG_IGN		1
#define	SGISIG_HOLD		2

/* * POSIX 1003.1b extensions */
typedef union sgisigval {
	int	sival_int;
	void	*sival_ptr;
} sgisigval_t ;

/* XXX Not POSIX93 compliant (nisigno) */
typedef union sginotifyinfo {
	int	nisigno;			/* signal info */
	void	(*nifunc) (sgisigval_t);	 	/* callback data */
} sginotifyinfo_t ;

typedef struct sgisigevent {
	int			sigev_notify;
	sginotifyinfo_t		sigev_notifyinfo;
	sgisigval_t		sigev_value;
	unsigned long		sigev_reserved[13];
	unsigned long		sigev_pad[6];
} sgisigevent_t;


#define	SGISIGEV_NONE	128
#define	SGISIGEV_SIGNAL	129
#define	SFISIGEV_CALLBACK	130


/*
 * POSIX 1003.1b / XPG4-UX extension for signal handler arguments
 * Note the double '_' - this is for XPG/POSIX symbol space rules. This
 * is different than MIPS ABI, but applications shouldn't be using anything
 * but the si_* names.
 */

#define	SGISI_MAXSZ	128
#define	SGISI_PAD		((SGISI_MAXSZ / sizeof(int)) - 3)

/*
 * ABI version of siginfo
 * Some elements change size in the 64 bit environment
 */
typedef struct sgisiginfo {
	int	si_signo;		/* signal from signal.h	*/
	int 	si_code;		/* code from above	*/
	int	si_errno;		/* error from errno.h	*/
	union {

		int	si_pad[SI_PAD];	/* for future growth	*/

		struct {			/* kill(), SIGCLD	*/
			pid_t	__pid;		/* process ID		*/
			union {
				struct {
					uid_t	__uid;
				} __kill;
				struct {
					clock_t __utime;
					int __status;
					clock_t __stime;
				} __cld;
			} __pdata;
		} __proc;			

		struct {	/* SIGSEGV, SIGBUS, SIGILL and SIGFPE	*/
			void 	*__addr;	/* faulting address	*/
		} __fault;

		struct {			/* SIGPOLL, SIGXFSZ	*/
		/* fd not currently available for SIGPOLL */
			int	__fd;	/* file descriptor	*/
			long	__band;
		} __file;
		union sgisigval	__value;

	} __data;

} sgisiginfo_t;

#define	si_pid		__data.__proc.__pid
#define	si_status	__data.__proc.__pdata.__cld.__status
#define	si_stime	__data.__proc.__pdata.__cld.__stime
#define	si_utime	__data.__proc.__pdata.__cld.__utime
#define	si_uid		__data.__proc.__pdata.__kill.__uid
#define	si_addr		__data.__fault.__addr
#define	si_fd		__data.__file.__fd
#define	si_band		__data.__file.__band

/* for si_code, things that we use now */
#define	SGISI_USER		0	/* user generated signal */
#define	SGISI_KILL		SI_USER		/* kill system call */
#define	SGISI_QUEUE	-1		/* sigqueue system call */
#define	SGISI_ASYNCIO	-2		/* posix 1003.1b aio */
#define	SGISI_TIMER	-3		/* posix 1003.1b timers */
#define	SGISI_MESGQ	-4		/* posix 1003.1b messages */

/*
 * POSIX90 added types (all except for ANSI)
 */
typedef struct {                /* signal set type */
        unsigned int	__sigbits[4];
} sgisigset_t;

/* these are not technically in POSIX90, but are permitted.. */
typedef union sgi__sighandler {
    	void	(*__sa_handler)(int); /* SIG_DFL, SIG_IGN, or *fn */
    	void (*__sa_sigaction)(int, sgisiginfo_t *, void *);
} sgi__sighandler_t;

typedef struct sgisigaction {
	int sa_flags;			/* see below for values		*/
    	sgi__sighandler_t sa_sighandler;	/* function to handle signal */
	sgisigset_t sa_mask;		/* additional set of sigs to be	*/
					/* blocked during handler execution */
	int sa_resv[2];
} sgisigaction_t;

/*
 * Posix defined two types of handlers. sa_handler is the default type
 * for use by programs that are not requesting SA_SIGINFO.  Programs
 * requesting SA_SIGINFO need to use a handler of type sa_sigaction.
 */
#ifdef	COMMENT
#define	sa_handler	sa_sighandler.__sa_handler
#define	sa_sigaction	sa_sighandler.__sa_sigaction
#endif /* COMMENT */

/*
 * Definitions for the "how" parameter to sigprocmask():
 *
 * The parameter specifies whether the bits in the incoming mask are to be
 * added to the presently-active set for the process, removed from the set,
 * or replace the active set.
 */
#define	SGISIG_NOP		0	/* Not using 0 will catch some user errors. */
#define	SGISIG_BLOCK	1
#define	SGISIG_UNBLOCK	2	
#define	SGISIG_SETMASK	3
#define	SGISIG_SETMASK32	256	/* SGI added so that BSD sigsetmask won't 
				   affect the upper 32 sigal set */

/* definitions for the sa_flags field */
/*
 * IRIX5/SVR4 ABI definitions
 */
#define	SGISA_ONSTACK	0x00000001	/* handle this signal on sigstack */
#define	SGISA_RESETHAND    0x00000002	/* reset handler */
#define	SGISA_RESTART      0x00000004	/* restart interrupted system call */
#define	SGISA_SIGINFO      0x00000008	/* provide siginfo to handler */
#define	SGISA_NODEFER      0x00000010	/* do not block current signal */
/* The next 2 are only meaningful for SIGCHLD */
#define	SGISA_NOCLDWAIT    0x00010000	/* do not save zombie children */
#define	SGISA_NOCLDSTOP	0x00020000	/* if set do not send SIGCLD	*/
					/* to parent when child stop	*/

/* IRIX5 additions */
#define	SGI_SA_BSDCALL	0x10000000	/* do not scan for dead children when */
					/* setting SIGCHLD */
					/* SJCTRL bit in proc struct.	*/


#define	SGINSIG            65      /* valid signal numbers are from 1 to NSIG-1 */
#define	SGIMAXSIG		(SGINSIG-1)    /* actual # of signals */
#define	SGINUMSIGS		(SGINSIG-1)    /* for POSIX array sizes, true # of sigs */


#endif /* !SGI_SYS_SIGNAL_H */



