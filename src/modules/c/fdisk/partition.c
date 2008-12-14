/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Martin Kleusberg.
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

#include <types.h>
#include <stdio.h>
#include <string.h>
#include "partition.h"
#include "typeids.h"

/// Kopie des MBR
unsigned char mbr[512];

/// Array fuer die Primary Partitions
struct partition_t mbr_partitions[4];

long get_end_sector(struct partition_t* p)
{
    return p->data.start_sector+p->data.num_sectors-1;
}
void uncompress_chs(uint8_t* compressed, unsigned int* cylinder,
    unsigned int* head, unsigned int* sector)
{
    *head = compressed[0];
    
    *cylinder = (compressed[1] & 0xC0) << 2;
    *cylinder += compressed[2];
    
    *sector = compressed[1] & 0x3F;
}
void compress_chs(uint8_t* compressed, unsigned int cylinder,
    unsigned int head, unsigned int sector)
{
    if (head > 255) {
        head = 254;
    }
    if (cylinder > 1024) {
        cylinder = 1023;
    }
    if (sector > 63) {
        sector = 63;
    }
    if (sector < 1) {
        sector = 1;
    }
    
    compressed[0] = head;
    
    compressed[1] = (cylinder & 0x300) >> 2;
    compressed[1] += sector;
    
    compressed[2] = cylinder & 0xFF;
}
unsigned int chs2lba(unsigned int cylinder, unsigned int head,
    unsigned int sector)
{
    return(cylinder * device_numheads * SECTORS_PER_TRACK + head *
        SECTORS_PER_TRACK + (sector - 1));
}
void lba2chs(unsigned int lba, unsigned int* cylinder, unsigned int* head,
    unsigned int* sector)
{
    *cylinder = lba / (device_numheads * SECTORS_PER_TRACK);
    *head = (lba % (device_numheads * SECTORS_PER_TRACK)) / SECTORS_PER_TRACK;
    *sector = ((lba % (device_numheads * SECTORS_PER_TRACK)) %
        SECTORS_PER_TRACK) + 1;
}

int apply_changes()
{
    // TODO extended/logical partitions unterstuetzen
    
    // Datei/Laufwerk oeffnen
    FILE* dev = fopen(device_name, "w");
    if (dev == 0) {
        printf("Konnte Laufwerk/Datei \"%s\" nicht oeffnen.\n"
            "Aenderungen wurden nicht geschrieben!\n", device_name);
        return -1;
    }
    
    // Die im RAM liegende Kopie des MBR aktualisieren, ohne jedoch den Code-
    // Teil zu aendern
    int i;
    for (i=0;i<4;i++) {
        // Wenn die Partition existiert, den Datenblock an die korrekte
        // Position kopieren. Wenn nicht, den entsprechenden Bereich
        // einfach mit Nullen fuellen
        if (mbr_partitions[i].exists) {
            memcpy(&mbr[PARTITION_TABLE_START + i *
                sizeof(struct partition_entry_t)], &mbr_partitions[i].data,
                sizeof(struct partition_entry_t));
        } else {
            memset(&mbr[PARTITION_TABLE_START + i *
                sizeof(struct partition_entry_t)], 0,
                sizeof(struct partition_entry_t));
        }
    }
    
    // Die Boot Signature nocheinmal setzen, um sicher zu gehen, dass diese
    // auch wirklich korrekt ist
    uint16_t* signature = (uint16_t*)(mbr + BOOT_SIGNATURE_POS);
    *signature = BOOT_SIGNATURE;
    
    // Neuen MBR schreiben, den Output Buffer leeren und die Datei schliessen
    fwrite(mbr, 1, 512, dev);
    fflush(dev);
    fclose(dev);
    
    return 0;
}

void delete_partition(struct partition_t* p)
{
    // TODO extended/logical partitions unterstuetzen
    
    memset(&(p->data), 0, sizeof(struct partition_entry_t));
    p->number = 0;
    p->exists = 0;
}

static void create_new_mbr_partition_table(void)
{
    memset(&mbr[PARTITION_TABLE_START], 0,
        4 * sizeof(struct partition_entry_t));
    *((uint16_t*)&mbr[BOOT_SIGNATURE_POS]) = BOOT_SIGNATURE;
}

