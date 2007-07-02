/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 
#ifndef PCF_H
#define PCF_H

#include "font.h"

#define PCF_PROPERTIES          (1<<0)
#define PCF_ACCELERATORS        (1<<1)
#define PCF_METRICS             (1<<2)
#define PCF_BITMAPS             (1<<3)
#define PCF_INK_METRICS         (1<<4)
#define PCF_BDF_ENCODINGS       (1<<5)
#define PCF_SWIDTHS             (1<<6)
#define PCF_GLYPH_NAMES         (1<<7)
#define PCF_BDF_ACCELERATORS    (1<<8)

#define PCF_DEFAULT_FORMAT      0x00000000
#define PCF_INKBOUNDS           0x00000200
#define PCF_ACCEL_W_INKBOUNDS   0x00000100
#define PCF_COMPRESSED_METRICS  0x00000100

#define PCF_GLYPH_PAD_MASK  (3<<0)      /* See the bitmap table for explanation */
#define PCF_BYTE_MASK       (1<<2)      /* If set then Most Sig Byte First */
#define PCF_BIT_MASK        (1<<3)      /* If set then Most Sig Bit First */
#define PCF_SCAN_UNIT_MASK  (3<<4)      /* See the bitmap table for explanation */

typedef struct pcf_toc_entry_t
{
    int type;
    int format;
    int size;
    int offset;
} __attribute__((__packed__)) pcf_toc_entry_t;

typedef struct pcf_file_header_t
{
    byte header[4];
    int table_count;
    pcf_toc_entry_t *toc;
} __attribute__((__packed__)) pcf_file_header_t;

typedef struct pcf_property_t
{
    int name_offset;
    byte is_string_prop;
    int value;
} __attribute__((__packed__)) pcf_property_t;

typedef struct pcf_property_table_t
{
    int format;
    int prop_count;
    pcf_property_t *props;
    int string_size;
    char *strings;
} __attribute__((__packed__)) pcf_property_table_t;

typedef struct pcf_compressed_metrics_t
{
    byte left_sided_bearing;
    byte right_sided_bearing;
    byte character_width;
    byte character_ascent;
    byte character_descent;
} __attribute__((__packed__)) pcf_compressed_metrics_t;

typedef struct pcf_metrics_t
{
    sword left_sided_bearing;
    sword right_sided_bearing;
    sword character_width;
    sword character_ascent;
    sword character_descent;
    word character_attributes;
} __attribute__((__packed__)) pcf_metrics_t;

typedef struct pcf_accelerator_table_t
{
    int format;
    byte no_overlap;
    byte constant_metrics;
    byte terminal_font;
    byte constant_width;
    byte ink_inside;
    byte ink_metrics;
    byte draw_direction;
    byte padding;
    int font_ascent;
    int font_descent;
    int max_overlap;
    pcf_metrics_t minbounds;
    pcf_metrics_t maxbounds;
} __attribute__((__packed__)) pcf_accelerator_table_t;

typedef struct pcf_accelerator_table_w_inkbounds_t
{
    pcf_accelerator_table_t accelerator_table;
    pcf_metrics_t ink_minbounds;
    pcf_metrics_t ink_maxbounds;
} __attribute__((__packed__)) pcf_accelerator_table_w_inkbounds_t;

typedef struct pcf_metrics_table_t
{
    int format;
    dword metrics_count;
    pcf_metrics_t *metrics;
} __attribute__((__packed__)) pcf_metrics_table_t;

typedef struct pcf_compressed_metrics_table_t
{
    int format;
    word metrics_count;
    pcf_compressed_metrics_t *metrics;
} __attribute__((__packed__)) pcf_compressed_metrics_table_t;

typedef struct pcf_bitmap_table_t
{
    int format;
    int glyph_count;
    int *offsets;
    int bitmap_sizes[4];
    byte *bitmap_data;
} __attribute__((__packed__)) pcf_bitmap_table_t;

typedef struct pcf_encoding_table_t
{
    int format;
    sword min_char_or_byte2;
    sword max_char_or_byte2;
    sword min_byte1;
    sword max_byte1;
    sword default_char;
    sword *glyphindeces;
} __attribute__((__packed__)) pcf_encoding_table_t;

typedef struct pcf_swidths_table_t
{
    int format;
    int glyph_count;
    int *swidths;
} __attribute__((__packed__)) pcf_swidths_table_t;

typedef struct pcf_glyph_name_table_t
{
    int format;
    int glyph_count;
    int *offsets;
    int string_size;
    char *string;
} __attribute__((__packed__)) pcf_glyph_name_table_t;

font_t load_pcf_font(char *filename);

#endif
