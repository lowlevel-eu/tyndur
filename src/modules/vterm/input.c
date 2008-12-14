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

#include <types.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <init.h>
#include <kbd.h>

#include "vterm.h"
#include "keymap.h"

extern vterminal_t* current_vterm;

/// Pid von kbc
static pid_t kbc_pid = 0;

/**
 * Calback handler
 */
static void rpc_kbd_callback(pid_t pid, dword cid, size_t data_size,
    void* data);

/**
 * Tastendruecke an ein virtuelles Terminal senden
 *
 * @param vterm     Terminal-Handle
 * @param keycode   Der Keycode
 * @param release   TRUE wenn die Taste losgelassen wurde, FALSE sonst
 */
static void send_key_to_vterm(vterminal_t* vterm, uint8_t keycode,
    bool release);



/**
 * Eingabe initialisieren
 *
 * @return TRUE bei Erfolg, FALSE sonst
 */
bool init_input()
{
    // RPC-Handler fuer die Callbacks von KBD einrichten
    register_message_handler(KBD_RPC_CALLBACK, rpc_kbd_callback);

    kbc_pid = init_service_get("kbc");
    if (!kbc_pid) {
        return FALSE;
    }

    // Callback registrtrieren bei KBC
    return rpc_get_dword(kbc_pid, KBD_RPC_REGISTER_CALLBACK, 0, NULL);
}

/**
 * Calback handler
 */
static void rpc_kbd_callback(pid_t pid, dword cid, size_t data_size,
    void* data)
{
    uint8_t* kbd_data = data;
    vterminal_t* vterm = current_vterm;

    if ((pid != kbc_pid) || (data_size != 2)) {
        return;
    }

    // Mit den statischen Variablen in send_key_to_vterm, in den vterm-Handles
    // und Spaeter in der vt100 Emulation kommt das nicht gut, wenn da ein Neues
    // kommt, solange das aktuelle noch nicht fertig verarbeitet ist.
    p();
    send_key_to_vterm(vterm, kbd_data[0], !kbd_data[1]);
    v();
}

/**
 * Tastendruecke an ein virtuelles Terminal senden
 *
 * @param vterm     Terminal-Handle
 * @param keycode   Der Keycode
 * @param down      TRUE wenn die Taste gedrueckt wurde, FALSE sonst
 */
static void send_key_to_vterm(vterminal_t* vterm, uint8_t keycode,
    bool down)
{
    static struct modifiers modifiers = {
        .shift      = FALSE,
        .control    = FALSE,
        .alt        = FALSE,
        .altgr      = FALSE
    };
    keymap_entry_t* e;
    int len;
    char buf[32];
    wchar_t c = 0;

    // Modifier-Tasten Verarbeiten
    switch (keycode) {
        case KEYCODE_SHIFT_LEFT:
        case KEYCODE_SHIFT_RIGHT:
            modifiers.shift = down;
            return;

        case KEYCODE_CONTROL_LEFT:
        case KEYCODE_CONTROL_RIGHT:
            modifiers.control = down;
            return;

        case KEYCODE_ALT:
            modifiers.alt = down;
            return;

        case KEYCODE_ALTGR:
            modifiers.altgr = down;
            return;
    }

    // Von hier an brauchen wir die losgelassenen Tasten nicht mehr
    if (!down) {
        return;
    }

    // Umschalten zwischen virtuellen Terminals (Alt + Fn), scrollen usw.
    if (vterm_process_raw_input(keycode, &modifiers)) {
        return;
    }

    // Eingabe an vt100-Emulation weiterleiten
    len = vt100_process_input(current_vterm, keycode, buf, sizeof(buf));
    if (len != -1) {
        vterm_process_input(buf, len);
        return;
    }

    // Zeichen auswaehlen
    e = keymap_get(keycode);
    // TODO: AltGr+Shift
    if (modifiers.shift) {
        c = e->shift;
    } else if (modifiers.altgr) {
        c = e->altgr;
    } else if (modifiers.control) {
        c = e->ctrl;
    } else {
        c = e->normal;
    }
    // TODO: Alt


    // FIXME: Mit einem richtigen wc tut das nicht, aber dafuer brauche ich erst
    // ein wctomb.
    if (c != 0) {
        vterm_process_input((char*) &c, 1);
    }
}

