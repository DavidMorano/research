/* lbp */

/* Levo Branch Predictor */


#define F_DEBUGS        0               /* non-switchable */
#define F_DEBUG         1               /* switchable */
#define F_LDECODE       1               /* turn on if it can build ! */
                                                

/* revision history :

	= 00/06/17, Marcos R. de Alba

	This object module was originally written.


	= 00/06/21, Dave Morano

	I am eliminating all global data so that this block
	can be replicated as needed.


*/
	

/******************************************************************************

   File name: lbp.c
   Authors:   Marcos R. de Alba
   Date:      Jan, 4 2001 (completed last revision for initial test)
   Function:  Two Level PAP predictor for Levo
 


******************************************************************************/


#include	<sys/types.h>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"lsim.h"
#include	"levoinfo.h"
#include        "lbp.h"


/* forward references */



/* initialize this object */
int lbp_init(lbp,gp,mip,lip,size)
LBP		*lbp ;
struct proginfo	*gp ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
int		size ;
{
 	int	i,j, rs ;

	(void) memset(lbp,0,sizeof(LBP)) ;

	lbp->gp = gp ;
	lbp->mip = mip ;
	lbp->lip = lip ;
	lbp->size = size;

	/* allocate the branch history information table */
#if F_DEBUG
 eprintf("lbp_init: entered\n");
#endif 

	rs = uc_malloc(size * sizeof(uint),&lbp->bhr);

	if (rs < 0)
	  goto bad0;

#if F_DEBUG
 eprintf("lbp_init: lbp->bhr allocated\n");
#endif 	
	rs = uc_malloc(size * sizeof(uint),&lbp->btable);
	if (rs < 0)
	  goto bad1;
#if F_DEBUG
 eprintf("lbp_init: lbp->btable allocated\n");
#endif 
	for(i=0; i< size; i++) {
	  rs = uc_malloc(size * sizeof(LBP_BRANCH),&lbp->btable[i]);
    	  if (rs < 0)
	    goto bad2;
	}
#if F_DEBUG
 eprintf("lbp_init: lbptable[i] allocated\n");
#endif  
        rs = uc_malloc(size *sizeof(struct lbp_state),&lbp->c);
	if(rs<0)
	  goto bad3;
#if F_DEBUG
 eprintf("lbp_init: lbp->c allocated\n");
#endif 
	for(i=0; i<size; i++){
	  rs = uc_malloc(size * sizeof(LBP_BRANCH),&lbp->c.b[i]);
	  if (rs<0)
	    goto bad4;
	}

	rs = uc_malloc(size * sizeof(struct lbp_state),&lbp->n);
	if(rs<0)
	  goto bad5;

	for(i=0; i<size; i++){
	  rs = uc_malloc(size * sizeof(LBP_BRANCH),&lbp->n.b[i]);
	  if (rs<0)
	    goto bad6;
	}
	
	/* Have to ask Dave about this */
	/*
	for(i=0; i<size; i++){
	  lbp->c.b[i] = (LPB_BRANCH) lbp->btable[i];
	  lbp->n.b[i] = lbp->btable[i] + lbp->size ;
	}
	*/
	return SR_OK ;
	
 bad6:
	free(&lbp->n);
	
 bad5:
	for(i=0; i < size; i++)
	  free(&lbp->c.b[i]);
	
 bad4:
	free(&lbp->c);
	
 bad3:
	for(i=0; i < size; i++)
	  free(lbp->btable[i]);
	
 bad2:
	free(lbp->btable);
	
 bad1:
	free(lbp->bhr);
	
 bad0:
	return rs;

}
/* end subroutine (lbp_init) */


int lbp_free(lbp)
LBP	*lbp ;
{

  if (&lbp->c != NULL)
    free(&lbp->c) ;		/* only free the top */
 
  if (&lbp->n != NULL)
    free(lbp->n) ;              /* only free the top */   
 
  if (lbp->btable != NULL)
    free(lbp->btable) ;		/* only free the top */
  
  if (lbp->bhr != NULL)
    free(lbp->bhr);
  
  lbp->btable = NULL ;
  lbp->bhr = NULL;
  
  return SR_OK ;
}
/* end subroutine (lbp_free) */


/* handle a clock transistion */
int lbp_clock(lbp)
LBP	*lbp ;
{
  LBP_BRANCH	*btable ;
  
  /* we swap the allocated parts and copy the rest */

  btable = lbp->c.b ;	    /* save allocated pointer */
  
  lbp->c = lbp->n ;	    /* copy everything */
  
  lbp->n.b = btable ;	    /* swap in allocated pointer */
  
  return SR_OK ;
}
/* end subroutine (lbp_clock) */


/* perform the combinatorial computations */
int lbp_comb(lbp,phase)
LBP	*lbp ;
int	phase ;
{
  int	rs = SR_OK ;
  
  switch (phase) {
    
  case 0:
    lbp->f.shift = FALSE ;
    rs = lbp_newstate(lbp) ;
    break ;
    
  case 1:
    break ;
    
  case 2:
    break ;
    
  case 3:
    rs = lbp_shift(lbp);
    break ;
    
  } /* end switch */
  return rs ;
}
/* end subroutine (lbp_comb) */


