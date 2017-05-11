/* misc */

/* miscellaneous stuff which essentially every program wants ! */
/* last modified %G% version %I% */


/* revision history :

	= 83/02/15, Dave Morano

	This code was started.  This is largely a rip off of my old
	'damutil.h' or some such file from use on Audix or early PPI
	embedded work.


	= 95/02/02, Dave Morano

	I added some definitions for 64-bit ints (longs).  This is a
	hack for Solaris and is easily messed up when not used there.


*/




#ifndef	MISC_INCLUDE
#define	MISC_INCLUDE	1



#ifdef	TRUE
#undef	TRUE
#undef	FALSE
#endif

#define	TRUE		1
#define	FALSE		0

#ifdef	YES
#undef	YES
#undef	NO
#endif

#define	YES		1
#define	NO		0

#ifdef	OK
#undef	OK
#undef	BAD
#endif

#define	OK		0
#define	BAD		-1


#ifndef	NULL
#define	NULL		((void *) 0)
#endif

#ifndef	VOID
#define	VOID		void
#endif

#ifndef	VOLATILE
#define	VOLATILE	volatile
#endif

#ifndef	CONST
#define	CONST		const
#endif


#ifndef	MIN
#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef	MAX
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef	UMIN
#define	UMIN(a,b)	(((unsigned long) (a)) < ((unsigned long) (b))) \
				 ? (a) : (b))
#endif

#ifndef	UMAX
#define	UMAX(a,b)	((((unsigned long) (a)) > ((unsigned long) (b))) \
				? (a) : (b))
#endif

#ifndef	ABS
#define	ABS(a)		(((a) < 0) ? (- (a)) : (a))
#endif

#ifndef	LEQUIV
#define	LEQUIV(a,b)	(((a) && (b)) || ((! (a)) && (! (b))))
#endif

#ifndef	LXOR
#define	LXOR(a,b)	(((a) && (! (b))) || ((! (a)) && (b)))
#endif

#ifndef	BCEIL
#define	BCEIL(v,m)	(((v) + ((m) - 1)) & (~ ((m) - 1)))
#endif

#ifndef	BFLOOR
#define	BFLOOR(v,m)	((v) & (~ ((m) - 1)))
#endif

#ifndef	LANGUAGE_NELEMENTS
#define	LANGUAGE_NELEMENTS	1
#ifndef	nelements
#define	nelements(n)	(sizeof(n) / sizeof(n[0]))
#endif
#endif


/* basic scalar types */

#ifndef	CHAR
#define	CHAR	char
#endif

#ifndef	BYTE
#define	BYTE	char
#endif

#ifndef	SHORT
#define	SHORT	short
#endif

#ifndef	INT
#define	INT	int
#endif


#ifndef	LONG
#if	defined(IRIX) || defined(SOLARIS) || defined(LONGLONG)

#define	LONG	long long

#else

#define	LONG	long

#endif /* (whether implementation has 'long long' or not) */
#endif

#ifndef	UCHAR
#define	UCHAR	unsigned char
#endif

#ifndef	USHORT
#define	USHORT	unsigned short
#endif

#ifndef	UINT
#define	UINT	unsigned int
#endif

#ifndef	ULONG
#define	ULONG	unsigned LONG
#endif


#if	defined(DARWIN) 
#define	TYPEDEF_USHORT	1
#define	TYPEDEF_UINT	1
#endif



#if	(! defined(__EXTENSIONS__)) && (! defined(P_MYID))
#if	defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE)
#if	(! defined(IRIX)) || (! defined(_SYS_BSD_TYPES_H))

#ifndef	TYPES_UNSIGNED
#define	TYPES_UNSIGNED	1

#ifndef	TYPEDEF_USHORT
#define	TYPEDEF_USHORT	1

typedef unsigned short	ushort ;

#endif

#ifndef	TYPEDEF_UINT
#define	TYPEDEF_UINT	1

typedef unsigned int	uint ;

#endif

#ifndef	TYPEDEF_ULONG
#define	TYPEDEF_ULONG	1

typedef unsigned long	ulong ;

#endif

#endif /* TYPES_UNSIGNED */

#endif
#endif
#endif


/* do it again ! */

#if	(! defined(TYPES_UNSIGNED)) && (! defined(P_MYID))
#if	(! defined(IRIX)) || (! defined(_SYS_BSD_TYPES_H))

#ifndef	TYPES_UNSIGNED
#define	TYPES_UNSIGNED	1

#ifndef	TYPEDEF_USHORT
#define	TYPEDEF_USHORT	1

typedef unsigned short	ushort ;

#endif

#ifndef	TYPEDEF_UINT
#define	TYPEDEF_UINT	1

typedef unsigned int	uint ;

#endif

#ifndef	TYPEDEF_ULONG
#define	TYPEDEF_ULONG	1

typedef unsigned long	ulong ;

#endif

#endif /* TYPES_UNSIGNED */

#endif
#endif



#ifndef	TYPEDEF_UCHAR
#define	TYPEDEF_UCHAR	1

typedef unsigned char	uchar ;

#endif /* TYPEDEF_UCHAR */


#ifndef	TYPEDEF_LONG64
#define	TYPEDEF_LONG64	1

typedef LONG	long64 ;

#endif /* TYPEDEF_LONG64 */


#ifndef	TYPEDEF_ULONG64
#define	TYPEDEF_ULONG64	1

typedef ULONG	ulong64 ;

#endif /* TYPEDEF_ULONG64 */



#ifndef	TYPEDEF_USTIMET
#define	TYPEDEF_USTIMET		1
typedef unsigned int		ustime_t ;
#endif

#ifndef	TYPEDEF_UTIMET
#define	TYPEDEF_UTIMET		1
#if	defined(_LP64)
typedef unsigned long		utime_t ;
#else
typedef unsigned long long	utime_t ;
#endif
#endif

#ifndef	TYPEDEF_UNIXTIMET
#define	TYPEDEF_UNIXTIMET	1
#if	defined(_LP64)
typedef long			unixtime_t ;
#else
typedef long long		unixtime_t ;
#endif
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

/* it would be nice if the implemenation had these */

#ifndef	SHORT_MIN
#ifdef	SHRT_MIN
#define	SHORT_MIN	SHRT_MIN
#else
#define	SHORT_MIN	(-32768)	/* min value of a "short int" */
#endif
#endif

#ifndef	SHORT_MAX
#ifdef	SHRT_MAX
#define	SHORT_MAX	SHRT_MAX
#else
#define	SHORT_MAX	32767		/* max value of a "short int" */
#endif
#endif

#ifndef	USHORT_MAX
#ifdef	USHRT_MAX
#define	USHORT_MAX	USHRT_MAX
#else
#define	USHORT_MAX	65535		/* max value of "unsigned short int" */
#endif
#endif



/* language features */


#ifndef	LANGUAGE_FOREVER
#define	LANGUAGE_FOREVER	1

#define	forever		for (;;)

#endif /* LANGUAGE_FOREVER */



#endif /* MISC_INCLUDE */



