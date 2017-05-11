/* density */


#ifndef	DENSITY_INCLUDE
#define	DENSITY_INCLUDE		1


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>

#include	"localmisc.h"



#define	DENSITY		struct density
#define	DENSITY_STATS	struct density_s



struct density {
	unsigned long	magic ;
	ULONG		*a ;
	ULONG		c ;
	uint		max ;
	uint		ovf ;
	uint		len ;
} ;

struct density_s {
	double		mean, var ;
	ULONG		ovf ;
	ULONG		count ;
	uint		len ;
	uint		max ;
} ;


#ifdef	__cplusplus
extern "C" {
#endif

extern int	density_init(DENSITY *,uint) ;
extern int	density_update(DENSITY *,uint) ;
extern int	density_slot(DENSITY *,uint,ULONG *) ;
extern int	density_stats(DENSITY *,DENSITY_STATS *) ;
extern int	density_free(DENSITY *) ;

#ifdef	__cplusplus
}
#endif


#endif /* DENSITY_INCLUDE */



