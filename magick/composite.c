/*
% Copyright (C) 2003 - 2008 GraphicsMagick Group
% Copyright (C) 2002 ImageMagick Studio
%
% This program is covered by multiple licenses, which are described in
% Copyright.txt. You should have received a copy of Copyright.txt with this
% package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        CCCC   OOO  M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE          %
%       C      O   O MM MM  P   P  O   O  SS       I      T    E              %
%       C      O   O M M M  PPPP   O   O   SSS     I      T    EEE            %
%       C      O   O M   M  P      O   O     SS    I      T    E              %
%        CCCC   OOO  M   M  P       OOO   SSSSS  IIIII    T    EEEEE          %
%                                                                             %
%                                                                             %
%                   GraphicsMagick Image Composition Methods                  %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/alpha_composite.h"
#include "magick/composite.h"
#include "magick/gem.h"
#include "magick/pixel_cache.h"
#include "magick/pixel_iterator.h"
#include "magick/utility.h"


/*
  Structure to pass any necessary options to composition callbacks.
*/
typedef struct _CompositePixelsOptions_t
{
  /* Composition operator */
  CompositeOperator compose;

  /* ModulateComposite */
  double            percent_brightness;

  /* ThresholdComposite */
  double            amount;
  double            threshold;
} CompositePixelsOptions_t;


/*
  Build a PixelPacket representing the canvas pixel.
*/
static inline void
PrepareDestinationPacket(PixelPacket *destination,
                         const PixelPacket *update_pixels,
                         const Image *update_image,
                         const IndexPacket *update_indexes,
                         const long i)
{
  *destination=update_pixels[i];
  if (!update_image->matte)
    destination->opacity=OpaqueOpacity;
  else
    if (update_image->colorspace == CMYKColorspace)
      destination->opacity=update_indexes[i];
}


/*
  Build a PixelPacket representing the update pixel.
*/
static inline void
PrepareSourcePacket(PixelPacket *source,
                    const PixelPacket *source_pixels,
                    const Image *source_image,
                    const IndexPacket *source_indexes,
                    const long i)
{
  *source=source_pixels[i];
  if (!source_image->matte)
    source->opacity=OpaqueOpacity;
  else
    if (source_image->colorspace == CMYKColorspace)
      source->opacity=source_indexes[i];
}


/*
  Apply composition updates to the canvas image.
*/
static inline void
ApplyPacketUpdates(PixelPacket *update_pixels,
                   IndexPacket *update_indexes,
                   const Image *update_image,
                   const PixelPacket *composite,
                   const long i
                   )
{
  if (update_image->colorspace != CMYKColorspace)
    {
      /*
        RGB stores opacity in 'opacity'.
      */
      update_pixels[i]=*composite;
    }
  else
    {
      /*
        CMYK(A) stores K in 'opacity' and A in the indexes.
      */
      update_pixels[i].red=composite->red;
      update_pixels[i].green=composite->green;
      update_pixels[i].blue=composite->blue;
      update_indexes[i]=composite->opacity; /* opacity */
    }
}


