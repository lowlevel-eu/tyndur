/*
 * Copyright (c) 2007-2009 Mathias Gottschlag, Janosch Graef, Alexander Siol
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_VIDEO_H_
#define _CDI_VIDEO_H_

#include "cdi.h"
#include "cdi/lists.h"

/**
 * Modus fuer Anzeigen
 */
struct cdi_video_displaymode {
    unsigned int width;
    unsigned int height;
    unsigned int depth; // In BPP
    int refreshrate; // Vsync in Hz, 0 fuer Irrelevant/Automatisch
};

enum cdi_video_bitmap_usage_hint {
  CDI_VIDEO_PRIO_HIGH,
  CDI_VIDEO_PRIO_NORMAL,
  CDI_VIDEO_PRIO_LOW,
};

struct cdi_video_pixel_format {
    uint8_t bpp;
    uint8_t red_bits;
    uint8_t red_offset;
    uint8_t green_bits;
    uint8_t green_offset;
    uint8_t blue_bits;
    uint8_t blue_offset;
    uint8_t alpha_bits;
    uint8_t alpha_offset;
};

struct cdi_video_bitmap {
    /// Aufloesung des Bitmaps - Breite
    int width;
    /// Aufloesung des Bitmaps - Hoehe
    int height;
    /// Verwendetes Pixelformat
    struct cdi_video_pixel_format format;

    /// Zeilenlaenge in Bytes. Achtung, muss nicht width * bpp sein!
    int pitch;

    /// Das Geraet zu dem das Bitmap gehoert.
    struct cdi_video_device* device;

    int buffercount;
    struct cdi_video_bitmap_buffer* buffers;
};

// TODO evtl. durch bitpacked field ersetzen?
enum cdi_video_buffer_type {
    CDI_VIDEO_BUFFER_UNUSED = 1,
    CDI_VIDEO_BUFFER_WRITE = 2,
    CDI_VIDEO_BUFFER_READ = 4,
    CDI_VIDEO_BUFFER_DISPLAY = 8,
};

struct cdi_video_bitmap_buffer {
    int in_vram;
    void* pixeldata;
    enum cdi_video_buffer_type type;
};


// TODO Wirklich notwendig? ROP-Hardware ist antik.
enum cdi_video_raster_op {
    CDI_VIDEO_ROP_COPY,
    CDI_VIDEO_ROP_OR,
    CDI_VIDEO_ROP_AND,
    CDI_VIDEO_ROP_XOR,
    // TODO: Erweitern
};

struct cdi_video_device {
    /// CDI-Geraet
    struct cdi_device dev;

    /// Liste der Bildschirme
    cdi_list_t displays;

    /// Aktuelle RasterOP (fuer Software-Helfer noetig)
    enum cdi_video_raster_op raster_op;

    /// Daten zur Moduswiederherstellung
    void* restore_data;

    /// VRAM - kann von dem Treiber nach belieben verwendet werden, eigentlich
    /// sollte kein Grund bestehen einem Treiber von außerhalb im Speicher
    /// rumzupfuschen.
    struct cdi_mem_area* vram;

    // deprecated, steht in vram->size. Wenn vram==null sollte die Situation eh klar sein.
    // /// Groesse des VRAM
    //size_t vram_size;

    /// Vorher gesetzter Textmodus
    uint8_t textmode;
};

/**
 * Eine benutzbare Anzeige - Geraet, Modus, verfuegbare Modi und der Frontbuffer
 */
struct cdi_video_display {
    /// Videogeraet, dem dieser Bildschirm angehoert
    struct cdi_video_device* device;

    /// Momentaner Modus
    struct cdi_video_displaymode* mode;

    /// Liste mit verfuegbaren Modi
    cdi_list_t modes;

    /// Zeichenbuffer fuer Display
    struct cdi_video_bitmap* frontbuffer;
};

/**
 * Speichert eine Farbe in jeweils 8-bit ARGB.
 */
