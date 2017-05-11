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
.\"_ .ND "October 2, 2001"
.ND "August 26, 2002"
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
.PI levo2.eps 8i,6i,0i,0i a270
.\"_
.\"_
.H 1 "Introduction"
.P
This document just serves to present some branch data gathered
from the SpecInt-2000 and SpecInt-95 benchmark suites.
.\"_
.H 1 "General Statistics"
.P
General statistics for each of the five benchmark programs
are presented in Figures \_BZIP2_S through \_GO_S below.
All data, here and below, are for executing 500 million 
instructions of each program after skipping the first 100
million instructions.  All counts of events are dynamic 
unless specifically mentioned otherwise.
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so bzip2_bp.etstats
.S +1
.sp 1
.FG "BZIP2 Statistics"
.TAG BZIP2_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so compress_bp.etstats
.S +1
.sp 1
.FG "COMPRESS Statistics"
.TAG COMPRESS_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so crafty_bp.etstats
.S +1
.sp 1
.FG "CRAFTY Statistics"
.TAG CRAFTY_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so gcc_bp.etstats
.S +1
.sp 1
.FG "GCC Statistics"
.TAG GCC_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so go_bp.etstats
.S +1
.sp 1
.FG "GO Statistics"
.TAG GO_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so gzip_bp.etstats
.S +1
.sp 1
.FG "GZIP Statistics"
.TAG GZIP_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so ijpeg_bp.etstats
.S +1
.sp 1
.FG "IJPEG Statistics"
.TAG IJPEG_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so mcf_bp.etstats
.S +1
.sp 1
.FG "MCF Statistics"
.TAG MCF_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so parser_bp.etstats
.S +1
.sp 1
.FG "PARSER Statistics"
.TAG PARSER_S
.DE
.SP 2
.\"_
.\"_ indentation from left margin
.nr Si 15
.DS I
.S -1
.so vortex_bp.etstats
.S +1
.sp 1
.FG "VORTEX Statistics"
.TAG VORTEX_S
.DE
.SP 2
.\"_
.\"_
.SK
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
.DS
.\"_ file(page) h w pos off flags label
.BP bpden.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean Branch Path Length Density"
.TAG BPDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP bpdis.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean Branch Path Length Distribution"
.TAG BPDIS
.DE
.\"_
.\"_
.\"_
.H 1 "Conditional Branch Traget Distances"
.P
The graphs in this section show probability density functions and
probability distributions for conditional branch target distances
for the mean of the benchmark programs.
Different densisties and distributions are shown for forward
conditional branches and backward conditional branches.
.\"_
Figures \_BFTDEN and \_BFTDIS show the density and distribution
for the target distances for forward conditional branches.
.DS
.\"_ file(page) h w pos off flags label
.BP bftden.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean Forward Conditional Branch Target Distance Density"
.TAG BFTDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP bftdis.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean Forward Conditional Branch Target Distance Distribution"
.TAG BFTDIS
.DE
.\"_
Figures \_BBTDEN and \_BBTDIS show the density and distribution
target distances for backward conditional branches.
.DS
.\"_ file(page) h w pos off flags label
.BP bbtden.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean Conditional Branch Target Distance Density"
.TAG BBTDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP bbtdis.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean Conditional Branch Target Distance Distribution"
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
distances for the mean of all benchmark programs investigated.
.\"_
Figures \_HTDEN and \_HTDIS show the density and distribution
respectively.
.DS
.\"_ file(page) h w pos off flags label
.BP htden.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean SS-Hammock Target Distance Density"
.TAG HTDEN
.DE
.\"_
.DS
.\"_ file(page) h w pos off flags label
.BP htdis.eps 5.0i 8.0i c 0.0i a0
.EP
.SP 1
.FG "Mean SS-Hammock Target Distance Distribution"
.TAG HTDIS
.DE
.\"_
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
