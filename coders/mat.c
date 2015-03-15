/*
% Copyright (C) 2003 - 2015 GraphicsMagick Group
% Copyright (C) 2002 ImageMagick Studio
%
% This program is covered by multiple licenses, which are described in
% Copyright.txt. You should have received a copy of Copyright.txt with this
% package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                  M   M   AAA   TTTTT  L       AAA   BBBB                    %
%                  MM MM  A   A    T    L      A   A  B   B                   %
%                  M M M  AAAAA    T    L      AAAAA  BBBB                    %
%                  M   M  A   A    T    L      A   A  B   B                   %
%                  M   M  A   A    T    LLLLL  A   A  BBBB                    %
%                                                                             %
%                                                                             %
%                        Read MATLAB Image Format.                            %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                              Jaroslav Fojtik                                %
%                                2001 - 2008                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%Currently supported formats:
%  2D matrices:      X*Y    byte, word, dword, qword, single, double
%  3D matrices: only X*Y*3  byte, word, dword, qword, single, double
%  complex:          X*Y                              single, double
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/analyze.h"
#include "magick/blob.h"
#include "magick/color.h"
#include "magick/colormap.h"
#include "magick/constitute.h"
#include "magick/magick.h"
#include "magick/magick_endian.h"
#include "magick/monitor.h"
#include "magick/pixel_cache.h"
#include "magick/shear.h"
#include "magick/tempfile.h"
#include "magick/transform.h"
#include "magick/utility.h"
#if defined(HasZLIB)
#  include "zlib.h"
#endif


/* Auto coloring method, sorry this creates some artefact inside data
MinReal+j*MaxComplex = red  MaxReal+j*MaxComplex = black
MinReal+j*0 = white          MaxReal+j*0 = black
MinReal+j*MinComplex = blue  MaxReal+j*MinComplex = black
*/

typedef struct
{
  char identific[124];
  unsigned short Version;
  char EndianIndicator[2];
  unsigned long DataType;
  unsigned long ObjectSize;
  unsigned long unknown1;
  unsigned long unknown2;

  unsigned short unknown5;
  unsigned char StructureFlag;
  unsigned char StructureClass;
  unsigned long unknown3;
  unsigned long unknown4;
  unsigned long DimFlag;

  unsigned long SizeX;
  unsigned long SizeY;
  unsigned short Flag1;
  unsigned short NameFlag;
}
MATHeader;

