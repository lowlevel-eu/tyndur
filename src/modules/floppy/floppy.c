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

#include <stdint.h>
#include <stdbool.h>
#include <types.h>
#include <string.h>
#include <collections.h>
#include <stdint.h>
#include <lock.h>

#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "rpc.h"
#include "io.h"
#include "ports.h"
#include "sleep.h"
#include "lostio.h"
#include "init.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
//#define DEBUG_MSG(s) printf("[ FLOPPY ] debug: '%s'\n", s)
#define DEBUG_MSG(s) //

#define READ_COMPLETE_TRACK
//#undef READ_COMPLETE_TRACK


/// Digital Output Register
#define FLOPPY_REG_DOR 0x3F2

/// Main Status Register - readonly
#define FLOPPY_REG_MSR 0x3F4

/// Data Rate Select Register - writeonly
#define FLOPPY_REG_DSR 0x3F4

#define FLOPPY_DSR_1MBPS 0x03
#define FLOPPY_DSR_500KBPS 0x00
#define FLOPPY_DSR_300KBPS 0x01
#define FLOPPY_DSR_250KBPS 0x02

/// Datenregister (FIFO)
#define FLOPPY_REG_DATA 0x3F5

/// Wenn das RQM-Bit gesetzt ist, kˆnnen Daten ¸bertragen werden
#define FLOPPY_MSR_RQM 0x80

/// Legt die Art der naechsten erlaubten Operation fest: 0 = Schreiben, 1 = Lesen
#define FLOPPY_MSR_DIO 0x40

#define FLOPPY_CMD_SPECIFY 0x03
#define FLOPPY_CMD_INT_SENSE 0x08
#define FLOPPY_CMD_SEEK 0x0F
#define FLOPPY_CMD_CONFIGURE 0x13
#define FLOPPY_CMD_READ_TRACK 0x42
#define FLOPPY_CMD_READ 0x46

#define FLOPPY_MULTITRACK 0x80

#define FLOPPY_SECTOR_SIZE 2
#define FLOPPY_SECTORS_PER_TRACK 18

#ifdef READ_COMPLETE_TRACK
    #define DMA_READ_SIZE 18432
    #define DMA_READ_PATH "dma:/2/68/18432"
    #define DMA_WRITE_SIZE 512
    #define DMA_WRITE_PATH "dma:/2/72/512"
#else
    #define DMA_READ_SIZE 512
    #define DMA_READ_PATH "dma:/2/68/512"
    #define DMA_WRITE_SIZE 512
    #define DMA_WRITE_PATH "dma:/2/72/512"
#endif
#define FLOPPY_READ_IRQS 1
#define FLOPPY_WRITE_IRQS 1

// Anzahl der Sekunden die gewartet werden soll, bis der Motor abgeschaltet
// werden darf.
#define FLOPPY_MOTOR_DELAY 5

typedef struct
{
    bool    available;
    uint8_t    cylinder;
    uint8_t    status;     //Das st0 register dieses Laufwerks
    bool    motor;
    uint64_t    last_access;

#ifdef READ_COMPLETE_TRACK
    int current_track;
    char current_track_data[512 * FLOPPY_SECTORS_PER_TRACK * 2];
#endif    

} floppy_drive_t;


floppy_drive_t  floppy_drives[2];
uint8_t            floppy_cur_drive = 0;   //Aktuelles Laufwerk: Wird mit
                                        //floppy_select_drive geaendert.
uint8_t            floppy_irq_cnt = 0;     //Gibt an, wieviele IRQ seit dem letzten
                                        //floppy_clr_irqcnt aufgetreten sind

// Sorgt dafuer, dass der Motor nicht ausgeschaltet wird, waehrend auf das
// Diskettenlaufwerk zugegriffen wird.
lock_t          floppy_lock = 0;

void    floppy_init(void);
void    floppy_clr_irqcnt(void);
void    floppy_select_drive(uint8_t drive);
void    floppy_send_byte(uint8_t cmd);
uint8_t    floppy_read_byte(void);
void    floppy_set_motor(bool state);
void    floppy_irq_handler(uint8_t irq);
bool    floppy_wait_irq(int num);
void    floppy_int_sense(void);
void    floppy_reset(void);
bool    floppy_recalibrate(void);
bool    floppy_seek(uint8_t cylinder);
bool    floppy_read_sector(uint32_t lba, void *data);
bool    floppy_write_sector(uint32_t lba, void *data);

