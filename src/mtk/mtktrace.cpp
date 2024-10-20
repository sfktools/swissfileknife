/*
   Micro Tracing Kernel 0.7.2 by stahlworks technologies.
   Unlimited Open Source License, free for use in any project.

   the following line protects this file against instrumentation:
   // [instrumented]

   NOTE: Win32 GUI apps should never use "term:" tracing, but only "file:".

   Changes:
   0.7.2
   -  add: time stamp in front of ever log record
   -  add: optional tracing of block exits,
           define MTK_TRACE_BLOCK_EXITS to use.
   0.7.1
   -  add: some marker characters {} around block entries.
   -  chg: improved error message if filename contains quotes.
   -  fix: dumpLastSteps produced no output (missing prefix check).
   -  fix: after dumpStackTrace, dumpLastSteps the log file was closed
           and therefore no further tracing to file was possible.
   -  fix: compile warnings (signed/unsigned comparison).

   0.7.0
   -  chg: mtkdump: now also dumps an ascii representation of the data.
   -  add: mtkb: if provided function name is empty, replace by line number.
   -  fix: syntax of instrumented tag, mtktrace.cpp was not excluded.
   -  add: missing includes for standalone compile of mtktrace.cpp.
   -  add: text "error: " and "warning:" on corresponding record types.
   -  chg: by default, short form "export MTK_TRACE=filename:test.log"
           now traces twex statements into file.
           this is different from "export MTK_TRACE=file:,filename:test.log"
           which defines an output file for mtkDump calls, but does NOT
           log normal trace messages into file.
   -  add: optimized cr/lf removal on tracing statements.
   -  mtkHexDump now expects type void*
   -  fflush on flog writing.
   -  full tracing support via file:
         export MTK_TRACE=file:twex,filename:test.log
      without the need to have any terminal output.
   -  no longer writing default logname, user must explicitely
      specify filename: to create a tracefile.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
 #include <windows.h>
 #define pthread_self GetCurrentThreadId
 #define vsnprintf _vsnprintf
#else
 #include <pthread.h>
#endif

#define MTKMAXTHREADS 300  // by default, threadid's are listed as 2-byte hex
#define MTKMAXLEVEL   200  // max. block nesting level supported for tracing
#define ulong unsigned long
#define uchar unsigned char

#define MTKMAXMSG    1000  // 1000 (0.1 MB) to 10000 (1.2 MB)
#define MTKMSGSIZE    118

// trace mode 'b' lists only block entries by default.
// to list also block exits, set this define:
// #define MTK_TRACE_BLOCK_EXITS

// some projects provide their own printf implementation
#ifdef PROJECT_PRINTF
extern "C" int PROJECT_PRINTF(const char *format, ...);
#else
 #define PROJECT_PRINTF printf
#endif

// =============================
// ===== MTK internal core =====
// =============================

#define MTKLBUFSIZE 1024  // output line buffer

class MTKMain
{
public:
   MTKMain ();

   void traceMethodEntry(void *pRefInst, const char *pszFile, int nLine, const char *pszfn);
   void traceMethodExit (void *pRefInst, const char *pszFile, int nLine, const char *pszfn);
   void traceMessageRaw (const char *pszFile, int nLine, char cPrefix, char *pszRaw);
   uchar tracePre       (const char *pszFile, int nLine, char cPrefix);
   // returns non-zero (OK) if there is an interest in this kind of message

   void dumpStackTrace  (bool bOnlyOfCurrentThread);
   void dumpLastSteps   (bool bOnlyOfCurrentThread);
   void setRingTrace    (char *pszMask);
   void setTermTrace    (char *pszMask);
   void setFileTrace    (char *pszMask);

   ulong nthreads;
   ulong asysid[MTKMAXTHREADS+2];
   ulong alevel[MTKMAXTHREADS+2];
   const char *apprefile[MTKMAXTHREADS+2];
   ulong anpreline[MTKMAXTHREADS+2];
   char  acpreprefix[MTKMAXTHREADS+2];
   const char *apfile[MTKMAXTHREADS+2][MTKMAXLEVEL+2];
   const char *apfunc[MTKMAXTHREADS+2][MTKMAXLEVEL+2];
   ulong anline[MTKMAXTHREADS+2][MTKMAXLEVEL+2];
   ulong iclmsg;  // counts endless, modulo past read
   char  amsg[MTKMAXMSG+2][MTKMSGSIZE+2];
   // record structure per amsg:
   // - three bytes prefix (thread,level,prefix)
   // - then the message text (mtklog content)
   char  alinebuf[MTKLBUFSIZE+10];
   char  alinebuf2[MTKLBUFSIZE+10];
   ulong ncurthrsid;
   ulong ncurthridx;
   uchar nlockmode;
   ulong nringx; // trace extended interface input into ringbuffer
   ulong ntermx;  // dump  extended interface input onto terminal
   ulong nfilex;  // dump  extended interface input info file
   const char *pszFilename;
   FILE *flog;
   uchar bflushlog;
};

static const char *glblPszBlank =
   "                                                  "
   "                                                  "
   "                                                  "
   "                                                  ";

static uchar glblMTKAlive = 0;
static MTKMain glblMTKInst;

MTKMain::MTKMain()
{
   nthreads = 0;
   memset(asysid, 0, sizeof(asysid));
   memset(alevel, 0, sizeof(alevel));
   memset(apprefile, 0, sizeof(apprefile));
   memset(anpreline, 0, sizeof(anpreline));
   memset(acpreprefix, 0, sizeof(acpreprefix));
   memset(apfile, 0, sizeof(apfile));
   memset(apfunc, 0, sizeof(apfunc));
   memset(anline, 0, sizeof(anline));
   iclmsg = 0;
   memset(amsg, 0, sizeof(amsg));
   memset(alinebuf, 0, sizeof(alinebuf));
   memset(alinebuf2, 0, sizeof(alinebuf2));
   ncurthrsid = 0xFFFFFFFF;
   ncurthridx = 0;
   nlockmode = 0;

   #ifdef MTK_TRACE_FULL_RING
   nringx = 0xFF;
   #else
   nringx = 0;
   #endif

   ntermx = 0;
   nfilex = 0;

   flog = 0;
   bflushlog = 1;

   const char *pszEnv = getenv("MTK_TRACE");
   if (pszEnv)
   {
      // by default, only some messages are stored in the ring.
      // allow extended settings here:
      const char *psz1 = strstr(pszEnv, "ring:");
      if (psz1)
      {
         psz1 += strlen("ring:");
         setRingTrace((char*)psz1);
      }
         
      // by default, only some messages are dumped onto terminal.
      // allow extended settings here:
      if ((psz1 = strstr(pszEnv, "term:")) != 0)
      {
         psz1 += strlen("term:");
         setTermTrace((char*)psz1);
      }

      if ((psz1 = strstr(pszEnv, "filename:")) != 0) {
         psz1 += strlen("filename:");
         pszFilename = psz1;
         flog = fopen(pszFilename, "w");
         if (!flog) {
            fprintf(stderr, "# # # MTKTRACE ERROR: UNABLE TO WRITE LOG: =>%s<= # # #\n", pszFilename);
            if (strchr(pszFilename, '\"') || strchr(pszFilename, '\''))
               fprintf(stderr, "... make sure that MTK_TRACE contains no \"\" or '' quotes.\n");
         } else {
            printf("MTKTrace: writing log output into %s\n", pszFilename);
            fflush(stdout);
            setFileTrace("twex,"); // may be overwritten by "file:" below
         }
      }

      if ((psz1 = strstr(pszEnv, "file:")) != 0)
      {
         psz1 += strlen("file:");
         setFileTrace((char*)psz1);
      }
   }

   glblMTKAlive = 1;
}

void MTKMain::setRingTrace(char *psz1) 
{
   nringx = 0;
   while (*psz1 && (*psz1 != ','))
      switch (*psz1++) {
         case 't': nringx |=  1; break;
         case 'b': nringx |=  2; break;
         case 'x': nringx |=  4; break;
         case 'w': nringx |=  8; break;
         case 'e': nringx |= 16; break;
         default:
            fprintf(stderr, "ERROR: MTK unsupported ring settings, use tbxwe\n");
            break;
      }
}

void MTKMain::setTermTrace(char *psz1)
{
   ntermx = 0;
   while (*psz1 && (*psz1 != ','))
      switch (*psz1++) {
         case 't': ntermx |=  1; break;
         case 'b': ntermx |=  2; break;
         case 'x': ntermx |=  4; break;
         case 'w': ntermx |=  8; break;
         case 'e': ntermx |= 16; break;
         default:
            fprintf(stderr, "ERROR: MTK unsupported term settings, use tbxwe\n");
            break;
      }
   nringx |= ntermx;
}

void MTKMain::setFileTrace(char *psz1)
{
   nfilex = 0;
   while (*psz1 && (*psz1 != ','))
      switch (*psz1++) {
         case 't': nfilex |=  1; break;
         case 'b': nfilex |=  2; break;
         case 'x': nfilex |=  4; break;
         case 'w': nfilex |=  8; break;
         case 'e': nfilex |= 16; break;
         case 'f': bflushlog = 0; break; // 'f'ast mode: do not flush on every line
         default:
            fprintf(stderr, "ERROR: MTK unsupported file settings, use tbxwe\n");
            break;
      }
   nringx |= nfilex;
}

void MTKMain::traceMethodEntry(void *pRefInst, const char *pszFile, int nLine, const char *pszFunc)
{
   if (nlockmode >= 2)
      return;

   ulong nthreadid = (ulong)(pthread_self());

   // assume we still have the same thread context
   ulong ithread = ncurthridx;

   // if thread context changed, find new index
   if (ncurthrsid != nthreadid)
   {
      for (ithread=0; ithread<nthreads; ithread++)
         if (asysid[ithread] == nthreadid)
            break;

      // if no index found, add a new thread index
      if (ithread==nthreads) {
         if (nthreads >= MTKMAXTHREADS) {
            fprintf(stderr, "ERROR: INSTR: too many threads, cannot trace\n");
            glblMTKAlive = 0;
            return;
         }
         nthreads++;
         asysid[ithread] = nthreadid;
         alevel[ithread] = 0;
      }

      // cache the current thread's system id and index mapping
      ncurthrsid = nthreadid;
      ncurthridx = ithread;
   }

   ulong ilevel = alevel[ithread];
   if (ilevel >= MTKMAXLEVEL) {
      fprintf(stderr, "ERROR: INSTR: max stack level reached, cannot trace\n");
      glblMTKAlive = 0;
      return;
   }

   // store current stack location
   apfile[ithread][ilevel] = pszFile;
   anline[ithread][ilevel] = nLine;
   apfunc[ithread][ilevel] = pszFunc;
   // if (strstr(pszFile, "BHDiag"))
   //   printf("tme %x %u %u %s %u\n", nthreadid, ithread, ilevel, pszFile, nLine);

   // increment and write back current stack level
   ilevel++;
   alevel[ithread] = ilevel;

   // trace block entry event?
   if (nringx & 2) 
   {
      char szBuf[120];
      szBuf[0] = '\0';

      // create formatted string: file,line,function
      //                           50   10    30
      long noff = 0, nlen = 0;
      const char *psz = 0;

      // block start marker
      szBuf[noff++] = '{';
      szBuf[noff] = '\0';

      // filename, if any
      psz  = pszFile ? pszFile : "";
      nlen = strlen(psz);
      if (nlen) {
         if (nlen > 50) { psz = psz + nlen - 50; nlen = 50; }
         if (noff+nlen < (long)sizeof(szBuf)-10) {
            memcpy(szBuf+noff, psz, nlen);
            szBuf[noff+nlen] = '\0';
            noff += nlen;
         }
      }

      // line number
      if (noff+20 < (long)sizeof(szBuf)-10) {
         szBuf[noff++] = ' ';
         #ifdef _WIN32
         _ultoa((unsigned long)nLine, szBuf+noff, 10);
         #else
         sprintf(szBuf+noff, "%lu", (unsigned long)nLine);
         #endif
         noff += strlen(szBuf+noff);
      }

      // function
      psz  = pszFunc ? pszFunc : "";
      nlen = strlen(psz);
      if (nlen) {
         if (nlen > 30) { psz = psz + nlen - 30; nlen = 30; }
         if (noff+nlen < (long)sizeof(szBuf)-10) {
            szBuf[noff++] = ' ';
            memcpy(szBuf+noff, psz, nlen);
            szBuf[noff+nlen] = '\0';
            noff += nlen;
         }
      }

      // block end marker
      szBuf[noff++] = '}';
      szBuf[noff] = '\0';

      // dump formatted string (file info is redundant)
      traceMessageRaw(pszFile, nLine, 'B', szBuf);
   }
}

void MTKMain::traceMethodExit(void *pRefInst, const char *pszFile, int nLine, const char *pszFunc)
{
   if (nlockmode >= 2)
      return;

   ulong nthreadid = (ulong)(pthread_self());

   // assume we still have the same thread context
   ulong ithread = ncurthridx;

   // if thread context changed, find new index
   if (ncurthrsid != nthreadid)
   {
      for (ithread=0; ithread<nthreads; ithread++)
         if (asysid[ithread] == nthreadid)
            break;

      // if no index found, there is an internal error
      if (ithread==nthreads) {
         fprintf(stderr, "ERROR: INSTR: non-matching method exit, cannot trace\n");
         glblMTKAlive = 0;
         return;
      }

      // cache the current thread's system id and index mapping
      ncurthrsid = nthreadid;
      ncurthridx = ithread;
   }

   ulong ilevel = alevel[ithread];
   if (ilevel == 0) {
      fprintf(stderr, "ERROR: INSTR: stack underflow, cannot trace\n");
      glblMTKAlive = 0;
      return;
   }

   #ifdef MTK_TRACE_BLOCK_EXITS
   // trace block exit event?
   if ((nringx & 2) && apfile[ithread][ilevel] && anline[ithread][ilevel])
   {
      const char *pszFile = apfile[ithread][ilevel];
      int   nLine         = anline[ithread][ilevel];
      const char *pszFunc = apfunc[ithread][ilevel];

      char szBuf[120];
      szBuf[0] = '\0';

      // create formatted string: file,line,function
      //                           50   10    30
      long noff = 0, nlen = 0;
      const char *psz = 0;

      // block start marker
      szBuf[noff++] = ' ';
      szBuf[noff++] = '{';
      szBuf[noff] = '\0';

      // filename, if any
      psz  = pszFile ? pszFile : "";
      nlen = strlen(psz);
      if (nlen) {
         if (nlen > 50) { psz = psz + nlen - 50; nlen = 50; }
         if (noff+nlen < (long)sizeof(szBuf)-10) {
            memcpy(szBuf+noff, psz, nlen);
            szBuf[noff+nlen] = '\0';
            noff += nlen;
         }
      }

      // line number
      if (noff+20 < (long)sizeof(szBuf)-10) {
         szBuf[noff++] = ' ';
         #ifdef _WIN32
         _ultoa((unsigned long)nLine, szBuf+noff, 10);
         #else
         sprintf(szBuf+noff, "%lu", (unsigned long)nLine);
         #endif
         noff += strlen(szBuf+noff);
      }

      // function
      psz  = pszFunc ? pszFunc : "";
      nlen = strlen(psz);
      if (nlen) {
         if (nlen > 30) { psz = psz + nlen - 30; nlen = 30; }
         if (noff+nlen < (long)sizeof(szBuf)-10) {
            szBuf[noff++] = ' ';
            memcpy(szBuf+noff, psz, nlen);
            szBuf[noff+nlen] = '\0';
            noff += nlen;
         }
      }

      // block end marker
      szBuf[noff++] = '}';
      szBuf[noff++] = ' ';
      szBuf[noff++] = '<';
      szBuf[noff] = '\0';

      // dump formatted string (file info is redundant)
      traceMessageRaw(pszFile, nLine, 'B', szBuf);
   }
   #endif // MTK_TRACE_BLOCK_EXITS

   apfile[ithread][ilevel] = 0;
   anline[ithread][ilevel] = 0;
   apfunc[ithread][ilevel] = 0;

   ilevel--;
   alevel[ithread] = ilevel;
}

static unsigned long localGetCurrentTime()
{
   #ifdef _WIN32
   return (unsigned long)GetTickCount();
   #else
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return ((unsigned long)tv.tv_sec) * 1000 + ((unsigned long)tv.tv_usec) / 1000;
   #endif
}

void MTKMain::traceMessageRaw(const char *pszFile, int nLine, char cPrefix, char *pszRaw)
{
   // IN: pszFile can be NULL!

   if (nlockmode >= 1)
      return;

   ulong nthreadid = (ulong)(pthread_self());

   // assume we still have the same thread context
   ulong ithread = ncurthridx;

   // if thread context changed, find new index
   if (ncurthrsid != nthreadid)
   {
      for (ithread=0; ithread<nthreads; ithread++)
         if (asysid[ithread] == nthreadid)
            break;

      // if no index found, add a new thread index
      if (ithread==nthreads) {
         if (nthreads >= MTKMAXTHREADS) {
            fprintf(stderr, "ERROR: INSTR: too many threads, cannot trace\n");
            glblMTKAlive = 0;
            return;
         }
         nthreads++;
         asysid[ithread] = nthreadid;
         alevel[ithread] = 0;
      }

      // cache the current thread's system id and index mapping
      ncurthrsid = nthreadid;
      ncurthridx = ithread;
   }

   ulong ilevel = alevel[ithread];

   // single nearly-atomic operation across all threads.
   // we expect that some messages can get lost.
   ulong imsg = (iclmsg++) % MTKMAXMSG;

   // if any context infos are missing, take them from the precache
   if (!pszFile) pszFile = apprefile[ithread];
   if (!nLine  ) nLine   = anpreline[ithread];
   if (!cPrefix) cPrefix = acpreprefix[ithread];
   if (!cPrefix) cPrefix = '?'; // shouldn't actually happen

   // store thread-index and nesting level
   ulong nprelen = 3;
   amsg[imsg][0] = (uchar)ithread;
   amsg[imsg][1] = (uchar)ilevel;
   amsg[imsg][2] = (uchar)cPrefix;

   // store actual message
   ulong imax = MTKMSGSIZE-nprelen-2;
   strncpy(&amsg[imsg][nprelen], pszRaw, imax);
   amsg[imsg][imax] = '\0';

   // strip cr/lf, if any
   char *pszMsg = &amsg[imsg][nprelen];
   long nLen = strlen(pszMsg);
   while (nLen > 0 && (pszMsg[nLen-1]=='\r' || pszMsg[nLen-1]=='\n')) {
      pszMsg[nLen-1] = '\0';
      nLen--;
   }

   // if there is space left, append location info with a linefeed
   if (pszFile != 0) {
      ulong ilen = strlen(&amsg[imsg][nprelen]);
      if (ilen < (imax-1)) {
         strcat(&amsg[imsg][nprelen+ilen+0], "\n");
         // make relative filename, full paths are too long
         const char *pszRel = 0;
         #ifdef _WIN32
         if ((pszRel = strrchr(pszFile, '\\')) != 0) pszFile = pszRel+1;
         #else
         if ((pszRel = strrchr(pszFile, '/')) != 0)  pszFile = pszRel+1;
         #endif
         strncpy(&amsg[imsg][nprelen+ilen+1], pszFile, (imax-1)-ilen);
         amsg[imsg][imax] = '\0';
      }
   }

   const char *pszAddInfo = "";
   if (cPrefix == 'E')
      pszAddInfo = "error: ";
   if (cPrefix == 'W')
      pszAddInfo = "warning: ";

   // tracing to file selected?
   if (nfilex != 0)
   {
      bool bSkip = 0;
      switch (cPrefix) {
         case 'X': if (!(nfilex &  4)) bSkip = 1; break; // extended input, general
         case 'W': if (!(nfilex &  8)) bSkip = 1; break; // extended input, warnings
         case 'E': if (!(nfilex & 16)) bSkip = 1; break; // extended input, errors
         case 'B': if (!(nfilex &  2)) bSkip = 1; break; // block entry
         default : if (!(nfilex &  1)) bSkip = 1; break; // all other messages
      }
      if (!bSkip && (flog != 0)) {
         // note that pszRaw still contains unwanted CR/LFs.
         long nLen = strlen(pszRaw);
         while (nLen > 0 && (pszRaw[nLen-1]=='\r' || pszRaw[nLen-1]=='\n'))
            nLen--;
         fprintf(flog, "%lu [%02lX:%c] %s%.*s%.*s\n", 
            (unsigned long)(localGetCurrentTime() & 65535UL),
            ithread, cPrefix, pszAddInfo,
            (int)ilevel, glblPszBlank, (int)nLen, pszRaw);
         if (bflushlog)
            fflush(flog);
      }
   }

   // instant tracing to terminal selected?
   if (ntermx != 0)
   {
      bool bRet = 0;
      switch (cPrefix) {
         case 'X': if (!(ntermx &  4)) bRet = 1; break; // extended input, general
         case 'W': if (!(ntermx &  8)) bRet = 1; break; // extended input, warnings
         case 'E': if (!(ntermx & 16)) bRet = 1; break; // extended input, errors
         case 'B': if (!(ntermx &  2)) bRet = 1; break; // block entry
         default : if (!(ntermx &  1)) bRet = 1; break; // all other messages
      }
      if (bRet) {
         // if (!strncmp(pszRaw, "P:", 2))
         //    printf("SKIP.1: %s %c %x\n", pszRaw, cPrefix, ntermx);
         return;
      }
      PROJECT_PRINTF("[%02lX:%c] %.*s%s\n", ithread, cPrefix, (int)ilevel, glblPszBlank, pszRaw);
   }
   else
   {
      // if (!strncmp(pszRaw, "P:", 2))
      //    printf("SKIP.2: %s\n", pszRaw);
   }

   // reset prelocation data to avoid reuse
   apprefile[ithread]   = 0;
   anpreline[ithread]   = 0;
   acpreprefix[ithread] = 0;
}

uchar MTKMain::tracePre(const char *pszFile, int nLine, char cPrefix)
{
   if (nlockmode >= 1)
      return 0;

   ulong nthreadid = (ulong)(pthread_self());

   // assume we still have the same thread context
   ulong ithread = ncurthridx;

   // if thread context changed, find new index
   if (ncurthrsid != nthreadid)
   {
      for (ithread=0; ithread<nthreads; ithread++)
         if (asysid[ithread] == nthreadid)
            break;

      // if no index found, add a new thread index
      if (ithread==nthreads) {
         if (nthreads >= MTKMAXTHREADS) {
            fprintf(stderr, "ERROR: INSTR: too many threads, cannot trace\n");
            glblMTKAlive = 0;
            return 0;
         }
         nthreads++;
         asysid[ithread] = nthreadid;
         alevel[ithread] = 0;
      }

      // cache the current thread's system id and index mapping
      ncurthrsid = nthreadid;
      ncurthridx = ithread;
   }

   // we expect that this thread will issue a call to traceMessageRaw next,
   // where the following infos are used:
   apprefile[ithread]   = pszFile;
   anpreline[ithread]   = nLine;
   acpreprefix[ithread] = cPrefix;

   // are we currently interested in this kind of trace messages?
   bool bRC = 0, bRCSet = 0;
   if (cPrefix == 'X') { bRC = ((nringx &  4) != 0) ? 1 : 0; bRCSet = 1; }
   if (cPrefix == 'W') { bRC = ((nringx &  8) != 0) ? 1 : 0; bRCSet = 1; }
   if (cPrefix == 'E') { bRC = ((nringx & 16) != 0) ? 1 : 0; bRCSet = 1; }
   if (cPrefix == 'B') { bRC = ((nringx &  2) != 0) ? 1 : 0; bRCSet = 1; }
   if (bRCSet) {
      // if (cPrefix == 'E')
      //    printf("tracePre.1 %c %u\n", cPrefix, bRC);
      return bRC;
   }

   // all other message types: depends on general flag
   if (nringx & 1) {
      // if (cPrefix == 'E')
      //    printf("tracePre.2 %c 1\n", cPrefix);
      return 1;
   } else {
      // if (cPrefix == 'E')
      //    printf("tracePre.3 %c 0\n", cPrefix);
      return 0;
   }
}

void MTKMain::dumpStackTrace(bool bJustCurrentThread)
{
   if (bJustCurrentThread) {
      // lockmode 1: do not accept further traceMessage calls.
      nlockmode = 1;
   } else {
      // lockmode 2: also do not accept block entry/exit events.
      // in most cases, mtk tracing cannot be used afterwards.
      nlockmode = 2;
   }

   ulong nthreadid = (ulong)(pthread_self());
   ulong ithread;
   for (ithread=0; ithread<nthreads; ithread++)
      if (asysid[ithread] == nthreadid)
         break;
   if (ithread==nthreads) {
      fprintf(stderr, "ERROR: INSTR: thread not found, cannot dump stack trace\n");
      nlockmode = 0;
      return;
   }

   FILE *fout = flog;
   if (!fout) fout = stdout;

   printf("> DUMPING STACKTRACE OF %s TO %s\n", bJustCurrentThread?"CURRENT THREAD":"ALL THREADS", pszFilename);
   fflush(stdout);

   if (!bJustCurrentThread) {
      fprintf(fout, "==================================================\n");
      fprintf(fout, "= current stack trace of ALL threads, %lu in total:\n", nthreads);
      fprintf(fout, "==================================================\n");
   }

   for (ulong ithread2=0; ithread2<nthreads; ithread2++)
   {
      if (bJustCurrentThread)
         ithread2 = ithread;

      ulong ilevel = alevel[ithread2];
      ulong nsysid = asysid[ithread2];
      fprintf(fout, "> ------- stack trace, thread %lx%s, sysid %lx, %lu levels -------\n", ithread2, (!bJustCurrentThread && (ithread2==ithread))?" (OWN)":"", nsysid, ilevel);

      for (ulong ilev=0; ilev<ilevel; ilev++)
      {
         const char *pszFile = apfile[ithread2][ilev];
         ulong nLine = anline[ithread2][ilev];
         const char *pszFunc = apfunc[ithread2][ilev];
         const char *pszRel = strrchr((char*)pszFile, '/');
         if (pszRel) pszRel++; else pszRel = pszFile;
         fprintf(fout, "> %02lX %.*s%s %s:%lu\n", ithread2, (int)ilev, glblPszBlank, pszFunc, pszRel, nLine);
      }

      if (bJustCurrentThread)
         break;
   }
   fprintf(fout, "> ---------------------- stack trace end ---------------------\n");
   fflush(fout);

   if (fout != stdout) {
      printf("> DUMPING OF STACKTRACE DONE.\n");
      fflush(stdout);
   }

   nlockmode = 0;
}

void MTKMain::dumpLastSteps(bool bJustCurrentThread)
{
   nlockmode = 1;

   // identify our own thread
   ulong nthreadid = (ulong)(pthread_self());
   ulong iown;
   for (iown=0; iown<nthreads; iown++)
      if (asysid[iown] == nthreadid)
         break;

   FILE *fout = flog;
   if (!fout) fout = stdout;

   printf("> DUMPING LAST STEPS OF %s TO %s\n", bJustCurrentThread?"CURRENT THREAD":"PROCESS", pszFilename);
   fflush(stdout);

   if (bJustCurrentThread)
   fprintf(fout, "> ------ last steps of current thread, index %02lX sysid %04lX: -----\n", iown, nthreadid);
   else
   fprintf(fout, "> ------ last system steps, own thread = %02lX sysid %04lX: -----\n", iown, nthreadid);

   // read out ringbuffer backwards
   ulong imsg = iclmsg % MTKMAXMSG;

   // first, step FORWARD to oldest line, skipping empty entries.
   // NOTE: first three bytes of a record are prefix infos.
   ulong icur = (imsg+1) % MTKMAXMSG;
   while ((icur != imsg) && (!amsg[icur][3]))
      icur = (icur+1) % MTKMAXMSG;

   if (icur == imsg)
      fprintf(fout, "[ring buffer is empty.]\n");

   ulong nrec = 0;

   // now walk icur forward until imsg, dumping the messages
   while (icur != imsg)
   {
      uchar *pmsg = (uchar*)amsg[icur];
      ulong nprelen = 3;
      uchar ithr  = pmsg[0];
      uchar ilev  = pmsg[1];
      char  cPre  = (char)pmsg[2];
      if (bJustCurrentThread && (iown != ithr)) {
         // skip record
      } else {
         // stored messages are zero-terminated.
         // if there is a linefeed, there is location info afterwards.
         strncpy((char*)alinebuf, (const char*)pmsg+nprelen, MTKLBUFSIZE);
         alinebuf[MTKLBUFSIZE] = '\0';
         char *pszLoc = strchr(alinebuf, '\n');
         if (pszLoc) {
            *pszLoc++ = '\0';
         }
         // format main content, yet w/o location
         sprintf(alinebuf2, "[%02X:%c%c %.*s%s", ithr, cPre, (iown==ithr)?'*':']', (int)ilev, glblPszBlank, alinebuf);
         // if available, add location info
         if (pszLoc) {
            ulong ilen = strlen(alinebuf2);
            ulong iloc = 90;
            if (iloc < ilen)
                iloc = ilen;
            iloc += 3;
            iloc += (6 - (iloc % 6));
            if (iloc < (MTKLBUFSIZE-10)) {
               for (ulong iins=ilen; iins<iloc; iins++)
                  alinebuf2[iins] = ' ';
               // add at least 3 chars of location info
               sprintf(&alinebuf2[iloc], "[%.*s]", (int)((MTKLBUFSIZE-5)-iloc), pszLoc);
            }
         }
         // finally, dump whole formatted line
         fprintf(fout, "%s\n", alinebuf2);
         nrec++;
      }
      icur = (icur+1) % MTKMAXMSG;
   }

   if (bJustCurrentThread)
   fprintf(fout, "> ------------- last thread steps end, %lu records ---------------\n", nrec);
   else
   fprintf(fout, "> ------------- last system steps end, %lu records ---------------\n", nrec);
   fflush(fout);

   if (fout != stdout) {
      printf("> DUMPING OF LAST STEPS DONE.\n");
      fflush(stdout);
   }

   nlockmode = 0;
}

// block entry registration, used by MTKBlock
void mtkTraceMethodEntry(void *pRefInst, const char *pszFile, int nLine, const char *pszfunc) {
   if (glblMTKAlive)
      glblMTKInst.traceMethodEntry(pRefInst, pszFile, nLine, pszfunc);
}

// block exit registration, used by MTKBlock
void mtkTraceMethodExit(void *pRefInst, const char *pszFile, int nLine, const char *pszfunc) {
   if (glblMTKAlive)
      glblMTKInst.traceMethodExit(pRefInst, pszFile, nLine, pszfunc);
}

// ============================================
// ===== MTK public interface C functions =====
// ============================================

/*
extern void mtkTraceRaw(const char *pszFile, int nLine, char cPrefix, char *pszRaw);
extern void mtkTraceForm(const char *pszFile, int nLine, char cPrefix, const char *pszFormat, ...);
extern int  mtkTracePre(const char *pszFile, int nLine, char cPrefix);
extern void mtkTracePost(const char *pszFormat, ...);
extern void mtkDumpStackTrace(int bOnlyOfCurrentThread);
extern void mtkDumpLastSteps(int bOnlyOfCurrentThread);
extern void mtkSetRingTrace(char *pszMask);
extern void mtkSetTermTrace(char *pszMask);
*/

