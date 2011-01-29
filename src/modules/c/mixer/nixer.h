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

#ifndef NIXER_H
#define NIXER_H

#include <rpc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <cdi/audio.h>

// Macros, damit man ein paar RPC-Sachen schneller schreiben kann
#define RPC(name) \
    void name(pid_t src, uint32_t corr_id, size_t length, void* data)

#define rpc_int_resp(val)   rpc_send_int_response(src, corr_id, val)
#define rpc_dword_resp(val) rpc_send_dword_response(src, corr_id, val)

// Abstrahiert den aktuellen Ausgabetreiber
struct sim_adriver {
    // PID des Treibers
    pid_t pid;

    // Zu verwendender Stream (dieser Nichtmixer = Nixer unterstützt nur eine
    // Quellanwendung, daher wäre die Verwendung mehrerer Ausgabestreams nicht
    // sehr sinnvoll)
    struct cdi_audio_stream* str0;

    /**
     * Gibt die Anzahl der vom Treiber verwalteten Geräte zurück.
     */
    size_t (*device_count)(struct sim_adriver* drv);
    /**
     * Gibt true zurück, wenn die Soundkarte mit dem angegebenen Index ein
     * Ausgabegerät ist.
     */
    bool (*is_playback)(struct sim_adriver* drv, int index);
    /**
     * Füllt das str0-Feld mit Informationen über den ersten Stream der
     * ausgewählten Karte.
     */
    bool (*get_stream_information)(struct sim_adriver* drv, int card);
    /**
     * Tauscht Daten mit dem Stream der angegebenen Karte aus (stream sollte
     * normalerweise 0 sein), buffer ist der Index des zu verwendenden Puffers,
     * offset der Zieloffset darin, shm die SHM-ID zum Datenaustausch und
     * shm_size die Größe des SHMs.
     * Bei Erfolg wird true zurückgegeben, sonst false.
     */
    bool (*transfer_data)(struct sim_adriver* drv, int card, int stream,
        int buffer, size_t offset, uint32_t shm, size_t shm_size);
    /**
     * Verändert den Zustand einer Soundkarte (z. B. Abspielen oder Anhalten).
     * Zurückgegeben wird der tatsächlich gesetzte Zustand.
     */
    cdi_audio_status_t (*change_device_status)(struct sim_adriver* drv,
        int card, cdi_audio_status_t status);
    /**
     * Versucht, die Samplerate (in Hz) des angegebenen Streams zu verändern.
     * Dies muss nicht immer gelingen, die tatsächlich eingestellte Samplerate
     * wird zurückgegeben (z. B. könnte das Setzen der Samplerate 44101 Hz zu
     * einer tatsächlichen Samplerate von 44100 Hz führen).
     */
    int (*set_sample_rate)(struct sim_adriver* drv, int card, int stream,
        int rate);
    /**
     * Versucht, die Anzahl der Ausgabechannel der angegebenen Karte zu setzen.
     * Zurückgegeben wird die tatsächliche Anzahl von Channeln (Versucht man,
     * eine Stereokarte mit der Channelanzahl 6 in den 5.1-Modus zu setzen, dann
     * wird wohl 2 zurückgegeben, weil nur zwei Channel unterstützt werden).
     */
    int (*set_number_of_channels)(struct sim_adriver* drv, int card,
        int channels);
    /**
     * Gibt den Index des aktuell abgespielten Frames zurück.
     */
    int (*get_current_frame)(struct sim_adriver* drv, int card, int stream);
    /**
     * Löst die Verbindung zu einem Audiotreiber und gibt den Speicher der
     * übergebenen Struktur frei.
     */
    void (*disconnect)(struct sim_adriver* drv);
};

// Struktur, die für transfer_data-RPC verwendet wird.
struct tfrinfo {
    int card, stream, buffer;
    size_t offset, shm_size;
    uint32_t shm;
} __attribute__((packed));

// Struktur, die von der Anwendung genutzt wird, um dem Mixer Informationen
// zum Sampling mitzuteilen
struct mixer_config {
    int sample_rate, channel_count;
    cdi_audio_sample_format_t sample_format;
} __attribute__((packed));

// Queue mit den eingehenden Daten
struct input_queue {
    struct input_queue* next;
    // Puffer mit den an die Soundkarte zu schickenden Samples
    void* buffer;
    // Anzahl der noch fehlenden Bytes, bis der Puffer vollständig gefüllt ist
    size_t missing;
} __attribute__((packed));

// Größe eines Samples in Abhängigkeit vom Sampleformat
static const size_t sample_size[] = {
    2, // CDI_AUDIO_16SI
    1, // CDI_AUDIO_8SI
    4  // CDI_AUDIO_32SI
};

void input_queue_push(struct input_queue* packet);
struct input_queue* input_queue_pop(void);
struct input_queue* last_input_queue_packet(void);
bool input_queue_complete(void);

struct sim_adriver* find_driver(const char* name);

void send_buffer(void);
void play(void);
void trash_sound(bool immediately);

RPC(buffer_completed);
RPC(get_position);
RPC(get_shm);
RPC(mixer_config);
RPC(play_input);

#endif