size_t  floppy_read_handler(lostio_filehandle_t* filehandle, void* buf,
            size_t blocksize, size_t blockcount);
size_t  floppy_write_handler(lostio_filehandle_t* filehandle,
            size_t blocksize, size_t blockcount, void* data);
int     floppy_seek_handler(lostio_filehandle_t* filehandle,
            uint64_t offset, int origin);

static void start_motor_shutdown_timeout(void);

int main(int argc, char* argv[])
{
    // Ports fuer den Zugriff auf den FDC reservieren
    if (!request_ports(FLOPPY_REG_DOR, 1) || !request_ports(FLOPPY_REG_MSR, 1)
        || !request_ports(FLOPPY_REG_DATA, 1)) 
    {
        puts("[ FLOPPY ] Fehler: Konnte die Ports nicht reservieren!");
        return -1;
    }

    // IRQ-Handler registrieren
    register_intr_handler(0x26, &floppy_irq_handler);
    
    //Den Dateisystem-Baum bereit machen
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    
    typehandle->id          = 255;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = &floppy_read_handler;
    typehandle->write       = &floppy_write_handler;
    typehandle->seek        = &floppy_seek_handler;
    typehandle->close       = NULL;
    typehandle->link        = NULL;
    typehandle->unlink      = NULL;
    lostio_register_typehandle(typehandle);
    
    // In diesem Verzeichnis liegen die Geraetedateien fuer die Einzelnen
    // Laufwerke.
    vfstree_create_node("/devices", LOSTIO_TYPES_DIRECTORY, 0, NULL, 
        LOSTIO_FLAG_BROWSABLE);
    
    // Infos zu den Diskettenlaufwerken vom CMOS-Ram holen.
    uint64_t timeout = get_tick_count() + 5000000;
    while (init_service_get("cmos") == 0) {
        yield();
        if (get_tick_count() > timeout) {
            puts("[ FLOPPY ] Fehler: Konnte cmos nicht finden!");
            return -1;
        }
    }
    
    uint8_t cmos_floppies[2] = {0, 0};
    FILE* cmos_floppy = fopen("cmos:/floppy/0", "r");
    fread(&cmos_floppies[0], 1, 1, cmos_floppy);
    fclose(cmos_floppy);

    cmos_floppy = fopen("cmos:/floppy/1", "r");
    fread(&cmos_floppies[1], 1, 1, cmos_floppy);
    fclose(cmos_floppy);

    // Wenn garkein Diskettenlaufwerk gefunden wurde, wird das Modul beendet,
    // da es dann nur sinnlos RAM verbraucht.
    if ((cmos_floppies[0] == 0) && ((cmos_floppies[1] == 0))) {
        puts("[ FLOPPY ] Warnung: Keine Diskettenlaufwerke gefunden. Der Treiber wird beendet.");
    }

    // Wenn das erste Diskettenlaufwerk vorhanden ist, wird es initialisiert
    if (cmos_floppies[0] != 0) {
        if (cmos_floppies[0] != 4) {
            puts("[ FLOPPY ] Warnung: Erstes Diskettenlaufwerk wird nicht unterstuetzt. (TODO)");
            floppy_drives[0].available = false;
        } else {
            floppy_drives[0].available = true;
            floppy_drives[0].motor = true;
            floppy_drives[0].last_access = 0;
            
#ifdef READ_COMPLETE_TRACK
            floppy_drives[0].current_track = -1;
#endif
            // Laufwerk initialisieren
            floppy_select_drive(0);
            floppy_init();

            vfstree_create_node("/devices/fd0", 255, 1440 * 1024, (void*)0, 0);
        }
    } else {
        floppy_drives[0].available = false;
    }
     
    if (cmos_floppies[1] != 0) {
        if (cmos_floppies[1] != 4) {
            puts("[ FLOPPY ] Warnung: Zweites Diskettenlaufwerk wird nicht unterstuetzt. (TODO)");
            floppy_drives[1].available = false;
        } else {
            floppy_drives[1].available = true;
            floppy_drives[1].motor = true;
            floppy_drives[1].last_access = 0;

#ifdef READ_COMPLETE_TRACK
            floppy_drives[1].current_track = -1;
#endif

            // Laufwerk initialisieren
            floppy_select_drive(1);
            floppy_init();

            vfstree_create_node("/devices/fd1", 255, 1440 * 1024, (void*)1, 0);
        }
    } else {
        floppy_drives[1].available = false;
    }

    DEBUG_MSG("ready");
    //Beim Init unter dem Namen "floppy" registrieren, damit die anderen
    //Prozesse den Treiber finden.
    init_service_register("floppy");
    
    start_motor_shutdown_timeout();
    while (true) {
        wait_for_rpc();
        // FIXME DMA-Handle nach Timeout schlieﬂen
    }
}


