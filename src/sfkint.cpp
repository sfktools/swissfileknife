/*
   sfk internal expererimental code, not part of the main sfk binary.

   this is included by sfkext.cpp.
*/

extern int nGlblFunc;

// - - - sfkpic - - -

#ifdef SFKPIC

uint swaprb(uint c)
{
   uchar a=(uchar)(c >> 24);
   uchar r=(uchar)(c >> 16);
   uchar g=(uchar)(c >>  8);
   uchar b=(uchar)(c >>  0);
   return
         (((uint)a)<<24)
      |  (((uint)b)<<16)
      |  (((uint)g)<< 8)
      |  (((uint)r)<< 0);
}

uint *sfkPicAllocFunc(int nWords)
{
   return new uint[nWords];
}

SFKPic::SFKPic( )
{
   memset(this, 0, sizeof(*this));
}

SFKPic::~SFKPic( )
{
   freepix();
}

int SFKPic::getObjectSize( )
{
   return (int)sizeof(*this);
}

uint SFKPic::pix(uchar a,uchar r,uchar g,uchar b)
{
   return
         (((uint)a) << 24)
      |  (((uint)r) << 16)
      |  (((uint)g) <<  8)
      |  (((uint)b) <<  0)
      ;
}

void SFKPic::freepix( )
{
   if (octl.ppix)
      delete [] octl.ppix;

   octl.ppix   = 0;
   octl.npix   = 0;
   octl.model  = 0;
}

int SFKPic::allocpix(uint w, uint h)
{
   freepix();

   uint nTotalPix = w * h;

   if (!(octl.ppix = new uint[nTotalPix+100]))
      return 9;

   memset(octl.ppix, 0xFF, nTotalPix * sizeof(uint));

   octl.width     = w;
   octl.height    = h;
   octl.npix      = nTotalPix;
   octl.model     = 0;
   octl.filecomp  = 4;

   return 0;
}

int SFKPic::load(char *pszFile)
{
   uchar *pPack = 0;
   num    nPack = 0;
   if (!(pPack = loadBinaryFlex(pszFile, nPack)))
      return 9+pferr(pszFile, "cannot load: %s", pszFile);

   uint *ppix = 0;
 
   if (cs.srcpicwidth)
   {
      do
      {
         int csrc = cs.srcpicchan;
         if (csrc == 0) csrc = 3;

         int wsrc = cs.srcpicwidth;
         int hsrc = nPack / (wsrc * csrc);
         int npix = wsrc * hsrc;
 
         ppix = new uint[npix+10];
         if (!ppix)
            { perr("outofmem"); break; }
         memset(ppix, 0, npix * sizeof(uint));

         uchar *psrc = pPack;
         int isrc = 0;
         int idst = 0;
         uint r,g,b,c,a;

         for (int y=0; y<hsrc; y++)
         {
            for (int x=0; x<wsrc; x++)
            {
               isrc = y * wsrc * csrc + x * csrc;
               idst = y * wsrc + x;

               r = psrc[isrc+0];

               a = 0xff;

               switch (csrc)
               {
                  case 1:
                     r = psrc[isrc+0];
                     g = r;
                     b = r;
                     break;
                  case 3:
                     r = psrc[isrc+0];
                     g = psrc[isrc+1];
                     b = psrc[isrc+2];
                     break;
                  case 4:
                     r = psrc[isrc+0];
                     g = psrc[isrc+1];
                     b = psrc[isrc+2];
                     a = psrc[isrc+3];
                     break;
               }

               c = pix(a, r, g, b);

               ppix[idst] = c;
            }
         }

         memset(&octl, 0, sizeof(octl));

         octl.width  = wsrc;
         octl.height = hsrc;
         octl.filecomp = csrc;
         octl.outcomp  = 3;

         octl.ppix   = ppix;
         octl.npix   = npix;
      }
      while (0);
   }
   else
   {
      ppix = loadpic(pPack,nPack,&octl);
   }

   delete [] pPack;

   return ppix ? 0 : 9;
}

int SFKPic::load(uchar *pPacked, int nPacked)
{
   uint *ppix = loadpic(pPacked,nPacked,&octl);

   return ppix ? 0 : 9;
}

// .
int SFKPic::save(char *pszFile)
{
   if (endsWithExt(pszFile,str(".png")))  octl.fileformat=1;
   if (endsWithExt(pszFile,str(".jpg")))  octl.fileformat=2;
   if (endsWithExt(pszFile,str(".jpeg"))) octl.fileformat=2;

   // allow 4 kb of meta data around pixels,
   // like palette, zip dictionaries etc.
   uint   nPack = octl.width*octl.height*4 + 4096;
   uchar *pPack = new uchar[nPack+100];
   if (!pPack)
      return 10+perr("outofmem\n");

   int isubrc = packpic(octl.ppix, &octl, &pPack, &nPack);
   if (isubrc)
      return isubrc+perr("packpic rc %d",isubrc);

   isubrc = saveFile(pszFile,pPack,nPack,"wb");

   delete [] pPack;

   return isubrc;
}

int SFKPic::getErrNum( ) { return octl.rc; }
char *SFKPic::getErrStr( ) { return octl.szerror; }

void SFKPic::setpix(uint x, uint y, uint c)
{
   if (!octl.ppix)
      return;
   uint ioff = y * octl.width + x;
   if (ioff >= octl.npix) {
      // printf("setpix.err\n");
      return;
   }
   octl.ppix[ioff] = c;
}

uint SFKPic::getpix(uint x, uint y)
{
   if (!octl.ppix)
      return 0;
   uint ioff = y * octl.width + x;
   if (ioff >= octl.npix) {
      // printf("getpix.err\n");
      return 0;
   }
   return octl.ppix[ioff];
}

void SFKPic::drawrect(int x1, int y1, int x2, int y2, uint c, int bfill)
{
   for (int y=y1; y<y2; y++)
   {
      for (int x=x1; x<x2; x++)
      {
         if (bfill == 0
             && (y > y1+1 && y+2 < y2)
             && (x > x1+1 && x+2 < x2))
            continue;
         setpix(x,y,c);
      }
   }
}