// all-in-one tracing entry for preformatted message buffers
void mtkTraceRaw(const char *pszFile, int nLine, char cPrefix, char *pszRaw) {
   if (glblMTKAlive)
      glblMTKInst.traceMessageRaw(pszFile, nLine, cPrefix, pszRaw);
}

// all-in-one tracing entry supporting printf-like parameters
void mtkTraceForm(const char *pszFile, int nLine, char cPrefix, const char *pszFormat, ...)
{
   if (glblMTKAlive)
   {
      va_list argList;
      va_start(argList, pszFormat);
      char szBuffer[1024];
      ::vsnprintf(szBuffer, sizeof(szBuffer)-10, pszFormat, argList);
      glblMTKInst.traceMessageRaw(pszFile, nLine, cPrefix, szBuffer);
   }
}

// the tracePre/Post combination is a workaround for complex caller macros
// that are unable to provide location and prefix together with the message
// in a single call. in this case, the caller first calls tracePre, storing
// context info for the current thread, and then tracePost with the message.

int mtkTracePre(const char *pszFile, int nLine, char cPrefix) {
   if (glblMTKAlive)
      return (int)glblMTKInst.tracePre(pszFile, nLine, cPrefix);
   else
      return (int)0;
}

void mtkTracePost(const char *pszFormat, ...)
{
   if (glblMTKAlive)
   {
      va_list argList;
      va_start(argList, pszFormat);
      char szBuffer[1024];
      // szBuffer[0] = 'P';
      // szBuffer[1] = ':';
      ::vsnprintf(szBuffer, sizeof(szBuffer)-10, pszFormat, argList);
      // the 0,0,0 are expected to be set through tracePre before.
      glblMTKInst.traceMessageRaw(0,0,0, szBuffer);
   }
}

