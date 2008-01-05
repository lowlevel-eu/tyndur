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

#include "ext2.h"


/**
 * EXT2-Dateisystem mounten
 */
bool ext2_mount(ext2_root_handle_t* root_handle)
{
    struct ext2_superblock* superblock = &root_handle->superblock;

    // Zuerst wird der Superblock eingelesen
    int result = ext2_superblock_read(root_handle);
    if (result != 0) {
        print_error2("Fehler beim Lesen des Superblocks!", result);
        return FALSE;
    }
    
    // Magic Number ueberpruefen
    if (superblock->magic != EXT2_MAGIC) {
        print_error("Fehler beim Pruefen der Magic Number");   
        return FALSE;
    }
    
    // Revision pruefen
    if (superblock->revision > EXT2_REV_DYNAMIC) {
        print_error("Dieser Treiber unterstuetzt im Moment nur normale ext2-"
            "Dateisysteme (Revision 0 und 1)");
        return FALSE;
    }

    // Falls es sich um REV1 handelt werden jetzt die Features geprueft
    root_handle->read_only = FALSE;
    if (superblock->revision == EXT2_REV_DYNAMIC) {
        // Auf inkompatible pruefen
        uint32_t incomp = superblock->features_incompat &
            (~EXT2_FEATURES_SUP_INCOMP);
        if (incomp != 0) {
            print_error2x("Dateisytem nicht unterstuezt. Inkompatible Features"
                ", die nicht unterstuetzt werden, sind vorhanden: ", incomp);
            return FALSE;
        }

        // Nur Lese-Flags testen
        uint32_t rocomp = superblock->features_ro &
            (~EXT2_FEATURES_SUP_ROCOMP);
        if (rocomp != 0) {
            print_error2x("Dateisystem kann nur gelesen werden da Features "
                "vorhanden sind, die eine nur Lesen-Kompatibilitaet "
                "garantieren:", rocomp);
            root_handle->read_only = TRUE;
        }
    }
    
    // Blockcache vorbereiten
    if (ext2_block_init(root_handle) != TRUE) {
        print_error("Fehler beim initialisieren des Block Caches");
        return FALSE;
    }

    // Gruppen laden
    result = ext2_groups_init(root_handle);
    if (result != 0) {
        print_error2("Fehler beim Laden der Gruppen:", result);
        return FALSE;
    }

    return TRUE;
}

/**
 * Noch im Speicher liegende Daten auf den Datentraeger schreiben
 */
bool ext2_sync(ext2_root_handle_t* root_handle)
{
    // Gruppendeskriptoren aktualisieren
    if (ext2_groups_write(root_handle) != 0) {
        print_error("Fehler beim Schreiben der Gruppendeskriptoren!");
        return FALSE;
    }

    // Superblocks aktualisieren
    if (ext2_superblock_write(root_handle) != 0) {
        print_error("Fehler beim schreiben des Superblocks!");
        return FALSE;
    }
    
    // Block-Cache synchronisieren
    if (ext2_block_sync(root_handle) != 0) {
        print_error("Fehler beim synchronisieren des Block-Caches");
        return FALSE;
    }

    return TRUE;
}

