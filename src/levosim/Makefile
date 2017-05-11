# MAKEFILE

T= levosim

ALL= $(T)

SRCROOT= $(LOCAL)
DSTROOT= $(LEVO)


INCDIR= $(SRCROOT)/include
LIBDIR= $(SRCROOT)/lib

BINDIR= $(DSTROOT)/bin

#LDCRTDIR= /opt/SUNWspro/WS6/lib
#LDCRTDIR= /opt/SUNWspro/SC5.0/lib
#LDCRTDIR= /opt/SUNWspro/SC4.0/lib
#LDCRTDIR= /opt/SUNWspro/lib
LDCRTDIR= $(SRCROOT)/lib


#CC= gcc
#CCOPTS= -O3 -mv8 # -fpic
#CCOPTS= -g -O

# HyperSPARC
#CCOPTS= -xO5 -xtarget=ss20/hs22 -dalign -xdepend

# UltraSPARC
CCOPTS= -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend

# native
#CCOPTS= -xO5 -native -xsafe=mem -dalign -xdepend


DEF0= -D$(OSTYPE)=1 -DSOLARIS=8 -DPOSIX=1 -DPTHREAD=1
DEF1= 
DEF2= -D__EXTENSIONS__=1 -D_REENTRANT=1
DEF3= -D_POSIX_C_SOURCE=199506L 
DEF4= -D_POSIX_PTHREAD_SEMANTICS=1 -D_POSIX_PER_PROCESS_TIMER_SOURCE=1
DEF5=
#DEF5= -DDMALLOC=1 -DMALLOCLOG=1
DEF6= -DF_MASTERDEBUG=1
DEF7=

DEFS= $(DEF0) $(DEF1) $(DEF2) $(DEF3) $(DEF4) $(DEF5) $(DEF6) $(DEF7)


INCDIRS= -I$(INCDIR)

CPPFLAGS= $(DEFS) $(INCDIRS)
CFLAGS= $(CCOPTS)

#LD= $(CC)
#LD= cc
#LD= ild
LD= ld


LDFLAGS= -m -R$(LIBDIR)/$(OFD):$(LIBDIR)
LIBDIRS= -L$(LEVOSIM)/lib -L$(LIBDIR)/$(OFD) -L$(LIBDIR)


LIB0=
LIB1= -Bstatic -ldam -Bdynamic
LIB2=
#LIB3= -Bstatic -lb -luc -Bdynamic -ldmallocth
LIB3= -Bstatic -lb -luc -Bdynamic
LIB4= -Bstatic -lu -Bdynamic
LIB5= -L$(GNU)/lib -lgcc
LIB6= -lposix4 -lpthread -lsocket -lnsl -lm
LIB7= -lc

LIBS= $(LIB0) $(LIB1) $(LIB2) $(LIB3) $(LIB4) $(LIB5) $(LIB6) $(LIB7)


CRTI= $(LDCRTDIR)/crti.o
CRT1= $(LDCRTDIR)/crt1.o
CRTN= $(LDCRTDIR)/crtn.o

CRT0= $(CRTI) $(CRT1)
CRTC= makedate.o

CRT= $(CRTI) $(CRT1) $(CRTN)

LINT= lint
LINTFLAGS= -uxn

NM= nm
NMFLAGS= -xs

AR= ar

CPP= cpp


INCS=


OBJ00=
OBJ01= frame.o
OBJ02= mips.o
OBJ03=
OBJ04= simple.o
OBJ05=
OBJ06= machine.o memory.o iwindow.o
OBJ07=
OBJ08=
OBJ09= malloclog.o sigdump.o sigdumper.o sigdumpmsg.o
OBJ10= fmeanvaral.o mkfname2.o
OBJ11=
OBJ12=
OBJ13=
OBJ14=

OBJA= $(OBJ00) $(OBJ01) $(OBJ02) $(OBJ03) $(OBJ04) $(OBJ05) $(OBJ06) $(OBJ07) 
OBJB= $(OBJ08) $(OBJ09) $(OBJ10) $(OBJ11) $(OBJ12) $(OBJ13) $(OBJ14) $(OBJ15)

OBJ= $(OBJA) $(OBJB)

OBJS= $(CRT0) $(OBJ) $(CRTC)


.SUFFIXES:		.ls .i .cx .cs


default:		below $(T).x

all:			below $(ALL)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

.cc.o:
	$(GPP) -c $(CFLAGS) $(CPPFLAGS) $<

.c.ln:
	$(LINT) -c -u $(CPPFLAGS) $<

.c.ln:
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $<

.c.i:
	$(CPP) $(CPPFLAGS) $< > $(*).i


$(T):			$(T).ee
	cp -p $(T).ee $(T)

$(T).x:			$(OBJ) Makefile
	makedate -m $(T) > makedate.c
	$(CC) -c makedate.c
	rm -f $(T).x
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBDIRS) $(LIBS) $(CRTN) > $(T).lm

$(T).$(OFF) $(OFF):	below $(T).x
	cp -p $(T).x $(T).$(OFF)

$(T).nm:		below $(T).x
	$(NM) $(NMFLAGS) $(T).x > $(T).nm

strip:			below $(T).x
	strip $(T).x
	rm -f $(T).$(OFF) $(T)

install:		below $(ALL)
	rm -f $(BINDIR)/$(T).$(OFF)
	bsdinstall $(ALL) $(BINDIR)

install.raw:		below $(T).x
	cp -p $(T).x $(T)
	rm -f $(BINDIR)/$(T).$(OFF)
	bsdinstall $(T) $(BINDIR)

again:
	rm -fr $(ALL) $(T) $(T).$(OFF) $(T).x

clean:			again
	rm -f *.o */*.o
	makebelow clean

cleaner:		clean
	rm -f $(LEVOSIM)/include/* */upincs */up

control:
	(uname -n ; date) > Control

incs:			belowincs

below:
	makebelow upincs up

belowincs:
	makebelow upincs


frame.o:		$(*)/$(@)
	cd $(*) ; make up

mips.o:			$(*)/$(@)
	cd $(*) ; make up

mint.o:			$(*)/$(@)
	cd $(*) ; make up

simple.o:		$(*)/$(@)
	cd $(*) ; make up

machine.o:		$(*)/$(@)
	cd $(*) ; make up

iwindow.o:		$(*)/$(@)
	cd $(*) ; make up

memory.o:		$(*)/$(@)
	cd $(*) ; make up

hdb.o:			hdb.c hdb.h

logfile.o:		logfile.c logfile.h

sigdumpmsg.o:		sigdumpmsg.c sigdumpmsg.h

sigdumper.o:		sigdumper.c sigdumpmsg.h


