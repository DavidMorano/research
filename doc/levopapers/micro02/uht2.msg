x-reply-to: dmorano@ece.neu.edu
bcc: dmorano@ece.neu.edu (verify test loop-back address)
x-bcc: (David Morano) morano@ele.uri.edu
From: (David Morano) morano@ele.uri.edu
To: (Professor Uht) uht@ele.uri.edu
cc: (Professor D. Kaeli) dkaeli@ac.upc.es 
cc: (Alireza Khalafi) akhalafi@ece.neu.edu
cc: (Peter Zeno) pzeno@ECE.NEU.EDU
x-cc: (Tim Toolan) toolan@ele.uri.edu
x-cc: (Marcos de Alba) mdealba@ECE.NEU.EDU
x-cc: (Team Levo) levo <morano@computer.org>
x-cc: (Sean Langford) langfors@ele.uri.edu
x-cc: (Thomas Wenisch) iota@ele.uri.edu
x-cc: (Yuan Lin) liny@ele.uri.edu
x-cc: (Professor Kaeli) kaeli@ece.neu.edu
x-cc: (David Kaeli) dkaeli@ac.upc.es
x-cc: action@ele.uri.edu (URI ELE Action Help)
Subject: 

Gus,

Prof Kaeli wrote :

> Figure 7. I would get the 8.7 GHz line out of this figure.  This will
> upset any circuit person.

:-)
This thing would appear to be a minor point in the context of the whole
paper but this might grate on those people that have had to struggle
with clock periods in silicon.  At a minimum, it is not clear how many
gate delays in a clock period you are assuming and what a gate delay is
in the target silicon technology.

As you know, routing, logic loading, gate prop delay, metal thickness
(or if the signal also passes through segments of silicide or
polysilicon), and usually least appreciated -- stray layout
capcitances, all influence how far in distance a particular signal
goes.  Uusually, only an analog extraction of the silicon layout and
then that put through either a switch-level (or more accurate full
analog) simulator of some sort can provide an approximate estimate.
Maybe there are not that many silicon guys reviewing Micro but if there
are and you want to keep them on-board (if our paper draws one of them
for review), you might want to somehow soften the assertion by either
equivocating on the frequency or the langth of the arrow.

Dave M.
