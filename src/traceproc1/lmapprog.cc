/* lmapprog SUPPORT */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* map an ELF object file into memory for execution */
/* version %I% last-modified %G% */

#define	CF_DEBUGS	0		/* non-switchable debugging */
#define	CF_DEBUG	0		/* switchable debug output */
#define	CF_STACKARGS	1		/* prepare stack args */
#define	CF_SGIENV	1		/* setup the same as SGI box */
#define	CF_SECTIONS	1		/* read section information */
#define	CF_SYMTAB	1		/* process symbol table */
#define	CF_SAFE		1		/* extra safe mode */
#define	CF_TESTSTACK	0		/* use a short stack for testing? */
#define	CF_CHECKACCESS	1		/* check access for data reads */
#define	CF_MIPS1	0		/* only accept MIPS1 */

/* revision history:

	= 2000-07-26, David Morano
	I am writing this from stratch after trying to make the best
	use of the MINT related code and becoming disgusted with the
	failure of the attempt!  MINT is changing the code that it
	reads in and also does not read every type of object file
	either!  Worse, MINT inserts into the code some stuff for its
	own purposes that currently escapes understanding.

	= 2001-04-03, David Morano
	I fixed a bug that had to do with the BSS segment being
	zeroed out.  I was not properly zeroing out what I was
	supposed to be.  I hope that it is correct now.  This,
	dispite all the attention to this matter originally!  I
	reviewed the establishment of the program stack area also.
	We need this to compare target program execution with that
	on the SGI box exactly.  It has held up and appears to be
	OK as far as I can tell.

	= 2001-10-25, David Morano
	Oh, I am trying to do something with the program "break" value.
	I would like to privide what UNIX does with |brk(2)| and
	|sbrk(2)|.

*/

/* Copyright © 1998 David A-D- Morano.  All rights reserved. */

/*******************************************************************************

	This module is used to map the simulated program (stored in an
	ELF object file) into memory for simulated execution.  We will
	map the program into memory in a similar way as the OS would do
	at load time.  We will only handle statically linked programs
	right now so we do not have to do any run-time dynamic
	linking.  We will check to make sure that we are not given a
	dynamic executable before we go too far.

	Any program symbols that are present in the ELF file are
	also accessible through this module.

	I am putting the program's stack at the same place that it
	would be on an SGI platform.  This should NOT be necessary but
	who can know the state of programming today with the sorry
	state of Computer Science education!!

	Update 2001-04-03 from Dave Morano:

	Having the program stack at the same place as the SGI box may
	have an advantage in that an exact program execution trace from
	this simulator and the SGI box **should** match up!  This will
	only happen if I put the arguments and environment on the
	program stack the same was as the SGI box does.  I tried to
	follow the SGI in that regard when I first wrote this code last
	year but I doubt that I did it exactly as it does it.  If I
	really need it to be exact, assumming that I can find out what
	that is, I will have to modify this code to match the SGI box
	exactly.  We seem to match up with the SGI box so far.

*******************************************************************************/

#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<elf.h>
#include	<ctime>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<cstring>
#include	<usystem.h>
#include	<vecitem.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"
#include	"elfmips.h"		/* MIPS specific options */
#include	"lmapprog.h"		/* ourselves */


/* local defines */

#define	LMAPPROG_MAGIC		0x14253592
#define	LMAPPROG_ZEROFILE	"/dev/zero"
#define	LMAPPROG_BSSNAME	".bss"

#if	CF_TESTSTACK
#define	LMAPPROG_STACKSIZE	0x00040000	/* 256 kiBytes */
#else
#define	LMAPPROG_STACKSIZE	0x01000000	/* 16 MiBytes */
#endif

#define	LMAPPROG_STACKTOP	0x80000000	/* please do not play with */
#define	LMAPPROG_STACKSTRSIZE	0x0000d000
#define	LMAPPROG_DATASIZE	0x10000000	/* 256 MiBytes */


/* external subroutines */


/* external variables */


/* local structures */


/* forward references */

int		lmapprog_getphysical(LMAPPROG *,uint,caddr_t *) ;

static int	lmapprog_initsymtabs(LMAPPROG *,Elf32_Ehdr *,
			Elf32_Shdr *,uint) ;
static int	lmapprog_freesymtabs(LMAPPROG *) ;
static int	lmapprog_initsymbols(LMAPPROG *) ;
static int	lmapprog_freesymbols(LMAPPROG *) ;
static int	lmapprog_loadargs(LMAPPROG *,char **,char **) ;
static int	lmapprog_zerobss(LMAPPROG *,struct proginfo *,
			Elf32_Shdr *,int,uint) ;

static ulong	ceiling(ulong,uint) ;

static int	checkelf(struct proginfo *,Elf32_Ehdr *) ;
static int	findsection(Elf32_Shdr *,int,char *,char *,int) ;
static int	findhighsection(Elf32_Shdr *,int,char *,char *,int) ;


/* exported subroutines */


int lmapprog_init(mpp,pip,filename,argv,envv)
LMAPPROG	*mpp ;
struct proginfo	*pip ;
char		filename[] ;
char		*argv[], *envv[] ;
{
	Elf32_Ehdr	ehead ;
	Elf32_Phdr	*pheads = NULL ;

	LMAPPROG_SEGMENT	ps, *psp ;

	offset_t	uoff ;

	ulong	pagesize ;
	ulong	vaddr, vextent ;
	ulong	vpbase, vmbase ;
	ulong	totalmem ;

	uint	size ;
	uint	mapalign, pagealign ;
	uint	progsize = 0 ;
	uint	vpsize, vmsize ;
	uint	vlen ;

	int	rs ;
	int	i ;
	int	len ;
	int	prot, flags ;
	int	si_bss ;

	char	*shstrings = NULL ; 	/* section header name strings */


	if (mpp == NULL)
	    return SR_FAULT ;

	memset(mpp,0,sizeof(LMAPPROG)) ;

	mpp->magic = 0 ;
	mpp->pip = pip ;
	mpp->f.bss = FALSE ;
	mpp->f.symtab = FALSE ;
	mpp->f.symbols = FALSE ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: progfile=%s\n",filename) ;
#endif

/* try and open the ELF file */

	mpp->ofd = -1 ;
	mpp->zfd = -1 ;
	rs = u_open(filename,O_RDONLY,0666) ;
	mpp->ofd = rs ;
	if (rs < 0)
	    goto bad0 ;

	mpp->lastaccess = time(NULL) ;

/* the file MUST be seekable! */

	rs = u_seek(mpp->ofd,0L,SEEK_CUR) ;
	if (rs < 0)
	    goto bad1 ;

/* read the ELF header */

	rs = u_read(mpp->ofd,&ehead,sizeof(Elf32_Ehdr)) ;
	if (rs < 0)
	    goto bad1 ;

/* check if it is an ELF file and if we can handle it! (MIPS, et cetera) */

	if ((rs = checkelf(pip,&ehead)) < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 2)
	        debugprintf("lmapprog_init: bad ELF file (%d)\n",rs) ;
#endif

	    bprintf(pip->efp,
	        "%s: bad or inappropriate program object file (%d)\n",
	        pip->progname,rs) ;

	    goto bad1 ;
	}

/* read some stuff that we want out of the ELF header */

	mpp->progentry = ehead.e_entry ;	/* program entry address */

#ifdef	COMMENT
	uc_sysconf(_SC_PAGESIZE,(long *) &pagesize) ;
#else
	pagesize = (ulong) getpagesize() ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_init: OS pagesize=%08lx\n",pagesize) ;
#endif

	mapalign = pagesize ;

/* find the BSS section so that we can set the "break" address! */

	mpp->f.bss = FALSE ;

#if	CF_SECTIONS

/* allocate space for the section header table */

	mpp->nsheads = ehead.e_shnum ;

	size = ehead.e_shentsize * ehead.e_shnum ;
	rs = uc_malloc(size,&mpp->sheads) ;
	if (rs < 0)
	    goto bad1 ;

	uoff = ehead.e_shoff ;
	u_seek(mpp->ofd,uoff,SEEK_SET) ;

	rs = u_read(mpp->ofd,mpp->sheads,size) ;
	len = rs ;
	if ((rs >= 0) && (len < size)) {
	    rs = SR_EOF ;
	    goto bad2 ;
	}

/* do we even have a section string table? */

	if (ehead.e_shstrndx == SHN_UNDEF)
	    goto bad2 ;

/* verify we have a section string table here! */

	i = ehead.e_shstrndx ;
	if (mpp->sheads[i].sh_type != SHT_STRTAB)
	    goto bad2 ;

	size = mpp->sheads[i].sh_size ;
	rs = uc_malloc(size,&shstrings) ;
	if (rs < 0)
	    goto bad2 ;

/* read in the section string table */

	uoff = mpp->sheads[i].sh_offset ;
	u_seek(mpp->ofd,uoff,SEEK_SET) ;

	rs = u_read(mpp->ofd,shstrings,size) ;
	len = rs ;
	if ((rs >= 0) && (len < size)) {
	    rs = SR_EOF ;
	    goto bad3 ;
	}

/* do we have a bad (not supported) dynamic section? */

	for (i = 0 ; i < ehead.e_shnum ; i += 1) {

	    if (mpp->sheads[i].sh_type == SHT_DYNAMIC) {

	        rs = SR_NOTSUP ;
	        goto bad3 ;
	    }

	} /* end for */

/* do we have a BSS section? (I hope so) */

	rs = findhighsection(mpp->sheads,ehead.e_shnum,shstrings,
	    NULL,SHT_NOBITS) ;

	si_bss = -1 ;
	if (rs >= 0) {

	    mpp->f.bss = TRUE ;
	    si_bss = i = rs ;
	    mpp->progbreak = mpp->sheads[i].sh_addr + mpp->sheads[i].sh_size ;
	    mpp->basebreak = mpp->progbreak ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 5)
	        debugprintf("lmapprog_init: progbreak=%08x\n",mpp->progbreak) ;
#endif

	} /* end if (had a BSS section) */

/* OK, now we want to map any symbol tables and their string tables! */

#if	CF_SYMTAB

	rs = lmapprog_initsymtabs(mpp,&ehead,mpp->sheads,mapalign) ;

	if (rs < 0)
	    goto bad3 ;

#endif /* F_SYMTAB */

#endif /* F_SECTIONS */


/* store the filename now that we are pretty sure we are OK */

	size = strlen(filename) + 1 ;
	rs = uc_malloc(size,&mpp->filename) ;

	if (rs < 0)
	    goto bad4 ;

	strcpy(mpp->filename,filename) ;

