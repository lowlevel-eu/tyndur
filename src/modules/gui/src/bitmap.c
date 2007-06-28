
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
