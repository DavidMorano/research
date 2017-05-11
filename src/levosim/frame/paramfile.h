/* paramfile */

/* object to handle parameter files */


/* revision history:

	= 2000-02-15, Dave Morano

	This code was started for Levo related work.


*/


#ifndef	PARAMFILE_INCLUDE
#define	PARAMFILE_INCLUDE	1


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<limits.h>
#include	<time.h>

#include	<vecobj.h>
#include	<vecstr.h>
#include	<varsub.h>

#include	"localmisc.h"



/* object defines */

#define	PARAMFILE		struct paramfile_head
#define	PARAMFILE_ENTRY		struct paramfile_e
#define	PARAMFILE_CURSOR	struct paramfile_c
#define	PARAMFILE_ERROR		struct paramfile_errline



struct paramfile_flags {
	uint		init_vsd : 1 ;
	uint		init_vse : 1 ;
} ;

struct paramfile_head {
	unsigned long	magic ;
	const char	**envv ;	/* program startup environment */
	VECSTR		*dvp ;
	VECOBJ		files ;
	VECOBJ		entries ;	/* parameter entries */
	VARSUB		d, e ;
	struct paramfile_flags	f ;
	time_t		checktime ;	/* time last checked */
	dev_t		pwd_dev ;
	ino_t		pwd_ino ;
	int		pwd_len ;
	char		*pwd ;
} ;

struct paramfile_e {
	char		*key ;
	char		*orig, *value ;	/* dynamic variable expansion */
	int		fi ;
	int		klen, vlen ;
} ;

struct paramfile_c {
	int	i ;
} ;



#ifdef	COMMENT

typedef struct paramfile_head	paramfile ;
typedef struct paramfile_e	paramfile_entry ;
typedef struct paramfile_c	paramfile_cursor ;

#endif /* COMMENT */



#if	(! defined(PARAMFILE_MASTER)) || (PARAMFILE_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int paramfile_open(PARAMFILE *,const char **,const char *) ;
extern int paramfile_fileadd(PARAMFILE *,const char *) ;
extern int paramfile_setdefines(PARAMFILE *,VECSTR *) ;
extern int paramfile_cursorinit(PARAMFILE *,PARAMFILE_CURSOR *) ;
extern int paramfile_cursorfree(PARAMFILE *,PARAMFILE_CURSOR *) ;
extern int paramfile_fetch(PARAMFILE *,const char *,
		PARAMFILE_CURSOR *,char *,int) ;
extern int paramfile_enum(PARAMFILE *,PARAMFILE_CURSOR *,
		PARAMFILE_ENTRY *,char *,int) ;
extern int paramfile_check(PARAMFILE *,time_t) ;
extern int paramfile_close(PARAMFILE *) ;

#ifdef	__cplusplus
}
#endif

#endif /* PARAMFILE_MASTER */


#endif /* PARAMFILE_INCLUDE */



