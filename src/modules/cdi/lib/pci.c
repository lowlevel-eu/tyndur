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
#include <stdlib.h>
#include <syscall.h>
#include <stdio.h>

#include <pci.h>
#include <dir.h>

#include "cdi.h"
#include "cdi/lists.h"
#include "cdi/pci.h"

/**
 * Liest aus einer LOST-pci:/devices-Datei die Daten eines PCI-Geraets ein
 */
static struct cdi_pci_device* read_pci_device(char* filename) 
{
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "PCI-Geraet %s nicht gefunden.\n", filename);
        return NULL;
    }

    struct pci_device device;
    if (!fread(&device, sizeof(device), 1, f)) {
        fprintf(stderr, "Konnte nicht von %s lesen.\n", filename);
        fclose(f);
        return NULL;
    }

    // CDI-Struktur initialisieren
    struct cdi_pci_device* cdi_device = malloc(sizeof(*cdi_device));
    cdi_device->bus          = device.bus;
    cdi_device->dev          = device.device;
    cdi_device->function     = device.function;
    cdi_device->vendor_id    = device.vendor_id;
    cdi_device->device_id    = device.device_id;
    cdi_device->class_id     = device.class_id;
    cdi_device->irq          = device.irq;
    cdi_device->resources    = cdi_list_create();

    // Ressourcen hinzufuegen
    struct pci_resource res;
    struct cdi_pci_resource* cdi_res;
    while (!feof(f)) {
        if (fread(&res, sizeof(res), 1, f)) {
            
            cdi_res = malloc(sizeof(*cdi_res));
            cdi_res->start = res.start;
            cdi_res->length = res.length;

            switch (res.type) {
                case PCI_RESOURCE_MEM:
                    cdi_res->type = CDI_PCI_MEMORY;
                    break;
                
                case PCI_RESOURCE_PORT:
                    cdi_res->type = CDI_PCI_IOPORTS;
                    break;
            }

            cdi_list_push(cdi_device->resources, cdi_res);

        } else {
            fprintf(stderr, "Konnte nicht von %s lesen.\n", filename);
            fclose(f);
            cdi_pci_device_destroy(cdi_device);
            return NULL;
        }
    }

    fclose(f);

    return cdi_device;
}
    
/**
 * Gibt alle PCI-Geraete im System zurueck. Die Geraete werden dazu
 * in die uebergebene Liste eingefuegt.
 */
void cdi_pci_get_all_devices(cdi_list_t list)
{
    io_resource_t* dir_res = directory_open("pci:/devices/");
    
    if (dir_res == NULL) {
        fprintf(stderr, "Konnte pci:/devices/ nicht oeffnen\n");
        return;
    }

    io_direntry_t* direntry;
    while ((direntry = directory_read(dir_res))) {
        if (direntry->type == IO_DIRENTRY_FILE) {
            struct cdi_pci_device* device;
            char* buffer = NULL;
            asprintf(&buffer, "pci:/devices/%s", direntry->name);

            device = read_pci_device(buffer);
            if (device != NULL) {
                cdi_list_push(list, device);
            }

            free(buffer);
        }
        free(direntry);
    }

    directory_close(dir_res);
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
