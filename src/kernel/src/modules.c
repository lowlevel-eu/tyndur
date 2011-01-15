/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#include "modules.h"
#include "multiboot.h"
#include "kprintf.h"
#include "elf32.h"
#include "vmm.h"
#include "kernel.h"
#include "string.h"
#include "paging.h"
#include "types.h"
#include "intr.h"
#include "tasks.h"
#include "kmm.h"
#include "paging.h"

#define ELF_MAGIC 0x464C457F
#define MODULE_ENTRY ((vaddr_t) 0x40000000)

#undef DEBUG

//Wird in schedule.c definiert und hier verwendet, um auf die Task-Struct
// des Init-Moduls zuzugreifen
extern struct task* first_task;

//Laed ein ELF-Dateiimage (wird nur noch fuer init verwendet)
void load_single_module(Elf32_Ehdr* elf_header, const char* cmdline);




/**
 * Laed nur das erste Modul, das in der Multiboot-Modulliste eingetragen
 * ist, das Init-Modul. Es wird im Moment nur ELF 
 * unterstuetzt.
 *
 * @param elf_header Pointer auf den ELF-Header
 */
void load_init_module(struct multiboot_info * multiboot_info)
{
    struct multiboot_module * multiboot_module;
    
    //Ueberpruefen, ob ueberhaupt module in der Liste sind
    if (multiboot_info->mi_mods_count == 0)
    {
        panic("Keine Multiboot-Module zu laden.");
    }
    
    
    //Multiboot-Modulliste in den Kerneladressraum mappen
    multiboot_module = map_phys_addr((paddr_t)multiboot_info->mi_mods_addr, sizeof(multiboot_module));
    if (multiboot_module == NULL)    
    {
        panic("Konnte Multiboot-Modulliste nicht in den Kernelspeicher mappen.");
    }
    
    //Befehlszeile des Moduls mappen
    char* cmdline = map_phys_addr((paddr_t) multiboot_module->cmdline, 4096);

    //Das Modul dem ELF-Loader uebergeben
    void* elf_image = map_phys_addr((paddr_t) multiboot_module->start,
        multiboot_module->end - multiboot_module->start);
    load_single_module(elf_image, cmdline);
    free_phys_addr(elf_image, multiboot_module->end - multiboot_module->start);
    
    //Modulbefehlszeile wieder freigeben
    free_phys_addr(cmdline, 4096);

    //Multiboot-Modulliste wieder aus dem Kerneladressraum freigeben
    free_phys_addr((vaddr_t)multiboot_module, sizeof(multiboot_module));
    
    //In der Multiboot Struct den Pointer und die anzahl der Module anpassen,
    // da Init nun geladen ist.
    multiboot_info->mi_mods_addr++;
    multiboot_info->mi_mods_count--;
}


/**
 * Hier werden die anderen Module Module an Init weiter geleitet
  *
 * @param elf_header Pointer auf den ELF-Header
 */
