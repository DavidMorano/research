/* llb */

/* Levo Load Buffer */



/* revision history :

	= 00/02/15, Alireza Khalafi 
	This code was started.

*/



#ifndef	LLB_INCLUDE
#define	LLB_INCLUDE



/* object defines */

#define	LLB		struct llb_head
#define	LLB_INSTRINFO	struct llb_instrinfo



#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"



/* maximum number of predicate registers in an AS */

#define MAX_PREDICATES 100	



struct llb_instrinfo {
	uint	ia ;			/* original */
	uint	instr ;			/* original */
	uint	btype ;			/* Marcos-style branch type */
	uint	opexec ;		/* original */
	uint	opclass ;		/* original */
} ;

struct llb_entry {
	int	valid;	/* valid entry */
	int	src1;	/* address of src register 1 */
	int	src2;	/* address of src register 2 */
	int	src3;	/* address of src register 3 */
	int	src4;	/* address of src register 4 */
	int	dst1;	/* address of dst register 1 */
	int	dst2;	/* address of dst register 2 */
	int	pina;	/* address of pin predicate register */
	int	pinv;	/* initial value of pin reg */
	int	*cpina; /* array of cpin predicate register addresses */
	int	*cpinv; /* array of cpin predicate register values */
	int	ncpin;	/* number of cpin registers */
	int	pouta;	/* address of pout registet */
	int	cpouta; /* address of cpout register */
	int	const1; /* immediate, offset, ... which is part of the inst */
	int	const1_valid;	/* whether the value of the above item is valid or not */

	uint	faddr ;		/* fetch address */
	uint	instr ;		/* original instruction */
	uint	btype ;		/* Marcos style i-fetch branch type */
	int	opclass ;	/* operation class */
	int	opexec ;	/* operation execution code */
	int	oopclass ;	/* original operation class */
	int	oopexec ;	/* original operation execution code */
} ;

struct llb_state {
	struct llb_entry * entry;
} ;

struct llb_flags {
	uint	shift : 1 ;		/* machine shift needed */
	uint	invalidate : 1 ;	/* invalidate ? */
} ;

struct llb_head {
	struct proginfo	*pip ;
	struct mintinfo	*mip ;
	struct levoinfo	*lip ;

	struct llb_flags	f ;	/* configuration or combinatorial */
	struct llb_state	c, n ;	/* machine state */

	int	load_enable;
	int	readyread;
	int	nentries ;		/* number of entries */
} ;



/* method declarations */

extern int	llb_init(LLB *, 
			struct proginfo *, struct mintinfo *, 
			struct levoinfo *);
extern int	llb_free(LLB *) ;
extern int	llb_shift(LLB *) ;
extern int	llb_clock(LLB *) ;
extern int	llb_comb(LLB *,int) ;


/* extern int	llb_load(LLB *,int,int,int,int,int,int,int,int
		,int,int,int,int,int*,int,int,int,int
		,int*); */
extern int	llb_load_complete(LLB *);
/* extern int	llb_read(LLB*,int*,int*,int*,int*,
			int*,int*,int*,int*,int*,int*,int*,
			int*,int**,int**,int*,int*,int*,int); */

extern int	llb_read_complete(LLB *);
extern int	llb_readyread(LLB *);
extern int	llb_loadenable(LLB *);
extern int	llb_invalidate(LLB *);
extern int	llb_is_free(LLB *);
extern int	llb_used(LLB *, int);
extern int	llb_getia(LLB *, int, uint *);
extern int	llb_commitstat(LLB *,int,int);
extern int	llb_instrinfo(LLB *,int, LLB_INSTRINFO *) ;


#endif /* LLB_INCLUDE */



