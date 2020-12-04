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
#include <types.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <lost/config.h>

#include "lock.h"
#include "cpu.h"
#include "apic.h"
#include "kprintf.h"
#include "tasks.h"
#include "mm.h"
#include "im.h"
#include "gdt.h"

#define MP_FP_SIGNATURE ('_' | ('M' << 8) | ('P' << 16) | ('_' << 24))
#define MP_CF_SIGNATURE ('P' | ('C' << 8) | ('M' << 16) | ('P' << 24))

#define CONFIG_SMP true

/// Der MP-Floating-Pointer ist die Haupstruktur. Von ihr aus, koennen die
/// anderen Informationen erreicht werden.
struct MP_floating_pointer {
    uint32_t signature;
    uint32_t config_table;
    uint8_t length;
    uint8_t version;
    uint8_t checksum;
    uint8_t features[5];
} __attribute((packed));

/// Der Anfang der Config-Table. Diese enthaelt genaue Informationen zu diesem
/// SMP-System. Ihr folgen mehrere Tabellen eintraege, die Komponenten wie
/// CPUs oder I/O-APICs beschreieben.
struct MP_config_table {
    uint32_t signature;
    uint16_t base_table_length;
    uint8_t spec_revision;
    uint8_t checksum;
    uint8_t oem_id[8];
    uint8_t product_id[12];
    uint32_t oem_table;
    uint16_t oem_table_size;
    uint16_t entry_count;
    uint32_t local_apic_address;
    uint32_t reserved;
} __attribute((packed));

/// CPU-Eintrag in der Config-Table
struct MP_cte_processor {
    uint8_t entry_type;
    uint8_t local_apic_id;
    uint8_t local_apic_version;
    uint8_t flags;
    uint8_t signature[4];
    uint32_t feature_flags;
    uint32_t reserved[2];
} __attribute((packed));

/// Dieser Kontext wird waehrend der initialisierung benutzt
#define init_context (&init_thread.process->context)
extern pm_thread_t init_thread;

/// Ist gesperrt waehrend ein Prozessor am initialisieren ist
extern lock_t init_lock;

/// Anzahl der Installierten CPUs
extern size_t cpu_count;

/// Weitere Prozessoren werden nur mit enable_smp = true initialisiert
bool enable_smp = false;

/**
 * Suchen des MP-Floating pointers in einem festgelegten Bereich. Diese
 * Struktur wird anhand der Signatur gesucht. Wenn die Signatur uebereinstimmt,
 * wird zur Sicherheit auch die Pruefsumme geprueft.
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
            // Zur Sicherheit wird jetzt noch die Pruefsumme geprueft. Das ist
            // hier ein einfaches Verfahren. Von der Spezifikation wird
            // vorgeschrieben, dass alle bytes der Strutktur, inklusiv die der
            // Pruefsumme, addiert 0 ergeben.
            uint8_t checksum = 0;
            uint8_t* fp = (uint8_t*) floating_pointer;
            uint8_t i;
            for (i = 0; i < sizeof(struct MP_floating_pointer); i++) {
                checksum += fp[i];
            }

            // Wenn diese auch stimmt, ist die Suche hier abgeschlossen. Falls
            // nicht, wird diese Adresse ignoriert, und weiter gesucht.
            if (checksum == 0) {
                return floating_pointer;
            }
        }
        floating_pointer++;
    }
    return NULL;
}

/**
 * Den MP-Floating-Pointer suchen. Das ist die wichtigste Informationsstruktur
 * fuer Mehrprozessorsysteme, da ohne sie auch keine Anderen erreicht werden
 * koennen.
 *
 * @return Pointer, wenn sie gefunden wurde, sonst NULL.
 */
static struct MP_floating_pointer* find_floating_pointer(void)
{
    struct MP_floating_pointer* floating_pointer;
    
    // Die Adresse dieser Struktur ist nicht genau festgelegt. In der Intel
    // Multiprozessor-Spezifikation sind drei Bereiche festgelegt, in denen sie
    // sich befinden kann. Hier wird versucht sie zu finden.
    
