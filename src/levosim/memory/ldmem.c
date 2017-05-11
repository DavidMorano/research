/* ldmem */


#define	F_DEBUGS	0		/* compile-time debug print-outs */
#define	F_DEBUG		0		/* run-time debug print-outs */


/* revision history :

	= 00/xx/xx, Maryam Ashouei

	This code object was started.


	= 01/01/24, Dave morano

	I added the #define's for debugging print-outs so that they
	could be turned OFF for multi-column code testing.


*/


/******************************************************************************

	This is the xx-whatever-xx object and it does xx-whatever-xx !


******************************************************************************/



#include	<sys/types.h>
#include	<stdlib.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"mintinfo.h"
#include	"levoinfo.h"
#include	"ldmem.h"	



/* local defines */



/* external subroutines */


/* forward references */

static int ldmem_phase4(LDMEM *) ;






/* initialize this object's internal state or structures */
int ldmem_init(ldmem,pip,pfp,mip,lip, ldm_parameter)
LDMEM		*ldmem ;
struct proginfo *pip ;
PARAMFILE	*pfp ;
struct mintinfo *mip ;
struct levoinfo *lip ;
LDMEM_PARAMETERS *ldm_parameter;
{
  int i;


	ldmem->pip = pip ;
	ldmem->mip = mip ;
	ldmem->lip = lip ;
	ldmem->param = (LDMEM_PARAMETERS *) malloc(sizeof(LDMEM_PARAMETERS));
        *(ldmem->param) =*(ldm_parameter);
	/*(void) memcpy(&ldmem->param,ldm_parameter,sizeof(LDMEM_PARAMETERS));
	 */
	ldmem -> busy = 0;
	ldmem -> current_state.wait = ldmem -> next_state.wait = 0;
	
	return 1 /*SR_OK*/;
}
/* end subroutine (ldmem_init) */


int ldmem_free(ldmp)
LDMEM	*ldmp ;
{


	if (ldmp == NULL)
		return SR_FAULT ;

/* free up everything that we allocated at initialization time */

	return SR_OK ;
}
/* end subroutine (ldmem_free) */


/* we perform all of our independent combinatorial logic here */
int ldmem_comb(LDMEM *ldmem, int phase)
{

/* do we have anything to do ? */

	switch (phase) {

	case 0:
		break ;

	case 1:
		break ;

	case 2:
		break ;

	case 3:
		break ;

	case 4: 
		ldmem_phase4(ldmem);

	        break;

	} /* end switch */

	return 1 /*SR_OK*/ ;
}
/* end subroutine (lmem_comb) */



/* we get clocked */
int ldmem_clock(LDMEM *ldmem)
{
  

  ldmem-> current_state = ldmem -> next_state;


  return SR_OK;
}
/* end subroutine (lmem_clock) */


int ldmem_write (LDMEM *ldmem)
{
  int i;
 

  ldmem-> busy = 1;

  ldmem -> next_state.wait = ldmem->param->write_cycles;
  return ldmem->param->write_cycles;
}


int ldmem_read (LDMEM *ldmem)
{
  int i;
 

  ldmem-> busy = 1;

  ldmem -> next_state.wait = ldmem->param->read_cycles;
  return ldmem->param->read_cycles;
}
 

int ldmem_busy (LDMEM *ldmem){

  return ldmem-> busy;
}



/* PRIVATE SUBROUTINES */



static int ldmem_phase4(LDMEM * ldmem)
{

  if (ldmem -> busy == 0 ) 
	return  1;

  switch (ldmem -> current_state.wait) {

  case 0 :
    /* we've already took care of next state in ldmem-read and 
       ldmem-write functions */
    break;
  default : 
	 ldmem-> next_state.wait = ldmem -> current_state.wait -1;
	 if (ldmem->next_state.wait == 0) 
	   ldmem->busy = 0;
	 break;
  }
}



