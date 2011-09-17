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
#include <init.h>
#include <syscall.h>
#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "lostio.h"
#include "io.h"
#include "time.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
//#define DEBUG_MSG(s) printf("[  TMP  ] debug: '%s'\n", s)
#define DEBUG_MSG(s) //

bool create_handler(char** path, uint8_t args, pid_t pid,FILE* source);

size_t current_id = 0;

int main(int argc, char* argv[])
{
    lostio_init();

    lostio_type_ramfile_use();
    //LOSTIO_TYPES_RAMFILE
    lostio_type_directory_use();    // Und mit Verzeichnissen ;)
    get_typehandle(LOSTIO_TYPES_DIRECTORY)->not_found = &create_handler;

    //Nun wird das Ramfile erstellt
    //vfstree_create_node("/version", LOSTIO_TYPES_RAMFILE, strlen(version) + 1,
    //    (void*)version, 0);

    //Beim Init unter dem Namen "cmos" registrieren, damit die anderen
    //Prozesse den Treiber finden.
    init_service_register("tmp");
    while(true)
    {
        wait_for_rpc();
    }
}


bool create_handler(char** path, uint8_t args, pid_t pid,FILE* source)
{
    current_id++;
    char* new_path;
    if (asprintf(&new_path, "/%d", current_id++) == -1) {
        return false;
    }
    *path = new_path;
    vfstree_create_node(new_path, LOSTIO_TYPES_RAMFILE, 0, NULL, 0);
    return true;
}


