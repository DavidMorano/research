/* ssid */



#ifndef	SSID_INCLUDE
#define	SSID_INCLUDE		1



#include	<sys/types.h>

#include	"localmisc.h"
#include	"machstate.h"
#include	"sscommon.h"



/* objects */

#define	SSID			struct ssid_head





struct ssid_flags {
	uint	v : 1 ;			/* valid (used) */
	uint	ibrpred : 1 ;		/* initial branch prediction */
} ;

struct ssid_head {
	ULONG	ia ;			/* instruction address */
	ULONG	instr ;			/* instruction bits */
	ULONG	immediate ;		/* any immediate value */
	ULONG	ta ;			/* any branch target address */
	struct ssid_flags	f ;
	int	op ;			/* S-S execution opcode */
	int	niops ;			/* N input ops */
	int	noops ;			/* N output ops */
	int	iops[MACHSTATE_MAXOPSI] ;
	int	oops[MACHSTATE_MAXOPSO] ;
} ;



#endif /* SSID_INCLUDE */



