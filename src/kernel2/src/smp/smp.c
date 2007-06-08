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
#include <config.h>

#include "lock.h"
#include "cpu.h"
#include "apic.h"
#include "kprintf.h"

#include "mm.h"
#include "im.h"

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

// Dieser Kontext wird waehrend der initialisierung benutzt
extern mmc_context_t init_context;

// Ist gesperrt waehrend ein Prozessor am initialisieren ist
extern lock_t init_lock;

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
    // Mehr als einen Prozessor am initialisieren... das kann nicht gut kommen
    // ;-)
    lock(&init_lock);
    unlock(&init_lock);
    // Nette Meldung mit CPU-Nummer ausgeben, damit der Benutzer sieht, das bis
    // hier alles geklappt hat.
    kprintf("smp_entry()  CPU %u\n", cpu_get_current()->id);
    
    // Interrupts aktivieren, damit Exceptions so frueh wie moeglich abgefangen
    // werden.
    //im_enable();
    // FIXME
    
    // Den Stack mappen
    // FIXME: Das will mir nicht so recht gefallen
  #if CONFIG_ARCH == ARCH_AMD64
    qword rsp;
    asm volatile("movq %%rsp, %0" : "=r"(rsp));
    mmc_map(init_context, (vaddr_t) PAGE_ALIGN_ROUND_DOWN(rsp),
        (paddr_t) PAGE_ALIGN_ROUND_DOWN(rsp), MM_FLAGS_KERNEL_DATA, 1);
    im_enable();
  #else
    dword esp;
    asm volatile("movl %%esp, %0" : "=r"(esp));
    mmc_map(init_context, (vaddr_t) PAGE_ALIGN_ROUND_DOWN(esp),
        (paddr_t) PAGE_ALIGN_ROUND_DOWN(esp), MM_FLAGS_KERNEL_DATA, 1);
  #endif
    
    // Die virtuelle Speicherverwaltung auf diesem Prozessor aktivieren
    vmm_init_local(init_context);
    
    // Den Lock wieder freigeben
    unlock(&init_lock);
    
    while (TRUE) {
        asm volatile("cli; hlt");
    }
}

/**
 * Initialisiert SMP
 */
