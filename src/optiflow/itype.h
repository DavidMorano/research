/* itype */



#ifndef	ITYPE_INCLUDE
#define	ITYPE_INCLUDE	1


/* object instantiation defines */
#define	ITYPE		struct itype_head


#include	<sys/types.h>
#include	<sys/param.h>

#include	<bfile.h>

#include	"localmisc.h"

#include	"sscommon.h"
#include	"ssas.h"



struct itype_head {
	ULONG	magic ;
	ULONG	itype_ins ;
	ULONG	itype_class[iclass_overlast] ;
	ULONG	itype_cf ;
	ULONG	itype_cfcond ;
	ULONG	itype_cfdir ;
	ULONG	itype_cfind ;
	ULONG	itype_cfsub ;
	ULONG	itype_mem ;
	ULONG	itype_memload, itype_memstore ;
} ;


extern int	itype_init(ITYPE *) ;
extern int	itype_proc(ITYPE *,SSAS *) ;
extern int	itype_writeout(ITYPE *,const char *) ;
extern int	itype_free(ITYPE *) ;


#endif /* ITYPE_INCLUDE */



