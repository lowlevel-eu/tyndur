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

#include <rpc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <syscall.h>

#include "nixer.h"

extern struct sim_adriver* adriver;
extern struct input_queue* iq_start;

extern int pbi;

extern bool playing;
extern int buffers_queued;

extern size_t shm_size;

// Anzahl der Ausgabechannels
static int out_channel_count;

// PID der aktuellen Quellanwendung
static pid_t current_source;
// Daten zum SHM in Richtung Anwendung
static uint32_t current_input_shm_id;
static size_t current_input_shm_size;
static void* current_input_shm = NULL;

// Anzahl der Eingabechannels
static int input_channel_count;
// Eingabeformat
static cdi_audio_sample_format_t input_format;

/**
 * Wird von der Anwendung einerseits aufgerufen, um den Mixer zu konfigurieren
 * und andererseits, um sich bei ihm anzumelden -- da dieser Mixer ein Nixer
 * ist, kann immer nur eine Anwendung zu einem bestimmten Zeitpunkt bedient
 * werden.
 */
RPC(mixer_config)
{
    struct mixer_config* mc = data;

    if (length != sizeof(*mc)) {
        rpc_int_resp(0);
        return;
    }

    // Wenn schon eine Anwendung angemeldet ist, kann die Anfrage nicht
    // akzeptiert werden
    if (current_source && (src != current_source)) {
        rpc_int_resp(0);
        return;
    }

    // Ist die Anzahl der Channels 0, dann meldet sich die Anwendung ab
    if (current_source && !mc->channel_count) {
        current_source = 0;
        current_input_shm_size = 0;
        if (current_input_shm != NULL) {
            current_input_shm = NULL;
            close_shared_memory(current_input_shm_id);
        }

        trash_sound(false);

        rpc_int_resp(1);
        return;
    }

    // Aktuelle Anwendung setzen
    current_source = src;

    // Aktuelles Eingabeformat speichern
    input_format = mc->sample_format;
    input_channel_count = mc->channel_count;

    // Wenn bereits ein SHM existiert, wird die Anwendung den wohl eher nicht
    // mehr nutzen wollen.
    if (current_input_shm != NULL) {
        current_input_shm = NULL;
        close_shared_memory(current_input_shm_id);
    }

    // TODO: Rückgabewert nutzen und bei Bedarf resamplen
    adriver->set_sample_rate(adriver, pbi, 0, mc->sample_rate);

    // Versuchen, die Anzahl der Ausgabechannel der Anzahl der Eingabechannel
    // anzupassen
    out_channel_count =
        adriver->set_number_of_channels(adriver, pbi, mc->channel_count);

    rpc_int_resp(1);
}

/**
 * Wird von der Anwendung aufgerufen, um einen SHM zur Übertragung von Daten zum
 * Nixer zu erhalten.
 */
RPC(get_shm)
{
    if ((length != sizeof(size_t)) || (src != current_source)) {
        rpc_dword_resp(0);
        return;
    }

    // Wenn es bereits einen SHM gibt, muss dieser zuerst geschlossen werden.
    if (current_input_shm != NULL) {
        current_input_shm = NULL;
        close_shared_memory(current_input_shm_id);
    }

    // Die Größe des SHMs muss immer ein Vielfaches der Größe eines Frames sein
    // (halbe Frames lassen sich schlecht abspielen).
    if (*((size_t*) data) % (sample_size[input_format] * input_channel_count)) {
        rpc_dword_resp(0);
        return;
    }

    current_input_shm_size = *((size_t*) data);

    current_input_shm_id = create_shared_memory(current_input_shm_size);
    if (current_input_shm_id) {
        current_input_shm = open_shared_memory(current_input_shm_id);
    }

    // SHM-ID zurückgeben
    rpc_dword_resp(current_input_shm_id);
}

/**
 * Überträgt die Daten aus dem Eingangs-SHM in die Eingangsqueue und bereitet
 * sie so auf das Abspielen durch den Soundtreiber vor.
 */
