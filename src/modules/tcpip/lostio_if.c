/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "string.h"

#include "main.h"
#include "network.h"
#include "ip.h"
#include "tcp.h"
#include "udp.h"
#include "lostio_if.h"
#include "dns.h"
#include "dhcp.h"

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


typehandle_t typehandle;
typehandle_t typehandle2;
typehandle_t typehandle3;

bool   lostio_tcp_not_found(char**, uint8_t, pid_t,FILE*);
bool   lostio_tcp_pre_open(char**, uint8_t, pid_t,FILE*);
size_t lostio_tcp_read(lostio_filehandle_t*,void*,size_t,size_t);
size_t lostio_tcp_write(lostio_filehandle_t*,size_t,size_t,void*);
int    lostio_tcp_close(lostio_filehandle_t*);
size_t lostio_netconfig_read(lostio_filehandle_t*,void*,size_t,size_t);
size_t lostio_netconfig_write(lostio_filehandle_t*,size_t,size_t,void*);
size_t lostio_route_read(lostio_filehandle_t*,void*,size_t,size_t);
size_t lostio_route_write(lostio_filehandle_t*,size_t,size_t,void*);
int    lostio_route_close(lostio_filehandle_t*);

void init_lostio_interface(void)
{
    lostio_init();
    lostio_type_directory_use();
    lostio_type_ramfile_use();

    //Typ fuer eine TCP-Verbindung
    typehandle.id          = LOSTIO_TYPES_TCPPORT;
    typehandle.not_found   = &lostio_tcp_not_found;
    typehandle.pre_open    = &lostio_tcp_pre_open;
    typehandle.post_open   = NULL;
    typehandle.read        = &lostio_tcp_read;
    typehandle.write       = &lostio_tcp_write;
    typehandle.seek        = NULL;
    typehandle.close       = &lostio_tcp_close;;
    typehandle.link        = NULL;
    typehandle.unlink      = NULL;

    lostio_register_typehandle(&typehandle);

    // Typ fuer die Netzwerkkartenconfigs
    memset(&typehandle2, 0, sizeof(typehandle_t));
    typehandle2.id          = LOSTIO_TYPES_NETCONFIG;
    typehandle2.read        = &lostio_netconfig_read;
    typehandle2.write       = &lostio_netconfig_write;

    lostio_register_typehandle(&typehandle2);

    // Typ fuer die Netzwerkkartenconfigs
    memset(&typehandle3, 0, sizeof(typehandle_t));
    typehandle3.id          = LOSTIO_TYPES_ROUTE;
    typehandle3.read        = &lostio_route_read;
    typehandle3.write       = &lostio_route_write;
    typehandle3.close       = &lostio_route_close;

    lostio_register_typehandle(&typehandle3);

    // Typ fuer das DNS-Verzeichnis
    lostio_type_directory_use_as(LOSTIO_TYPES_DNS);
    get_typehandle(LOSTIO_TYPES_DNS)->not_found = lostio_tcp_not_found;
    vfstree_get_node_by_path("/")->type = LOSTIO_TYPES_DNS;

    // Weitere Typen
    tcp_server_register_lostio();
    udp_register_lostio();
    dhcp_register_lostio();

    // Dateien/Verzeichnisse anlegen
    vfstree_create_node("/dns", LOSTIO_TYPES_DNS, 0, NULL, 
        LOSTIO_FLAG_BROWSABLE);
    vfstree_create_node("/route", LOSTIO_TYPES_ROUTE, 1, NULL, 0);
}

void lostio_add_driver(struct driver *driver)
{
    char fsname[strlen(driver->name) + 3];
    fsname[0] = '/';
    strcpy(fsname + 1, driver->name);
    vfstree_create_node(fsname, LOSTIO_TYPES_DIRECTORY, 0, NULL, 
                        LOSTIO_FLAG_BROWSABLE);
}

void lostio_add_device(struct device *device)
{
    char fsname[100];
    fsname[0] = '/';
    strcpy(fsname + 1, device->driver->name);
    strcat(fsname, "/");
    itoa(device->dev.number, fsname + strlen(fsname), 10);
    vfstree_create_node(fsname, LOSTIO_TYPES_DIRECTORY, 0, NULL, 
                        LOSTIO_FLAG_BROWSABLE);

    strcat(fsname, "/");
    char *fsname_suffix = fsname + strlen(fsname);
    strcpy(fsname_suffix, "ip");
    vfstree_create_node(fsname, LOSTIO_TYPES_NETCONFIG, 1, device, 0);
    strcpy(fsname_suffix, "mac");
    vfstree_create_node(fsname, LOSTIO_TYPES_NETCONFIG, 2, device, 0);
    strcpy(fsname_suffix, "dhcp");
    vfstree_create_node(fsname, LOSTIO_TYPES_DHCP, 0, device, 0);
}