static FILE* dma_handle = NULL;
static bool dma_handle_read = true;

void motor_shutdown(void)
{
    if (locked(&floppy_lock)) {
        return;
    } else {
        lock(&floppy_lock);

        uint32_t now = get_tick_count();
        int i;
        for (i = 0; i < 2; i++) {
            if ((floppy_drives[i].last_access + FLOPPY_MOTOR_DELAY * 1000000 <
                now) &&  (floppy_drives[i].available == true) && 
                (floppy_drives[i].motor == true))
            {
                floppy_select_drive(i);
                floppy_set_motor(false);
                DEBUG_MSG("Motor wird ausgeschaltet");
            } else if (floppy_drives[i].available) {
                start_motor_shutdown_timeout();
            }
        }
        unlock(&floppy_lock);
    }
}

static void start_motor_shutdown_timeout()
{
    static uint32_t timer_id = 0;
    timer_cancel(timer_id);
    timer_id = timer_register(&motor_shutdown, FLOPPY_MOTOR_DELAY * 1000000);
}

static FILE* get_dma_read_handle(void)
{
    // FIXME else-Zweig fehlt, der eine neue ‹bertragung initialisiert
    dma_handle_read = false;

    if (dma_handle == NULL) {
        dma_handle = fopen(DMA_READ_PATH, "r");
    } else if (!dma_handle_read) {
        fclose(dma_handle);
        dma_handle = fopen(DMA_READ_PATH, "r");
    }

    return dma_handle;
}

static FILE* get_dma_write_handle(void)
{
    // FIXME else-Zweig fehlt, der eine neue ‹bertragung initialisiert
    dma_handle_read = true;

    if (dma_handle == NULL) {
        dma_handle = fopen(DMA_WRITE_PATH, "w");
    } else if (dma_handle_read) {
        fclose(dma_handle);
        dma_handle = fopen(DMA_WRITE_PATH, "w");
    }

    return dma_handle;
}

/**
 * Initialisiert den Floppycontroller
 */
void floppy_init()
{
    // F¸hre einen Reset durch
    floppy_reset();

    // Setze Datenrate und warte auf Antwortinterrupt
    floppy_clr_irqcnt();
    outb(FLOPPY_REG_DSR, FLOPPY_DSR_500KBPS);
    outb(FLOPPY_REG_DOR, 0x0C);
    floppy_wait_irq(1);

    // Sense
    // TODO F¸r jedes Laufwerk einmal
    floppy_int_sense();
    
    // Configure

    // Specify
    floppy_send_byte(FLOPPY_CMD_SPECIFY);
    floppy_send_byte(0xDF);
    floppy_send_byte(0x02);
    
    floppy_recalibrate();
}

/**
 * Aktuelles Diskettelaufwerk setzen
 *
 * @param drive Neues aktives Laufwerk
*/
void floppy_select_drive(uint8_t drive)
{
    floppy_cur_drive = drive;
    uint8_t tmp = inb(0x3F2);
    outb(0x3F2, (tmp & (~0x03)) | floppy_cur_drive);
}


/**
 * Motor des aktuellen Diskettenlaufterks ein- oder auschalten.
 *
 * @param state Neuer Status des Motors: Ein=true, Aus=false
 */
