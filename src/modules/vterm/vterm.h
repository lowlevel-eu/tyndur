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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "lostio.h"

#define LOSTIO_TYPES_OUT 255
#define LOSTIO_TYPES_IN 254
#define LOSTIO_TYPES_KEYMAP 253

#define SCREEN_WIDTH_MAX 80
#define SCREEN_HEIGHT_MAX 25

#define VT100_BUFFER_LEN 16

/// LostIO-Interface vorbereiten
void init_lostio_interface(void);

/// Output-Treiber vorbereiten
void init_output(void);

/// Eingabe initialisieren
bool init_input(void);

// Farbe im Videospeicher
typedef union {
    uint8_t raw;
    struct {
        unsigned char foreground : 3;
        unsigned char bold : 1;
        unsigned char background : 3;
        unsigned char blink : 1;
    } __attribute__((packed));
} __attribute__((packed)) con_color_t;

// Zelle im Videospeicher
typedef struct {
    char ascii;
    con_color_t color;
} __attribute__((packed)) videomem_cell_t;



typedef videomem_cell_t out_buffer_t;
typedef struct position {
    /// Zeile
    int line;

    /// Spalte
    int column;
} position_t;

typedef struct vterm_output {
    /// Groesse des Puffers in Zeilen
    int buffer_lines;

    /// Letzte Zeile im Puffer vor dem Ringpuffer-Umbruch
    int buffer_last_line;

    /// Position im Puffer
    struct position buffer_pos;
    
    /// Aktuelle Farbe
    con_color_t current_color;

    /// Der eigentliche Puffer
    videomem_cell_t* buffer;
    
    /// true falls der Buffer gerade auf dem Bildschirm ist.
    bool active;
    
    /// Groesse des Bildschirms
    size_t screen_width;
    size_t screen_height;

    /// Oberste Zeile auf dem Bildschirm
    int screen_topline;

    /// Wird auf true gesetzt falls der Bildschirm neu gezeichnet werden soll
    bool screen_redraw;

    /// Scroll-Lock
    bool scroll_lock;
    

    /// true falls vt100_pos_backup und vt100_color_backup gueltig sind
    bool vt100_backup_valid;

    /// Backupposition fuer vt100-Emulation
    position_t vt100_pos_backup;

    /// Kopie der Farbe fuer vt100
    con_color_t vt100_color_backup;

    /// Wenn auf true gesetzt wurde die Farbe per vt100 invertiert
    bool vt100_color_reversed;
} vterm_output_t;


/// Typ fuer ein  Terminal
typedef struct {
    /// Name des Terminals
    char* name;

    /// Shortcut mit dem das Terminal aktiviert werden kann
    char shortcut;

    /// Struktur fuer den ganzen Ausgabekram
    vterm_output_t output;

    /// LostIO-Node fuer out-Datei
    vfstree_node_t* out_node;

    /// Groesse des Eingabepuffers
    size_t in_buffer_size;

    /// LIO-Offset, an dem der Eingabepuffer anfängt
    uint64_t in_buffer_offset;

    /// Pointer auf den Eingabenpuffer.
    char* in_buffer;

    /// LostIO-Node fuer in-Datei
    vfstree_node_t* in_node;

    /// Eingegebene Ziffern sofort ausgeben
    bool input_echo;

    /// vt100-Puffer
    char vt100_buffer[VT100_BUFFER_LEN];

    /// Anzahl der Zeichen im vt100-Puffer
    size_t vt100_buffer_offset;

    /// UTF-8 Puffer
    char utf8_buffer[4];

    /// Anzahl der Zeichen im utf-8 Puffer
    size_t utf8_buffer_offset;

    /// Beim naechsten Lesen EOF setzen
    bool set_eof;

    /// LostIO-Ressource für Tastatureingaben
    struct lio_resource* lio_in_res;
} vterminal_t;

/// Status der Modifiertasten
struct modifiers {
    bool shift;
    bool control;
    bool altgr;
    bool alt;
};

/// opaque-Struktur für LIO-Knoten
struct lio_vterm {
    /// LOSTIO_TYPES_*
    int type;

    /// Terminal, zu dem der Knoten gehört
    vterminal_t* vterm;
};

/// Neues vterm bei LostIO registrieren
void lio_add_vterm(vterminal_t* vterm);

/// Informiert LostIO, dass es @length neue Zeichen zu lesen gibt
void lio_update_in_file_size(vterminal_t* vterm, size_t length);

/// Informiert LostIO, dass ein EOF gesendet werden soll
void lio_set_eof(vterminal_t* vterm);



extern vterminal_t* current_vterm;
extern list_t* vterm_list;

/// Virtuelle terminals einrichten
void init_vterminals(unsigned int count);

/// Ausgabe fuer ein virtuelles Terminal initialisieren
bool vterm_output_init(vterminal_t* vterm, size_t buffer_lines);

/// Wird beim Wechsel zu einem neuen virtuellen Terminal aufgerufen
void vterm_output_change(vterminal_t* old, vterminal_t* new);

/// Ausgaben in ein virtuelles Terminal verwalten
void vterm_process_output(vterminal_t* vterm, char* data, size_t length);

/// Eingaben verarbeiten
void vterm_process_input(char* data, size_t length);

/// Text ohne vt100-Emulation ausgeben
void vterm_output_text(vterminal_t* vterm, char* data, size_t length);


/// Ausgabeposition aebdern
void output_set_position(vterm_output_t* out, size_t line, size_t column,
                         bool forward);


/// Alle Zeichen loeschen die bis zu count Zeichen von der aktuellen Position
/// entfernt sind.
void output_clear(vterm_output_t* out, size_t count);

/// An der aktuellen Cursorposition neue Zeilen einfügen
void output_insert_lines(vterm_output_t* out, int num);

/// Bildschirmfenster verschieben
void screen_scroll(vterm_output_t* out, int lines);

/// Bildschirmfenster durch den Benutzer verschieben (aktivert ein paar Checks)
void screen_user_scroll(vterm_output_t* out, int lines);

/// Pufferposition in Bildischmposition umrechnen
bool screen_position(vterm_output_t* out, position_t buffer_pos,
    position_t* position);

/**
 * Diese Funktion ist das Gegenstueck zu screen_position. Sie errechnet anhand
 * einer gegebenen Bildschirm-Position die Position im Puffer. Im Gegensatz zu
 * screen_position kann diese Funktion aber nicht fehlschlagen, da der
 * Bildschirm immer irgendwo innerhalb des Puffers sein muss.
 *
 * @param screen_pos Position auf dem Bildschirm
 * @param dest_pos Pointer auf die Positionsstruktur, in der das Ergebnis
 *                  abgelegt werden soll.
 */
static inline void buffer_position(vterm_output_t* out, position_t screen_pos,
    position_t* dest_pos)
{
    dest_pos->line = out->screen_topline + screen_pos.line;
    dest_pos->line %= out->buffer_lines;
    dest_pos->column = screen_pos.column;
}

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
    struct modifiers* mod, char* buffer, size_t buffer_sz);


/// Ausgabe in vt100-Emulation verarbeiten
void vt100_process_output(vterminal_t* vterm, char* data, size_t length);

/// UTF8-String in Codepage437-String verwandeln
int utf8_to_cp437(vterminal_t* vterm, const char* str, size_t len, char* buf);

/// Eigaben in der Verwaltung fuer die virtuellen Terminals verarbeiten
bool vterm_process_raw_input(uint8_t keycode, struct modifiers* modifiers);

