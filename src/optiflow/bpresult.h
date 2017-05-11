/* bpresult */


#ifndef	BPRESULT_INCLUDE
#define	BPRESULT_INCLUDE	1


#include	<sys/types.h>
#include	<time.h>

#include	<bio.h>

#include	"localmisc.h"



/* object defines */

#define	BPRESULT			struct bpresult_head




struct bpresult_head {
	unsigned long		magic ;
	bfile			rfile ;
} ;



#if	(! defined(BPRESULT_MASTER)) || (BPRESULT_MASTER == 0)

extern int	bpresult_open(BPRESULT *,char *) ;
extern int	bpresult_write(BPRESULT *,char *,double, char *,
			int,int,int,int,int) ;
extern int	bpresult_close(BPRESULT *) ;

#endif /* BPRESULT_MASTER */


#endif /* BPRESULT_INCLUDE */