void floppy_set_motor(bool state)
{
    DEBUG_MSG("Motor status setzen");
    if(floppy_drives[floppy_cur_drive].motor == state)
    {
        return;
    }
    floppy_drives[floppy_cur_drive].last_access = get_tick_count();
    uint8_t status = inb(0x3F2);
    
    if (state == true)
    {
        DEBUG_MSG("ein");
        floppy_drives[floppy_cur_drive].motor = true;
        //Notor einschalten
        outb(0x3F2, (status & 0x0F) | (0x10 << floppy_cur_drive));
    }
    else
    {
        DEBUG_MSG("aus");
        floppy_drives[floppy_cur_drive].motor = false;
        //Motor ausschalten
        outb(0x3F2, status & ~(0x10 << floppy_cur_drive));
    }
    
    //Ein wenig warten
    msleep(500);
    
    floppy_drives[floppy_cur_drive].cylinder = 255;
}


/**
 * Ein Befehl- oder Argumentbyte an den Controller senden
 *
 * @param b Zu sendendes Byte
 */
void floppy_send_byte(uint8_t b)
{
    int i;
    //printf("<%2x, %2x", b, inb(FLOPPY_REG_MSR));
    for (i = 0; i < 5000; i++)
    {
        //Ueberpruefen, ob der Controller bereit ist.
        uint8_t msr = inb(FLOPPY_REG_MSR);
        if ((msr & (FLOPPY_MSR_DIO | FLOPPY_MSR_RQM)) == FLOPPY_MSR_RQM)
        {
            //Daten senden
            outb(FLOPPY_REG_DATA, b);
            //printf(">");
            
            // Ein bisschen Zeit totschlagen, damit es dem Laufwerk
            // nicht zu hektisch wird
            inb(FLOPPY_REG_MSR);

            return;
        }
        msleep(1);
    }
    printf("[ FLOPPY ] send failed, MSR = %02x\n", inb(FLOPPY_REG_MSR));
}


/**
 * Ein Befehl- oder Argumentbyte vom Controller lesen
 *
 * @return Eingelesenes Byte
 */
uint8_t floppy_read_byte()
{
    int i;
    for (i = 0; i < 5000; i++)
    {
        //if ((inb(0x3F4) & 0xD0) == 0xD0)
        uint8_t msr = inb(FLOPPY_REG_MSR);
        if ((msr & (FLOPPY_MSR_RQM | FLOPPY_MSR_DIO)) 
            == (FLOPPY_MSR_RQM | FLOPPY_MSR_DIO))
        {
            return inb(FLOPPY_REG_DATA);
        }

        msleep(1);
    }
    puts("[ FLOPPY ] floppy_read_byte failed");
    return 0;
}


/**
 * Wird aufgerufen wenn der IRQ6 auftritt
 */
void floppy_irq_handler(uint8_t irq)
{
    DEBUG_MSG("IRQ");
    p();
    floppy_irq_cnt++;
    v();
}


/**
 * Wartet auf einen IRQ6. Wenn der IRQ zu lange nicht eintritt
 * wird false zurueck gegeben, sonst true. Die Funktion wartet darauf,
 * dass floppy_irq_cnt != 0 wird. Ist die Variable schon von begin an 0
 * wird true zureck gegeben.
 *
 * @return Gibt an, ob ein IRQ angekommen ist
 */
bool floppy_wait_irq(int num)
{
    DEBUG_MSG("Warte auf IRQ");
    int i;
    for(i = 0; i < 5000; i++)
    {
        p();
        bool complete = (floppy_irq_cnt >= num);
        v();

        if (complete)
        {
            return true;
        }
        else
        {
            msleep(1);
        }
    }
    DEBUG_MSG("Timeout waehrend auf ein IRQ gewartet wurde");
    printf("Floppy: IRQ-Timeout, MSR=0x%02x\n", inb(FLOPPY_REG_MSR));
    floppy_init();
    return false;
}


/**
 * Wird mit floppy_wait_irq verwendet und setzt floppy_irq_cnt auf 0.
 */
