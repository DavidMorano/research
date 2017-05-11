SS4DAM


This is, or hopefully will become, the Levo simulator based on the
SimpleScalar tool-kit.  Unfortunately, only the most basic
Alpha-instruction execution part of SimpleScalar is really useable for
a Levo-like simulator.  The Levo-type machine model is just too finely
grained in its ILP to borrow components directly from anything in
SimpleScalar.


files		description
------------------------------------

sim-levo	the Levo simulator program
levo.sub	program to submit for execution
levo.conf	the configuration file for a Levo simulation run

levo-specific.h	Levo specific stuff
levo-las.[ch]	Levo AS
levo-lid.[ch]	Levo instruction decode

lflowgroup.[ch]	transaction data struction

traceinfo.[ch]	extra
exectrace.[ch]	extra




Important notes:


= Switching compilers

When switching between any compiler other than GCC (like for example
the Sun Microsystems native compiler) to the GCC compiler, several
files need to be recompiled even though their source code does not
change!  This is because special processing (through preprocessor
defines) is done when using the GCC compiler.  Why this is, who can
know?

You will notice which files need to be unconditionally recompiled
because they will be the ones that originate undefined references
to subroutines.


= Program Invocation Options

#
# selection syntax
# -s in=<start>:<number>
# -s in=20:30 -s ck=30:200
#

This ONLY sets the "selection."  The "selection" is used
to accumulate statistics over a "selected" area.

To control the maxium number of instructions and clocks to
be executed use :

-c cnum
-n [skipnum:]inum
-skip skipnum		<= this is "fast forward"





