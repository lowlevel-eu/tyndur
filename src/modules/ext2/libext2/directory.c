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
 * Verzeichniseintraege verarbeiten
 *
 * @param ih Inode-Handle
 * @param handler Handler der fuer jeden Eintrag aufgerufen wird
 * #param param Erster Parameter fuer handler
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Inode konnte nicht gelesen werden
 *      -2: Daten konnten nicht gelesen werden
 */
int ext2_dir_foreach_entry(ext2_root_handle_t* root_handle, ext2_ih_t* ih,
    int (*handler) (void*, ext2_dir_entry_t*), void* param)
{
    ext2_raw_dir_entry_t* raw_dir_entry;
    size_t size_done;
    uint8_t buffer[ih->data->size];
    int result = 0;

    // Pruefen ob es sich auch wirklich um ein Verzeichnis handelt
    if (!EXT2_INODE_IS_DIR(*ih->data)) {
        print_error("Es handelt sich nicht um ein Verzeichnis");
        return -1;
    }
    
    // Jetzt die Daten einlesen
    if (ext2_inode_data_read(root_handle, ih, 0x0, ih->data->size, buffer) !=
       ih->data->size)
    {
        print_error("Fehler beim Einlesen der Verzeichnisdaten");
        return -2;
    }
    
    
    // Eintraege verarbeiten; Dies wird so lange gemacht, bis wir ueber die
    // Groesse des Inodes raus sind
    raw_dir_entry = (ext2_raw_dir_entry_t*) buffer;
    size_done = 0;
    while ((size_done < ih->data->size) && (result == 0)) {
        // Eine Record-Laenge von 0 ist nicht zulaessig und fuehrt zu einer
        // Endlosschleife
        if (raw_dir_entry->record_len == 0) {
            print_error("Korruptes Verzeichnis: Eintrag mit laenge 0");
            return -3;
        }
        
        // Wenn der Inode == 0, dann ist der Eintrag unbenutzt
        if (raw_dir_entry->inode != 0) {

            // Der Eintrag wird zuerst noch etwas rumkopiert bevor er an den
            // Handler geht
            ext2_dir_entry_t dir_entry;
        
            // Inodenummer kopieren
            dir_entry.inode = raw_dir_entry->inode;

            // Namen kopieren und mit Null terminieren
            memcpy(dir_entry.name, raw_dir_entry->name, raw_dir_entry->
                name_len);
            dir_entry.name[raw_dir_entry->name_len] = '\0';

            // Typ setzen
            // Wenn das Filetype-Flag gesetzt ist, geht das ganz praktisch
            if (ext2_feature_incomp(root_handle, EXT2_FEATURE_INCOMP_FILETYPE))
            {
                // Typ-Feld befuellen
                if (raw_dir_entry->type == EXT2_FT_DIR) {
                    dir_entry.type = DIRECTORY;
                } else if (raw_dir_entry->type == EXT2_FT_SYMLINK) {
                    dir_entry.type = SYMLINK;
                } else if (raw_dir_entry->type == EXT2_FT_REG_FILE) {
                    dir_entry.type = REG_FILE;
                }
            } else {
                // Sonst wird das etwas komplizierter. Dann muss das Format
                // naemlich im Inode nachgeschlagen werden.
                ext2_ih_t entry_ih;
                if (!ext2_ih_init(root_handle, &entry_ih, raw_dir_entry->
                    inode)) 
                {
                    print_error("Konnte Inode zum Verzeichniseintrag nicht "
                        "einlesen.");
                    dir_entry.inode = 0;
                }
                
                // Jetzt kann der Typ anhand es Inodes entschieden werden
                if (EXT2_INODE_IS_REG_FILE(*entry_ih.data)) {
                    dir_entry.type = REG_FILE;
                } else if (EXT2_INODE_IS_DIR(*entry_ih.data)) {
                    dir_entry.type = DIRECTORY;
                } else if (EXT2_INODE_IS_SYMLINK(*entry_ih.data)) {
                    dir_entry.type = SYMLINK;
                } else {
                    dir_entry.inode = 0;
                }

                ext2_ih_destroy(root_handle, &entry_ih);
            }

            // Jetzt wird der Handler aufgerufen
            if (dir_entry.inode != 0) {
                result = handler(param, &dir_entry);
            }

        }
        // Naechster Eintrag
        size_done += raw_dir_entry->record_len;
        raw_dir_entry = (ext2_raw_dir_entry_t*) (buffer + size_done); 
    }
    return 0;
}

