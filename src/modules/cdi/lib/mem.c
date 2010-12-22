/*
 * Copyright (c) 2010 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <stdint.h>
#include <stdlib.h>
#include <syscall.h>
#include <string.h>

#include "cdi.h"
#include "cdi/mem.h"

/**
 * \german
 * Reserviert einen Speicherbereich.
 *
 * @param size Größe des Speicherbereichs in Bytes
 * @param flags Flags, die zusätzliche Anforderungen beschreiben
 * \endgerman
 * \english
 * Allocates a memory area.
 *
 * @param size Size of the memory area in bytes
 * @param flags Flags that describe additional requirements
 * \endenglish
 */
struct cdi_mem_area* cdi_mem_alloc(size_t size, cdi_mem_flags_t flags)
{
    dma_mem_ptr_t buffer;
    struct cdi_mem_area* area;
    struct cdi_mem_sg_item* sg_item;
    size_t alignment = flags & CDI_MEM_ALIGN_MASK;
    size_t asize;

    // Wir vergeben nur ganze Pages
    size = (size + 0xFFF) & (~0xFFF);
    asize = size;

    // Wenn die physische Adresse nicht intressiert, koennen wir das Alignment
    // ignorieren.
    if ((flags & CDI_MEM_VIRT_ONLY)) {
        alignment = 0;
    }

    // Quick and dirty Loesung fuer das allozieren von Speicherbereichen mit
    // einem Alignment von mehr als Page-Size: Wir nehmen einfach die Groesse
    // plus einmal das Alignment, dann finden wir sicher ein Stueck mit
    // passendem Alignment innerhalb des allozierten Bereichs. Den Rest geben
    // wir wieder frei.
    if (alignment > 12) {
        // TODO: Momentan unterstuetzen wir das Alignment nur fuer
        // physisch zusammenhaengende Bereiche.
        if (!(flags & CDI_MEM_PHYS_CONTIGUOUS)) {
            return NULL;
        }

        asize = size + (1ULL << alignment);
    }

    // Speicher holen
    if (flags & CDI_MEM_VIRT_ONLY) {
        buffer = mem_dma_allocate(size, 0);
    } else {
        buffer = mem_dma_allocate(asize, 0x80);
    }

    if (buffer.virt == NULL) {
        return NULL;
    }

    // Wie oben erwaehnt alignment umsetzen
    if (alignment > 12) {
        uintptr_t amask = (1ULL << alignment) - 1;
        uintptr_t astart = ((uintptr_t) buffer.phys + amask) & (~amask);
        uintptr_t off = astart - (uintptr_t) buffer.phys;

        // Die beiden Stuecke vor und nach unserem ausgeschnittenen Bereich
        // freigeben.
        if (off) {
            mem_free(buffer.virt, off);
        }
        if (asize - size - off) {
            mem_free((vaddr_t) (((uintptr_t) buffer.virt) + size + off),
                asize - size - off);
        }

        buffer.phys = (paddr_t) astart;
        buffer.virt = (vaddr_t) (((uintptr_t) buffer.virt) + off);
    }

    // cdi_mem_area anlegen und befuellen
    sg_item = malloc(sizeof(*sg_item));
    *sg_item = (struct cdi_mem_sg_item) {
        .start  = (uintptr_t) buffer.phys,
        .size   = size,
    };

    area = malloc(sizeof(*area));
    *area = (struct cdi_mem_area) {
        .size   = size,
        .vaddr  = buffer.virt,
        .paddr  = {
            .num    = 1,
            .items  = sg_item,
        },
    };

    return area;
}

/**
 * Reserviert physisch zusammenhägenden Speicher an einer definierten Adresse
 *
 * @param paddr Physische Adresse des angeforderten Speicherbereichs
 * @param size Größe des benötigten Speichers in Bytes
 *
 * @return Eine cdi_mem_area bei Erfolg, NULL im Fehlerfall
 */
struct cdi_mem_area* cdi_mem_map(uintptr_t paddr, size_t size)
{
    struct cdi_mem_area* area;
    struct cdi_mem_sg_item* sg_item;
    void* vaddr = mem_allocate_physical(size, paddr, 0);

