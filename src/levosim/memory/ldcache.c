/* ldcache */


#define	F_DEBUG		1
#define	F_STANDALONE	0		/* define 'main' or not */


#include <stdio.h>
#include <math.h>

#include "ldcache.h"
#include "type.h"


/* local defines */

#define ldcache_not_found   -1
#define write_back 1




/*************************************************************************/



int ldcache_init(ldcache, pip, mip, lip, ldmem, ld_parameter, MBB, MFB, MWB)
LDCACHE * ldcache;
struct proginfo	*pip ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
LDMEM * ldmem;
LDCACHE_PARAMETERS * ld_parameter;
RFBUS * MBB;
RFBUS * MFB;
RFBUS * MWB;
{  
  
  int number_of_block_per_way;
  int i, j;
  

/* save the parameters for access later ! */
  
  (void) memcpy(&ldcache->param,ld_parameter,sizeof(LDCACHE_PARAMETERS)) ;
  
  ldcache->ldmem = ldmem;
  
  ldcache->pip = pip;
  ldcache->mip = mip;
  ldcache->lip = lip;

  ldcache->MBB = MBB;
  ldcache->MFB = MFB;   
  ldcache->MWB = MWB;

  ldcache->way = (LDCACHE_WAY_TYPE *) 
    malloc (ldcache->param.type * sizeof (LDCACHE_WAY_TYPE));
  
  if (!ldcache->way) {
    printf("ldcache: Out of memory\n");
    return -1;
  }
  
#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("ldcache_init: division\n") ;
#endif

	if (ldcache->param.type <= 0)
		ldcache->param.type = 1 ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("ldcache_init: blocksize=%d type=%d\n",
    		ldcache->param.blocksize, ldcache->param.type) ;
#endif

  number_of_block_per_way = 
    ldcache->param.size /(ldcache->param.blocksize * ldcache->param.type);

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("ldcache_init: looping 1\n") ;
#endif

  for (i = 0; i < ldcache->param.type; i ++) {

    ldcache->way[i]  = 
    (LDCACHE_BLOCK *) malloc (number_of_block_per_way * sizeof(LDCACHE_BLOCK));
    if (!ldcache->way[i]) {
       printf("ldcache: Out of memory\n");
       return -1;
    }
    for (j = 0; j < number_of_block_per_way; j++)
     ldcache->way[i][j].word = (uint *) malloc(ldcache->param.blocksize * sizeof (uint))
;

  } /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("ldcache_init: looping 2\n") ;
#endif

  for (i = 0; i < ldcache->param.type; i++) {

   for (j = 0; j <number_of_block_per_way; j++) {
      ldcache->way[i][j].valid = 0;
      ldcache->way[i][j].counter = i;
      ldcache->way[i][j].dirty = 0;
   }
   ldcache->flag = 0;
	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("ldcache_init: done looping\n") ;
#endif

  /* creating a pending write and read request list */ 
  create_list (ldcache, ldcache->param.pending_limit);
  ldcache->request = (struct ldcache_list_entry *) sizeof (struct ldcache_list_entry);
  
 
  /* initiallize ldcache state! */
  ldcache -> current_state.stage = ldcache -> next_state.stage = 0; 
  ldcache -> current_state.wait = ldcache -> next_state.wait = -1;
 
   ldcache -> rhit = 0;
   /* ldcache -> whit = 0; */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("ldcache_init: exiting\n") ;
#endif

  return 0;
}
/**************************************************************************/

int ldcache_comb (LDCACHE *ldcache, int phase)
{

  switch (phase) {
    
   case 2:  ldcache_phase2 (ldcache);
            break;
 
   case 3:  ldcache_phase3 (ldcache);
            break;
  }
  return 1;
}

/*************************************************************************/

/* requests to the cache come in phase0 or phase1 if any*/

int ldcache_phase2 (LDCACHE *ldcache)
{
  LFLOWGROUP fgpread;
  LFLOWGROUP fgpwrite;

  ldcache -> rhit = 0; 
  /*  ldcache -> whit = 0; */
  if ( rfbus_read(ldcache->MBB, &fgpread) ){
     int find;
     address_index * addbus_index;
     addbus_index = (address_index *) malloc (sizeof (address_index));
     addbus_index = find_address_index (ldcache, fgpread.addr);
     find = is_in_cache(ldcache, addbus_index);
     if ((find == ldcache_not_found) /*|| (is_in_list(ldcache, fgpread.addr) )*/){
       /* the request is not in the cache */
       struct ldcache_list_entry  request;
       /*printf("the read request from addr %d was not in the cache \n",
	       fgpread.addr);*/
	request.address = fgpread.addr;
	request.time_tag = fgpread.tt;
	request.rwbar = 1;
	insert_list  (ldcache, request);
     }else{
       /*printf("the read request from addr %d was in the cache\n", 
	      fgpread.addr);*/
       fgpread. dv = ldcache->way[find][addbus_index->set_value].word[addbus_index->index];
       rfbus_write(ldcache->MFB, &fgpread);
       ldcache->rhit = 1;
  }
  }
  if (rfbus_read(ldcache->MWB, &fgpwrite)){
     int find;
     address_index * addbus_index;
     addbus_index = (address_index *) malloc (sizeof (address_index));
     addbus_index = find_address_index (ldcache, fgpwrite.addr);
     find = is_in_cache(ldcache, addbus_index);
     if ( (find == ldcache_not_found) /*|| (is_in_list(ldcache, fgpread.addr))*/){ /* the request is a miss */
         struct ldcache_list_entry  request;
	 /*printf ("the write request to addr %d wasn't in the cache\n",
		 fgpwrite.addr);*/
         request.address = fgpwrite.addr;
	 request.data = fgpwrite. dv;
         request.dp = fgpwrite.dp;
	 request.time_tag = fgpwrite. tt;
	 request.rwbar = 0;
         insert_list (ldcache, request);
     }else{
       uint value;
       /* it is a hit*/
       /*printf ("the write request to addr %d was in the cache. \n",
	       fgpwrite.addr);*/
       
      
       value = ldcache -> way[find][addbus_index->set_value].word[addbus_index->index]; 
       value = ldcache_write(value, fgpwrite.dp, fgpwrite.dv);
       ldcache -> way[find][addbus_index->set_value].word[addbus_index->index]
	 = value; 
       ldcache -> way[find][addbus_index->set_value].dirty = 1;

       /*	  ldcache->whit = 1; */
           
     } 
  }
}

  
int ldcache_phase3 (LDCACHE *ldcache)
{

  address_index * addbus_index;
  int addr;
  int i;

 
  addbus_index = (address_index *)malloc (sizeof(address_index));
  /*  printf("ldcache phase 3");*/
  switch (ldcache->current_state.stage) {
    
    
  
  case  0 : 
    ldcache->request =  find_next_request(ldcache);
    if (ldcache->request == NULL) {
      /* there is no more pending request*/
      ldcache->next_state = ldcache->current_state;
    }
    else{
      int find;
      int rs;
      /* addbus_index = (address_index *) malloc (sizeof (address_index));*/
      addbus_index = find_address_index (ldcache, ldcache->request->address);
      find = is_in_cache(ldcache, addbus_index);
      if (find == -1) {
	ldcache->free = random () % ldcache -> param.type;
	  /*url_free_place (ldcache, addbus_index);*/
	/*	number = ldcache->request->address >> ldcache->interleaved;
	number = ldcache->request->address - number;
	*/
	if ((ldcache -> param.write_policy == write_back) 
	    &&
	    (ldcache -> way[ldcache->free][addbus_index-> set_value].dirty)
	    &&
	    (ldcache->way[ldcache->free][addbus_index->set_value].valid)) { 
	  if (! ldmem_busy (ldcache -> ldmem )) {
	    ldcache->way[ldcache->free][addbus_index->set_value].valid = 0;
	    addr = ldcache_get_addr(ldcache, ldcache->free, addbus_index->set_value, 2); 
	    for (i = 0; i < ldcache->param.blocksize; i++) 
	      rs = lsim_writeint(ldcache->mip, addr+(i*ldcache->param.interleaved),
			    ldcache->way[ldcache->free][addbus_index->set_value].word[i]
			    , LFLOWGROUP_DPALL);
	    
  	    ldcache->next_state.stage = 1;
            ldcache->next_state.wait = ldcache->param.writecycles; 
	    /* change it to times block-size */
	    /* how about wait state ???*/
	  } 
	  else
	  
	    ldcache -> next_state = ldcache -> current_state;
	}
	
	else {
          ldcache ->next_state.stage = 1;
       
	  ldcache -> next_state.wait = 0;
	}
      }else {
        ldcache -> flag = 1;
	ldcache -> next_state.stage = 2;
	ldcache -> next_state.wait = 0;
	ldcache->free = find;
      }      
    }
    
    break; 
    
  case 1 : 
    if (ldcache->current_state.wait == 0 ){
       
      if ( ! ldmem_busy(ldcache->ldmem) ){
       uint addr;
       int i;

      ldcache->next_state. stage = 2;
      ldcache->next_state.wait = ldmem_read (ldcache -> ldmem);
      /* change it to ldmem_read*block_size */
      
      addbus_index = find_address_index (ldcache, ldcache->request->address);	 
      addr = ldcache->request->address - (addbus_index->index<<(2+2));
      for (i = 0; i < ldcache->param.blocksize; i++)
	lsim_readint(ldcache->mip, addr+ (i*(ldcache->param.interleaved*4)), 
          &ldcache->way[ldcache->free][addbus_index->set_value].word[i]); 
	 
   }
    else 
      ldcache -> next_state = ldcache -> current_state;
    }  
  else 
    
    ldcache->next_state.wait = ldcache-> current_state.wait - 1;
  
  break;
  
  case 2:     
    addbus_index = find_address_index (ldcache, ldcache->request->address);
    if (ldcache->current_state.wait == 0){ 
      if  ((ldcache->request->rwbar == 1) && (ldcache->rhit)){
       
	ldcache->next_state = ldcache->current_state;
      }else{
	LFLOWGROUP lfg;
	int rs;
	ldcache->next_state.stage = 0;	
	
	if (ldcache->flag == 0) {
	  ldcache -> way[ldcache->free][addbus_index->set_value].valid = 1;
	  ldcache -> way[ldcache->free][addbus_index->set_value].dirty = 0;
	}
	ldcache->flag = 0;
	if (ldcache->request->rwbar == 1 ){
          /*printf ("writing stuff on MFB\n");*/
	  lflowgroup_init(&lfg);
	  lfg.dp = LFLOWGROUP_DPALL;
	  lfg.addr = ldcache->request->address;
	  lfg.tt = ldcache->request->time_tag;
	  lfg.dv = 
	    ldcache -> way[ldcache->free][addbus_index->set_value].word[addbus_index->index];
	  rs = rfbus_write(ldcache->MFB, &lfg);
	  /*if (rs >= 1) ;*/ /*printf("the writing on MFB was successful\n");*/
	}else {  /* write request */
	  uint value;

	  value = ldcache -> way[ldcache->free][addbus_index->set_value].
	    word[addbus_index->index];
	  ldcache -> way[ldcache->free][addbus_index->set_value].
	    word[addbus_index->index] = 
	    ldcache_write(value, ldcache->request->dp, ldcache->request->data);
			  
	  ldcache -> way[ldcache->free][addbus_index->set_value].dirty = 1;
	}
      	ldcache -> way[ldcache->free][addbus_index->set_value].tag = 
	  addbus_index->tag_value;
      }
      
     
    }else{
      ldcache->next_state.wait = ldcache->current_state.wait - 1;		   
    }
    break;  
  }

}

/***************************************************************************/
int ldcache_clock (LDCACHE * ldcache){

  ldcache -> current_state = ldcache -> next_state;
  return 1;
}
/**************************************************************************/
int ldcache_free (LDCACHE * ldcache){

  return 1;
}