void mtkDumpStackTrace(int bOnlyOfCurrentThread) {
   if (glblMTKAlive)
      glblMTKInst.dumpStackTrace((uchar)bOnlyOfCurrentThread);
}

void mtkDumpLastSteps(int bOnlyOfCurrentThread) {
   if (glblMTKAlive)
      glblMTKInst.dumpLastSteps((uchar)bOnlyOfCurrentThread);
}

void mtkSetRingTrace(char *pszMask) {
   if (glblMTKAlive)
      glblMTKInst.setRingTrace(pszMask);
}

void mtkSetTermTrace(char *pszMask) {
   if (glblMTKAlive)
      glblMTKInst.setTermTrace(pszMask);
}

void mtkHexDump(const char *pszLinePrefix, void *pDataIn, long lSize, const char *pszFile, int nLine, char cPrefix)
{
   char szBuf[128];
   char szPrn[32];
   uchar *pData = (uchar*)pDataIn;
   long iRead = 0;
   ulong noff = 0;
   for (long nRow=0; iRead<lSize; nRow++)
   {
      szBuf[0] = '\0';

      long nCol=0;
      for (; nCol<16 && iRead<lSize; nCol++)
      {
         uchar uc = pData[iRead++];
         sprintf(&szBuf[nCol*3], "%02X ", uc);
         szPrn[nCol] = isprint((char)uc) ? (char)uc : '.';
      }
      szPrn[nCol] = '\0';

      if (strlen(szBuf))
      {
         mtkTraceForm(pszFile, nLine, cPrefix, "%s%-48.48s %-16.16s  %04lX", pszLinePrefix, szBuf, szPrn, noff);
      }

      noff += 16;
   }
}