struct cdi_video_color {
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct cdi_video_driver {
    /// CDI-Treiber
    struct cdi_driver drv;

    // Basis-Funktionen
    /**
     * Sichert die aktiven Modi der Displays zur spaeteren Wiederherstellung
     *  @param device Das zu sichernde Device
     *  @return 0 = Erfolg; -1 = Fehlschlag
     */
    int (*driver_save)(struct cdi_video_device* device);

    /**
     * Stellt den gesicherten Zustand wieder her.
     *  @param device Das Device das wiederherstellen soll
     *  @return 0 = Erfolg; -1 = Fehlschlag
     */
    int (*driver_restore)(struct cdi_video_device* device);



    // Anzeigenkontrolle

    /**
     * Setzt die Aufloesung eines Bildschirms (grafische Modi)
     *  @param display Bildschirm
     *  @param mode Modus
     *  @return 0 = Erfolg; -1 = Fehlschlag
     */
    int (*display_set_mode)
        (struct cdi_video_device* device, struct cdi_video_display* display,
        struct cdi_video_displaymode* mode);

    /**
     * Setzt den vorherigen Textmodus
     *  @param device Das Device dessen Textmodus wiederhergestellt werden soll
     *  @return 0 = Erfolg; -1 = Fehlschlag
     */
    int (*restore_text_mode)(struct cdi_video_device* device);


    // Bitmap-Funktionen

    /**
     * Erstellt ein Bitmap
     * @param device Das Geraet auf dem das Bitmap liegt
     * @param width Breite des Bitmaps
     * @param height Hoehe des Bitmaps
     * @param data Pixeldaten, oder 0, wenn keine Initialisierung gewuenscht ist
     * @return Zeiger auf die neue Bitmap
     */
    struct cdi_video_bitmap* (*bitmap_create)
        (struct cdi_video_device* device, unsigned int width,
        unsigned int height, void* data);

    /**
     * Loescht ein Bitmap
     * @param bitmap Zu loeschendes Bitmap
     */
    void (*bitmap_destroy)
        (struct cdi_video_bitmap* bitmap);

    /**
     * Setzt den Gebrauchsmodus eines Bitmaps (bestimmt zB, ob ein Bitmap gecacht wird)
     * @param device Videogeraet
     * @param bitmap Betroffenes Bitmap
     * @param hint Hinweis auf die Prioritaet des Bitmaps
     */
    void (*bitmap_set_usage_hint)
        (struct cdi_video_device* device, struct cdi_video_bitmap* bitmap,
        enum cdi_video_bitmap_usage_hint hint);

    /**
     * Fordert Backbuffer fuer das Bitmap an
     * @param bitmap Das Bitmap das neue Backbuffer bekommen soll.
     * @param backbuffercount Anzahl der einzurichtenden Backbuffer
     * @return Anzahl der eingerichteten Backbuffer
     */
    int (*bitmap_request_backbuffers)
        (struct cdi_video_bitmap* bitmap, int backbuffercount);

    /**
     * Tauscht die Zeichenbuffer des Bitmaps
     * @param bitmap Das Bitmap dessen Buffer getauscht werden sollen
     */
    void (*bitmap_flip_buffers)
        (struct cdi_video_bitmap* bitmap);

    // Zeichen-Funktionen

    /**
     * Setzt den Raster-Op fuer die folgenden Zeichenfunktionen.
     * Der Raster-Op bestimmt, auf welche Art Grafik gezeichnet wird.
     * @param target Bitmap auf das gezeichnet werden soll
     * @param rop Raster-Op
     */
    void (*set_raster_op)
        (struct cdi_video_device* device, enum cdi_video_raster_op rop);

    /**
     * Zeichnet eine Linie
     * @param target Bitmap auf das gezeichnet wird
     * @param x1 X-Koordinate des Anfangs
     * @param y1 Y-Koordinate des Anfangs
     * @param x1 X-Koordinate des Endpunktes
     * @param y1 Y-Koordinate des Endpunktes
     * @param color Farbe der Linie
     */
    void (*draw_line)
        (struct cdi_video_bitmap* target, unsigned int x1, unsigned int y1,
        unsigned int x2, unsigned int y2, struct cdi_video_color* color);

    /**
     * Zeichnet ein Rechteck
     * @param target Bitmap auf das gezeichnet wird
     * @param x X-Koordinate
     * @param y Y-Koordinate
     * @param width Breite
     * @param height Hoehe
     * @param color Farbe des Rechtecks
     */
    void (*draw_rectangle)
        (struct cdi_video_bitmap* target, unsigned int x, unsigned int y,
        unsigned int width, unsigned int height, struct cdi_video_color* color);

    /**
     * Zeichnet eine Ellipse
     * @param target Bitmap auf das gezeichnet wird
     * @param x X-Koordinate
     * @param y Y-Koordinate
     * @param width Breite
     * @param height Hoehe
     * @param color Farbe der Ellipse
     */
    void (*draw_ellipse)
        (struct cdi_video_bitmap* target, unsigned int x, unsigned int y,
        unsigned int width, unsigned int height, struct cdi_video_color* color);

    /**
     * Zeichnet ein Bitmap
     * @param target Bitmap auf das gezeichnet wird
     * @param bitmap Zu zeichnendes Bitmap
     * @param x X-Koordinate
     * @param y Y-Koordinate
     * @note Es wird nur so viel kopiert wie das Zielbitmap aufnehmen kann
     */
    void (*draw_bitmap)
        (struct cdi_video_bitmap* target, struct cdi_video_bitmap* bitmap,
        unsigned int x, unsigned int y);

    /**
     * Zeichnet einen Teil einer Bitmap
     * @param target Bitmap auf das gezeichnet wird
     * @param bitmap Zu zeichnendes Bitmap
     * @param x X-Koordinate (Ziel)
     * @param y Y-Koordinate (Ziel)
     * @param srcx X-Koordinate (Quelle)
     * @param srcy Y-Koordinate (Quelle)
     * @param width Breite
     * @param height Hoehe
     */
    void (*draw_bitmap_part)
        (struct cdi_video_bitmap* target, struct cdi_video_bitmap* bitmap,
        unsigned int x, unsigned int y, unsigned int srcx, unsigned int srcy,
        unsigned int width, unsigned int height);

    /**
     * Zeichnet einen Punkt
     * @param target Display, auf dem gezeichnet wird
     * @param x X-Koordinate (Ziel)
     * @param y Y-Koordinate (Ziel)
     * @param color Zeiger auf die zu verwendende Farbe
     * @note Sollte Sparsam eingesetzt werden, Viele Punkte so zeichnen ist
     *  meistens sehr langsam.
     */
    void (*draw_dot)
        (struct cdi_video_bitmap* target, unsigned int x, unsigned int y,
        struct cdi_video_color* color);
};

/**
 * Initialisiert die Datenstrukturen fuer einen Grafikkartentreiber
 */
void cdi_video_driver_init(struct cdi_video_driver* driver);

/**
 * Deinitialisiert die Datenstrukturen fuer einen Grafikkartentreiber
 */
void cdi_video_driver_destroy(struct cdi_video_driver* driver);

/**
 * Registiert einen Grafikkartentreiber
 */
void cdi_video_driver_register(struct cdi_video_driver* driver);


#endif
