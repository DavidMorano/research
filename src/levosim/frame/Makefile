# MAKEFILE

T= frame
 
ALL= $(T) $(T).$(OFF)

SRCROOT= $(LOCAL)
DSTROOT= $(LEVO)


INCDIR= $(SRCROOT)/include
LIBDIR= $(SRCROOT)/lib

BINDIR= $(DSTROOT)/bin

LDCRTDIR= /opt/SUNWspro/WS6/lib
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
INCDIRS= -I$(LEVOSIM)/include -I$(INCDIR) 

CPPFLAGS= $(DEFS) $(INCDIRS)
CFLAGS= $(CCOPTS) 

#LD= $(CC)
#LD= cc
#LD= ild
LD= ld
LDFLAGS= -m -R$(LIBDIR)/$(OFD):$(LIBDIR)
LIBDIRS= -L$(LIBDIR)/$(OFD) -L$(LIBDIR)

LIB0=
LIB1= -Bstatic -ldam -Bdynamic
LIB2=
LIB3= -Bstatic -lb -luc -Bdynamic
LIB4= -Bstatic -lu -Bdynamic
LIB5= -L$(GNU)/lib -lgcc
LIB6= -lposix4 -lpthread -lsocket -lnsl
LIB7= -lc

LIBS= $(LIB0) $(LIB1) $(LIB2) $(LIB3) $(LIB4) $(LIB5) $(LIB6) $(LIB7)

CRTI= $(LCDRTDIR)/crti.o
CRT1= $(LCDRTDIR)/crt1.o
CRTN= $(LCDRTDIR)/crtn.o

CRT0= $(CRTI) $(CRT1)
CRTC= makedate.o

CRT= $(CRTI) $(CRT1) $(CRTN)

LINT= lint
LINTFLAGS= -uxn

NM= nm
NMFLAGS= -xs

AR= ar

CPP= cpp




INC0= config.h defs.h proginfo.h paramfile.h
INC1= mintinfo.h lsim.h elfmips.h plainq.h lmapprog.h
INC2= shiftregl.h
INC3= statemips.h
INC4= traceinfo.h exectrace.h xmlinfo.h fcount.h
INC5=
INC6= debugwin.h
INC7=

INCS= $(INC0) $(INC1) $(INC2) $(INC3) $(INC4) $(INC5) $(INC6) $(INC7)


INC0_PROCESS= config.h defs.h lsim.h 
INC1_PROCESS= $(LEVOSIM)/include/syscalls.h
INC2_PROCESS= 
INC3_PROCESS= 

INCS_PROCESS= $(INC0_PROCESS) $(INC1_PROCESS) $(INC2_PROCESS) $(INC3_PROCESS)



OBJ00=
OBJ01= main.o whatinfo.o proginfo.o
OBJ02= configfile.o varsub_extras.o expander.o
OBJ03= procfileenv.o procfilepaths.o strfieldcmp.o
OBJ04= 
OBJ05= process.o paramfile.o
OBJ06=
OBJ07= debug.o util.o varsub_dumpfd.o debugwin.o
OBJ08=
OBJ09= lsim.o plainq.o lmapprog.o
OBJ10= getinterleave.o getnumbuses.o
OBJ11= 
OBJ12= shiftregl.o procenv.o procargs.o
OBJ13= traceinfo.o exectrace.o xmlinfo.o 
OBJ14= fcount.o readsginm.o
OBJ15= bopenroot.o getfname.o

OBJA= $(OBJ00) $(OBJ01) $(OBJ02) $(OBJ03) $(OBJ04) $(OBJ05) $(OBJ06) $(OBJ07) 
OBJB= $(OBJ08) $(OBJ09) $(OBJ10) $(OBJ11) $(OBJ12) $(OBJ13) $(OBJ14) $(OBJ15)

OBJ= $(OBJA) $(OBJB)

OBJS= $(CRT0) $(OBJ) $(CRTC)



.SUFFIXES:	.ls .i


default:		$(T).o

upincs:			$(INCS) Makefile
	makenewer -d $(LEVOSIM)/include $(INCS) -t $@

up:			upincs $(T).o
	makenewer $(T).o .. -t $@

all:			$(ALL)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

.c.ln:
	$(LINT) -c -u $(CPPFLAGS) $<

.c.ls:
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $<


$(T):			$(T).o Makefile

$(T).o:			$(OBJ) Makefile
	makedate $(T) > makedate.c
	$(CC) -c makedate.c
	$(LD) -m -r -o $@ $(OBJ) $(CRTC) > $(T).lm

$(T).x:			$(T).o Makefile
	$(LD) -o $@ $(LDFLAGS) $(CRT0) $(T).o $(CRTC) $(CRTN) \
			$(LIBDIRS) $(LIBS) > $(T).lm

$(T).$(OFF) $(OFF):	$(T).x
	cp -p $(T).x $(T).$(OFF)

strip:			$(T).x
	strip $(T).x
	rm -f $(T).$(OFF)

install:		$(ALL)
	bsdinstall $(ALL) $(BINDIR)

again:
	rm -fr up $(ALL) $(T) $(T).x

clean:			again
	rm -f up upincs *.o $(ALL)
	makerm -d $(LEVOSIM)/include $(INCS)

control:
	(uname -n ; date) > Control




main.o:			main.c config.h defs.h

whatinfo.o:		whatinfo.c config.h

proginfo.o:		proginfo.c config.h defs.h

process.o:		process.c $(INCS_PROCESS)

lsim.o:			lsim.c config.h defs.h lsim.h

plainq.o:		plainq.c plainq.h

exectrace.o:		exectrace.c exectrace.h

xmlinfo.o:		xmlinfo.c xmlinfo.h

fcount.o:		fcount.c fcount.h

readsginm.o:		readsginm.c fcount.h



