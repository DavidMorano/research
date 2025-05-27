/* lifetch.c */

/* Levo Instruction Fetch Unit */

#define	F_DEBUGS	0		/* non-switchable */
#define	F_DEBUG		1		/* switchable */
#define	F_LDECODE	1		/* turn on if it can build ! */
#define F_PRINTF        0               /* STDOUT fprintf's ? */
#define F_SAFE          1               /* extra safe mode ? */
#define F_SAFE2         0               /* extra safe mode ? */
#define F_TRACE         1
#define F_TRACE2        1
#define F_TRACE3        1
#define F_BKBRANCH      1
#define F_LSIM_MEM      1        /* Read instructions from old memory system */ 
#define F_LMEM          0        /* Invokes levo (Maryam's) memory system */
#define F_OLDLLB        0        /* Protocol to load LLB 1:OLD, 0: NEW */
#define F_PERFETCH      1        /* Simulate perfect fetch */

/* revision history :

	= xxx, Marcos de Alba

	I first put the beginning of this code together.


	= 00/09/22, Dave Morano

	I helped fix up some pointer bugs


	= 00/12/21, Marcos de Alba

	Modified lifetch_init to allocate and release memory for the
	new buffers.  Extended lifetch_fetch to support several
	(NUMLBUFFERS) load buffers.


	= 01/05/29, MARCOS DE ALBA

	THIS IS A MODIFIED VERSION OF lifetch.c TO SIMULATE PERFECT
	FETCH THE BACKUP OF THE 'COMPLEX' FETCH IS BACKED UP IN
	lifetch.c.bak, SAME APPLIES TO ITS HEADER FILE.


	= 01/07/12, Dave Morano

	I carried forward the ability to get the i-fetch trace filename
	from the parameterized initialization.  Using a fixed named
	i-fetch file was becoming problemtatic when working with many
	different MIPS program at or near the same time !


*/



#include	<sys/types.h>
#include        <assert.h>
#include	<cstdio>

#include	<vsystem.h>
#include	<bio.h>
#include	<libuc.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#include	"ldecode.h"
#include        "lbtrb.h"
#include	"lifetch.h"			/* ourselves */


/* forward references */

static int lifetch_fetch(LIFETCH *, struct proginfo *, LSIM *, struct levoinfo *) ;
static int lifetch_sanitycheck(LIFETCH *, struct proginfo *, struct levoinfo *);
static void p(LIFETCH *lifp,  uint *faddr, int ncpin, int pina, int *cpina, int pouta, int cpouta, int pinv, int *cpinv);



/* Initialize and Allocate Memory for Fetch Objects */
int lifetch_init(lifp,pip,mip,lip,ap)
LIFETCH		*lifp ;
struct proginfo	*pip ;
LSIM		*mip ;
struct levoinfo	*lip ;
LIFETCH_INITARGS  *ap;
{
  uint exitaddr;

  int   i,j,rs, size;

	char	*cp ;

  
  if (lifp == NULL)
    return SR_FAULT ;
  
  (void) memset(lifp,0,sizeof(LIFETCH));
  
  lifp->pip = pip ;	
  lifp->mip = mip ;
  lifp->lip = lip ;
  lifp->llbp = lip->llbp ;
  lifp->index =0;
  /* Initialize the Window Size, here this refers to Fetch Window */
  
  lifp->rfbuses = ap->rfbuses;
  lifp->lases = ap->lases;
  
#if F_DEBUG
  if (pip->debuglevel > 1)
    eprintf("lifetch_init: LASes=%08lx\n", lifp->lases);
#endif
  lifp->winsize = lip->totalrows * lip->nsgcols;  
#if	F_DEBUG
  if (pip->debuglevel > 1)
    eprintf("lifetch_init: entered\n") ;
#endif
  
  lifp->ifetchwidth = lip->ifetchwidth ;
 
  lifp->fetchaddr = ap->startaddr ;
#if F_DEBUG 
  eprintf("lifetch_init: ifetchwidth=%d\n",lifp->ifetchwidth) ;
  eprintf("lifetch_init: ap->startaddr: 0x%08lx lifp->fetchaddr:0x%08lx\n",ap->startaddr, lifp->fetchaddr) ;
#endif 
  if (lsim_getsymval(lifp->mip,"exit",&exitaddr)>=0){
  
    lifp->exitaddr = exitaddr ;
#if F_DEBUG
    eprintf("EXIT  exitaddr: %08lx\n",exitaddr);
#endif
  }
  else
    assert(0);
  
  /* allocate space */
  
  size = lifp->winsize * sizeof(unsigned int);
  rs = uc_malloc(size, (void **) &lifp->inwindow);
  if (rs < 0)
    goto bad0;
  
  (void) memset(&lifp->c, 0, sizeof(struct lifetch_state));
  (void) memset(&lifp->n, 0, sizeof(struct lifetch_state));
  
  rs = uc_malloc(lifp->lip->totalrows * sizeof(LDECODE),	(void **) &lifp->buffer);
  if (rs < 0)
    goto bad1 ;

#if !(F_PERFETCH)
#if F_DEBUG 
  eprintf("lifetch_init: lifp->buffer initialized \n");
#endif
  rs = lloop_init(&lifp->loops, pip, mip, lip,  LOOPTABLESIZE);
  if (rs < 0)
    goto bad2 ;
 #if F_DEBUG
  eprintf("lifetch_init: lloop_init completed\n");
#endif 
  rs = lbp_init(&lifp->lbpred, pip, mip, lip, LBP_SIZE);
  if (rs < 0)
    goto bad3;
#if F_DEBUG
  eprintf("lifetch_init: lbp_init completed\n");
#endif
  rs = ltcache_init(&lifp->ltcache, pip, mip, lip, LTCACHE_SIZE);
  if (rs < 0)
    goto bad4 ;
#if F_DEBUG
  eprintf("lifetch_init: ltcache_init completed\n");
#endif 
  rs = lcallstack_init(&lifp->lstack, pip, mip, lip, LCALLSTACK_SIZE);
  if (rs < 0)
    goto bad5 ;
#endif
  
#if F_PERFETCH
  /* Read trace file: bzip2.trace to get addresses of instructions for perfect fetch */

	if ((cp = ap->ifetchtrace) == NULL)
		cp = "addr.trace" ;

  lifp->tracefile = fopen(cp,"r");

  assert(lifp->tracefile != NULL);

  /* Get trace file size */
  lifp->fsize=lseek(lifp->tracefile->_file,0,SEEK_END);
    
  /* Re-position file descriptor to beginning of trace file */
  lseek(lifp->tracefile->_file,-lifp->fsize,SEEK_CUR);

  /* set trace index to end of buffer to force a read from the trace file */
  lifp->traceidx = BUFSIZ;
  eprintf("lifetch_init: tracefile opened\n") ;
#endif
  
#ifdef	COMMENT
  lifp->index = (ap->startaddr >> 2) ;
#else
  lifp->index = 0 ;
#endif
  
#if	F_DEBUG
  if (pip->debuglevel > 1)
    eprintf("lifetch_init: exiting OK rs=%d\n",rs) ;
#endif
  
  return rs;
  
  /* bad things */
 bad6:
  lcallstack_free(&lifp->lstack);
 bad5:
  ltcache_free(&lifp->ltcache); 
 bad4:
  lbp_free(&lifp->lbpred);
 bad3:
  lloop_free(&lifp->loops);
 bad2:
  free(lifp->buffer);
 bad1:
  free(lifp->inwindow);
 bad0:
  return rs ;
}
/* end subroutine (lifetch_init) */