int read_partitions(char* filename)
{
    // TODO extended/logical partitions unterstuetzen
    
    // Dateinamen speichern
    device_name = filename;
    
    // Laufwerk/Datei oeffnen und MBR lesen
    FILE* dev = fopen(filename, "r");
    if (dev == 0) {
        printf("Konnte Laufwerk/Datei \"%s\" nicht oeffnen.\n", filename);
        return -1;
    }
    fread(mbr, 1, 512, dev);
    
    // Boot Signature ueberpruefen
    if (*(uint16_t*)(&mbr[BOOT_SIGNATURE_POS]) != BOOT_SIGNATURE) {
        printf("Ungueltige Boot Signatur!\nfdisk wird eine neue Partitions"
            "tabelle erstellen.\nDies sollte beim Schreiben der Aenderungen "
            "beachtet werden.\n");
        create_new_mbr_partition_table();
    }
    
    // Die Partitionstabelle analysieren und in die internen Strukturen
    // kopieren
    int i;
    for (i=0;i<4;i++) {
        memcpy(&mbr_partitions[i].data,
            &mbr[PARTITION_TABLE_START + i * sizeof(struct partition_entry_t)],
            sizeof(struct partition_entry_t));
        // Ob eine Partition existiert, ueberpruefen wir m.H. des Type Bytes
        if (mbr_partitions[i].data.type) {
            mbr_partitions[i].number = i + 1;
            mbr_partitions[i].exists = 1;
            
            // Handelt es sich um eine extended Partition?
            if (mbr_partitions[i].data.type == TYPE_EXTENDED_PARTITION_1 ||
                mbr_partitions[i].data.type == TYPE_EXTENDED_PARTITION_2 ||
                mbr_partitions[i].data.type == TYPE_EXTENDED_PARTITION_3)
            {
                // Es ist eine! Also diesen Eintrag extra behandeln
                mbr_partitions[i].type = PARTITION_TYPE_EXTENDED;
                printf("Erweiterte/Logische Partitionen werden noch nicht "
                    "unterstuetzt!\n");
            } else {
                mbr_partitions[i].type = PARTITION_TYPE_PRIMARY;
            }
        } else {
            mbr_partitions[i].number = 0;
            mbr_partitions[i].exists = 0;
            mbr_partitions[i].type = PARTITION_TYPE_PRIMARY;
        }
    }
    
    // Datei schliessen
    fclose(dev);
    
    return 0;
}

void create_partition(struct partition_t* p, unsigned int number,
    unsigned int start_lba, unsigned int end_lba)
{
    // TODO extended/logical partitions unterstuetzen
    
    // Die LBA Daten ins CHS Format konvertieren
    unsigned int start_cyl, start_head, start_sec;
    lba2chs(start_lba, &start_cyl, &start_head, &start_sec);
    unsigned int end_cyl, end_head, end_sec;
    lba2chs(end_lba, &end_cyl, &end_head, &end_sec);
    
    // Daraus die "komprimierten" CHS Eintraege erstellen
    uint8_t start_chs[3];
    compress_chs(start_chs, start_cyl, start_head, start_sec);
    uint8_t end_chs[3];
    compress_chs(end_chs, end_cyl, end_head, end_sec);
    
    // Die Strukturen entsprechend anpassen
    p->exists = 1;
    p->number = number;
    p->type = PARTITION_TYPE_PRIMARY;
    p->data.bootable = DEFAULT_BOOTABLE_FLAG;
    p->data.start[0] = start_chs[0];
    p->data.start[1] = start_chs[1];
    p->data.start[2] = start_chs[2];
    p->data.type = DEFAULT_PARTITION_TYPE;
    p->data.end[0] = end_chs[0];
    p->data.end[1] = end_chs[1];
    p->data.end[2] = end_chs[2];
    p->data.start_sector = start_lba;
    p->data.num_sectors = end_lba - start_lba + 1;
}

uint32_t get_disk_signature()
{
    return *(uint32_t*)(&mbr[DISK_SIGNATURE_START]);
}
