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
vterminal_t* vterm_create(const char* shortcut);

/// Virtuelles Terminal wechseln
void vterm_change(vterminal_t* vterm);

void init_vterminals(unsigned int count)
{

    const char* shortcuts[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9",
        "0", "'", "^"};

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
vterminal_t* vterm_create(const char* shortcut)
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
    vterm->shortcut = malloc(strlen(shortcut) + 1);
    memcpy(vterm->shortcut, shortcut, strlen(shortcut) + 1);

    // vterm an Liste Anhaengen
    vterm_list = list_push(vterm_list, vterm);
    

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
        current_vterm = vterm;
        
        // ANSI-Sequenz zum leeren des Bildschirms
        fwrite("\033[2J", 1, 4, output);

        // Ausgabenkanal-Buffer anzeigen
        fwrite(vterm->out_buffer, 1, vterm->out_buffer_size, output);
    }
}

/**
 * Ausgaben eines Prozesses fuer ein Virtuelles Terminal verarbeiten
 * 
 * @param data Pointer auf die auszugebenden Daten
 * @param length Laenge der auszugebenden Daten
 */
void vterm_process_output(vterminal_t* vterm, char* data, size_t length)
{
    // Wenn es sich um das aktuelle Terminal handelt, werden die Daten gleich
    // ausgegeben
    if (vterm == current_vterm) {
        fwrite(data, 1, length, output);
    }

    // Die Daten auch Puffer kopieren, dazu muss der zuerst vergroessert werden
    // FIXME: potentielles memleak
    vterm->out_buffer_size += length;
    vterm->out_buffer = realloc(vterm->out_buffer, vterm->out_buffer_size);

    // Daten in den Puffer kopieren
    memcpy(vterm->out_buffer + vterm->out_buffer_size - length, data, length);
}

/**
 * Eingaben verarbeiten
 *
 * @param data Pointer auf die eingegebenen Daten
 * @param length Laenge der eingebenen Daten
 */
void vterm_process_input(char* data, size_t length)
{
    static bool wait_switch = FALSE;
    static bool last_switch = FALSE;
    
    // Falls die Taste zum Terminalwechsel gedrueckt wurde, wird versucht zu
    // wechseln
    if (wait_switch == TRUE) {
        wait_switch = FALSE;

        // Verhindert Probleme, wenn Escape gedrueckt wurde, und dann nicht
        // gewechselt wurde
        last_switch = TRUE;

        vterminal_t* vterm = NULL;
        int i = 0;
        // Pruefe ob ein Passendes Terminal vorhanden ist
        while ((vterm = list_get_element_at(vterm_list, i++))) {
            if (length >= strlen(vterm->shortcut) && (strncmp(data, vterm->
                shortcut, strlen(vterm->shortcut)) == 0))
            {
                break;
            }
        }
        
        // Wenn nein wird abgebrochen und ganz normal ausgegeben
        if (vterm == NULL) {
            vterm_process_input(data, length);
            return;
        }
        
        // Falls doch etwas gefunden wurde, wird gewechselt.
        vterm_change(vterm);
        
        // Es wurde doch erfolgreich gewechselt, also darf das nächste Escape
        // wieder benutzt werden
        last_switch = FALSE;

        // Wenn noch mehr Daten da sind werden die ausgegeben
        size_t shortcut_len = strlen(vterm->shortcut);
        if (length - shortcut_len != 0) {
            vterm_process_input(data + shortcut_len, length - shortcut_len);
        }
    } else {    
        // Nach aufforderungen zum Terminal-Wechseln suchen
        char* chg = memchr(data, 27, length);
        
        // Wenn escape gedrueckt wurde, und nicht gewechselt wurde, darf ein
        // weiteres escape nicht benutzt werden
        if ((last_switch == TRUE) && (chg == data)) {
            chg = NULL;
        }
        last_switch = FALSE;

        // Falls eine vorhanden ist, wird bis davor ausgegeben
        if (chg != NULL) {
            // Wenn noch Daten davor da sind, werden diese verarbeitet
            if (chg != data) {
                vterm_process_input(data, chg - data);
                length -= chg - data;
                data += chg - data;
            }
            
            wait_switch = TRUE;
            
            // Wechselaufforderung ueberspringen
            data++;
            length--;

            // Wenn danach auch noch Daten vorhanden sind, werden diese auch
            // noch verarbeitet
            if (length != 0) {
                vterm_process_input(data, length);
            }
        } else {
            // Die Daten auch Puffer kopieren, dazu muss der zuerst
            // vergroessert werden
            // FIXME: potentielles memleak
            current_vterm->in_buffer_size += length;
            current_vterm->in_node->size = current_vterm->in_buffer_size;
            current_vterm->in_buffer = realloc(current_vterm->in_buffer,
                current_vterm->in_buffer_size);
            // Daten in den Puffer kopieren
            memcpy(current_vterm->in_buffer + current_vterm->in_buffer_size -
                length, data, length);
        }
    }
}

