
#include "sfkpicio.hpp"

#include <stdarg.h>

// to allow sfk core control of mem allocated by sfkpic
uint *(*sfkPicAllocCallback)(int nWords) = 0;

// mode 1: 4 -> 3
// mode 2: 3 -> 4 bluebox
uint mixcolor(uint cin, uint cback, uint nmode)
{
   uint c1=(cin>>0)&0xFFU;
   uint c2=(cin>>8)&0xFFU;
   uint c3=(cin>>16)&0xFFU;

   uint aa =(cin>>24)&0xFFU;
   uint ar =0xFFU-aa;
   uint ax =0xFFU;

   uint b1=(cback>>0)&0xFFU;
   uint b2=(cback>>8)&0xFFU;
   uint b3=(cback>>16)&0xFFU;

   if (nmode==1)
   {
      // 4 to 3 mixdown
      c1 =     ((c1 * aa) / 0xFFU)
            +  ((b1 * ar) / 0xFFU);
      c2 =     ((c2 * aa) / 0xFFU)
            +  ((b2 * ar) / 0xFFU);
      c3 =     ((c3 * aa) / 0xFFU)
            +  ((b3 * ar) / 0xFFU);
   }
   else if (nmode==2)
   {
      // 3 to 4 bluebox
      if ((cin&0xFFFFFFU) == (cback&0xFFFFFFU))
         ax = 0;
   }

   return      (c1<<0)
            |  (c2<<8)
            |  (c3<<16)
            |  (ax<<24);
}

struct MyPngReadControl
{
   uchar *pinbuf;
   int    ninused;
   int    ninmax;
};

static void MyPngReadFn(png_structp png_ptr, png_bytep data, png_size_t length)
{
   MyPngReadControl *io_ptr = (MyPngReadControl *)png_ptr->io_ptr;

   if ((int)io_ptr->ninused + (int)length > (int)io_ptr->ninmax) {
      png_error(png_ptr, "Read Error: Buffer Underflow");
   }

   memcpy(data, io_ptr->pinbuf + io_ptr->ninused, length);

   io_ptr->ninused += length;
}

uint mergeColorWithBack(uint c, uint bg)
{
   uchar alpha1 = ((c & 0xff000000) >> 24);

   // if pixel is opaque, return as is
   if (alpha1 == 0xFF) return c;

   // invert alpha for background rendering
   uchar alpha2 = 0xFF - alpha1;

   bg =      (((bg        & 0xff) * alpha2) >> 8)
         |  ((((bg >>  8) & 0xff) * alpha2) & 0xff00)
         | (((((bg >> 16) & 0xff) * alpha2) & 0xff00) << 8);

   // render foreground with original alpha

   c =     (((c        & 0xff) * alpha1) >> 8)
       |  ((((c >>  8) & 0xff) * alpha1) & 0xff00)
       | (((((c >> 16) & 0xff) * alpha1) & 0xff00) << 8);

   // output pixel is fully opaque
   c |= 0xFF000000UL;

   return c + bg;
}

