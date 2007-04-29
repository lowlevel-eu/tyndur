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
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "ports.h"
#include "sleep.h"
#include "collections.h"
#include "lostio.h"
#define _USE_START_
#include "init.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
//#define DEBUG_MSG(s) printf("[ FLOPPY ] debug: '%s'\n", s)
#define DEBUG_MSG(s) //

typedef struct
{
    byte    cylinder;
    byte    status;     //Das st0 register dieses Laufwerks
    bool    motor;
    qword   last_access;
} floppy_drive_t;


floppy_drive_t  floppy_drives[2];
byte            floppy_cur_drive = 0;   //Aktuelles Laufwerk: Wird mit
                                        //floppy_select_drive geaendert.
byte            floppy_irq_cnt = 0;     //Gibt an, wieviele IRQ seit dem letzten
                                        //floppy_clr_irqcnt aufgetreten sind



void    floppy_select_drive(byte drive);
void    floppy_send_byte(byte cmd);
byte    floppy_read_byte();
void    floppy_set_motor(bool state);
void    floppy_irq_handler();
bool    floppy_wait_irq();
void    floppy_int_sense();
void    floppy_reset();
bool    floppy_recalibrate();
bool    floppy_seek(byte cylinder);
bool    floppy_read_sector(dword lba, void *data);
bool    floppy_write_sector(dword lba, void *data);

read_hdl_reply  floppy_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount);
size_t          floppy_write_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data);
int             floppy_seek_handler(lostio_filehandle_t* filehandle , int offset, int origin);

int main(int argc, char* argv[])
{    
    //Diskettenlaufwerk initialisieren
    //TODO: Das 2. Diskettelaufwerk auch initialiseren falls vorhanden
    request_ports(0x3f0, 8);
    register_intr_handler(0x26, &floppy_irq_handler);

    
    floppy_select_drive(0);
    floppy_drives[0].motor = FALSE;
    floppy_reset();
    
    
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
    lostio_register_typehandle(typehandle);

    
    const char* version = "Version 0.0.1";
    vfstree_create_node("/version", LOSTIO_TYPES_RAMFILE, strlen(version) + 1, (void*)version, 0);
    vfstree_create_node("/devices", LOSTIO_TYPES_DIRECTORY, 0, NULL, LOSTIO_FLAG_BROWSABLE);
    vfstree_create_node("/devices/fd0", 255, 1440 * 1024, (void*)0, 0);
    //vfstree_create_node("/devices/fd1", 255, 1440 * 1024, (void*)1, 0);
    
    DEBUG_MSG("ready");
    //Beim Init unter dem Namen "floppy" registrieren, damit die anderen
    //Prozesse den Treiber finden.
    init_service_register("floppy");
    int i = 0;
    while(TRUE)
    {
        yield();
        if(i++ > 500)
        {
            i = 0;
            if(floppy_drives[0].last_access + 2000000 < get_tick_count())
            {
                floppy_set_motor(FALSE);
            }
        }
    }
}




/**
 * Aktuelles Diskettelaufwerk setzen
 *
 * @param drive Neues aktives Laufwerk
*/
void floppy_select_drive(byte drive)
{
    floppy_cur_drive = drive;
    byte tmp = inb(0x3F2);
    outb(0x3F2, (tmp & (~0x03)) | floppy_cur_drive);
}


/**
 * Motor des aktuellen Diskettenlaufterks ein- oder auschalten.
 *
 * @param state Neuer Status des Motors: Ein=TRUE, Aus=FALSE
 */