/* free up this object */
int lifetch_free(lifp)
LIFETCH	*lifp ;
{
  int i;
  struct proginfo	*pip ;
  
  if (lifp == NULL)
    return SR_FAULT ;
  
  pip = lifp->pip ;
  
  /* printf("lifetch_free: entered\n") ;*/
  
#if	F_DEBUG
  if (pip->debuglevel > 1)
    eprintf("lifetch_free: entered\n") ;
#endif
  
  if (lifp->buffer != NULL)
    free(lifp->buffer) ;

  if (lifp->inwindow != NULL)
    free(lifp->inwindow) ;
     
#if	F_DEBUG
  if (pip->debuglevel > 1)
    eprintf("lifetch_free: exiting\n") ;
#endif
  
#if F_PERFETCH
  fclose(lifp->tracefile);
#endif
  return SR_OK ;
}
/* end subroutine (lifetch_free) */


/* Fetch a number of instructions equal to the number of rows every cycle */
int lifetch_comb(lifp,phase)
LIFETCH	*lifp ;
int	phase ;
{
  struct proginfo	*pip ;
  LSIM  *mip;
  struct levoinfo *lip;
  
  int		addr = 0 , rs;
  
  pip = lifp->pip ;
  mip = lifp->mip;
  lip = lifp->lip;

#if	F_DEBUG
  if (pip->debuglevel > 1)
    eprintf("lifetch_comb: entered phase: %d\n",phase);
#endif
  
  switch (phase) {
  case 2:
    rs = lifetch_fetch(lifp,pip,mip,lip) ;
    break ;
  } /* end switch */
    return SR_OK ;
}
/* end subroutine (lifetch_comb) */


/* do a clock transistion */
int lifetch_clock(lifp)
     LIFETCH	*lifp ;
{
  struct proginfo	*pip ;
  /* printf("lifetch_clock: entered\n") ; */
  
  pip = lifp->pip ;
  
#if	F_DEBUG
  if (pip->debuglevel > 1)
    eprintf("lifetch_clock: entered\n") ;
#endif
  
  lifp->c = lifp->n;
    
  return SR_OK ;
}
/* end subroutine (lifetch_clock) */

/* Convert Backward Branches to Forward */
int lifetch_cv_bbr_to_fbr(instr, utimes, fbpos)
LDECODE *instr;
int utimes;
int fbpos;
{
  int bodysize;
  
  switch(instr[fbpos].opexec){
  case LEXECOP_BEQ:
    instr[fbpos].opexec = LEXECOP_BNE;
    break;
  case LEXECOP_BNE:
    instr[fbpos].opexec = LEXECOP_BEQ;
    break;
  case LEXECOP_BGTZ:
    instr[fbpos].opexec = LEXECOP_BLEZ;
      break;
  case LEXECOP_BLEZ:
    instr[fbpos].opexec = LEXECOP_BGTZ;
    break;
  case LEXECOP_BEQL:
    instr[fbpos].opexec = LEXECOP_BNEL;
    break;
  case LEXECOP_BNEL:
    instr[fbpos].opexec = LEXECOP_BEQL;
    break;
  case LEXECOP_BGTZL:
    instr[fbpos].opexec = LEXECOP_BLEZL;
    break;
  case LEXECOP_BLEZL:
    instr[fbpos].opexec = LEXECOP_BGTZL;
    break;
  case LEXECOP_BGEZ:
    instr[fbpos].opexec = LEXECOP_BLTZ;
    break;
  case LEXECOP_BGEZAL:
    instr[fbpos].opexec = LEXECOP_BLTZAL;
    break;
  case LEXECOP_BLTZAL:
    instr[fbpos].opexec = LEXECOP_BGEZAL;
    break;
  case LEXECOP_BLTZALL:
    instr[fbpos].opexec = LEXECOP_BGEZALL;
    break;
  case LEXECOP_BGEZL:
    instr[fbpos].opexec = LEXECOP_BLTZL;
    break;
  case LEXECOP_BGEZALL:
    instr[fbpos].opexec = LEXECOP_BLTZALL;
    break;
  case LEXECOP_BC0F:
    instr[fbpos].opexec = LEXECOP_BC0T;
    break;
  case LEXECOP_BC0T:
    instr[fbpos].opexec = LEXECOP_BC0F;
    break;
  case LEXECOP_BC1F:
    instr[fbpos].opexec = LEXECOP_BC1T;
    break;
  case LEXECOP_BC0FL:
    instr[fbpos].opexec = LEXECOP_BC0TL;
    break;
  case LEXECOP_BC0TL:
    instr[fbpos].opexec = LEXECOP_BC0FL;
    break;
  case LEXECOP_BC1T:
    instr[fbpos].opexec = LEXECOP_BC1F;
    break;
  case LEXECOP_BC1FL:
    instr[fbpos].opexec = LEXECOP_BC1TL;
    break;
  case LEXECOP_BC1TL:
    instr[fbpos].opexec = LEXECOP_BC1FL;
    break;
  }
  bodysize = (fbpos - ((instr[fbpos].ilptr - instr[fbpos].const1) >> 2)) +1; 
  /* New target address. */
  instr[fbpos].const1 = (((utimes * bodysize) + 1) << 2) + instr[fbpos].ilptr;
  
  return SR_OK;
}


/* (Pseudo) Dynamic Loop Unrolling */
int lifetch_unroll_loop(lifp, from, bodysize, utimes, predval, faddr)
LIFETCH *lifp;
int from;
int bodysize;
int utimes;
int predval;
uint *faddr;
{
  int dyntaken, j, k, loopnum, rs, pval, pina, ncpin, pouta, cpouta, pinv, cpina[100], cpinv[100];
  struct lbtrb_predicates pr;
  uint faddr_backup;
  
  /* loopnum is used in the loop predictor */
  loopnum =   lifp->buffer[lifp->index].const1 % lifp->loops.size ;
  
  /* Convert backward to forward branch */
  /*  lifetch_cv_bbr_to_fbr(lifp->buffer, utimes, lifp->index); */

  for(j=0; j<utimes; j++){ 
    faddr_backup = *faddr;
    for(k=from; k <= lifp->index; k++){
      /* Assign predicate values */
      /* WARNING:  I have to verify that the right predicted value is assigned 
	 to each branch, in case there are some nested inside the loop */
      if((lifp->buffer[k].opclass != INSTR_BREL) &&
	 (lifp->buffer[k].opclass != INSTR_JIND) &&
	 (lifp->buffer[k].opclass != INSTR_JREL)){
	pval = -1;
	dyntaken = 0;
      }
      else{
	pval = predval;
	dyntaken = 1;
      }
      lbtrb_assign_predicate(lifp->lip->lbtrbp, lifp->buffer[k].opclass, lifp->buffer[k].ilptr, lifp->buffer[lifp->index].const1, pval, &pr) ;
      pina = pr.ipaddr;
      ncpin = pr.numicp;
      
      /* have to check this with Ali */
      for(j = 0 ; j < MIN(ncpin, 100); j += 1)
	cpina[j] = pr.icpaddr[j];
      pouta = pr.opaddr;
      cpouta = pr.ocpaddr;
      pinv = pr.ipval == -1 ? 1 : pr.ipval;
      /* I am not complementing the prediction because I did not
         complement the branch */
      /* debo corregir esto?, revisar bien que se carguen las
	 instrucciones debidas */
      for(j = 0 ; j < MIN(ncpin, 100); j += 1)
	cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j]; 
      if(lifp->index < lifp->lip->totalrows){
	/* Tambien debo verificar que no se desenrollen branches y
	   que no queden en la ultima posicion de la columna */
	if((lifp->index == lifp->lip->totalrows - 1) &&
	   (lifp->buffer[k].opclass == INSTR_JIND ||
	    lifp->buffer[k].opclass == INSTR_BREL || 
	    lifp->buffer[k].opclass == INSTR_JREL)) {
	  lifp->buffer[k].opclass = INSTR_UNUSED;		
	  faddr_backup -= 4 ;
	  k -= 1;
	}
	lifp->inwindow[lifp->index] = faddr_backup;
	rs = llb_load(lifp->llbp, lifp->index, faddr_backup, lifp->buffer[k].instrword, lifp->buffer[k].opexec, lifp->buffer[k].opclass, lifp->buffer[k].opexec, lifp->buffer[k].opclass, lifp->buffer[k].btype, lifp->buffer[k].src1, lifp->buffer[k].src2, lifp->buffer[k].src3, lifp->buffer[k].src4, lifp->buffer[k].dst1, lifp->buffer[k].dst2, lifp->buffer[k].const1, lifp->buffer[k].const1_valid,    pina,   cpina,   ncpin,   pouta,   cpouta,   pinv, cpinv, dyntaken);
	if(rs<0){	
#if	F_DEBUG
	  printf("\nlifetch_unroll_loop: error loading llb ");
#endif
	  assert(0);
	}
#if F_TRACE
	eprintf("INSTR %6lx  %08lx   LOOP BRANCH\n",faddr_backup,lifp->buffer[k].instrword);
#endif
	/* fetch address of original instruction is repeated for different iterations of a loop */
	faddr_backup += 4;
	if(lifp->index == lifp->lip->totalrows -1){
	  lifp->index = 0;
	  /* load this column and continue */
	  llb_load_complete(lifp->llbp);
	}
      }
      else{	
#if	F_DEBUG
	eprintf("lifetch_unroll_loop: invalid load buffer index ");
#endif
	return SR_FAULT;
      }
      lifp->index +=1;
    }
  }
  return SR_OK;
}


