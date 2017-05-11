/* mint_init */

/* initialize MINT ? */


#define	CF_DEBUGS	0
#define	CF_DEBUG		0


/* revision history :

	= 00/02/04, Dave Morano
	I have changed quite a bit with regard to how MINT
	gets initialized.  We want to provide for a better
	isolation of the simulated program from the simulator
	itself than MINT did.


*/


/******************************************************************************

 * Routines for reading and parsing the text section and managing the
 * memory for an address space.
 *
 * Copyright (C) 1993 by Jack E. Veenstra (veenstra@cs.rochester.edu)
 * 
 * This file is part of MINT, a MIPS code interpreter and event generator
 * for parallel programs.
 * 
 * MINT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * MINT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MINT; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 

******************************************************************************/



#include	<sys/types.h>
#include	<sys/param.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <stdio.h>

#include	"getfname.h"
#include	"paramfile.h"

#include	"localmisc.h"
#include	"config.h"
#include	"defs.h"

#if defined(__svr4__) || defined(_SVR4_SOURCE)
#include <sys/times.h>
#endif
#include <limits.h>

#define MAIN

#include "icode.h"
#include "opcodes.h"
#include "globals.h"
#include "version.h"



/* Note: the debugger dbx gets line numbers confused because of embedded
 * newlines inside the double quotes in the following macro.
 */
#ifndef DEBUG
#define USAGE \
"\nUsage: %s [mint options] [-- simulator options] objfile [objfile options]\n\
\n\
mint options:\n\
	[-C interval]		checkpoint interval\n\
	[-c file]		file of cycles for operations\n\
	[-e file]		use file to generate events\n\
	[-h heap_size]		heap size in bytes, default: %d (0x%x)\n\
	[-I]			use hardware interlocking. Not supported yet\n\
	[-K keyfile]		file containing key for socket connections\n\
	[-k stack_size]		stack size in bytes, default: %d (0x%x)\n\
	[-l page=size,blk=size]	lock allocation policy, default: page=%d,blk=%d\n\
	[-m mem_size]		align private regions on mem_size boundary\n\
	[-O0]			no optimization\n\
	[-O1]			execute instruction blocks, default\n\
	[-P port]		port for socket connections, default: %d\n\
	[-p procs]		number of per-process regions, default: %d\n\
	[-q]			quiet mode -- suppress all warning messages\n\
	[-r]			back-end uses release consistency\n\
	[-R]			do not recycle process ids\n\
	[-s shmem_size]		shared memory size, default: %d (0x%x)\n\
	[-S]			spin, don't block, on a failed lock acquire\n\
	[-t i]			trace instrutions\n\
	[-t n]			trace nothing\n\
	[-t r]			trace memory references, default\n\
	[-t s]			trace shared memory references\n\
	[-u file]		log events to file for Upshot\n\
	[-V]			verify protocol\n\
	[-v]			print version number\n\
	[-W]			check load-linked addr on every write\n"
#else
#define USAGE "\nUsage: %s [-m mem_size] [-s shmem_size] [and other stuff...] objfile\n"
#endif

FILE *Fobj;		/* file descriptor for object file */
static int Nicode;	/* number of free icodes left */

static time_t Start_time ;
static time_t Finish_time ;
static char Start_date[40] ;
static char Finish_date[40] ;
static char * Mem_size_start ;
static char * Mem_size_finish ;
#ifdef __svr4__
static struct tms Tms_buffer ;
#else
static struct rusage Rusage ;
#endif
static double Total_elapsed_time ;
static double Total_cpu_time ;

static int Argc ;
static char **Argv ;

/* file name for cycles for each operation */
char *Fcycles ;

/* imported functions */
void get_sim_func_names(char *sim_file) ;

void init_all_queues() ;
void init_main_thread() ;
void malloc_share(thread_ptr child, thread_ptr parent) ;

/* exported functions */
void allocate_fixed(long addr, long nbytes) ;

/* private functions */
static void parse_args(int argc, char **argv) ;
static void save_args(int argc, char **argv) ;
static void copy_argv(int arg_start, int argc, char **argv, char **envp) ;
static void mint_stats() ;
static void read_cycles(char *fname) ;
static void create_addr_space() ;
static void read_text() ;
static void mint_usage() ;





int mint_init(pip, pfp,argc, argv, envp)
struct proginfo	*pip ;
PARAMFILE	*pfp ;
int	argc ;
char	*argv[] ;
char	*envp[] ;
{
	FILE *fd ;

	int	rs ;
	int next_arg ;


	Keyfile = NULL ;		/* MINT forgot to do this ! */

	(void) time(&Start_time) ;

	Mem_size_start = (char *) sbrk(0) ;

	Mint_output = stderr ;
	Simname = argv[0] ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: argv0=%s\n",Simname) ;
#endif

	save_args(argc, argv) ;

	parse_args(argc, argv) ;


/* find our real program executable file using our program root */

	if (Simname == NULL) {

	    char	fnamebuf[MAXPATHLEN + 1] ;
	    char	efname[MAXPATHLEN + 1] ;


	    bufprintf(fnamebuf,MAXPATHLEN,"bin/%s",PROG_LEVOSIM) ;

	    rs = getfname(pip->programroot,fnamebuf,GETFNAME_TYPEROOT,efname) ;

	    if (rs >= 0)
	        Simname = efname ;

	} /* end block (getting program executable) */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: Simname=%s\n",Simname) ;
#endif

	get_sim_func_names(Simname) ;


/* call the user's initialization subroutine */

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: initialize user args\n") ;
#endif

	next_arg = sim_init(argc, argv) ;

#ifdef	COMMENT
	if (next_arg >= argc && Fevents == NULL) {

	    fprintf(stderr, "Error: missing object file.\n") ;
	    mint_usage() ;
	    sim_usage(argc, argv) ;
	    exit(1) ;
	}
#else
#endif /* COMMENT */

	Objname = Argv[next_arg] ;

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: simprog Objname=%s\n",Objname) ;
#endif


/* check that the key file is readable */

	if (Keyfile != NULL) {

	    fd = fopen(Keyfile, "r") ;
	    if (fd == NULL) {
	        perror(Keyfile) ;
	        exit(1) ;
	    }

/* will read it later when setting up the socket */
	    fclose(fd) ;
	}

	if (Fcycles)
	    read_cycles(Fcycles) ;

	if (Upshot_log)
	    upshot_init(Upshot_log) ;

#ifndef NO_SOCKETS
	init_socket() ;
#endif

#if	CF_MASTERDEBUG && CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: init_all_queues()\n") ;
#endif

	init_all_queues() ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: Fevents=%d\n",Fevents) ;
#endif

	if (Fevents) {

	    read_events(Fevents) ;

	} else {

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: reading object file, Objname=%s\n",Objname) ;
#endif

	    read_hdrs(Objname) ;

/* do we really want to do what MINT does here ? */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: subst_init()\n") ;
#endif

	    subst_init() ;

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: read_text()\n") ;
#endif

	    read_text() ;

#ifdef	COMMENT
	    if (Optimization_level > 0)
	        optimize() ;
#endif /* COMMENT */

	    create_addr_space() ;
	    close_object() ;
	    copy_argv(next_arg, argc, argv, envp) ;

/* do we really want to do what MINT does here ? */
	    subst_functions() ;

	    init_main_thread() ;

	} /* end if (reading object file) */

#if	CF_DEBUG
	if (pip->debuglevel > 1)
	eprintf("mint_init: exiting\n") ;
#endif

	return 0 ;
}
/* end subroutine (mint_init) */



/* print out a dumb usage statement */
static void mint_usage()
{


	fprintf(stderr, USAGE, Simname,
	    HEAP_SIZE, HEAP_SIZE,
	    STACK_SIZE, STACK_SIZE,
	    LOCKPAGE_SIZE, LOCKBLK_SIZE,
	    PORT, DECF_MAX_NPROCS,
	    SHMEM_SIZE, SHMEM_SIZE) ;

	fflush(stderr) ;

}
/* end subroutine (mint_usage) */