/* process the program segments */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_init: processing program segments\n") ;
#endif

	if (ehead.e_phentsize < sizeof(Elf32_Phdr)) {

	    rs = SR_BADFMT ;
	    goto bad5 ;
	}

/* allocate space for the program header table */

	size = ehead.e_phentsize * ehead.e_phnum ;
	rs = uc_malloc(size,&pheads) ;
	if (rs < 0)
	    goto bad5 ;

	uoff = ehead.e_phoff ;
	u_seek(mpp->ofd,uoff,SEEK_SET) ;

	rs = u_read(mpp->ofd,pheads,size) ;
	len = rs ;
	if ((rs >= 0) && (len < size)) {
	    rs = SR_EOF ;
	    goto bad6 ;
	}


/* open up a special "file" for demand-zero pages */

	if ((rs = u_open(LMAPPROG_ZEROFILE,O_RDWR,0666)) < 0)
	    goto bad6 ;

	mpp->zfd = rs ;


/* initialize the container object to hold our program map segments */

	rs = vecitem_start(&mpp->segments,10,VECITEM_PNOHOLES) ;

	if (rs < 0)
	    goto bad7 ;


/* map the program pieces from the ELF file */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_init: about to map pieces\n") ;
#endif

/* find the maximum page size (Solaris only supports one size at a time) */

	mapalign = pagesize ;
	pagealign = 0x01000000 ;	/* start out bigger than expected */
	for (i = 0 ; i < ehead.e_phnum ; i += 1) {

	    if (pheads[i].p_type != PT_LOAD)
	        continue ;

	    if (pheads[i].p_memsz == 0)
	        continue ;

	    if (pheads[i].p_align > mapalign)
	        mapalign = pheads[i].p_align ;

	    if (pheads[i].p_align < pagealign)
	        pagealign = pheads[i].p_align ;

	    progsize += vpsize ;

	} /* end for */

	mpp->mapalign = mapalign ;	/* store the mapping alignment */
	mpp->pagealign = pagealign ;	/* store the v-page alignment */

/* loop through them looking for the LOADable ones */

	for (i = 0 ; i < ehead.e_phnum ; i += 1) {

	    offset_t	fmbase ;

	    ulong	fextent ;

	    uint	gap ;
	    uint	fmsize ;


	    if (pheads[i].p_type != PT_LOAD)
	        continue ;

	    if (pheads[i].p_memsz == 0)
	        continue ;

/* do it! */

	    ps.f.backwards = FALSE ;
	    ps.pagealign = pheads[i].p_align ;
	    ps.mapalign = mapalign ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 5)
	        debugprintf(
	            "lmapprog_init: program_section=%d target_align=%08x\n",
	            i,ps.pagealign) ;
#endif

	    ps.prot = PROT_NONE ;
	    flags = MAP_SHARED ;
	    if (pheads[i].p_flags & PF_X)
	        ps.prot |= PROT_EXEC ;

	    if (pheads[i].p_flags & PF_R)
	        ps.prot |= PROT_READ ;

	    if (pheads[i].p_flags & PF_W) {

	        ps.prot |= PROT_WRITE ;
	        flags = MAP_PRIVATE ;

	    }

/* create the exact (real) protection for the mapping */

	    prot = ps.prot ;
	    if (ps.prot & PROT_EXEC) {

	        prot |= PROT_READ ;
	        prot &= (~ PROT_EXEC) ;

	    }

/* calculate the file mapping size */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 5) {
	        debugprintf("lmapprog_init: p_vaddr=%08lx p_memsz=%08lx\n",
	            pheads[i].p_vaddr,pheads[i].p_memsz) ;
	        debugprintf("lmapprog_init: p_offset=%08lx p_filesz=%08lx\n",
	            pheads[i].p_offset,pheads[i].p_filesz) ;
	    }
#endif

	    fmbase = pheads[i].p_offset & (~ (mapalign - 1)) ;
	    fextent = pheads[i].p_offset + pheads[i].p_filesz ;
	    fmsize = ceiling(fextent,mapalign) - fmbase ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 5)
	        debugprintf(
	            "lmapprog_init: fmbase=%08lx fextent=%08lx fmsize=%08lx\n",
	            fmbase,fextent,fmsize) ;
#endif

/* calculate the memory mapping size */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_init: p_vaddr=%08lx p_memsz=%08lx\n",
	            pheads[i].p_vaddr,pheads[i].p_memsz) ;
#endif

/* virtual memory mapped to the file */

	    vaddr = pheads[i].p_vaddr ;
	    vlen = pheads[i].p_filesz ;
	    vextent = vaddr + vlen ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4) {
	        debugprintf("lmapprog_init: vaddr=%08lx vlen=%08x ve=%08lx\n",
	            vaddr,vlen,vextent) ;
	    }
#endif

/* virtual pages */

	    vpbase = vaddr & (~ (pagealign - 1)) ;
	    vpsize = ceiling(vextent,pagealign) - vpbase ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4) {
	        debugprintf(
	            "lmapprog_init: vpbase=%08x vpsize=%08x b+s=%08lx\n",
	            vpbase,vpsize,(vpbase+vpsize)) ;
	    }
#endif

/* virtual mappings */

	    vmbase = vaddr & (~ (mapalign - 1)) ;
	    vmsize = ceiling(vextent,mapalign) - vmbase ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4) {
	        debugprintf("lmapprog_init: vmbase=%08lx vmsize=%08x b+s=%08lx\n",
	            vmbase,vmsize,(vmbase+vmsize)) ;
	    }
#endif

/* gap? */

	    gap = vaddr - vmbase ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_init: gap=%08lx \n",gap) ;
#endif

/* map the file portion if any */

	    if (fmsize > 0) {

	        ps.vaddr = vaddr ;
	        ps.vlen = vlen ;
	        ps.offset = pheads[i].p_offset ;
	        ps.filesize = pheads[i].p_filesz ;
		ps.memsz = MIN(pheads[i].p_filesz,pheads[i].p_memsz) ;
	        ps.vmbase = vmbase ;
	        ps.vmsize = vmsize ;
	        ps.vpbase = vpbase ;
	        ps.vpsize = vpsize ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4) {
	            debugprintf("lmapprog_init: prot= %s %s %s\n",
	                ((ps.prot & PROT_READ) == PROT_READ) ? "R" : "",
	                ((ps.prot & PROT_WRITE) == PROT_WRITE) ? "W" : "",
	                ((ps.prot & PROT_EXEC) == PROT_EXEC) ? "X" : "") ;
	            debugprintf("lmapprog_init: flags= %s %s\n",
	                ((flags & MAP_SHARED) == MAP_SHARED) ? "S" : "",
	                ((flags & MAP_PRIVATE) == MAP_PRIVATE) ? "P" : "") ;
	        }
#endif /* CF_DEBUG */

	        rs = u_mapfile(NULL,(size_t) fmsize,prot,flags,
	            mpp->ofd,fmbase,&ps.pa) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: u_mapfile() rs=%d pa=%08lx\n",
	                rs,ps.pa) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	        if ((pip->debuglevel >= 2) && (rs < 0))
	            debugprintf("lmapprog_init: u_mapfile() rs=%d\n",
	                rs) ;
#endif

	        if (rs < 0)
	            goto bad8 ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: vecitem_add()\n") ;
#endif

	        rs = vecitem_add(&mpp->segments,&ps,sizeof(LMAPPROG_SEGMENT)) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: vecitem_add() rs=%d\n",rs) ;
#endif

	        if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel >= 2)
	                debugprintf("lmapprog_init: vecitem_add() rs=%d\n",rs) ;
#endif

	            u_munmap(ps.pa,ps.vmsize) ;

	            goto bad8 ;
	        }

/* do we have to zero-out some of the last page of this program segment? */

/* is there a writable segment with more virtual memory than that mapped? */

	        if ((pheads[i].p_memsz > pheads[i].p_filesz) &&
	            ((prot & PROT_WRITE) == PROT_WRITE)) {

	            int		mlen ;

	            char	*mextent ;


#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel >= 4) {
	                debugprintf("lmapprog_init: partial page zero write\n") ;
	                debugprintf("lmapprog_init: p_filesz=%08lx p_memsz=%08lx\n",
	                    pheads[i].p_filesz,pheads[i].p_memsz) ;
	                debugprintf("lmapprog_init: fmbase=%08lx fextent=%08lx\n",
	                    fmbase,fextent) ;
	                debugprintf("lmapprog_init: fmsize=%08lx \n",
	                    fmsize) ;
	            }
#endif

	            mextent = ((char *) ps.pa) + (fextent - fmbase) ;
	            mlen = fmsize - (fextent - fmbase) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel >= 4) {
	                debugprintf("lmapprog_init: memset() "
				"pa=%08lx len=%08x\n",
	                    mextent,mlen) ;
	            }
#endif

	            if (mlen > 0)
	                memset(mextent,0,mlen) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	            if (pip->debuglevel >= 4)
	                debugprintf(
	                    "lmapprog_init: finished partial page zero\n") ;
#endif

	        } /* end if (writable mapped segments) */

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: finished a file map\n") ;
#endif

	    } /* end if (mapped some part of the ELF file) */

/* map any extra virtual address portion (these will be demand-zero pages) */

	    vextent = pheads[i].p_vaddr + pheads[i].p_memsz ;
	    if (vextent > (vpbase + vpsize)) {

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: mapping extra virtual parts\n") ;
#endif

/* calculate residue address and length */

	        vaddr = vpbase + vpsize ;
	        vlen = vextent - vaddr ;

/* calculate new virtual page variables */

	        vpbase = vaddr & (~ (pagealign - 1)) ;
	        vpsize = ceiling(vextent,pagealign) - vpbase ;

/* calculate new virtual mapping variables */

	        vmbase = vaddr & (~ (mapalign - 1)) ;
	        vmsize = ceiling(vextent,mapalign) - vmbase ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: E vmsize=%08x\n", vmsize) ;
#endif

/* vanilla */

	        flags = MAP_PRIVATE ;

	        ps.vaddr = vaddr ;
	        ps.vlen = vlen ;
	        ps.offset = 0 ;
	        ps.filesize = 0 ;
		ps.memsz = vlen ;
	        ps.vmbase = vmbase ;
	        ps.vmsize = vmsize ;
	        ps.vpbase = vpbase ;
	        ps.vpsize = vpsize ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4) {
	            debugprintf(
	                "lmapprog_init: vaddr=%08lx vlen=%08x ve=%08lx\n",
	                vaddr,vlen,(vaddr+vlen)) ;
	            debugprintf(
	                "lmapprog_init: vpbase=%08lx vpsize=%08x b+s=%08lx\n",
	                vpbase,vpsize,(vpbase+vpsize)) ;
	            debugprintf(
	                "lmapprog_init: vmbase=%08lx vmsize=%08x b+s=%08lx\n",
	                vmbase,vmsize,(vmbase+vmsize)) ;
	        }
