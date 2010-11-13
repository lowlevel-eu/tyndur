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
#include <string.h>
#include <kbd.h>

#include "vterm.h"


/**
 * Eingabe fuer vt100-Emulation verarbeiten
 *
 * @param vterm     VTerm-Handle
 * @param keycode   Keycode der gedrueckten Taste
 * @param mod       Gedrueckte Modifier-Tasten
 * @param buffer    Puffer in dem das Resultat abgelegt werden kann
 * @param buffer_sz Groesse des Puffers
 *
 * @return Anzahl der Zeichen die neu im Puffer sind oder -1 wenn die Emulation
 *         nichts mit der Taste anfangen kann
 */
int vt100_process_input(vterminal_t* vterm, uint8_t keycode,
    struct modifiers* mod, char* buffer, size_t buffer_sz)
{
    const char* res = NULL;

    // TODO: vt52-Mode und Cursor Key Mode
    switch (keycode) {
        case KEYCODE_ARROW_UP:
            // vt52: <ESC>A  w CKM: \033[A  wo CKM: \033OA
            if (mod->control) {
                res = "\033[1;5A";
            } else {
                res = "\033[A";
            }
            break;

        case KEYCODE_ARROW_DOWN:
            // vt52: <ESC>B  w CKM: \033[B  wo CKM: \033OB
            if (mod->control) {
                res = "\033[1;5B";
            } else {
                res = "\033[B";
            }
            break;

        case KEYCODE_ARROW_RIGHT:
            // vt52: <ESC>C  w CKM: \033[C  wo CKM: \033OC
            if (mod->control) {
                res = "\033[1;5C";
            } else {
                res = "\033[C";
            }
            break;

        case KEYCODE_ARROW_LEFT:
            // vt52: <ESC>D  w CKM: \033[D  wo CKM: \033OD
            if (mod->control) {
                res = "\033[1;5D";
            } else {
                res = "\033[D";
            }
            break;

        case KEYCODE_HOME:
            // TODO: Wo ist der definiert? Der originale vt100 scheint keine
            // solche Taste gehabt zu haben (Sequenz von Linux wird hier benutzt)
            res = "\033[H";
            break;

        case KEYCODE_END:
            // TODO: Siehe KEYCODE_HOME
            res = "\033[F";
            break;

        case KEYCODE_PAGE_UP:
            // TODO: Siehe KEYCODE_HOME
            res = "\033[5~";
            break;

        case KEYCODE_PAGE_DOWN:
            // TODO: Siehe KEYCODE_HOME
            res = "\033[6~";
            break;

        case KEYCODE_INSERT:
            // TODO: Siehe KEYCODE_HOME
            res = "\033[2~";
            break;

        case KEYCODE_DELETE:
            // TODO: Siehe KEYCODE_HOME
            res = "\033[3~";
            break;

        case KEYCODE_F1:
            res = "\033OP";
            break;

        case KEYCODE_F2:
            res = "\033OQ";
            break;

        case KEYCODE_F3:
            res = "\033OR";
            break;

        case KEYCODE_F4:
            res = "\033OS";
            break;

        case KEYCODE_F5:
            res = "\033[15~";
            break;

        case KEYCODE_F6:
            res = "\033[17~";
            break;

        case KEYCODE_F7:
            res = "\033[18~";
            break;

        case KEYCODE_F8:
            res = "\033[19~";
            break;

        case KEYCODE_F9:
            res = "\033[20~";
            break;

        case KEYCODE_F10:
            res = "\033[21~";
            break;

        case KEYCODE_F11:
            res = "\033[23~";
            break;

        case KEYCODE_F12:
            res = "\033[24~";
            break;
    }

    if (res) {
        strncpy(buffer, res, buffer_sz);
        return strlen(res);
    }

    return -1;
}

