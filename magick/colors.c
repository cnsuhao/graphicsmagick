/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  CCCC   OOO   L       OOO   RRRR    SSSSS                   %
%                 C      O   O  L      O   O  R   R   SS                      %
%                 C      O   O  L      O   O  RRRR     SSS                    %
%                 C      O   O  L      O   O  R R        SS                   %
%                  CCCC   OOO   LLLLL   OOO   R  R    SSSSS                   %
%                                                                             %
%                                                                             %
%                       Count the Colors in an Image                          %
%                                                                             %
%                                                                             %
%                                                                             %
%                           Software Design                                   %
%                             John Cristy                                     %
%                              July 1992                                      %
%                                                                             %
%                                                                             %
%  Copyright 1998 E. I. du Pont de Nemours and Company                        %
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
%  E. I. du Pont de Nemours and Company be liable for any claim, damages or   %
%  other liability, whether in an action of contract, tort or otherwise,      %
%  arising from, out of or in connection with ImageMagick or the use or other %
%  dealings in ImageMagick.                                                   %
%                                                                             %
%  Except as contained in this notice, the name of the E. I. du Pont de       %
%  Nemours and Company shall not be used in advertising or otherwise to       %
%  promote the sale, use or other dealings in ImageMagick without prior       %
%  written authorization from the E. I. du Pont de Nemours and Company.       %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "magick.h"
#include "Colorlist.h"
/*
  Define declarations.
*/
#define MaxTreeDepth  8
#define NodesInAList  2048

/*
  Structures.
*/
typedef struct _ColorsList
{
  Quantum
    red,
    green,
    blue;

  unsigned long
    count;
} ColorsList;

typedef struct _NodeInfo
{
  unsigned char
    level;

  unsigned long
    number_unique;

  ColorsList
    *list;

  struct _NodeInfo
    *child[8];
} NodeInfo;

typedef struct _Nodes
{
  NodeInfo
    nodes[NodesInAList];

  struct _Nodes
    *next;
} Nodes;

