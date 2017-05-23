# MAKEFILE

T= traceproc

ALL= $(T)

SRCROOT=$(HOME)
DSTROOT=$(LEVO)


INCDIR= $(SRCROOT)/include
LIBDIR= $(SRCROOT)/lib
BINDIR= $(DSTROOT)/bin
MANDIR= $(SRCROOT)/man/man1
HELPDIR= $(SRCROOT)/share/help

#LDCRTDIR= /opt/SUNWspro/WS6/lib
#LDCRTDIR= /opt/SUNWspro/SC5.0/lib
#LDCRTDIR= /opt/SUNWspro/SC4.0/lib
#LDCRTDIR= /opt/SUNWspro/lib
LDCRTDIR= $(SRCROOT)/lib


CC= gcc
GPP= g++

CCOPTS_GCCALL= -pthreads
CCOPTS_CCALL= -MT

CCOPTS_GCCOPTSPARC= -mcpu=v9
CCOPTS_GCCLIBSPARC= -mno-app-regs -fno-builtin-bcopy

CCOPTS= $(CCOPTS_GCCALL) -O $(CCOPTS_GCCOPTSPARC)
#CCOPTS= $(CCOPTS_GCCALL) -g -O
#CCOPTS= $(CCOPTS_GCCALL) -g -pg
#CCOPTS= $(CCOPTS_GCCALL) -g -Wstrict-aliasing
#CCOPTS= $(CCOPTS_GCCALL) -g -Wall

CCOPTS_GCCLIB= -fpic $(CCOPTS_GCCLIBSPARC)

#CCOPTS= $(CCOPTS_CCALL) -g -xs

# HyperSPARC
#CCOPTS= -xO5 -xtarget=ss20/hs22 -dalign -xdepend

# UltraSPARC
# regular
#CCOPTS= -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
# profiling for 'prof(1)'
#CCOPTS= -p -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
# profiling for 'gprof(1)'
#CCOPTS= -xpg -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend


DEF0= -DOSNAME_$(OSNAME)=$(OSNUM)
DEF1=
DEF2=
DEF3=
DEF4=
DEF5=
DEF6=
DEF7= $(LF_DEFS)

DEFS= $(DEF0) $(DEF1) $(DEF2) $(DEF3) $(DEF4) $(DEF5) $(DEF6) $(DEF7)

INCDIRS= -I$(INCDIR)

CPPFLAGS= $(DEFS) $(INCDIRS)

CFLAGS= $(CCOPTS)

#LD= $(CC)
#LD= cc
LD= ld

# CHANGE FOR PROFILING
# regular
LDFLAGS= -m $(LF_LDFLAGS) -R$(LIBDIR)
# for profiling
#LDFLAGS= (LF_LDFLAGS) -L/usr/lib/libp # -m -R$(LIBDIR)

LIBDIRS= -L$(LIBDIR)

LIB0=
LIB1= -ldam -lb
LIB2=
LIB3= -luc -lu
LIB4= -L$(GNU)/lib -lstdc++ -lgcc_eh -lgcc
LIB5= 
LIB6= -lsecdb -lproject -lpthread -lrt -lxnet -lsocket -lnsl
LIB7= -ldl -lc

LIBS= $(LIB0) $(LIB1) $(LIB2) $(LIB3) $(LIB4) $(LIB5) $(LIB6) $(LIB7)


CRT1= $(LDCRTDIR)/crt1.o
CRTI= $(LDCRTDIR)/crti.o
VALUES= $(LDCRTDIR)/values-xa.o
CRTBEGIN= $(LDCRTDIR)/crtbegin.o
MCRT1= $(LDCRTDIR)/mcrt1.o
GCRT1= $(LDCRTDIR)/gcrt1.o
CRTEND= $(LDCRTDIR)/crtend.o
CRTN= $(LDCRTDIR)/crtn.o

CRTFRONT= $(CRT1) $(CRTI) $(VALUES) $(CRTBEGIN)
CRTBACK= $(CRTEND) $(CRTN)

CRT0= $(CRT1) $(CRTI) $(VALUES)
MCRT0= $(CRTI) $(MCRT1) $(VALUES)
GCRT0= $(CRTI) $(GCRT1) $(VALUES)
CRTC= makedate.o

LINT= lint
LINTFLAGS= -uxn

NM= nm
NMFLAGS= -xs

CPP= cpp


MIPSINCS= localmisc.h lmipsregs.h opclass.h lexecop.h
PREDINCS= yags.h gspag.h tourna.h gskew.h
DENSTATS= regstats.h memstats.h
STATINCS= ssh.h bpeval.h bpfifo.h bpresult.h $(PREDINCS) $(DENSTATS)

INCS= config.h defs.h exectrace.h


OBJ00= main.o whatinfo.o proginfo.o proginfo_setpiv.o
OBJ01= 
OBJ02= 
OBJ03= tracedata.o filtercalls.o
OBJ04= icounter.o fcounter.o analyze.o dump.o tcopy.o stats.o
OBJ05= bpfifo.o bpresult.o bpeval.o
OBJ06= ssh.o vpred.o fcount.o readsginm.o regstats.o memstats.o
OBJ07= getrootdname.o printhelp.o
OBJ08= yags.o tourna.o gspag.o eveight.o gskew.o
OBJ09= ldecode.o lexec.o lflowgroup.o
OBJ10=
OBJ11= exectrace.o lmapprog.o mipsdis.o
OBJ12= procargs.o procenv.o mkfname2.o
OBJ13= bopenroot.o getfname.o
OBJ14= denpercents.o
OBJ15= malloclog.o debug.o

