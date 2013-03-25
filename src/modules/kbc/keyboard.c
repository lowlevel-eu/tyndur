/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <syscall.h>
#include <rpc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lostio.h>
#include <ports.h>
#include <collections.h>
#include <kbd.h>
#include <sleep.h>

#include "keyboard.h"
#include "mouse.h"

/**
 * Liste mit den PIDs die benachrichtitg werden wollen, bei Tastendruecken
 */
static list_t* callback_list;

/**
 * Wird Benutzt um Meldungen ueber ungueltige Scancodes waehrend der
 * Initialisierungsphase zu verhindern
 */
static bool init_done = false;

/**
 * RPC-Handler zum registrieren eines Callbacks fuer Tastendruecke
 */
static void rpc_register_callback(pid_t pid, uint32_t cid, size_t data_size,
    void* data);


/**
 * IRQ-Handler
 */
static void kbd_irq_handler(uint8_t irq);


/**
 * Tastendruck an wartende Prozesse senden
 *
 * @param keycode Der zu sendende Keycode
 * @param release true wenn die Taste losgelassen wurde
 */
static void send_key_event(uint8_t keycode, bool release);

/**
 * Befehl an die Tastatur senden
 */
static void send_kbd_command(uint8_t command)
{
    do {
        while (inb(0x64) & 0x2) {
            yield();
        }
        outb(0x60, command);
        while ((inb(0x64) & 0x1) == 0) {
            yield();
        }
    } while (inb(0x60) == 0xfe);
}


/**
 * Tastaturtreiber initialisieren
 */
void keyboard_init(void)
{
    // Liste mit Prozessen, die benachrichtigt werden wollen ueber Tastendruecke
    callback_list = list_create();
    register_message_handler(KBD_RPC_REGISTER_CALLBACK, rpc_register_callback);

    // Interrupt-Handler fuer die Tastatur registrieren
    register_intr_handler(0x21, &kbd_irq_handler);

    // So, mal hoeren was uns die Tastatur noch so alles zu erzaehlen hat von
    // eventuell gedrueckten Tasten waerend dem Booten.
    while ((inb(0x64) & 0x1)) {
        inb(0x60);
    }

    // Leds alle ausloeschen
    send_kbd_command(0xED);
    send_kbd_command(0);


    // Schnellste Wiederholrate
    send_kbd_command(0xF3);
    send_kbd_command(0);

    // Tastatur aktivieren
    send_kbd_command(0xF4);

    init_done = true;
}

/**
 * IRQ-Handler
 */
void kbd_irq_handler(uint8_t irq) {
    uint8_t scancode;
    uint8_t keycode = 0;
    bool break_code = false;

    // Status-Variablen fuer das Behandeln von e0- und e1-Scancodes
    static bool     e0_code = false;
    // Wird auf 1 gesetzt, sobald e1 gelesen wurde, und auf 2, sobald das erste
    // Datenbyte gelesen wurde
    static int      e1_code = 0;
    static uint16_t  e1_prev = 0;

    // Abbrechen wenn die Initialisierung noch nicht abgeschlossen wurde
    if (!init_done) {
        return;
    }

    scancode = inb(0x60);

    // Um einen Breakcode handelt es sich, wenn das oberste Bit gesetzt ist und
    // es kein e0 oder e1 fuer einen Extended-scancode ist
    if ((scancode & 0x80) &&
        (e1_code || (scancode != 0xE1)) &&
        (e0_code || (scancode != 0xE0)))
    {
        break_code = true;
        scancode &= ~0x80;
    }

    if (e0_code) {
        // Fake shift abfangen
        // Siehe: http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html#ss1.6
        if ((scancode == 0x2A) || (scancode == 0x36)) {
            e0_code = false;
            return;
        }

        // Fertiger e0-Scancode
        keycode = translate_scancode(1, scancode);
        e0_code = false;
    } else if (e1_code == 2) {
        // Fertiger e1-Scancode
        // Zweiten Scancode in hoeherwertiges Byte packen
        e1_prev |= ((uint16_t) scancode << 8);
        keycode = translate_scancode(2, e1_prev);
        e1_code = 0;
    } else if (e1_code == 1) {
        // Erstes Byte fuer e1-Scancode
        e1_prev = scancode;
        e1_code++;
    } else if (scancode == 0xE0) {
        // Anfang eines e0-Codes
        e0_code = true;
    } else if (scancode == 0xE1) {
        // Anfang eines e1-Codes
        e1_code = 1;
    } else {
        // Normaler Scancode
        keycode = translate_scancode(0, scancode);
    }

    if (keycode != 0) {
        send_key_event(keycode, break_code);
    }
}


/**
 * RPC-Handler zum registrieren eines Callbacks fuer Tastendruecke
 */
static void rpc_register_callback(pid_t pid, uint32_t cid, size_t data_size,
    void* data)
{
    list_push(callback_list, (void*) pid);
    rpc_send_dword_response(pid, cid, 1);
}

/**
 * Tastendruck an wartende Prozesse senden
 *
 * Der RPC verschickt dabei 2 Bytes, das erste mit dem keycode, das zweite ist
 * entweder 0 oder != 0 je nachdem, ob die Taste gedrueckt oder losgelassen
 * wurde.
 *
 * @param keycode Der zu sendende Keycode
 * @param release true wenn die Taste losgelassen wurde
 */
static void send_key_event(uint8_t keycode, bool release)
{
    pid_t pid;
    int i;
    char data[8 + 2];

    strcpy(data, KBD_RPC_CALLBACK);
    data[8] = keycode;
    data[9] = release;

    for (i = 0; (pid = (pid_t) list_get_element_at(callback_list, i)); i++) {
        send_message(pid, 512, 0, 8 + 2, data);
    }
}

