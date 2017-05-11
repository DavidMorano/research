 /*licache.h*/



#ifndef LICACHE_INCLUDE
#define LICACHE_INCLUDE 1



/* public object defines */

#define LICACHE			struct licache_head
#define LICACHE_BLOCK		struct licache_block
#define LICACHE_LIST		struct licache_list
#define LICACHE_STATE		struct licache_state
#define LICACHE_PARAMETERS	struct licache_parameters



#include	<sys/types.h>

#include        "misc.h"
#include        "config.h"
#include        "defs.h"
#include        "mintinfo.h"
#include        "levoinfo.h"
#include        "rfbus.h"
#include        "libus.h"
#include        "ldmem.h"



/* interface define */

#define LICACHE_BLOCK_SIZE   16  
#define LICACHE_TOTAL_SIZE   2048
#define LICACHE_TYPE         2
#define LICACHE_INTERLEAVED  0
#define LDCACHE_WRITE_POLICY 1
#define LICACHE_MAX_PENDING_REQUEST 10



/* internal defines */

#define LICACHE_WAY_TYPE       LICACHE_BLOCK *
#define LICACHE_BLOCK          struct licache_block
#define LICACHE_LIST           struct licache_list
#define LICACHE_STATE          struct licache_state



struct licache_block { 
       uint tag;
       int valid;
       uint * word;
       uint counter;
       int dirty;
};  

/* our clock synchronoous state */

struct licache_state {
  int  wait; 
/* # of clock must to be waited before memory responses to the current request
 */
  int stage;
}; 

struct  licache_list_entry {
  int  rwbar;
  uint address;
  uint  data;
  uint  dp;
  int  time_tag;
 };

struct licache_list {
  struct licache_list_entry *head;
  int                 first;
  int                 last;
};


struct licache_parameters {
  int size;
  int blocksize;
  int type;
  int interleaved;
  /*
  int replacement_policy;
  */
  int write_policy;
  int interleave_scheduling;
  int pending_limit;     
  int	readcycles ;
  int	writecycles ;
};


/* our object header data */

struct  licache_head { 
  struct proginfo		*pip ;
  struct mintinfo		*mip ;
  struct levoinfo		*lip ;

  LICACHE_PARAMETERS     param;
  LICACHE_WAY_TYPE      *way;
  LIBUS                 *irb;
  RFBUS                 *ifb;

  struct licache_list_entry * request; 
  LICACHE_LIST           * pending_request_list;
  LICACHE_STATE          current_state, next_state;
  int                    rhit; 
  int                    free;
  int                    flag;
  LDMEM                  ** ldmem;
};


/*interface functions for licache object */
 
extern int licache_init(LICACHE *, struct proginfo *,
	struct mintinfo	*, struct levoinfo *, LDMEM **, LICACHE_PARAMETERS *,
	LIBUS *, RFBUS *) ;
extern int licache_free (LICACHE *) ;
extern int licache_comb (LICACHE *, int) ;
extern int licache_clock (LICACHE *) ;


#endif /* LICACHE_INCLUDE */



