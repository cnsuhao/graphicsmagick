/*
  ImageMagick MagickWand interface.
*/

#ifndef _MAGICK_WAND_H
#define _MAGICK_WAND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(_VISUALC_)
#  if defined(_MT) && defined(_DLL) && !defined(_LIB)
#    pragma warning( disable: 4273 )
#    if !defined(_WANDLIB_)
#      define WandExport __declspec(dllimport)
#    else
#     define WandExport __declspec(dllexport)
#    endif
#  else
#    define WandExport
#  endif

#  pragma warning(disable : 4018)
#  pragma warning(disable : 4244)
#  pragma warning(disable : 4142)
#else
#  define WandExport
#endif

#include "magick/api.h"
#include "wand/drawing_wand.h"
#include "wand/pixel_wand.h"

  /*
    ImageMagick compatibility definitions
  */
#define MagickSizeType magick_int64_t
#define ReplaceCompositeOp CopyCompositeOp
#define IndexChannel BlackChannel
#define AreaResource UndefinedResource /* not supported */

typedef enum
{
  UndefinedMetric,
  MeanAbsoluteErrorMetric,
  MeanSquaredErrorMetric,
  PeakAbsoluteErrorMetric,
  PeakSignalToNoiseRatioMetric,
  RootMeanSquaredErrorMetric
} MetricType;

typedef struct _MagickWand
  MagickWand;

extern WandExport char
  *MagickDescribeImage(MagickWand *),
  *MagickGetException(const MagickWand *,ExceptionType *),
  *MagickGetFilename(const MagickWand *),
  *MagickGetImageFilename(MagickWand *),
  *MagickGetImageFormat(MagickWand *),
  *MagickGetImageSignature(MagickWand *),
  **MagickQueryFonts(const char *,unsigned long *),
  **MagickQueryFormats(const char *,unsigned long *);

extern WandExport CompositeOperator
  MagickGetImageCompose(MagickWand *);

extern WandExport ColorspaceType
  MagickGetImageColorspace(MagickWand *);

extern WandExport CompressionType
  MagickGetImageCompression(MagickWand *);

extern WandExport DisposeType
  MagickGetImageDispose(MagickWand *);

extern WandExport double
  MagickGetImageGamma(MagickWand *),
  MagickGetImageXResolution(MagickWand *),
  MagickGetImageYResolution(MagickWand *),
  *MagickGetSamplingFactors(const MagickWand *,unsigned long *),
  *MagickQueryFontMetrics(MagickWand *,const DrawingWand *,const char *);

extern WandExport ImageType
  MagickGetImageType(MagickWand *);

extern WandExport InterlaceType
  MagickGetImageInterlaceScheme(MagickWand *);

extern WandExport long
  MagickGetImageIndex(MagickWand *);

extern WandExport MagickSizeType
  MagickGetImageSize(MagickWand *);

extern WandExport MagickWand
  *CloneMagickWand(const MagickWand *),
  *MagickAppendImages(MagickWand *,const unsigned int),
  *MagickAverageImages(MagickWand *),
  *MagickCoalesceImages(MagickWand *),
  *MagickCompareImageChannels(MagickWand *,const MagickWand *,const ChannelType,
    const MetricType,double *),
  *MagickCompareImages(MagickWand *,const MagickWand *,const MetricType,
    double *),
  *MagickDeconstructImages(MagickWand *),
  *MagickFlattenImages(MagickWand *),
  *MagickCloneImage(MagickWand *),
  *MagickMorphImages(MagickWand *,const unsigned long),
  *MagickMosaicImages(MagickWand *),
  *MagickMontageImage(MagickWand *,const DrawingWand *,const char *,
    const char *,const MontageMode,const char *),
  *MagickSteganoImage(MagickWand *,const MagickWand *,const long),
  *MagickStereoImage(MagickWand *,const MagickWand *),
  *MagickTextureImage(MagickWand *,const MagickWand *),
  *MagickTransformImage(MagickWand *,const char *,const char *),
  *NewMagickWand(void);

