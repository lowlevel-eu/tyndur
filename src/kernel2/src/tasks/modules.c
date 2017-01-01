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
#include <types.h>
#include <lost/config.h>
#include <stdlib.h>
#include <string.h>
#include <loader.h>
#include <errno.h>
#include <syscall_structs.h>

#include "multiboot.h"
#include "tasks.h"
#include "mm.h"
#include "kernel.h"
#include "kprintf.h"
#include "tasks.h"
#include "cpu.h"

static void load_module(struct multiboot_module* multiboot_module);

static pm_process_t* init_process;

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
    load_module(
        (struct multiboot_module*)(uintptr_t) multiboot_info->mi_mods_addr);

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
    

    // Datei-Image mappen
    size_t image_size = multiboot_module_mapped->end - multiboot_module_mapped->
        start;
    vaddr_t image_start = vmm_kernel_automap((paddr_t)
        multiboot_module_mapped->start, image_size);

    //load_module_bin(multiboot_module_mapped, cmdline);
    init_process = pm_create(NULL, cmdline);
    pm_unblock(init_process);
    if (loader_load_image(init_process->pid, (vaddr_t) image_start, image_size)
        == false)
    {
        panic("Init-Modul wurde nicht erfolgreich geladen!");
    }
    
    // Modul unmappen und Speicher freigeben
    vmm_kernel_unmap(image_start, image_size);
    pmm_free((paddr_t) multiboot_module_mapped->start, NUM_PAGES(image_size));
}

/**
 * Hier werden die anderen Module an init weitergeleitet
  *
 * @param elf_header Pointer auf den ELF-Header
 */
void load_multiboot_modules(struct multiboot_info* multiboot_info)
{
    int i;
    struct multiboot_module* multiboot_module_phys;
    struct multiboot_module* multiboot_module;

    if (multiboot_info->mi_mods_count == 0)
    {
        panic("Keine Multiboot-Module zu laden.");
    }
    else
    {
        multiboot_module_phys =
            (struct multiboot_module*)(uintptr_t) multiboot_info->mi_mods_addr;

        // Speicher für die Modulparameter
        paddr_t mod_cmdline_page_phys = pmm_alloc(1);
        void* mod_cmdline_task = mmc_automap(
            &init_process->context, mod_cmdline_page_phys, 1,
            MM_USER_START, MM_USER_END, MM_FLAGS_USER_DATA);

        char* mod_cmdline_page = vmm_kernel_automap(mod_cmdline_page_phys, PAGE_SIZE);
        char* mod_cmdline = mod_cmdline_page;
        size_t mod_cmdline_free = PAGE_SIZE;

        // Speicher für die Modulliste
        paddr_t mod_list_phys = pmm_alloc(1);
        void* mod_list_task = mmc_automap(
            &init_process->context, mod_list_phys, 1,
            MM_USER_START, MM_USER_END, MM_FLAGS_USER_DATA);

        init_module_list_t* modulelist =
            vmm_kernel_automap(mod_list_phys, PAGE_SIZE);

        // Stack von init mappen und Pointer auf die Modulliste uebergeben
        struct {
            uintptr_t return_address;
            uintptr_t mod_list_task;
        } init_stack = {
            .mod_list_task  = (uintptr_t) mod_list_task,
            .return_address = 0,
        };

        pm_thread_t* init_thread = (pm_thread_t*)
            list_get_element_at(init_process->threads, 0);
        vmm_userstack_push(init_thread, &init_stack, sizeof(init_stack));

        // Das unterste dword auf der Page mit der Liste beinhaltet die
        // Anzahl der Module
        modulelist->count = multiboot_info->mi_mods_count;

        // Module in den Adressraum von init mappen
        for (i = 0; i < multiboot_info->mi_mods_count; i++)
        {
            //Die multiboot_module struct des aktuellen Moduls mappen
            multiboot_module = vmm_kernel_automap(
                (paddr_t)multiboot_module_phys,
                sizeof(multiboot_module));

            // Anzahl der benoetigten Pages berechnen
            size_t pages = NUM_PAGES(
                  PAGE_ALIGN_ROUND_UP((uintptr_t) multiboot_module->end)
                - PAGE_ALIGN_ROUND_DOWN((uintptr_t) multiboot_module->start));

            // Freie virtuelle Adresse suchen um das Modul dort hin zu mappen.
            void* dest = mmc_automap(
                &init_process->context,
                (paddr_t) multiboot_module->start, pages,
                MM_USER_START, MM_USER_END, MM_FLAGS_USER_DATA);
            init_process->memory_used +=  pages << PAGE_SHIFT;

            // TODO: Den Speicher aus dem Kerneladressraum unmappen

            // Die Adresse in die Modulliste von Init schreiben
            modulelist->modules[i].start   = dest;
            modulelist->modules[i].size    = multiboot_module->end - multiboot_module->start;
            modulelist->modules[i].cmdline = (char*) mod_cmdline_task + (mod_cmdline - mod_cmdline_page);

            // Modulbefehlszeile kopieren
            char* cmdline = vmm_kernel_automap(
                (paddr_t) multiboot_module->cmdline, PAGE_SIZE);

            size_t length = strlen(cmdline);
            if (mod_cmdline_free > length) {
                strncpy(mod_cmdline, cmdline, length);
                mod_cmdline[length] = '\0';
                mod_cmdline += (length + 1);
                mod_cmdline_free -= (length + 1);
            }
            vmm_kernel_unmap(cmdline, PAGE_SIZE);

            // multiboot_module struct wieder freigeben
            vmm_kernel_unmap(multiboot_module, sizeof(multiboot_module));

            // Naechstes Modul
            multiboot_module_phys++;
        }

        // Die temporaer gemappte Modulliste wieder freigeben
        vmm_kernel_unmap(modulelist, PAGE_SIZE);
        vmm_kernel_unmap(mod_cmdline_page, PAGE_SIZE);
    }

    // TODO Muessen noch irgendwelche Multiboot-Structs physisch
    // freigegeben werden?
}

