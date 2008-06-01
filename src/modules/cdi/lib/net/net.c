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
#include <collections.h> 
#include <network.h>

#include "cdi/net.h"
#include "arp.h"
#include "ip.h"
#include "ethernet.h"

static struct cdi_net_device* the_one_and_only_netcard;

/**
 * Initialisiert die Datenstrukturen fuer einen Netzerktreiber
 */
void cdi_net_driver_init(struct cdi_net_driver* driver)
{
    cdi_driver_init((struct cdi_driver*) driver);
    
    cdi_net_arp_init();
    cdi_net_ip_init();
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
    the_one_and_only_netcard = device;
    if (device->ip == 0) {
        // FIXME
        device->ip = 0x4201a8c0;
    }
    register_netcard(0, device->ip);
}

/**
 * Wird von Netzwerktreibern aufgerufen, wenn ein Netzwerkpaket
 * empfangen wurde.
 */
void cdi_net_receive(
    struct cdi_net_device* device, uint8_t* buffer, size_t size)
{
    if (size < sizeof(struct eth_packet_header)) {
        return;
    }
    
    struct eth_packet_header* eth = (struct eth_packet_header*) buffer;
    switch (eth->type) {
        case ETH_TYPE_IP:
            cdi_net_ip_receive(device, buffer, size);
            break;

        case ETH_TYPE_ARP:
            cdi_net_arp_receive(device, buffer, size);
            break;
    }
}

/**
 * Gibt die Netzwerkkarte mit der uebergebenen Geraetenummer
 * zureck
 */
struct cdi_net_device* cdi_net_get_device(int num) 
{
    return the_one_and_only_netcard;
}