// .
void SFKPic::copyFrom(SFKPic *pSrc, uint x1dst, uint y1dst, uint wdst, uint hdst, uint x1src, uint y1src, uint wsrc, uint hsrc)
{
   if (!octl.ppix || !pSrc->octl.ppix)
      return;
   if (!octl.npix) { perr("copyFrom target npix error"); return; }
   if (!pSrc->octl.npix) { perr("copyFrom source npix error"); return; }

   if (wsrc==wdst && hsrc==hdst)
   {
      if (pSrc->octl.model == octl.model
          && pSrc->octl.npix == octl.npix)
      {
         memcpy(octl.ppix, pSrc->octl.ppix, octl.npix*4); // sfk1992 optim
      }
      else
      {
         for (int i=0; i<wdst; i++) {
            for (int k=0; k<hdst; k++) {
               setpix(x1dst+i,y1dst+k,pSrc->getpix(x1src+i,y1src+k));
            }
         }
      }
      return;
   }

   // todo: total failure if source offset is nonzero!

   uint iSrcWidth        = wsrc;
   uint iSrcHeight       = hsrc;
   uint iDstWidth        = wdst;
   uint iDstHeight       = hdst;

   uint dst_x1=0, dst_y1=0, dst_off=0;

   double HFactor = double(iSrcHeight) / iDstHeight;
   double WFactor = double(iSrcWidth)  / iDstWidth;

   uint srcpixxmax = iSrcWidth+x1src - 1;  // fix sfk1982 +x1src
   uint srcpixymax = iSrcHeight+y1src - 1; // fix sfk1982 +y1src

   double srcpixy, srcpixy1, srcpixy2, dy, dy1;
   double srcpixx, srcpixx1, srcpixx2, dx, dx1;

   double r1, g1, b1, a1 = 0;
   double r2, g2, b2, a2 = 0;
 
   int e1=0,e2=0;

   for (uint dsty = 0; dsty < iDstHeight; dsty++)
   {
      srcpixy  = double(dsty) * HFactor;
      srcpixy1 = (uint)(srcpixy + y1src);
      srcpixy2 = ( srcpixy1 == srcpixymax ) ? srcpixy1 : srcpixy1 + 1.0;

      dy  = srcpixy - (uint)srcpixy;
      dy1 = 1.0 - dy;

      int e1=0,e2=0;

      for (uint dstx = 0; dstx < iDstWidth; dstx++)
      {
         srcpixx  = double(dstx) * WFactor;
         srcpixx1 = (uint)(srcpixx + x1src);
         srcpixx2 = ( srcpixx1 == srcpixxmax ) ? srcpixx1 : srcpixx1 + 1.0;

         dx  = srcpixx - (int)srcpixx;
         dx1 = 1.0 - dx;

         uint x_offset1 = srcpixx1 > srcpixxmax ? srcpixxmax : (uint)srcpixx1;
         uint x_offset2 = srcpixx2 > srcpixxmax ? srcpixxmax : (uint)srcpixx2;

         uint y_offset1 = srcpixy1 > srcpixymax ? srcpixymax : (uint)srcpixy1;
         uint y_offset2 = srcpixy2 > srcpixymax ? srcpixymax : (uint)srcpixy2;

         if (x_offset1 > srcpixxmax) {e1++; break;}
         if (x_offset2 > srcpixxmax) {e1++; break;}
         if (y_offset1 > srcpixymax) {e1++; break;}
         if (y_offset2 > srcpixymax) {e1++; break;}
 
         uint csrc1 = pSrc->getpix(x_offset1, y_offset1);
         uint csrc2 = pSrc->getpix(x_offset2, y_offset1);
         uint csrc3 = pSrc->getpix(x_offset1, y_offset2);
         uint csrc4 = pSrc->getpix(x_offset2, y_offset2);

         // first line
         r1 =   red(csrc1) * dx1 + red(csrc2) * dx;
         g1 =   grn(csrc1) * dx1 + grn(csrc2) * dx;
         b1 =   blu(csrc1) * dx1 + blu(csrc2) * dx;
         a1 =   alp(csrc1) * dx1 + alp(csrc2) * dx;

         // second line
         r2 =   red(csrc3) * dx1 + red(csrc4) * dx;
         g2 =   grn(csrc3) * dx1 + grn(csrc4) * dx;
         b2 =   blu(csrc3) * dx1 + blu(csrc4) * dx;
         a2 =   alp(csrc3) * dx1 + alp(csrc4) * dx;

         // result lines
         dst_x1  = dstx + x1dst;
         dst_y1  = dsty + y1dst;
         dst_off = dst_y1 * octl.width + dst_x1;

         if (dst_off < 0 || dst_off > octl.npix)
            {e2++; break;}

         uchar rdst = (uchar)(r1 * dy1 + r2 * dy);
         uchar gdst = (uchar)(g1 * dy1 + g2 * dy);
         uchar bdst = (uchar)(b1 * dy1 + b2 * dy);
         uchar adst = 0xFFU;
         adst = (uchar)(a1 * dy1 + a2 * dy);

         setpix(dst_x1, dst_y1, pix(adst, rdst,gdst,bdst));
      }
   }
   // if (e1 || e2) printf("copypix.err: %d %d\n",e1,e2);
}

int SFKPic::rotate(int iang)
{
   struct SFKPicData oout;
   memset(&oout, 0, sizeof(oout));

   if (iang==180) {
      oout.width = octl.width;
      oout.height = octl.height;
   } else {
      oout.width = octl.height;
      oout.height = octl.width;
   }
   oout.npix = oout.width*oout.height;
   oout.ppix = new uint[oout.npix+100];
   if (!oout.ppix) return 9;

   int x2,y2;

   if (iang==270) {
      for (int x1=0;x1<octl.width;x1++) {
         for (int y1=0;y1<octl.height;y1++) {
            x2 = y1;
            y2 = octl.width-1-x1;
            oout.ppix[y2*oout.width+x2] = octl.ppix[y1*octl.width+x1];
         }
      }
   }
   else if (iang==90) {
      for (int x1=0;x1<octl.width;x1++) {
         for (int y1=0;y1<octl.height;y1++) {
            x2 = octl.height-1-y1;
            y2 = x1;
            oout.ppix[y2*oout.width+x2] = octl.ppix[y1*octl.width+x1];
         }
      }
   }
   else if (iang==180) {
      for (int x1=0;x1<octl.width;x1++) {
         for (int y1=0;y1<octl.height;y1++) {
            x2 = octl.width-1-x1;
            y2 = octl.height-1-y1;
            oout.ppix[y2*oout.width+x2] = octl.ppix[y1*octl.width+x1];
         }
      }
   }

   delete [] octl.ppix;
   octl.ppix = oout.ppix;
   octl.npix = oout.npix;
   octl.width = oout.width;
   octl.height = oout.height;

   return 0;
}

