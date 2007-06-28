
#ifndef FONT_H
#define FONT_H

#include "types.h"
#include "bitmap.h"

typedef struct glyph_t
{
    char character;
    int left_sided_bearing;
    int right_sided_bearing;
    int width;
    int ascent;
    int descent;
    byte *data;
} glyph_t;

typedef struct font_t
{
    char *name;
    dword glyphcount;
    dword height;
    dword max_ascent;
    dword max_descent;
    glyph_t *glyphs;
} font_t;

glyph_t *get_glyph(font_t font, char character);
dword get_string_length(font_t font, char *string);
bitmap_t render_string(font_t font, char *string);

#endif
