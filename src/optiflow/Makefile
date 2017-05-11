# Makefile - simulator suite make file

# SimpleScalar(TM) Tool Suite
# Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
# All Rights Reserved. 
# 
# THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
# YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
# 
# No portion of this work may be used by any commercial entity, or for any
# commercial purpose, without the prior, written permission of SimpleScalar,
# LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
# as described below.
# 
# 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
# or implied. The user of the program accepts full responsibility for the
# application of the program and the use of any results.
# 
# 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
# downloaded, compiled, executed, copied, and modified solely for nonprofit,
# educational, noncommercial research, and noncommercial scholarship
# purposes provided that this notice in its entirety accompanies all copies.
# Copies of the modified software can be delivered to persons who use it
# solely for nonprofit, educational, noncommercial research, and
# noncommercial scholarship purposes provided that this notice in its
# entirety accompanies all copies.
# 
# 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
# PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
# 
# 4. No nonprofit user may place any restrictions on the use of this software,
# including as modified by the user, by any other authorized user.
# 
# 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
# in compiled or executable form as set forth in Section 2, provided that
# either: (A) it is accompanied by the corresponding machine-readable source
# code, or (B) it is accompanied by a written offer, with no time limit, to
# give anyone a machine-readable copy of the corresponding source code in
# return for reimbursement of the cost of distribution. This written offer
# must permit verbatim duplication by anyone, or (C) it is distributed by
# someone who received only the executable form, and is accompanied by a
# copy of the written offer of source code.
# 
# 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
# currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
# 2395 Timbercrest Court, Ann Arbor, MI 48105.
# 
# Copyright (C) 2000-2001 by The Regents of The University of Michigan.
# Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.

##################################################################
#
# Modify the following definitions to suit your build environment,
# NOTE: most platforms should not require any changes
#
##################################################################


T= optiflow

#SRCROOT= $(LOCAL)

BINDIR= $(LEVO)/bin
INCDIR= $(SRCROOT)/include
LIBDIR= $(SRCROOT)/lib

LDCRTDIR= $(SRCROOT)/lib


CC= gcc

CCOPTS= -O2 -mcpu=v8 -g
#CCOPTS= -g
#CCOPTS= -g -O
#CCOPTS= -O
# debugging with extra symbol information w/ Sun Workshop C
#CCOPTS= -g -xs
# debugging with profiling w/ GNU C
#CCOPTS= -g -pg

# HyperSPARC
# regular
#CCOPTS= -xO5 -xtarget=ss20/hs22 -dalign -xdepend
# profiling 'prof(1)'
#CCOPTS= -p -xO5 -xtarget=ss20/hs22 -dalign -xdepend
# profiling 'gprof(1)' 
#CCOPTS= -xpg -xO5 -xtarget=ss20/hs22 -dalign -xdepend

# UltraSPARC
# regular
#CCOPTS= -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
# profiling 'prof(1)'
#CCOPTS= -p -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
# profiling 'gprof(1)' 
#CCOPTS= -xpg -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend


#LD= $(CC)
#LD= cc
LD= ld

# regular
LDFLAGS= -R$(LIBDIR)
# for profiling
LDFLAGS_P= -L/usr/lib/libp # -m -R$(LIBDIR)


CRTI= $(LDCRTDIR)/crti.o
CRT1= $(LDCRTDIR)/crt1.o
MCRT1= $(LDCRTDIR)/mcrt1.o
GCRT1= $(LDCRTDIR)/gcrt1.o
VALUES= $(LDCRTDIR)/values-xa.o
CRTN= $(LDCRTDIR)/crtn.o

# for regular (no profiling)
CRT0= $(CRTI) $(CRT1) $(VALUES)
# for 'prof(1)'
MCRT0= $(CRTI) $(MCRT1) $(VALUES)
# for 'gprof(1)'
GCRT0= $(CRTI) $(GCRT1) $(VALUES)

#CRTC= makedate.o
CRTC=

LINT= lint
LINTFLAGS= -uxn

NM= nm
NMFLAGS= -xs -v

CXREF= cxref
CXREFFLAGS= -R -s

#CPP= cpp

LORDER= lorder
TSORT= tsort

RM= rm -f


#
# Define below C compiler and flags, machine-specific flags and libraries,
# build tools and file extensions, these are specific to a host environment,
# pre-tested environments follow...
#

#CPP= cpp -D__STDC__=1
CPP= cc -E -D__STD__=1


