
#ifdef WITH_SSL
 #include <openssl/ssl.h>
 #include <openssl/err.h>
#endif // WITH_SSL

extern FILE *fGlblOut;
// map of MANAGED strings with attributes.
class AttribStringMap : public StringMap {
public:
      AttribStringMap   ( );
     ~AttribStringMap   ( );

   int  put      (char *pkey, char *ptext, char *pattr);
   char  *get     (char *pkey, char **ppattr);
   char  *iget    (int nindex, char **ppkey, char **ppattr);

   int  put      (num nkey, char *ptext, char *pattr);
   char *get      (num nkey, char **ppattr);
   char *iget     (int nindex, num *pkey, char **ppattr);

private:
   char *mixdup   (char *ptext, char *pattr);
   int  demix    (char *pmixed, char **pptext, char **ppattr);
};

class LongTable {
public:
   LongTable            ( );
  ~LongTable            ( );
   int addEntry        (int nValue, int nAtPos=-1);
   int updateEntry     (int nValue, int nIndex);
   int numberOfEntries ( );
   int getEntry        (int iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   int expand          (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   int *pClArray;
};

class StringTable {
friend class Array;
public:
   StringTable          ( );
  ~StringTable          ( );
   int addEntry        (char *psz, int nAtPos=-1, char **ppCopy=0);
   int removeEntry     (int nAtPos);
   int numberOfEntries ( );
   char *getEntry       (int iIndex, int nTraceLine);
   int  setEntry       (int iIndex, char *psz);
   void resetEntries    ( );
   bool isSet           (int iIndex);
   void dump            (int nIndent=0);
   int find            (char *psz); // out: index, or -1
private:
   int addEntryPrefixed(char *psz, char cPrefix);
   int setEntryPrefixed(int iIndex, char *psz, char cPrefix);
   int expand          (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   char **apClArray;
};

class NumTable {
public:
   NumTable            ( );
  ~NumTable            ( );
   int addEntry        (num nValue, int nAtPos=-1);
   int updateEntry     (num nValue, int nAtPos);
   int numberOfEntries ( );
   num  getEntry        (int iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   int expand          (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   num  *pClArray;
};

class StringPipe
{
public:
      StringPipe  ( );
      char *read  (char **ppAttr=0);  // returns 0 on EOD
      void  resetPipe ( );
      bool  eod   ( );
      void  dump  (cchar *pszTitle);
      int  numberOfEntries ( ) { return clText.numberOfEntries(); }
      char *getEntry        (int nIndex, int nLine, char **pAttr=0);
      void  resetEntries    ( );
      int  addEntry        (char *psz, char *pAttr);
      int  setEntry        (int iIndex, char *psz, char *pAttr);
private:
   StringTable clText;
   StringTable clAttr;
   int nReadIndex;
};

class FileList {
public:
   FileList       ( );
  ~FileList       ( );
   int  addFile        (char *pszAbsName, char *pszRoot, num nTimeStamp, num nSize, char cSortedBy=0);
   int  checkAndMark   (char *pszName, num nSize);
   void  reset          ( );
   StringTable clNames;
   StringTable clRoots;
   NumTable    clTimes;
   NumTable    clSizes;
};

class CoiTable {
public:
   CoiTable             ( );
  ~CoiTable             ( );

   // use THIS for a simple coi list:
   int addEntry         (Coi &ocoi, int nAtPos=-1);
   // it adds a COPY of the supplied coi.

   int  removeEntry     (int nAtPos);
   int  numberOfEntries ( );
   Coi  *getEntry       (int iIndex, int nTraceLine);
   int  setEntry        (int iIndex, Coi *pcoi);
   int  addSorted       (Coi &ocoi, char cSortedBy, bool bUseCase);
   void resetEntries    ( );
   bool isSet           (int iIndex);
   int  hasEntry        (char *pszFilename);

private:
   int expand           (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   Coi  **apClArray;
};

enum eProgressInfoKeepFlags {
   eKeepProg   = (1<<0),
   eKeepAdd    = (1<<1),
   eNoCycle    = (1<<2),
   eSlowCycle  = (1<<4),
   eNoPrint    = (1<<5)
};

class ProgressInfo
{
public:
   ProgressInfo          ( );
   void  setWidth        (int nColumns);
   void  setAddInfoWidth (int nColumns); // abs. columns, high prio
   void  setAddInfo      (const char *pszFormat, ...);
   void  setAction       (cchar *pszVerb, cchar *pszSubject, cchar *pszAddInfo=0, int nKeepFlags=0);
   void  setStatus       (cchar *pszVerb, cchar *pszSubject, cchar *pszAddInfo=0, int nKeepFlags=0);
   void  print           ( );  // print status now, keep line
   void  printLine       (int nFilter=0); // print final status, including newline
   void  cycle           ( );  // print status if enough time elapsed
   void  clear           ( );  // clear status, if it was printed
   int   print           (const char *pszFormat, ...);
   void  setProgress     (num nMax, num nCur, cchar *pszUnit, bool btriple=0);
   void  setStatProg     (cchar *pverb, cchar *psubj, num nMax, num nCur, cchar *pszUnit);
   void  clearProgress   ( );

// private:
   void  fixAddInfoWidth ( );
   void  dumpTermStatus  ( );
   void  clearTermStatus ( );  // if anything to clear
   int  nMaxChars;
   int  nMaxSubChars;
   int  nAddInfoCols;
   int  nDumped;
   num   nLastDumpTime;
   bool  bAddInfoPrio;
   int  nAddInfoReserve;
   int  nTurn;
   char  szVerb   [200];
   char  szSubject[200];
   char  szAddInfo[200];
   char  szTermBuf[400];
   char  szPrintBuf[400];
   char  szPerc   [20];
};

extern ProgressInfo info;

// simple double linked list

class ListEntry
{
public:
    ListEntry   ( );
   ~ListEntry   ( );

   ListEntry *next      ( ) { return pClNext; }
   ListEntry *previous  ( ) { return pClPrevious; }

   ListEntry *pClNext;
   ListEntry *pClPrevious;

   // user payload
   void  *data;
};

class List
{
public:
   List  ( );
  ~List  ( );

   ListEntry *first  ( ) { return pClFirst; }
   ListEntry *last   ( ) { return pClLast; }

   void add          (ListEntry *p);
   void addAsFirst   (ListEntry *p);
   void addAfter     (ListEntry *after, ListEntry *toadd);
   void remove       (ListEntry *entry);
   void reset        ( );
   int  size         ( );

private:
   ListEntry *pClFirst;
   ListEntry *pClLast;
};

class FileCloser {
public:
    FileCloser  (Coi *pcoi); // can be NULL
   ~FileCloser  ( );
private:
    Coi *pClCoi;
};

class CommandChaining
{
public:
   CommandChaining ( );

   bool  colfiles;   // collect filenames
   bool  usefiles;   // use collected filenames
   bool  coldata;    // collect data
   bool  usedata;    // use collected data
   bool  colbinary;  // with coldata: next command accepts binary

   CoiTable *infiles;   // while using filenames
   CoiTable *outfiles;  // while collecting filenames

   StringPipe *indata;  // text and attributes
   StringPipe *outdata; // text and attributes
   StringPipe *storedata;
   bool        storetextdone;
   StringPipe *perlinein;
   StringPipe *perlineout;

   KeyMap *justNamesFilter;

   bool  text2files;
   bool  files2text;

   int  init();
   void  reset();    // per loop
   void  shutdown();
   bool  colany() { return colfiles || coldata; }
   bool  useany() { return usefiles || usedata; }
   bool  usebin() { return (nClInBinarySize > 0) ? 1 : 0; }
   int  moveOutToIn(char *pszCmd);
   int  convInDataToInFiles ( );

   int  addLine(char *pszText, char *pszAttr, int iSplitByLF=0);
   int  addToCurLine(char *pszWords, char *pszAttr, bool bNewLine=0);
   int  addStreamAsLines(int iCmd, char *pData, int iData);
   int  addBlockAsLines(char *pData, int iData);

   int  addFile(Coi &ocoi); // is COPIED
   int  hasFile(char *psz);
   int  numberOfInFiles() { return infiles->numberOfEntries(); }
   Coi  *getFile(int nIndex); // returns null on wrong index

   int  print(char cattrib, int nflags, cchar *pszFormat, ...);
   int  print(cchar *pszFormat, ...); // multi-line support

   int  printFile(cchar *pszOutFile, bool bWriteFile, cchar *pszFormat, ...);

   void  dumpContents();   // to terminal

   int   addBinary(uchar *pData, int iSize);
   uchar *loadBinary(num &rSize); // owned by caller

   int openOverallOutputFile(cchar *pszMode);
   void closeOverallOutputFile(int iDoneFiles=0);

   num   nClOutBinarySize;  // for binary write
   num   nClInBinarySize;   // for binary read
   uint  nClOutCheckSum;
   uint  nClInCheckSum;

private:
   // prebuf is huge to allow long chain.print examples
   char  szClPreBuf[MAX_LINE_LEN*2+10];
   char  szClPreAttr[MAX_LINE_LEN*2+10];
   // buf is a normal line buffer
   char  szClBuf[MAX_LINE_LEN+10];
   char  szClAttr[MAX_LINE_LEN+10];
   char  szClBinBuf[32768+100];
   int   iClBinBufUsed;
   bool  btold1;
};

extern CommandChaining chain;

enum eConvTargetFormats
{
   eConvFormat_LF     = 1,
   eConvFormat_CRLF   = 2,
   eConvFormat_ShowLE = 4
};

extern KeyMap glblSFKVar;
bool   sfkhavevars();
int    sfksetvar(char *pname, uchar *pdata, int idata, int nadd=0);
uchar *sfkgetvar(char *pname, int *plen);
uchar *sfkgetvar(int i, char **ppname, int *plen);
int    sfkdelvar(char *pname);
void   sfkfreevars();
bool   isHttpURL(char *psz);

#ifndef USE_SFK_BASE

/*
   the idea of meta db is to store signatures and redundant copies
   when copying files to USB sticks, to detect damaged files later.
   try sfk xcopy for details. but this never became official.
*/

class FileMetaDB
{
public:
   FileMetaDB  ( );

   bool  canRead     ( ) { return nClMode == 1; }
   bool  canUpdate   ( ) { return nClMode == 2; }

   int  openUpdate  (char *pszFilename);
   int  openRead    (char *pszBaseName, bool bVerbose); // zz-sign w/o .dat
   int  updateFile  (char *pszName, uchar *pmd5cont = 0, bool bJustKeep=false);
   int  removeFile  (char *pszName, bool bPrefixLF = 0);
   int  updateDir   (char *pszName);
   int  save        (int &rnSignsWritten);
   void  reset       ( );
   int  checkFile   (char *pszName);
   int  numberOfFiles  ( ) { return aUnixTime.numberOfEntries(); }

   int  getFileFlags   (int nIndex) { return aFlags.getEntry(nIndex, __LINE__); }

   int  verifyFile  (char *pszFilename, char *pszShFile=0, bool bSilentAttribs=0);
   int  verifyFile  (int nIndex, bool bCleanup);
   // 0:ok 1:notfound 9:file_differs_inconsistently

   int  numberOfVerifies  ( ) { return nClVerified; }
   int  numberOfVerMissing( ) { return nClVerMissing; }
   int  numberOfVerFailed ( ) { return nClVerFailed; }
   bool  anyEvents         ( ) { return nClVerified || nClVerMissing || nClVerFailed; }
   char *filename          ( ) { return pszClDBFile; }
   int  setMetaDir        (char *psz);
   char  *metaDir          ( ) { return pszClMetaDir; }
   bool  isSignatureFile   (char *pszFile);

private:
   int  indexOf     (char *pszFile);
   int  writeRecord (FILE *fout, int nIndex, SFKMD5 *pmd5, bool bIsLastRec);
   int  writeEpilogue     (FILE *fout, SFKMD5 *pmd5);
   int  loadDB      (char *pszBasePath, bool bVerbose);
   int  loadRecord  (FILE *fin, SFKMD5 *pmd5, bool bSim); // uses szLineBuf
   int  loadHeader  (FILE *fin, SFKMD5 *pmd5);
   int  loadCheckEpilogue (FILE *fin, SFKMD5 *pmd5);

   static char *pszClFileDBHead;

   char     *pszClDBPath;
   char     *pszClDBFile;
   char     *pszClLineBuf;
   char     *pszClMetaDir;
   int     nClMode;
   int     nClVerified;
   int     nClVerMissing;
   int     nClVerFailed;
   NumTable  aUnixTime;
   NumTable  aWinTime;
   NumTable  aContSumLo;
   NumTable  aContSumHi;
   LongTable aFlags;
   StringTable aPath;

   uchar     abClRecBuf[1024];
};

extern FileMetaDB filedb;

class FileVerifier
{
public:
   FileVerifier   ( );
   int  remember (char *pszDstName, num nsumhi, num nsumlo);
   int  verify   ( );
   void  reset    ( );
   int  matchedFiles( ) { return nClMatched; }
   int  failedFiles ( ) { return nClFailed; }
   int  totalFiles  ( ) { return aClDst.numberOfEntries(); }
private:
   NumTable    aClSumHi;
   NumTable    aClSumLo;
   StringTable aClDst;
   int  nClMatched;
   int  nClFailed;
};

extern FileVerifier glblVerifier;

class CopyCache
{
public:
   CopyCache      ( );
   void setBuf    (uchar *pBuf, num nBufSize);
   int process   (char *pszSrcFile, char *pszDstFile, char *pszShDst, uint nflags);
   int flush     ( );
   void setEmpty  ( );
private:
   int putBlock  (uchar *pData, int nDataSize);
   uchar *pClBuf;
   num   nClBufSize;
   num   nClUsed;
   char  szTmpBuf[MAX_LINE_LEN+10];
};

extern CopyCache glblCopyCache;

class SFKMapArgs
{
public:
      SFKMapArgs  (char *pszCmd, int argc, char *argv[], int iDir);
     ~SFKMapArgs  ( );
   char  *eval    (char *pszExp);

   bool  bdead;

   StringTable  clDynaStrings;
   char       **clargx;
   bool         bDoneAlloc;

   char         szClEvalOut[300];   // for small results
   char        *pszClEvalOut;       // for large results
};

#ifdef SFKINT
 // internal: ftp server experimental folder space limits
 #define WITH_FTP_LIMITS
 // internal: inline calculations in variable expressions
 // #define WITH_VAR_CALC
#endif

#define MAX_FTP_VDIR 10

class FTPServer
{
public:
      FTPServer   ( );
     ~FTPServer   ( );

   int run           (uint nPort, bool bRW, bool bRun, bool bDeep, uint nPort2, uint nPasvPort);
   char *absPath     (char *pszFilePath=0);
   char *sysPath     (char *pszFilePath=0, int *piVDir=0);
   int   mapPath     (char *pszRelPath, bool bAllowRoot=0, bool bCheckDiskSpace=0);
   int   mapPathInt  (char *pszRelPath, bool bAllowRoot, bool bCheckDiskSpace);
   int   reply       (cchar *pszMask, ...);
   int   replyFromRC (int iSubRC);
   int   readLine    ( );
   void  setStart    ( );
   int   isTimeout   (int iTimeout);
   int   addUseDir   (char *psz);
   int   setFixDir   (char *psz);

private:
   int   addTrailSlash     (char *pszBuf, int iMaxBuf);
   void  stripTrailSlash   (char *pszPath, char cSlash);
   char *notslash          (char *pszPath);
   int   setLocalWalkDir   (char *pszPath);
   int   checkPath         (char *pszPath, bool bDeep);
   int   copyNormalized    (char *pdst, int imaxdst, char *psrc);

public:
char
   szClAuthUser      [50],
   szClAuthPW        [50],
   szClRunPW         [50],
   szClEnvInfoUser   [50],
   szClEnvInfoPW     [50],
   szClEnvInfoRunPW  [50];

private:
char
   szClWorkDir    [800],
   szClOldWorkDir [800],
   szClAbsPathBuf [800],
   szClSysPathBuf [800],
   szClTmpPathBuf [800],
   szClTmpPathBuf2[800],
   szClCmpPathBuf [800],
   szClFixSysDir  [800],
   szClRenameFrom [800],
   szClReplyBuf   [1024];

int  iClVDir;
char aClVDirSrc[MAX_FTP_VDIR+2][100];
char aClVDirDst[MAX_FTP_VDIR+2][200];

#ifdef WITH_FTP_LIMITS
char aClVDirLimMode[MAX_FTP_VDIR+2];    // null, 'd'eep, 'f'lat
int  aClVDirLimFreeMB[MAX_FTP_VDIR+2];  // with 'd'eep and 'flat'
int  aClVDirLimUsedMB[MAX_FTP_VDIR+2];  // 'f'lat only
int  aClVDirLimUsedFil[MAX_FTP_VDIR+2]; // 'f'lat only
#endif

struct sockaddr_in
   clServerAdr,
   clPasServAdr,
   clClientAdr,
   clDataAdr;

SOCKET
   hClServer,
   hClClient,
   hClPasServ,
   hClData;
 
num
   nClStart,
   nClDiskFree;

bool
   bClSendFailed,
   bClTimeout,
   bClDeep;
};

#endif // USE_SFK_BASE

// find: BinTexter remembers so many chars from previous line
//       to detect AND patterns spawning across soft-wraps.
#define BINTEXT_RECSIZE 3000
// 600 * 2 = 1200, 1200 * 2 = 2400

class BinTexter
{
public:
   BinTexter         (Coi *pcoi);
  ~BinTexter         ( );

   enum eDoWhat {
      eBT_Print   = 1,  // floating output, LFs blanked
      eBT_Grep    = 2,  // LFs lead to hard line break
      eBT_JamFile = 3   // floating output, LFs blanked
   };

   // uses szLineBuf, szLineBuf2.
   int  process     (int nDoWhat);
   int  processLine (char *pszBuf, int nDoWhat, int nLine, bool bHardWrap);

private:
   Coi  *pClCoi;
   char  szClPreBuf[80];   // just a short per line prefix
   char  szClOutBuf[BINTEXT_RECSIZE+100]; // fix: 1703: buffer too small.
   char  szClAttBuf[BINTEXT_RECSIZE+100]; // fix: 1703: buffer too small
   char  szClLastLine[BINTEXT_RECSIZE+100];
   bool  bClDumpedFileName;
};

// SFK home dir creation and filename building
class SFKHome
{
public:
      SFKHome  ( );

bool
      noHomeDir   ( );
char
      *makePath   (char *pszRelPath, bool bReadOnly=0),
       // also creates required folders.
       // returns NULL on any error.
      *getPath    (char *pszRelPath);
       // for readonly access.
       // returns NULL on any error.

bool  bClTold;
char  szClDir     [SFK_MAX_PATH+10];
char  szClPathBuf [SFK_MAX_PATH+10];
};

// --- sfk190 nocase with variable table ---

class SFKChars
{
public:
      SFKChars ( );

   int    init       ( );
   int    setocp     (ushort i);
   int    setacp     (ushort i);
   ushort getocp     ( );  // calls init().
   ushort getacp     ( );  // calls init().

   int    wlen       (ushort *puni);

   // fast calls using maps
   ushort ansitouni  (uchar  c);
   ushort oemtouni   (uchar  c);
   uchar  unitoansi  (ushort n); // returns 0 if n/a
   uchar  unitooem   (ushort n); // returns 0 if n/a

   uchar  oemtoansi  (uchar  c);
   uchar  ansitooem  (uchar  c);

   int    stroemtoansi  (char *psz, int *pChg=0, bool bNoDefault=0);
   int    stransitooem  (char *psz, int *pChg=0, bool bNoDefault=0);

   int    strunitoansi  (ushort *puni, int iunilen,
                         char *pansi, int imaxansi);

   // internal
   ushort ibytetouni (uchar c, ushort icp);

bool     bclinited;
ushort   iclocp, iclacp;
bool     bsysocp, bsysacp, banycp;

ushort   amap1[256];    // oem to uni
uchar    amap2[65536];  // uni to oem

ushort   amap3[256];    // ans to uni
uchar    amap4[65536];  // uni to ans

uchar    amap5[256];    // oem to ans
uchar    amap6[256];    // ans to oem

uchar    amap5err[256];
uchar    amap6err[256];

char     sztmp[50];
ushort   awtmp[50];
};

class SFKNoCase
{
public:
   SFKNoCase   (bool bfuzz);

   void  tellPage    ( );

   uchar itolower    (uchar c);
   uchar itoupper    (uchar c);
   uchar map1to1     (uchar c, uchar btolower, ushort *puni);
   bool  iisalpha    (uchar c);
   bool  iisalnum    (uchar c);
   bool  iisprint    (uchar c);

   // compat with xfind etc. calls
   uchar mapChar (uchar c, uchar bCase) { return bCase ? c : itolower(c); }
   uchar lowerUChar(uchar c) { return itolower(c); }
   uchar upperUChar(uchar c) { return itoupper(c); }
   void  isetStringToLower(char *psz);

bool   bclfuzz;
bool   bcltoldcp;

uchar  atolower [256];
uchar  atoupper [256];
uchar  aisalpha [256];
};

extern SFKNoCase sfknocasesharp;
extern SFKNoCase sfknocasefuzz;

#define sfkmaxname 1000

class sfkname
{
public:
   sfkname  (const char *psz, bool bpure=0);
   sfkname  (ushort *pwsz);

   char   *vname  ( );
   ushort *wname  ( );

char   szname  [sfkmaxname+20];
ushort awname  [sfkmaxname+20];
ushort nstate;
bool   bbadconv;
};


#ifdef USE_DCACHE

// area one, ordered by namesum
struct DMetaEntry {
   num   sumhi;
   num   sumlo;
   num   size;
   num   time;
};

// area two, ordered by fifo
struct DFifoEntry {
   num   sumhi;
   num   sumlo;
};

#endif // USE_DCACHE

// map of managed keys and MANAGED Cois.
// entries can also be read in a fifo way.
class CoiMap : public KeyMap {
public:
      CoiMap   ( );
     ~CoiMap   ( );

   void  reset (bool bWithDiskCache, const char *pszFromInfo);
   // expects: all coi entries have zero references.
   // if not, errors are dumped.
   // deletes all coi entries.

   int  put   (char *pkey, Coi *pTransferCoi, const char *pTraceFrom, int nmode=0);
   // enter the Coi under that key.
   // the Coi is NOT COPIED.
   // Coi ownership is TRANSFERED to the CoiMap.
   // the Coi may be DELETED anytime if it's
   // reference counter reaches zero.

   Coi *get    (char *pkey);
   // CALLER MUST RELEASE (decref) after use!
   // if a value was stored then it is returned.
   // the Coi is NOT COPIED.
   // if not, null is returned, no matter if the key is set.

   int  remove(char *pkey);
   // remove entry with that key, if any.
   // associated Coi must have zero refs.
   // associated Coi is deleted.
   // rc: 0:done 1:no_such_key >=5:error.

   num   byteSize(bool bCalcNow=0);
   // approximate added bytesize of all objects.
   // calcnow forces immediate, expensive calculation.

   num   bytesMax       ( );  // peak memory use
   num   filesDropped   ( );  // files dropped during processing

   int  tellByteSizeChange(Coi *pcoi, num nOldSize, num nNewSize);
   // if pcoi is in the cache, this method quickly
   // adapts the overall byte count.

protected:
   int  putDiskCache(char *pkey, Coi *pcoi, const char *pTraceFrom);
   int   bfindDMeta(num nsumlo,num nsumhi,int &rindex);

   List  clFifo;
   num   nClByteSize;   // approximately
   num   nClBytesMax;   // peak memory use
   num   nClDropped;    // files dropped during processing

   #ifdef USE_DCACHE
   int        nClDAlloc;
   int        nClDUsed;
   DMetaEntry *apClDMeta;
   DFifoEntry *apClDFifo;
   #endif // USE_DCACHE
};

class TCPCore;
class Coi;
class CoiTable;

// optional class: derive own objects from this
// and register with TCPCon's to associate own
// data with connections.
class TCPConData
{
public:
            TCPConData  ( );
   virtual ~TCPConData  ( ) = 0;
};

class TCPCon
{
public:
      TCPCon   (SOCKET hsock, TCPCore *pcorein, int nTraceLine=0);
     ~TCPCon   ( );

   // ===== data I/O functions =====
   void  setBlocking (bool bYesNo);

   int  read     (uchar *pblock, uint nlen, bool bReturnAny=0);
   // read raw block of bytes. this will block
   // until nlen bytes are read, or connection is closed.
   // returns <= 0 on connection close, else nlen.
   
   int  send     (uchar *pBlock, uint nLen);

   char *readLine (char *poptbuf=0, uint noptmaxbuf=0, bool braw=0);
   // read()'s char by char until CRLF is reached.
   // returns supplied buffer, or NULL on EOD/close.
   // if no buffer is supplied, provides internal buffer.
   // returned lines are stripped from CRLF except if braw.
   // SHARE: may share buffer with putf.

   // raw text I/O, sends all CRLF as supplied by caller:
   int  puts  (char *pline);
   int  putf  (cchar *pmask, ...);
   // SHARE: may share buffer with readLine.

   // ftp and http protocol support
   int  setProtocol (char *psz);   // "ftp", "http"
   int  readReply   (int nMinRC, int nMaxRC);
   // reads a reply like "200 OK", "HTTP/1.1 200 OK",
   // checks RC if it's within range, returns 0 if so.

   #ifdef WITH_SSL
   int  sslConnect  (char *phostorip, int iTraceLevel=0);
   #endif // WITH_SSL

// private:
   TCPCore &core  ( );
   void  rawClose ( );
   char *buffer   (int &rbufsize);

   TCPCore     *pClCore;
   SOCKET       clSock;
   TCPConData  *pClUserData;  // optional
   char        *pClIOBuf;     // alloc'ed on demand
   static int  nClIOBufSize;  // if it is alloc'ed

   int   nClTraceLine;
   num   nClStartTime;
   int   iClMaxWait;
   int   iClPort;

   struct sockaddr_in clFromAddr;

   #ifdef WITH_SSL
   SSL_CTX *pClSSLContext;
   SSL     *pClSSLSocket;
   #endif // WITH_SSL

friend class TCPCore;
};

// basic tcp i/o support.
class TCPCore
{
public:
      TCPCore  (char *pszID, char cProtocol);
     ~TCPCore  ( );

   static TCPCore &any( ); // generic core for scripting

   char  *getID   ( );

   // windows only:
   static int sysInit  ( );
   // run TCPCore::sysInit() before any tcp transfer

   static void sysCleanup  ( );
   // run this last before exiting the application

   void  shutdown ( );
   // close everything, delete all user objects
   // associated with connections (if any).
   // windows only: if bSysCleanup is set, runs also
   // WSACleanup() to cleanup the windows TCP stack.

   int makeServerSocket (int nport, TCPCon **ppout, bool bquiet=0);
   // returned object is managed by TPCCore.
   // returned server socket is added to readfds set.

   int accept (TCPCon *pServerCon, TCPCon **ppout);
   // returned object is managed by TPCCore.
   // returned client socket is added to core's readfds set.

   int connect(char *phostorip, int nport, TCPCon **ppout, bool bSSL=0);
   // returned object is managed by TPCCore.
   // client socket is added to readfds set.

   int  close    (TCPCon *pcon);
   // closes connection, deletes associated objects, if any.
   // DELETES supplied pcon.

   int  lastError   ( );
   // system errno of last operation. depends on call if it's set.

   // for a centralized read and processing thread:
   int  selectInput (TCPCon **pNextActiveCon, TCPCon *pToQuery=0, int iMaxWaitMSec=0);
   // from all sockets pending input, returns the first one
   // that received an accept/connect or read event.
   // if pToQuery is given, returns that pointer and rc0
   // if exactly that connection has input pending.
   // rc0=OK_active_socket_set rc1=nothing_to_do >=5:fatal error

   // optional: register a socket with a user-supplied object
   // derived from TCPConData.
   // -  the object must be created by the user through "new".
   // -  it is deleted by SFKTCP on close() or shutdown().
   int  registerData(TCPCon *pcon, TCPConData *pUserData);

   void  setVerbose  (bool bYesNo);
   // dump sent/received control lines to terminal

   bool  verbose     ( );

   // reference counting:
   int  incref   (cchar *pTraceFrom);  // increment refcnt
   int  decref   (cchar *pTraceFrom);  // decrement refcnt
   int  refcnt   ( );  // current no. of refs

   // close all open connections:
   void  closeConnections  ( );

   // set basic proxy infos, may be used by derived classes only.
   static int    setProxy (char *phost, int nport);

// protected:
   int  checksys ( );
   void  wipe     ( );
   int  addCon   (TCPCon *pcon);   // add to fdset
   int  remCon   (TCPCon *pcon);   // remove from fdset

   char   *pClID;
   int    nClCon;     // must be < FD_SETSIZE
   int    nClMaxSock;   // important for select

   // TODO: check sizes of these. maybe make them dynamic ptrs,
   //       to assure a tcp core still suits on a stack.
   TCPCon *aClCon[FD_SETSIZE+2];
   fd_set  clReadSet; // all sockets pending accept and read
   fd_set  clSetCopy;
   fd_set  clSetCopyW;
   fd_set  clSetCopyE;
   bool    bClVerbose;
   int     nClRefs;
   int     iClMaxWait;
   char    cClProtocol;

   static  char szClProxyHost[100];
   static  int  nClProxyPort;
   static  bool  bClProxySet;
   static  bool bSysInitDone;

   static TCPCore *pClGeneric;   // sfk1972
};

// class to auto close a connection,
// AND to automatically clear its pointer.
class ConAutoClose {
public:
      ConAutoClose   (TCPCore *pcore, TCPCon **pcon, int nTraceLine);
     ~ConAutoClose   ( );
private:
      int     nClTraceLine;
      TCPCore  *pClCore;
      TCPCon  **ppClCon;
};

class CharAutoDelete {
public:
      CharAutoDelete (char **pdata) { ppClPtr = pdata; }
     ~CharAutoDelete ( ) { if (*ppClPtr) delete [] *ppClPtr; }
private:
      char **ppClPtr;
};

// TODO: rearrange logic?
// -  one ftpclient instance per host
// -  this ftpclient must remember the ftp://hostname
// -  then list() returns absolute names based on this
// UNCLEAR: when to logout on such a client
class FTPClient : public TCPCore
{
public:
      FTPClient(char *pszID);
     ~FTPClient( );

   int   login (char *phostorip, int nport=21);
   // anonymous ftp only, so far.

   int   loginOnDemand(char *phostorip, int nport);
   // does nothing if already logged in

   void   logout( );

   int   openFile   (char *pfilename, cchar *pmode);
   // begin download of a file with a relative path,
   // e.g. subdir/thefile.txt

   int   readFile   (uchar *pbuf, int nbufsize);
   // read block from currently open file

   void   closeFile  ( );
   // close currently open fle

   int   list (char *pdir, CoiTable **pptable, char *pRootURL=0);
   // NOTE: if pptable is the address of a NULL pointer,
   //       a CoiTable will be created and returned.
   // NOTE: if pptable points to an existing CoiTable,
   //       file and dir entries will be filled therein.
   // returned CoiTable is always managed by caller.
   // RootURL: if supplied, every filename is prefixed by that,
   //          e.g. "ftp://thehost/thedir/"

   int  setPassive  (TCPCon **ppout);

   char  *line    ( );  // line i/o buffer
   int   lineMax ( );  // max size of that buffer

private:
   int  readLine (TCPCon *pcon);
   int  readLong (TCPCon *pcon, uint &rOut, cchar *pszInfo);
   int  sendLong (TCPCon *pcon, uint nOut, cchar *pszInfo);
   int  readNum  (TCPCon *pcon, num &rOut, cchar *pszInfo);
   int  readNum  (uchar *pbuf, int &roff, num &rOut, cchar *pszInfo);
   int  sendNum  (TCPCon *pcon, num nOut, cchar *pszInfo);

   void  wipe     ( );
   int  splitURL (char *purl);
   char *curhost  ( );  // from spliturl
   char *curpath  ( );  // from spliturl
   int  curport  ( );  // from spliturl
   int  sendLine (char *pline);
   char *readLine ( );
   int  readReply(char **ppline);

   TCPCon   *pClCtlCon;        // control connection
   TCPCon   *pClDatCon;        // data connection, if separate
   char      aClURLBuf[300];   // current hostname, path
   char     szClLineBuf[1000]; // for file i/o
   int      nClPort;          // default is 21
   char     *pClCurPath;       // within URLBuf

   bool      bClLoggedIn;
   bool      bClSFT;
   int      nClSFTVer;
   char     *pszClHost;

   struct   CurrentIOFile {
      char   *name;
      bool    blockmode;   // else bulk
      int    block;       // read block number
      num     time;
      num     size;
      num     remain;
      uchar   abmd5[20];
      SFKMD5 *md5;
   }  file;

   // reply string to PASV. if set, we're in passive mode,
   // receiving one or more downloads.
   char     *pClPassive;

friend class Coi;
};

class HTTPClient : public TCPCore
{
public:
      HTTPClient(char *pszID);
     ~HTTPClient( );

   int   open (char *purl, cchar *pmode, Coi *pcoi);
   // begin download from an url.
   // NOTE: pcoi's name might be changed on redirect.

   int   postraw (char *purl, char *pszRawReq);
   // header+data in one request text

   int   read (uchar *pbuf, int nbufsize);
   // read block from currently open url stream.
   // chunked transfer is managed transparently.

   void   resetCache( );
   // use between access to two url's.

   void   close( );
   // close currently open url.

   int   getFileHead   (char *purl, Coi *pout, cchar *pinfo="");
   // read header infos like content-type and size.
   // the provided Coi is filled.

   int   list (char *purl, CoiTable **pptable);
   // NOTE: if pptable is the address of a NULL pointer,
   //       a CoiTable will be created and returned.
   // NOTE: if pptable points to an existing CoiTable,
   //       file and dir entries will be filled therein.
   // returned CoiTable is always managed by caller.

   int  sendReq  (cchar *pcmd, char *pfile, char *phost, int nport);
   // requires an open connection

   int  connectHttp    (char *phostorip, int nport, TCPCon **ppout);
   // returned object is managed by TPCCore.
   // same as connect() but with optional proxy handling.

   int  iClWebRC;

private:
   bool  haveConnection ( );
   void  wipe     ( );

   int  readraw      (uchar *pbuf, int nbufsize);
   int  readrawfull  (uchar *pbuf, uint nbufsize);
   // read block without any chunk handling.

   int  splitURL  (char *purl);
   char *curhost  ( );  // from spliturl
   char *curpath  ( );  // from spliturl

   int  joinURL   (char *pabs, char *pref);
   char *curjoin  ( );  // from joinurl

   int  splitHeader (char *phead);
   char *curname  ( );  // of header entry
   char *curval   ( );  // of header entry

   int  sendLine  (char *pline);
   char *readLine (bool braw=0);

   int  rawReadHeaders (int &rwebrc, char *purlinfo, Coi *pOptCoi=0); // , StringMap *pheads=0);
   // returns sfk rc, 0 (OK) ... 9 (fatal error)
   // returns also webrc, e.g. 200 (OK) or 302 (redirect)

   int      nClPort;

   TCPCon   *pClCurCon; // current i/o connection

   char      aClURLBuf[1000];  // current hostname, path
   char     *pClCurPath;       // within URLBuf

   char      aClHeadBuf[200];  // current header line
   char     *pClCurVal;        // within HeadBuf

   char      aClJoinBuf[1000]; // join: recombined url

   char      aClIOBuf[4096];

   // Header-Free Servers: first reply line cache
   char     *pszClLineCache;  // only for first line

   // Transfer-Encoding: chunked support
   bool      bClChunked;
   uchar    *pClCache;        // alloc'ed on demand
   int      nClCacheAlloc;
   int      nClCacheUsed;    // remaining unread bytes

   // is http server blocking head requests?
   bool      bClFirstReq;
   bool      bClNoHead;

   // if set, no connections are possible:
   bool      bClNoCon;

   bool      bClSSL;

   #ifdef WITH_SSL
   SSL_CTX *pClSSLContext;
   SSL     *pClSSLSocket;
   #endif // WITH_SSL

friend class Coi;
};

class ConCache : public KeyMap
{
public:
      ConCache   ( );
     ~ConCache   ( );

      // creates clients on demand. returned clients
      // - are still MANAGED BY THE CACHE
      // - must be released after use
      FTPClient   *allocFtpClient (char *pBaseURL);
      // returns NULL if no locked con is available

      // must be called after temporary use,
      // e.g. download of a single web file:
      int  releaseClient (FTPClient  *p);

      void  closeConnections  ( );
      void  reset (bool bfinal, const char *pszFromInfo);

private:
      int   closeAndDelete(void *praw);
      // caller MUST remove() manually after that!

      int  forceCacheLimit( );

      // to remember fifo sequence:
      // fifo list with managed key strings
      num       nAddCnt;
      StringMap clFifo;
};

// implemented in sfknet.cpp, cleanup in sfk.cpp:
extern ConCache glblConCache;
extern CoiMap glblVCache;
extern KeyMap glblCircleMap;
extern KeyMap glblOutFileMap;



/*
   sfktxt protocol fields:
      v100  :  version 1.0.0
      reqn  :  request number n, with n >= 1
      rtn   :  retry number n, 0 to 3
      copy  :  request a reply for request
      repn  :  reply to request number n
      cs1   :  text is encoded using color scheme 1:
               \x1F + color code: rgbymc (dark) RGBYMC (bright) 
                  wW = white  , viewers with white background may use gray 
                  d  = default, viewers with white background will use black
               if \x1F was found in original input, it is escaped as \x1F\x1F
      scr   :  start color is 'r'ed for this message
      fl    :  finished line, last line of packet is complete
      sl    :  split line, last line continues in next packet
      
   sfktxt conventions:
      -  if sender sends ,copy the receiver must reply immediately
         with a ,rep record. otherwise, the line is broken.
      -  as long as no CR or LF is received, text must be joined into
         one large line. typical maximum line length is about 4000 chars.
         large lines spanning multiple packets are also marked by ,sl.
      -  if ,sc is found receiver should set initial color to that value.

   example message:

      data (\n == LF, \x1F == character with code 0x1F)
         :sfktxt:v100,req1,rt0,cs1,scR,fl\n
         :clear\n
         \n
         \x1FRfoo\x1Fd and \x1Fbbar\n

      remarks
         -  req1   : the first request from this client
         -  rt0    : this is the original message, no retries yet
         -  cs1    : uses color coded text with 0x1F tags
         -  scR    : start color is bright 'R'ed
         -  fl     : last line is finished, no spanning of messages
         -  :clear : viewer should clear the log then add text
         -  there is no ",copy" field, so sender expects no reply
         -  \x1FRfoo\x1Fd and \x1Fbbar\n
            print "foo" in bright red, then 'd'efault color etc.
*/

#define UDPIO_MAX_CLIENTS 128

class UDPIO
{
public:
      UDPIO    ( );
     ~UDPIO    ( );

void
      rawInit  ( );

int
      initSendReceive   (const char *pszDescription,
                         int iOwnReceivePort,
                         // provide -1 to allocate any port from system
                         int iTargetSendPort,
                         char *pszTargetHostname,
                         // provide a single machine's IP for Unicast.
                         // provide 224.0.0.x to use Multicast.
                         // provide NULL to use a Unicast IP later.
                         uint uiFlags=0
                         // 1 : force multicast
                         // 2 : reuse address
                         // 4 : retry on alternative ports
                        ),
      setTarget         (char *pszHostname, int iPort),

      // --------- raw I/O ---------
      sendData          (uchar *pData, int iDataSize),
      receiveData       (uchar *pBuffer, int iBufferSize,
                         struct sockaddr_in *pAddrIncoming=0,
                         int iSizeOfAddrIncoming=0
                        ),
      closeAll          ( );

bool
      isOpen            ( ),
      isMulticast       ( ),
      isDataAvailable   (int iSec=0, int iMSec=0);

      // -------- network text I/O ----------
int   // RC 0 == OK
      addCommand        (char *pszCmd), // call before sendText
      addOrSendText     (char *pszText, char *pszAttr),
      addOrSendText     (char *pszPhrase, int iPhraseLen, bool bNoWrap),
      flushSend         (bool bTellAboutSplitLine),
      addHeader         ( ),
      receiveText       ( ),  // MUST call getNextInput next
      storeHeader       (char *pszRaw, int iHeadLen),
      sendDuplexReply   (struct sockaddr_in *pTo),
      decodeColorText   (int iFromOffset),
      checkTellCurrentColor (char c),
      getClientIndex    (struct sockaddr_in *pAddr, bool *pFound),
      getTextSendDelay  ( );  // depending on (non)duplex mode
bool
      hasCachedInput    ( ),
      hasCachedOutput  ( );
char
     *getNextCommand    ( ),  // NULL if none
     *getNextInput      (char **ppAttr=0,
                         struct sockaddr_in *pSenderAddr=0,
                         bool bDontCache=0), 
                        // returns NULL if none
     *peekHeader        (char *pszField),
      sfkToNetColor     (char c);

char
      szClDescription   [30];
int
      iClOwnReceivePort,
      iClTargetSendPort,
      iClTimeout,
      iClNonDuplexSendDelay;
SOCKET
      fdClSocket;
bool
      bClMulticast,
      bClVerbose,
      bClRawText,
      bClCmdClear,
      bClDuplex,
      bClColor,
      bClCopyRequest,
      bClContinuedStream,
      bClForceNextInput,
      bClAppendLFOnRaw,
      bClDecodeColor,
      bClIPWasExpanded;

char
      cClTellColor,
      cClCurrentInColor;

struct sockaddr_in 
      clTargetAddr,
      clRawInAddr,      // of current received package
      clInBufInAddr;    // for start of text line
char  
      aClHeaderBuf      [1000+100],
      aClCommand        [10][100],
      aClRawOutBuf      [2500+100],
      aClRawInBuf1      [2500+100],
      aClRawInBuf2      [2500+100],
      aClRawInAttr2     [2500+100],
      aClInBuf          [MAX_LINE_LEN+1000],
      aClInAtt          [MAX_LINE_LEN+1000];
int
      iClPackageSize,
      iClCommand,
      iClHeadSize,
      iClOutIndex,
      iClInBufUsed,
      iClRawInputCached,
      iClReqNum,
      iClRecentReqNum,
      iClInReqNum,
      iClInRetryNum,
      iClRetryOff,      // retry number offset in header
      iClSLIOff,        // split line indicator offset in header
      iClClient,        // current client index
      iClUsingAltPortInsteadOf;  // if bind on selected port failed,
      // alternative port is used, and failed port is flagged here.

struct UCPClientState
{
   num    ntime;        // if 0, slot is empty
   int    reqnum;       // most recent received reqnum
   struct sockaddr_in addr;
   int    copyreq;      // recent reqnum of copy answer
   int    copytry;      // recent trynum of copy answer
   char   color;        // text color at end of recent record
}
   aClClients  [UDPIO_MAX_CLIENTS+10];
};

#ifndef USE_SFK_BASE
extern UDPIO sfkNetSrc;
#endif // USE_SFK_BASE

extern int netErrno();
extern char *netErrStr();
extern void printSamp(int nlang, char *pszOutFile, char *pszClassName, int bWriteFile, int iSystem);

#define MAX_UDP_CON (mymin(300, FD_SETSIZE))

struct UDPCon
{
   char     host[128];
   char     ipstr[128];
   char     reply[128];
   int      port;
   struct   sockaddr_in addr;
   SOCKET   sock;
   num      tsent;
   int      idelay;
   int      replylen;
   int      istate;
};

class UDPCore
{
public:
      UDPCore  (int iMaxWait);
     ~UDPCore  ( );

   int   makeSocket  (int iMode, char *phost, int nport);
   void  shutdown    ( );

   int   selectInput (int *pIndex, int iMaxWaitMSec);
   int   lastError   ( );

   void  stepPing    (int i);
   int   sendPing    (int i);
   int   recvPing    (int i, int *pDelay);

   UDPCon   aClCon[MAX_UDP_CON+2];
   int      nClCon;    // must be < FD_SETSIZE
   int      nClMaxSock;
   int      iClResponses;

   fd_set   clReadSet; // all sockets pending accept and read
   fd_set   clSetCopy;

   int   bverbose;
   int   bpure;
   num   tstart;
   int   imaxwait;
};

#ifndef SFK_PROFILING
 #define _p(id)
 extern void logProfile();
#endif

int copyFormStr(char *pszDst, int nMaxDst, char *pszSrc, int nSrcLen, uint nflags=0);
int submain(int argc, char *argv[], char *penv[], char *pszCmd, int iDir, bool &bFatal);
void skipOver(char **pp, cchar *pdelim);
int sfkcalc(double &r, char *pszFrom, char **ppNext, int iLevel, bool bStopPM=0);
bool isEmptyDir(char *pszIn);
uchar *loadBinaryFlex(Coi &ocoi, num &rnFileSize);
bool isEmpty(char *psz);
bool strbeg(char *pszStr, cchar *pszPat);
bool stribeg(char *psz, cchar *pstart);
bool stribeg(char *psz, cchar *pstart);
void setSystemSlashes(char *pdst);
void setNetSlashes(char *pdst);
bool isUniPathChar(char c);
const char *getPureSFKVersion();
const char *getShortOSName();
int sfkmkdir(char *pszName, bool braw=0);
int ansiToUTF(char *pdst, int imaxdst, char *psrc);
void utfToAnsi(char *pdst, int imaxdst, char *psrc);
int mySetFileTime(char *pszFile, num nTime);
void mystrcatf(char *pOut, int nOutMax, cchar *pszFormat, ...);
bool isPathTraversal(char *pszFileIn, bool bDeep);
void fixPathChars(char *pszBuf);
bool isAbsolutePath(char *psz1);
char *relName(char *pszRoot, char *pszAbs);
int copyFile(char *pszSrc, char *pszDst, char *pszShDst, uchar *pWorkBuf, num nBufSize, uint nflags);
bool endsWithExt(char *pname, char *pszextin);
bool endsWithArcExt(char *pname, int iTraceFrom);
int encode64(uchar *psrc, int nsrc, uchar *pdst, int nmaxdst, int nlinechars);
int getFileMD5NoCache(char *pszFile, uchar *abOut16, bool bSilent);
bool equalFileContent(char *pszFile1, char *pszFile2, uchar *psrcmd5=0, uchar *pdstmd5=0);
num getFileAge(char *pszName);
int copyFromFormText(char *pSrc, int iMaxSrc, char *pDstIn, int iMaxDst, uint nflags=0);
uint sfkPackSum(uchar *buf, uint len, uint crc);

// int perr(const char *pszFormat, ...);
// int pwarn(const char *pszFormat, ...);
// int pinf(const char *pszFormat, ...);

extern char szLineBuf[MAX_LINE_LEN+10];
extern char szLineBuf2[MAX_LINE_LEN+10];
extern char szLineBuf3[MAX_LINE_LEN+10];
extern char szAttrBuf[MAX_LINE_LEN+10];
extern char szAttrBuf2[MAX_LINE_LEN+10];
extern char szAttrBuf3[MAX_LINE_LEN+10];
extern char szRefNameBuf[MAX_LINE_LEN+10];
extern char szRefNameBuf2[MAX_LINE_LEN+10];
extern char szPrintBuf1[MAX_LINE_LEN+10];
extern char szPrintBuf2[MAX_LINE_LEN+10];
extern char szTopURLBuf[MAX_LINE_LEN+10];
extern char szOutNameBuf[MAX_LINE_LEN+10];
extern char szRunCmdBuf[MAX_LINE_LEN+10];

extern bool bGlblHaveInteractiveConsole;
extern bool bGlblEscape;


class AlignTest1 { public: int a1; char c1; };
class AlignTest2 { public: int a1; char c1; char c2; };
class AlignTest3 { public: int a1; char c1; char c2; char c3; };

extern void getAlignSizes1(int &n1, int &n2, int &n3);
extern void getAlignSizes2(int &n1, int &n2, int &n3);

