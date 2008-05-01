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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ext2.h"

/**
 * Neuen Symlink erstellen
 *
 * @param parent Inodenummer des Elternverzeichnisses
 * @param name Name des neuen Symlinks
 * @param inode_dest falls != NULL wird hier die Inode-Nummer gespeichert
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
int ext2_symlink_create(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name, uint32_t* inode_dest)
{
    uint32_t inode_num;
    ext2_ih_t inode;
    int result = 0;

    // Zuerst wird mal ein neuer Inode alloziert
    inode_num = ext2_inode_allocate(root_handle);
    if (inode_num == 0) {
        print_error("Inode konnte nicht alloziert werden");
        return -1;
    }
    
    ext2_ih_init(root_handle, &inode, inode_num);
    inode.modified = 1;

    // Inode sauber initialisieren
    memset(&inode, 0, sizeof(ext2_inode_t));
    inode.data->mode = EXT2_INODE_MODE_SYMLINK;
    
    // Berechtigungen auf 666 setzen (nur fuer andere Betriebsysteme)
    inode.data->mode |= EXT2_INODE_MODE_USER_READ | EXT2_INODE_MODE_USER_WRITE|
        EXT2_INODE_MODE_GROUP_READ | EXT2_INODE_MODE_GROUP_WRITE |
        EXT2_INODE_MODE_OTHER_READ | EXT2_INODE_MODE_OTHER_WRITE;
    
    // Jetzt noch abspeichern
    if (ext2_inode_write(root_handle, &inode) != 0) {
        print_error("Inode konnte nicht gespeichert werden");
        result = -2;
        goto end;
    }
        
    // Im Elternverzeichnis einen Link darauf erstellen
    if (ext2_dir_link(root_handle, parent, &inode, name) != 0) {
        print_error("Fehler beim anlegen des Eintrages im Elternverzeichnis");
        result = -5;
        goto end;
    }
        
    // Falls gewuenscht noch Inodenummer speichern
    if (inode_dest != NULL) {
        *inode_dest = inode_num;
    }

end:
    ext2_ih_destroy(root_handle, &inode);
    return result;
}

/**
 * Symlink loeschen
 *
 * @param parent Inodenummer des Elternverzeichnisses
 * @param name Name des Symlinks
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall
 */
int ext2_symlink_delete(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name)
{
    // Link auf die Datei loeschen
    if (ext2_dir_unlink(root_handle, parent, name) != 0) {
        print_error("Verzeichniseintrag konnte nicht geloescht werden");
        return -2;
    }

    return 0;
}

/**
 * Pfad aus einem Symlink auslesen, das abschliessende Null-Byte wird _NICHT_
 * mitkopiert.
 *
 * @param inode
 * @param size Groesse des bereitgestellten Puffers
 * @param buffer Pointer auf den Buffer in den der Pfad kopiert werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
int ext2_symlink_read(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode, size_t size, void* buffer)
{
    // Pruefen ob der Puffer reicht
    if (size < inode->data->size) {
        print_error("Puffer fuer Link-Pfad zu klein");
        return -1;
    }

    // Bei fast symlinks wird das block-Array zur Speicherung des Pfades
    // benutzt, die Block-Anzahl ist dann 0
    if (inode->data->block_count == 0) {
        // Wir haben es mit einem fast symlink zu tun, da ist das ganze sehr
        // einfach: Nur den String aus dem Blockarray in den Puffer kopieren
        // und gut ist.
        memcpy(buffer, inode->data->blocks, inode->data->size);
    } else {
        // Normale symlinks werden gleich wie regulaere Dateien behandelt
        size_t result = ext2_inode_data_read(root_handle, inode, 0,
            inode->data->size, buffer);
        
        // Wenn nicht die erwartete Anzahl Bytes gelesen wurde, wird
        // abgebrochen
        if (result != inode->data->size) {
            print_error("Fehler beim Auslesen des symbolischen Links");
            return -2;
        }
    }

    return 0;
}

/// Pfad eines Symlinks ueberschreiben
int ext2_symlink_write(ext2_root_handle_t* root_handle, ext2_ih_t* inode,
    size_t size, void* buffer);



