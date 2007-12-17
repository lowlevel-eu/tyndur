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
#include <collections.h> 

#include "cdi/net.h"

/**
 * Initialisiert die Datenstrukturen fuer einen Netzerktreiber
 */
void cdi_net_driver_init(struct cdi_net_driver* driver)
{
    cdi_driver_init((struct cdi_driver*) driver);
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Netzwerktreiber
 */
void cdi_net_driver_destroy(struct cdi_net_driver* driver)
{
    cdi_driver_destroy((struct cdi_driver*) driver);
}
