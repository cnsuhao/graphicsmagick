/*
  Copyright (C) 2003, 2004, 2005 GraphicsMagick Group
  Copyright (C) 2002 ImageMagick Studio
 
  This program is covered by multiple licenses, which are described in
  Copyright.txt. You should have received a copy of Copyright.txt with this
  package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
 
  ImageMagick Constitute Methods.
*/
#ifndef _MAGICK_CONSTITUTE_H
#define _MAGICK_CONSTITUTE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif /* defined(__cplusplus) || defined(c_plusplus) */

/*
  Quantum import/export types as used by ImportImagePixelArea() and
  ExportImagePixelArea(). Values are imported or exported in network
  byte order ("big endian").
*/
typedef enum
{
  UndefinedQuantum,  /* Not specified */
  IndexQuantum,      /* Colormap indexes */
  GrayQuantum,       /* Grayscale values (minimum value is black) */
  IndexAlphaQuantum, /* Colormap indexes with transparency */
  GrayAlphaQuantum,  /* Grayscale values with transparency */
  RedQuantum,        /* Red values only (RGBA) */
  CyanQuantum,       /* Cyan values only (CMYKA) */
  GreenQuantum,      /* Green values only (RGBA) */
  YellowQuantum,     /* Yellow values only (CMYKA) */
  BlueQuantum,       /* Blue values only (RGBA) */
  MagentaQuantum,    /* Magenta values only (CMYKA) */
  AlphaQuantum,      /* Transparency values (RGBA or CMYKA) */
  BlackQuantum,      /* Black values only (CMYKA) */
  RGBQuantum,        /* Red, green, and blue values */
  RGBAQuantum,       /* Red, green, blue, and transparency values */
  CMYKQuantum,       /* Cyan, magenta, yellow, and black values */
  CMYKAQuantum,      /* Cyan, magenta, yellow, black, and transparency values */
  CIEYQuantum,       /* CIE Y values, based on CCIR-709 primaries */
  CIEXYZQuantum      /* CIE XYZ values, based on CCIR-709 primaries */
} QuantumType;

/*
  Quantum sample type for when exporting/importing a pixel area.
*/
typedef enum
{
  UndefinedQuantumSampleType, /* Not specified */
  UnsignedQuantumSampleType,  /* Unsigned integral type (1 to 32 bits) */
  FloatQuantumSampleType      /* Floating point type (16, 32, or 64 bit) */
} QuantumSampleType;

/*
  Quantum size types as used by ConstituteImage() and DispatchImage()/
*/
typedef enum
{
  CharPixel,         /* Unsigned 8 bit 'unsigned char' */
  ShortPixel,        /* Unsigned 16 bit 'unsigned short int' */
  IntegerPixel,      /* Unsigned 32 bit 'unsigned int' */
  LongPixel,         /* Unsigned 32 or 64 bit (CPU dependent) 'unsigned long' */
  FloatPixel,        /* Floating point 32-bit 'float' */
  DoublePixel        /* Floating point 64-bit 'double' */
} StorageType;

/*
  Additional options for ExportImagePixelArea()
*/
typedef struct _ExportPixelAreaOptions
{
  QuantumSampleType
    sample_type;          /* Quantum sample type */

  unsigned int
    sample_bits;          /* Valid bits in unsigned quantum (e.g. 12 bits in 16 bit quantum) */

  double
    double_minvalue,      /* Minimum value for linear floating point samples */
    double_maxvalue;      /* Maximum value for linear floating point samples */

  MagickBool
    grayscale_miniswhite; /* Grayscale minimum value is white rather than black */

  unsigned long
    signature;
} ExportPixelAreaOptions;

/*
  Additional options for ImportImagePixelArea()
*/
typedef struct _ImportPixelAreaOptions
{
  QuantumSampleType
    sample_type;          /* Quantum sample type */

  unsigned int
    sample_bits;          /* Valid bits in unsigned quantum (e.g. 12 bits in 16 bit quantum) */

  double
    double_minvalue,      /* Minimum value for linear floating point samples */
    double_maxvalue;      /* Maximum value for linear floating point samples */

  MagickBool
    grayscale_miniswhite; /* Grayscale minimum value is white rather than black */

  unsigned long
    signature;
} ImportPixelAreaOptions;

extern MagickExport Image
  *ConstituteImage(const unsigned long width,const unsigned long height,
     const char *map,const StorageType type,const void *pixels,
     ExceptionInfo *exception),
  *PingImage(const ImageInfo *image_info,ExceptionInfo *exception),
  *ReadImage(const ImageInfo *image_info,ExceptionInfo *exception),
  *ReadInlineImage(const ImageInfo *image_info,const char *content,
     ExceptionInfo *exception);

MagickExport MagickPassFail
  DispatchImage(const Image *image,const long x_offset,const long y_offset,
    const unsigned long columns,const unsigned long rows,const char *map,
    const StorageType type,void *pixels,ExceptionInfo *exception),
  ExportImagePixelArea(const Image *image,const QuantumType quantum_type,
    const unsigned int quantum_size,unsigned char *destination,
    const ExportPixelAreaOptions *options);

extern MagickExport MagickPassFail
  ImportImagePixelArea(Image *image,const QuantumType quantum_type,
    const unsigned int quantum_size,const unsigned char *source,
    const ImportPixelAreaOptions *options),
  WriteImage(const ImageInfo *image_info,Image *image),
  WriteImages(const ImageInfo *image_info,Image *image,const char *filename,
    ExceptionInfo *exception);

extern MagickExport void
  DestroyConstitute(void),
  ExportPixelAreaOptionsInit(ExportPixelAreaOptions *options),
  ImportPixelAreaOptionsInit(ImportPixelAreaOptions *options);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif /* defined(__cplusplus) || defined(c_plusplus) */

#endif /* _MAGICK_CONSTITUTE_H */