    if (vaddr == NULL) {
        return NULL;
    }

    // cdi_mem_area anlegen und befuellen
    sg_item = malloc(sizeof(*sg_item));
    *sg_item = (struct cdi_mem_sg_item) {
        .start  = paddr,
        .size   = size,
    };

    area = malloc(sizeof(*area));
    *area = (struct cdi_mem_area) {
        .size   = size,
        .vaddr  = vaddr,
        .paddr  = {
            .num    = 1,
            .items  = sg_item,
        },
        .osdep = {
            .mapped = 1,
        },
    };

    return area;
}


/**
 * \german
 * Gibt einen durch cdi_mem_alloc oder cdi_mem_map reservierten Speicherbereich
 * frei
 * \endgerman
 * \english
 * Frees a memory area that was previously allocated by cdi_mem_alloc or
 * cdi_mem_map
 * \endenglish
 */
void cdi_mem_free(struct cdi_mem_area* p)
{
    if (p->osdep.mapped) {
        mem_free_physical(p->vaddr, p->size);
    } else {
        mem_free(p->vaddr, p->size);
    }

    free(p->paddr.items);
    free(p);
}

/**
 * \german
 * Gibt einen Speicherbereich zurück, der dieselben Daten wie @a p beschreibt,
 * aber mindestens die gegebenen Flags gesetzt hat.
 *
 * Diese Funktion kann denselben virtuellen und physischen Speicherbereich wie
 * @p benutzen oder sogar @p selbst zurückzugeben, solange der gemeinsam
 * benutzte Speicher erst dann freigegeben wird, wenn sowohl @a p als auch der
 * Rückgabewert durch cdi_mem_free freigegeben worden sind.
 *
 * Ansonsten wird ein neuer Speicherbereich reserviert und (außer wenn das
 * Flag CDI_MEM_NOINIT gesetzt ist) die Daten werden aus @a p in den neu
 * reservierten Speicher kopiert.
 * \endgerman
 * \english
 * Returns a memory area that describes the same data as @a p does, but for
 * which at least the given flags are set.
 *
 * This function may use the same virtual and physical memory areas as used in
 * @a p, or it may even return @a p itself. In this case it must ensure that
 * the commonly used memory is only freed when both @a p and the return value
 * of this function have been freed by cdi_mem_free.
 *
 * Otherwise, a new memory area is allocated and data is copied from @a p into
 * the newly allocated memory (unless CDI_MEM_NOINIT is set).
 * \endenglish
 */
struct cdi_mem_area* cdi_mem_require_flags(struct cdi_mem_area* p,
    cdi_mem_flags_t flags)
{
    // TODO Das geht unter Umstaenden effizienter
    struct cdi_mem_area* new = cdi_mem_alloc(p->size, flags);
    if (new == NULL) {
        return NULL;
    }

    memcpy(new->vaddr, p->vaddr, new->size);
    return new;
}

/**
 * \german
 * Kopiert die Daten von @a src nach @a dest. Beide Speicherbereiche müssen
 * gleich groß sein.
 *
 * Das bedeutet nicht unbedingt eine physische Kopie: Wenn beide
 * Speicherbereiche auf denselben physischen Speicher zeigen, macht diese
 * Funktion nichts. Sie kann auch andere Methoden verwenden, um den Speicher
 * effektiv zu kopieren, z.B. durch geeignetes Ummappen von Pages.
 *
 * @return 0 bei Erfolg, -1 sonst
 * \endgerman
 * \english
 * Copies data from @a src to @a dest. Both memory areas must be of the same
 * size.
 *
 * This does not necessarily involve a physical copy: If both memory areas
 * point to the same physical memory, this function does nothing. It can also
 * use other methods to achieve the same visible effect, e.g. by remapping
 * pages.
 *
 * @return 0 on success, -1 otherwise
 * \endenglish
 */
int cdi_mem_copy(struct cdi_mem_area* dest, struct cdi_mem_area* src)
{
    // TODO Das geht unter Umstaenden effizienter
    if (dest->size != src->size) {
        return -1;
    }

    if (dest->vaddr != src->vaddr) {
        memcpy(dest->vaddr, src->vaddr, dest->size);
    }

    return 0;
}