/* copy the args */
static void save_args(int argc, char **argv)
{
	int i ;


	Argc = argc ;
	Argv = (char **) malloc((argc + 1) * sizeof(char *)) ;

	if (Argv == NULL)
	    fatal("save_args: cannot allocate 0x%x bytes for Argv.\n",
	        (argc + 1) * sizeof(char *)) ;

	for (i = 0; i < argc; i++) {

	    Argv[i] = (char *) malloc(strlen(argv[i]) + 1) ;
	    if (Argv[i] == NULL)
	        fatal("save_args: cannot allocate 0x%x bytes for Argv[%d].\n",
	            strlen(argv[i]) + 1, i) ;
	    strcpy(Argv[i], argv[i]) ;

	}
	Argv[argc] = NULL ;

}
/* end subroutine (save_args) */


#ifndef NO_SOCKETS

/* ARGSUSED */
void
cmd_argv(mint_time_t curtime, char *arg)
{
	int i ;

	for (i = 0; i < Argc; i++) {
	    transmit(Argv[i]) ;
	    transmit(" ") ;
	}
	transmit("\n") ;
}

#endif /* NO_SOCKETS */


/* call when done */
void mint_done(pip)
struct proginfo	*pip ;
{


	if (Mint_output)
	    mint_stats() ;

	sim_done(Total_elapsed_time, Total_cpu_time) ;
	if (Upshot_log)
	    upshot_done() ;
}
/* end subroutine (mint_done) */


static void mint_stats()
{
	int i ;
	unsigned int elapsed, minutes, seconds ;
	unsigned int user_sec, user_min, user_usec, sys_sec, sys_min, sys_usec ;
	unsigned int total_sec, total_min, total_usec ;
	double user_fsec, sys_fsec, total_fsec, cpu_seconds ;
	int mem_used ;
	char *machine_name ;
	struct utsname utsname ;


#ifdef	COMMENT

	Finish_time = time(NULL) ;
#ifdef __svr4__
	times(&Tms_buffer) ;
#else
	getrusage(RUSAGE_SELF, &Rusage) ;
#endif

	fprintf(Mint_output, "\nCommand line: ") ;
	for (i = 0; i < Argc; i++)
	    fprintf(Mint_output, " %s", Argv[i]) ;
	fprintf(Mint_output, "\n") ;

	strcpy(Start_date, ctime(&Start_time)) ;

/* remove trailing newline */
	Start_date[strlen(Start_date) - 1] = 0 ;
	fprintf(Mint_output, "\nStarted:  %s\n", Start_date) ;
	strcpy(Finish_date, ctime(&Finish_time)) ;

/* remove trailing newline */
	Finish_date[strlen(Finish_date) - 1] = 0 ;
	fprintf(Mint_output, "Finished: %s\n", Finish_date) ;
	elapsed = Finish_time - Start_time ;
	minutes = elapsed / 60 ;
	seconds = elapsed - (minutes * 60) ;
	if (uname(&utsname) < 0)
	    machine_name = "(unknown)" ;
	    else
	    machine_name = utsname.nodename ;
	fprintf(Mint_output, "Elapsed time for simulation: %u:%02u on %s\n",
	    minutes, seconds, machine_name) ;

#ifdef __svr4__
	user_sec = Tms_buffer.tms_utime / CLK_TCK ;
	user_usec = 0 ;
#else
	user_sec = Rusage.ru_utime.tv_sec ;
	user_usec = Rusage.ru_utime.tv_usec ;
#endif
	user_min = user_sec / 60 ;
	user_sec = user_sec - (user_min * 60) ;
	user_fsec = user_sec + (double) user_usec / 1000000.0 ;

#ifdef __svr4__
	sys_sec = Tms_buffer.tms_stime / CLK_TCK ;
	sys_usec = 0 ;
#else
	sys_sec = Rusage.ru_stime.tv_sec ;
	sys_usec = Rusage.ru_stime.tv_usec ;
#endif
	sys_min = sys_sec / 60 ;
	sys_sec = sys_sec - (sys_min * 60) ;
	sys_fsec = sys_sec + (double) sys_usec / 1000000.0 ;

	total_min = user_min + sys_min ;
	total_sec = user_sec + sys_sec ;
	total_usec = user_usec + sys_usec ;
	if (total_usec >= 1000000) {
	    total_sec++ ;
	    total_usec -= 1000000 ;
	}
	if (total_sec >= 60) {
	    total_min++ ;
	    total_sec -= 60 ;
	}
	total_fsec = total_sec + (double) total_usec / 1000000.0 ;
	cpu_seconds = total_min * 60 + total_fsec ;

	fprintf(Mint_output,
	    "CPU time: user: %u:%05.2f, system: %u:%05.2f total: %u:%05.2f (%.2f sec)\n",
	    user_min, user_fsec, sys_min, sys_fsec, total_min, total_fsec,
	    cpu_seconds) ;

	Mem_size_finish = (char *) sbrk(0) ;
	mem_used = Mem_size_finish - Mem_size_start ;
	mem_used = (mem_used + 1023) / 1024 ;
	Total_elapsed_time = Threads[0].time ;
	Total_cpu_time = Threads[0].cpu_time + Threads[0].child_cpu ;

	fprintf(Mint_output, 
	    "Space used by malloc: %dK\n", mem_used) ;

	fprintf(Mint_output, 
	    "\nElapsed simulated cycles: %.0f, cpu cycles: %.0f\n",
	    Total_elapsed_time, Total_cpu_time) ;

	fprintf(Mint_output, 
	    "Processors used = %d, average cpu_time/proc = %.1f\n\n",
	    Maxpid + 1, Total_cpu_time / (Maxpid + 1)) ;

#endif /* COMMENT */

}
/* end subroutine (mint_stats) */


