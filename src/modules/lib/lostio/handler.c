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
#include <string.h>


///Handler zum oeffnen einer Datei
static lostio_filehandle_t* lostio_open(char* path, uint8_t attr, pid_t pid, FILE* io_source);

///Handler zum Schliessen einer Datei
static bool lostio_close(uint32_t id, pid_t pid);


uint32_t handle_id = 1;


/**
 * Wird aufgerufen, wenn ein Prozess ein fopen fuer irgendeine Datei in floppy:/
 * ausfuehrt.
 */
void rpc_io_open(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    static pid_t my_pid = 0;
    io_resource_t io_res;
    memset(&io_res, 0, sizeof(io_resource_t));
    //p();
    
    //Wird bei pipes als quelle verwendet
    //FIXME Das wird wohl nirgends wieder freigegeben
    io_resource_t* io_source = calloc(1, sizeof(*io_source));
    memcpy(io_source, (char*) data  + 1+ sizeof(pid_t), sizeof(*io_source));


    struct lostio_internal_file* int_source = calloc(1, sizeof(*int_source));
    int_source->res = io_source;
    int_source->free_buffer = false;
    
    uint8_t* attr = (uint8_t*) data;
    pid_t* caller_pid = (pid_t*) ((uint32_t)data + 1 );
    char* path = (char*) ((uint32_t)data + 1 + sizeof(pid_t) + sizeof(io_resource_t));
    
    if (my_pid == 0) {
        my_pid = get_pid();
    }
    io_res.pid = my_pid;
    
    lostio_filehandle_t* filehandle = lostio_open(path, *attr, *caller_pid, int_source);;
    
    if(filehandle == NULL)
    {
        io_res.pid = 0;
        rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
        free(int_source->res);
        free(int_source);
    }
    else
    {
        // Ueberpruefen ob es sich um einen Symlink handelt, und ob der
        // Aufrufer nicht die Adresse auslesen will.
        if (((filehandle->node->flags & LOSTIO_FLAG_SYMLINK) != 0) && 
            ((*attr & IO_OPEN_MODE_LINK) == 0)) 
        {
            // Erst muessen wir an den Pfad kommen. Das geht durch Lesen des
            // Symlinks.
            typehandle_t* typehandle = get_typehandle(filehandle->node->type);
            if((typehandle == NULL) || (typehandle->read == NULL)) {
                io_res.id = 0;
            } else {
                // FIXME: Diese Hartkodierte Laenge ist garnicht gut.
                uint8_t buf[1024];
                size_t size = typehandle->read(filehandle, buf, 1, 1024);

                if (size != 0) {
                    char* path = (char*) buf;
                    char msg[strlen(path) + 2];
                    msg[0] = *attr;
                    memcpy(msg + 1, path, strlen(path) + 1);

                    response_t* resp = rpc_get_response(1, "IO_OPEN ", 
                        strlen(path) + 2, msg);
                    if ((resp == NULL) || (resp->data == NULL)) {
                        io_res.id = 0;
                    } else {
                        memcpy(&io_res, resp->data, sizeof(io_resource_t));
                    }
                }

                // Symlink wieder schliessen, sonst kann er anschliessend nicht
                // mehr geloescht werden.
                lostio_close(pid, filehandle->id);
                goto out;
            }
        } else {
            io_res.id = filehandle->id;
            io_res.resid = filehandle->node->resid;
        }
       //Falls da Oeffnen fehlgeschlagen ist, wird die PID im Handle auf 0 gesetzt
        if(io_res.id == 0)
        {
            io_res.pid = 0;
            free(int_source->res);
            free(int_source);
        }
        else if ((filehandle->node->flags & LOSTIO_FLAG_READAHEAD) != 0) {
            io_res.flags |= IO_RES_FLAG_READAHEAD;
        }

    out:
        rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
    }
    //v();
}


/**
 * Wird aufgerufen, wenn ein Prozess ein fclose mit einem File-Handle vom
 * aktuellen Treiber als Parameter aufruft.
 */
void rpc_io_close(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_resource_t* io_res = (io_resource_t*) data;
    //p();

    if(lostio_close(pid, io_res->id) == true)
    {
        rpc_send_dword_response(pid, correlation_id, 0);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, (uint32_t)-1);
    }
    //v();
}


