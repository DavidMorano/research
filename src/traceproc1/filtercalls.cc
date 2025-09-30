/* filtercalls */

/* helper to filter out subroutine calls */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_SAFE		0


/* revision history:

	= 2001-11-05, David Morano

	I am starting this out from scratch for the SimpleSim simulator.


*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This little object is just a little data base to hold the
	addresses of subroutine calls that are to be filtered out of a
	trace.


*******************************************************************************/


#define	FILTERCALLS_MASTER	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<elf.h>
#include	<cstring>

#include	<usystem.h>
#include	<bfile.h>
#include	<mallocstuff.h>
#include	<localmisc.h>

#include	"filtercalls.h"
#include	"lmapprog.h"


/* local defines */

#define	FILTERCALLS_MAGIC	0x86553823

#ifndef	LINEBUFLEN
#define	LINEBUFLEN		2048
#endif


/* external subroutines */

extern int	sfshrink(const char *,int,char **) ;

extern char	*strwcpy(char *,const char *,int) ;


/* local structures */


/* forward references */

static int	filtercalls_indexinit(FILTERCALLS *,LMAPPROG *,
				int,const char *) ;
static int	filtercalls_indexfree(FILTERCALLS *,LMAPPROG *) ;


/* local variables */


/* exported subroutines */


int filtercalls_init(op,mp,fname)
FILTERCALLS	*op ;
LMAPPROG	*mp ;
const char	fname[] ;
{
	ustat	sb ;

	int	rs ;
	int	size ;


	if (op == NULL)
	    return SR_FAULT ;

	if (mp == NULL)
	    return SR_FAULT ;

	if (fname == NULL)
	    return SR_FAULT ;

	if (fname[0] == '\0')
	    return SR_INVALID ;

	op->mp = mp ;

	rs = u_stat(fname,&sb) ;

	if (rs < 0)
	    goto bad0 ;

	size = (sb.st_size / 4) ;
	rs = filtercalls_indexinit(op,mp,size,fname) ;

	if (rs < 0)
	    goto bad0 ;


	op->magic = FILTERCALLS_MAGIC ;

bad0:
	return rs ;
}
/* end subroutine (filtercalls_init) */


int filtercalls_free(op)
FILTERCALLS	*op ;
{
	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FILTERCALLS_MAGIC)
	    return SR_NOTOPEN ;

	filtercalls_indexfree(op,op->mp) ;

	op->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (filtercalls_free) */


/* is it a call that is on the filter list ? */
int filtercalls_have(op,addr,npp)
FILTERCALLS	*op ;
uint		addr ;
const char	**npp ;
{
	FILTERCALLS_ENTRY	*iep ;

	HDB_DATUM	key, value ;

	int	rs ;
	int	f_found ;


#if	CF_SAFE
	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != FILTERCALLS_MAGIC)
	    return (op->magic != 0) ? SR_BADFMT : SR_NOTOPEN ;
#endif /* F_SAFE */


#if	CF_DEBUGS
	debugprintf("filtercalls_have: entered\n") ;
#endif

	key.buf = &addr ;
	key.len = sizeof(uint) ;

#if	CF_DEBUGS
	debugprintf("filtercalls_have: hdb_fetch()\n") ;
#endif

	rs = hdb_fetch(&op->index,key,NULL,&value) ;

#if	CF_DEBUGS
	{
		if (rs == SR_NOTFOUND)
	debugprintf("filtercalls_have: hdb_fetch() rs=NOTFOUND\n") ;
		else
	debugprintf("filtercalls_have: hdb_fetch() rs=%d\n",rs) ;
	}
#endif

	if ((rs < 0) && (rs != SR_NOTFOUND))
		return rs ;

	f_found = (rs >= 0) ;
		rs = SR_OK ;

	if (npp != NULL) {

		iep = (FILTERCALLS_ENTRY *) value.buf ;

#if	CF_DEBUGS
	debugprintf("filtercalls_have: iep=%p \n",iep) ;
#endif

		*npp = NULL ;
		if (f_found)
			*npp = iep->name ;

	} /* end if (got one) */

#if	CF_DEBUGS
	debugprintf("filtercalls_have: exiting rs=%d f_found=%d\n",
		rs,f_found) ;
#endif

	return ((rs >= 0) ? f_found : rs) ;
}
/* end subroutine (filtercalls_have) */



/* PRIVATE SUBROUTINES */



