/* sgiresource */


/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)resource.h	7.1 (Berkeley) 6/4/86
 */
#ifndef SGI_SYS_RESOURCE_H
#define SGI_SYS_RESOURCE_H


#include	"sgisystime.h"



/* * Resource utilization information.  */

#define	SGIRUSAGE_SELF		0
#define	SGIRUSAGE_CHILDREN	-1

struct sgirusage {
	struct sgitimeval ru_utime;	/* user time used */
	struct sgitimeval ru_stime;	/* system time used */
	long	ru_maxrss;
#define	sgiru_first	ru_ixrss
	long	ru_ixrss;		/* integral shared memory size */
	long	ru_idrss;		/* integral unshared data " */
	long	ru_isrss;		/* integral unshared stack " */
	long	ru_minflt;		/* page reclaims */
	long	ru_majflt;		/* page faults */
	long	ru_nswap;		/* swaps */
	long	ru_inblock;		/* block input operations */
	long	ru_oublock;		/* block output operations */
	long	ru_msgsnd;		/* messages sent */
	long	ru_msgrcv;		/* messages received */
	long	ru_nsignals;		/* signals received */
	long	ru_nvcsw;		/* voluntary context switches */
	long	ru_nivcsw;		/* involuntary " */
#define	sgiru_last		ru_nivcsw
};


/* * Resource limits */

#define	SGIRLIMIT_CPU	0		/* cpu time in seconds */
#define	SGIRLIMIT_FSIZE	1		/* maximum file size */
#define	SGIRLIMIT_DATA	2		/* data size */
#define	SGIRLIMIT_STACK	3		/* stack size */
#define	SGIRLIMIT_CORE	4		/* core file size */
#define SGIRLIMIT_NOFILE	5		/* file descriptors */
#define SGIRLIMIT_VMEM	6		/* maximum mapped memory */
#define	SGIRLIMIT_RSS	7		/* resident set size */
#define SGIRLIMIT_AS	SGIRLIMIT_VMEM

#define	SGIRLIM_NLIMITS	8		/* number of resource limits */

#define	SGIRLIM_INFINITY	0x7fffffff

typedef unsigned int sgirlim_t;

struct sgirlimit {
	sgirlim_t	rlim_cur;		/* current (soft) limit */
	sgirlim_t	rlim_max;		/* maximum value for rlim_cur */
};


#endif /* SGI_SYS_RESOURCE_H */