/**
 * Ein Prozess hat ein fread auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_read(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_read_request_t* read_request = (io_read_request_t*) data;
    //p();

    lostio_filehandle_t* filehandle = get_filehandle(pid, read_request->id);
    
    if(filehandle == NULL)
    {
        puts("Filehandle nicht gefunden");
        rpc_send_dword_response(pid, correlation_id, 0);
        //v();
        return;
    }
    
    typehandle_t* typehandle = get_typehandle(filehandle->node->type);
    

    // Wenn kein Handler fuer read registriert ist, abbrechen
    if ((typehandle == NULL) || (typehandle->read == NULL)) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }

    // Falls kein Shared Memory uebergeben wurde, temporaeren Buffer auf dem
    // Stack anlegen und per RPC zurueckschicken
    if (read_request->shared_mem_id == 0) {
        uint8_t buf[read_request->blocksize * read_request->blockcount];
        size_t size = typehandle->read(filehandle, buf,
            read_request->blocksize, read_request->blockcount);

        rpc_send_response(pid, correlation_id, size, (char*) buf);
        return;
    }

    // Ansonsten direkt in den SHM einlesen
    void* shm_ptr = open_shared_memory(read_request->shared_mem_id);
    size_t size = typehandle->read(filehandle, shm_ptr,
            read_request->blocksize, read_request->blockcount);
    close_shared_memory(read_request->shared_mem_id);

/*            printf("size = %d, ptr = %p, value = %d\n",
        sizeof(reply.size), &reply.size, reply.size);
    fflush(stdout);*/
    rpc_send_dword_response(pid, correlation_id, size);
    //v();
}

void rpc_io_readahead(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data)
{
    io_read_request_t* read_request = (io_read_request_t*) data;

    lostio_filehandle_t* filehandle = get_filehandle(pid, read_request->id);

    if(filehandle == NULL) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }

    typehandle_t* typehandle = get_typehandle(filehandle->node->type);

    // Wenn kein Handler fuer read registriert ist, abbrechen
    if ((typehandle == NULL) || (typehandle->readahead == NULL)) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }

    // Falls kein Shared Memory uebergeben wurde, temporaeren Buffer auf dem
    // Stack anlegen und per RPC zurueckschicken
    if (read_request->shared_mem_id == 0) {
        uint8_t buf[read_request->blocksize * read_request->blockcount];
        size_t size = typehandle->readahead(filehandle, buf,
            read_request->blocksize, read_request->blockcount);

        rpc_send_response(pid, correlation_id, size, (char*) buf);
        return;
    }

    // Ansonsten direkt in den SHM einlesen
    void* shm_ptr = open_shared_memory(read_request->shared_mem_id);
    size_t size = typehandle->readahead(filehandle, shm_ptr,
            read_request->blocksize, read_request->blockcount);
    close_shared_memory(read_request->shared_mem_id);

    rpc_send_dword_response(pid, correlation_id, size);
}