// dmod prog
#if (sfk_prog)

int execPic(Coi *pcoi, char *pszOutFile)
{
   char szPwdBuf[SFK_MAX_PATH+100];
   char szAbsNameBuf[SFK_MAX_PATH+100];

   SFKPic opic,opic2;
   SFKPicData &opicd = opic.octl;
   char *pszFilename = pcoi->name();

   info.setAddInfo("%u images", cs.filesScanned);
   info.setStatus("scan ", pcoi->name(), 0, eKeepAdd);

   // can it be an image file at all?
   if (!cs.deeppic)
   {
      if (endsWithExt(pszFilename, str(".jpg"))
          || endsWithExt(pszFilename, str(".jpeg"))
          || endsWithExt(pszFilename, str(".png"))
         )
         { }
      else {
         cs.nopicfiles++;
         if (cs.verbose)
            info.print("no image: %s\n", pcoi->name());
         return 5;
      }
   }
   else
   {
      uchar abData[100];
      memset(abData, 0, sizeof(abData));
 
      if (pcoi->open("rb")) {
         cs.nopicfiles++;
         if (cs.verbose)
            info.print("cannot open: %s\n", pcoi->name());
         return 5;
      }
      pcoi->read(abData, 32);
      pcoi->close();
 
      if (abData[0]==0xFF && abData[1]==0xD8)
         { }
      else
      if (abData[0]==0x89 && abData[1]==0x50
          && abData[2]==0x4E && abData[3]==0x47)
         { }
      else {
         cs.nopicfiles++;
         if (cs.verbose)
            info.print("no image: %s\n", pcoi->name());
         return 5;
      }
   }

   // is it too large to load?
   num nFileSize = pcoi->getSize();
   if (nFileSize > nGlblMemLimit) {
      if (!cs.quiet) {
         if (cs.verbose)
            pinf("file too large to load: %s, %s mbytes\n",
               pcoi->name(), numtoa(nFileSize/1000000));
         tellMemLimitInfo();
      }
      return 5;
   }

   opicd.loadflags = cs.picloadflags;

   // try to load
   if (opic.load(pcoi->name())) {
      cs.nopicfiles++;
      if (cs.verbose)
         info.print("cannot load: %s\n", pcoi->name());
      return 5;
   }

   if (cs.minwidth > 0 && opic.width() < cs.minwidth) {
      if (cs.verbose) info.print("too small: %s\n", pcoi->name());
      return 1;
   }
   if (cs.minheight > 0 && opic.height() < cs.minheight) {
      if (cs.verbose) info.print("too small: %s\n", pcoi->name());
      return 1;
   }

   cchar *pform = "?";
   switch (opicd.fileformat) {
      case 0: pform = "---"; break;
      case 1: pform = "png"; cs.pnginfiles++; break;
      case 2: pform = "jpg"; cs.jpginfiles++; break;
   }

   if (!cs.pszgallery) info.clear();

   char *pext = "";
   switch (opicd.fileformat) {
      case 1: pext = "png"; break;
      case 2: pext = "jpg"; break;
   }
   mystrcopy(pcoi->szClOutExt, pext, sizeof(pcoi->szClOutExt)-2);

   if (cs.todir)
   {
      cs.procpic = 1;
      int nrc = renderOutMask(szRefNameBuf, pcoi, "$relpath-$base.$outext", cs.curcmd); // [f]pic
      if (nrc >= 9) {
         if (cs.verbose) info.print("cannot render output filename, rc=%d\n", nrc);
         return nrc;
      }
      for (char *psz=szRefNameBuf; *psz; psz++)
         if (isUniPathChar(*psz))
            *psz = '-';
      joinPath(szOutNameBuf, sizeof(szOutNameBuf)-10, cs.todir, szRefNameBuf);
      pszOutFile = szOutNameBuf;
   }
   else
   if (cs.tomask && !cs.tomaskfile)
   {
      cs.procpic = 1;
      int nrc = renderOutMask(szOutNameBuf, pcoi, cs.tomask, cs.curcmd); // [f]pic
      if (nrc >= 9) {
         if (cs.verbose) info.print("cannot render output filename, rc=%d\n", nrc);
         return nrc;
      }
      pszOutFile = szOutNameBuf;
   }

   cs.filesScanned++;

   if (!cs.dumppix && !cs.yes)
   {
      char *pname = pcoi->name();
      if (cs.listTargets && pszOutFile)
            pname = pszOutFile;
      printx("$%s<def> #%4u %4u %u<def><nocol> %s\n",
         pform,
         opic.octl.width, opic.octl.height, opic.octl.filecomp,
         pname);
      return 0;
   }

   // ----- process image -----

   SFKPic *pout = &opic;

   if (cs.dstpicwidth || cs.dstpicheight)
   {
      int targw = cs.dstpicwidth;
      int targh = cs.dstpicheight;

      if (cs.pszgallery != 0)
      {
         if (targh > 0 && opic.octl.height < targh)
             targh = opic.octl.height;
         if (targw > 0 && opic.octl.width < targw)
             targw = opic.octl.width;
      }

      if (targw && !targh)
         targh = ((num)opic.octl.height * (num)targw) / opic.octl.width;
      else
      if (targh && !targw)
         targw = ((num)opic.octl.width * (num)targh) / opic.octl.height;
      else
      if (!targw && !targh) {
         targw = opic.width();
         targh = opic.height();
      }

      if (opic2.allocpix(targw,targh))
         return 9+perr("cannot allocate %ux%u image",targw,targh);

      pout = &opic2;
      pout->copyFrom(&opic,0,0,targw,targh,0,0,opic.width(),opic.height());
   }

   // convert and write output
   cs.filesChg++;

   if (cs.pszgallery)
   {
      if (!cs.pfgallery)
      {
         if (!(cs.pfgallery = fopen(cs.pszgallery, "wb")))
            return 19+perr("cannot write: %s\n",cs.pszgallery);
         fprintf(cs.pfgallery, "<html><body style=\"background-color:#dddddd;\">\n");
      }

      // . pack as jpeg
      uint   nOutSize = pout->width()*pout->height()*4+4096;
      uchar *pOutPack = new uchar[nOutSize+100];
      if (!pOutPack)
         return 19+perr("outofmem\n");
 
      pout->octl.fileformat  = 1;
      // pout->octl.outcomp     = 3;
      // pout->octl.filequality = 90;
 
      uint rc = packpic(pout->octl.ppix, &pout->octl, &pOutPack, &nOutSize);

      int    nmax64  = nOutSize*2;
      uchar *pdump64 = new uchar[nmax64+100];
      if(!pdump64)
         return 19+perr("outofmem");

      // convert to base64
      if (encode64(pOutPack,nOutSize, pdump64,nmax64, 512))
         return 19+perr("output encoding failed\n");

      char *pf = pcoi->name();
      if (cs.forceabsname) {
         if (!isAbsolutePath(pcoi->name())) {
            szPwdBuf[0] = szAbsNameBuf[0] = '\0';
            if (getcwd(szPwdBuf,SFK_MAX_PATH)) { }
            joinPath(szAbsNameBuf, SFK_MAX_PATH, szPwdBuf, pcoi->name());
            pf = szAbsNameBuf;
         }
      }

      fprintf(cs.pfgallery, "<a href=\"%s\">",pf);
      fprintf(cs.pfgallery, "<img src=\"data:image/png;base64,");
      fwrite(pdump64,1,strlen((char*)pdump64),cs.pfgallery);
      fprintf(cs.pfgallery, "\"></a>\n");

      cs.jpgoutfiles++;

      // add to pfgallery
   }
   else
   if (pszOutFile)
   {
      if (   endsWithExt(pszOutFile, str(".jpg"))
          || endsWithExt(pszOutFile, str(".jpeg"))
         )
      {
         cs.jpgoutfiles++;
         pout->octl.outcomp = 3;
         pout->octl.filequality = 90;
         if (cs.dstpicquality)
            pout->octl.filequality = cs.dstpicquality;
         // jpeg to png: always reduce channels
         if (opic.octl.filecomp == 4) {
            pout->octl.mixonpack = 1;
            pout->octl.backcolor = cs.backcol;
         }
      }
      else if (endsWithExt(pszOutFile, str(".png")))
      {
         cs.pngoutfiles++;
         // png to png: reduce on demand
         if (pout->octl.filecomp == 4) {
            if (cs.notrans) {
               pout->octl.outcomp = 3;
               pout->octl.mixonpack = 1;
               pout->octl.backcolor = cs.backcol;
            }
         } else {
            pout->octl.outcomp=3;
         }
      }
      else
         cs.rgboutfiles++;

      // if (cs.dstchan > 0)
      //    pout->octl.outcomp = cs.dstchan;

      printx("$save #%4u %4u %u<def> <nocol>%s\n",
         pout->octl.width,
         pout->octl.height,
         pout->octl.outcomp ? pout->octl.outcomp : pout->octl.filecomp,
         pszOutFile
         );

      if (pout->save(pszOutFile)) {
         opic2.freepix();
         return 9+perr("cannot save: %s",pszOutFile);
      }
   }
   else if (cs.dumppix)
   {
      int nbytes = pout->octl.width * pout->octl.height*4;
      if (chain.coldata && chain.colbinary)
      {
         chain.addBinary((uchar*)pout->octl.ppix,nbytes);
      }
      else
      {
         printx("$%s\n", pcoi->name());
         // termHexdump((uchar*)pout->octl.ppix,nbytes);
         dumpOutput((uchar*)pout->octl.ppix, 0, nbytes, 1);
      }
   }

   opic2.freepix();

   return 0;
}
// emod prog
#endif // (sfk_prog)

