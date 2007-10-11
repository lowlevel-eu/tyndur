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

#include <types.h>
#include "syscall.h"
#include "rpc.h"
#include <stdlib.h>
#include <stdio.h>
#include "io.h"
#include "lostio.h"
#include "ports.h"
#include "ide.h"
#include <stdint.h>
#include "log.h"

#define _USE_START_
#include "init.h"

// Alle Registerzugriffe Protokollieren
//#define DEBUG_HARDCORE

/**
 * Haupfunktion ;-)
 */
int main(int argc, char* argv[])
{
    lostio_init();
    log_set_level(LOG_WARN);
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    lostio_type_directory_use();
    //Typ fuer ein Laufwerk
    typehandle->id          = LOSTIO_TYPES_DRIVE;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = &ide_device_read_handler;
    typehandle->write       = &ide_drive_write_handler;
    typehandle->seek        = ide_drive_seek_handler;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);
    
    typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = LOSTIO_TYPES_PARTITION;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = &ide_partition_read_handler;
    typehandle->write       = &ide_partition_write_handler;
    typehandle->seek        = &ide_partition_seek_handler;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);
    ide_init();
    
    init_service_register("ide");
    
    while(TRUE) {
        //wait_for_rpc();
        yield();
    }
}


/**
 * Den IDE-Treiber initialisieren
 */