void smp_init()
{
    // Pointer auf den Floating Pointer suchen. Dies ist die wichtigste
    // Struktur fuer SMP. Denn nur wenn diese Vorhanden ist, wird SMP
    // unterstuetzt. In ihr steht auch der Pointer zur Konfigurationstabelle
    struct MP_floating_pointer* floating_pointer = find_floating_pointer();
    
    // Wenn der Floating-Pointer nicht gefunden wurde, wird abgebrochen, denn
    // dann wurde er entweder ueberschrieben, oder oder aber kein SMP wird
    // unterstuetzt.
    if (floating_pointer == NULL) {
        return;
    }

    // Ueberpruefen ob eine Konfigurationstabelle vorhanden ist. Wenn diese
    // vorhanden ist, stehen in ihr alle Informationen die wir zum
    // initialisieren benoetigen. Die muss aber nicht zwingend vorhanden sein,
    // denn wenn sie nicht da ist, muss die richtige Standard-Konfiguration
    // benutzet werden.
    if (floating_pointer->features[0] == 0) {
        struct MP_config_table* config_table = (struct MP_config_table*) 
            ((uintptr_t) floating_pointer->config_table);
        
        // Nur wenn die Signatur stimmt, duerfen wir davon ausgehen, dass die
        // Konfigurationstabelle in Ordnung ist.
        // TODO: Pruefsumme?
        if (config_table->signature != MP_CF_SIGNATURE) {
            kprintf("Die Signatur der Konfiturationstabelle ist beschaedigt");
            return;
        }
        
        // Beinhaltet die Adresse des jeweils aktuellen Eintrages, waehrend dem
        // durchsuchen der Liste. Ist nur ein byte-Pointer, weil man damit
        // praktisch rechnen kann.
        byte* current_offset = (byte*) ((uintptr_t)config_table + 
            sizeof(struct MP_config_table));

        // Die einzelnen Eintraege durchgehen, und sie je nach Typ behandeln
        word entry_count;
        for (entry_count = 0; entry_count < config_table->entry_count;
            entry_count++)
        {
            // Je nachdem, welcher Typ der Eintrag hat, wird jetzt entschieden
            // was zu tun ist. Im moment werden nur die CPU-Eintraege am Anfang
            // der Tabelle beachtet.
            if (*current_offset == 0) {
                // Eintrag fuer einen Prozessor
                struct MP_cte_processor* processor = (struct MP_cte_processor*)
                    current_offset;
                kprintf(" -Prozessor %d", processor->local_apic_id);
                
                // Wenn der Prozessor nicht deaktiviert ist, wird er
                // eingetragen
                if ((processor->flags & 1) == 1) {
                    kprintf(" aktiviert ");

                    // CPU vorbereiten um sie in die CPU-Liste aufzunehmen
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

                    // CPU in die LISTE aufnehmen
                    cpu_add_cpu(cpu);
                } else {
                    kprintf(" deaktiviert ");
                }

                kprintf("\n");
                current_offset += sizeof(struct MP_cte_processor); 
            } else if (*current_offset == 1) {
                //kprintf(" -Bus\n");
                current_offset += 8;
            } else if (*current_offset == 2) {
                //kprintf(" -IO APIC\n");
                current_offset += 8;
            } else if (*current_offset == 3) {
                //kprintf(" -IO Interrupt Assignment\n");
                current_offset += 8;
            } else if (*current_offset == 4) {
                //kprintf(" -Local Interrupt Assignment\n");
                current_offset += 8;
            } else {
                // Das sollte eigentlich nicht passieren
                //break;
                current_offset += 8;
            }
        }
    } else {
        // TODO: Standard-Konfigurationen implementieren.
        kprintf("Die Standard-SMP-Konfigurationen werden noch nicht"
            "unterstuetzt!");
        return;
    }
    
    // Den lokalen APIC initialisieren
    apic_init();
    

    // Den Trampoline-Code Kopieren. Dieser wird fix nach 0x7000 kopiert.
    uintptr_t trampoline = 0x7000;

    // Die beiden Symbole werden von objcpy gesetzt, beim umwandeln der
    // Trampoline-Binary in eine Objektdatei.
    extern void* _binary_rm_trampoline_o_start;
    extern void* _binary_rm_trampoline_o_end;
    
    memcpy((void*) trampoline, (void*) &_binary_rm_trampoline_o_start, 
        (uintptr_t) &_binary_rm_trampoline_o_end - (uintptr_t)
        &_binary_rm_trampoline_o_start);
    

    // Hier wird die Adresse hingeschrieben, wo die Prozessoren nach dem
    // initialisieren hinspringen sollen.
    // Achtung: Die muss unbedingt nur aus 32 Bit bestehen und auch auf 32-Bit
    // Code zeigen. Wird in arch/$ARCH$/smp/trampoline.S definiert.
    extern void* smp_pm_entry;
    *((dword*) (trampoline + 2)) = (dword) (((uintptr_t) &smp_pm_entry) 
        & 0xFFFFFFFF);

  #if CONFIG_ARCH == ARCH_AMD64
    // Um in den Longmode wechseln zu koennen, muss bekanntlich Paging auf dem
    // Prozessor aktiviert werden. Dazu braucht der Trampoline-Code die Adresse
    // der Pagemap. Und die wird ihm jetzt mitgeteilt.
    uintptr_t cr3;
    asm("movq %%cr3, %0" : "=A" (cr3));
    extern dword smp_entry_context;
    smp_entry_context = cr3 & 0xFFFFFFFF;
  #endif
    
    // Los gehts! Den Startup Interprocessor-Interrupt verschicken. Dieser geht
    // an alle Prozessoren ausser dem aktuellen, und weckt sie auf.
    apic_startup_ipi(trampoline);
}

