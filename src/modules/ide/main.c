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

#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "io.h"
#include "lostio.h"
#include "ports.h"
#define _USE_START_
#include "init.h"


#define IDE_MAX_HWIF 2
#define IDE_HWIF_MAX_DRIVES 2

#define IDE_REG_DEVICE_HEAD 0x6
    #define IDE_DEVICE_HEAD_MASTER 0x0
    #define IDE_DEVICE_HEAD_SLAVE 0x10
#define IDE_REG_DATA 0x0
#define IDE_REG_ERROR 0x1
#define IDE_REG_SECTOR_COUNT 0x2
#define IDE_REG_SECTOR_NUMBER 0x3
#define IDE_REG_LBA_LOW 0x4
#define IDE_REG_LBA_MID 0x5
#define IDE_REG_PRIMARY_STATUS 0x7
#define IDE_REG_COMMAND IDE_REG_PRIMARY_STATUS
#define IDE_REG_DEVICE_CONTROL 0x208

//Kurze Pause, wie sie in der Spezifikation oft vorgeschrieben ist
//FIXME
#define DELAY_400NS() { inb(0x1f1); inb(0x1f1); inb(0x1f1); inb(0x1f1); }
#define TEST_BIT(val, bit) ((val & (1 << bit)) != 0)
#define LOSTIO_TYPES_DRIVE 255

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
    enum { none = 0, unknown = 1, pata = 2, patapi = 3, sata=4, satapi=5 }  type;
    bool removeable;
    bool support_dma;
    bool support_lba;
} ide_hw_interface_drive_t;

///Informationen zum Controller
typedef struct ide_hw_interface_t
{
    word portbase;
    byte irq_count;
    ide_hw_interface_drive_t drives[IDE_HWIF_MAX_DRIVES];
} ide_hw_interface_t;

ide_hw_interface_t ide_hw_interfaces[IDE_MAX_HWIF];

void ide_init();
void ide_hw_interface_init(byte index);
word ide_hw_interface_portbase(byte index);

byte ide_read_register(ide_hw_interface_t* hwif, word offset);
word ide_read_word_register(ide_hw_interface_t* hwif, word offset);
void ide_write_register(ide_hw_interface_t* hwif, word offset, byte value);

void ide_select_device(ide_hw_interface_t* hwif, byte device);
bool ide_identify_drive(ide_hw_interface_t* hwif, byte drive);

bool ide_wait_for_not_busy(ide_hw_interface_t* hwif, dword timeout);
void ide_wait_irq(ide_hw_interface_t* hwif);
void ide_clear_irq_count(ide_hw_interface_t* hwif);
void ide_irq14_handler();
void ide_irq15_handler();


/**
 * Haupfunktion ;-)
 */
int main(int argc, char* argv[])
{
    lostio_init();
    
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    lostio_type_directory_use();
    //Typ fuer ein Laufwerk
    typehandle->id          = LOSTIO_TYPES_DRIVE;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = NULL;
    typehandle->write       = NULL;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);

    ide_init();
    
    init_service_register("ide");
    while(1) yield();
}


/**
 * Den IDE-Treiber initialisieren
 */
void ide_init()
{
    int i, device_count;
    char* name = "/hd ";
    device_count = 0;
    register_intr_handler(0x2E, &ide_irq14_handler);
    register_intr_handler(0x2F, &ide_irq15_handler);
    for (i = 0; i < IDE_MAX_HWIF; i++) {
        //Controller bereit machen
        ide_hw_interface_init(i);
        
        //Wenn ein Master existiert, wird die zugehörige Geratedatei eingelegt
        if (ide_hw_interfaces[i].drives[0].type != none) {
            name[3] = (char) ('1' + device_count++);
            vfstree_create_node(name, LOSTIO_TYPES_DRIVE, 0, NULL, 0);
        }

        //Wenn ein Slave existiert, wird die zugehörige Geratedatei eingelegt
        if(ide_hw_interfaces[i].drives[1].type != none) {
            name[3] = (char) ('1' + device_count++);
            vfstree_create_node(name, LOSTIO_TYPES_DRIVE, 0, NULL, 0);
        }
    }
}


