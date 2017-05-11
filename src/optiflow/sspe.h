/* sspe */



#ifndef	SSPE_INCLUDE
#define	SSPE_INCLUDE	1



/* objects */

#define	SSPE		struct sspe_head
#define	SSPE_INITARGS	struct sspe_initargs
#define	SSPE_INFO	struct sspe_info
#define	SSPE_OPV	struct sspe_opv
#define	SSPE_FUREQ	struct sspe_fureq




#include	<sys/types.h>

#include	"localmisc.h"

#include	"ss.h"
#include	"machstate.h"
#include	"sscommon.h"
#include	"ssinfo.h"
#include	"ssiw.h"
#include	"sshiftreg.h"




struct sspe_fureq {
	int	fui ;
	int	ful ;
} ;

/* this is the value that gets shifted */
struct sspe_opv {
	ULONG	ia, instrword ;
	struct operand		opsi[MACHSTATE_MAXOPSI] ;
	int	class ;
	int	fui ;
	int	tag ;
	int	nopsi ;
} ;

struct sspe_initargs {
	SSIW	*wp ;
	SSAS	*ssas ;
} ;

struct sspe_sflags {
	uint	memunalign : 1 ;
} ;

struct sspe_nums {
	int	pe_none ;
	int	pe_ialu ;
	int	pe_imult ;
	int	pe_idiv ;
	int	pe_fadd ;
	int	pe_fcmp ;
	int	pe_fcvt ;
	int	pe_fmult ;
	int	pe_fdiv ;
	int	pe_fsqrt ;
	int	pe_ld ;
	int	pe_st ;
} ;

struct sspe_info {
	ULONG	cwaits[iclass_overlast] ;
	ULONG	cgrants[iclass_overlast] ;
	ULONG	cissues[iclass_overlast] ;
	ULONG	cinvalids ;
} ;

struct sspe_state {
	struct sspe_sflags	f ;
	int			cnums[iclass_overlast] ;
	int			nissue ;
} ;

struct sspe_flags {
	uint	shift : 1 ;		/* needs to be shifted */
} ;

struct sspe_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	SS			*mip ;
	struct ssinfo		*lip ;
	SSIW			*wp ;
	SSAS			*ssas ;
	SSHIFTREG		*ssrs ;
	SSHIFTREG		**sra ;

	struct sspe_state	c, n ;	/* state */
	struct sspe_flags	f ;
	struct sspe_info	s ;	/* statistics */

	uint	shiftamount ;		/* amount to shift by (on a "shift") */
	int	cnums[iclass_overlast] ;
	int	cdelays[iclass_overlast] ;
	int	nsr ;			/* number of shift registers */
} ;




/* public subroutine definitions here */

#if	(! defined(SSPE_MASTER)) || (SSPE_MASTER == 0)

extern int sspe_init(SSPE *,struct proginfo *,SS *,
		struct ssinfo *, SSPE_INITARGS *) ;
extern int sspe_free(SSPE *) ;
extern int sspe_comb(SSPE *,int) ;
extern int sspe_clock(SSPE *) ;
extern int sspe_shift(SSPE *,int) ;
extern int sspe_fusu(SSPE *,int) ;
extern int sspe_fureq(SSPE *,int,SSPE_FUREQ *) ;
extern int sspe_fuload(SSPE *,int,int,int,SSPE_OPV *) ;
extern int sspe_allocfu(SSPE *,int,int,SSPE_OPV *) ;
extern int sspe_pollresult(SSPE *,int,int,SSPE_OPV *) ;
extern int sspe_getfu(SSPE *,int,uint *) ;
extern int sspe_info(SSPE *,SSPE_INFO *) ;
extern int sspe_audit(SSPE *) ;

#ifdef	COMMENT

extern int	sspe_xmlinit(SSPE *,XMLINFO *) ;
extern int	sspe_xmlout(SSPE *,XMLINFO *) ;
extern int	sspe_xmlfree(SSPE *,XMLINFO *) ;

#endif /* COMMENT */

#endif /* SSPE_MASTER */

#endif /* SSPE_INCLUDE */



