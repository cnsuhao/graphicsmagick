/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        M   M  IIIII  FFFFF  FFFFF                           %
%                        MM MM    I    F      F                               %
%                        M M M    I    FFF    FFF                             %
%                        M   M    I    F      F                               %
%                        M   M  IIIII  F      F                               %
%                                                                             %
%                                                                             %
%                    Read/Write ImageMagick Image Format.                     %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright (C) 2002 ImageMagick Studio, a non-profit organization dedicated %
%  to making software imaging solutions freely available.                     %
%                                                                             %
%  Permission is hereby granted, free of charge, to any person obtaining a    %
%  copy of this software and associated documentation files ("ImageMagick"),  %
%  to deal in ImageMagick without restriction, including without limitation   %
%  the rights to use, copy, modify, merge, publish, distribute, sublicense,   %
%  and/or sell copies of ImageMagick, and to permit persons to whom the       %
%  ImageMagick is furnished to do so, subject to the following conditions:    %
%                                                                             %
%  The above copyright notice and this permission notice shall be included in %
%  all copies or substantial portions of ImageMagick.                         %
%                                                                             %
%  The software is provided "as is", without warranty of any kind, express or %
%  implied, including but not limited to the warranties of merchantability,   %
%  fitness for a particular purpose and noninfringement.  In no event shall   %
%  ImageMagick Studio be liable for any claim, damages or other liability,    %
%  whether in an action of contract, tort or otherwise, arising from, out of  %
%  or in connection with ImageMagick or the use or other dealings in          %
%  ImageMagick.                                                               %
%                                                                             %
%  Except as contained in this notice, the name of the ImageMagick Studio     %
%  shall not be used in advertising or otherwise to promote the sale, use or  %
%  other dealings in ImageMagick without prior written authorization from the %
%  ImageMagick Studio.                                                        %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "studio.h"
#include "attribute.h"
#include "blob.h"
#include "magick.h"
#include "monitor.h"
#include "utility.h"
#if defined(HasBZLIB)
#include "bzlib.h"
#endif
#if defined(HasZLIB)
#include "zlib.h"
#endif

/*
  Forward declarations.
*/
static unsigned int
  WriteMIFFImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M I F F                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IsMIFF returns True if the image format type, identified by the
