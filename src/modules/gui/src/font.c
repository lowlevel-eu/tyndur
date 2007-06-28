
#include "font.h"

#include "string.h"
#include "stdlib.h"

glyph_t *get_glyph(font_t font, char character)
{
    int i;
    for (i = 0; i < font.glyphcount; i++)
    {
        if (font.glyphs[i].character == character) {
            return &font.glyphs[i];
        }
    }
    return 0;
}
dword get_string_length(font_t font, char *string)
{
    if (!string) return 0;
    int length = 0;
    int i;
    for (i = 0; i < strlen(string); i++) {
        glyph_t *glyph = get_glyph(font, string[i]);
        if (glyph) {
            length = length + glyph->width;
        }
    }
    return length;
}
bitmap_t render_string(font_t font, char *string)
{
    bitmap_t bitmap;
    bitmap.width = get_string_length(font, string);
    bitmap.height = font.height;
    bitmap.data = malloc(bitmap.width * bitmap.height * sizeof(dword));
    if (!bitmap.data) {
        bitmap.width = 0;
        bitmap.height = 0;
        return bitmap;
    }
    memset(bitmap.data, 0xFF, bitmap.width * bitmap.height * sizeof(dword));
    int x = 0;
    int i;
    int ix;
    int iy;
    for (i = 0; i < strlen(string); i++) {
        glyph_t *glyph = get_glyph(font, string[i]);
        if (glyph) {
            //printf("%02X%02X%02X%02X\n", glyph->data[0], glyph->data[1], glyph->data[2], glyph->data[3]);
            //Zeichne Zeichen
            dword linesize = glyph->width;
            if (linesize & 0x7) linesize = (linesize & 0xFFFFFFF8) + 0x8;
            for (iy = 0; iy < (glyph->ascent + glyph->descent); iy++) {
                for (ix = 0; ix < glyph->width; ix++) {
                    if ((glyph->data[(ix >> 3) + iy * linesize / 8] >> (7 - (ix % 8))) & 1) {
                        bitmap.data[x + ix + (iy + font.max_ascent - glyph->ascent) * bitmap.width] = 0xFF000000;
                    } else {
                        bitmap.data[x + ix + (iy + font.max_ascent - glyph->ascent) * bitmap.width] = 0x00FFFFFF;
                    }
                    //printf("y: %d\n", iy * linesize / 8);
                }
            }
            
            x = x + glyph->width;
        }
    }
    return bitmap;
}
