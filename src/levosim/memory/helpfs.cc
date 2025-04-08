/* helpfs */



#include "ldcache.h"
#include "type.h"
#include "lflowgroup.h"


uint remove_interleave (uint addr, uint interleave_scheduling)
{ 
  int i;
  uint t, b, a;
  a = addr;
  b = interleave_scheduling;
  

  for (i=0, t=b; t; ) {
    if (t&(1<<i)) {
      t=(t&~(1<<i))>>1;
      a=(a&((1<<i)-1))|((a>>(i+1))<<i);
	}
    else
      i++;
  }
  return a;
}

/****************************************************************************/
address_index * find_address_index (LDCACHE * ldcache, uint address_bus)
{ 
  address_index * address_bus_index;
  int number_of_block_per_way, num;


  address_bus_index = (address_index *) malloc (sizeof (address_index));
  address_bus = address_bus >> 2 ; /* always should be done because address is aligned by 4 */  
  address_bus = 
    remove_interleave(address_bus, ldcache->param.interleave_scheduling);
  address_bus_index -> index =  address_bus & (ldcache->param.blocksize -1);
  num =  log (ldcache->param.blocksize)/log (2);
  address_bus = address_bus >> num;
  number_of_block_per_way  = 
    ldcache->param.size/(ldcache-> param.type * ldcache -> param.blocksize);
  address_bus_index -> set_value =  address_bus & (number_of_block_per_way
-1);
  num = log (number_of_block_per_way)/log (2);
  address_bus = address_bus >> num;
  address_bus_index -> tag_value = address_bus;
  return address_bus_index;
}


/**************************************************************************/
/* returns '-1 ' if the address is not found in the cache, otherwise returns the
way number that address is in */

int is_in_cache (LDCACHE * ldcache, address_index * address_bus_index)
{ 
	int i;



 for (i = 0; i < ldcache->param.type; i++)
  
   if ((ldcache->way[i][address_bus_index->set_value].tag == 
        address_bus_index->tag_value) &&
    (ldcache->way[i][address_bus_index->set_value].valid  == 1)) 
    return i;
  
      
  return -1;
 
}


/*****************************************************************************/
void update_url_counter (LDCACHE * ldcache, address_index * address_bus_index,
                          int find)
{
 int i;
 for (i = 0; i < ldcache->param.type; i++)
   if ( (i != find) &&
          (ldcache->way[i][address_bus_index->set_value].counter < 
            ldcache->way[find][address_bus_index->set_value].counter))

      ldcache-> way[i][address_bus_index->set_value].counter ++;
 ldcache -> way[find][address_bus_index->set_value].counter = 0;
 return ;
}
/*****************************************************************************/
int url_free_place (LDCACHE *ldcache, address_index * address_bus_index)
{
 int i;
 int free;
 for (i =0; i < ldcache->param.type; i++)
    if (ldcache->way[i][address_bus_index->set_value].valid == 0)
      return i;
 free = 0; 
 for (i=1; i < ldcache->param.type; i++)
      if (ldcache->way[i][address_bus_index->set_value].counter > 
          ldcache->way[free][address_bus_index->set_value].counter)
          free = i;
 return free;
}
/****************************************************************************/

int create_list (LDCACHE *ldcache, int list_size)
{
   
    ldcache -> pending_request_list = 
      (LDCACHE_LIST *) malloc (sizeof (LDCACHE_LIST));
    ldcache -> pending_request_list -> head = (struct ldcache_list_entry  *) 
      malloc (list_size * sizeof (struct ldcache_list_entry));
   
    if (ldcache ->pending_request_list == NULL) 
      return 0;
    else{
      ldcache->pending_request_list ->  first = 
	ldcache->pending_request_list -> last = 0;
      return 1;
    }
}
/***************************************************************************/
int insert_list  (LDCACHE * ldcache, 
		  struct  ldcache_list_entry request)
{
  LDCACHE_LIST * tp;
  tp = ldcache -> pending_request_list;

  if (tp -> first - tp -> last == 1) 
    return 0;
  else {
    tp -> head [tp->last] = request;
    tp -> last = (tp -> last + 1) % ldcache-> param.pending_limit;
  }   
}


/**************************************************************************/
struct ldcache_list_entry * find_next_request( LDCACHE *ldcache)
{
  
  LDCACHE_LIST * tp;
  tp = ldcache -> pending_request_list;
  if (tp ->  first == tp -> last ){
    return NULL;
    }
  else{
    struct ldcache_list_entry * request;


    request = (struct ldcache_list_entry *)
      malloc(sizeof(struct ldcache_list_entry)); 
    request = &(tp -> head [tp -> first]);
    tp -> first = (tp -> first + 1 ) %  ldcache-> param.pending_limit;
    return request;
  }
  
}    

/**************************************************************************/
uint ldcache_write(uint old_value, uint dp, uint new_value)
{
  
  switch  (dp) {
    case LFLOWGROUP_DPNONE :   return old_value; 
    case LFLOWGROUP_DPALL :    return new_value;
    case LFLOWGROUP_DPBYTE0 :  return ((old_value & 4095)|(new_value& ~4095));
                               break;
    case LFLOWGROUP_DPBYTE1 :  return((old_value & 61695)|(new_value& ~61695));
                               break;
    case LFLOWGROUP_DPBYTE2 :  return ((old_value & 65295)|(new_value& ~65295));
                               break;
    case LFLOWGROUP_DPBYTE3 :  return ((old_value & 65520)|(new_value& ~65520));
                               break;
  }
}
/**************************************************************************/
uint ldcache_get_addr (LDCACHE * ldcache, int free, int set_value, int interleave)
{
  uint addr;
  int index_bit, set_bit, tag;
	

  
  index_bit = log(ldcache->param.blocksize)/log((double) 2);
  set_bit = 
    log((double) ldcache->param.size/(ldcache->param.type*ldcache->param.blocksize))
    / log(2.0);
  tag = ldcache->way[free][set_value].tag;
  addr = (tag << (index_bit + set_bit+interleave+2)) + (set_value << index_bit+interleave+2);
}