/* Update Branch Predictors */
int lifetch_bu(lifp, ba, btype, bc, bta)
LIFETCH *lifp;   /* The fetch structure */
uint ba;       /* Commited branch address */
uint btype;   
int bc;       /* Outcome of the branch */
uint bta;	
{

#if F_DEBUG
  eprintf("lifetch_bu: BRANCH:%08lx  BR_TARGET:%08lx  BTYPE:%i  CONDITION:%i\n",ba,bta,btype,bc);
#endif
  switch(btype){
  case FORWARD:
    /* Update two level branch predictor */
    lbp_update(&lifp->lbpred, ba, bc);	
    break;     
  case BACKWARD:
    /* Update loop predictor */
    lloop_update(&lifp->loops, bc, ba);
    break;
  case INSTR_JIND:
    /* Update Indirect Branch Predictor (target cache) */
    ltcache_update(&lifp->ltcache, ba, bta);
    break;
  case RETURN_TYPE:
    /* do nothing, assumed that the predicted address was correct */
    break;
  }
  lifp->f_br_updated = 1;
  
  /* Have to add failure cases */
  return SR_OK;
}


/* Check Load Buffers Status */
int lifetch_loadstatus(lifp, sw)
LIFETCH *lifp;
int sw;
{
  int i, j, accum;
  /* This code will work when there are available many load buffers,
     for now use the simple approach */
  /*
    for(i=0; i<NUMLBUFFERS; i++){
    for(j=0;j<=i;j++){
    if(j==0)
    accum = 1;
    else
    accum *= 2;
    }
    lifp->lbstatus[i]= (sw & accum) >> i;
    }
    */
  /* lifp->n.f.loadit = ((sw & 1) != 0); */
  return SR_OK;
}


/* 
   Handle Conditional Branches.
   It includes forward and backward branches  */
int lifetch_condbranch(lifp, faddr, lip)
LIFETCH *lifp;
uint *faddr;
struct levoinfo	*lip ;
{
  int brnum, j, rs;
  int loopnum, TAindex, bodysize, utimes;
  int predval, pina, ncpin, pouta, cpouta, pinv, cpinv[100], cpina[100];
  float threshold = 0.666;
  struct lbtrb_predicates pr;
   
#if	F_DEBUG
  eprintf("\nlifetch_condbranch: conditional branch:0x%08lx\n",lifp->buffer[lifp->index].instrword);
#endif
  
