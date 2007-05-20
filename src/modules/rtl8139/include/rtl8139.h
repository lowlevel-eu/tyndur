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

#ifndef _RTL8139_H_
#define _RTL8139_H_

#define REG_ID0 0x00
#define REG_ID4 0x04

#define REG_TRANSMIT_STATUS0 0x10
#define REG_TRANSMIT_ADDR0 0x20
#define REG_RECEIVE_BUFFER 0x30
#define REG_COMMAND 0x37
#define REG_CUR_READ_ADDR 0x38
#define REG_INTERRUPT_MASK 0x3C
#define REG_INTERRUPT_STATUS 0x3E
#define REG_TRANSMIT_CONFIGURATION 0x40
#define REG_RECEIVE_CONFIGURATION 0x44

#define CR_RESET                (1 << 4)
#define CR_RECEIVER_ENABLE      (1 << 3)
#define CR_TRANSMITTER_ENABLE   (1 << 2)
#define CR_BUFFER_IS_EMPTY      (1 << 0)

#define TCR_IFG_STANDARD        (3 << 24)
#define TCR_MXDMA_512           (5 << 8)
#define TCR_MXDMA_1024          (6 << 8)
#define TCR_MXDMA_2048          (7 << 8)

#define RCR_MXDMA_512           (5 << 8)
#define RCR_MXDMA_1024          (6 << 8)
#define RCR_MXDMA_UNLIMITED     (7 << 8)
#define RCR_ACCEPT_BROADCAST    (1 << 3)
#define RCR_ACCEPT_MULTICAST    (1 << 2)
#define RCR_ACCEPT_PHYS_MATCH   (1 << 1)

#define ISR_TRANSMIT_OK         (1 << 2)
#define ISR_RECEIVE_OK          (1 << 0)

struct netcard {
    byte    irq;
    void*   mem_base;
    dword   port_base;

    qword   mac : 48;
    dword   ip;

    dma_mem_ptr_t buffer;
    bool buffer_used;
    byte cur_buffer;

    dma_mem_ptr_t rx_buffer;
    dword rx_buffer_offset;

    list_t* receive_handler;
};

typedef void (*packet_handler_t)(struct netcard*, void*, dword);

void init_rtl8139(void);
bool send(struct netcard* netcard, void* data, dword size);

void register_receive_handler
    (struct netcard* netcard, dword type, packet_handler_t func);
packet_handler_t get_receive_handler(struct netcard* netcard, dword type);

extern struct netcard* the_one_and_only_netcard;

#endif