/* parse the command line arguments */
static void parse_args(int argc, char **argv)
{
	char *cp, *tmp ;
	int c, errflag ;
	extern char *optarg ;
	extern int optind ;


/* set up the default values */
	Stack_size = STACK_SIZE ;
	Heap_size = HEAP_SIZE ;
	Shmem_size = SHMEM_SIZE ;
	Ckpoint_freq = DEFAULT_CKPOINT_FREQ ;
	Lockpage_size = LOCKPAGE_SIZE ;
	Lockblk_size = LOCKBLK_SIZE ;
	Interactive = 0 ;
	Mint_debug = 0 ;
	Verify_protocol = 0 ;
	Optimization_level = 1 ;
	Every_write_ll = 0 ;
	Trace_instr = 0 ;
	Trace_refs = 1 ;
	Trace_none = 0 ;
	Trace_sync = 0 ;
	Trace_shared = 0 ;
	Locks_block = 1 ;
	HwInterlock = 0 ;
	Recycle_threads = 1 ;
	Release_consist = 0 ;
	Max_nprocs = DECF_MAX_NPROCS ;

	errflag = 0 ;

	while ((c = getopt(argc,argv,
	    "C:c:De:h:IK:k:l:m:O:P:p:qrRSs:t:u:VvW")) != -1) {

	    switch (c) {

	    case 'C':
	        Ckpoint_freq = strtol(optarg, NULL, 0) ;
	        break ;
	    case 'D':
	        Mint_debug = 1 ;
	        break ;
	    case 'c':
	        Fcycles = optarg ;
	        break ;
	    case 'e':
	        Fevents = optarg ;
	        break ;
	    case 'h':
	        Heap_size = strtol(optarg, NULL, 0) ;
	        break ;
	    case 'I':
/* HwInterlock = 1; */	/* not supported yet */
	        break ;
	    case 'K':
	        Keyfile = optarg ;
	        break ;
	    case 'k':
	        Stack_size = strtol(optarg, NULL, 0) ;
	        break ;
	    case 'l':
	        cp = optarg ;
/* Parse the rest painfully since getopt() doesn't grok strings.
			             * Example input:
			             *        -l page=0x400,blk=32
			             * There must not be any spaces in the argument to "-l",
			             * or else getopt() will think it is a filename or a bad option.
			             * The comma separated options may appear in any order.
			             */
	        while (*cp) {
	            if (!strncmp(cp,"page",4)) {
	                cp += 5 ;
	                Lockpage_size = strtol(cp, &tmp, 0) ;
	                cp = tmp ;
	            } else if (!strncmp(cp,"blk",3)) {
	                cp += 4 ;
	                Lockblk_size = strtol(cp, &tmp, 0) ;
	                cp = tmp ;
	            } else {
	                error("Unknown option (-l `%s')\n", cp) ;
	                errflag = 1 ;
	                break ;
	            }
	            if (*cp == ',')
	                cp++ ;
	        }
	        if (Lockpage_size < 0)
	            Lockpage_size = 0 ;
	        if (Lockblk_size < 0)
	            Lockblk_size = 0 ;

/* round up to next power of two */
	        if (Lockpage_size)
	            base2roundup(&Lockpage_size) ;
	        if (Lockblk_size)
	            base2roundup(&Lockblk_size) ;

/* make page size as least as large as the block size */
	        if (Lockpage_size && Lockpage_size < Lockblk_size)
	            Lockpage_size = Lockblk_size ;
	        break ;
	    case 'm':
	        Mem_size = strtol(optarg, NULL, 0) ;
	        Mem_size_specified = 1 ;
	        break ;
	    case 'O':
	        Optimization_level = strtol(optarg, NULL, 0) ;
	        break ;
#ifndef NO_SOCKETS
	    case 'P':
	        Port = strtol(optarg, NULL, 0) ;
	        break ;
#endif
	    case 'p':
	        Max_nprocs = strtol(optarg, NULL, 0) ;
	        if (Max_nprocs < 1 || Max_nprocs > MAXPROC) {
	            errflag = 1 ;
	            fprintf(stderr, 
	                "Number of processes must be in the range: 1 to %d\n",
	                MAXPROC) ;
	        }
	        break ;
	    case 'q':
	        Quiet = 1 ;
	        break ;
	    case 'R':
	        Recycle_threads = 0 ;
	        break ;
	    case 'r':
	        Release_consist = 1 ;
	        break ;
	    case 'S':
	        Locks_block = 0 ;
	        break ;
	    case 's':
	        Shmem_size = strtol(optarg, NULL, 0) ;
	        break ;
	    case 't':
	        switch(*optarg) {
	        case 'i':
	            Trace_instr = 1 ;
	            Trace_sync = 0 ;
	            Trace_none = 0 ;
	            Trace_refs = 0 ;
	            Trace_shared = 0 ;
	            break ;
	        case 'l':
	            Trace_sync = 1 ;
	            Trace_instr = 0 ;
	            Trace_none = 0 ;
	            Trace_refs = 0 ;
	            Trace_shared = 0 ;
	            break ;
	        case 'n':
	            Trace_none = 1 ;
	            Trace_instr = 0 ;
	            Trace_sync = 0 ;
	            Trace_refs = 0 ;
	            Trace_shared = 0 ;
	            break ;
	        case 'r':
	            Trace_refs = 1 ;
	            Trace_instr = 0 ;
	            Trace_sync = 0 ;
	            Trace_none = 0 ;
	            Trace_shared = 0 ;
	            break ;
	        case 's':
	            Trace_shared = 1 ;
	            Trace_instr = 0 ;
	            Trace_sync = 0 ;
	            Trace_none = 0 ;
	            Trace_refs = 0 ;
	            break ;
	        }
	        break ;
	    case 'u':
	        Upshot_log = optarg ;
	        break ;
	    case 'V':
	        Verify_protocol = 1 ;
	        break ;
	    case 'W':
	        Every_write_ll = 1 ;
	        break ;
	    case 'v':
	        printf("mint version %s\n", MINT_VERSION) ;
	        exit(0) ;
	        break ;
	    default:
	        errflag = 1 ;
	        break ;

	    } /* end switch */

	} /* end while (processing arguments) */

	if (optind >= argc && Fevents == NULL)
	    errflag = 1 ;

	Trace_any = Trace_instr | Trace_refs | Trace_shared ;

	if (Verify_protocol && !Trace_any) {
	    printf("The verify option requires a trace option.\n") ;
	    errflag = 1 ;
	}

	if (errflag) {
	    mint_usage() ;
	    sim_usage(argc, argv) ;
	    exit(1) ;
	}

}
/* end subroutine (parse_args) */


static void read_cycles(char *fname)
{
	int i ;
	FILE *fd ;
	char opname[80] ;
	int cycles ;
	int errflag ;
	int lineno ;

/* open the file */
	fd = fopen(fname, "r") ;
	if (fd == NULL) {
	    perror(fname) ;
	    exit(1) ;
	}

/* read the file, each line contains a "name number" pair */
	errflag = 0 ;
	lineno = 0 ;
	while (fscanf(fd, "%s %d", opname, &cycles) == 2) {
	    lineno++ ;
/* Search for name in function list. The list is NULL-terminated. */
	    for (i = 0; i < Nfuncs - 1; i++)
	        if (strcmp(Func_subst[i].name, opname) == 0)
	            break ;
/* found it in the list of functions */
	    if (i < Nfuncs - 1) {
	        Func_subst[i].cycles = cycles ;
	        continue ;
	    }

/* Not found yet, search the NULL-terminated list of function pointers */
	    for (i = 0; i < Nfptr - 1; i++)
	        if (strcmp(Data_subst[i].aname, opname) == 0)
	            break ;
/* found it in the list of function pointers */
	    if (i < Nfptr - 1) {
	        Data_subst[i].cycles = cycles ;
	        continue ;
	    }

/* Not found yet, search the list of opcode names */
	    for (i = 0; i < max_opnum; i++)
	        if (strcmp(desc_table[i].opname, opname) == 0)
	            break ;
/* found it in the list of opcodes */
	    if (i < max_opnum) {
	        desc_table[i].cycles = cycles ;
	        continue ;
	    }

	    printf("Operation `%s' unknown (line %d)\n", opname, lineno) ;
	    errflag = 1 ;
	}
	fclose(fd) ;
	if (errflag)
	    exit(1) ;
}
/* end subroutine (read_cycles) */


/* copy application args onto the simulated stack for the main process */
static void
copy_argv(int arg_start, int argc, char **argv, char **envp)
{
	int i, size, nenv ;
	long *sp ;
	int argc_obj ;
	char **argv_obj, **eptr, **envp_obj ;
	thread_ptr pthread ;
	int sizeofptr ;

	pthread = &Threads[0] ;
	sizeofptr = sizeof(char *) ;


/* add up sizes of all the arguments */
	size = 0 ;
	for (i = arg_start; i < argc; i++)
	    size += strlen(argv[i]) + 1 ;

/* Add in the sizes of all the environment variables, and count
	     * the number of environment variable pointers that we need.
	     */
	nenv = 0 ;
	for (eptr = envp; *eptr; eptr++) {
	    size += strlen(*eptr) + 1 ;
	    nenv++ ;
	}

/* get the number of arguments to the simulated object program */
	argc_obj = argc - arg_start ;

/* add in space for the argv and envp pointers, including
	     * two NULL pointers
	     */
	size += (argc_obj + nenv + 2) * sizeofptr ;

/* add in space for argc */
	size += sizeof(int) ;

/* round the size up to a double word boundary */
	size = (size + 7) & ~0x7 ;

/* allocate stack space for all the args */
	pthread->reg[29] -= size ;

/* get the real address */
	if (Shared_stacks)
	    sp = (long *) pthread->reg[29] ;
	    else
	    sp = (long *) MAP(pthread->reg[29]) ;

/* the first stack item is the number of args (argc) */
	*sp++ = argc_obj ;

/* the next stack items are the array of pointers argv */
	argv_obj = (char **) sp ;

/* leave space for the argv array, including the NULL pointer */
	sp += argc_obj + 1 ;

/* next come the pointers for the environment variables */
	envp_obj = (char **) sp ;
	sp += nenv + 1 ;

/* copy the args to the stack of the main thread */
	for (i = arg_start; i < argc; i++) {
	    strcpy((char *) sp, argv[i]) ;
	    if (Shared_stacks)
	        argv_obj[i - arg_start] = (char *) sp ;
	        else
	        argv_obj[i - arg_start] = (char *) PUNMAP((long)sp) ;
	    sp = (long *)((long) sp + strlen(argv[i]) + 1) ;
	}
	argv_obj[argc - arg_start] = NULL ;

/* copy the environment variables to the stack of the main thread */
	for (i = 0, eptr = envp; i < nenv; i++, eptr++) {
	    strcpy((char *) sp, *eptr) ;
	    if (Shared_stacks)
	        envp_obj[i] = (char *) sp ;
	        else
	        envp_obj[i] = (char *) PUNMAP((long)sp) ;
	    sp = (long *)((long) sp + strlen(*eptr) + 1) ;
	}
	envp_obj[nenv] = NULL ;
}
/* end subroutine (copy_argv) */


