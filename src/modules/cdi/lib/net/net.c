/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <stdint.h>
#include <string.h>
#include <collections.h> 
#include <network.h>

#include "cdi/lists.h"
#include "cdi/net.h"
#include "syscall.h"
#include "init.h"
#include "rpc.h"

static uint32_t netcard_highest_id = 0;
static cdi_list_t netcard_list;
// FIXME: Sollten wahrscheinlich pro device sein und nicht nur pro driver
static list_t* ethernet_packet_receivers;

static void rpc_send_packet
    (pid_t pid, uint32_t cid, size_t data_size, void* data);
static void rpc_register_receiver
    (pid_t pid, uint32_t cid, size_t data_size, void* data);

/**
 * Initialisiert die Datenstrukturen fuer einen Netzerktreiber
 */
void cdi_net_driver_init(struct cdi_net_driver* driver)
{
    driver->drv.type = CDI_NETWORK;
    cdi_driver_init((struct cdi_driver*) driver);
    
    netcard_list = cdi_list_create();
    ethernet_packet_receivers = list_create();

    register_message_handler(RPC_ETHERNET_SEND_PACKET, rpc_send_packet);
    register_message_handler(RPC_ETHERNET_REGISTER_RECEIVER, rpc_register_receiver);
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Netzwerktreiber
 */
void cdi_net_driver_destroy(struct cdi_net_driver* driver)
{
    cdi_driver_destroy((struct cdi_driver*) driver);
}

/**
 * Initialisiert eine neue Netzwerkkarte
 */
void cdi_net_device_init(struct cdi_net_device* device)
{
    /*
     * Hier ist device->driver noch nicht gültig. Ein RPC an dieser Stelle
     * wäre zu früh.
     */
    device->number = netcard_highest_id;
    ++netcard_highest_id;
}

void cdi_tyndur_net_device_init(struct cdi_net_device* device)
{
    // Zur Liste der Netzwerkkarten hinzufügen
    cdi_list_push(netcard_list, device);

    // Beim tcpip Modul registrieren
    register_netcard(device->number,
                     device->mac);
}

/**
 * Wird von Netzwerktreibern aufgerufen, wenn ein Netzwerkpaket
 * empfangen wurde.
 */
void cdi_net_receive(
    struct cdi_net_device* device, void* buffer, size_t size)
{
    // Packet an das tcpip Modul senden
    size_t packet_size = 8 + sizeof(uint32_t) + size;
    char packet[packet_size];

    memcpy(packet, RPC_ETHERNET_RECEIVE_PACKET, 8);
    memcpy(packet + 8, &device->number, sizeof(uint32_t));
    memcpy(packet + 8 + sizeof(uint32_t), buffer, size);
    
    int i = 0;
    pid_t pid;
    for (; (pid = (pid_t) list_get_element_at(ethernet_packet_receivers, i)); i++)
    {
        send_message(pid, 512, 0, packet_size, packet);
    }
}

/**
 * Gibt die Netzwerkkarte mit der uebergebenen Geraetenummer
 * zureck
 */
struct cdi_net_device* cdi_net_get_device(int num) 
{
    int i = 0;
    for (;i < cdi_list_size(netcard_list);i++)
    {
        struct cdi_net_device *device = cdi_list_get(netcard_list, i);
        if (device->number == num)
            return device;
    }
    return NULL;
}

static void rpc_send_packet(pid_t pid, uint32_t cid, size_t data_size,
    void* data)
{
    uint32_t device_number = * (uint32_t *)data;
    void* packet = data + sizeof(uint32_t);
    size_t packet_size = data_size - sizeof(uint32_t);

    struct cdi_net_device* dev = cdi_net_get_device(device_number);
    if (dev == NULL) {
        return;
    }

    p();
    struct cdi_net_driver* driver = (struct cdi_net_driver*) dev->dev.driver;
    driver->send_packet(dev, packet, packet_size);
    v();
}

static void rpc_register_receiver
    (pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    list_push(ethernet_packet_receivers, (void*) pid);
}
