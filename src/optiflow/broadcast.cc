
/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */


int iw_broadcast(iwp)
IW	*isp ;
{
	OPERAND	*op ;

	int	rs, rs1, rs2 ;
	int	i, j, k ;
	int	n = NUMBER_OF_AS ;
	int	c = 0 ;


	for (i = 0 ; i < n ; i += 1) {

	    rs = SR_OK ;
	    for (j = 0 ; (j < LEVO_MAXOPSO) && (rs >= 0) ; j += 1) {

	        rs1 = las_opget(lasp + i,j,&op) ;

	        if (rs1 > 0) {

	            for (k = 0 ; k < LEVO_MAXOPSI ; k += 1) {

	                rs2 = las_opsnoop((lasp + k),op) ;

	                if (rs2 > 0)
	                    c += rs2 ;

	            } /* end for */

	        } /* end if (got one) */

	    } /* end for */

	} /* end for */

	return c ;
}



