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
 * Berechnen wieviele Bytes der neue Verzeichniseintrag in Anspruch nehmen
 * wird. Dabei wird noch etwas gepaddet.
 *
 * @param name_len Laenge des Namens
 * @return Anzahl Bytes
 */
static size_t ext2_dir_entry_size(ext2_root_handle_t* root_handle,
    size_t name_len)
{
    size_t size = name_len + sizeof(ext2_raw_dir_entry_t);
    
    // Auf EXT2_DIR_ENTRY_PAD runden
    if ((size % EXT2_DIR_ENTRY_PAD) != 0) {
        size += EXT2_DIR_ENTRY_PAD - (size % EXT2_DIR_ENTRY_PAD);
    }

    return size;
}

/**
 * Findet ein Plaetzchen fuer einen neuen Verzeichniseintrag
 *
 * @param dir_inode Verzeichnis-Inode
 * @param name_len Laenge des Namens
 * @param posotion Pointer auf die Variable in der die Position des neuen
 *                  Eintrages gespeichert werden soll.
 * @param record_len Pointer auf die Variable in der die Laenge des
 *                    Verzeichniseintrages abgelegt werden soll.
 *
 * @return 0 bei Erfolg, <0 im Fehlerfall
 */
static int ext2_dir_entry_alloc(ext2_root_handle_t* root_handle,
    ext2_inode_t* dir_inode, size_t name_len, size_t* position,
    size_t* record_len)
{
    // Notwendige Groesse berechnen
    size_t size = ext2_dir_entry_size(root_handle, name_len);

    // Buffer fuer Verzeichnis
    uint8_t buffer[dir_inode->size];
    ext2_raw_dir_entry_t* entry = (ext2_raw_dir_entry_t*) buffer;
    uint32_t entry_pos = 0;

    // Verzeichnis einlesen
    if (ext2_inode_data_read(root_handle, dir_inode, 0, dir_inode->size,
        buffer) != dir_inode->size)
    {
        return -1;
    }
    
    // Alle eintraege durchgehen
    while ((dir_inode->size != 0) && ((intptr_t) entry < ((uintptr_t) (buffer +
        dir_inode->size))))
    {
        // Eine record_len von 0 ist sicher boese. Die macht erstens keinen
        // Sinn, und zweitens fuehrt sie zu einer Endlosschleife ;-)
        if (entry->record_len == 0) {
            print_error("Korruptes Verzeichnis: Eintrag mit Laenge 0");
            return -3;
        }

        // Groesse berechnen, die der Eintrag wirklich braucht
        size_t used_size = ext2_dir_entry_size(root_handle, entry->name_len);
        
        // Der Eintrag passt uns, falls noch genug freier Speicher verfuegbar
        // ist.
        if ((used_size < entry->record_len) && ((entry->record_len - used_size)
            >= size))
        {
            // Laenge fuer den neuen Eintrag errechnen
            *record_len = entry->record_len - used_size;

            // Alten Eintrag kuerzen
            entry->record_len = used_size;
            if (ext2_inode_data_write(root_handle, dir_inode, entry_pos,
                used_size, entry) != used_size)
            {
                return -2;
            }

            // Position auch noch errechnen
            *position = entry_pos + used_size;
            return 0;
        }

        // Naechster Eintrag
        entry = (ext2_raw_dir_entry_t*) ((uintptr_t) entry + entry->
            record_len);
        entry_pos = (uintptr_t) entry - (uintptr_t) buffer;
    }

    // Ich glaube nicht, dass sich hier die Muehe lohnt, den letzten eintrag
    // noch um ein paar Bytes anzufressen. Und wenn der Eintrag so gross ist,
    // dass es was ausmachen wuerde gehoert er eh auf eine Blockgrenze.
    *position = dir_inode->size;
    *record_len = ext2_block_size(root_handle);

    return 0;
}

/**
 * Findet einen Verzeichniseintrag anhand seines Namens
 *
 * @param buffer Verzeichnisinhalt
 * @param size Buffergroesse
 * @param name Name
 * @param posotion Pointer auf die Variable in der die Position des neuen
 *                  Eintrages gespeichert werden soll.
 * @param prev_position Falls != NULL wird hier die Position des vorherigen
 *                      Eintrages gespeichert.
 *
 * @return 0 bei Erfolg, <0 im Fehlerfall
 */