void load_multiboot_modules(struct multiboot_info * multiboot_info)
{
    int i;
    struct multiboot_module * multiboot_module_phys;
    struct multiboot_module * multiboot_module;
    
    //Ueberpruefen, ausser dem 
    if (multiboot_info->mi_mods_count == 0)
    {
        //Ist hier ein Panic wirklich das richtige?
        panic("Keine Multiboot-Module zu laden.");
    }
    else
    {
        multiboot_module_phys = multiboot_info->mi_mods_addr;
        
        // Speicher für die Modulparameter
        paddr_t mod_cmdline_page_phys = phys_alloc_page();
        void* mod_cmdline_task = find_contiguous_pages((page_directory_t)first_task->cr3, 1, USER_MEM_START, USER_MEM_END);
        map_page((page_directory_t)first_task->cr3, mod_cmdline_task, mod_cmdline_page_phys, PTE_P | PTE_U | PTE_W);
        
        char* mod_cmdline_page = map_phys_addr(mod_cmdline_page_phys, PAGE_SIZE);
        char* mod_cmdline = mod_cmdline_page;
        size_t mod_cmdline_free = PAGE_SIZE;

        //Speicher für die Modulliste allokieren und 
        void* mod_list_task = find_contiguous_pages((page_directory_t)first_task->cr3, 1, USER_MEM_START, USER_MEM_END);
        paddr_t mod_list_phys = phys_alloc_page();
        map_page((page_directory_t)first_task->cr3, mod_list_task, mod_list_phys, PTE_P | PTE_U | PTE_W);
        
        
        //Stack von Init mappen um ein Pointer auf die Modulliste zu uebergeben
        uint32_t* stack = (uint32_t*) ((uint32_t)map_phys_addr(resolve_vaddr((page_directory_t)first_task->cr3, first_task->user_stack_bottom), 0x1000) - 0);
        stack[255] = (uint32_t)mod_list_task;
        free_phys_addr((vaddr_t)stack, 0x1000);
        
        //Modulliste temporaer mappen
        uint32_t* modulelist = (uint32_t*) map_phys_addr(mod_list_phys, 0x1000);
        
        //Das unterste dword auf der Page mit der Liste beinhaltet die Anzahl der Module
        modulelist[0] = multiboot_info->mi_mods_count;
        
        //Module in den Adressraum von Init mapen
        for (i = 0; i < multiboot_info->mi_mods_count; i++)
        {
            //Die multiboot_module Struct des aktuellen Moduls mappen
            multiboot_module = map_phys_addr((paddr_t)multiboot_module_phys, sizeof(multiboot_module));
            
            //Anzahl der benoetigten Pages berechnen
            size_t pages = (PAGE_ALIGN_ROUND_UP((uint32_t)multiboot_module->end) -PAGE_ALIGN_ROUND_DOWN((uint32_t)multiboot_module->start)) / 0x1000;
            
            //Freie virtuelle Adresse suchen um das Modul dort hin zu mappen.
            void* dest = find_contiguous_pages((page_directory_t)first_task->cr3, pages, USER_MEM_START, USER_MEM_END);
            map_page_range((page_directory_t)first_task->cr3, dest, (paddr_t) multiboot_module->start, PTE_P | PTE_U | PTE_W, pages);
            first_task->memory_used += pages * 0x1000;

            //TODO: Den Speicher aus dem Kerneladressraum unmappen
           
            //Die Adresse in die Modulliste von Init schreiben
            modulelist[1 + (3*i)] = (uint32_t) dest;
            modulelist[2 + (3*i)] = multiboot_module->end - multiboot_module->start;
            modulelist[3 + (3*i)] = (uint32_t) mod_cmdline_task + (mod_cmdline - mod_cmdline_page);

            // Modellbefehlszeile kopieren
            char* cmdline = map_phys_addr((paddr_t) multiboot_module->cmdline, 4096);
            size_t length = strlen(cmdline);
            if (mod_cmdline_free > length) {
                strncpy(mod_cmdline, cmdline, length);
                mod_cmdline[length] = '\0';
                mod_cmdline += (length + 1);
                mod_cmdline_free -= (length + 1);
            }
            free_phys_addr(cmdline, 4096);
            
            //multiboot_module Struct wieder freigeben
            free_phys_addr((vaddr_t)multiboot_module, sizeof(multiboot_module));
            
            //Physikalische Adresse der  multiboot_module Struct auf die naechste setzen
            multiboot_module_phys++;
        }
        
        //Die temporaer gemapte Modulliste wieder freigeben(nur hier im Kernel freigeben)
        free_phys_addr((vaddr_t)modulelist, 0x1000);
        free_phys_addr(mod_cmdline_page, 0x1000);
    }
    
    //TODO: Muessen noch irgendwelche Multiboot-Structs physikalisch freigegeben werden?
}  


/**
 * Ein einzelnes ELF-Image laden und den Prozess erstellen.
 *
 * @param elf_header Pointer auf den ELF-Header. Es wird erwartet, dass das
 * gesamte ELF-Image gemappt ist.
 */