/**
 * Ein Prozess hat ein fwrite auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_write(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    typehandle_t* typehandle = NULL;
    io_write_request_t* write_request = (io_write_request_t*) data;
    
    //p();

    lostio_filehandle_t* filehandle = get_filehandle(pid, write_request->id);
    if (filehandle != NULL) {
        typehandle = get_typehandle(filehandle->node->type);
    }
    
    if((typehandle != NULL) && (typehandle->write != NULL))
    {
        void* write_data;
        if (write_request->shared_mem_id == 0) {
            write_data = write_request->data;

            // Laenge pruefen
            if (write_request->blocksize * write_request->blockcount >
                data_size - offsetof(io_write_request_t, data))
            {
                rpc_send_dword_response(pid, correlation_id, (uint32_t)-1);
                return;
            }
        } else {        
            write_data = open_shared_memory(write_request->shared_mem_id);
            // TODO Groesse des Puffers pruefen
        }

        if (write_data == NULL) {
            rpc_send_dword_response(pid, correlation_id, (uint32_t)-1);
            return;
        }

        size_t result = typehandle->write(filehandle, write_request->blocksize, write_request->blockcount, write_data);

        if (write_request->shared_mem_id != 0) {
            close_shared_memory(write_request->shared_mem_id);
        }
 
        rpc_send_dword_response(pid, correlation_id, (uint32_t) result);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, (uint32_t)-1);
    }
    //v();
}


/**
 * Ein Prozess hat ein fseek auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_seek(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_seek_request_t* seek_request = (io_seek_request_t*) data;
    
    //p();

    lostio_filehandle_t* filehandle = get_filehandle(pid, seek_request->id);
    if(filehandle == NULL)
    {
        rpc_send_dword_response(pid, correlation_id, (uint32_t)-1);
        //v();
        return;
    }

    typehandle_t* typehandle = get_typehandle(filehandle->node->type);

    if((typehandle != NULL) && (typehandle->seek != NULL))
    {
        int result = typehandle->seek(filehandle, seek_request->offset, seek_request->origin);

        rpc_send_dword_response(pid, correlation_id, (uint32_t) result);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, (uint32_t)-1);
    }
    //v();
}


/**
 * Ein Prozess hat ein feof auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_eof(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_eof_request_t* eof_request = (io_eof_request_t*) data;
    
    //p();

    lostio_filehandle_t* filehandle = get_filehandle(pid, eof_request->id);


    if((filehandle != NULL) && ((filehandle->flags & LOSTIO_FLAG_EOF) != 0))
    {
        rpc_send_dword_response(pid, correlation_id, (uint32_t) EOF);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, (uint32_t)0);
    }
    //v();
}


/**
 * Ein Prozess hat ein feof auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_tell(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_eof_request_t* tell_request = (io_eof_request_t*) data;
    //p();

    lostio_filehandle_t* filehandle = get_filehandle(pid, tell_request->id);
    uint64_t response;
    
    if((filehandle != NULL))
    {
        response = filehandle->pos;
    }
    else
    {
        response = (uint64_t) -1;
    }

    rpc_send_response(pid, correlation_id, sizeof(uint64_t), (void*) &response);
    //v();
}


/**
 * Ein Prozess hat ein io_link auf einem File-Handle dieses Treibers ausgefuehrt
 * 
 * RPC-Rueckgabewerte:
 *   0: Erfolg
 *
 *  -1: Ziel oder Verzeichnis existiert nicht
 *  -2: Fuer das Verzeichnis ist kein Link-Handler eingetragen
 *  -3: Das Format der erhaltenen RPC-Daten stimmt nicht
 *  -4: Beim erstellen des Links ist ein Fehler im Handler aufgetreten
 */
void rpc_io_link(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_link_request_t* link_request = (io_link_request_t*) data;
    
    // Pruefen, ob das Format stimmt, also ob soviele Daten da sind, wie wir
    // erwarten
    if ((sizeof(io_link_request_t) > data_size) || ((link_request->name_len +
        sizeof(io_link_request_t)) > data_size) || (strnlen(link_request->name,
        link_request->name_len + 1) != link_request->name_len))
    {
        rpc_send_int_response(pid, correlation_id, -3);
        return;
    }
   

    // Filehandles fuer Link-Ziel und Verzeichnis holen
    lostio_filehandle_t* target_filehandle = get_filehandle(pid, link_request->
        target_id);
    lostio_filehandle_t* dir_filehandle = get_filehandle(pid, link_request->
        dir_id);

    // Wenn eines der Filehandles nicht exisitiert muss abgebrochen werden
    if((target_filehandle == NULL) || (dir_filehandle == NULL))
    {
        rpc_send_int_response(pid, correlation_id, -1);
        return;
    }

    // Typ-Handle fuer Verzeichnis holen
    typehandle_t* typehandle = get_typehandle(dir_filehandle->node->type);
    if((typehandle != NULL) && (typehandle->link != NULL))
    {
        int result = typehandle->link(target_filehandle, dir_filehandle,
            link_request->name);
        if (result != 0) {
            result = -4;
        }
        rpc_send_int_response(pid, correlation_id, result);
    }
    else
    {
        rpc_send_int_response(pid, correlation_id, -2);
    }
}

/**
 * Ein Prozess hat ein io_remove auf einem File-Handle dieses Treibers ausgefuehrt
 *
 * RPC-Rueckgabewerte:
 *   0: Erfolg
 *
 *  -1: Verzeichnis oder Datei existiert nicht
 *  -2: Fuer das Verzeichnis ist kein Unlink-Handler eingetragen
 *  -3: Das Format der erhaltenen RPC-Daten stimmt nicht
 *  -4: Beim eigentlichen Loeschen des Links ist ein Fehler aufgetreten
 */
