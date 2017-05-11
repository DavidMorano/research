/* lloop */

/* Levo Branch Predictor */


/* revision history :

	= 00/12/18, Marcos R. de Alba

	This object module was originally written.


	= 01/01/04, Marcos R. de Alba 

	Completed object for initial test.


	= 01/08/06, Dave Morano

	Marcos has not been able to fix the broken loop in here (a few
	months now) so I commented the troublesome thing out with a
	'BROKEN' comment.


*/



/*****************************************************************************

   File name: lloop.c
   Authors:   Marcos R. de Alba
   Date:      Dec, 18 2000 (updated version)
   Function:  Loop predictor for Levo


******************************************************************************/



#include	<sys/types.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include        "lloop.h"


/* forward references */

/* initialize this object */

int lloop_init(lloop,gp,mip,lip,size)
LLOOP		*lloop ;
struct proginfo	*gp ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
int		size ;
{
 	int	i,j,rs ;
	(void) memset(lloop,0,sizeof(LLOOP)) ;

	lloop->gp = gp ;
	lloop->mip = mip ;
	lloop->lip = lip ;
	lloop->size = size;
	lloop->ssize = 0;
	/* allocate the branch history information table */

	rs = uc_malloc(size * sizeof(LLOOP_INFO), (void **) &lloop->ltable);
	if (rs < 0)
	  goto bad0;
	
	rs = uc_malloc(LOOPSTACKSIZE *sizeof(LLOOP_ENTRY), (void **) &lloop->lstack);
	if (rs<0)
	  goto bad1;
	
	/* Have to ask Dave about this */
	
	lloop->c.l = lloop->ltable ;
   	lloop->n.lstack = lloop->lstack + lloop->size ; /* ??? */
	
	return SR_OK ;

bad1:
	free(lloop->ltable);
bad0:
	return rs;

}
/* end subroutine (lloop_init) */


int lloop_free(lloop)
LLOOP	*lloop ;
{

  if (lloop->ltable != NULL)
    free(lloop->ltable) ;		/* only free the top */
  
  if (lloop->lstack != NULL)
    free(lloop->lstack);
  
  lloop->ltable = NULL ;
  lloop->lstack = NULL ;
  
  return SR_OK ;
}
/* end subroutine (lloop_free) */


/* handle a clock transistion */
int lloop_clock(lloop)
LLOOP	*lloop ;
{
  LLOOP_INFO	*ltable ;
  LLOOP_ENTRY   *lstack ;
  
  /* we swap the allocated parts and copy the rest */

  ltable = lloop->c.l ;	    /* save allocated pointer */
  lstack = lloop->c.lstack;
  
  lloop->c = lloop->n ;	    /* copy everything */
  
  lloop->n.l = ltable ;	    /* swap in allocated pointer */
  lloop->n.lstack = lstack ;
  
  return SR_OK ;
}
/* end subroutine (lloop_clock) */


/* perform the combinatorial computations */
int lloop_comb(lloop,phase)
LLOOP	*lloop ;
int	phase ;
{
  int	rs = SR_OK ;
  
  switch (phase) {
    
  case 0:
    lloop->f.shift = FALSE ;
    rs = lloop_newstate(lloop) ;
    break ;
    
  case 1:
    break ;
    
  case 2:
    break ;
    
  case 3:
    rs = lloop_shift(lloop);
    break ;
    
  } /* end switch */
  return rs ;
}
/* end subroutine (lloop_comb) */


/* shift the machine (called in phase 1) */
int lloop_shift(lloop)
LLOOP	*lloop ;
{
  lloop->f.shift = TRUE ;
  return SR_OK ;
}
/* end subroutine (lloop_shift) */

/* INTERNAL SUBROUTINES */

int lloop_newstate(lloop)
LLOOP	*lloop ;
{
  int	i,size ;
  
#ifdef	COMMENT
  for (i = 0 ; i < lloop->size ; i += 1)
    lloop->n.l[i] = lloop->c.l[i] ;
  
  for (i = 0 ; i < lloop->size ; i += 1)
    lloop->n.lstack[i] = lloop->c.lstack[i] ;
#else
  
  size = lloop->size * sizeof(LLOOP_INFO) ;
  (void) memcpy(lloop->n.l,lloop->c.l,size) ;
  
  size = lloop->size * sizeof(char) ;
  (void) memcpy(lloop->n.lstack,lloop->c.lstack,size) ;
  
#endif
  
  return SR_OK ;
}
/* end subroutine (lloop_newstate) */


int lloop_pred(lloop, ilptr, lta)
     LLOOP *lloop;
     uint ilptr;
     uint lta;
{
  int loopnum;
  
  /* To experiment, if not accurate we can use xor for reducing
     aliasing */
  loopnum = lta % lloop->size ;
  lloop->ltable[loopnum].id = lta;

  /* predict taken unless it is highly to be not taken 
     deduced from statistics
     and call loop unrolling
  */

  /* these two values should be initialized to 1 */
  if((lloop->ltable[loopnum].avgt < 0.5)
     &&(lloop->ltable[loopnum].avgpred < 0.5))
    lloop->ltable[loopnum].predict = NOT_TAKEN;
  else
    lloop->ltable[loopnum].predict = TAKEN;
  return SR_OK;
}

