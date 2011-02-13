/*
 * Copyright (c) 2010 Max Reitz
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <init.h>
#include <rpc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <syscall.h>
#include <sys/types.h>

#include "cdi.h"
#include "cdi/audio.h"
#include "cdi/lists.h"
#include "cdi/mem.h"

static pid_t current_mixer = 0;
static struct cdi_audio_driver* adriver = NULL;

#define RPC(name) \
    static void name(pid_t src, uint32_t corr_id, size_t length, void* data)

#define rpc_int_resp(val) rpc_send_int_response(src, corr_id, val)

static size_t sample_size[] = {
    2, // CDI_AUDIO_16SI
    1, // CDI_AUDIO_8SI
    4  // CDI_AUDIO_32SI
};

RPC(register_mixer);
RPC(number_of_devices);
RPC(is_playback_device);
RPC(change_device_status);
RPC(transfer_data);
RPC(stream_info);
RPC(set_sample_rate);
RPC(set_number_of_channels);
RPC(get_current_frame);

void cdi_audio_driver_register(struct cdi_audio_driver* driver)
{
    if (adriver != NULL) {
        return;
    }

    adriver = driver;

    // Die Registrierung bei init machen schon andere Teile der CDI-Lib

    register_message_handler("REG", &register_mixer);
    register_message_handler("NUMODEV", &number_of_devices);
    register_message_handler("ISPLYBK", &is_playback_device);
    register_message_handler("CHGSTAT", &change_device_status);
    register_message_handler("TFRDATA", &transfer_data);
    register_message_handler("STRINFO", &stream_info);
    register_message_handler("SSPRATE", &set_sample_rate);
    register_message_handler("SETCHAN", &set_number_of_channels);
    register_message_handler("GTFRAME", &get_current_frame);
}

/**
 * Ein Mixer registriert sich bei diesem Treiber oder meldet sich ab.
 */
RPC(register_mixer)
{
    // Es kann immer nur ein Mixer einen Treiber ansteuern
    if ((length != sizeof(int)) || (current_mixer && (current_mixer != src))) {
        rpc_int_resp(0);
        return;
    }

    if (current_mixer) {
        if (!*(int*) data) {
            // Mixer abmelden
            current_mixer = 0;
            rpc_int_resp(1);
        } else {
            // Anmeldung bereits erfolgt, noch mal geht nicht
            rpc_int_resp(0);
        }
        return;
    } else if (*(int*) data != 1) {
        // Anmeldung noch nicht erfolgt, Abmeldung geht daher nicht
        rpc_int_resp(0);
        return;
    }

    // Mixer anmelden
    current_mixer = src;
    rpc_int_resp(1);
}

/**
 * Gibt die Anzahl der verfügbaren Soundkarten zurück
 */
RPC(number_of_devices)
{
    if (src != current_mixer) {
        rpc_int_resp(0);
    } else {
        rpc_int_resp(cdi_list_size(adriver->drv.devices));
    }
}

/**
 * Gibt 1 zurück, wenn der angegebene Index für ein Abspielgerät steht, bei
 * einem Aufnahmegerät wird 0 und im Fehlerfall -1 zurückgegeben.
 */
RPC(is_playback_device)
{
    if ((src != current_mixer) || (length < sizeof(int))) {
        rpc_int_resp(-1);
        return;
    }

    struct cdi_audio_device* dev =
        cdi_list_get(adriver->drv.devices, ((int*) data)[0]);

    if (dev == NULL) {
        rpc_int_resp(-1);
        return;
    }

    rpc_int_resp(!dev->record);
}

/**
 * Verändert den Zustand des Geräts.
 */
RPC(change_device_status)
{
    if ((src != current_mixer) || (length < 2 * sizeof(int))) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_device* dev =
        cdi_list_get(adriver->drv.devices, ((int*) data)[0]);

    if (dev == NULL) {
        rpc_int_resp(0);
        return;
    }

    rpc_int_resp(adriver->change_device_status(dev, ((int*) data)[1]));
}

/**
 * Überträgt Daten von oder zu einem Stream (per SHM).
 */
