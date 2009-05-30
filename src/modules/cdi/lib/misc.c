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
#include <stdio.h>

#include "cdi.h"
#include "cdi/misc.h"

/** Ab diesem Interrupt fangen bei tyndur die IRQs an */
#define TYNDUR_IRQ_OFFSET   0x20
/** Anzahl der verfuegbaren IRQs */
#define IRQ_COUNT           0x10

/** Array mit allen IRQ-Handlern; Als index wird die Nummer benutzt */
static void (*driver_irq_handler[IRQ_COUNT])(struct cdi_device*) = { NULL };
/** Array mit den passenden Geraeten zu den registrierten IRQs */
static struct cdi_device* driver_irq_device[IRQ_COUNT] = { NULL };
/**
 * Array, das die jeweilige Anzahl an aufgerufenen Interrupts seit dem
 * cdi_reset_wait_irq speichert.
 */
static uint8_t driver_irq_count[IRQ_COUNT] = { 0 };

/**
 * Interner IRQ-Handler, der den IRQ-Handler des Treibers aufruft
 */
static void irq_handler(uint8_t irq)
{
    // Ups, das war wohl kein IRQ, den wollen wir nicht.
    if ((irq < TYNDUR_IRQ_OFFSET) || (irq >= TYNDUR_IRQ_OFFSET + IRQ_COUNT)) {
        return;
    }

    irq -= TYNDUR_IRQ_OFFSET;
    driver_irq_count[irq]++;
    if (driver_irq_handler[irq]) {
        driver_irq_handler[irq](driver_irq_device[irq]);
    }
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
    if (irq >= IRQ_COUNT) {
        // FIXME: Eigentlich sollte diese Funktion etwas weniger optimistisch
        // sein, und einen Rueckgabewert haben.
        return;
    }

    // Der Interrupt wurde schon mal registriert
    if (driver_irq_handler[irq]) {
        fprintf(stderr, "cdi: Versuch IRQ %d mehrfach zu registrieren\n", irq);
        return;
    }

    driver_irq_handler[irq] = handler;
    driver_irq_device[irq] = device;

    register_intr_handler(TYNDUR_IRQ_OFFSET + irq, irq_handler);
}

/**
 * Setzt den IRQ-Zaehler fuer cdi_wait_irq zurueck.
 *
 * @param irq Nummer des IRQ
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_reset_wait_irq(uint8_t irq)
{
    if (irq > IRQ_COUNT) {
        return -1;
    }

    driver_irq_count[irq] = 0;
    return 0;
}

// Dummy-Callback fuer den timer_register-Aufruf in cdi_wait_irq
static void wait_irq_dummy_callback(void) { }

/**
 * Wartet bis der IRQ aufgerufen wurde. Der interne Zaehler muss zuerst mit
 * cdi_reset_wait_irq zurueckgesetzt werden, damit auch die IRQs abgefangen
 * werden koennen, die kurz vor dem Aufruf von dieser Funktion aufgerufen
 * werden.
 *
 * @param irq       Nummer des IRQ auf den gewartet werden soll
 * @param timeout   Anzahl der Millisekunden, die maximal gewartet werden sollen
 *
 * @return 0 wenn der irq aufgerufen wurde, -1 wenn eine ungueltige IRQ-Nummer
 * angegeben wurde, -2 wenn eine nicht registrierte IRQ-Nummer angegeben wurde,
 * und -3 im Falle eines Timeouts.
 */
int cdi_wait_irq(uint8_t irq, uint32_t timeout)
{
    uint64_t timeout_ticks;

    if (irq > IRQ_COUNT) {
        return -1;
    }

    if (!driver_irq_handler[irq]) {
        return -2;
    }

    // Wenn der IRQ bereits gefeuert wurde, koennen wir uns ein paar Syscalls
    // sparen
    if (driver_irq_count [irq]) {
        return 0;
    }

    timeout_ticks = get_tick_count() + (uint64_t) timeout * 1000;
    timer_register(wait_irq_dummy_callback, timeout * 1000);

    p();
    while (!driver_irq_count [irq]) {
        v_and_wait_for_rpc();

        if (timeout_ticks < get_tick_count()) {
            return -3;
        }
        p();
    }
    v();

    return 0;
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
 * Reserviert physisch zusammenhaengenden Speicher an einer definierten Adresse.
 *
 * @param size Groesse des benoetigten Speichers in Bytes
 * @param paddr Physikalische Adresse des angeforderten Speicherbereichs
 *
 * @return Virtuelle Adresse, wenn Speicher reserviert wurde, sonst 0
 */
void* cdi_alloc_phys_addr(size_t size, uintptr_t paddr)
{
    return mem_allocate_physical(size, paddr, 0);
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


