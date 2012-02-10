####################################################################
####### README file for andrewk42's BoundedBuffer project ##########
####################################################################

This repository contains a re-written version of a CS343 (UW Concurrent
and Parallel Programming course) assignment. It is a generic Bounded
Buffer implemented in uC++ (http://plg.uwaterloo.ca/usystem/uC++.html)
that can be built in a few flavours: one possible variation uses a busy-
wait design, where only 2 condition locks are used and barging is
possible. Another variation uses a non-busy-wait design, where a 3rd
condition lock is introduced for preventing barging tasks. Additionally,
the Buffer can be built to support multiprocessor use (4 kernel threads).
See Makefile for more details.

Most of the code here is written in C++/uC++. To compile and run this
code you will need to install uC++. Included also is a Python script I
used to rigorously test the Buffer, and a few sample outputs from
running the script.

Note that the Buffer fails for a small percentage of cases when MULTI
is enabled and many tests ( > 2000) are run consecutively. I am mostly
convinced that these failures are not due to the algorithm used in the
Buffer code. My reasons for this are mainly due to the fact that the
error message associated with these failures is one not currently
documented by the uC++ reference manual "Could not execute code 0x...
Possible cause is due to stack corruption". Since this only occurs when
each execution attempts to use 4 kernel threads, and each of these
executions are forked from the same Python parent process via
subprocess.Popen, I suspect there is some "magic" in uC++ that goes
wrong when being forked in this way. Some other relevant points:
- The error does not occur when the script runs around 2000 or less
  iterations
- The error does not occur when a single instance of the boundedBuffer
  exectuable with MULTI enabled is told to create very large amounts
  of producers and consumers and a small buffer size (e.g. 7000, 10000,
  and 3 respectively).
- The error does not occur when MULTI is disabled (1 kernel thread)
- The error occurs when MULTI is enabled for both the busy-wait and
  non-busy-wait implementations (i.e. 2 different algorithms)
