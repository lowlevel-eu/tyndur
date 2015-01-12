/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "vterm.h"
#include "lostio.h"
#include <collections.h>
#include <syscall.h>
#include <kbd.h>

// FIXME: Muesste der irgendwo deklariert sein?
char* strnstr(const char* s, const char* find, size_t slen);


/// Pointer auf das im Moment aktive Terminal
vterminal_t* current_vterm = NULL;

/// Liste in der alle virtuellen Terminals gespeichert werden
list_t* vterm_list;

/// Virtuelles Terminal erstellen
vterminal_t* vterm_create(char shortcut);

/// Virtuelles Terminal wechseln
void vterm_change(vterminal_t* vterm);

void init_vterminals(unsigned int count)
{

    char shortcuts[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    // Liste fuer alle vterms erstellen
    vterm_list = list_create();

    // Virtuelle Terminals erstellen
    int i;
    for (i = 0; i < count; i++) {
        vterm_create(shortcuts[i]);
    }

    // Zum ersten Terminal wechseln
    vterm_change((vterminal_t*) list_get_element_at(vterm_list,
        list_size(vterm_list) - 1));
}

/**
 * Virtuelles Terminal erstellen
 *
 * @return Pointer auf die vterm-Struktur
 */
vterminal_t* vterm_create(char shortcut)
{
    // vterm-Struktur vorbereiten
    vterminal_t* vterm = malloc(sizeof(vterminal_t));
    if (vterm == NULL) {
        return NULL;
    }
    memset(vterm, 0, sizeof(vterminal_t));
    
    static int next_vterm_id = 0;
    asprintf(&vterm->name, "vterm%d", next_vterm_id++);

    // LostIO-Knoten erstellen
    lio_add_vterm(vterm);
    
    // Shortcut zuweisen
    vterm->shortcut = shortcut;
    
    // Standardmaessig nichts ausgeben
    vterm->input_echo = false;

    // vterm an Liste Anhaengen
    vterm_list = list_push(vterm_list, vterm);
    
    if (vterm_output_init(vterm, 100) == false) {
        // TODO
    }
    return vterm;
}

/**
 * Virtuelles Terminal aktiviern
 *
 * @param vterm Pointer auf die vterm-Struktur
 */
void vterm_change(vterminal_t* vterm)
{
    if (current_vterm != vterm) {
        vterm_output_change(current_vterm, vterm);
        current_vterm = vterm;
    }
}


/**
 * Dem Eingabepuffer ein Zeichen anhaengen
 */
static void input_buffer_append(vterminal_t* vterm, char c)
{
    // FIXME; Ineffizienter konnte ich das nicht loesen. ;-)
    vterm->in_buffer_size += 1;
    vterm->in_buffer = realloc(vterm->in_buffer, vterm->in_buffer_size);
    // Daten in den Puffer kopieren
    vterm->in_buffer[vterm->in_buffer_size - 1] = c;
}

/**
 * Eingaben verarbeiten
 *
 * @param data Pointer auf die eingegebenen Daten
 * @param length Laenge der eingebenen Daten
 */
void vterm_process_input(char* data, size_t length)
{
    size_t i;

    // Eingabe wird zeichenweise verarbeitet
    for (i = 0; i < length; i++) {
        input_buffer_append(current_vterm, data[i]);
    }

    lio_update_in_file_size(current_vterm, length);
}

/**
 * Zu einem virtuellen Terminal anhand seines Shortcuts
 *
 * @param shortcut Shortcut des Terminals
 */
void vterm_switch_to(char shortcut) {
    vterminal_t* vterm_switch = NULL;
    int i = 0;

    // Passendes Terminal suchen
    while ((vterm_switch = list_get_element_at(vterm_list, i++))) {
        if (vterm_switch->shortcut == shortcut) {
            break;
        }
    }

    if (vterm_switch) {
        vterm_change(vterm_switch);
    }
}

/**
 * Eingaben verarbeiten, falls sie eine der Kontroll-Tastenkombinationen
 * beinhaltet.
 *
 * @param vterm     Handle
 * @param keycode   Keycode
 * @param modifiers Struktur mit Status der Modifier-Tasten
 *
 * @return true wenn der Keycode benutzt wurde, sonst false
 */
bool vterm_process_raw_input(uint8_t keycode, struct modifiers* modifiers)
{
    if (modifiers->shift) {
        switch (keycode) {
            case KEYCODE_PAGE_UP:
                screen_user_scroll(&(current_vterm->output), -10);
                return true;

            case KEYCODE_ARROW_UP:
                screen_user_scroll(&(current_vterm->output), -1);
                return true;

            case KEYCODE_PAGE_DOWN:
                screen_user_scroll(&(current_vterm->output), 10);
                return true;

            case KEYCODE_ARROW_DOWN:
                screen_user_scroll(&(current_vterm->output), 1);
                return true;
        }
    } else if (modifiers->alt) {
        switch (keycode) {
            case KEYCODE_F1: vterm_switch_to(1); return true;
            case KEYCODE_F2: vterm_switch_to(2); return true;
            case KEYCODE_F3: vterm_switch_to(3); return true;
            case KEYCODE_F4: vterm_switch_to(4); return true;
            case KEYCODE_F5: vterm_switch_to(5); return true;
            case KEYCODE_F6: vterm_switch_to(6); return true;
            case KEYCODE_F7: vterm_switch_to(7); return true;
            case KEYCODE_F8: vterm_switch_to(8); return true;
            case KEYCODE_F9: vterm_switch_to(9); return true;
            case KEYCODE_F10: vterm_switch_to(10); return true;
            case KEYCODE_F11: vterm_switch_to(11); return true;
            case KEYCODE_F12: vterm_switch_to(12); return true;
        }
    } else if (modifiers->control && (keycode == 32)) {
        // Strg + D = EOF
        current_vterm->set_eof = true;
        lio_set_eof(current_vterm);
        return true;
    } else if (keycode == KEYCODE_SCROLL_LOCK) {
        current_vterm->output.scroll_lock = !current_vterm->output.scroll_lock;
        return true;
    }

    return false;
}

