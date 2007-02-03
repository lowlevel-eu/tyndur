/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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

#include "modules.h"
#include "multiboot.h"
#include "kprintf.h"
#include "elf32.h"
#include "vmm.h"
#include "kernel.h"
#include "string.h"
#include "paging.h"
#include "types.h"
#include "tasks.h"
#include "kmm.h"

#define ELF_MAGIC 0x464C457F
#define MODULE_ENTRY ((vaddr_t) 0x40000000)

void load_single_module(Elf32_Ehdr * elf_header);

void load_multiboot_modules(struct multiboot_info * multiboot_info)
{
    dword i;
    struct multiboot_module * multiboot_module_phys;
    struct multiboot_module * multiboot_module;
    

    if (multiboot_info->mi_mods_count == 0) {
        panic("Keine Multiboot-Module zu laden.");
    } else {
        kprintf("Lade Multiboot-Module...\n");
        
        multiboot_module_phys = multiboot_info->mi_mods_addr;

        for (i = 0; i < multiboot_info->mi_mods_count; i++) {
            //kernel_identity_map(multiboot_module, sizeof(multiboot_module));
            multiboot_module = map_phys_addr((paddr_t)multiboot_module_phys, sizeof(multiboot_module));
            kprintf("[%d @ %x] ", i,  multiboot_module);
            load_single_module(multiboot_module->start);
            multiboot_module_phys++;
            free_phys_addr((vaddr_t)multiboot_module, sizeof(multiboot_module));
        }
    }
}  

void load_single_module(Elf32_Ehdr * elf_header) 
{
    Elf32_Phdr * program_header;
    dword i, j;
    dword pages;

    struct task * task = NULL;
    vaddr_t address, base;

//    kprintf("[ehdr: %x]", elf_header);
    kernel_identity_map(elf_header, sizeof(elf_header));

    if (elf_header->e_magic == ELF_MAGIC) {
        
        program_header = (Elf32_Phdr *) ((dword) elf_header + elf_header->e_phoff);

//        kprintf("    EntryCount: %d", elf_header->e_phnum);
//        kprintf("    Offset: 0x%x\n", elf_header->e_phoff);

        for (i = 0; i < elf_header->e_phnum; i++, program_header++) {
            kernel_identity_map(program_header, sizeof(program_header));

//            kprintf(" ph @ %x, type=%d\n", program_header, program_header->p_type);
            if (program_header->p_type == 1) {
                
              	if ((elf_header->e_entry >= program_header->p_vaddr) && (elf_header->e_entry <= program_header->p_vaddr + program_header->p_memsz)) {
//                    kprintf("Neuen Task erstellen\n");
                    task = create_task((vaddr_t) elf_header->e_entry);
                } else if (task == NULL) {
					// FIXME
					kprintf("Kein Task: phdr->vaddr = %x, ehdr->entry = %x\nDas Modul ist entweder beschaedigt oder wird von diesem Kernel nicht unterstuetzt (Entry Point muss im ersten Program Header liegen)\n", program_header->p_vaddr, elf_header->e_entry);
                    continue;
                }
                
                // Pages erstellen
                pages = program_header->p_memsz / 0x1000;
                if (program_header->p_memsz % 0x1000) {
                    pages++;
                }
                
//                kprintf("Mapping von %d Pages\n", pages);
                base = (vaddr_t) PAGE_ALIGN_ROUND_DOWN(program_header->p_vaddr);
                if (base < MODULE_ENTRY) {
                    base = MODULE_ENTRY;
                }

                for (j = 0; j < pages; j++) {
                    paddr_t curPage = phys_alloc_page();
//                    kprintf("  %x => %x", (vaddr_t) (base + 0x1000 * j), curPage);
                    map_page((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j), curPage, PTE_P | PTE_U | PTE_W);
                }

                // Programm kopieren
//                kprintf("Programm kopieren\n");
                pages = program_header->p_filesz / 0x1000;
                if (program_header->p_filesz % 0x1000) {
                    pages++;
                }
//                kprintf("%d Pages zu kopieren\n", pages);

                address = (vaddr_t) ((dword) elf_header + program_header->p_offset);
                for (j = 0; j < pages; j++) {
                    kernel_identity_map(address, 0x1000);
                    kernel_identity_map(resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j)), 0x1000);
//                    kprintf("  Kopiere 4K nach %x von %x\n", (base + 0x1000 * j), address);
                    memcpy(resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j)), address, 0x1000);
                    address += 0x1000;                
                }

                if (program_header->p_filesz % 0x1000) {
                    // Es ist ein Teilstück einer Page übrig, das mit Nullen gefüllt wird
//                    kprintf("  Setze Nullen in 0x%x (0x%x Stück)\n", (vaddr_t) (base + 0x1000 * (j - 1)) + (program_header->p_filesz % 0x1000), 0x1000 - (program_header->p_filesz % 0x1000));
                    memset(resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * (j - 1))) + (program_header->p_filesz % 0x1000), 0, 0x1000 - (program_header->p_filesz % 0x1000));
                }
                
                // Wenn memsz > filesz. muß mit Nullen aufgefüllt werden
                pages = program_header->p_memsz / 0x1000;
                if (program_header->p_memsz % 0x1000) {
                    pages++;
                }
                for (; j < pages; j++) {
                    kernel_identity_map(resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j)), 0x1000);
//                    kprintf("  Setze Nullen in %x\n", (base + 0x1000 * j));
                    memset(resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (base + 0x1000 * j)), 0, 0x1000);
                    address += 0x1000;                
                }
                
                
//                kprintf("    Modul geladen.\n");
            }
        }
      
    } else {
        kprintf("ELF-Magic ungueltig (0x%x)\n",elf_header->e_magic);
    }

}
