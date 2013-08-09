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

#ifndef __LIBVIDEO_VIDEO_H__
#define __LIBVIDEO_VIDEO_H__

#include <types.h>
#include <rpc.h>

#define VIDEORPC_GET_DWORD(size, data) \
    rpc_get_dword(context->driverpid, "VIDEODRV", size, data)

/*
#define REQUIRE_CMD_BUFFER(length) \
    do { \
    if (context->cmdbuffer == 0) return -1; \
    if (context->cmdbufferlen < context->cmdbufferpos + length*4) \
        libvideo_do_command_buffer(); \
    } while (0);

#define REQUIRE_CMD_BUFFER_RETURN_VOID(length) \
    do { \
    if (context->cmdbuffer == 0) return; \
    if (context->cmdbufferlen < context->cmdbufferpos + length*4) \
        libvideo_do_command_buffer(); \
    } while (0);

#define SET_CMD_BUFFER(value) \
    *(uint32_t*) (context->cmdbuffer + (context->cmdbufferpos )) = value; \
    context->cmdbufferpos += 4
  */

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t refreshrate;
} video_mode_t;


typedef struct {
    pid_t driverpid;
    uint32_t drivercontext;
    uint8_t* cmdbuffer;
    uint32_t cmdbufferpos;
    uint32_t cmdbufferlen;
    uint32_t cmdbufferid;
} driver_context_t;

typedef struct {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t upload_buffer_id;
    uint32_t upload_buffer_size;
    void* upload_buffer_ptr;
} video_bitmap_t;

// FIXME: Muss immer mit CDI (cdi/video.h) abgeglichen werden!
// struct cdi_video_pixel_format
typedef struct {
    uint8_t bpp;
    uint8_t red_bits;
    uint8_t red_offset;
    uint8_t green_bits;
    uint8_t green_offset;
    uint8_t blue_bits;
    uint8_t blue_offset;
    uint8_t alpha_bits;
    uint8_t alpha_offset;
} video_pixel_format_t;

// FIXME: Muss immer mit CDI (cdi/video.h) abgeglichen werden!
// enum cdi_video_raster_op
typedef enum {
    VIDEO_ROP_COPY,
    VIDEO_ROP_OR,
    VIDEO_ROP_AND,
    VIDEO_ROP_XOR,
    // TODO: Erweitern
} rop_t;

/// Kontextsteuerung

/**
 * Erstellt einen neuen Kontext fuer den angebenen Treiber
 * @param driver String, Name des Treibers (= Prozessname)
 * @return Zeiger auf Kontext. NULL im Fehlerfall.
 */
driver_context_t* libvideo_create_driver_context(char* driver);

/**
 * Setzt den aktuell aktiven Kontext
 * @param context Zeiger auf den zu aktivierenden Kontext.
 */
void libvideo_use_driver_context(driver_context_t* context);

/// Kontextabhaengig

/**
 * Setzt das aktive Geraet im aktuellen Kontext
 * @param devicenum Geraetenummer.
 * @return 0 bei Erfolg, sonst -1
 */
int libvideo_change_device(int devicenum);

/**
 * Setzt das aktive Ziel(bitmap) im aktuellen Kontext
 * @param bitmap Zeiger auf Bitmap.
 * @return 0 bei Erfolg, sonst -1
 */
int libvideo_change_target(video_bitmap_t *bitmap);

/**
 * Setzt die Zeichenfarbe im aktuellen Kontext
 * @param alpha Alpha-Anteil (0-255)
 * @param red Rot-Anteil (0-255)
 * @param green Gruen-Anteil (0-255)
 * @param blue Blau-Anteil (0-255)
 * @return 0 bei Erfolg, sonst -1
 */
int libvideo_change_color(char alpha, char red, char green, char blue);

/**
 * Setzt die Raster-OP fuer weitere Zeichenvorgaenge im aktuellen Kontext
 * @param rop Die Raster-OP
 * @return 0 bei Erfolg, sonst -1
 */
