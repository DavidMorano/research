/* exectrace */



#ifndef	EXECTRACE_INCLUDE
#define	EXECTRACE_INCLUDE	1



/* object defines */

#define	EXECTRACE		struct exectrace_head
#define	EXECTRACE_INFO		struct exectrace_info
#define	EXECTRACE_ENTRY		struct exectrace_e
#define	EXECTRACE_OPERAND	struct exectrace_operand




#include	<sys/types.h>
#include	<sys/param.h>
#include	<netdb.h>		/* for MAXHOSTNAMELEN */

#include	<bio.h>

#include	"localmisc.h"		/* for 'ULONG' */


#ifndef	UINT
#define	UINT	unsigned int
#endif



/* other defines */

#define	EXECTRACE_FILEMAGIC	"EXECTRACE"
#define	EXECTRACE_FILEVERSION	0
#define	EXECTRACE_NAMELEN	MAXNAMELEN
#define	EXECTRACE_WHERELEN	MAXHOSTNAMELEN
#define	EXECTRACE_NREG		256	/* maximum number of registers */
#define	EXECTRACE_NMEM		256	/* maximum number memory variables */

/* record types */

#define	EXECTRACE_RNAME		0	/* program name */
#define	EXECTRACE_RDATE		1	/* date started */
#define	EXECTRACE_RWHERE	2	/* where recorded */
#define	EXECTRACE_RCLOCK	3	/* clock record */
#define	EXECTRACE_RIA		4	/* instruction address record */
#define	EXECTRACE_RREG		5	/* register record (write) */
#define	EXECTRACE_RMEM		6	/* memory record (write) */
#define	EXECTRACE_RREGDUMP	7	/* register dump */
#define	EXECTRACE_RIN		8	/* instruction number */
#define	EXECTRACE_RSOM		9	/* start of major record */
#define	EXECTRACE_RRSA		10	/* register source address */
#define	EXECTRACE_RMSA		11	/* memory source address */
#define	EXECTRACE_RRSV		12	/* register source value */
#define	EXECTRACE_RMSV		13	/* memory source value */
#define	EXECTRACE_RSYSCALL	14	/* SYSCALL */

#define	EXECTRACE_ROVERLAST	15	/* end-valid marker */


#define	EXECTRACE_TMASK		15	/* mask for extracting record type */
#define	EXECTRACE_TBITS		4	/* number of type bits */

/* record data present flags */

#define	EXECTRACE_DPNONE	0
#define	EXECTRACE_DPALL		15
#define	EXECTRACE_DPBYTE0	1
#define	EXECTRACE_DPBYTE1	2
#define	EXECTRACE_DPBYTE2	4
#define	EXECTRACE_DPBYTE3	8




struct exectrace_reading {
	ULONG	clock ;			/* running clock value (reading) */
	ULONG	in ;			/* instruction number */
	int	type ;			/* last type (reading) */
} ;

struct exectrace_info {
	ULONG	in ;			/* instruction number */
	LONG	date ;
	int	version ;		/* file version */
	char	name[EXECTRACE_NAMELEN + 1] ;
	char	where[EXECTRACE_WHERELEN + 1] ;
} ;

struct exectrace_flags {
	UINT	eof : 1 ;		/* EOF indication on reading */
	UINT	in : 1 ;		/* got a starting instruction number */
	UINT	read : 1 ;		/* reading or writing */
	UINT	append : 1 ;		/* appending */
} ;

struct exectrace_head {
	unsigned long			magic ;
	struct exectrace_flags		f ;
	struct exectrace_reading	r ;
	struct exectrace_info		i ;
	bfile		tfile ;
	uint		ia_last ;	/* last IA */
	char		istypeinfo[EXECTRACE_ROVERLAST] ;
	char		istypesom[EXECTRACE_ROVERLAST] ;
	char		istypeextra[EXECTRACE_ROVERLAST] ;
} ;

struct exectrace_operand {
	UINT	a ;			/* address */
	UINT	dv ;			/* value */
	UINT	dp ;			/* data present bits */
} ;

struct exectrace_eflags {
	UINT	clock : 1 ;
	UINT	ia : 1 ;		/* instruction address */
	UINT	sc : 1 ;		/* system call */
	UINT	som : 1 ;		/* Start Of Major (SOM) record */
	UINT	in : 1 ;
	UINT	reg : 8 ;		/* number of registers */
	UINT	sreg : 8 ;		/* number of source registers */
	UINT	mem : 8 ;		/* number of memory variables */
	UINT	smem : 8 ;		/* number of source memory variables */
} ;

struct exectrace_e {
	struct exectrace_eflags		f ;
	ULONG				clock ;
	ULONG				in ;
	struct exectrace_operand	reg[EXECTRACE_NREG] ;
	struct exectrace_operand	sreg[EXECTRACE_NREG] ;
	struct exectrace_operand	mem[EXECTRACE_NMEM] ;
	struct exectrace_operand	smem[EXECTRACE_NMEM] ;
	UINT				ia ;
	UINT				som ;
	UINT				sc ;
} ;




#if	(! defined(EXECTRACE_MASTER)) || (EXECTRACE_MASTER == 1)

extern int	exectrace_open(EXECTRACE *,const char *,
			const char *,int, const char *) ;
extern int	exectrace_close(EXECTRACE *) ;
extern int	exectrace_wclock(EXECTRACE *,ULONG) ;
extern int	exectrace_win(EXECTRACE *,ULONG) ;
extern int	exectrace_wia(EXECTRACE *,UINT) ;
extern int	exectrace_wsom(EXECTRACE *,UINT) ;
extern int	exectrace_wsyscall(EXECTRACE *,UINT) ;
extern int	exectrace_wreg(EXECTRACE *,UINT,UINT,UINT) ;
extern int	exectrace_wrsa(EXECTRACE *,UINT) ;
extern int	exectrace_wrsv(EXECTRACE *,UINT,UINT) ;
extern int	exectrace_wmem(EXECTRACE *,UINT,UINT,UINT) ;
extern int	exectrace_wmsa(EXECTRACE *,UINT) ;
extern int	exectrace_wmsv(EXECTRACE *,UINT,UINT) ;
extern int	exectrace_read(EXECTRACE *,EXECTRACE_ENTRY *) ;
extern int	exectrace_flush(EXECTRACE *) ;

#endif /* (! defined(EXECTRACE_MASTER)) || (EXECTRACE_MASTER == 0) */


#endif /* EXECTRACE_INCLUDE */