OBJ16=
OBJ17=
OBJ18=
OBJ19=
OBJ20=
OBJ21=
OBJ22=
OBJ23=

OBJA= $(OBJ00) $(OBJ01) $(OBJ02) $(OBJ03) $(OBJ04) $(OBJ05) $(OBJ06) $(OBJ07)
OBJB= $(OBJ08) $(OBJ09) $(OBJ10) $(OBJ11) $(OBJ12) $(OBJ13) $(OBJ14) $(OBJ15)
OBJC= $(OBJ16) $(OBJ17) $(OBJ18) $(OBJ19) $(OBJ20) $(OBJ21) $(OBJ22) $(OBJ23)

OBJ= $(OBJA) $(OBJB) $(OBJC)

# for regular (no profiling)
OBJS= $(CRTFRONT) $(OBJ) $(CRTC) $(CRTBACK)
# for 'prof(1)'
MOBJS= $(MCRT0) $(OBJ) $(CRTC)
# for 'gprof(1)'
GOBJS= $(GCRT0) $(OBJ) $(CRTC)


.SUFFIXES:		.ls .i .cx .cs


default:		$(T).x

all:			$(ALL)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

.cc.o:
	$(GPP) -c $(CFLAGS) $(CPPFLAGS) $<

.c.ln:
	$(LINT) -c -u $(CPPFLAGS) $<

.c.ls:
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $<

.c.i:
	$(CPP) $(CPPFLAGS) $< > $(*).i


$(T):			$(T).ee
	cp -p $(T).ee $(T)

$(T).x:			$(OBJ) Makefile
	rm -f $@
	makedate > makedate.c
	$(CC) -c makedate.c
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBDIRS) $(LIBS) > $(T).lm

$(T).prof:		$(OBJ) Makefile
	makedate > makedate.c
	$(CC) -c makedate.c
	$(LD) -o $@ $(LDFLAGS) $(MOBJS) $(LIBDIRS) $(LIBS) > $(T).lm

$(T).gprof:		$(OBJ) Makefile
	makedate > makedate.c
	$(CC) -c makedate.c
	$(LD) -o $@ $(LDFLAGS) $(GOBJS) $(LIBDIRS) $(LIBS) > $(T).lm

$(T).$(OFF) $(OFF):	$(T).x
	cp -p $(T).x $(T).$(OFF)

$(T).nm nm:		$(T).x
	$(NM) $(NMFLAGS) $(T).x > $(T).nm

safe:
	makesafe -v=3 -I $(INCDIR) $(OBJ)

strip:			$(T).x
	strip $(T).x
	rm -f $(T).$(OFF) $(T)

install:		install-raw install-help

install-ee:		$(ALL)
	rm -f $(BINDIR)/$(T).$(OFF)
	bsdinstall $(ALL) $(BINDIR)

install.$(OFF):		$(T) $(T).$(OFF)
	bsdinstall $(T) $(T).$(OFF) $(BINDIR)

install-raw:		$(T).x
	cp -p $(T).x $(T)
	rm -f $(BINDIR)/$(T).$(OFF)
	bsdinstall $(T) $(BINDIR)

install-help:		$(T).help
	-mkdir -p $(HELPDIR) 2> /dev/null
	makenewer -o rmsuf $(T).help $(HELPDIR)

clean:			again
	rm -f *.o

again:
	rm -f $(ALL) $(T) $(T).x $(T).$(OFF)


main.o:			main.c $(INCS)

whatinfo.o:		whatinfo.c config.h

proginfo.o:		proginfo.c $(INCS)

proginfo_setpiv.o:	proginfo_setpiv.c defs.h


tracedata.o:		tracedata.c tracedata.h $(INCS)

filtercalls.o:		filtercalls.c $(INCS)

analyze.o:		analyze.c $(INCS)

icounter.o:	icounter.c $(INCS)

fcounter.o:	fcounter.c $(INCS)

stats.o:	stats.c $(INCS) $(MIPSINCS) $(STATINCS)

tcopy.o:	tcopy.c $(INCS)

ldecode.o:	ldecode.c ldecode.h $(MIPSINCS)

lexec.o:	lexec.c lexec.h $(MIPSINCS) dataval.h

lflowgroup.o:	lflowgroup.c

lmapprog.o:	lmapprog.c lmapprog.h $(INCS)

mipsdis.o:	mipsdis.c mipsdis.h $(INCS)

exectrace.o:	exectrace.c exectrace.h

ssh.o:		ssh.c ssh.h

fcount.o:	fcount.c fcount.h

regstats.o:	regstats.c regstats.h

memstats.o:	memstats.c memstats.h

readsginm.o:	readsginm.c fcount.h

bpresult.o:	bpresult.c bpresult.h

bpeval.o:	bpeval.c bpeval.h bpload.h bpfifo.o

bpfifo.o:	bpfifo.c bpfifo.h

vpred.o:	vpred.c vpred.h

yags.o:		yags.c yags.h bpload.h

tourna.o:	tourna.c tourna.h bpload.h

bpalpha.o:	bpalpha.c bpalpha.h bpload.h

gspag.o:	gspag.c gspag.h bpload.h

eveight.o:	eveight.c eveight.h bpload.h

gskew.o:	gskew.c gskew.h bpload.h


keyopt.o:	keyopt.c keyopt.h

hdb.o:		hdb.c hdb.h


