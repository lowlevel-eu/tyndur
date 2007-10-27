/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Kevin Wolf.
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
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "string.h"

#include "network.h"
#include "ip.h"
#include "tcp.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
#define DEBUG 0

#if DEBUG == 1
    #define DEBUG_MSG(s) printf("[ TCPIP ] debug: %s() '%s'\n",__FUNCTION__,s)
    #define DEBUG_MSG1(x, y) \
        printf("[ TCPIP ] debug: %s() '", __FUNCTION__); \
        printf(x, y); \
        printf("'\n");
    #define DEBUG_MSG2(x, y, z) \
        printf("[ TCPIP ] debug: %s() '", __FUNCTION__); \
        printf(x, y, z); \
        printf("'\n");
#else
    #define DEBUG_MSG(s)
    #define DEBUG_MSG1(s,x)
    #define DEBUG_MSG2(s,x,y)
#endif


#define LOSTIO_TYPES_TCPPORT 255

typehandle_t typehandle;

bool            lostio_tcp_not_found(char**, byte, pid_t,io_resource_t*);
bool            lostio_tcp_pre_open(char**, byte, pid_t,io_resource_t*);
read_hdl_reply  lostio_tcp_read(lostio_filehandle_t*,size_t,size_t);
size_t          lostio_tcp_write(lostio_filehandle_t*,size_t,size_t,void*);
int             lostio_tcp_close(lostio_filehandle_t*);

void init_lostio_interface()
{
    lostio_init();
    lostio_type_directory_use();

    //Typ fuer eine TCP-Verbindung
    typehandle.id          = LOSTIO_TYPES_TCPPORT;
    typehandle.not_found   = &lostio_tcp_not_found;
    typehandle.pre_open    = &lostio_tcp_pre_open;
    typehandle.post_open   = NULL;
    typehandle.read        = &lostio_tcp_read;
    typehandle.write       = &lostio_tcp_write;
    typehandle.seek        = NULL;
    typehandle.close       = &lostio_tcp_close;;

    lostio_register_typehandle(&typehandle);

    get_typehandle(LOSTIO_TYPES_DIRECTORY)->not_found = lostio_tcp_not_found;
   
/*   
    vfstree_create_node("/192.168.1.3:80", LOSTIO_TYPES_TCPPORT, 0, NULL, 0);
    vfstree_create_node("/192.168.1.3:1028", LOSTIO_TYPES_TCPPORT, 0, NULL, 0);
    vfstree_create_node("/192.168.1.3:6667", LOSTIO_TYPES_TCPPORT, 0, NULL, 0);
    vfstree_create_node("/217.26.49.12:6667", LOSTIO_TYPES_TCPPORT, 0, NULL, 0);
*/    
}

bool lostio_tcp_not_found(char** path, byte flags, pid_t pid, io_resource_t* ps)
{
    DEBUG_MSG("Knoten anlegen");
    return vfstree_create_node(*path, LOSTIO_TYPES_TCPPORT, 0, NULL, 0);
}

bool lostio_tcp_pre_open(char** path, byte flags, pid_t pid, io_resource_t* ps)
{
    DEBUG_MSG("pre_open");
    
    vfstree_node_t* node = vfstree_get_node_by_path(*path);
    if (node == NULL) {
        DEBUG_MSG1("Den Knoten %s gibt es nicht", *path);
        return FALSE;
    }

    char* delim = index(*path, ':');
    if (delim == NULL) {
        DEBUG_MSG("Doppelpunkt vermisst");
        return FALSE;
    }

    *delim = '\0';
    dword ip = string_to_ip(*path + 1);
    *delim = ':';

    char* port_string = delim + 1;
    dword port = atoi(port_string);

    if (port & ~0xFFFF) {
        DEBUG_MSG("Port zu gross");
        return FALSE;
    }

    // Beim Öffnen der Verbindung darf der Task nicht blockiert sein, weil
    // sonst das Antwortpaket nicht eintreffen kann

    DEBUG_MSG2("Oeffne Verbindung zu %08x:%d", ip, port);
    struct tcp_connection* conn = tcp_open_connection(ip, port);
    if (conn == NULL) {
        DEBUG_MSG("Konnte Verbindung nicht herstellen");
        return FALSE;
    }

    // FIXME Das sollte eigentlich in den Filehandle, aber auf den haben wir
    // hier keinen Zugriff. Solange es nur eine Verbindung mit dieser IP-Port-
    // Kombination geht, müsste es auch so funktionieren...
    node->data = conn;
    node->size++;

    DEBUG_MSG("Verbindung hergestellt");
    return TRUE;
}

read_hdl_reply lostio_tcp_read
    (lostio_filehandle_t* fh, size_t blocksize ,size_t count)
{
    read_hdl_reply reply = {
        .data = NULL,
        .size = 0
    };

    struct tcp_connection* conn = fh->node->data;
    size_t size = blocksize * count;

    p();

    if ((conn->status == TCPS_CLOSED) && (list_is_empty(conn->to_lostio))) {
        fh->flags |= LOSTIO_FLAG_EOF;
    }
     
    struct tcp_in_buffer* buffer;
    do {
        buffer = list_get_element_at(conn->to_lostio, 0);
        
        if (buffer == NULL) {
            v();
            return reply;
        }

        if (buffer->seq == buffer->size) {
            list_pop(conn->to_lostio);
            continue;
        }
    
    } while(buffer->seq == buffer->size);

    if (buffer->size - buffer->seq <= size) {
        size = buffer->size - buffer->seq;
    }

    reply.data = buffer->data + buffer->seq;
    reply.size = size;

    buffer->seq += size;
    
    v();
    return reply;
}

size_t lostio_tcp_write
    (lostio_filehandle_t* fh, size_t blocksize, size_t count, void* data)
{
    p();

    DEBUG_MSG1("%d Bytes", blocksize * count);
    struct tcp_connection* conn = fh->node->data;
    
    if (conn->status != TCPS_CONNECTED) 
    {
        if ((conn->status == TCPS_CLOSED) && (list_is_empty(conn->to_lostio))) 
        {
            fh->flags |= LOSTIO_FLAG_EOF;
        }

        v();
        return 0;
    }
    
    char xdata[blocksize * count + 1];
    memcpy(xdata, data, blocksize * count + 1);
    xdata[blocksize * count] = '\0';

    list_insert(conn->out_buffer, list_size(conn->out_buffer), xdata);
    //list_push(conn->out_buffer, "GET / HTTP/1.0\r\n\r\n");
    
    while (!list_is_empty(conn->out_buffer)) {
        tcp_send_ack(conn);
    }
    
    if ((conn->status == TCPS_CLOSED) && (list_is_empty(conn->to_lostio))) {
        fh->flags |= LOSTIO_FLAG_EOF;
    }
    DEBUG_MSG("Fertig gesendet.");
    
    v();
    return blocksize * count;
}

int lostio_tcp_close(lostio_filehandle_t* filehandle)
{
    p();
    // TODO tcp_connection_close
    /*
     * TODO Reaktivieren, sobald die Verbindungen nicht mehr hartkodiert sind
    if (--(filehandle->node->size) == 0) {
        vfstree_delete_node(filehandle->node->name);
    }
    */
    v();

    return 0;
}