/* shift the machine (called in phase 1) */
int lbp_shift(lbp)
LBP	*lbp ;
{
  lbp->f.shift = TRUE ;
  return SR_OK ;
}
/* end subroutine (lbp_shift) */

/* INTERNAL SUBROUTINES */

int lbp_newstate(lbp)
LBP	*lbp ;
{
  int	i,size ;
  
#ifdef	COMMENT
  for (i = 0 ; i < lbp->size ; i += 1)
    lbp->n.b[i] = lbp->c.b[i] ;
  
#else
  
  size = lbp->size * sizeof(LBP_BRANCH) ;
  (void) memcpy(lbp->n.b,lbp->c.b,size) ;
  
#endif
  
  return SR_OK ;
}
/* end subroutine (lbp_newstate) */

/* This is a Two-level PAP branch predictor */
int lbp_pred(lbp, ilptr, pred_val, bta)
     LBP *lbp;
     uint ilptr;
     int *pred_val;
     uint bta;
{
  uint bhridx, btableidx, outcome, *btable;
  LBP_BRANCH *bpred;

  btable = lbp->btable;
  bhridx =  ilptr % lbp->size;
  bpred = (LBP_BRANCH *) btable[bhridx];
  btableidx = (lbp->bhr[bhridx] ^ ilptr) & (lbp->size -1);
  bpred[btableidx].BTA = bta; 
  bpred[btableidx].ilptr = ilptr;
  bpred[btableidx].times += 1;


  /* Predict */
  if(bpred[btableidx].history >= 2){
    *pred_val= TAKEN;
    bpred[btableidx].predict = TAKEN;
#if F_DEBUG

eprintf("lbp_pred: TAKEN bhridx:%i btableidx:%i bta:%08lx ilptr:%08lx bpred[%i].history:%i bpred[%i].predict:%i\n",bhridx,btableidx,bta,ilptr,btableidx,bpred[btableidx].history,btableidx,bpred[btableidx].predict);

#endif
  }
  else{
    *pred_val = NOT_TAKEN;
    bpred[btableidx].predict = NOT_TAKEN;
#if F_DEBUG

eprintf("lbp_pred: NOT_TAKEN bhridx:%i btableidx:%i bta:%08lx ilptr:%08lx bpred[%i].history:%i bpred[%i].predict:%i\n",bhridx,btableidx,bta,ilptr,btableidx,bpred[btableidx].history,btableidx,bpred[btableidx].predict);

#endif
  }
  
  return SR_OK;
}

/* Routine to update branch predictor */

int lbp_update(lbp, ba, outcome)
     LBP *lbp; 
     uint ba;	
     int outcome;
{
  uint bhridx, btableidx, *btable;
  LBP_BRANCH *bpred;

  btable = lbp->btable;
  bhridx = ba % lbp->size;
  bpred = (LBP_BRANCH *) btable[bhridx];
  btableidx = (lbp->bhr[bhridx] ^ ba) & (lbp->size -1);

  if (outcome == bpred[btableidx].predict){
    lbp->misspred = 0; 
    bpred[btableidx].right_pred += 1;
    if (outcome == TAKEN){
      if (bpred[btableidx].history < 3)
        bpred[btableidx].history += 1;
      bpred[btableidx].taken += 1;
    }
    else {
     if(bpred[btableidx].history > 0)
      bpred[btableidx].history -= 1;
    }
#if F_DEBUG
    eprintf("lbp_update: branch:%08lx predicted:%d was:%d ========>CORRECT\n",ba,bpred[btableidx].predict,outcome);
    eprintf("lbp_update: bhridx:%i btableidx:%i bta:%08lx ba:%08lx bpred[%i].history:%i bpred[%i].predict:%i\n",bhridx,btableidx,bpred[btableidx].BTA,ba,btableidx,bpred[btableidx].history,btableidx,bpred[btableidx].predict);
#endif
  }               
  else{
    lbp->misspred = 1;
    if(outcome == TAKEN){
      if(bpred[btableidx].history < 3){ 
	bpred[btableidx].history++;
	if(bpred[btableidx].history > 1)
	  bpred[btableidx].predict = TAKEN; 
      }
    }
    else{
      if(bpred[btableidx].history > 0){
	bpred[btableidx].history--;
	if(bpred[btableidx].history < 2)
	  bpred[btableidx].predict = NOT_TAKEN;
      }                                                          
    }
#if F_DEBUG
    eprintf("lbp_update: branch:%08lx predicted:%d was:%d ========>INCORRECT\n",ba,bpred[btableidx].predict,outcome);
    eprintf("lbp_update: bhridx:%i btableidx:%i bta:%08lx ba:%08lx bpred[%i].history:%i bpred[%i].predict:%i\n",bhridx,btableidx,bpred[btableidx].BTA,ba,btableidx,bpred[btableidx].history,btableidx,bpred[btableidx].predict);
#endif
    
  } 
  /* Update Branch History Table for this branch */  
  lbp->bhr[bhridx] = ((lbp->bhr[bhridx] << 1) + outcome) & (lbp->size -1);
}
/* end subroutine */

