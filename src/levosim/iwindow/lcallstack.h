/* lcallstack.h

   Define a Call/Return stack pair for the Levo computer

   author: Marcos R. de Alba
   date:   3/2/2001  this program was originally written

*/


#ifndef LCALLSTACK_INCLUDE
#define LCALLSTACK_INCLUDE 1

 
#include	<sys/types.h>
#include <assert.h>
 
#include	"misc.h"
#include        "config.h"
#include        "defs.h"

#include        "lsim.h"
#include        "levoinfo.h"
 
/* object defines */
 
#define LCALLSTACK                 struct lcallstack_head
#define LCALLSTACK_TABLEENTRY      struct lcallstack_table_entry
#define LCALLSTACK_STACKENTRY      struct lcallstack_stack_entry

#define LCALLSTACK_SIZE       32 

struct lcallstack_table_entry{
  uint addr;
  uint ta;
  char sr;
};

struct lcallstack_stack_entry{
  uint sbraddr;
  uint retaddr;
  char sr;
};
 
struct lcallstack_head{
  struct proginfo          *gp ;
  struct mintinfo          *mip ;
  struct levoinfo          *lip ;
  int                      size ;    /* size of call/ret stacks */
  int                      top;
  LCALLSTACK_STACKENTRY    *stack;
  LCALLSTACK_TABLEENTRY    *table;
  int                      last;
} ;                                                         


extern int lcallstack_init(LCALLSTACK *, struct proginfo *, struct mintinfo *, struct levoinfo *, int);
 
extern int lcallstack_search_table(LCALLSTACK *, uint, uint, char);

extern int lcallstack_search_stack(LCALLSTACK *, uint, int);

extern int lcallstack_free(LCALLSTACK *);
 
extern int lcallstack_push(LCALLSTACK *, uint);
 
extern int lcallstack_pop(LCALLSTACK *, uint *);
 
extern int lcallstack_empty(LCALLSTACK *);

extern int lcallstack_full(LCALLSTACK *);
#endif /* lcallstack include */
