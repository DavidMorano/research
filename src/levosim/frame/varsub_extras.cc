/* varsub_extras */

/* variable manipulation (like for configuration files !) */


#define	CF_DEBUGS	0
#define	CF_DEBUGS2	0


/* revision history :

	= 97/12/12, Dave Morano

	This subroutine was written to replace some older variable
	substitution stuff from the old Automation Rsearch days with
	the old SAT tool stuff.


	= 01/09/11, Dave Morano

	This junk is not aging well !  And it was really rewritten from
	essentially scratch in 1997 (only a few years ago) !  This code
	has many qualities that is common with crap code.  When a new
	variable was "exported" from a configuration file and it didn't
	have any value part specified, and when the same variable was
	not in the existing environment (like if a daemon is executed
	directly from 'init(1m)' or something similar), then the
	variable name would end up in the exported environment
	variables list as just a variable key with no value !  I fixed
	this by not putting anything into the exported environment if
	it doesn't have a value (at least a value of zero length).  I
	*think* that a NULL value and and a zero-length value
	distinquish the case of the variable having and empty value
	string from one where it did not have any value specified at
	all.


*/


/******************************************************************************

	These routines operate on 'defined' and 'export'ed variables
	from a configuration file.  What we want to do is to perform
	a substitution on these variables from previous variables
	in the database or from the environment otherwise.  Variables
	that are already in the database get merged like in a path
	string with colon characters (':') separating the original
	values of the variables.


******************************************************************************/



#include	<sys/types.h>
#include	<sys/param.h>
#include	<string.h>
#include	<ctype.h>
#include	<malloc.h>

#include	<vsystem.h>
#include	<vecstr.h>
#include	<varsub.h>

#include	"localmisc.h"




/* external subroutines */

extern int	substring(const char *,int,const char *) ;

extern char	*getenv2(), *getenv3() ;


/* external variables */


/* forward references */

static int	keycmp(char *,char *,int) ;
static int	valuecmp() ;





int varsub_loadenv(varp,envp)
varsub	*varp ;
char	*envp[] ;
{
	int	rs ;
	int	i ;

	char	*cp ;
	char	*esp ;
	char	*vp ;


#if	CF_DEBUGS
	eprintf("varsub_loadenv: entered\n") ;
#endif

	rs = SR_OK ;
	for (i = 0 ; envp[i] != NULL ; i += 1) {

#if	CF_DEBUGS
	    eprintf("varsub_loadenv: top loop, envp[%d]=\"%s\"\n",
	        i,envp[i]) ;
#endif

	    esp = envp[i] ;
	    if ((cp = strchr(esp,'=')) == NULL) continue ;

	    vp = cp + 1 ;

#if	CF_DEBUGS

	    eprintf("varsub_loadenv: esp %c is %s\n",
	        esp[0],(isalnum(esp[0]) ? "yes" : "no")) ;

	    eprintf("varsub_loadenv: vp  %c is %s\n",
	        vp[0],(isalnum(vp[0]) ? "yes" : "no")) ;

#endif /* CF_DEBUG */

	    if (isprint(esp[0]) && ((vp[0] == '\0') || isprint(vp[0]))) {

#if	CF_DEBUGS
	        eprintf("varsub_loadenv: about to call 'varsub_add'\n") ;
#endif

	        if (varsub_find(varp,esp,(cp - esp),NULL,NULL) < 0) {

	            rs = varsub_add(varp,esp,cp - esp,vp,
	                ((vp[0] == '\0') ? 0 : -1)) ;

#if	CF_DEBUGS
	            eprintf("varsub_loadenv: varsub_add() rs=%d\n",rs) ;
#endif

	            if ((rs < 0) && (rs != SR_EMPTY))
	                break ;

	        } /* end if (adding) */

	    } /* end if */

	} /* end for */

#if	CF_DEBUGS
	eprintf("varsub_loadenv: exiting\n") ;
#endif

	return rs ;
}
/* end subroutine (varsub_loadenv) */


