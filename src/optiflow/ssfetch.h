/* ssfetch */



#ifndef	SSFETCH_INCLUDE
#define	SSFETCH_INCLUDE	1


/* objects */

#define	SSFETCH			struct ssfetch_head
#define	SSFETCH_INITARGS	struct ssfetch_initargs
#define	SSFETCH_INFO		struct ssfetch_i



#include	<sys/types.h>

#include	"localmisc.h"

#include	"ss.h"
#include	"sscommon.h"
#include	"ssinfo.h"
#include	"ssas.h"



/* NOTES:

We can't just use a regular synchronous FIFO for the fetch Q because we
continue fetching even if a memory OP stalls due to cache delays.  We
still stuff the "fetched" instruction (in a sort of ghost form) into
the fetch Q and mark it as waiting for its data to return from memory.

Fetch Q entries contains information about the instruction including
its pre-decoded form.  The real "decode" state of the real machine is
not modeled but is assumed to occur somewhere between the time the data
comes back from memory and when the data goes into the Q.  Dispatching
to ASes occurs with decoded instructions only.

*/


struct ssfetch_initargs {
	int	nstages ;
} ;

struct ssfetch_i {
	int	n ;			/* active */
	int	ndispatch ;		/* available to be dispatched */
	int	waitcount ;		/* clocks to wait until available */
} ;

struct ssfetch_track {
	uint	waitcount ;		/* latency count */
	uint	f_fetched : 1 ;		/* instruction fetched */
} ;

struct ssfetch_alloc {
	SSAS			**cas ;
	struct ssfetch_track	*t ;
} ;

struct ssfetch_sflags {
	uint	v : 1 ;			/* valid (used) */
} ;

struct ssfetch_state {
	struct ssfetch_sflags	f ;
	struct ssfetch_track	*t ;		/* tracking */
	SSAS			**cas ;		/* decoded instructions */
	uint			nactive ;	/* number used */
	uint			ri ;		/* read index */
	uint			wi ;		/* write index */
} ;

struct ssfetch_flags {
	uint	shift : 1 ;		/* needs to be shifted */
} ;

struct ssfetch_head {
	unsigned long		magic ;

	struct proginfo		*pip ;
	SS			*mip ;
	struct ssinfo		*lip ;

	struct ssfetch_state	c, n ;	/* state */
	struct ssfetch_flags	f ;
	struct ssfetch_alloc	a ;

	int	fqsize ;	/* fetch Q size */
} ;



/* public subroutine definitions here */

#if	(! defined(SSFETCH_MASTER)) || (SSFETCH_MASTER == 0)

extern int ssfetch_init(SSFETCH *,struct proginfo *,SS *,
		struct ssinfo *, SSFETCH_INITARGS *) ;
extern int ssfetch_nfetch(SSFETCH *) ;
extern int ssfetch_readyfetch(SSFETCH *) ;
extern int ssfetch_fetch(SSFETCH *,SSAS *,int) ;
extern int ssfetch_info(SSFETCH *,SSFETCH_INFO *) ;
extern int ssfetch_ndispatch(SSFETCH *) ;
extern int ssfetch_dispatch(SSFETCH *,int,SSAS **) ;
extern int ssfetch_dispatchu(SSFETCH *,int) ;
extern int ssfetch_comb(SSFETCH *,int) ;
extern int ssfetch_clock(SSFETCH *) ;
extern int ssfetch_free(SSFETCH *) ;

#endif /* SSFETCH_MASTER */


#endif /* SSFETCH_INCLUDE */