/*
 * logbase2() returns the log (base 2) of its argument, rounded up.
 * It also rounds up its argument to the next higher power of 2.
 */
static int
logbase2(long *pnum)
{
	int logsize, exp ;


	for (logsize = 0, exp = 1; exp < *pnum; logsize++)
	    exp *= 2 ;

/* round pnum up to nearest power of 2 */
	*pnum = exp ;

	return logsize ;
}


void
allocate_fixed(long addr, long nbytes)
{
	int id, mem_so_far ;
	char *real_addr, *shaddr ;
	long size2 ;


/* Use shared memory segments to allocate memory at a specific
	     * address. The mmap() system call would be more appropriate here,
	     * but the DECstation doesn't like it.
	     */

/* round nbytes up to the next power of 2 */
	size2 = nbytes ;
	logbase2(&size2) ;

/* Try to allocate the whole shared region in one piece. If that fails,
	     * try successively smaller sizes by dividing by 2 each time.
	     */
	for ( ; size2; size2 >>= 1) {
	    id = shmget(IPC_PRIVATE, size2, 0777) ;
	    if (id != -1)
	        break ;
	}
	if (id == -1) {
	    perror("shmget") ;
	    fprintf(stderr, "allocate_fixed: cannot allocate 0x%x bytes.\n",
	        nbytes) ;
	    fprintf(stderr, 
	        "Try using fewer processes or allocating less memory per process.\n") ;
	    fprintf(stderr, "(Use the options: -h, -p, or -s.)\n") ;
	    mint_usage() ;
	    exit(1) ;
	}

/*printf("allocate_fixed: nbytes = 0x%x, size2 = 0x%x\n", nbytes, size2);*/

/* map in the first piece */
	shaddr = (char *) addr ;
	real_addr = (char *) shmat(id, shaddr, 0) ;
	if (real_addr == (char *) -1 || real_addr != shaddr) {
	    shmctl(id, IPC_RMID, NULL) ;
	    perror("shmat") ;
	    fprintf(stderr, 
	        "allocate_fixed: cannot attach shared memory segment (%d bytes).\n",
	        size2) ;
	    fprintf(stderr, 
	        "Try using fewer processes or allocating less memory per process.\n") ;
	    fprintf(stderr, "(Use the options: -h, -p, or -s.)\n") ;
	    mint_usage() ;
	    exit(1) ;
	}

/* Delete the segment now so that we don't need to worry about
	     * cleaning up later.
	     */
	shmctl(id, IPC_RMID, NULL) ;

/* map all the rest of the pieces contiguously */
	for (mem_so_far = size2; mem_so_far < nbytes; mem_so_far += size2) {
	    id = shmget(IPC_PRIVATE, size2, 0777) ;
	    if (id == -1) {
	        perror("shmget") ;
	        fprintf(stderr, 
	            "allocate_fixed: can only allocate 0x%x out of 0x%x bytes.\n",
	            mem_so_far, nbytes) ;
	        fprintf(stderr, 
	            "Try using fewer processes or allocating less memory per process.\n") ;
	        fprintf(stderr, "(Use the options: -h, -p, or -s.)\n") ;
	    mint_usage() ;
	        exit(1) ;
	    }
	    shaddr += size2 ;
	    real_addr = (char *) shmat(id, shaddr, 0) ;
	    if (real_addr == (char *) -1 || real_addr != shaddr) {
	        perror("shmat") ;
	        fprintf(stderr, 
	            "allocate_fixed: cannot attach shared memory "
	            "segment (size = %d bytes, %d so far.\n",
	            size2, mem_so_far) ;
	        fprintf(stderr, 
	            "Try using fewer processes or allocating less "
	            "memory per process.\n") ;
	        fprintf(stderr, "(Use the options: -h, -p, or -s.)\n") ;
	        mint_usage() ;
	        shmctl(id, IPC_RMID, NULL) ;
	        exit(1) ;
	    }

/* Delete the segment now so that we don't need to worry about
		         * cleaning up later.
		         */
	    shmctl(id, IPC_RMID, NULL) ;
	}
}
/* end subroutine (allocate_fixed) */


/****

 Allocate memory for the shared memory region and all the per-process
 * private memory regions and initialize the address space for the main
 * process.

****/