uint *loadpng(uchar *pPacked, uint iPacked, struct SFKPicData *p)
{
   p->rc=0;
   p->szerror[0]='\0';

   if (p->ppix)
      { p->rc=10; return 0; }

   png_structp png_ptr;
   png_infop info_ptr;
   unsigned int sig_read = 0;

   if (!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0,0,0)))
      return 0;

   // sfk1973 do not use, causes exit
   if ((p->loadflags & 1) == 1)  // expand
      png_ptr->transformations |= PNG_EXPAND;

   if (!(info_ptr = png_create_info_struct(png_ptr)))
   {
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      return 0;
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      return 0;
   }

   MyPngReadControl oin;

   oin.pinbuf  = pPacked;
   oin.ninused = 0;
   oin.ninmax  = (int)iPacked;

   png_set_read_fn(png_ptr, &oin, MyPngReadFn);

   png_set_sig_bytes(png_ptr, sig_read);

   png_read_png(png_ptr, info_ptr, 0, png_voidp_NULL);

   p->width = info_ptr->width;
   p->height = info_ptr->height;
   p->filecomp = info_ptr->channels;

   uchar **ppSrc  = info_ptr->row_pointers;
   int nSrcWidth    = info_ptr->width;
   int nSrcHeight   = info_ptr->height;
   int nSrcChannels = info_ptr->channels;

   int nOutChannels = 3;
   int nOutWidth    = nSrcWidth;
   int nOutHeight   = nSrcHeight;

   int nOutPix      = nOutWidth * nOutHeight;

   if (nOutPix > 50 * 1000000)
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      return 0;
   }

   uint *pOutPix = 0;

   if (sfkPicAllocCallback)
      pOutPix = sfkPicAllocCallback(nOutPix + 100);
   else
      pOutPix = new uint[nOutPix + 100];

   if (!pOutPix)
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      return 0;
   }

   for (int y=0; y<nSrcHeight; y++)
   {
      uchar *pSrcBytes = ppSrc[y];
      uint  *pDstPix   = pOutPix + nOutWidth * y;

      for (int x=0; x<nSrcWidth; x++)
      {
         uint c =    (((uint)pSrcBytes[0]) <<  0)
                  |  (((uint)pSrcBytes[1]) <<  8)
                  |  (((uint)pSrcBytes[2]) << 16);

         if (nSrcChannels == 4)
            c |= (((uint)pSrcBytes[3]) << 24);
         else
            c |= (((uint)0xFF) << 24);

         if (p->mixonload)
            c = mixcolor(c, p->backcolor, p->mixonload ? 2 : 0);

         pSrcBytes += nSrcChannels;
         *pDstPix++ = c;
      }
   }

   png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

   p->ppix = pOutPix;
   p->npix = nOutPix;

   return (uint*)pOutPix;
}

struct i3_djpeg_dest_struct
{
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
};

// ----------------------- jpeg decomp ---------------------

struct tga_dest_struct
{
   struct i3_djpeg_dest_struct pub; /* public fields */
   char *iobuffer;               /* physical I/O buffer */
   JDIMENSION buffer_width;      /* width of one row */
};

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  (*cinfo->err->output_message) (cinfo);
  longjmp(myerr->setjmp_buffer, 1);
}

METHODDEF(void) mem_init_source (j_decompress_ptr cinfo) { }
METHODDEF(boolean) mem_fill_input_buffer (j_decompress_ptr cinfo)
   { return FALSE; }
METHODDEF(void) mem_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr *src = cinfo->src;
  if (src->bytes_in_buffer < num_bytes)
     return;
  src->next_input_byte += (size_t) num_bytes;
  src->bytes_in_buffer -= (size_t) num_bytes;
}
METHODDEF(void) mem_term_source (j_decompress_ptr cinfo) { }

