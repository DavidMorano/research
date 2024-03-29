LIBCHEETAH

[Greetings, this is the README and man pages from the original Cheetah release
 by Rabin A. Sugumar and Santosh G. Abraham.  All cheetah sources are
 contained in the directory "libcheetah/".  The simulator "sim-cheetah"
 interfaces to the cheetah engine, and supports most of the options described
 below.]


Cheetah is a cache simulation package which can simulate various cache
configurations in a single pass through the address trace.
Specifically, Cheetah can simulate ranges of set-associative,
fully-associative or direct-mapped caches. Here are answers to some
common questions.

Why a package for cache simulation ?

It is fairly trivial to write a cache simulator which simulates one
cache at a time and outputs the miss ratio.  However, cache simulators
are typically used to simulate traces on several configurations, and
single cache simulators are inefficient and difficult to manage for
such purposes.

Cheetah uses extremely efficient single-pass algorithms to simulate
multiple cache configurations.  These algorithms exploit inclusion and
other properties that hold between cache configurations.  Therefore,
Cheetah has the potential to speed-up the cache evaluation process
significantly.

Additionally, Cheetah can simulate caches under OPT replacement [2].
Cache simulators for OPT are difficult to develop. Cheetah uses new
algorithms that we developed to perform single-pass simulation of
caches under OPT replacement efficiently.



How does Cheetah differ from other cache simulation packages like
'Dinero III' (developed by Mark Hill, and distributed with the
Hennessy-Patterson book on architecture) and 'Tycho' also developed by
Mark Hill ?

Dinero III (V3.3) simulates one set-associative cache configuration at
a time. In contrast, Cheetah simulates a range of set-associative or
fully-associative cache configurations at a time.  Further, Cheetah
can simulate OPT as well as LRU replacement; Dinero III can simulate
LRU, random and FIFO.  However, Dinero III simulates sub-block
placement, prefetch strategies and write effects. Currently, Cheetah
cannot simulate these features.
  
Tycho is a single pass simulator for multiple LRU set-associative
caches.  Tycho's capabilities are a subset of those of Cheetah.
Further, the algorithm [1] that Cheetah uses to do the set-associative
cache simulation is more efficient both in terms of simulation time
and memory requirements than the one used in Tycho.  Typically, on a
trace generated using the pixie tool (available on workstations based
on the MIPS processor), Cheetah is about six times faster than Tycho,
partly due to algorithmic improvements (2X) and partly due to
formatting overheads (3X).

The following publications describe the various algorithms used in
Cheetah in greater detail:

[1] Rabin A. Sugumar and Santosh G. Abraham, ``Efficient simulation
    of caches using binomial trees,'' Technical Report CSE-TR  1991.
[2] Rabin A. Sugumar and Santosh G. Abraham, ``Efficient simulation
    of caches under OPT replacement with applications to miss
    characterization,'' in the proceedings of the 1993 ACM SIGMETRICS
    conference.



What are the conditions of use ?

You are free to use this package as you wish. You should understand,
however, that although we have tried hard to ensure that the simulator
is accurate, we do not guarantee anything, and that this package is
distributed with absolutely no warranty.

You are also free to modify and distribute this package as you wish as
long as it is not for commercial gain. We ask that you retain our
copyright notice and make clear what your modifications were.
If you use the simulator extensively in any of your publications,
please reference one of the relevant papers above.

Also, if you use the simulator, we would like to hear from you to have a
rough idea of where it is being used. We could then inform you about
bug fixes and new versions of the package.  We would also like to hear
any comments you have on the simulator, and about any improvements you
make so that we can incorporate them in future releases.

We can be reached by email at rabin@eecs.umich.edu or sga@eecs.umich.edu .


***********************************************************************************