static void
create_addr_space()
{
	int i, size, multiples ;
	int dwords ;
	long *addr ;
	thread_ptr pthread ;
	unsigned long min_seek, heap_start, heap_size, total_size ;


/* this assumes that the bss is located higher in memory than the data */
	DB_size = Bss_start + Bss_size - Data_start ;

	if (Mem_size > 0 && Mem_size < DB_size + Stack_size + Heap_size) {

	    fprintf(stderr,
	        "The memory size you specified, %d (0x%x), is "
	        "not large enough.\n",
	        Mem_size, Mem_size) ;

	    fprintf(stderr,
	        "Data + BSS = %d (0x%x), heap = %d (0x%x), and "
	        "stack = %d (0x%x)\n",
	        DB_size, DB_size,
	        Heap_size, Heap_size,
	        Stack_size, Stack_size) ;

	    total_size = DB_size + Heap_size + Stack_size ;
	    fprintf(stderr, "Total size in bytes: %d (0x%x)\n",
	        total_size, total_size) ;

	    fprintf(stderr, 
	        "\nSpecifying the memory size with \"-m\" is no longer necessary,\n") ;
	    fprintf(stderr, 
	        "unless you need to align the per-processor private memory regions.\n") ;
	    fprintf(stderr, 
	        "Try leaving out the \"-m\" option, or increase the size.\n") ;
	    mint_usage() ;
	    exit(1) ;

	} /* end if */

/* if the memory size was not specified, then allocate just enough */
	if (Mem_size == 0)
	    Mem_size = DB_size + Heap_size + Stack_size ;

/* round Mem_size up to the next larger page size */
	Mem_size = (Mem_size + M_ALIGN - 1) & ~(M_ALIGN - 1) ;

	pthread = &Threads[0] ;

/* Allocate all the private memory regions up front.
	     * Assign each region to a fixed location so that different
	     * simulation runs all use the same starting address for private
	     * memory.
	     */
	Private_start = PRIVATE_START ;
	size = Max_nprocs * Mem_size ;

	if (Rdata_start < Data_start) {

/****

	In addition to the per-processor copies of the data and bss, there
	is one copy of the rdata.  If the memory alignment was specified
	with the "-m" option, then round Rdata_size up to the next
	multiple of "Mem_size" so that the per-processor private regions
	start on a "Mem_size" boundary.
		         
****/

	    if (Mem_size_specified > 0) {
	        multiples = (Rdata_size + Mem_size - 1) / Mem_size ;
	        Rsize_round = multiples * Mem_size ;
	    } else
	        Rsize_round = Rdata_size ;
	    size += Rsize_round ;
	}

	allocate_fixed(Private_start, size) ;

	pthread->addr_space = Private_start ;
	Data_end = Data_start + Mem_size ;
	Rdata_end = Rdata_start + Rdata_size ;

/* allocate space for the PRDA (per region data area) */
	size = Max_nprocs * PRDA_SIZE ;
	allocate_fixed(PRDA_START, size) ;
	pthread->prda_space = PRDA_START ;
	pthread->prda_map = PRDA_START - PRDA_NATIVE ;
	*(long *) (pthread->prda_space + PRDA_PID_OFFSET) = 0 ;

/* allocate the shared memory region */
	Shmem_start = SHARED_START ;
	Shmem_end = Shmem_start ;
	if (Shmem_size)
	    allocate_fixed(Shmem_start, Shmem_size) ;
	if (Shared_stacks) {
	    if (Max_nprocs * Stack_size > Shmem_size) {
	        fprintf(stderr, 
	            "Not enough shared memory (0x%x) for stacks\n",
	            Shmem_size) ;
	        fprintf(stderr, 
	            "Try increasing shared memory using the \"-s\" option.\n") ;
	    mint_usage() ;
	        exit(1) ;
	    }
	    Shmem_end += Max_nprocs * Stack_size ;
	}
/* allocate a page for locks if necessary */
	if (Shmem_size && Lockpage_size) {
	    Lkmem_start = Shmem_end ;
	    Lkmem_end = Lkmem_start ;
	    Shmem_end += Lockpage_size ;
	} else {
/* initialize the variables so that it appears the space is used up */
	    Lkmem_start = 0 ;
	    Lkmem_end = Lockpage_size ;
	}

	addr = (long *) pthread->addr_space ;

	if (Rdata_start < Data_start) {

/****

	Read in the .rdata section first since the .rdata section is not
	contiguous with the rest of the data in the address space or object
	file.

****/

	    if (Rdata_seek != 0 && Rdata_size > 0) {
	        fseek(Fobj, Rdata_seek, SEEK_SET) ;

/* read in the .rdata section from the object file */

	        if (fread(addr, sizeof(char), Rdata_size, Fobj) < Rdata_size)
	            fatal("create_addr_space: EOF reading rdata section\n") ;

/* compute the memory mapping offset for the .rdata section */

	        pthread->rdata_map = Private_start - Rdata_start ;

/* move "addr" to prepare for reading in the data and bss */

	        addr = (long *) (Private_start + Rsize_round) ;
	        pthread->addr_space = (long) addr ;

	    } else {

/* If there was no .rdata section, then just map it to the
			             * start of the data section.
			             */

	        pthread->rdata_map = Private_start - Data_start ;
	    }
	}

/* Figure out which data section comes first. The SGI COFF files have
	     * .data first. DECstations have .rdata first except sometimes the
	     * .rdata section is before the .text section.
	     */

	min_seek = ULONG_MAX ;
	if (Rdata_start >= Data_start && Rdata_seek != 0)
	    min_seek = Rdata_seek ;
	if (Data_seek != 0 && Data_seek < min_seek)
	    min_seek = Data_seek ;
	if (Sdata_seek != 0 && Sdata_seek < min_seek)
	    min_seek = Sdata_seek ;
	if (min_seek == ULONG_MAX)
	    fatal("create_addr_space: no .rdata or .data section\n") ;

	fseek(Fobj, min_seek, SEEK_SET) ;
	dwords = Data_size / 4 ;

/* read in the initialized data from the object file */
	if (fread(addr, sizeof(long), dwords, Fobj) < dwords)
	    fatal("create_addr_space: end-of-file reading data section\n") ;

/* zero out the bss section */
	addr = (long *) (pthread->addr_space + Bss_start - Data_start) ;
	dwords = Bss_size / 4 ;
	for (i = 0; i < dwords; i++)
	    *addr++ = 0 ;

/* compute the memory mapping offset */
	pthread->mem_map = pthread->addr_space - Data_start ;

	heap_start = pthread->addr_space + DB_size ;
	heap_size = Mem_size - DB_size - Stack_size ;
	if (heap_size < HEAP_SIZE_MIN) {
	    fprintf(stderr, "Not enough memory for private malloc: %d (0x%x)\n",
	        heap_size) ;
	    fprintf(stderr, "Try increasing it using the \"-h\" option.\n") ;
	    mint_usage() ;
	    exit(1) ;
	}
/* initialize the private memory allocator for this thread */
	malloc_init(pthread, heap_start, heap_size) ;

/* point the sp to the top of the allocated space */
/* (The stack grows down toward lower memory addresses.) */

	if (Shared_stacks) {

	    pthread->reg[29] = Shmem_start + Stack_size ;

/****

	We don't need to change the mapping since shared memory is
	mapped directly, but include this anyway in case someday
	we change the mapping.

****/

	    pthread->reg[29] = SUNMAP(pthread->reg[29]) ;
	} else {
	    pthread->reg[29] = pthread->addr_space + Mem_size ;

/* Change the sp so that mapping will work for sp-relative addresses. */
	    pthread->reg[29] = PUNMAP(pthread->reg[29]) ;
	}
	pthread->stacktop = pthread->reg[29] - Stack_size ;

}
/* end subroutine (create_addr_space) */


/* Copy the parent's address space to the child's. This also sets up
 * all the mapping fields in the child's thread structure.
 */
void
copy_addr_space(thread_ptr parent, thread_ptr child)
{
	int i, offset, pid ;
	long *dest, *src, *last ;
	long parent_sp ;
	unsigned int maxdepth ;


/* The address space has already been allocated. */
	pid = child->pid ;
	child->addr_space = Private_start + Mem_size * pid ;
	child->addr_space += Rsize_round ;
	offset = child->addr_space - parent->addr_space ;
	child->mem_map = child->addr_space - Data_start ;
	child->rdata_map = parent->rdata_map ;

	child->prda_space = PRDA_START + PRDA_SIZE * pid ;
	child->prda_map = child->prda_space - PRDA_NATIVE ;
	*(long *) (child->prda_space + PRDA_PID_OFFSET) = pid ;

/* Initialize the private memory allocator for the child,
	     * copying the parent's allocated blocks to the child.
	     */
	malloc_copy(child, parent) ;

	src = (long *) parent->addr_space ;
	dest = (long *) child->addr_space ;
	last = (long *) (parent->addr_space + DB_size) ;

/* copy the data and bss from parent to child */
	while (src < last)
	    *dest++ = *src++ ;

/* copy the stack */
	if (Shared_stacks) {
	    parent_sp = parent->reg[29] ;
	    if (parent_sp < Shmem_start + parent->pid * Stack_size
	        || parent_sp >= Shmem_start + (parent->pid + 1) * Stack_size)
	        fatal("copy_addr_space: parent's stack pointer out of range (0x%x)\n",
	            parent_sp) ;
	    dest = (long *) (parent_sp + (pid - parent->pid) * Stack_size) ;
	    last = (long *) (Shmem_start + (parent->pid + 1) * Stack_size) ;
	} else {
	    parent_sp = parent->reg[29] + parent->mem_map ;
	    if (parent_sp < Data_end + parent->mem_map - Stack_size
	        || parent_sp >= Data_end + parent->mem_map)
	        fatal("copy_addr_space: parent's stack pointer out of range (0x%x)\n",
	            parent_sp) ;
	    dest = (long *) (parent_sp + offset) ;
	    last = (long *) (parent->addr_space + Mem_size) ;
	}
	src = (long *) parent_sp ;
	while (src < last)
	    *dest++ = *src++ ;

/* copy all the registers */
	for (i = 0; i < 32; i++)
	    child->reg[i] = parent->reg[i] ;
	child->lo = parent->lo ;
	child->hi = parent->hi ;
	for (i = 0; i < 32; i++)
	    child->fp[i] = parent->fp[i] ;
	child->fcr0 = parent->fcr0 ;
	child->fcr31 = parent->fcr31 ;

/* copy the call chain */
	maxdepth = parent->calldepth ;
	if (maxdepth > MAX_CALLS)
	    maxdepth = MAX_CALLS ;
	for (i = 0; i < maxdepth; i++)
	    child->calls[i] = parent->calls[i] ;
	child->calldepth = maxdepth ;

	if (Shared_stacks) {
	    child->reg[29] = parent->reg[29] + (pid - parent->pid) * Stack_size ;
	    child->stacktop = Shmem_start + pid * Stack_size ;
	} else {
/* The child's stacktop looks the same as the parent's, but it
		         * is really different since the stacktop, like the registers,
		         * represents an address in the object's virtual space.
		         */
	    child->stacktop = parent->stacktop ;
	}

}
/* end subroutine (copy_addr_space) */


