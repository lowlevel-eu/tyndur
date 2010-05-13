/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lostio.h>

#include "cdi/storage.h"
#include "libpartition.h"

static void lostio_mst_if_init(void);
int lostio_mst_if_newdev(struct partition* part);

static size_t lostio_mst_read_handler(lostio_filehandle_t* fh,
    void* buf, size_t blocksize, size_t blockcount);
static size_t lostio_mst_write_handler(lostio_filehandle_t* fh,
    size_t blocksize, size_t blockcount, void* data);
static int lostio_mst_seek_handler(lostio_filehandle_t* fh, uint64_t offset,
    int origin);

#define CDI_LOSTIO_TYPE_MST 255
static typehandle_t lostio_mst_typehandle = {
    .id = CDI_LOSTIO_TYPE_MST,

    .pre_open = NULL,
    .post_open = NULL,
    .not_found = NULL,
    .close = NULL,

    .read = lostio_mst_read_handler,
    .write = lostio_mst_write_handler,
    .seek = lostio_mst_seek_handler
};

/**
 * Initialisiert die Datenstrukturen fuer einen Massenspeichertreiber
 */
void cdi_storage_driver_init(struct cdi_storage_driver* driver)
{
    driver->drv.type = CDI_STORAGE;
    cdi_driver_init((struct cdi_driver*) driver);
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Massenspeichertreiber
 */
void cdi_storage_driver_destroy(struct cdi_storage_driver* driver)
{
    cdi_driver_destroy((struct cdi_driver*) driver);
}

/**
 * Registiert einen Massenspeichertreiber
 */
void cdi_storage_driver_register(struct cdi_storage_driver* driver)
{
    static int initialized = 0;

    if (initialized == 0) {
        lostio_mst_if_init();
        initialized = 1;
    }
}

/**
 * Initialisiert einen Massenspeicher
 */
void cdi_storage_device_init(struct cdi_storage_device* device)
{
    cdi_list_t partitions;
    struct partition* part;

    // Geraeteknoten fuer LostIO erstellen
    part = calloc(1, sizeof(*part));
    part->dev       = device;
    part->number    = (uint16_t) -1;
    part->start     = 0;
    part->size      = device->block_size * device->block_count;

    lostio_mst_if_newdev(part);

    // Partitionen suchen und Knoten erstellen
    partitions = cdi_list_create();
    cdi_tyndur_parse_partitions(device, partitions);
    while ((part = cdi_list_pop(partitions))) {
        lostio_mst_if_newdev(part);
    }
    cdi_list_destroy(partitions);
}

/**
 * Liest Daten von einem Massenspeichergeraet
 *
 * @param pos Startposition in Bytes
 * @param size Anzahl der zu lesenden Bytes
 * @param dest Buffer
 */
int cdi_storage_read(struct cdi_storage_device* device, uint64_t pos,
    size_t size, void* dest)
{
    struct cdi_storage_driver* driver = (struct cdi_storage_driver*) device->
        dev.driver;
    size_t block_size = device->block_size;
    // Start- und Endblock
    uint64_t block_read_start = pos / block_size;
    uint64_t block_read_end = (pos + size) / block_size;
    // Anzahl der Blocks die gelesen werden sollen
    uint64_t block_read_count = block_read_end - block_read_start;

    // Wenn nur ganze Bloecke gelesen werden sollen, geht das etwas effizienter
    if (((pos % block_size) == 0) && (((pos + size) %  block_size) == 0)) {
        // Nur ganze Bloecke
        return driver->read_blocks(device, block_read_start, block_read_count,
            dest);
    } else {
        // FIXME: Das laesst sich garantiert etwas effizienter loesen
        block_read_count++;
        uint8_t buffer[block_read_count * block_size];

        // In Zwischenspeicher einlesen
        if (driver->read_blocks(device, block_read_start, block_read_count,
            buffer) != 0)
        {
            return -1;
        }

        // Bereich aus dem Zwischenspeicher kopieren
        memcpy(dest, buffer + (pos % block_size), size);
    }
    return 0;
}

/**
 * Schreibt Daten auf ein Massenspeichergeraet
 *
 * @param pos Startposition in Bytes
 * @param size Anzahl der zu schreibendes Bytes
 * @param src Buffer
 */
int cdi_storage_write(struct cdi_storage_device* device, uint64_t pos,
    size_t size, void* src)
{
    struct cdi_storage_driver* driver = (struct cdi_storage_driver*) device->
        dev.driver;

    size_t block_size = device->block_size;
    uint64_t block_write_start = pos / block_size;
    uint8_t buffer[block_size];
    size_t offset;
    size_t tmp_size;

    // Wenn die Startposition nicht auf einer Blockgrenze liegt, muessen wir
    // hier zuerst den ersten Block laden, die gewuenschten Aenderungen machen,
    // und den Block wieder Speichern.
    offset = (pos % block_size);
    if (offset != 0) {
        tmp_size = block_size - offset;
        tmp_size = (tmp_size > size ? size : tmp_size);

        if (driver->read_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }
        memcpy(buffer + offset, src, tmp_size);

        // Buffer abspeichern
        if (driver->write_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }

        size -= tmp_size;
        src += tmp_size;
        block_write_start++;
    }

    // Jetzt wird die Menge der ganzen Blocks auf einmal geschrieben, falls
    // welche existieren
    tmp_size = size / block_size;
    if (tmp_size != 0) {
        // Buffer abspeichern
        if (driver->write_blocks(device, block_write_start, tmp_size, src)
            != 0)
        {
            return -1;
        }
        size -= tmp_size * block_size;
        src += tmp_size * block_size;
        block_write_start += block_size;
    }

    // Wenn der letzte Block nur teilweise beschrieben wird, geschieht das hier
    if (size != 0) {
        // Hier geschieht fast das Selbe wie oben beim ersten Block
        if (driver->read_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }
        memcpy(buffer, src, size);

        // Buffer abspeichern
        if (driver->write_blocks(device, block_write_start, 1, buffer) != 0) {
            return -1;
        }
    }
    return 0;
}


/**
 * Initialisiert die Massenspeichernschnittstelle fuer LostIO
 */
static void lostio_mst_if_init()
{
    lostio_register_typehandle(&lostio_mst_typehandle);
}

/**
 * Erstellt den Dateisystemknoten fuer ein Geraet.
 */
int lostio_mst_if_newdev(struct partition* part)
{
    static int has_cdrom = 0;
    struct cdi_storage_device* device = part->dev;

    // Slash vor Pfad anhaengen
    char* path;
    if (part->number == (uint16_t) -1) {
        asprintf(&path, "/%s", device->dev.name);
    } else {
        asprintf(&path, "/%s_p%d", device->dev.name, part->number);
    }

    if (vfstree_create_node(path, CDI_LOSTIO_TYPE_MST, device->block_size *
        device->block_count, (void*) part, 0) == false)
    {
        free(path);
        return -1;
    }

    if (!has_cdrom && !strncmp(device->dev.name, "atapi", strlen("atapi"))) {
        has_cdrom = 1;
        vfstree_create_node("/cdrom", CDI_LOSTIO_TYPE_MST, device->block_size *
            device->block_count, (void*) part, 0);
    }

    free(path);
    return 0;
}

/**
 * Lesehandler fuer LostIO
 */
static size_t lostio_mst_read_handler(lostio_filehandle_t* fh,
    void* buf, size_t blocksize, size_t blockcount)
{
    struct partition* part = (struct partition*) fh->node->data;
    struct cdi_storage_device* device = part->dev;
    size_t size = blocksize * blockcount;

    // Groesse anpassen, wenn ueber das Medium hinaus gelesen werden soll
    if (size > (part->size - fh->pos)) {
        size = part->size - fh->pos;
        if (size == 0) {
            fh->flags |= LOSTIO_FLAG_EOF;
            return 0;
        }
    }

    // In den uebergebenen Buffer einlesen
    if (cdi_storage_read(device, fh->pos + part->start, size, buf) != 0) {
        return 0;
    }

    fh->pos += size;

    return size;
}

/**
 * Schreibhandler fuer LostIO
 */
static size_t lostio_mst_write_handler(lostio_filehandle_t* fh,
    size_t blocksize, size_t blockcount, void* data)
{
    struct partition* part = (struct partition*) fh->node->data;
    struct cdi_storage_device* device = part->dev;
    size_t size = blocksize * blockcount;
    size_t result = size;

    // Groesse anpassen, wenn ueber das Medium hinaus geschrieben werden soll
    if (size > (part->size - fh->pos)) {
        size = part->size - fh->pos;
    }

    // Daten schreiben
    if (cdi_storage_write(device, fh->pos + part->start, size, data) != 0) {
        result = 0;
    } else {
        fh->pos += size;
    }

    return result;
}

/**
 * Seek-Handler fuer LostIO
 */
static int lostio_mst_seek_handler(lostio_filehandle_t* fh, uint64_t offset,
    int origin)
{
    struct partition* part = (struct partition*) fh->node->data;
    uint64_t new_pos = fh->pos;
    uint64_t size = part->size;

    switch (origin)  {
        case SEEK_SET:
            new_pos = offset;
            break;

        case SEEK_CUR:
            new_pos += offset;
            break;

        case SEEK_END:
            new_pos = size;
            break;
    }

    // Position nur aktualisieren, wenn sie nicht ausserhalb des Datentraegers
    // liegt.
    if (new_pos > size) {
        return -1;
    } else if (new_pos == size) {
        // Gegebenen Falles noch EOF setzen
        fh->flags |= LOSTIO_FLAG_EOF;
    } else {
        // Sonst EOF loeschen
        fh->flags &= ~LOSTIO_FLAG_EOF;
    }
    fh->pos = new_pos;
    return 0;
}