void rpc_io_unlink(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_unlink_request_t* unlink_request = (io_unlink_request_t*) data;

    // Pruefen, ob das Format stimmt, also ob soviele Daten da sind, wie wir
    // erwarten
    if ((sizeof(io_unlink_request_t) > data_size) || ((unlink_request->name_len
        + sizeof(io_unlink_request_t)) > data_size) || (strnlen(
        unlink_request->name, unlink_request->name_len + 1) != unlink_request->
        name_len))
    {
        rpc_send_int_response(pid, correlation_id, -3);
        return;
    }

    // Filehandles fuer Verzeichnis holen
    lostio_filehandle_t* dir_filehandle = get_filehandle(pid, unlink_request->
        dir_id);

    // Wenn eines der Filehandles nicht exisitiert muss abgebrochen werden
    if (dir_filehandle == NULL)
    {
        rpc_send_int_response(pid, correlation_id, -1);
        return;
    }

    vfstree_node_t* node = vfstree_get_node_by_name(dir_filehandle->node,
       unlink_request->name);
    if (node == NULL) {
        rpc_send_int_response(pid, correlation_id, -1);
        return;
    }

    // Keine Datei loeschen die geoffnet ist.
    lostio_filehandle_t* fh;
    int i;
    for (i = 0; (fh = list_get_element_at(filehandles, i)); i++) {
        if (fh->node == node) {
            rpc_send_int_response(pid, correlation_id, -1);
            return;
        }
    }
    // Typ-Handle fuer Verzeichnis holen
    typehandle_t* typehandle = get_typehandle(dir_filehandle->node->type);
    if((typehandle != NULL) && (typehandle->unlink != NULL))
    {
        int result = typehandle->unlink(dir_filehandle, unlink_request->name);
        if (result != 0) {
            result = -4;
        }
        rpc_send_int_response(pid, correlation_id, result);
    }
    else
    {
        rpc_send_int_response(pid, correlation_id, -2);
    }
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
static lostio_filehandle_t* lostio_open(char* path, uint8_t attr, pid_t pid, FILE* io_source)
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
                if(typehandle->not_found(&path, attr, pid, io_source) == false)
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
                return 0;
            }
            else
            {
                if(typehandle->not_found(&path, attr, pid, io_source) == false)
                {
                    return 0;
                }
            }
        }
    }
    
    node = vfstree_get_node_by_path(path);

    if(node != NULL)
    {
        if(((attr & IO_OPEN_MODE_DIRECTORY) != 0) && ((node->flags & LOSTIO_FLAG_BROWSABLE) == 0))
        {
            return 0;
        }
        typehandle = get_typehandle(node->type);
        if((typehandle != NULL) && (typehandle->pre_open != NULL))
        {
            if(typehandle->pre_open(&path, attr, pid, io_source) == false)
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

    
    if(((attr & IO_OPEN_MODE_DIRECTORY) != 0) && ((node->flags & LOSTIO_FLAG_BROWSABLE) == 0))
    {
        return 0;
    }

    
    lostio_filehandle_t* handle = malloc(sizeof(lostio_filehandle_t));
    
    handle->id      = handle_id++;
    handle->pid     = pid;
    handle->flags   = attr;
    handle->source  = io_source;
    handle->pos     = 0;
    handle->data    = NULL;
    handle->node    = node;
    
    filehandles = list_push(filehandles, (void*) handle);
    //printf("[ LOSTIO ] Oeffne '%s'  '%s'\n", node->name, path);
    
    // Wenn der Knoten eine Groesse von 0 hat, muss schon das EOF-Flag gesetzt
    // werden.
    if ((node->size == 0) && ((node->flags & LOSTIO_FLAG_NOAUTOEOF) == 0)) {
        handle->flags |= LOSTIO_FLAG_EOF;
    }

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
 * @return true, wenn erfolg
 */
static bool lostio_close(uint32_t pid, pid_t id)
{
    lostio_filehandle_t* handle;
    typehandle_t* typehandle;
    int i;
    
    //Ab hier darf uns nichts mehr dazwischen kommen, da sonst der gespeicherte
    //Index ind der Liste nicht mehr stimmen könnte.
    p();
    
    //Zuerst das Handle anhand der ID und PID suchen
    for(i = 0; (handle = list_get_element_at(filehandles, i)); i++)
    {
        if((handle->id == id) /*&& (handle->pid == pid)*/)
        {
            break;
        }
    }
    
    //Abbrechen falls das Handle nicht gefunden wurde
    if(handle == NULL)
    {
        v();
        return false;
    }
    
    list_remove(filehandles, i);
    v();
    
    typehandle = get_typehandle(handle->node->type);
    if((typehandle != NULL) && (typehandle->close != NULL))
    {
        typehandle->close(handle);
    }   
    
    free(handle);
    
    return true;
}

/** @} */

