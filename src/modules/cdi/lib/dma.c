/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <stdio.h>
#include <stdlib.h>

#include "cdi/dma.h"

/**
 * Initialisiert einen Transport per DMA
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_open(struct cdi_dma_handle* handle, uint8_t channel, uint8_t mode,
    size_t length, void* buffer)
{
    char* dma_path;
    int result = 0;
    // DMA-Path zusammensetzen
    if (asprintf(&dma_path, "dma:/%d/%d/%d", channel, mode, length) < 0) {
        return -1;
    }
    
    // Handle oeffnen
    handle->meta.file = fopen(dma_path, "r+");
    if (handle->meta.file == NULL) {
        result = -1;
    }
    
    // Struktur befuellen
    handle->length = length;
    handle->channel = channel;
    handle->mode = mode;
    handle->buffer = buffer;

    free(dma_path);
    return result;
}

/**
 * Liest Daten per DMA ein
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_read(struct cdi_dma_handle* handle)
{
    // Daten einlesen
    if (fread(handle->buffer, 1, handle->length, handle->meta.file) == handle->
        length) 
    {
        return 0;
    }
    return -1;
}

/**
 * Schreibt Daten per DMA
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_write(struct cdi_dma_handle* handle)
{
    // Daten schreiben
    if (fwrite(handle->buffer, 1, handle->length, handle->meta.file) ==
        handle->length)
    {
        return 0;
    }
    return -1;
}

/**
 * Schliesst das DMA-Handle wieder
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_close(struct cdi_dma_handle* handle)
{
    return fclose(handle->meta.file);
}
