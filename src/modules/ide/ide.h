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

#ifndef _IDE_H_
#define _IDE_H_
#include "types.h"
#include "collections.h"

#define IDE_MAX_HWIF 2
#define IDE_HWIF_MAX_DRIVES 2
#define IDE_SECTOR_SIZE 512

#define IDE_REG_DEVICE_HEAD 0x6
    #define IDE_DEVICE_HEAD_MASTER 0x0
    #define IDE_DEVICE_HEAD_SLAVE 0x10
#define IDE_REG_DATA 0x0
#define IDE_REG_ERROR 0x1
#define IDE_REG_SECTOR_COUNT 0x2
#define IDE_REG_SECTOR_NUMBER 0x3
#define IDE_REG_LBA_LOW 0x3
#define IDE_REG_LBA_MID 0x4
#define IDE_REG_LBA_HIGH 0x5
#define IDE_REG_PRIMARY_STATUS 0x7
#define IDE_REG_COMMAND IDE_REG_PRIMARY_STATUS
#define IDE_REG_DEVICE_CONTROL 0x208


#define IDE_CMD_READ_SECTORS 0x20
#define IDE_CMD_DEVICE_DIAG 0x90
#define IDE_CMD_IDENTIFY_PACKET_DEVICE 0xA1
#define IDE_CMD_IDENTIFY_DEVICE 0xEC

#define IDE_SOME_JUNK 0x55
#define IDE_SOME_OTHER_JUNK 0xAA


//Kurze Pause, wie sie in der Spezifikation oft vorgeschrieben ist
//FIXME
#define DELAY_400NS() { inb(0x1f1); inb(0x1f1); inb(0x1f1); inb(0x1f1); }
#define TEST_BIT(val, bit) ((val & (1 << bit)) != 0)

#define LOSTIO_TYPES_DRIVE 255
#define LOSTIO_TYPES_PARTITION 254


///Resultat eines IDENTIFY DEVICE Befehls
typedef struct ide_identify_result {
    word general_config;
    word res_1[9];
    char serial_number[20];
    word res_2[3];
    char firmware_revision[8];
    char model_number[40];
    word sectors_per_int;
    word res_3;
    word capabilities;
    word res_4[206];
} ide_identify_result;

///Beinhaltet Informationen zu einem Laufwerk
typedef struct ide_hw_interface_drive_t
{
    enum { none = 0, unknown = 1, pata = 2, patapi = 3, sata=4, satapi=5 } 
        type;

    bool removeable;
    bool support_dma;
    bool support_lba;

    list_t* partitions;
} ide_hw_interface_drive_t;

///Informationen zum Controller
typedef struct ide_hw_interface_t
{
    word portbase;
    byte irq_count;
    byte irq_status_reg;
    ide_hw_interface_drive_t drives[IDE_HWIF_MAX_DRIVES];
} ide_hw_interface_t;


///Eintrag einer Partitionstabelle
typedef struct partition_table_entry_t
{
    ///Wenn das hoechste Bit hier gesetzt ist, ist die Partion Aktiv und kann
    /// gebootet werden.
    byte active;

    byte chs_start[3];
    byte type;
    byte chs_end[3];
    dword lba_start;
    dword size;
} __attribute__((packed)) partition_table_entry_t;

ide_hw_interface_t ide_hw_interfaces[IDE_MAX_HWIF];

void ide_init();
void ide_hw_interface_init(byte index);
word ide_hw_interface_portbase(byte index);
bool ide_drive_init(ide_hw_interface_t* hwif, byte drive_num);

byte ide_read_register(ide_hw_interface_t* hwif, word offset);
word ide_read_word_register(ide_hw_interface_t* hwif, word offset);
void ide_write_register(ide_hw_interface_t* hwif, word offset, byte value);

void ide_select_device(ide_hw_interface_t* hwif, byte device);
bool ide_identify_drive(ide_hw_interface_t* hwif, byte drive);

bool ide_read_sectors(ide_hw_interface_t* hwif, byte drive, qword lba,
    dword count, void* dest);
bool ide_read_sectors_pata_lba(ide_hw_interface_t* hwif, byte drive, qword lba,
    dword count, void* dest);
bool ide_read_sectors_pata_chs(ide_hw_interface_t* hwif, byte drive, qword lba,
    dword count, void* dest);

bool ide_wait_for_not_busy(ide_hw_interface_t* hwif, dword timeout);
void ide_wait_irq(ide_hw_interface_t* hwif);
void ide_clear_irq_count(ide_hw_interface_t* hwif);
void ide_irq14_handler();
void ide_irq15_handler();


//LOSTIO-Handler fuer den rohen Zugriff auf das ganze Laufwerk ohne Partitionen
read_hdl_reply ide_device_read_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount);
size_t ide_drive_write_handler(lostio_filehandle_t* filehandle, size_t blocksize,
    size_t blockcount, void* data);
int ide_drive_seek_handler(lostio_filehandle_t* filehandle, int offset,
    int origin);

//LOSTIO-Handler fuer den Zugriff auf Partitionen
read_hdl_reply ide_partition_read_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount);
size_t ide_partition_write_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount, void* data);
int ide_partition_seek_handler(lostio_filehandle_t* filehandle, int offset,
    int origin);


#endif //ifndef _IDE_H_

