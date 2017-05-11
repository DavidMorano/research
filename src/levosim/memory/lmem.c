/* lmem */

/* top of the Levo memory subsystem */


#define	F_DEBUGS	0		/* compile-time only switchable */
#define	F_DEBUG		1		/* dynamically switchable */
#define	F_SAFE		1		/* safe mode */


/* revision history :

	= 00/02/15, Dave Morano

	This code was started.


	= 00/12/01, Maryam Ashouei

	I took over this code.


	= 01/01/20, Dave Morano

	I fix some little interface issues and added the supprt
	to get parameters from the parameter file.


*/



/******************************************************************************

	This is the top object for the Levo machine memory subsystem.


******************************************************************************/




#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"paramfile.h"
#include	"config.h"
#include	"defs.h"
#include	"mintinfo.h"
#include	"levoinfo.h"
#include	"bus.h"			/* generic buses */
#include	"libus.h"		/* Instruction Response Bus */
#include	"lmem.h"		/* ourself */
#include	"ldmem.h"		/* Levo main memory */
#include	"licache.h"		/* Levo i-cache */
#include	"ldcache.h"		/* Levo d-cache */



/* local defines */



/* external subroutines */

extern int	ffbsi(uint), flbsi(uint) ;
extern int	cfnumui(char *,int,uint *) ;
extern int	cfnumi(char *,int,int *) ;


/* local structures */



/* forward references */


/* local variables */

static char	*const params[] = {
        "mem:icachesize",	/* I-cache size */
        "mem:dcachesize",	/* D-cache size */
	"mem:icblocksize",	/* I-cache blocksize */
	"mem:dcblocksize",	/* D-cache blocksize */
	"mem:icreadck",		/* i-cache read clocks per access */
	"mem:icwriteck",	/* i-cache write clocks per access */
	"mem:icreadck",		/* d-cache read clocks per access */
	"mem:icwriteck",	/* d-cache write clocks per access */
	"mem:mmreadck",		/* main memory read clocks per access */
	"mem:mmwriteck",	/* main memory write clocks per access */
	 NULL,
};

#define	LMEMPARAM_ICACHESIZE	0
#define	LMEMPARAM_DCACHESIZE	1
#define	LMEMPARAM_ICBLOCKSIZE	2
#define	LMEMPARAM_DCBLOCKSIZE	3
#define	LMEMPARAM_ICREADCK	4
#define	LMEMPARAM_ICWRITECK	5
#define	LMEMPARAM_DCREADCK	6
#define	LMEMPARAM_DCWRITECK	7
#define	LMEMPARAM_MMREADCK	8
#define	LMEMPARAM_MMWRITECK	9






