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

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cdi/audio.h>

#include "nixer.h"

extern struct input_queue* iq_start;
extern struct input_queue** iq_end;
extern struct sim_adriver* adriver;

extern int pbi;

extern uint32_t shm_id;
extern void* shm;
extern size_t shm_size;

// true, wenn derzeit Daten abgespielt werden
volatile bool playing;

// Anzahl der bereits zum Soundkartentreiber geschickten Puffer (sollte
// mindestens zwei sein, damit keine Lücken beim Abspielen entstehen)
int buffers_queued;
// Nächster zu sendender Puffer
int current_buffer;

/**
 * Überträgt, wenn möglich, so viele Pakete aus der Eingangsqueue an den
 * Treiber, bis dieser zwei Pakete zu bearbeiten hat (ein Paket wird
 * aktuell bearbeitet, ein zweites dient als Puffer).
 */
void send_buffer(void)
{
    struct input_queue* ip;

    // Klasse, Queue leer oder erstes Paket unvollständig...
    if ((iq_start == NULL) || (iq_start->missing)) {
        if (!buffers_queued) {
            adriver->change_device_status(adriver, pbi, CDI_AUDIO_STOP);
            playing = false;
        }
        return;
    }

    while (buffers_queued < 2) {
        ip = input_queue_pop();

        if ((ip == NULL) || (ip->missing)) {
            break;
        }

        // Die Daten im Paket besitzen bereits das Format, dass der Treiber
        // erwartet
        memcpy(shm, ip->buffer, shm_size);

        // Paket freigeben
        free(ip->buffer);
        free(ip);

        // Daten an den Treiber übergeben
        if (!adriver->transfer_data(adriver, pbi, 0, current_buffer, 0, shm_id,
            shm_size))
        {
            fprintf(stderr,
                "[nixer] Failed to transfer data (this is fatal).\n");
            adriver->disconnect(adriver);
            abort();
        }

        buffers_queued++;
        current_buffer = (current_buffer + 1) % adriver->str0->num_buffers;
    }
}

/**
 * Startet den Abspielvorgang und überträgt die ersten Pakete an den Treiber.
 */
void play(void)
{
    bool has_played = playing;

    playing = true;

    send_buffer();

    if (playing && !has_played) {
        // Wiedergabe im Erfolgsfall starten
        adriver->change_device_status(adriver, pbi, CDI_AUDIO_PLAY);
    }
}

/**
 * Verwirft alle Eingabedaten und beendet den Abspielvorgang.
 */
void trash_sound(bool immediately)
{
    if (immediately) {
        playing = false;
        adriver->change_device_status(adriver, pbi, CDI_AUDIO_STOP);
        buffers_queued = 0;

        struct input_queue* ip = iq_start;
        iq_end = &iq_start;
        iq_start = NULL;

        while (ip != NULL) {
            free(ip->buffer);

            void* tmp = ip;
            ip = ip->next;
            free(tmp);
        }
    }

    while (playing);

    current_buffer = (current_buffer + 1) % adriver->str0->num_buffers;
}

/**
 * Wird vom Treiber aufgerufen, wenn ein Puffer abgeschlossen wurde.
 */
RPC(buffer_completed)
{
    if (length != sizeof(size_t)) {
        rpc_int_resp(0);
        return;
    }

    // current_buffer sollte derzeit auf diesem Wert stehen
    int expected =
        (*(size_t*) data + buffers_queued) % adriver->str0->num_buffers;

    buffers_queued--;

    // Merkwürdigerweise wurden in diesem Fall mehr Puffer abgespielt, als
    // eigentlich an die Karte übergeben wurden.
    if (buffers_queued < 0) {
        fprintf(stderr, "[nixer] Strange underrun; %i packets missing.\n",
            -buffers_queued);

        // Versuch, zu retten, was zu retten ist
        current_buffer = (*(size_t*) data + 1) % adriver->str0->num_buffers;
        buffers_queued = 0;
    }

    // current_buffer entspricht nicht dem Wert, den die Soundkarte erwartet
    if (expected != current_buffer) {
        fprintf(stderr, "[nixer] Buffer index discrepancy; %i expected, but "
            "should be %i\n", current_buffer, expected);

        current_buffer = expected;
    }

    // Nächsten Puffer aus der Eingabequeue senden
    send_buffer();

    rpc_int_resp(0);
}
