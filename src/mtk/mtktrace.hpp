#ifndef _MTKTRACE_DEFINED_
#define _MTKTRACE_DEFINED_

/*
   Micro Tracing Kernel 1.0.0 by stahlworks technologies.
   Unlimited Open Source License, free for use in any project.

   NOTE: changing this header probably forces recompile of MANY files!

   SYNTAX CHANGE SINCE MTK 1.0.0:
      -  by default, mtklog now requires DOUBLE brackets: mtklog(("mystring"))
         to allow complete string removal in production code.
      -  to use the old behaviour (single brackets), define
            MTK_SINGLE_BRACKETS
      -  read below under [01] on how to adapt source to the new behaviour

   Simple instant tracing to a logfile:

      export MTK_TRACE=file:twex,filename:log.txt

         t  = normal trace msges - e.g. mtklog(("received %lu bytes",n));
         w  = warnings           - e.g. mtkwarn(("no input received"));
         e  = errors             - e.g. mtkerr(("cannot open file"));
         x  = extended traces    - for internal interfaces
         b  = block entrys/exits - all _mtkb_ blocks

      it is NOT recommended to trace 'b'blocks into tracefile.
      instead, block entries/exits may be logged into the ring buffer
      
         export MTK_TRACE=ring:twexb,file:twex,filename:log.txt

      and then dumped on demand by explicite calls in the code to
      mtkDumpStackTrace and mtkDumpLastSteps.

   Simple tracing to terminal:

         export MTK_TRACE=term:twex

   Usage example, using a compile symbol 'WITH_TRACING':

#ifdef WITH_TRACING
 #include "mtk/mtktrace.hpp"
 #define _  mtklog(("[%d]",__LINE__));
 #define __ MTKBlock tmp983451(__FILE__,__LINE__,"");tmp983451.dummy();
#else
 #define mtklog(x)
 #define mtkerr(x)
 #define mtkwarn(x)
 #define _
 #define __
#endif

   With the above code, you may use _ and __ for program flow tracing.
   example:

   void foo()
   {__                                    // block entry trace
      long i=0;
_     for (i=0; i<100; i++)               // simple line trace
         printf("%ld and counting\n",i);  // (1)
_     printf("done\n");                   // simple line trace
   }

   // NOTE: do NOT write _ at the start of line (1) as the _ macro
   //       would break the code logic.

[01] how to adapt source code to double brackets

To change source code to the new behaviour, sfk might be used like:

   sfk filt src.cpp -spat -within "mtklog(*);" -rep "_mtklog(\q_mtklog((\q_" -within "mtklog(*);" -rep "_);_));_"

however this comes without any warranty. backup your source code first!
this command only changes one-line mtklog statements.
mtkerr and mtkwarn statements need to be changed the same way.
be aware of side effects:

   if (condition)
      mtklog("...");
   else
      mtklog("...");

will behave DIFFERENT with double brackets! it is always
a required practice to write it instead like:

   if (condition) {
      mtklog("...");
   } else {
      mtklog("...");
   }
*/

extern void mtkTraceMethodEntry  (void *p, const char *psz, int n, const char *pszfn);
extern void mtkTraceMethodExit   (void *p, const char *psz, int n, const char *pszfn);

class MTKBlock {
public:
   MTKBlock(const char *pszfile, int nline, const char *pszfn) {
      mfile = pszfile;
      mline = nline;
      mfunc = pszfn;
      mtkTraceMethodEntry(this, mfile, mline, mfunc);
   }
   ~MTKBlock() {
      mtkTraceMethodExit(this, mfile, mline, mfunc);
   }
   void dummy() { }
private:
   const char *mfile;
   int mline;
   const char *mfunc;
};

#define _mtkb_(mtkbfn) MTKBlock tmp983451(__FILE__,__LINE__,mtkbfn);tmp983451.dummy();

extern void mtkTraceRaw(const char *pszFile, int nLine, char cPrefix, char *pszRaw);
extern void mtkTraceForm(const char *pszFile, int nLine, char cPrefix, const char *pszFormat, ...);
extern int  mtkTracePre(const char *pszFile, int nLine, char cPrefix);
extern void mtkTracePost(const char *pszFormat, ...);
extern void mtkDumpStackTrace(int bOnlyOfCurrentThread);
extern void mtkDumpLastSteps(int bOnlyOfCurrentThread);
extern void mtkSetRingTrace(char *pszMask);
extern void mtkSetTermTrace(char *pszMask);

extern void mtkHexDump(const char *pszLinePrefix, void *pDataIn, long lSize, const char *pszFile, int nLine, char cPrefix);
// mtkHexDump("mydata> ",abData,nSize,__FILE__,__LINE__,'D');

#ifdef MTK_SINGLE_BRACKETS

   // simple format: mtklog("mystring"). depending on the platform,
   // string "mystring" may always stay in the code, even if mtklog
   // is defined as nothing!

   #ifdef _WIN32
    #define mtklog  mtkTracePre(__FILE__,__LINE__,'D'),mtkTracePost
    #define mtkerr  mtkTracePre(__FILE__,__LINE__,'E'),mtkTracePost
    #define mtkwarn mtkTracePre(__FILE__,__LINE__,'W'),mtkTracePost
   #else
    #define mtklog(form, args...)  mtkTraceForm(__FILE__,__LINE__,'D',form, ##args)
    #define mtkerr(form, args...)  mtkTraceForm(__FILE__,__LINE__,'E',form, ##args)
    #define mtkwarn(form, args...) mtkTraceForm(__FILE__,__LINE__,'W',form, ##args)
   #endif
   #define mtkdump(pLinePrefix, pData, nSize) mtkHexDump(pLinePrefix, pData, nSize, __FILE__,__LINE__,'T')

#else

   // default format since MTK 1.0.0:
   // double brakets: mtklog(("mystring")). this allows to completely
   // strip string "mystring" from production code by defining mtklog(x) as nothing.

   #define mtklog(x)  {mtkTracePre(__FILE__,__LINE__,'D');mtkTracePost x;}
   #define mtkerr(x)  {mtkTracePre(__FILE__,__LINE__,'E');mtkTracePost x;}
   #define mtkwarn(x) {mtkTracePre(__FILE__,__LINE__,'W');mtkTracePost x;}
   #define mtkdump(pLinePrefix, pData, nSize) mtkHexDump(pLinePrefix, pData, nSize, __FILE__,__LINE__,'T')

#endif

/*
   // example for redirecting a foreign printf-like trace macro to mtk:
   #undef BADTRACE
   extern int  mtkTracePre (const char *pszFile, int nLine, char cPrefix);
   extern void mtkTracePost(const char *pszFormat, ...);
   #define BADTRACE(xwrap) do { if (mtkTracePre(__FILE__,__LINE__,'E')) mtkTracePost xwrap; } while(0)
*/

#endif
