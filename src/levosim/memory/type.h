/* type */



#include <stdio.h>
#include <math.h>

#include	"ldcache.h"




typedef struct { uint tag_value;
                 int set_value; 
                 int index;
           
} address_index;    


/****************************************************************************/
address_index * find_address_index (LDCACHE * ldcache, uint addbus);

int is_in_cache (LDCACHE * ldcache, address_index * addbus_index);

void update_url_counter (LDCACHE * ldcache, address_index * addbus_index, 
			 int find);

int url_free_place (LDCACHE *ldcache, address_index * addbus_index);

int create_list (LDCACHE * ldcache, int list_size);

int insert_list  (LDCACHE * ldcache, struct  ldcache_list_entry request);

struct ldcache_list_entry * find_next_request( LDCACHE *ldcache);

int is_in_list(LDCACHE *ldcache, uint addr);