uint *loadjpg(uchar *pPacked, uint iPacked, struct SFKPicData *p)
{
   struct jpeg_decompress_struct cinfo;
   struct my_error_mgr jerr;
   struct tga_dest_struct *dest;
   struct jpeg_source_mgr mymgr;
   int num_scanlines;
   int iLines = 0;
   uchar *pbuf;

   p->rc=0;
   p->szerror[0]='\0';

   cinfo.err = jpeg_std_error(&jerr.pub);
   jerr.pub.error_exit = my_error_exit;

   if (setjmp(jerr.setjmp_buffer))
   {
      jpeg_destroy_decompress(&cinfo);
      p->rc = 1;
      strcpy(p->szerror, "decoding error");
      return 0;
   }

   jpeg_create_decompress(&cinfo);

   cinfo.src = &mymgr;

   mymgr.next_input_byte = (JOCTET*)pPacked;
   mymgr.bytes_in_buffer = iPacked;

   mymgr.init_source       = mem_init_source;
   mymgr.fill_input_buffer = mem_fill_input_buffer;
   mymgr.skip_input_data   = mem_skip_input_data;
   mymgr.resync_to_restart = jpeg_resync_to_restart;
   mymgr.term_source       = mem_term_source;

   jpeg_read_header(&cinfo, TRUE);

   p->width    = (int)cinfo.image_width;
   p->height   = (int)cinfo.image_height;
   p->filecomp = (int)cinfo.num_components;

   if (cinfo.image_width > 10000 || cinfo.image_height > 10000) {
      p->rc = 2;
      sprintf(p->szerror, "too large (%dx%d)", cinfo.image_width, cinfo.image_height);
      return 0;
   }

   if (   cinfo.num_components != 1
       && cinfo.num_components != 3
       && cinfo.num_components != 4
      )
   {
      p->rc = 3;
      sprintf(p->szerror, "invalid color components (%d)", cinfo.num_components);
      return 0;
   }

   dest = (struct tga_dest_struct *)
       (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_IMAGE,
               SIZEOF(struct tga_dest_struct));

   // dest->pub.start_output  = 0;
   // dest->pub.finish_output = 0;
   jpeg_calc_output_dimensions(&cinfo);

   p->width    = (int)cinfo.output_width;
   p->height   = (int)cinfo.output_height;
   p->filecomp = (int)cinfo.output_components;

   dest->buffer_width = cinfo.output_width * cinfo.output_components;
   dest->iobuffer = (char *)
     (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_IMAGE,
             (size_t) (dest->buffer_width * SIZEOF(char)));

   dest->pub.buffer = (*cinfo.mem->alloc_sarray)
     ((j_common_ptr) &cinfo, JPOOL_IMAGE, dest->buffer_width, (JDIMENSION) 1);

   dest->pub.buffer_height = 1;

   int nOutPix = p->width * p->height;

   p->npix = p->width * p->height;

   if (sfkPicAllocCallback)
      p->ppix = sfkPicAllocCallback(nOutPix + 100);
   else
      p->ppix = new uint[nOutPix + 100];

   if (!p->ppix)
   {
      p->rc = 4;
      sprintf(p->szerror, "out of memory (%u)", p->npix);
      return 0;
   }

   jpeg_start_decompress(&cinfo);

   iLines = 0;
   while (cinfo.output_scanline < cinfo.output_height)
   {
      num_scanlines = jpeg_read_scanlines(
         &cinfo,
         dest->pub.buffer,
         dest->pub.buffer_height
         );

      if (num_scanlines < 1) {
         p->rc = 5;
         sprintf(p->szerror, "incomplete decoding (%d/%d)", cinfo.output_scanline, cinfo.output_height);
         delete [] p->ppix;
         p->ppix=0;
         return 0;
      }

      pbuf = dest->pub.buffer[0];

      if (pbuf == 0) {
         p->rc = 6;
         sprintf(p->szerror, "incomplete decoding.2 (%d/%d)", cinfo.output_scanline, cinfo.output_height);
         delete [] p->ppix;
         p->ppix=0;
         return 0;
      }

      // FROM bytes ARGB
      // TO   uint  ARGB
      // uchar *pdst = (uchar*)p->ppix;
      // memcpy(pdst + dest->buffer_width * iLines, dest->pub.buffer[0], dest->buffer_width);

      uchar *pSrcBytes = dest->pub.buffer[0];
      uint  *pDstPix   = p->ppix + p->width * iLines;

      uint   c=0;

      for (int x=0; x<p->width; x++)
      {
         switch (cinfo.num_components)
         {
            case 1:
               c =    (((uint)pSrcBytes[x]) <<  0)
                   |  (((uint)pSrcBytes[x]) <<  8)
                   |  (((uint)pSrcBytes[x]) << 16);
               break;
            case 3:
               c =    (((uint)pSrcBytes[x*3+0]) <<  0)
                   |  (((uint)pSrcBytes[x*3+1]) <<  8)
                   |  (((uint)pSrcBytes[x*3+2]) << 16);
               break;
         }

         c |= (((uint)0xFF) << 24);

         if (p->mixonload)
            c = mixcolor(c, p->backcolor, p->mixonload ? 2 : 0);

         *pDstPix++ = c;
      }

      iLines++;
   }

   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);

   return p->ppix;
}

// --------------------- jpeg comp --------------------