#endif // SFKPIC

static cchar *pszPicReaderTpl1 =
"<html><head><title>image reader</title>\n"
"<script type=\"text/javascript\">\n"
"var apic = new Array();\n"
"apic = [\n";

static cchar *pszPicReaderTpl2 =
"   ];\n"
"var ipic = 0;\n"
"function gopic() {\n"
"   var opic = document.getElementById(\"pic\");\n"
"   opic.src = apic[ipic];\n"
"   document.title = apic[ipic];\n"
"   scroll(0,0);\n"
"}\n"
"function doprev() { if (ipic > 0) ipic--; else ipic = apic.length-1; gopic(); }\n"
"function donext() { if (ipic < apic.length-1) ipic++; else ipic = 0; gopic(); }\n"
"function doinit() {\n"
"   gopic();\n"
"   var w     = document.body.clientWidth;\n"
"   var h     = document.body.clientHeight;\n"
"   var oarea = document.getElementById(\"aprev\");\n"
"   oarea.coords = \"0,0,\"+(w/2)+\",\"+(h*2);\n"
"   var oarea = document.getElementById(\"anext\");\n"
"   oarea.coords = \"\"+(w/2)+\",0,\"+w+\",\"+(h*2);\n"
"}\n"
"</script></head>\n"
"<body bgcolor=\"#aaaaaa\" id=\"doc\" leftmargin=\"0\" topmargin=\"0\" marginwidth=\"0\" marginheight=\"0\" onload=\"javascript:doinit()\">\n"
" <center>\n"
" <img id=\"pic\" src=\"tmp.png\" ";

static cchar *pszPicReaderTpl3 =
" usemap=\"#mymap\">\n"
" <map name=\"mymap\">\n"
"  <area id=\"aprev\" shape=\"rect\" coords=\"0,0,400,400\"   title=\"previous\" onclick=\"javascript:doprev()\">\n"
"  <area id=\"anext\" shape=\"rect\" coords=\"0,400,400,800\" title=\"next\"     onclick=\"javascript:donext()\">\n"
" </map>\n"
"</body>\n"
"</html>\n"
;

static cchar *pszPicListTpl1 =
"<html><body bgcolor=\"#cccccc\">\n"
;

static cchar *pszPicListTpl2 =
"</body></html>\n"
;

