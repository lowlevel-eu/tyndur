/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
#include <types.h>
#include <string.h>
#include <stdint.h>

#include "lock.h"
#include "cpu.h"
#include "apic.h"
#include "kprintf.h"
#include "mm.h"

#define MP_FP_SIGNATURE ('_' | ('M' << 8) | ('P' << 16) | ('_' << 24))
#define MP_CF_SIGNATURE ('P' | ('C' << 8) | ('M' << 16) | ('P' << 24))

/// Der MP-Floating-Pointer enthält alle moeglichen Informationen ueber SMP
struct MP_floating_pointer {
    dword signature;
    dword config_table;
    byte length;
    byte version;
    byte checksum;
    byte features[5];
} __attribute((packed));

/// Der Anfang der Config-Table
struct MP_config_table {
    dword signature;
    word base_table_length;
    byte spec_revision;
    byte checksum;
    byte oem_id[8];
    byte product_id[12];
    dword oem_table;
    word oem_table_size;
    word entry_count;
    dword local_apic_address;
    /*word extended_table_length;
    byte extended_table_checksum;
    byte reserved;*/
    dword reserved;
} __attribute((packed));

/// CPU-Eintrag in der Config-Table
struct MP_cte_processor {
    byte entry_type;
    byte local_apic_id;
    byte local_apic_version;
    byte flags;
    byte signature[4];
    dword feature_flags;
    dword reserved[2];
} __attribute((packed));

/**
 * Suchen des MP-Floating pointers in einem angegebenen bereich.
 *
 * @return Pointer darauf, wenn sie gefunden wurde, sonst NULL.
 */
static struct MP_floating_pointer* find_floating_pointer_in(uintptr_t start,
    size_t size)
{
    struct MP_floating_pointer* floating_pointer = (struct MP_floating_pointer*)
        start;
    
    // Den gegebenen Bereich durchsuchen
    while ((uintptr_t) floating_pointer < (start + size)) {
        // Wurde die Signatur gefunden?
        if (floating_pointer->signature == MP_FP_SIGNATURE) {
            // Zur Sicherheit wird jetzt noch die Pruefsumme geprueft.
            byte checksum = 0;
            byte* fp = (byte*) floating_pointer;
            byte i;
            for (i = 0; i < sizeof(struct MP_floating_pointer); i++) {
                checksum += fp[i];
            }

            // Wenn diese auch stimmt, ist die Suche hier abgeschlossen.
            if (checksum == 0) {
                return floating_pointer;
            }
        }
        floating_pointer++;
    }
    return NULL;
}

/**
 * Den MP-Floating-Pointer suchen
 *
 * @return Pointer darauf, wenn sie gefunden wurde, sonst NULL.
 */
static struct MP_floating_pointer* find_floating_pointer()
{
    struct MP_floating_pointer* floating_pointer;

    floating_pointer = find_floating_pointer_in(0, 0x400);
    if (floating_pointer == NULL) {
        floating_pointer = find_floating_pointer_in(0x9FC00, 0x400);
        if (floating_pointer == NULL) {
            floating_pointer = find_floating_pointer_in(0xE0000, 0x20000);
        }
    }
    return floating_pointer;
}

/**
 * Hier kommen alle APs an, sobald sie initialisiert sind
 */
void smp_entry()
{
    static lock_t l;

    lock(&l);
    kprintf("smp_entry()  CPU ");
    cpu_t* cpu = cpu_get_current();
    kputn(cpu->id, 10, 0, ' ');
    kprintf("\n");
    unlock(&l);

    asm("hlt");
}

/**
 * Initialisiert SMP
 */
