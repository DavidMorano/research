/* tracedata */

/* process trace information */


#define	CF_DEBUGS	0		/* compile-time debugging */


/* revision history:

	= 2001-09-01, David Morano

	This subroutine was originally written.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */


#define	TRACEDATA_MASTER	1


#include	<envstandards.h>

#include	<sys/types.h>

#include	<vsystem.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"

#include	"exectrace.h"
#include	"tracedata.h"


/* local defines */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */


/* local (module-scope static) data */


/* exported subroutines */


int tracedata_init(ip)
struct tracedata	*ip ;
{
	int	size ;


#ifdef	COMMENT
	size = NREGS * sizeof(uint) ;
	(void) memset(ip->reg,0,size) ;

	ip->clock = 0 ;
	ip->in = 0 ;
#else
	(void) memset(ip,0,sizeof(struct tracedata)) ;
#endif

	return 0 ;
}


int tracedata_free(ip)
struct tracedata	*ip ;
{


	return 0 ;
}


int tracedata_proc(ip,ep)
struct tracedata	*ip ;
EXECTRACE_ENTRY		*ep ;
{
	int	f = TRUE ;


	if (ep->f.clock) {

	    f = TRUE ;
	    ip->clock = ep->clock ;

	}

	if (ep->f.reg)
	    ip->f.gotregs = f = TRUE ;

#if	CF_DEBUGS
	debugprintf("tracedata_proc: ip->f.ia=%d \n",
		ip->f.ia) ;
	debugprintf("tracedata_proc: ep->f.ia=%d ep->f.in=%d ep->in=%lld\n",
		ep->f.ia,ep->f.in,ep->in) ;
#endif

	if (ep->f.in)
		ip->in = ep->in ;

#if	CF_DEBUGS
	debugprintf("tracedata_proc: ip->f.ia=%d ip->in=%lld\n",
		ip->f.ia,ip->in) ;
#endif

	if (ep->f.ia) {

		ip->f.ia = TRUE ;
	}

	return f ;
}
/* end subroutine (tracedata_proc) */


/* did we receive any registers ? */
int tracedata_recregs(ip,ep)
struct tracedata	*ip ;
EXECTRACE_ENTRY		*ep ;
{
	uint	a ;

	int	i = 0 ;


	if (ep->f.reg) {

	    for (i = 0 ; i < ep->f.reg ; i += 1) {

	        if (ep->reg[i].dp) {

	            a = ep->reg[i].a ;
	            ip->gotreg[a] = TRUE ;

	        } /* end if (data present) */

	    } /* end for */

	} /* end if */

	return i ;
}
/* end subroutine (tracedata_recregs) */


int tracedata_writeback(ip,ep)
struct tracedata	*ip ;
EXECTRACE_ENTRY		*ep ;
{
	uint	a ;

	int	i = 0 ;


	if (ep->f.reg) {

	    for (i = 0 ; i < ep->f.reg ; i += 1) {

	        if (ep->reg[i].dp) {

	            a = ep->reg[i].a ;
	            ip->reg[a] = ep->reg[i].dv ;

	        } /* end if (data present) */

	    } /* end for */

	} /* end if */

	return i ;
}
/* end subroutine (tracedata_writeback) */


