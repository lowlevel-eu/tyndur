/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Mathias Gottschlag.
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
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include "fonts.h"

#include "stdlib.h"

FT_Library freetype;

list_t *fonts;

dword last_data_id = 0;

int init_fonts(void)
{
    printf("[ GUI ] Initialisiere Freetype.\n");
    int error = FT_Init_FreeType(&freetype);
    if ( error ) {
        printf("[ GUI ] Konnte FreeType 2 nicht initialisieren (%d)!\n", error);
        return 0;
    }
    fonts = list_create();
    return 1;
}

server_font_t *load_font(const char *name)
{
    //TODO: Nach Font suchen, auch andere Formate?
    char filename[256];
    snprintf(filename, 256, "floppy:/devices/fd0|fat:/data/%s.ttf", name);
    //Datei in den RAM laden
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("[ GUI ] Konnte Font \"%s\" nicht oeffnen.\n", name);
        return 0;
    }
	  fseek(file, 0, SEEK_END);
	  int filesize = ftell(file);
	  fseek(file, 0, SEEK_SET);
    byte *data = malloc(filesize);
    dword position = 0;
    while ((filesize - position) > 30000) {
        fread(data + position, sizeof(char), 30000, file);
        position += 30000;
    }
  	if ((filesize - position) != fread(data + position, sizeof(char), filesize - position, file)) {
		    free(data);
		    printf("Konnte Font \"%s\" nicht lesen.\n", name);
  		  return 0;
  	}
  	fclose(file);
    
    //
    server_font_t *newfont = malloc(sizeof(server_font_t));
    memset(newfont, 0, sizeof(server_font_t));
    //int error = FT_New_Face(freetype, filename, 0, &newfont->face);
    int error = FT_New_Memory_Face(freetype, data, filesize, 0, &newfont->face);
    if (error) {
        printf("Konnte Font \"%s\" nicht laden (%d)!\n", name, error);
        return 0;
    }
    newfont->name = strdup(name);
    newfont->data = list_create();
    list_push(fonts, newfont);
    //free(data);
    return newfont;
}

