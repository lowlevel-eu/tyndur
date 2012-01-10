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
/*! \addtogroup LostIO
 * @{
 */

#include <stdint.h>
#include "types.h"
#include <syscall.h>
#include "rpc.h"
#include "collections.h"
#include "lostio_internal.h"

///Die Liste mit den Handles fuer die geoeffneten Dateien
list_t* filehandles;

///Die Liste mit den Typehandles
list_t* lostio_types;

/**
 * Initialisiert die LostIO Schnittstelle. Wenn diese Funktion nicht aufgerufen
 * wurde funktioniert LostIO NICHT. Darin werden unter anderem die RPC-Handler
 * registriert und der Root-Knoten wird Vorbereitet.
 */
void lostio_init()
{
    //Root-Verzeichnis initialisieren
    vfstree_root.children = (void*) list_create();
    vfstree_root.name ="/";
    vfstree_root.type = LOSTIO_TYPES_DIRECTORY;
    vfstree_root.flags = LOSTIO_FLAG_BROWSABLE;

    //Liste fuer die geoeffneten Dateihandles erstellen
    filehandles = list_create();

    //Liste fuer die Handler 
    lostio_types = list_create();

    //RPC-Handler registrierenn
    register_message_handler("IO_OPEN ", &rpc_io_open);
    register_message_handler("IO_CLOSE", &rpc_io_close);
    register_message_handler("IO_READ ", &rpc_io_read);
    register_message_handler("IO_READA", &rpc_io_readahead);
    register_message_handler("IO_WRITE", &rpc_io_write);
    register_message_handler("IO_SEEK ", &rpc_io_seek);
    register_message_handler("IO_EOF  ", &rpc_io_eof);
    register_message_handler("IO_TELL ", &rpc_io_tell);
    register_message_handler("IO_LINK ", &rpc_io_link);
    register_message_handler("IO_ULINK", &rpc_io_unlink);
}

/**
 * LostIO-Interne Vorgaenge abarbeiten
 */
void lostio_dispatch()
{
    lostio_sync_dispatch();
}

/**
 * Ein Typehandle aus der Typendatenbank suchen.
 *
 * @param id Id
 *
 * @return Pointer auf das Handle, oder NULL falls es nicht gefunden wurde.
 */
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


/**
 * Trägt ein Typehandle in der Typendatenbank ein. Danach kann es bei den 
 * Knoten verwendet werden.
 *
 * @param typehandle Pointer auf das Typehandle
 */
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


/**
 * Ein geoeffnetes Filehandle suchen.
 *
 * @param pid Pid
 * @param id ID
 *
 * @return Poinrer auf das Filehandle
 */
lostio_filehandle_t* get_filehandle(pid_t pid, uint32_t id)
{
    lostio_filehandle_t* filehandle;
    int i = 0;

    p();
    for (i = 0; (filehandle = list_get_element_at(filehandles, i)) &&
        (filehandle->id != id); i++);
    v();
    return filehandle;
}

/*! @} */