#endif /* CF_DEBUG */

	        rs = u_mapfile(NULL,(size_t) vmsize,prot,flags,
	            mpp->zfd,0L,&ps.pa) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: u_mapfile() rs=%d pa=%08lx\n",
	                rs,ps.pa) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	        if ((pip->debuglevel >= 2) && (rs < 0))
	            debugprintf("lmapprog_init: u_mapfile() rs=%d\n", rs) ;
#endif

	        if (rs < 0)
	            goto bad8 ;

	        rs = vecitem_add(&mpp->segments,&ps,sizeof(LMAPPROG_SEGMENT)) ;
	        if (rs < 0) {

	            u_munmap(ps.pa,ps.vmsize) ;

	            goto bad8 ;
	        }

	    } /* end if (extra mapping is needed) */

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_init: bottom mapping loop\n") ;
#endif

	} /* end for (looping through link-time program segments) */


/* map a stack segment */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: mapping stack \n") ;
#endif

	vextent = LMAPPROG_STACKTOP ;
	vaddr = vextent - ceiling(LMAPPROG_STACKSIZE,mapalign) ;
	vlen = vextent - vaddr ;

	vpbase = vaddr & (~ (pagealign - 1)) ;
	vpsize = ceiling(vextent,pagealign) - vpbase ;

	vmbase = vaddr & (~ (mapalign - 1)) ;
	vmsize = ceiling(vextent,mapalign) - vmbase ;

	mpp->progstack = vextent ;
	mpp->basestack = mpp->progstack ;

	flags = MAP_PRIVATE ;

	ps.f.backwards = TRUE ;
	ps.pagealign = pagealign ;
	ps.mapalign = mapalign ;
	ps.prot = PROT_READ | PROT_WRITE ;

	ps.vaddr = vaddr ;
	ps.vlen = vlen ;
	ps.offset = 0 ;
	ps.filesize = 0 ;
	ps.memsz = vlen ;
	ps.vmbase = vmbase ;
	ps.vmsize = vmsize ;
	ps.vpbase = vpbase ;
	ps.vpsize = vpsize ;

	progsize += vpsize ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {
	    debugprintf("lmapprog_init: STACK vaddr=%08lx vlen=%08x ve=%08lx\n",
	        vaddr,vlen,(vaddr+vlen)) ;
	    debugprintf(
	        "lmapprog_init: STACK vpbase=%08lx vpsize=%08x b+s=%08lx\n",
	        vpbase,vpsize,(vpbase+vpsize)) ;
	    debugprintf(
	        "lmapprog_init: STACK vmbase=%08lx vmsize=%08x b+s=%08lx\n",
	        vmbase,vmsize,(vmbase+vmsize)) ;
	}
#endif

	rs = u_mapfile(NULL,(size_t) vmsize,ps.prot,flags,
	    mpp->zfd,0L,&ps.pa) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: mapfile rs=%d pa=%08lx\n",
	        rs,ps.pa) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if ((pip->debuglevel >= 2) && (rs < 0))
	    debugprintf("lmapprog_init: u_mapfile() rs=%d\n", rs) ;
#endif

	if (rs < 0)
	    goto bad8 ;

	rs = vecitem_add(&mpp->segments,&ps,sizeof(LMAPPROG_SEGMENT)) ;

	if (rs < 0) {

	    u_munmap(ps.pa,ps.vmsize) ;

	    goto bad8 ;
	}


/* map a starting data segment (maybe big enough for most purposes) */

	if (mpp->f.bss) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_init: mapping data\n") ;
#endif

/* start with program "break" value */

	    vaddr = ceiling(mpp->progbreak,pagealign) ;

	    vextent = mpp->progbreak + LMAPPROG_DATASIZE ;
	    vlen = vextent - vaddr ;

/* page view */

	    vpbase = vaddr & (~ (pagealign - 1)) ;
	    vpsize = ceiling(vextent,pagealign) - vpbase ;

/* mapping view */

	    vmbase = vaddr & (~ (mapalign - 1)) ;
	    vmsize = ceiling(vextent,mapalign) - vmbase ;

	    flags = MAP_PRIVATE ;

	    ps.f.backwards = FALSE ;
	    ps.pagealign = pagealign ;
	    ps.mapalign = mapalign ;
	    ps.prot = PROT_READ | PROT_WRITE ;

	    ps.vaddr = vaddr ;
	    ps.vlen = vlen ;
	    ps.offset = 0 ;
	    ps.filesize = 0 ;
	    ps.memsz = vlen ;
	    ps.vmbase = vmbase ;
	    ps.vmsize = vmsize ;
	    ps.vpbase = vpbase ;
	    ps.vpsize = vpsize ;

	    progsize += vpsize ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4) {
	        debugprintf("lmapprog_init: DATA vaddr=%08lx vlen=%08x ve=%08lx\n",
	            vaddr,vlen,(vaddr+vlen)) ;
	        debugprintf(
	            "lmapprog_init: DATA vpbase=%08lx vpsize=%08x b+s=%08lx\n",
	            vpbase,vpsize,(vpbase+vpsize)) ;
	        debugprintf(
	            "lmapprog_init: DATA vmbase=%08lx vmsize=%08x b+s=%08lx\n",
	            vmbase,vmsize,(vmbase+vmsize)) ;
	    }
#endif

	    rs = u_mapfile(NULL,(size_t) vmsize,ps.prot,flags,
	        mpp->zfd,0L,&ps.pa) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_init: u_mapfile() rs=%d pa=%08lx\n",
	            rs,ps.pa) ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	    if ((pip->debuglevel >= 2) && (rs < 0))
	        debugprintf("lmapprog_init: u_mapfile() rs=%d \n", rs) ;
#endif

	    if (rs < 0)
	        goto bad8 ;

	    rs = vecitem_add(&mpp->segments,&ps,sizeof(LMAPPROG_SEGMENT)) ;

	    if (rs < 0) {

	        u_munmap(ps.pa,ps.vmsize) ;

	        goto bad8 ;
	    }

	} /* end if (mapped DATA segment) */


/* initialize and populate the page table */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: segment review & PTE creation\n") ;
#endif

	size = 2 * (progsize + LMAPPROG_STACKSIZE + LMAPPROG_DATASIZE) / 
	    pagealign ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: hdb_start() size=%d\n",size) ;
#endif

	rs = hdb_start(&mpp->pagetable,size,1,NULL,NULL) ;
	if (rs < 0)
	    goto bad8 ;

/* loop, creating pages for the virtual mapped areas */

	for (i = 0 ; vecitem_get(&mpp->segments,i,&psp) >= 0 ; i += 1) {

	    LMAPPROG_PTE	pte, *ptep ;

	    HDB_DATUM	key, value ;

	    caddr_t	pa ;

	    ulong	vaddr ;
	    ulong	vbase ;

	    uint	vlen ;
	    uint	vsize ;

	    int		n, j ;


	    if (psp == NULL)
	        continue ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4) {

	        debugprintf("lmapprog_init: seg vaddr=%08lx vlen=%08x\n",
	            psp->vaddr,psp->vlen) ;

	        debugprintf("lmapprog_init: seg vpbase=%08lx vpsize=%08x\n",
	            psp->vpbase,psp->vpsize) ;

	        debugprintf("lmapprog_init: seg vmbase=%08lx vmsize=%08x\n",
	            psp->vmbase,psp->vmsize) ;

	        debugprintf("lmapprog_init: seg pframe=%08lx b+s=%08lx\n",
	            psp->pa,(psp->vmbase+psp->vmsize)) ;

	    }
#endif /* CF_DEBUG */

	    memset(&pte.f,0,sizeof(struct lmapprog_pteflags)) ;

	    if (psp->prot & PROT_READ)
	        pte.f.read = TRUE ;

	    if (psp->prot & PROT_WRITE)
	        pte.f.write = TRUE ;

	    if (psp->prot & PROT_EXEC)
	        pte.f.exec = TRUE ;

	    vaddr = psp->vaddr ;
	    vbase = psp->vpbase ;
	    vextent = psp->vaddr + psp->vlen ;

	    pa = (caddr_t) (((ulong) psp->pa) + (vbase - psp->vmbase)) ;

	    n = psp->vpsize / pagealign ;
	    for (j = 0 ; j < n  ; j += 1) {

	        pte.vbase = vbase ;
	        pte.vaddr = vaddr ;
	        pte.vlen = MIN(((vbase + pagealign) - vaddr),vextent - vaddr) ;
	        pte.pa = pa ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if (pip->debuglevel >= 4)
	            debugprintf("lmapprog_init: vbase=%08lx pframe=%08lx\n",
	                pte.vbase,pte.pa) ;
#endif

	        vaddr = vbase + pagealign ;
	        vbase += pagealign ;
	        pa = (caddr_t) (((ulong) pa) + pagealign) ;

/* store it away */

	        rs = uc_malloc(sizeof(LMAPPROG_PTE),&ptep) ;
	        if (rs < 0)
	            goto bad9 ;

	        memcpy(ptep,&pte,sizeof(LMAPPROG_PTE)) ;

	        value.buf = ptep ;
	        value.len = sizeof(LMAPPROG_PTE) ;

	        key.buf = &ptep->vbase ;
	        key.len = sizeof(ulong) ;

	        rs = hdb_store(&mpp->pagetable,key,value) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	        if ((pip->debuglevel >= 2) && (rs < 0))
	            debugprintf("lmapprog_init: hdb_store() rs=%d\n",rs) ;
#endif

	        if (rs < 0) {
	            uc_free((void *) value.buf) ;
	            goto bad9 ;
	        }

	    } /* end while (looping creating pages) */

	} /* end for (looping over mapped segments) */


/* we're almost done! , set this so we can use ourselves! */

	if (rs >= 0)
	    mpp->magic = LMAPPROG_MAGIC ;


/* very important : zero out the leading part of all NOBITS sections */

	rs = lmapprog_zerobss(mpp,pip,mpp->sheads,ehead.e_shnum,mapalign) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: zerobss() rs=%d\n",rs) ;
#endif


/* OK, now that we have a simulated stack, load the user's arguments on it */