void ide_init()
{
    int i, j, device_count, device_count_pi;
    char* name = "/hd ";
    char* name_pi = "/cd ";
    device_count = 0;
    device_count_pi = 0;
    
    
    register_intr_handler(0x2E, &ide_irq14_handler);
    register_intr_handler(0x2F, &ide_irq15_handler);
    
    //Einzelne Kanaele abarbeiten
    for (i = 0; i < IDE_MAX_HWIF; i++) {
        //Controller bereit machen
        ide_hw_interface_init(i);
        ide_hw_interface_t* hwif = &ide_hw_interfaces[i];

        //Die Laufwerke durchgehen
        for (j = 0; j < IDE_HWIF_MAX_DRIVES; j++) {
            ide_hw_interface_drive_t* drive = &(hwif->drives[j]);

            //Wenn das Laufwerk nicht existiert wird es uebersprungen
            if (drive->type == none) {
                continue;
            }

            //Beim auswaehlen des Namens wird nur zwischen hd = xATA und
            // cd = xATAPI unterschieden.
            //
            // Von LostIO werden hier die data- und size-Eigenschaften der
            // Nodes Missbraucht. In data speichern wir einen Pointer auf
            // die Hardware-Interface-Struct. In size wird die
            // Laufwerksnummer gespeichert. Das koennen wir problemlos so
            // machen, da die beiden Eigenschaften eh nur von unseren
            // Handlern benutzt werden.
            if ((drive->type == pata) ||(drive->type == sata)) {
                name[3] = (char) ('1' + device_count++);

                vfstree_create_node(name, LOSTIO_TYPES_DRIVE, j, hwif, 0);
            } else {
                name_pi[3] = (char) ('1' + device_count_pi++);

                vfstree_create_node(name_pi, LOSTIO_TYPES_DRIVE, j, hwif, 0);
            }
                
            //Falls auf dem Laufwerk Partitionen extistieren, werden dafuer
            // jetzt auch Nodes erstellt.
            dword y = 0;

            //TODO: Die Partitionsnodes sollten eigentlich in den
            //Laufwerksnodes als Kindernodes drin sein.
            char* partition_name = "/ d  ";
            if ((drive->type == pata) ||(drive->type == sata)) {
                partition_name[1] = 'h';
                partition_name[3] = (char) ('0' + device_count);
            } else {
                partition_name[1] = 'c';
                partition_name[3] = (char) ('0' + device_count_pi);
            }
            while (list_get_element_at(drive->partitions, y) != NULL) {
                partition_name[4] = (char) ('a' + y);
                vfstree_create_node(partition_name, LOSTIO_TYPES_PARTITION,
                    j | (y << 16), hwif, 0);
                y++;
            }
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
    
    log_msg(LOG_INFO, "Initialisiere Kontroller 0x%x", hwif->portbase);

    //Am Anfang wird angenommen, dass garkeine Laufwerke vorhanden sind.
    hwif->drives[0].type = none;
    hwif->drives[1].type = none;
    
    
    byte sector_count, sector_number;

    // FIXME: Nachschlagen, ob das wirklich nicht notwendig ist.
    // ide_write_register(hwif, IDE_REG_DEVICE_CONTROL, 8);

    // Hier wird mit einem sehr einfachen Verfahren geprueft, ob der Controller
    // Geraete am Master-Port zulaesst. Denn wenn er dies nicht tut, dann kommt
    // es spaeter zu Timeouts und langen wartezeiten. Dieses Verfahren Basiert
    // auf der Annahme, dass der Controller die Werte, die wir in die Register
    // schreiben, nur dann zurueck schreibt, wenn er den Kanal auch benutzen
    // will. Das reicht aber _nicht_ um festzustellen, ob dort auch ein
    // Laufwerk ist. Die Werte, die auf die Ports geschrieben werden, koennen
    // beliebig gewaehlt werden.
    ide_select_device(hwif, 0);
    
    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, IDE_SOME_JUNK);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, IDE_SOME_OTHER_JUNK);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, IDE_SOME_OTHER_JUNK);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, IDE_SOME_JUNK);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, IDE_SOME_JUNK);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, IDE_SOME_OTHER_JUNK);
    
    sector_count = ide_read_register(hwif, IDE_REG_SECTOR_COUNT);
    sector_number = ide_read_register(hwif, IDE_REG_SECTOR_NUMBER);

    if ((sector_count == IDE_SOME_JUNK) && 
        (sector_number == IDE_SOME_OTHER_JUNK))
    {
        log_msg(LOG_INFO, "  Port 0 verfuegbar");
        hwif->drives[0].type = unknown;
    } else {
        log_msg(LOG_INFO, "  Port 0 nicht verfuegbar");
        hwif->drives[0].type = none;
    }


    // Hier kommt wieder die gleiche Prozedur wie oben, nur diesmal fuer den
    // Slave-Kanal.
    ide_select_device(hwif, 1);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, IDE_SOME_JUNK);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, IDE_SOME_OTHER_JUNK);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, IDE_SOME_OTHER_JUNK);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, IDE_SOME_JUNK);

    ide_write_register(hwif, IDE_REG_SECTOR_COUNT, IDE_SOME_JUNK);
    ide_write_register(hwif, IDE_REG_SECTOR_NUMBER, IDE_SOME_OTHER_JUNK);
    
    sector_count = ide_read_register(hwif, IDE_REG_SECTOR_COUNT);
    sector_number = ide_read_register(hwif, IDE_REG_SECTOR_NUMBER);

    if ((sector_count == IDE_SOME_JUNK) && 
        (sector_number == IDE_SOME_OTHER_JUNK))
    {
        log_msg(LOG_INFO, "  Port 1 verfuegbar");
        hwif->drives[1].type = unknown;
    } else {
        log_msg(LOG_INFO, "  Port 1 nicht verfuegbar");
        hwif->drives[1].type = none;
    }
    
    char* interfaces[] = {"keine", "unbekannt", "pata", "patapi", "sata",
        "satapi"};

    if (hwif->drives[0].type != none) {
        // Master-Laufwerk identifizieren. Wenn das identifizieren
        // fehlschlaegt, wird angenommen, dass das Laufwerk nicht vorhanden
        // ist.
        if (ide_identify_drive(hwif, 0) == FALSE)
        {
            log_msg(LOG_INFO, "  Geraet 0 konnte nicht identifiziert werden");
            hwif->drives[0].type = none;
        } else {
            ide_drive_init(hwif, 0);
            log_msg(LOG_INFO, "  Geraet 0 identifiziert; Schnittstelle: %s", 
                interfaces[hwif->drives[0].type]);
        }
    }
    
    if (hwif->drives[1].type != none) {
        // Slave-Laufwerk identifizieren. Wenn das identifizieren fehlschlaegt,
        // wird angenommen, dass das Laufwerk nicht vorhanden ist.
        if (ide_identify_drive(hwif, 1) == FALSE)
        {
            log_msg(LOG_WARN, "  Geraet 1 konnte nicht identifiziert werden");
            hwif->drives[1].type = none;
        } else {
            ide_drive_init(hwif, 1);
            log_msg(LOG_INFO, "  Geraet 1 identifiziert; Schnittstelle: %s",
                interfaces[hwif->drives[1].type]);
        }
    }
}


/**
 * 
 *
 * @param hwif      Pointer auf Interface-Struktur
 * @param drive_num Nummer des Laufwerks
 */
