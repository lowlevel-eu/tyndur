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

#include <stdio.h>
#include <types.h>
#include "lostio.h"

#define LOSTIO_TYPES_OUT 255
#define LOSTIO_TYPES_IN 254

// Filehandles fuer Eingabe- und Ausgabekanal
extern FILE* input;
extern FILE* output;


/// LostIO-Interface vorbereiten
void init_lostio_interface();


/// Typ fuer ein  Terminal
typedef struct {
    /// Name des Terminals
    char* name;

    /// Shortcut mit dem das Terminal aktiviert werden kann
    char* shortcut;

    /// Groesse des Ausgabenpuffers
    size_t out_buffer_size;

    /// Pointer auf den Ausgabenpuffer
    char* out_buffer;
    
    /// LostIO-Node fuer out-Datei
    vfstree_node_t* out_node;

    /// Groesse des Eingabepuffers
    size_t in_buffer_size;

    /// Pointer auf den Eingabenpuffer.
    char* in_buffer;

    /// LostIO-Node fuer in-Datei
    vfstree_node_t* in_node;
} vterminal_t;

extern vterminal_t* current_vterm;

/// Virtuelle terminals einrichten
void init_vterminals(unsigned int count);

/// Ausgaben in ein virtuelles Terminal verwalten
void vterm_process_output(vterminal_t* vterm, char* data, size_t length);

/// Eingaben verarbeiten
void vterm_process_input(char* data, size_t length);