/* Routine to update branch predictor */
/* For now, have not been considered the overlap and return cases */
int lloop_update(lloop, outcome, ilptr)
LLOOP *lloop;
int outcome;
uint ilptr;
{
  int loopnum;
  uint lta;
  

  loopnum = lta % lloop->size ;
  /* Update with furthest branch that accesses this loop */
  lta = lloop->ltable[loopnum].id;
  if(ilptr > lloop->ltable[loopnum].braddr)
    lloop->ltable[loopnum].braddr = ilptr;
  lloop->ltable[loopnum].outcome = outcome;
  /* if this loop is taken it ended an iteration or begin an execution */
  lloop_stack(lloop, ilptr, lta, outcome);
  return SR_OK;
}
/* end subroutine (lloop_update) */


/* keeps loop stack */
int lloop_stack(lloop, ilptr, lta, outcome)
LLOOP *lloop;
uint ilptr;
uint lta;
int outcome;
{
  int i,ix;


  ix = lta % lloop->size;

  if (lloop->size>0)
    i = lloop->ssize - 1;

  else 
    i = 0;

#ifdef	BROKEN /* this following loop always exits immediately */

  while (i >= 0) {

    if (lloop->lstack[i].t != lta) {

      if (i>0)
	i -= 1 ;
      else {

	/* insert this loop into stack */
	lloop->lstack[i].t = lloop->ltable[ix].id = lta;
	lloop->lstack[i].b = lloop->ltable[ix].braddr = ilptr;
	lloop->ltable[ix].predict = TAKEN;
	lloop->ltable[ix].lsize = ((ilptr - lta) >> 2) + 1;
	lloop->ssize++;
	break;
      }

    } else{

      /* this loop is in the loop stack already
	 update its info */
      lloop->ltable[ix].itns++;
      
      if (outcome == lloop->ltable[ix].predict)
	lloop->ltable[ix].right_pred++;
      
	if(lloop->ltable[ix].outcome == NOT_TAKEN){
	  lloop->ltable[ix].execs++;
	  lloop->ltable[ix].avgitns += lloop->ltable[ix].itns;
	  lloop->ltable[ix].avgpred += lloop->ltable[ix].right_pred;
	  lloop->ltable[ix].avgt += lloop->ltable[ix].ttaken;
	  
     	  /* Cumulative info per execution */
	  if(lloop->ltable[ix].execs != 0){
	    lloop->ltable[ix].totavgitns += lloop->ltable[ix].avgitns;
	    lloop->ltable[ix].totavgitns = lloop->ltable[ix].totavgitns/lloop->ltable[ix].execs;
	    lloop->ltable[ix].totavgpred += lloop->ltable[ix].avgpred;
	    lloop->ltable[ix].totavgpred = lloop->ltable[ix].totavgpred/lloop->ltable[ix].execs;
	    lloop->ltable[ix].totavgt += lloop->ltable[ix].avgt;
	    lloop->ltable[ix].totavgt = lloop->ltable[ix].totavgt/lloop->ltable[ix].execs;
	    /* Clean info for next loop execution */
	    lloop->ltable[ix].avgitns=0;
	    lloop->ltable[ix].avgpred=0;
	    lloop->ltable[ix].avgt=0;
	    lloop->ltable[ix].itns =0;
	    lloop->ltable[ix].right_pred=0;
	    lloop->ltable[ix].ttaken=0;
	  }
	  else{
	    eprintf("lloop_stack: wrong number of executions for loop %x",lta);
	    return SR_FAULT;
	  }

	  /* this loop finished execution */
	  lloop_del_loop_fromstack(lloop, ilptr, lta);

 	} else{

	  lloop->ltable[ix].ttaken++;

	}

    }

    break;

  } /* end while */

#endif /* BROKEN (this loop always exits immediately !) */

  return SR_OK;
}
/* end subroutine (lloop_stack) */


int lloop_del_loop_fromstack(lloop, ilptr, lta)
     LLOOP *lloop;
     uint ilptr;
     uint lta;
{
  int ix;


  ix = lta % lloop->size;
  /* The loop that is being executed is at the top of the stack.
     therefore, if we are removing a loop from the stack
     we remove the top element
  */
  if (lloop->lstack[ix].t != lta) {
    eprintf("lloop_del_loop_fromstack: top elmnt stack mismatch" );
    return SR_FAULT;
  }
  else{
    lloop->lstack[ix].t = -1;
    lloop->lstack[ix].b = -1;
    lloop->ltable[ix].predict = -1;
    lloop->ltable[ix].lsize = -1;
  }
  lloop->ssize = lloop->ssize -1;
  return SR_OK;
}
/* end subroutine (lloop_del_loop_fromstack) */



