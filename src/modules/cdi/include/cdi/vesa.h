/*
 * Copyright (c) 2008-2009 Mathias Gottschlag, Janosch Graef, Alexander Siol
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_VESA_H_
#define _CDI_VESA_H_

#include <stdint.h>
#include <cdi/lists.h>
#include <cdi/video.h>

/**
 * Allgemeiner VESA / VBE Informationsblock, wird verwendet um Informationen
 * ueber vorhande Faehigkeiten und Modi der Grafikkarte zu erhalten.
 */
struct cdi_video_vesa_info_block {
    char signature[4];
    char version[2];
    uint32_t oemname;
    uint32_t capabilities;
    uint32_t modeptr;
    uint16_t videomem;
    // VBE 2.0
    uint16_t oemversion;
    uint32_t vendornameptr;
    uint32_t productnameptr;
    uint32_t revisionptr;
    uint16_t modes[111];
    uint8_t  oem[256];
} __attribute__ ((packed));

/**
 * VBE Modus-Informationsblock.
 * Enthaelt Infos ueber den speziellen Modus.
 */
struct cdi_video_vesa_mode_info {
    uint16_t modeattr;
    uint8_t  windowattra;
    uint8_t  windowattrb;
    uint16_t windowgran;
    uint16_t windowsize;
    uint16_t startsega;
    uint16_t startsegb;
    uint32_t posfunc;
    uint16_t pitch;

    uint16_t width;
    uint16_t height;
    uint8_t  charwidth;
    uint8_t  charheight;
    uint8_t  planecount;
    uint8_t  depth;
    uint8_t  banks;
    uint8_t  type;
    uint8_t  banksize;
    uint8_t  imagepages;
    uint8_t  reserved;
    // VBE v1.2+
    uint8_t  redmasksize;
    uint8_t  redfieldpos;
    uint8_t  greenmasksize;
    uint8_t  greenfieldpos;
    uint8_t  bluemasksize;
    uint8_t  bluefieldpos;
    uint8_t  resmasksize;
    uint8_t  resfieldsize;
    uint8_t  dircolormode;
    // VBE v2.0
    uint32_t linearfb;
    uint32_t offscreenmem;
    uint32_t offscreensize;

    char reserved2[206];
} __attribute__ ((packed));

/**
 * CDI-Video-Mode mit Erweiterung um VESA/VBE-Modusnummer
 */
struct cdi_video_vesa_mode {
    struct cdi_video_displaymode mode;
    uint16_t vesamode;
};

/**
 * Liest den allgemeinen VESA/VBE-Informationsblock aus und uebergibt einen
 * Pointer auf diesen, ruft fuer jeden gefundenen Modus einen
 * Callback auf.
 * @param vesainfo Zeiger auf Infoblock-Zeiger
 * @param mode_callback Zeiger auf Modus-Callback-Funktion
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_video_vesa_initialize
    (struct cdi_video_vesa_info_block **vesainfo,
    void (*mode_callback)(int modenum, struct cdi_video_vesa_mode_info *mode));

/**
 * Liefert Informationen zu einem spezifischen Modus
 * @param modenum Die Modusnummer - fuer gueltige Nummern siehe den allgemeinen
 *  Infoblock
 * @return Zeiger auf Modus-Informationsblock. 0 bei Fehler.
 */
void cdi_video_vesa_get_mode(int modenum, struct cdi_video_vesa_mode_info* mi);

/**
 * Setzt den Modus via int10
 * @param modenum Die Modusnummer - fuer gueltige Nummern siehe den allgemeinen
 *  Infoblock
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_video_vesa_change_mode(int modenum);

#endif