bool lostio_tcp_not_found(char** path, uint8_t flags, pid_t pid,
    FILE* ps)
{
    DEBUG_MSG("Knoten anlegen");

    if (strncmp(*path, "/dns/", 5) == 0) {
        // DNS Request absetzen
        struct dns_request_result* result = dns_request(strrchr(*path, '/') + 1);

        // Keine IP-Adresse erhalten?
        if ((result == NULL) || (result->ip_count == 0)) {
            return false;
        }

        char* data = calloc(16 * result->ip_count,sizeof(char));

        int i;
        for(i = 0; i < result->ip_count; i++) {
            char* ip = ip_to_string(result->ip_address[i]);
            DEBUG_MSG2("IP =  %s , len = %d\n",ip,strlen(ip));

            strcat(data, ip);
            if((i + 1) < result->ip_count) {
                strcat(data, "\n");
            }

            free(ip);
        }

        // TODO: Data wird nicht gefreet. (vorerst!)
        return vfstree_create_node(*path, LOSTIO_TYPES_RAMFILE,
            16 * result->ip_count, data, 0);
    } else {
        return vfstree_create_node(*path, LOSTIO_TYPES_TCPPORT, 0, NULL, 0);
    }
}

bool lostio_tcp_pre_open(char** path, uint8_t flags, pid_t pid,
    FILE* ps)
{
    DEBUG_MSG("pre_open");
    
    vfstree_node_t* node = vfstree_get_node_by_path(*path);
    if (node == NULL) {
        DEBUG_MSG1("Den Knoten %s gibt es nicht", *path);
        return false;
    }

    char* delim = index(*path, ':');
    if (delim == NULL) {
        DEBUG_MSG("Doppelpunkt vermisst");
        return false;
    }

    *delim = '\0';
    uint32_t ip = string_to_ip(*path + 1);
    if (!ip) {
        // DNS Request absetzen
        struct dns_request_result* result = dns_request(*path + 1);
        if (result && (result->ip_count > 0)) {
            ip = result->ip_address[0];
        }
    }

    if (!ip) {
        DEBUG_MSG("Konnte Adresse nicht aufloesen");
        return false;
    }

    *delim = ':';

    char* port_string = delim + 1;
    uint32_t port = atoi(port_string);

    if (port & ~0xFFFF) {
        DEBUG_MSG("Port zu gross");
        return false;
    }

    // Beim �ffnen der Verbindung darf der Task nicht blockiert sein, weil
    // sonst das Antwortpaket nicht eintreffen kann

    DEBUG_MSG2("Oeffne Verbindung zu %08x:%d", ip, port);
    struct tcp_connection* conn = tcp_open_connection(ip, port);
    if (conn == NULL) {
        DEBUG_MSG("Konnte Verbindung nicht herstellen");
        return false;
    }

    // FIXME Das sollte eigentlich in den Filehandle, aber auf den haben wir
    // hier keinen Zugriff. Solange es nur eine Verbindung mit dieser IP-Port-
    // Kombination geht, m�sste es auch so funktionieren...
    node->data = conn;
    node->size++;

    DEBUG_MSG("Verbindung hergestellt");
    return true;
}

static size_t lostio_tcp_read_packet(struct tcp_connection* conn, void* buf,
    size_t size)
{
    struct tcp_in_buffer* buffer;

    do {
        buffer = list_get_element_at(conn->to_lostio, 0);

        if (buffer == NULL) {
            return 0;
        }

        if (buffer->seq == buffer->size) {
            list_pop(conn->to_lostio);
            free(buffer->data);
            free(buffer);
            buffer = NULL;
        }

    } while(!buffer || (buffer->seq == buffer->size));

    if (buffer->size - buffer->seq <= size) {
        size = buffer->size - buffer->seq;
    }

    memcpy(buf, buffer->data + buffer->seq, size);

    buffer->seq += size;

    return size;
}

size_t lostio_tcp_read(lostio_filehandle_t* fh, void* _buf,
    size_t blocksize, size_t count)
{
    struct tcp_connection* conn = fh->node->data;
    size_t size = blocksize * count;
    size_t read, ret;
    uint8_t* buf = _buf;

    p();

    if ((conn->status == TCPS_CLOSED) && (list_is_empty(conn->to_lostio))) {
        fh->flags |= LOSTIO_FLAG_EOF;
    }

    read = 0;
    while (read < size) {
        ret = lostio_tcp_read_packet(conn, &buf[read], size - read);
        if (ret == 0) {
            break;
        }

        read += ret;
    }


    v();

    return read;
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

    // Zu schreibende Teile auf passende Paketgroessen zusammenhacken und in
    // den ausgehenden Puffer speichern
    struct tcp_out_buffer* out_buffer;
    size_t size = blocksize * count;
    size_t mss = tcp_get_mss(conn);
    while (size > 0) {
        size_t packet_size = size > mss ? mss : size;

        out_buffer = malloc(sizeof(*out_buffer));
        out_buffer->data = data;
        out_buffer->size = packet_size;

        list_insert(conn->out_buffer, list_size(conn->out_buffer), out_buffer);

        data += packet_size;
        size -= packet_size;
    }

    // Ausgehenden Puffer senden
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
    struct tcp_connection* conn = filehandle->node->data;
    struct tcp_in_buffer* buffer;

    p();

    while ((buffer = list_pop(conn->to_lostio))) {
        free(buffer->data);
        free(buffer);
    }
    v();

    tcp_close_connection(conn);
    /*
     * TODO Reaktivieren, sobald die Verbindungen nicht mehr hartkodiert sind
    if (--(filehandle->node->size) == 0) {
        vfstree_delete_node(filehandle->node->name);
    }
    */

    return 0;
}