uint packjpg(uint *ppix, struct SFKPicData *p, uchar **ppout, uint *noutmax)
{
   struct jpeg_compress_struct cinfo;
   JSAMPROW row_pointer[1];
   row_pointer[0] = 0;
   struct my_error_mgr jerr;

   uchar *praw = (uchar*)ppix;

   cinfo.err = jpeg_std_error(&jerr.pub);

   jerr.pub.error_exit = my_error_exit;

   if (setjmp(jerr.setjmp_buffer))
   {
      jpeg_destroy_compress(&cinfo);
      return 9;
   }

   jpeg_create_compress(&cinfo);

   int nsrccmp = p->filecomp;
   int ndstcmp = 3;

   cinfo.image_width       = p->width;
   cinfo.image_height      = p->height;
   cinfo.input_components  = ndstcmp;
   cinfo.in_color_space    = JCS_RGB;
   jpeg_set_defaults(&cinfo);

   unsigned long ulOutMax   = *noutmax;
   unsigned long *pulOutMax = &ulOutMax;

   jpeg_mem_dest(&cinfo, ppout, pulOutMax);

   jpeg_set_quality(&cinfo, p->filequality, true);
   jpeg_start_compress(&cinfo, true);

   row_pointer[0] = new uchar[cinfo.image_width * cinfo.input_components];

   int w = cinfo.image_width;
   int ncmp = cinfo.input_components;

   while (cinfo.next_scanline < cinfo.image_height)
   {
      uint *psrcrow = ppix + cinfo.next_scanline*p->width;
      uchar *prowbuf = row_pointer[0];

      for (int x=0; x<p->width; x++)
      {
         uint c = psrcrow[x];

         if (p->mixonpack)
            c = mixcolor(c, p->backcolor, p->mixonpack);

         prowbuf[x*ncmp+0] = (uchar)(c >>  0);

         if (ncmp>=2)
         prowbuf[x*ncmp+1] = (uchar)(c >>  8);

         if (ncmp>=3)
         prowbuf[x*ncmp+2] = (uchar)(c >> 16);

         if (ncmp>=4)
         prowbuf[x*ncmp+3] = (uchar)(c >> 24);
      }

      jpeg_write_scanlines(&cinfo, row_pointer, 1);
   }

   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);

   delete [] row_pointer[0];
 
   if (noutmax)
      *noutmax = ulOutMax;

   return 0;
}

// ------------------- png encode --------------------

struct MyPngWriteControl
{
   uchar *poutbuf;
   int    noutused;
   int    noutmax;
};

static void MyPngWriteFn(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_uint_32 check;

   MyPngWriteControl *io_ptr = (MyPngWriteControl *)png_ptr->io_ptr;

   int nremain = io_ptr->noutmax - io_ptr->noutused;

   if ((int)length >= (int)(nremain - 10))
   {
      png_error(png_ptr, "Write Error: Compressed Output Overflow");
      return;
   }

   memcpy(io_ptr->poutbuf + io_ptr->noutused, data, length);
   io_ptr->noutused += length;
}

static void MyPngFlushFn(png_structp png_ptr)
{
}

static void MyPngErrorFn(png_structp p1, png_const_charp p2)
{
   printf("PNGError: %s\n", p2);
}

static void MyPngWarnFn(png_structp p1, png_const_charp p2)
{
   printf("PNGWarn: %s\n", p2);
}