  if(lifp->buffer[lifp->index].ilptr > lifp->buffer[lifp->index].const1){
#if	F_BKBRANCH
    eprintf("lifetch_condbranch: backward branch:0x%08lx ilptr:0x%08lx BTA:0x%08lx\nPredicting loop...\n",lifp->buffer[lifp->index].instrword, lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1);
#endif
    lifp->buffer[lifp->index].btype = BACKWARD;
    /* Predict this backward branch */
    lloop_pred(&lifp->loops, lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1); 
    loopnum = lifp->buffer[lifp->index].const1 % lifp->loops.size;
    /* TAindex holds the index in fetch buffer of the TA */
    TAindex = lifp->index - ((lifp->buffer[lifp->index].ilptr - lifp->buffer[lifp->index].const1) >> 2); 
    bodysize = (lifp->index - TAindex) + 1;	
#if	F_BKBRANCH
	  eprintf("lifetch_condbranch: backward loop was predicted\n");
	  eprintf("lifetch_condbranch: loopnum:%i lifp->index:%i TAindex:%i bodysize:%i threshold:%f lifp->lip->totalrows:%i threshold*lifp->lip->totalrows:%i\n",loopnum,lifp->index,TAindex,bodysize,threshold, lifp->lip->totalrows,(int)(threshold*lifp->lip->totalrows));
#endif
    if(TAindex > 0){
      if(bodysize <= (int) (threshold * lifp->lip->totalrows )){
#if	F_BKBRANCH
	eprintf("lifetch_condbranch: loop_capture\n");
#endif
	/* For now I am only replicating the code and tranforming backward to forward branches. I am not elimintating the branches when unrolling since it would be difficult to compare at run time how many times do the branch was taken? */
	predval = lifp->loops.ltable[loopnum].predict;
	if(predval == TAKEN){
	  utimes = (int)((threshold * lifp->lip->totalrows) - lifp->index)/bodysize;
#if	F_DEBUG
	  eprintf("lifetch_condbranch: Backward Predicted TAKEN unrolling loop\n");
#endif
	  /* backup next instruction address and unroll loop */
	  lifp->nextinstr = *faddr + 4;
	  *faddr = lifp->buffer[lifp->index].const1;
	  lifetch_unroll_loop(lifp, TAindex, bodysize, utimes, predval, faddr);
	  *faddr = lifp->nextinstr;
	}/* TAKEN */
	else{
	  utimes = 1;	
#if	F_BKBRANCH
	  eprintf("lifetch_condbranch: Backward Predicted NOT TAKEN unrolling once (branch conversion)\n");
#endif
	  /* backup next instruction address and unroll loop */
	  lifp->nextinstr = *faddr + 4; 
	  *faddr = lifp->buffer[lifp->index].const1;
	  /* The NOT TAKEN case is like unrolling once */
	  lifetch_unroll_loop(lifp, TAindex, bodysize, utimes, predval, faddr);
	  *faddr = lifp->nextinstr; 
	}/* NOT TAKEN */
      }/* bodysize <= (threshold * lifp->lip->totalrows )*/
      else
	{
	  /* Can't unroll this loop. The bodysize is larger than available space in fetch window.
	     For now, refetch instructions from target address. Later can be fixed to store the body of
	     the loop in an auxiliar buffer and then when load buffer has free space copy the body
	     to the load buffer. */	
	  if(predval == TAKEN){ 	
#if	F_BKBRANCH
	  eprintf("lifetch_condbranch: Backward Predicted TAKEN body loop larger than fetch window\n");
#endif
	    /* set predicates */
	  lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1,predval,&pr);
	    pina = pr.ipaddr;
	    ncpin = pr.numicp;
	    for(j = 0 ; j < MIN(ncpin,100); j += 1)
	      cpina[j] = pr.icpaddr[j];
	    pouta = pr.opaddr;
	    cpouta = pr.ocpaddr;
	    pinv = pr.ipval == -1 ? 1 : pr.ipval;
	    for(j = 0 ; j < MIN(ncpin,100); j += 1)
	      cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j];
	    lifp->inwindow[lifp->index] = *faddr;
	    rs = llb_load(lifp->llbp, lifp->index, *faddr, lifp->buffer[lifp->index].instrword,  lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].btype, lifp->buffer[lifp->index].src1,  lifp->buffer[lifp->index].src2, lifp->buffer[lifp->index].src3, lifp->buffer[lifp->index].src4, lifp->buffer[lifp->index].dst1, lifp->buffer[lifp->index].dst2, lifp->buffer[lifp->index].const1, lifp->buffer[lifp->index].const1_valid,  pina, cpina, ncpin, pouta, cpouta, pinv, cpinv, TAKEN);
	    if (rs < 0){ 	
#if	F_BKBRANCH
	      eprintf("lifetch_condbranch: backward error loading LLB\n");
#endif
	      assert(0);
	    }
#if F_TRACE
	    eprintf("INSTR %6lx  %08lx    COND_BACKWARD\n",*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_BKBRANCH
	    eprintf("lifetch_condbranch: backward \tidx:%i  %08lx  %08lx\n",lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
	    lifp->btarget = lifp->buffer[lifp->index].const1;
	    *faddr += 4;
	    lifp->f_loaddelayslot = *faddr;
	  } /* predval == TAKEN */
	  else{
	    /* NOT TAKEN */    	
#if	F_BKBRANCH
	  eprintf("lifetch_condbranch: Backward Predicted NOT TAKEN body loop larger than fetch window\n");
#endif 
	    /* set predicates */
	    lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1,predval,&pr);
	    pina = pr.ipaddr;
	    ncpin = pr.numicp;
	    for(j = 0 ; j < MIN(ncpin,100); j += 1)
	      cpina[j] = pr.icpaddr[j];
	    pouta = pr.opaddr;
	    cpouta = pr.ocpaddr;
	    pinv = pr.ipval == -1 ? 1 : pr.ipval;
	    for(j = 0 ; j < MIN(ncpin,100); j += 1)
	      cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j];
	    lifp->inwindow[lifp->index] = *faddr;
	    rs = llb_load(lifp->llbp, lifp->index, *faddr, lifp->buffer[lifp->index].instrword,  lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].btype, lifp->buffer[lifp->index].src1,  lifp->buffer[lifp->index].src2, lifp->buffer[lifp->index].src3, lifp->buffer[lifp->index].src4, lifp->buffer[lifp->index].dst1, lifp->buffer[lifp->index].dst2, lifp->buffer[lifp->index].const1, lifp->buffer[lifp->index].const1_valid,  pina, cpina, ncpin, pouta, cpouta, pinv, cpinv, NOT_TAKEN);
	    if (rs < 0){ 	
#if	F_BKBRANCH
	      eprintf("lifetch_condbranch: backward error loading LLB\n");
#endif
	      assert(0);
	    }
#if F_BKBRANCH
	    eprintf("lifetch_condbranch: backward \tidx:%i  %08lx  %08lx\n",lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_TRACE
	    eprintf("INSTR %6lx  %08lx COND_BACKWARD\n",*faddr,lifp->buffer[lifp->index].instrword);
#endif
	    *faddr += 4;
	  }
#if	F_DEBUG
	  eprintf("lifetch_condbranch: Backward unknown case search how to handle it\n");
#endif
	}
    }/* TAindex > 0*/
    else{
      /* Backward target OOB Re-Fetch instructions  from TA This case can be improved only refetching instructions from TA until the beginning of the fetch buffer but after re-fetching again have to be decoded and verified the types of the instructions */ 
#if	F_BKBRANCH
      eprintf("lifetch_condbranch: loop target outside fetch buffer!. Have to re-fetch\n instr:0x%x ilptr:0x%08lx #BTA:0x%08lx\n",lifp->buffer[lifp->index].instrword,lifp->buffer[lifp->index].ilptr,lifp->buffer[lifp->index].const1);
#endif
      /* set predicates */
      lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1,predval,&pr) ;
      pina = pr.ipaddr;
      ncpin = pr.numicp;
      for(j = 0 ; j < MIN(ncpin,100); j += 1)
	cpina[j] = pr.icpaddr[j];
      pouta = pr.opaddr;
      cpouta = pr.ocpaddr;
      pinv = pr.ipval == -1 ? 1 : pr.ipval;
      for(j = 0 ; j < MIN(ncpin,100); j += 1)
	cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j];
      lifp->inwindow[lifp->index] = *faddr;
      rs = llb_load(lifp->llbp,lifp->index,*faddr,lifp->buffer[lifp->index].instrword,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].btype,lifp->buffer[lifp->index].src1,lifp->buffer[lifp->index].src2, lifp->buffer[lifp->index].src3,lifp->buffer[lifp->index].src4, lifp->buffer[lifp->index].dst1,lifp->buffer[lifp->index].dst2,lifp->buffer[lifp->index].const1,lifp->buffer[lifp->index].const1_valid,pina,cpina,ncpin,pouta,cpouta,pinv,cpinv, TAKEN);
      if (rs < 0){ 	
#if	F_DEBUG
	eprintf("lifetch_condbranch: error loading llb at backward branch");
#endif
	assert(0);
      }
#if F_DEBUG
      eprintf("lifetch_condbranch: \tidx:%i  %08lx  %08lx\n",lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_TRACE
      eprintf("INSTR %6lx  %08lx COND_BACKWARD OOB\n",*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_DEBUG
    eprintf("lifetch_condbranch: llb_loaded backward branch\n");
#endif
    /* determinar faddr de acuerdo a prediccion */
    if(predval == TAKEN){
      *faddr += 4;
      lifp->f_loaddelayslot = *faddr;
      lifp->btarget = lifp->buffer[lifp->index].const1; 
    }
    else
      *faddr += 4;
    
#if F_BKBRANCH
      eprintf("lifetch_condbranch: backward OOB\n"); 
#endif
    }
  } /* backward target */
  else{
    /* Forward Branch */
#if	F_TRACE
    eprintf("lifetch_condbranch:forward branch instruction ilptr:%08lx  %08lx  BTA:%08lx\n",lifp->buffer[lifp->index].ilptr,lifp->buffer[lifp->index].instrword,lifp->buffer[lifp->index].const1);
#endif
    lifp->buffer[lifp->index].btype = FORWARD;
    /* Predict this branch */
    brnum =  lifp->buffer[lifp->index].ilptr % lifp->lbpred.size;
#if	F_TRACE2
    eprintf("lifetch_condbranch: brnum:%i = lifp->buffer[lifp->index].ilptr mod lifp->brpredsize\nlifetch_condbranch: Predicting branch\n",brnum);
#endif
    lbp_pred(&lifp->lbpred, *faddr, &predval, lifp->buffer[lifp->index].const1);
#if	F_TRACE2
    eprintf("lifetch_condbranch: predicted value for this branch:%i\n",predval);
#endif

    /* set predicates */
    
    lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1,predval,&pr) ;
    pina = pr.ipaddr;
    ncpin = pr.numicp;
    for(j = 0 ; j < MIN(ncpin,100); j += 1)
      cpina[j] = pr.icpaddr[j];
    pouta = pr.opaddr;
    cpouta = pr.ocpaddr;
    pinv = pr.ipval == -1 ? 1 : pr.ipval;
    for(j = 0 ; j < MIN(ncpin,100); j += 1)
      cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j];
      lifp->inwindow[lifp->index] = *faddr;
    rs = llb_load(lifp->llbp,lifp->index,*faddr,lifp->buffer[lifp->index].instrword,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].btype,lifp->buffer[lifp->index].src1,lifp->buffer[lifp->index].src2, lifp->buffer[lifp->index].src3,lifp->buffer[lifp->index].src4, lifp->buffer[lifp->index].dst1,lifp->buffer[lifp->index].dst2,lifp->buffer[lifp->index].const1,lifp->buffer[lifp->index].const1_valid,pina,cpina,ncpin,pouta,cpouta,pinv,cpinv,predval);
    if (rs < 0){ 	
#if	F_DEBUG
      eprintf("lifetch_condbranch: error loading llb at forward branch");
#endif
      assert(0);
    }