static MagickPassFail
OverCompositePixels(void *user_data,                   /* User provided mutable data */
                    const Image *source_image,         /* Source image */
                    const PixelPacket *source_pixels,  /* Pixel row in source image */
                    const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                    Image *update_image,               /* Update image */
                    PixelPacket *update_pixels,        /* Pixel row in update image */
                    IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                    const long npixels,                /* Number of pixels in row */
                    ExceptionInfo *exception           /* Exception report */
                    )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result will be the union of the two image shapes, with
    opaque areas of change-image obscuring base-image in the
    region of overlap.
  */
  for (i=0; i < npixels; i++)
    {
      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      destination=AlphaComposite(&source,source.opacity,&destination,destination.opacity);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
InCompositePixels(void *user_data,                   /* User provided mutable data */
                  const Image *source_image,         /* Source image */
                  const PixelPacket *source_pixels,  /* Pixel row in source image */
                  const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                  Image *update_image,               /* Update image */
                  PixelPacket *update_pixels,        /* Pixel row in update image */
                  IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                  const long npixels,                /* Number of pixels in row */
                  ExceptionInfo *exception           /* Exception report */
                  )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result is simply change-image cut by the shape of
    base-image. None of the image data of base-image will be
    in the result.
  */
  for (i=0; i < npixels; i++)
    {
      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else if (destination.opacity == TransparentOpacity)
        {
        }
      else
        {
          double
            opacity;

          opacity=(double)
            (((double) MaxRGBDouble-source.opacity)*
             (MaxRGBDouble-destination.opacity)/MaxRGBDouble);

          destination.red=(Quantum)
            (((double) MaxRGBDouble-source.opacity)*
             (MaxRGBDouble-destination.opacity)*source.red/MaxRGBDouble/opacity+0.5);

          destination.green=(Quantum)
            (((double) MaxRGBDouble-source.opacity)*
             (MaxRGBDouble-destination.opacity)*source.green/MaxRGBDouble/opacity+0.5);

          destination.blue=(Quantum)
            (((double) MaxRGBDouble-source.opacity)*
             (MaxRGBDouble-destination.opacity)*source.blue/MaxRGBDouble/opacity+0.5);

          destination.opacity=(Quantum) (MaxRGBDouble-opacity+0.5);
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
OutCompositePixels(void *user_data,                   /* User provided mutable data */
                   const Image *source_image,         /* Source image */
                   const PixelPacket *source_pixels,  /* Pixel row in source image */
                   const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                   Image *update_image,               /* Update image */
                   PixelPacket *update_pixels,        /* Pixel row in update image */
                   IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                   const long npixels,                /* Number of pixels in row */
                   ExceptionInfo *exception           /* Exception report */
                   )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The resulting image is change-image with the shape of
    base-image cut out.
  */
  for (i=0; i < npixels; i++)
    {
      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else if (destination.opacity == OpaqueOpacity)
        {
          destination.opacity=TransparentOpacity;
        }
      else
        {
          double
            opacity;

          opacity=(double)
            (MaxRGBDouble-source.opacity)*destination.opacity/MaxRGBDouble;

          destination.red=(Quantum)
            (((double) MaxRGBDouble-source.opacity)*
             destination.opacity*source.red/MaxRGBDouble/opacity+0.5);

          destination.green=(Quantum)
            (((double) MaxRGBDouble-source.opacity)*
             destination.opacity*source.green/MaxRGBDouble/opacity+0.5);

          destination.blue=(Quantum)
            (((double) MaxRGBDouble-source.opacity)*
             destination.opacity*source.blue/MaxRGBDouble/opacity+0.5);

          destination.opacity=(Quantum) (MaxRGBDouble-opacity+0.5);
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}

static MagickPassFail
AtopCompositePixels(void *user_data,                   /* User provided mutable data */
                    const Image *source_image,         /* Source image */
                    const PixelPacket *source_pixels,  /* Pixel row in source image */
                    const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                    Image *update_image,               /* Update image */
                    PixelPacket *update_pixels,        /* Pixel row in update image */
                    IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                    const long npixels,                /* Number of pixels in row */
                    ExceptionInfo *exception           /* Exception report */
                    )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result is the same shape as base-image, with
    change-image obscuring base-image where the image shapes
    overlap. Note this differs from over because the portion
    of change-image outside base-image's shape does not appear
    in the result.
  */
  for (i=0; i < npixels; i++)
    {
      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      destination=AtopComposite(&destination,&source);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
XorCompositePixels(void *user_data,                   /* User provided mutable data */
                   const Image *source_image,         /* Source image */
                   const PixelPacket *source_pixels,  /* Pixel row in source image */
                   const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                   Image *update_image,               /* Update image */
                   PixelPacket *update_pixels,        /* Pixel row in update image */
                   IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                   const long npixels,                /* Number of pixels in row */
                   ExceptionInfo *exception           /* Exception report */
                   )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result is the image data from both change-image and
    base-image that is outside the overlap region. The overlap
    region will be blank.
  */
  for (i=0; i < npixels; i++)
    {
      double gamma;
      double source_alpha;
      double dest_alpha;
      double composite;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      source_alpha=(double) source.opacity/MaxRGBDouble;
      dest_alpha=(double) destination.opacity/MaxRGBDouble;
          
      gamma=(1.0-source_alpha)+(1.0-dest_alpha)-
        2.0*(1.0-source_alpha)*(1.0-dest_alpha);
          
      composite=MaxRGBDouble*(1.0-gamma);
      destination.opacity=RoundDoubleToQuantum(composite);
          
      gamma=1.0/(gamma <= MagickEpsilon ? 1.0 : gamma);
          
      composite=((1.0-source_alpha)*source.red*dest_alpha+
                 (1.0-dest_alpha)*destination.red*source_alpha)*gamma;
      destination.red=RoundDoubleToQuantum(composite);
          
      composite=((1.0-source_alpha)*source.green*dest_alpha+
                 (1.0-dest_alpha)*destination.green*source_alpha)*gamma;
      destination.green=RoundDoubleToQuantum(composite);
          
      composite=((1.0-source_alpha)*source.blue*dest_alpha+
                 (1.0-dest_alpha)*destination.blue*source_alpha)*gamma;
      destination.blue=RoundDoubleToQuantum(composite);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
PlusCompositePixels(void *user_data,                   /* User provided mutable data */
                    const Image *source_image,         /* Source image */
                    const PixelPacket *source_pixels,  /* Pixel row in source image */
                    const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                    Image *update_image,               /* Update image */
                    PixelPacket *update_pixels,        /* Pixel row in update image */
                    IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                    const long npixels,                /* Number of pixels in row */
                    ExceptionInfo *exception           /* Exception report */
                    )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result is just the sum of the image data. Output values are
    cropped to MaxRGB (no overflow). This operation is independent of
    the matte channels.
  */
  for (i=0; i < npixels; i++)
    {
      double
        value;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      value=((double) (MaxRGBDouble-source.opacity)*source.red+(double)
                 (MaxRGBDouble-destination.opacity)*destination.red)/MaxRGBDouble;
      destination.red=RoundDoubleToQuantum(value);
      
      value=((double) (MaxRGBDouble-source.opacity)*source.green+(double)
                   (MaxRGBDouble-destination.opacity)*destination.green)/MaxRGBDouble;
      destination.green=RoundDoubleToQuantum(value);
      
      value=((double) (MaxRGBDouble-source.opacity)*source.blue+(double)
                  (MaxRGBDouble-destination.opacity)*destination.blue)/MaxRGBDouble;
      destination.blue=RoundDoubleToQuantum(value);
      
      value=((double) (MaxRGBDouble-source.opacity)+
                     (double) (MaxRGBDouble-destination.opacity))/MaxRGBDouble;
      destination.opacity=MaxRGB-RoundDoubleToQuantum(value);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
MinusCompositePixels(void *user_data,                   /* User provided mutable data */
                     const Image *source_image,         /* Source image */
                     const PixelPacket *source_pixels,  /* Pixel row in source image */
                     const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                     Image *update_image,               /* Update image */
                     PixelPacket *update_pixels,        /* Pixel row in update image */
                     IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                     const long npixels,                /* Number of pixels in row */
                     ExceptionInfo *exception           /* Exception report */
                     )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result of change-image - base-image, with underflow cropped to
    zero. The matte channel is ignored (set to opaque, full coverage).
  */
  for (i=0; i < npixels; i++)
    {
      double
        value;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      value=((double) (MaxRGBDouble-destination.opacity)*destination.red-
             (double) (MaxRGBDouble-source.opacity)*source.red)/MaxRGBDouble;
      destination.red=RoundDoubleToQuantum(value);

      value=((double) (MaxRGBDouble-destination.opacity)*destination.green-
             (double) (MaxRGBDouble-source.opacity)*source.green)/MaxRGBDouble;
      destination.green=RoundDoubleToQuantum(value);

      value=((double) (MaxRGBDouble-destination.opacity)*destination.blue-
             (double) (MaxRGBDouble-source.opacity)*source.blue)/MaxRGBDouble;
      destination.blue=RoundDoubleToQuantum(value);

      value=((double) (MaxRGBDouble-destination.opacity)-
             (double) (MaxRGBDouble-source.opacity))/MaxRGBDouble;
      destination.opacity=MaxRGB-RoundDoubleToQuantum(value);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
AddCompositePixels(void *user_data,                   /* User provided mutable data */
                   const Image *source_image,         /* Source image */
                   const PixelPacket *source_pixels,  /* Pixel row in source image */
                   const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                   Image *update_image,               /* Update image */
                   PixelPacket *update_pixels,        /* Pixel row in update image */
                   IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                   const long npixels,                /* Number of pixels in row */
                   ExceptionInfo *exception           /* Exception report */
                   )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result of change-image + base-image, with overflow wrapping
    around (mod MaxRGB+1).
  */
  for (i=0; i < npixels; i++)
    {
      double
        value;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      value=(double) source.red+destination.red;
      if (value > MaxRGBDouble) value -= ((double) MaxRGBDouble+1.0);
      destination.red=RoundDoubleToQuantum(value);
      
      value=(double) source.green+destination.green;
      if (value > MaxRGBDouble) value -= ((double) MaxRGBDouble+1.0);
      destination.green=RoundDoubleToQuantum(value);
      
      value=(double) source.blue+destination.blue;
      if (value > MaxRGBDouble) value -= ((double) MaxRGBDouble+1.0);
      destination.blue=RoundDoubleToQuantum(value);
      
      destination.opacity=OpaqueOpacity;
      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
SubtractCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result of change-image - base-image, with underflow wrapping
    around (mod MaxRGB+1). The add and subtract operators can be used
    to perform reversible transformations.
  */
  for (i=0; i < npixels; i++)
    {
      double
        value;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      value=(double) source.red-destination.red;
      if (value < 0) value += ((double) MaxRGBDouble+1.0);
      destination.red=RoundDoubleToQuantum(value);

      value=(double) source.green-destination.green;
      if (value < 0) value += ((double) MaxRGBDouble+1.0);
      destination.green=RoundDoubleToQuantum(value);

      value=(double) source.blue-destination.blue;
      if (value < 0) value += ((double) MaxRGBDouble+1.0);
      destination.blue=RoundDoubleToQuantum(value);

      destination.opacity=OpaqueOpacity;

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
DifferenceCompositePixels(void *user_data,                   /* User provided mutable data */
                          const Image *source_image,         /* Source image */
                          const PixelPacket *source_pixels,  /* Pixel row in source image */
                          const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                          Image *update_image,               /* Update image */
                          PixelPacket *update_pixels,        /* Pixel row in update image */
                          IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                          const long npixels,                /* Number of pixels in row */
                          ExceptionInfo *exception           /* Exception report */
                          )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result of abs(change-image - base-image). This is useful for
    comparing two very similar images.
  */
  for (i=0; i < npixels; i++)
    {
      double
        value;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      value=source.red-(double) destination.red;
      destination.red=(Quantum) AbsoluteValue(value);

      value=source.green-(double) destination.green;
      destination.green=(Quantum) AbsoluteValue(value);

      value=source.blue-(double) destination.blue;
      destination.blue=(Quantum) AbsoluteValue(value);

      value=source.opacity-(double) destination.opacity;
      destination.opacity=(Quantum) AbsoluteValue(value);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
MultiplyCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result of change-image * base-image. This is useful for the
    creation of drop-shadows.
  */
  for (i=0; i < npixels; i++)
    {
      double
        value;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      value=((double) source.red*destination.red)/MaxRGBDouble;
      destination.red=RoundDoubleToQuantum(value);

      value=((double) source.green*destination.green)/MaxRGBDouble;
      destination.green=RoundDoubleToQuantum(value);

      value=((double) source.blue*destination.blue)/MaxRGBDouble;
      destination.blue=RoundDoubleToQuantum(value);

      value=((double) source.opacity*destination.opacity)/MaxRGBDouble;
      destination.opacity=RoundDoubleToQuantum(value);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
BumpmapCompositePixels(void *user_data,                   /* User provided mutable data */
                       const Image *source_image,         /* Source image */
                       const PixelPacket *source_pixels,  /* Pixel row in source image */
                       const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                       Image *update_image,               /* Update image */
                       PixelPacket *update_pixels,        /* Pixel row in update image */
                       IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                       const long npixels,                /* Number of pixels in row */
                       ExceptionInfo *exception           /* Exception report */
                       )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The result base-image shaded by change-image.
  */
  for (i=0; i < npixels; i++)
    {
      double value;
      double source_intensity;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      source_intensity=(double) PixelIntensity(&source)/MaxRGBDouble;

      value=source_intensity*destination.red;
      destination.red=RoundDoubleToQuantum(value);

      value=source_intensity*destination.green;
      destination.green=RoundDoubleToQuantum(value);

      value=source_intensity*destination.blue;
      destination.blue=RoundDoubleToQuantum(value);

      value=source_intensity*destination.opacity;
      destination.opacity=RoundDoubleToQuantum(value);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}




static MagickPassFail
CopyCompositePixels(void *user_data,                   /* User provided mutable data */
                    const Image *source_image,         /* Source image */
                    const PixelPacket *source_pixels,  /* Pixel row in source image */
                    const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                    Image *update_image,               /* Update image */
                    PixelPacket *update_pixels,        /* Pixel row in update image */
                    IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                    const long npixels,                /* Number of pixels in row */
                    ExceptionInfo *exception           /* Exception report */
                    )
{
  register long
    i;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The resulting image is base-image replaced with change-image. Here
    the matte information is ignored.
  */
  if ((update_image->colorspace == CMYKColorspace) &&
      (update_image->matte))
    {
      if (source_image->matte)
        {
          for (i=0; i < npixels; i++)
            {
              update_pixels[i]  = source_pixels[i];
              update_indexes[i] = source_indexes[i];
            }
        }
      else
        {
          for (i=0; i < npixels; i++)
            {
              update_pixels[i]  = source_pixels[i];
              update_indexes[i] = OpaqueOpacity;
            }
        }
    }
  else
    {
      for (i=0; i < npixels; i++)
        {
          update_pixels[i] = source_pixels[i];
        }
    }
      
  return MagickPass;
}

static MagickPassFail
CopyRedCompositePixels(void *user_data,                   /* User provided mutable data */
                       const Image *source_image,         /* Source image */
                       const PixelPacket *source_pixels,  /* Pixel row in source image */
                       const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                       Image *update_image,               /* Update image */
                       PixelPacket *update_pixels,        /* Pixel row in update image */
                       IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                       const long npixels,                /* Number of pixels in row */
                       ExceptionInfo *exception           /* Exception report */
                       )
{
  register long
    i;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(source_image);
  ARG_NOT_USED(source_indexes);
  ARG_NOT_USED(update_image);
  ARG_NOT_USED(update_indexes);
  ARG_NOT_USED(exception);

  /*
    The resulting image is the red channel in base-image replaced with
    the red channel in change-image. The other channels are copied
    untouched.
  */
  for (i=0; i < npixels; i++)
    {
      update_pixels[i].red = source_pixels[i].red;
    }
      
  return MagickPass;
}

static MagickPassFail
CopyGreenCompositePixels(void *user_data,                   /* User provided mutable data */
                         const Image *source_image,         /* Source image */
                         const PixelPacket *source_pixels,  /* Pixel row in source image */
                         const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                         Image *update_image,               /* Update image */
                         PixelPacket *update_pixels,        /* Pixel row in update image */
                         IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                         const long npixels,                /* Number of pixels in row */
                         ExceptionInfo *exception           /* Exception report */
                         )
{
  register long
    i;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(source_image);
  ARG_NOT_USED(source_indexes);
  ARG_NOT_USED(update_image);
  ARG_NOT_USED(update_indexes);
  ARG_NOT_USED(exception);

  /*
    The resulting image is the green channel in base-image replaced
    with the green channel in change-image. The other channels are
    copied untouched.
  */
  for (i=0; i < npixels; i++)
    {
      update_pixels[i].green = source_pixels[i].green;
    }
      
  return MagickPass;
}


static MagickPassFail
CopyBlueCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  register long
    i;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(source_image);
  ARG_NOT_USED(source_indexes);
  ARG_NOT_USED(update_image);
  ARG_NOT_USED(update_indexes);
  ARG_NOT_USED(exception);

  /*
    The resulting image is the blue channel in base-image replaced
    with the blue channel in change-image. The other channels are
    copied untouched.
  */
  for (i=0; i < npixels; i++)
    {
      update_pixels[i].blue = source_pixels[i].blue;
    }

  return MagickPass;
}

static MagickPassFail
CopyOpacityCompositePixels(void *user_data,                   /* User provided mutable data */
                           const Image *source_image,         /* Source image */
                           const PixelPacket *source_pixels,  /* Pixel row in source image */
                           const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                           Image *update_image,               /* Update image */
                           PixelPacket *update_pixels,        /* Pixel row in update image */
                           IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                           const long npixels,                /* Number of pixels in row */
                           ExceptionInfo *exception           /* Exception report */
                           )
{
  register long
    i;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  /*
    The resulting image is the opacity channel in base-image replaced
    with the opacity channel in change-image.  The other channels are
    copied untouched.
  */
  if (update_image->colorspace == CMYKColorspace)
    {
      if (!source_image->matte)
        {
          for (i=0; i < npixels; i++)
            {
              update_indexes[i] =
                (Quantum) (MaxRGB-PixelIntensityToQuantum(&source_pixels[i]));
            }
        }
      else
        {
          for (i=0; i < npixels; i++)
            {
              update_indexes[i] = source_indexes[i];
            }
        }
    }
  else
    {
      if (!source_image->matte)
        {
          for (i=0; i < npixels; i++)
            {
              update_pixels[i].opacity =
                (Quantum) (MaxRGB-PixelIntensityToQuantum(&source_pixels[i]));
            }
        }
      else
        {
          for (i=0; i < npixels; i++)
            {
              update_pixels[i].opacity = source_pixels[i].opacity;
            }
        }
    }
      
  return MagickPass;
}

static MagickPassFail
ClearCompositePixels(void *user_data,                   /* User provided mutable data */
                     const Image *source_image,         /* Source image */
                     const PixelPacket *source_pixels,  /* Pixel row in source image */
                     const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                     Image *update_image,               /* Update image */
                     PixelPacket *update_pixels,        /* Pixel row in update image */
                     IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                     const long npixels,                /* Number of pixels in row */
                     ExceptionInfo *exception           /* Exception report */
                     )
{
  register long
    i;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(source_image);
  ARG_NOT_USED(source_pixels);
  ARG_NOT_USED(source_indexes);
  ARG_NOT_USED(exception);

  /*
    Set destination pixels to transparent.
  */
  if (update_image->colorspace == CMYKColorspace)
    {
      update_image->matte=MagickTrue;
      for (i=0; i < npixels; i++)
        {
          update_indexes[i] = TransparentOpacity;
        }
    }
  else
    {
      for (i=0; i < npixels; i++)
        {
          update_pixels[i].opacity = TransparentOpacity;
        }
    }
      
  return MagickPass;
}



static MagickPassFail
DissolveCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {
      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      destination.red=(Quantum)
        (((double) source.opacity*source.red+
          (MaxRGBDouble-source.opacity)*destination.red)/MaxRGBDouble+0.5);
      destination.green=(Quantum)
        (((double) source.opacity*source.green+
          (MaxRGBDouble-source.opacity)*destination.green)/MaxRGBDouble+0.5);
      destination.blue=(Quantum)
        (((double) source.opacity*source.blue+
          (MaxRGBDouble-source.opacity)*destination.blue)/MaxRGBDouble+0.5);
      destination.opacity=OpaqueOpacity;

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
ModulateCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  const CompositePixelsOptions_t
    *options = (const CompositePixelsOptions_t *) user_data;

  const double
    percent_brightness = options->percent_brightness;


  double
    midpoint;

  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(exception);

  midpoint=((double) MaxRGB+1.0)/2;
  for (i=0; i < npixels; i++)
    {
      double
        offset;

      double
        brightness,
        hue,
        saturation;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      offset=(long) (PixelIntensityToQuantum(&source)-midpoint);
      if (offset == 0)
        break;
      TransformHSL(destination.red,destination.green,destination.blue,
                   &hue,&saturation,&brightness);
      brightness+=(percent_brightness*offset)/midpoint;
      if (brightness < 0.0)
        brightness=0.0;
      else
        if (brightness > 1.0)
          brightness=1.0;
      HSLTransform(hue,saturation,brightness,&destination.red,
                   &destination.green,&destination.blue);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
ThresholdCompositePixels(void *user_data,                   /* User provided mutable data */
                         const Image *source_image,         /* Source image */
                         const PixelPacket *source_pixels,  /* Pixel row in source image */
                         const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                         Image *update_image,               /* Update image */
                         PixelPacket *update_pixels,        /* Pixel row in update image */
                         IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                         const long npixels,                /* Number of pixels in row */
                         ExceptionInfo *exception           /* Exception report */
                         )
{
  const CompositePixelsOptions_t
    *options = (const CompositePixelsOptions_t *) user_data;

  const double
    amount = options->amount,
    threshold = options->threshold;

  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {
      double
        value;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      value=destination.red-(double) source.red;
      if (fabs(2.0*value) < threshold)
        value=destination.red;
      else
        value=destination.red+(value*amount);
      destination.red=RoundDoubleToQuantum(value);

      value=destination.green-(double) source.green;
      if (fabs(2.0*value) < threshold)
        value=destination.green;
      else
        value=destination.green+(value*amount);
      destination.green=RoundDoubleToQuantum(value);

      value=destination.blue-(double) source.blue;
      if (fabs(2.0*value) < threshold)
        value=destination.blue;
      else
        value=destination.blue+(value*amount);
      destination.blue=RoundDoubleToQuantum(value);

      value=destination.opacity-(double) source.opacity;
      if (fabs(2.0*value) < threshold)
        value=destination.opacity;
      else
        value=destination.opacity+(value*amount);
      destination.opacity=RoundDoubleToQuantum(value);

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
DarkenCompositePixels(void *user_data,                   /* User provided mutable data */
                      const Image *source_image,         /* Source image */
                      const PixelPacket *source_pixels,  /* Pixel row in source image */
                      const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                      Image *update_image,               /* Update image */
                      PixelPacket *update_pixels,        /* Pixel row in update image */
                      IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                      const long npixels,                /* Number of pixels in row */
                      ExceptionInfo *exception           /* Exception report */
                      )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
        }
      else if (destination.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else
        {
          if (source.red < destination.red)
            destination.red=source.red;
          if (source.green < destination.green)
            destination.green=source.green;
          if (source.blue < destination.blue)
            destination.blue=source.blue;
          if (source.opacity < destination.opacity)
            destination.opacity=source.opacity;
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
LightenCompositePixels(void *user_data,                   /* User provided mutable data */
                       const Image *source_image,         /* Source image */
                       const PixelPacket *source_pixels,  /* Pixel row in source image */
                       const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                       Image *update_image,               /* Update image */
                       PixelPacket *update_pixels,        /* Pixel row in update image */
                       IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                       const long npixels,                /* Number of pixels in row */
                       ExceptionInfo *exception           /* Exception report */
                       )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
        }
      else if (destination.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else
        {
          if (source.red > destination.red)
            destination.red=source.red;
          if (source.green > destination.green)
            destination.green=source.green;
          if (source.blue > destination.blue)
            destination.blue=source.blue;
          if (source.opacity > destination.opacity)
            destination.opacity=source.opacity;
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
HueCompositePixels(void *user_data,                   /* User provided mutable data */
                   const Image *source_image,         /* Source image */
                   const PixelPacket *source_pixels,  /* Pixel row in source image */
                   const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                   Image *update_image,               /* Update image */
                   PixelPacket *update_pixels,        /* Pixel row in update image */
                   IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                   const long npixels,                /* Number of pixels in row */
                   ExceptionInfo *exception           /* Exception report */
                   )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {
      double
        brightness,
        hue,
        saturation,
        sans;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
        }
      else if (destination.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else
        {
          TransformHSL(destination.red,destination.green,destination.blue,
                       &hue,&saturation,&brightness);
          TransformHSL(source.red,source.green,source.blue,&hue,&sans,&sans);
          HSLTransform(hue,saturation,brightness,&destination.red,
                       &destination.green,&destination.blue);
          if (source.opacity < destination.opacity)
            destination.opacity=source.opacity;
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
SaturateCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {
      double
        brightness,
        hue,
        saturation,
        sans;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
        }
      else if (destination.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else
        {
          TransformHSL(destination.red,destination.green,destination.blue,
                       &hue,&saturation,&brightness);
          TransformHSL(source.red,source.green,source.blue,&sans,&saturation,
                       &sans);
          HSLTransform(hue,saturation,brightness,&destination.red,
                       &destination.green,&destination.blue);
          if (source.opacity < destination.opacity)
            destination.opacity=source.opacity;
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
ColorizeCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {
      double
        brightness,
        hue,
        saturation,
        sans;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
        }
      else if (destination.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else
        {
          TransformHSL(destination.red,destination.green,destination.blue,
                       &sans,&sans,&brightness);
          TransformHSL(source.red,source.green,source.blue,&hue,&saturation,
                       &sans);
          HSLTransform(hue,saturation,brightness,&destination.red,
                       &destination.green,&destination.blue);
          if (source.opacity < destination.opacity)
            destination.opacity=source.opacity;
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
LuminizeCompositePixels(void *user_data,                   /* User provided mutable data */
                        const Image *source_image,         /* Source image */
                        const PixelPacket *source_pixels,  /* Pixel row in source image */
                        const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                        Image *update_image,               /* Update image */
                        PixelPacket *update_pixels,        /* Pixel row in update image */
                        IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                        const long npixels,                /* Number of pixels in row */
                        ExceptionInfo *exception           /* Exception report */
                        )
{
  register long
    i;

  PixelPacket
    destination,
    source;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(exception);

  for (i=0; i < npixels; i++)
    {
      double
        brightness,
        hue,
        saturation,
        sans;

      PrepareSourcePacket(&source,source_pixels,source_image,source_indexes,i);
      PrepareDestinationPacket(&destination,update_pixels,update_image,update_indexes,i);

      if (source.opacity == TransparentOpacity)
        {
        }
      else if (destination.opacity == TransparentOpacity)
        {
          destination=source;
        }
      else
        {
          TransformHSL(destination.red,destination.green,destination.blue,
                       &hue,&saturation,&brightness);
          TransformHSL(source.red,source.green,source.blue,&sans,&sans,
                       &brightness);
          HSLTransform(hue,saturation,brightness,&destination.red,
                       &destination.green,&destination.blue);
          if (source.opacity < destination.opacity)
            destination.opacity=source.opacity;
        }

      ApplyPacketUpdates(update_pixels,update_indexes,update_image,&destination,i);
    }

  return MagickPass;
}


static MagickPassFail
CopyBlackCompositePixels(void *user_data,                   /* User provided mutable data */
                         const Image *source_image,         /* Source image */
                         const PixelPacket *source_pixels,  /* Pixel row in source image */
                         const IndexPacket *source_indexes, /* Pixel row indexes in source image */
                         Image *update_image,               /* Update image */
                         PixelPacket *update_pixels,        /* Pixel row in update image */
                         IndexPacket *update_indexes,       /* Pixel row indexes in update image */
                         const long npixels,                /* Number of pixels in row */
                         ExceptionInfo *exception           /* Exception report */
                         )
{
  register long
    i;

  ARG_NOT_USED(user_data);
  ARG_NOT_USED(source_indexes);
  ARG_NOT_USED(update_indexes);
  ARG_NOT_USED(exception);

  /*
    Copy the CMYK Black (K) channel into the image.
  */
  if ((update_image->colorspace == CMYKColorspace) &&
      (source_image->colorspace == CMYKColorspace))
    {
      for (i=0; i < npixels; i++)
        {
          update_pixels[i].opacity=source_pixels[i].opacity;
        }
    }
  else
    {
      for (i=0; i < npixels; i++)
        {
          update_pixels[i].opacity = PixelIntensityToQuantum(&source_pixels[i]);
        }
    }

  return MagickPass;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImage() returns the second image (composite_image) composited
%  onto the first (canvas_image) at the specified offsets.
%
%  The format of the CompositeImage method is:
%
%      unsigned int CompositeImage(Image *canvas_image,
%        const CompositeOperator compose,const Image *composite_image,
%        const long x_offset,const long y_offset)
%
%  A description of each parameter follows:
%
%    o canvas_image: The image to be updated.
%
%    o compose: This operator affects how the composite is applied to
%      the image.  The default is Over.  Choose from one of these
%      operators: OverCompositeOp, InCompositeOp, OutCompositeOP,
%      AtopCompositeOP, XorCompositeOP, PlusCompositeOP, MinusCompositeOP,
%      AddCompositeOP, SubtractCompositeOP, DifferenceCompositeOP,
%      BumpmapCompositeOP, CopyCompositeOP, CopyRedCompositeOP,
%      CopyGreenCompositeOP, CopyBlueCompositeOP, CopyOpacityCompositeOP.
%
%    o composite_image: The composite image.
%
%    o x_offset: The column offset of the composited image.
%
%    o y_offset: The row offset of the composited image.
%
%
*/

MagickExport MagickPassFail CompositeImage(Image *canvas_image,
                                           const CompositeOperator compose,const Image *composite_image,
                                           const long x_offset,const long y_offset)
{
  CompositePixelsOptions_t
    options;

  Image
    *change_image;

  double
    amount,
    percent_brightness,
    percent_saturation,
    threshold;

  long
    y;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  MagickPassFail
    status=MagickPass;

  /*
    Prepare composite image.
  */
  assert(canvas_image != (Image *) NULL);
  assert(canvas_image->signature == MagickSignature);
  assert(composite_image != (Image *) NULL);
  assert(composite_image->signature == MagickSignature);
  if (compose == NoCompositeOp)
    return(MagickPass);

  /*
    Clone composite image so that we can modify it if need be.
  */
  change_image=CloneImage(composite_image,0,0,True,&canvas_image->exception);
  if (change_image == (Image *) NULL)
    return(MagickFail);

  canvas_image->storage_class=DirectClass;
  switch (compose)
    {
    case CopyCyanCompositeOp:
    case CopyMagentaCompositeOp:
    case CopyYellowCompositeOp:
    case CopyBlackCompositeOp:
      {
        canvas_image->colorspace=CMYKColorspace;
        break;
      }
    case CopyOpacityCompositeOp:
      {
        canvas_image->matte=MagickTrue;
        break;
      }
    case DisplaceCompositeOp:
      {
        double
          x_displace,
          y_displace;

        double
          horizontal_scale,
          vertical_scale;

        register PixelPacket
          *r;

        horizontal_scale=20.0;
        vertical_scale=20.0;
        if (composite_image->geometry != (char *) NULL)
          {
            int
              count;

            /*
              Determine the horizontal and vertical displacement scale.
            */
            count=GetMagickDimension(composite_image->geometry,
                                     &horizontal_scale,&vertical_scale);
            if (count == 1)
              vertical_scale=horizontal_scale;
          }
        /*
          Shift image pixels as defined by a displacement map.
        */
        for (y=0; y < (long) composite_image->rows; y++)
          {
            if (((y+y_offset) < 0) || ((y+y_offset) >= (long) canvas_image->rows))
              continue;
            p=AcquireImagePixels(composite_image,0,y,composite_image->columns,1,
                                 &canvas_image->exception);
            q=GetImagePixels(canvas_image,0,y+y_offset,canvas_image->columns,1);
            r=GetImagePixels(change_image,0,y,change_image->columns,1);
            if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL) ||
                (r == (PixelPacket *) NULL))
              {
                status=MagickFail;
                break;
              }
            q+=x_offset;
            for (x=0; x < (long) composite_image->columns; x++)
              {
                if (((x_offset+x) < 0) || ((x_offset+x) >= (long) canvas_image->columns))
                  {
                    p++;
                    q++;
                    continue;
                  }
                x_displace=(horizontal_scale*(PixelIntensityToQuantum(p)-
                                              (((double) MaxRGB+1.0)/2)))/(((double) MaxRGB+1.0)/2);
                y_displace=x_displace;
                if (composite_image->matte)
                  y_displace=(vertical_scale*(p->opacity-
                                              (((double) MaxRGB+1.0)/2)))/(((double) MaxRGB+1.0)/2);
                *r=InterpolateColor(canvas_image,x_offset+x+x_displace,y_offset+y+y_displace,
                                    &canvas_image->exception);
                p++;
                q++;
                r++;
              }
            if (!SyncImagePixels(change_image))
              {
                status=MagickFail;
                break;
              }
          }
        break;
      }
    case ModulateCompositeOp:
      {
        percent_saturation=50.0;
        percent_brightness=50.0;
        if (composite_image->geometry != (char *) NULL)
          {
            int
              count;

            /*
              Determine the brightness and saturation scale.
            */
            count=GetMagickDimension(composite_image->geometry,
                                     &percent_brightness,&percent_saturation);
            if (count == 1)
              percent_saturation=percent_brightness;
          }
        percent_brightness/=100.0;
        percent_saturation/=100.0;
        break;
      }
    case ThresholdCompositeOp:
      {
        /*
          Determine the amount and threshold.
        */
        amount=0.5;
        threshold=0.05;
        if (composite_image->geometry != (char *) NULL)
          (void) GetMagickDimension(composite_image->geometry,&amount,&threshold);
        threshold*=MaxRGB;
        break;
      }
    default:
      break;
    }

  /*
    Make sure that the composite image is in a colorspace which is
    compatible (as need be) with the canvas image.
  */
  switch (compose)
    {
    case CopyRedCompositeOp:
    case CopyGreenCompositeOp:
    case CopyBlueCompositeOp:
    case CopyCyanCompositeOp:
    case CopyMagentaCompositeOp:
    case CopyYellowCompositeOp:
    case CopyBlackCompositeOp:
      {
        /*
          Assume that the user is right for channel copies.
        */
        break;
      }
    default:
      {
        if (IsRGBColorspace(canvas_image->colorspace))
          {
            if (!IsRGBColorspace(change_image->colorspace))
              TransformColorspace(change_image,RGBColorspace);
          }
        else if (IsYCbCrColorspace(canvas_image->colorspace))
          {
            if (canvas_image->colorspace != change_image->colorspace)
              TransformColorspace(change_image,canvas_image->colorspace);
          }
        else if (IsCMYKColorspace(canvas_image->colorspace))
          {
            if (!IsCMYKColorspace(change_image->colorspace))
              TransformColorspace(change_image,canvas_image->colorspace);
          }
        else
          {
            TransformColorspace(change_image,canvas_image->colorspace);
          }
        break;
      }
    }

  /*
    Composite image.
  */
  options.compose=compose;
  options.percent_brightness=percent_brightness;
  options.amount=amount;
  options.threshold=threshold;

  {
    unsigned long
      columns,
      rows;

    long
      composite_x,
      composite_y,
      canvas_x,
      canvas_y;

    columns=change_image->columns;
    rows=change_image->rows;

    composite_x=0;
    composite_y=0;
    canvas_x=x_offset;
    canvas_y=y_offset;

    if (x_offset < 0)
      composite_x += -x_offset;
    if (y_offset < 0)
      composite_y += -y_offset;

    columns -= composite_x;
    rows -= composite_y;

    if (canvas_x < 0)
      canvas_x=0;
    if (canvas_y < 0)
      canvas_y=0;

#if 0
    printf("canvas=%lux%lu composite=%lux%lu offset=%ldx%ld | canvas=%ldx%ld composite=%ldx%ld size=%ldx%ld\n",
           canvas_image->columns,canvas_image->rows,
           change_image->columns,change_image->rows,
           x_offset,y_offset,
           canvas_x,canvas_y,
           composite_x,composite_y,
           columns,rows);
#endif

    if (((unsigned long) canvas_x < canvas_image->columns) &&
        ((unsigned long) canvas_y < canvas_image->rows) &&
        ((unsigned long) composite_x < change_image->columns) &&
        ((unsigned long) composite_y < change_image->rows))
      {
        PixelIteratorDualModifyCallback
          call_back = (PixelIteratorDualModifyCallback) NULL;

        if ((canvas_x + change_image->columns) > canvas_image->columns)
          columns -= ((canvas_x + change_image->columns) - canvas_image->columns);
        if ((canvas_y + change_image->rows) > canvas_image->rows)
          rows -= ((canvas_y + change_image->rows) - canvas_image->rows);

        switch (compose)
          {
          case UndefinedCompositeOp:
            /* Does nothing */
            break;
          case OverCompositeOp:
            call_back=OverCompositePixels;
            break;
          case InCompositeOp:
            call_back=InCompositePixels;
            break;
          case OutCompositeOp:
            call_back=OutCompositePixels;
            break;
          case AtopCompositeOp:
            call_back=AtopCompositePixels;
            break;
          case XorCompositeOp:
            call_back=XorCompositePixels;
            break;
          case PlusCompositeOp:
            call_back=PlusCompositePixels;
            break;
          case MinusCompositeOp:
            call_back=MinusCompositePixels;
            break;
          case AddCompositeOp:
            call_back=AddCompositePixels;
            break;
          case SubtractCompositeOp:
            call_back=SubtractCompositePixels;
            break;
          case DifferenceCompositeOp:
            call_back=DifferenceCompositePixels;
            break;
          case MultiplyCompositeOp:
            call_back=MultiplyCompositePixels;
            break;
          case BumpmapCompositeOp:
            call_back=BumpmapCompositePixels;
            break;
          case CopyCompositeOp:
            call_back=CopyCompositePixels;
            break;
          case CopyRedCompositeOp:
            call_back=CopyRedCompositePixels;
            break;
          case CopyGreenCompositeOp:
            call_back=CopyGreenCompositePixels;
            break;
          case CopyBlueCompositeOp:
            call_back=CopyBlueCompositePixels;
            break;
          case CopyOpacityCompositeOp:
            call_back=CopyOpacityCompositePixels;
            break;
          case ClearCompositeOp:
            call_back=ClearCompositePixels;
            break;
          case DissolveCompositeOp:
            call_back=DissolveCompositePixels;
            break;
          case DisplaceCompositeOp:
            call_back=CopyCompositePixels;
            break;
          case ModulateCompositeOp:
            call_back=ModulateCompositePixels;
            break;
          case ThresholdCompositeOp:
            call_back=ThresholdCompositePixels;
            break;
          case NoCompositeOp:
            break;
          case DarkenCompositeOp:
            call_back=DarkenCompositePixels;
            break;
          case LightenCompositeOp:
            call_back=LightenCompositePixels;
            break;
          case HueCompositeOp:
            call_back=HueCompositePixels;
            break;
          case SaturateCompositeOp:
            call_back=SaturateCompositePixels;
            break;
          case ColorizeCompositeOp:
            call_back=ColorizeCompositePixels;
            break;
          case LuminizeCompositeOp:
            call_back=LuminizeCompositePixels;
            break;
          case ScreenCompositeOp:
            /* Not implemented (Photoshop & PDF) */
            break;
          case OverlayCompositeOp:
            /* Not implemented (Photoshop & PDF) */
            break;
          case CopyCyanCompositeOp:
            call_back=CopyRedCompositePixels;
            break;
          case CopyMagentaCompositeOp:
            call_back=CopyGreenCompositePixels;
            break;
          case CopyYellowCompositeOp:
            call_back=CopyBlueCompositePixels;
            break;
          case CopyBlackCompositeOp:
            call_back=CopyBlackCompositePixels;
            break;
          default:
            {
              break;
            }
          }
        if (call_back != (PixelIteratorDualModifyCallback) NULL)
          {
            status=PixelIterateDualModify(call_back,              /* Callback */
                                          "Composite image pixels ...", /* Description */
                                          &options,               /* Options */
                                          columns,                /* Number of columns */
                                          rows,                   /* Number of rows */
                                          change_image,           /* Composite image */
                                          composite_x,            /* Composite x offset */
                                          composite_y,            /* Composite y offset */
                                          canvas_image,           /* Canvas image */
                                          canvas_x,               /* Canvas x offset */
                                          canvas_y,               /* Canvas y offset */
                                          &canvas_image->exception); /* Exception */
          }
        else
          {
            status=MagickFail;
          }
      }
  }

  DestroyImage(change_image);
  change_image=(Image *) NULL;

  return(status);
}