Files in the directory

  COPYRIGHT  Copyright notice
  README     This file
  VERSION    Version number
  Makefile   Make file
  main.c     Common user interface
  saclru.c   LRU set-associative cache simulator.
  faclru.c   LRU fully-associative cache simulator.
  sacopt.c   OPT set-associative cache simulator.
  facopt.c   OPT fully-associative cache simulator.
  ppopt.c    Preprocessor for OPT simulators.
  dmvl.c     Variable line size direct-mapped cache simulator.
  ascbin.c   Conversion program. ASCII to basic binary format.
  pixie.c    Routine that converts pixie format to simulator format.
  din.c      Routine that converts DIN format to simulator format.
  utils.c    Some common functions used by the simulators.

  orig_results   Result file used to test installation.
  trace.ccom.Z   A sample trace in ASCII format. To run this on cheetah

                  uncompress trace.ccom
                  ascbin < trace.ccom > trace_file
		  cheetah -Fbasic -ftrace_file [other options]

  cheetah.1         Man page. To view: 'nroff -man cheetah.1' .


Installation
  'make cheetah' will make the simulator.
  'make test' will make the simulator and also test it on the sample trace.

--

     cheetah(1)                                             cheetah(1)



     NAME
          cheetah - Single-pass simulator for direct-mapped, set-
          associative and fully associative caches.

     SYNTAX
          cheetah [ -options ]

     DESCRIPTION
          cheetah is a cache simulation package which can simulate
          various cache configurations.  It is a collection of
          single-pass simulators which simulate ranges of direct-
          mapped,  set-associative and fully-associative caches.  The
          input to cheetah is a memory address trace in one of three
          formats:

          (i) A binary format where the trace is a sequence of four
          byte addresses.  This is the format that is actually used in
          the simulator, and hence no internal conversion step is
          required. If the trace format you have is not among those
          supported by cheetah it is most efficient to convert it to
          the binary format for use with cheetah.

          (ii) The pixie format, where the trace is as output by the
          pixie tool available on MIPS processor based workstations.

          (iii) The DIN format, developed by Mark Hill, and used by
          earlier cache simulators such as Tycho and Dinero III.

          cheetah outputs the miss ratios of the various caches
          simulated.

          cheetah uses efficient single-pass simulation algorithms,
          and so in each simulation run it can determine the miss-
          ratios of a range of cache configurations.  It exploits
          inclusion and other relations that hold between cache
          configurations to significantly speedup the simulation.
          When many cache configurations need to be simulated on
          several traces the use of cheetah should greatly improve the
          efficiency of the process over running a large number of
          simulations with simple one-cache-at-a-time simulators.
          Tycho (Mark Hill 1989) is an earlier cache simulation
          package that also used single-pass simulation algorithms for
          simulating multiple LRU set-associative caches. The
          capabilities of cheetah are a superset of those of Tycho.
          Further, the algorithm that cheetah uses to simulate set-
          associative caches is more efficient both in terms of
          simulation time and memory requirements that the one used in
          Tycho.

          cheetah can simulate caches under LRU and OPT replacement
          strategies.  With OPT replacement the set-associative
          simulators permit cache bypass.  That is, on a miss the



     Page 1                                          (printed 1/21/97)






     cheetah(1)                                             cheetah(1)



          referenced line is installed in cache only if its priority
          is high enough.  OPT replacement is done by doing a limited
          look-ahead in the trace and fixing errors in the stack using
          a stack repair procedure.

          For further details on the algorithms used in cheetah please
          see the publications referenced below.

     OPTIONS
          (If the option is not specified, the corresponding parameter
          takes a default value in all cases.)

          cheetah has four major options which set the replacement
          strategy, the cache configuration, the trace format and the
          trace type.

          -R repl
               Used to specify the replacement strategy.  Replacement
               policies that are currently supported are lru and opt.
               (default lru )

          -C config
               Used to specify the configuration. Configuration can be
               one of

               fa        Simulate fully-associative caches.

               sa        Simulate set-associative caches.

               dm        Simulate direct-mapped caches.  Direct-mapped
                         should be specified only if direct-mapped
                         caches of varying line sizes but constant
                         size need to be simulated. Direct-mapped
                         caches of varying sizes may be simulated by
                         setting the configuration to set-associative
                         and the maximum associativity to one.

               The default configuration is sa.

          -F format
               Used to specify the trace format. Currently, the trace
               format can be one of

               pixie     Trace as generated by pixie.

               din       Trace in the DIN format (Mark Hill).

               basic     Trace is a sequence of four byte addresses.

               The default format is basic.

          -T type



     Page 2                                          (printed 1/21/97)






     cheetah(1)                                             cheetah(1)



               Used to specify the trace type.  i[nstruction] for
               instruction traces, d[ata] for data traces and
               u[nified] for unified instruction and data traces may
               be specified. This option is ignored for the basic
               trace format.  The default trace type is unified.

          Along with the above four major options, for each
          configuration a set of options are recognized that serve to
          set various parameter values.  Options that are valid for
          all configurations are first described, followed by the
          specific options for each of the three configurations.

          The following options are used for all configurations and
          set the input/output files and other global simulation
          parameters.

          -t number
               Specifies an upper limit on the number of addresses
               processed (default 1 billion).

          -f file
               Specifies the input file (default stdin).

          -o file
               Specifies the output file (default stdout).

          -p number
               Specifies the intervals at which simulation progress
               should be printed.  Useful for checking the status of
               the simulation (default 10 million).

          -s number
               Specifies the intervals at which intermediate results
               should be saved to the output file.

          The following options are used when the configuration is
          set-associative to set parameter values and ranges.  All
          logs are to base 2.

          -a number
               Specifies the log of the minimum number of sets
               (default 7).

          -b number
               Specifies the log of the maximum number of sets
               (default 14).

          -l number
               Specifies the log of the line size of the caches
               (default 4).

          -n number



     Page 3                                          (printed 1/21/97)






     cheetah(1)                                             cheetah(1)



               Specifies the log of the maximum degree of
               associativity (default 1).

          The following options are used when the configuration is
          fully-associative to set parameter values.

          -l number
               Specifies the log of the line size of the caches
               (default 4).

          -i number
               Specifies the cache size intervals at which the miss
               ratio is desired If this is less than the line size it
               is set to the line size (default 512 bytes).

          -M number
               Specifies the maximum cache size of interest.  When the
               contents of the stack go beyond this limit entries are
               deleted from the bottom of the stack. This is primarily
               to avoid thrashing (default 512 Kbytes).

          The following options are used when the configuration is
          direct-mapped to set parameter values

          -a number
               Specifies the log of the minimum line size (default 7).

          -b number
               Specifies the log of the maximum line size (default
               14).

          -c number
               Specifies the log of the cache size (default 16).

     EXAMPLE
          cheetah -C sa -F basic -T u -R lru -a 5 -b 14 -l 4 -n 2 -t 1000000
          -f tfile -o results

          Determines the miss ratios of set-associative caches with
          number of sets ranging from 32 (2^5) to 16384 (2^14), and
          associativities ranging from 1 to 4 (2^2), with a line size
          of 16 (2^4) bytes.  The input trace format is basic and is
          read from the file tfile. A unified instruction and data
          cache is simulated on the first 1 million addresses in the
          trace.  The output is written to the file results.

     BUGS
          Currently, cheetah cannot handle traces that are longer than
          2^31 addresses.

          With OPT replacement, the memory requirement is not bounded,
          and thrashing might occur with some traces.



     Page 4                                          (printed 1/21/97)






     cheetah(1)                                             cheetah(1)



          The conversion routine for DIN traces is simple and not very
          efficient.  If you use DIN traces often it would help to
          make this routine more efficient.

          The package has not been thoroughly debugged on 64-bit
          architectures such as the KSR and the DEC Alpha.

     AUTHORS
          Rabin A. Sugumar (rabin@eecs.umich.edu) and Santosh G.
          Abraham (sga@eecs.umich.edu), The University of Michigan.

     SEE ALSO
          pixie (1), dineroIII ()

          Rabin A. Sugumar and Santosh G. Abraham, ``Efficient
          Simulation of Caches using Binomial Trees'', Technical
          Report CSE TR-111-91, University of Michigan, 1991.

          Rabin A. Sugumar and Santosh G. Abraham, ``Efficient
          Simulation of Caches under OPT replacement with Applications
          to Miss Characterization,'' in the proceedings of the 1993
          ACM SIGMETRICS Conference.

































     Page 5                                          (printed 1/21/97)