#if F_DEBUG
	    eprintf("lifetch_condbranch: \tidx:%i  %08lx  %08lx\n",lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_TRACE
	    eprintf("INSTR %6lx  %08lx COND_FORWARD\n",*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_DEBUG
    eprintf("lifetch_condbranch: llb_loaded forward branch\n");
#endif
    if(predval == TAKEN){
      if(lifp->buffer[lifp->index].const1 > *faddr + (int) (lifp->lip->totalrows *threshold *4)){
	/* If this branch is OOB follow it, but first load the instruction
	   in the delay slot */
	*faddr += 4;
	lifp->f_loaddelayslot = *faddr;
	lifp->btarget = lifp->buffer[lifp->index].const1;
      }
      else
	*faddr += 4;
    }
    else
      *faddr += 4;
  }/* forward branch */
  return SR_OK;
} /* lifetch_condbranch */


/* Handle Jump Instructions */
int lifetch_jump(lifp, faddr, lip)
LIFETCH *lifp;
uint *faddr;
struct levoinfo	*lip ;
{
  int j,rs, pina, ncpin, pouta, cpouta, pinv, cpinv[100], cpina[100];
  float threshold = 2/3;
  struct lbtrb_predicates pr;
  
  lifp->buffer[lifp->index].btype = INSTR_JREL;	
#if	F_DEBUG
  eprintf("lifetch_jump: Direct Jump or Subroutine Call\n");
#endif
  /* A subroutine call */
  if((lifp->buffer[lifp->index].opexec == LEXECOP_JAL) &&
     (lifp->buffer[lifp->index].dst1 == 31)){
    /*
      lcallstack_search_table(&lifp->lstack, lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1, 0);
      */
    /* The return address is the adress after the delay slot */
    /* Tengo que asegurarme de usar el metodo propuesto por Prof Kaeli. Ya hice el codigo solo debo
     identificar la rutina correspondiente */
    lcallstack_push(&lifp->lstack, *faddr + 8);
  }
  
  lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass, 
			 *faddr, lifp->buffer[lifp->index].const1,TAKEN,&pr) ;
  pina = pr.ipaddr;
  ncpin = pr.numicp;
  for(j = 0 ; j <  MIN(ncpin,100); j += 1)
    cpina[j] = pr.icpaddr[j];
  pouta = pr.opaddr;
  cpouta = pr.ocpaddr;
  
  pinv = pr.ipval == -1 ? 1 : pr.ipval;
  for(j = 0 ; j <  MIN(ncpin,100); j += 1)
    cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j]; 
  lifp->inwindow[lifp->index] = *faddr;
  rs = llb_load(lifp->llbp,lifp->index, *faddr,lifp->buffer[lifp->index].instrword,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].btype,lifp->buffer[lifp->index].src1, lifp->buffer[lifp->index].src2,lifp->buffer[lifp->index].src3,lifp->buffer[lifp->index].src4,lifp->buffer[lifp->index].dst1,lifp->buffer[lifp->index].dst2,lifp->buffer[lifp->index].const1,lifp->buffer[lifp->index].const1_valid, pina,cpina,ncpin,pouta,cpouta,pinv,cpinv,TAKEN);
  if (rs < 0){ 	
#if	F_DEBUG
      eprintf("lifetch_jump: error loading llb at direct jump");
#endif
    assert(0);
  }
 
#if F_DEBUG
  eprintf("lifetch_jump: llb_loaded direct jump\n");
  eprintf("lifetch_jump:\t\tidx:%i  %08lx  %08lx\n",lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_TRACE
  eprintf("INSTR %6lx  %08lx   DIRECT JUMP OR CALL\n",*faddr,lifp->buffer[lifp->index].instrword);
#endif
  /*  if(lifp->buffer[lifp->index].const1 > *faddr + (int) (lifp->lip->totalrows *threshold*4)){*/
  *faddr += 4;
  lifp->f_loaddelayslot = *faddr;
#if F_DEBUG
  eprintf("lifetch_jump: lifp->f_loaddelayslot:%08lx\n",lifp->f_loaddelayslot);
#endif
  lifp->btarget = lifp->buffer[lifp->index].const1;
  /*  {
      else
      *faddr +=4;
      */
  return SR_OK;
}


/* Handle Indirect Jumps */
int lifetch_indjump(lifp, faddr, lip)
LIFETCH *lifp;
uint *faddr;
struct levoinfo	*lip ;
{
  int j, idx, rs, retaddr;
  int predval, pina, ncpin, pouta, cpouta, pinv, cpinv[100], cpina[100];
  struct lbtrb_predicates pr;
  
  if((lifp->buffer[lifp->index].opexec == LEXECOP_JR) 
     &&(lifp->buffer[lifp->index].src1 == 31))
	lifp->buffer[lifp->index].btype = RETURN_TYPE;
      else
	lifp->buffer[lifp->index].btype = INSTR_JIND;	
#if	F_DEBUG
  eprintf("lifetch_indjump: Indirect jump \n");
#endif
  /* Predict this indirect jump */
  ltcache_pred(&lifp->ltcache, *faddr);
  idx = (*faddr ^ lifp->ltcache.aux) & (lifp->ltcache.size - 1);
#if	F_DEBUG
  eprintf("lifetch_indjump: indirect jump at last pos in column \n");
  eprintf("lifetch_indjump: ind jump, lifp->buffer[%d].opexec:%08lx lifp->buffer[%d
].src1:%08lx\n",lifp->index,lifp->buffer[lifp->index].opexec, lifp->index,lifp->buffer[lifp->index].src1);
#endif	
 /* The direction of the prediction is dependent upon the target address */
  if(lifp->ltcache.paddr == *faddr + 4)
    predval = 1;
  else
    predval = 0;
  lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].ilptr,lifp->ltcache.paddr, predval,&pr);
  pina = pr.ipaddr;
  ncpin = pr.numicp;
  for(j = 0 ; j < MIN(ncpin,100) ; j += 1)
    cpina[j] = pr.icpaddr[j];
  pouta = pr.opaddr;
  cpouta = pr.ocpaddr;/* Branch predictor would assign initial predicate values */
  pinv = pr.ipval == -1 ? 1 : pr.ipval;
  for(j = 0 ; j < MIN(ncpin,100) ; j += 1)
    cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j] ;
#if	F_DEBUG
  eprintf("lifetch_indjump: ind jump predicates assigned \n");
#endif
  lifp->inwindow[lifp->index] = *faddr;
  rs = llb_load(lifp->llbp, lifp->index, *faddr,lifp->buffer[lifp->index].instrword,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].btype,lifp->buffer[lifp->index].src1, lifp->buffer[lifp->index].src2,lifp->buffer[lifp->index].src3,lifp->buffer[lifp->index].src4,lifp->buffer[lifp->index].dst1,lifp->buffer[lifp->index].dst2,lifp->ltcache.paddr,lifp->buffer[lifp->index].const1_valid, pina,cpina,ncpin,pouta,cpouta,pinv,cpinv,predval);
  if (rs < 0){ 	
#if	F_DEBUG
      eprintf("lifetch_indjump: error loading llb at indirect jump");
#endif
    assert(0);
  }