static const char *MonthsTab[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
static const char *DayOfWTab[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char *OsDesc=
#ifdef __WIN32__
    "PCWIN";
#else
 #ifdef __APPLE__
    "MAC";
 #else
    "LNX86";
 #endif
#endif

typedef enum
  {
    miINT8 = 1,			/* 8 bit signed */
    miUINT8,			/* 8 bit unsigned */
    miINT16,			/* 16 bit signed */
    miUINT16,			/* 16 bit unsigned */
    miINT32,			/* 32 bit signed */
    miUINT32,			/* 32 bit unsigned */
    miSINGLE,			/* IEEE 754 single precision float */
    miRESERVE1,
    miDOUBLE,			/* IEEE 754 double precision float */
    miRESERVE2,
    miRESERVE3,
    miINT64,			/* 64 bit signed */
    miUINT64,			/* 64 bit unsigned */
    miMATRIX,		        /* MATLAB array */
    miCOMPRESSED,	        /* Compressed Data */
    miUTF8,		        /* Unicode UTF-8 Encoded Character Data */
    miUTF16,		        /* Unicode UTF-16 Encoded Character Data */
    miUTF32			/* Unicode UTF-32 Encoded Character Data */
  } mat5_data_type;

typedef enum
  {
    mxCELL_CLASS=1,		/* cell array */
    mxSTRUCT_CLASS,		/* structure */
    mxOBJECT_CLASS,		/* object */
    mxCHAR_CLASS,		/* character array */
    mxSPARSE_CLASS,		/* sparse array */
    mxDOUBLE_CLASS,		/* double precision array */
    mxSINGLE_CLASS,		/* single precision floating point */
    mxINT8_CLASS,		/* 8 bit signed integer */
    mxUINT8_CLASS,		/* 8 bit unsigned integer */
    mxINT16_CLASS,		/* 16 bit signed integer */
    mxUINT16_CLASS,		/* 16 bit unsigned integer */
    mxINT32_CLASS,		/* 32 bit signed integer */
    mxUINT32_CLASS,		/* 32 bit unsigned integer */
    mxINT64_CLASS,		/* 64 bit signed integer */
    mxUINT64_CLASS,		/* 64 bit unsigned integer */
    mxFUNCTION_CLASS            /* Function handle */
  } arrayclasstype;

#define FLAG_COMPLEX 0x8
#define FLAG_GLOBAL  0x4
#define FLAG_LOGICAL 0x2

static const QuantumType z2qtype[4] = {GrayQuantum, BlueQuantum, GreenQuantum, RedQuantum};


static void InsertComplexDoubleRow(double *p, int y, Image * image, double MinVal,
                                  double MaxVal)
{
  double f;
  int x;
  register PixelPacket *q;

  if (MinVal == 0)
    MinVal = -1;
  if (MaxVal == 0)
    MaxVal = 1;

  q = SetImagePixels(image, 0, y, image->columns, 1);
  if (q == (PixelPacket *) NULL)
    return;
  for (x = 0; x < (long) image->columns; x++)
  {
    if (*p > 0)
    {
      f = (*p / MaxVal) * (MaxRGB - q->red);
      if (f + q->red > MaxRGB)
        q->red = MaxRGB;
      else
        q->red += (int) f;
      if ((int) f / 2.0 > q->green)
        q->green = q->blue = 0;
      else
        q->green = q->blue -= (int) (f / 2.0);
    }
    if (*p < 0)
    {
      f = (*p / MaxVal) * (MaxRGB - q->blue);
      if (f + q->blue > MaxRGB)
        q->blue = MaxRGB;
      else
        q->blue += (int) f;
      if ((int) f / 2.0 > q->green)
        q->green = q->red = 0;
      else
        q->green = q->red -= (int) (f / 2.0);
    }
    p++;
    q++;
  }
  if (!SyncImagePixels(image))
    return;
  /*          if (image->previous == (Image *) NULL)
     if (QuantumTick(y,image->rows))
     MagickMonitor(LoadImageText,image->rows-y-1,image->rows); */
  return;
}


static void InsertComplexFloatRow(float *p, int y, Image * image, double MinVal,
                                  double MaxVal)
{
  double f;
  int x;
  register PixelPacket *q;

  if (MinVal == 0)
    MinVal = -1;
  if (MaxVal == 0)
    MaxVal = 1;

  q = SetImagePixels(image, 0, y, image->columns, 1);
  if (q == (PixelPacket *) NULL)
    return;
  for (x = 0; x < (long) image->columns; x++)
  {
    if (*p > 0)
    {
      f = (*p / MaxVal) * (MaxRGB - q->red);
      if (f + q->red > MaxRGB)
        q->red = MaxRGB;
      else
        q->red += (int) f;
      if ((int) f / 2.0 > q->green)
        q->green = q->blue = 0;
      else
        q->green = q->blue -= (int) (f / 2.0);
    }
    if (*p < 0)
    {
      f = (*p / MaxVal) * (MaxRGB - q->blue);
      if (f + q->blue > MaxRGB)
        q->blue = MaxRGB;
      else
        q->blue += (int) f;
      if ((int) f / 2.0 > q->green)
        q->green = q->red = 0;
      else
        q->green = q->red -= (int) (f / 2.0);
    }
    p++;
    q++;
  }
  if (!SyncImagePixels(image))
    return;
  /*          if (image->previous == (Image *) NULL)
     if (QuantumTick(y,image->rows))
     MagickMonitor(LoadImageText,image->rows-y-1,image->rows); */
  return;
}


/************** READERS ******************/



static void FixSignedValues(PixelPacket *q, int y)
{
  while(y-->0)
  {
     /* Please note that negative values will overflow
        Q=8; MaxRGB=255: <0;127> + 127+1 = <128; 255> 
		       <-1;-128> + 127+1 = <0; 127> */
    q->red += MaxRGB/2 + 1;
    q->green += MaxRGB/ + 1;
    q->blue += MaxRGB/ + 1;
    q++;
  }
}


/** Fix whole row of logical/binary data. It means pack it. */
static void FixLogical(unsigned char *Buff,int ldblk)
{
unsigned char mask=128;
unsigned char *BuffL = Buff;
unsigned char val = 0;

  while(ldblk-->0)
  {
    if(*Buff++ != 0)
      val |= mask;    

    mask >>= 1;
    if(mask==0)
    {
      *BuffL++ = val;
      val = 0;
      mask = 128;
    }   
      
  }
  *BuffL = val;
}

#if defined(HasZLIB)
static voidpf ZLIBAllocFunc(voidpf opaque, uInt items, uInt size)
{
  ARG_NOT_USED(opaque);
  return MagickMallocCleared((size_t) items*size);
}
static void ZLIBFreeFunc(voidpf opaque, voidpf address)
{
  ARG_NOT_USED(opaque);
  MagickFree(address);
}

/** This procedure decompreses an image block for a new MATLAB format. */
static Image *DecompressBlock(Image *orig, magick_off_t Size, ImageInfo *clone_info, ExceptionInfo *exception)
{
Image *image2;
void *cache_block, *decompress_block;
z_stream zip_info;
FILE *mat_file;
size_t magick_size;
int status;

  if(clone_info==NULL) return NULL;
  if(clone_info->file)		/* Close file opened from previous transaction. */
  {
    fclose(clone_info->file);
    clone_info->file = NULL;
    (void) unlink(clone_info->filename);
  }

  cache_block = MagickAllocateMemory(unsigned char *,(size_t)((Size<16384)?Size:16384));
  if(cache_block==NULL) return NULL;
  decompress_block = MagickAllocateMemory(unsigned char *,(size_t)(4096));
  if(decompress_block==NULL) 
  {
    MagickFreeMemory(cache_block);    
    return NULL;
  }

  mat_file = AcquireTemporaryFileStream(clone_info->filename,BinaryFileIOMode);
  if(!mat_file)
  {
    MagickFreeMemory(cache_block);
    MagickFreeMemory(decompress_block);
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Gannot create file stream for PS image");
    return NULL;
  }

  zip_info.zalloc = ZLIBAllocFunc;
  zip_info.zfree = ZLIBFreeFunc;
  zip_info.opaque = (voidpf) NULL;
  inflateInit(&zip_info);
  /* zip_info.next_out = 8*4; */

  zip_info.avail_in = 0;
  zip_info.total_out = 0;
  while(Size>0 && !EOFBlob(orig))
  {    
    magick_size = ReadBlob(orig, (Size<16384)?Size:16384, cache_block);
    zip_info.next_in = cache_block;
    zip_info.avail_in = (uInt) magick_size;    

    while(zip_info.avail_in>0)
    {
      zip_info.avail_out = 4096;    
      zip_info.next_out = decompress_block;
      status = inflate(&zip_info,Z_NO_FLUSH);      
      fwrite(decompress_block, 4096-zip_info.avail_out, 1, mat_file);

      if(status == Z_STREAM_END) goto DblBreak;
    }

    Size -= magick_size;
  }
DblBreak:
 
  inflateEnd(&zip_info);			/* Release all caches used by zip. */
  (void)fclose(mat_file);
  MagickFreeMemory(cache_block);
  MagickFreeMemory(decompress_block);

  if((clone_info->file=fopen(clone_info->filename,"rb"))==NULL) goto UnlinkFile;
  if( (image2 = AllocateImage(clone_info))==NULL ) goto EraseFile;  
  status = OpenBlob(clone_info,image2,ReadBinaryBlobMode,exception);
  if (status == False)
  {
    DeleteImageFromList(&image2);    
EraseFile:
    fclose(clone_info->file);
    clone_info->file = NULL;
UnlinkFile:
    (void) unlink(clone_info->filename);
    return NULL; 
  }

  return image2;
}
#endif /*  defined(HasZLIB) */



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d M A T L A B i m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadMATImage reads an MAT X image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadMATImage method is:
%
%      Image *ReadMATImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadMATImage returns a pointer to the image after
%      reading. A null image is returned if there is a memory shortage or if
%      the image cannot be read.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadMATImage(const ImageInfo *image_info, ExceptionInfo *exception)
{
  Image *image, *image2=NULL,
   *rotated_image;
  PixelPacket *q;
  unsigned int status;
  MATHeader MATLAB_HDR;
  unsigned long size;  
  magick_uint32_t CellType;
  ImportPixelAreaOptions import_options;
  int i;
  long ldblk;
  unsigned char *BImgBuff = NULL;
  double MinVal, MaxVal;
  unsigned z, z2;
  unsigned Frames;
  int logging;
  int sample_size;
  magick_off_t filepos=0x80;
  BlobInfo *blob;
  ImageInfo *clone_info = NULL;
  
  magick_uint32_t (*ReadBlobXXXLong)(Image *image);
  magick_uint16_t (*ReadBlobXXXShort)(Image *image);
  size_t (*ReadBlobXXXDoubles)(Image * image, size_t len, double *data);
  size_t (*ReadBlobXXXFloats)(Image * image, size_t len, float *data);


  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  logging = LogMagickEvent(CoderEvent,GetMagickModule(),"enter"); 

  /*
     Open image file.
   */
  image = AllocateImage(image_info);

  status = OpenBlob(image_info, image, ReadBinaryBlobMode, exception);
  if (status == False)
    ThrowReaderException(FileOpenError, UnableToOpenFile, image);
  /*
     Read MATLAB image.
   */
  if(ReadBlob(image,124,&MATLAB_HDR.identific) != 124)
    ThrowReaderException(CorruptImageError,ImproperImageHeader,image);  
  MATLAB_HDR.Version = ReadBlobLSBShort(image);
  if(ReadBlob(image,2,&MATLAB_HDR.EndianIndicator) != 2)
    ThrowReaderException(CorruptImageError,ImproperImageHeader,image);

  ImportPixelAreaOptionsInit(&import_options);

  if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),"  Endian %c%c",
        MATLAB_HDR.EndianIndicator[0],MATLAB_HDR.EndianIndicator[1]);
  if (!strncmp(MATLAB_HDR.EndianIndicator, "IM", 2))
  {
    ReadBlobXXXLong = ReadBlobLSBLong;
    ReadBlobXXXShort = ReadBlobLSBShort;
    ReadBlobXXXDoubles = ReadBlobLSBDoubles;
    ReadBlobXXXFloats = ReadBlobLSBFloats;
    import_options.endian = LSBEndian;
  } 
  else if (!strncmp(MATLAB_HDR.EndianIndicator, "MI", 2))
  {
    ReadBlobXXXLong = ReadBlobMSBLong;
    ReadBlobXXXShort = ReadBlobMSBShort;
    ReadBlobXXXDoubles = ReadBlobMSBDoubles;
    ReadBlobXXXFloats = ReadBlobMSBFloats;
    import_options.endian = MSBEndian;
  }
  else 
    goto MATLAB_KO;    /* unsupported endian */

  if (strncmp(MATLAB_HDR.identific, "MATLAB", 6))
MATLAB_KO: ThrowReaderException(CorruptImageError,ImproperImageHeader,image);

  filepos = TellBlob(image);
  while(!EOFBlob(image)) /* object parser loop */
  {
    Frames = 1;
    (void) SeekBlob(image,filepos,SEEK_SET);
    /* printf("pos=%X\n",TellBlob(image)); */

    MATLAB_HDR.DataType = ReadBlobXXXLong(image);
    if(EOFBlob(image)) break;
    MATLAB_HDR.ObjectSize = ReadBlobXXXLong(image);
    if(EOFBlob(image)) break;
    filepos += MATLAB_HDR.ObjectSize + 4 + 4;

    image2 = image;
#if defined(HasZLIB)
    if(MATLAB_HDR.DataType == miCOMPRESSED)
    {
      if(clone_info==NULL)
        if((clone_info=CloneImageInfo(image_info)) == NULL)
		{
		  if(logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
                                   "CloneImageInfo failed");
		  continue;
		}
      image2 = DecompressBlock(image,MATLAB_HDR.ObjectSize,clone_info,exception);
      if(image2==NULL) 
      {
        if(logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
                                   "Decompression failed");
        continue;
      }
      MATLAB_HDR.DataType = ReadBlobXXXLong(image2); /* replace compressed object type. */
    }
#endif    

    if(MATLAB_HDR.DataType!=miMATRIX) continue;  /* skip another objects. */
 
    MATLAB_HDR.unknown1 = ReadBlobXXXLong(image2);
    MATLAB_HDR.unknown2 = ReadBlobXXXLong(image2);  

    MATLAB_HDR.unknown5 = ReadBlobXXXLong(image2);
    MATLAB_HDR.StructureClass = MATLAB_HDR.unknown5 & 0xFF;
    MATLAB_HDR.StructureFlag = (MATLAB_HDR.unknown5>>8) & 0xFF;  

    MATLAB_HDR.unknown3 = ReadBlobXXXLong(image2);
    if(image!=image2)
      MATLAB_HDR.unknown4 = ReadBlobXXXLong(image2);	/* ??? don't understand why ?? */
    MATLAB_HDR.unknown4 = ReadBlobXXXLong(image2);
    MATLAB_HDR.DimFlag = ReadBlobXXXLong(image2);
    MATLAB_HDR.SizeX = ReadBlobXXXLong(image2);
    MATLAB_HDR.SizeY = ReadBlobXXXLong(image2);  
   

    switch(MATLAB_HDR.DimFlag)
    {     
      case  8: z2=z=1; break;			/* 2D matrix*/
      case 12: z2=z = ReadBlobXXXLong(image2);	/* 3D matrix RGB*/
	       (void) ReadBlobXXXLong(image2);  /* Unknown6 */
	       if(z!=3) ThrowReaderException(CoderError, MultidimensionalMatricesAreNotSupported,
                         image);
	       break;
      case 16: z2=z = ReadBlobXXXLong(image2);	/* 4D matrix animation */
	       if(z!=3 && z!=1)
		 ThrowReaderException(CoderError, MultidimensionalMatricesAreNotSupported, image);
               Frames = ReadBlobXXXLong(image2);
	       break;
      default: ThrowReaderException(CoderError, MultidimensionalMatricesAreNotSupported,
                         image);
    }  

    MATLAB_HDR.Flag1 = ReadBlobXXXShort(image2);
    MATLAB_HDR.NameFlag = ReadBlobXXXShort(image2);

    if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
          "MATLAB_HDR.StructureClass %d",MATLAB_HDR.StructureClass);
    if (MATLAB_HDR.StructureClass != mxCHAR_CLASS && 
        MATLAB_HDR.StructureClass != mxSINGLE_CLASS &&		/* float + complex float */
        MATLAB_HDR.StructureClass != mxDOUBLE_CLASS &&		/* double + complex double */
        MATLAB_HDR.StructureClass != mxINT8_CLASS &&
        MATLAB_HDR.StructureClass != mxUINT8_CLASS &&		/* uint8 + uint8 3D */
        MATLAB_HDR.StructureClass != mxINT16_CLASS &&
        MATLAB_HDR.StructureClass != mxUINT16_CLASS &&		/* uint16 + uint16 3D */
        MATLAB_HDR.StructureClass != mxINT32_CLASS &&
        MATLAB_HDR.StructureClass != mxUINT32_CLASS &&		/* uint32 + uint32 3D */
        MATLAB_HDR.StructureClass != mxINT64_CLASS &&
        MATLAB_HDR.StructureClass != mxUINT64_CLASS)		/* uint64 + uint64 3D */
      ThrowReaderException(CoderError,UnsupportedCellTypeInTheMatrix,image);

    switch (MATLAB_HDR.NameFlag)
    {
      case 0:
        size = ReadBlobXXXLong(image2);	/* Object name string size */
        size = 4 * (long) ((size + 3 + 1) / 4);
        (void) SeekBlob(image2, size, SEEK_CUR);
        break;
      case 1:
      case 2:
      case 3:
      case 4:
        (void) ReadBlob(image2, 4, &size); /* Object name string */
        break;
      default:
        goto MATLAB_KO;
    }

    CellType = ReadBlobXXXLong(image2);    /* Additional object type */
    if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
          "MATLAB_HDR.CellType: %d",CellType);
  
    (void) ReadBlob(image2, 4, &size);     /* data size */

