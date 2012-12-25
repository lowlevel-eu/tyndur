/*
 * Copyright (c) 2007-2009 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <stdint.h>
#include <stdlib.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include <services.h>

#include <init.h>
#include <rpc.h>
#include <pci.h>
#include <dir.h>

#include "cdi.h"
#include "cdi/lists.h"
#include "cdi/pci.h"

pid_t pci_pid;

/*
 * Wartet, bis der PCI-Service auftaucht
 */
static void wait_for_pci_service(void)
{
    static bool pci_started = false;

    if (!pci_started) {
        servmgr_need("pci");
        pci_started = true;
    }

    pci_pid = init_service_get("pci");
}

/**
 * Liest die Daten eines PCI-Geraets ein
 */
static struct cdi_pci_device* read_pci_device(uint8_t* data, size_t size)
{
    struct pci_device* device = (struct pci_device*) data;

    // CDI-Struktur initialisieren
    struct cdi_pci_device* cdi_device = malloc(sizeof(*cdi_device));
    cdi_device->bus          = device->bus;
    cdi_device->dev          = device->device;
    cdi_device->function     = device->function;
    cdi_device->vendor_id    = device->vendor_id;
    cdi_device->device_id    = device->device_id;
    cdi_device->class_id     = device->class_id;
    cdi_device->subclass_id  = device->subclass_id;
    cdi_device->interface_id = device->interface_id;
    cdi_device->rev_id       = device->rev_id;
    cdi_device->irq          = device->irq;
    cdi_device->resources    = cdi_list_create();

    // Ressourcen hinzufuegen
    struct pci_resource res;
    struct cdi_pci_resource* cdi_res;

    size -= sizeof(*device);
    data += sizeof(*device);

    while (size >= sizeof(res)) {
        memcpy(&res, data, sizeof(res));
        size -= sizeof(res);
        data += sizeof(res);

        cdi_res = malloc(sizeof(*cdi_res));
        cdi_res->start = res.start;
        cdi_res->length = res.length;
        cdi_res->index = res.index;
        cdi_res->address = 0;

        switch (res.type) {
            case PCI_RESOURCE_MEM:
                cdi_res->type = CDI_PCI_MEMORY;
                break;

            case PCI_RESOURCE_PORT:
                cdi_res->type = CDI_PCI_IOPORTS;
                break;

            case PCI_RESOURCE_IRQ:
                continue;

            default:
                *(int*)0x0 = 42;
                break;
        }

        cdi_list_push(cdi_device->resources, cdi_res);
    }

    return cdi_device;
}

/**
 * Gibt alle PCI-Geraete im System zurueck. Die Geraete werden dazu
 * in die uebergebene Liste eingefuegt.
 */
void cdi_pci_get_all_devices(cdi_list_t list)
{
    struct init_dev_desc* devs;
    uint8_t* data;
    int ret;

    wait_for_pci_service();

    ret = init_dev_list(&devs);
    if (ret < 0) {
        return;
    }

    data = (uint8_t*) devs;
    while (1) {
        size_t size;

        if (ret < sizeof(struct init_dev_desc)) {
            break;
        }

        devs = (struct init_dev_desc*) data;
        size = sizeof(struct init_dev_desc) + devs->bus_data_size;
        if (ret < size) {
            break;
        }

        if (devs->type == CDI_PCI) {
            cdi_list_push(list, read_pci_device(devs->bus_data,
                devs->bus_data_size));
        }
        ret -= size;
        data += size;
    };
}

/**
 * Gibt die Information zu einem PCI-Geraet frei
 */
void cdi_pci_device_destroy(struct cdi_pci_device* device)
{
    // TODO Liste abbauen
    free(device);
}

/**
 * Reserviert die IO-Ports des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_ioports(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_IOPORTS) {
            request_ports(res->start, res->length);
        }
    }
}

/**
 * Gibt die IO-Ports des PCI-Geraets frei
 */
void cdi_pci_free_ioports(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_IOPORTS) {
            release_ports(res->start, res->length);
        }
    }
}
/**
 * Reserviert den MMIO-Speicher des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_memory(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_MEMORY) {
            res->address = mem_allocate_physical(res->length, res->start, 0);
        }
    }
}

/**
 * Gibt den MMIO-Speicher des PCI-Geraets frei
 */
void cdi_pci_free_memory(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_MEMORY) {
            mem_free_physical(res->address, res->length);
            res->address = 0;
        }
    }
}

static uint32_t cdi_pci_config_rw(struct cdi_pci_device* dev,
    enum pci_csa_type type, int reg, uint32_t val)
{
    struct pci_config_space_access csa = {
        .type       = type,
        .bus        = dev->bus,
        .device     = dev->dev,
        .function   = dev->function,
        .reg        = reg,
        .value      = val,
    };

    return rpc_get_dword(pci_pid, RPC_PCI_CONFIG_SPACE_ACCESS, sizeof(csa),
        (char*) &csa);
}

uint8_t cdi_pci_config_readb(struct cdi_pci_device* device, uint8_t offset)
{
    return cdi_pci_config_rw(device, PCI_CSA_READ8, offset, 0);
}

uint16_t cdi_pci_config_readw(struct cdi_pci_device* device, uint8_t offset)
{
    return cdi_pci_config_rw(device, PCI_CSA_READ16, offset, 0);
}

uint32_t cdi_pci_config_readl(struct cdi_pci_device* device, uint8_t offset)
{
    return cdi_pci_config_rw(device, PCI_CSA_READ32, offset, 0);
}

void cdi_pci_config_writeb(struct cdi_pci_device* device, uint8_t offset,
    uint8_t value)
{
    cdi_pci_config_rw(device, PCI_CSA_WRITE8, offset, value);
}

void cdi_pci_config_writew(struct cdi_pci_device* device, uint8_t offset,
    uint16_t value)
{
    cdi_pci_config_rw(device, PCI_CSA_WRITE16, offset, value);
}

void cdi_pci_config_writel(struct cdi_pci_device* device, uint8_t offset,
    uint32_t value)
{
    cdi_pci_config_rw(device, PCI_CSA_WRITE32, offset, value);
}