/**
 * Initialisiert einen Kanal des Controllers
 *
 * @param index Id des Kontrollerkanals
 */
void ide_hw_interface_init(byte index)
{
    ide_hw_interface_t* hwif = &ide_hw_interfaces[index];
    hwif->portbase = ide_hw_interface_portbase(index);
    
    //Am Anfang wird angenommen, dass garkeine Laufwerke vorhanden sind.
    hwif->drives[0].type = none;
    hwif->drives[1].type = none;
    
    
    byte sector_count, sector_number;
    ide_write_register(hwif, IDE_REG_DEVICE_CONTROL, 8);

    //Puefen ob ein Geraet als Master da ist
    ide_select_device(hwif, 0);
    
    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, 0x55);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, 0xaa);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, 0xaa);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, 0x55);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, 0x55);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, 0xaa);
    
    sector_count = ide_read_register(hwif, IDE_REG_SECTOR_COUNT);
    sector_number = ide_read_register(hwif, IDE_REG_SECTOR_NUMBER);

    if ((sector_count == 0x55) && (sector_number == 0xaa)) {
        hwif->drives[0].type = unknown;
    }
    else {
        hwif->drives[0].type = none;
    }

    //Puefen ob ein Geraet als Slave da ist
    ide_select_device(hwif, 1);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, 0x55);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, 0xaa);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, 0xaa);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, 0x55);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, 0x55);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, 0xaa);
    
    sector_count = ide_read_register(hwif, IDE_REG_SECTOR_COUNT);
    sector_number = ide_read_register(hwif, IDE_REG_SECTOR_NUMBER);

    if ((sector_count == 0x55) && (sector_number == 0xaa)) {
        hwif->drives[1].type = unknown;
    }
    else {
        hwif->drives[1].type = none;
    }
    

    //Master-Laufwerk identifizieren
    if ((hwif->drives[0].type != none) && 
        (ide_identify_drive(hwif, 0) == FALSE))
    {
        hwif->drives[0].type = none;
    }
    
    //Slave-Laufwerk identifizieren
    if ((hwif->drives[1].type != none) &&
        (ide_identify_drive(hwif, 1) == FALSE))
    {
        hwif->drives[1].type = none;
    }
}


/**
 * Portbasis fuer einen Controller-Kanal ausfindig machen. Und beim Kernel
 * registrieren.
 *
 * @param index Id des Kanals
 *
 * @return Portbasis
 */
word ide_hw_interface_portbase(byte index)
{
    word portbase = 0;
    switch (index) {
        case 0: portbase = 0x1F0; break;
        case 1: portbase = 0x170; break;
    }

    if ((portbase != 0) && ((request_ports(portbase - 1, 10) == FALSE) 
        || (request_ports(portbase + 0x208, 1) == FALSE)))
    {
        puts("Ports konnten nicht angefordert werden!");
        portbase = 0;
    }
    return portbase;
}


/**
 * Aktuelles Laufwerk fuer den Controller auswaehlen.
 *
 * @param hwif      Pointer auf Interface-Struktur
 * @param device    Nummer des Laufwerks
 */
void ide_select_device(ide_hw_interface_t* hwif, byte device)
{
    if (device == 0) {
        ide_write_register(hwif, IDE_REG_DEVICE_HEAD, IDE_DEVICE_HEAD_MASTER);
    }
    else if (device == 1) {
        ide_write_register(hwif, IDE_REG_DEVICE_HEAD, IDE_DEVICE_HEAD_SLAVE);
    }
    DELAY_400NS();
}