    floating_pointer = find_floating_pointer_in(0, 0x400);
    if (floating_pointer != NULL) {
        return floating_pointer;
    }
    
    floating_pointer = find_floating_pointer_in(0x9FC00, 0x400);
    if (floating_pointer != NULL) {
        return floating_pointer;
    }
    
    floating_pointer = find_floating_pointer_in(0xE0000, 0x20000);

    return floating_pointer;
}

/**
* Die anderen Prozessoren aufwecken.
*/
void smp_start(paddr_t entry)
{
    #if CONFIG_ARCH == ARCH_AMD64
        // Um in den Longmode wechseln zu koennen, muss bekanntlich Paging auf dem
        // Prozessor aktiviert werden. Dazu braucht der Trampoline-Code die Adresse
        // der Pagemap. Und die wird ihm jetzt mitgeteilt.
        uintptr_t cr3;
        asm("movq %%cr3, %0" : "=A" (cr3));
        extern uint32_t smp_entry_context;
        smp_entry_context = cr3 & 0xFFFFFFFF;
    #endif

    // Los gehts! Den Startup Interprocessor-Interrupt verschicken. Dieser geht
    // an alle Prozessoren ausser dem aktuellen, und weckt sie auf.
    apic_startup_ipi((uintptr_t) entry);
}


/**
 * Initialisiert SMP
 */
void smp_init(void)
{
    // Pointer auf den Floating Pointer suchen. Dies ist die wichtigste
    // Struktur fuer SMP. Denn nur wenn diese Vorhanden ist, wird SMP
    // unterstuetzt. In ihr steht auch der Pointer zur Konfigurationstabelle
    struct MP_floating_pointer* floating_pointer = find_floating_pointer();
    
    // Wenn der Floating-Pointer nicht gefunden wurde, wird abgebrochen, denn
    // dann wurde er entweder ueberschrieben, oder oder aber kein SMP wird
    // unterstuetzt.
    if ((floating_pointer == NULL) || (!CONFIG_SMP) || !enable_smp) {
        // Wenn kein SMP aktiviert ist, muss der Bootstrapprozessor manuell in
        // die CPU-Liste eingetragen werden, damit im Rest des Codes nicht
        // unterschieden werden muss.
        cpu_t cpu;
        cpu.apic_id = 0;
        cpu.thread = &init_thread;
        cpu.bootstrap = true;
        cpu_add_cpu(cpu);

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
        uint8_t* current_offset = (uint8_t*) ((uintptr_t)config_table + 
            sizeof(struct MP_config_table));
        
        kprintf("Anzahl:%d\n", config_table->entry_count);
        // Die einzelnen Eintraege durchgehen, und sie je nach Typ behandeln
        uint16_t entry_count;
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
                    // Die APIC-Id wird spaeter benoetigt, um mit den einzelnen
                    // CPUs Kontakt aufnehmen zu koennen.
                    cpu.apic_id = processor->local_apic_id;
                    cpu.thread = NULL;

                    // Wenn das der Bootstrap-Prozessor ist, wird er
                    // entsprechend markiert.
                    if ((processor->flags & 2) == 2) {
                        cpu.bootstrap = true;
                        kprintf(" bootstrap ");
                    } else {
                        cpu.bootstrap = false;
                    }

                    // CPU in die Liste aufnehmen
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
                kprintf(" -Seltsamer\n");
                current_offset += 8;

            }
        }
    } else {
        // TODO: Standard-Konfigurationen implementieren.
        kprintf("Die Standard-SMP-Konfigurationen werden noch nicht"
            "unterstuetzt!");
        return;
    }

    // Den Trampoline-Code Kopieren. Dieser wird fix nach 0x7000 kopiert. Unten
    // beim Senden der Startup-IPI wird diese Adresse mitgesendet, damit die
    // CPUs wissen, wo sie ihre Arbeit beginnen sollen.
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
    *((uint32_t*) (trampoline + 2)) = (uint32_t) (((uintptr_t) &smp_pm_entry)
        & 0xFFFFFFFF);

    smp_start((paddr_t) trampoline);
}

