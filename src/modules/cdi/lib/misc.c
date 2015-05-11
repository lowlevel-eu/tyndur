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
#include <stdlib.h>
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

struct irq_handler {
    struct irq_handler* next;

    void (*handler)(struct cdi_device* dev);
    struct cdi_device* dev;
};

/** Array mit allen IRQ-Handlern; Als index wird die Nummer benutzt */
static struct irq_handler* driver_irq_handlers[IRQ_COUNT];

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
    for (struct irq_handler* h = driver_irq_handlers[irq]; h; h = h->next) {
        h->handler(h->dev);
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

    struct irq_handler* h = malloc(sizeof(*h));
    *h = (struct irq_handler){
        .next    = driver_irq_handlers[irq],
        .handler = handler,
        .dev     = device
    };

    driver_irq_handlers[irq] = h;

    if (!h->next) {
        register_intr_handler(TYNDUR_IRQ_OFFSET + irq, irq_handler);
    }
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

    if (!driver_irq_handlers[irq]) {
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


uint64_t cdi_elapsed_ms(void)
{
    return get_tick_count() / 1000;
}
