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

static wchar_t deadkey = 0;

static wchar_t deadkey_chars[53][4] = {
  { L'Ã', L'Á', L'À', L'Â' }, //A
  {    0,    0,    0,    0 },
  {    0, L'Ć',    0, L'Ĉ' },
  {    0,    0,    0,    0 },
  { L'Ẽ', L'É', L'È', L'Ê' },
  {    0,    0,    0,    0 },
  {    0, L'Ǵ',    0, L'Ĝ' },
  {    0,    0,    0, L'Ĥ' },
  { L'Ĩ', L'Í', L'Ì', L'Î' },
  {    0,    0,    0, L'Ĵ' },
  {    0, L'Ḱ',    0,    0 },
  {    0, L'Ĺ',    0,    0 },
  {    0, L'Ḿ',    0,    0 },
  { L'Ñ', L'Ń', L'Ǹ',    0 },
  { L'Õ', L'Ó', L'Ò', L'Ô' },
  {    0, L'Ṕ',    0,    0 },
  {    0,    0,    0,    0 },
  {    0, L'Ŕ',    0,    0 },
  {    0, L'Ś',    0, L'Ŝ' },
  {    0,    0,    0,    0 },
  { L'Ũ', L'Ú', L'Ù', L'Û' },
  { L'Ṽ', L'Ǘ', L'Ǜ',    0 },
  {    0, L'Ẃ', L'Ẁ', L'Ŵ' },
  {    0,    0,    0,    0 },
  { L'Ỹ', L'Ý', L'Ỳ', L'Ŷ' },
  {    0, L'Ź',    0, L'Ẑ' },
  { L'ã', L'á', L'à', L'â' }, //a
  {    0,    0,    0,    0 },
  {    0, L'ć',    0, L'ĉ' },
  {    0,    0,    0,    0 },
  { L'ẽ', L'é', L'è', L'ê' },
  {    0,    0,    0,    0 },
  {    0, L'ǵ',    0, L'ĝ' },
  {    0,    0,    0, L'ĥ' },
  { L'ĩ', L'í', L'ì', L'î' },
  {    0,    0,    0, L'ĵ' },
  {    0, L'ḱ',    0,    0 },
  {    0, L'ĺ',    0,    0 },
  {    0, L'ḿ',    0,    0 },
  { L'ñ', L'ń', L'ǹ',    0 },
  { L'õ', L'ó', L'ò', L'ô' },
  {    0, L'ṕ',    0,    0 },
  {    0,    0,    0,    0 },
  {    0, L'ŕ',    0,    0 },
  {    0, L'ś',    0, L'ŝ' },
  {    0,    0,    0,    0 },
  { L'ũ', L'ú', L'ù', L'û' },
  { L'ṽ', L'ǘ', L'ǜ',    0 },
  {    0, L'ẃ', L'ẁ', L'ŵ' },
  {    0,    0,    0,    0 },
  { L'ỹ', L'ý', L'ỳ', L'ŷ' },
  {    0, L'ź',    0, L'ẑ' },
  {  '~', L'´', '`',   '^' }, //space
};

/**
 * Calback handler
 */
static void rpc_kbd_callback(pid_t pid, uint32_t cid, size_t data_size,
    void* data);

/**
 * Tastendruecke an ein virtuelles Terminal senden
 *
 * @param vterm     Terminal-Handle
 * @param keycode   Der Keycode
 * @param release   true wenn die Taste losgelassen wurde, false sonst
 */
static void send_key_to_vterm(vterminal_t* vterm, uint8_t keycode,
    bool release);



/**
 * Eingabe initialisieren
 *
 * @return true bei Erfolg, false sonst
 */
bool init_input()
{
    // RPC-Handler fuer die Callbacks von KBD einrichten
    register_message_handler(KBD_RPC_CALLBACK, rpc_kbd_callback);

    kbc_pid = init_service_get("kbc");
    if (!kbc_pid) {
        return false;
    }

    // Callback registrtrieren bei KBC
    return rpc_get_dword(kbc_pid, KBD_RPC_REGISTER_CALLBACK, 0, NULL);
}

/**
 * Calback handler
 */
static void rpc_kbd_callback(pid_t pid, uint32_t cid, size_t data_size,
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
 * Aus Deadkey und Zeichen das passende Zeichen bestimmen
 *
 * @param c  Zeichen
 */
static wchar_t get_real_key(wchar_t c)
{
    c &= ~KEYMAP_DEADKEY;
    if (c >= 'A' && c <= 'Z') {
	c -= 'A';
    } else if (c >= 'a' && c <= 'z') {
	c -= 'a' - 26;
    } else if (c == ' ' || c == deadkey) {
	c = 52;
    } else {
	deadkey = 0;
	return 0;
    }
    switch (deadkey) {
	case '~':
	    c = deadkey_chars[c][0];
	    break;
	case L'´':
	    c = deadkey_chars[c][1];
	    break;
	case '`':
	    c = deadkey_chars[c][2];
	    break;
	case '^':
	    c = deadkey_chars[c][3];
	    break;
	default:
	    c = 0;
	    break;
    }
    deadkey = 0;
    return c;
}

/**
 * Tastendruecke an ein virtuelles Terminal senden
 *
 * @param vterm     Terminal-Handle
 * @param keycode   Der Keycode
 * @param down      true wenn die Taste gedrueckt wurde, false sonst
 */
static void send_key_to_vterm(vterminal_t* vterm, uint8_t keycode,
    bool down)
{
    static struct modifiers modifiers = {
        .shift      = false,
        .control    = false,
        .alt        = false,
        .altgr      = false
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

    // Wenn alt gedrueckt wurde, senden wir vorher einfach ein ESC
    if (modifiers.alt) {
        vterm_process_input("\033", 1);
    }

    // Eingabe an vt100-Emulation weiterleiten
    len = vt100_process_input(current_vterm, keycode, &modifiers,
        buf, sizeof(buf));
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

    if ((c & KEYMAP_DEADKEY) && !deadkey) {
	deadkey = (c & ~KEYMAP_DEADKEY);
	return;
    } else if (deadkey) {
	c = get_real_key(c);
    }

    if ((c != 0) && ((len = wctomb(buf, c)) != -1)) {
        vterm_process_input(buf, len);
    }
}

