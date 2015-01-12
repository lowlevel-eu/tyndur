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

#include <types.h>
#include "syscall.h"
#include <stdio.h>
#include <rpc.h>
#include "lostio.h"
#include "vterm.h"
#include "sleep.h"
#include "init.h"
#include "keymap.h"


/// Hier kommen die Eingaben fuer all die virtuellen Terminals her
FILE* input = NULL;

int main(int argc, char* argv[])
{
    pid_t servmgr_pid;

    // Schnittstelle fuer die anderen Prozesse einrichten
    init_lostio_interface();

    // Virtuelle Terminal initialisieren
    init_output();
    init_vterminals(10);

    // Service registrieren
    //
    // HACK: Wenn wir uns fast gleichzeitig bei init und servmgr anmelden,
    // treiben wir sie direkt in einen Deadlock (servmgr brauch init, um
    // vterm:/vterm8/out zu oeffnen und init will "service_register" auf stdout
    // schreiben, das beim servmgr liegt). Also warten wir, bis init fertig
    // ist.
    init_service_register("vterm");
    while(!init_service_get("vterm")) {
        yield();
    }

    // servmgr kann aufhoeren, syscall_putsn zu benutzen
    servmgr_pid = init_service_get("servmgr");
    if (servmgr_pid != 0) {
        rpc_get_dword(servmgr_pid, "VTERM   ", 0, NULL);
    }

    // Eingabe initialisieren
    // Dies registriert einen RPC-Callbackhandler bei kbc. Der Aufruf
    // von init_input() darf daher erst geschehen, wenn auch das Registrieren
    // des Services bei init sicher ist.
    if (init_input() == false) {
        // Falls das nicht klappt, ist alles sinnlos. ;-)
        return -1;
    }

    // Auf Anfragen warten
    while(true) {
        lio_dispatch();
        lio_srv_wait();
	}
}
