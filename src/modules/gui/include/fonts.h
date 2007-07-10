
#ifndef FONTS_H
#define FONTS_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "collections.h"
#include "video/bitmap.h"

typedef struct server_font_t
{
    FT_Face face;
    char *name;
    list_t *data;
} server_font_t;

typedef struct server_font_data_t
{
    int use_count;
    int size;
    dword id;
    dword shmid;
    byte *data;
} server_font_data_t;

typedef struct glyph_t
{
    int width;
    int height;
    int bearing_x;
    int bearing_y;
    int advance;
    bitmap_t bitmap;
} glyph_t;

int init_fonts(void);

server_font_t *load_font(const char *name);
int load_font_data(const char *name, int size);
server_font_data_t *get_font_data(const char *name, int size);
server_font_data_t *use_font_data(dword id);
void drop_font_data(dword id);

#endif
