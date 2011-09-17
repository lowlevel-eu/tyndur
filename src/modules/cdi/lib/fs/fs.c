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

#include "cdi/fs.h"

#define CDI_TYPEHANDLE 200
#define CDI_TYPEHANDLE_SYMLINK 201


struct cdi_fs_driver* the_one_and_only_driver = NULL;

bool lostio_not_found_handler(char** path, uint8_t args, pid_t pid,
    FILE* source);
bool lostio_pre_open_handler(char** path, uint8_t args, pid_t pid,
    FILE* source);
size_t lostio_read_handler(lostio_filehandle_t* fh, void* buf,
    size_t bs, size_t bc);
size_t lostio_write_handler(lostio_filehandle_t* fh, size_t bs, size_t bc,
    void* data);
int lostio_seek_handler(lostio_filehandle_t* fh, uint64_t offset, int origin);
int lostio_unlink_handler(lostio_filehandle_t* fh, const char* name);
size_t lostio_symlink_read_handler(lostio_filehandle_t* fh, void* buf,
    size_t bs, size_t bc);
size_t lostio_symlink_write_handler(lostio_filehandle_t* fh, size_t bs,
    size_t bc, void* data);

typehandle_t cdi_typehandle = {
    .id = CDI_TYPEHANDLE,
    .not_found = &lostio_not_found_handler,
    .pre_open = &lostio_pre_open_handler,
    .post_open = NULL,
    .read = &lostio_read_handler,
    .write = &lostio_write_handler,
    .seek = &lostio_seek_handler,
    .unlink = &lostio_unlink_handler,
    .close = NULL
};

typehandle_t cdi_symlink_typehandle = {
    .id = CDI_TYPEHANDLE_SYMLINK,
    .not_found = &lostio_not_found_handler,
    .pre_open = &lostio_pre_open_handler,
    .post_open = NULL,
    .read = &lostio_symlink_read_handler,
    .write = &lostio_symlink_write_handler,
    .seek = &lostio_seek_handler,
    .unlink = &lostio_unlink_handler,
    .close = NULL
};


typehandle_t cdi_null_typehandle = {
    .id = 0,
    .not_found = &lostio_not_found_handler,
    .pre_open = NULL,
    .post_open = NULL,
    .read = NULL,
    .write = NULL,
    .seek = NULL,
    .close = NULL
};


void cdi_fs_driver_init(struct cdi_fs_driver* driver)
{
    static int initialized = 0;

    if (!initialized) {

        lostio_init();
        lostio_type_directory_use();
        lostio_register_typehandle(&cdi_typehandle);
        lostio_register_typehandle(&cdi_symlink_typehandle);
        lostio_register_typehandle(&cdi_null_typehandle);


        // Preopen-Handler fuer Verzeichnisse setzen
        typehandle_t* typehandle = get_typehandle(LOSTIO_TYPES_DIRECTORY);
        typehandle->pre_open = &lostio_pre_open_handler;
        typehandle->not_found = &lostio_not_found_handler;
        typehandle->post_open = NULL;
        typehandle->unlink = &lostio_unlink_handler;

        initialized = 1;
    }
}

void cdi_fs_driver_destroy(struct cdi_fs_driver* driver)
{
}

void cdi_fs_driver_register(struct cdi_fs_driver* driver)
{
    if (the_one_and_only_driver) {
        return;
    }

    the_one_and_only_driver = driver;
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
    if (!lio_seek(fs->device, start, SEEK_SET)) {
        return -1;
    }

    return fread(buffer, 1, size, fs->device);
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
    if (!lio_seek(fs->device, start, SEEK_SET)) {
        return -1;
    }

    return fwrite(buffer, 1, size, fs->device);
}
