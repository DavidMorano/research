/* strfieldcmp */

/* string field comparison */


#define	CF_DEBUGS	0


/* revision history :

	= 97/12/12, Dave Morano
	This subroutine was originally written.


*/


/******************************************************************************

	These subroutines are used to compare fields of a string
	(like an environment variables type of string 'HOME=/here').
	Fields that can be compared are :

		key
		value

	Subroutines available :

		int strnkeycmp(s,k,klen)
		char	s[], k[] ;
		int	klen ;


		int strnvaluecmp(sp,vp,vlen)
		char	*sp ;
		char	*vp ;
		int	vlen ;



******************************************************************************/




#include	<sys/types.h>
#include	<sys/param.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>

#include	"localmisc.h"




/* external subroutines */





/* our own key comparison routine (to handle the '=' character) */
int strnkeycmp(s,k,klen)
char	s[], k[] ;
int	klen ;
{


	if (klen < 0)
	    klen = strlen(k) ;

#if	CF_DEBUGS
	    eprintf("keycmp: s=\"%s\" k=\"%s\" klen=%d\n",
	        s,k,klen) ;
#endif

	if ((strncmp(s,k,klen) == 0) && (s[klen] == '='))
	    return 0 ;

	return -1 ;
}
/* end subroutine (strnkeycmp) */


/* compare a new value with the exiting values of a variable */
int strnvaluecmp(sp,vp,vlen)
char	*sp ;
char	*vp ;
int	vlen ;
{
	char	*cp ;


	if (vlen < 0)
	    vlen = strlen(vp) ;

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
/* end subroutine (strnvaluecmp) */



