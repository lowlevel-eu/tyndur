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
    uintptr_t                   reloc_addr;
    size_t                      num_pages;
    uint8_t*                    child_mem;

    const char*                 strtab;
    size_t                      strtabsz;

    Elf32_Sym*                  symtab;
    uint32_t*                   hashtab;
    uint32_t                    hashtab_nbucket;
    uint32_t                    hashtab_nchain;

    void*                       jmprel;
    uint32_t                    pltrel;
    uint32_t                    pltrelsz;

    void*                       rel;
    uint32_t                    relsz;

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
    struct elf32_image* last;
    uintptr_t min_addr = -1;
    uintptr_t max_addr = 0;
    int i;

    /* Nur gültige ELF-Images vom richtigen Typ laden */
    if (elf_header->e_magic != ELF_MAGIC || elf_header->e_type != type) {
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

    if (!s->last) {
        SLIST_INSERT_HEAD(&s->images, im, next);
    } else {
        SLIST_INSERT_AFTER(s->last, im, next);
    }
    last = s->last;
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

    if (elf_header->e_type == ET_DYN) {
        im->reloc_addr = last->min_addr + (last->num_pages * PAGE_SIZE);
    } else {
        im->reloc_addr = im->min_addr;
    }

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

static const char* get_string(struct elf32_image* im, int index)
{
    if (index > im->strtabsz) {
        return NULL;
    }

    return im->strtab + index;
}

static int handle_dynamic_ph(struct elf32_loader_state* s,
                             struct elf32_image* im)
{
    uint8_t* src_mem = (uint8_t*) im->header;
    size_t child_memsz = im->num_pages * PAGE_SIZE;
    Elf32_Phdr* program_header;
    Elf32_Dyn* dyn = NULL;
    Elf32_Dyn* p;
    uint32_t strtab = 0;
    uint32_t symtab = 0;
    uint32_t hashtab = 0;
    uint32_t jmprel = 0;
    uint32_t rel = 0;
    uint32_t strtabsz = 0;
    uint32_t pltrel = 0;
    uint32_t pltrelsz = 0;
    uint32_t relsz = 0;
    char* str;
    int i, ret;

    /* DYNAMIC-Program-Header suchen */
    program_header = (Elf32_Phdr*) (src_mem + im->header->e_phoff);

    for (i = 0; i < im->header->e_phnum; i++) {
        if (program_header[i].p_type == PT_DYNAMIC) {
            dyn = (Elf32_Dyn*) (src_mem + program_header[i].p_offset);
            break;
        }
    }
    if (dyn == NULL) {
        return 0;
    }

    /* Diverse Pointer und Längen aus dem PH lesen */
    for (p = dyn; p->d_tag != DT_NULL; p++) {
        switch (p->d_tag) {
            case DT_HASH:
                hashtab = p->d_un.d_ptr - im->min_addr;
                break;
            case DT_STRTAB:
                strtab = p->d_un.d_ptr - im->min_addr;
                break;
            case DT_STRSZ:
                strtabsz = p->d_un.d_val;
                break;
            case DT_SYMTAB:
                symtab = p->d_un.d_ptr - im->min_addr;
                break;
            case DT_JMPREL:
                jmprel = p->d_un.d_ptr - im->min_addr;
                break;
            case DT_PLTREL:
                pltrel = p->d_un.d_val;
                break;
            case DT_PLTRELSZ:
                pltrelsz = p->d_un.d_val;
                break;
            case DT_REL:
                rel = p->d_un.d_ptr - im->min_addr;
                break;
            case DT_RELSZ:
                relsz = p->d_un.d_val;
                break;
        }
    }

    /* Stringtabelle */
    if (strtabsz > UINT32_MAX - strtab || strtab + strtabsz > child_memsz) {
        return -EINVAL;
    }

    str = (char*) &im->child_mem[strtab];
    if (str[strtabsz - 1] != '\0') {
        return -EINVAL;
    }
    im->strtab = str;
    im->strtabsz = strtabsz;

    /* (Dynamische) Symboltabelle */
    if (symtab > child_memsz) {
        return -EINVAL;
    }
    im->symtab = (Elf32_Sym*) (im->child_mem + symtab);

    if (hashtab > child_memsz - 8) {
        return -EINVAL;
    }

    /* Hashtabelle für Symbolname -> Symboltabellenindex */
    im->hashtab = (uint32_t*) (im->child_mem + hashtab);
    im->hashtab_nbucket = im->hashtab[0];
    im->hashtab_nchain = im->hashtab[1];

    if (im->hashtab_nbucket > UINT32_MAX - im->hashtab_nchain ||
        im->hashtab_nbucket + im->hashtab_nchain > (child_memsz - hashtab) / 4)
    {
        return -EINVAL;
    }

    /* Relokationstabelle JMPREL */
    if (pltrelsz > UINT32_MAX - jmprel || jmprel + pltrelsz > child_memsz) {
        return -EINVAL;
    }
    if (jmprel) {
        im->jmprel = (uint32_t*) (im->child_mem + jmprel);
        im->pltrel = pltrel;
        im->pltrelsz = pltrelsz;
    } else {
        im->jmprel = NULL;
    }

    /* Relokationstabelle REL */
    if (relsz > UINT32_MAX - rel || rel + relsz > child_memsz) {
        return -EINVAL;
    }
    if (rel) {
        im->rel = (uint32_t*) (im->child_mem + rel);
        im->relsz = relsz;
    } else {
        im->rel = NULL;
    }

    /* Schauen, ob wir Shared Libs dazuladen müssen */
    for (p = dyn; p->d_tag != DT_NULL; p++) {
        if (p->d_tag == DT_NEEDED) {
            uint32_t libname_off = p->d_un.d_val;
            void* lib_image;
            size_t lib_size;

            if (libname_off > strtabsz) {
                return -EINVAL;
            }

            ret = loader_get_library(str + libname_off, &lib_image, &lib_size);
            if (ret < 0) {
                return ret;
            }

            ret = queue_image(s, lib_image, lib_size, ET_DYN);
            if (ret < 0) {
                return ret;
            }
        }
    }

    return 0;
}

/* Direkt aus der Spec übernommen */
static uint32_t elf32_hash(const char* name)
{
    uint32_t h = 0, g;

    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000)) {
            h ^= g >> 24;
        }
        h &= ~g;
    }

    return h;
}

