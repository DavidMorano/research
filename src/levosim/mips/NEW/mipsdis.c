/* mipsdis */

/* disassemble a MIPS instruction into its assembly language */


#define	F_DEBUGS	0


/* revision history:

	= 01/08/06, Dave Morano

	This subroutine was written to interface to Sean's MIPS
	disassember program.


	= 01/08/29, Dave Morano

	Sean's disassember is not ready yet (it is delivered but it has
	some bugs) so I changed this whole thing into something that
	reads a disassembly file (created on 'ovel' from the
	executable) and stores the data in a database later retrieval.
	This should be a good solution for the most part since the only
	instructions that we should be executing are those that are in
	the MIPS executable file !


*/


/******************************************************************************

	This module reads a disassembly output file from a MIPS
	executable (the one that we are executing) and extracts the
	data from it and puts the data into a database.  The data is
	indexed three ways : according to the uniqueness of the
	disassembled output, according to the instruction word
	generating that disassembled output, and according to the
	instruction address of that instruction (when available).

	Yes, yes, I should have processed the source disassembly file
	into some sort of form (indexed) that could be memory mapped by
	later executions.  In that way, I would not have to process and
	index the source disassembly file every time.  Unfortunately,
	on our schedule (!) everything that we should do is not
	possible due to time constraints.  We will have to suffer for a
	little while whenever we initialize to process the file on
	every separate execution as a result.


******************************************************************************/



#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/wait.h>
#include	<limits.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>

#include	<vsystem.h>
#include	<bio.h>
#include	<hdb.h>
#include	<char.h>

#include	"localmisc.h"		/* for 'uint' */
#include	"mipsdis.h"



/* local defines */

#define	MIPSDIS_MAGIC		0x93765638
#define	MIPSDIS_PROGRAM		"levoobjdump"
#define	MIPSDIS_BUFLEN		100
#define	CMDLEN			(MIPSDIS_BUFLEN + MAXPATHLEN + 20)
#define	LINELEN			150
#define	NWRITE			1000



/* external subroutines */

extern uint	elfhash(const char *,int) ;

extern int	cfhexui(const char *,int,uint *) ;
extern int	sfshrink(const char *,int,char **) ;

extern char	*strwcpy(char *,const char *,int) ;


/* local structures */


/* forward references */

static int	mipsdis_disload(MIPSDIS *,char *) ;

static int	entry_init(struct mipsdis_entry *,uint,
			char *,int,char *,int) ;
static int	entry_equal(struct mipsdis_entry *,struct mipsdis_entry *) ;
static int	entry_free(struct mipsdis_entry *) ;






