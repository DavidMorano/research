/* getfname INCLUDE */



#ifndef	GETFNAME_INCLUDE
#define	GETFNAME_INCLUDE	1



#ifndef	GETFNAME_TYPELOCAL
#define	GETFNAME_TYPEUNKNOWN	-1
#define	GETFNAME_TYPELOCAL	0
#define	GETFNAME_TYPEROOT	1
#endif



#if	(! defined(GETFNAME_MASTER)) || (GETFNAME_MASTER == 0)

#ifdef	__cplusplus
extern "C" {
#endif

extern int getfname(char *,char *,int,char *) ;

#ifdef	__cplusplus
}
#endif

#endif /* GETFNAME_MASTER */


#endif /* GETFNAME_INCLUDE */



