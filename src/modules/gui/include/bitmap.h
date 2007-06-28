
#ifndef BITMAP_H
#define BITMAP_H

#include "types.h"

typedef struct bitmap_t
{
    dword *data;
    dword width;
    dword height;
} bitmap_t;

//BMP
typedef struct bmp_file_header_t
{
    word type;
    dword size;
    dword reserved;
    dword dataoffset;
} __attribute__((__packed__)) bmp_file_header_t;

typedef struct bmp_info_header_t
{
    dword size;
    dword width;
    dword height;
    word planes;
    word bitcount;
    dword compression;
    dword imagesize;
    dword pixelspermeterx;
    dword pixelspermetery;
    dword colorsused;
    dword colorsimportant;
} __attribute__((__packed__)) bmp_info_header_t;

typedef struct bmp_rgbquad_t
{
    byte blue;
    byte green;
    byte red;
    byte reserved;
} __attribute__((__packed__)) bmp_rgbquad_t;

bitmap_t load_bitmap_bmp(char *filename);

#endif
