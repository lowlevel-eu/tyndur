/*
 * Copyright (c) 2008 Mathias Gottschlag
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <stdint.h>
#include <cdi/bios.h>
#include <syscall.h>
#include <stdlib.h>

/**
 * Ruft den BIOS-Interrupt 0x10 auf.
 * @param registers Pointer auf eine Register-Struktur. Diese wird beim Aufruf
 * in die Register des Tasks geladen und bei Beendigung des Tasks wieder mit den
 * Werten des Tasks gefuellt.
 * @param memory Speicherbereiche, die in den Bios-Task kopiert und bei Beendigung
 * des Tasks wieder zurueck kopiert werden sollen. Die Liste ist vom Typ struct
 * cdi_bios_memory.
 * @return 0, wenn der Aufruf erfolgreich war, -1 bei Fehlern
 */
int cdi_bios_int10(struct cdi_bios_registers *registers, cdi_list_t memory)
{
    // Speicherbereiche kopieren
    int memcount = 0;
    if (memory) {
        memcount = cdi_list_size(memory);
    }
    uint32_t meminfo[sizeof(uint32_t) * memcount * 3 + 1];
    meminfo[0] = memcount;
    int i;
    for (i = 0; i < memcount; i++) {
        struct cdi_bios_memory *memarea = cdi_list_get(memory, i);
        meminfo[1 + i * 3] = memarea->dest;
        meminfo[1 + i * 3 + 1] = (uint32_t)memarea->src;
        meminfo[1 + i * 3 + 2] = memarea->size;
    }

    // BIOS aufrufen
    return vm86_int((vm86_regs_t*)registers, meminfo) == 0 ? 0 : -1;
}

