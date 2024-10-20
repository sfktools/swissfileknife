
/*
   must be included directly after sfkbase.hpp.

   internal stuff used by the sfk command line tool.
*/


uint *sfkPicAllocFunc(int nWords);
uint swaprb(uint c);

#ifdef SFK_PROFILING // windows only

class StaticPerformancePoint;

#define MAX_PERF_POINTS 100

class StaticPerformanceStats
{
public:
      StaticPerformanceStats  ( );

   void  addPoint       (StaticPerformancePoint *pPoint);
   int   numberOfPoints ( );
   StaticPerformancePoint *getPoint(int iIndex);

StaticPerformancePoint
   *apClPoints [MAX_PERF_POINTS+10],
   *pClCurrentPoint;
int
    iClPoints;
};

extern StaticPerformanceStats glblPerfStats;

class StaticPerformancePoint
{
public:
      StaticPerformancePoint  (const char *pszID, const char *pszFile, int iTraceLine);

      inline void  blockEntry ( );
      inline void  blockExit  (int iElapsedTicks);

const char *pszClID;
const char *pszClFile;
int   iClTraceLine;
num   iClHits;
num   iClTotalTime;
num   iClSubTimes;
};

class DynamicPerformancePoint
{
public:
       DynamicPerformancePoint (StaticPerformancePoint *pStaticPoint);
      ~DynamicPerformancePoint ( );

StaticPerformancePoint
      *pClStaticPoint,
      *pClStaticParent;
num
       nClEntryTickCount;
};

#define _p2(id,file,line)  \
   static StaticPerformancePoint oStatPoint##line(id,file,line); \
   DynamicPerformancePoint oDynaPoint##line(&oStatPoint##line);

#define _p(id) _p2(id,__FILE__,__LINE__)

extern void logProfile();

inline num getPerfCnt()
{
   LARGE_INTEGER val1;
   QueryPerformanceCounter(&val1);
   return val1.QuadPart;
}

inline num getPerfFreq()
{
   LARGE_INTEGER val1;
   QueryPerformanceFrequency(&val1);
   return val1.QuadPart;
}

#else

#define _p(id)

extern void logProfile();

#endif

#define MAX_MOV_CMD 100
#define SFKMOV_KEEP   1
#define SFKMOV_CUT    2

class Media
{
public:
      Media ( );

   void  reset             ( );
   void  closeOutput       ( );
   void  clearCommands     ( );
   void  shutdown          ( );
   int   parseM3UFile      (char *pszFilename);
   int   processMediaFile  (char *pszSrc, char *pszOutFile);
   int   findSeconds       (num nBytePos); // with M3U only
   int   renderTempName    (char *pszFromname);
   int   analyze           (uchar *pbuf, int isize);
   void  setFixParms       (char *psz);

static Media *pClCurrent;
static Media &current ( );

int   aCmd[MAX_MOV_CMD];
num   aBeg[MAX_MOV_CMD];
num   aEnd[MAX_MOV_CMD];
int   aBegSec[MAX_MOV_CMD];
int   aEndSec[MAX_MOV_CMD];
int   iFirstCmd,iCmd;
int   iClInvalidFiles;
int   iClDoneFiles;
int   iClDoneTS;
bool  bClHaveM3UCommands;
bool  bClHaveKeep;
bool  bClKeepAll;
bool  bClJoinOutput;
bool  bClFixOutput;
bool  bClScan;
bool  bClKeepTmp;
bool  bClShowTmp;
num   nClGlobalBytes;

char  szClTmpOutFile[SFK_MAX_PATH+10];
char  szClRecentOutFile[SFK_MAX_PATH+10];
char  szClFinalFile[SFK_MAX_PATH+10];
char  szClFixParms[200];

char  szClMoveSrcOutDir[200];
char  szClMoveSrcOutFile[SFK_MAX_PATH+10];

FILE  *fClOut;
FileStat clOutStat;

char  *pszClM3UText;
char  *pszClM3UFileEntry;  // pointer into M3UText
};

#ifndef SFKPIC
uint sfkPackSum(uchar *buf, uint len, uint crc);
#endif

#ifdef SFKPIC

#ifdef USE_SFKLIB3
 #include "sfkpicio.h"
#else
 #include "sfkpicio.hpp"
#endif

class SFKPic
{
public:
   SFKPic   ( );
  ~SFKPic   ( );

   int   load     (char *pszFile);
   int   load     (uchar *pPacked, int nPacked);
   int   save     (char *pszFile);
   int   allocpix (uint w, uint h);
   int   rotate   (int idir);
   void  freepix  ( );

   uint  width    ( )   { return octl.width;  }
   uint  height   ( )   { return octl.height; }

   uint  pix      (uchar a,uchar r,uchar g,uchar b);
   uchar red      (uint c) { return (uchar)(c >> 16); }
   uchar grn      (uint c) { return (uchar)(c >>  8); }
   uchar blu      (uint c) { return (uchar)(c >>  0); }
   uchar alp      (uint c) { return (uchar)(c >> 24); }

   void  setpix   (uint x, uint y, uint c);
   uint  getpix   (uint x, uint y);
   void  drawrect (int x1, int y1, int x2, int y2, uint c, int bfill);
   void  copyFrom (SFKPic *pSrc, uint x1dst, uint y1dst, uint wdst, uint hdst, uint x1src, uint y1src, uint wsrc, uint hsrc);

   int   getErrNum   ( );
   char *getErrStr   ( );

   int   getObjectSize  ( );  // for internal checks

   uint  getnumpix( ) { return octl.npix; }
   uint *getpixptr( ) { return octl.ppix; }

struct SFKPicData
   octl;
};
#endif // SFKPIC