static int lookup_symbol(struct elf32_loader_state* s,
                         struct elf32_image* for_im,
                         const char* name,
                         Elf32_Sym** result,
                         struct elf32_image** provider)
{
    struct elf32_image* im;
    uint32_t hash = elf32_hash(name);
    uint32_t idx;

    SLIST_FOREACH(im, &s->images, next) {
        uint32_t nbucket = im->hashtab_nbucket;
        uint32_t nchain = im->hashtab_nchain;
        uint32_t* buckets = &im->hashtab[2];
        uint32_t* chain = &im->hashtab[2 + nbucket];
        Elf32_Sym* sym;
        const char* sym_name;

        idx = buckets[hash % nbucket];
        while (idx != STN_UNDEF) {
            if (idx > nchain) {
                return -EINVAL;
            }

            sym = &im->symtab[idx];
            sym_name = get_string(im, sym->st_name);
            if (sym_name == NULL) {
                return -EINVAL;
            }
            if (!strcmp(sym_name, name)) {
                if (sym->st_shndx != SHN_UNDEF) {
                    *result = sym;
                    if (provider) {
                        *provider = im;
                    }
                    return 0;
                } else {
                    /* Symbol ist zwar drin, aber leider undefiniert */
                    break;
                }
            }
            idx = chain[idx];
        }
    }

    return -ENOENT;
}

static int lookup_undef_symbol(struct elf32_loader_state* s,
                               struct elf32_image* im,
                               int symtab_idx,
                               Elf32_Sym** result,
                               struct elf32_image** provider)
{
    const char* name;

    if (symtab_idx > im->hashtab_nchain) {
        return -EINVAL;
    }

    name = get_string(im, im->symtab[symtab_idx].st_name);
    if (name == NULL) {
        return -EINVAL;
    }

    return lookup_symbol(s, im, name, result, provider);
}

static int lookup_undef_symbol_addr(struct elf32_loader_state* s,
                                    struct elf32_image* im,
                                    int symtab_idx,
                                    uintptr_t* result)
{
    struct elf32_image* provider;
    Elf32_Sym* sym;
    int ret;

    ret = lookup_undef_symbol(s, im, symtab_idx, &sym, &provider);
    if (ret < 0) {
        return ret;
    }

    *result = (provider->reloc_addr - provider->min_addr) + sym->st_value;
    return 0;
}