RPC(transfer_data)
{
    struct {
        int card, stream, buffer;
        size_t offset, shm_size;
        uint32_t shm;
    } __attribute__((packed))* tfrd = data;

    if ((src != current_mixer) || (length < sizeof(*tfrd))) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_device* dev =
        cdi_list_get(adriver->drv.devices, tfrd->card);

    if (dev == NULL) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_stream* str =
        cdi_list_get(dev->streams, tfrd->stream);

    if (str == NULL) {
        rpc_int_resp(0);
        return;
    }

    size_t ssize;

    switch (str->sample_format) {
        case CDI_AUDIO_8SI:
        case CDI_AUDIO_16SI:
        case CDI_AUDIO_32SI:
            ssize = sample_size[str->sample_format];
            break;
        default:
            rpc_int_resp(0);
            return;
    }

    size_t size =
        (str->buffer_size - (dev->record ? 0 : tfrd->offset)) * ssize;

    // Größe des SHM ist nicht ausreichend
    if (tfrd->shm_size < size) {
        rpc_int_resp(0);
        return;
    }

    void* shm = open_shared_memory(tfrd->shm);

    // CDI-mem-area-Struktur erstellen, mit der der Treiber dann arbeitet
    struct cdi_mem_area* buffer = calloc(1, sizeof(*buffer));
    buffer->vaddr = shm;
    buffer->size  = size;

    int ret = adriver->transfer_data(str, tfrd->buffer, buffer, tfrd->offset);

    free(buffer);

    close_shared_memory(tfrd->shm);

    rpc_int_resp(!ret);
}

/**
 * Gibt Informationen über einen Stream zurück (also die cdi_audio_stream-
 * Struktur).
 */
RPC(stream_info)
{
    char dummy = 0;

    if ((src != current_mixer) || (length < 2 * sizeof(int))) {
        rpc_send_response(src, corr_id, 1, &dummy);
        return;
    }

    struct cdi_audio_device* dev =
        cdi_list_get(adriver->drv.devices, ((int*) data)[0]);

    if (dev == NULL) {
        rpc_send_response(src, corr_id, 1, &dummy);
        return;
    }

    struct cdi_audio_stream* str = cdi_list_get(dev->streams, ((int*) data)[1]);

    if (str == NULL) {
        rpc_send_response(src, corr_id, 1, &dummy);
        return;
    }

    rpc_send_response(src, corr_id, sizeof(*str), (char*) str);
}

/**
 * Setzt die Samplerate eines Streams.
 */
RPC(set_sample_rate)
{
    if ((src != current_mixer) || (length < 3 * sizeof(int))) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_device* dev =
        cdi_list_get(adriver->drv.devices, ((int*) data)[0]);

    if (dev == NULL) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_stream* str = cdi_list_get(dev->streams, ((int*) data)[1]);

    if (str == NULL) {
        rpc_int_resp(0);
        return;
    }

    rpc_int_resp(adriver->set_sample_rate(str, ((int*) data)[2]));
}

/**
 * Legt die Anzahl der Channels einer Soundkarte fest.
 */
RPC(set_number_of_channels)
{
    if ((src != current_mixer) || (length < 2 * sizeof(int))) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_device* dev =
        cdi_list_get(adriver->drv.devices, ((int*) data)[0]);

    if (dev == NULL) {
        rpc_int_resp(0);
        return;
    }

    rpc_int_resp(adriver->set_number_of_channels(dev, ((int*) data)[1]));
}

/**
 * Gibt den Index des aktuell abgespielten Frames im aktuellen Puffer zurück.
 */
RPC(get_current_frame)
{
    if ((src != current_mixer) || (length < 2 * sizeof(int))) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_device* dev =
        cdi_list_get(adriver->drv.devices, ((int*) data)[0]);

    if (dev == NULL) {
        rpc_int_resp(0);
        return;
    }

    struct cdi_audio_stream* str = cdi_list_get(dev->streams, ((int*) data)[1]);

    if (str == NULL) {
        rpc_int_resp(0);
        return;
    }

    cdi_audio_position_t pos;
    adriver->get_position(str, &pos);

    rpc_int_resp(pos.frame);
}

/**
 * Wird vom Soundkartentreiber aufgerufen, wenn die Bearbeitung eines Puffers
 * abgeschlossen wurde.
 */
void cdi_audio_buffer_completed(struct cdi_audio_stream* stream, size_t buffer)
{
    if (current_mixer) {
        rpc_get_int(current_mixer, "BUFCOM", sizeof(buffer), (char*) &buffer);
    }
}