/* 
Share the parent's address space with the child. This is used to
 * support the sproc() system call on the SGI. This also sets up
 * all the mapping fields in the child's thread structure.
 */

void share_addr_space(thread_ptr parent, thread_ptr child, 
int share_all, int copy_stack)
{
	int i, pid ;
	long *dest, *src, *last ;
	long parent_sp ;
	unsigned int maxdepth ;
	long addr_space ;

/* The address space has already been allocated. */
	pid = child->pid ;
	child->rdata_map = parent->rdata_map ;

/* Even if everything else is shared, the PRDA is still private */
	child->prda_space = PRDA_START + PRDA_SIZE * pid ;
	child->prda_map = child->prda_space - PRDA_NATIVE ;
	*(long *) (child->prda_space + PRDA_PID_OFFSET) = pid ;

	if (share_all) {
	    child->mem_map = parent->mem_map ;
	    child->addr_space = parent->addr_space ;
	    malloc_share(child, parent) ;
	} else {
	    addr_space = Private_start + Mem_size * pid ;
	    addr_space += Rsize_round ;
	    child->mem_map = addr_space - Data_start ;
	    child->addr_space = addr_space ;

/* Initialize the private memory allocator for the child,
		         * copying the parent's allocated blocks to the child.
		         */
	    malloc_copy(child, parent) ;

	    src = (long *) parent->addr_space ;
	    dest = (long *) child->addr_space ;
	    last = (long *) (parent->addr_space + DB_size) ;

/* copy the data and bss from parent to child */
	    while (src < last)
	        *dest++ = *src++ ;
	}

/* copy all the registers */
	for (i = 0; i < 32; i++)
	    child->reg[i] = parent->reg[i] ;
	child->lo = parent->lo ;
	child->hi = parent->hi ;
	for (i = 0; i < 32; i++)
	    child->fp[i] = parent->fp[i] ;
	child->fcr0 = parent->fcr0 ;
	child->fcr31 = parent->fcr31 ;

	if (!Shared_stacks)
	    fatal("share_addr_space: stacks not shared.\n") ;

	if (copy_stack) {
/* copy the stack */
	    parent_sp = parent->reg[29] ;
	    if (parent_sp < Shmem_start + parent->pid * Stack_size
	        || parent_sp >= Shmem_start + (parent->pid + 1) * Stack_size)
	        fatal("copy_addr_space: parent's stack pointer out of range (0x%x)\n",
	            parent_sp) ;
	    dest = (long *) (parent_sp + (pid - parent->pid) * Stack_size) ;
	    last = (long *) (Shmem_start + (parent->pid + 1) * Stack_size) ;
	    src = (long *) parent_sp ;
	    while (src < last)
	        *dest++ = *src++ ;

/* change the stack pointer to point into the child's stack copy */
	    child->reg[29] = parent->reg[29] + (pid - parent->pid) * Stack_size ;
	    child->stacktop = Shmem_start + pid * Stack_size ;

/* copy the call chain */
	    maxdepth = parent->calldepth ;
	    if (maxdepth > MAX_CALLS)
	        maxdepth = MAX_CALLS ;
	    for (i = 0; i < maxdepth; i++)
	        child->calls[i] = parent->calls[i] ;
	    child->calldepth = maxdepth ;
	} else {
/* change the stack pointer to point to the top of the child's stack */
	    child->reg[29] = Shmem_start + (pid + 1) * Stack_size - FRAME_SIZE ;
	    child->stacktop = Shmem_start + pid * Stack_size ;

/* clear the call chain */
	    child->calls[0] = 0 ;
	    child->calldepth = 0 ;
	}

}
/* end subroutine (share_addr_space) */


/* create a new copy of an icode */
icode_ptr newcopy_icode(icode_ptr picode)
{
	int i ;
	icode_ptr inew ;


/* 
reduce calls to malloc and make more efficient use of space
by allocating several icodes at once
*/

	if (Nicode == 0) {

	    Icode_free = (icode_ptr) malloc(NICODE * sizeof(struct icode)) ;
	    if (Icode_free == NULL)
	        fatal("newcopy_icode: out of memory\n") ;
	    Nicode = NICODE ;
	}
	inew = &Icode_free[--Nicode] ;
	inew->func = picode->func ;
	inew->addr = picode->addr ;
	inew->instr = picode->instr ;
	for (i = 0; i < 4; i++)
	    inew->args[i] = picode->args[i] ;
	inew->immed = picode->immed ;
	inew->cycles = picode->cycles ;
	inew->next = picode->next ;
	inew->target = picode->target ;
	inew->not_taken = picode->not_taken ;
	inew->is_target = picode->is_target ;
	inew->opnum = picode->opnum ;
	inew->opflags = picode->opflags ;

	return inew ;
}
/* end subroutine (newcopy_icode) */


/* 
Reads the text section of the object file and creates the linked list
of icode structures.
*/

