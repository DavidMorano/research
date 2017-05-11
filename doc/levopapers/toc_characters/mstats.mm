.\"_ mstats
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
Memory Access Statistics
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
.H 1 "Memory Access Statistics"
.P
In this section, we show memory statistics for
the memory useful-lifetimes as well as for the def-use intervals.
Following are graphs of the cumulative data for :
.BL 10
.LI
memory access-use intervals (inter-reference gaps)
.LI
memory useful-lifetimes (def-last-useful-use intervals)
.LI
memory def-use intervals
.LE
.sp 0.5
.LE
.SP
Both a density and a distribution over the
associated intervals is shown.
The data is cummulative over all programs and memory variables
of each program.
.\"_
.\"_
.\"_
.\"_
.P
Figures \_MRINTDEN and \_MRINTDIS show the density and distribution
respectively for the memory reads over instruction intervals.
An access is defined as either a read or a write.
So an access-usage interval is the interval in instructions
from either a read or a write to the next read of the same
variable.  
There an instance of an access-usage for each memory
read in the executed program.
.DS
.\"_ file(page) h w pos off flags label
.BP mrintden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Memory Access-Usage Density"
.TAG MRINTDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP mrintdis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Memory Access-Usage Distribution"
.TAG MRINTDIS
.DE
.\"_
.\"_
.\"_
.\"_
.\"_
.P
Figures \_MLIFEDEN and \_MLIFEDIS show the density and distribution
respectively for the memory writes over the useful-lifetime interval
as measured in instructions.
Our definition of \fiUseful-lifetime\fP 
(which is the same as that by Franklin and Sohi)
is the interval in dynamic instructions from a write of a variable
to the last useful read of the same variable.
There is an instance of a useful-lifetime associated with
each write of the executed programs.  However, tt should be noted
that the present data does not include the last write for which
the last useful read has not yet been determined. 
This would seem to be more appropriate than assuming that the
last executed read of a variable was indeed the same useful read (since
it is unknown whether there are further reads afterwards).
.DS
.\"_ file(page) h w pos off flags label
.BP mlifeden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Register Useful-Lifetime Density"
.TAG MLIFEDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP mlifedis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Memory Useful-Lifetime Distribution"
.TAG MLIFEDIS
.DE
.\"_
.\"_
.P
Figures \_MUSEDEN and \_MUSEDIS show the density and distribution
respectively for the memory reads over
intervals
as measured in instructions.
.DS
.\"_ file(page) h w pos off flags label
.BP museden.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Memory Def-Use Density"
.TAG MUSEDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP musedis.eps 3.0i 6.0i c 0.0i a0
.EP
.SP 1
.FG "Cumulative Memory Def-Use Distribution"
.TAG MUSEDIS
.DE
.\"_
.\"_
.\"_
.\"_