void floppy_set_motor(bool state)
{
    DEBUG_MSG("Motor status setzen");
    if(floppy_drives[floppy_cur_drive].motor == state)
    {
        return;
    }
    floppy_drives[floppy_cur_drive].last_access = get_tick_count();
    byte status = inb(0x3F2);
    
    if (state == TRUE)
    {
        DEBUG_MSG("ein");
        floppy_drives[floppy_cur_drive].motor = TRUE;
        //Notor einschalten
        outb(0x3F2, (status & 0x0F) | (0x10 << floppy_cur_drive));
    }
    else
    {
        DEBUG_MSG("aus");
        floppy_drives[floppy_cur_drive].motor = FALSE;
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
void floppy_send_byte(byte b)
{
    int i;
    for (i = 0; i < 5000; i++)
    {
        //Ueberpruefen, ob der Controller bereit ist.
        if ((inb(0x3F4) & 0xC0) == 0x80)
        {
            //Daten senden
            outb(0x3F5, b);
            return;
        }
        msleep(1);
    }
    puts("[ FLOPPY ] send failed");
}


/**
 * Ein Befehl- oder Argumentbyte vom Controller lesen
 *
 * @return Eingelesenes Byte
 */
byte floppy_read_byte()
{
    int i;
    for (i = 0; i < 5000; i++)
    {
        if ((inb(0x3F4) & 0xD0) == 0xD0)
        {
            return inb(0x3F5);
        }

        msleep(1);
    }
    puts("[ FLOPPY ] floppy_read_byte failed");
    return 0;
}


/**
 * Wird aufgerufen wenn der IRQ6 auftritt
 */
void floppy_irq_handler()
{
    DEBUG_MSG("IRQ");
    floppy_irq_cnt++;
}


/**
 * Wartet auf einen IRQ6. Wenn der IRQ zu lange nicht eintritt
 * wird FALSE zurueck gegeben, sonst TRUE. Die Funktion wartet darauf,
 * dass floppy_irq_cnt != 0 wird. Ist die Variable schon von begin an 0
 * wird TRUE zureck gegeben.
 *
 * @return Gibt an, ob ein IRQ angekommen ist
 */
bool floppy_wait_irq()
{
    DEBUG_MSG("Warte auf IRQ");
    int i;
    for(i = 0; i < 1000; i++)
    {
        if(floppy_irq_cnt != 0)
        {
            return TRUE;
        }
        else
        {
            msleep(1);
        }
    }
    DEBUG_MSG("timeout waehrend auf ein IRQ gewartet wurde");
    return FALSE;
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
    floppy_send_byte(0x08);
    floppy_drives[floppy_cur_drive].status = floppy_read_byte();
    floppy_drives[floppy_cur_drive].cylinder = floppy_read_byte();
}


/**
 * Kalibriert das Laufwerk
 *
 * @return TRUE wenn das Neukalibrieren gelungen ist, sonst FALSE
 */
bool floppy_recalibrate()
{
    DEBUG_MSG("Kalibriere Laufwerk neu");
    floppy_drives[floppy_cur_drive].cylinder = 255;
    floppy_set_motor(TRUE);

    int i = 0;
    while ((i < 5) && (floppy_drives[floppy_cur_drive].cylinder != 0))
    {
        floppy_clr_irqcnt();
        floppy_send_byte(0x07);
        floppy_send_byte(floppy_cur_drive);
        floppy_wait_irq();
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
    floppy_recalibrate(0);
}


/**
 * Positioniert den Lesekopf ueber einen Bestimmten Zylinder.
 *
 * @param cylinder Zylindernummer
 * @return TRUE wenn das Positionieren gelungen ist, sonst FALSE
 */
bool floppy_seek(byte cylinder)
{
    DEBUG_MSG("Kopf neu positionieren");
    if(floppy_drives[floppy_cur_drive].cylinder == cylinder)
    {
        DEBUG_MSG("    Steht bereits richtig");
        return TRUE;
    }
    

    //floppy_set_motor(TRUE);
    
    floppy_clr_irqcnt();

    floppy_send_byte(0x0F);
    floppy_send_byte(floppy_cur_drive);
    floppy_send_byte(cylinder);

    if(floppy_wait_irq() == FALSE)
    {
        return FALSE;
    }

    floppy_int_sense();
    
    msleep(15);
    
    if(floppy_drives[floppy_cur_drive].status != 0x20)
    {   
        DEBUG_MSG("    FALSE");
        return FALSE;
    }
    else
    {
        DEBUG_MSG("    TRUE");
        return TRUE;
    }
}


/**
 * Einen Sektor vom aktuellen Laufwerk lesen.
 * 
 * @param lba Logische Adresse des zu lesenden Sektors.
 * @param data Pointer auf den Speicherbereich, wo die daten hikopiert werden
 *             sollen.
 * @return bool TRUE wenn der Sektor erfolgreich gelesen wurde, sonst FALSE.
 */
bool floppy_read_sector(dword lba, void *data)
{
    DEBUG_MSG("Sektor einlesen");
    int i;
    FILE* io_res = NULL;

    floppy_drives[floppy_cur_drive].last_access = get_tick_count();
    floppy_set_motor(TRUE);

    for (i = 0; i < 3; i++)
    {           
        //Die CHS-Werte berechnen
        byte sector = (lba % 18) + 1;
        byte cylinder = (lba / 18) / 2;
        byte head = (lba / 18) % 2;

        //printf("Lese sector %d, cyl %d, head %d\n", sector, cylinder, head);
        printf(".");
        
        if(floppy_seek(cylinder) != TRUE)
        {
            continue;
        }
        
        floppy_clr_irqcnt();
        
        io_res = fopen("dma:/2/68/512", "r");

        //Befehl und Argumente Sendeb
        floppy_send_byte(0x46);
        floppy_send_byte((head << 2) | floppy_cur_drive);
        floppy_send_byte(cylinder);
        floppy_send_byte(head);
        floppy_send_byte(sector);
        floppy_send_byte(2);
        floppy_send_byte(18);
        floppy_send_byte(27);
        floppy_send_byte(0);
        
        //Falls der IRQ nich kam(timeout), wird ein weiterer Versuch gestertet
        if (floppy_wait_irq() == FALSE)
        {
            continue;
        }

        byte status = floppy_read_byte();
        floppy_read_byte(); // St1
        floppy_read_byte(); // St2
        floppy_read_byte(); // cylinder
        floppy_read_byte(); // head
        floppy_read_byte(); // sector
        floppy_read_byte(); // size
        
        if((status & 0xC0) == 0)
        {          
            //Die Daten von DMA-Treiber holen
            fread(data, 512, 1, io_res);
            fclose(io_res);
            return TRUE;
        }
    }
    
    return FALSE;
}


/**
 * Einen einzelnen Sektor auf die Diskette schreiben.
 *
 * @param lba Logische Adresse des Sektors
 * @param data Pointer auf den Speicherbereich, wo die Quelldaten fuer den 
 *             Sektor liegen.
 * @return TRUE wenn der Schreibvorgang erfolgreich abgeschlossen wurde, sonst
 *         FALSE.
 */
bool floppy_write_sector(dword lba, void *data)
{
    DEBUG_MSG("Sektor schreiben");
    int i;

    FILE* io_res;
    floppy_set_motor(TRUE);
    for (i = 0; i < 3; i++)
    {        
        // Calculate the CHS values
        dword sector = (lba % 18) + 1;
        dword cylinder = (lba / 18) / 2;
        dword head = (lba / 18) % 2;
        
        if (floppy_seek(cylinder) != TRUE)
        {
            continue;
        }

        //Daten in den DMA-Buffer schreiben
        io_res = fopen("dma:/2/72/512", "w");
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
        floppy_wait_irq();
        byte status = floppy_read_byte();
        floppy_read_byte(); // St1
        floppy_read_byte(); // St2
        floppy_read_byte(); // cylinder
        floppy_read_byte(); // head
        floppy_read_byte(); // sector
        floppy_read_byte(); // size
        
        if ((status & 0xC0) == 0)
        {
            //fclose(io_res);
            return TRUE;
        }
    }
    return FALSE;
}


read_hdl_reply  floppy_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount)
{
    read_hdl_reply reply;
    size_t size = blocksize * blockcount;
    
    if(size + filehandle->pos > filehandle->node->size)
    {
        size = filehandle->node->size - filehandle->pos;
    }
    
    size_t read_end = size + filehandle->pos;

    //Startsektor berechnen
    dword sector_start = filehandle->pos / 512;
    /*if(filehandle->pos % 512 != 0)
    {
        sector_start++;
    }*/
    
    //Sektorenanzahl berechnen
    dword sector_count = (read_end / 512) - (filehandle->pos / 512) + 1;
    /*if()
    {
        sector_count++;
    }*/
    //printf("RPC: floppy read %d sectors\n", sector_count);
    
    //Buffer allokieren
    static byte* buffer = NULL;
    buffer = realloc(buffer, sector_count * 512);
    
    
    int i;
    reply.data = (void*) (buffer + (filehandle->pos % 512));
    reply.size = size;

    for(i = 0; i < sector_count; i++)
    {
        if(floppy_read_sector(sector_start + i, (void*)((dword)buffer + 512 * i)) == FALSE)
        {
            //TODO: Gelesene daten Mitsenden
            reply.data = NULL;
            reply.size = 0;
        }
    }
        
    filehandle->pos += size;

    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }

    return reply;
}


size_t floppy_write_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data)
{
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
    dword sector_start = filehandle->pos / 512;
    
    
    //Sektorenanzahl berechnen
    dword sector_count = size / 512;
    if(size % 512 != 0)
    {
        sector_count++;
    }
    
    
       
    //TODO: Der erste und der letzte Block muessen einzeln geschrieben werden,
    //weil er eventuell nicht genau auf einer Sektorgrenze liegt. Falls dem so
    //ist, sollte man ihn zuerst eingelesen, die nötigen daten überschrieben,
    //und dann so auf die Diskette schreiben.
 

    int i;
    //floppy_set_motor(TRUE);
    for(i = 0; i < sector_count; i++)
    {
        floppy_write_sector(sector_start + i, (void*) ((dword)data + i * 512));
    }
    //floppy_set_motor(FALSE);
    filehandle->pos += size;

    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }    
    
    return size;
}


int floppy_seek_handler(lostio_filehandle_t* filehandle , int offset, int origin)
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


