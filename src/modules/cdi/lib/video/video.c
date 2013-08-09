/*
 * Copyright (c) 2008 Mathias Gottschlag, Alexander Siol
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lostio.h>
#include <rpc.h>
#include <syscall.h>

#include <cdi.h>
#include <cdi/video.h>
#include "video.h"
#include "context.h"
#include "bitmap.h"
#include <video/commands.h>

int videocard_count = 0;
struct cdi_video_driver* videodriver = 0;



unsigned long int video_do_command_buffer(void* data, size_t len, pid_t origin);


/**
 * Initialisiert die Datenstrukturen fuer einen Grafikkartentreiber
 */
void cdi_video_driver_init(struct cdi_video_driver* driver)
{
    cdi_driver_init((struct cdi_driver*) driver);

    context_initialize();
    bitmaps_initialize();

    register_message_handler("VIDEODRV", rpc_video_interface);
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Grafikkartentreiber
 */
void cdi_video_driver_destroy(struct cdi_video_driver* driver)
{
    cdi_driver_destroy((struct cdi_driver*) driver);
}

/**
 * Registiert einen Grafikkartentreiber
 */
void cdi_video_driver_register(struct cdi_video_driver* driver)
{
    videodriver = driver;
    cdi_driver_register((struct cdi_driver*) driver);
}