size_t lostio_netconfig_read(lostio_filehandle_t *handle, void *buf, size_t blocksize, size_t count)
{
    struct device *device = (struct device *) handle->node->data;

    char *data = NULL;
    // <treibername>/<devicenummer>/ip
    if (handle->node->size == 1)
        data = ip_to_string(device->ip);
    // <treibername>/<devicenummer>/mac
    else if (handle->node->size == 2)
        data = mac_to_string(device->dev.mac);
    size_t size = strlen(data);

    if (handle->pos >= size) {
        handle->flags |= LOSTIO_FLAG_EOF;
        return 0;
    }

    size_t readsize = blocksize * count;
    if ((readsize + handle->pos) > size)
        readsize = size - handle->pos;
    memcpy(buf, data + handle->pos, readsize);
    handle->pos += readsize;
    if (handle->pos == size)
        handle->flags |= LOSTIO_FLAG_EOF;
    return readsize;
}
size_t lostio_netconfig_write(lostio_filehandle_t *handle, size_t blocksize, size_t count,void *data)
{
    if (blocksize * count < 7)
        return 0;

    struct device *device = (struct device *) handle->node->data;
    if (handle->node->size == 1)
    {
        device->ip = string_to_ip(data);
        return blocksize * count;
    }
    else if (handle->node->size == 2)
        // TODO FIXME
        printf("TODO: lostio_netconfig_write auf mac\n");
    return 0;
}

size_t lostio_route_read(lostio_filehandle_t *handle, void *buf, size_t blocksize, size_t count)
{
    if ((handle->flags & IO_OPEN_MODE_TRUNC) != 0) {
        handle->flags |= LOSTIO_FLAG_EOF;
        return 0;
    }

    char* buffer = strdup("");
    char* old_buffer;

    size_t i = 0;
    size_t entry_count = routing_get_entry_count();
    for (;i < entry_count;i++)
    {
        struct routing_entry* entry = enum_routing_entry(i);
        char *target_ip = ip_to_string(entry->target);
        char *subnet_mask = ip_to_string(entry->subnet);
        char *gateway_ip = ip_to_string(entry->gateway);

        old_buffer = buffer;
        asprintf(&buffer, "%s%s %s %s\n",
            old_buffer, target_ip, gateway_ip, subnet_mask);
        free(old_buffer);

        free(target_ip);
        free(subnet_mask);
        free(gateway_ip);
    }
    size_t size = strlen(buffer);

    if (handle->pos >= size) {
        handle->flags |= LOSTIO_FLAG_EOF;
        free(buffer);
        return 0;
    }
    
    size_t readsize = blocksize * count;
    if ((readsize + handle->pos) > size)
        readsize = size - handle->pos;
    memcpy(buf, buffer + handle->pos, readsize);
    handle->pos += readsize;
    if (handle->pos == size)
        handle->flags |= LOSTIO_FLAG_EOF;

    free(buffer);
    return readsize;
}

size_t lostio_route_write(lostio_filehandle_t *handle, size_t blocksize, size_t count,void *data)
{
    size_t size = blocksize * count;
    handle->data = realloc(handle->data, handle->pos + size + 1);
    memcpy(handle->data + handle->pos, data, size);
    handle->pos += size;
    return size;
}

int lostio_route_close(lostio_filehandle_t *handle)
{
    if (handle->data != NULL)
    {
        * ( (char *) handle->data + handle->pos) = '\0';
        // FIXME: Kann momentan nur den ersten eintrag in der routing
        //        tabelle ändern
        uint32_t ips[3];
        bool failed = false;
        char *cur = strtok(handle->data, " ");
        size_t i = 0;
        for (;i < 3;i++)
        {
            if (cur == NULL)
            {
                failed = true;
                break;
            }
            ips[i] = string_to_ip(cur);
            cur = strtok(NULL, " ");
        }

        if (failed == false)
        {
            struct routing_entry *entry = enum_routing_entry(0);

            if (entry == NULL) {
                entry = calloc(1, sizeof(*entry));
                add_routing_entry(entry);
            }

            entry->target = ips[0];
            entry->gateway = ips[1];
            entry->subnet = ips[2];
        }
    }
    return 0;
}