int execToHtml(int imode, int iaspect, char *plist, char *pszOutFile)
{
   FILE *fout = fopen(pszOutFile, "w");
   if (!fout)
      return 9+perr("cannot write: %s", pszOutFile);

   if (imode==1)
      fwrite(pszPicReaderTpl1, 1, strlen(pszPicReaderTpl1), fout);
   else
      fwrite(pszPicListTpl1, 1, strlen(pszPicListTpl1), fout);

   char *pcur=plist;
   char *pmax=pcur+strlen(pcur);
   while (pcur<pmax)
   {
      char *pnext = pcur;

      while (*pnext!=0 && *pnext!='\r' && *pnext!='\n') {
         if (*pnext=='\\')
            *pnext='/';
         pnext++;
      }

      if (*pnext)
         *pnext++ = '\0';

      while (*pnext=='\r' || *pnext=='\n')
         pnext++;

      /*
         outfile: mydir\index.html
         pcur   : mydir/pic1.jpg
      */
      int ioff=0;
      for (int i=0; pcur[i] && pszOutFile[i]; i++) {
         if (ispathchr(pcur[i]) && ispathchr(pszOutFile[i]))
            ioff=i+1;
         else
         if (pcur[i] != pszOutFile[i])
            break;
      }
      pcur+=ioff; // -> pic1.jpg

      if (imode==1) {
         fprintf(fout, "   \"%s\",\n", pcur);
      } else {
         fprintf(fout,
            "<a href=\"%s\" target=\"_blank\">"
            "<img src=\"%s\" width=\"20%%\" title=\"%s\">"
            "</a> "
            , pcur
            , pcur
            , pcur
            );
      }

      pcur=pnext;
   }

   if (imode==1) {
      fwrite(pszPicReaderTpl2, 1, strlen(pszPicReaderTpl2), fout);
      if (iaspect==1)
         fprintf(fout, "width=\"94%%\"");
      else
         fprintf(fout, "height=\"94%%\"");
      fwrite(pszPicReaderTpl3, 1, strlen(pszPicReaderTpl3), fout);
   } else {
      fwrite(pszPicListTpl2, 1, strlen(pszPicListTpl2), fout);
   }

   fclose(fout);

   return 0;
}

// - - - sfkweb - - -


// =========== Profiling ==========

// inactive by default

#ifdef SFK_PROFILING

// CRITICAL: static constructor sequence
// <-> StaticPerformancePoint
StaticPerformanceStats glblPerfStats;

StaticPerformanceStats::StaticPerformanceStats( )
{
   memset(this, 0, sizeof(*this));
}

void StaticPerformanceStats::addPoint(StaticPerformancePoint *pPoint)
{
   if (iClPoints >= MAX_PERF_POINTS)
   {
      fprintf(stdout, "ERROR: too many profiling points\n");
      fprintf(stderr, "ERROR: too many profiling points\n");
      return;
   }

   apClPoints[iClPoints++] = pPoint;
}

int StaticPerformanceStats::numberOfPoints( )
   { return iClPoints; }

StaticPerformancePoint *StaticPerformanceStats::getPoint(int iIndex)
   { return apClPoints[iIndex]; }

// CRITICAL: static constructor sequence
// <-> StaticPerformanceStats
StaticPerformancePoint::StaticPerformancePoint(const char *pszID,
   const char *pszFile, int iTraceLine)
{
   pszClID = pszID;
   pszClFile = pszFile;
   iClTraceLine = iTraceLine;
   iClHits = 0;
   iClTotalTime = 0;
   iClSubTimes = 0;
   glblPerfStats.addPoint(this);
}

void StaticPerformancePoint::blockEntry( )
   { iClHits++; }

void StaticPerformancePoint::blockExit(int iElapsedTicks)
   { iClTotalTime += iElapsedTicks; }

DynamicPerformancePoint::DynamicPerformancePoint(StaticPerformancePoint *pStaticPoint)
{
   nClEntryTickCount = getPerfCnt();
   pClStaticPoint = pStaticPoint;

   // enter next level
   pClStaticParent = glblPerfStats.pClCurrentPoint;
   glblPerfStats.pClCurrentPoint = pClStaticPoint;

   pClStaticPoint->blockEntry();
}

DynamicPerformancePoint::~DynamicPerformancePoint( )
{
   num iElapsed = getPerfCnt() - nClEntryTickCount;

   pClStaticPoint->blockExit((int)iElapsed);

   // up one level
   if (pClStaticParent)
      pClStaticParent->iClSubTimes += iElapsed;
   glblPerfStats.pClCurrentPoint = pClStaticParent;
}

void logProfile( )
{
   num nTicksPerMSec = getPerfFreq() / 1000;
   if (!nTicksPerMSec)
       nTicksPerMSec = 1;

   printf("performance (%d points):\n", glblPerfStats.numberOfPoints());

   int iTotalMSec = 0;
   int iTotalPerc = 0;

   for (int ipass=0; ipass<2; ipass++)
   for (int i=0; i<glblPerfStats.numberOfPoints(); i++)
   {
      StaticPerformancePoint *p = glblPerfStats.getPoint(i);

      num nNetto = p->iClTotalTime - p->iClSubTimes;
      num nMSec  = nNetto / nTicksPerMSec;

      if (!ipass) {
         iTotalMSec += (int)nMSec;
      } else {
         int iPerc = nMSec * 100 / (iTotalMSec ? iTotalMSec : 1);
         iTotalPerc += iPerc;
         printf("% 10s : % 10I64d % 5d % 3d%% % 8I64d   %s:%d   (%d-%d)\n",
            p->pszClID,
            nNetto, (int)nMSec, iPerc,
            p->iClHits,
            p->pszClFile,
            p->iClTraceLine,
            (int)p->iClTotalTime, (int)p->iClSubTimes
            );
      }
   }

      printf("% 10s : % 10I64d % 5d % 3d%% % 8I64d   %s:%d   (%d-%d)\n",
         "total",
         (num)0, iTotalMSec, iTotalPerc,
         (num)0,
         "any",
         0,
         0, 0
         );
}

#else

void logProfile( )
{
}

#endif

#ifdef WITH_PORTMON

#define SFK_PM_MAXREC  1024
#define SFK_PM_RECSIZE  300
#define SFK_PM_MAXBUF  1000

class PortMon
{
public:
      PortMon  ( );
     ~PortMon  ( );

   void  run      ( );
   void  sendlog  (TCPCon *pcon, int bwide);

UDPCore  cludp;
TCPCore  cltcp;
UDPIO    oForward;
struct   sockaddr_in claddr;
char     clforward   [100];
int      nforward;
int      bfwinfo;
int      icurlog;
int      imaxlog;
int      ilogport;
char     szlogkey    [200];
char     sztlisten   [SFK_PM_MAXBUF+100];
char     szulisten   [SFK_PM_MAXBUF+100];
char     sztskip     [SFK_PM_MAXBUF+100];
char     szuskip     [SFK_PM_MAXBUF+100];
char     alog[SFK_PM_MAXREC+10][SFK_PM_RECSIZE+10];
};

PortMon::PortMon( )
 : cludp(500),
   cltcp(str("portmon"),'h')
{
   cltcp.iClMaxWait = 500;
   mclear(claddr);
   claddr.sin_family      = AF_INET;
   claddr.sin_addr.s_addr = htonl(INADDR_ANY);
   mclear(clforward);
   nforward = 0;
   bfwinfo  = 0;
   icurlog  = 0;
   imaxlog  = SFK_PM_MAXREC;
   mclear(alog);
   mclear(szlogkey);
   mclear(sztlisten);
   mclear(szulisten);
   mclear(sztskip);
   mclear(szuskip);
   ilogport = 0;
}

