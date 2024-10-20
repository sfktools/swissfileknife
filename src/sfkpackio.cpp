
#include "sfkpackio.hpp"

uint sfkPackSum(uchar *buf, uint len, uint crc)
{
   return crc32_z(crc, (const uchar *)buf, len);
}

// IN: bPack
int sfkPackStart(SFKPackStream *p)
{
   z_stream *pstrm = new z_stream;

   memset(pstrm, 0, sizeof(z_stream));

   int ret = 0;
   p->szerr[0] = '\0';

   if (p->bPack)
   {
      int windowBits = 15;
      int GZIP_ENCODING = 16;

      ret = deflateInit2(pstrm,
               p->bFast ? Z_BEST_SPEED : Z_DEFAULT_COMPRESSION,
               Z_DEFLATED,
               windowBits | GZIP_ENCODING,
               8,
               Z_DEFAULT_STRATEGY);
   }
   else
   {
      ret = inflateInit2(pstrm, 16+MAX_WBITS);
   }

   p->pstream = pstrm;

   return ret;
}

// IN: pin, nin, pout, nout
// CONV: if nin returned is >0 call again
int sfkPackProc(SFKPackStream *p, int bLastBlock, int *bReloop)
{
   z_stream *pstrm = (z_stream *)p->pstream;
 
   int ret = 0;
   p->szerr[0] = '\0';

   if (p->bPack)
   {
      pstrm->next_in   = p->pin;
      pstrm->avail_in  = p->nin;
      pstrm->next_out  = p->pout;
      pstrm->avail_out = p->nout;

      ret = deflate(pstrm, bLastBlock ? Z_FINISH : Z_PARTIAL_FLUSH);
   }
   else
   {
      if (*bReloop == 0)
      {
         pstrm->next_in   = p->pin;
         pstrm->avail_in  = p->nin;
      }

      pstrm->next_out  = p->pout;
      pstrm->avail_out = p->nout;

      ret = inflate(pstrm, Z_NO_FLUSH);

      // printf("inf nout=%d availout=%d availin=%d\n",
      //   p->nout, pstrm->avail_out, pstrm->avail_in);
   }

   if (ret!=0 && pstrm->msg!=0)
   {
      int imaxlen = (int)sizeof(p->szerr)-10;
      strncpy(p->szerr, pstrm->msg, imaxlen);
      p->szerr[imaxlen] = '\0';
   }

   p->nout = p->nout - pstrm->avail_out;
   p->nin  = pstrm->avail_in;

   // reference does this. sometimes it causes reloops 
   // with empty input, followed by Z_BUF_ERROR,
   // but the Z_BUF_ERROR is IGNORED, and it works.
   *bReloop = (pstrm->avail_out == 0) ? 1 : 0;

   return ret;
}

int sfkPackEnd(SFKPackStream *p)
{
   z_stream *pstrm = (z_stream *)p->pstream;

   int ret = 0;
   p->szerr[0] = '\0';

   if (p->bPack)
      ret = deflateEnd(pstrm);
   else
      ret = inflateEnd(pstrm);

   p->pstream = 0;

   return ret;
}

void zipGetStatus(void *file, sfkuint64 *pTotalOut)
{
   zip64_internal* zi = (zip64_internal *)file;
   *pTotalOut = zi->ci.totalCompressedData;
}

