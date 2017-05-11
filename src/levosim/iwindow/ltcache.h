/* ltcache */

/* Target Cache: Levo Indirect Branch Predictor */


/* NOTAS

     1 de Febrero del 2001

     En este codigo debo revisar si es necesario incluir una
     tabla de historia como en el predictor de branches
     PHTBOX. De momento no lo estoy usando. Es muy IMPORTANTE
     considerar esto.
     */

#ifndef	LTCACHE_INCLUDE
#define	LTCACHE_INCLUDE	


/* object defines */

#define	LTCACHE		struct ltcache_head
#define	LTCACHE_INFO	struct ltcache_info
#define LTCACHE_STATE   struct ltcache_state



#include	<sys/types.h>
#include <assert.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"



#define LTCACHE_SIZE  256    /* size of target cache table */
#define TAKEN 1
#define NOT_TAKEN 0
#define LSB_(_ilptr)(_ilptr & 0xff)

/* Structure to keep track of branch statistics */
struct ltcache_info {
  uint BTA;  /* target addresses visited by this indirect branch */
  uint ilptr;
  int misses;
  int right_pred; 
  int times;       /* Number of times this ind branch has been commited */
  int f_cstatus;  /* Commited status: 0 not commited, 1 commited */
} ;

struct ltcache_phtbox{
  uint *entry;
} ;

struct ltcache_state {
  struct ltcache_info	*b ;
} ;

struct ltcache_flags {
  uint			shift : 1 ;
} ;

struct ltcache_head {
  struct ltcache_flags   f ;	     /* flags */
  struct proginfo	 *gp ;
  struct mintinfo	 *mip ;
  struct levoinfo	 *lip ;
  LTCACHE_INFO	         *table ;    /* target cache table */  
  LTCACHE_STATE	         c, n ;	   
  int	                 size ;      /* size of table, max is 2^32 */
  uint                   ghp;        /* global history pattern */
  uint                   paddr;      /* predicted address */
  uint                   idx;        /* index to table */
  uint                   aux;
} ;


extern int ltcache_newstate(LTCACHE *) ;
extern int ltcache_pred(LTCACHE *, uint);
extern int ltcache_update(LTCACHE *, uint,  uint);  
extern int ltcache_init(LTCACHE	*, struct proginfo *, struct mintinfo *, struct levoinfo *, int);
extern int ltcache_free(LTCACHE	*);
extern int ltcache_clock(LTCACHE *);
extern int ltcache_comb(LTCACHE	*,int);

#endif /* LTCACHE_INCLUDE */