static void read_text()
{
	int i, make_copy, voffset, wroffset, err, num_pointers ;
	long instr, opflags, iflags ;
	icode_ptr picode, prev_picode, dslot, pcopy, *pitext, pcopy2 ;
	struct op_desc *pdesc ;
	unsigned int addr ;
	int opnum ;
	int regfield[4], target ;
	int prev_was_branch ;
	signed short immed;	/* immed must be a signed short */
	PFPI simfunc ;
	PFPI *pfunc ;
#ifdef DEBUG_READ_TEXT
	static int Debug_addr = 0 ;
#endif

/* 
Allocate space for pointers to icode, plus the SGI function pointers
for the _lock routines, plus some for special function
pointers that have no other place to go, plus 1 for a NULL pointer.
Text_size is the number of instructions.
*/

	num_pointers = Text_size + Nfptr + EXTRA_ICODES ;
	Itext = (icode_ptr *) malloc((num_pointers + 1) * sizeof(icode_ptr)) ;
	if (Itext == NULL)
	    fatal("read_text: cannot allocate 0x%x bytes for Itext.\n",
	        (num_pointers + 1) * sizeof(icode_ptr)) ;

/* Allocate space for the icode structures */

	picode = (icode_ptr) malloc(num_pointers * sizeof(struct icode)) ;
	if (picode == NULL)
	    fatal("read_text: cannot allocate 0x%x bytes for icode structs.\n",
	        num_pointers * sizeof(struct icode)) ;

/* 
Assign each pointer to its corresponding icode, and link each
icode to point to the next one in the array.
*/

	pitext = Itext ;
	for (i = 0; i < num_pointers; i++) {
	    *pitext++ = picode ;
	    picode->next = picode + 1 ;
	    picode++ ;
	}
	*pitext = NULL ;

	fseek(Fobj, Text_seek, SEEK_SET) ;
/*    ldnsseek(Ldptr, ".text"); */

#ifdef notdef
	for (i = 0; i < max_opnum; i++)
	    printf("%d %s\n", i, desc_table[i]) ;
#endif

/* loop performing the main function of reading text */

	prev_was_branch = 0 ;
	prev_picode = NULL ;
	picode = Itext[0] ;
	addr = Text_start ;
	for (i = 0; i < Text_size; i++, addr += 4, picode++) {

	    err = fread(&instr, sizeof(long), 1, Fobj) ;
	    if (err < 1)
	        fatal("read_text: end-of-file reading text section\n") ;

	    if (Swap_bytes)
	        instr = SWAP_WORD(instr) ;

	    picode->addr = addr ;
	    regfield[RS] = (instr >> 21) & 0x1f ;
	    regfield[RT] = (instr >> 16) & 0x1f ;
	    regfield[RD] = (instr >> 11) & 0x1f ;
	    regfield[SA] = (instr >> 6) & 0x1f ;
	    immed = instr & 0xffff ;

	    opnum = decode_instr(picode, instr) ;
	    pdesc = &desc_table[opnum] ;
	    opflags = pdesc->opflags ;
	    iflags = pdesc->iflags ;

	    picode->opflags = opflags ;
	    pfunc = pdesc->func ;

/* replace instructions that use r0 with faster equivalent ones */

	    switch(opnum) {

	    case beq_opn:
	        if (picode->args[RT] == 0)
	            if (picode->args[RS] == 0) {
	                pfunc = b_op ;
	                opnum = b_opn ;

	            } else
	                pfunc = beq0_op ;
	        break ;
	    case bne_opn:
	        if (picode->args[RT] == 0)
	            pfunc = bne0_op ;
	        break ;
	    case addiu_opn:
	        if (picode->args[RS] == 0) {
	            pfunc = li_op ;
	            opnum = li_opn ;
	        } else if (picode->args[RS] == picode->args[RT]) {
	            pfunc = addiu_xx_op ;
	        }
	        break ;
	    case addu_opn:
	        if (picode->args[RT] == 0)
	            if (picode->args[RS] == 0) {
	                pfunc = move0_op ;
	                opnum = move_opn ;

	            } else {
	                pfunc = move_op ;
	                opnum = move_opn ;
	            }
	        break ;
	    }
	    picode->opnum = opnum ;
	    picode->cycles = desc_table[opnum].cycles ;

/* 
Change instructions that modify the stack pointer to
include an overflow check. A stack underflow cannot happen.
*/

	    if (opnum == addiu_opn && regfield[RT] == 29) {

	        if (immed < 0) {

/* zero the opnum so that the optimizer won't inline this instruction */

	            picode->opnum = 0 ;
	            pfunc = sp_over_op ;
	        }
	    }

/* Precompute the branch and jump targets */

	    if (iflags & IS_BRANCH) {

	        if (immed == -1)
	            warning("branch to itself at addr 0x%x -- infinite "
			"loop if executed.\n",
	                picode->addr) ;

	        picode->target = picode + immed + 1 ;
	        picode->target->is_target = 1 ;

	    } else if (iflags & IS_JUMP) {

/* for jump instrns, the target address is: */

	        target = 
		    ((instr & 0x03ffffff) << 2) | ((addr + 4) & 0xf0000000) ;

/* target == 0 if it has not been relocated yet */

	        if (target > 0) {

/* 
if the target address is out of range, then this
* is probably a jump to a shared library function.
*/

	            if ((unsigned) target > Text_start + 4 * (Text_size + 10)) {

	                fatal("target address (0x%x) of jump instruction "
				"at addr 0x%x is past end of text.\n",
	                    target, picode->addr) ;
	            }
	            picode->target = T2I(target) ;
	            picode->target->is_target = 1 ;
	        }

	    } else if (opnum == lui_opn)
	        picode->target = (icode_ptr) (immed << 16) ;

/* precompute the shift, use the target field */

	    picode->not_taken = picode + 2 ;

/****

Compute the offset into the function tables for
* memory reference events. This computation is intimately
* connected with the m4 macros that generate the function
* tables. Every function to simulate an instruction
* comes in 2 versions: one normal, and one for branch delay
* slots. Memory reference instructions have 4 additional
* versions of the functions, depending on whether protocol
* verification is being used. The address mapping is
* computed in the event generating function that precedes
* the execution of the load or store.
*     The memory reference instructions are organized
* into a table of 6 entries.
*
* Each pair in the following list represents the indices for
* the normal and branch delay slot versions, respectively.
*
*   Index  Description
*   0,1    Trace_none or not a memory reference
*   2,3    No verify (perform load or store)
*   4,5    Verify (use value from simulator for loads)
*
* In addition, for instructions that write memory there
* are two versions of the above set of functions: one
* version checks, on EVERY WRITE, the address being written
* against a list of addresses that have pending ll (load-linked)
* instructions. The second version checks the address only
* when a sc (store-conditional) instruction is executed.
* The second version is used by default since it is more
* efficient and correct programs should not use normal
* load and store instructions on the same data on which
* they use ll and sc.
*
* The versions of the functions that check an address on
* on every write instruction (to see if it conflicts with a
* pending load-linked instruction) are stored following the
* versions that check only on a store-conditional instruction.
*
*   Index  Description
*   6,7    Check addr on every write; Trace_none
*   8,9    Check addr on every write; no verify
*   10,11  Check addr on every write; verify
*
* The reason for having all these specialized functions is
* to reduce the runtime overhead while simulating. A single
* function could be used instead, but it would have to check
* for all these cases at runtime.

****/

	voffset = 2 ;
	    if (Verify_protocol)
	        voffset += 2 ;
	    wroffset = 0 ;
	    if (Every_write_ll)
	        if (opflags & E_WRITE)
	            wroffset = 6 ;

#ifdef DEBUG_READ_TEXT
	    if (addr == Debug_addr)
	        printf("got here\n") ;
#endif

/* If this is a memory reference instruction, then optimize
		         * for private refs */
	    if ((opflags & E_MEM_REF) && !Trace_none && !Trace_sync) {
	        picode->func = pfunc[voffset + wroffset] ;

/****

	If tracing only shared addresses and protocol verification is
	being used, then we only use the return value from the
	simulator for shared addresses and must perform the load or
	store for private addresses.  This decision must be made at
	run-time for most addresses. So two function pointers need to
	be stored. The otherwise unused fields "target" and "not_taken"
	are used to store these function pointers.

****/

	        if (Trace_shared) {
/* if the address is shared, use the "target" function */
	            picode->target = (icode_ptr) pfunc[voffset + wroffset] ;

/* if the address is private, use the "not_taken" function */
	            picode->not_taken = (icode_ptr) pfunc[wroffset + 2] ;
	        }

	    } else {
/****

Either not tracing any memory references or
not a memory reference; use normal version.

****/
	        picode->func = pfunc[wroffset] ;
	    }

	    dslot = NULL ;
	    if (prev_was_branch) {

/****

	This instruction is in the delay slot of a branch or jump.
	Make a copy of this icode so that jumps to this instruction
	won't use the branch target. This copy is the one that the
	the statically preceding instruction (the branch or jump)
	should execute next.

****/

	        dslot = newcopy_icode(picode) ;

/* link in the new copy to the "next" field of the previous icode */
	        prev_picode->next = dslot ;

/****

	The "next" field of the dslot icode should never be used
	since the current value of pthread->target is used instead.
	So set the "next" field to NULL to catch bugs.

****/
	        dslot->next = NULL ;

	        if ((opflags & E_MEM_REF) && !Trace_none && !Trace_sync) {
	            dslot->func = pfunc[voffset + wroffset + 1] ;

/* See comment above for why we need two function pointers */

	            if (Trace_shared) {

/* if the address is shared, use the "target" function */

	                dslot->target = (icode_ptr) pfunc[voffset + wroffset + 1] ;

/* if the address is private, use the "not_taken" function */

	                dslot->not_taken = (icode_ptr) pfunc[wroffset + 3] ;
	            }

	        } else {
/* Either not tracing any memory references or
				                 * not a memory reference; use branch delay slot version.
				                 */
	            dslot->func = pfunc[wroffset + 1] ;
	        }
	    }

/* ll, sc, and sync instructions call sim_sync() */

	    if (opflags & E_SYNC)
	        simfunc = event_sync ;
	    else if (opflags & E_MEM_REF) {
	        if (Trace_refs || Trace_instr) {

/* use normal version with unknown address */

	            if (opflags & E_READ)
	                simfunc = Use_prmcache_read ? event_prmread : event_read ;
	            else {
	                simfunc = 
			Use_prmcache_write ? event_prmwrite : event_write ;
	            }
	        } else if (Trace_shared) {
	            if (opflags & E_READ)
	                simfunc = event_shr_read ;
	            else {
	                simfunc = event_shr_write ;
	            }
	        }
	    } else
	        simfunc = event_instr ;

	    make_copy = 0 ;
	    if (Trace_instr)
	        make_copy = 1 ;
	    else if (Trace_refs || Trace_shared) {
	        if (opflags & E_MEM_REF)
	            make_copy = 1 ;
	    } else if (Trace_sync) {
	        if (opflags & E_SYNC)
	            make_copy = 1 ;
	    }

	    if (opflags & E_SYNC)
	        make_copy = 1 ;

	    if (make_copy) {

	        pcopy = newcopy_icode(picode) ;
	        picode->next = pcopy ;
	        picode->func = simfunc ;
	        picode->cycles = 0 ;
	        picode->opnum = 0 ;

/****

For "sc" (store conditional) we need to generate either
a write event after the sc completes.

****/

	        if ((opflags & E_SYNC) && (opflags & E_WRITE)) {

/* this is an sc instruction */

	            pcopy2 = newcopy_icode(pcopy) ;
	            pcopy->next = pcopy2 ;

/****

If the sc succeeds, then sim_write() will be called.
If the sc fails, the processor will continue with the
next instruction.

****/

/* just need a dummy event */

	            if (Verify_protocol)
	                pcopy2->func = event_yield ;

	                else
	                pcopy2->func = event_write ;

	            pcopy2->cycles = 0 ;
	            pcopy2->opnum = 0 ;
	        }

	        if (dslot) {

	            pcopy = newcopy_icode(dslot) ;
	            dslot->next = pcopy ;
	            dslot->func = simfunc ;
	            dslot->cycles = 0 ;
	            dslot->opnum = 0 ;

/* see above comment about sc instructions */

	            if ((opflags & E_SYNC) && (opflags & E_WRITE)) {

/* this is an sc instruction */
	                pcopy2 = newcopy_icode(pcopy) ;
	                pcopy->next = pcopy2 ;

/****

If the sc succeeds, then sim_write() will be called.  If the sc fails,
the processor will continue w ith the target of the preceeding branch
or jump.

****/

/* just need a dummy event */

	                if (Verify_protocol)
	                    pcopy2->func = event_yield ;

	                    else
	                    pcopy2->func = event_write ;

	                pcopy2->cycles = 0 ;
	                pcopy2->opnum = 0 ;

/****

The "next" field of pcopy2 will be set to the current value of
pthread->target if the sc suc ceeds.  If the sc fails, the "next" field
is not used.

****/

	                pcopy2->next = NULL ;
	            }
	        }

	    } /* end if */

/****

Set or clear the "prev_was_branch" flag so that the next instruction
knows which version of the function to call.

****/

	    prev_was_branch = iflags & BRANCH_OR_JUMP ;

/* prev_picode is needed only for branch delay slot instructions */
	    prev_picode = picode ;

/****

If we are tracing instructions, then picode points to the event
function instead of the instruction function, so change
prev_picode to point to the instruction function.

****/

	    if (Trace_instr)
	        prev_picode = picode->next ;

	} /* end for (main read loop) */

}
/* end subroutine (read_text) */


