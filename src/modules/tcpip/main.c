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
#include <init.h>
#include <string.h>
#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#include "network.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "tcp.h"
#include "lostio_if.h"
#include "dns.h"
#include "lostio.h"

#include "main.h"

static list_t *net_drivers;

struct module_options options = {
    .gateway = 0x0202000a,
    .nameserver = 0xdede43d0,
    .ip = 0x0e02000a,
    .dhcp = false,
};

void init_routing(struct device *device);
void init_ethernet_receive(pid_t driver);
void rpc_register_driver(pid_t pid, uint32_t cid, size_t data_size, void* data);
void process_parameter(struct module_options* options, char* param);


int main(int argc, char* argv[])
{
    net_drivers = list_create();

    int i;
    for (i = 1; i < argc; i++) {
        process_parameter(&options, argv[i]);
    }

    init_ethernet();
    init_arp();
    init_ip();
    init_tcp();
    init_lostio_interface();

    register_message_handler(RPC_IP_REGISTER_DRIVER, rpc_register_driver);
    
    init_service_register("tcpip");

    while(1) {
        wait_for_rpc();
    }
}

struct driver *get_driver(pid_t pid)
{
    int i = 0;
    for (;i < list_size(net_drivers);i++)
    {
        struct driver *driver = list_get_element_at(net_drivers, i);
        if (driver->pid == pid)
            return driver;
    }
    return NULL;
}

struct driver *get_driver_by_name(const char *name)
{
    int i = 0;
    for (;i < list_size(net_drivers);i++)
    {
        struct driver *driver = list_get_element_at(net_drivers, i);
        if (strcmp(driver->name, name) == 0)
            return driver;
    }
    return NULL;
}

struct device *get_device(struct driver *driver, uint32_t device_number)
{
    int i = 0;
    for (;i < list_size(driver->devices);i++)
    {
        struct device *device = list_get_element_at(driver->devices, i);
        if (device->dev.number == device_number)
            return device;
    }
    return NULL;
}

struct device *get_device_by_pid(pid_t pid, uint32_t device_number)
{
    struct driver *driver = get_driver(pid);
    if (driver == NULL)return NULL;
    return get_device(driver, device_number);
}

void rpc_register_driver(pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    if (data_size != sizeof(struct net_device)) {
        return;
    }

    bool created = false;
    struct driver *driver = get_driver(pid);
    if (driver == NULL)
    {
        // Treiber hinzufügen in die Liste
        driver = malloc(sizeof(struct driver));
        driver->pid = pid;
        driver->devices = list_create();
        list_push(net_drivers, driver);
        created = true;

        // Namen des Treibers von init holen
        driver->name = init_service_get_name(driver->pid);
        lostio_add_driver(driver);
    }

    // Device in die Liste hinzufügen
    struct device *device = calloc(1, sizeof(struct device));
    memcpy(&device->dev, data, sizeof(struct net_device));
    device->driver = driver;
    device->ip = options.ip;
    list_push(driver->devices, device);

    init_routing(device);

    if (created == true)
    {
        // TODO: evtl. will man mal nur device-weit und nicht treiber-weit
        //       dann sollte man das hier anpassen
        init_ethernet_receive(pid);
    }

    if (options.dhcp) {
        printf("Konfiguriere %s/%d über DHCP...\n",
            driver->name, device->dev.number);
        dhcp_configure(device);
    }

    // VFS Ordner und Dateien für das Device erstellen
    lostio_add_device(device);
}

void init_routing(struct device *device)
{
    struct routing_entry* entry = malloc(sizeof(struct routing_entry));
    entry->target = 0;
    entry->subnet = 0;
    entry->gateway = options.gateway;
    entry->device = device;

    add_routing_entry(entry);
}

void init_ethernet_receive(pid_t driver)
{
    send_message(
        driver,
        512,
        0,
        8,
        RPC_ETHERNET_REGISTER_RECEIVER
    );
}

#if 0
void test_ping()
{
    dword ping_size = 3 * sizeof(dword);
    dword ping[3] = {
        0xFFF70008,
        0x00000000,
        0x00000000
    };

    ip_send(0x0202000a, IP_PROTO_ICMP, ping, ping_size);
}
#endif

void process_parameter(struct module_options* options, char* param)
{
    if (strncmp(param, "gw=", 3) == 0) {
        uint32_t gateway = string_to_ip(&param[3]);
        printf("Gateway: %08x\n", gateway);
        options->gateway = gateway;
    } else if (strncmp(param, "ns=", 3) == 0) {
        uint32_t nameserver = string_to_ip(&param[3]);
        printf("Nameserver: %08x\n", nameserver);
        options->nameserver = nameserver;
    } else if (strncmp(param, "ip=", 3) == 0) {
        uint32_t ip = string_to_ip(&param[3]);
        printf("IP-Adresse: %08x\n", ip);
        options->ip = ip;
    } else if (strncmp(param, "dhcp=", 5) == 0) {
        bool dhcp = atoi(&param[5]);
        printf("DHCP: %d\n", dhcp);
        options->dhcp = dhcp;
    } else {
        printf("Unbekannter Parameter %s\n", param);
    }
}
