 /*ldcache.h*/



#ifndef LDCACHE_INCLUDE
#define LDCACHE_INCLUDE 1



/* public object defines */

#define LDCACHE			struct ldcache_head
#define LDCACHE_BLOCK		struct ldcache_block
#define LDCACHE_LIST		struct ldcache_list
#define LDCACHE_STATE		struct ldcache_state
#define LDCACHE_PARAMETERS	struct ldcache_parameters



#include	<sys/types.h>

#include        "misc.h"
#include        "config.h"
#include        "defs.h"
#include        "mintinfo.h"
#include        "levoinfo.h"
#include        "rfbus.h"
#include        "ldmem.h"



/* interface define */

#define LDCACHE_BLOCK_SIZE   16  
#define LDCACHE_TOTAL_SIZE   2048
#define LDCACHE_TYPE         2
#define LDCACHE_INTERLEAVED  0
#define LDCACHE_WRITE_POLICY 1
#define LDCACHE_MAX_PENDING_REQUEST 10



/* internal defines */

#define LDCACHE_WAY_TYPE       LDCACHE_BLOCK *
#define LDCACHE_BLOCK          struct ldcache_block
#define LDCACHE_LIST           struct ldcache_list
#define LDCACHE_STATE          struct ldcache_state



struct ldcache_block { 
       uint tag;
       int valid;
       uint * word;
       uint counter;
       int dirty;
};  

/* our clock synchronoous state */

struct ldcache_state {
  int  wait; 
/* # of clock must to be waited before memory responses to the current request
 */
  int stage;
}; 

struct  ldcache_list_entry {
  int  rwbar;
  uint address;
  uint  data;
  uint  dp;
  int  time_tag;
 };

struct ldcache_list {
  struct ldcache_list_entry *head;
  int                 first;
  int                 last;
};


struct ldcache_parameters {
  int size;
  int blocksize;
  int type;
  int interleaved;
  int interleave_scheduling;
  int replacement_policy;
  int write_policy;
  int pending_limit;     
  int	readcycles ;
  int	writecycles ;
};


/* our object header data */

struct  ldcache_head { 
  struct proginfo		*pip ;
  struct mintinfo		*mip ;
  struct levoinfo		*lip ;

  LDCACHE_PARAMETERS     param;
  LDCACHE_WAY_TYPE      *way;
  RFBUS			*MBB;
  RFBUS                 *MFB;
  RFBUS                 *MWB;

  struct ldcache_list_entry * request; 
  LDCACHE_LIST           * pending_request_list;
  LDCACHE_STATE          current_state, next_state;
  int                    rhit; /*, whit;*/
  int                    free;
  int                    flag;
  LDMEM                  * ldmem;
};


/*interface functions for ldcache object */
 
extern int ldcache_init(LDCACHE *, struct proginfo *,
	struct mintinfo	*, struct levoinfo *, LDMEM *, LDCACHE_PARAMETERS *,
	RFBUS *, RFBUS *, RFBUS *) ;
extern int ldcache_free (LDCACHE *) ;
extern int ldcache_comb (LDCACHE *, int) ;
extern int ldcache_clock (LDCACHE *) ;


#endif /* LDCACHE_INCLUDE */







































