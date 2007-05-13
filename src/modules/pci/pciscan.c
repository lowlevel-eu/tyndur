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
#include "stdlib.h"
#include "ports.h"
#include "stdio.h"
#include "collections.h"

#include "pci.h"
#include "pcihw.h"

list_t* pci_devices;

void add_device(dword bus, dword device, dword func)
{
    struct pci_device pci_device = pci_get_device(bus, device, func);        
    //dword header_type = pci_config_read_dword(bus, device, func, PCI_HEADERTYPE);

    // Nicht vorhandene Geräte haben als vendor_id entweder 0x0 oder 0xffff
    if ((pci_device.vendor_id != 0xffff) && pci_device.vendor_id) {
        struct pci_device* element = malloc(sizeof(struct pci_device));       
        *element = pci_device;
    
        int bar_number;
        for (bar_number = 5; bar_number >= 0; bar_number--) {
            struct bar* bar = pci_get_bar(bus, device, func, bar_number);
            if (!bar || !bar->addr) continue;

            struct pci_resource* resource = 
                malloc(sizeof(struct pci_resource));

            switch(bar->type) {
                case bar_mem:
                    resource->type = PCI_RESOURCE_MEM;
                    break;
                
                case bar_io:
                    resource->type = PCI_RESOURCE_PORT;
                    break;
                    
                default:
                    printf("PCI: Unbekannter Ressourcentyp %d\n", bar->type);
                    free(resource);
                    continue;
            }

            resource->start = (dword) bar->addr;
            resource->length = bar->size;

            if (element->resources == NULL) {
                element->resources = list_create();
            }
            list_push(element->resources, resource);
        }
        
        if (pci_device.irq != 0) {
            struct pci_resource* resource = 
                malloc(sizeof(struct pci_resource));
            
            resource->type = PCI_RESOURCE_IRQ;
            resource->start = pci_device.irq;
            resource->length = 0;            

            if (element->resources == NULL) {
                element->resources = list_create();
            }
            list_push(element->resources, resource);
        }
    
        list_push(pci_devices, element);
    }

}

void scanpci()
{
    int bus;
    int device;
    int func;

    if (pci_devices != NULL) {
        return;
    }

    pci_devices = list_create();

    for (bus = 7; bus >= 0; bus--) {
        for (device = 31; device >= 0; device--) {
            for (func = 7; func >= 0; func--) {
                add_device(bus, device, func);
            }
        }
    }
}

static char* pci_res_info(struct pci_resource* res)
{
    char* buffer = NULL;

    switch (res->type) {
        case PCI_RESOURCE_MEM:
            asprintf(
                &buffer,
                "    Speicher an Adresse 0x%x, Groesse = %d Bytes\n", 
                res->start,
                res->length
            );
            break;

        case PCI_RESOURCE_PORT:
            asprintf(
                &buffer,
                "    IO-Ports ab 0x%x, Anzahl = %d\n", 
                res->start,
                res->length
            );
            break;

        case PCI_RESOURCE_IRQ:
            asprintf(
                &buffer,
                "    IRQ %d\n", 
                res->start
            );
            break;
    }

    return buffer;
}

char* pciinfo() 
{
    struct pci_device* dev;
    struct pci_resource* res;
    char* buffer = NULL;
    char* old_buffer;
    char* resinfo;
    dword i, j;

    for(i = 0; (dev = list_get_element_at(pci_devices, i)); i++) {
        old_buffer = buffer;
        asprintf(
            &buffer, 
            "%s%x:%x:%x -- Hersteller %x, Geraete-ID %x [Klasse %x]\n",
            old_buffer ? old_buffer : "",
            dev->bus,
            dev->device,
            dev->function,
            dev->vendor_id,
            dev->device_id,
            dev->class_id
        );
        free(old_buffer);

        for (j = 0; (res = list_get_element_at(dev->resources, j)); j++) {
            old_buffer = buffer;
            resinfo = pci_res_info(res);
            asprintf(
                &buffer,
                "%s%s",
                old_buffer,
                resinfo
            );
            free(old_buffer);
            free(resinfo);
        }
    }

    return buffer;
}