PortMon::~PortMon( )
{
   cltcp.closeConnections();
   cludp.shutdown();
}

char *ownIPList(int &rhowmany, uint nOptPort, const char *psep, int nmode);
extern num nGlblStartTime;

char *safeHtml(const char *psz)
{
   static char szBuf[500+100];

   int i=0,k=0,bmark=0;

   if (   mystrstri((char*)psz, "admin")
       || mystrstri((char*)psz, "passwd")
      )
   {
      bmark=1;
      strcpy(szBuf,"<font color=\"#ff0000\">");
      k = strlen(szBuf);
   }

   for (i=0; k<500 && psz[i]!=0; i++)
   {
      switch (psz[i])
      {
         case '<': strcpy(szBuf+k, "&lt;");  k += 4; break;
         case '>': strcpy(szBuf+k, "&gt;");  k += 4; break;
         case '&': strcpy(szBuf+k, "&amp;"); k += 5; break;
         default:
            szBuf[k++] = psz[i];
      }
   }

   szBuf[k] = '\0';

   if (bmark)
      strcat(szBuf, "</font>");

   return szBuf;
}

void PortMon::sendlog(TCPCon *pcon, int bwide)
{
   int iuptimesec = (int)((getCurrentTime()-nGlblStartTime)/1000);

   int inumips=0;
   pcon->putf("HTTP/1.1 200 OK\r\n"
      "Connection: close\r\n"
      "Content-Type: text/html\r\n"
      "\r\n"
      "<html><body><pre><code>"
      "uptime %u sec - ownip: %s - listening on:\r\n"
      , iuptimesec, ownIPList(inumips,0," ",0)); // portmon

   int i=0;
   int nevents=0;
   for (i=0;i<imaxlog;i++)
      if (alog[i][0])
         nevents++;

   pcon->putf("tcp %s\r\n", sztlisten);
   pcon->putf("udp %s\r\n", szulisten);
   if (sztskip[0]) pcon->putf("skipped tcp %s\r\n",sztskip);
   if (szuskip[0]) pcon->putf("skipped udp %s\r\n",szuskip);
   pcon->putf("\r\n[events] %d\r\n",nevents);

   i=icurlog;
   while (1)
   {
      if (i<imaxlog) i++; else i=0;
      if (i==icurlog) break;
      if (alog[i][0])
      {
         pcon->putf("%.*s\r\n",bwide?600:130,safeHtml(alog[i]));
      }
   }
}

void PortMon::run( )
{
   uchar packet[512];
   uchar packet2[512+100]; mclear(packet2);
   int   packlen = 500;
   char  szIP[200];

   struct sockaddr_in from;
   int fromlen = sizeof(sockaddr_in);

   int   ihowmany = 1; // force first ip
   char *pszOwnIP = ownIPList(ihowmany,0,"",0); // portmon
   char *pShortIP = pszOwnIP+strlen(pszOwnIP);

   while (pShortIP>pszOwnIP && isdigit(pShortIP[-1])==0) pShortIP--;
   while (pShortIP>pszOwnIP && isdigit(pShortIP[-1])!=0) pShortIP--;

   if (nforward > 0)
      printf("[%s listens on %u tcp and %u udp ports. logport=%d. forward %s:%u]\n",
         pszOwnIP, cltcp.nClCon, cludp.nClCon, ilogport, clforward,nforward);
   else
      printf("[%s listens on %u tcp and %u udp ports. logport=%d]\n",
         pszOwnIP, cltcp.nClCon, cludp.nClCon, ilogport);

   while (!userInterrupt())
   {
      int icon=0,ilen=0,bany=0;
      TCPCon *pcon=0,*pcln=0;

      char *plog = alog[icurlog];

      if (!cludp.selectInput(&icon, 10))
      if ((ilen = recvfrom(cludp.aClCon[icon].sock, (char *)packet, packlen, 0,(struct sockaddr *)&from, (socklen_t*)&fromlen)) > 0)
      {
         if (from.sin_addr.s_addr == 0x0100007F) // 127.0.0.1
            continue;
         bany=1;
         char *ptime=timeAsString(time(0),0);
         ptime += 8;
         snprintf(plog,SFK_PM_RECSIZE,"%s U%05u %s %03u %s",
            ptime, cludp.aClCon[icon].port,
            ipAsString(&from,szIP,sizeof(szIP)-10,0), ilen,
            dataAsTrace(packet,ilen));
         printf("%s\n",plog);
         icurlog = (icurlog+1) % SFK_PM_MAXREC;
         if (nforward > 0) {
            sprintf((char*)packet2,":portmon %s on %s U%05u %s %03u: ",
               pShortIP, ptime,
               cludp.aClCon[icon].port,
               ipAsString(&from,szIP,sizeof(szIP)-10,0), ilen
               );
            int ihead=strlen((char*)packet2);
            if (bfwinfo) {
               snprintf((char*)packet2+ihead,500-ihead,"%s\n",dataAsTrace(packet,ilen));
               oForward.sendData(packet2,ihead+(int)strlen((char*)packet2+ihead));
            } else {
               memcpy(packet2+ihead,packet,ilen);
               oForward.sendData(packet2,ihead+ilen);
            }
         }
      }

      if (!cltcp.selectInput(&pcon,0,10))
      {
         bany=1;
         cltcp.accept(pcon,&pcln);
         if (pcln)
         {
            mclear(packet);
            ilen = pcln->read(packet, packlen, 1);
            if (   szlogkey[0]!=0
                && strncmp((char*)packet, szlogkey, strlen(szlogkey))==0
               )
            {
               if (strstr((char*)packet, "?clear")) {
                  memset(alog, 0, sizeof(alog));
                  icurlog=0;
               } else {
                  sendlog(pcln, strstr((char*)packet, "?wide") ? 1 : 0);
               }
            }
            else if (pcon->iClPort == ilogport)
            {
               if (cs.verbose) {
                  printf("miss.1: %s\n",szlogkey);
                  printf("miss.2: %s\n",(char*)packet);
               }
               // pcln->putf("HTTP/1.1 404 no such file\r\n"
               //    "Connection: close\r\n"
               //    "\r\n");
            }
            else
            {
               if (pcln->clFromAddr.sin_addr.s_addr == 0x0100007F) // 127.0.0.1
                  continue;
               char *ptime=timeAsString(time(0),0);
               ptime += 8;
               if (ilen > 0)
                  snprintf(plog,SFK_PM_RECSIZE,"%s T%05u %s %03u %s",
                     ptime, pcon->iClPort,
                     ipAsString(&pcln->clFromAddr,szIP,sizeof(szIP)-10,0), ilen,
                     dataAsTrace(packet,ilen));
               else
                  snprintf(plog,SFK_PM_RECSIZE,"%s T%05u %s 000 [no data]",
                     ptime, pcon->iClPort,
                     ipAsString(&pcln->clFromAddr,szIP,sizeof(szIP)-10,0));
               printf("%s\n",plog);
               icurlog = (icurlog+1) % SFK_PM_MAXREC;
               if (nforward > 0) {
                  sprintf((char*)packet2,":portmon %s on %s T%05u %s %03u: ",
                     pShortIP, ptime, pcon->iClPort,
                     ipAsString(&pcln->clFromAddr,szIP,sizeof(szIP)-10,0), ilen
                     );
                  int ihead=strlen((char*)packet2);
                  if (bfwinfo) {
                     snprintf((char*)packet2+ihead,500-ihead,"%s\n",dataAsTrace(packet,ilen));
                     oForward.sendData(packet2,ihead+(int)strlen((char*)packet2+ihead));
                  } else {
                     memcpy(packet2+ihead,packet,ilen);
                     oForward.sendData(packet2,ihead+ilen);
                  }
               }
            }
            cltcp.close(pcln);
         }
         else
         {
            printf("t %u: accept failed\n",pcon->iClPort);
         }
      }

      if (!bany)
         doSleep(10);
   }
}

