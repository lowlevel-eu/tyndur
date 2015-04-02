/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include <cdi/video.h>
#include <cdi/mem.h>
#include <stdio.h>
#include "drawing.h"
#include <stdlib.h>
#include <syscall.h>
#include <string.h>


#define min(x,y) (((x) < (y)) ? (x) : (y))

uint32_t colormasks[3] = { 0xffff0000, 0x00ffffff, 0xffffffff }; // 16, 24, 32 bpp

void vesa_set_raster_op(struct cdi_video_device* device,
    enum cdi_video_raster_op rop)
{
    device->raster_op = rop;
}

void vesa_draw_line(struct cdi_video_bitmap* target, unsigned int x1,
    unsigned int y1, unsigned int x2, unsigned int y2,
    struct cdi_video_color* color)
{
    // FIXME: Soviel zeichen wie moeglich. Grad zu faul.
    if (x1 < 0 || x1 >= target->width || y1 < 0 || y1 >= target->height ||
        x2 < 0 || x2 >= target->width || y2 < 0 || y2 >= target->height) {
        return;
    }
    int x_shift = target->format.bpp / 8;

   // Speicher suchen
    char *dstchar = NULL;
    dstchar = get_write_buffer_addr(target);
    dstchar += y1 * target->pitch;
    dstchar += x1 * x_shift;


    // Farben und Farbmasken
    uint32_t new = 0;// old = 0;
    uint32_t colormask, keepmask;

    colormask = colormasks[(target->format.bpp - 16) / 8];
    keepmask = ~colormask;

    new = convert_color(target->format, color);

    // Bresenham Initialisierung
    int dx = x2 - x1;
    int dy = y2 - y1;

    int xstep = 0, ystep = 0;
    int normal_step_x, normal_step_y, error_fast, error_slow;
    int error, i;

    if (dx > 0) {
        xstep = x_shift;
    } else if (dx < 0) {
        xstep = -x_shift;
        dx = -dx;
    }

    if (dy > 0) {
        ystep = target->pitch;
    } else if (dy < 0) {
        ystep = -target->pitch;
        dy = -dy;
    }

    if (dx > dy) {
        normal_step_x = xstep;
        normal_step_y = 0;
        error_fast = dy;
        error_slow = dx;
    } else {
        normal_step_x = 0;
        normal_step_y = ystep;
        error_fast = dx;
        error_slow = dy;
    }
    error = error_slow >> 1;


    for (i = 0; i <= error_slow; i++) {
        //old = *(uint32_t*)dstchar & colormask;
        *(uint32_t*)dstchar &= keepmask;
        *(uint32_t*)dstchar |= new & colormask;

        error -= error_fast;
        if (error < 0) {
            error += error_slow;
            dstchar += xstep;
            dstchar += ystep;
        } else {
            dstchar += normal_step_x;
            dstchar += normal_step_y;
        }
    }
}

