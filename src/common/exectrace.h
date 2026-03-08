/* exectrace HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* create and read an execution trace */
/* version %I% last-modified %G% */


/* revision history:

	= 1998-11-01, David Morano
	Originally written for Audix Database Processor (DBP) work.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	EXECTRACE_INCLUDE
#define	EXECTRACE_INCLUDE


#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<netdb.h>		/* for MAXHOSTNAMELEN */
#include	<bfile.h>


/* object defines */
#define	EXECTRACE		struct exectrace_head
#define	EXECTRACE_INFO		struct exectrace_infomation
#define	EXECTRACE_ENT		struct exectrace_entry
#define	EXECTRACE_OPER		struct exectrace_operand
/* other defines */
#define	EXECTRACE_MAGIC		0x86553823
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
	ulong	clock ;			/* running clock value (reading) */
	ulong	in ;			/* instruction number */
	int	type ;			/* last type (reading) */
} ; /* end struct */

struct exectrace_infomation {
	ulong	in ;			/* instruction number */
	long	date ;
	int	version ;		/* file version */
	char	name[EXECTRACE_NAMELEN + 1] ;
	char	where[EXECTRACE_WHERELEN + 1] ;
} ; /* end struct */

struct exectrace_flags {
	uint	eof : 1 ;		/* EOF indication on reading */
	uint	in : 1 ;		/* got a starting instruction number */
	uint	read : 1 ;		/* reading or writing */
	uint	append : 1 ;		/* appending */
} ; /* end struct */

struct exectrace_head {
	uint		magic ;
	struct exectrace_flags		f ;
	struct exectrace_reading	r ;
	struct exectrace_info		i ;
	bfile		tfile ;
	uint		ia_last ;	/* last IA */
	int		rs ;		/* latched error */
	char		istypeinfo[EXECTRACE_ROVERLAST] ;
	char		istypesom[EXECTRACE_ROVERLAST] ;
	char		istypeextra[EXECTRACE_ROVERLAST] ;
} ; /* end struct */

struct exectrace_operand {
	uint	a ;			/* address */
	uint	dv ;			/* value */
	uint	dp ;			/* data present bits */
} ; /* end struct */

struct exectrace_eflags {
	uint	clock : 1 ;
	uint	ia : 1 ;		/* instruction address */
	uint	sc : 1 ;		/* system call */
	uint	som : 1 ;		/* Start Of Major (SOM) record */
	uint	in : 1 ;
	uint	reg : 8 ;		/* number of registers */
	uint	sreg : 8 ;		/* number of source registers */
	uint	mem : 8 ;		/* number of memory variables */
	uint	smem : 8 ;		/* number of source memory variables */
} ;

struct exectrace_entry {
	ulong				clock ;
	ulong				in ;
	struct exectrace_operand	reg[EXECTRACE_NREG] ;
	struct exectrace_operand	sreg[EXECTRACE_NREG] ;
	struct exectrace_operand	mem[EXECTRACE_NMEM] ;
	struct exectrace_operand	smem[EXECTRACE_NMEM] ;
	struct exectrace_eflags		f ;
	uint				ia ;
	uint				som ;
	uint				sc ;
} ; /* end struct */

typedef	EXECTRACE		exectrace ;
typedef	EXECTRACE_INFO		exectrace_info ;
typedef	EXECTRACE_ENT		exectrace_ent ;
typedef	EXECTRACE_OPER		exectrace_oper ;

EXTERNC_begin

extern int	exectrace_open(exectrace *,cchar *,
			cchar *,int, cchar *) noex ;
extern int	exectrace_close(exectrace *) noex ;
extern int	exectrace_wclock(exectrace *,ulong) noex ;
extern int	exectrace_win(exectrace *,ulong) noex ;
extern int	exectrace_wia(exectrace *,uint) noex ;
extern int	exectrace_wsom(exectrace *,uint) noex ;
extern int	exectrace_wsyscall(exectrace *,uint) noex ;
extern int	exectrace_wreg(exectrace *,uint,uint,uint) noex ;
extern int	exectrace_wrsa(exectrace *,uint) noex ;
extern int	exectrace_wrsv(exectrace *,uint,uint) noex ;
extern int	exectrace_wmem(exectrace *,uint,uint,uint) noex ;
extern int	exectrace_wmsa(exectrace *,uint) noex ;
extern int	exectrace_wmsv(exectrace *,uint,uint) noex ;
extern int	exectrace_read(exectrace *,evecstrace_ent *) noex ;
extern int	exectrace_flush(exectrace *) noex ;

EXTERNC_end


#endif /* EXECTRACE_INCLUDE */


