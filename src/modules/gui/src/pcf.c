
#include "pcf.h"

#include "stdio.h"
#include "stdlib.h"

//Big-Endian nach Little-Endian
int convert_be_to_le(int number)
{
    int value;
    byte *inputdata = (byte*)&number;
    byte *outputdata = (byte*)&value;
    outputdata[0] = inputdata[3];
    outputdata[1] = inputdata[2];
    outputdata[2] = inputdata[1];
    outputdata[3] = inputdata[0];
    return value;
}
word convert_be_to_le_short(word number)
{
    word value;
    byte *inputdata = (byte*)&number;
    byte *outputdata = (byte*)&value;
    outputdata[0] = inputdata[1];
    outputdata[1] = inputdata[0];
    return value;
}

pcf_metrics_t convert_metrics(pcf_metrics_t metrics)
{
    pcf_metrics_t output;
    output.left_sided_bearing = convert_be_to_le(metrics.left_sided_bearing);
    output.right_sided_bearing = convert_be_to_le(metrics.right_sided_bearing);
    output.character_width = convert_be_to_le(metrics.character_width);
    output.character_ascent = convert_be_to_le(metrics.character_ascent);
    output.character_descent = convert_be_to_le(metrics.character_descent);
    return output;
}

pcf_compressed_metrics_t convert_compressed_metrics(pcf_compressed_metrics_t metrics)
{
    pcf_compressed_metrics_t output;
    output.left_sided_bearing = convert_be_to_le_short(metrics.left_sided_bearing);
    output.right_sided_bearing = convert_be_to_le_short(metrics.right_sided_bearing);
    output.character_width = convert_be_to_le_short(metrics.character_width);
    output.character_ascent = convert_be_to_le_short(metrics.character_ascent);
    output.character_descent = convert_be_to_le_short(metrics.character_descent);
    return output;
}

