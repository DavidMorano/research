# MAKEFILE

DOC= tr0202


PRTFMT= prtfmt

INC0= nucar2.eps
INC1= levo.eps levo2.eps
INC2=
INC3=

INCS= $(INC0) $(INC1) $(INC2) $(INC3)



troff:		$(DOC).troff

ps:		$(DOC).ps

pdf:		$(DOC).pdf

dvi:		$(DOC).dvi

bbl:		$(DOC).bbl

print:		$(DOC).mm $(INC)
	$(PRTFMT) $(DOC).mm

clean:
	rm -f $(DOC).ps $(DOC).c $(DOC).troff


$(DOC).troff:	$(DOC).mm
	$(PRTFMT) -Z $(DOC).mm > $(DOC).troff

$(DOC).ps::	$(DOC).tex $(INCS)
	dvips $(DOC)

$(DOC).ps::	$(DOC).troff $(INCS)
	dpost -x 0.25 $(DOC).troff | postreverse -r | fixdwbps > $(DOC).ps

$(DOC).pdf:	$(DOC).ps
	ps2pdf $(DOC).ps $(DOC).pdf

$(DOC).bbl:	$(DOC).bib
	bibtex $(DOC)

$(DOC).dvi:	$(DOC).tex
	latex $(DOC)
	latex $(DOC)



