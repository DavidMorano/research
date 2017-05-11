
	    struct machstate	ms ;

	    SS_MACHOBJ		mobj ;

	    SSIW		iwindow ;

	    SSIW_INITARGS	iwa ;

	    bfile	sfile ;

	    ULONG	ins, in_end ;
	    ULONG	cks, ck_end ;

	    double	ipc_instrs, ipc_clocks, ipc ;
	    double	ips_elapsed, ips ;

	    time_t	time_start = time(NULL) ;
	    time_t	elapsed ;

	    int		f_sfile ;

	    char	timebuf[TIMEBUFLEN + 1] ;


	fprintf(stderr, "sim: ** starting functional simulation **\n") ;

/* open up the checker window */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_main: open checker window, sim_exec() \n") ;
#endif

	n = csp->wsize ;
	rs = sim_exec(ssp,n) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (DEBUGLEVEL(3))
	    eprintf("sim_main: sim_exec() rs=%d in=%llu\n",
	        rs,csp->in) ;
#endif

	    if (pip->f.iflags)
	        iflags_init(&pip->ifl) ;

/* initialize the machine state so far */

	    memset(&ms,0,sizeof(struct machstate)) ;

	    ms.in = csp->in ;
	    ms.ia = csp->regs.regs_PC ;


/* initialize the clocker */

	    memset(&mobj,0,sizeof(SS_MACHOBJ)) ;

	    mobj.topobjp = &iwindow ;
	    mobj.topcombp = (int (*)(void *,int)) ssiw_comb ;
	    mobj.topclockp = (int (*)(void *)) ssiw_clock ;

	    ss_setmach(ssp,&mobj) ;


/* initialize the Levo instruction window (SSIW) */

	    memset(&iwa,0,sizeof(SS_MACHOBJ)) ;

	    iwa.smp = &ms ;
	    iwa.iwsize = lip->iwsize ;

	    ssiw_init(&iwindow,pip,ssp,lip,&iwa) ;


/* initialize the functional starting registers */

	    for (j = 0 ; j < MACHSTATE_NREGS ; j += 1) {

	        ULONG	v ;


	        regs_getval(&csp->regs,j,&v) ;

	        ssiw_setreg(&iwindow,j,v) ;

	    } /* end for */


/* do the main loop */

/* set starting instruction count for "tell" progress dumps */

	    pip->s.in_start = csp->in ;
	    pip->s.ck_start = csp->in ;		/* started w/ fake clock */

	    fprintf(stderr,"start instruction count=%llu\n",
	        pip->s.in_start) ;

	    rs = ss_loop(ssp,csp->in,csp->in) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (DEBUGLEVEL(3))
	        eprintf("sim_main: ss_loop() rs=%d\n",rs) ;
#endif

	    if (rs < 0) {

	        bfile	dumpfile ;


	        mkfnamesuf(tmpfname,pip->jobname,FNE_DUMP) ;

	        rs1 = bopen(&dumpfile,tmpfname,"wct",0666) ;

	        if (rs1 >= 0) {

	            bprintf(&dumpfile,"dump rs=%d jobname=%s\n",
	                rs,pip->jobname) ;

	            bprintf(&dumpfile,"ck=%llu:%u\n",
	                ck_end,ssp->phase) ;

	            ssiw_dump(&iwindow,&dumpfile) ;

	            bclose(&dumpfile) ;

	        } /* end if (opened dump file) */

	    } /* end if (a dump was needed) */

	    ss_getinstr(ssp,&in_end) ;

	    ss_getclock(ssp,&ck_end) ;

	    pip->s.ins = in_end - pip->s.in_start ;
	    pip->s.cks = ck_end - pip->s.ck_start ;

/* do we have an STATS file */

	    rs1 = SR_NOENT ;
	    if (pip->statsfname[0] != '\0')
	        rs1 = bopen(&sfile,pip->statsfname,"wa",0666) ;

	    f_sfile = (rs1 >= 0) ;

	    fprintf(stderr,"sfile rs=%d\n",rs1) ;

