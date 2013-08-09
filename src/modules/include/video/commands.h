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

/// Kommandos

enum video_commands {
    VIDEO_CMD_NOP = 0,
    // Kontextsteuerung

    // Erzeugt einen neuen Treiberkontext
    VIDEO_CMD_CREATE_CONTEXT,
    // Setzt den aktiven Treiberkontext
    VIDEO_CMD_USE_CONTEXT,
    // Setzt das Geraet (per Nummer) im aktiven Treiberkontext
    VIDEO_CMD_USE_DEVICE,
    // Setzt das aktive Ziel (Bitmap) im aktiven Treiberkontext
    VIDEO_CMD_USE_TARGET,
    // Setzt die aktive Zeichenfarbe im aktiven Treiberkontext
    VIDEO_CMD_USE_COLOR,
    // Setzt die aktive Raster-OP im aktiven Treiberkontext
    VIDEO_CMD_USE_ROP,
    // Fordert einen Kommandobuffer fuer den Treiberkontext an.
    VIDEO_CMD_GET_CMD_BUFFER,
    // Fordert die Ausfuehrung des Buffers an.
    VIDEO_CMD_DO_CMD_BUFFER,

    // Geräteabfrage

    // Fragt die Anzahl Geraete ab
    VIDEO_CMD_GET_NUM_DEVICES,
    // Fragt die Anzahl der Anzeigen am Geraet ab
    VIDEO_CMD_GET_NUM_DISPLAYS,
    // Fragt die Displaymodi ab
    VIDEO_CMD_GET_DISPLAY_MODES,

    // Anderes

    // Setzt den Modus
    // FIXME Umbenennen (DONE?)
    VIDEO_CMD_SET_MODE,
    // Setzt einen Textmodus
    VIDEO_CMD_RESTORE_TEXT_MODE,

    // Bitmaps
    // Erzeugt ein Bitmap
    VIDEO_CMD_CREATE_BITMAP,
    // Zerstoert ein Bitmap
    VIDEO_CMD_DESTROY_BITMAP,
    // Erzeugt eine Bitmap auf den Display-Frontbuffer
    VIDEO_CMD_CREATE_FRONTBUFFER_BITMAP,
    // Fordert Backbuffer an
    VIDEO_CMD_REQUEST_BACKBUFFERS,
    // Tauscht die Zeichenbuffer
    VIDEO_CMD_FLIP_BUFFERS,
    // Fordert das Format des Bitmaps an
    VIDEO_CMD_FETCH_FORMAT,
    // Fordert den SHM-Buffer zum Bitmap-Upload an
    VIDEO_CMD_GET_UPLOAD_BUFFER,
    // Schliesst den SHM-Buffer
    VIDEO_CMD_CLOSE_UPLOAD_BUFFER,
    // Laedt den aktuellen Bitmapbuffer aus SHM-Bereichen neu
    VIDEO_CMD_UPLOAD_BITMAP,

    // Primitiven Zeichnen
    VIDEO_CMD_DRAW_PIXEL,
    VIDEO_CMD_DRAW_RECTANGLE,
    VIDEO_CMD_DRAW_ELLIPSE,
    VIDEO_CMD_DRAW_LINE,
    VIDEO_CMD_DRAW_BITMAP,
    VIDEO_CMD_DRAW_BITMAP_PART,

    VIDEO_CMD_LATEST
};

// Datenstrukturen fuer RPC-Kommandos.

#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint32_t command;
} rpc_data_cmd_header_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t context_id;
} rpc_data_cmd_use_context_t;


typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t device_id;
} rpc_data_cmd_use_device_t;


typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t target_id;
} rpc_data_cmd_use_target_t;


typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t raster_op;
} rpc_data_cmd_use_rop_t;


typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t buffer_size;
} rpc_data_cmd_get_cmd_buffer_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t buffer_size;
} rpc_data_cmd_do_cmd_buffer_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rpc_data_cmd_use_color_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t width;
    uint32_t height;
    uint32_t data_length;
} rpc_data_cmd_create_bitmap_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t display_id;
} rpc_data_cmd_create_frontbuffer_bitmap_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t bitmap_id;
} rpc_data_cmd_destroy_bitmap_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t bitmap_id;
    uint32_t backbuffer_count;
} rpc_data_cmd_request_backbuffers_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t bitmap_id;
} rpc_data_cmd_flip_buffers_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t bitmap_id;
} rpc_data_cmd_fetch_format_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t bitmap_id;
} rpc_data_cmd_upload_bitmap_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t bitmap_id;
} rpc_data_cmd_get_upload_buffer_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t bitmap_id;
} rpc_data_cmd_close_upload_buffer_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t device_id;
} rpc_data_cmd_get_num_displays_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t display;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t refreshrate;
} rpc_data_cmd_set_mode_t;

typedef struct {
    rpc_data_cmd_header_t header;
    uint32_t device_id;
    uint32_t display_id;
} rpc_data_cmd_get_display_modes_t;

typedef struct {
    rpc_data_cmd_header_t header;
    int x;
    int y;
} rpc_data_cmd_draw_pixel_t;

typedef struct {
    rpc_data_cmd_header_t header;
    int x;
    int y;
    int width;
    int height;
} rpc_data_cmd_draw_rectangle_t;

typedef struct {
    rpc_data_cmd_header_t header;
    int x1;
    int y1;
    int x2;
    int y2;
} rpc_data_cmd_draw_line_t;

typedef struct {
    rpc_data_cmd_header_t header;
    int x;
    int y;
    int width;
    int height;
} rpc_data_cmd_draw_ellipse_t;

typedef struct {
    rpc_data_cmd_header_t header;
    int dst_x;
    int dst_y;
    int src_bitmap_id;
    int src_x;
    int src_y;
    int width;
    int height;
} rpc_data_cmd_draw_bitmap_part_t;

typedef struct {
    rpc_data_cmd_header_t header;
    int dst_x;
    int dst_y;
    int src_bitmap_id;
} rpc_data_cmd_draw_bitmap_t;

#pragma pack(pop)