#if	CF_STACKARGS

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: loading arguments\n") ;
#endif

	rs = lmapprog_loadargs(mpp,argv,envv) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: lmapprog_loadargs() rs=%d\n",rs) ;
#endif

#endif /* F_STACKARGS */



/* we're done, let's get out!! */
ret2:
	u_close(mpp->zfd) ;
	mpp->zfd = -1 ;			/* mark as closed */

/* free up the program headers */
ret1:
	uc_free(pheads) ;
	pheads = NULL ;

/* free up section strings */

	if (shstrings != NULL) {
	    uc_free(shstrings) ;
	    shstrings = NULL ;
	}

ret0:
	u_close(mpp->ofd) ;
	mpp->ofd = -1 ;			/* mark as closed */

/* let's index the symbols if we have any (by name) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: index symbols? rs=%d f_symtab=%d\n",
	        rs,mpp->f.symtab) ;
#endif

	if ((rs >= 0) && mpp->f.symtab) {
	    HDB_DATUM		key, value ;
	    LMAPPROG_SYMTAB	*stp ;
#ifdef	COMMENT
	    LMAPPROG_SYMBOL	*sep ;
#endif
	    Elf32_Sym	*eep ;
	    int		j ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_init: indexing symbols\n") ;
#endif

	    rs = hdb_start(&mpp->symbols,200,1,NULL,NULL) ;
	    if (rs < 0)
	        goto bad9 ;

	    for (i = 0 ; vecitem_get(&mpp->symtabs,i,&stp) >= 0 ; i += 1) {
	        if (stp == NULL) continue ;

	        for (j = 0 ; j < stp->nsyms ; j += 1) {

	            eep = stp->symtab + j ;
	            if (eep->st_name == 0) continue ;

#if	CF_DEBUG && 0
	            if (pip->debuglevel >= 4)
	                debugprintf("lmapprog_init: s> %s\n",
	                    (stp->strings + eep->st_name)) ;
#endif

	            key.buf = (stp->strings + eep->st_name) ;
	            key.len = -1 ;

#ifdef	COMMENT
	            rs = uc_malloc(sizeof(LMAPPROG_SYMBOL),&sep) ;

	            if (rs < 0)
	                break ;

	            sep->ep = eep ;
	            sep->name = (char *) (stp->strings + eep->st_name) ;

	            value.buf = sep ;
	            value.len = sizeof(LMAPPROG_SYMBOL) ;
#else
	            value.buf = eep ;
	            value.len = sizeof(Elf32_Sym) ;
#endif /* COMMENT */

	            rs = hdb_store(&mpp->symbols,key,value) ;

	            if (rs < 0) {
#ifdef	COMMENT
	                uc_free(sep) ;
#endif
	                break ;
	            }

	        } /* end for */

	        if (rs < 0) break ;
	    } /* end for (looping over symbol tables) */

	    if (rs < 0) {
	        lmapprog_freesymbols(mpp) ;
	        goto bad9 ;
	    }

	    mpp->f.symbols = TRUE ;

	} /* end if (indexing symbols by name) */

/* clean up some stuff before exiting */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: exiting rs=%d\n",rs) ;
#endif

	return rs ;

/* bad things come here */
bad9:
	{
	    HDB_CUR	cur ;
	    HDB_DATUM	key, value ;
	    LMAPPROG_PTE	*ptep ;

	    hdb_curbegin(&mpp->pagetable,&cur) ;

	    while (hdb_enum(&mpp->pagetable,&cur,&key,&value) >= 0) {

	        ptep = (LMAPPROG_PTE *) value.buf ;

	        uc_free(ptep) ;

	    } /* end while */

	    hdb_curend(&mpp->pagetable,&cur) ;

	} /* end block */

	hdb_finish(&mpp->pagetable) ;

bad8:
	for (i = 0 ; vecitem_get(&mpp->segments,i,&psp) >= 0 ; i += 1) {

	    if (psp == NULL) continue ;

	    if (psp->filesize > 0)
	        u_munmap(psp->pa,psp->vmsize) ;

	} /* end for */

	vecitem_finish(&mpp->segments) ;

bad7:
	u_close(mpp->zfd) ;

	mpp->zfd = -1 ;

bad6:
	if (pheads != NULL) {
	    uc_free(pheads) ;
	    pheads = NULL ;
	}

bad5:
	if (mpp->filename != NULL) {
	    uc_free(mpp->filename) ;
	    mpp->filename = NULL ;
	}

bad4:
	if (mpp->f.symtab) {
	    LMAPPROG_SYMTAB	*stp ;

	    for (i = 0 ; vecitem_get(&mpp->symtabs,i,&stp) >= 0 ; i += 1) {
	        if (stp == NULL) continue ;

	        if (stp->pa_symtab != NULL) {
	            u_munmap(stp->pa_symtab,stp->maplen_symtab) ;
		}

	        if (stp->pa_strings != NULL) {
	            u_munmap(stp->pa_strings,stp->maplen_strings) ;
		}

	    } /* end for */

	    vecitem_finish(&mpp->symtabs) ;
	    mpp->f.symtab = FALSE ;

	} /* end if */

bad3:
	if (shstrings != NULL) {
	    uc_free(shstrings) ;
	}

bad2:
	if (mpp->sheads != NULL) {
	    uc_free(mpp->sheads) ;
	}

bad1:
	u_close(mpp->ofd) ;

bad0:

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_init: exiting bad rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (lmapprog_init) */


/* free up this whole program mapping! */
int lmapprog_free(mpp)
LMAPPROG	*mpp ;
{
	struct proginfo		*pip ;
	LMAPPROG_SEGMENT	*psp ;
	int		i ;

	if (mpp == NULL) return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;
#endif

	if (mpp->magic != LMAPPROG_MAGIC) return SR_NOTOPEN ;

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_free: entered\n") ;
#endif

/* free up the indexed symbols (if any) */

	if (mpp->f.symbols)
	    lmapprog_freesymbols(mpp) ;

/* free up the page table */

	{
	    HDB_CUR		cur ;
	    HDB_DATUM		key, value ;
	    LMAPPROG_PTE	*ptep ;

	    hdb_curbegin(&mpp->pagetable,&cur) ;

	    while (hdb_enum(&mpp->pagetable,&cur,&key,&value) >= 0) {

	        ptep = (LMAPPROG_PTE *) value.buf ;

	        uc_free(ptep) ;

	    } /* end while */

	    hdb_curend(&mpp->pagetable,&cur) ;

	} /* end block */

	hdb_finish(&mpp->pagetable) ;

/* unmap everything */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_free: freeing segments\n") ;
#endif

	for (i = 0 ; vecitem_get(&mpp->segments,i,&psp) >= 0 ; i += 1) {
	    if (psp == NULL) continue ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 5)
	        debugprintf("lmapprog_free: pa=%08x len=%08x\n",
	            psp->pa,psp->vmsize) ;
#endif

	    if (psp->filesize > 0) {
	        u_munmap(psp->pa,psp->vmsize) ;
	    }

	} /* end for */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_free: segment list\n") ;
#endif

	vecitem_finish(&mpp->segments) ;

/* free up any symbol tables (and their string tables) */

	if (mpp->f.symtab) {
	    LMAPPROG_SYMTAB	*stp ;

	    for (i = 0 ; vecitem_get(&mpp->symtabs,i,&stp) >= 0 ; i += 1) {
	        if (stp == NULL) continue ;

	        if (stp->pa_symtab != NULL) {
	            u_munmap(stp->pa_symtab,stp->maplen_symtab) ;
		}

	        if (stp->pa_strings != NULL) {
	            u_munmap(stp->pa_strings,stp->maplen_strings) ;
		}

	    } /* end for */

	    vecitem_finish(&mpp->symtabs) ;
	    mpp->f.symtab = FALSE ;

	} /* end if (freeing up the symbol tables) */

/* close any open files */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_free: closing files\n") ;
#endif

	if (mpp->ofd >= 0) {
	    u_close(mpp->ofd) ;
	    mpp->ofd = -1 ;
	}

	if (mpp->zfd >= 0) {
	    u_close(mpp->zfd) ;
	    mpp->zfd = -1 ;
	}

/* free up the data structures that were allocated */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_free: freeing other memory\n") ;
#endif

	if (mpp->filename != NULL) {
	    uc_free(mpp->filename) ;
	   mpp->filename = NULL ;
	}

/* free up sections headers */

	if (mpp->sheads != NULL) {
	    uc_free(mpp->sheads) ;
	    mpp->sheads = NULL ;
	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_free: clearing magic\n") ;
#endif

	mpp->magic = 0 ;
	return SR_OK ;
}
/* end subroutine (lmapprog_free) */


/* return the program entry point address */
int lmapprog_getentry(mpp,ap)
LMAPPROG	*mpp ;
uint		*ap ;
{
	struct proginfo		*pip ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_getentry: entered\n") ;
#endif

	*ap = mpp->progentry ;

	return SR_OK ;
}
/* end subroutine (lmapprog_getentry) */


/* return the initial program stack pointer */
int lmapprog_getstack(mpp,ap)
LMAPPROG	*mpp ;
uint		*ap ;
{
	struct proginfo		*pip ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_getstack: entered\n") ;
#endif

	*ap = mpp->progstack ;

	return SR_OK ;
}
/* end subroutine (lmapprog_getstack) */


/* return the program break address */
int lmapprog_getbreak(mpp,ap)
LMAPPROG	*mpp ;
uint		*ap ;
{
	struct proginfo		*pip ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_getbreak: entered\n") ;
#endif

	*ap = mpp->progbreak ;

	return SR_OK ;
}
/* end subroutine (lmapprog_getbreak) */


/* get the target program page size */
int lmapprog_getpagesize(mpp,ap)
LMAPPROG	*mpp ;
uint		*ap ;
{
	struct proginfo		*pip ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_getpagesize: entered\n") ;
#endif

	*ap = mpp->pagealign ;

	return SR_OK ;
}
/* end subroutine (lmapprog_getpagesize) */


/* get the program maximums */
int lmapprog_getmax(mpp,mp)
LMAPPROG		*mpp ;
LMAPPROG_MAXPROG	*mp ;
{
	struct proginfo		*pip ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_getmax: entered\n") ;
#endif

	if (mp == NULL)
	    return SR_FAULT ;

	memset(mp,0,sizeof(LMAPPROG_MAXPROG)) ;

	mp->data = LMAPPROG_DATASIZE ;
	mp->stack = LMAPPROG_STACKSIZE ;

	return SR_OK ;
}
/* end subroutine (lmapprog_getmax) */


