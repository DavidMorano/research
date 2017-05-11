/* regfilter */

/* Levo Register Filter component */



/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


	= 00/09/22, Dave Morano

	Just a note to say that this code started life as the Register
	Forwarding Unit but became the Register Filter Unit after I
	realized that the masking type operations for register snooping
	was more complicated than need be for our purposes.  When
	actual time-tag values are passed, then it made the situation
	easy to provide more general caching of register values.


*/




#ifndef	REGFILTER_INCLUDE
#define	REGFILTER_INCLUDE	1

#ifndef	UINT
#define	UINT	unsigned int
#endif




/* object defines */

#define	REGFILTER		struct regfilter_head
#define	REGFILTER_INITARGS	struct regfilter_a
#define	REGFILTER_REG		struct regfilter_reg



#include	<sys/types.h>

#include	<vsystem.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"bus.h"
#include	"lmipsregs.h"
#include	"lfgf.h"		/* Levo FlowGroup FIFO */



/* get the number of ISA registers for MIPS */

#define	REGFILTER_NREGS		LMIPSREG_NREGS
#define	REGFILTER_FSIZE		15	/* minimum FIFO size */
#define	REGFILTER_MINFREE	5	/* minimum FIFO entries before hold */
#define	REGFILTER_DONECOUNT	2	/* clocks to wait for done-ness */




/* a single bus read register */
struct regfilter_busreg {
	LFLOWGROUP	fg ;		/* the data */
	int		f_v ;		/* valid bit */
} ;

struct regfilter_regflags {
	uint		forward : 1 ;
	uint		backward : 1 ;
	uint		brout : 1 ;	/* backward request outstanding */
	uint		new : 1 ;
} ;

struct regfilter_reg {
	ULONG		clock ;		/* clock written */
	LFLOWGROUP	fg ;		/* the Levo FlowGroup */
	struct regfilter_regflags	f ;	/* flags */
	uint		ftseq, frseq ;	/* forwarding sequence numbers */
	uint		btseq, brseq ;	/* backwarding sequence numbers */
	int		fage ;		/* forward age counter */
	int		bage ;		/* backward age counter */
	int		btt ;		/* backward time-tag */
} ;

/* arguments */
struct regfilter_a {
	char	*options ;		/* options string */
	BUS	*rfbuses, *rbbuses ;
	int	id ;			/* our ID */
	int	fbid, bbid ;		/* bus master IDs ! */
	int	fpri, bpri ;		/* our bus priorities */
	int	tt ;			/* our time-tag */
	int	fri ;			/* forwarding bus read index */
	int	bri ;			/* backwarding bus read index */
	int	fwi ;			/* forward bus write index */
	int	bwi ;			/* backward bus write index */
	int	fspan ;			/* forwarding bus span */
	int	bspan ;			/* backwarding bus span */
	int	ffsize ;		/* forward FIFO size */
	int	bfsize ;		/* backward FIFO size */
	int	minfree ;		/* minimum free entries on FIFOs */
} ;

struct regfilter_sflags {
	uint	frequested : 1 ;	/* forward bus has been requested */
	uint	fownbus : 1 ;		/* forward bus is currently owned */
	uint	brequested : 1 ;	/* backward bus has been requested */
	uint	bownbus : 1 ;		/* backward bus is currently owned */
	uint	scanall : 1 ;		/* scan all backwarding buses ? */
} ;

struct regfilter_fifoinfo {
	uint	age ;			/* age */
	int	f_fpd ;			/* first-part-done indication */
} ;

struct regfilter_state {
	struct regfilter_sflags		f ;
	struct regfilter_busreg		*brr ;	/* bus read registers */
	struct regfilter_busreg		fwd ;	/* RFB write data register */
	struct regfilter_busreg		bwd ;	/* EBB write data register */
	REGFILTER_REG			*regs ;	/* all ISA registers */
	struct regfilter_fifoinfo	*fifoinfo ;
	uint				checksum ;
	uint				donecount ;	/* done-ness counter */
	int				tt ;		/* our time-tag */
} ;

struct regfilter_flags {
	uint	shift : 1 ;		/* machine shift requested ? */
	uint	fwdregs : 1 ;		/* forward from register file */
	uint	fwdfifo : 1 ;		/* forward from FIFO */
	uint	backward : 1 ;		/* a backward op is needed */
	uint	settt : 1 ;		/* setting the TT value */
	uint	goingempty : 1 ;	/* FIFO is going empty */
} ;

struct regfilter_allocs {
	struct regfilter_busreg		*brr ;		/* read registers */
	REGFILTER_REG			*regs ;		/* ISA registers */
	struct regfilter_fifoinfo	*fifoinfo ;	/* FIFO information */
} ;

struct regfilter_head {
	unsigned long		magic ;
	struct regfilter_flags	f ;
	struct proginfo		*pip ;
	LSIM			*mip ;
	struct levoinfo		*lip ;
	struct regfilter_state	c, n ;		/* machine state */
	BUS			*rfbuses ;	/* RF buses */
	BUS			*rbbuses ;	/* RB buses bus */
	struct regfilter_allocs	a ;		/* allocations */
	char			*brr_v ;	/* extra 'valid' array */
	struct regfilter_busreg	*fifoshifts ;	/* forwarding FIFO shift-ins */
	LFGF			*fifos ;	/* forwarding FIFOs */
	int			id ;		/* our ID */
	int			nregs ;		/* number of allocated regs */
	int			errors ;	/* programming errors */
	int			ffsize ;	/* forward FIFO size */
	int			bfsize ;	/* forward FIFO size */
	int			minfree ;	/* minimum free entries */
	int			fri ;		/* forward bus read index */
	int			fwi ;		/* forward bus write index */
	int			bwi ;		/* out bus write index */
	int			bri ;		/* backward bus read index */
	int	fspan ;			/* forward bus span */
	int	bspan ;			/* backwarding bus span */
	int	fbid, bbid ;		/* bus master IDs ! */
	int	fpri, bpri ;		/* our bus priorities */
	int	ovtt ;			/* overflow time-tag */
	int	fifocount ;		/* temporary: maximum FIFO count */
} ;


#if	(! defined(REGFILTER_MASTER)) || (REGFILTER_MASTER == 0)

extern int regfilter_init(REGFILTER *,struct proginfo *,LSIM *,
	struct levoinfo *,REGFILTER_INITARGS *) ;
extern int regfilter_initval(REGFILTER *,uint,uint,uint) ;
extern int regfilter_free(REGFILTER *) ;
extern int regfilter_comb(REGFILTER *,int) ;
extern int regfilter_clock(REGFILTER *) ;
extern int regfilter_shift(REGFILTER *) ;
extern int regfilter_scanall(REGFILTER *,int) ;
extern int regfilter_pathcopy(REGFILTER *,REGFILTER *,uint) ;
extern int regfilter_setval(REGFILTER *,uint,uint,uint) ;
extern int regfilter_getval(REGFILTER *,uint,uint,uint *) ;
extern int regfilter_getfg(REGFILTER *,uint,uint,LFLOWGROUP *) ;
extern int regfilter_settt(REGFILTER *,int) ;
extern int regfilter_invalidate(REGFILTER *,uint) ;
extern int regfilter_merge(REGFILTER *,REGFILTER *) ;
extern int regfilter_readycommit(REGFILTER *) ;

#endif /* REGFILTER_MASTER */


#endif /* REGFILTER_INCLUDE */