void floppy_clr_irqcnt()
{
    floppy_irq_cnt = 0;
}


/**
 * Mir ist nicht bekannt, was diese Funktion genau macht. Sie schein aber
 * notwendig zu sein.
 */
void floppy_int_sense()
{
    floppy_send_byte(FLOPPY_CMD_INT_SENSE);
    floppy_drives[floppy_cur_drive].status = floppy_read_byte();
    floppy_drives[floppy_cur_drive].cylinder = floppy_read_byte();
}


/**
 * Kalibriert das Laufwerk
 *
 * @return true wenn das Neukalibrieren gelungen ist, sonst false
 */
bool floppy_recalibrate(void)
{
    DEBUG_MSG("Kalibriere Laufwerk neu");
    floppy_drives[floppy_cur_drive].cylinder = 255;
    floppy_set_motor(true);

    int i = 0;
    while ((i < 5) && (floppy_drives[floppy_cur_drive].cylinder != 0))
    {
        floppy_clr_irqcnt();
        floppy_send_byte(0x07);
        floppy_send_byte(floppy_cur_drive);
        floppy_wait_irq(1);
        floppy_int_sense();
        ++i;
    }
    return (i < 5);
}


/**
 * Fuehrt einen Laufwerksreset inklusive Neukalibrierung durch.
 */
void floppy_reset()
{
    DEBUG_MSG("Fuehre einen Laufwerksreset durch");
    // Reset
    outb(0x3F2, 0);
    floppy_set_motor(false);
    
/*
    msleep(20);
    
    floppy_clr_irqcnt();
    outb(0x3F4, 0);
    outb(0x3F2, 0x0C);
    
    floppy_wait_irq();
    floppy_int_sense();
    
    // Specify
    floppy_send_byte(0x03);
    floppy_send_byte(0xDF);
    floppy_send_byte(0x02);
    
    // Recalibrate
    floppy_recalibrate();
*/    
}


/**
 * Positioniert den Lesekopf ueber einen Bestimmten Zylinder.
 *
 * @param cylinder Zylindernummer
 * @return true wenn das Positionieren gelungen ist, sonst false
 */
bool floppy_seek(uint8_t cylinder)
{
    DEBUG_MSG("Kopf neu positionieren");
    if(floppy_drives[floppy_cur_drive].cylinder == cylinder)
    {
        DEBUG_MSG("    Steht bereits richtig");
        return true;
    }
    

    //floppy_set_motor(true);
    
    floppy_clr_irqcnt();

    floppy_send_byte(FLOPPY_CMD_SEEK);
    floppy_send_byte(floppy_cur_drive);
    floppy_send_byte(cylinder);

    if(floppy_wait_irq(1) == false)
    {
        return false;
    }

    floppy_int_sense();
    
    msleep(15);

    while (inb(FLOPPY_REG_MSR) & 0x0F) {
        DEBUG_MSG("Laufwerk ist noch beschaeftigt");
        msleep(1);
    }

    if (floppy_drives[floppy_cur_drive].cylinder != cylinder) {
        printf("Warnung: Floppy steht nach SEEK(%d) auf %d\n", 
            cylinder, floppy_drives[floppy_cur_drive].cylinder);
    }
    
    if(floppy_drives[floppy_cur_drive].status != 0x20)
    {   
        DEBUG_MSG("    false");
        return false;
    }
    else
    {
        DEBUG_MSG("    true");
        return true;
    }
}


/**
 * Einen Sektor vom aktuellen Laufwerk lesen.
 * 
 * @param lba Logische Adresse des zu lesenden Sektors.
 * @param data Pointer auf den Speicherbereich, wo die daten hikopiert werden
 *             sollen.
 * @return bool true wenn der Sektor erfolgreich gelesen wurde, sonst false.
 */