int load_font_data(const char *name, int size)
{
    //Font suchen und evtl laden
    server_font_t *font = 0;
    int i;
    for (i = 0; i < list_size(fonts); i++) {
        server_font_t *tmpfont = (server_font_t*)list_get_element_at(fonts, i);
        if (!strcmp(tmpfont->name, name)) {
            font = tmpfont;
            break;
        }
    }
    if (!font) {
        font = load_font(name);
    }
    if (!font) {
        return 0;
    }
    //Größe suchen
    for (i = 0; i < list_size(font->data); i++) {
        server_font_data_t *data = (server_font_data_t*)list_get_element_at(font->data, i);
        if (data->size == size) {
            return 1;
        }
    }
    //Noch nicht vorhanden, muss erstellt werden
    server_font_data_t *newdata = malloc(sizeof(server_font_data_t));
    memset(newdata, 0, sizeof(server_font_data_t));
    newdata->use_count = 0;
    newdata->size = size;
    newdata->id = ++last_data_id;
    
    int glyphindex[256];
    for (i = 0; i < 256; i++) {
        glyphindex[i] = FT_Get_Char_Index(font->face, i);
    }
    
    int error = FT_Set_Char_Size(font->face, 0, size * 64, 0, 0);
    if (error) {
        return 0;
    }
    
    int glyph_count = font->face->num_glyphs;
    glyph_t *glyphs = malloc(glyph_count * sizeof(glyph_t));
    memset(glyphs, 0, glyph_count * sizeof(glyph_t));
    FT_GlyphSlot slot = font->face->glyph;
    for (i = 0; i < glyph_count; i++) {
        //printf("[ GUI ] Rendere Font: %d/%d.\n", i, glyph_count);
        /*error = FT_Load_Glyph(font->face, i, 0);
        if (error) {
            printf("Konnte nicht geladen werden :|\n");
            continue;
        }
        error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        if (error) {
            printf("Konnte nicht geerendert werden :|\n");
            continue;
        }*/
        int error = FT_Load_Char(font->face, i, FT_LOAD_RENDER);
        if (error) {
            printf("Zeichen konnte nicht gerendert werden.\n");
            continue;
        }
        //printf("Zeichen gerendert.\n");
        //Erstelle Zeichen
        glyphs[i].width = slot->metrics.width;
        glyphs[i].height = slot->metrics.height;
        glyphs[i].bearing_x = slot->metrics.horiBearingX;
        glyphs[i].bearing_y = slot->metrics.horiBearingY;
        glyphs[i].advance = slot->metrics.horiAdvance;
        glyphs[i].bitmap.bpp = 8;
        glyphs[i].bitmap.width = slot->bitmap.width;
        glyphs[i].bitmap.height = slot->bitmap.rows;
        glyphs[i].bitmap.data = malloc(slot->bitmap.width * slot->bitmap.rows);
        memcpy(glyphs[i].bitmap.data, slot->bitmap.buffer,
            slot->bitmap.width * slot->bitmap.rows);
    }
    //Das ganze in eine andere von den Clients lesbare Form bringen
    //Größe ausrechnen:
    dword datasize = 256 * 4 + 4; //Zuordnung Char->Index + Anzahl Zeichen
    datasize += glyph_count * 4; //Offset der Zeichen
    for (i = 0; i < glyph_count; i++) {
        int glyph_size = 9 * 4 + glyphs[i].bitmap.width * glyphs[i].bitmap.height;
        if (glyph_size % 4) {
            glyph_size = (glyph_size & ~0x3) + 4;
        }
        datasize += glyph_size;
    }
    //Speicher anlegen
    newdata->shmid = create_shared_memory(datasize);
    newdata->data = open_shared_memory(newdata->shmid);
    //Ausfüllen
    int *data = (int*)newdata->data;
    memcpy(data, glyphindex, 256 * 4);
    data[256] = glyph_count;
    int offset = 257 * 4 + glyph_count * 4;
    for (i = 0; i < glyph_count; i++) {
        data[257 + i] = offset;
        //printf("Offset: %d\n", offset);
        int glyph_size = 9 * 4 + glyphs[i].bitmap.width * glyphs[i].bitmap.height;
        int *tmpdata = (int*)(newdata->data + offset);
        tmpdata[0] = glyphs[i].width;
        tmpdata[1] = glyphs[i].height;
        tmpdata[2] = glyphs[i].bearing_x;
        tmpdata[3] = glyphs[i].bearing_y;
        tmpdata[4] = glyphs[i].advance;
        tmpdata[5] = glyphs[i].bitmap.width;
        tmpdata[6] = glyphs[i].bitmap.height;
        tmpdata[7] = glyphs[i].bitmap.bpp;
        memcpy(&tmpdata[8], glyphs[i].bitmap.data, tmpdata[5] * tmpdata[6]);
        if (glyph_size % 4) {
            glyph_size = (glyph_size & ~0x3) + 4;
        }
        offset += glyph_size;
        free(glyphs[i].bitmap.data);
    }
    //Aufräumen
    free(glyphs);
    list_push(font->data, newdata);
    return 1;
}
server_font_data_t *get_font_data(const char *name, int size)
{
    //Font suchen und evtl laden
    server_font_t *font = 0;
    int i;
    for (i = 0; i < list_size(fonts); i++) {
        server_font_t *tmpfont = (server_font_t*)list_get_element_at(fonts, i);
        if (!strcmp(tmpfont->name, name)) {
            font = tmpfont;
            break;
        }
    }
    if (!font) {
        font = load_font(name);
    }
    if (!font) {
        return 0;
    }
    //Größe suchen
    for (i = 0; i < list_size(font->data); i++) {
        server_font_data_t *data = (server_font_data_t*)list_get_element_at(font->data, i);
        if (data->size == size) {
            return data;
        }
    }
    load_font_data(name, size);
    for (i = 0; i < list_size(font->data); i++) {
        server_font_data_t *data = (server_font_data_t*)list_get_element_at(font->data, i);
        if (data->size == size) {
            return data;
        }
    }
    return 0;
}
server_font_data_t *get_font_data_by_id(dword id)
{
    int i, j;
    for (i = 0; i < list_size(fonts); i++) {
        server_font_t *font = (server_font_t*)list_get_element_at(fonts, i);
        for (j = 0; j < list_size(font->data); j++) {
            server_font_data_t *data = (server_font_data_t*)list_get_element_at(font->data, j);
            if (data->id == id) {
                return data;
            }
        }
    }
    return 0;
}
server_font_data_t *use_font_data(dword id)
{
    server_font_data_t *data = get_font_data_by_id(id);
    if (!data) {
        return 0;
    }
    data->use_count++;
    return data;
}
void drop_font_data(dword id)
{
    server_font_data_t *data = get_font_data_by_id(id);
    if (!data) {
        return;
    }
    data->use_count--;
    if (data->use_count <= 0) {
        //Größe wird nicht mehr benötigt, weg damit?
    }
}