int mipsdis_init(mdp,server,execfname)
MIPSDIS	*mdp ;
char	server[] ;
char	execfname[] ;
{
	int	rs, rs1 ;


	if (mdp == NULL)
	    return SR_FAULT ;

	(void) memset(mdp,0,sizeof(MIPSDIS)) ;

	rs = hdb_init(&mdp->dis,1000,NULL) ;

	if (rs < 0)
	    goto bad0 ;

	rs = hdb_init(&mdp->addr,1000,NULL) ;

	if (rs < 0)
	    goto bad1 ;

	rs = hdb_init(&mdp->instr,1000,NULL) ;

	if (rs < 0)
	    goto bad2 ;

	mdp->mode = 0 ;

/* if we are in mode 0, try and find a disassembled file */

	if ((execfname != NULL) && (execfname[0] != '\0')) {

	    char	tmpfname[MAXPATHLEN + 1] ;
	    char	*bnp, *dp ;
	    char	*sp ;


	    bnp = strrchr(execfname,'/') ;

	    dp = strrchr(execfname,'.') ;

	    if ((dp != NULL) && (bnp != NULL) && (dp > bnp))
	        sp = strwcpy(tmpfname,execfname,(dp - execfname)) ;

	    else if ((dp != NULL) && (bnp == NULL))
	        sp = strwcpy(tmpfname,execfname,(dp - execfname)) ;

	    else
	        sp = strwcpy(tmpfname,execfname,-1) ;

	    if ((sp - tmpfname + 4) < (MAXPATHLEN - 1)) {

	        strcpy(sp,".dis") ;

#if	F_DEBUGS
	        eprintf("mipsdis_init: dis file=%s\n",tmpfname) ;
#endif

	        rs1 = mipsdis_disload(mdp,tmpfname) ;

#if	F_DEBUGS
	        eprintf("mipsdis_init: mipsdis_disload() rs=%d\n",rs1) ;
#endif

	        if (rs1 >= 0)
	            mdp->f.disfile = TRUE ;

	    } /* end if (loading DB) */

	} /* end if (given an executable filename) */

/* do we have a server anyway ? */

	mdp->server = NULL ;
	if ((server != NULL) && (server[0] != '\0')) {

	    rs = uc_mallocstrn(server,-1,&mdp->server) ;

	    if (rs < 0)
	        goto bad3 ;

	    mdp->f.server = TRUE ;

	} /* end if */


	mdp->magic = MIPSDIS_MAGIC ;
	return rs ;

/* bad things */
bad3:
	hdb_free(&mdp->instr) ;

bad2:
	hdb_free(&mdp->addr) ;

bad1:
	hdb_free(&mdp->dis) ;

bad0:

#if	F_DEBUGS
	eprintf("mipsdis_init: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (mipsdis_init) */


int mipsdis_getstd(mdp,ia,instr,disbuf,disbuflen)
MIPSDIS	*mdp ;
uint	ia, instr ;
char	disbuf[] ;
int	disbuflen ;
{
	HDB_DATUM	key, value ;

	struct mipsdis_entry	*ep ;

	int	rs, sl ;


	rs = SR_NOTFOUND ;

	if (ia != 0) {

/* try the address DB first */

	    key.buf = &ia ;
	    key.len = sizeof(uint) ;

	    rs = hdb_fetch(&mdp->addr,key,NULL,&value) ;

	} /* end if (trying address first) */

	if (rs < 0) {

	    key.buf = &instr ;

	    rs = hdb_fetch(&mdp->instr,key,NULL,&value) ;

	}

	if ((rs >= 0) && (disbuf != NULL)) {

	    char	*sp ;


	    ep = (struct mipsdis_entry *) value.buf ;
	    sp = strwcpy(disbuf,ep->std,disbuflen) ;

	    rs = (sp - disbuf) ;

	} /* end if (got one) */

	return rs ;
}
/* end subroutine (mipsdis_getstd) */


int mipsdis_getlevo(mdp,ia,instr,disbuf,disbuflen)
MIPSDIS	*mdp ;
uint	ia, instr ;
char	disbuf[] ;
int	disbuflen ;
{


	return mipsdis_getstd(mdp,ia,instr,disbuf,disbuflen) ;
}
/* end subroutine (mipsdis_getlevo) */


/* free up this object */
int mipsdis_free(mdp)
MIPSDIS	*mdp ;
{
	HDB_DATUM	key, value ;

	HDB_CURSOR	cur ;

	int	rs ;


	if (mdp == NULL)
	    return SR_FAULT ;

	if (mdp->magic != MIPSDIS_MAGIC)
	    return SR_NOTOPEN ;

	if (mdp->server != NULL)
	    free(mdp->server) ;

/* free the keys in the ADDR DB */

	hdb_cursorinit(&mdp->addr,&cur) ;

	while ((rs = hdb_enum(&mdp->addr,&cur,&key,&value)) >= 0) {

	    if (key.buf != NULL)
	        free(key.buf) ;

	} /* end while */

	hdb_cursorfree(&mdp->addr,&cur) ;

/* free up the values in the DIS DB */

	hdb_cursorinit(&mdp->instr,&cur) ;

	while ((rs = hdb_enum(&mdp->instr,&cur,&key,&value)) >= 0) {

	    if (value.buf != NULL) {

	        entry_free((struct mipsdis_entry *) value.buf) ;

	        free(value.buf) ;

	    }

	} /* end while */

	hdb_cursorfree(&mdp->instr,&cur) ;

/* close out all DBs */

	hdb_free(&mdp->instr) ;

	hdb_free(&mdp->addr) ;

	hdb_free(&mdp->dis) ;

	return SR_OK ;
}
/* end subroutine (mipsdis_free) */



/* PRIVATE SUBROUTINES */



static int mipsdis_disload(mdp,fname)
MIPSDIS	*mdp ;
char	fname[] ;
{
	bfile	disfile ;

	HDB_DATUM	key, value, value2 ;

	HDB_CURSOR	cur ;

	struct mipsdis_entry	e, *ep ;

	uint	ia, instr ;

	int	rs, len, ll ;
	int	size ;
	int	n = 0 ;

	char	linebuf[LINELEN + 1] ;
	char	*lp, *sp ;


	rs = bopen(&disfile,fname,"r",0666) ;

#if	F_DEBUGS
	eprintf("mipsdis_disload: bopen() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

	while ((len = bgetline(&disfile,linebuf,LINELEN)) > 0) {

	    int	i, j, sig ;

	    char	hexbuf[10] ;


	    ll = sfshrink(linebuf,len,&lp) ;

	    lp[ll] = '\0' ;
	    if (lp[0] != '[')
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_disload: possible 1\n") ;
#endif

	    if ((sp = strchr(lp,']')) == NULL)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_disload: possible 2\n") ;
#endif

	    lp = sp + 1 ;
	    while (*lp && CHAR_ISWHITE(*lp))
	        lp += 1 ;

	    if ((lp[0] == '0') && (tolower(lp[1]) == 'x'))
	        lp += 2 ;

	    if ((sp = strchr(lp,':')) == NULL)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_disload: possible 3, lp=%W sl=%d\n",
	        lp,(sp - lp), (sp - lp)) ;
#endif

	    rs = cfhexui(lp,(sp - lp),&ia) ;

	    if (rs < 0)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_disload: possible 4, ia=%08x\n",ia) ;
#endif

	    lp = sp + 1 ;
	    j = 0 ;
	    for (i = 0 ; i < 4 ; i += 1) {

	        while (*lp && CHAR_ISWHITE(*lp))
	            lp += 1 ;

	        if ((! isxdigit(lp[0])) || (! isxdigit(lp[1])))
	            break ;

	        hexbuf[j++] = *lp++ ;
	        hexbuf[j++] = *lp++ ;

	    } /* end for */

	    if (i < 4)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_disload: possible 5\n") ;
#endif

	    rs = cfhexui(hexbuf,8,&instr) ;

	    if (rs < 0)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_disload: possible 6, instr=%08x\n",instr) ;
#endif

	    while (*lp && CHAR_ISWHITE(*lp))
	        lp += 1 ;

#if	F_DEBUGS
	    eprintf("mipsdis_disload: ia=%08x instr=%08x >%s<\n",
	        ia,instr,lp) ;
#endif

		n += 1 ;
	    

		mipsdis_writerec() ;


	} /* end while (reading lines) */

#if	F_DEBUGS
	eprintf("mipsdis_disload: OOL len=%d\n",len) ;
#endif

	if (len < 0)
	    rs = len ;

	if (rs < 0)
	    goto bad ;

#if	F_DEBUGS
	{
	    bfile	dump ;

	    int	rs1 ;


	    if (bopen(&dump,"disdump","wct",0666) >= 0) {

	        HDB_CURSOR	cur ;

	        hdb_cursorinit(&mdp->addr,&cur) ;

	        while ((rs1 = hdb_enum(&mdp->addr,&cur,&key,&value)) >= 0) {

	            ia = *((int *) key.buf) ;
	            ep = (struct mipsdis_entry *) value.buf ;
	            bprintf(&dump,"%08x %08x %s\n",
	                ia,ep->instr,ep->std) ;

	        }

	        hdb_cursorfree(&mdp->addr,&cur) ;

	        bclose(&dump) ;

	    }
	}
#endif /* F_DEBUGS */

done:
	bclose(&disfile) ;

	return rs ;

bad:
	entry_free(&e) ;

	goto done ;
}
/* end subroutine (mipsdis_disload) */


static int mipsdis_mkindex(mdp,fname)
MIPSDIS	*mdp ;
char	fname[] ;
{
	bfile	disfile ;

	HDB_DATUM	key, value, value2 ;

	HDB_CURSOR	cur ;

	struct writeblock	w ;

	struct mipsdis_entry	e, *ep ;

	uint	ia, instr ;
	uint	buffer[NWRITE][3] ;

	int	rs, len, ll ;
	int	size ;
	int	n = 0 ;
	int	ifd ;
	int	offset ;

	char	linebuf[LINELEN + 1] ;
	char	ifname[MAXPATHLEN + 1] ;
	char	*lp, *sp ;
	char	*cp ;


	rs = bopen(&disfile,fname,"r",0666) ;

#if	F_DEBUGS
	eprintf("mipsdis_mkindex: bopen() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

/* create the index file */

	len = ifname - strwcpy(ifname,fname,MAXPATHLEN) ;

	if ((cp = strrchr(ifname,'.')) != NULL) {

		len = cp - ifname ;
		*cp = '\0' ;
	}

	if ((len + 4) > (MAXPATHLEN - 1))
		goto bad1 ;

	strcpy(ifname + len,".di") ;

	rs = u_open(ifname,O_RDWR | O_CREAT | O_TRUNC,0666) ;

	if (rs < 0)
		goto bad1 ;

	ifd = rs ;

/* process the original */

	offset = 0 ;
	while ((len = bgetline(&disfile,linebuf,LINELEN)) > 0) {

		uint	thisoff ;

		int	lo ;

	    char	hexbuf[10] ;


	thisoff = offset ;
	offset += len ;

	    ll = sfshrink(linebuf,len,&lp) ;

	    lp[ll] = '\0' ;
	    if (lp[0] != '[')
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_mkindex: possible 1\n") ;
#endif

	    if ((sp = strchr(lp,']')) == NULL)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_mkindex: possible 2\n") ;
#endif

	    lp = sp + 1 ;
	    while (*lp && CHAR_ISWHITE(*lp))
	        lp += 1 ;

	    if ((lp[0] == '0') && (tolower(lp[1]) == 'x'))
	        lp += 2 ;

	    if ((sp = strchr(lp,':')) == NULL)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_mkinde: possible 3, lp=%W sl=%d\n",
	        lp,(sp - lp), (sp - lp)) ;
#endif

	    rs = cfhexui(lp,(sp - lp),&ia) ;

	    if (rs < 0)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_mkindex: possible 4, ia=%08x\n",ia) ;
#endif

	    lp = sp + 1 ;
	    j = 0 ;
	    for (i = 0 ; i < 4 ; i += 1) {

	        while (*lp && CHAR_ISWHITE(*lp))
	            lp += 1 ;

	        if ((! isxdigit(lp[0])) || (! isxdigit(lp[1])))
	            break ;

	        hexbuf[j++] = *lp++ ;
	        hexbuf[j++] = *lp++ ;

	    } /* end for */

	    if (i < 4)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_mkindex: possible 5\n") ;
#endif

	    rs = cfhexui(hexbuf,8,&instr) ;

	    if (rs < 0)
	        continue ;

#if	F_DEBUGS
	    eprintf("mipsdis_mkindex: possible 6, instr=%08x\n",instr) ;
#endif

	    while (*lp && CHAR_ISWHITE(*lp))
	        lp += 1 ;

#if	F_DEBUGS
	    eprintf("mipsdis_mkindex: ia=%08x instr=%08x >%s<\n",
	        ia,instr,lp) ;
#endif


		lo = (lp - linebuf) ;
		buffer[n][0] = ia ;
		buffer[n][1] = instr ;
		buffer[n][2] = (uint) (offset + lo) ;

		n += 1 ;
		if (n >= NWRITE) {

			u_write(ifd,buffer,n * 3 * sizeof(int)) ;

			n = 0 ;
		}

	} /* end while (reading lines) */

#if	F_DEBUGS
	eprintf("mipsdis_mkindex: OOL len=%d\n",len) ;
#endif

	if (len < 0)
	    rs = len ;

	if (rs < 0)
	    goto bad ;

	if (n > 0) {

			u_write(ifd,buffer,n * 3 * sizeof(int)) ;

	}

#if	F_DEBUGS
	{
	    bfile	dump ;

	    int	rs1 ;


	    if (bopen(&dump,"disdump","wct",0666) >= 0) {

	        HDB_CURSOR	cur ;

	        hdb_cursorinit(&mdp->addr,&cur) ;

	        while ((rs1 = hdb_enum(&mdp->addr,&cur,&key,&value)) >= 0) {

	            ia = *((int *) key.buf) ;
	            ep = (struct mipsdis_entry *) value.buf ;
	            bprintf(&dump,"%08x %08x %s\n",
	                ia,ep->instr,ep->std) ;

	        }

	        hdb_cursorfree(&mdp->addr,&cur) ;

	        bclose(&dump) ;

	    }
	}
#endif /* F_DEBUGS */

done:
bad1:
	bclose(&disfile) ;

	return rs ;

bad:
	entry_free(&e) ;

	goto done ;
}
/* end subroutine (mipsdis_mkindex) */


static int entry_init(ep,instr,std,stdlen,levo,levolen)
struct mipsdis_entry	*ep ;
uint	instr ;
char	std[], levo[] ;
int	stdlen, levolen ;
{
	int	rs, sig1, sig2 ;


	ep->std = ep->levo = NULL ;
	sig1 = sig2 = 0 ;
	ep->instr = instr ;

	if (std != NULL) {

	    rs = uc_mallocstrn(std,stdlen,&ep->std) ;

	    if (rs < 0)
	        goto bad0 ;

	    sig1 = elfhash(ep->std,-1) ;

	}

	if (levo != NULL) {

	    rs = uc_mallocstrn(levo,levolen,&ep->levo) ;

	    if (rs < 0)
	        goto bad1 ;

	    sig2 = elfhash(ep->levo,-1) ;

	}

	ep->sig = (sig1 ^ sig2) & INT_MAX ;
	return ep->sig ;

bad1:
	if (ep->std != NULL)
	    free(ep->std) ;

bad0:
	return rs ;
}
/* end subroutine (entry_init) */


static int entry_equal(e1p,e2p)
struct mipsdis_entry	*e1p, *e2p ;
{


	if (e1p->sig != e2p->sig)
	    return FALSE ;

	if (! EQUIV((e1p->std == NULL),(e2p->std == NULL)))
	    return FALSE ;

	if (! EQUIV((e2p->levo == NULL),(e2p->levo == NULL)))
	    return FALSE ;

	if (e1p->std != NULL) {

	    if (strcmp(e1p->std,e2p->std) != 0)
	        return FALSE ;

	}

	if (e1p->levo != NULL) {

	    if (strcmp(e1p->levo,e2p->levo) != 0)
	        return FALSE ;

	}

	return TRUE ;
}
/* end subroutine (entry_equal) */


static int entry_free(ep)
struct mipsdis_entry	*ep ;
{


	if (ep->std != NULL)
	    free(ep->std) ;

	if (ep->levo != NULL)
	    free(ep->levo) ;

	ep->std = ep->levo = NULL ;
	return 0 ;
}




/* this is the old clunker that interfaced to Sean's thing */
int mipsdis_server(program,instr,disstd,dislevo,slp,llp)
char	program[] ;
uint	instr ;
char	disstd[] ;
char	dislevo[] ;
int	*slp, *llp ;
{
	bfile	ofile, efile ;
	bfile	*fpa[3] ;

	int	rs, pid, sl ;
	int	childstat ;

	char	cmd[CMDLEN + 1] ;
	char	*sp ;


#if	F_DEBUGS
	eprintf("mipsdis: entered program=%s instr=%08x\n",
	    program,instr) ;
#endif

	if ((program == NULL) || (program[0] == '\0'))
	    program = MIPSDIS_PROGRAM ;

	if (disstd != NULL)
	    disstd[0] = '\0' ;

	if (dislevo != NULL)
	    dislevo[0] = '\0' ;

	if (slp != NULL)
	    *slp = -1 ;

	if (llp != NULL)
	    *llp = -1 ;

#if	F_DEBUGS
	eprintf("mipsdis: 2\n") ;
#endif

	fpa[0] = NULL ;
	fpa[1] = &ofile ;
	fpa[2] = &efile ;

	rs = bufprintf(cmd,CMDLEN,"%s --single-instruction=0x%x",
	    program,instr) ;

#if	F_DEBUGS
	eprintf("mipsdis: bufprintf() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return rs ;

#if	F_DEBUGS
	eprintf("mipsdis: cmd=>%s<\n",cmd) ;
#endif

	if ((rs = bopencmd(fpa,cmd)) >= 0) {

	    pid = rs ;

#if	F_DEBUGS
	    eprintf("mipsdis: pid=%d\n",pid) ;
#endif

	    rs = bread(fpa[1],cmd,MIPSDIS_BUFLEN) ;

#if	F_DEBUGS
	    eprintf("mipsdis: bread() 1 rs=%d\n",rs) ;
#endif

	    if (rs > 0) {

	        sl = MIN(MIPSDIS_BUFLEN,rs) ;
	        sp = strwcpy(disstd,cmd,sl) ;

#if	F_DEBUGS
	        eprintf("mipsdis: disstd=>%s>\n",disstd) ;
#endif

	        if (slp != NULL)
	            *slp = (sp - disstd) ;

	    }

	    rs = bread(fpa[2],cmd,MIPSDIS_BUFLEN) ;

#if	F_DEBUGS
	    eprintf("mipsdis: bread() 2 rs=%d\n",rs) ;
#endif

	    if (rs > 0) {

	        sl = MIN(MIPSDIS_BUFLEN,rs) ;
	        sp = strwcpy(dislevo,cmd,sl) ;
#if	F_DEBUGS
	        eprintf("mipsdis: dislevo=>%s>\n",dislevo) ;
#endif

	        if (llp != NULL)
	            *llp = (sp - dislevo) ;

	    }

#if	F_DEBUGS
	    eprintf("mipsdis: waitpid()\n") ;
#endif

	    u_waitpid(pid,&childstat,WUNTRACED) ;

	    bclose(fpa[1]) ;

	    bclose(fpa[2]) ;

	} /* end if (opened the program) */

#if	F_DEBUGS
	eprintf("mipsdis: exiting rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (mipsdis) */



