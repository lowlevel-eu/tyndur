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
//#define DEBUG_MSG(s) printf("[ CMOS ] debug: '%s'\n", s)
#define DEBUG_MSG(s) //

static byte read_hours(void);
static byte read_minutes(void);
static byte read_seconds(void);

static byte read_day(void);
static byte read_month(void);
static byte read_year(void);
static byte read_weekday(void);


int main(int argc, char* argv[])
{
	//Beim Init unter dem Namen "cmos" registrieren, damit die anderen
	//Prozesse den Treiber finden.
	init_service_register("cmos");

	lostio_init();

	lostio_type_ramfile_use();	// Wir wollen mit Dateien arbeiten
	lostio_type_directory_use();	// Und mit Verzeichnissen ;)

	//Speicher fuer ein neues typehandle allokieren
	typehandle_t* typehandle = malloc(sizeof(typehandle_t));

	//Die Typen-ID
	typehandle->id          = 255;

	//Die Pointer auf unsere Handles setzen. NULL wenn für
	//diejenigen Pointer wofür kein Handler zur Verfügung steht.
	typehandle->pre_open    = NULL;
	typehandle->post_open   = NULL;
	typehandle->read        = NULL;
	typehandle->write       = NULL;
	typehandle->seek        = NULL;
	typehandle->close       = NULL;
	
	//Das Typehandle bei LostIO registrieren, damit es verfügbar ist.
	lostio_register_typehandle(typehandle);

	// Port 0x70 = Index Register, write only (waehlt das Byte im CMOS Ram aus)
	// Port 0x71 = Data Register, read/write

	byte seconds = read_seconds();
	byte minutes = read_minutes();
	byte hours = read_hours();

	byte day = read_day();
	byte month = read_month();
	byte year = read_year();
	byte weekday = read_weekday();

	//Der Inhalt des Version-Ramfile
	const char* version = "Version 0.0.1";
	//Nun wird das Ramfile erstellt
	vfstree_create_node("/version", LOSTIO_TYPES_RAMFILE, strlen(version) + 1, (void*)version);

	// Erstelle das Verzeichnis time
	vfstree_create_node("/time", LOSTIO_TYPES_DIRECTORY, 0, NULL);
	//Die drei Nodes fuer die jeweiligen Zeiteinheiten
	vfstree_create_node("/time/hours", 255, 1, (void*)&hours);
	vfstree_create_node("/time/minutes", 255, 1, (void*)&minutes);
	vfstree_create_node("/time/seconds", 255, 1, (void*)&seconds);

	// Erstelle das Verzeichnis date
	vfstree_create_node("/date", LOSTIO_TYPES_DIRECTORY, 0, NULL);
	vfstree_create_node("/date/day", 255, 1, (void*)&day);
	vfstree_create_node("/date/minutes", 255, 1, (void*)&month);
	vfstree_create_node("/date/seconds", 255, 1, (void*)&year);
	vfstree_create_node("/date/weekday", 255, 1, (void*)&weekday);

    while(TRUE) yield();
}

static byte read_hours() {
	outb(0x70, 0x04);	// Byte 4 im CMOS Ram auslesen (Stunden)
	return inb(0x71);
}

static byte read_minutes() {
	outb(0x70, 0x02);	// Byte 2 im CMOS Ram auslesen (Minuten)
	return inb(0x71);
}

static byte read_seconds() {
	outb(0x70, 0x00);	// Byte 0 im CMOS Ram auslesen (Sekunden)
	return inb(0x71);
}

static byte read_day() {
	outb(0x70, 0x07);	// Byte 7 im CMOS Ram auslesen (Tag)
	return inb(0x71);
}

static byte read_month() {
	outb(0x70, 0x08);	// Byte 8 im CMOS Ram auslesen (Monat)
	return inb(0x71);
}

static byte read_year() {
	outb(0x70, 0x09);	// Byte 9 im CMOS Ram auslesen (Jahr)
	return inb(0x71);
}

static byte read_weekday() {
	outb(0x70, 0x06);	// Byte 6 im CMOS Ram auslesen (Wochentag: 1 - Montag ... 7 - Sonntag)
	return inb(0x71);
}