/* load from vector strings */
int varsub_loadvec(varp,vsp)
varsub	*varp ;
vecstr	*vsp ;
{
	int	rs ;
	int	i ;

	char	*sp, *cp ;
	char	*esp ;
	char	*vp ;


#if	CF_DEBUGS
	eprintf("varsub_loadvec: entered\n") ;
#endif

	rs = SR_OK ;
	for (i = 0 ; vecstr_get(vsp,i,&sp) >= 0 ; i += 1) {

	    if (sp == NULL) continue ;

#if	CF_DEBUGS
	    eprintf("varsub_loadvec: top loop, sp[%d]=\"%s\"\n",
	        i,sp) ;
#endif

	    esp = sp ;
	    if ((cp = strchr(esp,'=')) == NULL) continue ;

	    vp = cp + 1 ;

#if	CF_DEBUGS2
	    eprintf("varsub_loadenv: esp %c is %s\n",
	        esp[0],(isalnum(esp[0]) ? "yes" : "no")) ;

	    eprintf("varsub_loadenv: vp  %c is %s\n",
	        vp[0],(isalnum(vp[0]) ? "yes" : "no")) ;
#endif /* CF_DEBUGS2 */

	    if (isprint(esp[0]) && ((vp[0] == '\0') || isprint(vp[0]))) {

#if	CF_DEBUGS2
	        eprintf("varsub_loadenv: about to call 'varsub_add'\n") ;
#endif

	        rs = varsub_add(varp,esp,cp - esp,vp,-1) ;

	        if (rs < 0)
	            break ;

	    } /* end if */

	} /* end for */

#if	CF_DEBUGS
	eprintf("varsub_loadvec: exiting\n") ;
#endif

	return rs ;
}
/* end subroutine (varsub_loadvec) */


/* perform variable substitutions on a buffer */

/*
	Returns :
	- length of output string

*/

int varsub_subbuf(var1p,var2p,s1,s1len,s2,s2len)
varsub	*var1p, *var2p ;
char	*s1, *s2 ;
int	s1len, s2len ;
{
	int	slen ;


#if	CF_DEBUGS
	eprintf("varsub_subbuf: entered, s1len=%d s2len=%d\n",
	    s1len,s2len) ;
#endif

	if (var1p == NULL)
	    return SR_FAULT ;

	if ((s1 == NULL) || (s2 == NULL)) {

#if	CF_DEBUGS
	    eprintf("varsub_subbuf: exiting invalid\n") ;
#endif

	    return SR_FAULT ;
	}

#if	CF_DEBUGS
	{
	    int	sl2 = MIN(strlen(s1),10) ;

	    eprintf("varsub_subbuf: src=%W\n",s1,sl2) ;
	}
#endif /* CF_DEBUGS */

	slen = varsub_buf(var1p,s1,s1len,s2,s2len) ;

#if	CF_DEBUGS
	eprintf("varsub_subbuf: 1 slen=%d\n",slen) ;
#endif

	if ((var2p != NULL) && (slen <= 0))
	    slen = varsub_buf(var2p,s1,s1len,s2,s2len) ;

#if	CF_DEBUGS
	eprintf("varsub_subbuf: exiting w/ slen=%d\n",slen) ;
#endif

	return slen ;
}
/* end subroutine (varsub_subbuf) */