void load_single_module(Elf32_Ehdr * elf_header, const char* cmdline) 
{
    Elf32_Phdr * program_header;
    uint32_t i, j;
    uint32_t pages;

    struct task * task = NULL;
    vaddr_t address, base, dst;
    paddr_t phys_dst;

    //kprintf("[ehdr: %x]", elf_header);

    if (elf_header->e_magic == ELF_MAGIC) {
        
        program_header = (Elf32_Phdr *) ((uint32_t) elf_header + elf_header->e_phoff);

#ifdef DEBUG
        kprintf("    EntryCount: %d", elf_header->e_phnum);
        kprintf("    Offset: 0x%x\n", elf_header->e_phoff);
#endif

        for (i = 0; i < elf_header->e_phnum; i++, program_header++) {

#ifdef DEBUG
            kprintf(" ph @ %x, type=%d\n", program_header, program_header->p_type);
#endif
            if (program_header->p_type == 1) {
                
              	if ((elf_header->e_entry >= program_header->p_vaddr) && (elf_header->e_entry <= program_header->p_vaddr + program_header->p_memsz)) {
#ifdef DEBUG
                    kprintf("Neuen Task erstellen\n");
#endif
                    task = create_task((vaddr_t) elf_header->e_entry, cmdline, 0);
                } else if (task == NULL) {
					// FIXME
					kprintf("Kein Task: phdr->vaddr = %x, ehdr->entry = %x\nDas Modul ist entweder beschaedigt oder wird von diesem Kernel nicht unterstuetzt (Entry Point muss im ersten Program Header liegen)\n", program_header->p_vaddr, elf_header->e_entry);
                    continue;
                }
                
                // Pages erstellen
                pages = 1 + ((program_header->p_offset + program_header->p_memsz) / 0x1000) - (program_header->p_offset / 0x1000);
                
#ifdef DEBUG
                kprintf("Mapping von %d Pages\n", pages);
#endif
                base = (vaddr_t) PAGE_ALIGN_ROUND_DOWN(program_header->p_vaddr);
                if (base < MODULE_ENTRY) {
                    base = MODULE_ENTRY;
                }

                for (j = 0; j < pages; j++) {
                    paddr_t curPage = phys_alloc_page();
#ifdef DEBUG
                    kprintf("  %x => %x", (vaddr_t) (base + 0x1000 * j), curPage);
#endif
                    map_page((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j), curPage, PTE_P | PTE_U | PTE_W);
                }

                // Programm kopieren
#ifdef DEBUG
                kprintf("Programm kopieren\n");
#endif
                pages = 1 + ((program_header->p_offset + program_header->p_filesz) / 0x1000) - (program_header->p_offset / 0x1000);
#ifdef DEBUG
                kprintf("%d Pages zu kopieren\n", pages);
#endif
                
                
                uint32_t bytes_on_first_page;
                if (program_header->p_filesz > 0x1000 - (program_header->p_vaddr % 0x1000)) {
                    bytes_on_first_page = 0x1000 - (program_header->p_vaddr % 0x1000);
                } else {
                    bytes_on_first_page = program_header->p_filesz;
                }

                uint32_t zero_bytes_on_last_page = 0x1000 - (
                    (program_header->p_offset + program_header->p_filesz) 
                    % 0x1000);

#ifdef DEBUG
                kprintf("bytes_on_first_page: 0x%x\n", bytes_on_first_page);
                kprintf("zero_bytes_on_last_page: 0x%x\n", zero_bytes_on_last_page);
#endif

                // Erste Page kopieren (möglicherweise fangen die Daten mitten in der
                // Page an)
                address = (vaddr_t) ((uint32_t) elf_header + program_header->p_offset);
                phys_dst = resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) program_header->p_vaddr);
                dst = map_phys_addr(phys_dst, bytes_on_first_page);
#ifdef DEBUG
                kprintf("  Kopiere 0x%x Bytes nach %x von %x [1]\n", bytes_on_first_page, program_header->p_vaddr, address);
#endif
                memcpy(dst, address, bytes_on_first_page);
                free_phys_addr(dst, bytes_on_first_page);


                address += bytes_on_first_page;
                for (j = 1; j < pages; j++) {
                    //address = map_phys_addr(phys_address, 0x1000);

                    phys_dst = resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j));
                    dst = map_phys_addr(phys_dst, 0x1000);
#ifdef DEBUG
                    kprintf("  Kopiere 4K nach %x von %x\n", (base + 0x1000 * j), address);
#endif
                    memcpy(dst, address, 0x1000);

                    free_phys_addr(dst, 0x1000);
                    //free_phys_addr(address, 0x1000);

                    address += 0x1000;                
                }

                if (zero_bytes_on_last_page) {
                    // Es ist ein Teilstück einer Page übrig, das mit Nullen gefüllt wird
#ifdef DEBUG
                    kprintf("  Setze Nullen in 0x%x (0x%x Stueck)\n", 
                            (vaddr_t) (base + 0x1000 * (j - 1)) + (0x1000 - zero_bytes_on_last_page), 
                            zero_bytes_on_last_page);
#endif

                    phys_dst = resolve_vaddr(
                        (page_directory_t) task->cr3, 
                        (vaddr_t) (base + 0x1000 * (j - 1)) + (0x1000 - zero_bytes_on_last_page)
                    ) /*(bytes_on_last_page)*/;

                    dst = map_phys_addr(phys_dst, zero_bytes_on_last_page);
                    memset(dst, 0, zero_bytes_on_last_page);
                    free_phys_addr(dst, zero_bytes_on_last_page);
                }
                
                // Wenn memsz > filesz. muß mit Nullen aufgefüllt werden
                pages = program_header->p_memsz / 0x1000;
                if (program_header->p_memsz % 0x1000) {
                    pages++;
                }
                for (; j < pages; j++) {
                    phys_dst = resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j));
                    dst = map_phys_addr(phys_dst, 0x1000);

#ifdef DEBUG
                    kprintf("  Setze Nullen in %x\n", (base + 0x1000 * j));
#endif
                    memset(dst, 0, 0x1000);

                    free_phys_addr((vaddr_t)phys_dst, 0x1000);
                }
                
#ifdef DEBUG
                kprintf("    Modul geladen.\n");
#endif
            }
        }
    } else {
        kprintf("ELF-Magic ungueltig (0x%x)\n",elf_header->e_magic);
    }
}
