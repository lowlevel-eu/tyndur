/*
 * Copyright (c) 2007-2009 The tyndur Project. All rights reserved.
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

#ifndef _PCI_HW_
#define _PCI_HW_

#include <stdint.h>
#include <stdbool.h>

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_VENDOR_ID   0x00
#define PCI_DEVICE_ID   0x02
#define PCI_COMMAND     0x04
#define PCI_STATUS      0x06
#define PCI_REVISION    0x08
#define PCI_CLASS       0x0B
#define PCI_SUBCLASS    0x0A
#define PCI_INTERFACE   0x09
#define PCI_HEADERTYPE  0x0E
#define PCI_BAR0        0x10
#define PCI_INTERRUPT   0x3C


struct bar {
    void* addr;
    size_t size;
    enum { bar_mem = 0, bar_io = 1} type;
};

void pci_request_io_ports(void);

void scanpci(void);
char* pciinfo(void);

void pci_request_io_ports(void);
struct bar* pci_get_bar(uint32_t bus, uint32_t device, uint32_t func,
    uint32_t bar_number);
struct pci_device pci_get_device(uint32_t bus, uint32_t device, uint32_t func);

/**
 * Prueft ob ein PCI-Geraet Funktionen unterstuetzt.
 *
 * @param bus Bus-Nummer des Geräts
 * @param device Gerätenummer des Geräts
 */
bool pci_has_functions(uint32_t bus, uint32_t device);

size_t pci_device_size(struct pci_device* device);

extern list_t* pci_devices;

#endif
