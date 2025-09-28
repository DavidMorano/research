/* lcallstack.c */



/* revision history :

   Define a Call/Return stack pair for the Levo computer

   author: Marcos R. de Alba
   date:   3/2/2001  this program was originally written


*/
 


#include	<sys/types.h>

#include        <usystem.h>
 
#include	"localmisc.h"
#include        "config.h"
#include        "defs.h"

#include        "lsim.h"
#include        "levoinfo.h"
#include        "lcallstack.h"



                                   
int lcallstack_init(lcs,gp,mip,lip,size)
     LCALLSTACK		*lcs ;
     struct proginfo	*gp ;
     struct mintinfo	*mip ;
     struct levoinfo	*lip ;
     int		size ;
{
  int	i,j, rs ;
  
  (void) memset(lcs,0,sizeof(LCALLSTACK_SIZE)) ;
  
  lcs->gp = gp ;
  lcs->mip = mip ;
  lcs->lip = lip ;
  lcs->size = size;
  lcs->top = -1;          /*initialize this stack */
  
  
  rs = uc_malloc(2 * size * sizeof(uint),&lcs->table);

  if (rs < 0)
    goto bad0;

  rs = uc_malloc(size * sizeof(uint),&lcs->stack);

  if (rs < 0)
    goto bad1;
  
  return SR_OK;

 bad1:
  free(lcs->table);
  
 bad0:
  return;
}

int lcallstack_free(lcs)
LCALLSTACK	*lcs ;
{

  if (lcs->stack != NULL)
    free(lcs->stack) ;		/* only free the top */
  
  lcs->stack = NULL ;
  
  return SR_OK ;
}
/* end subroutine (lcallstack_free) */


/*
int lcallstack_shift(){}

int lcallstack_clock(){}
*/

/* The next two procedures help to reduce the
   misprediction rate for a BHT 
   But I am not using them now, since I discover
   we can handle the return addresses using
   a stack. 

*/

/*
int lcallstack_search_stack(lcs, addr, idx)
     LCALLSTACK *lcs;
     uint       addr;
     int idx;
{
  int i;
  i=lcs->size -1;
  if(lcs->table[idx].sr == 0){
    while(i>=0){
      if(lcs->stack[i].retaddr == addr){
	lcs->table[idx].ta = lcs->stack[i].sbraddr;
	lcs->table[idx].sr =1 ;
	return SR_OK;
      }
      else{
	if(i!=0)
	  i--;
	else
	  eprintf("lcallstack_seach_stack: no entry on stack for that return");
	return SR_OK;
      }
    }
  }
  else{
    while(i>=0){
      if(lcs->stack[i].sbraddr == addr){
	lcs->table[idx].ta = lcs->stack[i].retaddr;
	lcs->table[idx].sr = 1;
	return SR_OK;
      }
      else{
	if(i!=0)
	  i--;
	else
	  eprintf("lcallstack_seach_stack: no entry on stack for that return");
	return SR_OK;
      }
    }
  }
}

int lcallstack_search_table(lcs, addr, ta, type)
     LCALLSTACK *lcs;
     uint       addr;
     uint       ta;
     char       type;  
{
  int i=0;
 
  while(i<(2 * lcs->size)-1){
    if(lcs->table[i].addr != addr){
      if(i == lcs->last){

	i = lcs->last;
	lcs->table[i].addr = addr;
	lcs->table[i].ta = ta;
	lcallstack_push(lcs->stack, ta, addr + 8);
	if(type){
	  lcallstack_search_stack(lcs->stack, addr + 8, i);
	}
	if(lcs->last < (2 * lcs->size)-1)
	  lcs->last +=1;
	return SR_OK;
      }
      else{
	if(i < lcs->last)
	  i++;
      }
    }
    else{
      if(type){
	if(lcs->table[i].sr = 1)
	  lcallstack_search(lcs->stack, lcs->table[i].ta, i);
      }
    }
  }
}
*/
/*

 This push version has to be used when using the above 
 routines
int lcallstack_push(lcs, sbraddr, retaddr)
     LCALLSTACK *lcs;
     uint       sbraddr;
     uint       retaddr;
{
  int i;
  ++(lcs->top);
  if(lcs->top == lcs->size){
    --(lcs->top);
    for(i=lcs->size-1; i>0; i--){
      lcs->stack[i-1]=lcs->stack[i];
    }
  }
  lcs->stack[lcs->top].sbraddr = sbraddr;
  lcs->stack[lcs->top].retaddr = retaddr;
  
  return SR_OK;
}
*/

/* For this I am assuming Mips performs
   subroutine calls and returns in order and
   that for each call there is a matching
   return */

int lcallstack_push(lcs, retaddr)
     LCALLSTACK *lcs;
     uint       retaddr;
{
  int i;
  ++(lcs->top);
  if(lcs->top == lcs->size){
    --(lcs->top);
    for(i=lcs->size-1; i>0; i--){
      lcs->stack[i-1]=lcs->stack[i];
    }
  }
  lcs->stack[lcs->top].retaddr = retaddr;
  
  return SR_OK;
}

/* Becarefull here I am modifying item
   only to return the retaddr.
   When used with the "SEARCH" routines
   has to be handled as a structure */
int lcallstack_pop(lcs, item)
     LCALLSTACK       *lcs;
     /* LCALLSTACK_ENTRY *item;*/
     uint *item;
{
  if(lcs->top < 0){
    eprintf("lcallstack_pop: trying to pop from empty stack\n");
    return SR_FAULT;
  }
  else{
    /* item = lcs->stack[lcs->top];*/
    *item = lcs->stack[lcs->top].retaddr;
    --(lcs->top);
  }
}

int lcallstack_empty(lcs)
     LCALLSTACK *lcs;
{
  return (lcs->top == -1);
}

int lcallstack_full(lcs)
     LCALLSTACK *lcs;
{
  return (lcs->top == lcs->size -1);
}