/* merge variables that are the same into just one (and update all DBs) */
int varsub_merge(varp,vsp,buf,buflen)
varsub	*varp ;
vecstr	*vsp ;
char	buf[] ;
int	buflen ;
{
	int	rs ;
	int	i, j ;
	int	klen, vlen ;
	int	olen, nlen ;
	int	len2 ;
	int	f_novalue ;

	char	*cp ;
	char	*kp, *vp ;
	char	*sp ;
	char	*buf2  = NULL ;
	char	*cp2 ;
	char	*ep ;


#if	CF_DEBUGS
	eprintf("varsub_merge: entered\n") ;
#endif

/* separate the string into its key and value */

	ep = buf ;
	kp = buf ;
	vp = NULL ;
	vlen = -1 ;
	f_novalue = TRUE ;
	if ((klen = substring(buf,buflen,"=")) >= 0) {

	    f_novalue = FALSE ;
	    cp = buf + klen ;
	    vp = cp + 1 ;
	    vlen = strnlen(vp,buflen - klen - 1) ;

	} else
	    klen = strnlen(buf,buflen) ;

#if	CF_DEBUGS
	eprintf("varsub_merge: kp=\"%s\" klen=%d\n",kp,klen) ;
	eprintf("varsub_merge: vp=\"%s\" vlen=%d\n",vp,vlen) ;
#endif /* CF_DEBUGS */


/* search for this key in the given vector string list (exports) */

	for (i = 0 ; (rs = vecstr_get(vsp,i,&sp)) >= 0 ; i += 1) {

	    if (sp == NULL) continue ;

	    if (keycmp(sp,kp,klen) == 0)
	        break ;

	} /* end for */

	nlen = klen + ((vlen >= 0) ? vlen + 1 : 0) ;

#if	CF_DEBUGS
	eprintf("varsub_merge: nlen=%d klen=%d vlen=%d\n",
	    nlen,klen,vlen) ;
#endif

	if (rs >= 0) {

#if	CF_DEBUGS
	    eprintf("varsub_merge: found it already\n") ;
#endif

	    if (vlen >= 0) {

#if	CF_DEBUGS
	        eprintf("varsub_merge: value has non-negative length\n") ;
#endif

/* we already had an entry like this one */

/* do we have this particular value in this variable's value list already ? */

	        if (valuecmp(sp,vp,vlen) == 0) {

#if	CF_DEBUGS
	            eprintf("varsub_merge: already had this value\n") ;
#endif

	            return SR_OK ;
	        }

/* prepare to add this new value to the end of the existing values ! */

	        olen = strlen(sp) ;

	        nlen = olen + ((vlen >= 0) ? vlen + 1 : 0) ;
	        buf2 = (char *) malloc(nlen + 1) ;

	        if (buf2 == NULL)
	            return SR_FAULT ;

	        ep = buf2 ;
	        kp = buf2 ;
	        strcpy(buf2,sp) ;

/* delete it from the vector string list */

#if	CF_DEBUGS
	        eprintf("varsub_merge: deleting from VECSTR\n") ;
#endif

	        (void) vecstr_del(vsp,i) ;

/* delete the old one from the substitution array */

	        if (varp != NULL) {

#if	CF_DEBUGS
	            eprintf("varsub_merge: deleting from VARSUB\n") ;
#endif

	            if ((j = varsub_find(varp,kp,klen,&cp2,&len2)) >= 0) {

#if	CF_DEBUGS
	                eprintf("varsub_merge: found it \n") ;
#endif

	                (void) varsub_del(varp,j) ;

	            } /* end if (found it) */

#if	CF_DEBUGS
	            eprintf("varsub_merge: deleted from VARSUB\n") ;
#endif

	        } /* end if (deleting from VARSUB DB) */

/* add this new part to the end of what we already have */

#if	CF_DEBUGS
	        eprintf("varsub_merge: rest 1\n") ;
#endif

	        buf2[olen] = ':' ;
	        strwcpy(buf2 + olen + 1,vp,vlen) ;

	        vp = buf2 + klen + 1 ;
	        vlen = nlen - klen - 1 ;

	    } else
	        return SR_OK ;

	} else {

#if	CF_DEBUGS
	    eprintf("varsub_merge: we did not have it already\n") ;
#endif

/* it did NOT exist already */

	    if (vlen < 0) {

	        vlen = 0 ;
	        if ((vp = getenv3(kp,klen,&ep)) != NULL) {

/* we let anyone who cares to figure out these lengths by themselves */

	            nlen = -1 ;
	            vlen = -1 ;

	        } /* end if (we had it as an environment variable) */

#if	CF_DEBUGS
	        eprintf("varsub_merge: getenv3, vp=%s\n",vp) ;
#endif

	    } /* end if (vlen < 0) */

	} /* end if */


/* add the new variable to the various DBs */

	if (vp != NULL) {

/* add the new string variable to the running string list */

#if	CF_DEBUGS
	    eprintf("varsub_merge: bottom part, ep=\"%s\" nlen=%d\n",
	        ep,nlen) ;
#endif

	    if ((rs = vecstr_add(vsp,ep,nlen)) < 0) {

#if	CF_DEBUGS
	        eprintf("varsub_merge: returning after vecstr_add\n") ;
#endif

	        return rs ;
	    }


/* add the new string variable to the variable_substitution_array */

#if	CF_DEBUGS
	    eprintf("varsub_merge: about to add to varsub (for defines)\n") ;
	    eprintf("varsub_merge: klen=%d kp=%W\n",klen,kp,klen) ;
	    eprintf("varsub_merge: vlen=%d vp=%W\n",vlen,vp,vlen) ;
#endif /* CF_DEBUGS */

	    if ((varp != NULL) && (vlen >= 0)) {

#if	CF_DEBUGS
	        eprintf("varsub_merge: adding to the VARSUB DB\n") ;
#endif

	        rs = varsub_add(varp,kp,klen,vp,vlen) ;

#if	CF_DEBUGS

	        rs = varsub_find(varp,kp,klen,&cp2,&len2) ;

	        eprintf("varsub_merge: was it indeed added, rs=%d, kp=%s\n",
	            rs,kp) ;

#endif /* CF_DEBUGS */

	    } /* end if (adding to the variable substitution VARSUB DB) */

#if	CF_DEBUGS
	    eprintf("varsub_merge: freeing buf\n") ;
#endif

	} /* end if (adding new variable to DBs) */

	if (buf2 != NULL)
	    free(buf2) ;

#if	CF_DEBUGS
	if (varp != NULL) {

	    int	len1 ;

	    char	*cp1 ;


	    eprintf("varsub_merge: VSA so far :\n") ;

	    for (i = 0 ; 
	        varsub_get(varp,i,&cp1,&len1,&cp2,&len2) >= 0 ; i += 1) {

	        if (cp1 == NULL)
	            eprintf("varsub_merge: kp=NULL\n") ;

	            else
	            eprintf("varsub_merge: kp=%s vp=%s\n",cp1,cp2) ;

	    }
	}

	eprintf("varsub_merge: exiting\n") ;
#endif /* CF_DEBUG */

	return rs ;
}
/* end subroutine (varsub_merge) */



