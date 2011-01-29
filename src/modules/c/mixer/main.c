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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <sys/types.h>

#include "nixer.h"

// Daten zum SHM in Richtung Soundtreiber
uint32_t shm_id;
void* shm;
size_t shm_size;

// Index der Ausgabesoundkarte
int pbi = -1;

// Verwendeter Soundtreiber
struct sim_adriver* adriver;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: mixer <driver>\n");
        return 1;
    }

    if ((adriver = find_driver(argv[1])) == NULL) {
        fprintf(stderr, "No such service \"%s\".\n", argv[1]);
        return 1;
    }

    printf("Found service \"%s\" found and registered there.\n", argv[1]);

    int devnum = adriver->device_count(adriver);

    printf("Devices reported: %i\n", devnum);

    int i;

    for (i = 0; i < devnum; i++) {
        if (adriver->is_playback(adriver, i)) {
            pbi = i;
            break;
        }
    }

    if (pbi == -1) {
        adriver->disconnect(adriver);
        fprintf(stderr, "No playback devices found.\n");
        return 1;
    }

    printf("Device %i is a playback device.\n", pbi);

    if (adriver->get_stream_information(adriver, pbi) == false) {
        adriver->disconnect(adriver);
        fprintf(stderr, "Invalid answer.\n");
        return 1;
    }

    switch (adriver->str0->sample_format) {
        case CDI_AUDIO_8SI:
        case CDI_AUDIO_16SI:
        case CDI_AUDIO_32SI:
            shm_size = adriver->str0->buffer_size *
                sample_size[adriver->str0->sample_format];
            break;
        default:
            fprintf(stderr, "Unknown sample format %i.\n",
                (int) adriver->str0->sample_format);
            adriver->disconnect(adriver);
            return 1;
    }

    shm_id = create_shared_memory(shm_size);
    shm = open_shared_memory(shm_id);

    adriver->change_device_status(adriver, pbi, CDI_AUDIO_STOP);

    init_service_register("mixer");

    register_message_handler("BUFCOM", &buffer_completed);
    register_message_handler("MIXCONF", &mixer_config);
    register_message_handler("GETSHM", &get_shm);
    register_message_handler("PLAY", &play_input);
    register_message_handler("POS", &get_position);

    for (;;) {
        wait_for_rpc();
    }
}
