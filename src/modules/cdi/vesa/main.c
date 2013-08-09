/*
 * Copyright (c) 2008 Alexander Siol
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cdi.h>
#include <cdi/misc.h>
#include <cdi/video.h>
#include <cdi/pci.h>
#include <cdi/lists.h>
#include <cdi/vesa.h>

#include "vesa.h"
#include "bitmap.h"
#include "device.h"
#include "drawing.h"

#define DRIVER_NAME "vesa"
struct cdi_video_driver vesa_driver;

#define DEBUG printf
#define TODO //

/**
 * Initialisiert den Treiber und registriert ihn
 */
static int vesa_driver_init(void)
{
    cdi_list_t pci_devices;
    struct cdi_pci_device* pci;

    cdi_video_driver_init(&vesa_driver);


    // Suche VGA-Karten
    pci_devices = cdi_list_create();
    cdi_pci_get_all_devices(pci_devices);
    while ((pci = cdi_list_pop(pci_devices))) {
        if (pci->class_id == 0x03 && pci->subclass_id == 0x00 &&
            pci->interface_id == 0x00) {
            char* name = malloc(8);
            snprintf(name, 8, "vesa%01d", cdi_list_size(vesa_driver.drv.devices));
            struct vesa_device *dev = vesa_device_create(name, pci);
            dev->dev.dev.driver = (struct cdi_driver*)&vesa_driver;
            cdi_list_push(vesa_driver.drv.devices, dev);
        } else {
            cdi_pci_device_destroy(pci);
        }
    }
    cdi_list_destroy(pci_devices);

    vesa_bitmap_list = cdi_list_create();

    // TODO FIXME Nach init_device verschieben!
    struct cdi_video_vesa_info_block **vesainfo_ptr;
    vesainfo_ptr = malloc(sizeof(void*));
    // Ohne VBE abbrechen
    if (cdi_video_vesa_initialize(vesainfo_ptr, vesa_mode_callback) == -1) {
        return -1;
    }

    // Treiber registrieren
    cdi_video_driver_register((struct cdi_video_driver*)&vesa_driver);
    return 0;
}

static struct cdi_device* vesa_init_device(struct cdi_bus_data* bus_data)
{
    return NULL;
}

void vesa_remove_device(struct cdi_device* device)
{
}

/**
 * Zerstoert den Treiber
 * @param driver Pointer to the driver
 */
static int vesa_driver_destroy(void)
{
    cdi_video_driver_destroy(&vesa_driver);
    return 0;
}

struct cdi_video_driver vesa_driver = {
    .drv = {
        .name           = DRIVER_NAME,
        .type           = CDI_VIDEO,
        .init           = vesa_driver_init,
        .destroy        = vesa_driver_destroy,
        .init_device    = vesa_init_device,
        .remove_device  = vesa_remove_device,
    },

    .display_set_mode       = vesa_display_set_mode,
    .restore_text_mode      = vesa_restore_text_mode,

    .bitmap_create              = vesa_bitmap_create,
    .bitmap_destroy             = vesa_bitmap_destroy,
    .bitmap_set_usage_hint      = vesa_bitmap_set_usage_hint,
    .bitmap_request_backbuffers = vesa_bitmap_request_backbuffers,
    .bitmap_flip_buffers        = vesa_bitmap_flip_buffers,

    .set_raster_op          = vesa_set_raster_op,
    .draw_line              = vesa_draw_line,
    .draw_rectangle         = vesa_draw_rectangle,
    .draw_ellipse           = NULL,//vesa_draw_ellipse,
    .draw_bitmap            = vesa_draw_bitmap,
    .draw_bitmap_part       = vesa_draw_bitmap_part,
    .draw_dot               = vesa_draw_dot,
};

CDI_DRIVER(DRIVER_NAME, vesa_driver)
