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
#include <io.h>

#include "cdi/storage.h"
#include "libpartition.h"

int lostio_mst_if_newdev(struct partition* part);

static int read(struct lio_resource* res, uint64_t offset,
    size_t size, void* buf);
static int write(struct lio_resource* res, uint64_t offset,
    size_t size, void* buf);
static struct lio_resource* load_root(struct lio_tree* tree);
static int load_children(struct lio_resource* res);
int cdi_storage_read(struct cdi_storage_device* device, uint64_t pos,
    size_t size, void* dest);

static const char* service_name;

static const char* get_service_name(struct cdi_storage_driver* drv)
{
    const char* name;

    name = drv->drv.name;
    if (!strcmp(name, "ahci-disk")) {
        name = "ahci";
    }

    return name;
}

/**
 * Initialisiert die Datenstrukturen fuer einen Massenspeichertreiber
 */
void cdi_storage_driver_init(struct cdi_storage_driver* driver)
{
    static struct lio_resource* first_res = NULL;
    struct lio_resource* res;
    struct lio_service* service;

    driver->drv.type = CDI_STORAGE;
    cdi_driver_init((struct cdi_driver*) driver);

    /* Alle Treiber in einer Binary teilen sich einen Baum */
    if (first_res) {
        driver->osdep.root = first_res;
        return;
    }

    service_name = get_service_name(driver);

    res = lio_resource_create(NULL);
    res->browsable = 1;
    res->blocksize = 1024;

    driver->osdep.root = res;
    if (!first_res) {
        first_res = res;
    }

    service = malloc(sizeof(*service));
    *service = (struct lio_service) {
        .name               = service_name,
        .lio_ops = {
            .load_root      = load_root,
            .load_children  = load_children,
            .read           = read,
            .write          = write,
        },
    };

    lio_add_service(service);
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


struct fs_detection {
    char* path;
};

static void fs_detection_thread(void* opaque)
{
    struct fs_detection* fsd = opaque;
    lio_resource_t fd;
    char* link_path;
    char* target_path;
    struct lio_probe_service_result probe_data;
    int ret;

    fd = lio_resource(fsd->path, false);
    if (fd < 0) {
        goto fail;
    }

    ret = lio_probe_service(fd, &probe_data);
    if (ret < 0) {
        goto fail;
    }

    asprintf(&link_path, "dev:/fs/%s", &probe_data.volname);
    asprintf(&target_path, "%s|%s:/", fsd->path, &probe_data.service);

    io_create_link(target_path, link_path, false);

    free(link_path);
    free(target_path);
fail:
    free(fsd->path);
    free(fsd);
}

/**
 * Versucht, einen dev:/-Link für das Dateisystem anlegen. Wenn es schiefgeht,
 * ist nicht schlimm, dann fehlt der Link halt. Die eigentliche Erkennung des
 * Dateisystems muss in einem anderen Thread ablaufen, weil der Kernel Requests
 * an den Service schicken wird und wir nicht deadlocken wollen.
 */
static void queue_fs_detection(const char* service, const char* relpath)
{
    struct fs_detection* fsd;
    int ret;

    fsd = malloc(sizeof(*fsd));
    if (fsd == NULL) {
        return;
    }

    ret = asprintf(&fsd->path, "%s:/%s", service, relpath);
    if (ret < 0) {
        fsd->path = NULL;
        goto fail;
    }

    ret = create_thread((uint32_t) &fs_detection_thread, fsd);
    if (ret < 0) {
        goto fail;
    }
    return;

fail:
    free(fsd->path);
    free(fsd);
}

/**
 * Erstellt den Dateisystemknoten fuer ein Geraet.
 */
int lostio_mst_if_newdev(struct partition* part)
{
    static int has_cdrom = 0;
    struct cdi_storage_device* device = part->dev;
    struct cdi_storage_driver* driver = (void*) device->dev.driver;
    struct lio_resource* res;

    // Dateinamen erzeugen
    char* path;
    if (part->number == (uint16_t) -1) {
        asprintf(&path, "%s", device->dev.name);
    } else {
        asprintf(&path, "%s_p%d", device->dev.name, part->number);
    }

    // Neue Ressource anlegen
    res = lio_resource_create(NULL);
    res->readable = 1;
    res->writable = 1;
    res->seekable = 1;
    res->blocksize = device->block_size;
    res->size = part->size;
    res->opaque = part;

    lio_node_create(driver->osdep.root, res, path);

    // Das erste ATAPI-Laufwerk wird ata:/cdrom
    if (!has_cdrom && !strncmp(device->dev.name, "atapi", strlen("atapi"))) {
        has_cdrom = 1;
        res = lio_resource_create(NULL);
        res->opaque = strdup(device->dev.name);
        res->resolvable = 1;
        res->blocksize = 0x1000;
        res->size = strlen(res->opaque);
        lio_node_create(driver->osdep.root, res, "cdrom");
    }

    // Versuchen, das Dateisystem zu erkennen
    queue_fs_detection(service_name, path);

    free(path);
    return 0;
}


extern list_t* cdi_tyndur_get_drivers(void);
static struct lio_resource* load_root(struct lio_tree* tree)
{
    list_t* drivers = cdi_tyndur_get_drivers();
    struct cdi_driver* drv;
    int i;

    if (tree->source != -1) {
        return NULL;
    }

    for (i = 0; (drv = list_get_element_at(drivers, i)); i++) {
        if (drv->type == CDI_STORAGE) {
            struct cdi_storage_driver* sdrv = (void*) drv;
            if (!strcmp(tree->service->name, drv->name)) {
                struct lio_resource* root = sdrv->osdep.root;
                root->tree = tree;
                lio_resource_modified(root);
                return root;
            }
        }
    }

    return NULL;
}

static int load_children(struct lio_resource* root)
{
    struct lio_node* it;
    int j;

    for (j = 0; (it = list_get_element_at(root->children, j)); j++)
    {
        it->res->tree = root->tree;
        lio_resource_modified(it->res);
        lio_srv_node_add(root->server.id, it->res->server.id,
            it->name);
    }

    return 0;
}

#define NUM_BLOCKS(x) (((x) + res->blocksize - 1) / res->blocksize)

/**
 * Lesehandler fuer LostIO
 */
static int read(struct lio_resource* res, uint64_t offset,
    size_t size, void* buf)
{
    struct partition* part = res->opaque;
    struct cdi_storage_device* device = part->dev;
    int result;

    // cdrom-Symlink auslesen
    if (res->resolvable) {
        int len = strlen(res->opaque);
        if (offset >= len) {
            return -1;
        } else if (len - offset < size) {
            size = len - offset;
        }
        memcpy(buf, res->opaque, size);
        return 0;
    }

    // Groesse anpassen, wenn ueber das Medium hinaus gelesen werden soll
    if (size > (part->size - offset)) {
        size = part->size - offset;
    }

    // In den uebergebenen Buffer einlesen
    result = cdi_storage_read(device, offset + part->start, size, buf);

    return (result ? -1 : 0);
}

/**
 * Schreibhandler fuer LostIO
 */
static int write(struct lio_resource* res, uint64_t offset,
    size_t size, void* buf)
{
    struct partition* part = res->opaque;
    struct cdi_storage_device* device = part->dev;
    int result;

    // cdrom-Symlink ist nicht schreibbar
    if (res->resolvable) {
        return -1;
    }

    // Groesse anpassen, wenn ueber das Medium hinaus geschrieben werden soll
    if (size > (part->size - offset)) {
        size = part->size - offset;
    }

    // Daten schreiben
    result = cdi_storage_write(device, offset + part->start, size, buf);

    return (result ? -1 : 0);
}