/* set the program break address */
int lmapprog_setbreak(mpp,newval)
LMAPPROG	*mpp ;
uint		newval ;
{
	struct proginfo		*pip ;

	ulong	breakextent ;
	ulong	breakmax ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_setbreak: entered, newval=%08x\n",newval) ;
#endif


	breakextent = mpp->basebreak + LMAPPROG_DATASIZE ;
	breakmax = ceiling(breakextent,mpp->mapalign) ;

	if (newval > breakmax)
	    return SR_NOMEM ;

	mpp->progbreak = newval ;
	return SR_OK ;
}
/* end subroutine (lmapprog_setbreak) */


/* get the physical address associated with a virtual one */
int lmapprog_getphysical(mpp,vaddr,pap)
LMAPPROG	*mpp ;
uint		vaddr ;
caddr_t		*pap ;
{
	struct proginfo		*pip ;

	LMAPPROG_PTE		*ptep ;

	HDB_DATUM		key, value ;

	ulong	vbase ;

	uint	pa ;

	int	rs ;
	int	i ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_getphysical: vaddr=%08x\n",vaddr) ;
#endif

#ifdef	COMMENT
	if (vaddr & 3)
	    return SR_INVALID ;
#endif

/* find this virtual address in the mapping */

	vbase = vaddr & (~ (mpp->pagealign - 1)) ;
	key.buf = &vbase ;
	key.len = sizeof(ulong) ;

	rs = hdb_fetch(&mpp->pagetable,key,NULL,&value) ;

	if (rs < 0)
	    return SR_FAULT ;

	ptep = (LMAPPROG_PTE *) value.buf ;
	*pap = (caddr_t) (((uint) ptep->pa) + (vaddr - ptep->vbase)) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_getphysical: pa=%08lx\n",*pap) ;
#endif

	return SR_OK ;
}
/* end subroutine (lmapprog_getphysical) */


/* read an INSTRUCTION integer (32 bits) from the program memory */
int lmapprog_readinstr(mpp,vaddr,vp)
LMAPPROG	*mpp ;
uint		vaddr ;
uint		*vp ;
{
	struct proginfo		*pip ;

	HDB_DATUM		key, value ;

	LMAPPROG_PTE		*ptep ;

	ulong	vbase ;

	uint	*pa ;

	int	rs ;
	int	i ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readinstr: vaddr=%08x\n",vaddr) ;
#endif

	if (vaddr & 3)
	    return SR_INVALID ;

/* find this virtual address in the mapping */

	vbase = vaddr & (~ (mpp->pagealign - 1)) ;
	key.buf = &vbase ;
	key.len = sizeof(ulong) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readinstr: hdb_fetch() vbase=%08lx\n",vbase) ;
#endif

	rs = hdb_fetch(&mpp->pagetable,key,NULL,&value) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readinstr: hdb_fetch() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return SR_FAULT ;

	ptep = (LMAPPROG_PTE *) value.buf ;

	if (! ptep->f.exec)
	    return SR_ACCESS ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5) {
	    debugprintf("lmapprog_readinstr: pte vaddr=%08lx\n",ptep->vaddr) ;
	    debugprintf("lmapprog_readinstr: pframe=%08lx\n",ptep->pa) ;
	}
#endif

	pa = (uint *) (((uint) ptep->pa) + (vaddr - ptep->vbase)) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readinstr: pframe=%08lx pa=%08lx\n",
	        ptep->pa,pa) ;
#endif

	*vp = *pa ;
	return SR_OK ;
}
/* end subroutine (lmapprog_readinstr) */


/* read an integer (32 bits) from the program memory */
int lmapprog_readint(mpp,vaddr,vp)
LMAPPROG	*mpp ;
uint		vaddr ;
uint		*vp ;
{
	struct proginfo		*pip ;

	HDB_DATUM		key, value ;

	LMAPPROG_PTE		*ptep ;

	ulong	vbase ;

	uint	*pa ;

	int	rs ;
	int	i ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = mpp->pip ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readint: entered vaddr=%08x\n",vaddr) ;
#endif

	if (vaddr & 3)
	    return SR_INVALID ;

#if	CF_SAFE
	if (vp == NULL)
	    return SR_FAULT ;
#endif /* CF_SAFE */

/* find this virtual address in the segments */

	vbase = vaddr & (~ (mpp->pagealign - 1)) ;
	key.buf = &vbase ;
	key.len = sizeof(ulong) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readint: hdb_fetch() vbase=%08lx\n",vbase) ;
#endif

	rs = hdb_fetch(&mpp->pagetable,key,NULL,&value) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readint: hdb_fetch() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return SR_FAULT ;

	ptep = (LMAPPROG_PTE *) value.buf ;

#if	CF_CHECKACCESS
	if ((! ptep->f.read) && (! ptep->f.exec))
	    return SR_ACCESS ;
#endif

#if	CF_DEBUG
	if (pip->debuglevel >= 5) {
	    debugprintf("lmapprog_readint: pte vaddr=%08lx\n",ptep->vaddr) ;
	    debugprintf("lmapprog_readint: pframe=%08lx\n",ptep->pa) ;
	}
#endif

	pa = (uint *) (((uint) ptep->pa) + (vaddr - ptep->vbase)) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_readint: pframe=%08lx pa=%08lx\n",
	        ptep->pa,pa) ;
#endif

	*vp = *pa ;
	return SR_OK ;
}
/* end subroutine (lmapprog_readint) */


/* write an integer (32 bits) to the simulated program memory */
int lmapprog_writeint(mpp,vaddr,data,dp)
LMAPPROG	*mpp ;
uint		vaddr ;
uint		data ;
uint		dp ;
{
	struct proginfo		*pip ;

	HDB_DATUM		key, value ;

	LMAPPROG_PTE		*ptep ;

	ulong	vbase ;

	uint	*pa ;

	int	rs ;
	int	i ;

	uchar	*pac ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_DEBUG 
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_writeint: vaddr=%08x\n",vaddr) ;
#endif

	if (! dp)
	    return SR_OK ;

	if (vaddr & 3)
	    return SR_INVALID ;

/* find this virtual address in the mapping */

	vbase = vaddr & (~ (mpp->pagealign - 1)) ;
	key.buf = &vbase ;
	key.len = sizeof(ulong) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_writeint: hdb_fetch() vbase=%08lx\n",vbase) ;
#endif

	rs = hdb_fetch(&mpp->pagetable,key,NULL,&value) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_writeint: hdb_fetch() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return SR_FAULT ;

	ptep = (LMAPPROG_PTE *) value.buf ;

	if (! ptep->f.write)
	    return SR_ACCESS ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5) {
	    debugprintf("lmapprog_writeint: pte vaddr=%08lx\n",ptep->vaddr) ;
	    debugprintf("lmapprog_writeint: pframe=%08lx\n",ptep->pa) ;
	}
#endif

	pa = (uint *) (((uint) ptep->pa) + (vaddr - ptep->vbase)) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_writeint: pframe=%08lx pa=%08lx\n",
	        ptep->pa,pa) ;
#endif

/* we assume MIPS byte order totally */

	dp &= 0x0f ;
	if (dp == 0x0f) {

	    *pa = data ;

	} else {

	    int		shift = 24 ;


	    pac = (uchar *) pa ;
	    while (dp) {

	        if (dp & 1)
	            *pac = (uchar) (data >> shift) ;

	        dp = dp >> 1 ;
	        pac += 1 ;
	        shift -= 8 ;

	    } /* end while */

	} /* end if */

	return SR_OK ;
}
/* end subroutine (lmapprog_writeint) */


/* test accessibility */
int lmapprog_memaccess(mpp,vaddr,mode)
LMAPPROG	*mpp ;
uint		vaddr ;
uint		mode ;
{
	struct proginfo		*pip ;

	HDB_DATUM		key, value ;

	LMAPPROG_PTE		*ptep ;

	ulong	vbase ;

	int	rs ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_memaccess: vaddr=%08x\n",vaddr) ;
#endif

	if (vaddr & 3)
	    return SR_INVALID ;

/* find this virtual address in the mapping */

	vbase = vaddr & (~ (mpp->pagealign - 1)) ;
	key.buf = &vbase ;
	key.len = sizeof(ulong) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_memaccess: hdb_fetch() vbase=%08lx\n",vbase) ;
#endif

	rs = hdb_fetch(&mpp->pagetable,key,NULL,&value) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_memaccess: hdb_fetch() rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return SR_FAULT ;

	if (mode == 0)
	    return SR_OK ;

	ptep = (LMAPPROG_PTE *) value.buf ;

	if (((mode & R_OK) == R_OK) && (! ptep->f.read))
	    return SR_ACCESS ;

	if (((mode & W_OK) == W_OK) && (! ptep->f.write))
	    return SR_ACCESS ;

	if (((mode & X_OK) == X_OK) && (! ptep->f.exec))
	    return SR_ACCESS ;

	return SR_OK ;
}
/* end subroutine (lmapprog_memaccess) */


/* get a symbol table entry */

/***

	Note that this subroutine is not very useful when there are
	more than one symbol by the same name!  This subroutine will
	only return the first of what could be many possible symbols.

***/

int lmapprog_getsym(mpp,name,sepp)
LMAPPROG	*mpp ;
char		name[] ;
Elf32_Sym	**sepp ;
{
	struct proginfo	*pip ;

	LMAPPROG_SYMTAB	*stp ;

	HDB_DATUM	key, value ;

	Elf32_Sym	*sep ;

	int	rs ;
	int	i ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	if (sepp != NULL)
	    *sepp = NULL ;

	if (name == NULL)
	    return SR_FAULT ;

	if (! mpp->f.symbols)
	    return SR_NOTFOUND ;

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_getsym: symbol=%s\n",name) ;
#endif

	if (name[0] == '\0')
	    return SR_INVALID ;

	key.buf = name ;
	key.len = -1 ;

	rs = hdb_fetch(&mpp->symbols,key,NULL,&value) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_getsym: rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return SR_NOTFOUND ;

	sep = (Elf32_Sym *) value.buf ;
	if (sepp != NULL)
	    *sepp = sep ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5) {
	    debugprintf("lmapprog_getsym: value=%08x\n",
	        (uint) sep->st_value) ;
	}
#endif

	return SR_OK ;
}
/* end subroutine (lmapprog_getsym) */