int sfkOpenNewFileInZip(void *file,
      const char* filename, const zip_fileinfo* zipfi,
      const void* extrafield_local, uInt size_extrafield_local,
      const void* extrafield_global, uInt size_extrafield_global,
      const char* comment, int method, int level, int raw,
      int windowBits,int memLevel, int strategy,
      const char* password, uLong crcForCrypting,
      uLong versionMadeBy, uLong flagBase, int zip64
 )
{
    zip64_internal* zi;
    uInt size_filename;
    uInt size_comment;
    uInt i;
    int err = ZIP_OK;
#    ifdef NOCRYPT
    (crcForCrypting);
    if (password != NULL)
        return ZIP_PARAMERROR;
#    endif
    if (file == NULL)
        return ZIP_PARAMERROR;
#ifdef HAVE_BZIP2
    if ((method!=0) && (method!=Z_DEFLATED) && (method!=Z_BZIP2ED))
      return ZIP_PARAMERROR;
#else
    if ((method!=0) && (method!=Z_DEFLATED))
      return ZIP_PARAMERROR;
#endif
    zi = (zip64_internal*)file;
    if (zi->in_opened_file_inzip == 1)
    {
        err = zipCloseFileInZip (file);
        if (err != ZIP_OK)
            return err;
    }
    if (filename==NULL)
        filename="-";
    if (comment==NULL)
        size_comment = 0;
    else
        size_comment = (uInt)strlen(comment);
    size_filename = (uInt)strlen(filename);
    if (zipfi == NULL)
        zi->ci.dosDate = 0;
    else
    {
        if (zipfi->dosDate != 0)
            zi->ci.dosDate = zipfi->dosDate;
        else
          zi->ci.dosDate = zip64local_TmzDateToDosDate(&zipfi->tmz_date);
    }

    zi->ci.zip64 = zip64; // fix: stw

    zi->ci.flag = flagBase;
    if ((level==8) || (level==9))
      zi->ci.flag |= 2;
    if (level==2)
      zi->ci.flag |= 4;
    if (level==1)
      zi->ci.flag |= 6;
    if (password != NULL)
      zi->ci.flag |= 1;
    zi->ci.crc32 = 0;
    zi->ci.method = method;
    zi->ci.encrypt = 0;
    zi->ci.stream_initialised = 0;
    zi->ci.pos_in_buffered_data = 0;
    zi->ci.raw = raw;
    zi->ci.pos_local_header = ZTELL64(zi->z_filefunc,zi->filestream);
    zi->ci.size_centralheader = SIZECENTRALHEADER + size_filename + size_extrafield_global + size_comment;
    zi->ci.size_centralExtraFree = 32; // Extra space we have reserved in case we need to add ZIP64 extra info data
    zi->ci.central_header = (char*)ALLOC((uInt)zi->ci.size_centralheader + zi->ci.size_centralExtraFree);
    zi->ci.size_centralExtra = size_extrafield_global;
    zip64local_putValue_inmemory(zi->ci.central_header,(uLong)CENTRALHEADERMAGIC,4);

    zip64local_putValue_inmemory(zi->ci.central_header+4,(uLong)versionMadeBy,2);

    if(zi->ci.zip64) {
      // printf("sfkOpenNew: write 45 1 %d\n",zip64);
      zip64local_putValue_inmemory(zi->ci.central_header+6,(uLong)45,2); // fix: stw
    } else {
      // printf("sfkOpenNew: write 20 0 %d\n",zip64);
      zip64local_putValue_inmemory(zi->ci.central_header+6,(uLong)20,2);
    }

    zip64local_putValue_inmemory(zi->ci.central_header+8,(uLong)zi->ci.flag,2);
    zip64local_putValue_inmemory(zi->ci.central_header+10,(uLong)zi->ci.method,2);
    zip64local_putValue_inmemory(zi->ci.central_header+12,(uLong)zi->ci.dosDate,4);
    zip64local_putValue_inmemory(zi->ci.central_header+16,(uLong)0,4);
    zip64local_putValue_inmemory(zi->ci.central_header+20,(uLong)0,4);
    zip64local_putValue_inmemory(zi->ci.central_header+24,(uLong)0,4);
    zip64local_putValue_inmemory(zi->ci.central_header+28,(uLong)size_filename,2);
    zip64local_putValue_inmemory(zi->ci.central_header+30,(uLong)size_extrafield_global,2);
    zip64local_putValue_inmemory(zi->ci.central_header+32,(uLong)size_comment,2);
    zip64local_putValue_inmemory(zi->ci.central_header+34,(uLong)0,2);
    if (zipfi==NULL)
        zip64local_putValue_inmemory(zi->ci.central_header+36,(uLong)0,2);
    else
        zip64local_putValue_inmemory(zi->ci.central_header+36,(uLong)zipfi->internal_fa,2);
    if (zipfi==NULL)
        zip64local_putValue_inmemory(zi->ci.central_header+38,(uLong)0,4);
    else
        zip64local_putValue_inmemory(zi->ci.central_header+38,(uLong)zipfi->external_fa,4);
    if(zi->ci.pos_local_header >= 0xffffffff)
      zip64local_putValue_inmemory(zi->ci.central_header+42,(uLong)0xffffffff,4);
    else
      zip64local_putValue_inmemory(zi->ci.central_header+42,(uLong)zi->ci.pos_local_header - zi->add_position_when_writing_offset,4);
    for (i=0;i<size_filename;i++)
        *(zi->ci.central_header+SIZECENTRALHEADER+i) = *(filename+i);
    for (i=0;i<size_extrafield_global;i++)
        *(zi->ci.central_header+SIZECENTRALHEADER+size_filename+i) =
              *(((const char*)extrafield_global)+i);
    for (i=0;i<size_comment;i++)
        *(zi->ci.central_header+SIZECENTRALHEADER+size_filename+
              size_extrafield_global+i) = *(comment+i);
    if (zi->ci.central_header == NULL)
        return ZIP_INTERNALERROR;
    zi->ci.zip64 = zip64;
    zi->ci.totalCompressedData = 0;
    zi->ci.totalUncompressedData = 0;
    zi->ci.pos_zip64extrainfo = 0;
    err = Write_LocalFileHeader(zi, filename, size_extrafield_local, extrafield_local);
#ifdef HAVE_BZIP2
    zi->ci.bstream.avail_in = (uInt)0;
    zi->ci.bstream.avail_out = (uInt)Z_BUFSIZE;
    zi->ci.bstream.next_out = (char*)zi->ci.buffered_data;
    zi->ci.bstream.total_in_hi32 = 0;
    zi->ci.bstream.total_in_lo32 = 0;
    zi->ci.bstream.total_out_hi32 = 0;
    zi->ci.bstream.total_out_lo32 = 0;
#endif
    zi->ci.stream.avail_in = (uInt)0;
    zi->ci.stream.avail_out = (uInt)Z_BUFSIZE;
    zi->ci.stream.next_out = zi->ci.buffered_data;
    zi->ci.stream.total_in = 0;
    zi->ci.stream.total_out = 0;
    zi->ci.stream.data_type = Z_BINARY;
#ifdef HAVE_BZIP2
    if ((err==ZIP_OK) && (zi->ci.method == Z_DEFLATED || zi->ci.method == Z_BZIP2ED) && (!zi->ci.raw))
#else
    if ((err==ZIP_OK) && (zi->ci.method == Z_DEFLATED) && (!zi->ci.raw))
#endif
    {
        if(zi->ci.method == Z_DEFLATED)
        {
          zi->ci.stream.zalloc = (alloc_func)0;
          zi->ci.stream.zfree = (free_func)0;
          zi->ci.stream.opaque = (voidpf)0;
          if (windowBits>0)
              windowBits = -windowBits;
          err = deflateInit2(&zi->ci.stream, level, Z_DEFLATED, windowBits, memLevel, strategy);
          if (err==Z_OK)
              zi->ci.stream_initialised = Z_DEFLATED;
        }
        else if(zi->ci.method == Z_BZIP2ED)
        {
#ifdef HAVE_BZIP2
            // Init BZip stuff here
          zi->ci.bstream.bzalloc = 0;
          zi->ci.bstream.bzfree = 0;
          zi->ci.bstream.opaque = (voidpf)0;
          err = sp_BZ2_bzCompressInit(&zi->ci.bstream, level, 0,35);
          if(err == BZ_OK)
            zi->ci.stream_initialised = Z_BZIP2ED;
#endif
        }
    }
#    ifndef NOCRYPT
    zi->ci.crypt_header_size = 0;
    if ((err==Z_OK) && (password != NULL))
    {
        unsigned char bufHead[RAND_HEAD_LEN];
        unsigned int sizeHead;
        zi->ci.encrypt = 1;
        zi->ci.pcrc_32_tab = get_crc_table();

        sizeHead=crypthead(password,bufHead,RAND_HEAD_LEN,zi->ci.keys,zi->ci.pcrc_32_tab,crcForCrypting);
        zi->ci.crypt_header_size = sizeHead;
        if (ZWRITE64(zi->z_filefunc,zi->filestream,bufHead,sizeHead) != sizeHead)
                err = ZIP_ERRNO;
    }
#    endif
    if (err==Z_OK)
        zi->in_opened_file_inzip = 1;
    return err;
}

