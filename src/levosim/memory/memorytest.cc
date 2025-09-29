/* memorytest */

/* Levo memory  Test */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		1		/* switchable debug print-outs */
#define	F_SAFE		1


/* revision history :

	= 01/02/23, Maryam Ashouei

	Module was originally written.


*/


/**************************************************************************

	This object module put some load and store requests on  the buses
	go to the memory system to test the "LMEM"  object.



**************************************************************************/


#include	<sys/types.h>
#include	<cstdlib>
#include	<cstring>

#include	<usystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"mintinfo.h"
#include	"levoinfo.h"
#include	"bus.h"
#include	"lflowgroup.h"
#include	"lbusint.h"
#include	"memorytest.h"



/* local defines */



/* external subroutines */


/* forward references */

static int	memorytest_dostuff(MEMORYTEST *) ;







int  memorytest_init(mtp, pip, mip, lip, MBB, MFB, MWB, interleave, 
		     interleave_scheduling)
MEMORYTEST	*mtp ;
struct proginfo	*pip ;
struct mintinfo	*mip ;
struct levoinfo	*lip ;
RFBUS * MBB;
RFBUS * MFB;
RFBUS * MWB;
int interleave;
int interleave_scheduling;
{
	int	rs = SR_OK ;
	int	i ;
	int size;

	(void) memset(mtp,0,sizeof(MEMORYTEST)) ;

	mtp->pip = pip;
	mtp->mip = lip->mip;
	mtp->lip = lip;

	mtp->MBB = MBB;
	mtp->MFB = MFB;
	mtp->MWB = MWB;
	mtp -> interleave = interleave;
        mtp -> interleave_scheduling = interleave_scheduling;
	mtp -> value = 0x1000ba10;
    

#if	F_DEBUG
	    if (pip->debuglevel > 1)
	        eprintf("memorytest_init: ");
#endif

	return SR_OK;
}
/* end subroutine (memorytest_init) */


int memorytest_free(mtp)
MEMORYTEST	*mtp ;
{
	     
}
/* end subroutine (memorytest_free) */


/* perform the combinatorial computations */
int memorytest_comb(mtp,phase)
MEMORYTEST		*mtp ;
int		phase ;
{
	struct proginfo	*pip ;

	struct mintinfo	*mip ;

	int	rs = SR_OK ;
	int	i ;


	if (mtp == NULL)
	    return SR_FAULT ;

	pip = mtp->pip ;
	mip = mtp->mip ;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("memorytest_comb: entered ck=%lld ph=%d\n",
	        mip->clock, phase) ;
#endif


	rs = SR_OK;
	switch (phase) {

	case 0:
	  rs = memorytest_dostuff(mtp) ;
	  break ;
	case 4 :
	  rs = memorytest_read(mtp);

	} /* end switch */

	return rs ;
}
/* end subroutine (memorytest_comb) */


/* handle a clock transition */
int memorytest_clock(mtp)
MEMORYTEST	*mtp ;
{
	return SR_OK ;
}
/* end subroutine (memorytest_clock) */



/* PRIVATE SUBROUTINES */



/* do stuff */
static int memorytest_dostuff(mtp)
MEMORYTEST	*mtp ;
{
	struct proginfo	*pip ;

	struct mintinfo	*mip ;

	LFLOWGROUP	wr, rd ;

	int	rs = SR_OK;
	int	i;
	int	f_vdp;
	int     index;

	pip = mtp->pip;
	mip = mtp->mip;

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("memorytest_dostuff: ck=%lld\n",mip->clock) ;
#endif

/* write all buses that are ready ! */
	printf("form memroytest: ");
	/*	for (i = 0 ; i < mtp->interleave; i += 1) {*/
                lflowgroup_init(&wr) ;
		wr.tt = 0 ;
		wr.addr = mtp->value;
	        wr.dv = mtp->value ;
	        wr.dp = LFLOWGROUP_DPALL;
		index = getinterleave(mtp->interleave_scheduling, (wr.addr>>2));
		rs = rfbus_write(mtp->MWB + index, &wr);
                if (rs>=1) {
		  printf("write the value %d and the add %d to MWB\n",
			 mtp->value, mtp->value+1);
		}else 
		  printf("write to MWB was not successful");
                rs = rfbus_write(mtp->MBB + index, &wr);
		if (rs >=1){
		  printf("write the value %d and the add %d to MBB\n",
			 mtp->value, mtp->value);
		  mtp -> value += 4;
                
		  if ((mtp->value > (0x1000ba10+30)) && (mtp->value < (0x1000ba10+1024)))
		    mtp->value = 0x1000ba10+1028;
		  if (mtp->value >(0x1000ba10+1050))
		    mtp->value = 0x1000ba10; 

		}else 
		  printf("writing on MBB wasn't successful\n");


	        wr.dp = LFLOWGROUP_DPNONE ;

		/*	} */   /* end for */




	return SR_OK ;
}
/* end subroutine (bustest_dostuff) */

int memorytest_read(mtp)
MEMORYTEST *mtp;
{
  int i;
  int rs;
  LFLOWGROUP rd;

  /* read all buses that have something */

  printf("from memorytest:\n");
  for (i = 0 ; i < mtp->interleave ; i += 1) {
    rs = rfbus_read(mtp->MFB + i,&rd) ;
    if (rs>0){
      printf("mfb%d  %x %x\n",i, rd.addr, rd.dv);
      
    }else 
      printf("there was nothing on MFbus\n");
            
  } /* end for */
  return rs;
}