/**
 * Inhalt aus einem Register des Controllers auslesen
 
 * @param hwif      Pointer auf Interface-Struktur
 * @param register  Register das beschrieben werden soll
 *
 * @return          Registerinhalt
 */
byte ide_read_register(ide_hw_interface_t* hwif, word reg)
{
    //printf("IDE-read from %x:", hwif->portbase + reg);
    byte result = inb(hwif->portbase + reg);
    //printf(" %x\n", result);
    return result;
}


/**
 * Inhalt aus einem word-Register des Controllers auslesen
 *
 * @param hwif      Pointer auf Interface-Struktur
 * @param register  Register das beschrieben werden soll
 *
 * @return          Registerinhalt
 */
word ide_read_word_register(ide_hw_interface_t* hwif, word reg)
{
    //printf("IDE-word-read from %x:", hwif->portbase + reg);
    word result = inw(hwif->portbase + reg);
    //printf(" %x\n", result);
    return result;
}


/**
 * Wert in ein Register des Controllers schreiben
 *
 * @param hwif      Pointer auf Interface-Struktur
 * @param register  Register das beschrieben werden soll
 * @param value     Wert
 */
void ide_write_register(ide_hw_interface_t* hwif, word reg, byte value)
{
    //printf("IDE-write to %x: %x\n", hwif->portbase + reg, value);
    outb(hwif->portbase + reg, value);
}


/**
 * Wartet bis das BSY-Flag im Statusregister nicht mehr gesetzt ist.
 *
 * @param hwif      Pointer auf Interface-Struktur
 * @param time      Zeit wie lange maximal gewartet wird.
 *
 * @return          FALSE bei einem Timeout, sonst TRUE
 */
