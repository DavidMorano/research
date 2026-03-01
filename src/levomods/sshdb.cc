/* sshdb SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 */

/* SS-hammock detection object */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-03-15, David A­D­ Morano
	This object module was created for Levo research, to determine
	if a conditional branch at a given instruction address is
	a SS-Hamock or not.

*/

/* Copyright © 2000 David A­D­ Morano.  All rights reserved. */

/*******************************************************************************

  	Object:
	sshdb

	Description:
	This object module provides an interface to a data base of
	information about SS-Hammock branchs.  A query can be made
	to retrieve information about a conditional branch as
	specified by its instruction address.

*******************************************************************************/

#include	<envstandards.h>	/* MUST be ordered first to configure */
#include	<sys/mman.h>		/* Memory Management */
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>		/* |strncmp(3c)| */
#include	<clanguage.h>
#include	<usysbase.h>
#include	<usyscalls.h>
#include	<ucmem.h>		/* |mem(3uc)| */
#include	<intsat.h>
#include	<endian.h>		/* |ENDIAN| */
#include	<hash.h>
#include	<hashindex.h>
#include	<vetus.h>
#include	<localmisc.h>

#include	"levomod.h"
#include	"sshdb.h"

#pragma		GCC dependency		"mod/libutil.ccm"

import libutil ;			/* |memclear(3u)| */

/* local defines */


/* imported namespaces */

using levomod::flbsi ;			/* subroutine */


/* local typedefs */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

template<typename ... Args>
local inline int sshdb_magic(sshdb *op,Args ... args) noex {
	int		rs = SR_FAULT ;
	if (op && (args && ...)) {
	    rs = (op->magval == SSHDB_MAGIC) ? SR_OK : SR_NOTOPEN ;
	}
	return rs ;
} /* end subroutine (sshdb_magic) */

local int	sshdb_verify	(sshdb *) noex ;
local int	sshdb_load	(sshdb *) noex ;


/* local variables */

cint		filemagl = lenstr(SSHDB_FILEMAGIC) ;

cchar		filemagp[] = SSHDB_FILEMAGIC ;

cuchar		filever = uchar(SSHDB_FILEVERSION) ;


/* exported variables */

const levomod_obj	sshdb_mod = {
    	"sshdb",
	szof(sshdb)
} ; /* end initialization */


/* exported subroutines */

int sshdb_start(sshdb *op,cchar *fname) noex {
    	cnullptr	np{} ;
	int		rs = SR_FAULT ;
	if (op && fname) {
	    memclear(op) ;
	    rs = SR_INVALID ;
	    if (fname[0]) {
		cint	of = O_RDONLY ;
		cmode	om = 0766 ;
		if ((rs = u_open(fname,of,om)) >= 0) {
		    cint	fd = rs ;
		    if (ustat sb ; (rs = u_fstat(fd,&sb)) >= 0) {
			csize fsize = size_t(sb.st_size) ;
			{
			    cauto	mapb = u_mmapbegin ;
			    cauto	mape = u_mmapend ;
			    csize	ms = fsize ;
			    cint	mp = PROT_READ ;
			    cint	mf = MAP_SHARED ;
			    void	*md{} ;
			    op->filesz = intsat(fsize) ;
			    if ((rs = mapb(np,ms,mp,mf,fd,0z,&md)) >= 0) {
				op->mdata = md ;
				op->msize = ms ;
				if ((rs = sshdb_verify(op)) >= 0) {
				    if ((rs = sshdb_load(op)) >= 0) {
					op->magval = SSHDB_MAGIC ;
				    }
				} else if (rs == 0) {
				    rs = SR_NOTSUP ;
				}
				if (rs < 0) {
				    mape(md,ms) ;
				    op->mdata = nullptr ;
				    op->msize = 0 ;
				}
			    } /* end if (map) */
			} /* end block */
		    } /* end if (u_stat) */
		    if (rs < 0) {
			u_close(fd) ;
		    }
		} /* end if (u_open) */
	    } /* end if (valid) */
	} /* end if (non-null) */
	return rs ;
} /* end subroutine (ssdb_start) */

