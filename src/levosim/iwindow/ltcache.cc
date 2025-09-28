/* ltcache */

/* Levo Indirect Branch Predictor */


/* revision history :

	= 01/12/27, Marcos R. de Alba
	This object module was originally written.

*/
	
/******************************************************************************

   File name: ltcache.c
   Authors:   Marcos R. de Alba
   Function:  Levo Target Cache for Indirect Branches Prediction 
 
******************************************************************************/


#include	<sys/types.h>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include	"ldecode.h"
#include        "ltcache.h"


/* forward references */

/* initialize this object */

int ltcache_init(ltcache,gp,mip,lip,size)
LTCACHE		*ltcache ;
struct proginfo	*gp ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
int		size ;
{
  int	i,j, rs ;
  
  (void) memset(ltcache,0,sizeof(LTCACHE)) ;
  
  ltcache->gp = gp ;
  ltcache->mip = mip ;
  ltcache->lip = lip ;
  ltcache->size = size;
  
  /* allocate the branch history information table */
  
  rs = uc_malloc(size * sizeof(LTCACHE_INFO), (void **) &ltcache->table);
  if (rs < 0)
    goto bad0;
  
  rs = uc_malloc(size *sizeof(LTCACHE_STATE), (void **) &ltcache->c);
  if (rs<0)
    goto bad1;

  rs = uc_malloc(size *sizeof(LTCACHE_STATE), (void **) &ltcache->n);
  if (rs<0)
    goto bad2;

  rs = uc_malloc(size *sizeof(LTCACHE_INFO), (void **) &ltcache->c.b);
  if (rs<0)
    goto bad3;
  
  rs = uc_malloc(size *sizeof(LTCACHE_INFO), (void **) &ltcache->n.b);
  if (rs<0)
    goto bad4;
  
  /* Have to ask Dave about this */
  
  ltcache->c.b = ltcache->table ;
  ltcache->n.b = ltcache->table + ltcache->size ;
	
  /* allocate the pattern history table */
  /*
    ltcache->c.phbox = ltcache->phbox ;
    ltcache->n.phbox = ltcache->phbox + ltcache->size ;
    
    for(i=0; i < size ; i += 1){
    for(j=0; j < size ; j++)
    ltcache->n.phbox[i].entry[j] = 0 ;
    }
    */
	return SR_OK ;

bad4:
	free(ltcache->c.b);
bad3:
	free(ltcache->n);
bad2:
	free(ltcache->c);
bad1:
	free(ltcache->table);
bad0:
	return rs;

}
/* end subroutine (ltcache_init) */


int ltcache_free(ltcache)
LTCACHE	*ltcache ;
{

  if (ltcache->table != NULL)
    free(ltcache->table) ;		/* only free the top */
  
  ltcache->table = NULL ;
   
  return SR_OK ;
}
/* end subroutine (ltcache_free) */


/* handle a clock transistion */
int ltcache_clock(ltcache)
LTCACHE	*ltcache ;
{
  LTCACHE_INFO	*table ;
  
  /* we swap the allocated parts and copy the rest */

  table = ltcache->c.b ;	    /* save allocated pointer */
  
  ltcache->c = ltcache->n ;	    /* copy everything */
  
  ltcache->n.b = table ;	    /* swap in allocated pointer */
  
  return SR_OK ;
}
/* end subroutine (ltcache_clock) */


/* perform the combinatorial computations */
int ltcache_comb(ltcache,phase)
LTCACHE	*ltcache ;
int	phase ;
{
  int	rs = SR_OK ;
  
  switch (phase) {
    
  case 0:
    ltcache->f.shift = FALSE ;
    rs = ltcache_newstate(ltcache) ;
    break ;
    
  case 1:
    break ;
    
  case 2:
    break ;
    
  case 3:
    rs = ltcache_shift(ltcache);
    break ;
    
  } /* end switch */
  return rs ;
}
/* end subroutine (ltcache_comb) */


/* shift the machine (called in phase 1) */
int ltcache_shift(ltcache)
LTCACHE	*ltcache ;
{
  ltcache->f.shift = TRUE ;
  return SR_OK ;
}
/* end subroutine (ltcache_shift) */

/* INTERNAL SUBROUTINES */

int ltcache_newstate(ltcache)
LTCACHE	*ltcache ;
{
  int	i,size ;
  
#ifdef	COMMENT
  for (i = 0 ; i < ltcache->size ; i += 1)
    ltcache->n.b[i] = ltcache->c.b[i] ;
  
#else
  size = ltcache->size * sizeof(LTCACHE_INFO) ;
  (void) memcpy(ltcache->n.b,ltcache->c.b,size) ;
  
  
#endif
  
  return SR_OK ;
}
/* end subroutine (ltcache_newstate) */

/* Indirect Branch Predictor: a target cache */
int ltcache_pred(ltcache, ilptr)
LTCACHE *ltcache;
uint ilptr;
{
  unsigned int idx;

  idx = (ilptr ^ ltcache->ghp) & (ltcache->size - 1);
  ltcache->table[idx].ilptr = ilptr;
  ltcache->table[idx].times++;
  ltcache->idx = idx;

  /* This is the predicted TA for this branch */
  ltcache->paddr = ltcache->table[idx].BTA;
  
  /* Update Global pattern history */
  /* Based on Karel Driesen and Urs Hozle, paper: 
     "Accurate Indirect Branch Prediction"  
     b*p = 32  assuming  p=4, thus b=8  
     Path length history is 4 only 8 LSB of current branch address
     */
  ltcache->aux = ltcache->ghp;
  ltcache->ghp = (ltcache->ghp << 8) | LSB_(ilptr); 
  
  return SR_OK;
}

/* Routine to update branch predictor */

int ltcache_update(ltcache, ilptr, ta)
     LTCACHE *ltcache;
     uint ilptr;
     uint ta;
{
  unsigned int idx;


/* NOTE: This design assumes all branches commit in order */

  idx =  (ilptr ^ ltcache->aux) & (ltcache->size - 1);
  ltcache->table[idx].times++;
  if (ltcache->table[idx].ilptr != ilptr) {

#ifdef	COMMENT
    ldecode_panic("ltcache_update: indirect branch address mismatch: %x",ilptr);
#endif

    return SR_BADFMT ; 
  }
  else{
    ltcache->table[idx].f_cstatus = 1;
    if (ta != ltcache->table[idx].BTA) {
      /* Missprediction occured */
      if(ltcache->table[idx].misses < 2)
	ltcache->table[idx].misses++;
      else{
	if(ltcache->table[idx].misses == 2){
	  ltcache->table[idx].BTA = ta;
	  ltcache->table[idx].misses = 0;
	}
      }
    }
    else{
      /* Predicted correctly */
      ltcache->table[idx].right_pred++;
      if(ltcache->table[idx].misses > 0)
	ltcache->table[idx].misses--;
    }
    
    return SR_OK;
    
  }
  /* end subroutine */
}




