this is some parts which david had added to my lmem-init function and
i removed it.

I have to ask first and then let them be in my code


/* set some defaults */

      
	if (lmp->interleaved == 0)
		lmp->interleaved = LMEM_INTERLEAVED ;


/* fetch some IW specific stuff from the parameter files */

	for (i = 0 ; params[i] != NULL ; i += 1) {

	    if ((sl = paramfile_fetch(pfp,params[i],NULL,&cp)) >= 0) {

	        switch (i) {

	        case LMEMPARAM_ICACHESIZE:
	        case LMEMPARAM_DCACHESIZE:
	            if (cfnumui(cp,sl,&uv) >= 0) {

	                switch (i) {

	                case LMEMPARAM_ICACHESIZE:
	                    p_icachesize = uv ;
	                    break ;

	                case LMEMPARAM_DCACHESIZE:
	                    p_dcachesize = uv ;
	                    break ;

	                } /* end switch */

	            } /* end if */

	            break ;

	        } /* end switch (outer) */

	    } /* end if (got a good parameter) */

	} /* end for (parameters) */


/* apply some default is necessary */

#if	F_DEBUG
	if (pip->debuglevel > 1)
	    eprintf("lmem_init: 1 btrbsize=%d\n",iwp_btrbsize) ;
#endif

	if (p_icachesize == 0)
	    p_icachesize = LMEM_ICACHESIZE ;


