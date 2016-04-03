/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
#include <stdlib.h>

#include "cdi/lists.h"
#include "cdi/storage.h"
#include "libpartition.h"

#define DEBUG(x)

/**
 * Roher Partitionstabelleneintrag
 */
struct raw_entry {
    uint8_t active;
    uint8_t begin_chs[3];
    uint8_t type;
    uint8_t end_chs[3];
    uint32_t start;
    uint32_t size;
} PACKED;

struct partition_table {
    struct partition entries[4];
};

extern int cdi_storage_read(struct cdi_storage_device* device, uint64_t pos,
    size_t size, void* dest);
/**
 * Uebergebene Partitionstabelle anhand des Sektors fuellen
 *
 * @return 1 wenn die Tabelle erfolgreich gefuellt wurde, 0 sonst
 */
static int partition_table_fill(struct partition_table* table, void* sector)
{
    struct raw_entry* entry = (struct raw_entry*) (sector +
        PARTITION_TABLE_OFFSET);
    uint16_t* signature = sector + PARTITION_TABLE_SIG_OFFSET;
    int i;

    // Zuerst die Signatur pruefen, denn ohne die muss garnicht weiter gesucht
    // werden.
    if (*signature != PARTITION_TABLE_SIGNATURE) {
        return 0;
    }

    // Wenn die Signatur existiert koennen die einzelnen Eintraege ausgelesen
    // werden
    for (i = 0; i < 4; i++) {
        // Wenn 0 als Groesse eingetragen ist, ist der Eintrag unbenutzt
        if (entry->size == 0) {
            table->entries[i].used = 0;
        } else {
            table->entries[i].used = 1;

            // Fuer erweiterte Partitionen wollen wir generell Typ 0x5, die
            // anderen aus Windows und Linux werden deshalb ausgetauscht.
            if ((entry->type == 0x0F) || (entry->type == 0x85)) {
                table->entries[i].type = PARTITION_TYPE_EXTENDED;
            } else {
                table->entries[i].type = entry->type;
            }

            table->entries[i].start = entry->start * 512;
            table->entries[i].size = entry->size * 512;
        }
        entry++;
    }
    return 1;
}

static void parse_ext_partitions(struct cdi_storage_device* dev,
                                 cdi_list_t partitions,
                                 uint64_t ebr_offset)
{
    uint8_t ebr[512];
    struct raw_entry* entry;
    struct partition* part;
    uint16_t* signature;
    int i = 4;

    do {
        /* EBR vom Medium lesen */
        if (cdi_storage_read(dev, ebr_offset, 512, ebr) != 0) {
            DEBUG("Fehler beim Einlesen der erweiterten Partitionen");
            return;
        }

        /* Erweiterte Partition in die Liste eintragen */
        entry = (struct raw_entry*) &ebr[PARTITION_TABLE_OFFSET];
        signature = (uint16_t*) &ebr[PARTITION_TABLE_SIG_OFFSET];
        if (*signature != PARTITION_TABLE_SIGNATURE) {
            DEBUG("Falsche Signatur beim Einlesen der erweiterten Partitionen");
            return;
        }

        part = malloc(sizeof(*part));
        *part = (struct partition) {
            .dev    = dev,
            .used   = 1,
            .type   = entry->type,
            .number = i++,
            .start  = ebr_offset + entry->start * 512,
            .size   = entry->size * 512,
        };
        cdi_list_push(partitions, part);

        /* Prüfen, ob eine weitere Partition in der Liste ist */
        entry++;
        if (entry->start != 0) {
            ebr_offset += entry->start * 512;
        }
    } while (entry->start);
}

/**
 * Partitionstabelle auf einem Massenspeichergeraet verarbeiten
 *
 * @param dev Geraet, auf dem nach Partitionen gesucht werden soll
 * @param partitions Liste, in die gefundene Partitionen eingefuegt werden
 * sollen (als struct partition)
 */
void cdi_tyndur_parse_partitions(struct cdi_storage_device* dev,
    cdi_list_t partitions)
{
    struct partition_table partition_table;
    uint8_t mbr[512];
    int i;

    // Die Partitionstabelle liegt im MBR
    if (cdi_storage_read(dev, 0, 512, mbr) != 0) {
        DEBUG("Fehler beim Einlesen der Partitionstabelle");
        return;
    }

    // Partitionstabelle Verarbeiten
    if (!partition_table_fill(&partition_table, mbr)) {
        DEBUG("Fehler beim Verarbeiten der Partitionstabelle");
        return;
    }

    // Ansonsten wird die Tabelle jetzt verarbeitet
    for (i = 0; i < 4; i++) {
        if (partition_table.entries[i].used) {
            // Erweiterter Eintrag => ueberspringen
            if (partition_table.entries[i].type == PARTITION_TYPE_EXTENDED) {
                parse_ext_partitions(dev, partitions,
                                     partition_table.entries[i].start);
                continue;
            }

            struct partition* part = malloc(sizeof(*part));
            *part = partition_table.entries[i];
            part->dev = dev;
            part->number = i;

            cdi_list_push(partitions, part);
        }
    }
}
