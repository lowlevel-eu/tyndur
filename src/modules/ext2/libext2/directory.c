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
 * @param inode Inode-Nummer des Verzeichnisses
 * @param handler Handler der fuer jeden Eintrag aufgerufen wird
 * #param param Erster Parameter fuer handler
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Inode konnte nicht gelesen werden
 *      -2: Daten konnten nicht gelesen werden
 */
int ext2_dir_foreach_entry(ext2_root_handle_t* root_handle, uint32_t inode,
    int (*handler) (void*, ext2_dir_entry_t*), void* param)
{
    ext2_inode_t inode_struct;
    
    // Zuerst den Inode zum Verzeichnis einlesen
    int result = ext2_inode_read(root_handle, inode, &inode_struct);
    if (result != 0) {
        print_error2("Fehler beim Einlesen des Inodes: %d", result);
        return result - 20;
    }
    
    // Pruefen ob es sich auch wirklich um ein Verzeichnis handelt
    if (!EXT2_INODE_IS_DIR(inode_struct)) {
        return -1;
    }
    
    // Jetzt die Daten einlesen
    uint8_t buffer[inode_struct.size];
    if (ext2_inode_data_read(root_handle, &inode_struct, 0x0, inode_struct.
        size, buffer) != inode_struct.size)
    {
        print_error("Fehler beim Einlesen der Verzeichnisdaten");
        return -2;
    }
    
    
    // Eintraege verarbeiten; Dies wird so lange gemacht, bis wir ueber die
    // Groesse des Inodes raus sind
    ext2_raw_dir_entry_t* raw_dir_entry = (ext2_raw_dir_entry_t*) buffer;
    size_t size_done = 0;
    while ((size_done < inode_struct.size) && (result == 0)) {
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
        
            // Inode kopieren
            dir_entry.inode = raw_dir_entry->inode;

            // Namen kopieren und mit Null terminieren
            memcpy(dir_entry.name, raw_dir_entry->name, raw_dir_entry->
                name_len);
            dir_entry.name[raw_dir_entry->name_len] = '\0';

            // Typ setzen
            // Wenn das Filetype-Flag gesetzt ist, geht das ganz praktisch
            if (ext2_feature_incomp(root_handle, EXT2_FEATURE_INCOMP_FILETYPE)
                == TRUE)
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
                ext2_inode_t entry_inode;
                if (ext2_inode_read(root_handle, raw_dir_entry->inode,
                    &entry_inode) != 0)
                {
                    print_error("Konnte Inode zum Verzeichniseintrag nicht "
                        "einlesen");
                    dir_entry.inode = 0;
                }
                
                // Jetzt kann der Typ anhand es Inodes entschieden werden
                if (EXT2_INODE_IS_REG_FILE(entry_inode)) {
                    dir_entry.type = REG_FILE;
                } else if (EXT2_INODE_IS_DIR(entry_inode)) {
                    dir_entry.type = DIRECTORY;
                } else if (EXT2_INODE_IS_SYMLINK(entry_inode)) {
                    dir_entry.type = SYMLINK;
                } else {
                    dir_entry.inode = 0;
                }
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
uint32_t ext2_dir_entry_get(ext2_root_handle_t* root_handle, uint32_t parent,
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
uint32_t ext2_dir_entry_count(ext2_root_handle_t* root_handle, uint32_t inode)
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
 * @param parent Inodenummer des Elternverzeichnisses
 * @param name Name des neuen Verzeichnisses
 * @param inode_dest falls != NULL wird hier die Inode-Nummer gespeichert
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
int ext2_dir_create(ext2_root_handle_t* root_handle, uint32_t parent,
    const char* name, uint32_t* inode_dest)
{
    // Zuerst wird mal ein neuer Inode alloziert
    uint32_t inode_num = ext2_inode_allocate(root_handle);
    ext2_inode_t inode;
    if (inode_num == 0) {
        print_error("Inode konnte nicht alloziert werden");
        return -1;
    }

    // Inode sauber initialisieren
    memset(&inode, 0, sizeof(ext2_inode_t));
    inode.mode = EXT2_INODE_MODE_DIR;
    
    // Berechtigungen auf 777 setzen
    inode.mode |= EXT2_INODE_MODE_USER_READ | EXT2_INODE_MODE_USER_WRITE |
        EXT2_INODE_MODE_USER_EXEC | EXT2_INODE_MODE_GROUP_READ |
        EXT2_INODE_MODE_GROUP_WRITE | EXT2_INODE_MODE_GROUP_EXEC |
        EXT2_INODE_MODE_OTHER_READ | EXT2_INODE_MODE_OTHER_WRITE |
        EXT2_INODE_MODE_OTHER_EXEC;

    // Jetzt noch abspeichern
    if (ext2_inode_write(root_handle, inode_num, &inode) != 0) {
        print_error("Inode konnte nicht gespeichert werden");
        return -2;
    }
    
    // Dot- und Dotdot-Eintraege anlegen
    if (ext2_dir_link(root_handle, inode_num, inode_num, ".") != 0) {
        print_error("Fehler beim anlegen des Dot-Eintrages");
        return -3;
    }
    if (ext2_dir_link(root_handle, inode_num, parent, "..") != 0) {
        print_error("Fehler beim anlegen des Dot-Eintrages");
        return -4;
    }
    
    // Im Elternverzeichnis einen Link darauf erstellen
    if (ext2_dir_link(root_handle, parent, inode_num, name) != 0) {
        print_error("Fehler beim anlegen des Eintrages im Elternverzeichnis");
        return -5;
    }
    
    // Die Anzahl der Verzeichnisse wird auch im Gruppendeskriptor nachgetragen
    int group = (inode_num - 1) / root_handle->superblock.inodes_per_group;
    root_handle->group_descs[group].used_directories++;
    
    // Falls gewuenscht noch Inodenummer speichern
    if (inode_dest != NULL) {
        *inode_dest = inode_num;
    }
    
    ext2_sync(root_handle);

    return 0;
}

/**
 * Verzeichnis loeschen
 *
 * @param parent Inodenummer des Elternverzeichnisses
 * @param name Name des Verzeichnisses
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall oder 1 wenn es sich um kein
 * Verzeichnis handelt.
 */
int ext2_dir_delete(ext2_root_handle_t* root_handle, uint32_t parent,
    const char* name)
{
    // Versuchen die Inode-Nummer zu funden
    uint32_t inode = ext2_dir_entry_get(root_handle, parent, name);
    if (inode == 0) {
        return -1;
    }

    ext2_inode_t inode_struct;
    if (ext2_inode_read(root_handle, inode, &inode_struct) != 0) {
        print_error("Fehler beim laden des Inodes");
        return -2;
    }
    
    // Wenn es sich um kein Verzeichnis handelt wird etwas sanfter abgebrochen
    if (!EXT2_INODE_IS_DIR(inode_struct)) {
        return 1;
    }
    
    // Pruefen ob das Verzeichnis bis auf . und .. leer ist
    if (ext2_dir_entry_count(root_handle, inode) > 2) { 
        print_error("Verzeichnis ist nicht leer");
        return -3;
    }
    
    // Dot und Dotdot-Eintrag loeschen
    if ((ext2_dir_unlink(root_handle, inode, "..") != 0) || (ext2_dir_unlink(
        root_handle, inode, ".") != 0))
    {
        print_error("Fehler beim Loeschen der Dot-Eintraege");
        return -4;
    }

    // Jetzt kann der eigentliche Link auch geloescht werden
    if (ext2_dir_unlink(root_handle, parent, name) != 0) {
        print_error("Verzeichniseintrag konnte nicht geloescht werden");
        return -5;
    }

    // Die Anzahl der Verzeichnisse wird auch im Gruppendeskriptor nachgetragen
    int group = (inode - 1) / root_handle->superblock.inodes_per_group;
    root_handle->group_descs[group].used_directories--;

    return 0;
}

