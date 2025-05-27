/* licache */


#define	F_DEBUG		1
#define	F_STANDALONE	0		/* define 'main' or not */


#include <cstdio>
#include <math.h>

#include "licache.h"
#include "type.h"
#include "lflowgroup.h"

/* local defines */

#define licache_not_found   -1
#define write_back 1



address_index * find_licache_address_index (LICACHE * licache, uint address_bus);
int is_in_licache (LICACHE * licache, address_index * address_bus_index);
int licache_create_list (LICACHE *licache, int list_size);
int licache_insert_list  (LICACHE * licache,  struct  licache_list_entry request);
struct licache_list_entry * find_next_irequest( LICACHE *licache);
uint licache_write(uint old_value, uint dp, uint new_value);
uint licache_get_addr (LICACHE * licache, int free, int set_value, int interleave);
/*************************************************************************/


int  licache_init(licache, pip, mip, lip, ldmem, lc_parameter, irb, ifb)
LICACHE * licache;
struct proginfo * pip;
struct mintinfo	*mip;
struct levoinfo *lip;
LDMEM **ldmem;
LICACHE_PARAMETERS * lc_parameter;
LIBUS *irb;
RFBUS *ifb;
{  
  
  int number_of_block_per_way;
  int i, j;
  

/* save the parameters for access later ! */
  
  (void) memcpy(&licache->param, lc_parameter,sizeof(LICACHE_PARAMETERS)) ;
  
  licache->ldmem = ldmem;
  
  licache->pip = pip;
  licache->mip = mip;
  licache->lip = lip;

  licache->irb = irb;
  licache->ifb = ifb;   

  licache->way = (LICACHE_WAY_TYPE *) 
    malloc (licache->param.type * sizeof (LICACHE_WAY_TYPE));
  
  if (!licache->way) {
    printf("licache: Out of memory\n");
    return -1;
  }
  
#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("licache_init: division\n") ;
#endif

	if (licache->param.type <= 0)
		licache->param.type = 1 ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("licache_init: blocksize=%d type=%d\n",
    		licache->param.blocksize, licache->param.type) ;
#endif

  number_of_block_per_way = 
    licache->param.size /(licache->param.blocksize * licache->param.type);

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("licache_init: looping 1\n") ;
#endif

  for (i = 0; i < licache->param.type; i ++) {

    licache->way[i]  = 
    (LICACHE_BLOCK *) malloc (number_of_block_per_way * sizeof(LICACHE_BLOCK));
    if (!licache->way[i]) {
       printf("licache: Out of memory\n");
       return -1;
    }
    for (j = 0; j < number_of_block_per_way; j++)
     licache->way[i][j].word = (uint *) malloc(licache->param.blocksize * sizeof (uint))
;

  } /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("licache_init: looping 2\n") ;
#endif

  for (i = 0; i < licache->param.type; i++) {

   for (j = 0; j <number_of_block_per_way; j++) {
      licache->way[i][j].valid = 0;
      licache->way[i][j].counter = i;
      licache->way[i][j].dirty = 0;
   }
   licache->flag = 0;
	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("licache_init: done looping\n") ;
#endif

  /* creating a pending write and read request list */ 
  licache_create_list (licache, licache->param.pending_limit);
  licache->request = (struct licache_list_entry *) sizeof (struct licache_list_entry);
  
 
  /* initiallize licache state! */
  licache -> current_state.stage = licache -> next_state.stage = 0; 
  licache -> current_state.wait = licache -> next_state.wait = -1;
 
   licache -> rhit = 0;
   /* licache -> whit = 0; */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("licache_init: exiting\n") ;
#endif

  return 0;
}
/**************************************************************************/

int licache_comb (LICACHE *licache, int phase)
{

  switch (phase) {
    
   case 3:  licache_phase3 (licache);
            break;
 
   case 4:  licache_phase4 (licache);
            break;
  }
  return 1;
}

/*************************************************************************/

/* requests to the cache come in phase0 or phase1 if any*/

int licache_phase3 (LICACHE *licache)
{
  LFLOWGROUP fgpread;
  LFLOWGROUP fgpwrite;

  /*printf("from licache phase3\n");*/
  licache -> rhit = 0; 
  /*  licache -> whit = 0; */
  if ( rfbus_read(licache->ifb, &fgpread) ){
     int find;
     address_index * addbus_index;
     addbus_index = (address_index *) malloc (sizeof (address_index));
     addbus_index =  find_licache_address_index (licache, fgpread.addr);
     find = is_in_licache(licache, addbus_index);
     if (find == licache_not_found){ /* the request is not in the cache */
       struct licache_list_entry  request;
       /*printf("the read request from addr %d was not in the cache \n",
	       fgpread.addr);*/
	request.address = fgpread.addr;
	request.time_tag = fgpread.tt;
	request.rwbar = 1;
	licache_insert_list  (licache, request);
     }else{
       /*printf("the read request from addr %d was in the cache\n", 
	      fgpread.addr);*/
       fgpread. dv = licache->way[find][addbus_index->set_value].word[addbus_index->index];
       libus_write(licache->irb, &fgpread, 1);
       licache->rhit = 1;
  }
  }
} 
 

  
int licache_phase4 (LICACHE *licache)
{

  address_index * addbus_index;
  int busy;
  int bank_number;
  uint addr;
  int i;

 
  addbus_index = (address_index *)malloc (sizeof(address_index));
  /*  printf("licache phase 3");*/
  switch (licache->current_state.stage) {
    
    
  
  case  0 : 
    licache->request =  find_next_irequest(licache);
    if (licache->request == NULL) {
      /* there is no more pending request*/
      licache->next_state = licache->current_state;
    }
    else{
      int find;
      int rs;
      /* addbus_index = (address_index *) malloc (sizeof (address_index));*/
      addbus_index = find_licache_address_index (licache, licache->request->address);
      find = is_in_licache(licache, addbus_index);
      if (find == -1) {
	licache->free = random () % licache -> param.type;
	  /*url_free_place (licache, addbus_index);*/
	/*	number = licache->request->address >> licache->interleaved;
	number = licache->request->address - number;
	*/
	if ((licache -> param.write_policy == write_back) 
	    &&
	    (licache -> way[licache->free][addbus_index-> set_value].dirty)
	    &&
	    (licache->way[licache->free][addbus_index->set_value].valid)) { 
	  /* this case won't happen because there is no write to the icache */
	  ;
	}
	
	else {
          licache ->next_state.stage = 1;
       
	  licache -> next_state.wait = 0;
	}
      }else {
        licache -> flag = 1;
	licache -> next_state.stage = 2;
	licache -> next_state.wait = 0;
	licache->free = find;
      }      
    }
    
    break; 
    
  case 1 : 
    if (licache->current_state.wait == 0 ){
      busy = 0;
      for ( i = 0; i < licache->param.interleaved; i++)
	busy = busy || ldmem_busy(licache->ldmem[i]);
	
      if ( ! busy){
       uint addr;
       int i;

      licache->next_state. stage = 2;
      licache->next_state.wait = licache->param.readcycles;
      /* change it to ldmem_read*block_size */
      
      addbus_index = find_licache_address_index (licache, licache->request->address);	 
      bank_number = getinterleave(licache->param.interleave_scheduling, 
		    licache->request->address);
      /*??????? 2 should be replaced */
      addr = licache->request->address - (addbus_index->index<<(2+2));
      for (i = 0; i < licache->param.blocksize; i++)
        lsim_readint(licache->mip, addr+(i*4), 
	       &licache->way[licache->free][addbus_index->set_value].word[i]); 
 
      }else 
	licache -> next_state = licache -> current_state;
    } 
  else 
    licache->next_state.wait = licache-> current_state.wait - 1;
  
    break;
  
  case 2:     
    addbus_index = find_licache_address_index (licache, licache->request->address);
    if (licache->current_state.wait == 0){ 
      if  ((licache->request->rwbar == 1) && (licache->rhit)){
       
	licache->next_state = licache->current_state;
      }else{
	LFLOWGROUP lfg;
	int rs;
	licache->next_state.stage = 0;	
	
	if (licache->flag == 0) {
	  licache -> way[licache->free][addbus_index->set_value].valid = 1;
	  licache -> way[licache->free][addbus_index->set_value].dirty = 0;
	}
	licache->flag = 0;
	if (licache->request->rwbar == 1 ){
	  lflowgroup_init(&lfg);
	  lfg.dp = LFLOWGROUP_DPALL;
	  lfg.addr = licache->request->address;
	  lfg.tt = licache->request->time_tag;
	  lfg.dv = 
	    licache -> way[licache->free][addbus_index->set_value].word[addbus_index->index];
	  rs = libus_write(licache->irb, &lfg, 1);
	}else {  /* write request */
	  uint value;

	  value = licache -> way[licache->free][addbus_index->set_value].
	    word[addbus_index->index];
	  licache -> way[licache->free][addbus_index->set_value].
	    word[addbus_index->index] = 
	    licache_write(value, licache->request->dp, licache->request->data);
			  
	  licache -> way[licache->free][addbus_index->set_value].dirty = 1;
	}
      	licache -> way[licache->free][addbus_index->set_value].tag = 
	  addbus_index->tag_value;
      }
      
     
    }else{
      licache->next_state.wait = licache->current_state.wait - 1;		   
    }
    break;  
  }

}

/***************************************************************************/
int licache_clock (LICACHE * licache){

  licache -> current_state = licache -> next_state;
  return 1;
}
/**************************************************************************/
int licache_free (LICACHE * licache){

  return 1;
}

/*************************************************************************/






/* 
   the following functions are not interface, they are just used to do 
   some internal work 

*/

/* helpfs */


/****************************************************************************/
address_index * find_licache_address_index (LICACHE * licache, uint address_bus)
{ 
  address_index * address_bus_index;
  int number_of_block_per_way, num;


  address_bus_index = (address_index *) malloc (sizeof (address_index));
  address_bus = address_bus >> 2 ; /* always shold be done because address is aligned by 4 */  
  address_bus_index -> index =  address_bus & (licache->param.blocksize -1);
  num =  log (licache->param.blocksize)/log (2);
  address_bus = address_bus >> num;
  number_of_block_per_way  = 
    licache->param.size/(licache-> param.type * licache -> param.blocksize);
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

int is_in_licache (LICACHE * licache, address_index * address_bus_index)
{ 
	int i;



 for (i = 0; i < licache->param.type; i++)
  
   if ((licache->way[i][address_bus_index->set_value].tag == 
        address_bus_index->tag_value) &&
    (licache->way[i][address_bus_index->set_value].valid  == 1)) 
    return i;
  
      
  return -1;
 
}


/*****************************************************************************/

int licache_create_list (LICACHE *licache, int list_size)
{
   
    licache -> pending_request_list = 
      (LICACHE_LIST *) malloc (sizeof (LICACHE_LIST));
    licache -> pending_request_list -> head = (struct licache_list_entry  *) 
      malloc (list_size * sizeof (struct licache_list_entry));
   
    if (licache ->pending_request_list == NULL) 
      return 0;
    else{
      licache->pending_request_list ->  first = 
	licache->pending_request_list -> last = 0;
      return 1;
    }
}
/***************************************************************************/
int licache_insert_list  (LICACHE * licache, 
		  struct  licache_list_entry request)
{
  LICACHE_LIST * tp;
  tp = licache -> pending_request_list;

  if (tp -> first - tp -> last == 1) 
    return 0;
  else {
    tp -> head [tp->last] = request;
    tp -> last = (tp -> last + 1) % licache-> param.pending_limit;
  }   
}


/**************************************************************************/
struct licache_list_entry * find_next_irequest( LICACHE *licache)
{
  
  LICACHE_LIST * tp;
  tp = licache -> pending_request_list;
  if (tp ->  first == tp -> last ){
    return NULL;
    }
  else{
    struct licache_list_entry * request;


    request = (struct licache_list_entry *)
      malloc(sizeof(struct licache_list_entry)); 
    request = &(tp -> head [tp -> first]);
    tp -> first = (tp -> first + 1 ) %  licache-> param.pending_limit;
    return request;
  }
  
}    

/**************************************************************************/
uint licache_write(uint old_value, uint dp, uint new_value)
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
uint licache_get_addr (LICACHE * licache, int free, int set_value, int interleave)
{
  uint addr;
  int index_bit, set_bit, tag;
	

  
  index_bit = log(licache->param.blocksize)/log((double) 2);
  set_bit = 
    log((double) licache->param.size/(licache->param.type*licache->param.blocksize))
    / log(2.0);
  tag = licache->way[free][set_value].tag;
  addr = (tag << (index_bit + set_bit+interleave+2)) + (set_value << index_bit+interleave+2);
}