#ifdef TESTCOMP

#include <stdarg.h>

#ifdef _WIN32
 typedef __int64 num;
#else
 typedef long long num;
#endif

static int perr(const char *pszFormat, ...)
{
   va_list argList;
   va_start(argList, pszFormat);
   char szBuf[1024];
   ::vsprintf(szBuf, pszFormat, argList);
   fprintf(stderr, "error: %s", szBuf);
   return 0;
}

#define CHUNK 16384
#define USE_GZIP

num nDoneIn=0,nDoneOut=0;

int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    #ifdef USE_GZIP
    int windowBits = 15;
    int GZIP_ENCODING = 16;
    ret = deflateInit2 (&strm,
            Z_DEFAULT_COMPRESSION,
            Z_DEFLATED,
            windowBits | GZIP_ENCODING,
            8,
            Z_DEFAULT_STRATEGY);
    #else
    ret = deflateInit(&strm, level);
    #endif

    if (ret != Z_OK)
        return ret;

    do
    {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        // printf("Read %d eof=%d\n", strm.avail_in, flush);
        nDoneIn += strm.avail_in;

        do
        {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            have = CHUNK - strm.avail_out;

            // printf("Write %d\n", have);
            if (fwrite(out, 1, have, dest) != have || ferror(dest))
            {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
            nDoneOut += have;

        } while (strm.avail_out == 0);

    } while (flush != Z_FINISH);

    (void)deflateEnd(&strm);
    return Z_OK;
}

