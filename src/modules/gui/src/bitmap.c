/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "bitmap.h"

#include "stdlib.h"
#include "stdio.h"

bitmap_t load_bitmap_bmp(char *filename)
{
    bitmap_t newbitmap;
    newbitmap.data = 0;
    newbitmap.height = 0;
    newbitmap.width = 0;
    FILE *bitmapfile = fopen(filename, "r");
    if (!bitmapfile) {
        return newbitmap;
    }
    //Lese Dateiheader
    bmp_file_header_t fileheader;
    if (fread(&fileheader, sizeof(fileheader), 1, bitmapfile) < sizeof(fileheader)) {
        fclose(bitmapfile);
        return newbitmap;
    }
    //Lese Infoheader
    bmp_info_header_t infoheader;
    if (fread(&infoheader, sizeof(infoheader), 1, bitmapfile) < sizeof(infoheader)) {
        fclose(bitmapfile);
        return newbitmap;
    }
    //Lese Farbtabelle (wenn vorhanden)
    bmp_rgbquad_t *color_table = 0;
    switch (infoheader.bitcount) {
        case 1:
            color_table = malloc(2 * sizeof(bmp_rgbquad_t));
            if (fread(color_table, 2 * sizeof(bmp_rgbquad_t), 1, bitmapfile) < 2 * sizeof(bmp_rgbquad_t)) {
                fclose(bitmapfile);
                free(color_table);
                return newbitmap;
            }
            break;
        case 4:
            color_table = malloc(16 * sizeof(bmp_rgbquad_t));
            if (fread(color_table, 16 * sizeof(bmp_rgbquad_t), 1, bitmapfile) < 16 * sizeof(bmp_rgbquad_t)) {
                fclose(bitmapfile);
                free(color_table);
                return newbitmap;
            }
            break;
        case 8:
            color_table = malloc(256 * sizeof(bmp_rgbquad_t));
            if (fread(color_table, 256 * sizeof(bmp_rgbquad_t), 1, bitmapfile) < 256 * sizeof(bmp_rgbquad_t)) {
                fclose(bitmapfile);
                free(color_table);
                return newbitmap;
            }
            break;
        case 24:
            //Keine Farbtabelle vorhanden!
            break;
        default:
            fclose(bitmapfile);
            return newbitmap;
    }
    //Lese und konvertiere Daten
    newbitmap.data = malloc(infoheader.width * infoheader.height * sizeof(dword));
    if (!newbitmap.data) {
        fclose(bitmapfile);
        if (color_table) free(color_table);
        return newbitmap;
    }
    newbitmap.width = infoheader.width;
    newbitmap.height = infoheader.height;
    
    dword linesize = infoheader.bitcount * newbitmap.width;
    byte *tmpdata = malloc(linesize / 8);
    if (linesize & 0x1F) linesize = (linesize & 0xFFFFFE0) + 0x20;
    int y;
    int x;
    switch (infoheader.bitcount) {
        case 1:
            break;
        case 4:
            break;
        case 8:
            break;
        case 24:
            printf("[ GUI ] Lese 24-bit BMP.\n");
            for (y = 0; y < newbitmap.height; y++) {
                //Zeile einlesen
                if (fread(tmpdata, linesize / 8, 1, bitmapfile) < linesize / 8) {
                    //Datei zu klein!
                    printf("[ GUI ] BMP-Datei zu klein!\n");
                }
                //Zeile analysieren
                for (x = 0; x < newbitmap.width; x++) {
                    newbitmap.data[x + (newbitmap.height - y - 1) * newbitmap.width] =
                            (tmpdata[x * 3] << 16) + (tmpdata[x * 3 + 1] << 8)
                            + (tmpdata[x * 3 + 2]) + 0xFF000000;
                }
            }
            break;
        default:
            fclose(bitmapfile);
            if (color_table) free(color_table);
            return newbitmap;
    }
    
    return newbitmap;
}
