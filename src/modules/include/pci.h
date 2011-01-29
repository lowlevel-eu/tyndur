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

#ifndef _PCI_H_
#define _PCI_H_

#include <stdint.h>
#include "collections.h"

#define PCI_RESOURCE_MEM 0
#define PCI_RESOURCE_PORT 1
#define PCI_RESOURCE_IRQ 2

struct pci_device {
    uint16_t    bus;
    uint16_t    device;
    uint16_t    function;

    uint16_t    vendor_id;
    uint16_t    device_id;
    uint8_t     rev_id;
    uint8_t     irq;

    uint8_t     interface_id;
    uint8_t     subclass_id;
    uint8_t     class_id;

    /// XXX: Fuer Abwaertskompatibilitaet eingefuehrt
    uint8_t    :8 ;

    list_t* resources;
} __attribute__ ((packed));

struct pci_resource {
    uint32_t   type;
    uint32_t   start;
    uint32_t   length;

    int        index;
} __attribute__ ((packed));


// Direkter Zugriff auf den PCI-Konfigurationsraum eines Geräts
// Es wird immer ein dword zurückgegeben (das bei Schreibzugriffen keine
// Bedeutung hat)
#define RPC_PCI_CONFIG_SPACE_ACCESS "PCICFGS"

// Typen des Zugriffs
enum pci_csa_type {
    PCI_CSA_READ32,
    PCI_CSA_READ16,
    PCI_CSA_READ8,
    PCI_CSA_WRITE32,
    PCI_CSA_WRITE16,
    PCI_CSA_WRITE8
};

// RPC-Request
struct pci_config_space_access {
    enum pci_csa_type type;
    // Adresse auf dem PCI-Bus
    uint16_t bus, device, function;
    // Offset im Konfigurationsraum
    uint32_t reg;
    // Nur notwendig, wenn geschrieben werden soll
    uint32_t value;
};

#endif