int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    #ifdef USE_GZIP
    ret = inflateInit2(&strm, 16+MAX_WBITS);
    #else
    ret = inflateInit(&strm);
    #endif

    if (ret != Z_OK)
        return ret;

    do
    {
        strm.avail_in = fread(in, 1, CHUNK, source);

        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }

        if (strm.avail_in == 0)
            break;

        strm.next_in = in;

        nDoneIn += strm.avail_in;

        do
        {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;

            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
            nDoneOut += have;

        } while (strm.avail_out == 0);

    } while (ret != Z_STREAM_END);

    (void)inflateEnd(&strm);

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int execPackFile(FILE *pin, FILE *pout, bool bPack, char *pszFilename)
{
   int iInBufSize  = 100000;
   int iOutBufSize = 100000;

   char *pInBuf  = new char[iInBufSize+100];
   char *pOutBuf = new char[iOutBufSize+100];

   if (!pInBuf || !pOutBuf)
      return 9+perr("outofmem\n");

   int nLastDoneMB = 0;

   SFKPackStream ostrm;
   memset(&ostrm, 0, sizeof(ostrm));

   ostrm.bPack = bPack;

   sfkPackStart(&ostrm);

   bool bLastBlock = 0;
   int  bReloop = 0, nRead = 0, isubrc = 0;

   int irc = 0;

   while (1)
   {
      if (bReloop == 0)
      {
         int iMaxRead = iInBufSize;

         nRead  = fread(pInBuf, 1, iMaxRead, pin);

         if (nRead < 1)
            break;
         if (nRead < iMaxRead)
            bLastBlock = 1;
 
         nDoneIn += nRead;
 
         ostrm.pin  = (uchar*)pInBuf;
         ostrm.nin  = nRead;
      }

      ostrm.pout = (uchar*)pOutBuf;
      ostrm.nout = iOutBufSize;

      isubrc = sfkPackProc(&ostrm, bLastBlock, &bReloop);

      if (isubrc > 1)
      {
         perr("extract error %d\n",isubrc);
         irc = isubrc;
         break;
      }

      if (ostrm.nout > 0) {
         if (fwrite(pOutBuf, 1, ostrm.nout, pout) < ostrm.nout) {
            perr("cannot fully write, disk full\n");
            irc = 9;
            break;
         }
      }

      nDoneOut += ostrm.nout;

      int iDoneMB = nDoneIn/1000000;
      int iOutMB  = nDoneOut/1000000;
      if (iDoneMB != nLastDoneMB)
      {
         nLastDoneMB = iDoneMB;
         printf("%05d/%05d mb %s %s\r",
            iDoneMB, iOutMB, bPack ? "pack":"unpk", pszFilename);
      }
   }

   sfkPackEnd(&ostrm);

   delete [] pInBuf;
   delete [] pOutBuf;

   return irc;
}

/*
cl -DTESTCOMP -omycomp.exe sfkpack.cpp
*/

int main(int argc, char *argv[])
{
   if (argc<4)
   {
      printf("usage: mycomp c in.dat out.gz\n"
             "       mycomp d in.gz  out.dat\n");
      return 0;
   }

   char *pszMode    = argv[1];
   char *pszInFile  = argv[2];
   char *pszOutFile = argv[3];

   int err = 0;
   int bcomp = 0;

   if (!strcmp(pszMode, "c"))
      bcomp = 1;
   else
   if (!strcmp(pszMode, "d"))
      bcomp = 0;
   else
      return 9+perr("wrong mode\n");

   FILE *fin = fopen(pszInFile,"rb");
   if (!fin)
      return 9+perr("cannot read\n");

   FILE *fout=fopen(pszOutFile,"wb");
   if (!fout)
      return 9+perr("cannot write\n");

   #if 1

   err = execPackFile(fin, fout, bcomp, pszInFile);

   #else

   if (bcomp)
      err = def(fin, fout, Z_DEFAULT_COMPRESSION);
   else
      err = inf(fin, fout);

   #endif

   fclose(fout);
   fclose(fin);

   if (err > 0)
      printf("rc %d\n",err);

   #ifdef _WIN32
   printf("Done from %I64d to %I64d bytes.\n", nDoneIn, nDoneOut);
   #else
   printf("Done from %lld to %lld bytes.\n", nDoneIn, nDoneOut);
   #endif

   return 0;
}

#endif

