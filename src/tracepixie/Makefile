# MAKEFILE

T= tracepixie

ALL= $(T) $(T).$(OFF)

SRCROOT=$(HOME)
DSTROOT=$(LEVO)


INCDIR= $(SRCROOT)/include
LIBDIR= $(SRCROOT)/lib

BINDIR= $(DSTROOT)/bin

#LDCRTDIR= /opt/SUNWspro/WS6/lib
#LDCRTDIR= /opt/SUNWspro/SC5.0/lib
#LDCRTDIR= /opt/SUNWspro/SC4.0/lib
#LDCRTDIR= /opt/SUNWspro/lib
LDCRTDIR= $(SRCROOT)/lib


# CHANGE FOR PROFILING

#CC= gcc
#CCOPTS= -O3 -mcpu=v9 # -fpic
#CCOPTS= -g -O
#CCOPTS= -g

# HyperSPARC
#CCOPTS= -xO5 -xtarget=ss20/hs22 -dalign -xdepend

# UltraSPARC
# regular
CCOPTS= -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
# profiling for 'prof(1)'
#CCOPTS= -p -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
# profiling for 'gprof(1)'
#CCOPTS= -xpg -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend


DEF0= -DOSNAME_$(OSNAME)=$(OSNUM) -DOSTYPE_$(OSTYPE)=1 -DOSNUM=$(OSNUM) 
DEF1= -DSOLARIS=$(OSNUM) -DPOSIX=1 -DPTHREAD=1
DEF2=
DEF3=
DEF4=
DEF5=
DEF6= -DF_MASTERDEBUG=0
DEF7=
#DEF7= -DDMALLOC=1 -DMALLOCLOG=1


DEFS= $(DEF0) $(DEF1) $(DEF2) $(DEF3) $(DEF4) $(DEF5) $(DEF6) $(DEF7)

INCDIRS= -I$(INCDIR)

CPPFLAGS= $(DEFS) $(INCDIRS)

CFLAGS= $(CCOPTS)

#LD= $(CC)
#LD= cc
LD= ld

# CHANGE FOR PROFILING
# regular
LDFLAGS= -m -R$(LIBDIR)
# for profiling
#LDFLAGS= -L/usr/lib/libp # -m -R$(LIBDIR)

LIBDIRS= -L$(LIBDIR)

LIB0=
LIB1= -Bstatic -ldam -Bdynamic
LIB2=
LIB3= -Bstatic -lb -luc -Bdynamic
#LIB3= -Bstatic -lb -luc -Bdynamic -ldmallocth
LIB4= -Bstatic -lu -Bdynamic
LIB5= -L$(GNU)/lib -lgcc
LIB6= -ldl -lrt -lpthread -lsocket -lnsl -lm
#LIB6= -ldl -lnsl -lm
LIB7= -lc

LIBS= $(LIB0) $(LIB1) $(LIB2) $(LIB3) $(LIB4) $(LIB5) $(LIB6) $(LIB7)

CRTI= $(LDCRTDIR)/crti.o
CRT1= $(LDCRTDIR)/crt1.o
MCRT1= $(LDCRTDIR)/mcrt1.o
GCRT1= $(LDCRTDIR)/gcrt1.o
VALUES= $(LDCRTDIR)/values-xa.o
CRTN= $(LDCRTDIR)/crtn.o

CRT0= $(CRTI) $(CRT1) $(VALUES)
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


OBJ00=
OBJ01= main.o whatinfo.o
OBJ02= convert.o
OBJ03=
OBJ04=
OBJ05= exectrace.o
OBJ06=
OBJ07=
OBJ08=
OBJ09=
OBJ10=
OBJ11=
OBJ12=
OBJ13=
OBJ14=
OBJ15=

OBJA= $(OBJ00) $(OBJ01) $(OBJ02) $(OBJ03) $(OBJ04) $(OBJ05) $(OBJ06) $(OBJ07)
OBJB= $(OBJ08) $(OBJ09) $(OBJ10) $(OBJ11) $(OBJ12) $(OBJ13) $(OBJ14) $(OBJ15)

OBJ= $(OBJA) $(OBJB)

# for regular (no profiling)
OBJS= $(CRT0) $(OBJ) $(CRTC)
# for 'prof(1)'
MOBJS= $(MCRT0) $(OBJ) $(CRTC)
# for 'gprof(1)'
GOBJS= $(GCRT0) $(OBJ) $(CRTC)




.SUFFIXES:	.ls .i


default:		$(T).x

all:			$(ALL)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

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
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBDIRS) $(LIBS) $(CRTN) > $(T).lm

$(T).prof:		$(OBJ) Makefile
	makedate > makedate.c
	$(CC) -c makedate.c
	$(LD) -o $@ $(LDFLAGS) $(MOBJS) $(LIBDIRS) $(LIBS) $(CRTN) > $(T).lm

$(T).gprof:		$(OBJ) Makefile
	makedate > makedate.c
	$(CC) -c makedate.c
	$(LD) -o $@ $(LDFLAGS) $(GOBJS) $(LIBDIRS) $(LIBS) $(CRTN) > $(T).lm

$(T).$(OFF) $(OFF):	$(T).x
	cp -p $(T).x $(T).$(OFF)

$(T).nm nm:		$(T).x
	$(NM) $(NMFLAGS) $(T).x > $(T).nm

strip:			$(T).x
	strip $(T).x
	rm -f $(T).$(OFF) $(T)

install:		$(ALL)
	rm -f $(BINDIR)/$(T).$(OFF)
	bsdinstall $(ALL) $(BINDIR)

install.$(OFF):		$(T) $(T).$(OFF)
	bsdinstall $(T) $(T).$(OFF) $(BINDIR)

install-raw:		$(T).x
	cp -p $(T).x $(T)
	rm -f $(BINDIR)/$(T).$(OFF)
	bsdinstall $(T) $(BINDIR)

clean:			again
	rm -f *.o

again:
	rm -f $(ALL) $(T) $(T).x $(T).$(OFF)



main.o:		main.c $(INCS)

whatinfo.o:	whatinfo.c config.h

proginfo.o:	proginfo.c $(INCS)

tracedata.o:	tracedata.c tracedata.h $(INCS)

filtercalls.o:	filtercalls.c $(INCS)

analyze.o:	analyze.c $(INCS)

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

convert.o:	convert.c $(INCS)

