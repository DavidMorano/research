/* lmem */

/* memory subsystem */


#ifndef	LMEM_INCLUDE
#define	LMEM_INCLUDE	1


/* object defines */

#define	LMEM		struct lmem_head
#define	LMEM_ARGS 	struct lmem_parameter 




#include	<sys/types.h>

#include	<vsystem.h>

#include	"misc.h"
#include	"paramfile.h"
#include	"config.h"
#include	"defs.h"
#include	"mintinfo.h"
#include	"levoinfo.h"
#include	"licache.h"
#include	"ldcache.h"
#include	"ldmem.h"
#include        "memorytest.h"


/* defaults of some things */

#define LMEM_INTERLEAVED 	4	/* default interleave factor ? */

#define	LMEM_ICACHESIZE		0x100
#define	LMEM_DCACHESIZE		0x100




struct lmem_parameter {
  int interleave;
} ;

struct lmem_flags {
	uint	shift : 1 ;
} ;

struct lmem_head {
  struct proginfo     * pip ;
  struct mintinfo     * mip ;
  struct levoinfo     * lip ;
  struct lmem_flags	f ;
  LICACHE	      * licache;
  LDCACHE	      ** ldcache;
  LDMEM               ** ldmem;
  MEMORYTEST          * memtest;
  uint	magic ;
  int	interleave ;
} ;





extern int lmem_init(LMEM *, struct proginfo *, PARAMFILE *,struct mintinfo *,
		     struct levoinfo *, LMEM_ARGS *) ;
extern int lmem_comb(LMEM *,int) ;
extern int lmem_clock(LMEM *) ;
extern int lmem_free(LMEM *) ;
extern int lmem_shift(LMEM *) ;



#endif /* LMEM_INCLUDE */



