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
#include <config.h>
#include <stdlib.h>

#include "multiboot.h"
#include "tasks.h"
#include "mm.h"
#include "kernel.h"
#include "kprintf.h"

static void load_module_bin(struct multiboot_module* multiboot_module,
    char* cmdline);
static void load_module(struct multiboot_module* multiboot_module);

/**
 * Das Init-Modul laden. Dieses Modul ist dafuer verantwortlich, die restlichen
 * Module zu laden.
 *
 * @param multiboot_info Pointer zu der vom Bootloader uebergebenen
 *                          Info-Struktur.
 */
void load_init_module(struct multiboot_info* multiboot_info)
{
    // Wenn keine Module vorhanden sind, ist es sinnlos weiter zu machen, denn
    // der Kernel alleine ist nutzlos.
    if (multiboot_info->mi_mods_count == 0) {
        panic("Kein Init-Modul verfuegbar.");
    }
    
    // Das Modul laden
    load_module((struct multiboot_module*) multiboot_info->mi_mods_addr);

    // Damit das Init-Modul spaeter nicht nochmal geladen wird, wird der
    // Pointer in der Info-Struktur auf das naechste Element umgebogen, und die
    // Anzahl wird um 1 verringert.
    multiboot_info->mi_mods_addr += sizeof(struct multiboot_module);
    multiboot_info->mi_mods_count--;
}

/**
 * Ein Modul anhand der Modul-struktur vom Multibootloader
 *
 * @param multiboot_module
 */
static void load_module(struct multiboot_module* multiboot_module)
{
    // Die Modul-Struktur mappen. Das ist kein Problem, weil sie bei der
    // Initialisierung der physikalischen Speicherverwaltung reserviert wurde.
    struct multiboot_module* multiboot_module_mapped = vmm_kernel_automap(
        (paddr_t) multiboot_module,
        sizeof(struct multiboot_module));

    if (multiboot_module_mapped == NULL) {
        panic("Die Multiboot-Modul-Struktur des Moduls konnte nicht "
            "gemapt werden");
    }

    // Die Befehlszeile des Moduls mappen. Auch diese wurde reserviert.
    // FIXME: Die laenge muss unbedingt geprueft werden.
    char* cmdline = vmm_kernel_automap((paddr_t) 
        multiboot_module_mapped->cmdline, 4096);
    
    load_module_bin(multiboot_module_mapped, cmdline);
}

/**
 * Ein Modul als flache Binaerdatei laden.
 *
 * @param multiboot_module
 * @param cmdline Kommandozeile, mit der das Modul gestartet wurde.
 */
static void load_module_bin(struct multiboot_module* multiboot_module,
    char* cmdline)
{
    pm_process_t* process = pm_create(USER_MEM_START, cmdline);
    mmc_map(&process->context, USER_MEM_START, multiboot_module->start,
        MM_FLAGS_USER_CODE,
        NUM_PAGES(multiboot_module->end - multiboot_module->start));
}