bool floppy_read_sector(uint32_t lba, void *data)
{
    DEBUG_MSG("Sektor einlesen");
    int i;
    FILE* io_res = NULL;

    //Die CHS-Werte berechnen
    uint8_t sector = (lba % 18) + 1;
    uint8_t cylinder = (lba / 18) / 2;
    uint8_t head = (lba / 18) % 2;

    // Wenn die Spur schon im Speicher ist, einfach die bereits gelesenen
    // Daten zur¸ckgeben
#ifdef READ_COMPLETE_TRACK    
    p();
    if (floppy_drives[floppy_cur_drive].current_track == cylinder) {
        memcpy(data, floppy_drives[floppy_cur_drive].current_track_data +
            (head * FLOPPY_SECTORS_PER_TRACK * 512) + ((sector - 1) * 512),
            512);
        #if 0
            {
                printf("** Lesen aus dem Buffer - floppy: 0x%x, CHS = %d/%d/%d **\n", lba, cylinder, head, sector);
                int i;
                for (i = 0; i < 512; i++) {
                    printf("%02hx ", (uint16_t) (((char*) data)[i]));
                }
                printf("\n");
            }
        #endif
        v();
        return true;
    }
    v();
#endif

    //oppy_drives[floppy_cur_drive].last_access = get_tick_count();
    floppy_drives[floppy_cur_drive].last_access = 0;
    floppy_set_motor(true);

    for (i = 0; i < 5; i++)
    {           
        //printf("Lese sector %d, cyl %d, head %d\n", sector, cylinder, head);

        if(floppy_seek(cylinder) != true)
        {
            continue;
        }
        
        floppy_clr_irqcnt();
        
        io_res = get_dma_read_handle(); //fopen("dma:/2/68/512", "r");

        uint8_t msr = inb(FLOPPY_REG_MSR);
        if (msr != FLOPPY_MSR_RQM) {
            printf("Warnung: MSR vor Senden ist %02x\n", msr);
        }

        // Befehl und Argumente senden
        // Byte 0: FLOPPY_CMD_READ
        // Byte 1: Bit 2:       Kopf (0 oder 1)
        //         Bits 0/1:    Laufwerk
        // Byte 2: Zylinder
        // Byte 3: Kopf
        // Byte 4: Sektornummer (bei Multisektortransfers des ersten Sektors)
        // Byte 5: Sektorgrˆﬂe
        // Byte 6: Letzte Sektornummer in der aktuellen Spur
        // Byte 7: Gap Length
        // Byte 8: DTL (ignoriert, wenn Sektorgrˆﬂe != 0. 0xFF empfohlen)
        
#ifdef READ_COMPLETE_TRACK        
        // Multitrack wird benoetigt, um eine komplette Spur einzulesen. Ohne
        // Multitrack wuerde nur von Kopf 0 korrekt gelesen. Mit Multitrack
        // wird der komplette Zylinder als eine einzige Spur behandelt, wobei
        // zunaechst alle Sektoren von Kopf 0 und anschliessend alle Sektoren
        // von Kopf 1 geliefert werden.        
        floppy_send_byte(FLOPPY_CMD_READ | FLOPPY_MULTITRACK);
        floppy_send_byte(0 | floppy_cur_drive);
        floppy_send_byte(cylinder);
        floppy_send_byte(0);
        floppy_send_byte(1);
#else        
        floppy_send_byte(FLOPPY_CMD_READ);
        floppy_send_byte((head << 2) | floppy_cur_drive);
        floppy_send_byte(cylinder);
        floppy_send_byte(head);
        floppy_send_byte(sector);
#endif        
        floppy_send_byte(FLOPPY_SECTOR_SIZE);
        floppy_send_byte(FLOPPY_SECTORS_PER_TRACK);        
        floppy_send_byte(27);
        floppy_send_byte(0xFF);
        
        //Falls der IRQ nicht kam (Timeout), wird ein weiterer Versuch gestertet
        if (floppy_wait_irq(FLOPPY_READ_IRQS) == false)
        {
            continue;
        }

        // Statusregister
        uint8_t st0, st1, st2;

        st0 = floppy_read_byte();
        st1 = floppy_read_byte(); // St1
        st2 = floppy_read_byte(); // St2
        floppy_read_byte(); // cylinder
        floppy_read_byte(); // head
        floppy_read_byte(); // sector
        floppy_read_byte(); // size
        
        if((st0 & 0xC0) == 0)
        {          
            //Die Daten von DMA-Treiber holen
#ifdef READ_COMPLETE_TRACK            
            // FIXME p/v waere hier wirklich schoen, aber fread blockiert dann
            //p();
            fread(floppy_drives[floppy_cur_drive].current_track_data,
                DMA_READ_SIZE, 1, io_res);
            floppy_drives[floppy_cur_drive].current_track = cylinder;
            memcpy(data, floppy_drives[floppy_cur_drive].current_track_data +
                (head * FLOPPY_SECTORS_PER_TRACK * 512) + ((sector -  1) *
                512), 512);
            //v();
#else
            fread(data, DMA_READ_SIZE, 1, io_res);
#endif

#if 0
            {
                printf("** Lesen von floppy: 0x%x, CHS = %d/%d/%d **\n", lba, cylinder, head, sector);
                int i;
                for (i = 0; i < 512; i++) {
                    printf("%02hx ", (uint16_t) (((char*) data)[i]));
                }
                printf("\n");
            }
#endif            

            //fclose(io_res);
            floppy_drives[floppy_cur_drive].last_access = get_tick_count();

            return true;
        } else {
            printf("floppy: Fehler beim Lesen von sector %d, cyl %d, head %d\n", 
                sector, cylinder, head);
            printf("floppy: READ-Status 0x%02x/0x%02x/0x%02x, MSR=0x%02x\n", 
                st0, st1, st2, inb(FLOPPY_REG_MSR));

            // Seek nochmal durchf¸hren
            floppy_drives[floppy_cur_drive].cylinder = 255;
            
            // Nach drei fehlgeschlagenen Versuchen einen Reset durchf¸hren
            if (i == 3) {
                printf("floppy: Reset.\n");
                floppy_init();
            }
        }
    }
    floppy_drives[floppy_cur_drive].last_access = get_tick_count();

    return false;
}


