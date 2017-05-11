/* sshiftreg */

/* synchronous shift register */


#ifndef	SSHIFTREG_INCLUDE
#define	SSHIFTREG_INCLUDE		1


#include	<sys/types.h>

#include	"localmisc.h"		/* for { uint, ULONG } */



#define	SSHIFTREG		struct sshiftreg_head



struct sshiftreg_state {
	int	head, tail, c ;
} ;

struct sshiftreg_head {
	unsigned long	magic ;
	struct sshiftreg_state	c, n ;
	char		*buf ;
	char		*valid ;
	int		objsize, nobj ;
} ;




#if	(! defined(SSHIFTREG_MASTER)) || (SSHIFTREG_MASTER == 0)

extern int	sshiftreg_init(SSHIFTREG *,int,int) ;
extern int	sshiftreg_free(SSHIFTREG *) ;
extern int	sshiftreg_writea(SSHIFTREG *) ;
extern int	sshiftreg_write(SSHIFTREG *,void *) ;
extern int	sshiftreg_read(SSHIFTREG *,void *) ;
extern int	sshiftreg_gethead(SSHIFTREG *,void *) ;
extern int	sshiftreg_gettail(SSHIFTREG *,void *) ;
extern int	sshiftreg_clock(SSHIFTREG *) ;
extern int	sshiftreg_comb(SSHIFTREG *,int) ;
extern int	sshiftreg_audit(SSHIFTREG *) ;

#endif /* SSHIFTREG */


#endif /* SSHIFTREG */