#endif // WITH_PORTMON

// tocheck: sfkui

#ifdef _WIN32

#define ID_HELP   150
#define ID_TEXT   200

// .
class SFKUI
{
public:
      SFKUI ( );

int
      show  (int x,int y,char *pszlist),
      query ( ),
      close ( );

BOOL  dlgproc (HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

int
      bclopen;
HWND
      cldlg;
char
      acltext[20][20];
int
      aclid[20],
      aclclick[20],
      nclbut,
      iclshowx,iclshowy,
      iclshoww,iclshowh;
};

SFKUI::SFKUI()
{
   memset(this, 0, sizeof(*this));
}

SFKUI glblui;

LPWORD lpwAlign(LPWORD lpIn)
{
    uint64_t ul;

    ul = (uint64_t)lpIn;
    ul ++;
    ul >>=1;
    ul <<=1;
    return (LPWORD)ul;
}

int bGlblOK = 0;

BOOL SFKUI::dlgproc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   BOOL fError;
 
   glblui.cldlg = hwndDlg;

   int icmd = LOWORD(wParam);
 
   switch (message)
   {
      case WM_INITDIALOG:
      {
         // SetForegroundWindow(hwndDlg)
         //   works but also takes focus
         SetWindowPos(hwndDlg, HWND_TOPMOST,
            iclshowx,iclshowy,
            iclshoww,iclshowh,
            SWP_SHOWWINDOW|SWP_NOACTIVATE
            );
         return TRUE;
      }

      case WM_COMMAND:
      {
         // if (icmd == IDCANCEL) {
         //    printf("got cancel\n");
         //    DestroyWindow(hwndDlg);
         //    return TRUE;
         // }

         for (int i=0; i<nclbut; i++) {
            if (icmd == aclid[i]) {
               aclclick[i]=1;
               return TRUE;
            }
         }
      }
   }

   return FALSE;
}

BOOL CALLBACK sfkuiDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   return glblui.dlgproc(hwndDlg,message,wParam,lParam);
}

int SFKUI::close()
{
   if (cldlg) {
      DestroyWindow(cldlg);
      cldlg = 0;
   }
   bclopen = 0;
   return 0;
}