bool ide_drive_init(ide_hw_interface_t* hwif, byte drive_num)
{
    ide_hw_interface_drive_t* drive = &(hwif->drives[drive_num]);

    // Liste mit den Partitionen initialisieren, egal ob Partitionen existieren
    // oder nicht.
    drive->partitions = list_create();
                
    // Hier wird der MBR eingelesen um daraus die Partitionstabelle verwerten zu
    // koennen.
    byte first_sector[512];
    partition_table_entry_t* partition_table = (partition_table_entry_t*) 
        (first_sector + 0x1BE);
    if (ide_read_sectors(hwif, 0, 0, 1, first_sector) == FALSE)
    {
        log_msg(LOG_ERROR, "  Geraet %d konnte die Partitionstabelle nicht"
            "einlesen", drive_num);
        return FALSE;
    }
    
    // Nach Partitionen suchen

    dword i;
    for (i = 0; i < 4; i++) {
        // Wenn dieser Eintrag unbenutzt ist, wird er
        // uebersprungen.
        if (partition_table[i].type == 0) {
            continue;
        }
        
        //Hier wird eine Kopie des Eintrages erstellt, um ihn in die Liste
        // aufzunehmen.
        partition_table_entry_t* pte = malloc(sizeof(partition_table_entry_t));
        *pte = partition_table[i];
        
        drive->partitions = list_push(drive->partitions, pte);
    }
    
    log_msg(LOG_INFO, "  Geraet %d, Partitionstabelle erfolgreich eingelesen",
        drive_num);

    return TRUE;
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

    //Was hier im Moment verteilt wird, sind nur die reservierten standard-
    // Ports. Diese sind nur fuer die ersten 2 Kanaele festgelegt. Die anderen
    // muessten, falls vorhanen, per PCI ausfindig gemacht werden. Diese sind
    // zwar auch oft gleich, aber ein "auf gut Glueck!" duerfen wir hier nicht
    // riskieren, da sie theoretisch gesehen auch von etwas ganz anderem
    // benutzt werden koennten.
    switch (index) {
        case 0: portbase = 0x1F0; break;
        case 1: portbase = 0x170; break;
    }
    
    //Nun werden die Ports noch beim Kernel angefordert, damit der Treiber
    // sie benutzen darf, und nicht mit einem GPF abstirbt.
    if ((portbase != 0) && ((request_ports(portbase - 1, 10) == FALSE) 
        || (request_ports(portbase + 0x208, 1) == FALSE)))
    {
        log_msg(LOG_EMERG, "Ports %d-%d und %d konnten nicht angefordert"
            "werden!", portbase - 1, portbase +9, portbase + 0x208);
        exit(-1);
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
    log_msg(LOG_DEBUG, "Aktives Lauferk %d setzen fuer Kontroller 0x%x",
        device, hwif->portbase);
    // Aktuelles Laufwerk durch schreiben des DRIVE-Registers setzen.
    if (device == 0) {
        ide_write_register(hwif, IDE_REG_DEVICE_HEAD, IDE_DEVICE_HEAD_MASTER);
    } else if (device == 1) {
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
    byte result = inb(hwif->portbase + reg);
  #ifdef DEBUG_HARDCORE
    log_msg(LOG_DEBUG, "Lese Byte 0x%x aus dem Register %d des Kontrollers"
        " 0x%x", result, reg, hwif->portbase);
  #endif
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
    word result = inw(hwif->portbase + reg);
  #ifdef DEBUG_HARDCORE
    log_msg(LOG_DEBUG, "Lese Wort 0x%x aus dem Register %d des Kontrollers"
        " 0x%x", result, reg, hwif->portbase);
  #endif
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
  #ifdef DEBUG_HARDCORE
    log_msg(LOG_DEBUG, "Schreibe Byte 0x%x in das Register %d des Kontrollers"
        " 0x%x", value, reg, hwif->portbase);
  #endif
    outb(hwif->portbase + reg, value);
}


/**
 * Inhalt aus einem word-Register des Controllers auslesen
 *
 * @param hwif      Pointer auf Interface-Struktur
 * @param register  Register das beschrieben werden soll
 *
 * @return          Registerinhalt
 */
void ide_write_word_register(ide_hw_interface_t* hwif, word reg, word value)
{
  #ifdef DEBUG_HARDCORE
    log_msg(LOG_DEBUG, "Schreibe Wort 0x%x in das Register %d des Kontrollers"
        " 0x%x", value, reg, hwif->portbase);
  #endif
    outw(hwif->portbase + reg, value);
}


/**
 * Wartet bis das BSY-Flag im Statusregister nicht mehr gesetzt ist, ider ein
 * Timeout auftritt. Die Zeit dafuer kann mit dem letzen Parameter festgelegt
 * werden.
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

        if (!TEST_BIT(status, 7)) {
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
    log_msg(LOG_INFO, "Identifiziere Laufwerk %d an Kontroller 0x%x", drive,
        hwif->portbase);
    // Zuerst versuchen ausfindig zu machen, welches Interface das Laufwerk
    // benutzt. Dafür wird ein DEVICE DIAG gemacht. Das bringt das Laufwerk
    // dazu, die Signatur in die vorgeschriebenen Register zu schreiben. Die
    // Signatur ist aber noch kein Beweis, dass das Laufwerk tatsaechlich 
    // exisitert!
    ide_select_device(hwif, drive);
    ide_write_register(hwif, IDE_REG_COMMAND, IDE_CMD_DEVICE_DIAG);
    DELAY_400NS();
    
    // FIXME: Haesslicher FIX
    hwif->drives[drive].type = pata;
    
    // Sector-Count und -Number-Register auslesen. Diese enthalteb beide 1,
    // wenn die Signatur korrekt gesetzt wurde.
    byte sector_count = ide_read_register(hwif, IDE_REG_SECTOR_COUNT);
    byte sector_number = ide_read_register(hwif, IDE_REG_SECTOR_NUMBER);
    if ((sector_count == 1) && (sector_number == 1)) {
        //Die restlichen Register auslesen und deren Inhalt auswerten.
        byte status = ide_read_register(hwif, IDE_REG_PRIMARY_STATUS);
        byte lba_low = ide_read_register(hwif, IDE_REG_LBA_LOW);
        byte lba_mid = ide_read_register(hwif, IDE_REG_LBA_MID);

        if ((lba_low == 0x14) && (lba_mid == 0xeb)) {
            hwif->drives[drive].type = patapi;
        } else if ((lba_low == 0x69) && (lba_mid == 0x96)) {
            hwif->drives[drive].type = satapi;
        } else if ((status != 0) && (lba_low == 1) && (lba_mid == 0)) {
            hwif->drives[drive].type = pata;
        } else if ((status != 0) && (lba_low == 0x3c) && (lba_mid == 0xc3)) {
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
        ide_write_register(hwif, IDE_REG_COMMAND, IDE_CMD_IDENTIFY_DEVICE);
        DELAY_400NS();
    } else {
        ide_write_register(hwif, IDE_REG_COMMAND, 
            IDE_CMD_IDENTIFY_PACKET_DEVICE);
        DELAY_400NS();
    }
    
    qword timeout = get_tick_count() + 5000000;
    byte status = 0;
    // Solange das Data-Ready-Bit nicht gesetzt ist
    while (!TEST_BIT(status, 6)) {
        status = ide_read_register(hwif, IDE_REG_PRIMARY_STATUS);

        //Ein Fehler ist aufgetreten, entweder Fehler-Bit gesetzt, oder Busy
        // geloescht und Data-Ready auch gelöscht oder ein Timeout
        if ((TEST_BIT(status, 0)) || ((!TEST_BIT(status, 7)) && 
                (!TEST_BIT(status, 6))) || 
            (timeout < get_tick_count()))
        {
            log_msg(LOG_ERROR, "  Timeout beim warten auf die Fertistellung"
                " des IDENTIFY-Befehles fuer Laufwerk %d", drive);
            hwif->drives[drive].type = none;
            return FALSE;
        }
    }
    
    //Auf den IRQ warten, der signalisiert, dass die Daten abholbereit sind
    //ide_wait_irq(hwif);
    
    int i;
    ide_identify_result identify;
    word* wdest = (word*) &identify;

    //Die 256 Identifikations-Words einlesen
    for (i = 0; i < 256; i++) {
        wdest[i] = ide_read_word_register(hwif, IDE_REG_DATA);
    }
    

    /*if (!TEST_BIT(identify.general_config, 15)) {
        hwif->drives[drive].type = unknown;
    }*/
    
    hwif->drives[drive].removeable = TEST_BIT(identify.general_config, 7);
    
    //Laufwerk hat kein PI
    if ((hwif->drives[drive].type == pata) ||
        (hwif->drives[drive].type == sata) ||
        (hwif->drives[drive].type == unknown))
    {
        // Pruefen ob das Laufwerk DMA unterstuetzt
        hwif->drives[drive].support_dma = TEST_BIT(identify.capabilities, 8);

        // Pruefen ob LBA unterstuetzt wird
        hwif->drives[drive].support_lba = TEST_BIT(identify.capabilities, 9);
        //hwif->drives[drive].support_lba = FALSE;
        // Wenn das nicht der Fall ist, werden noch CHS translation Daten
        // eingelesen
        if (hwif->drives[drive].support_lba == FALSE) {
            hwif->drives[drive].chs_heads = identify.chs_default_heads;
            hwif->drives[drive].chs_cylinders = identify.chs_default_cylinders;
            hwif->drives[drive].chs_sectors = identify.chs_default_sectors;
        }
        

    } else {

    }

    
    return TRUE;
}


/**
 * IRQ-Handler fuer Primaeren IDE-Kanal
 */
void ide_irq14_handler()
{
    log_msg(LOG_DEBUG, "IRQ fuer Kontroller 0x%x", ide_hw_interfaces[0].
        portbase);
    ide_hw_interfaces[0].irq_count++;
    ide_hw_interfaces[0].irq_status_reg = ide_read_word_register(
        &ide_hw_interfaces[0], IDE_REG_PRIMARY_STATUS);
}


/**
 * IRQ-Handler fuer Sekundaeren IDE-Kanal
 */
void ide_irq15_handler()
{
    log_msg(LOG_DEBUG, "IRQ fuer Kontroller 0x%x", ide_hw_interfaces[1].
        portbase);
    ide_hw_interfaces[1].irq_count++;
    ide_hw_interfaces[1].irq_status_reg = ide_read_word_register(
        &ide_hw_interfaces[1], IDE_REG_PRIMARY_STATUS);
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


/**
 * 
 */
bool ide_read_sectors(ide_hw_interface_t* hwif, byte drive, qword lba,
    dword count, void* dest)
{
    //printf("ide_read_sectors(%d, %d);\n", (dword)lba, count);
    //Lesen von PATA-Laufwerk mit Hilfe von PIO
    //printf("%d, %d\n", hwif->drives[drive].type, hwif->drives[drive].support_lba);
    if ((hwif->drives[drive].type == pata))
    {
        return ide_read_sectors_pata_pio(hwif, drive, lba, count, dest);
    } else {
        log_msg(LOG_ERROR, "Nur PATA-Laufwerke werden unterstuetzt.");
        return FALSE;
    }
}


/**
 * 
 */
bool ide_read_sectors_pata_pio(ide_hw_interface_t* hwif, byte drive, qword lba,
    dword count, void* dest)
{
    log_msg(LOG_DEBUG, "Lese %d Sektoren ab %d", count, (dword) lba);
    // Da wir nicht mehr als 255 Sektoren auf einmal lesen koennen ist hier
    // eine Schleife.
    while (count != 0) {
        ide_clear_irq_count(hwif);

        ide_write_register(hwif, IDE_REG_ERROR, 0x0);
        
        byte current_count;
        #define MAX 1
        //Anzahl der Sektoren
        if (count > MAX) {
            current_count = MAX;
        } else {
            current_count = count;
        }
        
        ide_write_register(hwif, IDE_REG_SECTOR_COUNT, current_count);
        
        // Wenn LBA direkt addressiert wird, wird es benutzt, sonst muss halt
        // CHS her.
        if (hwif->drives[drive].support_lba == TRUE) {
            ide_write_register(hwif, IDE_REG_LBA_LOW, (dword)lba & 0xFF);
            ide_write_register(hwif, IDE_REG_LBA_MID, ((dword)lba >> 8) & 
                0xFF);
            ide_write_register(hwif, IDE_REG_LBA_HIGH, ((dword)lba >> 16) &
                0xFF);
            ide_write_register(hwif, IDE_REG_DEVICE_HEAD, 0xE0 | (drive << 4) |
                ((lba >> 24) & 0x0F));
        } else {
            // TODO: Ist die Adresse nicht zu gross?
            dword temp = (dword) lba % (hwif->drives[drive].chs_sectors *
                hwif->drives[drive].chs_heads);
            byte sector = (dword) lba % hwif->drives[drive].chs_sectors + 1;
            byte head = temp / hwif->drives[drive].chs_sectors;
            word cylinder = (dword) lba / (hwif->drives[drive].chs_sectors * 
                hwif->drives[drive].chs_heads);

            ide_write_register(hwif, IDE_REG_LBA_LOW, sector & 0xFF);
            ide_write_register(hwif, IDE_REG_LBA_MID, cylinder & 0xFF);
            ide_write_register(hwif, IDE_REG_LBA_HIGH, (cylinder >> 8) & 0xFF);
            ide_write_register(hwif, IDE_REG_DEVICE_HEAD, (drive << 4) | (head
                & 0xF) | 0xA0);
        }
        ide_write_register(hwif, IDE_REG_COMMAND, IDE_CMD_READ_SECTORS);
      
        int i, j;
        word* wdest = (word*) dest;
        // Nach jedem Sektor kommt ein IRQ
        for (j = 0; j < current_count; j++) {
            ide_wait_irq(hwif);
            ide_clear_irq_count(hwif);

            // Die Words vom Controller holen
            for (i = 0; i < IDE_SECTOR_SIZE / 2; i++) {
                *wdest = ide_read_word_register(hwif, IDE_REG_DATA);
                wdest++;
            }
            log_msg(LOG_DEBUG, "Sektor %d gelesen\n", j);
        }
        
        if (count > MAX) {
            dest = (void *) ((uintptr_t) dest + IDE_SECTOR_SIZE * MAX);
        }

        count -= current_count;
        lba += current_count;
    }
    log_msg(LOG_DEBUG, "Lesen erfolgreich beendet");
    return TRUE;
}


/**
 * Sektoren schreiben
 */
bool ide_write_sectors(ide_hw_interface_t* hwif, byte drive, qword lba,
    dword count, void* src)
{
    //printf("ide_write_sectors(%x, %x);\n", (dword)lba, count);
    if ((hwif->drives[drive].type == pata) && 
        (hwif->drives[drive].support_lba == TRUE))
    {
        return ide_write_sectors_pata_lba(hwif, drive, lba, count, src);
    } else {
        log_msg(LOG_ERROR, "Nur PATA-Laufwerke werden unterstuetzt.");
        return FALSE;
    }

}


/**
 * Sektoren auf ein ATA-Geraet per PIO schreiben
 */
bool ide_write_sectors_pata_lba(ide_hw_interface_t* hwif, byte drive,
    qword lba, dword count, void* src)
{
    // Da wir nicht mehr als 255 Sektoren auf einmal schreiben koennen ist hier
    // eine Schleife.
    while (count != 0) {
        ide_clear_irq_count(hwif);

        ide_write_register(hwif, IDE_REG_ERROR, 0x0);
        
        byte current_count;
        //Anzahl der Sektoren
        if (count > 0xFF) {
            current_count = 0xFF;
        } else {
            current_count = count;
        }

        ide_write_register(hwif, IDE_REG_SECTOR_COUNT, current_count);
        
        ide_write_register(hwif, IDE_REG_LBA_LOW, (dword)lba & 0xFF);
        ide_write_register(hwif, IDE_REG_LBA_MID, ((dword)lba >> 8) & 0xFF);
        ide_write_register(hwif, IDE_REG_LBA_HIGH, ((dword)lba >> 16) & 0xFF);
        ide_write_register(hwif, IDE_REG_DEVICE_HEAD, 0xE0 | (drive << 4) | 
            ((lba >> 24) & 0x0F));
        ide_write_register(hwif, IDE_REG_COMMAND, IDE_CMD_WRITE_SECTORS);
      
        int i, j;
        word* wsrc = (word*) src;
        // Nach jedem Sektor kommt ein IRQ
        for (j = 0; j < current_count; j++) {
            // Die Words vom Controller holen
            for (i = 0; i < IDE_SECTOR_SIZE / 2; i++) {
                ide_write_word_register(hwif, IDE_REG_DATA, *wsrc);
                wsrc++;
            }
            ide_wait_irq(hwif);
            ide_clear_irq_count(hwif);
        }
        
        if (count > 0xFF) {
            src = (void *) ((uintptr_t) src + IDE_SECTOR_SIZE * 0xFF);
        }

        count -= current_count;
        lba += current_count;
    }
    return TRUE;
}