/* calculate some stuff */

	    elapsed = time(NULL) - time_start ;

	    fprintf(stderr,"jobname=%s ers=%d\n",pip->jobname,rs) ;

	    fprintf(stderr,"end instruction count EIC==%llu\n",
	        in_end) ;

	    ins = in_end - pip->s.in_start ;
	    fprintf(stderr,"clocked instruction count CIC=%llu\n",
	        ins) ;

	    cks = ck_end - pip->s.ck_start ;
	    fprintf(stderr,"clock count CC=%llu\n",cks) ;

	    if (f_sfile) {

	        bprintf(&sfile,"= execution profile\n") ;

	        bprintf(&sfile,
	            "itype_ins=          %10llu (N)\n",
	            pip->s.itype_ins) ;

	        bprintf(&sfile,
	            "itype_cf=           %10llu (%6.1f%% CF/N)\n",
	            pip->s.itype_cf,
	            percentll(pip->s.itype_cf, pip->s.itype_ins)) ;

	        bprintf(&sfile,
	            "itype_cfcond=       %10llu (%6.1f%% CFC/N)\n",
	            pip->s.itype_cfcond,
	            percentll(pip->s.itype_cfcond, pip->s.itype_ins)) ;

	        bprintf(&sfile,
	            "itype_cfsub=        %10llu (%6.1f%% CFS/N)\n",
	            pip->s.itype_cfsub,
	            percentll(pip->s.itype_cfsub, pip->s.itype_ins)) ;

	        bprintf(&sfile,
	            "itype_cfdir=        %10llu (%6.1f%% CFD/N)\n",
	            pip->s.itype_cfdir,
	            percentll(pip->s.itype_cfdir, pip->s.itype_ins)) ;

	        bprintf(&sfile,
	            "itype_cfind=        %10llu (%6.1f%% CFI/N)\n",
	            pip->s.itype_cfind,
	            percentll(pip->s.itype_cfind, pip->s.itype_ins)) ;

	        bprintf(&sfile,
	            "itype_mem=          %10llu (%6.1f%% M/N)\n",
	            pip->s.itype_mem,
	            percentll(pip->s.itype_mem, pip->s.itype_ins)) ;

	        bprintf(&sfile,
	            "itype_memload=      %10llu "
	            "(%6.1f%% ML/N, %6.1f%% ML/M)\n",
	            pip->s.itype_memload,
	            percentll(pip->s.itype_memload,pip->s.itype_ins),
	            percentll(pip->s.itype_memload,pip->s.itype_mem)) ;

	        bprintf(&sfile,
	            "itype_memstore=     %10llu "
	            "(%6.1f%% MS/N, %6.1f%% MS/M)\n",
	            pip->s.itype_memstore,
	            percentll(pip->s.itype_memstore,pip->s.itype_ins),
	            percentll(pip->s.itype_memstore,pip->s.itype_mem)) ;

	        for (i = 0 ; i < iclass_overlast ; i += 1) {

	            bprintf(&sfile,"itype_class_%s= %10llu\n",
	            "(%6.1f%% type/N)\n",
	                ssfunames[i],
	                pip->s.itype_class[i],
	            percentll(pip->s.itype_class[i],pip->s.itype_ins)) ;

	        } /* end for */

	        bprintf(&sfile,"\n") ;

/* simulation results */

	        bprintf(&sfile,"= simulation results\n") ;

	        bprintf(&sfile,"ers=%d\n",rs) ;

	        bprintf(&sfile,"end instruction count EIC=%llu\n",
	            in_end) ;

	        bprintf(&sfile,"clocked instruction count CIC=%llu\n",
	            ins) ;

	        bprintf(&sfile,"clock count CC=%llu\n",
	            cks) ;

	    }

	    ipc_instrs = (double) ins ;
	    ipc_clocks = (double) cks ;
	    ips_elapsed = (double) (elapsed > 0) ? elapsed : 1 ;

	    ipc = 0.0 ;
	    if (cks > 0)
	        ipc = ipc_instrs / ipc_clocks ;

	    fprintf(stderr,"IPC=%8.3f\n",ipc) ;

	    fprintf(stderr,"elapsed clocked time=%s\n",
	        timestr_elapsed(elapsed,timebuf)) ;

	    ips = ipc_instrs / ips_elapsed ;
	    fprintf(stderr,"sim speed IPS=%16.2f\n",ips) ;

	    fprintf(stderr,"sim speed IPH=%16.2f\n",(ips * 3600.0)) ;

	    if (f_sfile) {

	        bprintf(&sfile,"IPC=%8.3f\n",ipc) ;

	        bprintf(&sfile,"elapsed clocked time=%s\n",
	            timestr_elapsed(elapsed,timebuf)) ;

	        bprintf(&sfile,"sim speed IPS=%16.2f\n",ips) ;

	        bprintf(&sfile,"sim speed IPH=%16.2f\n",(ips * 3600.0)) ;

	        bprintf(&sfile,"\n") ;

	        bclose(&sfile) ;

	    }

/* get and print some statistics */

	    ssiw_printstats(&iwindow,NULL) ;

/* free stuff up */

	    ssiw_free(&iwindow) ;

	    if (pip->f.iflags) {

	        char	tmpfname[MAXPATHLEN + 1] ;


	        mkfnamesuf(tmpfname,pip->jobname,FNE_IFLAGS1) ;

	        iflags_free(&pip->ifl,ins,tmpfname) ;

	    }