/* initialize this object's internal state or structures */
int lmem_init(lmp,pip,pfp,mip,lip, lmap)
LMEM		*lmp ;
struct proginfo	*pip ;
PARAMFILE	*pfp ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
LMEM_ARGS	*lmap ;
{
	LIBUS *irb ;		/* instruction response bus */

	RFBUS *ifb ;		/* instruction fetch bus */
	RFBUS *mbb ;		/* memory backwarding bus (load requests) */
	RFBUS *mfb ;		/* memory forwarding bus (load respnses) */
	RFBUS *mwb ;		/* memory write bus (committed writes) */

  	LICACHE_PARAMETERS	lic_param ;	/* i-cache paramaters */

  	LDCACHE_PARAMETERS	ldc_param ;	/* d-cache parameters */

  	LDMEM_PARAMETERS	ldm_param ;	/* main memory parameters */

	uint	uv ;			/* miscellaneous unsigned values */

	int	rs, i, sl ;
	char	*cp ;

	if (lmp == NULL)
	  return SR_FAULT ;
	

#if	F_DEBUG
	if (pip->debuglevel > 1)
	eprintf("lmem_init: entered\n") ;
#endif
	
	(void) memset(lmp,0,sizeof(LMEM)) ;

	lmp->pip = pip ;
	lmp->mip = mip ;
	lmp->lip = lip ;


/* load up some stuff from the 'levoinfo' structure */

	irb = lip->irbp ;		/* instruction response bus */
	ifb = lip->ifbp ;		/* instruction fetch bus */
	mbb = lip->mbbuses ;		/* memory backwarding buses */
	mfb = lip->mfbuses ;		/* memory forwarding bus */
	mwb = lip->mwbuses ;		/* memory write bus */


/* get some of our parameters from the top Levo component */

	lmp->interleave = lip->nmembuses; 
	/* lmap->interleave */


/* get the rest of our configuration paramaters from the parameter file */

/* set some defaults */

	lic_param.size = LMEM_ICACHESIZE ;
	lic_param.interleaved = lip->nmembuses ;
  	lic_param.interleave_scheduling = lmap -> interleave;
	lic_param.blocksize = 16 ;
	lic_param.type = 1;
	lic_param.pending_limit = 100;
	lic_param.readcycles = 2 ;
	lic_param.writecycles = 2 ;
	

	ldc_param.size = LMEM_DCACHESIZE ;
  	ldc_param.blocksize = 16 ;
  	ldc_param.type = 1 ;
  	ldc_param.interleaved = lip->nmembuses ;
	ldc_param.interleave_scheduling = lmap -> interleave;
  	ldc_param.replacement_policy = 0 ;
  	ldc_param.write_policy = 1 ;
  	ldc_param.pending_limit = 100;
	ldc_param.readcycles = 2 ;
	ldc_param.writecycles = 2 ;

	ldm_param.read_cycles = 2 ;
	ldm_param.write_cycles = 2 ;


/* fetch some IW specific stuff from the parameter files */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: getting parameters\n") ;
#endif

	for (i = 0 ; params[i] != NULL ; i += 1) {

	    if ((sl = paramfile_fetch(pfp,params[i],NULL,&cp)) >= 0) {

	        switch (i) {

	        case LMEMPARAM_ICACHESIZE:
	        case LMEMPARAM_DCACHESIZE:
	        case LMEMPARAM_ICBLOCKSIZE:
	        case LMEMPARAM_DCBLOCKSIZE:
		case LMEMPARAM_ICREADCK:
		case LMEMPARAM_ICWRITECK:
		case LMEMPARAM_DCREADCK:
		case LMEMPARAM_DCWRITECK:
		case LMEMPARAM_MMREADCK:
		case LMEMPARAM_MMWRITECK:
	            if (cfnumui(cp,sl,&uv) >= 0) {

	                switch (i) {

	                case LMEMPARAM_ICACHESIZE:
	                    lic_param.size = uv ;
	                    break ;

	                case LMEMPARAM_DCACHESIZE:
	                    ldc_param.size = uv ;
	                    break ;

	                case LMEMPARAM_ICBLOCKSIZE:
	                    lic_param.blocksize = uv ;
	                    break ;

	                case LMEMPARAM_DCBLOCKSIZE:
	                    ldc_param.blocksize = uv ;
	                    break ;

	                case LMEMPARAM_ICREADCK:
	                    lic_param.readcycles = uv ;
	                    break ;

	                case LMEMPARAM_ICWRITECK:
	                    lic_param.writecycles = uv ;
	                    break ;

	                case LMEMPARAM_DCREADCK:
	                    ldc_param.readcycles = uv ;
	                    break ;

	                case LMEMPARAM_DCWRITECK:
	                    ldc_param.writecycles = uv ;
	                    break ;

	                case LMEMPARAM_MMREADCK:
	                    ldm_param.read_cycles = uv ;
	                    break ;

	                case LMEMPARAM_MMWRITECK:
	                    ldm_param.write_cycles = uv ;
	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        } /* end switch (outer) */

	    } /* end if (got a good parameter) */

	} /* end for (parameters) */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: done getting parameter\n") ;
#endif


/* verify some argument and apply some defaults as necessary */

/* i-cache */

	if (lic_param.size == 0)
	    lic_param.size = LMEM_ICACHESIZE ;

	if (lic_param.blocksize == 0)
	    lic_param.blocksize = 16 ;

/* d-cache */

	if (ldc_param.size == 0)
	    ldc_param.size = LMEM_DCACHESIZE ;

	if (ldc_param.blocksize == 0)
	    ldc_param.blocksize = 16 ;


/* there are no "invalid" values for access clock times ! */



/* continue with the rest of the initialization */

        
	lmp->licache = 
	   (LICACHE *) malloc(sizeof(LICACHE));
	

	lmp->ldcache = 
 	  (LDCACHE **) malloc(lmp->interleave * sizeof(LDCACHE *));

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: LDCACHE=%08lx\n",lmp->ldcache) ;
#endif
       

	lmp->ldmem = 
	  (LDMEM **) malloc(lmp->interleave * sizeof(LDMEM *));


	lmp->memtest = 
	  (MEMORYTEST *) malloc(sizeof(MEMORYTEST));

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: looping\n") ;
#endif
       
	for (i = 0 ; i < lmp->interleave ; i += 1) {

	  lmp->ldmem[i] = (LDMEM *) malloc(sizeof (LDMEM));

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: ldmem_init()\n") ;
#endif

	  rs = ldmem_init(lmp->ldmem[i], pip, pfp,mip, lip, &ldm_param);

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: ldmem_init() rs=%d\n",rs) ;
#endif

	  lmp->ldcache[i] = (LDCACHE *) malloc(sizeof(LDCACHE));

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: LDCACHE(%d)=%08lx ldcache_init() \n",
		i,lmp->ldcache[i]) ;
#endif

	  ldcache_init(lmp->ldcache[i], pip, mip, lip, lmp->ldmem[i], 
		       &ldc_param, mbb + i, mfb + i, mwb + i) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: ldcache_init() rs=%d\n",rs) ;
#endif

	} /* end for */

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_init: done looping\n") ;
#endif


	   licache_init(lmp->licache, pip, mip, lip, lmp->ldmem, 
	        &lic_param, irb, ifb);
	
	   /*
	memorytest_init(lmp->memtest, pip, mip, lip, mbb, mfb, mwb, 
			lmp->interleave, lmap->interleave);
			*/
	
	return SR_OK ;
}
/* end subroutine (lmem_init) */


/* free up any resources that this object may have inside */
int lmem_free(lmp)
LMEM	*lmp ;
{
	struct proginfo	*pip ;

	int	i ;


	if (lmp == NULL)
	  return SR_FAULT ;

	pip = lmp->pip ;

	/*
	
	for (i = 0; i < lmp->interleave; i ++) {
	  ldmem_free(lmp->ldmem[i]);
	  ldcache_free(lmp->ldcache[i]) ;
	}
   
        */
	/*
	licache_free(lmp->licache);
       	*/
	return SR_OK ;
}
/* end subroutine (lmem_free) */


/* we perform all of our independent combinatorial logic here */
int lmem_comb(lmp,phase)
LMEM	*lmp ;
int	phase ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;
	int	rs1 ;

	uint newdata;


	if (lmp == NULL)
	  return SR_FAULT ;

	pip = lmp->pip ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_comb: entered\n",rs) ;
#endif


#if	F_SAFE
	rs1 = lmem_sanitycheck(lmp) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_comb: 0 bad sanity rs=%d\n",rs) ;
#endif

		return rs1 ;
	}