/* index the calls by their function address */
static int filtercalls_indexinit(op,mp,size,fname)
FILTERCALLS	*op ;
LMAPPROG	*mp ;
int		size ;
const char	fname[] ;
{
	FILTERCALLS_ENTRY	*iep ;

	LMAPPROG_SNCURSOR	cur ;

	HDB_DATUM	key, value ;

	Elf32_Sym	*eep ;

	bfile	callfile ;

	int	rs, i ;
	int	len, sl ;
	int	f_got ;

	char	linebuf[LINEBUFLEN + 1] ;
	char	*name ;


	rs = hdb_start(&op->index,size,1,NULL,NULL) ;
	if (rs < 0)
	    return rs ;

	rs = bopen(&callfile,(char *) fname,"r",0666) ;
	if (rs < 0)
	    goto bad1 ;

/* loop through */

	while ((rs = breadline(&callfile,linebuf,LINEBUFLEN)) > 0) {

	    len = rs ;
	    sl = sfshrink(linebuf,len,&name) ;

	    if (sl <= 0)
	        continue ;

	    if (name[0] == '#')
	        continue ;

	    name[sl] = '\0' ;

/* look it up */

	    lmapprog_sncurbegin(mp,&cur) ;

	    f_got = FALSE ;
	    while (TRUE) {

	        rs = lmapprog_fetchsym(mp,name,&cur,&eep) ;

	        if (rs < 0)
	            break ;

#if	CF_DEBUGS
	        {
	            debugprintf("filtercalls_indexinit: name=%s rs=%d\n",
	                name,rs) ;
	            debugprintf("filtercalls_indexinit: BIND=%d TYPE=%d\n",
	                ELF32_ST_BIND(eep->st_info),
	                ELF32_ST_TYPE(eep->st_info)) ;
	        }
#endif /* CF_DEBUG */

	        if (((ELF32_ST_BIND(eep->st_info) == STB_GLOBAL) ||
	            (ELF32_ST_BIND(eep->st_info) == STB_WEAK)) &&
	            (ELF32_ST_TYPE(eep->st_info) == STT_FUNC)) {

	            f_got = TRUE ;
	            break ;
	        }

	    } /* end while (looping through candidate symbols) */

	    lmapprog_sncurend(mp,&cur) ;

	    if (f_got) {

#if	CF_DEBUGS
	            debugprintf("filtercalls_indexinit: sym=%s value=%08x\n",
	                name,eep->st_value) ;
#endif

	        rs = uc_malloc(sizeof(FILTERCALLS_ENTRY),&iep) ;

	        if (rs < 0)
	            break ;

	        iep->symval = eep->st_value ;
	        iep->name = mallocstrw(name,sl) ;

	        key.buf = &eep->st_value ;
	        key.len = sizeof(Elf32_Addr) ;

	        value.buf = iep ;
	        value.len = sizeof(FILTERCALLS_ENTRY) ;

	        rs = hdb_store(&op->index,key,value) ;

	        if (rs < 0) {
	            uc_free(iep) ;
	            break ;
	        } /* end if */

	    } /* end if (got one) */

	} /* end while (reading lines) */

	bclose(&callfile) ;

	if (rs < 0)
	    goto bad2 ;

	return SR_OK ;

/* bad things come here */
bad2:
	(void) filtercalls_indexfree(op,mp) ;

	return rs ;

bad1:
	hdb_finish(&op->index) ;

	return rs ;
}
/* end subroutine (filtercalls_indexinit) */


/* free up the data used to hold SYSCALL information */
static int filtercalls_indexfree(op,mp)
FILTERCALLS	*op ;
LMAPPROG	*mp ;
{
	HDB_CUR		cur ;
	HDB_DATUM	key, value ;
	FILTERCALLS_ENTRY	*iep ;
	int		rs = SR_OK ;

	hdb_curbegin(&op->index,&cur) ;

	while (hdb_enum(&op->index,&cur,&key,&value) >= 0) {

	    iep = (FILTERCALLS_ENTRY *) value.buf ;

	    if (iep != NULL) {
		if (iep->name != NULL) {
		    uc_free(iep->name) ;
		    iep->name = NULL ;
		}
	        uc_free(iep) ;
	    }

	} /* end while */

	hdb_curend(&op->index,&cur) ;

/* pop it */

	rs = hdb_finish(&op->index) ;

	return rs ;
}
/* end subroutine (filtercalls_indexfree) */