typedef struct _CubeInfo
{
  NodeInfo
    *root;

  unsigned int
    progress,
    colors;

  unsigned int
    free_nodes;

  NodeInfo
    *node_info;

  Nodes
    *node_list;
} CubeInfo;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  D e s t r o y L i s t                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure DestroyList traverses the color cube tree and free the list of
%  unique colors.
%
%  The format of the DestroyList routine is:
%
+      DestroyList(node_info)
%
%  A description of each parameter follows.
%
%    o node_info: The address of a structure of type NodeInfo which points to a
%      node in the color cube tree that is to be pruned.
%
%
*/
static void DestroyList(const NodeInfo *node_info)
{
  register unsigned int
    id;

  /*
    Traverse any children.
  */
  for (id=0; id < 8; id++)
    if (node_info->child[id] != (NodeInfo *) NULL)
      DestroyList(node_info->child[id]);
  if (node_info->level == MaxTreeDepth)
    if (node_info->list != (ColorsList *) NULL)
      FreeMemory((char *) node_info->list);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  H i s t o g r a m                                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure Histogram traverses the color cube tree and produces a list of
%  unique pixel field values and the number of times each occurs in the image.
%
%  The format of the Histogram routine is:
%
%      Histogram(color_cube,node_info,file)
%
%  A description of each parameter follows.
%
%    o color_cube: A pointer to the CubeInfo structure.
%
%    o node_info: The address of a structure of type NodeInfo which points to a
%      node in the color cube tree that is to be pruned.
%
%
*/
static void Histogram(CubeInfo *color_cube,const NodeInfo *node_info,FILE *file)
{
#define HistogramImageText  "  Computing image histogram...  "

  register unsigned int
    id;

  /*
    Traverse any children.
  */
  for (id=0; id < 8; id++)
    if (node_info->child[id] != (NodeInfo *) NULL)
      Histogram(color_cube,node_info->child[id],file);
  if (node_info->level == MaxTreeDepth)
    {
      char
        name[MaxTextExtent];

      ColorsList
        *p;

      double
        distance_squared,
        min_distance;

      int
        distance;

      register int
        i;

      register XColorlist
        *q;

      p=node_info->list;
      for (i=0; i < node_info->number_unique; i++)
      {
        (void) fprintf(file,"%10lu: (%3d,%3d,%3d)  #%02x%02x%02x",
          p->count,p->red,p->green,p->blue,(unsigned int) p->red,
          (unsigned int) p->green,(unsigned int) p->blue);
        min_distance=3.0*65536.0*65536.0;
        for (q=Colorlist; q->name != (char *) NULL; q++)
        {
          distance=(int) DownScale(p->red)-(int) q->red;
          distance_squared=distance*distance;
          distance=(int) DownScale(p->green)-(int) q->green;
          distance_squared+=distance*distance;
          distance=(int) DownScale(p->blue)-(int) q->blue;
          distance_squared+=distance*distance;
          if (distance_squared < min_distance)
            {
              min_distance=distance_squared;
              (void) strcpy(name,q->name);
            }
        }
        (void) fprintf(file,"  ");
        if (min_distance < 16)
          {
            if (min_distance > 0)
              (void) fprintf(file,"~");
            (void) fprintf(file,"%s",name);
          }
        (void) fprintf(file,"\n");
      }
      if (((color_cube->progress+1) == color_cube->colors) ||
          ((color_cube->progress % ((color_cube->colors >> 8)+1)) == 0))
        ProgressMonitor(HistogramImageText,color_cube->progress,
          color_cube->colors);
      color_cube->progress++;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I n i t i a l i z e N o d e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method InitializeNode allocates memory for a new node in the color cube
%  tree and presets all fields to zero.
%
%  The format of the InitializeNode routine is:
%
+      node_info=InitializeNode(color_cube,level)
%
%  A description of each parameter follows.
%
%    o color_cube: A pointer to the CubeInfo structure.
%
%    o level: Specifies the level in the classification the node resides.
%
%
*/
static NodeInfo *InitializeNode(CubeInfo *color_cube,const unsigned int level)
{
  register int
    i;

  NodeInfo
    *node_info;

  if (color_cube->free_nodes == 0)
    {
      Nodes
        *nodes;

      /*
        Allocate a new nodes of nodes.
      */
      nodes=(Nodes *) AllocateMemory(sizeof(Nodes));
      if (nodes == (Nodes *) NULL)
        return((NodeInfo *) NULL);
      nodes->next=color_cube->node_list;
      color_cube->node_list=nodes;
      color_cube->node_info=nodes->nodes;
      color_cube->free_nodes=NodesInAList;
    }
  color_cube->free_nodes--;
  node_info=color_cube->node_info++;
  for (i=0; i < 8; i++)
    node_info->child[i]=(NodeInfo *) NULL;
  node_info->level=level;
  node_info->number_unique=0;
  node_info->list=(ColorsList *) NULL;
  return(node_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s P s e u d o C l a s s                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IsPseudoClass returns True if the image is PseudoClass and has 256
%  unique colors or less.  If the image is DirectClass and has less 256 colors
%  or less, the image is demoted to PseudoClass.
%
%  The format of the IsPseudoClass routine is:
%
%      status=IsPseudoClass(image)
%
%  A description of each parameter follows.
%
%    o status:  Method IsPseudoClass returns True is the image is
%      PseudoClass or has 256 color or less.
%
%    o image: The address of a byte (8 bits) array of run-length
%      encoded pixel data of your source image.  The sum of the
%      run-length counts in the source image must be equal to or exceed
%      the number of pixels.
%
%
*/
unsigned int IsPseudoClass(Image *image)
{
  CubeInfo
    color_cube;

  NodeInfo
    *node_info;

  Nodes
    *nodes;

  register int
    i,
    j;

  register RunlengthPacket
    *p;

  register unsigned int
    id,
    index,
    level;

  assert(image != (Image *) NULL);
  if ((image->class == PseudoClass) && (image->colors <= 256))
    return(True);
  if (image->matte)
    return(False);
  /*
    Initialize color description tree.
  */
  color_cube.node_list=(Nodes *) NULL;
  color_cube.progress=0;
  color_cube.colors=0;
  color_cube.free_nodes=0;
  color_cube.root=InitializeNode(&color_cube,0);
  if (color_cube.root == (NodeInfo *) NULL)
    {
      MagickWarning(ResourceLimitWarning,"Unable to count colors",
        "Memory allocation failed");
      return(False);
    }
  p=image->pixels;
  for (i=0; (i < image->packets) && (color_cube.colors <= 256); i++)
  {
    /*
      Start at the root and proceed level by level.
    */
    node_info=color_cube.root;
    index=MaxTreeDepth-1;
    for (level=1; level <= MaxTreeDepth; level++)
    {
      id=(((Quantum) DownScale(p->red) >> index) & 0x01) << 2 |
         (((Quantum) DownScale(p->green) >> index) & 0x01) << 1 |
         (((Quantum) DownScale(p->blue) >> index) & 0x01);
      if (node_info->child[id] == (NodeInfo *) NULL)
        {
          node_info->child[id]=InitializeNode(&color_cube,level);
          if (node_info->child[id] == (NodeInfo *) NULL)
            {
              MagickWarning(ResourceLimitWarning,"Unable to count colors",
                "Memory allocation failed");
              return(False);
            }
        }
      node_info=node_info->child[id];
      index--;
      if (level != MaxTreeDepth)
        continue;
      for (j=0; j < node_info->number_unique; j++)
         if ((p->red == node_info->list[j].red) &&
             (p->green == node_info->list[j].green) &&
             (p->blue == node_info->list[j].blue))
           break;
      if (j < node_info->number_unique)
        {
          node_info->list[j].count+=p->length+1;
          continue;
        }
      if (node_info->number_unique == 0)
        node_info->list=(ColorsList *) AllocateMemory(sizeof(ColorsList));
      else
        node_info->list=(ColorsList *)
          ReallocateMemory(node_info->list,(j+1)*sizeof(ColorsList));
      if (node_info->list == (ColorsList *) NULL)
        {
          MagickWarning(ResourceLimitWarning,"Unable to count colors",
            "Memory allocation failed");
          return(False);
        }
      node_info->list[j].red=p->red;
      node_info->list[j].green=p->green;
      node_info->list[j].blue=p->blue;
      node_info->list[j].count=1;
      node_info->number_unique++;
      color_cube.colors++;
    }
    p++;
  }
  /*
    Release color cube tree storage.
  */
  DestroyList(color_cube.root);
  do
  {
    nodes=color_cube.node_list->next;
    FreeMemory((char *) color_cube.node_list);
    color_cube.node_list=nodes;
  }
  while (color_cube.node_list != (Nodes *) NULL);
  if (color_cube.colors <= 256)
    {
      QuantizeInfo
        quantize_info;

      /*
        Demote DirectClass to PseudoClass.
      */
      GetQuantizeInfo(&quantize_info);
      quantize_info.number_colors=256;
      (void) QuantizeImage(&quantize_info,image);
      SyncImage(image);
    }
  return((image->class == PseudoClass) && (image->colors <= 256));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  N u m b e r C o l o r s                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method NumberColors returns the number of unique colors in an image.
%
%  The format of the NumberColors routine is:
%
%      NumberColors(image,file)
%
%  A description of each parameter follows.
%
%    o image: The address of a byte (8 bits) array of run-length
%      encoded pixel data of your source image.  The sum of the
%      run-length counts in the source image must be equal to or exceed
%      the number of pixels.
%
%    o file:  An pointer to a FILE.  If it is non-null a list of unique pixel
%      field values and the number of times each occurs in the image is
%      written to the file.
%
%
%
*/
Export void NumberColors(Image *image,FILE *file)
{
#define NumberColorsImageText  "  Computing image colors...  "

  CubeInfo
    color_cube;

  NodeInfo
    *node_info;

  Nodes
    *nodes;

  register int
    i,
    j;

  register RunlengthPacket
    *p;

  register unsigned int
    id,
    index,
    level;

  /*
    Initialize color description tree.
  */
  assert(image != (Image *) NULL);
  image->total_colors=0;
  color_cube.node_list=(Nodes *) NULL;
  color_cube.colors=0;
  color_cube.free_nodes=0;
  color_cube.root=InitializeNode(&color_cube,0);
  if (color_cube.root == (NodeInfo *) NULL)
    {
      MagickWarning(ResourceLimitWarning,"Unable to count colors",
        "Memory allocation failed");
      return;
    }
  p=image->pixels;
  for (i=0; i < image->packets; i++)
  {
    /*
      Start at the root and proceed level by level.
    */
    node_info=color_cube.root;
    index=MaxTreeDepth-1;
    for (level=1; level <= MaxTreeDepth; level++)
    {
      id=(((Quantum) DownScale(p->red) >> index) & 0x01) << 2 |
         (((Quantum) DownScale(p->green) >> index) & 0x01) << 1 |
         (((Quantum) DownScale(p->blue) >> index) & 0x01);
      if (node_info->child[id] == (NodeInfo *) NULL)
        {
          node_info->child[id]=InitializeNode(&color_cube,level);
          if (node_info->child[id] == (NodeInfo *) NULL)
            {
              MagickWarning(ResourceLimitWarning,"Unable to count colors",
                "Memory allocation failed");
              return;
            }
        }
      node_info=node_info->child[id];
      index--;
      if (level != MaxTreeDepth)
        continue;
      for (j=0; j < node_info->number_unique; j++)
         if ((p->red == node_info->list[j].red) &&
             (p->green == node_info->list[j].green) &&
             (p->blue == node_info->list[j].blue))
           break;
      if (j < node_info->number_unique)
        {
          node_info->list[j].count+=p->length+1;
          continue;
        }
      if (node_info->number_unique == 0)
        node_info->list=(ColorsList *) AllocateMemory(sizeof(ColorsList));
      else
        node_info->list=(ColorsList *)
          ReallocateMemory(node_info->list,(j+1)*sizeof(ColorsList));
      if (node_info->list == (ColorsList *) NULL)
        {
          MagickWarning(ResourceLimitWarning,"Unable to count colors",
            "Memory allocation failed");
          return;
        }
      node_info->list[j].red=p->red;
      node_info->list[j].green=p->green;
      node_info->list[j].blue=p->blue;
      node_info->list[j].count=p->length+1;
      node_info->number_unique++;
      color_cube.colors++;
    }
    p++;
    if (QuantumTick(i,image))
      ProgressMonitor(NumberColorsImageText,i,image->packets);
  }
  if (file != (FILE *) NULL)
    Histogram(&color_cube,color_cube.root,file);
  /*
    Release color cube tree storage.
  */
  DestroyList(color_cube.root);
  do
  {
    nodes=color_cube.node_list->next;
    FreeMemory((char *) color_cube.node_list);
    color_cube.node_list=nodes;
  }
  while (color_cube.node_list != (Nodes *) NULL);
  image->total_colors=color_cube.colors;
}
