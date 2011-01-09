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
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <page.h>
#include <elf_common.h>
#include <elf32.h>
#include <loader.h>
#include <lost/config.h>
#define ELF_MAGIC (ELFMAG0 | (ELFMAG1 << 8) | (ELFMAG2 << 16) | (ELFMAG3 << 24))

/**
 * Ueberprueft ob die Datei vom Typ ELF32 ist
 */
bool loader_is_elf32(vaddr_t image_start, size_t image_size)
{
    // Wenn die Datei kleiner ist als ein ELF-Header, ist der Fall klar
    if (image_size < sizeof(Elf32_Ehdr)) {
        return false;
    }
    
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*) image_start;

    // Wenn die ELF-Magic nicht stimmt auch
    if (elf_header->e_magic != ELF_MAGIC) {
        return false;
    }

    // Jetzt muss nur noch sichergestellt werden, dass es sich um 32-Bit ELFs
    // handelt
    if (elf_header->e_ident[EI_CLASS] != ELFCLASS32) {
        return false;
    }

    return true;
}


/**
 * Laedt ein ELF-32-Image
 */
bool loader_elf32_load_image(pid_t process, vaddr_t image_start,
    size_t image_size)
{

#if CONFIG_ARCH == ARCH_AMD64
	return false;
#else
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*) image_start;

    // Nur gueltige ELF-Images laden
    if (elf_header->e_magic != ELF_MAGIC) {
        return false;
    }

    // Hauptthread erstellen
    loader_create_thread(process, (vaddr_t) elf_header->e_entry);

    // Pointer auf den ersten Program-Header, dieser wird von e_phnum weiteren
    // PHs gefolgt
    Elf32_Phdr* program_header = (Elf32_Phdr*) (((uintptr_t) elf_header) +
        elf_header->e_phoff);

    // Groesse des gesamten Zielspeichers bestimmen
    uintptr_t min_addr = -1;
    uintptr_t max_addr = 0;
    size_t num_pages;
    int i;

    for (i = 0; i < elf_header->e_phnum; i++) {
        if (program_header[i].p_type != PT_LOAD) {
            continue;
        }
        if (program_header[i].p_vaddr < min_addr) {
            min_addr = program_header[i].p_vaddr;
        }
        if (program_header[i].p_vaddr + program_header[i].p_memsz > max_addr) {
            max_addr = program_header[i].p_vaddr + program_header[i].p_memsz;
        }
    }

    num_pages = (max_addr >> PAGE_SHIFT) - (min_addr >> PAGE_SHIFT) + 1;
    min_addr &= PAGE_MASK;

    // Jetzt werden die einzelnen Program-Header geladen
    uint8_t* src_mem = image_start;
    uint8_t* child_mem = loader_allocate_mem(num_pages * PAGE_SIZE);
    memset(child_mem, 0, num_pages * PAGE_SIZE);

    for (i = 0; i < elf_header->e_phnum; i++) {
        // Nur ladbare PHs laden ;-)
        if (program_header->p_type == PT_LOAD) {
            memcpy(
                &child_mem[program_header->p_vaddr - min_addr],
                &src_mem[program_header->p_offset],
                program_header->p_filesz);
        }
        program_header++;
    }

    // Den geladenen Speicher in den Adressraum des Prozesses
    // verschieben
    loader_assign_mem(process, (void*) min_addr, child_mem,
        num_pages * PAGE_SIZE);

    return true;
#endif
}