font_t load_pcf_font(char *filename)
{
    font_t newfont;
    newfont.glyphcount = 0;
    newfont.name = "";
    newfont.height = 0;
    newfont.glyphcount = 0;
    newfont.glyphs = 0;
    FILE *fontfile = fopen(filename, "r");
    if (!fontfile) {
        printf("[ GUI ] Kann Font-Datei nicht oeffnen!\n");
        return newfont;
    }

    //Lese Dateiheader
    pcf_file_header_t fileheader;
    if (fread(&fileheader, 8, 1, fontfile) < 8) {
        fclose(fontfile);
        return newfont;
    }
    fileheader.toc = malloc(fileheader.table_count * sizeof(pcf_toc_entry_t));
    if (fread(fileheader.toc, fileheader.table_count * sizeof(pcf_toc_entry_t),
        1, fontfile)< fileheader.table_count * sizeof(pcf_toc_entry_t)) {
        fclose(fontfile);
        free(fileheader.toc);
        return newfont;
    }
    
    //Lese Tabellen
    void **tables = malloc(sizeof(void*) * fileheader.table_count);
    memset(tables, 0, sizeof(void*) * fileheader.table_count);
    
    //printf("[ GUI ] Lese Tabellen!\n");
    
    int i;
    for (i = 0; i < fileheader.table_count; i++) {
        int big_endian = 0;
        if (fileheader.toc[i].format & PCF_BYTE_MASK) big_endian = 1;
        switch (fileheader.toc[i].type)
        {
            case PCF_PROPERTIES:
                //printf("[ GUI ] Lese Eigenschaftentabelle (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                tables[i] = malloc(sizeof(pcf_property_table_t));
                if (tables[i]) {
                    pcf_property_table_t *table = tables[i];
                    fread(table, 8, 1, fontfile);
                    if (big_endian) table->prop_count = convert_be_to_le(table->prop_count);
                    //printf("[ GUI ] Eigenschaftenanzahl: %d\n", table->prop_count);
                    table->props = malloc(table->prop_count * sizeof(pcf_property_t));
                    fread(table->props, table->prop_count * sizeof(pcf_property_t), 1, fontfile);
                    int j;
                    for (j = 0; j < table->prop_count; j++) {
                        table->props[j].name_offset = convert_be_to_le(table->props[j].name_offset);
                        table->props[j].value = convert_be_to_le(table->props[j].value);
                    }
                    if ((9 * table->prop_count) & 0x3) {
                        fseek(fontfile, fileheader.toc[i].offset + 8 + ((9 * table->prop_count) % 4) + 4, SEEK_SET);
                    }
                    fread(&(table->string_size), 4, 1, fontfile);
                    //printf("[ GUI ] string_size: %d\n", table->string_size);
                    if (big_endian) table->string_size = convert_be_to_le(table->string_size);
                    table->strings = malloc(table->string_size);
                    fread(table->strings, table->string_size, 1, fontfile);
                }
                break;
            case PCF_ACCELERATORS:
                //printf("[ GUI ] Lese Acceleratortabelle (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                if (fileheader.toc[i].format & PCF_ACCEL_W_INKBOUNDS) {
                    tables[i] = malloc(sizeof(pcf_accelerator_table_w_inkbounds_t));
                    if (tables[i]) {
                        pcf_accelerator_table_w_inkbounds_t *table = tables[i];
                        fread(table, sizeof(pcf_accelerator_table_w_inkbounds_t), 1, fontfile);
                        if (big_endian) {
                            table->accelerator_table.font_ascent = convert_be_to_le(table->accelerator_table.font_ascent);
                            table->accelerator_table.font_descent = convert_be_to_le(table->accelerator_table.font_descent);
                            table->accelerator_table.max_overlap = convert_be_to_le(table->accelerator_table.max_overlap);
                        }
                    }
                } else {
                    tables[i] = malloc(sizeof(pcf_accelerator_table_t));
                    if (tables[i]) {
                        pcf_accelerator_table_t *table = tables[i];
                        fread(table, sizeof(pcf_accelerator_table_t), 1, fontfile);
                        if (big_endian) {
                            table->font_ascent = convert_be_to_le(table->font_ascent);
                            table->font_descent = convert_be_to_le(table->font_descent);
                            table->max_overlap = convert_be_to_le(table->max_overlap);
                        }
                    }
                }
                break;
            case PCF_METRICS:
                //printf("[ GUI ] Lese metrics table (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                if (fileheader.toc[i].format & PCF_COMPRESSED_METRICS) {
                    tables[i] = malloc(sizeof(pcf_compressed_metrics_table_t));
                    if (tables[i]) {
                        pcf_compressed_metrics_table_t *table = tables[i];
                        fread(table, 6, 1, fontfile);
                        if (big_endian) table->metrics_count = convert_be_to_le_short(table->metrics_count);
                        //printf("[ GUI ] Komprimiert, Anzahl: %d\n", table->metrics_count);
                        table->metrics = malloc(sizeof(pcf_compressed_metrics_t) * table->metrics_count);
                        fread(table->metrics, sizeof(pcf_compressed_metrics_t) * table->metrics_count, 1, fontfile);
                        int j;
                        for (j = 0; j < table->metrics_count; j++) {
                            //table->metrics[j] = convert_compressed_metrics(table->metrics[j]);
                        }
                    }
                } else {
                    tables[i] = malloc(sizeof(pcf_metrics_table_t));
                    if (tables[i]) {
                        pcf_metrics_table_t *table = tables[i];
                        fread(table, 8, 1, fontfile);
                        if (big_endian) table->metrics_count = convert_be_to_le(table->metrics_count);
                        //printf("[ GUI ] Unkomprimiert, Anzahl: %d\n", table->metrics_count);
                        table->metrics = malloc(sizeof(pcf_metrics_t) * table->metrics_count);
                        fread(table->metrics, sizeof(pcf_metrics_t) * table->metrics_count, 1, fontfile);
                        int j;
                        for (j = 0; j < table->metrics_count; j++) {
                            table->metrics[j] = convert_metrics(table->metrics[j]);
                        }
                    }
                }
                break;
            case PCF_BITMAPS:
                //printf("[ GUI ] Lese bitmap table (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                tables[i] = malloc(sizeof(pcf_bitmap_table_t));
                if (tables[i]) {
                    pcf_bitmap_table_t *table = tables[i];
                    fread(table, 8, 1, fontfile);
                    if (big_endian) table->glyph_count = convert_be_to_le(table->glyph_count);
                    //printf("[ GUI ] Anzahl: %d\n", table->glyph_count);
                    table->offsets = malloc(table->glyph_count * 4);
                    fread(table->offsets, table->glyph_count * 4, 1, fontfile);
                    int j;
                    for (j = 0; j < table->glyph_count; j++) {
                        if (big_endian) table->offsets[j] = convert_be_to_le(table->offsets[j]);
                    }
                    fread(table->bitmap_sizes, 16, 1, fontfile);
                    for (j = 0; j < 4; j++) {
                        if (big_endian) table->bitmap_sizes[j] = convert_be_to_le(table->bitmap_sizes[j]);
                    }
                    table->bitmap_data = malloc(table->bitmap_sizes[table->format & 0x03]);
                    fread(table->bitmap_data, table->bitmap_sizes[table->format & 0x03], 1, fontfile);
                }
                break;
            case PCF_INK_METRICS:
                //printf("[ GUI ] Lese ink metrics table (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                if (fileheader.toc[i].format & PCF_COMPRESSED_METRICS) {
                    tables[i] = malloc(sizeof(pcf_compressed_metrics_table_t));
                    if (tables[i]) {
                        pcf_compressed_metrics_table_t *table = tables[i];
                        fread(table, 6, 1, fontfile);
                        if (big_endian) table->metrics_count = convert_be_to_le_short(table->metrics_count);
                        //printf("[ GUI ] Komprimiert, Anzahl: %d\n", table->metrics_count);
                        table->metrics = malloc(sizeof(pcf_compressed_metrics_t) * table->metrics_count);
                        fread(table->metrics, sizeof(pcf_compressed_metrics_t) * table->metrics_count, 1, fontfile);
                    }
                } else {
                    tables[i] = malloc(sizeof(pcf_metrics_table_t));
                    if (tables[i]) {
                        pcf_metrics_table_t *table = tables[i];
                        fread(table, 8, 1, fontfile);
                        if (big_endian) table->metrics_count = convert_be_to_le(table->metrics_count);
                        //printf("[ GUI ] Unkomprimiert, Anzahl: %d\n", table->metrics_count);
                        table->metrics = malloc(sizeof(pcf_metrics_t) * table->metrics_count);
                        fread(table->metrics, sizeof(pcf_metrics_t) * table->metrics_count, 1, fontfile);
                    }
                }
                break;
            case PCF_BDF_ENCODINGS:
                //printf("[ GUI ] Lese bdf encodings table (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                tables[i] = malloc(sizeof(pcf_encoding_table_t));
                if (tables[i]) {
                    pcf_encoding_table_t *table = tables[i];
                    fread(table, 16, 1, fontfile);
                    if (big_endian) {
                        table->max_char_or_byte2 = convert_be_to_le(table->max_char_or_byte2);
                        table->min_char_or_byte2 = convert_be_to_le(table->min_char_or_byte2);
                        table->max_byte1 = convert_be_to_le(table->max_byte1);
                        table->min_byte1 = convert_be_to_le(table->min_byte1);
                    }
                    table->glyphindeces = 
                            malloc((table->max_char_or_byte2-table->min_char_or_byte2+1)*(table->max_byte1-table->min_byte1+1) * 2);
                    fread(table->glyphindeces,
                          (table->max_char_or_byte2-table->min_char_or_byte2+1)*(table->max_byte1-table->min_byte1+1) * 2, 1, fontfile);
                }
                break;
            case PCF_SWIDTHS:
                //printf("[ GUI ] Lese bdf encodings table (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                tables[i] = malloc(sizeof(pcf_swidths_table_t));
                if (tables[i]) {
                    pcf_swidths_table_t *table = tables[i];
                    fread(table, 8, 1, fontfile);
                    if (big_endian) table->glyph_count = convert_be_to_le(table->glyph_count);
                    table->swidths = malloc(table->glyph_count * 4);
                    fread(table->swidths, table->glyph_count * 4, 1, fontfile);
                    int j;
                    for (j = 0; j < table->glyph_count; j++) {
                        if (big_endian) table->swidths[j] = convert_be_to_le(table->swidths[j]);
                    }
                }
                break;
            case PCF_GLYPH_NAMES:
                //printf("[ GUI ] Lese bdf encodings table (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                tables[i] = malloc(sizeof(pcf_glyph_name_table_t));
                if (tables[i]) {
                    pcf_glyph_name_table_t *table = tables[i];
                    fread(table, 8, 1, fontfile);
                    if (big_endian) table->glyph_count = convert_be_to_le(table->glyph_count);
                    table->offsets = malloc(table->glyph_count * 4);
                    fread(table->offsets, table->glyph_count * 4, 1, fontfile);
                    int j;
                    for (j = 0; j < table->glyph_count; j++) {
                        if (big_endian) table->offsets[j] = convert_be_to_le(table->offsets[j]);
                    }
                    fread(&(table->string_size), 4, 1, fontfile);
                    if (big_endian) table->string_size = convert_be_to_le(table->string_size);
                    table->string = malloc(table->string_size);
                    fread(table->string, table->string_size, 1, fontfile);
                }
                break;
            case PCF_BDF_ACCELERATORS:
                //printf("[ GUI ] Lese BDF Acceleratortabelle (Offset: %d)!\n", fileheader.toc[i].offset);
                fseek(fontfile, fileheader.toc[i].offset, SEEK_SET);
                if (fileheader.toc[i].format & PCF_ACCEL_W_INKBOUNDS) {
                    tables[i] = malloc(sizeof(pcf_accelerator_table_w_inkbounds_t));
                    if (tables[i]) {
                        pcf_accelerator_table_w_inkbounds_t *table = tables[i];
                        fread(table, sizeof(pcf_accelerator_table_w_inkbounds_t), 1, fontfile);
                        table->accelerator_table.font_ascent = convert_be_to_le(table->accelerator_table.font_ascent);
                        table->accelerator_table.font_descent = convert_be_to_le(table->accelerator_table.font_descent);
                        table->accelerator_table.max_overlap = convert_be_to_le(table->accelerator_table.max_overlap);
                    }
                } else {
                    tables[i] = malloc(sizeof(pcf_accelerator_table_t));
                    if (tables[i]) {
                        pcf_accelerator_table_t *table = tables[i];
                        fread(table, sizeof(pcf_accelerator_table_t), 1, fontfile);
                        table->font_ascent = convert_be_to_le(table->font_ascent);
                        table->font_descent = convert_be_to_le(table->font_descent);
                        table->max_overlap = convert_be_to_le(table->max_overlap);
                    }
                }
                break;
            default:
                break;
        }
    }
    
    //Erstelle Zeichen
    int metrics_table = -1;
    for (i = 0; i < fileheader.table_count; i++) {
        if (fileheader.toc[i].type == PCF_METRICS) {
            metrics_table = i;
            break;
        }
    }
    if (metrics_table == -1) {
        return newfont;
    }
    int bitmap_table = -1;
    for (i = 0; i < fileheader.table_count; i++) {
        if (fileheader.toc[i].type == PCF_BITMAPS) {
            bitmap_table = i;
            break;
        }
    }
    if (bitmap_table == -1) {
        return newfont;
    }
    pcf_bitmap_table_t *bitmap = tables[i];
    int glyph_count = bitmap->glyph_count;
    
    newfont.glyphcount = glyph_count;
    newfont.glyphs = malloc(sizeof(glyph_t) * glyph_count);
    int maxascent = 0;
    int maxdescent = 0;
    //TODO: Zuordnungen aus der Datei herausholen
    char characters[192] = "\000 !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[/]^_`abcdefghijklmnopqrstuvwxyz{|}~ ¡¢£¤¥¦";
    //TODO: Positionierung der Zeichen korrigieren (Überlappung etc.)
    int compressed = 0;
    pcf_compressed_metrics_table_t *cmetrics;
    pcf_metrics_table_t *metrics;
    if (fileheader.toc[metrics_table].format & PCF_COMPRESSED_METRICS) {
        compressed = 1;
        cmetrics = tables[metrics_table];
    } else {
        metrics = tables[metrics_table];
    }
    for (i = 0; i < glyph_count; i++) {
        newfont.glyphs[i].character = characters[i];
        if (compressed) {
            newfont.glyphs[i].width = cmetrics->metrics[i].character_width - 0x80;
            newfont.glyphs[i].left_sided_bearing = cmetrics->metrics[i].left_sided_bearing - 0x80;
            newfont.glyphs[i].right_sided_bearing = cmetrics->metrics[i].right_sided_bearing - 0x80;
            newfont.glyphs[i].ascent = cmetrics->metrics[i].character_ascent - 0x80;
            newfont.glyphs[i].descent = cmetrics->metrics[i].character_descent - 0x80;
        } else {
            newfont.glyphs[i].width = metrics->metrics[i].character_width;
            newfont.glyphs[i].left_sided_bearing = metrics->metrics[i].left_sided_bearing;
            newfont.glyphs[i].right_sided_bearing = metrics->metrics[i].right_sided_bearing;
            newfont.glyphs[i].ascent = metrics->metrics[i].character_ascent;
            newfont.glyphs[i].descent = metrics->metrics[i].character_descent;
        }
        if (newfont.glyphs[i].ascent > maxascent) maxascent = newfont.glyphs[i].ascent;
        if (newfont.glyphs[i].descent > maxdescent) maxdescent = newfont.glyphs[i].descent;
        //printf("Erstelle Zeichen (%d).\n", newfont.glyphs[i].width);
        int linesize = newfont.glyphs[i].width;
        if (linesize & 0x7) linesize = (linesize & 0xFFFFFFF8) + 8;
        int linesize_orig = newfont.glyphs[i].width;
        if (linesize_orig & 0x1F) linesize_orig = (linesize_orig & 0xFFFFFFE0) + 0x20;
        newfont.glyphs[i].data = malloc(linesize / 8 * (newfont.glyphs[i].ascent + newfont.glyphs[i].descent));
        byte *glyph_data = &(bitmap->bitmap_data[bitmap->offsets[i]]);
        int y;
        int x;
        for (y = 0; y < (newfont.glyphs[i].ascent + newfont.glyphs[i].descent); y++) {
            for (x = 0; x < linesize / 8; x++) {
                newfont.glyphs[i].data[y * linesize / 8 + x] = glyph_data[y * linesize_orig / 8 + x];
            }
        }
    }
    newfont.height = maxascent + maxdescent + 2;
    newfont.max_ascent = maxascent;
    newfont.max_descent = maxdescent;
    //printf("Font erstellt (%d/%d/%d).\n", newfont.height, newfont.max_ascent, newfont.max_descent);
    
    //TODO: Aufräumen!
    return newfont;
}
