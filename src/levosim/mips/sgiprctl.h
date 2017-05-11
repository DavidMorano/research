/* sgiprctl */


/**************************************************************************
 *									  *
 * 		 Copyright (C) 1986-1996, Silicon Graphics, Inc.	  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/* * prctl.h - struct for private process data area and prctl and sproc */

#ifndef SGI_PRCTL_H__
#define	SGI_PRCTL_H__


/* * PRDA area - mapped privately into each process/thread */
/* address of the process data area */
#define	SGIPRDA		((struct prda *)0x00200000L)

/* the system portion of the prda */
struct sgiprda_sys {
	pid_t		t_pid;		/* epid */
	unsigned int	t_hint;		/* Share group scheduling hint */
	unsigned int	t_dlactseq;	/* Deadline scheduler activation seq# */
	unsigned int	t_fpflags;	/* Current p_fpflags (SMM, precise) */
	unsigned int	t_prid;		/* Processor type */
	unsigned int	t_dlendseq;     /* Deadline sched allocation seq # */
	unsigned int	t_unused1[10];
	pid_t		t_rpid;		/* pid */
	unsigned int	t_resched;	/* send pthread resched signal */
	unsigned int	t_unused[8];
	
	/*
	 * The following field is used to hold the cpu number that the
	 * process is currently or last ran on
	 */
    	unsigned int 	t_cpu;		/* Last/current cpu */

	/*
	 * The following fields are used by sigprocmask if the hold mask is
	 * to be in user space.
	 */
	unsigned int	t_flags;	/* t_hold mask is being used? */

#ifdef	COMMENT
	k_sigset_t	t_hold;		/* Hold signal bit mask */
#endif /* COMMENT */

};

struct sgiprda {
	char unused[2048];
	union {
		char fill[512];
		unsigned int	rsvd[8];
	} sys2_prda;
	union {
		char fill[512];
	} lib2_prda;
	union {
		char fill[512];
	} usr2_prda;

	union {
		char fill[128];
		struct sgiprda_sys prda_sys;
	} sys_prda;
	union {
		char fill[256];
	} lib_prda;
	union {
		char fill[128];
	} usr_prda;
};

#define	sgit_sys		sys_prda.prda_sys



/* values for prctl */
#define	SGIPR_MAXPROCS	1	/* maximum # procs per user */
#define	SGIPR_ISBLOCKED	2	/* return if pid is blocked */
#define	SGIPR_SETSTACKSIZE 3	/* set max stack size */
#define	SGIPR_GETSTACKSIZE 4	/* get max stack size */
#define	SGIPR_MAXPPROCS	5	/* max parallel processes */
#define	SGIPR_UNBLKONEXEC	6	/* unblock pid on exec/exit */
#define	SGIPR_SETEXITSIG	8	/* signal to be sent to share group on exit */
#define	SGIPR_RESIDENT	9	/* set process immune to swapout */
#define	SGIPR_ATTACHADDR	10	/* re-attach a region */
#define	SGIPR_DETACHADDR	11	/* detach a region */
#define	SGIPR_TERMCHILD	12	/* terminate child proc when parent exits */
#define	SGIPR_GETSHMASK	13	/* retrieve share mask */
#define	SGIPR_GETNSHARE	14	/* return # of members of share group */
#define	SGIPR_COREPID	15	/* if process dumps core, add pid to name */
#define	SGIPR_ATTACHADDRPERM 16	/* re-attach a region, specify permissions */
#define	SGIPR_PTHREADEXIT	17	/* toss a pthread without prejudice */
#define	SGIPR_SETABORTSIG	18	/* signal to be sent to share group on abort */

/* * sproc(2) sharing options */

#define	SGIPR_SPROC	0x00000001	/* doing an sproc(2) call */
#define	SGIPR_SFDS		0x00000002	/* share file descriptors */
#define	SGIPR_SDIR		0x00000004	/* share current/root directory */
#define	SGIPR_SUMASK	0x00000008	/* share umask value */
#define	SGIPR_SULIMIT	0x00000010	/* share ulimit value */
#define	SGIPR_SID		0x00000020	/* share uid/gid values */
#define	SGIPR_SADDR	0x00000040	/* share virtual address space */
#define	SGIPR_SALL		0x0000007f	/* share all sproc(2) options */

/* nsproc(2) sharing options.  These are ignored by sproc and sprocsp. */
#define	SGIPR_SSIGVEC	0x00000080	/* share signal vectors */
#define	SGIPR_SPID		0x00000100	/* share PID among sprocs */

/* sproc(2) and nsproc(2) flags */
#define	SGIPR_FLAGMASK	0xff000000
#define	SGIPR_BLOCK	0x01000000	/* caller blocks on sproc */
#define	SGIPR_NOLIBC	0x02000000	/* do not start libc arena */
#define	SGIPR_PTHREAD	0x04000000	/* set appropriate attributes for
					   POSIX threads. */

/* blockproc[all](2), unblockproc[all](2), setblockproccnt[all](2) limits */
#define	SGIPR_MAXBLOCKCNT	 10000
#define	SGIPR_MINBLOCKCNT	-10000

/*
 * Flags for t_flags
 */
#define	SGIT_HOLD_VALID	0x1		/* Set if t_hold contains valid mask */
#define	SGIT_HOLD_KSIG_O32	0x2		/* k_sigset_t signal position */


#endif /* SGI_PRCTL_H__ */