#if F_TRACE
	    eprintf("INSTR %6lx  %08lx   IND JUMP\n",*faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_DEBUG
    eprintf("lifetch_indjump: ind jump loaded \n");
  eprintf("lifetch_indjump: ind jump, lifp->buffer[%d].opexec:%08lx lifp->buffer[%d].src1:%08lx\n",lifp->index,lifp->buffer[lifp->index].opexec, lifp->index,lifp->buffer[lifp->index].src1);
#endif
  /* Is this a return from subroutine ? */
  if((lifp->buffer[lifp->index].opexec == LEXECOP_JR) 
     &&(lifp->buffer[lifp->index].src1 == 31))
    {
#if	F_DEBUG
    	eprintf("lifetch_indjump: return from subroutine \n");
#endif
      /* Predict the next address getting it from the stack */
      lcallstack_pop(&lifp->lstack, &retaddr);
      *faddr += 4; 
      lifp->f_loaddelayslot = *faddr;
      lifp->btarget = retaddr;
      /*  lcallstack_search_table(lifp->lstack, lifp->buffer[lifp->index].const1 */
    }
  else{
    /* If there is a minimum of history for this branch with no misses then predict fetch address as target cache predicted address for this branch */
    if((lifp->ltcache.table[idx].times > 0)
       &&(lifp->ltcache.table[idx].misses == 0)){
      lifp->btarget = lifp->ltcache.paddr;
      *faddr += 4;
      lifp->f_loaddelayslot = *faddr;
    }
    else
      *faddr += 4;
  }
#if F_DEBUG
  eprintf("lifetch_indjump: leaving, next instruction faddr:%08lx\n",*faddr);
#endif
  
  return SR_OK;
} /* lifetch_indjump */
 
 
/* Handle non-branch instructions */
int lifetch_nonbranch(lifp, faddr, lip)
LIFETCH *lifp;
uint *faddr;
struct levoinfo	*lip ;
{
  int j, rs, pina, ncpin, pouta, cpouta, pinv, cpinv[100], cpina[100];
  struct lbtrb_predicates pr;
  
#if	F_DEBUG
  eprintf("lifetch_nonbranch: non-branch instruction:0x%08lx ilptr: 0x%08lx\n", lifp->buffer[lifp->index].instrword, *faddr);
  eprintf("lifetch_nonbranch: non-branch instr, setting predicates\n");
#endif
  lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass, 
			 *faddr, lifp->buffer[lifp->index].const1,-1,&pr) ;
  pina = pr.ipaddr;
  ncpin = pr.numicp;
  for(j = 0 ; j < MIN(ncpin,100); j += 1)
    cpina[j] = pr.icpaddr[j];
  pouta = pr.opaddr;
  cpouta = pr.ocpaddr;
  pinv = pr.ipval == -1 ? 1 : pr.ipval;
  for(j = 0 ; j < MIN(ncpin,100); j += 1)
    cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j]; 
  lifp->buffer[lifp->index].btype = 0;
#if F_TRACE
  eprintf("lifetch_nonbranch: load delay slot: %08lx idx:%i  faddr:%08lx  instr:%08lx\n",lifp->f_loaddelayslot,lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
  if(lifp->f_loaddelayslot == *faddr){
#if F_TRACE
  eprintf("lifetch_nonbranch: load delay slot: %08lx \tidx:%i  %08lx  %08lx\n",lifp->f_loaddelayslot,lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
    lifp->f_loaddelayslot = 0;
  }
  lifp->inwindow[lifp->index] = *faddr;
  rs = llb_load(lifp->llbp, lifp->index, *faddr,lifp->buffer[lifp->index].instrword,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].opexec,lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].btype,lifp->buffer[lifp->index].src1, lifp->buffer[lifp->index].src2,lifp->buffer[lifp->index].src3,lifp->buffer[lifp->index].src4,lifp->buffer[lifp->index].dst1,lifp->buffer[lifp->index].dst2,lifp->buffer[lifp->index].const1,lifp->buffer[lifp->index].const1_valid, pina,cpina,ncpin,pouta,cpouta,pinv,cpinv,NOT_TAKEN);
  p(lifp, faddr, ncpin, pina, cpina, pouta, cpouta, pinv, cpinv);
#if F_DEBUG
  eprintf("lifetch_nonbranch: non-branch was loaded, rs:%d\n",rs);
#endif
  if (rs < 0){ 	
#if	F_DEBUG
    eprintf("lifetch_nonbranch: error loading llb at non_branch instruction");
#endif
    assert(0);
  }
 
#if F_DEBUG
  eprintf("lifetch_nonbranch: \tidx:%i  %08lx  %08lx\n",lifp->index,*faddr,lifp->buffer[lifp->index].instrword);
#endif
 
#if F_TRACE
  eprintf("INSTR %6lx  %08lx        NON BRANCH\n",*faddr,lifp->buffer[lifp->index].instrword);
#endif
  *faddr += 4; 
  return SR_OK;
}


/* we are told to go to some address */
int lifetch_goto(lifp,ta)
LIFETCH	*lifp ;
uint	ta ;
{
	int	rs = SR_OK ;
	if (lifp == NULL)
		return SR_FAULT ;
	/*	lifp->btarget = (ta != -1) ? ta : 0 ;

#if F_DEBUG
	eprintf("lifetch_goto:\tADDRESS: %6lx\n",lifp->btarget);
#endif
	*/
	return rs ;
}
/* end subroutine (lifetch_goto) */


/* Read instruction trace file */
int lifetch_readtrace(lifp)
     LIFETCH *lifp;
{
  int offset,i;
  lifp->traceidx = 0;
  
  if((lifp->fsize % 7) !=0){
    /* Each instruction address uses 7 bytes, lifp->fsize is given in bytes */
    printf("lifetch_readtrace: trace file size:%d not divisible by 7\n",lifp->fsize);
    return SR_FAULT;
  }
#if F_DEBUG
  eprintf("lifetch_readtrace: fsize:%d fsize/7:%d\n",lifp->fsize,lifp->fsize/7);  
#endif
  if(((lifp->fsize/7) - lifp->instrsread) > BUFSIZ)
    offset = BUFSIZ;
  else{
    if(((lifp->fsize/7) - lifp->instrsread)>0)
      offset = lifp->fsize - lifp->instrsread;
    else{
      offset = 0;
      lifp->finished = 1;
      eprintf("lifetch_readtrace: finished reading trace file\n");
      return SR_OK;
    }
  }
  i=0;
  while(i < offset){
    fscanf(lifp->tracefile,"%08lx\n",&lifp->addrtrace[i]);
    /*    eprintf("\t%i:%08lx\n",i,lifp->addrtrace[i]);*/
    i++;
  }
  lifp->instrsread +=i;
  
  lseek(lifp->tracefile->_file,offset,SEEK_CUR);
  return SR_OK;
}


/* Fetch some instructions, decode, and load them */
static int lifetch_fetch(lifp, pip, mip, lip)
LIFETCH	*lifp ;
struct proginfo *pip;
LSIM   *mip;
struct levoinfo *lip;
{ 
  LIBUS *irbp;   
  RFBUS *ifbp;
  LFLOWGROUP fw_lflowgroup, fr_lflowgroup;
  uint	faddr,  instr;
  int   bpred,numlb, j, rs, utimes, pina,  ncpin, pouta, predval,cpouta, pinv, cpina[100], cpinv[100];
  struct lbtrb_predicates pr;
  
  pip = lifp->pip ;
  mip = lifp->mip ;
  lip = lifp->lip ;
  irbp = lip->irbp;
  ifbp = lip->ifbp;
  
#if	F_SAFE2
  rs = lifetch_sanitycheck(lifp,pip,lip) ;
  if (rs < 0) {
#if	F_DEBUG
    if (pip->debuglevel > 2) {
      eprintf("lifetch_fetch: 0a sanity i=%d rs=%d\n", 
	      lifp->index,rs) ;
    }
#endif
#if	F_DEBUGS
    eprintf("lifetch_fetch: 0a sanity i=%d rs=%d\n", 
	    lifp->index,rs) ;
#endif
    return rs ;
  }
#endif /* F_SAFE2 */
  