uint packpng(uint *ppix, struct SFKPicData *p, uchar **ppout, uint *noutmax)
{
   uchar *praw = (uchar*)ppix;

   uchar *poutbuf = *ppout;

   png_structp png_ptr  = 0;
   png_infop   info_ptr = 0;
   png_colorp  palette  = 0;

   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, MyPngErrorFn, MyPngWarnFn);
   if (png_ptr == NULL) return 9;

   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
      return 10;
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return 11;
   }

   int nsrccmp = p->filecomp;
   int ndstcmp = p->outcomp ? p->outcomp : nsrccmp;

   MyPngWriteControl oout;

   oout.poutbuf  = poutbuf;
   oout.noutused = 0;
   oout.noutmax  = (int)(*noutmax);

   png_set_write_fn(png_ptr, &oout, MyPngWriteFn, MyPngFlushFn);

   png_set_IHDR(png_ptr, info_ptr, p->width, p->height, 8,
      ndstcmp == 4 ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
 
   png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);

   palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH
             * png_sizeof(png_color));
   png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);

   png_color_8 sig_bit;
   memset(&sig_bit, 0, sizeof(sig_bit));

   sig_bit.red    = 8;
   sig_bit.green  = 8;
   sig_bit.blue   = 8;
   if (ndstcmp == 4)
      sig_bit.alpha = 8;
   png_set_sBIT(png_ptr, info_ptr, &sig_bit);

   png_write_info(png_ptr, info_ptr);

   png_bytep row_pointers[1];

   uint nrowbytes = p->width * 4;
   uchar *prowbuf = new uchar[nrowbytes+100];

   for (int y = 0; y < p->height; y++)
   {
      uint *psrcrow = ppix + y*p->width;

      for (int x=0; x<p->width; x++)
      {
         uint c = psrcrow[x];

         if (p->mixonpack)
            c = mixcolor(c, p->backcolor, p->mixonpack);

         prowbuf[x*ndstcmp+0] = (uchar)(c >>  0);

         if (ndstcmp>=2)
         prowbuf[x*ndstcmp+1] = (uchar)(c >>  8);

         if (ndstcmp>=3)
         prowbuf[x*ndstcmp+2] = (uchar)(c >> 16);

         if (ndstcmp>=4)
         prowbuf[x*ndstcmp+3] = (uchar)(c >> 24);
      }

      row_pointers[0] = prowbuf;
      png_write_rows(png_ptr, &row_pointers[0], 1);
   }

   delete [] prowbuf;

   png_write_end(png_ptr, info_ptr);

   png_free(png_ptr, palette);
   palette = NULL;

   png_destroy_write_struct(&png_ptr, &info_ptr);

   *noutmax = oout.noutused;

   return 0;
}

uint packpic(uint *ppix, struct SFKPicData *p, uchar **ppout, uint *noutmax)
{
   uchar *pout = *ppout;
   if (!pout)
      return 9;
   if (*noutmax < 100)
      return 10;

   if (p->fileformat==2)
      return packjpg(ppix, p, ppout, noutmax);

   if (p->fileformat==1)
      return packpng(ppix, p, ppout, noutmax);

   if (p->fileformat==3) {
      if (p->model != 0)
         return 11;
      uchar *pdst = pout;
      uchar *pmax = pout + *noutmax;
      snprintf((char*)pdst, 64,
         ":flatpic width=%05u,height=%05u,format=argb,order=le,pad=...\n\n",
         p->width, p->height);
      pdst += strlen((char*)pdst);
      *pdst++ = '\0';
      int npix = p->npix;
      int nbytes = npix*4;
      if (pdst+nbytes > pmax)
         return 12;
      memcpy(pdst, ppix, nbytes);
      pdst += nbytes;
      *noutmax = (pdst-pout);
      return 0;
   }

   return 9;
}

uint *loadpic(uchar *pPacked, uint iPacked, struct SFKPicData *p)
{
   p->filequality=90;
   p->filecomp=3;

   if (pPacked[1]=='P' && pPacked[2]=='N' && pPacked[3]=='G') {
      p->fileformat=1;
      strcpy(p->szformat,"png");
      return loadpng(pPacked,iPacked,p);
   }

   if (pPacked[0]==0xFF && pPacked[1]==0xD8) {
      p->fileformat=2;
      strcpy(p->szformat,"jpeg");
      return loadjpg(pPacked,iPacked,p);
   }

   if (!strncmp((char*)pPacked, ":flatpic ", 9))
   {
      memset(p, 0, sizeof(struct SFKPicData));

      p->fileformat=3;
      strcpy(p->szformat,"flatpic");
      p->filecomp=3;

      int iwidth=0, iheight=0;
      char format[20] = {0};
      char order[20] = {0};
      char szline[200], *pline;

      uchar *psrccur=pPacked;
      uchar *psrcmax=pPacked+iPacked;

      sscanf((char*)psrccur, ":flatpic width=%d,height=%d,format=%[^,],order=%[^,]", &iwidth, &iheight, format, order);

      while (psrccur<psrcmax && *psrccur != '\0')
         psrccur++;

      if (psrccur>=psrcmax)
         { p->rc=10; return 0; }

      // accept only these
      if (strcmp(format,"argb"))
         { p->rc=11; return 0; }
      if (strcmp(order,"le")) {
         snprintf(p->szerror,sizeof(p->szerror)-10,"unsupported byte order '%s'",order);
         p->rc=12;
         return 0;
      }

      // reject implausible sizes
      if (iwidth<0 || iwidth>=100000 || iheight<0 || iheight>100000)
         { p->rc=13; return 0; }

      int npix = iwidth*iheight;
      int nbytes = npix*4;
      if (nbytes > 300 * 1048576)
         { p->rc=14; return 0; }

      uint *ppix = new uint[npix+100];
      if (!ppix)
         { p->rc=15; return 0; }

      p->width  = iwidth;
      p->height = iheight;
      p->model  = 0;
      p->ppix   = ppix;
      p->npix   = npix;
      p->fileformat = 3;
      p->filecomp   = 4;

      return ppix;
   }

   return 0;
}