/**
 * Einen einzelnen Sektor auf die Diskette schreiben.
 *
 * @param lba Logische Adresse des Sektors
 * @param data Pointer auf den Speicherbereich, wo die Quelldaten fuer den 
 *             Sektor liegen.
 * @return true wenn der Schreibvorgang erfolgreich abgeschlossen wurde, sonst
 *         false.
 */
bool floppy_write_sector(uint32_t lba, void *data)
{
    DEBUG_MSG("Sektor schreiben");
    int i;

    FILE* io_res;
    floppy_drives[floppy_cur_drive].last_access = 0;
    floppy_set_motor(true);
    for (i = 0; i < 3; i++)
    {        
        // Calculate the CHS values
        uint32_t sector = (lba % 18) + 1;
        uint32_t cylinder = (lba / 18) / 2;
        uint32_t head = (lba / 18) % 2;
        
        if (floppy_seek(cylinder) != true)
        {
            continue;
        }

        //Daten in den DMA-Buffer schreiben
        io_res = get_dma_write_handle(); //fopen("dma:/2/72/512", "w");
        fwrite(data, 512, 1, io_res);
        
        floppy_clr_irqcnt();

        //
        floppy_send_byte(0x45);
        floppy_send_byte((head << 2) | floppy_cur_drive);
        floppy_send_byte(cylinder);
        floppy_send_byte(head);
        floppy_send_byte(sector);
        floppy_send_byte(2);
        floppy_send_byte(18);
        floppy_send_byte(27);
        floppy_send_byte(0);
        
        // Wait for interrupt
        floppy_wait_irq(FLOPPY_WRITE_IRQS);
        uint8_t status = floppy_read_byte();
        floppy_read_byte(); // St1
        floppy_read_byte(); // St2
        floppy_read_byte(); // cylinder
        floppy_read_byte(); // head
        floppy_read_byte(); // sector
        floppy_read_byte(); // size
        
        if ((status & 0xC0) == 0)
        {
            //fclose(io_res);
            floppy_drives[floppy_cur_drive].last_access = get_tick_count();

            // Wenn die Aktuelle Spur gecached ist, muss der Cache auch
            // aktualisiert werden
#ifdef READ_COMPLETE_TRACK
            if (floppy_drives[floppy_cur_drive].current_track == cylinder) {
                memcpy(floppy_drives[floppy_cur_drive].current_track_data +
                    (head * FLOPPY_SECTORS_PER_TRACK * 512) + ((sector - 1) *
                    512), data, 512);
            }
#endif
            return true;
        }
    }
    floppy_drives[floppy_cur_drive].last_access = get_tick_count();
    return false;
}


