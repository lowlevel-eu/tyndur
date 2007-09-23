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
#include <stdio.h>
#include <rpc.h>
#include "lostio.h"
#include "vterm.h"
#include "sleep.h"

#define _NO_STDIO_
#define _USE_START_
#include "init.h"


/// Oeffnet Eingabe- und Ausgabedatei
static bool init_io_files();
/// Hier kommen die Eingaben fuer all die virtuellen Terminals her
FILE* input = NULL;
/// Und hier gehen die Ausgaben hin
FILE* output = NULL;

/// Eingaben einlesen
static void get_input();

int main(int argc, char* argv[])
{
    // Filehandles fuer Ein- und Ausgabe oeffnen
    if (init_io_files() == FALSE) {
        // Falls das nicht klappt, ist alles sinnlos. ;-)
        return -1;
    }

    // Schnittstelle fuer die anderen Prozesse einrichten
    init_lostio_interface();
    
    init_vterminals(10);
    
    init_service_register("vterm");
    
    // Auf Anfragen warten
    timer_register(&get_input, 200000);
    while(TRUE) {
        wait_for_rpc();
        lostio_dispatch();
	}
}

/**
 * File-Handle fuer die Eingaben und Ausgaben aller Terminals oeffnen
 *
 * @return TRUE bei Erfolg, FALSE im Fehlerfall
 */
bool init_io_files()
{
    int i;
    
    // FIXME: Diese Pfade sollten aus einer Konfigurationsdatei oder aus den
    // Parametern kommen!
    
    // Falls das öffnen nicht beim ersten Anlauf klappt, wird weiter Probiert
    // 5 Mal im Abstand von einer Sekunde
    for (i = 0; i < 5; i++) {
        if (input == NULL) {
            input = fopen("kbc:/keyboard/ascii", "r");
        }

        if (output == NULL) {
            output = fopen("textterm:/echo", "w");
        }
        
        // Pruefen ob schon beide erfolgreich geoeffnet wurden
        if ((input == NULL) || (output == NULL)) {
            msleep(1000);
        } else {
            break;
        }
    }
    
    // Wenn nicht beide geoeffnet werden konnten, muss abgebrochen werden
    if ((input == NULL) || (output == NULL)) {
        if (input == NULL) {
            puts("Input konnte nicht geoeffnet werden!");
        } else {
            fclose(input);
        }

        if (output == NULL) {
            puts("Output konnte nicht geoeffnet werden!");
        } else {
            fclose(output);
        }
        return FALSE;
    }
    
    return TRUE;
}

/**
 * Eingaben holen... wird vom Timer aufgerufen
 */
static void get_input()
{
    char buffer[32];
    size_t buffer_pos;

    buffer_pos = fread(buffer, 1, sizeof(buffer), input);
    if (buffer_pos != 0) {
        p();
        vterm_process_input(buffer, buffer_pos);
        v();
    }
    timer_register(&get_input, 50000);
}