RPC(play_input)
{
    if ((src != current_source) || (length != sizeof(uint32_t))) {
        rpc_int_resp(0);
        return;
    }

    // Aufpassen, dass uns current_input_shm nicht unter den Füßen weggezogen
    // wird, weil eine bösartige Anwendung gleich hinterher ein close schickt
    p();

    // Kleine Kontrolle
    if ((current_input_shm == NULL) ||
        (*((uint32_t*) data) != current_input_shm_id))
    {
        rpc_int_resp(0);
        return;
    }

    // Daten über das aktuell erstellte Eingangspaket
    size_t current_size = 0, remaining = current_input_shm_size;
    uint8_t* current_buffer;
    struct input_queue* current_packet;

    uint8_t* input_buffer = current_input_shm;

    size_t input_size = sample_size[input_format];
    size_t output_size = sample_size[adriver->str0->sample_format];

    // Gibt an, ob ein neues Paket erstellt wurde
    bool allocated;

    // Nun werden die Daten vom Eingangsformat in das Ausgabeformat umgewandelt
    while (remaining) {
        if ((allocated = input_queue_complete())) {
            // Letztes Paket vollständig, also erstellen wir ein neues
            current_packet = malloc(sizeof(*current_packet));
            current_buffer = malloc(shm_size);
            current_packet->buffer = current_buffer;

            current_size = shm_size;
        } else {
            // Letztes Paket unvollständig, also füllen wir das
            current_packet = last_input_queue_packet();
            current_size = current_packet->missing;

            current_buffer = current_packet->buffer +
                shm_size - current_packet->missing;
        }

        // TODO: Müsste man bei mehr Sampleformaten anpassen (außer Signed
        // Integer mit Little Endian)
        int_fast32_t samples[(out_channel_count > input_channel_count) ?
            out_channel_count : input_channel_count];
        size_t in_frame_size = input_channel_count * input_size;
        size_t out_frame_size = out_channel_count * output_size;

        // Solange noch Eingabedaten vorhanden sind und der Ausgabepuffer nicht
        // voll ist
        // TODO: Das funktioniert, ist aber langsam. Mit MMX oder so kann man da
        // bestimmt was machen.
        while (current_size && remaining) {
            int_fast32_t val = 0;
            int i, j;

            // Eingabesamples einlesen
            for (j = 0; j < input_channel_count; j++) {
                val = 0;
                for (i = 0; i < input_size; i++) {
                    val |= *(input_buffer++) << (i * 8);
                }

                // Für die Ausgabe zurechtshiften
                val <<= 8 * (4 - input_size);
                val >>= 8 * (4 - output_size);

                samples[j] = val;
            }

            remaining -= in_frame_size;

            // TODO: Das geht sicher besser
            for (j = input_channel_count; j < out_channel_count; j++) {
                samples[j] = val;
            }

            // Ausgabesamples schreiben
            for (j = 0; j < out_channel_count; j++) {
                for (i = 0; i < output_size; i++) {
                    *(current_buffer++) = samples[j] & 0xFF;
                    samples[j] >>= 8;
                }
            }

            current_size -= out_frame_size;
        }

        current_packet->missing = current_size;
        if (allocated) {
            // Wurde das Paket neu erstellt, dann muss es noch an die
            // Eingangsqueue angehangen werden
            input_queue_push(current_packet);
        }
    }

    // Abspielvorgang starten
    play();

    v();
    rpc_int_resp(1);
}

/**
 * Gibt die Anzahl der im Puffer vorhandenen Bytes zurück
 */
RPC(get_position)
{
    if (src != current_source) {
        rpc_int_resp(0);
        return;
    }

    if (!playing && (iq_start == NULL)) {
        rpc_int_resp(0);
        return;
    }

    int queued = 0;

    // FIXME: Warum auch immer, jedenfalls ist es schlecht, wenn diese Pakete
    // mitgezählt werden und man mehr als einmal laut möchte
    /*
    // Berechnet, wie viele Bytes bei der Soundkarte warten
    if (buffers_queued > 1) {
        queued = (buffers_queued - 1) * shm_size;
    }
    */

    // Untersucht, wie viele Bytes in der Eingangsqueue warten
    struct input_queue* iq = iq_start;

    while (iq != NULL) {
        queued += shm_size - iq->missing;
        iq = iq->next;
    }

    // Fragt ab, wie viele Bytes im aktuell in der Soundkarte abgespielten
    // Puffer noch warten
    if (playing && buffers_queued) {
        queued += (adriver->str0->buffer_size -
            adriver->get_current_frame(adriver, pbi, 0) * out_channel_count) *
            sample_size[adriver->str0->sample_format];
    }

    // Die Größe des Ausgabe-SHMs abziehen, damit die Anwendung immer genügend
    // Daten liefert, sodass der gefüllt werden kann.
    queued -= shm_size;
    if (queued < 0) {
        queued = 0;
    }

    rpc_int_resp(queued);
}