bool ide_wait_for_not_busy(ide_hw_interface_t* hwif, dword time)
{
    qword timeout = get_tick_count() + time * 1000;
    byte status;

    while (timeout > get_tick_count()) {
        status = ide_read_register(hwif, IDE_REG_PRIMARY_STATUS);

        if ((status & 0x80) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}


/**
 * Versucht alle Informationen zu einem Laufwerk zu finden.
 *
 * @param hwif      Pointer auf Interface-Struktur
 * @param drive     Laufwerksnummer
 *
 * @return          TRUE wenn alle Informationen zusammengesucht wurden.
 *                  FALSE wenn das Laufwerk nicht vorhanden ist, oder ein
 *                      anderer Fehler aufgetretet ist.
 */
bool ide_identify_drive(ide_hw_interface_t* hwif, byte drive)
{
    //Zuerst versuchen ausfindig zu machen, welches Interface das Laufwerk
    // benutzt. Dafür wird ein DEVICE DIAG gemacht. Das bringt das Laufwerk
    // dazu, die Signatur in die vorgeschriebenen Register zu schreiben. Die
    // Signatur ist aber noch kein Beweis, dass das Laufwerk tatsaechlich 
    // exisitert!
    ide_select_device(hwif, drive);
    ide_write_register(hwif, IDE_REG_COMMAND, 0x90);
    DELAY_400NS();
 
    byte sector_count = ide_read_register(hwif, IDE_REG_SECTOR_COUNT);
    byte sector_number = ide_read_register(hwif, IDE_REG_SECTOR_NUMBER);

    if ((sector_count == 1) && (sector_number == 1)) {
        byte status = ide_read_register(hwif, IDE_REG_PRIMARY_STATUS);
        byte lba_low = ide_read_register(hwif, IDE_REG_LBA_LOW);
        byte lba_mid = ide_read_register(hwif, IDE_REG_LBA_MID);

        if ((lba_low == 0x14) && (lba_mid == 0xeb)) {
            hwif->drives[drive].type = patapi;
        }
        else if ((lba_low == 0x69) && (lba_mid == 0x96)) {
            hwif->drives[drive].type = satapi;
        }
        else if ((status != 0) && (lba_low == 0) && (lba_mid == 0)) {
            hwif->drives[drive].type = pata;
        }
        else if ((status != 0) && (lba_low == 0x3c) && (lba_mid == 0xc3)) {
            hwif->drives[drive].type = sata;
        }
    }

    //IRQ-Zaehler zurueck setzen, da ein IRQ auftreten wird, sobald die Daten
    // bereit sind.
    ide_clear_irq_count(hwif);

    //Laufwerk auswaehlen und Befehl zum Identifizieren abschicken
    ide_select_device(hwif, drive);
    
    //Wenn das Laufwerk kein PI implementiert, wird das normale IDENTIFY DEVICE
    // benutzt. Sonst muessen wir das IDENTIFY PACKET DEVICE Kommando benutzen
    if ((hwif->drives[drive].type == pata) || 
        (hwif->drives[drive].type == sata) ||
        (hwif->drives[drive].type == unknown))
    {
        ide_write_register(hwif, IDE_REG_COMMAND, 0xec);
        DELAY_400NS();
    }
    else {
        ide_write_register(hwif, IDE_REG_COMMAND, 0xa1);
        DELAY_400NS();
    }
    
    qword timeout = get_tick_count() + 5000000;
    byte status = 0;
    //Solange das Data-Ready-Bit nicht gesetzt ist
    while ((status & 0x40) == 0) {
        status = ide_read_register(hwif, IDE_REG_PRIMARY_STATUS);

        //Ein Fehler ist aufgetreten, entweder Fehler-Bit gesetzt, oder Busy
        // geloescht und Data-Ready auch gelöscht oder ein Timeout
        if (((status & 1) == 1) || (((status & 0x80) == 0) && 
                ((status & 0x40) == 0)) || 
            (timeout < get_tick_count()))
        {
            hwif->drives[drive].type = none;
            return FALSE;
        }
    }
    
    //Auf den IRQ warten, der signalisiert, dass die Daten abholbereit sind
    ide_wait_irq(hwif);
    
    int i;
    ide_identify_result identify;
    word* wdest = (word*) &identify;

    //Die 256 Identifikations-Words einlesen
    for (i = 0; i < 256; i++) {
        wdest[i] = ide_read_word_register(hwif, IDE_REG_DATA);
    }
    

    if (!TEST_BIT(identify.general_config, 15)) {
        hwif->drives[drive].type = unknown;
    }
    
    hwif->drives[drive].removeable = TEST_BIT(identify.general_config, 7);
    
    //Laufwerk hat kein PI
    if ((hwif->drives[drive].type == pata) ||
        (hwif->drives[drive].type == sata) ||
        (hwif->drives[drive].type == unknown))
    {
        hwif->drives[drive].support_dma = TEST_BIT(identify.capabilities, 8);
        if (hwif->drives[drive].support_dma) puts("Unterstuetzt dma");
        hwif->drives[drive].support_lba = TEST_BIT(identify.capabilities, 9);
        if (hwif->drives[drive].support_lba) puts("Unterstuetzt lba");

    }
    else {

    }

    
    return TRUE;
}


/**
 * IRQ-Handler fuer Primaeren IDE-Kanal
 */
void ide_irq14_handler()
{
    ide_hw_interfaces[0].irq_count++;
}


/**
 * IRQ-Handler fuer Sekundaeren IDE-Kanal
 */
void ide_irq15_handler()
{
    ide_hw_interfaces[1].irq_count++;
}


/**
 * Zaehler fuer aufgetretene IRQs zuruecksetzen
 *
 * @param hwif Pointer auf Interface-Struktur
 */
void ide_clear_irq_count(ide_hw_interface_t* hwif)
{
    hwif->irq_count = 0;
}


/**
 * Warten bis der IRQ-Zaehler nicht mehr 0 ist.
 *
 * @param hwif Pointer auf Interface-Struktur
 */
void ide_wait_irq(ide_hw_interface_t* hwif)
{
    while (hwif->irq_count == 0);
}

