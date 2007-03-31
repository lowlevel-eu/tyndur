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
/** \addtogroup LostIO
 * @{
 */

#include "types.h"
#include "rpc.h"
#include "collections.h"
#include "lostio_internal.h"
#include "syscall.h"
#include "init.h"
#include "io.h"
#include "stdio.h"
#include "stdlib.h"


///Handler zum oeffnen einer Datei
static lostio_filehandle_t* lostio_open(char* path, byte attr, pid_t pid, io_resource_t* io_source);

///Handler zum Schliessen einer Datei
static bool lostio_close(dword id, pid_t pid);


dword handle_id = 1;



/**
 * Wird aufgerufen, wenn ein Prozess ein fopen fuer irgendeine Datei in floppy:/
 * ausfuehrt.
 */
void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_resource_t io_res;
    
    //Wird bei pipes als quelle verwendet
    io_resource_t* io_source = (io_resource_t*) ((dword)data + 1 + sizeof(pid_t));
    
    byte* attr = (byte*) data;
    pid_t* caller_pid = (pid_t*) ((dword)data + 1 );
    char* path = (char*) ((dword)data + 1 + sizeof(pid_t) + sizeof(io_resource_t));
    
    io_res.pid = get_pid();
    
    lostio_filehandle_t* filehandle = lostio_open(path, *attr, *caller_pid, io_source);;
    
    if(filehandle == NULL)
    {
        io_res.pid = 0;
        rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
    }
    else
    {
        io_res.id = filehandle->id;
        io_res.resid = filehandle->node->resid;
    
       //Falls da Oeffnen fehlgeschlagen ist, wird die PID im Handle auf 0 gesetzt
        if(io_res.id == 0)
        {
            io_res.pid = 0;
        }
    
        rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
    }

}


/**
 * Wird aufgerufen, wenn ein Prozess ein fclose mit einem File-Handle vom
 * aktuellen Treiber als Parameter aufruft.
 */
void rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_resource_t* io_res = (io_resource_t*) data;
    
    if(lostio_close(pid, io_res->id) == TRUE)
    {
        rpc_send_dword_response(pid, correlation_id, 0);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, (dword)-1);
    }
}


/**
 * Ein Prozess hat ein fread auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_read_request_t* read_request = (io_read_request_t*) data;
    
    lostio_filehandle_t* filehandle = get_filehandle(pid, read_request->id);
    
    if(filehandle == NULL)
    {
        puts("Filehandle nicht gefunden");
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }
    
    typehandle_t* typehandle = get_typehandle(filehandle->node->type);
    

    if((typehandle != NULL) && (typehandle->read != NULL))
    {
        read_hdl_reply reply = typehandle->read(filehandle, read_request->blocksize, read_request->blockcount);
        
        rpc_send_response(pid, correlation_id, reply.size, (char*)(reply.data));
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, 0);
    }
    /*v();
    yield();*/
}


/**
 * Ein Prozess hat ein fwrite auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    printf("0pid:%d\n", pid);
    io_write_request_t* write_request = (io_write_request_t*) data;
    printf("1pid:%d\n", pid);
    lostio_filehandle_t* filehandle = get_filehandle(pid, write_request->id);
    typehandle_t* typehandle = get_typehandle(filehandle->node->type);
    printf("2pid:%d\n", pid);
    if((typehandle != NULL) && (typehandle->write != NULL))
    {
        puts("as");
        printf("%d\n", filehandle->node->type);
        printf("2.5pid:%d\n", pid);
        size_t result = typehandle->write(filehandle, write_request->blocksize, write_request->blockcount, write_request->data);
        puts("---");
        printf("3pid:%d\n", pid);
        rpc_send_dword_response(pid, correlation_id, (dword) result);
    }
    else
    {
        puts("bs");
        printf("4pid:%d\n", pid);
        rpc_send_dword_response(pid, correlation_id, (dword)-1);
    }
}


/**
 * Ein Prozess hat ein fseek auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_seek(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_seek_request_t* seek_request = (io_seek_request_t*) data;
    
    //printf("id: %d\n", seek_request->id);

    lostio_filehandle_t* filehandle = get_filehandle(pid, seek_request->id);
    if(filehandle == NULL)
    {
        rpc_send_dword_response(pid, correlation_id, (dword)-1);
        return;
    }

    typehandle_t* typehandle = get_typehandle(filehandle->node->type);

    if((typehandle != NULL) && (typehandle->seek != NULL))
    {
        int result = typehandle->seek(filehandle, seek_request->offset, seek_request->origin);

        rpc_send_dword_response(pid, correlation_id, (dword) result);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, (dword)-1);
    }
}


/**
 * Ein Prozess hat ein feof auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_eof(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_eof_request_t* eof_request = (io_eof_request_t*) data;

    lostio_filehandle_t* filehandle = get_filehandle(pid, eof_request->id);


    if((filehandle != NULL) && ((filehandle->flags & LOSTIO_FLAG_EOF) != 0))
    {
        rpc_send_dword_response(pid, correlation_id, (dword) EOF);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, (dword)0);
    }
}


/**
 *  * Ein Prozess hat ein feof auf einem File-Handle dieses Treibers ausgefuehrt
 *   */
