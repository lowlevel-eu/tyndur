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
#include <elf64.h>
#include <multiboot.h>
#include <string.h>

#define ELF_MAGIC 0x464C457F
#define MULTIBOOT_MAGIC 0x2BADB002
void print(const char* str);

/**
 * Hauptfunktion des Loaders.
 */
void loader(uint64_t multiboot_info_addr)
{
    extern void* _binary_kernel_start;
    Elf64_Ehdr* elf_header = (Elf64_Ehdr*) (&_binary_kernel_start);
    Elf64_Phdr* program_header;

    // Ueberpruefen ob die Magic-Number des Kernels korrekt ist.
    if (elf_header->e_magic == ELF_MAGIC) {
        program_header = (Elf64_Phdr*) ((uintptr_t) elf_header + 
            elf_header->e_phoff);
        
        uintptr_t entry = elf_header->e_entry;

        int i;
        for (i = 0; i < elf_header->e_phnum; i++, program_header++) {
            memcpy((void*) program_header->p_vaddr, (void*)
                ((uintptr_t) elf_header + program_header->p_offset), 
                program_header->p_filesz);

            memset((void*) (program_header->p_vaddr + program_header->p_filesz),
                0, program_header->p_memsz - program_header->p_filesz);
        }

        asm("call *%2;" : : "a" (MULTIBOOT_MAGIC), "b" (multiboot_info_addr), "D" (entry));
    } else {
        print("Die ELF-Magic des dazugelinkten Kernels ist ungueltig!");
    }

}


/**
 * Eine Zeichenkette anzeigen.
 *
 * @param str Pointer auf die Zeichenkette.
 */
void print(const char* str)
{
    char* video_memory = (char*) 0xB8000;
    static uintptr_t offset = 0;

    while (*str != '\0') {
        if (*str == '\n') {
            offset += 80 - (offset % 80);
        } else {
            video_memory[offset * 2] = *str;
            video_memory[offset * 2 + 1] = (char) 0x7;
            offset++;
        }

        str++;
    }
}


