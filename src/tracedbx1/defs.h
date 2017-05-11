/* INCLUDE FILE defs */



#include	<sys/types.h>
#include	<sys/param.h>

#include	<vevstr.h>

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

struct proginfo_flags {
	uint	aparams : 1 ;
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
	vecstr	stores ;
	char	**envv ;
	char	*progename ;		/* program name */
	char	*progdname ;		/* program name */
	char	*progname ;		/* program name */
	char	*pr ;		/* program root directory */
	char	*searchname ;
	char	*version ;
	char	*banner ;
	void	*efp ;
	void	*ofp ;
	struct proginfo_flags	f ;
	struct proginfo_flags	open ;
	int	pwdlen ;
	int	progmode ;		/* program mode */
	int	debuglevel ;		/* debugging level */
	int	verboselevel ;		/* verbosity level */
} ;

struct traceinfo {
	ULONG	clock ;
	uint	reg[NREGS] ;
} ;