/* initialize a cursor for fetching symbols by name */
int lmapprog_sncurbegin(mpp,cp)
LMAPPROG		*mpp ;
LMAPPROG_SNCURSOR	*cp ;
{
	struct proginfo	*pip ;

	int	rs ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0)) {

	    debugprintf("lmapprog_sncurbegin: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (! mpp->f.symbols)
	    return SR_NOTAVAIL ;

	if (cp == NULL)
	    return SR_FAULT ;

	pip = mpp->pip ;

	rs = hdb_curbegin(&mpp->symbols,cp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5) {
	    if (rs < 0)
	        debugprintf("lmapprog_sncurbegin: hdb_curbegin() rs=%d\n",
	            rs) ;
	}
#endif /* CF_SAFE */

	return rs ;
}
/* end subroutine (lmapprog_sncurbegin) */


int lmapprog_sncurend(mpp,cp)
LMAPPROG		*mpp ;
LMAPPROG_SNCURSOR	*cp ;
{
	int	rs ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0)) {

	    debugprintf("lmapprog_sncurend: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (cp == NULL)
	    return SR_FAULT ;

	rs = hdb_curend(&mpp->symbols,cp) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5) {
	    if (rs < 0)
	        debugprintf("lmapprog_sncurbegin: hdb_curend() rs=%d\n",
	            rs) ;
	}
#endif /* F_SAFE */

	return rs ;
}
/* end subroutine (lmapprog_sncurend) */


/* fetch a symbol table entry */
int lmapprog_fetchsym(mpp,name,cp,sepp)
LMAPPROG		*mpp ;
char			name[] ;
LMAPPROG_SNCURSOR	*cp ;
Elf32_Sym		**sepp ;
{
	struct proginfo		*pip ;

	LMAPPROG_SYMTAB		*stp ;

	HDB_DATUM		key, value ;

	Elf32_Sym		*sep ;

	int	rs ;
	int	i ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	if (sepp != NULL)
	    *sepp = NULL ;

	if (name == NULL)
	    return SR_FAULT ;

	if (! mpp->f.symbols)
	    return SR_NOTAVAIL ;

	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_fetchsym: symbol=%s\n",name) ;
#endif

	if (name[0] == '\0')
	    return SR_INVALID ;

	key.buf = name ;
	key.len = -1 ;

	rs = hdb_fetch(&mpp->symbols,key,cp,&value) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_fetchsym: rs=%d\n",rs) ;
#endif

	if (rs < 0)
	    return SR_NOTFOUND ;

	sep = (Elf32_Sym *) value.buf ;
	if (sepp != NULL)
	    *sepp = sep ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_fetchsym: value=%08x\n",
	        (uint) sep->st_value) ;
#endif

	return SR_OK ;
}
/* end subroutine (lmapprog_fetchsym) */


/* enumerate the symbols */
int lmapprog_enumsym(mpp,cp,namepp,sepp)
LMAPPROG		*mpp ;
LMAPPROG_SNCURSOR	*cp ;
char			**namepp ;
Elf32_Sym		**sepp ;
{
	struct proginfo		*pip ;

	LMAPPROG_SYMTAB		*stp ;

	HDB_DATUM		key, value ;

	Elf32_Sym		*sep ;

	int	rs ;
	int	i ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	if (sepp != NULL)
	    *sepp = NULL ;

	if (! mpp->f.symbols)
	    return SR_NOTAVAIL ;

	pip = mpp->pip ;

	rs = hdb_enum(&mpp->symbols,cp,&key,&value) ;
	if (rs < 0) {
	    rs = SR_NOTFOUND ;
	    goto ret0 ;
	}

	if (namepp != NULL)
	    *namepp = (char *) key.buf ;

	sep = (Elf32_Sym *) value.buf ;
	if (sepp != NULL)
	    *sepp = sep ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_enumsym: value=%08x\n",
	        (uint) sep->st_value) ;
#endif

ret0:
	return rs ;
}
/* end subroutine (lmapprog_enumsym) */


/* initialize a Segment Cursor */
int lmapprog_segcurbegin(mpp,cp)
LMAPPROG		*mpp ;
LMAPPROG_SEGCURSOR	*cp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0)) {

	    debugprintf("lmapprog_segcurbegin: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = mpp->pip ;

	if (cp == NULL)
	    return SR_FAULT ;

	cp->i = -1 ;

	return rs ;
}
/* end subroutine (lmapprog_segcurbegin) */


/* free up a Segment Cursor */
int lmapprog_segcurend(mpp,cp)
LMAPPROG		*mpp ;
LMAPPROG_SEGCURSOR	*cp ;
{
	struct proginfo	*pip ;

	int	rs = SR_OK ;


	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0)) {

	    debugprintf("lmapprog_segcurbegin: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = mpp->pip ;

	if (cp == NULL)
	    return SR_FAULT ;

	cp->i = -1 ;

	return rs ;
}
/* end subroutine (lmapprog_segcurend) */


/* enumerate some segment information */
int lmapprog_enumseg(mpp,cp,sip)
LMAPPROG		*mpp ;
LMAPPROG_SEGCURSOR	*cp ;
LMAPPROG_SEGINFO	*sip ;
{
	struct proginfo		*pip ;

	LMAPPROG_SEGMENT	*psp ;

	int	rs = SR_OK ;
	int	i ;


#if	CF_DEBUGS
	    debugprintf("lmapprog_enumseg: entered \n") ;
#endif

	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0)) {

	    debugprintf("lmapprog_segenum: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = mpp->pip ;

	if (cp == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	    debugprintf("lmapprog_enumseg: cp_i=%d\n",cp->i) ;
#endif

	if (sip == NULL)
	    return SR_FAULT ;

	i = (cp->i < 0) ? 0 : cp->i + 1 ;

#if	CF_DEBUGS
	    debugprintf("lmapprog_enumseg: pip=%p dl=%d i=%d\n",
		pip,pip->debuglevel,i) ;
#endif

	rs = vecitem_get(&mpp->segments,i,&psp) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	    debugprintf("lmapprog_enumseg: vecitem_get() rs=%d\n",rs) ;
#endif

	if (rs >= 0) {

	sip->start = psp->vaddr ;
	sip->len = psp->vlen ;
	sip->prot = psp->prot ;

		cp->i = i ;

	}

	return rs ;
}
/* end subroutine (lmapprog_enumseg) */


/* get pointers to section headers */
int lmapprog_getsec(mpp,i,shpp)
LMAPPROG		*mpp ;
int			i ;
Elf32_Shdr		**shpp ;
{
	struct proginfo		*pip ;

	int	rs = SR_OK ;


#if	CF_DEBUGS
	    debugprintf("lmapprog_getsec: entered \n") ;
#endif

	if (mpp == NULL)
	    return SR_FAULT ;

#if	CF_SAFE
	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0)) {

	    debugprintf("lmapprog_getsec: bad format\n") ;

	    return SR_BADFMT ;
	}

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif /* CF_SAFE */

	pip = mpp->pip ;

	if (shpp == NULL)
	    return SR_FAULT ;

	if (i < 0)
		return SR_INVALID ;

	if (i >= mpp->nsheads)
		return SR_NOTFOUND ;

	*shpp = mpp->sheads + i ;

	return rs ;
}
/* end subroutine (lmapprog_getsec) */



/* TEST SUBROUTINES (public but should not be used) */



/* read an integer (32 bits) from the program memory */
int lmapprog_readtest(mpp,vaddr,vp)
LMAPPROG	*mpp ;
uint		vaddr ;
uint		*vp ;
{
	struct proginfo		*pip ;

	LMAPPROG_SEGMENT	*psp ;

	HDB_DATUM		key, value ;

	LMAPPROG_PTE		*ptep ;

	ulong	vbase ;

	uint	*pa ;

	int	rs ;
	int	i ;


#if	CF_SAFE
	if (mpp == NULL)
	    return SR_FAULT ;

	if ((mpp->magic != LMAPPROG_MAGIC) && (mpp->magic != 0))
	    return SR_BADFMT ;

	if (mpp->magic != LMAPPROG_MAGIC)
	    return SR_NOTOPEN ;
#endif

	pip = mpp->pip ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_readtest: vaddr=%08x\n",vaddr) ;
#endif

	if (vaddr & 3)
	    return SR_INVALID ;

/* find this virtual address in the segments */

	for (i = 0 ; (rs = vecitem_get(&mpp->segments,i,&psp)) >= 0 ; i += 1) {

	    if ((vaddr >= psp->vaddr) &&
	        ((vaddr + 4) <= (psp->vaddr + psp->vlen)))
	        break ;

	} /* end for */

	if (rs < 0)
	    return SR_FAULT ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4) {
	    debugprintf("lmapprog_readtest: progsect=%d\n",i) ;
	    debugprintf("lmapprog_readtest: vbase=%08x pa=%08lx\n",
	        psp->vbase,psp->pa) ;
	}
#endif

	pa = (uint *) (((uint) psp->pa) + (vaddr - psp->vpbase)) ;

#if	CF_DEBUG && 0
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_readtest: pframe=%08lx pa=%08lx\n",
	        psp->pa,pa) ;
#endif

	*vp = *pa ;
	return SR_OK ;
}
/* end subroutine (lmapprog_readtest) */



/* PRIVATE SUBROUTINES */