#endif /* F_SAFE */



	/*
	printf("        LSIM READ AND WRITE TEST         \n");
        rs = -1;
        i = -4;
	while (rs <0 ){ 
	  i += 4;
	  rs = lsim_writeint(lmp->mip,  0x1000ba10+i, 20, LFLOWGROUP_DPALL);
	}
        printf("%d \n", rs);
	rs = lsim_readint(lmp->mip, 0x1000ba10+i, &newdata);
        printf("%d %d\n", rs, newdata);

	*/

/* our children have stuff to do */

	licache_comb(lmp->licache, phase) ;

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_comb: LDCACHE=%08lx\n",lmp->ldcache) ;
#endif

	for ( i = 0 ; i < lmp->interleave; i += 1) {

	  rs = ldmem_comb(lmp->ldmem[i], phase);

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_comb: ldmem_comb() rs=%d\n",rs) ;
#endif

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_comb: LDCACHE(%d)=%08lx ldcache_comb() \n",
		i,lmp->ldcache[i]) ;
#endif

	  rs = ldcache_comb(lmp->ldcache[i], phase);

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_comb: ldcache_comb() rs=%d\n",rs) ;
#endif

	} /* end for */

	
	
	/*
	memorytest_comb(lmp->memtest, phase);
	*/

#if	F_SAFE
	rs1 = lmem_sanitycheck(lmp) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_comb: 10 bad sanity rs=%d\n",rs) ;
