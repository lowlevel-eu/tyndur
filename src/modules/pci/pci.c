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

#include <stdbool.h>
#include <stdint.h>
#include <init.h>
#include <syscall.h>
#include <string.h>
#include "stdio.h"
#include "types.h"
#include "ports.h"
#include "stdlib.h"
#include "io.h"
#include "collections.h"
#include "lostio.h"

#include "pci.h"
#include "pcihw.h"



#define LOSTIO_TYPES_PCI_DEVICE 255

void create_device_files(void);
bool pre_open_handler(char** path, uint8_t args, pid_t pid,
    io_resource_t* pipe_source);
int close_handler(lostio_filehandle_t* handle);
void init_rpc_interface(void);

int main(int argc, char* argv[])
{

    pci_request_io_ports();

    lostio_init();

    lostio_type_ramfile_use();
    lostio_type_directory_use();

    lostio_type_ramfile_use_as(LOSTIO_TYPES_PCI_DEVICE);
//    typehandle_t* pci_device_typehandle = 
//        get_typehandle(LOSTIO_TYPES_PCI_DEVICE);

//    pci_device_typehandle->pre_open = &pre_open_handler;
//    pci_device_typehandle->close    = &close_handler;



    scanpci();


    // Nun wird das Ramfile erstellt
    char* pciinfo_buffer = pciinfo();
/*    vfstree_node_t* root_node = vfstree_get_node_by_path("/");
    root_node->type = LOSTIO_TYPES_RAMFILE;
    root_node->data = (void*) pciinfo_buffer;
    root_node->size = strlen(pciinfo_buffer) + 1;*/

    vfstree_create_node("/info", LOSTIO_TYPES_RAMFILE, 
        strlen(pciinfo_buffer) + 1, (void*) pciinfo_buffer, 0);

    create_device_files();

    init_rpc_interface();

    init_service_register("pci");
    
    while(1) {
        wait_for_rpc();
    }

    return 0;
}

void register_pci_device(void* data, size_t size)
{
    struct pci_device* device = data;
    const char* class;

    struct {
        struct init_dev_desc desc;
        uint8_t bus_data[size];
    } info;

    switch (device->class_id) {
        case 0x00:
            if (device->subclass_id == 0x1) {
                class = "vga";
            } else {
                class = "legacy";
            }
            break;

        case 0x01:
            class = "storage";
            break;

        case 0x02:
            class = "net";
            break;

        case 0x03:
            class = "display";
            break;

        case 0x04:
            switch (device->subclass_id) {
                case 0x0:   class = "video"; break;
                case 0x1:   class = "audio"; break;
                default:    class = "multimedia"; break;
            }
            break;

        case 0x06:
            class = "bridge";
            break;

        default:
            class = "device";
            break;
    }

    snprintf(info.desc.name, sizeof(info.desc.name), "pci_%s_%d.%d.%d",
        class, device->bus, device->device, device->function);

    info.desc.type = 10; /* CDI_PCI */
    info.desc.bus_data_size = size;

    memcpy(info.bus_data, data, size);
    init_dev_register(&info.desc, sizeof(info.desc) + size);
}

void create_device_files(void)
{
    int i, j; 
    struct pci_device* device;
    struct pci_resource* res;
        
    vfstree_create_node("/devices", LOSTIO_TYPES_DIRECTORY, 0, NULL, 
        LOSTIO_FLAG_BROWSABLE);

    for (i = 0; (device = list_get_element_at(pci_devices, i)); i++)
    {
        char* filename;
        size_t size;
        uint32_t pos;
        void* content;

        asprintf(&filename, "/devices/%x:%x", 
            device->vendor_id, device->device_id);

        size = pci_device_size(device);
        content = malloc(size);
        memcpy(content, device, sizeof(struct pci_device));
        
        pos = sizeof(struct pci_device);
        for (j = 0; (res = list_get_element_at(device->resources, j)); j++)
        {
            memcpy(content + pos, res, sizeof(struct pci_resource));
            pos += sizeof(struct pci_resource);
        }

        register_pci_device(content, size);
        vfstree_create_node(filename, LOSTIO_TYPES_PCI_DEVICE, 
            size, content, 0);

        free(filename);
    }
}

/*
bool pre_open_handler(char** path, byte args, pid_t pid, 
    io_resource_t* pipe_source)
{
    vfstree_node_t* node = vfstree_get_node_by_path(*path);

    if (!node) {
        return FALSE;
    }

    
    
    return TRUE;
}

int close_handler(lostio_filehandle_t* handle)
{
    return -1;
}
*/
