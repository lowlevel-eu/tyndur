/*
 * Copyright (c) 2008 Matthew Iselin
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <cdi/bios.h>
#include <syscall.h>
#include <stdlib.h>

/**
 * Performs CMOS reads
 * @param index Index within CMOS RAM to read from.
 * @return Result of reading CMOS RAM at specified index.
 */
uint8_t cdi_cmos_read(uint8_t index)
{
    // TODO
    return 0xff;
}

/**
 * Performs CMOS writes
 * @param index Index within CMOS RAM to write to.
 * @param value Value to write to the CMOS
 */
void cdi_cmos_write(uint8_t index, uint8_t value)
{
    // TODO
}

