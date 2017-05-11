/* rfbus */

/* Registered Flowgroup Bus */


#define	F_DEBUGS	0		/* non-switchable debug print-outs */
#define	F_DEBUG		0		/* switchable debug print-outs */
#define	F_WIDEBUS	0		/* handle wide data path ? */
#define	F_SAFE		1		/* run in extra SAFE mode ? */


/* revision history :

	= 00/05/26, Dave Morano

	This object was taken from the previous 'busmemdata' object.


	= 00/09/27, Dave Morano

	This code has been changed into the new RFBUS code for use
	in connecting the memory subsystem to the rest of the machine.


	= 01/05/1, Dave Morano

	Maryam found a bug in this code that was making it work
	combinatorially even when it was programmed to be registered.


*/


/******************************************************************************

	This object is a data bus.  This data bus is intended to be used
	for connections from the Levo Instruction Window componets
	to the memory data subsystem.

	Although there are two buses serving memory data to the Levo
	IW, one for commands to memory, the other for responses from
	memory, this object attempts to meet both of these needs in one
	object.  There may be extra methods that are not used in all
	circumstances.


******************************************************************************/



#include	<sys/types.h>
#include	<limits.h>

#include	<vsystem.h>

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"
#include	"levoinfo.h"
#include	"lflowgroup.h"
#include	"rfbus.h"



/* local defines */

#define	RFBUS_MAGIC	0x12023034



/* external subroutines */


/* forward references */

static int	rfbus_move(RFBUS *) ;
static int	rfbus_handleshift(RFBUS *,int) ;
static int	rfbus_clear(RFBUS *) ;







int rfbus_init(busp,pip,lip,delay,width)
RFBUS		*busp ;
struct proginfo	*pip ;
struct levoinfo	*lip ;
int		delay ;
int		width ;
{
	int	rs ;
	int	size ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (pip == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("rfbus_init: entered delay=%d width=%d\n",
	        delay,width) ;
#endif

	(void) memset(busp,0,sizeof(RFBUS)) ;

	busp->magic = 0 ;

	busp->pip = pip ;
	busp->lip = lip ;

	if (delay < 0)
	    return SR_INVALID ;

	busp->delay = delay ;
	busp->nregs = delay ;
	if (busp->nregs < 1)
	    busp->nregs = 1 ;

	busp->f.shift = FALSE ;

/* allocate the bus FIFO registers */

	size = 2 * busp->nregs * sizeof(char) ;
	if ((rs = uc_malloc(size,&busp->a.dps)) < 0)
	    goto bad0 ;

	(void) memset(busp->a.dps,0,size) ;

	busp->c.dps = busp->a.dps + 0 ;
	busp->n.dps = busp->a.dps + busp->nregs ;

/* allocate the other */

	size = 2 * busp->nregs * sizeof(LFLOWGROUP) ;
	if ((rs = uc_malloc(size,&busp->a.fgroups)) < 0)
	    goto bad1 ;

	(void) memset(busp->a.fgroups,0,size) ;

	busp->c.fgroups = busp->a.fgroups + 0 ;
	busp->n.fgroups = busp->a.fgroups + busp->nregs ;

/* we're out of here */

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&busp->c,sizeof(struct rfbus_state),&busp->c.checksum) ;
#endif

	busp->magic = RFBUS_MAGIC ;
	return SR_OK ;

/* bad things */
bad2:
	free(busp->a.fgroups) ;

bad1:
	free(busp->a.dps) ;

bad0:
	return rs ;
}
/* end subroutine (rfbus_init) */


/* free up this object */
int rfbus_free(busp)
RFBUS	*busp ;
{


	if (busp == NULL)
	    return SR_FAULT ;

#if	F_MASTERDEBUG && F_SAFE
	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_free: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (busp->a.dps != NULL)
	    free(busp->a.dps) ;

	if (busp->a.fgroups != NULL)
	    free(busp->a.fgroups) ;

	busp->c.dps = busp->n.dps = NULL ;
	busp->c.fgroups = busp->n.fgroups = NULL ;

	return SR_OK ;
}
/* end subroutine (rfbus_free) */