##
## vanilla Unix, GCC build
##
## NOTE: the SimpleScalar simulators must be compiled with an ANSI C
## compatible compiler.
##
## tested hosts:
##
##	Slackware Linux version 2.0.33, GNU GCC version 2.7.2.2
##	FreeBSD version 3.0-current, GNU egcs version 2.91.50
##	Alpha OSF1 version 4.0, GNU GCC version 2.7.2
##	PA-RISC HPUX version B.10.01, GNU GCC version 2.7-96q3
##	SPARC SunOS version 5.5.1, GNU egcs-2.90.29
##	RS/6000 AIX Unix version 4, GNU GCC version cygnus-2.7-96q4
##	Windows NT version 4.0, Cygnus CygWin/32 beta 19
##
#CC = gcc
#OFLAGS = -O0 -g -Wall
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/
#
##
## Solaris 2.6, GNU GCC version 2.7.2.3
##
#CC = gcc
#OFLAGS = -O0 -g -Wall
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` libflock.a -lm -lsocket -lnsl
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## Alpha OSF1 version 4.0, DEC C compiler version V5.2-036
##
#CC = cc -std
#OFLAGS = -O0 -g -w
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/


##
##
## SPARC SunOS 5.8 (Solaris 8)
## REMEMBER: switching compilers requires a whole re-compile in this setup
##
## GNU GCC
#LD= ld
#CC= gcc
#CCOPTS= -O4 -mv8
#CCOPTS= -O -g
#CCOPTS= -g

## Sun Microsystems Forte 6.x CC
#CC= cc
#CCOPTS= -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
#CCOPTS= -g


# we need this for all compilers since some modules might have used GCC
CLIBS= -L$(GNU)/lib -lgcc
OFLAGS= $(CCOPTS)


DEF0= -DSYSV=1 -DSOLARIS=8 -DPOSIX=1 -DHAVE_LONGLONG=1
DEF1=
DEF2= -D__EXTENSIONS__=1
DEF3= -D_POSIX_C_SOURCE=199506L
DEF4=
DEF5= # -DCF_MASTERDEBUG=0
DEF6=
DEF7= -DP_OPTIFLOW=1 

DFLAGS= $(DEF0) $(DEF1) $(DEF2) $(DEF3) $(DEF4) $(DEF5) $(DEF6) $(DEF7)


I0= -I$(INCDIR)
I1=
I2=
I3=

INCDIRS= $(I0) $(I1) $(I2) $(I3)


LIBDIRS= -L$(SRCROOT)/lib


L0=
L1= -ldam
#L2= -lb -luc -lu
L2= -lb -luc $(SRCROOT)/lib/libu.a
L3=
L4=
L5=
L6=
L7= -lsecdb -lproject

LIBS= $(L0) $(L1) $(L2) $(L3) $(L4) $(L5) $(L6) $(L7)


MFLAGS = `./sysprobe -flags` -D__svr4__=1 -D__unix__=1
MLIBS  = `./sysprobe -libs` $(CLIBS) libflock.a -ldl -lrt -lsocket -lnsl -lm
ENDIAN = `./sysprobe -s`
MAKE = make
AR = ar qcv
AROPT =
RANLIB = ranlib
RM = rm -f
RMDIR = rm -f
LN = ln -s
LNDIR = ln -s
DIFF = diff
OEXT = o
LEXT = a
EEXT =
CS = ;
X=/




#
# Compilation-specific feature flags
#
# -DDEBUG	- turns on debugging features
# -DBFD_LOADER	- use libbfd.a to load programs (also required BINUTILS_INC
#		  and BINUTILS_LIB to be defined, see below)
# -DGZIP_PATH	- specifies path to GZIP executable, only needed if SYSPROBE
#		  cannot locate binary
# -DSLOW_SHIFTS	- emulate all shift operations, only used for testing as
#		  sysprobe will auto-detect if host can use fast shifts
#
FFLAGS = -DDEBUG

#
# Point the Makefile to your Simplescalar-based bunutils, these definitions
# should indicate where the include and library directories reside.
# NOTE: these definitions are only required if BFD_LOADER is defined.
#
#BINUTILS_INC = -I../include
#BINUTILS_LIB = -L../lib



# for Simple-Scalar stuff
BI0= syscall.h memory.h regs.h sim.h loader.h cache.h bpred.h ptrace.h
BI1= eventq.h resource.h endian.h dlite.h symbol.h eval.h bitmap.h
BI2= eio.h range.h version.h endian.h misc.h
BI3= ssconfig.h defs.h bpeval.h bpresult.h
BI4= iflags.h itype.h density.h
BASEINCS= machine.h $(BI0) $(BI1) $(BI2) $(BI3) $(BI4)

# for Levo-like stuff 
SSI0= ssnames.h ss.h checker.h sscommon.h ssinfo.h ssdecode.h
SSI1= 
SSI2= ssiw.h ssas.h ssreg.h sspe.h sslsq.h ssfetch.h
SSI3= 
SSINCS= $(SSI0) $(SSI1) $(SSI2) $(SSI3)

STI0= regstats.h memstats.h
STI1=
STI2=
STI3=
STINCS= $(STI0) $(STI1) $(STI2) $(STI3)





##################################################################
#
# YOU SHOULD NOT NEED TO MODIFY ANYTHING BELOW THIS COMMENT
#
##################################################################

#
# complete flags
#
CFLAGS= $(DFLAGS) $(MFLAGS) $(FFLAGS) $(INCDIRS) \
		$(OFLAGS) $(BINUTILS_INC) $(BINUTILS_LIB)

CPPFLAGS= $(DFLAGS) $(MFLAGS) $(FFLAGS) $(INCDIRS)





#
# all the sources
#
SRCS =	main.c sim-fast.c sim-safe.c sim-cache.c sim-profile.c \
	sim-eio.c sim-bpred.c sim-cheetah.c sim-outorder.c \
	memory.c regs.c cache.c bpred.c ptrace.c eventq.c \
	resource.c endian.c dlite.c symbol.c eval.c options.c range.c \
	eio.c stats.c endian.c misc.c \
	mase-checker.c mase-commit.c mase-debug.c \
	mase-exec.c mase-fe.c mase.c mase-mem.c mase-opts.c \
	target-pisa/pisa.c target-pisa/loader.c target-pisa/syscall.c \
	target-pisa/symbol.c \
	target-alpha/alpha.c target-alpha/loader.c target-alpha/syscall.c \
	target-alpha/symbol.c \
	sim-optiflow.c \
	swap.c strsimpoint.c

HDRS =	syscall.h memory.h regs.h sim.h loader.h cache.h bpred.h ptrace.h \
	eventq.h resource.h endian.h dlite.h symbol.h eval.h bitmap.h \
	eio.h range.h version.h endian.h misc.h \
	mase-checker.h mase-commit.h mase-debug.h \
	mase-decode.h mase-exec.h mase-fe.h mase.h \
	mase-mem.h mase-opts.h mase-structs.h \
	target-pisa/pisa.h target-pisa/pisabig.h target-pisa/pisalittle.h \
	target-pisa/pisa.def target-pisa/ecoff.h \
	target-alpha/alpha.h target-alpha/alpha.def target-alpha/ecoff.h \
	sim-levo.h \
	levo-checker.h levo-commit.h levo-debug.h \
	levo-decode.h levo-exec.h levo-fe.h levo.h \
	levo-mem.h levo-opts.h levo-structs.h

#
# common objects
#
OBJS=	main.$(OEXT) syscall.$(OEXT) memory.$(OEXT) regs.$(OEXT) \
	loader.$(OEXT) endian.$(OEXT) dlite.$(OEXT) symbol.$(OEXT) \
	eval.$(OEXT) options.$(OEXT) stats.$(OEXT) eio.$(OEXT) \
	range.$(OEXT) misc.$(OEXT) machine.$(OEXT) swap.$(OEXT) \
	iflags.o itype.o density.o percentll.o \
	regstats.o memstats.o denpercents.o \
	strsimpoint.o \
	getfname.o paramfile.o varsub_extras.o proginfo.o debug.o

#	bpeval.o bpfifo.o bpresult.o 


# sim-outoder

OOBJS=

OO_OBJS= sim-outorder.o


# sim-mase

MOBJ0= mase-checker.$(OEXT) mase-commit.$(OEXT) mase-debug.$(OEXT) 
MOBJ1= mase-exec.$(OEXT) mase-fe.$(OEXT) mase.$(OEXT) mase-mem.$(OEXT) 
MOBJ2= mase-opts.$(OEXT)
MOBJ3=
MOBJ4=
MOBJ5=

MASE_OBJS= $(MOBJ0) $(MOBJ1) $(MOBJ2) $(MOBJ3)


# sim-levo

LOBJ0= levo-checker.o levo-commit.o levo-debug.o 
LOBJ1= levo-exec.o levo-fe.o levo.o levo-mem.o 
LOBJ2= levo-opts.o
LOBJ3=
LOBJ4=
LOBJ5=

LEVO_OBJS= $(LOBJ0) $(LOBJ1) $(LOBJ2) $(LOBJ3) $(LOBJ4) $(LOBJ5)


# sim-optiflow

TOBJ0= sim-optiflow.o checker.o
TOBJ1= ss.o ss_instrexec.o
TOBJ2= ssinfo.o 
TOBJ3= ssiw.o ssas.o ssreg.o sspe.o sslsq.o ssnames.o ssfetch.o sshiftreg.o
TOBJ4= seqok.o
TOBJ5= 
TOBJ6= isvalidtextaddr.o

TEST_OBJS= $(TOBJ0) $(TOBJ1) $(TOBJ2) $(TOBJ3) $(TOBJ4) $(TOBJ5) $(TOBJ6)


# sim-safe

SOBJ0= sim-safe.o
SOBJ1=
SOBJ2=
SOBJ3=
SOBJ4=
SOBJ5=

SAFE_OBJS= $(SOBJ0) $(SOBJ1) $(SOBJ2) $(SOBJ3) $(SOBJ4) $(SOBJ5)




#
# programs to build
#
PROGS = sim-fast$(EEXT) sim-safe$(EEXT) sim-eio$(EEXT) \
	sim-bpred$(EEXT) sim-profile$(EEXT) \
	sim-cheetah$(EEXT) sim-cache$(EEXT) sim-outorder$(EEXT) \
	sim-mase$(EXT) \
	sim-optiflow$(EXT)


default:	$(T)

strip:		$(T)
	strip $(T)


.SUFFIXES:		.ls .i .cx .cs

#
# all targets, NOTE: library ordering is important...
#
all:		$(PROGS)
	@echo "my work is done here..."

.c.i:
	$(CPP) $(CPPFLAGS) $< > $(*).i

.c.ln:
	$(LINT) -c -u $(CPPFLAGS) $<

.c.ls:
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $<



config-pisa:
	-$(RM) config.h machine.h machine.c machine.def \
		loader.c symbol.c syscall.c
	$(LN) target-pisa$(X)config.h config.h
	$(LN) target-pisa$(X)pisa.h machine.h
	$(LN) target-pisa$(X)pisa.c machine.c
	$(LN) target-pisa$(X)pisa.def machine.def
	$(LN) target-pisa$(X)loader.c loader.c
	$(LN) target-pisa$(X)symbol.c symbol.c
	$(LN) target-pisa$(X)syscall.c syscall.c
	-$(RMDIR) tests
	$(LNDIR) tests-pisa tests

config-pisabig:
	-$(RM) config.h machine.h machine.c machine.def \
		loader.c symbol.c syscall.c
	$(LN) target-pisa$(X)configbig.h config.h
	$(LN) target-pisa$(X)pisa.h machine.h
	$(LN) target-pisa$(X)pisa.c machine.c
	$(LN) target-pisa$(X)pisa.def machine.def
	$(LN) target-pisa$(X)loader.c loader.c
	$(LN) target-pisa$(X)symbol.c symbol.c
	$(LN) target-pisa$(X)syscall.c syscall.c
	-$(RMDIR) tests
	$(LNDIR) tests-pisa tests

config-pisalit:
	-$(RM) config.h machine.h machine.c machine.def \
		loader.c symbol.c syscall.c
	$(LN) target-pisa$(X)configlit.h config.h
	$(LN) target-pisa$(X)pisa.h machine.h
	$(LN) target-pisa$(X)pisa.c machine.c
	$(LN) target-pisa$(X)pisa.def machine.def
	$(LN) target-pisa$(X)loader.c loader.c
	$(LN) target-pisa$(X)symbol.c symbol.c
	$(LN) target-pisa$(X)syscall.c syscall.c
	-$(RMDIR) tests
	$(LNDIR) tests-pisa tests

config-alpha:
	-$(RM) config.h machine.h machine.c machine.def \
		loader.c symbol.c syscall.c
	$(LN) target-alpha$(X)config.h config.h
	$(LN) target-alpha$(X)alpha.h machine.h
	$(LN) target-alpha$(X)alpha.c machine.c
	$(LN) target-alpha$(X)alpha.def machine.def
	$(LN) target-alpha$(X)loader.c loader.c
	$(LN) target-alpha$(X)symbol.c symbol.c
	$(LN) target-alpha$(X)syscall.c syscall.c
	-$(RMDIR) tests
	$(LNDIR) tests-alpha tests

sysprobe$(EEXT):	sysprobe.c
	$(CC) $(FFLAGS) -o sysprobe$(EEXT) sysprobe.c
	@echo endian probe results: $(ENDIAN)
	@echo probe flags: $(MFLAGS)
	@echo probe libs: $(MLIBS)
	-$(RM) tests$(X)sysprobe$(EEXT)
	$(LN) ../sysprobe$(EEXT) tests$(X)sysprobe$(EEXT)
	-$(RM) libcheetah$(X)sysprobe$(EEXT)
	$(LN) ../sysprobe$(EEXT) libcheetah$(X)sysprobe$(EEXT)
	-$(RM) libexo$(X)sysprobe$(EEXT)
	$(LN) ../sysprobe$(EEXT) libexo$(X)sysprobe$(EEXT)

sim-fast$(EEXT):	sysprobe$(EEXT) sim-fast.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-fast$(EEXT) $(CFLAGS) sim-fast.$(OEXT) $(OBJS) \
		libexo/libexo.$(LEXT) $(MLIBS)

sim-profile$(EEXT):	sysprobe$(EEXT) sim-profile.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-profile$(EEXT) $(CFLAGS) sim-profile.$(OEXT) \
		$(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-eio$(EEXT):	sysprobe$(EEXT) sim-eio.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-eio$(EEXT) $(CFLAGS) sim-eio.$(OEXT) $(OBJS) \
		libexo/libexo.$(LEXT) $(MLIBS)

sim-bpred$(EEXT):	sysprobe$(EEXT) sim-bpred.$(OEXT) bpred.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-bpred$(EEXT) $(CFLAGS) sim-bpred.$(OEXT) \
		bpred.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-cheetah$(EEXT):	sysprobe$(EEXT) sim-cheetah.$(OEXT) $(OBJS) libcheetah/libcheetah.$(LEXT) libexo/libexo.$(LEXT)
	$(CC) -o sim-cheetah$(EEXT) $(CFLAGS) sim-cheetah.$(OEXT) $(OBJS) \
		libcheetah/libcheetah.$(LEXT) libexo/libexo.$(LEXT) $(MLIBS)

sim-cache$(EEXT):	sysprobe$(EEXT) sim-cache.$(OEXT) cache.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-cache$(EEXT) $(CFLAGS) sim-cache.$(OEXT) \
		cache.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)



EDEPS= sysprobe$(EEXT) 

EOBJ0= 
EOBJ1= cache.$(OEXT) bpred.$(OEXT) resource.$(OEXT) 
EOBJ2= ptrace.$(OEXT)
EOBJ3=

EOBJS= $(EOBJ0) $(EOBJ1) $(EOBJ2) $(EOBJ3)


ELIB0= libexo/libexo.$(LEXT)
ELIB1=
ELIB2=
ELIB3=

ELIBS= $(ELIB0) $(ELIB1) $(ELIB2) $(ELIB3)



sim-outorder$(EEXT):	sysprobe$(EEXT) $(OO_OBJS) cache.$(OEXT) \
	bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) $(OBJS) \
	libexo/libexo.$(LEXT)
	$(CC) -o $@ $(LDFLAGS) $(OO_OBJS) cache.$(OEXT) \
		bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) $(OBJS) \
		$(ELIBS) $(LIBDIRS) $(LIBS) $(MLIBS)

sim-mase$(EEXT):	sysprobe$(EEXT) $(MASE_OBJS) cache.$(OEXT) \
	bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) $(OBJS) \
	libexo/libexo.$(LEXT)
	$(CC) -o $@ $(LDFLAGS) $(MASE_OBJS) cache.$(OEXT) \
		bpred.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) $(OBJS) \
		$(ELIBS) $(LIBDIRS) $(LIBS) $(MLIBS)

sim-levo$(EEXT):	$(EDEPS) $(EOBJS) $(LEVO_OBJS) $(OBJS) $(ELIBS)
	$(CC) -o $@ $(LDFLAGS) $(LEVO_OBJS) $(EOBJS) $(OBJS) \
		$(ELIBS) $(LIBDIRS) $(LIBS) $(MLIBS)


$(T):		Makefile $(EDEPS) $(EOBJS) $(TEST_OBJS) $(OBJS) $(ELIBS)
	rm -f $@
	$(LD) -o $@ $(LDFLAGS) $(CRT0) $(TEST_OBJS) $(EOBJS) $(OBJS) $(CRTN) \
		$(ELIBS) $(LIBDIRS) $(LIBS) $(MLIBS) -lc > $(T).lm


sim-optiflow$(EEXT):	$(EDEPS) $(EOBJS) $(TEST_OBJS) $(OBJS) $(ELIBS)
	$(CC) -o $@ $(LDFLAGS) $(TEST_OBJS) $(EOBJS) $(OBJS) \
		$(ELIBS) $(LIBDIRS) $(LIBS) $(MLIBS) > sim-optiflow.lm

sim-safe$(EEXT):	$(EDEPS) $(EOBJS) $(SAFE_OBJS) $(OBJS) $(ELIBS)
	$(CC) -o $@ $(LDFLAGS) $(SAFE_OBJS) $(EOBJS) $(OBJS) \
		$(ELIBS) $(LIBDIRS) $(LIBS) $(MLIBS)

# sim-safe$(EEXT):	sysprobe$(EEXT) sim-safe.$(OEXT) $(OBJS) \
#		libexo/libexo.$(LEXT)
#	$(CC) -o sim-safe$(EEXT) $(CFLAGS) sim-safe.$(OEXT) $(OBJS) \
#		libexo/libexo.$(LEXT) $(MLIBS)





exo libexo/libexo.$(LEXT): sysprobe$(EEXT)
	cd libexo $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" \
		"RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(OFLAGS)" \
		"OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" \
		"RM=$(RM)" libexo.$(LEXT)

cheetah libcheetah/libcheetah.$(LEXT): sysprobe$(EEXT)
	cd libcheetah $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" \
		"RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(OFLAGS)" \
		"OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" \
		"RM=$(RM)" libcheetah.$(LEXT)

.c.$(OEXT):
	$(CC) -c $(CFLAGS) $<

#	echo $(CC) $(CFLAGS) -c $*.c

filelist:
	@echo $(SRCS) $(HDRS) Makefile

diffs:
	-rcsdiff RCS/*
	-cd config; rcsdiff RCS/*
	-cd libcheetah; rcsdiff RCS/*
	-cd libexo; rcsdiff RCS/*
	-cd target-alpha; rcsdiff RCS/*
	-cd target-pisa; rcsdiff RCS/*

sim-optiflows sim-optiflows-nt: sysprobe$(EEXT) $(PROGS)
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-fast$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-safe$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-cache$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-cheetah$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-bpred$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-profile$(EEXT)" \
		"X=$(X)" "CS=$(CS)" "SIM_OPTS=-all" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-outorder$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-mase$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..

mase-tests: sysprobe$(EEXT) $(PROGS)
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-mase$(EEXT) -config ../config/mase.cfg" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..

safe:
	makesafe -v -I $(INCDIR) $(EDEPS) $(EOBJS) $(TEST_OBJS) $(OBJS) $(ELIBS)

install:	$(T)
	makenewer -r $(T) $(BINDIR)

again:
	rm -f $(T)

clean:
	-$(RM) *.o *.obj core *~ Makefile.bak sysprobe$(EEXT) $(PROGS)
	cd libcheetah $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd libexo $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd tests-alpha $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd tests-pisa $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd makedepend $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..

unpure:
	rm -f sim.pure *pure*.o sim.pure.pure_hardlink sim.pure.pure_linkinfo

depend:	mkdepend
	makedepend/makedepend -n $(BINUTILS_INC) $(SRCS)

mkdepend:
	cd makedepend $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" $(CS) cd ..



sim-safe.i:	sim-safe.o
	$(CPP) $(CPPFLAGS) sim-safe.c > sim-safe.i

mase-exec.i:	mase-exec.c



# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o: host.h misc.h machine.h config.h endian.h machine.def version.h
main.o: dlite.h regs.h memory.h options.h stats.h eval.h loader.h sim.h

main.o:		main.c $(BASEINCS) $(SSINCS)

sim-fast.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-fast.o: memory.h options.h stats.h eval.h loader.h syscall.h dlite.h
sim-fast.o: sim.h

sim-safe.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-safe.o: memory.h options.h stats.h eval.h loader.h syscall.h dlite.h
sim-safe.o: sim.h

sim-cache.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-cache.o: memory.h options.h stats.h eval.h cache.h loader.h syscall.h
sim-cache.o: dlite.h sim.h

sim-profile.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-profile.o: memory.h options.h stats.h eval.h loader.h syscall.h dlite.h
sim-profile.o: symbol.h sim.h

sim-eio.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-eio.o: memory.h options.h stats.h eval.h loader.h syscall.h dlite.h eio.h
sim-eio.o: range.h sim.h

sim-bpred.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-bpred.o: memory.h options.h stats.h eval.h loader.h syscall.h dlite.h
sim-bpred.o: bpred.h sim.h
sim-cheetah.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-cheetah.o: memory.h options.h stats.h eval.h loader.h syscall.h dlite.h
sim-cheetah.o: libcheetah/libcheetah.h sim.h
sim-outorder.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
sim-outorder.o: memory.h options.h stats.h eval.h cache.h loader.h syscall.h
sim-outorder.o: bpred.h resource.h bitmap.h ptrace.h range.h dlite.h sim.h
memory.o: host.h misc.h machine.h config.h endian.h machine.def options.h
memory.o: stats.h eval.h memory.h
regs.o: host.h misc.h machine.h config.h endian.h machine.def loader.h regs.h
regs.o: memory.h options.h stats.h eval.h
cache.o: host.h misc.h machine.h config.h endian.h machine.def cache.h
cache.o: memory.h options.h stats.h eval.h
bpred.o: host.h misc.h machine.h config.h endian.h machine.def bpred.h
bpred.o: stats.h eval.h
ptrace.o: host.h misc.h machine.h config.h endian.h machine.def range.h
ptrace.o: ptrace.h
eventq.o: host.h misc.h machine.h config.h endian.h machine.def eventq.h
eventq.o: bitmap.h
resource.o: host.h misc.h resource.h
endian.o: endian.h loader.h host.h misc.h machine.h config.h machine.def
endian.o: regs.h memory.h options.h stats.h eval.h
dlite.o: host.h misc.h machine.h config.h endian.h machine.def version.h
dlite.o: eval.h regs.h memory.h options.h stats.h sim.h symbol.h loader.h
dlite.o: range.h dlite.h
symbol.o: host.h misc.h loader.h machine.h config.h endian.h machine.def
symbol.o: regs.h memory.h options.h stats.h eval.h symbol.h
symbol.o: target-alpha/ecoff.h target-alpha/alpha.h
eval.o: host.h misc.h eval.h machine.h config.h endian.h machine.def
options.o: host.h misc.h options.h
range.o: host.h misc.h machine.h config.h endian.h machine.def symbol.h
range.o: loader.h regs.h memory.h options.h stats.h eval.h range.h
eio.o: host.h misc.h machine.h config.h endian.h machine.def regs.h memory.h
eio.o: options.h stats.h eval.h loader.h libexo/libexo.h host.h misc.h
eio.o: machine.h syscall.h sim.h eio.h
stats.o: host.h misc.h machine.h config.h endian.h machine.def eval.h stats.h
endian.o: endian.h loader.h host.h misc.h machine.h config.h machine.def
endian.o: regs.h memory.h options.h stats.h eval.h
misc.o: host.h misc.h machine.h config.h endian.h machine.def

mase-checker.o: mase.h host.h misc.h machine.h config.h endian.h machine.def
mase-checker.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
mase-checker.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h
mase-checker.o: dlite.h sim.h mase-structs.h mase-opts.h mase-checker.h
mase-checker.o: mase-debug.h mase-fe.h mase-decode.h mase-exec.h mase-mem.h
mase-checker.o: mase-commit.h mase-macros-oracle.h
mase-commit.o: mase.h host.h misc.h machine.h config.h endian.h machine.def
mase-commit.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
mase-commit.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
mase-commit.o: sim.h mase-structs.h mase-opts.h mase-checker.h mase-debug.h
mase-commit.o: mase-fe.h mase-decode.h mase-exec.h mase-mem.h mase-commit.h
mase-debug.o: mase.h host.h misc.h machine.h config.h endian.h machine.def
mase-debug.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
mase-debug.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
mase-debug.o: sim.h mase-structs.h mase-opts.h mase-checker.h mase-debug.h
mase-debug.o: mase-fe.h mase-decode.h mase-exec.h mase-mem.h mase-commit.h
mase-exec.o: mase.h host.h misc.h machine.h config.h endian.h machine.def
mase-exec.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
mase-exec.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
mase-exec.o: sim.h mase-structs.h mase-opts.h mase-checker.h mase-debug.h
mase-exec.o: mase-fe.h mase-decode.h mase-exec.h mase-mem.h mase-commit.h
mase-exec.o: mase-macros-exec.h
mase-fe.o: mase.h host.h misc.h machine.h config.h endian.h machine.def
mase-fe.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
mase-fe.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
mase-fe.o: sim.h mase-structs.h mase-opts.h mase-checker.h mase-debug.h
mase-fe.o: mase-fe.h mase-decode.h mase-exec.h mase-mem.h mase-commit.h
mase.o: mase.h host.h misc.h machine.h config.h endian.h machine.def regs.h
mase.o: memory.h options.h stats.h eval.h cache.h loader.h syscall.h bpred.h
mase.o: resource.h bitmap.h ptrace.h range.h dlite.h sim.h mase-structs.h
mase.o: mase-opts.h mase-checker.h mase-debug.h mase-fe.h mase-decode.h
mase.o: mase-exec.h mase-mem.h mase-commit.h mase-macros-oracle.h
mase-mem.o: mase.h host.h misc.h machine.h config.h endian.h machine.def
mase-mem.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
mase-mem.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
mase-mem.o: sim.h mase-structs.h mase-opts.h mase-checker.h mase-debug.h
mase-mem.o: mase-fe.h mase-decode.h mase-exec.h mase-mem.h mase-commit.h
mase-opts.o: mase.h host.h misc.h machine.h config.h endian.h machine.def
mase-opts.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
mase-opts.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
mase-opts.o: sim.h mase-structs.h mase-opts.h mase-checker.h mase-debug.h
mase-opts.o: mase-fe.h mase-decode.h mase-exec.h mase-mem.h mase-commit.h

pisa.o: host.h misc.h machine.h config.h endian.h machine.def eval.h regs.h
loader.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
loader.o: memory.h options.h stats.h eval.h sim.h eio.h loader.h
loader.o: target-pisa/ecoff.h
syscall.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
syscall.o: memory.h options.h stats.h eval.h loader.h sim.h eio.h syscall.h
symbol.o: host.h misc.h target-pisa/ecoff.h loader.h machine.h config.h
symbol.o: endian.h machine.def regs.h memory.h options.h stats.h eval.h
symbol.o: symbol.h
alpha.o: host.h misc.h machine.h config.h endian.h machine.def eval.h regs.h
loader.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
loader.o: memory.h options.h stats.h eval.h sim.h eio.h loader.h
loader.o: target-alpha/ecoff.h target-alpha/alpha.h
syscall.o: host.h misc.h machine.h config.h endian.h machine.def regs.h
syscall.o: memory.h options.h stats.h eval.h loader.h sim.h eio.h syscall.h
symbol.o: host.h misc.h loader.h machine.h config.h endian.h machine.def
symbol.o: regs.h memory.h options.h stats.h eval.h symbol.h
symbol.o: target-alpha/ecoff.h target-alpha/alpha.h


levo-checker.o: levo.h host.h misc.h machine.h config.h endian.h machine.def
levo-checker.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
levo-checker.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h
levo-checker.o: dlite.h sim.h levo-structs.h levo-opts.h levo-checker.h
levo-checker.o: levo-debug.h levo-fe.h levo-decode.h levo-exec.h levo-mem.h
levo-checker.o: levo-commit.h levo-macros-oracle.h
levo-commit.o: levo.h host.h misc.h machine.h config.h endian.h machine.def
levo-commit.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
levo-commit.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
levo-commit.o: sim.h levo-structs.h levo-opts.h levo-checker.h levo-debug.h
levo-commit.o: levo-fe.h levo-decode.h levo-exec.h levo-mem.h levo-commit.h
levo-debug.o: levo.h host.h misc.h machine.h config.h endian.h machine.def
levo-debug.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
levo-debug.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
levo-debug.o: sim.h levo-structs.h levo-opts.h levo-checker.h levo-debug.h
levo-debug.o: levo-fe.h levo-decode.h levo-exec.h levo-mem.h levo-commit.h
levo-exec.o: levo.h host.h misc.h machine.h config.h endian.h machine.def
levo-exec.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
levo-exec.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
levo-exec.o: sim.h levo-structs.h levo-opts.h levo-checker.h levo-debug.h
levo-exec.o: levo-fe.h levo-decode.h levo-exec.h levo-mem.h levo-commit.h
levo-exec.o: levo-macros-exec.h
levo-fe.o: levo.h host.h misc.h machine.h config.h endian.h machine.def
levo-fe.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
levo-fe.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
levo-fe.o: sim.h levo-structs.h levo-opts.h levo-checker.h levo-debug.h
levo-fe.o: levo-fe.h levo-decode.h levo-exec.h levo-mem.h levo-commit.h
levo.o: levo.h host.h misc.h machine.h config.h endian.h machine.def regs.h
levo.o: memory.h options.h stats.h eval.h cache.h loader.h syscall.h bpred.h
levo.o: resource.h bitmap.h ptrace.h range.h dlite.h sim.h levo-structs.h
levo.o: levo-opts.h levo-checker.h levo-debug.h levo-fe.h levo-decode.h
levo.o: levo-exec.h levo-mem.h levo-commit.h levo-macros-oracle.h
levo-mem.o: levo.h host.h misc.h machine.h config.h endian.h machine.def
levo-mem.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
levo-mem.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
levo-mem.o: sim.h levo-structs.h levo-opts.h levo-checker.h levo-debug.h
levo-mem.o: levo-fe.h levo-decode.h levo-exec.h levo-mem.h levo-commit.h
levo-opts.o: levo.h host.h misc.h machine.h config.h endian.h machine.def
levo-opts.o: regs.h memory.h options.h stats.h eval.h cache.h loader.h
levo-opts.o: syscall.h bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
levo-opts.o: sim.h levo-structs.h levo-opts.h levo-checker.h levo-debug.h
levo-opts.o: levo-fe.h levo-decode.h levo-exec.h levo-mem.h levo-commit.h


proginfo.o:	proginfo.c defs.h ssconfig.h

sim-optiflow.o:	sim-optiflow.c $(BASEINCS) $(SSINCS) $(STINCS)

checker.o:	checker.c checker.h $(BASEINCS) $(SSINCS)

ss.o:		ss.c ss.h $(BASEINCS) $(SSINCS)

ss_instrexec.o:	ss_instrexec.c $(BASEINCS) $(SSINCS)

ssnames.o:	ssnames.c $(BASEINCS) $(SSINCS)

ssinfo.o:	ssinfo.c ssinfo.h $(BASEINCS)

ssiw.o:		ssiw.c ssiw.h $(BASEINCS) $(SSINCS) ssnames.h

ssas.o:		ssas.c ssas.h $(BASEINCS) $(SSINCS)

ssreg.o:	ssreg.c ssreg.h $(BASEINCS) $(SSINCS)

sspe.o:		sspe.c sspe.h $(BASEINCS) $(SSINCS)

ssfetch.o:	ssfetch.c ssfetch.h $(BASEINCS) $(SSINCS)

sslsq.o:	sslsq.c sslsq.h $(BASEINCS) $(SSINCS)


iflags.o:	iflags.c iflags.h $(BASEINCS)

itype.o:	itype.c itype.h $(BASEINCS)

debug.o:	debug.c $(BASEINCS) $(SSINCS)


regstats.o:	regstats.c regstats.h

memstats.o:	memstats.c memstats.h

density.o:	density.c density.h


percentll.o:	percentll.c

denpercents.o:	denpercents.c

swap.o:		swap.c

strsimpoint.o:	strsimpoint.c $(BASEINCS)


bpeval.o:	bpeval.c bpeval.h bpfifo.h

bpfifo.o:	bpfifo.c bpfifo.h

gspag.o:	gspag.c gspag.h

tourna.o:	tourna.c tourna.h

gskew.o:	gskew.c gskew.h


paramfile.o:	paramfile.c paramfile.h

sbuf.o:		sbuf.c sbuf.h