void vesa_draw_rectangle(struct cdi_video_bitmap* target, unsigned int x,
    unsigned int y, unsigned int width, unsigned int height,
    struct cdi_video_color* color)
{
    // Rechteck auf Koordinaten anpassen
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x + width > target->width) {
        width = target->width - x;
    }
    if (y + height > target->height) {
        height = target->height - y;
    }
    if (width < 0 || height < 0 || x >= target->width  || y >= target->height) {
        return;
    }

    int widthctr;
    char* linestart = NULL;
    char* in_line;
    uint32_t* in_line_32;

    // Startadresse des Bitmaps setzen
    linestart = get_write_buffer_addr(target);

    // Offsets
    int x_shift = target->format.bpp / 8;
    int pitch = target->pitch;

    // Startpixel-Adresse berechnen
    linestart += y * pitch;
    linestart += x * x_shift;

    // Farbe
    uint32_t new = 0;
    new = convert_color(target->format, color);

    // Raster-OP abfragen
    enum cdi_video_raster_op rop = target->device->raster_op;

    // Groesse gegebenenfalls korrigieren
    if ((y + height) > target->height) {
        height = target->height - y;
    }

    if ((x + width) > target->width) {
        width = target->width - x;
    }

    // Ungueltige Werte abfangen
    if (height < 0 || width < 0 || x < 0 || y < 0) {
        return;
    }

    // Hier beginnt das eigentliche Zeichnen

    // 24-bit Farbtiefe
    if (target->format.bpp == 24) {

        // Schnelle Variante falls eh alles ueberschrieben wird
        if (rop == CDI_VIDEO_ROP_COPY) {
            // Allgemeine vorgehensweise: Solange wie moeglich mit 32bit auf
            // einmal arbeiten, d.h. 3 Zuweisungen per 4 Pixel. Anschliessend
            // wird Byteweise weitergearbeitet.

            // Buffer an 4 Pixeln in uint32_t vorbereiten
            uint8_t pixel_buffer_wrongtype[12] = {
                new & 0xFF,
                (new >> 8) & 0xFF,
                (new >> 16) & 0xFF,
                new & 0xFF,
                (new >> 8) & 0xFF,
                (new >> 16) & 0xFF,
                new & 0xFF,
                (new >> 8) & 0xFF,
                (new >> 16) & 0xFF,
                new & 0xFF,
                (new >> 8) & 0xFF,
                (new >> 16) & 0xFF,
            };
            uint32_t* pixel_buffer = (uint32_t*)pixel_buffer_wrongtype;

            // Kopieren!
            while (height > 0) {
                // Initialisierung
                in_line_32 = (uint32_t*)linestart;
                widthctr = width / 4;

                //printf("Line %d, at 0x%p\n", height, linestart);

                // 4-Pixel-Pakete
                while (widthctr > 0) {
                    //printf("%p ", in_line_32);
                    in_line_32[0] = pixel_buffer[0];
                    in_line_32[1] = pixel_buffer[1];
                    in_line_32[2] = pixel_buffer[2];
                    in_line_32 += 3;
                    widthctr -= 1;
                }

                // Die restlichen Pixel
                widthctr = width % 4;
                in_line = (char*)in_line_32;
                while (widthctr > 0) {
                    in_line[0] = new & 0xFF;
                    in_line[1] = (new >> 8) & 0xFF;
                    in_line[2] = (new >> 16) & 0xFF;
                    in_line += 3;

                    widthctr--;
                }
                //printf("\nEnd of line %d, at 0x%p\n", height, in_line);

                // Initialisierung fuer naechsten Durchgang
                linestart += pitch;
                height--;
            }

        // Standard-Variante mit Raster-OP
        } else {
            while(height >  0) {
                widthctr = width;
                in_line = linestart;
                while (widthctr > 0) {
                    char old[4] = { 0,0,0,0 };
                    old[0] = in_line[0];
                    old[1] = in_line[1];
                    old[2] = in_line[2];
                    char after_rop[4];
                    *(uint32_t*)after_rop = do_rop(rop, new, *(uint32_t*)old);
                    in_line[0] = after_rop[0];
                    in_line[1] = after_rop[1];
                    in_line[2] = after_rop[2];
                    in_line += 3;

                    widthctr--;
                }
                height--;
                linestart += pitch;
            }
        }

    // 32-Bit Farbtiefe
    } else if (target->format.bpp == 32) {
        // Es wird alles ueberschrieben -> schnelle Variante
        if (rop == CDI_VIDEO_ROP_COPY) {
            while (height > 0) {
                // Initialisierungsarbeit
                in_line_32 = (uint32_t*)linestart;
                widthctr = width;

                while (widthctr > 0) {
                    *in_line_32 = new;
                    in_line_32++;
                    widthctr--;
                }

                height--;
                linestart += pitch;
            }

        // Langsame Variante mit ROP
        } else {
            while (height > 0) {
                // Initialisierungsarbeit
                in_line_32 = (uint32_t*)linestart;
                widthctr = width;

                while (widthctr > 0) {
                    *in_line_32 = do_rop(rop, new, *in_line_32);
                    in_line_32++;
                    widthctr--;
                }

                height--;
                linestart += pitch;
            }
        }

    // Andere Farbtiefen
    } else {
        // TODO Andere Farbtiefen waeren schon toll...
        // Andererseits, wir haben bereits Support fuer 24 und 32-bit
        // Also, falls tatsaechlich mal 16bit notwendig wird...
    }
}

// TODO TODO TODO TODO TODO
void vesa_draw_ellipse(struct cdi_video_bitmap* target, unsigned int x,
    unsigned int y, unsigned int width, unsigned int height,
    struct cdi_video_color* color);

