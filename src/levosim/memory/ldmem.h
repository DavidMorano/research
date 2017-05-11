/* ldmem */

/* memory subsystem */


#ifndef LDMEM_INCLUDE
#define LDMEM_INCLUDE    1



/* public object defines */

#define LDMEM    		struct ldmem_head
#define LDMEM_PARAMETERS	struct ldmem_parameter



#include	<sys/types.h>

#include        "misc.h"
#include        "paramfile.h"
#include        "config.h"
#include        "defs.h"
#include        "mintinfo.h"
#include        "levoinfo.h"




struct ldmem_parameter{
  int read_cycles;
  int write_cycles;
};

struct ldmem_state {
   int wait;
};

struct ldmem_head {
 
  struct proginfo *pip ;
  struct mintinfo *mip ;
  struct levoinfo *lip ;
  
	uint	magic ;
	int	busy;
  LDMEM_PARAMETERS	*param;
  struct ldmem_state       current_state, next_state;
} ;



extern int ldmem_init(LDMEM * ldmem, struct proginfo *,PARAMFILE *,
		      struct mintinfo *,struct levoinfo *, 
		      LDMEM_PARAMETERS *);
extern int ldmem_write(LDMEM *);
extern int ldmem_read(LDMEM *);
extern int ldmem_busy(LDMEM *ldmem);
extern int ldmem_comb(LDMEM *ldmem, int phase);
extern int ldmem_clock(LDMEM *ldmem);


#endif /* LDMEM_INCLUDE */