static int relocate_rel(struct elf32_loader_state* s,
                        struct elf32_image* im,
                        Elf32_Rel* rel, size_t bytes)
{
    Elf32_Rel* end = rel + (bytes / sizeof(*rel));

    while (rel < end) {
        uint32_t* reloc = (uint32_t*) (im->child_mem +
                                       rel->r_offset - im->min_addr);
        uint32_t symbol = ELF32_R_SYM(rel->r_info);
        uintptr_t new_addr;
        int ret;

        switch (ELF32_R_TYPE(rel->r_info)) {
            case R_386_JUMP_SLOT:
            case R_386_GLOB_DAT:
                ret = lookup_undef_symbol_addr(s, im, symbol, &new_addr);
                if (ret < 0) {
                    return ret;
                }
                break;
            case R_386_32:
                ret = lookup_undef_symbol_addr(s, im, symbol, &new_addr);
                if (ret < 0) {
                    return ret;
                }
                new_addr += *reloc;
                break;
            case R_386_PC32:
                ret = lookup_undef_symbol_addr(s, im, symbol, &new_addr);
                if (ret < 0) {
                    return ret;
                }
                new_addr -= (rel->r_offset + im->reloc_addr);
                new_addr += *reloc;
                break;
            case R_386_RELATIVE:
                new_addr = im->reloc_addr;
                new_addr += *reloc;
                break;
            case R_386_NONE:
                new_addr = *reloc;
                break;
            case R_386_COPY:
            {
                struct elf32_image* provider;
                Elf32_Sym* sym;
                void* src;

                ret = lookup_undef_symbol(s, im, symbol, &sym, &provider);
                if (ret < 0) {
                    return ret;
                }

                src = provider->child_mem + sym->st_value - provider->min_addr;
                memcpy(reloc, src, sym->st_size);
                break;
            }
            default:
                /* TODO */
                return -ENOSYS;
        }

        if (ELF32_R_TYPE(rel->r_info) != R_386_COPY) {
            *reloc = new_addr;
        }
        rel++;
    }

    return 0;
}

static int relocate(struct elf32_loader_state* s,
                    struct elf32_image* im)
{
    int ret;

    if (im->jmprel) {
        if (im->pltrel != DT_REL) {
            /* TODO DT_RELA */
            return -ENOSYS;
        }
        ret = relocate_rel(s, im, im->jmprel, im->pltrelsz);
        if (ret < 0) {
            return ret;
        }
    }

    if (im->rel) {
        ret = relocate_rel(s, im, im->rel, im->relsz);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

static int assign_flat_image(struct elf32_loader_state* s,
                             struct elf32_image* im)
{
    int ret;

    // Den geladenen Speicher in den Adressraum des Prozesses
    // verschieben
    ret = loader_assign_mem(s->process, (void*) im->reloc_addr, im->child_mem,
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

    /* Alle Bibliotheken laden */
    SLIST_FOREACH(im, &s.images, next) {
        ret = create_flat_image(&s, im);
        if (ret < 0) {
            goto fail;
        }

        ret = handle_dynamic_ph(&s, im);
        if (ret < 0) {
            goto fail;
        }
    }

    /* Dynamische Relokationen durchführen, wenn nötig */
    SLIST_FOREACH(im, &s.images, next) {
        ret = relocate(&s, im);
        if (ret < 0) {
            goto fail;
        }
    }

    /* Hauptthread erstellen */
    loader_create_thread(process, (vaddr_t) elf_header->e_entry);

    /* Speicher dem neuen Thread zuweisen. im->child_mem ist anschließend
     * in diesem Prozess nicht mehr gemappt. */
    SLIST_FOREACH(im, &s.images, next) {
        ret = assign_flat_image(&s, im);
        if (ret < 0) {
            goto fail;
        }
    }

    ret = 0;
fail:
    /* FIXME Im Fehlerfall wird der komplette neue Prozess geleakt */
    SLIST_FOREACH_SAFE(im, &s.images, next) {
        if (im->header->e_type == ET_DYN) {
            free(im->header);
        }
        free(im);
    }

    return (ret == 0);
}

#endif