// Wird beim suchen eines Verzeichniseintrages benutzt fuer die Kommunikation
// zwischen Handler und ext2_dir_entry_get
struct dir_entry_get {
    /// Zielvariable fuer Inodenummer
    uint32_t inode;

    /// Pointer auf namen
    const char* name;
};

/**
 * Handler zum duchsuchen eines Verzeichnisses.
 */
static int ext2_dir_entry_get_handler(void* param,
    ext2_dir_entry_t* dir_entry)
{
    struct dir_entry_get* dir_entry_get = (struct dir_entry_get*) param;

    // Wenn die Namen ueberein stimmen, ist der gewuenschte Eintrag gefunden,
    // und kann eingetragen werden
    if (strcmp(dir_entry->name, dir_entry_get->name) == 0) {
        dir_entry_get->inode = dir_entry->inode;

        // Die Suche ist beendet
        return 1;
    }

    return 0;
}

/**
 * Inodenummer von einem Verzeichniseintrag anhand des Namens ausfindig machen
 * 
 * @param parent Inodenummer des Verzeichnisses
 * @param name Name des gesuchten Eintrages
 *
 * @return Inode-Nummer oder 0 im Fehlerfall
 */
uint32_t ext2_dir_entry_get(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name)
{
    // Struktur fuer Kommunikation mit Handler vorbereiten
    struct dir_entry_get dir_entry_get;
    dir_entry_get.inode = 0;
    dir_entry_get.name = name;

    
    // Verzeichnis durchsuchen
    if (ext2_dir_foreach_entry(root_handle, parent, ext2_dir_entry_get_handler,
        &dir_entry_get) != 0)
    {
        // Fehler beim duchsuchen
        return 0;
    }

    return dir_entry_get.inode;
}

/**
 * Handler zum Zaehlen der Eintraege in einem Verzeichnis
 */
static int ext2_dir_entry_count_handler(void* param,
    ext2_dir_entry_t* dir_entry)
{
    uint32_t* count = (uint32_t*) param;

    (*count)++;
    return 0;
}

/**
 * Anzahl der Eintraege in einem Verzeichnis zaehlen
 *
 * @param inode Inodenummer des Verzeichnisses
 *
 * @return Anzahl der Eintraege (min 2 fuer dot und dotdot), im Fehlerfall 0
 */
uint32_t ext2_dir_entry_count(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode)
{
    uint32_t count = 0;

    // Eintraege durchgehen
    // Falls ein Fehler auftritt, ist count eh schon 0
    ext2_dir_foreach_entry(root_handle, inode, ext2_dir_entry_count_handler,
        &count);

    return count;
}