/****

 Performs the work for decoding an instruction word, filling in the
 * fields of an icode structure, and replacing instructions that use
 * register r0 (always zero) with simpler, faster equivalent ones.
 * The register indices are pre-shifted here so that register value
 * lookups at execution time will be faster.
 *
 * Returns: opnum, the index into the opcode description table.

****/

int decode_instr(icode_ptr picode, int instr)
{
	int opflags, iflags ;
	struct op_desc *pdesc ;
	int j, opcode, bits31_28, bits20_16, bits5_0, fmt, opnum ;
	int coproc, cofun ;
	int regfield[4] ;
	signed short immed;	/* immed must be a signed short */


	picode->instr = instr ;
	opcode = (instr >> 26) & 0x3f ;
	bits31_28 = (instr >> 28) & 0xf ;
	bits20_16 = (instr >> 16) & 0x1f ;
	bits5_0 = instr & 0x3f ;
	regfield[RS] = (instr >> 21) & 0x1f ;
	regfield[RT] = (instr >> 16) & 0x1f ;
	regfield[RD] = (instr >> 11) & 0x1f ;
	regfield[SA] = (instr >> 6) & 0x1f ;
	immed = instr & 0xffff ;
	picode->immed = immed ;

	if (opcode == 0) {
/* special instruction */
	    opnum = special_opnums[bits5_0] ;
	} else if (opcode == 1) {
/* regimm instruction */
	    opnum = regimm_opnums[bits20_16] ;
	} else if (bits31_28 != 4) {
/* normal operation */
	    opnum = normal_opnums[opcode] ;
	} else {
/* coprocessor instruction */
	    coproc = (instr >> 26) & 0x3 ;

	    fmt = regfield[FMT] ;
	    if ((instr >> 25) & 1) {
/* general coprocessor operation, uses cofun */
	        if (coproc == 1) {
	            cofun = instr & 0x03f ;
	            if (fmt == 16)		/* single precision format */
	                opnum = cop1func_opnums[0][cofun] ;
	            else if (fmt == 17)		/* double precision format */
	                opnum = cop1func_opnums[1][cofun] ;
	            else	/* fixed-point format */
	                opnum = cop1func_opnums[2][cofun] ;

	        } else {
/* coprocessor other than 1 */
	            opnum = normal_opnums[opcode] ;
	        }
	    } else {
	        switch (fmt) {
	        case 0:
/* mfc1, move register from coprocessor */
	            opnum = mfc_opnums[coproc] ;
	            break ;
	        case 2:
/* cfc1, move control from coprocessor */
	            opnum = cfc_opnums[coproc] ;
	            break ;
	        case 4:
/* mtc1, move register to coprocessor */
	            opnum = mtc_opnums[coproc] ;
	            break ;
	        case 6:
/* ctc1, move control to coprocessor */
	            opnum = ctc_opnums[coproc] ;
	            break ;
	        case 8:
/* coprocessor branch */
	            if (regfield[FT] < 4)
	                opnum = bc_opnums[coproc][regfield[FT]] ;
	                else
	                opnum = reserved_opn ;
	            break ;
	        default:
	            opnum = reserved_opn ;
	            break ;
	        }
	    }
	}

	pdesc = &desc_table[opnum] ;
	opflags = pdesc->opflags ;
	iflags = pdesc->iflags ;

/* for coprocessor instructions, the fmt field should not be shifted */
	picode->args[FMT] = regfield[FMT] ;

/* 1. Pre-shift the register indices.
* 2. Modify instructions that write to r0 so that they write
*    to r32 instead (so that r0 remains zero).
* 3. Modify instructions that access the floating point
*    registers so that they use the correct index into the fp[] array.
*    This requires flipping the low order bit.
*/

	for (j = 0; j < 4; j++) {

	    if (pdesc->regflags[j] & MOD0)
	        if (regfield[j] == 0)
	            if (iflags & MOD0_IS_NOP) {

/* replace this instruction with a nop */
	                opnum = nop_opn ;
	            } 
	    else
	                regfield[j] = 32 ;

/* shift the register values so they can be used
* as indices directly */

	    if ((pdesc->regflags[j] & REG0) || (pdesc->regflags[j] & DREG1))
	        picode->args[j] = regfield[j] << 2 ;

	    else if (pdesc->regflags[j] & REG1) {

#ifdef MIPSEL
	        picode->args[j] = regfield[j] << 2 ;
#else

/* flip the low order bit of single precision fp regs */
	        picode->args[j] = (regfield[j] ^ 1) << 2 ;
#endif
	    } else
	        picode->args[j] = regfield[j] ;
	}
	return opnum ;
}
/* end subroutine (decode_instr) */