static int liGetFileSize(char *pszName)
{
   FILE *fin = fopen(pszName, "rb");
   if (!fin) return -1;

   if (fseek(fin, 0, SEEK_END))
      { fclose(fin); return -1; }

   int npos = (int)ftell(fin);

   fclose(fin);

   return npos;
}

static char *liLoadFile(char *pszFile)
{
   int nFileSize = liGetFileSize(pszFile);
   if (nFileSize < 0)
      return 0;

   int nTolerance = 10;
   char *pOut = new char[nFileSize+nTolerance+4];
   if (!pOut)
      return 0;
   memset(pOut+nFileSize, 0, nTolerance); // added safety

   FILE *fin = fopen(pszFile, "rb");
   if (!fin) {
      delete [] pOut;
      return 0;
   }

   int nRead = fread(pOut, 1, nFileSize, fin);
   fclose(fin);

   if (nRead != nFileSize) {
      delete [] pOut;
      return 0;
   }

   pOut[nFileSize] = '\0';

   return pOut;
}

#ifdef TESTSFKPIC

/*
   // flatpic write/read sample code:

   uint apix[16*16];

   {
      for (int i=0;i<16*16;i++)
         apix[i]=0xff112233;
 
      FILE *f=fopen("in.flatpic","wb");
 
      fprintf(f,":flatpic width=00016,height=00016,format=argb,order=le\n\n");
      fputc(0,f);
 
      fwrite(apix,1,sizeof(apix),f);
 
      fclose(f);
   }
 
   {
      int iwidth = 0, iheight = 0;
      char format[20] = {0};
      char order[20] = {0};

      char szline[200], *pline;
      FILE *f=fopen("in.flatpic","rb");

      while (pline=fgets(szline,sizeof(szline)-10,f))
      {
         if (!strncmp(szline, ":flatpic ", 9)) {
            sscanf(szline, ":flatpic width=%d,height=%d,format=%[^,],order=%[^,]", &iwidth, &iheight, format, order);
            printf("%dx%d '%s' '%s'\n",iwidth, iheight, format, order);
            continue;
         }
         if (!strncmp(szline, ":text ", 6)) {
            continue;
         }
         if (szline[0]=='\r' || szline[0]=='\n')
            break;
      }

      do
      {
         // null byte must follow
         int c = fgetc(f);
         if (c != 0)
            { printf("wrong header end (%d)\n",c); break; }

         // accept only these
         if (strcmp(format,"argb"))
            { printf("cannot read format '%s'\n",format); break; }
         if (strcmp(order,"le"))
            { printf("cannot read order '%s'\n",order); break; }

         // pixel data follows
         int npix=iwidth*iheight;
         if (npix*4<=sizeof(apix)) {
            fread(apix,1,npix*4,f);
            printf("read %d pix %x\n",npix,apix[0]);
         }
      }
      while (0);

      fclose(f);
   }
*/

static int lierr(const char *pszFormat, ...) {
   va_list argList;
   va_start(argList, pszFormat);
   char szBuf[1024];
   ::vsprintf(szBuf, pszFormat, argList);
   fprintf(stderr, "error: %s", szBuf);
   return 0;
}