extern WandExport PixelWand
  **MagickGetImageHistogram(MagickWand *,unsigned long *);

extern WandExport RenderingIntent
  MagickGetImageRenderingIntent(MagickWand *);

extern WandExport ResolutionType
  MagickGetImageUnits(MagickWand *);

extern WandExport unsigned int
  DestroyMagickWand(MagickWand *),
  MagickAdaptiveThresholdImage(MagickWand *,const unsigned long,
    const unsigned long,const long),
  MagickAddImage(MagickWand *,const MagickWand *),
  MagickAddNoiseImage(MagickWand *,const NoiseType),
  MagickAffineTransformImage(MagickWand *,const DrawingWand *),
  MagickAnnotateImage(MagickWand *,const DrawingWand *,const double,
    const double,const char *),
  MagickAnimateImages(MagickWand *,const char *),
  MagickBlackThresholdImage(MagickWand *,const PixelWand *),
  MagickBlurImage(MagickWand *,const double,const double),
  MagickBorderImage(MagickWand *,const PixelWand *,const unsigned long,
    const unsigned long),
  MagickChannelImage(MagickWand *,const ChannelType),
  MagickCharcoalImage(MagickWand *,const double,const double),
  MagickChopImage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  MagickClipImage(MagickWand *),
  MagickClipPathImage(MagickWand *,const char *,const unsigned int),
  MagickColorFloodfillImage(MagickWand *,const PixelWand *,const double,
    const PixelWand *,const long,const long),
  MagickColorizeImage(MagickWand *,const PixelWand *,const PixelWand *),
  MagickCommentImage(MagickWand *,const char *),
  MagickCompositeImage(MagickWand *,const MagickWand *,const CompositeOperator,
    const long,const long),
  MagickContrastImage(MagickWand *,const unsigned int),
  MagickConvolveImage(MagickWand *,const unsigned long,const double *),
  MagickCropImage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  MagickCycleColormapImage(MagickWand *,const long),
  MagickDespeckleImage(MagickWand *),
  MagickDisplayImage(MagickWand *,const char *),
  MagickDisplayImages(MagickWand *,const char *),
  MagickDrawImage(MagickWand *,const DrawingWand *),
  MagickEdgeImage(MagickWand *,const double),
  MagickEmbossImage(MagickWand *,const double,const double),
  MagickEnhanceImage(MagickWand *),
  MagickEqualizeImage(MagickWand *),
  MagickFlipImage(MagickWand *),
  MagickFlopImage(MagickWand *),
  MagickFrameImage(MagickWand *,const PixelWand *,const unsigned long,
    const unsigned long,const long,const long),
  MagickFxImageChannel(MagickWand *,const MagickWand *,const ChannelType,
    const char *),
  MagickGammaImage(MagickWand *,const double),
  MagickGammaImageChannel(MagickWand *,const ChannelType,const double),
  MagickGetImageBackgroundColor(MagickWand *,PixelWand *),
  MagickGetImageBluePrimary(MagickWand *,double *,double *),
  MagickGetImageBorderColor(MagickWand *,PixelWand *),
  MagickGetImageChannelExtrema(MagickWand *,const ChannelType,unsigned long *,
    unsigned long *),
  MagickGetImageChannelMean(MagickWand *,const ChannelType,double *,double *),
  MagickGetImageColormapColor(MagickWand *,const unsigned long,PixelWand *),
  MagickGetImageExtrema(MagickWand *,unsigned long *,unsigned long *),
  MagickGetImageGreenPrimary(MagickWand *,double *,double *),
  MagickGetImageMatteColor(MagickWand *,PixelWand *),
  MagickGetImageMean(MagickWand *,double *,double *),
  MagickGetImagePixels(MagickWand *,const long,const long,const unsigned long,
    const unsigned long,const char *,const StorageType,unsigned char *),
  MagickGetImageRedPrimary(MagickWand *,double *,double *),
  MagickGetImageWhitePoint(MagickWand *,double *,double *),
  MagickGetSize(const MagickWand *,unsigned long *,unsigned long *),
  MagickHasNextImage(MagickWand *),
  MagickHasPreviousImage(MagickWand *),
  MagickImplodeImage(MagickWand *,const double),
  MagickLabelImage(MagickWand *,const char *),
  MagickLevelImage(MagickWand *,const double,const double,const double),
  MagickLevelImageChannel(MagickWand *,const ChannelType,const double,
    const double,const double),
  MagickMagnifyImage(MagickWand *),
  MagickMapImage(MagickWand *,const MagickWand *,const unsigned int),
  MagickMatteFloodfillImage(MagickWand *,const Quantum,const double,
    const PixelWand *,const long,const long),
  MagickMedianFilterImage(MagickWand *,const double),
  MagickMinifyImage(MagickWand *),
  MagickModulateImage(MagickWand *,const double,const double,const double),
  MagickMotionBlurImage(MagickWand *,const double,const double,const double),
  MagickNegateImage(MagickWand *,const unsigned int),
  MagickNegateImageChannel(MagickWand *,const ChannelType,const unsigned int),
  MagickNextImage(MagickWand *),
  MagickNormalizeImage(MagickWand *),
  MagickOilPaintImage(MagickWand *,const double),
  MagickOpaqueImage(MagickWand *,const PixelWand *,const PixelWand *,
    const double),
  MagickPingImage(MagickWand *,const char *),
  MagickPreviousImage(MagickWand *),
  MagickProfileImage(MagickWand *,const char *,const unsigned char *,
    const unsigned long),
  MagickQuantizeImage(MagickWand *,const unsigned long,const ColorspaceType,
    const unsigned long,const unsigned int,const unsigned int),
  MagickQuantizeImages(MagickWand *,const unsigned long,const ColorspaceType,
    const unsigned long,const unsigned int,const unsigned int),
  MagickRaiseImage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long,const unsigned int),
  MagickReadImage(MagickWand *,const char *),
  MagickReadImageBlob(MagickWand *,const unsigned char *,const size_t length),
  MagickReadImageFile(MagickWand *,FILE *),
  MagickReduceNoiseImage(MagickWand *,const double),
  MagickRelinquishMemory(void *),
  MagickRemoveImage(MagickWand *),
  MagickResampleImage(MagickWand *,const double,const double,const FilterTypes,
    const double),
  MagickResizeImage(MagickWand *,const unsigned long,const unsigned long,
    const FilterTypes,const double),
  MagickRollImage(MagickWand *,const long,const long),
  MagickRotateImage(MagickWand *,const PixelWand *,const double),
  MagickSampleImage(MagickWand *,const unsigned long,const unsigned long),
  MagickScaleImage(MagickWand *,const unsigned long,const unsigned long),
  MagickSetImage(MagickWand *,const MagickWand *),
  MagickSetFilename(MagickWand *,const char *),
  MagickSetImageBackgroundColor(MagickWand *,const PixelWand *),
  MagickSetImageBluePrimary(MagickWand *,const double,const double),
  MagickSetImageBorderColor(MagickWand *,const PixelWand *),
  MagickSetImageChannelDepth(MagickWand *,const ChannelType,
    const unsigned long),
  MagickSetImageColormapColor(MagickWand *,const unsigned long,
    const PixelWand *),
  MagickSetImageCompose(MagickWand *,const CompositeOperator),
  MagickSetImageCompression(MagickWand *,const CompressionType),
  MagickSetImageDelay(MagickWand *,const unsigned long),
  MagickSetImageDepth(MagickWand *,const unsigned long),
  MagickSetImageDispose(MagickWand *,const DisposeType),
  MagickSetImageColorspace(MagickWand *,const ColorspaceType),
  MagickSetImageGreenPrimary(MagickWand *,const double,const double),
  MagickSetImageGamma(MagickWand *,const double),
  MagickSetImageFilename(MagickWand *,const char *),
  MagickSetImageIndex(MagickWand *,const unsigned long),
  MagickSetImageInterlaceScheme(MagickWand *,const InterlaceType),
  MagickSetImageIterations(MagickWand *,const unsigned long),
  MagickSetImageMatteColor(MagickWand *,const PixelWand *),
  MagickSetImageOption(MagickWand *,const char *,const char *),
  MagickSetImagePixels(MagickWand *,const long,const long,const unsigned long,
    const unsigned long,const char *,const StorageType,unsigned char *),
  MagickSetImageRedPrimary(MagickWand *,const double,const double),
  MagickSetImageRenderingIntent(MagickWand *,const RenderingIntent),
  MagickSetImageResolution(MagickWand *,const double,const double),
  MagickSetImageScene(MagickWand *,const unsigned long),
  MagickSetImageType(MagickWand *,const ImageType),
  MagickSetImageUnits(MagickWand *,const ResolutionType),
  MagickSetImageVirtualPixelMethod(MagickWand *,const VirtualPixelMethod),
  MagickSetPassphrase(MagickWand *,const char *),
  MagickSetResourceLimit(const ResourceType type,const unsigned long limit),
  MagickSetSamplingFactors(MagickWand *,const unsigned long,const double *),
  MagickSetSize(MagickWand *,const unsigned long,const unsigned long),
  MagickSetImageWhitePoint(MagickWand *,const double,const double),
  MagickSetInterlaceScheme(MagickWand *,const InterlaceType),
  MagickSharpenImage(MagickWand *,const double,const double),
  MagickShaveImage(MagickWand *,const unsigned long,const unsigned long),
  MagickShearImage(MagickWand *,const PixelWand *,const double,const double),
  MagickSolarizeImage(MagickWand *,const double),
  MagickSpreadImage(MagickWand *,const double),
  MagickStripImage(MagickWand *),
  MagickSwirlImage(MagickWand *,const double),
  MagickTintImage(MagickWand *,const PixelWand *,const PixelWand *),
  MagickThresholdImage(MagickWand *,const double),
  MagickThresholdImageChannel(MagickWand *,const ChannelType,const double),
  MagickTransparentImage(MagickWand *,const PixelWand *,const Quantum,
    const double),
  MagickTrimImage(MagickWand *,const double),
  MagickUnsharpMaskImage(MagickWand *,const double,const double,const double,
    const double),
  MagickWaveImage(MagickWand *,const double,const double),
  MagickWhiteThresholdImage(MagickWand *,const PixelWand *),
  MagickWriteImage(MagickWand *,const char *),
  MagickWriteImageFile(MagickWand *,FILE *),
  MagickWriteImages(MagickWand *,const char *,const unsigned int);

extern WandExport unsigned long
  MagickGetImageColors(MagickWand *),
  MagickGetImageDelay(MagickWand *),
  MagickGetImageChannelDepth(MagickWand *,const ChannelType),
  MagickGetImageDepth(MagickWand *),
  MagickGetImageHeight(MagickWand *),
  MagickGetImageIterations(MagickWand *),
  MagickGetImageScene(MagickWand *),
  MagickGetImageWidth(MagickWand *),
  MagickGetNumberImages(MagickWand *),
  MagickGetResource(const ResourceType);

extern WandExport VirtualPixelMethod
  MagickGetImageVirtualPixelMethod(MagickWand *);

extern WandExport unsigned char
  *MagickGetImageProfile(MagickWand *,const char *,unsigned long *),
  *MagickWriteImageBlob(MagickWand *,size_t *);

extern WandExport void
  MagickResetIterator(MagickWand *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* ifndef _MAGICK_WAND_H */