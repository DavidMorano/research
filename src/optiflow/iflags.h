/* iflags */

/* instruction flag accounting */


#ifndef	IFLAGS_INCLUDE
#define	IFLAGS_INCLUDE		1



#include	<sys/types.h>

#include	"localmisc.h"

#include	"machine.h"



#define	IFLAGS		struct iflags



struct iflags {
	ULONG	icount ;		/* instruction count (call-count) */
	ULONG	icomp ;
	ULONG	fcomp ;
	ULONG	ctrl ;
	ULONG	uncond ;
	ULONG	cond ;
	ULONG	mem ;
	ULONG	load ;
	ULONG	store ;
	ULONG	disp ;
	ULONG	rr ;
	ULONG	direct ;
	ULONG	trap ;
	ULONG	longlat ;
	ULONG	dirjmp ;
	ULONG	indirjmp ;
	ULONG	call ;
	ULONG	fpcond ;
	ULONG	imm ;
	ULONG	ctrl_dirjmp ;
	ULONG	ctrl_indirjmp ;
	ULONG	ctrl_call ;
	ULONG	ctrl_fpcond ;
	ULONG	class[NUM_FU_CLASSES + 1] ;
	ULONG	ctrl_class[NUM_FU_CLASSES + 1] ;
} ;



extern int	iflags_init(struct iflags *) ;
extern int	iflags_proc(struct iflags *,int,int) ;
extern int	iflags_free(struct iflags *,const char *) ;



#endif /* IFLAGS_INCLUDE */