/* perform the combinatorial computations */
int rfbus_comb(busp,phase)
RFBUS	*busp ;
int	phase ;
{
	struct proginfo	*pip ;

	struct levoinfo	*lip ;

	int	rs = SR_OK ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_combfree: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	lip = busp->lip ;
	pip = busp->pip ;

	switch (phase) {

	case 0:
	    busp->n.dps[0] = FALSE ;
	    break ;

	case 1:
	    break ;

	case 2:
	    break ;

	case 3:
		if (busp->delay > 0)
	    (void) rfbus_move(busp) ;


	    if (busp->f.shift)
	        (void) rfbus_handleshift(busp,lip->totalrows) ;

#ifdef	COMMENT
	    (void) rfbus_clear(busp) ;
#endif

	    break ;

	} /* end switch */

	return rs ;
}
/* end subroutine (rfbus_comb) */


/* the system clocks us */
int rfbus_clock(busp)
RFBUS	*busp ;
{
	LFLOWGROUP	*fgp ;

	int	rs, size ;

	char	*dps ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_clock: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

#if	F_MASTERDEBUG && F_SAFE
	rs = d_checkverify(&busp->c,sizeof(struct rfbus_state),
	    &busp->c.checksum) ;

	if (rs < 0) {

	    eprintf("rfbus_clock: check bad format\n") ;

	    return SR_BADFMT ;
	}
#endif /* F_SAFE */

	{

/* we copy, we are not swapping */

	    dps = busp->c.dps ;
	    fgp = busp->c.fgroups ;

	    size = busp->nregs * sizeof(char) ;
	    (void) memcpy(busp->c.dps,busp->n.dps,size) ;

	    size = busp->nregs * sizeof(LFLOWGROUP) ;
	    (void) memcpy(busp->c.fgroups,busp->n.fgroups,size) ;

	    busp->c = busp->n ;

	    busp->c.dps = dps ;
	    busp->c.fgroups = fgp ;

	} /* end block */

#if	F_MASTERDEBUG && F_SAFE
	d_checkcalc(&busp->c,sizeof(struct rfbus_state),&busp->c.checksum) ;
#endif

	return SR_OK ;
}
/* end subroutine (rfbus_clock) */


/* hold the bus (do this in phase 0 or 1) */
int rfbus_hold(busp)
RFBUS	*busp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_hold: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	busp->c.f.hold = TRUE ;
	return SR_OK ;
}
/* end subroutine (rfbus_hold) */


#ifdef	COMMENT

/* clear the data off of the bus for the NEXT cycle */
int rfbus_clear(busp)
RFBUS	*busp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_clear: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

/* clear the data present bits of the bus */

	if (! busp->c.f.hold) {

	    busp->n.f.dp = FALSE ;
	    (void) memset(busp->n.content.dp,0,(sizeof(RFBUS_DATAVAL) /8)) ;

	}

	return SR_OK ;
}
/* end subroutine (rfbus_clear) */

#endif /* COMMENT */


/* write for NEXT clock cycle */
int rfbus_write(busp,fgp)
RFBUS		*busp ;
LFLOWGROUP	*fgp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_write: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (busp->c.f.hold)
	    return SR_BUSY ;

	busp->n.dps[0] = TRUE ;
	busp->n.fgroups[0] = *fgp ;

	busp->n.f.busy = TRUE ;


	return 1 ;
}
/* end subroutine (rfbus_write) */


/* read the data off of the bus (if there was any) */
int rfbus_read(busp,fgp)
RFBUS		*busp ;
LFLOWGROUP	*fgp ;
{


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_read: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	if (busp->delay <= 0) {

	    if (! busp->n.dps[0])
	        return 0 ;

	    *fgp = busp->n.fgroups[0] ;

	} else {

	    if (! busp->c.dps[busp->delay - 1])
	        return 0 ;

	    *fgp = busp->c.fgroups[busp->delay - 1] ;

	}

	return 1 ;
}
/* end subroutine (rfbus_read) */