/* map the symbol tables along with their string tables */
static int lmapprog_initsymtabs(mpp,ehp,sheads,mapalign)
LMAPPROG	*mpp ;
Elf32_Ehdr	*ehp ;
Elf32_Shdr	*sheads ;
uint		mapalign ;
{
	LMAPPROG_SYMTAB	st ;

	offset_t	moff ;

	ulong	fextent, fbase ;
	ulong	leadgap ;

	uint	fmsize ;

	int	rs ;
	int	i, j ;
	int	mflags, mprot ;


/* initialize the container for symbol tables */

	rs = vecitem_start(&mpp->symtabs,100,VECITEM_PNOHOLES) ;
	if (rs < 0)
	    goto bad3 ;

	mpp->f.symtab = TRUE ;

/* loop searching for symbol tables and mapping them! */

	for (i = 0 ; i < ehp->e_shnum ; i += 1) {

	    if (sheads[i].sh_type == SHT_SYMTAB) {

	        st.nsyms = sheads[i].sh_size / sheads[i].sh_entsize ;

	        fbase = sheads[i].sh_offset & (~ (mapalign - 1)) ;
	        fextent = sheads[i].sh_offset + sheads[i].sh_size ;
	        fmsize = ceiling(fextent,mapalign) - fbase ;

	        leadgap = sheads[i].sh_offset - fbase ;
	        st.maplen_symtab = (uint) fmsize ;
	        st.entsize = sheads[i].sh_entsize ;

	        mflags = MAP_SHARED ;
	        mprot = PROT_READ ;
		moff = fbase ;
	        rs = u_mapfile(NULL,(size_t) fmsize,mprot,mflags,
	            mpp->ofd,moff,&st.pa_symtab) ;

	        st.pa_strings = NULL ;
	        st.symtab = (Elf32_Sym *) (st.pa_symtab + leadgap) ;
	        st.strings = NULL ;

	        if ((rs >= 0) && (sheads[i].sh_link > 0)) {

	            j = (int) sheads[i].sh_link ;

/* map the string table */

	            fbase = sheads[j].sh_offset & (~ (mapalign - 1)) ;
	            fextent = sheads[j].sh_offset + sheads[j].sh_size ;
	            fmsize = ceiling(fextent,mapalign) - fbase ;

	            leadgap = sheads[j].sh_offset - fbase ;
	            st.maplen_strings = (uint) fmsize ;
	            st.entsize = sheads[j].sh_entsize ;

	            mflags = MAP_SHARED ;
	            mprot = PROT_READ ;
		    moff = fbase ;
	            rs = u_mapfile(NULL,(size_t) fmsize,mprot,mflags,
	                mpp->ofd,moff,&st.pa_strings) ;

	            st.strings = (char *) (st.pa_strings + leadgap) ;

	            if (rs < 0) {

	                if (st.pa_symtab != NULL)
	                    u_munmap(st.pa_symtab,st.maplen_symtab) ;

	            }

	        } /* end if (had strings) */

	        if (rs < 0)
	            goto bad4 ;

/* save this symbol table */

	        rs = vecitem_add(&mpp->symtabs,&st,
	            sizeof(LMAPPROG_SYMTAB)) ;

	        if (rs < 0) {

	            if (st.pa_symtab != NULL)
	                u_munmap(st.pa_symtab,st.maplen_symtab) ;

	            if (st.pa_strings != NULL)
	                u_munmap(st.pa_strings,st.maplen_strings) ;

	            goto bad4 ;
	        }

	    } /* end if (got a symbol table) */

	} /* end for (searching for symbol tables) */

	return rs ;

/* bad things come here */
bad4:
	if (mpp->f.symtab) {

	    LMAPPROG_SYMTAB	*stp ;


	    for (i = 0 ; vecitem_get(&mpp->symtabs,i,&stp) >= 0 ; i += 1) {

	        if (stp == NULL) continue ;

	        if (stp->pa_symtab != NULL)
	            u_munmap(stp->pa_symtab,stp->maplen_symtab) ;

	        if (stp->pa_strings != NULL)
	            u_munmap(stp->pa_strings,stp->maplen_strings) ;

	    } /* end for */

	    vecitem_finish(&mpp->symtabs) ;

	    mpp->f.symtab = FALSE ;

	} /* end if */

bad3:
	return rs ;
}
/* end subroutine (lmapprog_initsymtabs) */


/* free up the symbol tables */
static int lmapprog_freesymtabs(mpp)
LMAPPROG	*mpp ;
{
	    LMAPPROG_SYMTAB	*stp ;

	int	rs ;
	int	i ;


	    for (i = 0 ; vecitem_get(&mpp->symtabs,i,&stp) >= 0 ; i += 1) {

	        if (stp == NULL) continue ;

	        if (stp->pa_symtab != NULL)
	            u_munmap(stp->pa_symtab,stp->maplen_symtab) ;

	        if (stp->pa_strings != NULL)
	            u_munmap(stp->pa_strings,stp->maplen_strings) ;

	} /* end for */

	rs = vecitem_finish(&mpp->symtabs) ;

	return rs ;
} 
/* end subroutine (lmapprog_freesymtabs) */


/* initialize the symbols */
static int lmapprog_initsymbols(mpp)
LMAPPROG	*mpp ;
{
	    HDB_DATUM		key, value ;

	    LMAPPROG_SYMTAB	*stp ;

#ifdef	COMMENT
	    LMAPPROG_SYMBOL	*sep ;
#endif

	    Elf32_Sym	*eep ;

	int	rs, c = 0 ;
	    int	i, j ;


	if (! mpp->f.symtab)
		return SR_NOTAVAIL ;

	    rs = hdb_start(&mpp->symbols,100,1,NULL,NULL) ;
	    if (rs < 0)
	        goto bad9 ;

	    for (i = 0 ; vecitem_get(&mpp->symtabs,i,&stp) >= 0 ; i += 1) {

	        if (stp == NULL) continue ;

	        for (j = 0 ; j < stp->nsyms ; j += 1) {

	            eep = stp->symtab + j ;
	            if (eep->st_name == 0) continue ;

#if	CF_DEBUG && 0
	            if (pip->debuglevel >= 4)
	                debugprintf("lmapprog_initsymbols: s> %s\n",
	                    (stp->strings + eep->st_name)) ;
#endif

	            key.buf = (stp->strings + eep->st_name) ;
	            key.len = -1 ;

#ifdef	COMMENT
	            rs = uc_malloc(sizeof(LMAPPROG_SYMBOL),&sep) ;

	            if (rs < 0)
	                break ;

	            sep->ep = eep ;
	            sep->name = (char *) (stp->strings + eep->st_name) ;

	            value.buf = sep ;
	            value.len = sizeof(LMAPPROG_SYMBOL) ;
#else
	            value.buf = eep ;
	            value.len = sizeof(Elf32_Sym) ;
#endif /* COMMENT */

	            rs = hdb_store(&mpp->symbols,key,value) ;

	            if (rs < 0) {
#ifdef	COMMENT
	                uc_free(sep) ;
#endif
	                break ;
	            }

			c += 1 ;

	        } /* end for */

	        if (rs < 0) break ;
	    } /* end for (looping over symbol tables) */

	    if (rs < 0) {
	        lmapprog_freesymbols(mpp) ;
	        goto bad9 ;
	    }

	    mpp->f.symbols = TRUE ;

bad9:
	return (rs >= 0) ? c : rs ;
} 
/* end subroutine (lmapprog_initsymbols) */


/* free up the indexed symbols, if any */
static int lmapprog_freesymbols(mpp)
LMAPPROG	*mpp ;
{
	HDB_CUR		cur ;
	HDB_DATUM	key, value ;
#ifdef	COMMENT
	LMAPPROG_SYMBOL	*sep ;
#endif
	int		rs = SR_OK ;


	if (! mpp->f.symbols)
	    return rs ;

#ifdef	COMMENT

	hdb_curbegin(&mpp->symbols,&cur) ;

	while (hdb_enum(&mpp->symbols,&cur,&key,&value) >= 0) {

	    sep = (LMAPPROG_SYMBOL *) value.buf ;

	    if (sep != NULL) {
	        uc_free(sep) ;
	    }

	} /* end while */

	hdb_curend(&mpp->symbols,&cur) ;

#endif /* COMMENT */

	hdb_finish(&mpp->symbols) ;

	mpp->f.symbols = FALSE ;
	return rs ;
}
/* end subroutine (lmapprog_freesymbols) */


/* zero out leading parts of BSS sections */
static int lmapprog_zerobss(mpp,pip,sheads,n,mapalign)
LMAPPROG	*mpp ;
struct proginfo	*pip ;
Elf32_Shdr	*sheads ;
int		n ;
uint		mapalign ;
{
	caddr_t		pa ;
	uint		vaddr, vextent ;
	uint		vmbase ;
	int		rs ;
	int		i ;
	int		c = 0 ;
	int		wlen ;

	for (i = 0 ; i < n ; i += 1) {

	    if ((sheads[i].sh_addr > 0) && (sheads[i].sh_size > 0) &&
	    	(sheads[i].sh_type == SHT_NOBITS)) {

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	         debugprintf("lmapprog_zerobss: si=%d addr=%08lx size=%08lx\n",
	                i,sheads[i].sh_addr,sheads[i].sh_size) ;
#endif

	        vaddr = sheads[i].sh_addr ;
	        rs = lmapprog_getphysical(mpp,vaddr,&pa) ;

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	            debugprintf("lmapprog_zerobss: PA rs=%d paddr=%08lx\n",
			rs,pa) ;
#endif

	        if (rs >= 0) {

	            vmbase = vaddr & (~ (mapalign - 1)) ;
	            wlen = (vmbase + mapalign) - vaddr ;
	            if (sheads[i].sh_size < wlen)
			wlen = sheads[i].sh_size ;

#if	CF_DEBUG
	if (pip->debuglevel >= 5)
	                debugprintf("lmapprog_zerobss: wlen=%08x\n", wlen) ;
#endif

	            memset(pa,0,wlen) ;

	            c += 1 ;
	        }

	    } /* end if */

	} /* end for */

	return c ;
}
/* end subroutine (lmapprog_zerobss) */


#if	CF_STACKARGS

/* load up the user specified arguments on the simulated program stack */
static int lmapprog_loadargs(mpp,argv,envv)
LMAPPROG	*mpp ;
char		*argv[] ;
char		*envv[] ;
{
	struct proginfo	*pip ;

	caddr_t	dummy ;

	uint	vstackp, vstringp ;

	int	rs, i ;
	int	argc, envc ;
	int	arglen, envlen ;
	int	stringlen, totallen ;
	int	vargvp, venvvp ;
	int	vargp, venvp ;
	int	sl ;

#if	CF_MASTERDEBUG && CF_DEBUG
	int	left ;
#endif

	char	*pstackp ;	/* physical stack pointer */
	char	**envvp, **argvp ;
	char	*envp, *argp ;
	char	*stringp ;	/* string pointer */


	pip = mpp->pip ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: entered\n") ;
#endif

	vstackp = mpp->progstack ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: initial virtual SP %08lx\n",
	        vstackp) ;
#endif

	argc = 0 ;
	arglen = 0 ;
	for (i = 0 ; argv[i] != NULL ; i += 1) {

	    argc += 1 ;
	    arglen += (strlen(argv[i]) + 1) ;

	}

	envc = 0 ;
	envlen = 0 ;
	for (i = 0 ; envv[i] != NULL ; i += 1) {

	    envc += 1 ;
	    envlen += (strlen(envv[i]) + 1) ;

	}

	stringlen = arglen + envlen ;
	if (stringlen > LMAPPROG_STACKSTRSIZE)
	    return SR_TOOBIG ;

/* get the physical start of string area */

	vstringp = vstackp - LMAPPROG_STACKSTRSIZE ;
	rs = lmapprog_getphysical(mpp,vstringp,&stringp) ;

	if (rs < 0) {

	    rs = SR_TOOBIG ;
	    goto bad ;
	}