/**
 * Neues Verzeichis erstellen
 *
 * @param parent Inode-Handle des Elternverzeichnisses
 * @param name Name des neuen Verzeichnisses
 * @param inode_dest falls != NULL wird hier die Inode-Nummer gespeichert
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
int ext2_dir_create(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name, uint32_t* inode_dest)
{
    int result = 0;
    uint32_t inode_num;
    ext2_ih_t inode;
    int group;

    // Zuerst wird mal ein neuer Inode alloziert
    inode_num = ext2_inode_allocate(root_handle);
    if (inode_num == 0) {
        print_error("Inode konnte nicht alloziert werden");
        result = -1;
        goto end;
    }
    ext2_ih_init(root_handle, &inode, inode_num);
    inode.modified = 1;

    // Inode sauber initialisieren
    memset(inode.data, 0, sizeof(ext2_inode_t));
    inode.data->mode = EXT2_INODE_MODE_DIR;
    
    // Berechtigungen auf 777 setzen
    inode.data->mode |= EXT2_INODE_MODE_USER_READ | EXT2_INODE_MODE_USER_WRITE|
        EXT2_INODE_MODE_USER_EXEC | EXT2_INODE_MODE_GROUP_READ |
        EXT2_INODE_MODE_GROUP_WRITE | EXT2_INODE_MODE_GROUP_EXEC |
        EXT2_INODE_MODE_OTHER_READ | EXT2_INODE_MODE_OTHER_WRITE |
        EXT2_INODE_MODE_OTHER_EXEC;

    // Dot- und Dotdot-Eintraege anlegen
    if (ext2_dir_link(root_handle, &inode, &inode, ".") != 0) {
        print_error("Fehler beim anlegen des Dot-Eintrages");
        result = -3;
        goto end;
    }
    if (ext2_dir_link(root_handle, &inode, parent, "..") != 0) {
        print_error("Fehler beim anlegen des Dotdot-Eintrages");
        result = -4;
        goto end;
    }
    
    // Im Elternverzeichnis einen Link darauf erstellen
    if (ext2_dir_link(root_handle, parent, &inode, name) != 0) {
        print_error("Fehler beim anlegen des Eintrages im Elternverzeichnis");
        result = -5;
        goto end;
    }
    
    // Die Anzahl der Verzeichnisse wird auch im Gruppendeskriptor nachgetragen
    group = (inode_num - 1) / root_handle->superblock.inodes_per_group;
    root_handle->group_descs[group].used_directories++;
    root_handle->group_descs_dirty = 1;
    
    // Falls gewuenscht noch Inodenummer speichern
    if (inode_dest != NULL) {
        *inode_dest = inode_num;
    }
    
end:
    ext2_ih_destroy(root_handle, &inode);
    return result;
}

/**
 * Verzeichnis loeschen
 *
 * @param parent Inode des Elternverzeichnisses
 * @param name Name des Verzeichnisses
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall oder 1 wenn es sich um kein
 * Verzeichnis handelt.
 */
int ext2_dir_delete(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name)
{
    int result = 0;
    uint32_t inode;
    ext2_ih_t ih;
    int group;

    // Versuchen die Inode-Nummer zu funden
    inode = ext2_dir_entry_get(root_handle, parent, name);
    if (inode == 0) {
        print_error("Verzeichnis nicht gefunden");
        return -1;
    }

    ext2_ih_init(root_handle, &ih, inode);

    // Wenn es sich um kein Verzeichnis handelt wird etwas sanfter abgebrochen
    if (!EXT2_INODE_IS_DIR(*ih.data)) {
        result = 1;
        print_error("Ist kein Verzeichnis");
        goto end;
    }
    
    // Pruefen ob das Verzeichnis bis auf . und .. leer ist
    if (ext2_dir_entry_count(root_handle, &ih) > 2) { 
        print_error("Verzeichnis ist nicht leer");
        result = -3;
        goto end;
    }
    
    // Dot und Dotdot-Eintrag loeschen
    if ((ext2_dir_unlink(root_handle, &ih, "..") != 0) || (ext2_dir_unlink(
        root_handle, &ih, ".") != 0))
    {
        print_error("Fehler beim Loeschen der Dot-Eintraege");
        result = -4;
        goto end;
    }

    // Jetzt kann der eigentliche Link auch geloescht werden
    if (ext2_dir_unlink(root_handle, parent, name) != 0) {
        print_error("Verzeichniseintrag konnte nicht geloescht werden");
        result = -5;
        goto end;
    }

    // Die Anzahl der Verzeichnisse wird auch im Gruppendeskriptor nachgetragen
    group = (inode - 1) / root_handle->superblock.inodes_per_group;
    root_handle->group_descs[group].used_directories--;
    root_handle->group_descs_dirty = 1;
    
/*    // Dtime auf != 0 setzen, damit fsck nicht verwirrt ist. ;-)
    memset(ih.data, 0, sizeof(*ih.data));
    ih.data->deletion_time = 1337;
    ih.modified = 1;
*/
end:
    return result;
}

