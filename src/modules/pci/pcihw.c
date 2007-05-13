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
#include "ports.h"
#include "syscall.h"
#include "stdlib.h"

#include "pci.h"
#include "pcihw.h"

void pci_request_io_ports()
{
    request_ports(0xCF8, 0x8);
}

static dword pci_config_read(dword bus, dword device, dword func, dword reg)
{
    dword offset = reg % 0x04;
    reg &= ~0x3;

    outl(PCI_CONFIG_ADDR, 
        0x1 << 31
        | ((bus     & 0xFF) << 16) 
        | ((device  & 0x1F) << 11) 
        | ((func    & 0x07) <<  8) 
        | ((reg     & 0xFC)));

    return inl(PCI_CONFIG_DATA) >> (8 * offset);
}

static inline dword pci_config_read_dword(dword bus, dword device, dword func, dword reg)
{
    return pci_config_read(bus, device, func, reg);
}

static inline word pci_config_read_word(dword bus, dword device, dword func, dword reg)
{
    return pci_config_read(bus, device, func, reg) & 0xffff;
}

static inline byte pci_config_read_byte(dword bus, dword device, dword func, dword reg)
{
    return pci_config_read(bus, device, func, reg) & 0xff;
}

static void pci_config_write_dword(dword bus, dword device, dword func, dword reg, dword value)
{
    reg &= ~0x3;

    outl(PCI_CONFIG_ADDR, 
        0x1 << 31
        | ((bus     & 0xFF) << 16) 
        | ((device  & 0x1F) << 11) 
        | ((func    & 0x07) <<  8) 
        | ((reg     & 0xFC)));

    outl(PCI_CONFIG_DATA, value);
}


struct bar* pci_get_bar(dword bus, dword device, dword func, dword bar_number)
{
    struct bar* bar = malloc(sizeof(struct bar));
    dword bar_value;
    dword headertype = pci_config_read_dword(bus, device, func, PCI_HEADERTYPE);

    if (!bar || headertype & 0x3F) {
        return NULL;
    }

    bar_value = pci_config_read_dword(bus, device, func, PCI_BAR0 + (4*bar_number));
    bar->type = bar_value & 1;
    bar->addr = (void*) (bar_value & ~0xF);

    pci_config_write_dword(bus, device, func, PCI_BAR0 + (4*bar_number), 0xFFFFFFF0 | bar->type);
    bar->size = pci_config_read_dword(bus, device, func, PCI_BAR0 + (4*bar_number));
    bar->size = (~bar->size | 0xF) + 1;
    
    pci_config_write_dword(bus, device, func, PCI_BAR0 + (4*bar_number), bar_value);

    return bar;
}

struct pci_device pci_get_device(dword bus, dword device, dword func)
{
    struct pci_device pci_device = {
        .bus        = bus,
        .device     = device,
        .function   = func,

        .vendor_id  = pci_config_read_word(bus, device, func, PCI_VENDOR_ID),
        .device_id  = pci_config_read_word(bus, device, func, PCI_DEVICE_ID),
        .class_id   = pci_config_read_dword(bus, device, func, PCI_CLASS),
        .rev_id     = pci_config_read_dword(bus, device, func, PCI_REVISION),
        .irq        = pci_config_read_byte(bus, device, func, PCI_INTERRUPT),

        .resources  = NULL
    };

    return pci_device;
}
