.\"_ characters
.\"_
.\"_
.\"_ starting section number is (this + one)
.\"_ .nr H1 2
.\"_
.\"_ heading level saved for table of contents
.nr Cl 3
.\"_
.\"_
.\"_ heading fonts
.ds HF 3 3 3 2 2 2 2
.\"_
.\"_
.EQ
delim $$
.EN
.\"_
.\"_
.\"_ .ND "October 2, 2001"
.ND "August 29, 2002"
.\"_
.OH "'Levo''Branch and Register Data'"
.EH "'Branch and Register Data''Levo'"
.OF "'''\\*(DT'"
.EF "'\\*(DT'''"
.\"_
.\"_
.TL
SpecInt Branch and Register Data
.\"_
.AF "Northeastern University"
.AU "David Morano"
.\"_
.\"_
.\"_ released paper style
.MT 4
.\"_
.\"_
.\"_ default text width on page is 6 inches !
.\"_ file(page) height,width,yoffset,xoffset flags
.PI levo3.eps 8i,6i,0i,0i a270
.\"_
.\"_
.H 1 "Introduction"
.P
This document serves to present some branch data gathered
from the SpecInt-2000 and SpecInt-95 benchmark suites.
Data was accumulated over ten benchmarks programs.
Seven programs were taken from the SpecInt-2000 suite and
three programs were taken from the SpecInt-95 suite.
The following SpecInt-2000 programs were used for gathering
data :
.BL
.LI
bzip2
.LI
crafty
.LI
gcc
.LI
gzip
.LI
mcf
.LI
parser
.LI
vortex
.LE
.SP
The following SpecInt-95 programs were also used :
.BL
.LI
go
.LI
ijpeg
.LI
compress
.LE
.SP
All programs were compiled for the MIPS-1 ISA and used
the native SGI compiler under IRIX 6.4 with the standard
optimization turned on.
All data collected was on executing 500 million instructions
after skipping the first 100 million instructions.
All microarchitecture structures and data accumulation apparatus
were allowed to warm 
up during the first 100 million instructions.
.\"_
.\"_
.\"_
.H 1 "Branch Path Lengths"
.P
The graphs in this section show percent density functions and
percent distributions for branch path
lengths in each of the five SpecInt programs that we have
characterized.
.\"_
Figures \_BPDEN and \_BPDIS give the probability density and
the probability distribution (respectively) for branch paths
versus branch path lengths.
The lower bound on the mean branch path length is \fB9.4\fP and the 
lower-bound standard deviation is \fB10.4\fP.
.DS
.\"_ file(page) h w pos off flags label
.BP bpden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Branch Path Length Density"
.TAG BPDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP bpdis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Branch Path Length Distribution"
.TAG BPDIS
.DE
.\"_
.\"_
.\"_
.H 1 "Conditional Branch Traget Distances"
.P
The graphs in this section show probability density functions and
probability distributions for conditional branch target distances
over all of the benchmark programs.
Different densisties and distributions are shown for forward
conditional branches and backward conditional branches.
Although backward conditional branches are most usually
associated with program loops, this does not have to be the
case.  
Loops can, of course, be effected with forward conditional
branches and an unconditional branch, with or without possible
backward conditional branches in the body of the loop.
Loops may there for be indicated by strong peaks in either
forward or backward conditional branches.
.\"_
Figures \_BFTDEN and \_BFTDIS show the density and distribution
for the target distances for forward conditional branches.
The lower bound of the
mean forward conditional branch target distance is \fB31.3\fP and the 
lower-bound standard deviation is \fB91.6\fP.
.DS
.\"_ file(page) h w pos off flags label
.BP bftden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Forward Conditional Branch Target Distance Density"
.TAG BFTDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP bftdis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Forward Conditional Branch Target Distance Distribution"
.TAG BFTDIS
.DE
.\"_
Figures \_BBTDEN and \_BBTDIS show the density and distribution
target distances for backward conditional branches.
The lower bound of the
mean conditional branch target distance is \fB41.8\fP and the 
lower-bound standard deviation is \fB105.1\fP.
.DS
.\"_ file(page) h w pos off flags label
.BP bbtden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Conditional Backward Branch Target Distance Density"
.TAG BBTDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP bbtdis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Conditional Backward Branch Target Distance Distribution"
.TAG BBTDIS
.DE
.\"_
.\"_
.\"_
.\"_
.H 1 "Simple Single-Sided Hammock Branch Target Distances"
.P
The graphs in this section show a probability density function and
distribution for simple single-sided Hammock conditional branch target
distances for all of the benchmark programs investigated.
Simple single-sided Hammock conditional branches have only forward
going targets.  
A minimal simple Hammock conditional branch with
a backward going target would have to be similar to a double
sided Hammock branch.
.\"_
Figures \_HTDEN and \_HTDIS show the density and distribution
respectively.
The lower bound of the
mean S-S Hammock branch target distance is \fB5.6\fP and the 
lower-bound standard deviation is \fB3.6\fP.
.DS
.\"_ file(page) h w pos off flags label
.BP htden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative SS-Hammock Target Distance Density"
.TAG HTDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP htdis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative SS-Hammock Target Distance Distribution"
.TAG HTDIS
.DE
.\"_
.\"_
.\"_
.\"_
.H 1 "Register Access Statistics"
.P
In this section, we show register statistics for
the register useful-lifetimes as well as for the def-use intervals.
Further, information on which registers are used most often by
the code is also provided.  For the register useful-lifetimes and
def-use intervals, both a density and a distribution over the
associated intervals is shown
over all benchmarks programs.  For the register read and write
usage by the programs, densities on register addresses are 
provided (one for reads and the other for writes).
.\"_
.P
Figures \_RLIFEDEN and \_RLIFEDIS show the density and distribution
respectively for the register useful-lifetimes over the lifetime interval
as measured in instructions.
The lower bound of the
mean register useful-lifetime (in instructions) is \fB25.4\fP and the 
lower-bound standard deviation is \fB87.8\fP.
.DS
.\"_ file(page) h w pos off flags label
.BP rlifeden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Register Useful-Lifetime Density"
.TAG RLIFEDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP rlifedis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Register Useful-Lifetime Distribution"
.TAG RLIFEDIS
.DE
.\"_
.\"_
.P
Figures \_RUSEDEN and \_RUSEDIS show the density and distribution
respectively for the register def-use intervals
as measured in instructions.
Registers $ r0 $ and $ r28 $ are exluded from this
def-use data since they are only read-only or only written
once during the entire execution of a program.
The lower bound of the
mean register def-use interval (in instructions) is \fB79.3\fP and the 
lower-bound standard deviation is \fB326.3\fP.
.DS
.\"_ file(page) h w pos off flags label
.BP ruseden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Register Def-Use Density"
.TAG RUSEDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP rusedis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Register Def-Use Distribution"
.TAG RUSEDIS
.DE
.\"_
.\"_
.P
Figures \_READDEN and \_WRITEDEN show the densities over
register addresses for register reads and register writes
respectively.  
On the MIPS ISA, registers $ r0 $ through $ r31 $ are integer registers.
Registers $ r64 $ through $ r85 $ are floating point registers.
Other rgisters (which do not get much use) are various status
or control registers (usually for floating pointer operations).
It should be noted that register $ r0 $ is hardwired to
the value zero and is a read-only register.
For reference, register $ r29 $ is the stack pointer.
Further, register $ r28 $ is the pointer to the global offset
table and is only written once at the start of the program.
The data shown is only for the execution of 500 million instructions
after skipping the first 100 million, so the single write of register
$ r28 $ would not show up in this data anyway.
Registers $ r26 $ and $ r27 $ are never written or read
by any of the benchmark programs investigated (the ten that we executed).
.DS
.\"_ file(page) h w pos off flags label
.BP rreadden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Register Read Density Over Register Address"
.TAG READDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP rwriteden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Register Write Density Over Register Address"
.TAG WRITEDEN
.DE
.\"_
.\"_
.\"_
.\"_
.\"_ force odd page
.OP
.\"_ table of contents
.TC
.\"_
.\"_