int libvideo_change_rop(rop_t rop);

/**
 * Fordert einen neuen Kommandobuffer fuer den Kontext an.
 * @param length Die Laenge des Zeichenbuffers in Bytes.
 * @return 0 bei Erfolg, sonst -1
 */
int libvideo_get_command_buffer(size_t length);

/**
 * Gibt dem Treiber die Anweisung den Buffer auszufuehren.
 * Achtung, dies wird auch automatisch getan wenn der Buffer voll ist!
 * Achtung, nicht nach jedem Zeichenvorgang aufrufen - kostet Geschwindigkeit!
 * @return 0 bei Erfolg, sonst -1
 */
int libvideo_do_command_buffer(void);

/// Geraeteabfrage

/**
 * Gibt die Anzahl der Geraete am Treiber des aktuellen Kontexts zurueck.
 * @return die Anzahl der Geraete des Treibers.
 */
int libvideo_get_num_devices(void);

/**
 * Gibt die Zahl der Anzeigen am angegebenen Geraete im aktuellen Kontext zurueck
 * @param device Geraetenummer
 * @return Anzahl der Anzeigen am Grafikadapter
 */
int libvideo_get_num_displays(int device);

/**
 * Liefert ein Array mit Modusinformationen
 * @param device Geraetenummer
 * @param display Anzeigennummer
 * @param modeptr Zeiger auf video_mode_t-Arrayzeiger. Kann NULL sein.
 *                Buffer muss freigegeben werden.
 * @return Anzahl der vorhandenen Modi
 */
uint32_t libvideo_get_modes(int device, int display, video_mode_t** modeptr);

/// Bitmaps
/**
 * Erzeugt ein Bitmap, laedt ggf. Initialisierungsdaten zum Treiber.
 * Ohne Initialisierung erhaelt man ein schwarzes Bitmap.
 * @param width Breite in Pixel
 * @param height Hoehe in Pixel
 * @param length Laenge der Nutzdaten, 0 fuer keine.
 * @param data Zeiger auf Datenbuffer, Pixelformat wird als 32-bit ARGB angenommen
 */
video_bitmap_t* libvideo_create_bitmap(int width, int height,
    size_t data_length, void* data);

/**
 * Gibt ein Bitmap wieder frei.
 * @param bitmap Das Bitmap.
 */
void libvideo_destroy_bitmap(video_bitmap_t* bitmap);

/**
 * Tauscht die Zeichenbuffer aus
 * @param bitmap Das Bitmap
 */
void libvideo_flip_buffers(video_bitmap_t* bitmap);

/**
 * Fordert Backbuffer fuer ein Bitmap an
 * @param bitmap Das Bitmap
 * @param backbuffer_count Die Anzahl der gewuenschten Backbuffer
 * @return Die Anzahl der erhaltenen Backbuffer
 */
uint32_t libvideo_request_backbuffers(video_bitmap_t* bitmap,
    uint32_t backbuffer_count);

/**
 * Laedt ein Bitmap hoch (d.h., der Inhalt des Uploadbuffers wird uebernommen)
 * @param bitmap Das Bitmap
 * @return Fehlercode
 */
uint32_t libvideo_upload_bitmap(video_bitmap_t* bitmap);

/**
 * Schliesst den Uploadbuffer für das Bitmap
 * @param bitmap Das Bitmap
 * @return Fehlercode
 */
uint32_t libvideo_close_upload_buffer(video_bitmap_t* bitmap);

/**
 * Oeffnet einen Uploadbuffer für das Bitmap
 * @param bitmap Das Bitmap
 * @param buffer_size Zeiger auf Buffergroesse.
 * @return Zeiger auf den Buffer (Format beachten!)
 * @see libvideo_fetch_bitmap_format
 */
void* libvideo_open_upload_buffer(video_bitmap_t* bitmap, size_t* buffer_size);