void arch_init_smp()
{
    struct MP_floating_pointer* floating_pointer = find_floating_pointer();
    
    // Wenn der Floating-Pointer nicht gefunden wurde, wird abgebrochen.
    if (floating_pointer == NULL) {
        return;
    }

    // Ueberpruefen ob eine Konfigurationstabelle vorhanden ist.
    if (floating_pointer->features[0] == 0) {
        struct MP_config_table* config_table = (struct MP_config_table*) 
            ((uintptr_t) floating_pointer->config_table);
        
        if (config_table->signature != MP_CF_SIGNATURE) {
            kprintf("Die Signatur der Konfiturationstabelle ist beschaedigt");
            return;
        }
        
        byte* current_offset = (byte*) ((uintptr_t)config_table + 
            sizeof(struct MP_config_table));
        word entry_count;
        
        // Die einzelnen Eintraege durchgehen
        for (entry_count = 0; entry_count < config_table->entry_count;
            entry_count++)
        {
            //Je nachdem, welcher Typ der Eintrag hat, wird jetzt entschieden
            // was zu tun ist. Im moment werden nur die CPU-Eintraege am Anfang
            // der Tabelle beachtet.
            if (*current_offset == 0) {
                // Eintrag fuer einen Prozessor
                struct MP_cte_processor* processor = (struct MP_cte_processor*)
                    current_offset;
                kprintf(" -Prozessor ");
                kputn(processor->local_apic_id, 10, 0, ' ');
                
                // Wenn der Prozessor nicht deaktiviert ist, wird er
                // eingetragen
                if ((processor->flags & 1) == 1) {
                    kprintf(" aktiviert ");

                    // CPU in die Liste eintragen
                    cpu_t cpu;
                    cpu.apic_id = processor->local_apic_id;
                    
                    // Wenn das der Bootstrap-Prozessor ist, wird er
                    // entsprechend markiert.
                    if ((processor->flags & 2) == 2) {
                        cpu.bootstrap = TRUE;
                        kprintf(" bootstrap ");
                    } else {
                        cpu.bootstrap = FALSE;
                    }
                    cpu_add_cpu(cpu);
                }

                 kprintf("\n");
                current_offset += sizeof(struct MP_cte_processor);
            } else {
                break;
            }
            /*if (*current_offset == 1) {
                kprintf(" -Bus\n");
                current_offset += 8;
            } if (*current_offset == 2) {
                kprintf(" -IO APIC\n");
                current_offset += 8;
            } if (*current_offset == 3) {
                kprintf(" -IO Interrupt Assignment\n");
                current_offset += 8;
            } if (*current_offset == 4) {
                kprintf(" -Local Interrupt Assignment\n");
                current_offset += 8;
            } else {
                // Das sollte eigentlich nicht passieren
            }*/
        }
    } else {
        kprintf("Die Standard-SMP-Konfigurationen werden noch nicht"
            "unterstuetzt!");
    }
    
    // Den lokalen APIC initialisieren
    apic_initialize();
    
    // Den Trampolin-Code Kopieren
    extern void* _binary_rm_trampoline_o_start;
    extern void* _binary_rm_trampoline_o_end;
    uintptr_t trampoline = 0x7000;
    size_t bytes = (uintptr_t) &_binary_rm_trampoline_o_end - (uintptr_t) 
        &_binary_rm_trampoline_o_start;
    memcpy((void*) trampoline, (void*) &_binary_rm_trampoline_o_start, bytes);
    // FIXME: Den Speicher reservieren. Siehe unten.

    
    // FIXME: Falls der Kernel mal ueber 4GB ist, haben wir hier ein Problem
    // Hier wird die Adresse hingeschrieben, wo die Prozessoren nach dem
    // initialisieren hinspringen sollen
    extern void* smp_pm_entry;
    *((dword*) (trampoline + 2)) = (dword) (uintptr_t) &smp_pm_entry;


    
    // Die Adresse der Pagemap dort hinschreiben
    uintptr_t cr3;
    asm("movq %%cr3, %0" : "=A" (cr3));
    extern dword smp_entry_context;
    smp_entry_context = cr3 & 0xFFFFFFFF;
   
    // Los gehts! Statup-IPI schicken
    apic_startup_ipi(trampoline); 
    
    // FIXME: Steht nur da, damit beim allozieren des Stacks nicht der
    // Trampoline-Code ueberschrieben wird. Ist aber garnicht sauber.
    pmm_alloc(10);
    smp_entry();
}
