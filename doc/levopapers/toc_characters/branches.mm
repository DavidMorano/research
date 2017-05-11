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
.ND "January 19, 2002"
.\"_
.OH "'Levo''Branch Data'"
.EH "'Branch Data''Levo'"
.OF "'''\\*(DT'"
.EF "'\\*(DT'''"
.\"_
.\"_
.TL
SpecInt Branch Data
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
.\"_ .PI levo2.eps 8i,6i,0i,0i a270
.\"_
.\"_
.H 1 "introduction"
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
.nr Si 15
.DS I
.so bzip2r.bstats
.sp 1
.FG "BZIP2 Statistics"
.TAG BZIP2_S
.DE
.SP 2
.\"_
.nr Si 15
.DS I
.so parser.bstats
.sp 1
.FG "PARSER Statistics"
.TAG PARSER_S
.DE
.SP 2
.\"_
.nr Si 15
.DS I
.so gzip.bstats
.sp 1
.FG "GZIP Statistics"
.TAG GZIP_S
.DE
.SP 2
.\"_
.nr Si 15
.DS I
.so gap.bstats
.sp 1
.FG "GAP Statistics"
.TAG GAP_S
.DE
.SP 2
.\"_
.nr Si 15
.DS I
.so go.bstats
.sp 1
.FG "GO Statistics"
.TAG GO_S
.DE
.SP 2
.SK
.\"_
.H 1 "Branch Path Lengths"
.P
The graphs in this section show percent density functions and
percent distributions for branch path
lengths in each of the five SpecInt programs that we have
characterized.
.\"_
Figures \_BZIP2_BPLDEN and \_BZIP2_BPLDIS are for the 
.B bzip2
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "bzip2r.bpden"
.G2
.SP 1
.FG "BZIP2 -- Branch Path Length Percent Density"
.TAG BZIP2_BPLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "bzip2r.bpdis"
.G2
.SP 1
.FG "BZIP2 -- Branch Path Length Percent Distribution"
.TAG BZIP2_BPLDIS
.DE
.\"_
Figures \_PARSER_BPLDEN and \_PARSER_BPLDIS are for the 
.B parser
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "parser.bpden"
.G2
.SP 1
.FG "PARSER -- Branch Path Length Percent Density"
.TAG PARSER_BPLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "parser.bpdis"
.G2
.SP 1
.FG "PARSER -- Branch Path Length Percent Distribution"
.TAG PARSER_BPLDIS
.DE
.\"_
Figures \_GZIP_BPLDEN and \_GZIP_BPLDIS are for the 
.B gzip
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "gzip.bpden"
.G2
.SP 1
.FG "GZIP -- Branch Path Length Percent Density"
.TAG GZIP_BPLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "gzip.bpdis"
.G2
.SP 1
.FG "GZIP -- Branch Path Length Percent Distribution"
.TAG GZIP_BPLDIS
.DE
.\"_
Figures \_GAP_BPLDEN and \_GAP_BPLDIS are for the 
.B gap
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "gap.bpden"
.G2
.SP 1
.FG "GAP -- Branch Path Length Percent Density"
.TAG GAP_BPLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "gap.bpdis"
.G2
.SP 1
.FG "GAP -- Branch Path Length Percent Distribution"
.TAG GAP_BPLDIS
.DE
.\"_
Figures \_GO_BPLDEN and \_GO_BPLDIS are for the 
.B go
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "go.bpden"
.G2
.SP 1
.FG "GO -- Branch Path Length Percent Density"
.TAG GO_BPLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch path length"
coord x 0,40 y 0.0,1.0
ticks bot out at 0, 10, 20, 30, 40
draw solid dot
copy "go.bpdis"
.G2
.SP 1
.FG "GO -- Branch Path Length Percent Distribution"
.TAG GO_BPLDIS
.DE
.\"_
.H 1 "Conditional Branch Domain Sizes"
.P
The graphs in this section show percent density functions and
percent distributions for conditional branch domain
sizes in each of the five SpecInt programs that we have
characterized.
.\"_
Figures \_BZIP2_BTLDEN and \_BZIP2_BTLDIS are for the 
.B bzip2
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "bzip2r.btden"
.G2
.SP 1
.FG "BZIP2 -- Branch Domain Size Percent Density"
.TAG BZIP2_BTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "bzip2r.btdis"
.G2
.SP 1
.FG "BZIP2 -- Branch Domain Size Percent Distribution"
.TAG BZIP2_BTLDIS
.DE
.\"_
Figures \_PARSER_BTLDEN and \_PARSER_BTLDIS are for the 
.B parser
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "parser.btden"
.G2
.SP 1
.FG "PARSER -- Branch Domain Size Percent Density"
.TAG PARSER_BTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "parser.btdis"
.G2
.SP 1
.FG "PARSER -- Branch Domain Size Percent Distribution"
.TAG PARSER_BTLDIS
.DE
.\"_
Figures \_GZIP_BTLDEN and \_GZIP_BTLDIS are for the 
.B gzip
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gzip.btden"
.G2
.SP 1
.FG "GZIP -- Branch Domain Size Percent Density"
.TAG GZIP_BTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gzip.btdis"
.G2
.SP 1
.FG "GZIP -- Branch Domain Size Percent Distribution"
.TAG GZIP_BTLDIS
.DE
.\"_
Figures \_GAP_BTLDEN and \_GAP_BTLDIS are for the 
.B gap
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gap.btden"
.G2
.SP 1
.FG "GAP -- Branch Domain Size Percent Density"
.TAG GAP_BTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gap.btdis"
.G2
.SP 1
.FG "GAP -- Branch Domain Size Percent Distribution"
.TAG GAP_BTLDIS
.DE
.\"_
Figures \_GO_BTLDEN and \_GO_BTLDIS are for the 
.B go
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "go.btden"
.G2
.SP 1
.FG "GO -- Branch Domain Size Percent Density"
.TAG GO_BTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "go.btdis"
.G2
.SP 1
.FG "GO -- Branch Domain Size Percent Distribution"
.TAG GO_BTLDIS
.DE
.\"_
.H 1 "Simple Single-Sides Hammock Branch Domain Sizes"
.P
The graphs in this section show percent density functions and
percent distributions for simple single-sided conditional branch domain
sizes in each of the five SpecInt programs that we have
characterized.
.\"_
Figures \_BZIP2_HTLDEN and \_BZIP2_HTLDIS are for the 
.B bzip2
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "bzip2r.htden"
.G2
.SP 1
.FG "BZIP2 -- SS-Hammock Domain Size Percent Density"
.TAG BZIP2_HTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "bzip2r.htdis"
.G2
.SP 1
.FG "BZIP2 -- SS-Hammock Domain Size Percent Distribution"
.TAG BZIP2_HTLDIS
.DE
.\"_
Figures \_PARSER_HTLDEN and \_PARSER_HTLDIS are for the 
.B parser
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "parser.htden"
.G2
.SP 1
.FG "PARSER -- SS-Hammock Domain Size Percent Density"
.TAG PARSER_HTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "parser.htdis"
.G2
.SP 1
.FG "PARSER -- SS-Hammock Domain Size Percent Distribution"
.TAG PARSER_HTLDIS
.DE
.\"_
Figures \_GZIP_HTLDEN and \_GZIP_HTLDIS are for the 
.B gzip
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gzip.htden"
.G2
.SP 1
.FG "GZIP -- SS-Hammock Domain Size Percent Density"
.TAG GZIP_HTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gzip.htdis"
.G2
.SP 1
.FG "GZIP -- SS-Hammock Domain Size Percent Distribution"
.TAG GZIP_HTLDIS
.DE
.\"_
Figures \_GAP_HTLDEN and \_GAP_HTLDIS are for the 
.B gap
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gap.htden"
.G2
.SP 1
.FG "GAP -- SS-Hammock Domain Size Percent Density"
.TAG GAP_HTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "gap.htdis"
.G2
.SP 1
.FG "GAP -- SS-Hammock Domain Size Percent Distribution"
.TAG GAP_HTLDIS
.DE
.\"_
Figures \_GO_HTLDEN and \_GO_HTLDIS are for the 
.B go
benchmark program.
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "go.htden"
.G2
.SP 1
.FG "GO -- SS-Hammock Domain Size Percent Density"
.TAG GO_HTLDEN
.DE
.\"_
.DS
.G1
frame ht 2 wid 4.5
label left "percent" "bracnhes" left .1
label bot "SS-hammock branch domain size"
coord x 0,200 y 0.0,1.0
ticks bot out at 0, 50, 100, 150, 200
draw solid dot
copy "go.htdis"
.G2
.SP 1
.FG "GO -- SS-Hammock Domain Size Percent Distribution"
.TAG GO_HTLDIS
.DE
.\"_
.\"_
.\"_
.\"_ force odd page
.OP
.\"_ table of contents
.TC
.\"_
.\"_