/**
 * Holt das Pixelformat des Bitmaps
 * @param bitmap Das Bitmap
 * @return Zeiger auf Pixelformat - muss freigegeben werden!
 */
video_pixel_format_t* libvideo_fetch_bitmap_format(video_bitmap_t* bitmap);

/**
 * Holt ein Bitmap-Objekt fuer den Anzeigenfrontbuffer
 * @param display Die Anzeige deren Frontbuffer einen interessiert
 * @return Zeiger auf Bitmap, NULL im Fehlerfall
 * @note Es kann global nur eine Bitmap fuer Frontbufferzugriff geben!
 * @note Vor erneutem libvideo_set_mode oder libvideo_restore_text_mode muss das
 *  Frontbuffer-Bitmap zerstoert werden (libvideo_destroy_bitmap).
 */
video_bitmap_t* libvideo_get_frontbuffer_bitmap(int display);

/// Primitiven Zeichnen

/**
 * Zeichnet einen Pixel in der gesetzten Farbe auf das aktuelle Ziel.
 * @param x X-Koordinate
 * @param y Y-Koordinate
 */
void libvideo_draw_pixel(int x, int y);

/**
 * Zeichnet einen Rechteck in der gesetzten Farbe auf das aktuelle Ziel.
 * @param x X-Koordinate
 * @param y Y-Koordinate
 * @param width Breite in Pixel
 * @param height Hoehe in Pixel
 */
void libvideo_draw_rectangle(int x, int y, int width, int height);

/**
 * Zeichnet eine Ellipse (die das spezifizierte Rechteck bestmoeglich ausfuellt)
 * in der gesetzten Farbe auf das aktuelle Ziel.
 * @param x X-Koordinate
 * @param y Y-Koordinate
 * @param width Breite in Pixel
 * @param height Hoehe in Pixel
 */
void libvideo_draw_ellipse(int x, int y, int width, int height);


/**
 * Zeichnet eine Linie in der gesetzten Farbe auf das aktuelle Ziel.
 * @param x1 Start-X-Koordinate
 * @param y1 Start-Y-Koordinate
 * @param x2 End-X-Koordinate
 * @param y2 End-Y-Koordinate
 */
void libvideo_draw_line(int x1, int y1, int x2, int y2);

/**
 * Zeichnet eine Bitmap auf das aktuelle Ziel.
 * @param bitmap die Quell-Bitmap
 * @param x X-Koordinate
 * @param y Y-Koordinate
 */
void libvideo_draw_bitmap(video_bitmap_t *bitmap, int x, int y);

/**
 * Zeichnet einen Bitmapausschnitt auf das aktuelle Ziel.
 * @param bitmap die Quell-Bitmap
 * @param x Ziel-X-Koordinate
 * @param y Ziel-Y-Koordinate
 * @param srcx Start-X-Koordinate des Quellrechtecks
 * @param srcy Start-Y-Koordinate des Quellrechtecks
 * @param width Breite des zu kopierenden Rechtecks in Pixel
 * @param height Hoehe des zu kopierenden Rechtecks in Pixel
 */
void libvideo_draw_bitmap_part(video_bitmap_t *bitmap, int x, int y, int srcx,
    int srcy, int width, int height);

/// Anderes

/**
 * Setzt die Aufloesung einer Anzeige
 * @param display Die Anzeige
 * @param width Breite
 * @param height Hoehe
 * @param depth Gewuenschte Farbtiefe (in BPP)
 * @return 0 bei Erfolg, sonst -1
 * @note Die Farbtiefe ist nur ein Richtwert - es wird gegebenfalls eine andere
 *  verwendet, allerdings werden die Farbwerte transparent umgerechnet.
 */
int libvideo_change_display_resolution(int display, int width, int height,
    int depth);

int libvideo_restore_text_mode(void);

#endif /* __LIBVIDEO_VIDEO_H__ */
