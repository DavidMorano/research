/* lmapprog HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* simulated program mapping manager */
/* version %I% last-modified %G% */


/* revision history:

	= 2000-07-26, David Morano
	This code was started.

*/

/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	LMAPPROG_INCLUDE
#define	LMAPPROG_INCLUDE	1


#include	<envstandards.h>	/* must be ordered first to configure */
#include	<sys/types.h>
#include	<unistd.h>
#include	<elf.h>
#include	<ctime>
#include	<cstddef>		/* |nullptr_t| */
#include	<cstdlib>
#include	<usystem>
#include	<vecitem.h>
#include	<hdb.h>
#include	<localmisc.h>

#include	"config.h"
#include	"defs.h"


/* object defines */
#define	LMAPPROG		struct lmapprog_head
#define	LMAPPROG_SYMTAB		struct lmapprog_symtab
#define	LMAPPROG_SEGMENT	struct lmapprog_segment
#define	LMAPPROG_PTE		struct lmapprog_pte
#define	LMAPPROG_SYMBOL		struct lmapprog_symbol
#define	LMAPPROG_MAXPROG	struct lmapprog_mp
/* Segment Cursor */
#define	LMAPPROG_SEGCURSOR	struct lmapprog_segcur
/* Segment Information */
#define	LMAPPROG_SEGINFO	struct lmapprog_seginfo

/* more object defines (need HDB first) */

/* Symbol Name (SN) Cursor */
#define	LMAPPROG_SNCURSOR	HDB_CUR


/* a segment iteration cursor */
struct lmapprog_segcur {
	int	i ;
} ;

/* a structure to return a segment start and length */
struct lmapprog_seginfo {
	uint	start, len ;
	int	prot ;
} ;

/* just some structure to return some numbers */
struct lmapprog_mp {
	uint	data ;
	uint	stack ;
} ;

/* a single humble symbol */
struct lmapprog_symbol {
	Elf32_Sym	*ep ;
	char		*name ;
} ;

/* this is a symbol table, there can be many per object file ! */
struct lmapprog_symtab {
	caddr_t		pa_symtab ;	/* base address */
	caddr_t		pa_strings ;	/* base address */
	Elf32_Sym	*symtab ;	/* the symbol table */
	char		*strings ;	/* the string table */
	uint		maplen_symtab ;
	uint		maplen_strings ;
	uint		entsize ;	/* entry size */
	uint		nsyms ;		/* number of symbols */
} ;

struct lmapprog_pteflags {
	uint	read : 1 ;		/* page is readable */
	uint	write : 1 ;		/* page is writable */
	uint	exec : 1 ;		/* page is executable */
} ;

struct lmapprog_pte {
	struct lmapprog_pteflags	f ;
	caddr_t		pa ;		/* memory physical address */
	ulong		vbase ;		/* virtual page base address (key) */
	ulong		vaddr ;		/* starting virtual address */
	ulong		vlen ;		/* length within page */
} ;

struct lmapprog_segmentflags {
	uint		backwards : 1 ;	/* direction of growth */
} ;

struct lmapprog_segment {
	struct lmapprog_segmentflags	f ;
	caddr_t		pa ;		/* memory physical address */
	offset_t		offset ;	/* original ELF offset */
	ulong		vaddr ;		/* program virtual address */
	ulong		vmbase ;	/* map base */
	ulong		vpbase ;	/* virtual page base */
	uint		pagealign ;	/* segment page alignment */
	uint		mapalign ;	/* segment map alignment */
	uint		filesize ;
	uint		memsz ;		/* memory size (program segment) */
	uint		vlen ;		/* valid length from 'vaddr' ! */
	uint		vmsize ;	/* map size */
	uint		vpsize ;	/* virtual page size */
	int		prot ;		/* protection information */
} ;

struct lmapprog_flags {
	uint		bss : 1 ;	/* there was a BSS section */
	uint		symtab : 1 ;	/* there were symbol tables */
	uint		symbols : 1 ;	/* there are indexed symbols */
} ;

struct lmapprog_head {
	unsigned long	magic ;
	struct lmapprog_flags	f ;
	vecitem		segments ;	/* program segments */
	vecitem		symtabs ;	/* symbol tables */
	HDB		pagetable ;
	HDB		symbols ;	/* fast symbol access */
	struct proginfo		*pip ;	/* program information */
	char		*filename ;	/* ELF program file name */
	Elf32_Shdr	*sheads  ;
	time_t		lastaccess ;	/* last access time (informational) */
	ulong		pagealign ;	/* virtual page alignment */
	ulong		mapalign ;	/* alignment for mapping */
	ulong		progentry ;	/* program entry address */
	ulong		progstack ;	/* program stack pointer */
	ulong		progbreak ;	/* program break address */
	ulong		basestack ;	/* base program stack */
	ulong		basebreak ;	/* base program break */
	int		nsheads ;	/* number of sections */
	int		ofd ;		/* object file descriptor */
	int		zfd ;		/* "/dev/zero" FD */
} ;


#if	(! defined(LMAPPROG_MASTER)) || (LMAPPROG_MASTER == 1)

#ifdef	__cplusplus
extern "C" {
#endif

extern int lmapprog_init(LMAPPROG *,struct proginfo *,char *,char **,char **) ;
extern int lmapprog_free(LMAPPROG *) ;
extern int lmapprog_getentry(LMAPPROG *,uint *) ;
extern int lmapprog_getstack(LMAPPROG *,uint *) ;
extern int lmapprog_getbreak(LMAPPROG *,uint *) ;
extern int lmapprog_getpagesize(LMAPPROG *,uint *) ;
extern int lmapprog_getmax(LMAPPROG *,LMAPPROG_MAXPROG *) ;
extern int lmapprog_setbreak(LMAPPROG *,uint) ;
extern int lmapprog_getphysical(LMAPPROG *,uint,caddr_t *) ;
extern int lmapprog_readinstr(LMAPPROG *,uint,uint *) ;
extern int lmapprog_readint(LMAPPROG *,uint,uint *) ;
extern int lmapprog_writeint(LMAPPROG *,uint,uint,uint) ;
extern int lmapprog_memaccess(LMAPPROG *,uint,uint) ;
extern int lmapprog_getsym(LMAPPROG *,char *,Elf32_Sym **) ;
extern int lmapprog_sncurbegin(LMAPPROG *,LMAPPROG_SNCURSOR *) ;
extern int lmapprog_sncurend(LMAPPROG *,LMAPPROG_SNCURSOR *) ;
extern int lmapprog_fetchsym(LMAPPROG *,char *, LMAPPROG_SNCURSOR *,
		Elf32_Sym **) ;
extern int lmapprog_enumsym(LMAPPROG *, LMAPPROG_SNCURSOR *,
		char **, Elf32_Sym **) ;
extern int lmapprog_segcurbegin(LMAPPROG *,LMAPPROG_SEGCURSOR *) ;
extern int lmapprog_segcurend(LMAPPROG *,LMAPPROG_SEGCURSOR *) ;
extern int lmapprog_enumseg(LMAPPROG *,LMAPPROG_SEGCURSOR *,
		LMAPPROG_SEGINFO *) ;
extern int lmapprog_getsec(LMAPPROG *,int,Elf32_Shdr **) ;

#ifdef	__cplusplus
}
#endif

#endif /* (! defined(LMAPPROG_MASTER)) || (LMAPPROG_MASTER == 1) */

#endif /* LMAPPROG_INCLUDE */


