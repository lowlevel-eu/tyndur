/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
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
#include <lostio.h>
#include <syscall.h>
#define _USE_START_
#include "init.h"
#undef _USE_START_

#include "lostio_if.h"
#include "libext2/ext2.h"

/// Liste mit allen Root handles von ext2
list_t* root_handle_list;

/// Diese Funktion synchronisiert die Caches saemtlicher Dateisystemfunktionen
static void sync_caches();

// Da wir mehrere Verzeichnisbaeume brauchen, weil der Treiber mehrere Devices
// gleichzeitig verwalten soll, werden die Pfade alle folgendermassen
// umgebogen:
// /<Source_PID>/<Source_RES_ID>/<Pfad>

/// Dieser Typ wird fuer das Root-Verzeichnis und fuer die PID-Verzeichnisse
/// eingesetzt
typehandle_t root_dir_type_handle = {
    .id = EXT2_LOSTIO_TYPE_ROOT_DIR,
    .not_found = &lostio_not_found_handler,
    .pre_open = &lostio_pre_open_handler,
    .post_open = &lostio_post_open_handler,
    .read = NULL,
    .write = NULL,
    .seek = NULL,
    .close = NULL
};

/// Dieser Typ wird benuzt wenn kein anderer gefunden wird
typehandle_t null_type_handle = {
    .id = 0,
    .not_found = &lostio_not_found_handler,
    .pre_open = NULL,
    .post_open = NULL,
    .read = NULL,
    .write = NULL,
    .seek = NULL,
    .close = NULL
};


/// Normale Ext2-Datei
typehandle_t file_type_handle = {
    .id = EXT2_LOSTIO_TYPE_FILE,
    .not_found = NULL,
    .pre_open = &lostio_pre_open_handler,
    .post_open = &lostio_post_open_handler,
    .read = &lostio_read_handler,
    .write = &lostio_write_handler,
    .seek = &lostio_seek_handler,
    .close = &lost_close_handler
};

/// Ext2-Symlink
typehandle_t symlink_type_handle = {
    .id = EXT2_LOSTIO_TYPE_SYMLINK,
    .not_found = NULL,
    .pre_open = &lostio_pre_open_handler,
    .post_open = &lostio_post_open_handler,
    .read = &lostio_read_symlink_handler,
    .write = &lostio_write_symlink_handler,
    .seek = NULL,
    .close = &lost_close_handler
};


/**
 * Hauptfunktion
 */
int main(int argc, char* argv[])
{
    // LostIO initialisieren
    lostio_init();
    lostio_type_directory_use(); 
    lostio_register_typehandle(&root_dir_type_handle);
    lostio_register_typehandle(&file_type_handle);
    lostio_register_typehandle(&null_type_handle);
    lostio_register_typehandle(&symlink_type_handle);
    
    // Preopen-Handler fuer Verzeichnisse setzen
    typehandle_t* typehandle = get_typehandle(LOSTIO_TYPES_DIRECTORY);
    typehandle->pre_open = &lostio_pre_open_handler;
    typehandle->not_found = &lostio_not_found_handler;
    typehandle->post_open = &lostio_post_open_handler;
    typehandle->link = &lostio_link_handler;
    typehandle->unlink = &lostio_unlink_handler;
    typehandle->close = &lost_close_handler;
    // Root-Node-Typ muss jetzt auch noch gesetzt werden ;-)
    vfstree_node_t* node = vfstree_get_node_by_path("/");
    node->type = EXT2_LOSTIO_TYPE_ROOT_DIR;

    root_handle_list = list_create();
    
    // Sync_caches erstes Mal aufrufen, damit der Timer initialisiert wird
    sync_caches();

    // So, jetzt sind wir fuer anfragen bereit
    init_service_register("ext2");
    while (TRUE) {
        wait_for_rpc();
    }
}

static void sync_caches()
{
    int i;
    ext2_root_handle_t* root_handle;
    p();
    for (i = 0; (root_handle = list_get_element_at(root_handle_list, i)); i++)
    {
        ext2_sync(root_handle);
    }
    v();
    timer_register(sync_caches, 2000000);
}