/*##############################
 * Hilfsfunktionen => loader.h #
 *############################*/

/**
 * Alloziert Speicher um ihn an den Loader weiter zu geben, der ihn mit daten
 * Fuellt und anschliessend per loader_assign_mem an einen Prozess uebertraegt.
 */
vaddr_t loader_allocate_mem(size_t size) {
   paddr_t phys = pmm_alloc(NUM_PAGES(size));
   return vmm_kernel_automap(phys, size);
}

/**
 * Uebertraegt einen Speicherbereich in den Adressraum eines Prozesses.
 */
bool loader_assign_mem(pid_t pid, vaddr_t dest_address,
    vaddr_t src_address, size_t size)
{
    pm_process_t* process = pm_get(pid);
    size_t i;
    for (i = 0; i < NUM_PAGES(size); i++) {
        // Wenn beim mappen der Seiten ein Fehler aufgetreten ist, wird das
        // Mapping rueckgaengig gemacht
        if (mmc_map(&process->context, (vaddr_t) ((uintptr_t) dest_address +
            PAGE_SIZE * i), mmc_resolve(&mmc_current_context(), (vaddr_t)
            ((uintptr_t) src_address + i * PAGE_SIZE)), MM_FLAGS_USER_CODE,
            1) == false)
        {
            panic("Fehler beim Uebertragen von Pages vom Loader in den Prozess");
        }
    }

    vmm_kernel_unmap(src_address, size);

    return true;
}

/**
 * Lädt eine Shared Library in den Speicher.
 */
int loader_get_library(const char* name, void** image, size_t* size)
{
    return -ENOSYS;
}

/**
 * Erstellt einen Thread in einem Prozess der anhand seiner PID gefunden wird.
 */
bool loader_create_thread(pid_t pid, vaddr_t address) {
    pm_process_t* process = pm_get(pid);
    pm_thread_t* thread = pm_thread_create(process, address);
    
    // Nur wenn der Thread erfolgreich erstellt wurde,
    if (thread == NULL) {
        return false;
    } else {
        return true;
    }
}