  /* Check llb status */

#if	F_OLDLLB
  if (! llb_is_free(lifp->llbp))
    return SR_OK ;
#else
  rs = llb_inready(lifp->llbp) ;
  if (rs < 0){
    printf("lifetch_fetch: llb_inready returned negative \n");
    return rs ;
  }
  else {
    if (rs > 0)
      lifp->f_loadllb = 1;
    /* Cannot load the LLB at this clock, try in the next one */
    else{
      lifp->f_loadllb = 0;
      /*
	printf("lifetch_fetch: load buffer can't be loaded at this clock \n");
      */
    }
  }
#endif /* F_OLDLLB */
  
  /* do we load the llb ? */
  if(lifp->f_loadllb){

  if (! lbtrb_isready(lip->lbtrbp)) {
#if	F_DEBUG
    if (pip->debuglevel > 2)
      eprintf("lifetch_fetch: LBTRB says not ready\n") ;
#endif
    return SR_OK ;
  }

/* global sanity */
  
#if	F_SAFEFUNC
#if	F_DEBUG
  if (pip->debuglevel > 4)
    eprintf("lifetch_fetch: 4 sanity global\n") ;
#endif
  if (lifp->sanity.func != NULL) {
    rs = (*lifp->sanity.func)(lifp->sanity.arg) ;
    if (rs1 < 0) {
#if	F_DEBUG
      if (pip->debuglevel > 1)
	eprintf("lifetch_fetch: 4 bad sanity global rs=%d\n",
		rs1) ;
#endif
      return rs1 ;
    }
  }
#endif /* F_SAFEFUNC */

#if F_PERFETCH
  if(lifp->traceidx <BUFSIZ){
    faddr = lifp->addrtrace[lifp->traceidx];
  }
  else{
    lifetch_readtrace(lifp);
    return SR_OK;
  }
#else
  /* Get fetch address according to program behavior */
  if(lifp->btarget != 0){
    if(lifp->f_loaddelayslot != 0){
      faddr = lifp->f_loaddelayslot;
    }
    else{
      /* faddr = lifp->btarget;*/
      lifp->fetchaddr = lifp->btarget;
      faddr = lifp->fetchaddr;
      lifp->btarget = 0;
    }
  }
  else
    faddr = lifp->fetchaddr ;
#endif

#if F_DEBUG 
  eprintf("\nlifetch_fetch: entered, faddr: %08lx\n",faddr);
#endif   

#if !(F_PERFETCH)
  if(faddr == lifp->exitaddr){
#if	F_TRACE
    eprintf("lifetch_fetch: simulation terminated\n");
#endif
  }
#endif

#if F_LSIM_MEM
  
  for(lifp->index =0; lifp->index < lifp->lip->totalrows; lifp->index+=1){
nextinstr:
    /* Get the right fetch address */
#if F_PERFETCH
  /* Get perfect fetch address */
  if(lifp->traceidx <BUFSIZ){
    faddr = lifp->addrtrace[lifp->traceidx];
  }
  else{
    lifetch_readtrace(lifp);
    return SR_OK;
  }
#else
  if(lifp->btarget != 0){
    if(lifp->f_loaddelayslot != 0){
      faddr = lifp->f_loaddelayslot;
    }
    else{
      faddr = lifp->btarget;
      lifp->btarget = 0;
    }
  }
#endif
  rs = lsim_readinstr(mip,faddr,&instr);

#endif


#if F_LMEM
    /* request instructions to I-cache memory */
#if F_PERFETCH
  /* Get perfect fetch address */
  if(lifp->traceidx <BUFSIZ){
    faddr = lifp->addrtrace[lifp->traceidx];
  }
  else{
    lifetch_readtrace(lifp);
    return SR_OK;
  }
#endif
    printf("lifetch_fetch: faddr:%08lx lastreq_addr:%08lx\n",faddr, lifp->lastreqaddr);
    if((faddr == lifp->lastreqaddr)&&(!lifp->f_lastpos)){
      printf("lifetch_fetch: waiting for instruction: %08lx from last request\n",lifp->lastreqaddr);
    }
    else{
      if(lifp->lastreqaddr == lifp->lastrdaddr){
	if(lifp->f_lastpos)
	  lifp->f_lastpos = 0;
	fw_lflowgroup.addr = faddr;
	fw_lflowgroup.dp= LFLOWGROUP_DPALL;
	printf("lifetch_fetch: requesting bus \n");
	rs = rfbus_write(ifbp, &fw_lflowgroup);
	lifp->lastreqaddr = faddr;
	if (rs < 0){
	  eprintf("lifetch_fetch: fetch req bus returned negative \n");
	  return rs;
	}
	printf("lifetch_fetch: instruction %08lx was requested\n",faddr);
      }
    }
    /* read instructions from I-cache memory */
    fr_lflowgroup.addr = faddr;
    fr_lflowgroup.dp = LFLOWGROUP_DPALL;
    printf("lifetch_fetch: reading fetch bus \n");
    rs = libus_read(irbp, &fr_lflowgroup, 1);
    if (rs == 0){
      printf("lifetch_fetch: nothing to read from ibus\n");
      /* lifp->f_readcompleted = 0;*/
      return rs;
    }
    /*   lifp->f_readcompleted = 1;*/
    lifp->lastrdaddr = faddr;
    printf("lifetch_fetch: read lifp->lastrdaddr: %08lx\n",lifp->lastrdaddr);
    instr = fr_lflowgroup.dv;
#endif
		     
    if (rs < 0){
#if	F_TRACE3
      eprintf("\nlifetch_fetch reading instruction returned negative %08lx:%08lx\n",faddr,instr);
#endif
      return rs;
    }
    else{
      lifp->buffer[lifp->index].instrword = instr ;
      lifp->buffer[lifp->index].ilptr = faddr; 
#if	F_TRACE
      /*    eprintf("lifetch_fetch: decoding:%08x:%08x\n",faddr,instr);*/
    printf("lifetch_fetch: decoding:%08x:%08x\n",faddr,instr);
#endif
    /* decode this instruction */
    (void) ldecode_decode(&lifp->buffer[lifp->index]);  
    /* solo para verificar decode */
                lifp->traceidx++; 
		goto nextinstr;    
	}
    
#if	F_SAFE2
    rs = lifetch_sanitycheck(lifp,pip,lip) ;
    
    if (rs < 0) {
#if	F_DEBUG
      if (pip->debuglevel > 2) {
	eprintf("lifetch_fetch: 1 sanity i=%d rs=%d\n", 
		lifp->index,rs) ;
      }
#endif
      
#if	F_DEBUGS
      eprintf("lifetch_fetch: 1 sanity i=%d rs=%d\n", 
	      lifp->index,rs) ;
#endif
      return rs ;
    }
#endif /* F_SAFE2 */ 

    /* Do we load the llb ? */

      if((lifp->index == lifp->lip->totalrows - 1) &&
	 (lifp->buffer[lifp->index].opclass == INSTR_JIND ||
	  lifp->buffer[lifp->index].opclass == INSTR_BREL || 
	  lifp->buffer[lifp->index].opclass == INSTR_JREL)) {
	lifp->f_lastpos = 1;
	lifp->buffer[lifp->index].opclass = INSTR_UNUSED;


#if F_PERFETCH
	lifp->traceidx -=1;
	faddr = lifp->addrtrace[lifp->traceidx]; 
#else
	faddr -= 4 ;  
#endif
	lifp->inwindow[lifp->index] = faddr;


	rs = llb_load(lifp->llbp, lifp->index, faddr, lifp->buffer[lifp->index].instrword,  lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].src1,  lifp->buffer[lifp->index].src2, lifp->buffer[lifp->index].src3, lifp->buffer[lifp->index].src4, lifp->buffer[lifp->index].dst1, lifp->buffer[lifp->index].dst2, lifp->buffer[lifp->index].const1, lifp->buffer[lifp->index].const1_valid,  pina, cpina, ncpin, pouta, cpouta, pinv, cpinv, NOT_TAKEN);
	if (rs < 0){ 	
#if	F_TRACE
	  if (pip->debuglevel > 1)
	    eprintf("lifetch_fetch: error loading LLB\n");
#endif
	  assert(0);
	}
#if F_DEBUG
	printf("lifetch_fetch:bec\tidx:%i  %08lx  %08lx\n",lifp->index,faddr,lifp->buffer[lifp->index].instrword);
#endif
#if F_PERFETCH
	  lifp->traceidx += 1;
	  faddr = lifp->addrtrace[lifp->traceidx]; 
#else
	faddr += 4;
#endif
#if F_TRACE 
	printf("INSTR %6lx  %08lxUUUUUUUNNNNNNNNUUUUUUSSSSSEEEEEEDDDDDDDDDD\n",faddr,lifp->buffer[lifp->index].instrword);
	eprintf("lifetch_fetch: UNUSED instruction loaded\n"); 
#endif

#if F_LMEM
	lifp->index = 0;
#endif
      }
      else {
#if F_PERFETCH
		  /*
	if (lifp->buffer[lifp->index].opclass == INSTR_JIND ||
	    lifp->buffer[lifp->index].opclass == INSTR_BREL || 
	    lifp->buffer[lifp->index].opclass == INSTR_JREL)
	  bpred = (lifp->addrtrace[lifp->traceidx-1] == lifp->addrtrace[lifp->traceidx]-4)? NOT_TAKEN:TAKEN; 
	else
	  bpred = -1;
		  */
	lbtrb_assign_predicate(lip->lbtrbp, lifp->buffer[lifp->index].opclass,lifp->buffer[lifp->index].ilptr, lifp->buffer[lifp->index].const1,TAKEN,&pr);
	pina = pr.ipaddr;
	ncpin = pr.numicp;
	for(j = 0 ; j < MIN(ncpin,100); j += 1)
	  cpina[j] = pr.icpaddr[j];
	pouta = pr.opaddr;
	cpouta = pr.ocpaddr;
	pinv = pr.ipval == -1 ? 1 : pr.ipval;
	for(j = 0 ; j < MIN(ncpin,100); j += 1)
	  cpinv[j] = pr.icpval[j] == -1 ? 0 : pr.icpval[j];
	rs = llb_load(lifp->llbp, lifp->index, faddr, lifp->buffer[lifp->index].instrword,  lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].opexec, lifp->buffer[lifp->index].opclass, lifp->buffer[lifp->index].btype, lifp->buffer[lifp->index].src1,  lifp->buffer[lifp->index].src2, lifp->buffer[lifp->index].src3, lifp->buffer[lifp->index].src4, lifp->buffer[lifp->index].dst1, lifp->buffer[lifp->index].dst2, lifp->buffer[lifp->index].const1, lifp->buffer[lifp->index].const1_valid,  pina, cpina, ncpin, pouta, cpouta, pinv, cpinv, bpred);
	if (rs < 0){ 	
#if	F_TRACE
	  if (pip->debuglevel > 1)
	    eprintf("lifetch_fetch: error loading LLB\n");
#endif
	  assert(0);
	}
	lifp->inwindow[lifp->index] = faddr;
	lifp->traceidx +=1;
#else
	switch(lifp->buffer[lifp->index].opclass){
	case INSTR_BREL:
	  /* Conditional Branch */
	  lifetch_condbranch(lifp, &faddr, lip);
	  break;
	case INSTR_JREL:
	  /* Direct Jump */
	  lifetch_jump(lifp, &faddr, lip);
	  break;
	case INSTR_JIND:
	  /* Indirect Jump */
	  lifetch_indjump(lifp, &faddr, lip);
	  break;
	default:
	  /* Non Branch Instruction */
	  lifetch_nonbranch(lifp, &faddr, lip);
	  break;
	}
#endif
#if F_LMEM
	(lifp->index == lifp->lip->totalrows - 1)? lifp->index = 0:lifp->index += 1;
#endif
      }

#if F_LSIM_MEM
  } /* for */
  llb_load_complete(lifp->llbp) ;
    } /* if (lifp->f_loadllb) */
  else
    return lifp->f_loadllb;