int main(int argc, char *argv[])
{
   if (argc<4) 
   {
      printf("usage: sfkpic conv in.ext out.ext "
             // "[detrans|entrans backcolor]"
             "\n"
             "supported extensions: .png .jpg .jpeg\n"
             /*
             "detrans ffffff: change transparent pixels to white color.\n"
             "                works only with 4-channel input png.\n"
             "entrans 0000ff: change blue pixels to transparent color.\n"
             "                requires .png extension as output.\n"
             */
            );
      return 0;
   }

   if (strcmp(argv[1], "conv")) {
      printf("wrong command: %s\n", argv[1]);
      return 9;
   }

   char *pszInFile  = argv[2];
   char *pszOutFile = argv[3];

   uint  nmixmode   = 0;
   uint  nbackcol   = 0;

   if (argc>=6)
   {
      // 1: mix on save
      // 2: mix on load
      if (!strcmp(argv[4],"detrans"))
         nmixmode=1;
      else if (!strcmp(argv[4],"entrans"))
         nmixmode=2;
      else
         return 9+lierr("wrong mix mode\n");
      char *psz=argv[5];
      if (strlen(psz)!=6)
         return 9+lierr("backcolor must be 6 hex characters\n");
      char szr[10],szg[10],szb[10];
      memcpy(szr,psz+0,2); szr[2]='\0';
      memcpy(szg,psz+2,2); szg[2]='\0';
      memcpy(szb,psz+4,2); szb[2]='\0';
      uint r=strtoul(szr,0,0x10);
      uint g=strtoul(szg,0,0x10);
      uint b=strtoul(szb,0,0x10);
      nbackcol=(r<<0)|(g<<8)|(b<<16);
      printf("backcolor: %06x\n",nbackcol);
   }

   int nInSize = liGetFileSize(pszInFile);
   if (nInSize < 1)
      return 9+lierr("no or empty input file\n");

   uchar *pInPack = (uchar*)liLoadFile(pszInFile);
   if (!pInPack)
      return 10+lierr("outofmem\n");

   struct SFKPicData octl;
   memset(&octl,0,sizeof(octl));

   octl.backcolor=nbackcol;

   if (nmixmode==1) 
   {
      octl.mixonpack=1;
      octl.outcomp=3;
   }
   if (nmixmode==2) 
   {
      octl.mixonpack=2;
      octl.outcomp=4;
   }

   uint *ppix = loadpic(pInPack,nInSize,&octl);
   if (!ppix)
      return 9+lierr("cannot decode (%d,%s)\n", octl.rc, octl.szerror);

   printf("> load %s %u x %u x %u\n",
      octl.szformat,
      octl.width,octl.height,octl.filecomp);

   uint   nOutSize = octl.width*octl.height*4+100;
   uchar *pOutPack = new uchar[nOutSize+100];
   if (!pOutPack)
      return 10+lierr("outofmem\n");

   if (strstr(pszOutFile, ".png"))
   {
      octl.fileformat=1;
      strcpy(octl.szformat, "png");
   }
   else if (strstr(pszOutFile, ".jpg") || strstr(pszOutFile, ".jpeg"))
   {
      octl.fileformat=2;
      strcpy(octl.szformat, "jpeg");
      octl.outcomp=3;
   }
   else if (strstr(pszOutFile, ".flatpic")) // internal
   {
      octl.fileformat=3;
      strcpy(octl.szformat, "flatpic");
      octl.outcomp=4;
   }

   uint rc = packpic(ppix, &octl, &pOutPack, &nOutSize);

   if (rc == 0)
   {
      printf("< save %s %u x %u x %u\n",
         octl.szformat,
         octl.width,octl.height,octl.outcomp?octl.outcomp:octl.filecomp);
 
      FILE *fout=fopen(pszOutFile, "wb");
      if (!fout) return 9;
      fwrite(pOutPack,1,nOutSize,fout);
      fclose(fout);
      printf("wrote %s\n",pszOutFile);
   }
   else
   {
      printf("- cannot encode %s rc %u\n",
         (octl.fileformat==1)?"png":"jpeg", rc);
   }

   delete [] octl.ppix;

   return 0;
}

#endif // TEST

