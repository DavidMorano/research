/* sslsq */



#ifndef	SSLSQ_INCLUDE
#define	SSLSQ_INCLUDE	1


/* objects */

#define	SSLSQ			struct sslsq_head
#define	SSLSQ_INITARGS		struct sslsq_initargs
#define	SSLSQ_COMMITINFO	struct sslsq_commitinfo
#define	SSLSQ_CURSOR		struct sslsq_c




#include	<sys/types.h>

#include	"localmisc.h"

#include	"ss.h"
#include	"machstate.h"
#include	"sscommon.h"
#include	"ssinfo.h"




struct sslsq_c {
	int	si ;			/* structure index */
	int	ei ;			/* entry index */
} ;

struct sslsq_initargs {
	int	rftype ;
	int	nwrites ;
	int	nreads ;
} ;

struct sslsq_rflags {
	uint	v : 1 ;			/* used-valid */
	uint	export : 1 ;		/* needs to be exported */
} ;

struct sslsq_slot {
	struct operand		fg ;
	struct sslsq_rflags	f, set, clr ;
	uint			cc ;	/* clock count */
} ;

struct sslsq_sflags {
	uint	v : 1 ;			/* valid (used) */
} ;

struct sslsq_state {
	struct sslsq_sflags	f ;
	struct sslsq_slot	*writes ;
	struct sslsq_slot	*reads ;
	uint			nwrites, nreads ;
} ;

struct sslsq_allocs {
	struct sslsq_slot	*writes ;
	struct sslsq_slot	*reads ;
} ;

struct sslsq_flags {
	uint	loaded : 1 ;		/* it's been loaded */
	uint	retire : 1 ;		/* will be retired */
	uint	commit : 1 ;		/* do a commitment if nothing else */
	uint	shift : 1 ;		/* needs to be shifted */
	uint	needexec : 1 ;		/* needs a new execution */
} ;

struct sslsq_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	SS			*mip ;
	struct ssinfo		*lip ;

	struct sslsq_allocs	a ;
	struct sslsq_state	c, n ;	/* state */
	struct sslsq_flags	f ;

	int	rftype ;
	int	nwrites ;
	int	nreads ;
	int	shiftamount ;
} ;



/* public subroutine definitions here */

#if	(! defined(SSLSQ_MASTER)) || (SSLSQ_MASTER == 0)

extern int sslsq_init(SSLSQ *,struct proginfo *,SS *,
		struct ssinfo *, SSLSQ_INITARGS *) ;
extern int sslsq_free(SSLSQ *) ;
extern int sslsq_comb(SSLSQ *,int) ;
extern int sslsq_clock(SSLSQ *) ;
extern int sslsq_shift(SSLSQ *,int) ;
extern int sslsq_setval(SSLSQ *,int,ULONG) ;
extern int sslsq_opmatch(SSLSQ *,OPERAND *) ;
extern int sslsq_opget(SSLSQ *,int,OPERAND **) ;
extern int sslsq_oprequest(SSLSQ *,ULONG,uint) ;
extern int sslsq_opmerge(SSLSQ *,OPERAND *) ;

extern int sslsq_cursorinit(SSLSQ *,SSLSQ_CURSOR *) ;
extern int sslsq_cursorfree(SSLSQ *,SSLSQ_CURSOR *) ;
extern int sslsq_opexport(SSLSQ *,SSLSQ_CURSOR *,OPERAND **) ;

#ifdef	COMMENT

extern int	sslsq_xmlinit(SSLSQ *,XMLINFO *) ;
extern int	sslsq_xmlout(SSLSQ *,XMLINFO *) ;
extern int	sslsq_xmlfree(SSLSQ *,XMLINFO *) ;

#endif /* COMMENT */

#endif /* SSLSQ_MASTER */


#endif /* SSLSQ_INCLUDE */



