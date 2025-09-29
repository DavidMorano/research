/* itype */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */


#define	CF_DEBUGS	0


#include	<sys/types.h>
#include	<sys/param.h>

#include	<usystem.h>
#include	<bio.h>

#include	"localmisc.h"

#include	"itype.h"
#include	"ssnames.h"
#include	"ssas.h"




#define	ITYPE_MAGIC	0x33221158



/* external subroutines */

extern double	percentll(ULONG,ULONG) ;






int itype_init(itp)
ITYPE	*itp ;
{


	if (itp == NULL)
	    return SR_FAULT ;

	memset(itp,0,sizeof(ITYPE)) ;

	itp->magic = ITYPE_MAGIC ;
	return SR_OK ;
}


int itype_proc(itp,casp)
ITYPE	*itp ;
SSAS	*casp ;
{
	int	n ;


	if (itp == NULL)
	    return SR_FAULT ;

	if (itp->magic != ITYPE_MAGIC)
	    return SR_NOTOPEN ;

	    itp->itype_ins += 1 ;

	    n = MIN(casp->n.class,iclass_overlast) ;
	    itp->itype_class[n] += 1 ;

	    if (casp->n.f.cf) {

	        itp->itype_cf += 1 ;
	        if (casp->n.f.cfcond)
	            itp->itype_cfcond += 1 ;

	        if (casp->n.f.cfsub)
	            itp->itype_cfsub += 1 ;

	        if (casp->n.f.cfdir)
	            itp->itype_cfdir += 1 ;

	        else if (casp->n.f.cfind)
	            itp->itype_cfind += 1 ;

	    } else if (casp->n.f.mem) {

	        itp->itype_mem += 1 ;
	        if (casp->n.f.memload)
	            itp->itype_memload += 1 ;

	        else if (casp->n.f.memstore)
	            itp->itype_memstore += 1 ;

	    } /* end if */

	return SR_OK ;
}
/* end subroutine (itype_proc) */


int itype_writeout(itp,statsfname)
ITYPE		*itp ;
const char	statsfname[] ;
{
	bfile	sfile, *tfp = &sfile ;

	int	rs = SR_OK, i ;
	int	wlen ;


	if (itp == NULL)
	    return SR_FAULT ;

	if (itp->magic != ITYPE_MAGIC)
	    return SR_NOTOPEN ;

	wlen = 0 ;
	if (statsfname[0] == '\0')
		goto ret0 ;

#if	CF_DEBUGS
	eprintf("itype_writeout: statsfname=%s\n",statsfname) ;
#endif

	    rs = bopen(tfp,statsfname,"wa",0666) ;

#if	CF_DEBUGS
	eprintf("itype_writeout: bopen() rs=%d\n",rs) ;
#endif

	if (rs < 0)
		goto ret0 ;

	rs = bprintf(tfp,"= execution profile\n") ;

#if	CF_DEBUGS
	eprintf("itype_writeout: bprintf() rs=%d\n",rs) ;
#endif

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_ins=          %10llu (N)\n",
	    itp->itype_ins) ;

	wlen += rs ;
	if (itp->itype_ins <= 0)
		goto ret1 ;

	rs = bprintf(tfp,
	    "itype_cf=           %10llu (%6.1f%% CF/N)\n",
	    itp->itype_cf,
	    percentll(itp->itype_cf, itp->itype_ins)) ;

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_cfcond=       %10llu (%6.1f%% CFC/N)\n",
	    itp->itype_cfcond,
	    percentll(itp->itype_cfcond, itp->itype_ins)) ;

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_cfsub=        %10llu (%6.1f%% CFS/N)\n",
	    itp->itype_cfsub,
	    percentll(itp->itype_cfsub, itp->itype_ins)) ;

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_cfdir=        %10llu (%6.1f%% CFD/N)\n",
	    itp->itype_cfdir,
	    percentll(itp->itype_cfdir, itp->itype_ins)) ;

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_cfind=        %10llu (%6.1f%% CFI/N)\n",
	    itp->itype_cfind,
	    percentll(itp->itype_cfind, itp->itype_ins)) ;

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_mem=          %10llu (%6.1f%% M/N)\n",
	    itp->itype_mem,
	    percentll(itp->itype_mem, itp->itype_ins)) ;

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_memload=      %10llu "
	    "(%6.1f%% ML/N, %6.1f%% ML/M)\n",
	    itp->itype_memload,
	    percentll(itp->itype_memload,itp->itype_ins),
	    percentll(itp->itype_memload,itp->itype_mem)) ;

	wlen += rs ;
	rs = bprintf(tfp,
	    "itype_memstore=     %10llu "
	    "(%6.1f%% MS/N, %6.1f%% MS/M)\n",
	    itp->itype_memstore,
	    percentll(itp->itype_memstore,itp->itype_ins),
	    percentll(itp->itype_memstore,itp->itype_mem)) ;

	for (i = 0 ; i < iclass_overlast ; i += 1) {

	    rs = bprintf(tfp,"itype_class_%-6s= %10llu"
	        " (%6.1f%% type/N)\n",
	        ssfunames[i],
	        itp->itype_class[i],
	        percentll(itp->itype_class[i],itp->itype_ins)) ;

	    wlen += rs ;
	    if (rs < 0)
	        break ;

	} /* end for */

	rs = bprintf(tfp,"\n") ;

	wlen += rs ;

ret1:
	bclose(tfp) ;

ret0:
	return (rs >= 0) ? wlen : rs ;
}
/* end subroutine (itype_writeout) */


int itype_free(itp)
ITYPE	*itp ;
{


	if (itp == NULL)
	    return SR_FAULT ;

	if (itp->magic != ITYPE_MAGIC)
	    return SR_NOTOPEN ;

	itp->magic = 0 ;
	return SR_OK ;

}
/* end subroutine (itype_free) */



