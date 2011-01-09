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
#include <stdbool.h>
#include <string.h>
#include <page.h>
#include <elf_common.h>
#include <elf64.h>
#include <loader.h>

#include <lost/config.h>
#define ELF_MAGIC (ELFMAG0 | (ELFMAG1 << 8) | (ELFMAG2 << 16) | (ELFMAG3 << 24))

/**
 * Ueberprueft ob die Datei vom Typ ELF64 ist
 */
bool loader_is_elf64(vaddr_t image_start, size_t image_size)
{
    // Wenn die Datei kleiner ist als ein ELF-Header, ist der Fall klar
    if (image_size < sizeof(Elf64_Ehdr)) {
        return false;
    }
    
    Elf64_Ehdr* elf_header = (Elf64_Ehdr*) image_start;

    // Wenn die ELF-Magic nicht stimmt auch
    if (elf_header->e_magic != ELF_MAGIC) {
        return false;
    }

    // Jetzt muss nur noch sichergestellt werden, dass es sich um 64-Bit ELFs
    // handelt
    if (elf_header->e_ident[EI_CLASS] != ELFCLASS64) {
        return false;
    }

    return true;
}


/**
 * Laed ein ELF-64-Image
 */
bool loader_elf64_load_image(pid_t process, vaddr_t image_start,
    size_t image_size)
{
#if CONFIG_ARCH == ARCH_I386
    return false;
#else
    Elf64_Ehdr* elf_header = (Elf64_Ehdr*) image_start;
    // Nur gueltige ELF-Images laden
    if (elf_header->e_magic != ELF_MAGIC) {
        return false;
    }
    
    loader_create_thread(process, (vaddr_t) elf_header->e_entry);
    
    // FIXME: Das gibt bei i386 viele warnungen ;-)
    
    // Pointer auf den ersten Program-Header, dieser wird von e_phnum weiteren
    // PHs gefolgt
    Elf64_Phdr* program_header = (Elf64_Phdr*) ((uintptr_t) elf_header +
        elf_header->e_phoff);

    // Jetzt werden die einzelnen Program-Header geladen
    int i;
    for (i = 0; i < elf_header->e_phnum; i++) {
        // Nur ladbare PHs laden ;-)
        if (program_header->p_type == PT_LOAD) {
            // Speicher allokieren, daten kopieren und anschliessend den
            // Beriech zwischen filesz und memsz mit Nullen auffuellen
            size_t page_count = NUM_PAGES(PAGE_ALIGN_ROUND_UP(
                program_header->p_memsz));
            vaddr_t mem_image = loader_allocate_mem(page_count * PAGE_SIZE);
            memcpy((vaddr_t) ((uintptr_t) mem_image + program_header->p_offset
                % PAGE_SIZE), (vaddr_t) ((uintptr_t) elf_header +
                program_header->p_offset), program_header->p_filesz);
            memset((vaddr_t) ((uintptr_t) mem_image + program_header->p_offset
                % PAGE_SIZE +  program_header->p_filesz), 0, program_header->
                p_memsz - program_header->p_filesz);

            // Den geladenen Speicher in den Adressraum des Prozesses
            // verschieben
            loader_assign_mem(process, (vaddr_t) program_header->p_vaddr, mem_image,
                page_count * PAGE_SIZE);
        }
    }

    return true;
#endif
}