/* shift the machine */
int rfbus_shift(busp)
RFBUS	*busp ;
{
	int	i ;


#if	F_MASTERDEBUG && F_SAFE
	if (busp == NULL)
	    return SR_FAULT ;

	if ((busp->magic != RFBUS_MAGIC) && (busp->magic != 0)) {

	    eprintf("rfbus_shift: bad magic\n") ;

	    return SR_BADFMT ;
	}

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;
#endif /* F_SAFE */

	busp->f.shift = TRUE ;
	return SR_OK ;
}
/* end subroutine (rfbus_shift) */


int rfbus_xmlinit(busp,xip)
RFBUS	*busp ;
XMLINFO	*xip ;
{
	int	rs, size ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;

	size = sizeof(struct rfbus_xml) ;
	rs = uc_malloc(size,&busp->xsp) ;

	if (rs < 0)
	    return rs ;

	(void) memset(busp->xsp,0,size) ;

	return SR_OK ;
}
/* end subroutine (rfbus_xmlinit) */


int rfbus_xmlout(busp,xip)
RFBUS	*busp ;
XMLINFO	*xip ;
{
	struct proginfo	*pip ;

	LFLOWGROUP	*fgp ;

	int	rs, i ;
	int	f_dp ;


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;

	pip = busp->pip ;

	bprintf(&xip->xmlfile,"<rfbus>\n") ;

	bprintf(&xip->xmlfile,"<uid>%p</uid>\n",
	    busp) ;

	bprintf(&xip->xmlfile,"<hold>%d</hold>\n", busp->c.f.hold) ;

	bprintf(&xip->xmlfile,"<busy>%d</busy>\n", busp->c.f.busy) ;

	if (busp->delay <= 0) {

	    f_dp = busp->n.dps[0] ;
	    fgp = busp->n.fgroups + 0 ;

	} else {

	    f_dp = busp->c.dps[busp->delay - 1] ;
	    fgp = busp->c.fgroups + (busp->delay - 1) ;

	}

	bprintf(&xip->xmlfile,"<dp>%d</dp>\n",f_dp) ;

	if (f_dp) {

	    lflowgroup_xmlout(fgp,xip) ;

	} /* end if (data present) */

	bprintf(&xip->xmlfile,"</rfbus>\n") ;


	return SR_OK ;
}
/* end subroutine (rfbus_xmlout) */


int rfbus_xmlfree(busp,xip)
RFBUS	*busp ;
XMLINFO	*xip ;
{


	if (busp == NULL)
	    return SR_FAULT ;

	if (busp->magic != RFBUS_MAGIC)
	    return SR_NOTOPEN ;

	if (busp->xsp != NULL) {

	    free(busp->xsp) ;

	    busp->xsp = NULL ;

	} /* end if */


	return SR_OK ;
}
/* end subroutine (rfbus_xmlfree) */



/* PRIVATE SUBROUTINES */



static int rfbus_handleshift(busp,totalrows)
RFBUS	*busp ;
int	totalrows ;
{
	int	i ;
	int	tt_test ;


	if (! busp->f.shift)
	    return SR_OK ;

	busp->f.shift = FALSE ;
	tt_test = INT_MIN + totalrows ;
	for (i = 0 ; i < busp->nregs ; i += 1) {

	    if (busp->n.dps[i]) {

		if (busp->n.fgroups[i].ftt >= tt_test)
	        	busp->n.fgroups[i].ftt -= totalrows ;

		if (busp->n.fgroups[i].tt >= tt_test)
	        	busp->n.fgroups[i].tt -= totalrows ;

		if (busp->n.fgroups[i].rtt >= tt_test)
	        	busp->n.fgroups[i].rtt -= totalrows ;


	    }

	} /* end for */

	return SR_OK ;
}
/* end subroutine (rfbus_handleshift) */


/* shift the FIFO */
static int rfbus_move(busp)
RFBUS	*busp ;
{
	int	i ;


	for (i = 0 ; i < (busp->nregs - 1) ; i += 1) {

	    busp->n.dps[i + 1] = busp->c.dps[i] ;
	    busp->n.fgroups[i + 1] = busp->n.fgroups[i] ;

	} /* end for */

	return SR_OK ;
}
/* end subroutine (rfbus_move) */



