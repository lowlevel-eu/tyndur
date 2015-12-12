/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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


#include <stdint.h>
#include <lostio.h>
#include <stdlib.h>

#include "cdi/fs.h"

int cdi_fs_lio_probe(struct lio_service* service,
                     lio_stream_t source,
                     struct lio_probe_service_result* probe_data);
struct lio_resource* cdi_fs_lio_load_root(struct lio_tree* tree);
int cdi_fs_lio_load_children(struct lio_resource* res);
int cdi_fs_lio_read(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf);
int cdi_fs_lio_write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf);
int cdi_fs_lio_resize(struct lio_resource* res, uint64_t size);
struct lio_resource* cdi_fs_lio_make_file(struct lio_resource* parent,
    const char* name);
struct lio_resource* cdi_fs_lio_make_dir(struct lio_resource* parent,
    const char* name);
struct lio_resource* cdi_fs_lio_make_symlink(struct lio_resource* parent,
    const char* name, const char* target);
int cdi_fs_lio_unlink(struct lio_resource* parent, const char* name);
int cdi_fs_lio_sync(struct lio_resource* res);


void cdi_fs_driver_init(struct cdi_fs_driver* driver)
{
}

void cdi_fs_driver_destroy(struct cdi_fs_driver* driver)
{
    // TODO
}

void cdi_fs_driver_register(struct cdi_fs_driver* driver)
{
    struct lio_service* service;

    service = malloc(sizeof(*service));
    service->name = driver->drv.name;
    service->opaque = driver;
    service->lio_ops = (struct lio_driver) {
        .probe          = cdi_fs_lio_probe,
        .load_root      = cdi_fs_lio_load_root,
        .load_children  = cdi_fs_lio_load_children,
        .read           = cdi_fs_lio_read,
        .write          = cdi_fs_lio_write,
        .resize         = cdi_fs_lio_resize,
        .make_file      = cdi_fs_lio_make_file,
        .make_dir       = cdi_fs_lio_make_dir,
        .make_symlink   = cdi_fs_lio_make_symlink,
        .unlink         = cdi_fs_lio_unlink,
        .sync           = cdi_fs_lio_sync,
    };

    lio_add_service(service);
}


/**
 * Quelldateien fuer ein Dateisystem lesen
 * XXX Brauchen wir hier auch noch irgendwas errno-Maessiges?
 *
 * @param fs Pointer auf die FS-Struktur des Dateisystems
 * @param start Position von der an gelesen werden soll
 * @param size Groesse des zu lesenden Datenblocks
 * @param buffer Puffer in dem die Daten abgelegt werden sollen
 *
 * @return die Anzahl der gelesenen Bytes
 */
size_t cdi_fs_data_read(struct cdi_fs_filesystem* fs, uint64_t start,
    size_t size, void* buffer)
{
    ssize_t result;

    if ((result = lio_pread(fs->osdep.source, start, size, buffer)) < 0) {
        // FIXME: Das ist irgendwie ein bescheiden
        return 0;
    }

    return result;
}

/**
 * Quellmedium eines Dateisystems beschreiben
 * XXX Brauchen wir hier auch noch irgendwas errno-Maessiges?
 *
 * @param fs Pointer auf die FS-Struktur des Dateisystems
 * @param start Position an die geschrieben werden soll
 * @param size Groesse des zu schreibenden Datenblocks
 * @param buffer Puffer aus dem die Daten gelesen werden sollen
 *
 * @return die Anzahl der geschriebenen Bytes
 */
size_t cdi_fs_data_write(struct cdi_fs_filesystem* fs, uint64_t start,
    size_t size, const void* buffer)
{
    ssize_t result;

    if ((result = lio_pwrite(fs->osdep.source, start, size, buffer)) < 0) {
        // FIXME: Das ist irgendwie ein bescheiden
        return 0;
    }

    return result;
}
