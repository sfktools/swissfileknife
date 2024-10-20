
The SFK Micro Tracing Kernel (MTK)
==================================

LICENSE
   The Micro Tracing Kernel by StahlWorks Technologies
   is provided as unlimited open source, and free for use
   in any project, no matter if commercial or non-commercial.
   LIKE ANYTHING ELSE THAT'S FREE, MTK IS PROVIDED AS IS AND COMES
   WITH NO WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED.

HOW TO USE

Imagine you have a large source code base with thousands of .cpp files,
including either a buggy, inefficient tracing system, or even no
tracing system at all ("printf everywhere"). some companies even have
many different tracing systems within the same source code which,
in the end, also leads to everyone using "printf" again...

if you want to try out a decent, easy-to-use tracing, do this:

make a backup of all sources, and then

   -  cd into the root directory of your source base.

   -  copy the mtk/ directory supplied with sfk into there.
      this dir contains the micro tracing kernel sources.
      they are unlimited open source and can be used for free
      in any project, both commercial and non-commercial.

   -  then say

         sfk inst mtk/mtktrace.hpp _mtkb_ -dir . !save_ -file .cpp

      and wait a while. all .cpp files will be processed ("instrumented"),
      and afterwards, in (most) c++ methods you will find that
      some "_mtkb_" statement has been added at the method entry.

      furthermore, the changed .cpp files contain an include statement

         #include "mtk/mtktrace.hpp"

      just like what you specified with the 'inst' command.

      the source code now contains Block Nesting Control.
      you may now add your actual tracing statements.
      for example:

         mtklog("main entered with %d parms", argc);

      traces a printf-like text.

   -  now configure what is done with the traces:

         export MTK_TRACE=ring:twe,term:twe,filename:log.txt

      this will
      -  trace all mtklog, mtkwarn, mtkerr statements into an internal ring buffer
      -  trace them also to terminal and into file "log.txt".
         notice that the text is indented according to each thread's nesting level.

      furthermore, if you say somewhere in your sourcecode

         mtkDumpStackTrace(1);

      then you get a stack trace of the current thread
      written into log.txt. if you say

         mtkDumpLastSteps(1);

      then you get the last steps of the current thread
      written into log.txt.

      other examples:

         mtkDumpStackTrace(0);
         mtkDumpLastSteps(0);

      dumps the stack trace/last steps of ALL threads.
      this could become a long list (last steps by default
      lists up to 10000 records), therefore it's written to file.

         export MTK_TRACE=ring:tb,term:tb,filename:log.txt

      lists every single block (method) entry and exit
      both into the ring buffer and onto terminal.

this is just a short introduction about how to use mtk.
experienced  developers will find out by themselves how this can be used,
for example, in combination with other tracing systems already present
in their source base. mtk is very low-level, low-overhead, and can be
used as a layer below anything else, to mix tracing statements
of different tracing systems back together.

NOTE: if you decide to integrate mtk permanently within a project,
      be careful, and make sure that you provide some compile option
      to disable the whole tracing. mtk is intended as an analysis tool,
      and as with all free things, there is no guarantee whatsoever 
      that mtk will work reliantly, especially within production systems.

NOTE: if you use temporary (non-check-in) sfk instrumentation together
      with sfk patching, it is recommended that you instrument first,
      and patch second. this way you can redo your patches easily
      and anytime, whereas re-instrumentation ususally takes much longer.

A further technical note: mtk does not use mutex semaphores so far.
this way it stays as low-overhead as possible. however, you have to expect
that the one or other tracing record may get lost on heavy multithreading.
