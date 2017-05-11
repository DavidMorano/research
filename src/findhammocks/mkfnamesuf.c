/* mkfnamesuf */

/* make a file name from four parts (one base and some suffixes) */


/* revision history:

	= 2001-12-03, David Morano

	This was made specifically for the HDB UUCP modified code.


*/

/* Copyright © 2001 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine constructs a filename (a single filename
	component) out of four possible strings.  The first string is
	a base name, the other three strings are suffix components.

	Synopsis:

	int mkfnamesuf(ofname,p1,s1,s2,s3)
	char		ofname[] ;
	const char	p1[] ;
	const char	s1[] ;
	const char	s2[] ;
	const char	s3[] ;

	Arguments:

	ofname		buffer for the resulting filename
	p1		first component
	s1		first suffix
	s2		second suffix
	s3		third suffix

	Returns:

	>=0		OK, length of resulting filename
	<0		error


*******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<string.h>

#include	<vsystem.h>


/* forward references */

int mkfnamesuf(char *,const char *,const char *,const char *,const char *) ;


/* exported subroutines */


int mkfnamesuf1(ofname,p1,s1)
char		ofname[] ;
const char	p1[] ;
const char	s1[] ;
{


	return mkfnamesuf(ofname,p1,s1,NULL,NULL) ;
}
/* end subroutine (mkfnamesuf1) */


int mkfnamesuf2(ofname,p1,s1,s2)
char		ofname[] ;
const char	p1[] ;
const char	s1[], s2[] ;
{


	return mkfnamesuf(ofname,p1,s1,s2,NULL) ;
}
/* end subroutine (mkfnamesuf2) */


int mkfnamesuf3(ofname,p1,s1,s2,s3)
char		ofname[] ;
const char	p1[] ;
const char	s1[], s2[], s3[] ;
{


	return mkfnamesuf(ofname,p1,s1,s2,s3) ;
}
/* end subroutine (mkfnamesuf3) */


/* COKE: the real thing! */
int mkfnamesuf(ofname,p1,s1,s2,s3)
char		ofname[] ;
const char	p1[] ;
const char	s1[], s2[], s3[] ;
{
	int	rs = SR_OVERFLOW ;
	int	rlen = MAXPATHLEN ;
	int	ml ;
	int	f ;

	char	*bp = ofname ;


/* the base path-name */

	ml = strlcpy(bp,p1,rlen) ;
	if (ml >= rlen) goto ret0 ;
	rlen -= ml ;
	bp += ml ;

/* the period */

	f = FALSE ;
	f = f || (s1 != NULL) ;
	f = f || (s2 != NULL) ;
	f = f || (s3 != NULL) ;
	if (f && (s1 != NULL)) f = (s1[0] != '.') ;
	if (f) {
	    if (rlen < 1) goto ret0 ;
	        *bp++ = '.' ;
	        rlen -= 1 ;
	} /* end if (suffix was non-NUL) */

/* suffix one */

	if (s1 != NULL) {
	    ml = strlcpy(bp,s1,rlen) ;
	    if (ml >= rlen) goto ret0 ;
	    rlen -= ml ;
	    bp += ml ;
	}

/* suffix two */

	if (s2 != NULL) {
	    ml = strlcpy(bp,s2,rlen) ;
	    if (ml >= rlen) goto ret0 ;
	    rlen -= ml ;
	    bp += ml ;
	}

/* suffix three */

	if (s3 != NULL) {
	    ml = strlcpy(bp,s3,rlen) ;
	    if (ml >= rlen) goto ret0 ;
	    bp += ml ;
	}

/* done */

	*bp = '\0' ;
	rs = SR_OK ;

ret0:
	return (rs >= 0) ? (bp - ofname) : rs ;
}
/* end subroutine (mkfnamesuf) */



