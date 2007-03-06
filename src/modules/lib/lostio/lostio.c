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
#include "rpc.h"
#include "collections.h"
#include "lostio_internal.h"

list_t* filehandles;
list_t* lostio_types;

/**
 * 
 */
void lostio_init()
{
    //Root-Verzeichnis initialisieren
    vfstree_root.children = (void*) list_create();
    vfstree_root.name ="/";
    vfstree_root.type = LOSTIO_TYPES_DIRECTORY;

    //Liste fuer die geoeffneten Dateihandles erstellen
    filehandles = list_create();

    //Liste fuer die Handler 
    lostio_types = list_create();

    //RPC-Handler registrierenn
    register_message_handler("IO_OPEN ", &rpc_io_open);
    register_message_handler("IO_CLOSE", &rpc_io_close);
    register_message_handler("IO_READ ", &rpc_io_read);
    register_message_handler("IO_WRITE", &rpc_io_write);
    register_message_handler("IO_SEEK ", &rpc_io_seek);
}


typehandle_t* get_typehandle(typeid_t id)
{
    typehandle_t* typehandle;
    int i = 0;

    while((typehandle = list_get_element_at(lostio_types, i++)))
    {
        if(typehandle->id == id)
        {
            break;
        }
    }
    //printf("[ LOSTIO ] Typ-Handle gesucht: %d = 0x%x\n", id, (dword) typehandle);
    return typehandle;
}


void lostio_register_typehandle(typehandle_t* typehandle)
{
    typehandle_t* oldtypehandle = get_typehandle(typehandle->id);
    
    //printf("[ LOSTIO ] Typ-Handle registriert: %d,  0x%x\n", typehandle->id, (dword) typehandle);

    //Falls schon ein Handler fuer die ID registriert
    
    if(oldtypehandle == NULL)
    {
        lostio_types = list_push(lostio_types, (void*)typehandle);
    }
    else
    {
        //altes Handle ueberschreiben.
        *oldtypehandle = *typehandle;
    }
}


lostio_filehandle_t* get_filehandle(pid_t pid, dword id)
{
    lostio_filehandle_t* filehandle;
    int i = 0;

    while((filehandle = list_get_element_at(filehandles, i++)))
    {
        if((filehandle->id == id) && (filehandle->pid == pid))
        {
            break;
        }
    }
    
    return filehandle;
}