#endif

		return rs1 ;
	}
#endif /* F_SAFE */


	return rs ;
}
/* end subroutine (lmem_comb) */


/* our creator clocks us */
int lmem_clock(lmp)
LMEM *lmp;
{
	struct proginfo	*pip ;

	int	rs = SR_OK, i ;
	int	rs1 ;


	if (lmp == NULL)
	  return SR_FAULT ;

	pip = lmp->pip ;


#if	F_SAFE
	rs1 = lmem_sanitycheck(lmp) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_clock: 0 bad sanity rs=%d\n",rs) ;
#endif

		return rs1 ;
	}
#endif /* F_SAFE */

	for (i = 0; i < lmp->interleave ; i += 1) {

	  ldmem_clock(lmp->ldmem[i]);

	  ldcache_clock(lmp->ldcache[i]);

	} /* end for */

	
	licache_clock(lmp->licache) ;
	
	/*
        memorytest_clock(lmp->memtest);
	*/


#if	F_SAFE
	rs1 = lmem_sanitycheck(lmp) ;

	if (rs1 < 0) {

#if	F_DEBUG
	if (pip->debuglevel > 3)
	eprintf("lmem_clock: 10 bad sanity rs=%d\n",rs) ;
#endif

		return rs1 ;
	}
#endif /* F_SAFE */

	return rs ;
}
/* end subroutine (lmem_clock) */


/* the Levo machine is shifting */
int lmem_shift(lmp)
LMEM	*lmp ;
{


	if (lmp == NULL)
	  return SR_FAULT ;

	lmp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (lmem_shift) */


/* so some sanity checking */
int lmem_sanitycheck(lmp)
LMEM	*lmp ;
{
	struct proginfo	*pip ;

	int	rs, i ;


	if (lmp == NULL)
	  return SR_FAULT ;

	pip = lmp->pip ;


#if	F_DEBUG
	if (pip->debuglevel > 3) {
	eprintf("lmem_sanitycheck: entered\n") ;

	eprintf("lmem_sanitycheck: LDCACHE=%08lx\n",lmp->ldcache) ;

	for (i = 0 ; i < lmp->interleave ; i += 1)
	eprintf("lmem_sanitycheck: LDCACHE(%d)=%08lx\n",
		i,lmp->ldcache[i]) ;

	}
#endif


	return SR_OK ;
}
/* end subroutine (lmem_sanitycheck) */


/* XML stuff */
int lmem_xmlinit(fp,xip)
LMEM	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}


int lmem_xmlout(fp,xip)
LMEM	*fp ;
XMLINFO	*xip ;
{


	bprintf(&xip->xmlfile,"<lmem>\n") ;

	bprintf(&xip->xmlfile,"<uid>%p</uid>\n",fp) ;


	bprintf(&xip->xmlfile,"</lmem>\n") ;

	return SR_OK ;
}


int lmem_xmlfree(fp,xip)
LMEM	*fp ;
XMLINFO	*xip ;
{



	return SR_OK ;
}
/* end subroutine (lmem_xmlfree) */



/* PRIVATE SUBROUTINES */