%  magick string, is MIFF.
%
%  The format of the IsMIFF method is:
%
%      unsigned int IsMIFF(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o status:  Method IsMIFF returns True if the image format type is MIFF.
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static unsigned int IsMIFF(const unsigned char *magick,const size_t length)
{
  if (length < 14)
    return(False);
  if (LocaleNCompare((char *) magick,"id=ImageMagick",14) == 0)
    return(True);
  return(False);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d M I F F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadMIFFImage reads a MIFF image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadMIFFImage method is:
%
%      Image *ReadMIFFImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  Decompression code contributed by Kyle Shorter.
%
%  A description of each parameter follows:
%
%    o image: Method ReadMIFFImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or
%      if the image cannot be read.
%
%    o image_info: Specifies a pointer to an ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadMIFFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#if defined(HasBZLIB)
  bz_stream
    bzip_info;
#endif

  char
    id[MaxTextExtent],
    keyword[MaxTextExtent],
    *values;

  Image
    *image;

  IndexPacket
    index;

  int
    c,
    length;

  long
    y;

  PixelPacket
    pixel;

  QuantumType
    quantum_type;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  register unsigned char
    *p;

  unsigned char
    *compress_pixels,
    *pixels;

  unsigned int
    status;

  unsigned long
    colors,
    packet_size;

#if defined(HasZLIB)
  z_stream
    zip_info;
#endif

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == False)
    ThrowReaderException(FileOpenError,"UnableToOpenFile",image);
  /*
    Decode image header;  header terminates one character beyond a ':'.
  */
  c=ReadBlobByte(image);
  if (c == EOF)
    {
      DestroyImage(image);
      return((Image *) NULL);
    }
  *id='\0';
  do
  {
    /*
      Decode image header;  header terminates one character beyond a ':'.
    */
    length=MaxTextExtent;
    values=(char *) AcquireMemory(length);
    if (values == (char *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",image);
    colors=0;
    image->depth=8;
    image->compression=NoCompression;
    while (isgraph(c) && (c != ':'))
    {
      register char
        *p;

      if (c == '{')
        {
          char
            *comment;

          /*
            Read comment-- any text between { }.
          */
          length=MaxTextExtent;
          comment=(char *) AcquireMemory(length);
          p=comment;
          for ( ; comment != (char *) NULL; p++)
          {
            c=ReadBlobByte(image);
            if ((c == EOF) || (c == '}'))
              break;
            if ((p-comment+1) >= (int) length)
              {
                *p='\0';
                length<<=1;
                ReacquireMemory((void **) &comment,length);
                if (comment == (char *) NULL)
                  break;
                p=comment+strlen(comment);
              }
            *p=c;
          }
          if (comment == (char *) NULL)
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",
              image);
          *p='\0';
          (void) SetImageAttribute(image,"comment",comment);
          LiberateMemory((void **) &comment);
          c=ReadBlobByte(image);
        }
      else
        if (isalnum(c))
          {
            /*
              Determine a keyword and its value.
            */
            p=keyword;
            do
            {
              if ((p-keyword) < (MaxTextExtent-1))
                *p++=c;
              c=ReadBlobByte(image);
            } while (isalnum(c) || (c == '-'));
            *p='\0';
            while (isspace(c) || (c == '='))
              c=ReadBlobByte(image);
            p=values;
            while ((c != '}') && (c != EOF))
            {
              if ((p-values+1) >= (int) length)
                {
                  *p='\0';
                  length<<=1;
                  ReacquireMemory((void **) &values,length);
                  if (values == (char *) NULL)
                    break;
                  p=values+strlen(values);
                }
              if (values == (char *) NULL)
                ThrowReaderException(ResourceLimitError,
                  "MemoryAllocationFailed",image);
              *p++=c;
              c=ReadBlobByte(image);
              if (*values != '{')
                if (isspace(c))
                  break;
            }
            *p='\0';
            /*
              Assign a value to the specified keyword.
            */
            switch (*keyword)
            {
              case 'b':
              case 'B':
              {
                if (LocaleCompare(keyword,"background-color") == 0)
                  {
                    (void) QueryColorDatabase(values,&image->background_color,
                      exception);
                    break;
                  }
                if (LocaleCompare(keyword,"blue-primary") == 0)
                  {
                    (void) sscanf(values,"%lf,%lf",
                      &image->chromaticity.blue_primary.x,
                      &image->chromaticity.blue_primary.y);
                    break;
                  }
                if (LocaleCompare(keyword,"border-color") == 0)
                  {
                    (void) QueryColorDatabase(values,&image->border_color,
                      exception);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'c':
              case 'C':
              {
                if (LocaleCompare(keyword,"class") == 0)
                  {
                    if (LocaleCompare(values,"PseudoClass") == 0)
                      image->storage_class=PseudoClass;
                    else
                      if (LocaleCompare(values,"DirectClass") == 0)
                        image->storage_class=DirectClass;
                      else
                        image->storage_class=UndefinedClass;
                    break;
                  }
                if (LocaleCompare(keyword,"colors") == 0)
                  {
                    colors=atol(values);
                    break;
                  }
                if (LocaleCompare(keyword,"colorspace") == 0)
                  {
                    if (LocaleCompare(values,"CMYK") == 0)
                      image->colorspace=CMYKColorspace;
                    else
                      if (LocaleCompare(values,"RGB") == 0)
                        image->colorspace=RGBColorspace;
                    break;
                  }
                if (LocaleCompare(keyword,"compression") == 0)
                  {
                    if (LocaleCompare(values,"Zip") == 0)
                      image->compression=ZipCompression;
                    else
                      if (LocaleCompare(values,"BZip") == 0)
                        image->compression=BZipCompression;
                      else
                        if ((LocaleCompare(values,"RLE") == 0) ||
                            (LocaleCompare(values,"RunlengthEncoded") == 0))
                          image->compression=RLECompression;
                        else
                          image->compression=UndefinedCompression;
                    break;
                  }
                if (LocaleCompare(keyword,"columns") == 0)
                  {
                    image->columns= atol(values);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'd':
              case 'D':
              {
                if (LocaleCompare(keyword,"delay") == 0)
                  {
                    image->delay=atol(values);
                    break;
                  }
                if (LocaleCompare(keyword,"depth") == 0)
                  {
                    image->depth=atol(values);
                    break;
                  }
                if (LocaleCompare(keyword,"dispose") == 0)
                  {
                    image->dispose=(DisposeType) atol(values);
                    if (LocaleCompare(values,"Background") == 0)
                      image->dispose=BackgroundDispose;
                    else
                      if (LocaleCompare(values,"None") == 0)
                        image->dispose=NoneDispose;
                      else
                        if (LocaleCompare(values,"Previous") == 0)
                          image->dispose=PreviousDispose;
                        else
                          image->dispose=UndefinedDispose;
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'g':
              case 'G':
              {
                if (LocaleCompare(keyword,"gamma") == 0)
                  {
                    image->gamma=atof(values);
                    break;
                  }
                if (LocaleCompare(keyword,"green-primary") == 0)
                  {
                    (void) sscanf(values,"%lf,%lf",
                      &image->chromaticity.green_primary.x,
                      &image->chromaticity.green_primary.y);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'i':
              case 'I':
              {
                if (LocaleCompare(keyword,"id") == 0)
                  {
                    (void) strncpy(id,values,MaxTextExtent-1);
                    break;
                  }
                if (LocaleCompare(keyword,"iterations") == 0)
                  {
                    image->iterations=atol(values);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'm':
              case 'M':
              {
                if (LocaleCompare(keyword,"matte") == 0)
                  {
                    image->matte=(LocaleCompare(values,"True") == 0) ||
                      (LocaleCompare(values,"true") == 0);
                    break;
                  }
                if (LocaleCompare(keyword,"matte-color") == 0)
                  {
                    (void) QueryColorDatabase(values,&image->matte_color,
                      exception);
                    break;
                  }
                if (LocaleCompare(keyword,"montage") == 0)
                  {
                    (void) CloneString(&image->montage,values);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'o':
              case 'O':
              {
                if (LocaleCompare(keyword,"opaque") == 0)
                  {
                    image->matte=(LocaleCompare(values,"True") == 0) ||
                      (LocaleCompare(values,"true") == 0);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'p':
              case 'P':
              {
                if (LocaleCompare(keyword,"page") == 0)
                  {
                    char
                      *geometry;

                    geometry=GetPageGeometry(values);
                    (void) GetGeometry(geometry,&image->page.x,&image->page.y,
                      &image->page.width,&image->page.height);
                    LiberateMemory((void **) &geometry);
                    break;
                  }
                if (LocaleNCompare(keyword,"profile-",8) == 0)
                  {
                    if (LocaleCompare(keyword,"profile-icc") == 0)
                      {
                        image->color_profile.length=atol(values);
                        break;
                      }
                    if (LocaleCompare(keyword,"profile-iptc") == 0)
                      {
                        image->iptc_profile.length=atol(values);
                        break;
                      }
                    i=(long) image->generic_profiles;
                    if (image->generic_profile == (ProfileInfo *) NULL)
                      image->generic_profile=(ProfileInfo *)
                        AcquireMemory(sizeof(ProfileInfo));
                    else
                      ReacquireMemory((void **) &image->generic_profile,
                        (i+1)*sizeof(ProfileInfo));
                    if (image->generic_profile == (ProfileInfo *) NULL)
                      ThrowReaderException(ResourceLimitError,
                        "MemoryAllocationFailed",image);
                    image->generic_profile[i].name=AllocateString(keyword+8);
                    image->generic_profile[i].length=atol(values);
                    image->generic_profile[i].info=(unsigned char *) NULL;
                    image->generic_profiles++;
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'r':
              case 'R':
              {
                if (LocaleCompare(keyword,"red-primary") == 0)
                  {
                    (void) sscanf(values,"%lf,%lf",
                      &image->chromaticity.red_primary.x,
                      &image->chromaticity.red_primary.y);
                    break;
                  }
                if (LocaleCompare(keyword,"rendering-intent") == 0)
                  {
                    if (LocaleCompare(values,"Saturation") == 0)
                      image->rendering_intent=SaturationIntent;
                    else
                      if (LocaleCompare(values,"perceptual") == 0)
                        image->rendering_intent=PerceptualIntent;
                      else
                        if (LocaleCompare(values,"absolute") == 0)
                          image->rendering_intent=AbsoluteIntent;
                        else
                          if (LocaleCompare(values,"relative") == 0)
                            image->rendering_intent=RelativeIntent;
                          else
                            image->rendering_intent=UndefinedIntent;
                    break;
                  }
                if (LocaleCompare(keyword,"resolution") == 0)
                  {
                    (void) sscanf(values,"%lfx%lf",&image->x_resolution,
                      &image->y_resolution);
                    break;
                  }
                if (LocaleCompare(keyword,"rows") == 0)
                  {
                    image->rows= atol(values);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 's':
              case 'S':
              {
                if (LocaleCompare(keyword,"scene") == 0)
                  {
                    image->scene=atol(values);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'u':
              case 'U':
              {
                if (LocaleCompare(keyword,"units") == 0)
                  {
                    if (LocaleCompare(values,"undefined") == 0)
                      image->units=UndefinedResolution;
                    else
                      if (LocaleCompare(values,"pixels-per-inch") == 0)
                        image->units=PixelsPerInchResolution;
                      else
                        if (LocaleCompare(values,"pixels-per-centimeter") == 0)
                          image->units=PixelsPerCentimeterResolution;
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              case 'w':
              case 'W':
              {
                if (LocaleCompare(keyword,"white-point") == 0)
                  {
                    (void) sscanf(values,"%lf,%lf",
                      &image->chromaticity.white_point.x,
                      &image->chromaticity.white_point.y);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
              default:
              {
                (void) SetImageAttribute(image,keyword,
                  *values == '{' ? values+1 : values);
                break;
              }
            }
          }
        else
          c=ReadBlobByte(image);
      while (isspace(c))
        c=ReadBlobByte(image);
    }
    LiberateMemory((void **) &values);
    (void) ReadBlobByte(image);
    /*
      Verify that required image information is defined.
    */
    if ((LocaleCompare(id,"ImageMagick") != 0) ||
        (image->storage_class == UndefinedClass) ||
        (image->compression == UndefinedCompression) || (image->columns == 0) ||
        (image->rows == 0))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader",image);
    if (image->montage != (char *) NULL)
      {
        register char
          *p;

        /*
          Image directory.
        */
        image->directory=(char *) AcquireMemory(MaxTextExtent);
        if (image->directory == (char *) NULL)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData",image);
        p=image->directory;
        do
        {
          *p='\0';
          if (((strlen(image->directory)+1) % MaxTextExtent) == 0)
            {
              /*
                Allocate more memory for the image directory.
              */
              ReacquireMemory((void **) &image->directory,
                (strlen(image->directory)+MaxTextExtent+1));
              if (image->directory == (char *) NULL)
                ThrowReaderException(CorruptImageError,"UnableToReadImageData",
                  image);
              p=image->directory+strlen(image->directory);
            }
          c=ReadBlobByte(image);
          *p++=c;
        } while (c != '\0');
      }
    if (image->color_profile.length != 0)
      {
        /*
          ICC profile.
        */
        image->color_profile.info=(unsigned char *)
          AcquireMemory(image->color_profile.length);
        if (image->color_profile.info == (unsigned char *) NULL)
          ThrowReaderException(CorruptImageError,"UnableToReadColorProfile",
            image);
        (void) ReadBlob(image,image->color_profile.length,
          image->color_profile.info);
      }
    if (image->iptc_profile.length != 0)
      {
        /*
          IPTC profile.
        */
        image->iptc_profile.info=(unsigned char *)
          AcquireMemory(image->iptc_profile.length);
        if (image->iptc_profile.info == (unsigned char *) NULL)
          ThrowReaderException(CorruptImageError,
            "UnableToReadiPTCProfile",image);
        (void) ReadBlob(image,image->iptc_profile.length,
          image->iptc_profile.info);
      }
    if (image->generic_profiles != 0)
      {
        /*
          Generic profile.
        */
        for (i=0; i < (long) image->generic_profiles; i++)
        {
          if (image->generic_profile[i].length == 0)
            continue;
          image->generic_profile[i].info=(unsigned char *)
            AcquireMemory(image->generic_profile[i].length);
          if (image->generic_profile[i].info == (unsigned char *) NULL)
            ThrowReaderException(CorruptImageError,"UnableToReadGenericProfile",
              image);
          (void) ReadBlob(image,image->generic_profile[i].length,
            image->generic_profile[i].info);
        }
      }
    if (image->storage_class == PseudoClass)
      {
        /*
          Create image colormap.
        */
        if (!AllocateImageColormap(image,colors != 0 ? colors : 256))
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",
            image);
        if (colors != 0)
          {
            unsigned char
              *colormap;

            unsigned int
              packet_size,
              pixel;

            /*
              Read image colormap from file.
            */
            packet_size=3*image->depth/8;
            colormap=(unsigned char *) AcquireMemory(packet_size*image->colors);
            if (colormap == (unsigned char *) NULL)
              ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",
                image);
            (void) ReadBlob(image,packet_size*image->colors,colormap);
            p=colormap;
            if (image->depth <= 8)
              for (i=0; i < (long) image->colors; i++)
              {
                image->colormap[i].red=ScaleCharToQuantum(*p++);
                image->colormap[i].green=ScaleCharToQuantum(*p++);
                image->colormap[i].blue=ScaleCharToQuantum(*p++);
              }
            else
              if (image->depth <= 16)
                for (i=0; i < (long) image->colors; i++)
                {
                  pixel=(*p << 8) | *(p+1);
                  image->colormap[i].red=pixel;
                  p+=2;
                  pixel=(*p << 8) | *(p+1);
                  image->colormap[i].green=pixel;
                  p+=2;
                  pixel=(*p << 8) | *(p+1);
                  image->colormap[i].blue=pixel;
                  p+=2;
                }
              else
                for (i=0; i < (long) image->colors; i++)
                {
                  pixel=(*p << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3);
                  image->colormap[i].red=pixel;
                  p+=4;
                  pixel=(*p << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3);
                  image->colormap[i].green=pixel;
                  p+=4;
                  pixel=(*p << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3);
                  image->colormap[i].blue=pixel;
                  p+=4;
                }
            LiberateMemory((void **) &colormap);
          }
      }
    if (image_info->ping && (image_info->subrange != 0))
      if (image->scene >= (image_info->subimage+image_info->subrange-1))
        break;
    /*
      Allocate image pixels.
    */
    if (image->storage_class == DirectClass)
      packet_size=3*image->depth/8;
    else
      packet_size=image->depth/8;
    if (image->colorspace == CMYKColorspace)
      packet_size+=image->depth/8;
    if (image->matte)
      packet_size+=image->depth/8;
    if (image->compression == RLECompression)
      packet_size+=image->depth/8;
    pixels=(unsigned char *) AcquireMemory(packet_size*image->columns);
    compress_pixels=(unsigned char *)
      AcquireMemory((size_t) (1.01*packet_size*image->columns+600));
    if ((pixels == (unsigned char *) NULL) ||
        (compress_pixels == (unsigned char *) NULL))
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",image);
    /*
      Read image pixels.
    */
    quantum_type=RGBQuantum;
    if (image->storage_class == PseudoClass)
      {
        if (!image->matte)
          quantum_type=IndexQuantum;
        else
          quantum_type=IndexAlphaQuantum;
      }
    else
      if (image->colorspace == CMYKColorspace)
        {
          if (!image->matte)
            quantum_type=CMYKQuantum;
          else
            quantum_type=CMYKAQuantum;
        }
      else
        if (!image->matte)
          quantum_type=RGBQuantum;
        else
          quantum_type=RGBAQuantum;
    index=0;
    length=0;
    for (y=0; y < (long) image->rows; y++)
    {
      q=SetImagePixels(image,0,y,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
#if defined(HasZLIB)
      if (image->compression == ZipCompression)
        {
          if (y == 0)
            {
              zip_info.zalloc=NULL;
              zip_info.zfree=NULL;
              zip_info.opaque=NULL;
              (void) inflateInit(&zip_info);
              zip_info.avail_in=0;
            }
          zip_info.next_out=pixels;
          zip_info.avail_out=(uInt) (packet_size*image->columns);
          do
          {
            if (zip_info.avail_in == 0)
              {
                zip_info.next_in=compress_pixels;
                length=(int) (1.01*packet_size*image->columns+12);
                zip_info.avail_in=(uInt)
                  ReadBlob(image,length,zip_info.next_in);
              }
            if (inflate(&zip_info,Z_NO_FLUSH) == Z_STREAM_END)
              break;
          } while (zip_info.avail_out != 0);
          if (y == (long) (image->rows-1))
            {
              (void) SeekBlob(image,-((ExtendedSignedIntegralType)
                zip_info.avail_in),SEEK_CUR);
              status=!inflateEnd(&zip_info);
            }
        }
      else
#endif
#if defined(HasBZLIB)
        if (image->compression == BZipCompression)
          {
            if (y == 0)
              {
                bzip_info.bzalloc=NULL;
                bzip_info.bzfree=NULL;
                bzip_info.opaque=NULL;
                (void) BZ2_bzDecompressInit(&bzip_info,image_info->verbose,
                  False);
                bzip_info.avail_in=0;
              }
            bzip_info.next_out=(char *) pixels;
            bzip_info.avail_out=(unsigned int) (packet_size*image->columns);
            do
            {
              if (bzip_info.avail_in == 0)
                {
                  bzip_info.next_in=(char *) compress_pixels;
                  length=(int) (1.01*packet_size*image->columns+12);
                  bzip_info.avail_in=(unsigned int)
                    ReadBlob(image,length,bzip_info.next_in);
                }
              if (BZ2_bzDecompress(&bzip_info) == BZ_STREAM_END)
                break;
            } while (bzip_info.avail_out != 0);
            if (y == (long) (image->rows-1))
              {
                (void) SeekBlob(image,-((ExtendedSignedIntegralType)
                  bzip_info.avail_in),SEEK_CUR);
                status=!BZ2_bzDecompressEnd(&bzip_info);
              }
          }
        else
#endif
          if (image->compression != RLECompression)
            (void) ReadBlob(image,packet_size*image->columns,pixels);
      if (image->compression != RLECompression)
        (void) PushImagePixels(image,quantum_type,pixels);
      else
        {
          if (y == 0)
            {
              (void) memset(&pixel,0,sizeof(PixelPacket));
              pixel.opacity=TransparentOpacity;
            }
          p=pixels;
          for (x=0; x < (long) image->columns; x++)
          {
            if (length == 0)
              {
                if (image->storage_class != DirectClass)
                  {
                    index=ReadBlobByte(image);
                    if (image->depth > 8)
                      index=(index << 8)+ReadBlobByte(image);
                    if (image->depth > 16)
                      {
                        index=(index << 8)+ReadBlobByte(image);
                        index=(index << 8)+ReadBlobByte(image);
                      }
                    if (index >= image->colors)
                      {
                        ThrowException(&image->exception,CorruptImageError,
                          "InvalidColormapIndex",image->filename);
                        index=0;
                      }
                    pixel=image->colormap[index];
                  }
                else
                  {
                    if (image->depth <= 8)
                      {
                        pixel.red=ScaleCharToQuantum(ReadBlobByte(image));
                        pixel.green=ScaleCharToQuantum(ReadBlobByte(image));
                        pixel.blue=ScaleCharToQuantum(ReadBlobByte(image));
                        if (image->colorspace == CMYKColorspace)
                          {
                            pixel.opacity=ScaleCharToQuantum(ReadBlobByte(image));
                            if (image->matte)
                              index=ScaleCharToQuantum(ReadBlobByte(image));
                          }
                        else
                          if (image->matte)
                            pixel.opacity=ScaleCharToQuantum(ReadBlobByte(image));
                      }
                    else
                      if (image->depth <= 16)
                        {
                          pixel.red=ReadBlobMSBShort(image) >>
                            (image->depth-QuantumDepth);
                          pixel.green=ReadBlobMSBShort(image) >>
                            (image->depth-QuantumDepth);
                          pixel.blue=ReadBlobMSBShort(image) >>
                            (image->depth-QuantumDepth);
                          if (image->colorspace == CMYKColorspace)
                            {
                              pixel.opacity=(ReadBlobMSBShort(image) >>
                                (image->depth-QuantumDepth));
                              if (image->matte)
                                index=(ReadBlobMSBShort(image) >>
                                  (image->depth-QuantumDepth));
                            }
                          else
                            if (image->matte)
                              pixel.opacity=(ReadBlobMSBShort(image) >>
                                (image->depth-QuantumDepth));
                        }
                      else
                        {
                          pixel.red=(Quantum) (ReadBlobMSBLong(image) >>
                            (image->depth-QuantumDepth));
                          pixel.green=(Quantum) (ReadBlobMSBLong(image) >>
                            (image->depth-QuantumDepth));
                          pixel.blue=(Quantum) (ReadBlobMSBLong(image) >>
                            (image->depth-QuantumDepth));
                          if (image->colorspace == CMYKColorspace)
                            {
                              pixel.opacity=(Quantum) (ReadBlobMSBLong(image) >>
                                (image->depth-QuantumDepth));
                              if (image->matte)
                                index=(IndexPacket) (ReadBlobMSBLong(image) >>
                                  (image->depth-QuantumDepth));
                            }
                          else
                            if (image->matte)
                              pixel.opacity=(Quantum) (ReadBlobMSBLong(image) >>
                                (image->depth-QuantumDepth));
                        }
                  }
                length=ReadBlobByte(image)+1;
              }
            length--;
            if ((image->storage_class == PseudoClass) ||
                (image->colorspace == CMYKColorspace))
              indexes[x]=index;
            *q++=pixel;
          }
        }
      if (!SyncImagePixels(image))
        break;
    }
    LiberateMemory((void **) &pixels);
    LiberateMemory((void **) &compress_pixels);
    if (status == False)
      {
        DestroyImageList(image);
        return((Image *) NULL);
      }
    if (EOFBlob(image))
      ThrowReaderException(CorruptImageError,"UnexpectedEndOfFile",image);
    /*
      Proceed to next image.
    */
    if (image_info->subrange != 0)
      if (image->scene >= (image_info->subimage+image_info->subrange-1))
        break;
    do
    {
      c=ReadBlobByte(image);
    } while (!isgraph(c) && (c != EOF));
    if (c != EOF)
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,image);
        if (image->next == (Image *) NULL)
          {
            DestroyImageList(image);
            return((Image *) NULL);
          }
        image=image->next;
        status=MagickMonitor(LoadImagesText,TellBlob(image),GetBlobSize(image),
          exception);
        if (status == False)
          break;
      }
  } while (c != EOF);
  while (image->previous != (Image *) NULL)
    image=image->previous;
  CloseBlob(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M I F F I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterMIFFImage adds attributes for the MIFF image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMIFFImage method is:
%
%      RegisterMIFFImage(void)
%
*/
ModuleExport void RegisterMIFFImage(void)
{
  char
    version[MaxTextExtent];

  MagickInfo
    *entry;

  *version='\0';
#if defined(MagickLibVersionText)
  (void) strncpy(version,MagickLibVersionText,MaxTextExtent >> 1);
#if defined(ZLIB_VERSION)
  (void) strcat(version," with Zlib ");
  (void) strncat(version,ZLIB_VERSION,MaxTextExtent-strlen(version)-1);
#endif
#if defined(HasBZLIB)
  (void) strcat(version," and BZlib");
#endif
#endif
  entry=SetMagickInfo("MIFF");
  entry->decoder=(DecoderHandler) ReadMIFFImage;
  entry->encoder=(EncoderHandler) WriteMIFFImage;
  entry->magick=(MagickHandler) IsMIFF;
  entry->description=AcquireString("Magick Image File Format");
  if (*version != '\0')
    entry->version=AcquireString(version);
  entry->module=AcquireString("MIFF");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M I F F I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterMIFFImage removes format registrations made by the
%  MIFF module from the list of supported formats.
%
%  The format of the UnregisterMIFFImage method is:
%
%      UnregisterMIFFImage(void)
%
*/
ModuleExport void UnregisterMIFFImage(void)
{
  (void) UnregisterMagickInfo("MIFF");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M I F F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteMIFFImage writes a MIFF image to a file.
%
%  The format of the WriteMIFFImage method is:
%
%      unsigned int WriteMIFFImage(const ImageInfo *image_info,Image *image)
%
%  Compression code contributed by Kyle Shorter.
%
%  A description of each parameter follows:
%
%    o status: Method WriteMIFFImage return True if the image is written.
%      False is returned if there is a memory shortage or if the image file
%      fails to write.
%
%    o image_info: Specifies a pointer to an ImageInfo structure.
%
%    o image: A pointer to a Image structure.
%
%
*/

static unsigned int WriteMIFFImage(const ImageInfo *image_info,Image *image)
{
#define WriteRunlengthPacket(image,pixel,length,q,index) \
{ \
  if (image->storage_class != DirectClass) \
    { \
      if (image->depth > 16) \
        { \
          *q++=(unsigned char) (index >> 24); \
          *q++=(unsigned char) (index >> 16); \
        } \
      if (image->depth > 8) \
        *q++=(unsigned char) (index >> 8); \
      *q++=(unsigned char) index; \
    } \
  else \
    { \
      if (image->depth <= 8) \
        { \
          *q++=ScaleQuantumToChar(pixel.red); \
          *q++=ScaleQuantumToChar(pixel.green); \
          *q++=ScaleQuantumToChar(pixel.blue); \
          if (image->colorspace == CMYKColorspace) \
            { \
              *q++=ScaleQuantumToChar(pixel.opacity); \
              if (image->matte) \
                *q++=ScaleQuantumToChar(index); \
            } \
          else \
            if (image->matte) \
              *q++=ScaleQuantumToChar(pixel.opacity); \
        } \
      else \
        if (image->depth <= 16) \
          { \
            value=ScaleQuantumToShort(pixel.red); \
            *q++=(unsigned char) (value >> 8); \
            *q++=(unsigned char) value; \
            value=ScaleQuantumToShort(pixel.green); \
            *q++=(unsigned char) (value >> 8); \
            *q++=(unsigned char) value; \
            value=ScaleQuantumToShort(pixel.blue); \
            *q++=(unsigned char) (value >> 8); \
            *q++=(unsigned char) value; \
            if (image->colorspace == CMYKColorspace) \
              { \
                value=ScaleQuantumToShort(pixel.opacity); \
                *q++=(unsigned char) (value >> 8); \
                *q++=(unsigned char) value; \
                if (image->matte) \
                  { \
                    value=ScaleQuantumToShort(index); \
                    *q++=(unsigned char) (value >> 8); \
                    *q++=(unsigned char) value; \
                  } \
              } \
            else \
              if (image->matte) \
                { \
                  value=ScaleQuantumToShort(pixel.opacity); \
                  *q++=(unsigned char) (value >> 8); \
                  *q++=(unsigned char) value; \
                } \
          } \
        else \
          { \
            value=ScaleQuantumToLong(pixel.red); \
            *q++=(unsigned char) (value >> 24); \
            *q++=(unsigned char) (value >> 16); \
            *q++=(unsigned char) (value >> 8); \
            *q++=(unsigned char) value; \
            value=ScaleQuantumToShort(pixel.green); \
            *q++=(unsigned char) (value >> 24); \
            *q++=(unsigned char) (value >> 16); \
            *q++=(unsigned char) (value >> 8); \
            *q++=(unsigned char) value; \
            value=ScaleQuantumToShort(pixel.blue); \
            *q++=(unsigned char) (value >> 24); \
            *q++=(unsigned char) (value >> 16); \
            *q++=(unsigned char) (value >> 8); \
            *q++=(unsigned char) value; \
            if (image->colorspace == CMYKColorspace) \
              { \
                value=ScaleQuantumToShort(pixel.opacity); \
                *q++=(unsigned char) (value >> 24); \
                *q++=(unsigned char) (value >> 16); \
                *q++=(unsigned char) (value >> 8); \
                *q++=(unsigned char) value; \
                if (image->matte) \
                  { \
                    value=ScaleQuantumToShort(index); \
                    *q++=(unsigned char) (value >> 24); \
                    *q++=(unsigned char) (value >> 16); \
                    *q++=(unsigned char) (value >> 8); \
                    *q++=(unsigned char) value; \
                  } \
              } \
            else \
              if (image->matte) \
                { \
                  value=ScaleQuantumToShort(pixel.opacity); \
                  *q++=(unsigned char) (value >> 24); \
                  *q++=(unsigned char) (value >> 16); \
                  *q++=(unsigned char) (value >> 8); \
                  *q++=(unsigned char) value; \
                } \
          } \
    } \
  *q++=(unsigned char) length; \
}

#if defined(HasBZLIB)
  bz_stream
    bzip_info;
#endif

  char
    buffer[MaxTextExtent];

  CompressionType
    compression;

  const ImageAttribute
    *attribute;

  IndexPacket
    index;

  long
    y;

  PixelPacket
    pixel;

  QuantumType
    quantum_type;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register unsigned char
    *q;

  size_t
    length;

  unsigned char
    *compress_pixels,
    *pixels;

  unsigned int
    status;

  unsigned long
    packet_size,
    scene,
    value;

#if defined(HasZLIB)
  z_stream
    zip_info;
#endif

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == False)
    ThrowWriterException(FileOpenError,"UnableToOpenFile",image);
  compression=image->compression;
  if (image_info->compression != UndefinedCompression)
    compression=image_info->compression;
  scene=0;
  do
  {
    /*
      Allocate image pixels.
    */
    if (((image_info->colorspace != UndefinedColorspace) ||
         (image->colorspace != CMYKColorspace)) &&
         (image_info->colorspace != CMYKColorspace))
      (void) TransformRGBImage(image,RGBColorspace);
    else
      if (image->colorspace != CMYKColorspace)
        (void) RGBTransformImage(image,CMYKColorspace);
    if (image->storage_class == DirectClass)
      packet_size=3*image->depth/8;
    else
      packet_size=image->depth/8;
    if (image->colorspace == CMYKColorspace)
      packet_size+=image->depth/8;
    if (image->matte)
      packet_size+=image->depth/8;
    if (compression == RLECompression)
      packet_size+=image->depth/8;
    length=packet_size*image->columns;
    pixels=(unsigned char *) AcquireMemory(length);
    length=(size_t) 1.01*packet_size*image->columns+600;
    if ((compression == BZipCompression) || (compression == ZipCompression))
      if (length != (unsigned int) length)
        compression=NoCompression;
    compress_pixels=(unsigned char *) AcquireMemory(length);
    if ((pixels == (unsigned char *) NULL) ||
        (compress_pixels == (unsigned char *) NULL))
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",image);
    /*
      Write MIFF header.
    */
    (void) WriteBlobString(image,"id=ImageMagick\n");
    if (image->storage_class == PseudoClass)
      FormatString(buffer,"class=PseudoClass  colors=%lu  matte=%.1024s\n",
        image->colors,image->matte ? "True" : "False");
    else
      if (image->colorspace == CMYKColorspace)
        FormatString(buffer,
          "class=DirectClass  colorspace=CMYK  matte=%.1024s\n",
          image->matte ? "True" : "False");
      else
        FormatString(buffer,"class=DirectClass  matte=%.1024s\n",
          image->matte ? "True" : "False");
    (void) WriteBlobString(image,buffer);
    *buffer='\0';
    if (compression == RLECompression)
      FormatString(buffer,"compression=RLE\n");
    else
      if (compression == BZipCompression)
        FormatString(buffer,"compression=BZip\n");
      else
        if (compression == ZipCompression)
          FormatString(buffer,"compression=Zip\n");
    if (*buffer != '\0')
      (void) WriteBlobString(image,buffer);
    FormatString(buffer,"columns=%lu  rows=%lu  depth=%lu\n",image->columns,
      image->rows,image->depth);
    (void) WriteBlobString(image,buffer);
    if ((image->x_resolution != 0) && (image->y_resolution != 0))
      {
        char
          units[MaxTextExtent];

        /*
          Set image resolution.
        */
        (void) strcpy(units,"undefined");
        if (image->units == PixelsPerInchResolution)
          (void) strcpy(units,"pixels-per-inch");
        if (image->units == PixelsPerCentimeterResolution)
          (void) strcpy(units,"pixels-per-centimeter");
        FormatString(buffer,"Resolution=%gx%g  units=%.1024s\n",
          image->x_resolution,image->y_resolution,units);
        (void) WriteBlobString(image,buffer);
      }
    if ((image->page.width != 0) && (image->page.height != 0))
      {
        FormatString(buffer,"page=%lux%lu%+ld%+ld\n",image->page.width,
          image->page.height,image->page.x,image->page.y);
        (void) WriteBlobString(image,buffer);
      }
    if ((image->next != (Image *) NULL) || (image->previous != (Image *) NULL))
      {
        if (image->scene == 0)
          FormatString(buffer,"iterations=%lu  delay=%lu\n",image->iterations,
            image->delay);
        else
          FormatString(buffer,"scene=%lu  iterations=%lu  delay=%lu\n",
            image->scene,image->iterations,image->delay);
        (void) WriteBlobString(image,buffer);
      }
    else
      {
        if (image->scene != 0)
          {
            FormatString(buffer,"scene=%lu\n",image->scene);
            (void) WriteBlobString(image,buffer);
          }
        if (image->iterations != 0)
          {
            FormatString(buffer,"iterations=%lu\n",image->iterations);
            (void) WriteBlobString(image,buffer);
          }
        if (image->delay != 0)
          {
            FormatString(buffer,"delay=%lu\n",image->delay);
            (void) WriteBlobString(image,buffer);
          }
      }
    if (image->dispose != UndefinedDispose)
      {
        if (image->dispose == BackgroundDispose)
          (void) strcpy(buffer,"dispose=background\n");
        else
          if (image->dispose == NoneDispose)
            (void) strcpy(buffer,"dispose=none\n");
          else
            (void) strcpy(buffer,"dispose=previous\n");
        (void) WriteBlobString(image,buffer);
      }
    if (image->rendering_intent != UndefinedIntent)
      {
        if (image->rendering_intent == SaturationIntent)
          (void) strcpy(buffer,"rendering-intent=saturation\n");
        else
          if (image->rendering_intent == PerceptualIntent)
            (void) strcpy(buffer,"rendering-intent=perceptual\n");
          else
            if (image->rendering_intent == AbsoluteIntent)
              (void) strcpy(buffer,"rendering-intent=absolute\n");
            else
              (void) strcpy(buffer,"rendering-intent=relative\n");
        (void) WriteBlobString(image,buffer);
      }
    if (image->gamma != 0.0)
      {
        FormatString(buffer,"gamma=%g\n",image->gamma);
        (void) WriteBlobString(image,buffer);
      }
    if (image->chromaticity.white_point.x != 0.0)
      {
        /*
          Note chomaticity points.
        */
        FormatString(buffer,
          "red-primary=%g,%g  green-primary=%g,%g  blue-primary=%g,%g\n",
          image->chromaticity.red_primary.x,image->chromaticity.red_primary.y,
          image->chromaticity.green_primary.x,
          image->chromaticity.green_primary.y,
          image->chromaticity.blue_primary.x,
          image->chromaticity.blue_primary.y);
        (void) WriteBlobString(image,buffer);
        FormatString(buffer,"white-point=%g,%g\n",
          image->chromaticity.white_point.x,image->chromaticity.white_point.y);
        (void) WriteBlobString(image,buffer);
      }
    if (image->color_profile.length != 0)
      {
        FormatString(buffer,"profile-icc=%lu\n",(unsigned long)
          image->color_profile.length);
        (void) WriteBlobString(image,buffer);
      }
    if (image->iptc_profile.length != 0)
      {
        FormatString(buffer,"profile-iptc=%lu\n",(unsigned long)
          image->iptc_profile.length);
        (void) WriteBlobString(image,buffer);
      }
    if (image->generic_profiles != 0)
      {
        /*
          Generic profile.
        */
        for (i=0; i < (long) image->generic_profiles; i++)
        {
          FormatString(buffer,"profile-%.1024s=%lu\n",
            image->generic_profile[i].name == (char *) NULL ? "generic" :
            image->generic_profile[i].name,(unsigned long)
            image->generic_profile[i].length);
          (void) WriteBlobString(image,buffer);
        }
      }
    if (image->montage != (char *) NULL)
      {
        FormatString(buffer,"montage=%.1024s\n",image->montage);
        (void) WriteBlobString(image,buffer);
      }
    attribute=GetImageAttribute(image,(char *) NULL);
    for ( ; attribute != (const ImageAttribute *) NULL; attribute=attribute->next)
    {
      if (*attribute->key == '[')
        continue;
      FormatString(buffer,"%.1024s=",attribute->key);
      (void) WriteBlobString(image,buffer);
      for (i=0; i < (long) strlen(attribute->value); i++)
        if (isspace((int) attribute->value[i]))
          break;
      if (i < (long) strlen(attribute->value))
        (void) WriteBlobByte(image,'{');
      (void) WriteBlob(image,strlen(attribute->value),attribute->value);
      if (i < (long) strlen(attribute->value))
        (void) WriteBlobByte(image,'}');
      (void) WriteBlobByte(image,'\n');
    }
    (void) WriteBlobString(image,"\f\n:\032");
    if (image->montage != (char *) NULL)
      {
        /*
          Write montage tile directory.
        */
        if (image->directory != (char *) NULL)
          (void) WriteBlob(image,strlen(image->directory),image->directory);
        (void) WriteBlobByte(image,'\0');
      }
    if (image->color_profile.length != 0)
      (void) WriteBlob(image,image->color_profile.length,
        (char *) image->color_profile.info);
    if (image->iptc_profile.length != 0)
      (void) WriteBlob(image,image->iptc_profile.length,
        (char *) image->iptc_profile.info);
    if (image->generic_profiles != 0)
      {
        /*
          Generic profile.
        */
        for (i=0; i < (long) image->generic_profiles; i++)
        {
          if (image->generic_profile[i].length == 0)
            continue;
          (void) WriteBlob(image,image->generic_profile[i].length,
            (char *) image->generic_profile[i].info);
        }
      }
    if (image->storage_class == PseudoClass)
      {
        register unsigned char
          *q;

        unsigned char
          *colormap;

        unsigned int
          packet_size;

        /*
          Allocate colormap.
        */
        packet_size=3*image->depth/8;
        colormap=(unsigned char *) AcquireMemory(packet_size*image->colors);
        if (colormap == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",
            image);
        /*
          Write colormap to file.
        */
        q=colormap;
        if (image->depth <= 8)
          for (i=0; i < (long) image->colors; i++)
          {
            *q++=ScaleQuantumToChar(image->colormap[i].red);
            *q++=ScaleQuantumToChar(image->colormap[i].green);
            *q++=ScaleQuantumToChar(image->colormap[i].blue);
          }
        else
          if (image->depth <= 16)
            for (i=0; i < (long) image->colors; i++)
            {
              *q++=ScaleQuantumToShort(image->colormap[i].red) >> 8;
              *q++=ScaleQuantumToShort(image->colormap[i].red);
              *q++=ScaleQuantumToShort(image->colormap[i].green) >> 8;
              *q++=ScaleQuantumToShort(image->colormap[i].green);
              *q++=ScaleQuantumToShort(image->colormap[i].blue) >> 8;
              *q++=ScaleQuantumToShort(image->colormap[i].blue);
            }
          else
            for (i=0; i < (long) image->colors; i++)
            {
              *q++=image->colormap[i].red >> 24;
              *q++=image->colormap[i].red >> 16;
              *q++=image->colormap[i].red >> 8;
              *q++=image->colormap[i].red;
              *q++=image->colormap[i].green >> 24;
              *q++=image->colormap[i].green >> 16;
              *q++=image->colormap[i].green >> 8;
              *q++=image->colormap[i].green;
              *q++=image->colormap[i].blue >> 24;
              *q++=image->colormap[i].blue >> 16;
              *q++=image->colormap[i].blue >> 8;
              *q++=image->colormap[i].blue;
            }
        (void) WriteBlob(image,packet_size*image->colors,colormap);
        LiberateMemory((void **) &colormap);
      }
    /*
      Write image pixels to file.
    */
    quantum_type=RGBQuantum;
    if (image->storage_class == PseudoClass)
      {
        if (!image->matte)
          quantum_type=IndexQuantum;
        else
          quantum_type=IndexAlphaQuantum;
      }
    else
      if (image->colorspace == CMYKColorspace)
        {
          if (!image->matte)
            quantum_type=CMYKQuantum;
          else
            quantum_type=CMYKAQuantum;
        }
      else
        if (!image->matte)
          quantum_type=RGBQuantum;
        else
          quantum_type=RGBAQuantum;
    status=True;
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      q=pixels;
      if (compression != RLECompression)
        (void) PopImagePixels(image,quantum_type,pixels);
      else
        {
          pixel=(*p);
          index=0;
          if (image->storage_class == PseudoClass)
            index=(*indexes);
          length=255;
          for (x=0; x < (long) image->columns; x++)
          {
            if ((p->red == pixel.red) && (p->green == pixel.green) &&
                (p->blue == pixel.blue) && (p->opacity == pixel.opacity) &&
                (length < 255) && (x < (long) (image->columns-1)))
              length++;
            else
              {
                if (x > 0)
                  WriteRunlengthPacket(image,pixel,length,q,index);
                length=0;
              }
            if (image->storage_class == PseudoClass)
              index=indexes[x];
            pixel=(*p);
            p++;
          }
          WriteRunlengthPacket(image,pixel,length,q,index);
        }
#if defined(HasZLIB)
        if (compression == ZipCompression)
          {
            if (y == 0)
              {
                zip_info.zalloc=NULL;
                zip_info.zfree=NULL;
                zip_info.opaque=NULL;
                (void) deflateInit(&zip_info,
                  (int) Min(image_info->quality/10,9));
              }
            zip_info.next_in=pixels;
            zip_info.avail_in=(uInt) (packet_size*image->columns);
            do
            {
              zip_info.next_out=compress_pixels;
              zip_info.avail_out=(uInt) (1.01*packet_size*image->columns+12);
              status=!deflate(&zip_info,Z_NO_FLUSH);
              length=zip_info.next_out-compress_pixels;
              if (zip_info.next_out != compress_pixels)
                (void) WriteBlob(image,length,compress_pixels);
            } while (zip_info.avail_in != 0);
            if (y == (long) (image->rows-1))
              {
                for ( ; ; )
                {
                  zip_info.next_out=compress_pixels;
                  zip_info.avail_out=(uInt)
                    (1.01*packet_size*image->columns+12);
                  status=!deflate(&zip_info,Z_FINISH);
                  if (zip_info.next_out == compress_pixels)
                    break;
                  length=zip_info.next_out-compress_pixels;
                  (void) WriteBlob(image,length,compress_pixels);
                }
                status=!deflateEnd(&zip_info);
              }
          }
        else
#endif
#if defined(HasBZLIB)
        if (compression == BZipCompression)
          {
            if (y == 0)
              {
                bzip_info.bzalloc=NULL;
                bzip_info.bzfree=NULL;
                bzip_info.opaque=NULL;
                (void) BZ2_bzCompressInit(&bzip_info,
                  (int) Min(image_info->quality/10,9),image_info->verbose,0);
              }
            bzip_info.next_in=(char *) pixels;
            bzip_info.avail_in=(unsigned int) (packet_size*image->columns);
            do
            {
              bzip_info.next_out=(char *) compress_pixels;
              bzip_info.avail_out=(unsigned int)
                (packet_size*image->columns+600);
              (void) BZ2_bzCompress(&bzip_info,BZ_RUN);
              length=bzip_info.next_out-(char *) compress_pixels;
              if (bzip_info.next_out != (char *) compress_pixels)
                (void) WriteBlob(image,length,compress_pixels);
            } while (bzip_info.avail_in != 0);
            if (y == (long) (image->rows-1))
              {
                for ( ; ; )
                {
                  bzip_info.next_out=(char *) compress_pixels;
                  bzip_info.avail_out=(unsigned int)
                    (packet_size*image->columns+600);
                  (void) BZ2_bzCompress(&bzip_info,BZ_FINISH);
                  if (bzip_info.next_out == (char *) compress_pixels)
                    break;
                  length=bzip_info.next_out-(char *) compress_pixels;
                  (void) WriteBlob(image,length,compress_pixels);
                }
                status=!BZ2_bzCompressEnd(&bzip_info);
              }
          }
        else
#endif
          if (compression == RLECompression)
            (void) WriteBlob(image,q-pixels,pixels);
          else
            (void) WriteBlob(image,packet_size*image->columns,pixels);
      if (image->previous == (Image *) NULL)
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(SaveImageText,y,image->rows,&image->exception))
            break;
    }
    LiberateMemory((void **) &pixels);
    LiberateMemory((void **) &compress_pixels);
    if (image->next == (Image *) NULL)
      break;
    image=GetNextImage(image);
    status=MagickMonitor(SaveImagesText,scene++,GetImageListSize(image),
      &image->exception);
    if (status == False)
      break;
  } while (image_info->adjoin);
  if (image_info->adjoin)
    while (image->previous != (Image *) NULL)
      image=image->previous;
  CloseBlob(image);
  return(status);
}