NEXT_FRAME:
      /* Image is gray when no complex flag is set and 2D Matrix */
    image->is_grayscale = (MATLAB_HDR.DimFlag==8) && 
           ((MATLAB_HDR.StructureFlag & FLAG_COMPLEX) == 0);

    switch (CellType)
    {
      case miINT8:
      case miUINT8:
        sample_size = 8;
        if(MATLAB_HDR.StructureFlag & FLAG_LOGICAL) 
          image->depth = 1;
        else
          image->depth = Min(QuantumDepth,8);         /* Byte type cell */
        import_options.sample_type = UnsignedQuantumSampleType;
        ldblk = (long) MATLAB_HDR.SizeX;      
        break;
      case miINT16:
      case miUINT16:
        sample_size = 16;
        image->depth = Min(QuantumDepth,16);        /* Word type cell */
        ldblk = (long) (2 * MATLAB_HDR.SizeX);
        import_options.sample_type = UnsignedQuantumSampleType;       
        break;
      case miINT32:
      case miUINT32:
        sample_size = 32;
        image->depth = Min(QuantumDepth,32);        /* Dword type cell */
        ldblk = (long) (4 * MATLAB_HDR.SizeX);      
        import_options.sample_type = UnsignedQuantumSampleType;      
        break;
      case miINT64:
      case miUINT64:
        sample_size = 64;
        image->depth = Min(QuantumDepth,32);        /* Qword type cell */
        ldblk = (long) (8 * MATLAB_HDR.SizeX);      
        import_options.sample_type = UnsignedQuantumSampleType;      
        break;   
      case miSINGLE:
        sample_size = 32;
        image->depth = Min(QuantumDepth,32);        /* double type cell */
        import_options.sample_type = FloatQuantumSampleType;
#if 0
      if (sizeof(float) != 4)
        ThrowReaderException(CoderError, IncompatibleSizeOfFloat, image);
#endif
        if (MATLAB_HDR.StructureFlag & FLAG_COMPLEX)
	{					    /* complex float type cell */
	}
        ldblk = (long) (4 * MATLAB_HDR.SizeX);
        break;
      case miDOUBLE:
        sample_size = 64; 
        image->depth = Min(QuantumDepth,32);        /* double type cell */
        import_options.sample_type = FloatQuantumSampleType;
        if (sizeof(double) != 8)
          ThrowReaderException(CoderError, IncompatibleSizeOfDouble, image);
        if (MATLAB_HDR.StructureFlag & FLAG_COMPLEX)
	{                         /* complex double type cell */        
	}
        ldblk = (long) (8 * MATLAB_HDR.SizeX);
        break;
      default:
        ThrowReaderException(CoderError, UnsupportedCellTypeInTheMatrix, image)
    }

    image->columns = MATLAB_HDR.SizeX;
    image->rows = MATLAB_HDR.SizeY;    
    image->colors = 1l << image->depth;
    if (image->columns == 0 || image->rows == 0)
      goto MATLAB_KO;

    /* ----- Create gray palette ----- */

    if (CellType==miUINT8 && z!=3)
    {
      if(image->colors>256) image->colors = 256;

      if (!AllocateImageColormap(image, image->colors))
      {
NoMemory: ThrowReaderException(ResourceLimitError, MemoryAllocationFailed,
                           image)}
    }

    /*
      If ping is true, then only set image size and colors without
      reading any image data.
    */
    if (image_info->ping)
    {
      unsigned long temp = image->columns;  /* The true image is rotater 90 degs. Do rotation without data. */
      image->columns = image->rows;
      image->rows = temp;
      goto done_reading; /* !!!!!! BAD  !!!! */
    }  

  /* ----- Load raster data ----- */
    BImgBuff = MagickAllocateMemory(unsigned char *,(size_t) (ldblk));    /* Ldblk was set in the check phase */
    if (BImgBuff == NULL)
      goto NoMemory;

    MinVal = 0;
    MaxVal = 0;
    if (CellType==miDOUBLE)        /* Find Min and Max Values for floats */
    {
      (void) MagickFindRawImageMinMax(image2, import_options.endian,MATLAB_HDR.SizeX,
                                      MATLAB_HDR.SizeY,DoublePixel, ldblk, BImgBuff,
                                      &import_options.double_minvalue, 
                                      &import_options.double_maxvalue);
    }
    if (CellType==miSINGLE)        /* Find Min and Max Values for floats */
    {
      (void) MagickFindRawImageMinMax(image2, import_options.endian,MATLAB_HDR.SizeX,
                                      MATLAB_HDR.SizeY,FloatPixel, ldblk, BImgBuff, 
                                      &import_options.double_minvalue, 
                                      &import_options.double_maxvalue);
    }

    /* Main loop for reading all scanlines */
    if(z==1) z=0; /* read grey scanlines */
		/* else read color scanlines */
    do
    {
      for (i = 0; i < (long) MATLAB_HDR.SizeY; i++)
      {
        q=SetImagePixels(image,0,MATLAB_HDR.SizeY-i-1,image->columns,1);
        if (q == (PixelPacket *)NULL)
	{
	  if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
              "  MAT set image pixels returns unexpected NULL on a row %u.", (unsigned)(MATLAB_HDR.SizeY-i-1));
	  goto done_reading;		/* Skip image rotation, when cannot set image pixels */	  
	}
        if(ReadBlob(image2,ldblk,(char *)BImgBuff) != (size_t) ldblk)
	{
	  if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
             "  MAT cannot read scanrow %u from a file.", (unsigned)(MATLAB_HDR.SizeY-i-1));
	  goto ExitLoop;
	}
        if((CellType==miINT8 || CellType==miUINT8) && (MATLAB_HDR.StructureFlag & FLAG_LOGICAL))
        {
          FixLogical((unsigned char *)BImgBuff,ldblk);
          if(ImportImagePixelArea(image,z2qtype[z],1,BImgBuff,&import_options,0) == MagickFail)
	  {
ImportImagePixelAreaFailed:
	    if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
              "  MAT failed to ImportImagePixelArea for a row %u", (unsigned)(MATLAB_HDR.SizeY-i-1));
	    break;
	  }
        }
        else
        {
          if(ImportImagePixelArea(image,z2qtype[z],sample_size,BImgBuff,&import_options,0) == MagickFail)
	    goto ImportImagePixelAreaFailed;

          if (z<=1 &&			 /* fix only during a last pass z==0 || z==1 */
	        (CellType==miINT8 || CellType==miINT16 || CellType==miINT32 || CellType==miINT64))
	    FixSignedValues(q,MATLAB_HDR.SizeX);
        }

        if (!SyncImagePixels(image))
	{
	  if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),
            "  MAT failed to sync image pixels for a row %u", (unsigned)(MATLAB_HDR.SizeY-i-1));
	  goto ExitLoop;
	}
      }
    } while(z-- >= 2);
