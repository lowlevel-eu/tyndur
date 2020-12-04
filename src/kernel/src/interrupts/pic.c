/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
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

#include <stdint.h>

#include "kernel.h"
#include "mm.h"
#include "im.h"
#include "ports.h"

#define PIC_MASTER          0x20
#define PIC_MASTER_COMMAND  PIC_MASTER
#define PIC_MASTER_DATA     (PIC_MASTER + 1)

#define PIC_SLAVE           0xA0
#define PIC_SLAVE_COMMAND   PIC_SLAVE
#define PIC_SLAVE_DATA      (PIC_SLAVE + 1)

#define PIC_ICW1_ICW4       0x01
#define PIC_ICW1_INIT       0x10

#define PIC_ICW4_PC         0x01

#define PIC_EOI             0x20

/**
 * Den PIC initialisieren.
 */
void pic_init(void)
{
    // Die Initialisierung starten
    outb(PIC_MASTER_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INIT);
    outb(PIC_SLAVE_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INIT);

    // IRQ-Basis Interrupt-Nummer setzen
    outb(PIC_MASTER_DATA, IM_IRQ_BASE);
    outb(PIC_SLAVE_DATA, IM_IRQ_BASE + 8);

    // Den Slave auf IRQ 2 setzen
    outb(PIC_MASTER_DATA, 4);
    outb(PIC_SLAVE_DATA, 2);

    outb(PIC_MASTER_DATA, PIC_ICW4_PC);
    outb(PIC_SLAVE_DATA, PIC_ICW4_PC);
    
    // Alle IRQs aktivieren
    outb(PIC_MASTER_DATA, 0x00);
    outb(PIC_SLAVE_DATA, 0x00);
}

/**
 * Einen oder mehrere IRQs deaktivieren
 *
 * @param start Start-IRQ
 * @param cout Anzahl der IRQs
 */
void pic_disable_irq(uint8_t irq)
{
    if (irq < 8) {
        // IRQ fuer den Master-PIC
        outb(PIC_MASTER_DATA, inb(PIC_MASTER_DATA) | (1 << irq));
    } else if (irq < 16) {
        // IRQ fuer den Slave-PIC
        outb(PIC_SLAVE_DATA, inb(PIC_SLAVE_DATA) | (1 << (irq - 8)));
    } else {
        panic("Ungueltiger IRQ %d", irq);
    }

}

/**
 * Einen oder mehrere IRQs aktivieren
 *
 * @param start Start-IRQ
 * @param cout Anzahl der IRQs
 */
void pic_enable_irq(uint8_t irq)
{
    if (irq < 8) {
        // IRQ fuer den Master-PIC
        outb(PIC_MASTER_DATA, inb(PIC_MASTER_DATA) & (~(1 << irq)));
    } else if (irq < 16) {
        // IRQ fuer den Slave-PIC
        outb(PIC_SLAVE_DATA, inb(PIC_SLAVE_DATA) & (~(1 << (irq - 8))));
    } else {
        panic("Ungueltiger IRQ %d", irq);
    }
}

/**
 * Benachrichtigt den PIC dass der IRQ fertig bearbeitet wurde.
 *
 * @param irq IRQ-Nummer
 */
void pic_eoi(uint8_t irq)
{
    if (irq > 16) {
        panic("Ungueltiger IRQ %d", irq);
    }

    if (irq >= 8) {
        // Den Slave auch noch benachrichtigen
        outb(PIC_SLAVE_COMMAND, PIC_EOI);
    }
    
    // Den Master benachrichtigen
    outb(PIC_MASTER_COMMAND, PIC_EOI);
}

