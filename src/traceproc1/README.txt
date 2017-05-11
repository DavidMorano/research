TRACECMP

This program compares execution traces with each other.

Synopsis:

$ tracecmp <a.et> <b.et> [<option(s)>]

where:

-i {m,mv,r,rv}		ignore the specified items
-p <program>		MIPS program
-e <envfile>		program environment
-m <mode>		program mode
-o <dumpopt(s)>		dump options
-s <start>:<num>	instructions to be dumped
-f <filtercalls>	file of names of call to filter out

program modes include:

analyze
dump
tracedump

