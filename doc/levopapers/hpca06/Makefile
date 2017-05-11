# MAKEFILE

DOC= hpca06


PRTFMT= prtfmt

INC0= nucar2.eps
INC1= levo.eps levo2.eps
INC2=
INC3=
INC4=
INC5= bzip2_rrint.eps
INC6=

INCS= $(INC0) $(INC1) $(INC2) $(INC3) $(INC4) $(INC5) $(INC6)



dit:		$(DOC).dit

ps:		$(DOC).ps

pdf:		$(DOC).pdf

print:		$(DOC).mm $(INCS)
	$(PRTFMT) $(DOC).mm

clean:
	rm -f $(DOC).ps $(DOC).c $(DOC).dit


$(DOC).dit:	$(DOC).mm $(INCS)
	$(PRTFMT) -Z $(DOC).mm > $(DOC).dit

$(DOC).ps:	$(DOC).dit $(INCS)
	dpost -x 0.25 $(DOC).dit | postreverse -r | fixdwbps > $(DOC).ps

$(DOC).pdf:	$(DOC).ps
	ps2pdf $(DOC).ps $(DOC).pdf


bpden.eps:	bpden.dwb
	grap bpden.dwb | pic | troff -Tpost | dpost | addbbox > bpden.eps