/* EXTRA SUBROUTINES */



int varsub_load(varp,envp)
varsub	*varp ;
char	*envp[] ;
{


	return varsub_loadenv(varp,envp) ;
}



/* INTERNAL SUBROUTINES */




/* our own key comparison routine (to handle the '=' character) */
static int keycmp(s,k,klen)
char	s[], k[] ;
int	klen ;
{


#if	CF_DEBUGS
	eprintf("keycmp: s=\"%s\" k=\"%s\" klen=%d\n",
	    s,k,klen) ;
#endif

	if ((strncmp(s,k,klen) == 0) && (s[klen] == '='))
	    return 0 ;

	return -1 ;
}
/* end subroutine (keycmp) */


/* compare a new value with the exiting values of a variable */
static int valuecmp(sp,vp,vlen)
char	*sp ;
char	*vp ;
int	vlen ;
{
	char	*cp ;


#if	CF_DEBUGS
	eprintf("var/valuecmp: entered\n") ;
	eprintf("var/valuecmp: sp=%s\n",sp) ;
	eprintf("var/valuecmp: vp=%W\n",vp,vlen) ;
#endif

	if ((cp = strchr(sp,'=')) == NULL)
	    return -1 ;

	sp = cp + 1 ;

#if	CF_DEBUGS
	eprintf("var/valuecmp: 2 sp=%s\n",sp) ;
#endif

	while (*sp) {

	    if ((strncmp(sp,vp,vlen) == 0) &&
	        ((sp[vlen] == '\0') || (sp[vlen] == ':')))
	        return 0 ;

	    if ((cp = strchr(sp,':')) == NULL)
	        break ;

	    sp = cp + 1 ;

	} /* end while */

	return -1 ;
}
/* end subroutine (valuecmp) */



