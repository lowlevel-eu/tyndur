/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <stdint.h>
#include <syscall.h>
#include <rpc.h>
#include <sleep.h>

#include "cdi.h"
#include "cdi/misc.h"

// TODO Bei mehr als einem registrierten IRQ geht das so nicht mehr
static void (*driver_irq_handler)(struct cdi_device*) = NULL;
static struct cdi_device* driver_irq_device = NULL;

/**
 * Interner IRQ-Handler, der den IRQ-Handler des Treibers aufruft
 */
static void irq_handler() 
{
    driver_irq_handler(driver_irq_device);
}

/**
 * Registiert einen neuen IRQ-Handler.
 *
 * @param irq Nummer des zu reservierenden IRQ
 * @param handler Handlerfunktion
 * @param device Geraet, das dem Handler als Parameter uebergeben werden soll
 */
void cdi_register_irq(uint8_t irq, void (*handler)(struct cdi_device*), 
    struct cdi_device* device)
{
    driver_irq_handler = handler;
    driver_irq_device = device;

    register_intr_handler(0x20 + irq, irq_handler);
}
    
/**
 * Reserviert physisch zusammenhaengenden Speicher.
 *
 * @param size Groesse des benoetigten Speichers in Bytes
 * @param vaddr Pointer, in den die virtuelle Adresse des reservierten
 * Speichers geschrieben wird.
 * @param paddr Pointer, in den die physische Adresse des reservierten
 * Speichers geschrieben wird.
 *
 * @return 0 wenn der Speicher erfolgreich reserviert wurde, -1 sonst
 */
int cdi_alloc_phys_mem(size_t size, void** vaddr, void** paddr)
{
    dma_mem_ptr_t buffer = mem_dma_allocate(size, 0x80);

    if (buffer.virt == NULL) {
        return -1;
    }

    *vaddr = buffer.virt;
    *paddr = buffer.phys;

    return 0;
}

/**
 * Reserviert IO-Ports
 *
 * @return 0 wenn die Ports erfolgreich reserviert wurden, -1 sonst.
 */
int cdi_ioports_alloc(uint16_t start, uint16_t count)
{
    return (request_ports(start, count) ? 0 : -1);
}

/**
 * Gibt reservierte IO-Ports frei
 *
 * @return 0 wenn die Ports erfolgreich freigegeben wurden, -1 sonst.
 */
int cdi_ioports_free(uint16_t start, uint16_t count)
{
    return (release_ports(start, count) ? 0 : -1);
}

/**
 * Unterbricht die Ausfuehrung fuer mehrere Millisekunden
 */
void cdi_sleep_ms(uint32_t ms)
{
    msleep(ms);
}


