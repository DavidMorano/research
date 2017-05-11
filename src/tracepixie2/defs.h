/* INCLUDE FILE defs */



#include	<sys/types.h>
#include	<sys/param.h>

#include	<bfile.h>

#include	"localmisc.h"




#define	LINELEN		256		/* size of input line */

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	4096
#endif

#define	BUFLEN		8192
#define	CMDBUFLEN	(8 * MAXPATHLEN)

#ifndef	FD_STDIN
#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2
#endif

#define	DUMPOPTS	"dump"

#define	NREGS		200


/* program data */

struct proginfo_select {
	ULONG	start, end ;
	ULONG	n ;
} ;

struct proginfo_flags {
	uint	log : 1 ;
	uint	quiet : 1 ;
	uint	progmap : 1 ;
	uint	mipsdis : 1 ;
	uint	opt_regs : 1 ;
	uint	opt_regvals : 1 ;
	uint	opt_mems : 1 ;
	uint	opt_memvals : 1 ;
	uint	opt_instr : 1 ;
	uint	opt_instrdis : 1 ;
} ;

struct proginfo {
	bfile	*efp ;
	bfile	*ofp ;
	char	*version ;
	char	*pwd ;
	char	*progdir ;		/* program directory */
	char	*progname ;		/* program name */
	char	*searchname ;
	char	*programroot ;		/* program root directory */
	struct proginfo_flags	f ;
	struct proginfo_select	in ;
	int	debuglevel ;		/* debugging level */
	int	verboselevel ;		/* verbosity level */
	int	progmode ;		/* program mode */
} ;

struct traceinfo {
	ULONG	clock ;
	uint	reg[NREGS] ;
} ;



