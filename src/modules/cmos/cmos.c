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
#include "time.h"
#include "init.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
//#define DEBUG_MSG(s) printf("[ CMOS ] debug: '%s'\n", s)
#define DEBUG_MSG(s) //

#define TYPE_DATE_TIME_FILE 255

static uint8_t read_hours(void);
static uint8_t read_minutes(void);
static uint8_t read_seconds(void);

static uint8_t read_day(void);
static uint8_t read_month(void);
static uint16_t read_year(void);
static uint8_t read_weekday(void);

static void refresh(void);


// Uhrzeit
uint8_t seconds;
uint8_t minutes;
uint8_t hours;

// Datum
uint8_t day;
uint8_t month;
uint16_t year;
uint8_t weekday;

// Zeit und Datum als Unix-Timestamp
time_t unix_timestamp;

// Information zu den 2 Diskettenlaufwerken
uint8_t floppy[2];

int main(int argc, char* argv[])
{
    request_ports(0x70, 2);

    lostio_init();

    lostio_type_ramfile_use_as(255);//Wir wollen mit Dateien arbeiten
                                    // Als neuen Typ, weil wir jedes mal ein
                                    // refresh machen wollen.
    get_typehandle(TYPE_DATE_TIME_FILE)->post_open = 
        (void(*)(lostio_filehandle_t*)) &refresh;
                                    
    lostio_type_directory_use();    // Und mit Verzeichnissen ;)
    lostio_type_ramfile_use();

    //Der Inhalt des Version-Ramfile
    const char* version = "Version 0.0.1";
    //Nun wird das Ramfile erstellt
    vfstree_create_node("/version", LOSTIO_TYPES_RAMFILE, strlen(version) + 1,
        (void*)version, 0);

    // Erstelle das Verzeichnis time
    vfstree_create_node("/time", LOSTIO_TYPES_DIRECTORY, 0, NULL, 
        LOSTIO_FLAG_BROWSABLE);
    //Die drei Nodes fuer die jeweiligen Zeiteinheiten
    vfstree_create_node("/time/hours", TYPE_DATE_TIME_FILE, 1, (void*)&hours,
        0);
    vfstree_create_node("/time/minutes", TYPE_DATE_TIME_FILE, 1, 
        (void*)&minutes, 0);
    vfstree_create_node("/time/seconds", TYPE_DATE_TIME_FILE, 1,
        (void*)&seconds, 0);

    // Erstelle das Verzeichnis date
    vfstree_create_node("/date", LOSTIO_TYPES_DIRECTORY, 0, NULL,
        LOSTIO_FLAG_BROWSABLE);
    vfstree_create_node("/date/day", TYPE_DATE_TIME_FILE, 1, (void*)&day, 0);
    vfstree_create_node("/date/month", TYPE_DATE_TIME_FILE, 1, (void*)&month,
        0);
    vfstree_create_node("/date/year", TYPE_DATE_TIME_FILE, 2, (void*)&year, 0);
    vfstree_create_node("/date/weekday", TYPE_DATE_TIME_FILE, 1, 
        (void*)&weekday, 0);
    vfstree_create_node("/unix_time", TYPE_DATE_TIME_FILE, sizeof(time_t), (void*)&unix_timestamp, 0);
   
    // Erstelle das Verzeichnis floppy
    vfstree_create_node("/floppy", LOSTIO_TYPES_DIRECTORY, 0, NULL,
        LOSTIO_FLAG_BROWSABLE);
    vfstree_create_node("/floppy/0", TYPE_DATE_TIME_FILE, 1,
        (void*) &floppy[0], 0);
    vfstree_create_node("/floppy/1", TYPE_DATE_TIME_FILE, 1,
        (void*) &floppy[1], 0);

    //Beim Init unter dem Namen "cmos" registrieren, damit die anderen
    //Prozesse den Treiber finden.
    init_service_register("cmos");
    while(true)
    {
        wait_for_rpc();
    }
}

// Port 0x70 = Index Register, write only (waehlt das Byte im CMOS Ram aus)
// Port 0x71 = Data Register, read/write
static uint8_t read_cmos(uint8_t address)
{
    //Wenn eines der Zeit/Datums Register ausgelesen werden soll, muss
    // sichergestellt werden, dass keine aktualisierung am laufen ist.
    if(address < 10)
    {
        outb(0x70, 0xa);
        while((inb(0x71) & (1 << 7)) != 0)
        {
            asm volatile("nop");
        }
    }

    outb(0x70, address);
    return inb(0x71);
}


static uint8_t convert(uint8_t num)
{
    //Ueberpruefen, ob BCD verwendet wird
    if((read_cmos(0xb) & (1 << 2)) == 0)
    {
        return (num & 0xf) + ((num >> 4) & 0xf) * 10;
    }
    else
    {
        return num;
    }
}

static uint8_t read_hours()
{
    return convert(read_cmos(0x4));
}

static uint8_t read_minutes()
{
    return convert(read_cmos(0x2));
}

static uint8_t read_seconds()
{
    return convert(read_cmos(0x0));
}

static uint8_t read_day()
{
    return convert(read_cmos(0x7));
}

static uint8_t read_month()
{
    return convert(read_cmos(0x8));
}

static uint16_t read_year()
{
    //Jahrhundert wird nicht ausgelesen, weil es nicht ueberall verfuegbar ist
    //century = convert(read_cmos(0x32));

    return 2000 + convert(read_cmos(0x9));
}

static uint8_t read_weekday()
{
    return convert(read_cmos(0x6));
}

static void refresh()
{
    // Zeit auslesen
    seconds = read_seconds();
    minutes = read_minutes();
    hours = read_hours();
    
    // Datum auslesen
    day = read_day();
    month = read_month();
    year = read_year();
    weekday = read_weekday();
    
    // UNIX-Timestamp aktualisieren
    unix_timestamp = ((((year - 1970) * 365 + day) * 24 + hours) * 60 + minutes
        ) * 60 + seconds;
    
    // Informationen zu den Diskettenlaufwerken auslesen
    uint8_t floppies = read_cmos(0x10);
    // Die Infos zum ersten Laufwerk sind in den oberen 4 Bits
    floppy[0] = floppies >> 4;
    // Die tum Zweiten sind in den unteren 4 Bits
    floppy[1] = floppies & 0xF;
}



