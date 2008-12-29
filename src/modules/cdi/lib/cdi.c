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
#include <collections.h> 
#include <lostio.h>
#include <init.h>
#include <rpc.h>

#include "cdi.h"

static list_t* drivers = NULL;

static void cdi_destroy(void);

/**
 * Caches Synchronisieren
 */
extern void caches_sync_all(void);

/**
 * Timerhandler um die Caches zu syncen
 */
static void timer_sync_caches(void)
{
    caches_sync_all();
    timer_register(timer_sync_caches, 2000000);
}


/**
 * Muss vor dem ersten Aufruf einer anderen CDI-Funktion aufgerufen werden.
 * Initialisiert interne Datenstruktur der Implementierung fuer das jeweilige
 * Betriebssystem.
 */
void cdi_init(void)
{
    drivers = list_create();
    atexit(cdi_destroy);

    lostio_init();
    lostio_type_directory_use();
    timer_sync_caches();
}

/**
 * Wird bei der Deinitialisierung aufgerufen
 */
static void cdi_destroy(void) 
{
    struct cdi_driver* driver;
    int i;

    for (i = 0; (driver = list_get_element_at(drivers, i)); i++) {
        if (driver->destroy) {
            driver->destroy(driver);
        }
    }
}

/**
 * Fuehrt alle registrierten Treiber aus. Nach dem Aufruf dieser Funktion 
 * duerfen keine weiteren Befehle ausgefuehrt werden, da nicht definiert ist,
 * ob und wann die Funktion zurueckkehrt.
 */
void cdi_run_drivers(void)
{
    // Geraete initialisieren
    struct cdi_driver* driver;
    struct cdi_device* device;
    int i, j;
    for (i = 0; (driver = list_get_element_at(drivers, i)); i++) {

        if (driver->type == CDI_NETWORK) {
            init_service_register((char*) driver->name);
        }

        for (j = 0; (device = cdi_list_get(driver->devices, j)); j++) {
            device->driver = driver;

            if (driver->init_device) {
                driver->init_device(device);
            }
        }

        if (driver->type != CDI_NETWORK) {
            init_service_register((char*) driver->name);
        }
    }

    // Warten auf Ereignisse
    while (1) {
        wait_for_rpc();
    }
}

/**
 * Initialisiert die Datenstrukturen fuer einen Treiber
 */
void cdi_driver_init(struct cdi_driver* driver)
{
    driver->devices = cdi_list_create();
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Treiber
 */
void cdi_driver_destroy(struct cdi_driver* driver)
{
    cdi_list_destroy(driver->devices);
}

/**
 * Registriert den Treiber fuer ein neues Geraet
 *
 * @param driver Zu registierender Treiber
 */
void cdi_driver_register(struct cdi_driver* driver)
{
    list_push(drivers, driver);
}