#endif
  
#if F_LMEM
  llb_load_complete(lifp->llbp) ;
#if F_PERFETCH
lifp->traceidx +=1;
#endif
 } /* if (lifp->f_loadllb) */ 
else
return lifp->f_loadllb;
#endif
lifp->fetchaddr = faddr ; 
 printf("lifetch_fetch: leaving......................................faddr: %08lx\n",faddr);
return SR_OK ; 
} /* end subroutine (lifetch_fetch) */
  

static int lifetch_sanitycheck(lifp,pip,lip)
LIFETCH		*lifp ;
struct proginfo *pip ;
struct levoinfo	*lip ;
{
	int	rs = SR_OK, i ;


/* check the RF buses */

	for (i = 0 ; i < lip->nsg ; i += 1) {

		rs = bus_sanitycheck(lifp->rfbuses + i) ;

		if (rs < 0)
			break ;

	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3) {
		if (rs < 0)
		eprintf("lifetch_sanitycheck: bad RFBUS i=%d rs=%d\n",
			i,rs) ;
	}
#endif

	if (rs < 0)
		return rs ;

/* check the ASes */

	for (i = 0 ; i < (lip->totalrows * lip->totalcols) ; i += 1) {

		rs = las_sanitycheck(lifp->lases + i) ;

		if (rs < 0)
			break ;

	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3) {
		if (rs < 0)
		eprintf("lifetch_sanitycheck: bad AS=%08lx i=%d rs=%d\n",
			i,(lifp->lases + i),rs) ;
	}
#endif

	return rs ;
}
/* end subroutine (lifetch_sanitycheck) */

static void p(LIFETCH *lifp, uint *faddr,int ncpin, int pina, int *cpina, int pouta, int cpouta, int pinv, int *cpinv){
  printf("lifetch_p: loading instruction:\n
lifp->llbp:%p\n
lifp->index:%i\n
*faddr:%08lx\n
lifp->buffer[lifp->index].instrword:%08lx\n
lifp->buffer[lifp->index].opexec:%i\n
lifp->buffer[lifp->index].opclass:%i\n
lifp->buffer[lifp->index].opexec:%i\n
lifp->buffer[lifp->index].opclass:%i\n
lifp->buffer[lifp->index].btype:%i\n
lifp->buffer[lifp->index].src1:%i\n
lifp->buffer[lifp->index].src2:%i\n
lifp->buffer[lifp->index].src3:%i\n
lifp->buffer[lifp->index].src4:%i\n
lifp->buffer[lifp->index].dst1:%i\n
lifp->buffer[lifp->index].dst2:%i\n
lifp->buffer[lifp->index].const1:%08lx\n
lifp->buffer[lifp->index].const1_valid:%i\n
pina:%i\n
cpina:%p\n
ncpin:%i\n
pouta:%i\n
cpouta:%i\n
pinv:%i\n
cpinv:%p\n
NOT_TAKEN:%i\n",
	 lifp->llbp, 
	 lifp->index, 
	 *faddr,
	 lifp->buffer[lifp->index].instrword,
	 lifp->buffer[lifp->index].opexec,
	 lifp->buffer[lifp->index].opclass,
	 lifp->buffer[lifp->index].opexec,
	 lifp->buffer[lifp->index].opclass,
	 lifp->buffer[lifp->index].btype,
	 lifp->buffer[lifp->index].src1,
	 lifp->buffer[lifp->index].src2,
	 lifp->buffer[lifp->index].src3,
	 lifp->buffer[lifp->index].src4,
	 lifp->buffer[lifp->index].dst1,
	 lifp->buffer[lifp->index].dst2,
	 lifp->buffer[lifp->index].const1,
	 lifp->buffer[lifp->index].const1_valid,
	 pina,
	 cpina,
	 ncpin,
	 pouta,
	 cpouta,
	 pinv,
	 cpinv,
	 NOT_TAKEN);
}
