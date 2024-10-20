#ifndef SFKPICIO_HPP
#define SFKPICIO_HPP

// SFKPic V1.0 library interface

#ifndef uint
 typedef unsigned int  uint;
#endif
#ifndef uchar
 typedef unsigned char uchar;
#endif

struct SFKPicData
{
   uint  width;         // 800
   uint  height;        // 600
   uint  model;         // 0:ARGB

   uint  *ppix;         // owned by caller
   uint   npix;         // width*height

   uint  fileformat;    // 0:UNKNOWN 1:PNG 2:JPEG
   uint  filecomp;      // 1, 3 or 4
   uint  outcomp;       // 0 if same as filecomp
   uint  filequality;   // 90

   uint  loadflags;     // bit0:NoPaletteExpand
   uint  mixonload;
   uint  mixonpack;
   uint  backcolor;

   uint  rc;
   char  szerror[200];
   char  szformat[30];
};

extern uint *(*sfkPicAllocCallback)(int nWords);

uint *loadpic(uchar *pPacked, uint iPacked, struct SFKPicData *p);

uint  packpic(uint *pRGB, struct SFKPicData *p,
              uchar **ppout, uint *noutmax);

#endif
