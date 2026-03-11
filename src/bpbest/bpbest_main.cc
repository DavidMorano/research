/* bpbest_main SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* estimate for branch prediction */
/* version %I% last-modified %G% */


/* revision history:

	= 2001-04-13, David A-D- Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright © 2001 David A­D­ Morano.  All rights reserved. */
/* Use is subject to license terms. */

/*******************************************************************************

  	Name:
	bpbest

	Description:
	Determine (calculate) the best branch predictor.

*******************************************************************************/

#include	<envstandards.h>	/* ordered first to configure */
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>		/* |getenv(3c)| */
#include	<cstdio>
#include	<clanguage.h>
#include	<usysbase.h>
#include	<localmisc.h>

#include	<levomod.h>

/* local defines */

#define	N_ROWS		32
#define	N_ENTRIES	64
#define	N_OP		2
#define	N_STRIDE	8

#define	NB_BIT	8
#define	NB_NAND	4
#define	NB_INV	2
#define	NB_XOR	(4 * NB_NAND)


/* imported namespaces */

using levomod::flbs ;			/* subroutine */


/* local typedefs */


/* external subroutines */


/* external subroutines */


/* forward references */

local int	costbpred(int) noex ;


/* local variables */


/* exported variables */


/* exported subroutines */

int main(int argc,mainv argv) {
	int	ex = EXIT_SUCCESS ;
	int	rows ;
	int	a ;
	int	cost ;

	rows = 32 ;
	a = 32 ;
	cost = costbpred(a) ;
	fprintf(stdout,
		"32-bit address cost=%d total=%d\n",cost,(cost * rows)) ;

	a = 64 ;
	cost = costbpred(a) ;
	fprintf(stdout
		,"64-bit address cost=%d total=%d\n",cost,(cost * rows)) ;

	fclose(stdout) ;
	return ex ;
}
/* end subroutine (main) */


/* local subroutines */

local int costbpred(int a) noex {
	int	nand = 4 ;
	int	cs, cd, cc, cm, csh, ci, ca ;
	int	h, j, k, s, p ;
	int	i ;
	int	cost ;
	int	cost_bht, cost_pht ;

	j = 0 ;
	h = 1024 ;
	k = 12 ; /* 4096 */
	s = 2 ;
	p = 4096 ;
	i = flbsi(h) ;

	cs = 8 ;
	cd = 10 ;	/* decoder transitors per bit */
	cc = (4 * nand) ;
	cm = cd ;
	csh = (2 * nand) ;
	ci = (4 * nand) ;
	ca = (6 * nand) ;

	cost_bht = h * ((a + 2 * j + k + 1 - i) * cs + k * csh) ;
	cost_pht = (1 << k) * (s * cs + cd) ;
	cost = cost_bht + cost_pht ;

	return cost ;
} /* end subroutine (costbpred) */