static int ext2_dir_raw_entry_find(ext2_root_handle_t* root_handle,
    void* buffer, size_t size, const char* name, size_t* position,
    size_t* prev_position)
{
    size_t name_len = strlen(name);
    ext2_raw_dir_entry_t* entry = (ext2_raw_dir_entry_t*) buffer;
    size_t entry_pos = 0;
    size_t prev_pos = 0;
    
    
    // Alle eintraege durchgehen
    while ((size != 0) && ((intptr_t) entry < ((uintptr_t) (buffer + size))))
    {
        // Testen ob es sich um den gesuchten handelt
        if ((entry->name_len == name_len) && (memcmp(entry->name, name,
            name_len) == 0))
        {
            // Position speichern und fertig ;-)
            *position = entry_pos;
            
            if (prev_position != NULL) {
                *prev_position = prev_pos;
            }

            return 0;
        }
        // Naechster Eintrag
        entry = (ext2_raw_dir_entry_t*) ((uintptr_t) entry + entry->
            record_len);
        prev_pos = entry_pos;
        entry_pos = (uintptr_t) entry - (uintptr_t) buffer;
    }

    return -2;
}

/**
 * Neuer Verzeichnisentrag erstellen
 *
 * @param dir_inode Inode des Vezeichnisses
 * @param target_inode Ziel des Links
 * @param name Pointer auf den Namen. Dieser wird abgeschnitten
 *
 * @return 0 im Erfolgsfall, sonst < 0
 */
int ext2_dir_link(ext2_root_handle_t* root_handle, uint32_t dir_inode_num,
    uint32_t target_inode, const char* name)
{
    // Inode des Elternverzeichnisses laden
    ext2_inode_t dir_inode;
    if (ext2_inode_read(root_handle, dir_inode_num, &dir_inode) != 0) {
        print_error("Verzeichnis-Inode konnte nicht geladen werden");
        return -1;
    }

    // Jetzt wird der Inode noch geladen, damit der Referenzzaehler erhoeht
    // werden und der Typ ausgelesen werden kann 
    ext2_inode_t inode;
    if (ext2_inode_read(root_handle, target_inode, &inode) != 0) {
        print_error("Inode konnte nicht geladen werden");
        return -2;
    }

    // Berechnen wieviel Speicher benoetigt wird fuer den Link
    // Name wenn noetig beschneiden
    size_t name_len = strlen(name);
    if (name_len > EXT2_MAX_NAME_LEN) {
        name_len = EXT2_MAX_NAME_LEN;
    } 
       

    // Position an der der neue Eintrag geschrieben werden kann
    size_t position;
    size_t record_len;
    if (ext2_dir_entry_alloc(root_handle, &dir_inode, name_len, &position,
        &record_len) != 0)
    {
        print_error("Fehler beim Suchen eines Verzeichnis-Eintrages");
        return -6;
    }

    // Zuerst wird der Verzeichniseintrag in einem Buffer zusammenkopiert
    uint8_t buffer[record_len];
    ext2_raw_dir_entry_t* raw_entry = (ext2_raw_dir_entry_t*) buffer;
    raw_entry->inode = target_inode;
    raw_entry->record_len = record_len;
    raw_entry->name_len = name_len;

    // Name kopieren
    memcpy(raw_entry->name, name, name_len);
    if (name_len < EXT2_MAX_NAME_LEN) {
        raw_entry->name[name_len] = 0;
    }
    
    // Dateityp im Verzeichniseintrag hinterlegen
    if (ext2_feature_incomp(root_handle, EXT2_FEATURE_INCOMP_FILETYPE) == TRUE)
    {
        // Dies muss nur geschehen, wenn das Filetype-Flag auch gesetzt ist
        if (EXT2_INODE_IS_REG_FILE(inode)) {
            raw_entry->type = EXT2_FT_REG_FILE;
        } else if (EXT2_INODE_IS_REG_FILE(inode)) {
            raw_entry->type = EXT2_FT_DIR;
        } else if (EXT2_INODE_IS_SYMLINK(inode)) {
            raw_entry->type = EXT2_FT_SYMLINK;
        } else {
            return 0;
        }
    } else {
        // Sonst muss hier einfach 0 stehen, weil das aeltere Treiber, die
        // name_len und filetyp zusammen als einen 16-Bit wert haben,
        // verwirren koennte
        raw_entry->type = 0;
    }

    // Eintrag abspeichern
    if (ext2_inode_data_write(root_handle, &dir_inode, position, record_len,
        buffer) != record_len)
    {
        print_error("Verzeichniseintrag wurde nicht vollstaendig geschrieben");
        return -1;
    }
    
    // Verzeichnis-Inode speichern
    if (ext2_inode_write(root_handle, dir_inode_num, &dir_inode) != 0) {
        print_error("Verzeichnis-Inode konnte nicht geschrieben werden");
        return -4;
    }
    
    // Hier gibt es eine kleine Besonderheit, fals target_inode und dir_inode
    // gleich sind. Denn dann wuerde das 2. schreiben die Aenderungen des
    // ersten ueberschreiben. Deshalb wird der Inode in diesem Fall nochmal
    // eingelesen
    if ((target_inode == dir_inode_num) &&(ext2_inode_read(root_handle,
        target_inode, &inode) != 0))
    {
        print_error("Verzeichnis-Inode konnte nicht geladen werden");
        return -5;
    }

    // Anzahl der Links auf den Inode erhoehen
    inode.link_count++;

    // Inode wieder abspeichern
    if (ext2_inode_write(root_handle, target_inode, &inode) != 0) {
        print_error("Inode konnte nicht geschrieben werden");
        return -6;
    }
    return 0;
}

