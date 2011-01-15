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
#include <stdbool.h>

#include "apic.h"
#include "cpu.h"
#include "mm.h"

#define APIC_BASE_MSR 0x1B

#define APIC_REG_SPIV 0xF0
#define APIC_REG_ICR_LOW 0x300
#define APIC_REG_ICR_HIGH 0x310
#define APIC_REG_DIV_CONFIG 0x3E0
#define APIC_REG_LVT_TIMER 0x320
#define APIC_REG_LVT_THERM 0x330
#define APIC_REG_LVT_PERFMON 0x340
#define APIC_REG_LVT_LINT0 0x350
#define APIC_REG_LVT_LINT1 0x360
#define APIC_REG_LVT_ERROR 0x370

vaddr_t apic_base_virt = 0;
paddr_t apic_base_phys = 0;

mmc_context_t init_context;

/**
 * Ueberprueft ob ein APIC vorhanden ist
 *
 * @return true wenn er vorhanden ist.
 */
bool apic_available()
{
    // FIXME ;-)
    return false;

    uint32_t edx;

    asm("movl $1, %%eax;"
        "cpuid;"
        : "=d"(edx) : : "eax", "ebx", "ecx");
    
    // Kein APIC
    if ((edx & (1 << 9)) == 0) {
        return false;
    } else {
        return true;
    }
}

/**
 * Die APIC-Register mappen, damit sie mit Paging benutzt werden koennen
 */
void apic_map(mmc_context_t* context)
{
    apic_base_virt = mmc_automap(context, apic_base_phys, 1,
        KERNEL_MEM_START, KERNEL_MEM_END, MM_FLAGS_KERNEL_DATA);
}

/**
 * APIC initialisiern
 */
void apic_init()
{
    // FIXME: Hier wird ueberall angenommen, dass alle APICs die selbe Basis
    // habe!
    if (apic_base_virt == 0) {
        apic_base_phys = (paddr_t) (uintptr_t) (cpu_read_msr(APIC_BASE_MSR) &
            (~0xFFF));
        apic_base_virt = (vaddr_t) apic_base_phys;
    }

    // Jetzt wird der APIC aktiviert
    // TODO: Was machen wir mit dem Spurious interrupt vector?
    uint32_t spiv;
    spiv = 1 << 8;
    apic_write(APIC_REG_SPIV, spiv);
}

/**
 * Ein APIC-Register auslesen
 *
 * @param offset Speicherstelle im APIC
 * 
 * @return Wert der Speicherstelle
 */
uint32_t apic_read(uint32_t offset)
{
    uint32_t result = *((uint32_t*) ((uintptr_t) apic_base_virt + offset));
    //kprintf("apic_read(0x%x) = 0x%x\n", offset, result);
    return result;
}

/**
 * Ein APIC-Register setzen
 *
 * @param offset Speicherstelle im APIC
 * @param value Neuer Wert fuer die Speicherstelle
 */
void apic_write(uint32_t offset, uint32_t value)
{
    //kprintf("apic_write(0x%x, 0x%x)  (base:%x)\n", offset, value, apic_base_virt);
    uint32_t* ptr = (uint32_t*) ((uintptr_t) apic_base_virt + offset);
    *ptr = value;
}

/**
 * Ein Startup-IPI an die anderen Prozessoren schicken
 *
 * @param trampoline Adresse des Trampolin-Codes. Muss 4KB-Aligned und unter
 *                      dem ersten Megabyte sein.
 */
void apic_startup_ipi(uint32_t trampoline)
{
    // Startup-IPI an alle ausser dem aktuellen Prozessor senden.
    // Mehr dazu im Intel manual "Intel 64 and IA32 Architectures Software
    // Developer Manual, Volume 3a, System Programming Guide Part 1" im Kapitel
    // 7.5 und 8.6
    apic_write(APIC_REG_ICR_HIGH, 0);
    apic_write(APIC_REG_ICR_LOW, ((trampoline / 0x1000) & 0xFF) | 0xC4600);
}

/**
 * Einen IPI an mehrere Prozessoren schicken.
 *
 * @param vector Interrupt-Nummer
 * @param self true falls der aktuelle Prozessor mit einbezogen werden soll,
 *              false sonst.
 */
void apic_ipi_broadcast(uint8_t vector, bool self)
{
    // Genauere Infos dazu im Intel manual (siehe apic_startup_ipi)
    apic_write(APIC_REG_ICR_HIGH, 0);
    apic_write(APIC_REG_ICR_LOW, (vector | (1 << 14) | (((self ? 2
        : 3) << 18))));
}

/**
 * Den APIC-Timer einrichten
 *
 * @param interrupt Die Interruptnummer
 * @param periodic Wenn true wird der Timer immer wieder ausgeloest, sonst wird
 *                  er nur einmal aufgerufen.
 * @param divisor Teiler durch den die Bus-Frequenz geteilt wird. Muss eine
 *                  2er Potenz sein.
 */
void apic_setup_timer(uint8_t interrupt, bool periodic, uint8_t divisor)
{
    uint32_t div_conf = 0;
    
    // Den Wert fuers "Divide Configuration Register" bestimmen
    if ((divisor & (1 << 7)) != 0) {
        div_conf = 0xA;
    } else if ((divisor & (1 << 6)) != 0) {
        div_conf = 0x9;
    } else if ((divisor & (1 << 5)) != 0) {
        div_conf = 0x8;
    } else if ((divisor & (1 << 4)) != 0) {
        div_conf = 0x3;
    } else if ((divisor & (1 << 3)) != 0) {
        div_conf = 0x2;
    } else if ((divisor & (1 << 2)) != 0) {
        div_conf = 0x1;
    } else if ((divisor & (1 << 1)) != 0) {
        div_conf = 0x0;
    } else {
        div_conf = 0xB;
    }

    // Das DCR schreiben
    apic_write(APIC_REG_DIV_CONFIG, div_conf);
    
    // Das Timer-Register in der IVT setzen
    uint32_t timer_reg = interrupt;
    if (periodic == true) {
        timer_reg |= 1 << 17;
    }
    apic_write(APIC_REG_LVT_TIMER, timer_reg);
}

