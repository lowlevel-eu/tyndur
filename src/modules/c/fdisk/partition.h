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

#ifndef __FDISK_PARTITION_H__
#define __FDISK_PARTITION_H__

#include <stdint.h>

#define DISK_SIGNATURE_START 0x1B8
#define PARTITION_TABLE_START 0x1BE
#define BOOT_SIGNATURE_POS 0x1FE
#define BOOT_SIGNATURE 0xAA55

#define SECTORS_PER_TRACK 63

#define BOOTFLAG_SET 0x80
#define BOOTFLAG_UNSET 0x00

#define DEFAULT_PARTITION_TYPE 0x83
#define DEFAULT_BOOTABLE_FLAG BOOTFLAG_UNSET

#define PARTITION_TYPE_PRIMARY 0
#define PARTITION_TYPE_EXTENDED 1
#define PARTITION_TYPE_LOGICAL 2

/// Ein Zeiger auf argv[1] oder besser gesagt den Laufwerksnamen
char* device_name;

/// Anzahl der Heads
unsigned long device_numheads;

/** 
 * Ein einzelner Eintrag der Partitionstabelle, wie er auch auf der Festplatte
 * vorkommt.
 */
struct partition_entry_t
{
    uint8_t bootable;        ///< Bootflag (0x00 nicht bootable, 0x80 bootable)
    uint8_t start[3];        ///< Startposition im CHS Format
    uint8_t type;            ///< Partitionstyp
    uint8_t end[3];          ///< Endposition im CHS Format
    uint32_t start_sector;   ///< LBA Startposition
    uint32_t num_sectors;    ///< Anzahl der Sektoren in der Partition
} __attribute__ ((__packed__));

/** Alle Informationen ueber eine Partition, die fdisk benoetigt */
struct partition_t
{
    int exists;              ///< 1, wenn die Partition existiert; 0 wenn nicht
    unsigned int number;     ///< Nummer der Partition, beginnend mit 1
    int type;                ///< 0 = primary, 1 = extended, 2 = logical
    
    struct partition_entry_t data; ///< Die eigentlichen Daten
};

/// Berechnet den Endsektor einer Partition
long get_end_sector(struct partition_t* p);
/// Gibt die Disk Signature des zurueck
uint32_t get_disk_signature(void);
/// Zerlegt die 3 Byte grossen CHS Eintraege
void uncompress_chs(uint8_t* compressed, unsigned int* cylinder,
    unsigned int* head, unsigned int* sector);
/// Erstellt die 3 Byte grossen CHS Eintraege
void compress_chs(uint8_t* compressed, unsigned int cylinder,
    unsigned int head, unsigned int sector);
/// Konvertiert CHS nach LBA
unsigned int chs2lba(unsigned int cylinder, unsigned int head,
    unsigned int sector);
/// Konvertiert LBA nach CHS
void lba2chs(unsigned int lba, unsigned int* cylinder, unsigned int* head,
    unsigned int* sector);
/// Partitionsdaten von der Fesplatte lesen. Gibt 0 zurueck, wenn erfolgreich
int read_partitions(char* filename);

/**
 * Neuen MBR erstellen und auf Festplatte schreiben. Gibt 0 zurueck,
 * wenn erfolgreich
 */
int apply_changes(void);

/// Loescht eine Partition
void delete_partition(struct partition_t* p);
/// Erstellt eine neue Partition
void create_partition(struct partition_t* p, unsigned int number,
    unsigned int start_lba, unsigned int end_lba);

/// Kopie des MBR 
extern unsigned char mbr[512];

/// Array fuer die Primary Partitions
extern struct partition_t mbr_partitions[4];

#endif
