/* stdsio HEADER */
/* lang=C20 */

/* convert a single character to a given case */
/* version %I% last-modified %G% */


/* revision history:

	= 1998-04-10, David A­D­ Morano
	This subroutine was written for Rightcore Network Services.

*/

/* Copyright (c) 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	STDSIO_INCLUDE
#define	STDSIO_INCLUDE


#include	<envstandards.h>	/* MUST be first to configure */
#include	<stdio.h>
#include	<clanguage.h>
#include	<utypedefs.h>
#include	<utypealiases.h>
#include	<usysdefs.h>
#include	<usysrets.h>


EXTERNC_begin

extern int	freadln(FILE *,char *,int) noex ;
extern int	fgetline(FILE *,char *,int) noex ;

EXTERNC_end


#endif /* STDSIO_INCLUDE */


