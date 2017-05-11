# MAKEFILE

T= findhammocks

ALL= $(T) $(T).$(OFF)

SRCROOT=$(HOME)


BINDIR= $(SRCROOT)/bin
INCDIR= $(SRCROOT)/include
LIBDIR= $(SRCROOT)/lib

#LDCRTDIR= /opt/SUNWspro/WS6/lib
#LDCRTDIR= /opt/SUNWspro/SC5.0/lib
#LDCRTDIR= /opt/SUNWspro/SC4.0/lib
#LDCRTDIR= /opt/SUNWspro/lib
LDCRTDIR= $(SRCROOT)/lib


CC= gcc

CCOPTS= -O3 -mcpu=v9 # -fpic
# regular
#CCOPTS= -g
# profiling
#CCOPTS= -p

# HyperSPARC
#CCOPTS= -xO5 -xtarget=ss20/hs22 -dalign -xdepend

# UltraSPARC
# regular
#CCOPTS= -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend
# profiling
#CCOPTS= -p -xO5 -xtarget=ultra -xsafe=mem -dalign -xdepend


DEF0= -DOSNAME_$(OSNAME)=$(OSNUM) -DOSTYPE_$(OSTYPE)=1
DEF1=
DEF2=
DEF3=
DEF4=
DEF5= -DF_MASTERDEBUG=1
DEF6=
DEF7= $(LF_DEFS)

DEFS= $(DEF0) $(DEF1) $(DEF2) $(DEF3) $(DEF4) $(DEF5) $(DEF6) $(DEF7)

INCDIRS= -I$(INCDIR)

CPPFLAGS= $(DEFS) $(INCDIRS)

CFLAGS= $(CCOPTS)

#LD= $(CC)
#LD= cc
LD= ld

LDFLAGS= -R$(LIBDIR)

LIBDIRS= -L$(LIBDIR)

LIB0=
LIB1= -ldam -lb -luc
LIB2=
LIB3= -Bstatic -lu -Bdynamic
LIB4= -L$(GNU)/lib -lgcc
LIB5= 
LIB6= -lkstat -lsecdb -lproject -lrt -lpthread -lxnet -lsocket -lnsl
LIB7= -ldl -lc

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

INCS= config.h defs.h lmapprog.h


OBJ00=
OBJ01= main.o whatinfo.o proginfo.o printhelp.o
OBJ02=
OBJ03= process.o
OBJ04= recorder.o ssh.o
OBJ05= exectrace.o lmapprog.o mipsdis.o
OBJ06= ldecode.o lexec.o lflowgroup.o
OBJ07= vecstr_loadfile.o vecstr_env.o
OBJ08= 
OBJ09= 
OBJ10=
OBJ11=
OBJ12=
OBJ13= procargs.o procenv.o mkfname2.o
OBJ14= bopenroot.o getfname.o fmeanvaral.o
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



.SUFFIXES:		.ls .i


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


# special experiments
ssh.so:			ssh.o
	$(LD) -o $@ -G ssh.o $(LIBDIRS) $(LIBS)

hammock.o:
	$(LD) -o $@ -r ssh.o $(LIBDIRS) $(LIBS)


main.o:		main.c $(INCS)

whatinfo.o:	whatinfo.c config.h

proginfo.o:	proginfo.c $(INCS)

process.o:	process.c $(INCS) recorder.h ssh.h $(MIPSINCS)

lmapprog.o:	lmapprog.c lmapprog.h config.h defs.h

exectrace.o:	exectrace.c exectrace.h

recorder.o:	recorder.c recorder.h

ssh.o:		ssh.c ssh.h

ldecode.o:	ldecode.c ldecode.h $(MIPSINCS)

lexec.o:	lexec.c lexec.h $(MIPSINCS) dataval.h