// .
int SFKUI::show(int x,int y,char *pszlist)
{
   iclshowx=x;
   iclshowy=y;

   // 1:raw 2:note 3:x
   nclbut=0;
   for (; *pszlist!=0 && *pszlist!='\"';)
   {
      aclid[nclbut] = atoi(pszlist);

      while (*pszlist!=0 && *pszlist!=':') pszlist++;
      if (*pszlist==':') pszlist++;

      char *pstart=pszlist;
      while (*pszlist!=0 && *pszlist!=' ') pszlist++;
      char *pend=pszlist;
      int ilen=pend-pstart;
      if (ilen>18) ilen=18;
      memcpy(acltext[nclbut],pstart,ilen);
      acltext[nclbut][ilen]='\0';

      if (*pszlist==' ') pszlist++;

      aclclick[nclbut]=0;
      nclbut++;
      if (nclbut >= 18) break;
   }

   HINSTANCE hinst = GetModuleHandle(0);
   HWND hwndOwner = GetDesktopWindow();

   RECT rWork;
   if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rWork, 0)) // getdeskrect
   {
      rWork.right=800;
      rWork.bottom=600;
   }

   LPSTR lpszMessage = str("yo");

   HGLOBAL hgbl;
   LPDLGTEMPLATE lpdt;
   LPDLGITEMTEMPLATE lpdit;
   LPWORD lpw;
   LPWSTR lpwsz;
   LRESULT ret;
   int nchar;
 
   hgbl = GlobalAlloc(GMEM_ZEROINIT, 1024);
   if (!hgbl)
     return -1;
 
   lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
 
   // Define a dialog box.
 
   // lpdt->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION;
   lpdt->style = WS_POPUP | DS_MODALFRAME;

   lpdt->cdit = nclbut;         // Number of controls

   // .
   int wnet = nclbut*40*2;
   int wbru = wnet+2;

   // printf("desk2 is %d %d\n", rWork.right, rWork.bottom);

   lpdt->x  = 10; // NOT PIXEL. (rt.right - wbru) - 200;
   lpdt->y  = 10;  // rt.bottom - 50;
   lpdt->cx = wbru;
   lpdt->cy = 30;
 
   iclshowx = rWork.right-wbru-4;
   iclshowy = rWork.bottom-40-4;
   iclshoww = wbru;
   iclshowh = 40;

   lpw = (LPWORD)(lpdt + 1);
   *lpw++ = 0;             // No menu
   *lpw++ = 0;             // Predefined dialog box class (by default)
 
   lpwsz = (LPWSTR)lpw;
   nchar = 1 + MultiByteToWideChar(CP_ACP, 0, "My Dialog", -1, lpwsz, 50);
   lpw += nchar;
 
   //-----------------------
   // Define an OK button.
   //-----------------------

   #if 1

   char sztext[100];

   for (int i=0; i<nclbut; i++)
   {
      lpw = lpwAlign(lpw);    // Align DLGITEMTEMPLATE on DWORD boundary
      lpdit = (LPDLGITEMTEMPLATE)lpw;

      lpdit->x  = 2+i*40;
      lpdit->y  = 2;
      lpdit->cx = 34;
      lpdit->cy = 12;
 
      lpdit->id = aclid[i]; // IDOK;       // OK button identifier
 
      lpdit->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
 
      lpw = (LPWORD)(lpdit + 1);
      *lpw++ = 0xFFFF;
      *lpw++ = 0x0080;        // Button class
 
      lpwsz = (LPWSTR)lpw;

      strcopy(sztext, acltext[i]);
      if ((strlen(sztext)&1))
         strcat(sztext," ");

      nchar = 1 + MultiByteToWideChar(CP_ACP, 0, sztext, -1, lpwsz, 50);

      lpw += nchar;
      *lpw++ = 0;             // No creation data
   }
 
   #endif

   #if 0

   //-----------------------
   // Define a Help button.
   //-----------------------
   lpw = lpwAlign(lpw);    // Align DLGITEMTEMPLATE on DWORD boundary
   lpdit = (LPDLGITEMTEMPLATE)lpw;
   lpdit->x  = 55; lpdit->y  = 10;
   lpdit->cx = 40; lpdit->cy = 20;
   lpdit->id = ID_HELP;    // Help button identifier
   lpdit->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
 
   lpw = (LPWORD)(lpdit + 1);
   *lpw++ = 0xFFFF;
   *lpw++ = 0x0080;        // Button class atom
 
   lpwsz = (LPWSTR)lpw;
   nchar = 1 + MultiByteToWideChar(CP_ACP, 0, "Help", -1, lpwsz, 50);
   lpw += nchar;
   *lpw++ = 0;             // No creation data
 
   //-----------------------
   // Define a static text control.
   //-----------------------
   lpw = lpwAlign(lpw);    // Align DLGITEMTEMPLATE on DWORD boundary
   lpdit = (LPDLGITEMTEMPLATE)lpw;
   lpdit->x  = 10; lpdit->y  = 10;
   lpdit->cx = 40; lpdit->cy = 20;
   lpdit->id = ID_TEXT;    // Text identifier
   lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
 
   lpw = (LPWORD)(lpdit + 1);
   *lpw++ = 0xFFFF;
   *lpw++ = 0x0082;        // Static class
 
   for (lpwsz = (LPWSTR)lpw; *lpwsz++ = (WCHAR)*lpszMessage++;);
   lpw = (LPWORD)lpwsz;
   *lpw++ = 0;             // No creation data

   #endif
 
   GlobalUnlock(hgbl);
 
   CreateDialogIndirectA(hinst,
                        (LPDLGTEMPLATE)hgbl,
                        hwndOwner,
                        (DLGPROC)sfkuiDialogProc);
 
   bclopen = 1;
 
   query();
 
   return 0;
}

int SFKUI::query()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
      if (GetMessage(&msg, NULL, 0, 0))
      {
       TranslateMessage(&msg);
       DispatchMessage(&msg);
      }
    }
    return bGlblOK;
}

#endif

/*
   zlib/foo/bar/header1.h
*/
char *pGlblCScanFiles = 0;

int execCScan(char *pszFile, int iLevel)
{
   if (iLevel>20) return 0;

   if (iLevel==0)
      chain.print('f', 1, "%.*s%s",iLevel*2,pszGlblBlank,pszFile);
   else
      chain.print(' ', 1, "%.*s%s",iLevel*2,pszGlblBlank,pszFile);

   char *ptext = loadFile(pszFile);

   char szCurHdr1[SFK_MAX_PATH+10];
   char szCurHdr2[SFK_MAX_PATH+10];

   char *pcur=ptext;
   while (*pcur)
   {
      // isolate line
      char *pnext=pcur;
      while (*pnext!=0 && *pnext!='\r' && *pnext!='\n') pnext++;
      if (!*pnext) break;
      int ilen=pnext-pcur;
      if (ilen+1>MAX_LINE_LEN) break;
      memcpy(szLineBuf,pcur,ilen);
      szLineBuf[ilen]='\0';

      /*
         anytext
         #include "../header1.h"
         #  include <header1.h>
      */
      do
      {
         // get ../header1.h
         if (!strchr(szLineBuf, '#')) break;
         if (!strstr(szLineBuf, "include")) break;
         pcur=szLineBuf;
         while (*pcur!=0 && *pcur!='<' && *pcur!='\"') pcur++;
         if (!*pcur) break;
         char csep1=*pcur++;
         char csep2=csep1;
         if (csep1=='<') csep2='>';
         // ../foo.h -> foo.h
         while (!strncmp(pcur,"../",3)) pcur+=3;
         while (!strncmp(pcur,"./",2)) pcur+=2;
         char *pleft=pcur;
         while (*pcur!=0 && *pcur!=csep2) pcur++;
         if (!*pcur) break;
         ilen=pcur-pleft;
         if (ilen+1>SFK_MAX_PATH) break;
         memcpy(szCurHdr1,pleft,ilen);
         szCurHdr1[ilen]='\0';

         // FROM: header1.h
         // TO  : zlib/foo/bar/header1.h
         char *phit = mystrstri(pGlblCScanFiles,szCurHdr1);
         if (!phit) {
            // #include <stdio.h>
            chain.print('a', 1, "%.*s%c%s%c",
               (iLevel+1)*2,pszGlblBlank,csep1,szCurHdr1,csep2);
            break;
         }
         while (phit>pGlblCScanFiles && phit[-1]!='\n') phit--;

         char *peol=phit;
         while (*peol!=0 && *peol!='\r' && *peol!='\n') peol++;
         if (!*peol) break;

         ilen=peol-phit;
         if (ilen+1>SFK_MAX_PATH) break;
         memcpy(szCurHdr2,phit,ilen);
         szCurHdr2[ilen]='\0';

         execCScan(szCurHdr2, iLevel+1);
      }
      while (0);

      while (*pnext!=0 && (*pnext=='\r' || *pnext=='\n')) pnext++;
      pcur=pnext;
   }

   delete [] ptext;

   return 0;
}