void rpc_io_tell(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_eof_request_t* tell_request = (io_eof_request_t*) data;
    lostio_filehandle_t* filehandle = get_filehandle(pid, tell_request->id);
    qword response;

    if((filehandle != NULL))
    {
        response = filehandle->pos;
    }
    else
    {
        response = (qword) -1;
    }

    rpc_send_response(pid, correlation_id, sizeof(qword), (void*) &response);
}





/**
 * Wird vom RPC-Handler bei einem fopen aufgerufen.
 *
 * @param path Pfad
 * @param attr Das Atributbyte
 * @param pid Prozess-ID
 *
 * @return Die ID des Handles oder 0 wenn ein Fehler aufgetreten ist.
 */
static lostio_filehandle_t* lostio_open(char* path, byte attr, pid_t pid, io_resource_t* io_source)
{ 
    vfstree_node_t* node = vfstree_get_node_by_path(path);
    typehandle_t* typehandle;
    
    //puts(path);
    //Wenn der Knoten nicht gefunden wurde, wird jetzt der Handler fuer
    //inexistente Dateien(id=0) aufgerufen.
    if(node == NULL)
    {
        char* parent_path = vfstree_dirname(path);
        vfstree_node_t* parent_node = vfstree_get_node_by_path(parent_path);
        free(parent_path);
        
        if(parent_node != NULL)
        {
            typehandle = get_typehandle(parent_node->type);

            if((typehandle != NULL) && (typehandle->not_found != NULL))
            {
                if(typehandle->not_found(&path, attr, pid, io_source) == FALSE)
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }
        else
        {
            typehandle = get_typehandle(0);
        
            //Wenn der Handler nicht vorhanden ist, oder der Typ nicht registriert,
            //dann wird das oeffnen abgebrochen.
            if((typehandle == NULL) || (typehandle->not_found == NULL))
            {
                printf("Datei nicht vorhanden: '%s'\n", path);
                return 0;
            }
            else
            {
                if(typehandle->not_found(&path, attr, pid, io_source) == FALSE)
                {
                    return 0;
                }
            }
        }
    }
    
    node = vfstree_get_node_by_path(path);

    if(node != NULL)
    {
        if(((attr & LOSTIO_MODE_BROWSE) != 0) && ((node->flags & LOSTIO_FLAG_BROWSABLE) == 0))
        {
            return 0;
        }
        typehandle = get_typehandle(node->type);
        if((typehandle != NULL) && (typehandle->pre_open != NULL))
        {
            if(typehandle->pre_open(&path, attr, pid, io_source) == FALSE)
            {
                return 0;
            }
        }
    }

    
    
    
    node = vfstree_get_node_by_path(path);
    

    if(node == NULL)
    {
        return 0;
    }

    
    if(((attr & LOSTIO_MODE_BROWSE) != 0) && ((node->flags & LOSTIO_FLAG_BROWSABLE) == 0))
    {
        return 0;
    }

    
    lostio_filehandle_t* handle = malloc(sizeof(lostio_filehandle_t));
    
    handle->id      = handle_id++;
    handle->pid     = pid;
    handle->flags   = attr;
    handle->node    = node;
    handle->source  = io_source;
    handle->pos     = 0;

    filehandles = list_push(filehandles, (void*) handle);
    //printf("[ LOSTIO ] Oeffne '%s'  '%s'\n", node->name, path);

    //post_open Handler aufrufen falls vorhanden
    typehandle = get_typehandle(node->type);
    if((typehandle != NULL) && (typehandle->post_open != NULL))
    {
        typehandle->post_open(handle);
    }
    return handle;
}


/**
 * Wird vom RPC-Handler bei einem fclose aufgerufen.
 *
 * @param id Handle-ID, die beim oeffnen zurueck gegeben wurde
 * @param pid Prozess-ID
 *
 * @return TRUE, wenn erfolg
 */
static bool lostio_close(dword id, pid_t pid)
{
    lostio_filehandle_t* handle;
    int i = 0;
    
    //Ab hier darf uns nichts mehr dazwischen kommen, da sonst der gespeicherte
    //Index ind der Liste nicht mehr stimmen könnte.
    p();
    //Zuerst das Handle anhand der ID und PID suchen
    while((handle = list_get_element_at(filehandles, i++)))
    {
        if((handle->id == id) && (handle->pid == pid))
        {
            break;
        }
    }

    //Abbrechen falls das Handle nicht gefunden wurde
    if(handle == NULL)
    {
        v();
        return FALSE;
    }
    
    
    free(handle);

    //Das Handle aus der liste entfernen
    list_remove(filehandles, i);
    v();
    return TRUE;
}

/** @} */