void vesa_draw_bitmap(struct cdi_video_bitmap* target,
    struct cdi_video_bitmap* source, unsigned int x, unsigned int y)
{
    vesa_draw_bitmap_part(target, source, x, y, 0, 0, source->width,
        source->height);
}

void vesa_draw_bitmap_part(struct cdi_video_bitmap* target,
    struct cdi_video_bitmap* source, unsigned int x, unsigned int y,
    unsigned int srcx, unsigned int srcy, unsigned int width,
    unsigned int height)
{
    int copywidth, copyheight, origcopywidth;

    // Zu kopierenden Bereich festlegen
    copywidth = min( min( target->width - x, width ),
                     source->width - srcx);
    copyheight = min (min (target->height - y, height),
                    source->height - srcy);

    // Speicherstellen berechnen
    char* srcchar = NULL;
    srcchar = get_read_buffer_addr(source);
    srcchar += (int)(srcx * source->format.bpp / 8);
    srcchar += (int)(srcy * source->pitch);

    char* dstchar = NULL;
    dstchar = get_write_buffer_addr(target);
    dstchar += (int)(x * target->format.bpp / 8);
    dstchar += (int)(y * target->pitch);

    char *dstline, *srcline;

    // Farben und Farbmasken
    uint32_t new = 0, old = 0;
    uint32_t colormask, keepmask;//sourcemask;

    colormask = colormasks[(target->format.bpp - 16) / 8];
    //sourcemask = colormasks[(source->format.bpp - 16) / 8];
    keepmask = ~colormask;

    // Offsets
    char src_bytes_per_pixel = source->format.bpp / 8;
    char dst_bytes_per_pixel = target->format.bpp / 8;

    // Schleifenvorbereitung
    dstline = dstchar;
    srcline = srcchar;
    origcopywidth = copywidth;

    // Schnelle Variante (memcpy), falls ROP_COPY und gleiches Format
    if ((target->device->raster_op == CDI_VIDEO_ROP_COPY) &&
        (memcmp(&(source->format), &(target->format),
            sizeof(struct cdi_video_pixel_format)) == 0))
    {
        while (copyheight > 0) {
//            stdout = 0;
//            printf("dstline: 0x%32x | srcline: 0x%32x | length: 0x32x\n", dstline, srcline, copywidth * dst_bytes_per_pixel);
            memcpy(dstline, srcline, copywidth * dst_bytes_per_pixel);
            dstline += target->pitch;
            srcline += source->pitch;
            copyheight--;
        }
    // Standard-Variante mit Support fuer alles
    } else {
        while (copyheight > 0) {
            copywidth = origcopywidth;
            while (copywidth > 0) {
                // Farben vorbereiten
                old = *(uint32_t*)dstchar & colormask;

                #if 1 // Korrekte Version
                struct cdi_video_color* newcolor;
                newcolor = convert_color_from_pixel(source->format,
                    *(uint32_t*)srcchar);
                new = convert_color(target->format, newcolor);
                free(newcolor);
                #else // Schnelle 32/24-Bit-Version - Achtung, nur gleiches Format!
                new = *(uint32_t*)srcchar & sourcemask;
                #endif
                new = do_rop(target->device->raster_op, new, old);

                // Malen
                // Fuer 32-bit-modi ginge auch *(uint32_t*) = new
                *(uint32_t*)dstchar &= keepmask;
                *(uint32_t*)dstchar |= new & colormask;

                // Zeiger einen Pixel weitersetzen
                srcchar += src_bytes_per_pixel;
                dstchar += dst_bytes_per_pixel;
                copywidth--;
            }
            // Linie fertig, naechste benutzen.
            dstline += target->pitch;
            srcline += source->pitch;
            srcchar = srcline;
            dstchar = dstline;
            copyheight--;
        }
    }
}

void vesa_draw_dot(struct cdi_video_bitmap* target, unsigned int x,
    unsigned int y, struct cdi_video_color* color)
{
    // Ungueltige Positionen abfangen
    if (x < 0 || y < 0 || x >= target->width || y >= target->height) {
        return;
    }

    // Speicherposition berechnen
    char *tmp;
    tmp = get_write_buffer_addr(target);
    tmp += (int)(y * (target->pitch));
    tmp += (int)(x * (target->format.bpp) / 8);

    uint32_t *start = (uint32_t*)tmp;

    // Farben und Farbmasken definieren.
    uint32_t newcolor = 0, oldcolor = 0;
    uint32_t colormask, keepmask;
    colormask = colormasks[(target->format.bpp - 16) / 8];
    keepmask = ~colormask;

    // Farbe berechnen
    oldcolor = *start & colormask;
    newcolor = convert_color(target->format, color);
    newcolor = do_rop(target->device->raster_op, newcolor, oldcolor);

    // und setzen.
    *start &= keepmask;
    *start |= newcolor & colormask;
}