/* continue */

#if	CF_SGIENV
	totallen = ((1 + argc + 1 + envc + 1) * sizeof(uint)) +
	    LMAPPROG_STACKSTRSIZE ;
#else
	totallen = ((1 + argc + 1 + envc + 1) * sizeof(uint)) +
	    arglen + envlen ;
#endif

/* round the total length up to the next multiple of (?) 16! */

	totallen = (totallen + (16 - 1)) & (~ (16 - 1)) ;

	totallen += (6 * sizeof(uint)) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: totallen=%08x\n", totallen) ;
#endif

	vstackp -= totallen ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: planned virtual SP %08lx\n",
	        vstackp) ;
#endif

/* see if enough stack space was configured from the start! */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: lmapprog_getphysical() \n") ;
#endif

	rs = lmapprog_getphysical(mpp,vstackp,&pstackp) ;

	if (rs < 0) {

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_loadargs: lmapprog_getphysical() rs=%d\n",
	            rs) ;
#endif

	    rs = SR_TOOBIG ;
	    return rs ;
	}

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: pstackp=%08lx\n",pstackp) ;
#endif

/* we could do this virtual or physical, we'll choose physical! */

	argvp = (char **) (pstackp + (7 * sizeof(int))) ;
	envvp = argvp + (argc + 1) ;

/* calculate the virtual address equivalents of above */

	vargvp = vstackp + (7 * sizeof(int)) ;
	venvvp = vargvp + ((argc + 1) * sizeof(int)) ;

	vargp = vstringp ;
	venvp = vstringp + arglen ;


/* load the direct arguments to 'main()' first */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: the 3 direct args\n") ;
#endif

	{
	    uint	*ip ;


	    ip = (uint *) pstackp ;

	    *ip++ = (uint) argc ;
	    *ip++ = (uint) vargvp ;
	    *ip++ = (uint) venvvp ;
	    for (i = 0 ; i < 3 ; i += 1)
	        *ip++ = 0 ;

	    *ip++ = (uint) argc ;

	} /* end block */

/* do the arguments and their pointer array */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: argument strings\n") ;
#endif

	for (i = 0 ; argv[i] != NULL ; i += 1) {

	    sl = strlen(argv[i]) ;

	    *argvp++ = (char *) vargp ;
	    strcpy(stringp,argv[i]) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_loadargs: len=%2d a> %s\n",sl,argv[i]) ;
#endif


	    stringp += (sl + 1) ;
	    vargp += (sl + 1) ;

	} /* end for */

	*argvp = NULL ;

/* do the environment variables and their pointer array */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf("lmapprog_loadargs: environment strings\n") ;
#endif

	for (i = 0 ; envv[i] != NULL ; i += 1) {

	    sl = strlen(envv[i]) ;

	    *envvp++ = (char *) venvp ;
	    strcpy(stringp,envv[i]) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	    if (pip->debuglevel >= 4)
	        debugprintf("lmapprog_loadargs: len=%2d e> %s\n",sl,envv[i]) ;
#endif

	    stringp += (sl + 1) ;
	    venvp += (sl + 1) ;

	} /* end for */

	*envvp = NULL ;

/* we should be done with this! */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4) {

	    char	*s ;


	    debugprintf("lmapprog_loadargs: virtual argument review\n") ;

	    argvp = (char **) (pstackp + (7 * sizeof(int))) ;
	    envvp = argvp + (argc + 1) ;

	    debugprintf("lmapprog_loadargs: argvp[0]=%08x\n",argvp[0]) ;
	    debugprintf("lmapprog_loadargs: argvp[1]=%08x\n",argvp[1]) ;

	    for (i = 0 ; argvp[i] != NULL ; i += 1) {

	        rs = lmapprog_getphysical(mpp,(uint) argvp[i],&s) ;

	        if (rs >= 0)
	            debugprintf("lmapprog_loadargs: a> %s\n",s) ;

	        else
	            debugprintf("lmapprog_loadargs: error for va=%08x\n",
	                (uint) argvp[i]) ;

	    } /* end for */

	    for (i = 0 ; envvp[i] != NULL ; i += 1) {

	        rs = lmapprog_getphysical(mpp,(uint) envvp[i],&s) ;

	        if (rs >= 0)
	            debugprintf("lmapprog_loadargs: e> %s\n",s) ;

	        else
	            debugprintf("lmapprog_loadargs: error for va=%08x\n",
	                (uint) envvp[i]) ;

	    } /* end for */

	}
#endif /* CF_DEBUG */

/* update the simulated program stack pointer */

	mpp->progstack -= (totallen - 24) ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel >= 4)
	    debugprintf(
	        "lmapprog_loadargs: exiting OK progstack=%08x vstackp=%08x\n",
	        mpp->progstack,vstackp) ;
#endif

	return SR_OK ;

/* bad stuff */
bad:
	return rs ;
}
/* end subroutine (lmapprog_loadargs) */

#endif /* F_STACKARGS */


/* check if we have an ELF file and if we can handle it */
static int checkelf(pip,ehp)
struct proginfo	*pip ;
Elf32_Ehdr	*ehp ;
{
	int	rs = SR_OK ;


#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: magic\n") ;
#endif

	if ((ehp->e_ident[0] != 0x7f) ||
	    (strncmp((char *) (ehp->e_ident + 1),"ELF",3) != 0)) {

	    bprintf(pip->efp,
	        "%s: program file is not an ELF object\n",
	        pip->progname) ;

	    rs = SR_BADFMT ;
	    goto bad2 ;
	}

/* check the file class (we can only handle 32 bit) */

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: file class\n") ;
#endif

	if (ehp->e_ident[EI_CLASS] != ELFCLASS32) {

	    bprintf(pip->efp,
	        "%s: program file is not a 32-bit class executable\n",
	        pip->progname) ;

	    rs = SR_NOTSUP ;
	    goto bad2 ;
	}

/* check for a version of the ELF file that we understand */

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: ident version\n") ;
#endif

	if ((ehp->e_ident[EI_VERSION] == 0) ||
	    (ehp->e_ident[EI_VERSION] > EV_CURRENT)) {

	    bprintf(pip->efp,
	        "%s: program file is an unsupported ELF version\n",
	        pip->progname) ;

	    rs = SR_NOTSUP ;
	    goto bad2 ;
	}

/* check the data encoding (we can only handle 32 bit Big-Endian) */

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: byte ordering\n") ;
#endif

	if (ehp->e_ident[EI_DATA] != ELFDATA2MSB) {

	    bprintf(pip->efp,
	        "%s: program file is not a MSB-data ELF object\n",
	        pip->progname) ;

	    rs = SR_NOTSUP ;
	    goto bad2 ;
	}

/* check version in the header entry */

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: header version\n") ;
#endif

	if ((ehp->e_version == 0) ||
	    (ehp->e_version > EV_CURRENT)) {

	    bprintf(pip->efp,
	        "%s: program file is not a supported ELF version object\n",
	        pip->progname) ;

	    rs = SR_NOTSUP ;
	    goto bad2 ;
	}

/* must be for a MIPS machine! */

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: MIPS ISA\n") ;
#endif

	if (ehp->e_machine != EM_MIPS) {

	    bprintf(pip->efp,
	        "%s: program file is not a MIPS executable object\n",
	        pip->progname) ;

	    rs = SR_INVALID ;
	    goto bad2 ;
	}

/* check that flags represent a static executable (not dynamic) */

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: static executable\n") ;
#endif

	if ((ehp->e_type != ET_EXEC) ||
	    ((ehp->e_flags & EF_MIPS_PIC) != 0) ||
	    ((ehp->e_flags & EF_MIPS_CPIC) != 0)) {

	    bprintf(pip->efp,
	        "%s: program file is not a statically linked executable\n",
	        pip->progname) ;

	    rs = SR_INVALID ;
	    goto bad2 ;
	}

/* must be a MIPS1 (R3000) executable */

#if	CF_MIPS1

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: MIPS R3000\n") ;
#endif

	if ((ehp->e_flags & EF_MIPS_ARCH) != EF_MIPS_ARCH_1) {

#if	CF_DEBUGS
	    debugprintf("lmapprog/checkelf: not MIPS R3000!\n") ;
#endif

	    bprintf(pip->efp,
	        "%s: program file is not a MIPS1 executable\n",
	        pip->progname) ;

	    rs = SR_INVALID ;
	    goto bad2 ;
	}

#endif /* F_MIPS1 */

bad2:

#if	CF_DEBUGS
	debugprintf("lmapprog/checkelf: rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (checkelf) */


/* find a section by its name (and type if specified) */
static int findsection(sheads,n,shstrings,name,type)
Elf32_Shdr	*sheads ;
int		n ;
char		shstrings[], name[] ;
int		type ;
{
	int	i ;
	int	namei ;


	for (i = 0 ; i < n ; i += 1) {

	    namei = sheads[i].sh_name ;
	    if (strcmp(shstrings + namei,name) == 0) {

	        if (type < 0)
	            break ;

	        if (sheads[i].sh_type == type)
	            break ;

	    }

	} /* end for */

	return ((i < n) ? i : -1) ;
}
/* end subroutine (findsection) */


/* find a section by its name (and type if specified) */
static int findhighsection(sheads,n,shstrings,name,type)
Elf32_Shdr	*sheads ;
int		n ;
char		shstrings[], name[] ;
int		type ;
{
	uint	maxaddr ;

	int	si, i ;
	int	namei ;


	si = -1 ;
	maxaddr = 0 ;
	for (i = 0 ; i < n ; i += 1) {

	    if ((name != NULL) && (shstrings != NULL)) {

	        namei = sheads[i].sh_name ;
	        if (strcmp(shstrings + namei,name) != 0)
	            continue ;

	    }

	    if (type < 0)
	        continue ;

	    if ((sheads[i].sh_type == type) && 
	        (sheads[i].sh_addr > maxaddr)) {

	        si = i ;
	        maxaddr = sheads[i].sh_addr ;

	    } /* end if (it was higher!) */

	} /* end for */

	return si ;
}
/* end subroutine (findhighsection) */


/* calculate the ceiling of a value based on alignment */
static ulong ceiling(extent,align)
ulong	extent ;
uint	align ;
{
	long	rem ;
	long	top ;


	rem = extent & (align - 1) ;
	top = extent & (~ (align - 1)) ;
	if (rem)
	    top += align ;

	return top ;
}
/* end subroutine (ceiling) */