ExitLoop:


    /* Read complex part of numbers here */
    if (MATLAB_HDR.StructureFlag & FLAG_COMPLEX)
    {        /* Find Min and Max Values for complex parts of floats */
      CellType = ReadBlobXXXLong(image2);    /* Additional object type */
      i = ReadBlobXXXLong(image2);           /* size of a complex part - toss away*/

      if (CellType==miDOUBLE)
      {
        (void) MagickFindRawImageMinMax(image2, import_options.endian, MATLAB_HDR.SizeX,
                                        MATLAB_HDR.SizeY, DoublePixel, ldblk, BImgBuff,
                                        &MinVal, &MaxVal);
      }
      if(CellType==miSINGLE)
      {
        (void) MagickFindRawImageMinMax(image2, import_options.endian, MATLAB_HDR.SizeX,
                                        MATLAB_HDR.SizeY, FloatPixel, ldblk, BImgBuff,
                                        &MinVal, &MaxVal);
      }

      if (CellType==miDOUBLE)
        for (i = 0; i < (long) MATLAB_HDR.SizeY; i++)
	{
          ReadBlobXXXDoubles(image2, ldblk, (double *)BImgBuff);
          InsertComplexDoubleRow((double *)BImgBuff, i, image, MinVal, MaxVal);
	}

      if (CellType==miSINGLE)
        for (i = 0; i < (long) MATLAB_HDR.SizeY; i++)
	{
          ReadBlobXXXFloats(image2, ldblk, (float *)BImgBuff);
          InsertComplexFloatRow((float *)BImgBuff, i, image, MinVal, MaxVal);
	}    
    }

      /* Image is gray when no complex flag is set and 2D Matrix AGAIN!!! */
    image->is_grayscale = (MATLAB_HDR.DimFlag==8) && 
             ((MATLAB_HDR.StructureFlag & FLAG_COMPLEX) == 0);
    image->is_monochrome = image->depth==1;
    if(image->is_monochrome)
      image->colorspace=GRAYColorspace;

    if(image2==image) 
        image2 = NULL;		/* Remove shadow copy to an image before rotation. */

      /*  Rotate image. */
    rotated_image = RotateImage(image, 90.0, exception);
    if (rotated_image != (Image *) NULL)
    {
        /* Remove page offsets added by RotateImage */
      rotated_image->page.x=0;
      rotated_image->page.y=0;

      blob = rotated_image->blob;
      rotated_image->blob = image->blob;
      rotated_image->colors = image->colors;
      image->blob = blob;      
      AppendImageToList(&image,rotated_image);
      DeleteImageFromList(&image);      
    }