uint32_t do_rop(enum cdi_video_raster_op rop, uint32_t new, uint32_t old)
{
    switch (rop) {
        case CDI_VIDEO_ROP_OR:
            return new | old;
        case CDI_VIDEO_ROP_XOR:
            return new ^ old;
        case CDI_VIDEO_ROP_AND:
            return new & old;
        default:
            fprintf(0, "VESA unterstützt ROP %d nicht\n",
                rop);
        // Hier _kein_ break, damit im Fehlerfall wenigstens was rauskommt.
        case CDI_VIDEO_ROP_COPY:
            return new;
    }
}

uint32_t convert_color(struct cdi_video_pixel_format fmt,
    struct cdi_video_color* cdicolor)
{
    uint32_t red, blue, green, alpha, color;
    red = cdicolor->red << fmt.red_bits >> 8;
    green = cdicolor->green << fmt.green_bits >> 8;
    blue = cdicolor->blue << fmt.blue_bits >> 8;

    // Hier pruefen reicht, RGB sollte eigentlich immer da sein.
    if (fmt.alpha_bits > 0) {
        alpha = cdicolor->alpha << fmt.alpha_bits >> 8;
    } else {
        alpha = 0;
    }
    color = red << fmt.red_offset |
        green << fmt.green_offset |
        blue << fmt.blue_offset |
        alpha << fmt.alpha_offset;
    return color;
}

struct cdi_video_color* convert_color_from_pixel(struct cdi_video_pixel_format
    fmt, uint32_t value)
{
    struct cdi_video_color* color = malloc(sizeof(struct cdi_video_color));
    int redmask, bluemask, greenmask, alphamask;

    // Masken setzen
    redmask = ((1 << fmt.red_bits) - 1) << fmt.red_offset;
    greenmask = ((1 << fmt.green_bits) - 1) << fmt.green_offset;
    bluemask = ((1 << fmt.blue_bits) - 1) << fmt.blue_offset;
    alphamask = ((1 << fmt.alpha_bits) - 1) << fmt.alpha_offset;

    // Farben extrahieren
    color->red = (value & redmask) >> fmt.red_offset;
    color->green = (value & greenmask) >> fmt.green_offset;
    color->blue = (value & bluemask) >> fmt.blue_offset;
    color->alpha = (value & alphamask) >> fmt.alpha_offset;

    // Auf die richtige Aufloesung bringen.
    color->red = color->red << 8 >> fmt.red_bits;
    color->green = color->green << 8 >> fmt.green_bits;
    color->blue = color->blue << 8 >> fmt.blue_bits;
    color->alpha = color->alpha << 8 >> fmt.alpha_bits;

    return color;
}

void* get_read_buffer_addr(struct cdi_video_bitmap* bitmap)
{
    int x;
    char* addr = NULL;
    for (x=0; x < bitmap->buffercount; x++) {
        if ((bitmap->buffers[x].type & CDI_VIDEO_BUFFER_READ) == CDI_VIDEO_BUFFER_READ) {
            if (bitmap->buffers[x].in_vram) {
                addr = bitmap->device->vram->vaddr;
            }
            addr += (uint32_t)bitmap->buffers[x].pixeldata;
            return addr;
        }
    }
    // Sollte wirklich nie passieren
    return NULL;
}

void* get_write_buffer_addr(struct cdi_video_bitmap* bitmap)
{
    int x;
    char* addr = NULL;
    for (x=0; x < bitmap->buffercount; x++) {
        if ((bitmap->buffers[x].type & CDI_VIDEO_BUFFER_WRITE) == CDI_VIDEO_BUFFER_WRITE) {
            if (bitmap->buffers[x].in_vram) {
                addr = bitmap->device->vram->vaddr;
            }
            addr += (uint32_t)bitmap->buffers[x].pixeldata;
            return addr;
        }
    }
    // Sollte wirklich nie passieren
    return NULL;
}