/**
 * Verzeichnisentrag loeschen
 *
 * @param dir_inode Inode des Vezeichnisses
 * @param target_inode Ziel des Links
 * @param name Pointer auf den Namen. Dieser wird abgeschnitten
 *
 * @return 0 im Erfolgsfall, sonst < 0
 */
int ext2_dir_unlink(ext2_root_handle_t* root_handle, uint32_t dir_inode_num,
    const char* name)
{
    // Inode des Elternverzeichnisses laden
    ext2_inode_t dir_inode;
    if (ext2_inode_read(root_handle, dir_inode_num, &dir_inode) != 0) {
        print_error("Verzeichnis-Inode konnte nicht geladen werden");
        return -1;
    }
    
    // Inhalt auch laden
    uint8_t buffer[dir_inode.size];
    if (ext2_inode_data_read(root_handle, &dir_inode, 0, dir_inode.size,
        buffer) != dir_inode.size)
    {
        print_error("Verzeichnisinhalt konnte nicht vollstaendig geladen "
            "werden");
        return -2;
    }
 
    // Eintrag suchen
    size_t position;
    size_t prev_position;
    if (ext2_dir_raw_entry_find(root_handle, buffer, dir_inode.size, name,
        &position, &prev_position) != 0) 
    {
        print_error("Verzeichniseintrag nicht gefunden");
        return -4;
    }

    ext2_raw_dir_entry_t* entry = (ext2_raw_dir_entry_t*) (buffer + position);
    ext2_raw_dir_entry_t* prev_entry = (ext2_raw_dir_entry_t*) (buffer +
        prev_position);
    
    // Jetzt kann der betroffene Inode geladen werden
    ext2_inode_t inode;
    uint32_t target_inode = entry->inode;
    
    if (ext2_inode_read(root_handle, target_inode, &inode) != 0) {
        print_error("Ziel-Inode konnte nicht geladen werden");
        return -5;
    }

    // Wenn es sich nicht um den ersten Eintrag handelt, muss der Speicher dem
    // Vorherigen angehaengt werden.
    if (position != 0) {
        prev_entry->record_len += entry->record_len;
    } else {
        entry->inode = 0;
    }

  
    // So, jetzt koennen die Daten wieder gespeichert werden
     if (ext2_inode_data_write(root_handle, &dir_inode, 0, dir_inode.size,
        buffer) != dir_inode.size)
    {
        print_error("Verzeichnisinhalt konnte nicht vollstaendig gespeichert "
            "werden");
        return -6;
    }
    
    // Referenzzaehler dekrementieren. Wenn dieser danach den Wert 0 hat, kann
    // der Inode geloescht werden.
    if (--inode.link_count == 0) {
        // Daten freigeben
        inode.mode = 0;
        ext2_inode_data_truncate(root_handle, &inode, 0);
        // Inode freigeben
        ext2_inode_free(root_handle, target_inode);
    }
    
    if (ext2_inode_write(root_handle, target_inode, &inode) != 0) {
        print_error("Inode konnte nicht gespeichert werden");
        return -7;
    }
    
    // Wenn wir hier ankommen, ist alles in Ordnung ;-)
    return 0;
}

