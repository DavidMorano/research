/* debugsched */

/* object to handle parameter files */


/* revision history :

	= 2000-02-15, Dave Morano

	This code was started.


*/


#ifndef	DEBUGSCHED_INCLUDE
#define	DEBUGSCHED_INCLUDE	1


#include	<sys/types.h>
#include	<limits.h>
#include	<time.h>

#include	<vecitem.h>
#include	<vecstr.h>
#include	<varsub.h>


/* object defines */

#define	DEBUGSCHED		struct debugsched_head
#define	DEBUGSCHED_FILE		struct debugsched_file
#define	DEBUGSCHED_ENTRY	struct debugsched_e
#define	DEBUGSCHED_CURSOR	struct debugsched_c
#define	DEBUGSCHED_ERROR	struct debugsched_errline

/* returns */

#define	PFR_ADD		3
#define	PFR_COMPILE	2
#define	PFR_READ	1
#define	PFR_OK		0

/* other */


struct debugsched_flags {
	uint	init_vsd : 1 ;
	uint	init_vse : 1 ;
} ;

struct debugsched_head {
	unsigned long	magic ;
	struct debugsched_flags	f ;
	time_t	checktime ;		/* time last checked */
	vecitem	files ;
	vecitem	entries ;		/* parameter entries */
	vecstr	*defines ;
	char	**envv ;		/* program startup environment */
	varsub	d, e ;
} ;

struct debugsched_file {
	char	*filename ;
	time_t	mtime ;
} ;

struct debugsched_e {
	int	fi ;
	int	klen, vlen ;
	char	*key ;
	char	*orig, *value ;		/* dynamic variable expansion */
} ;

struct debugsched_c {
	int	i ;
} ;

struct debugsched_errline {
	char	*filename ;
	int	line ;
} ;


#ifndef	DEBUGSCHED_MASTER

#ifdef	__cplusplus
extern "C" {
#endif

extern int debugsched_init(DEBUGSCHED *,char **,char *,vecitem *) ;
extern int debugsched_free(DEBUGSCHED *) ;
extern int debugsched_cursorinit(DEBUGSCHED *,DEBUGSCHED_CURSOR *) ;
extern int debugsched_cursorfree(DEBUGSCHED *,DEBUGSCHED_CURSOR *) ;
extern int debugsched_fetch(DEBUGSCHED *,char *,DEBUGSCHED_CURSOR *,char **) ;
extern int debugsched_enum(DEBUGSCHED *,DEBUGSCHED_CURSOR *,
			DEBUGSCHED_ENTRY **) ;

#ifdef	__cplusplus
}
#endif

#endif /* DEBUGSCHED_MASTER */

#endif /* DEBUGSCHED_INCLUDE */


