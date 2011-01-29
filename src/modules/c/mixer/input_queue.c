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
#include <stdint.h>
#include <stdlib.h>

#include "nixer.h"

// Anfang und Ende der Eingangsqueue
struct input_queue* iq_start;
struct input_queue** iq_end = &iq_start;

/**
 * Fügt ein Paket an das Ende der Eingangsqueue an
 */
void input_queue_push(struct input_queue* packet)
{
    packet->next = NULL;

    *iq_end = packet;
    iq_end = &packet->next;
}

/**
 * Entfernt ein Paket vom Beginn der Eingangsqueue und gibt es zurück.
 */
struct input_queue* input_queue_pop(void)
{
    if (iq_start == NULL) {
        return NULL;
    }

    struct input_queue* packet = iq_start;
    iq_start = packet->next;

    // Ende der Queue, somit wird iq_end ungültig und muss auf iq_start zeigen
    if (packet->next == NULL) {
        iq_end = &iq_start;
    }

    return packet;
}

/**
 * Gibt einen Zeiger auf das letzte Paket in der Eingangsqueue zurück.
 */
struct input_queue* last_input_queue_packet(void)
{
    // Keine Pakete in der Queue
    if (iq_end == &iq_start) {
        return NULL;
    }

    // Da iq_end ein Zeiger auf das next-Feld des letzten Pakets ist, kann man
    // daraus die Adresse dieses Paketes ermitteln.
    return (void*) ((uintptr_t) iq_end - offsetof(struct input_queue, next));
}

/**
 * Ist der letzte Puffer in der Eingangsqeue vollständig gefüllt, so wird true
 * zurückgegeben, ebenso, wenn die Queue leer ist, sonst false.
 */
bool input_queue_complete(void)
{
    struct input_queue* last_packet = last_input_queue_packet();
    if (last_packet == NULL) {
        return true;
    }

    return !last_packet->missing;
}