size_t floppy_read_handler(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount)
{
    // Nur lock reicht hier nicht, da sonst das ganze blockiert wird
    if (locked(&floppy_lock)) {
        return 0;
    } else {
        lock(&floppy_lock);
    }
    
    size_t size = blocksize * blockcount;

    // Nur ganze Sektoren lesen
    if (size % 512) {
        printf("Floppy: size = %d ist nicht aligned!\n", size);
        return 0;
    }
    
    if(size + filehandle->pos > filehandle->node->size)
    {
        size = filehandle->node->size - filehandle->pos;
    }
    
    //size_t read_end = size + filehandle->pos;

    //Startsektor berechnen
    uint32_t sector_start = filehandle->pos / 512;
    /*if(filehandle->pos % 512 != 0)
    {
        sector_start++;
    }*/
    
    //Sektorenanzahl berechnen
    uint32_t sector_count = size / 512; //(read_end / 512) - (filehandle->pos / 512) + 1;
    /*if()
    {
        sector_count++;
    }*/
    //printf("RPC: floppy read %d sectors\n", sector_count);

    // Das Laufwerk aktivieren
    floppy_select_drive((uintptr_t) filehandle->node->data);

    int i;
    for(i = 0; i < sector_count; i++)
    {
        if(floppy_read_sector(sector_start + i, buf + (512 * i)) == false) {
            return 0;
        }
    }

    filehandle->pos += size;

    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }

    start_motor_shutdown_timeout();
    unlock(&floppy_lock);
    return size;
}


size_t floppy_write_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data)
{
    // Nur lock reicht hier nicht, da sonst das ganze blockiert wird
    // FIXME: Das kann auch so noch passieren, die Warscheinlichkeit ist aber
    // viel kleiner.
    if (locked(&floppy_lock)) {
        return 0;
    } else {
        lock(&floppy_lock);
    }
    size_t size = blocksize * blockcount;

    if(size + filehandle->pos > filehandle->node->size)
    {
        size = filehandle->node->size - filehandle->pos;
    }
    
    //Wenn die Groesse 0 ist, dann ist hier Endstation
    if(size == 0)
    {
        return 0;
    }

    //Startsektor berechnen
    uint32_t sector_start = filehandle->pos / 512;
    
    
    //Sektorenanzahl berechnen
    uint32_t sector_count = size / 512;
    if(size % 512 != 0)
    {
        sector_count++;
    }
    
    // Das Laufwerk aktivieren
    floppy_select_drive((uintptr_t) filehandle->node->data);

       
    //TODO: Der erste und der letzte Block muessen einzeln geschrieben werden,
    //weil er eventuell nicht genau auf einer Sektorgrenze liegt. Falls dem so
    //ist, sollte man ihn zuerst eingelesen, die n√∂tigen daten √ºberschrieben,
    //und dann so auf die Diskette schreiben.
 

    int i;
    //floppy_set_motor(true);
    for(i = 0; i < sector_count; i++)
    {
        floppy_write_sector(sector_start + i, (void*) ((uint32_t)data + i * 512));
    }
    //floppy_set_motor(false);
    filehandle->pos += size;

    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    
    start_motor_shutdown_timeout();
    unlock(&floppy_lock);
    return size;
}


int floppy_seek_handler(lostio_filehandle_t* filehandle, uint64_t offset,
    int origin)
{
    switch(origin)
    {
        case SEEK_SET:
        {
            if(offset > filehandle->node->size)
            {
                return -1;
            }

            filehandle->pos = offset;
            break;
        }

        case SEEK_CUR:
        {
            if((offset + filehandle->pos) > filehandle->node->size)
            {
                return -1;
            }
            filehandle->pos += offset;
            break;
        }

        case SEEK_END:
        {
            filehandle->pos = filehandle->node->size;
            break;
        }
    }

    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    else
    {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }


    return 0;
}


