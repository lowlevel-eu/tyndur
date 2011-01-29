/******************************************************************************
 * Copyright (c) 2010 Max Reitz                                               *
 *                                                                            *
 * Permission  is hereby granted,  free of charge,  to any person obtaining a *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction,  including without limitation *
 * the rights to use, copy,  modify, merge, publish,  distribute, sublicense, *
 * and/or  sell copies of  the Software,  and to permit  persons to  whom the *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED,  INCLUDING BUT NOT LIMITED TO  THE WARRANTIES OF MERCHANTABILITY, *
 * FITNESS  FOR A PARTICULAR  PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING *
 * FROM,  OUT OF OR  IN CONNECTION  WITH THE  SOFTWARE  OR THE  USE OR  OTHER *
 * DEALINGS IN THE SOFTWARE.                                                  *
 ******************************************************************************/

#include <init.h>
#include <rpc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "nixer.h"

// struct sim_adriver.device_count
static size_t sim_adriver_device_count(struct sim_adriver* drv)
{
    int dummy = 0;
    return rpc_get_int(drv->pid, "NUMODEV", sizeof(dummy), (char*) &dummy);
}

// struct sim_adriver.is_playback
static bool sim_adriver_is_playback(struct sim_adriver* drv, int index)
{
    return rpc_get_int(drv->pid, "ISPLYBK", sizeof(index), (char*) &index) == 1
        ? true : false;
}

// struct sim_adriver.get_stream_information
static bool sim_adriver_get_stream_information(struct sim_adriver* drv,
    int card)
{
    int strnum[2] = { card, 0 };

    response_t* resp = rpc_get_response(drv->pid, "STRINFO", sizeof(strnum),
        (char*) &strnum);

    if (resp->data_length != sizeof(*drv->str0)) {
        free(resp->data);
        free(resp);
        return false;
    }

    free(drv->str0);
    drv->str0 = resp->data;
    free(resp);

    return true;
}

// struct sim_adriver.transfer_data
static bool sim_adriver_transfer_data(struct sim_adriver* drv, int card,
    int stream, int buffer, size_t offset, uint32_t shm, size_t shm_size)
{
    struct tfrinfo tfrinfo = {
        .card = card,
        .stream = stream,
        .buffer = buffer,
        .offset = offset,
        .shm_size = shm_size,
        .shm = shm
    };

    return rpc_get_int(drv->pid, "TFRDATA", sizeof(tfrinfo), (char*) &tfrinfo) ?
        true : false;
}

// struct sim_adriver.change_device_status
static cdi_audio_status_t sim_adriver_change_device_status(
    struct sim_adriver* drv, int card, cdi_audio_status_t status)
{
    int data[2] = { card, status };
    return rpc_get_int(drv->pid, "CHGSTAT", sizeof(data), (char*) &data);
}

// struct sim_adriver.set_sample_rate
static int sim_adriver_set_sample_rate(struct sim_adriver* drv, int card,
    int stream, int rate)
{
    int ssr[3] = { card, stream, rate };

    return rpc_get_int(drv->pid, "SSPRATE", sizeof(ssr), (char*) &ssr);
}

// struct sim_adriver.set_number_of_channels
static int sim_adriver_set_number_of_channels(struct sim_adriver* drv, int card,
    int channels)
{
    int snoc[2] = { card, channels };

    return rpc_get_int(drv->pid, "SETCHAN", sizeof(snoc), (char*) &snoc);
}

// struct sim_adriver.get_current_frame
static int sim_adriver_get_current_frame(struct sim_adriver* drv, int card,
    int stream)
{
    int gcf[2] = { card, stream };

    return rpc_get_int(drv->pid, "GTFRAME", sizeof(gcf), (char*) &gcf);
}

// struct sim_adriver.disconnect
static void sim_adriver_disconnect(struct sim_adriver* drv)
{
    int tmp = 0;
    rpc_get_int(drv->pid, "REG", sizeof(tmp), (char*) &tmp);
    free(drv);
}

/**
 * Sucht den Soundtreiber mit dem angegebenen Namen, wird er gefunden, dann
 * meldet sich die Funktion beim Treiber an und gibt im Erfolgsfall eine
 * Struktur zurück, die den Treiber repräsentiert.
 */
struct sim_adriver* find_driver(const char* name)
{
    pid_t pid = init_service_get((char*) name);
    if (!pid) {
        fprintf(stderr, "No such service \"%s\".\n", name);
        return NULL;
    }

    int tmp = 1;
    if (rpc_get_int(pid, "REG", sizeof(tmp), (char*) &tmp) != 1) {
        fprintf(stderr, "Could not register at \"%s\".\n", name);
        return NULL;
    }

    struct sim_adriver* ret = calloc(1, sizeof(*ret));

    // Eintragen der PID und der Funktionszeiger
    ret->pid = pid;
    ret->device_count = &sim_adriver_device_count;
    ret->is_playback = &sim_adriver_is_playback;
    ret->get_stream_information = &sim_adriver_get_stream_information;
    ret->transfer_data = &sim_adriver_transfer_data;
    ret->change_device_status = &sim_adriver_change_device_status;
    ret->set_sample_rate = &sim_adriver_set_sample_rate;
    ret->set_number_of_channels = &sim_adriver_set_number_of_channels;
    ret->get_current_frame = &sim_adriver_get_current_frame;
    ret->disconnect = &sim_adriver_disconnect;

    return ret;
}
