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
#include <stdlib.h>
#include <stdbool.h>
#include <page.h>
#include <elf_common.h>
#include <elf32.h>
#include <errno.h>
#include <loader.h>
#include <lost/config.h>
#include <sys/queue.h>

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

#if CONFIG_ARCH == ARCH_AMD64

bool loader_elf32_load_image(pid_t process, vaddr_t image_start,
    size_t image_size)
{
	return false;
}

#else

struct elf32_image {
    Elf32_Ehdr*                 header;
    size_t                      size;

    uintptr_t                   min_addr;
    size_t                      num_pages;
    uint8_t*                    child_mem;

    SLIST_ENTRY(elf32_image)    next;
};

struct elf32_loader_state {
    pid_t                       process;
    SLIST_HEAD(, elf32_image)   images;
    struct elf32_image*         last;
};

static int queue_image(struct elf32_loader_state* s,
                       uint8_t* image_start, size_t image_size, int type)
{
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*) image_start;
    Elf32_Phdr* program_header;
    struct elf32_image* im;
    uintptr_t min_addr = -1;
    uintptr_t max_addr = 0;
    int i;

    /* Nur gültige ELF-Images vom richtigen Typ laden */
    if (elf_header->e_magic != ELF_MAGIC || elf_header->e_type != ET_EXEC) {
        return -EINVAL;
    }

    /* In die Liste einfügen */
    im = malloc(sizeof(*im));

    if (im == NULL) {
        return -ENOMEM;
    }
    *im = (struct elf32_image) {
        .header = elf_header,
        .size   = image_size,
    };
    SLIST_INSERT_HEAD(&s->images, im, next);
    s->last = im;

    /* Erster Program Header, dieser wird von e_phnum weiteren PHs gefolgt */
    program_header = (Elf32_Phdr*) (image_start + elf_header->e_phoff);

    /* Größe des gesamten Zielspeichers bestimmen */
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

    im->num_pages = (max_addr >> PAGE_SHIFT) - (min_addr >> PAGE_SHIFT) + 1;
    im->min_addr = min_addr & PAGE_MASK;

    return 0;
}

static int create_flat_image(struct elf32_loader_state* s,
                             struct elf32_image* im)
{
    Elf32_Phdr* program_header;
    uint8_t* src_mem = (uint8_t*) im->header;
    uint8_t* child_mem;
    int i;

    /* Speicher für dieses Image anfordern */
    child_mem = loader_allocate_mem(im->num_pages * PAGE_SIZE);
    if (child_mem == NULL) {
        return -ENOMEM;
    }
    memset(child_mem, 0, im->num_pages * PAGE_SIZE);

    /* Jetzt werden die einzelnen Program-Header geladen */
    program_header = (Elf32_Phdr*) (src_mem + im->header->e_phoff);
    for (i = 0; i < im->header->e_phnum; i++) {
        /* Nur ladbare PHs laden ;-) */
        if (program_header->p_type == PT_LOAD) {
            memcpy(
                &child_mem[program_header->p_vaddr - im->min_addr],
                &src_mem[program_header->p_offset],
                program_header->p_filesz);
        }
        program_header++;
    }

    im->child_mem = child_mem;
    return 0;
}

static int assign_flat_image(struct elf32_loader_state* s,
                             struct elf32_image* im)
{
    int ret;

    // Den geladenen Speicher in den Adressraum des Prozesses
    // verschieben
    ret = loader_assign_mem(s->process, (void*) im->min_addr, im->child_mem,
                            im->num_pages * PAGE_SIZE);
    if (!ret) {
        /* FIXME child_mem wird geleakt */
        return -EFAULT;
    }

    return 0;
}

/**
 * Laedt ein ELF-32-Image
 */
bool loader_elf32_load_image(pid_t process, vaddr_t image_start,
    size_t image_size)
{
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*) image_start;
    struct elf32_loader_state s = {
        .process    = process,
        .images     = SLIST_HEAD_INITIALIZER(&images),
        .last       = NULL,
    };
    struct elf32_image* im;
    int ret;

    /* Das Hauptimage selber in die Liste eintragen */
    ret = queue_image(&s, image_start, image_size, ET_EXEC);
    if (ret < 0) {
        return false;
    }

    /* Hauptthread erstellen */
    loader_create_thread(process, (vaddr_t) elf_header->e_entry);

    /* Und jetzt alles laden */
    SLIST_FOREACH(im, &s.images, next) {
        ret = create_flat_image(&s, im);
        if (ret < 0) {
            /* FIXME Der komplette neue Prozess wird geleakt */
            goto fail;
        }

        ret = assign_flat_image(&s, im);
        if (ret < 0) {
            /* FIXME Der komplette neue Prozess wird geleakt */
            goto fail;
        }
    }

    ret = 0;
fail:
    SLIST_FOREACH_SAFE(im, &s.images, next) {
        free(im);
    }

    return (ret == 0);
}

#endif
