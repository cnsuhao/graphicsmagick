/*
  Copyright (C) 2003, 2005 GraphicsMagick Group
 
  This program is covered by multiple licenses, which are described in
  Copyright.txt. You should have received a copy of Copyright.txt with this
  package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.

  Bitstream benchmarks and tests.  
  Written by Bob Friesenhahn, September 2003
 
*/
#include <magick/studio.h>
#include <magick/bit_stream.h>

int main ( int argc, char *argv[])
{
  const char
    *mode;

  register unsigned int
    rep,
    x;

  register unsigned char
    *bytes;


  unsigned int
    result=0;

  unsigned int
    quantum;

  FILE
    *file;

  if (argc < 2)
    {
      printf("usage: %s read/write/test args\n", argv[0]);
      exit(1);
    }

  mode = argv[1];

  file=fopen("/dev/null","wb+");
  if (!file)
    {
      printf("Failed to open file\n");
      exit(1);
    }

  if (strcmp(mode,"read") == 0)
    {
      BitStreamReadHandle
        read_stream;

      unsigned int
        bits;

      size_t
        reps;

      if (argc != 4)
        {
          printf("usage: %s read repetitions bits\n", argv[0]);
          exit(1);
        }

      reps = atoi(argv[2]);
      bits = atoi(argv[3]);

      bytes=(unsigned char *)malloc(reps*2);
      if (!bytes)
        {
          printf("Failed to allocate %lu bytes\n", (unsigned long) reps*2);
          exit(1);
        }
      for (rep=0; rep < reps*2; rep++)
        bytes[rep]=(unsigned char) rep;

      for (rep=reps; rep > 0; rep--)
        {
          BitStreamInitializeRead(&read_stream,bytes);
          for (x=reps; x > 0; x--)
            {
              quantum=BitStreamMSBRead(&read_stream,bits);
              result += quantum;
            }
        }

      free(bytes);
    }
  else if (strcmp(mode,"write") == 0)
    {
      BitStreamWriteHandle
        write_stream;

      unsigned int
        bits;

      size_t
        reps;

      if (argc != 4)
        {
          printf("usage: %s write repetitions bits\n", argv[0]);
          exit(1);
        }

      reps = atoi(argv[2]);
      bits = atoi(argv[3]);

      bytes=(unsigned char *)malloc(reps*2);
      if (!bytes)
        {
          printf("Failed to allocate %lu bytes\n", (unsigned long) reps*2);
          exit(1);
        }
      memset(bytes,0,reps*2);

      for (rep=reps; rep > 0; rep--)
        {
          BitStreamInitializeWrite(&write_stream,bytes);
          for (x=reps; x > 0; x--)
            {
              BitStreamMSBWrite(&write_stream,bits,x);
              result += bytes[x];
            }
        }
    }
  else if (strcmp(mode,"test") == 0)
    {
      BitStreamWriteHandle
        write_stream;

      BitStreamReadHandle
        read_stream;

      size_t
        allocated_bytes;

      unsigned int
        bits,
        max_bits,
        max_quantum,
        read_quantum,
        write_quantum;

      if (argc != 3)
        {
          printf("usage: %s test max_bits\n", argv[0]);
          exit(1);
        }

      max_bits = atoi(argv[2]);

      for (bits=1 ; bits <= max_bits; bits++)
        {
          max_quantum=0x01 << bits;

          allocated_bytes=max_quantum;
          if (bits > 8)
            allocated_bytes *= 2;
          if (bits > 16)
            allocated_bytes *= 2;
          
          bytes=(unsigned char *)malloc(allocated_bytes);
          if (!bytes)
            {
              printf("Failed to allocate %lu bytes\n",(unsigned long) allocated_bytes);
              exit(1);
            }
          memset(bytes,0xff,allocated_bytes);
          
          BitStreamInitializeWrite(&write_stream,bytes);
          BitStreamInitializeRead(&read_stream,bytes);
          
          for (write_quantum=0; write_quantum < max_quantum; write_quantum++)
            {
              BitStreamMSBWrite(&write_stream,bits,write_quantum);
              read_quantum=BitStreamMSBRead(&read_stream,bits);
              if (read_quantum != write_quantum)
                {
                  printf("mismatch: bits=%u write_quantum=%u read_quantum=%u\n",
                         bits,write_quantum,read_quantum);
                }
            }
          
          free(bytes);
        }
    }

  fprintf(file,"result=%u\n",result);
  fclose(file);

  return 0;
}