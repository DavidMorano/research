/* ssreg */



#ifndef	SSREG_INCLUDE
#define	SSREG_INCLUDE	1


/* objects */

#define	SSREG			struct ssreg_head
#define	SSREG_INITARGS		struct ssreg_initargs
#define	SSREG_COMMITINFO	struct ssreg_commitinfo
#define	SSREG_CURSOR		struct ssreg_c



#include	<sys/types.h>

#include	"defs.h"
#include	"localmisc.h"

#include	"ss.h"
#include	"machstate.h"
#include	"sscommon.h"
#include	"ssinfo.h"




struct ssreg_initargs {
	int	rftype ;
} ;

struct ssreg_rflags {
	uint	export : 1 ;
} ;

struct ssreg_c {
	int	ei ;			/* entry index */
} ;

struct ssreg_reg {
	struct operand		fg ;
	struct ssreg_rflags	f, set, clr ;
} ;

struct ssreg_sflags {
	uint	v : 1 ;			/* valid (used) */
} ;

struct ssreg_state {
	struct ssreg_sflags	f ;
	struct ssreg_reg	regs[MACHSTATE_NREGS] ;
} ;

struct ssreg_flags {
	uint	loaded : 1 ;		/* it's been loaded */
	uint	retire : 1 ;		/* will be retired */
	uint	commit : 1 ;		/* do a commitment if nothing else */
	uint	shift : 1 ;		/* needs to be shifted */
	uint	needexec : 1 ;		/* needs a new execution */
} ;

struct ssreg_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	SS			*mip ;
	struct ssinfo		*lip ;

	struct ssreg_state	c, n ;	/* state */
	struct ssreg_flags	f ;

	int	rftype ;
	int	shiftamount ;
} ;



/* public subroutine definitions here */

#if	(! defined(SSREG_MASTER)) || (SSREG_MASTER == 0)

extern int ssreg_init(SSREG *,struct proginfo *,SS *,
		struct ssinfo *, SSREG_INITARGS *) ;
extern int ssreg_free(SSREG *) ;
extern int ssreg_comb(SSREG *,int) ;
extern int ssreg_clock(SSREG *) ;
extern int ssreg_shift(SSREG *,int) ;
extern int ssreg_setval(SSREG *,int,ULONG) ;
extern int ssreg_opmatch(SSREG *,OPERAND *) ;
extern int ssreg_opget(SSREG *,int,OPERAND **) ;
extern int ssreg_oprequest(SSREG *,int) ;
extern int ssreg_opmerge(SSREG *,OPERAND *) ;

extern int ssreg_cursorinit(SSREG *,SSREG_CURSOR *) ;
extern int ssreg_cursorfree(SSREG *,SSREG_CURSOR *) ;
extern int ssreg_opexport(SSREG *,SSREG_CURSOR *,OPERAND **) ;

#ifdef	COMMENT

extern int	ssreg_xmlinit(SSREG *,XMLINFO *) ;
extern int	ssreg_xmlout(SSREG *,XMLINFO *) ;
extern int	ssreg_xmlfree(SSREG *,XMLINFO *) ;

#endif /* COMMENT */

#endif /* SSREG_MASTER */


#endif /* SSREG_INCLUDE */