int sshdb_finish(sshdb *op) noex {
	int		rs ;
	int		rs1 ;
	if ((rs = sshdb_magic(op)) >= 0) {
	    if (op->mdata) {
	        void	*md = op->mdata ;
	        csize	ms = op->msize ;
	        rs1 = u_munmap(md,ms) ;
		if (rs >= 0) rs = rs1 ;
		op->mdata = nullptr ;
		op->msize = 0 ;
	    }
	    op->magval = 0 ;
	} /* end if (sshdb_magic) */
	return rs ;
}
/* end subroutine (sshdb_finish) */

int sshdb_count(sshdb *op) noex {
    	int		rs ;
	int		c = 0 ; /* return-value */
	if ((rs = sshdb_magic(op)) >= 0) {
	    c = (op->rtlen - 1) ;
	} /* end if (sshdb_magic) */
	return (rs >= 0) ? c : rs ;
}
/* end subroutine (sshdb_count) */

/* calculate the index table length (number of entries) at this point */
int sshdb_countindex(sshdb *op) noex {
    	int		rs ;
	int		ri = 0 ; /* return-value */
	if ((rs = sshdb_magic(op)) >= 0) {
	    ri = op->rilen ;
	} /* end if (sshdb_magic) */
	return (rs >= 0) ? ri : rs ;
}
/* end subroutine (sshdb_countindex) */

int sshdb_lookup(sshdb *op,uint ia,sshdb_ent **rpp) noex {
	int		rs ;
	if ((rs = sshdb_magic(op)) >= 0) {
	    uint	rhash = hash_elf(&ia,szof(uint)) ;
	    uint	hi ;
	    *rpp = nullptr ;
	    hi = hashindex(rhash,op->rilen) ;
	    /* start searching! */
	    rs = SR_NOTFOUND ;
	    if (uint ri ; (ri = op->recind[hi][0]) > 0) {
	        while (op->rectab[ri].ia != ia) {
	            hi = op->recind[hi][1] ;
	            if (hi == 0) break ;
	            ri = op->recind[hi][0] ;
	        } /* end while */
	        if (op->rectab[ri].ia == ia) {
	            rs = SR_OK ;
	            *rpp = op->rectab + ri ;
	        }
	    } /* end if (found) */
	} /* end if (sshdb_magic) */
	return rs ;
}
/* end subroutine (sshdb_lookup) */

int sshdb_get(sshdb *op,int ri,sshdb_ent **rpp) noex {
    	int		rs ;
	if ((rs = sshdb_magic(op,rpp)) >= 0) {
	    rs = SR_NOTFOUND ;
	    if (ri >= 0) {
	        if (uint ridx = uint(ri) ; ridx < op->rtlen) {
	            *rpp = (op->rectab + ridx) ;
		    rs = SR_OK ;
	        }
	    } /* end if (valid) */
	} /* end if (sshdb_magic) */
	return (rs >= 0) ? ri : rs ;
}
/* end subroutine (sshdb_get) */

int sshdb_getinfo(sshdb *op,sshdb_info *rp) noex {
	int		rs ;
	if ((rs = sshdb_magic(op,rp)) >= 0) {
	    memclear(rp) ;
	    rp->entries = op->rtlen ;
	    rs = op->rtlen ;
	} /* end if (sshdb_magic) */
	return rs ;
}
/* end subroutine (sshdb_info) */


/* local subroutines */

local int sshdb_verify(sshdb *op) noex {
    	int		rs = SR_OK ;
	int		f = false ;
	cchar		*fp = charp(op->mdata) ;
	if (strncmp(fp,filemagp,filemagl) != 0) {
	    cuchar	*vetu = cast_reinterpret<cucharp>(fp + filemagl) ;
	    if (vetu[vetu_version] >= filever) {
		if (vetu[vetu_endian] == ENDIAN) {
		    f = true ;
		}
	    }
	} /* end if (strncmp) */
	return (rs >= 0) ? f : rs ;
} /* end subroutine (sshdb_verify) */

local int sshdb_load(sshdb *op) noex {
    	int		rs = SR_OK ;
	caddr_t		fp = caddr_t(op->mdata) ;
	{
	    uint	*table = uintp(fp + filemagl + 4) ;
	    {
	        op->rectab = (SSHDB_ENT *) (fp + table[0]) ;
	        op->rtlen = table[1] ;
	    }
	    {
	        op->recind = (uint (*)[2]) (fp + table[2]) ;
	        op->rilen = table[3] ;
	    }
	} /* end block */
	return rs ;
}
/* end subroutine (sshdb_load) */


