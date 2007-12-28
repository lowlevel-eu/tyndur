/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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
#include "vterm.h"
#include "lostio.h"
#include <collections.h>
#include <syscall.h>

// FIXME: Muesste der irgendwo deklariert sein?
char* strnstr(const char* s, const char* find, size_t slen);


/// Pointer auf das im Moment aktive Terminal
vterminal_t* current_vterm = NULL;

/// Liste in der alle virtuellen Terminals gespeichert werden
static list_t* vterm_list;

/// Virtuelles Terminal erstellen
vterminal_t* vterm_create(char shortcut);

/// Virtuelles Terminal wechseln
void vterm_change(vterminal_t* vterm);

void init_vterminals(unsigned int count)
{

    char shortcuts[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'',
        '^'};

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
    
    // Knoten fuer LostIO-Schnittstelle erstellen
    size_t name_len = strlen(vterm->name);
    char path[name_len + 7];
    path[0] = '/';
    memcpy(path + 1, vterm->name, name_len + 1);
    
    // Verzeichnis
    vfstree_create_node(path, LOSTIO_TYPES_DIRECTORY, 0, NULL,
        LOSTIO_FLAG_BROWSABLE);
    // In-Node
    memcpy(path + name_len + 1, "/in", 4);
    vfstree_create_node(path, LOSTIO_TYPES_IN, 0, vterm, 0);
    vterm->in_node = vfstree_get_node_by_path(path);

    // Out-Node
    memcpy(path + name_len + 1, "/out", 5);
    vfstree_create_node(path, LOSTIO_TYPES_OUT, 0, vterm, 0);
    vterm->out_node = vfstree_get_node_by_path(path);
    
    // Shortcut zuweisen
    vterm->shortcut = shortcut;
    
    // Standardmaessig nichts ausgeben
    vterm->input_echo = FALSE;

    // vterm an Liste Anhaengen
    vterm_list = list_push(vterm_list, vterm);
    
    if (vterm_output_init(vterm, 100) == FALSE) {
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
    vterm->in_node->size = current_vterm->in_buffer_size;
    vterm->in_buffer = realloc(vterm->in_buffer, vterm->in_buffer_size);
    // Daten in den Puffer kopieren
    vterm->in_buffer[vterm->in_buffer_size - 1] = c;
}

/**
 * Ein zeichen der Eingabe verarbeiten
 */
static void input_process_char(vterminal_t* vterm, char c)
{
    static bool wait_command = FALSE;

    if (wait_command == TRUE) {
        wait_command = FALSE;
        switch (c) {
            // ESC wird momentan zum einleiten von Befehlen Benutzt
            case '\033':
                // Wenn schon einmal ESC gedrueckt wurde, wird ein ESC
                // weitergeleitet.
                input_buffer_append(vterm, c);
                break;
        
            // ESC + s => Scroll lock
            case 's':
                vterm->output.scroll_lock = !vterm->output.scroll_lock;
                break;
        
            // ESC + u => Scroll up
            case 'u':
                screen_scroll(&(vterm->output), -10);
                break;

            // ESC + d => Scroll down
            case 'd':
                screen_scroll(&(vterm->output), 10);
                break;

            // Alles andere => Terminalwechsel
            default: {
                vterminal_t* vterm_switch = NULL;
                int i = 0;

                // Passendes Terminal suchen
                while ((vterm_switch = list_get_element_at(vterm_list, i++))) {
                    if (vterm_switch->shortcut == c) {
                        break;
                    }
                }

                // Wenn kein passendes Terminal gefunden werden konnte, wird
                // das Zeichen normal ausgegeben
                if (vterm_switch == NULL) {
                    input_buffer_append(vterm, c);
                    break;
                }
                
                // Ansonsten wird jetzt gewechselt
                vterm_change(vterm_switch);
                break;
            }
        }
    } else {
        switch (c) {
            // ESC wird momentan zum einleiten von Befehlen Benutzt
            case '\033':
                // Das naechste Zeichen wird zum identifizieren des neuen
                // Terminals benutzt.
                wait_command = TRUE;
                
            break;
        
            // Alles andere
            default:
                input_buffer_append(vterm, c);
                break;
        }
    }
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
        input_process_char(current_vterm, data[i]);
    }
}