done_reading:

    if(image2==image) image2 = NULL;

      /* Allocate next image structure. */    
    AllocateNextImage(image_info,image);
    if (image->next == (Image *) NULL) break;                
    image=SyncNextImageInList(image);
    image->columns=image->rows=0;
    image->colors=0;

      /* row scan buffer is no longer needed */
    MagickFreeMemory(BImgBuff);
    BImgBuff = NULL;

    if(--Frames>0)
    {
      z = z2;
      if(image2==NULL) image2 = image;
      goto NEXT_FRAME;
    }

    if(image2!=NULL)
      if(image2!=image)		/* Does shadow temporary decompressed image exist? */
      {
/*	CloseBlob(image2); */
        DeleteImageFromList(&image2);
	if(clone_info)
	{
          if(clone_info->file)
	  {
            fclose(clone_info->file);
            clone_info->file = NULL;
            (void) unlink(clone_info->filename);
	  }
        }    
      }

  }


  MagickFreeMemory(BImgBuff);
  CloseBlob(image);


  {
    Image *p;    
    long scene=0;
    
    /*
      Rewind list, removing any empty images while rewinding.
    */
    p=image;
    image=NULL;
    while (p != (Image *)NULL)
      {
        Image *tmp=p;
        if ((p->rows == 0) || (p->columns == 0)) {
          p=p->previous;
          DeleteImageFromList(&tmp);
        } else {
          image=p;
          p=p->previous;
        }
      }
    
    /*
      Fix scene numbers
    */
    for (p=image; p != (Image *) NULL; p=p->next)
      p->scene=scene++;
  }

  if(clone_info != NULL)	/* cleanup garbage file from compression */
  {
    if(clone_info->file)
    {
      fclose(clone_info->file);
      clone_info->file = NULL;
      (void) unlink(clone_info->filename);
    }
    DestroyImageInfo(clone_info);
    clone_info = NULL;
  }
  if (logging) (void)LogMagickEvent(CoderEvent,GetMagickModule(),"return");
  if(image==NULL)
    ThrowReaderException(CorruptImageError,ImageFileDoesNotContainAnyImageData,image);
  return (image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M A T L A B I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Function WriteMATLABImage writes an Matlab matrix to a file.  
%
%  The format of the WriteMATLABImage method is:
%
%      unsigned int WriteMATLABImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o status: Function WriteMATLABImage return True if the image is written.
%      False is returned is there is a memory shortage or if the image file
%      fails to write.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o image:  A pointer to an Image structure.
%
*/
static unsigned int WriteMATLABImage(const ImageInfo *image_info,Image *image)
{
  long y;
  unsigned z;
  unsigned int status;
  int logging;
  unsigned long DataSize;
  char padding;
  char MATLAB_HDR[0x80];
  time_t current_time;
  const struct tm *t;
  unsigned char *pixels;
  int is_gray;
  unsigned char ImageName = 'A';
  
  current_time = time((time_t *)NULL);
  t = localtime(&current_time);

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  logging=LogMagickEvent(CoderEvent,GetMagickModule(),"enter MAT");
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == False)
    ThrowWriterException(FileOpenError,UnableToOpenFile,image);  

  /*
    Store MAT header.
  */
  (void) memset(MATLAB_HDR,' ',Min(sizeof(MATLAB_HDR),124));
  sprintf(MATLAB_HDR,"MATLAB 5.0 MAT-file, Platform: %.8s, Created on: %.3s %.3s %2d %2d:%2d:%2d %d",
    OsDesc,
    DayOfWTab[t->tm_wday],
    MonthsTab[t->tm_mon],
    t->tm_mday,
    t->tm_hour,t->tm_min,t->tm_sec,
    t->tm_year+1900);
  MATLAB_HDR[0x7C]=0;
  MATLAB_HDR[0x7D]=1;
  MATLAB_HDR[0x7E]='I';
  MATLAB_HDR[0x7F]='M';  
  (void) WriteBlob(image,sizeof(MATLAB_HDR),MATLAB_HDR);

  pixels=(unsigned char *) NULL;
  while(image!=NULL)
  {
    pixels=MagickAllocateMemory(unsigned char *,image->rows);
    if (pixels == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,MemoryAllocationFailed,image);  

    is_gray = IsGrayImage(image,&image->exception);
    z = is_gray ? 0 : 3;

    DataSize = image->rows /*Y*/ * image->columns /*X*/;
    if(!is_gray) DataSize *= 3 /*Z*/;
    padding=((unsigned char)(DataSize-1) & 0x7) ^ 0x7;

    (void) WriteBlobLSBLong(image, miMATRIX); /* 0x80 */
    (void) WriteBlobLSBLong(image, DataSize + padding + (is_gray?48l:56l)); /* 0x84 */
    (void) WriteBlobLSBLong(image, 0x6); /* 0x88 */
    (void) WriteBlobLSBLong(image, 0x8); /* 0x8C */
    (void) WriteBlobLSBLong(image, 0x6); /* 0x90 */  
    (void) WriteBlobLSBLong(image, 0);   
    (void) WriteBlobLSBLong(image, 0x5); /* 0x98 */
    (void) WriteBlobLSBLong(image, is_gray?0x8:0xC); /* 0x9C - DimFlag */
    (void) WriteBlobLSBLong(image, image->rows);    /* x: 0xA0 */  
    (void) WriteBlobLSBLong(image, image->columns); /* y: 0xA4 */  
    if(!is_gray)
    {
      (void) WriteBlobLSBLong(image, 3); /* z: 0xA8 */  
      (void) WriteBlobLSBLong(image, 0);
    }
    (void) WriteBlobLSBShort(image, 1);  /* 0xB0 */  
    (void) WriteBlobLSBShort(image, 1);  /* 0xB2 */
    (void) WriteBlobLSBLong(image, ImageName++); /* 0xB4  - here is a small bug only 'A' .. 'Z images could be generated properly */
    (void) WriteBlobLSBLong(image, 0x2); /* 0xB8 */  
    (void) WriteBlobLSBLong(image, DataSize); /* 0xBC */

    /*
      Store image data.
    */
    {
      magick_uint64_t
        progress_span;

      magick_int64_t
        progress_quantum;

      progress_span = image->columns;
      if(!is_gray) progress_span *= 3;
      progress_quantum = 0;

      do
      {
        for (y=0; y<(long)image->columns; y++)
	{
          progress_quantum++;
          (void) AcquireImagePixels(image,y,0,1,image->rows,&image->exception);
          (void) ExportImagePixelArea(image,z2qtype[z],8,pixels,0,0);
          (void) WriteBlob(image,image->rows,pixels);
          if (QuantumTick(progress_quantum,progress_span))
            if (!MagickMonitorFormatted(progress_quantum,progress_span,&image->exception,
                                        SaveImageText,image->filename,
					image->columns,image->rows))
              goto BreakAll;
	}    
      } while(z-- >= 2);
    }
BreakAll:

    while(padding-->0) (void) WriteBlobByte(image,0);

    status=True;

    if(pixels) 
     {MagickFreeMemory(pixels);pixels=NULL;}
    if(image->next==NULL) break;
    image=image->next;
  }

  CloseBlob(image);
  MagickFreeMemory(pixels);

  if (logging)
    (void)LogMagickEvent(CoderEvent,GetMagickModule(),"return MAT");
  
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M A T I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterMATImage adds attributes for the MAT image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMATImage method is:
%
%      RegisterMATImage(void)
%
*/
ModuleExport void RegisterMATImage(void)
{
  MagickInfo * entry;

  entry = SetMagickInfo("MAT");
  entry->decoder = (DecoderHandler) ReadMATImage;
  entry->encoder = (EncoderHandler) WriteMATLABImage;
  entry->seekable_stream = True;
  entry->description =
#if defined(HasZLIB)
                        "MATLAB Level 5.0-7.0 image formats";
#else
			"MATLAB Level 5.0-6.0 image formats";
#endif
  entry->module = "MAT";
  entry->blob_support=False;
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M A T I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterMATImage removes format registrations made by the
%  MAT module from the list of supported formats.
%
%  The format of the UnregisterMATImage method is:
%
%      UnregisterMATImage(void)
%
*/
ModuleExport void UnregisterMATImage(void)
{
  (void) UnregisterMagickInfo("MAT");
}
