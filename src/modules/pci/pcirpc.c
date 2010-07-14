/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Max Reitz.
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

#include <stdint.h>

#include <rpc.h>
#include <syscall.h>

#include "pci.h"
#include "pcihw.h"


void pci_config_write_dword(uint32_t bus, uint32_t device, uint32_t func,
    uint32_t reg, uint32_t value);
void pci_config_write_word(uint32_t bus, uint32_t device, uint32_t func,
    uint32_t reg, uint16_t value);
void pci_config_write_byte(uint32_t bus, uint32_t device, uint32_t func,
    uint32_t reg, uint8_t value);
uint32_t pci_config_read(uint32_t bus, uint32_t device, uint32_t func,
    uint32_t reg);


static void config_space_handler(pid_t src, uint32_t corr_id, size_t length,
    void* data)
{
    struct pci_config_space_access* csa = data;

    if (length < sizeof(*csa)) {
        rpc_send_dword_response(src, corr_id, 0);
        return;
    }

    uint32_t value = csa->value;

    switch (csa->type) {
        case PCI_CSA_READ32:
            value = pci_config_read(csa->bus, csa->device, csa->function,
                csa->reg);
            rpc_send_dword_response(src, corr_id, value);
            break;
        case PCI_CSA_READ16:
            value = pci_config_read(csa->bus, csa->device, csa->function,
                csa->reg);
            rpc_send_dword_response(src, corr_id, value & 0xFFFF);
            break;
        case PCI_CSA_READ8:
            value = pci_config_read(csa->bus, csa->device, csa->function,
                csa->reg);
            rpc_send_dword_response(src, corr_id, value & 0xFF);
            break;
        case PCI_CSA_WRITE32:
            pci_config_write_dword(csa->bus, csa->device, csa->function,
                csa->reg, value);
            rpc_send_dword_response(src, corr_id, 0);
            break;
        case PCI_CSA_WRITE16:
            pci_config_write_word(csa->bus, csa->device, csa->function,
                csa->reg, value);
            rpc_send_dword_response(src, corr_id, 0);
            break;
        case PCI_CSA_WRITE8:
            pci_config_write_byte(csa->bus, csa->device, csa->function,
                csa->reg, value);
            rpc_send_dword_response(src, corr_id, 0);
            break;
        default:
            rpc_send_dword_response(src, corr_id, 0);
    }
}

void init_rpc_interface(void)
{
    register_message_handler(RPC_PCI_CONFIG_SPACE_ACCESS,
        &config_space_handler);
}
