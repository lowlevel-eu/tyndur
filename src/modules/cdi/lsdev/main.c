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

#include <init.h>
#include <stdio.h>
#include <cdi.h>
#include <pci.h>

int main(void)
{
    struct init_dev_desc* devs;
    uint8_t* data;
    int ret;

    ret = init_dev_list(&devs);
    if (ret < 0) {
        fprintf(stderr, "Konnte Geraete nicht abfragen\n");
        return 1;
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

        switch (devs->type) {
            case CDI_PCI:
            {
                struct pci_device* device =
                    (struct pci_device*) devs->bus_data;

                printf("PCI  %04x:%04x  %s\n",
                    device->vendor_id, device->device_id,
                    devs->name);
                break;
            }

            default:
                printf("???             %s\n",
                    devs->name);
                break;
        }
        ret -= size;
        data += size;
    };

    return 0;
}
