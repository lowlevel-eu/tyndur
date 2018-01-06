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

#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <rpc.h>
#include <lostio.h>
#include "vterm.h"
#include <sleep.h>
#include <ports.h>

#define PORT_HW_CURSOR 0x3D4

out_buffer_t* video_memory;

static void screen_draw(vterm_output_t* out);
static void screen_draw_line(vterm_output_t* out, size_t line);
static void screen_adjust(vterm_output_t* out, position_t pos);
static void screen_cursor_update(vterm_output_t* out);
static inline void buffer_cell_set(vterm_output_t* out, out_buffer_t* buffer,
    position_t position, videomem_cell_t c);
static inline videomem_cell_t buffer_cell_get(vterm_output_t* out,
    out_buffer_t* buffer, position_t position);
static void buffer_line_clear(vterm_output_t* out, out_buffer_t* buffer,
    size_t line);
static void output_add_cell(vterm_output_t* out, videomem_cell_t c,
                            bool append_line);
static void output_newline(vterm_output_t* out);


/**
 * Ausgabetreiber allgemein vorbereiten
 */
void init_output()
{
    video_memory = mem_allocate_physical(SCREEN_WIDTH_MAX * SCREEN_HEIGHT_MAX *
        sizeof(videomem_cell_t), 0xb8000, 0);

    if (request_ports(PORT_HW_CURSOR, 2) == false) {
        puts("[ vterm ] Konnte Port fuer Hardware-Cursor nicht registrieren.");
        exit(-1);
    }
}


/**
 * Ausgabe fuer ein vterminal vorbereiten
 */
bool vterm_output_init(vterminal_t* vterm, size_t buffer_lines)
{
    size_t buffer_size;
    size_t i;
    videomem_cell_t c;
    vterm_output_t* out = &(vterm->output);

    // Struktur initialisieren
    out->buffer_lines = buffer_lines;
    out->buffer_pos.line = 0;
    out->buffer_pos.column = 0;
    
    out->active = false;
    
    out->screen_redraw = false;
    out->screen_topline = 0;
    
    // Im Moment wird standardmaessig der 80x25 Texmode benutzt. Eine
    // Moeglichkeit um in den 90x50 Modus zu wechseln waere praktisch.
    out->screen_width = 80;
    out->screen_height = 25;
    out->buffer_last_line = out->screen_height - 1;
    
    out->scroll_lock = false;
    
    // Standardschriftart: Grau auf schwarz
    out->current_color.raw = 0x7;
    
    out->vt100_backup_valid = false;

    // Puffer vorbereiten
    buffer_size = sizeof(videomem_cell_t) * out->screen_width * buffer_lines;
    out->buffer = malloc(buffer_size);
    if (out->buffer == NULL) {
        return false;
    }


    c.color = out->current_color;
    c.ascii = ' ';
    for (i = 0; i < out->screen_width * buffer_lines; i++) {
        out->buffer[i] = c;
    }

    return true;
}

/**
 * Wird beim Wechsel zu einem neuen virtuellen Terminal aufgerufen
 */
void vterm_output_change(vterminal_t* old, vterminal_t* new)
{
    // Nur wenn vorher ueberhaupt ein terminal aktiviert war, muss es jetzt auf
    // inaktiv gesetzt werden
    if (old != NULL) {
        old->output.active = false;
    }

    new->output.active = true;
    screen_draw(&new->output);
    screen_cursor_update(&new->output);
}

/**
 * Ausgaben eines Prozesses fuer ein Virtuelles Terminal verarbeiten
 * 
 * @param data Pointer auf die auszugebenden Daten
 * @param length Laenge der auszugebenden Daten
 */
void vterm_process_output(vterminal_t* vterm, char* data, size_t length)
{
    vterm_output_t* out = &(vterm->output);
    char cpdata[length + vterm->utf8_buffer_offset];
    int len;

    if ((len = utf8_to_cp437(vterm, data, length, cpdata)) == 0) {
        return;
    }

    // Ausgaben durch vt100-Emulation taetigen
    vt100_process_output(vterm, cpdata, len);

    // Anzeige aktualisieren falls es sich um aenderungen auf dem aktiven
    // Terminal handelt.
    screen_adjust(out, vterm->output.buffer_pos);
    if (out->active) {
        screen_cursor_update(out);
        
        if (out->screen_redraw) {
            out->screen_redraw = false;
            screen_draw(out);
        }
    }

}

/**
 * Ausgaben eines Prozesses in Form von reinem Text ausgeben
 * 
 * @param data Pointer auf die auszugebenden Daten
 * @param length Laenge der auszugebenden Daten
 */
void vterm_output_text(vterminal_t* vterm, char* data, size_t length)
{
    size_t i;
    videomem_cell_t c;
    vterm_output_t* out = &(vterm->output);

    c.color = out->current_color;

    // Zeichen einzeln ausgeben
    for (i = 0; i < length; i++) {
        if (data[i] == '\n') {
            output_newline(out);
        } else if (data[i] == '\r') {
            output_set_position(out, out->buffer_pos.line, 0, false);
        } else if(data[i] == '\t') {
            output_set_position(out, out->buffer_pos.line,
                out->buffer_pos.column + (8 - (out->buffer_pos.column % 8)),
                true);
        } else if (data[i] == '\a') {
            // TODO: Hier muesste man irgendwann noch Bimmeln einbauen ;-)
        } else {
            c.ascii = data[i];
            output_add_cell(out, c, true);
        }
    }
}



/**
 * Position auf dem Bildschirm anhand einer beliebigen Position im Puffer
 * errechnen. Das klappt natuerlich nur, wenn die Position im Puffer auch
 * irgendwo auf dem Bildschirm ist.
 *
 * @param buffer_pos Position im Puffer
 * @param dest_pos Pointer auf die Positionsstruktur, in der das Ergebnis
 *                  abgelegt werden soll.
 *
 * @return true wenn die Position auch auf dem Bildschirm liegt, false sonst.
 */
bool screen_position(vterm_output_t* out, position_t buffer_pos,
    position_t* dest_pos)
{
    size_t screen_bottom = (out->screen_topline + out->screen_height - 1) %
        out->buffer_lines;;

    // Wenn die Position ausserhalb des angezeigten Bereiches liegt, ist der
    // Fall klar.
    if (((screen_bottom > out->screen_topline) && (
            (buffer_pos.line < out->screen_topline) || 
            (buffer_pos.line > screen_bottom))
        ) ||
        ((screen_bottom < out->screen_topline) &&
            (buffer_pos.line > screen_bottom) &&
            (buffer_pos.line < out->screen_topline)
        ))
        
    {
        return false;
    }

    // Wenn die Position auf dem Bildschirm liegt, kann sie ganz einfach
    // errechnet werden:
    if (screen_bottom > out->screen_topline) {
        dest_pos->line = buffer_pos.line - out->screen_topline;
    } else {
        dest_pos->line = (out->buffer_lines - out->screen_topline +
            buffer_pos.line) % out->buffer_lines;
    }
    dest_pos->column = buffer_pos.column;

    return true;
}

/**
 * Position des Hardware-Cursors anpassen
 */
static void screen_cursor_update(vterm_output_t* out)
{
    static bool cursor_enabled = false;
    uint16_t position = 0;
    position_t pos;

    // Position errechnen
    if (screen_position(out, out->buffer_pos, &pos) == true) {
        position = pos.line * out->screen_width + pos.column;
        if (!cursor_enabled) {
            outb(PORT_HW_CURSOR, 10);
            outb(PORT_HW_CURSOR + 1, 0x4e);
            cursor_enabled = true;
        }
    } else if (cursor_enabled) {
        outb(PORT_HW_CURSOR, 10);
        outb(PORT_HW_CURSOR + 1, 0x20);
        cursor_enabled = false;
    }

    // Hardware Cursor verschieben
    outb(PORT_HW_CURSOR, 15);
    outb(PORT_HW_CURSOR + 1, position);
    outb(PORT_HW_CURSOR, 14);
    outb(PORT_HW_CURSOR + 1, position >> 8);
}

/**
 * Den richtigen Teil des Buffers auf in den Videospeicher kopieren.
 */
static void screen_draw(vterm_output_t* out)
{
    size_t i;
    for (i = 0; i < out->screen_height; i++) {
        screen_draw_line(out, i);
    }
}

/**
 * Eine einzelne Zeile aus dem Puffer direkt in den Videospeicher kopieren
 *
 * @param line Zeilennummer auf dem Bildschirm
 */
static void screen_draw_line(vterm_output_t* out, size_t line)
{
    size_t i;
    position_t screen_pos;
    position_t buffer_pos;
    videomem_cell_t c;

    // Nein groessere Zeilennummern als die Bildschirmhoehe gibts nicht.
    if (line >= out->screen_height) {
        return;
    }
    
    screen_pos.line = line;
    
    for (i = 0; i < out->screen_width; i++) {
        // Position aktualisieren
        screen_pos.column = i;

        // Bufferposition aktualisieren und Zeichen aus dem Buffer auslesen
        buffer_position(out, screen_pos, &buffer_pos);
        c = buffer_cell_get(out, out->buffer, buffer_pos);

        // Zeichen in den Videospeicher kopieren
        buffer_cell_set(out, video_memory, screen_pos, c);
    }
}

/**
 * Bildschirmfenster um @lines Zeilen herunterschieben und Bildschirminhalt
 * entsprechend anpassen.
 * 
 * @param lines Anzahl der Zeilen um die der Inhalt verschoben werden soll.
 */
void screen_scroll(vterm_output_t* out, int lines)
{
    // TODO: Optimierungspotential ;-)
    out->screen_topline += lines + out->buffer_lines;
    out->screen_topline %= out->buffer_lines;

    // Der Bildschrim muss garantiert neu gezeichnet werden
    // TODO: Da sie Funktion auch von aussen aufgerufen wird, muss hier im
    // Moment neu gezeichnet werden. Hier waere vielleicht eine eigene Funktion
    // dafuer nicht schlecht.
    // out->screen_redraw = true;
    if (out->active) {
        screen_draw(out);
        screen_cursor_update(out);
    }
}

/**
 * Prueft, ob sich ein bestimmter Bereich im Buffer mit einer Zeile
 * ueberschneidet. Dies kann genutzt werden, um beim Scrollen festzustellen, ob
 * im Ringpuffer eine Umdrehung voll ist und das Scrollen die aktuelle unterste
 * Zeile kreuzen wuerde.
 */
static bool crosses_current_line(vterm_output_t* out, int start, int length)
{
    int current = out->buffer_last_line;
    int end = (start + length) % out->buffer_lines;

    if (start < end) {
        return (current >= start) && (current <= end);
    } else {
        return (current >= start) || (current <= end);
    }
}

/**
 * Bildschirmfenster um @lines Zeilen herunterschieben und Bildschirminhalt
 * entsprechend anpassen. Dabei wird sichergestellt, dass nicht im Kreis
 * gescrollt wird.
 *
 * @param lines Anzahl der Zeilen um die der Inhalt verschoben werden soll.
 */
void screen_user_scroll(vterm_output_t* out, int lines)
{
    int cur_bottom = out->buffer_last_line;
    int win_top = out->screen_topline;
    int win_bottom = win_top + SCREEN_HEIGHT_MAX - 1;

    // Nicht unter aktuelle Zeile runterscrollen
    if ((lines > 0) && (crosses_current_line(out, win_bottom, lines))) {
        lines = (cur_bottom - win_bottom);
        lines = (lines + out->buffer_lines) % out->buffer_lines;
    }

    // Nicht ueber oberste Zeile hochscrollen
    if ((lines < 0) && crosses_current_line(out, win_top + lines, -lines)) {
        lines = (cur_bottom - win_top + 1);
        lines = (lines + out->buffer_lines) % out->buffer_lines;
    }

    screen_scroll(out, lines);
}

/**
 * Dafuer sorgen, dass das Bildschirmfenster so angepasst wird, dass die
 * Pufferposition pos sichtbar ist.
     */
static void screen_adjust(vterm_output_t* out, position_t pos)
{
    position_t screen_pos;
    // Wenn position noch nicht sichtbar ist, wird jetzt entsprechend
    // gescrollt, falls das moeglich ist.
    if ((screen_position(out, pos, &screen_pos) == false) && (out->scroll_lock
        == false))
    {
        out->screen_topline = ((pos.line - out->screen_height + 1) +
            out->buffer_lines) % out->buffer_lines;

        if (out->active) {
            screen_draw(out);
        }
    }
}



/**
 * Ein einzelnes Zeichen in einem Buffer setzen
 * 
 * @param buffer Puffer in den das Zeichen gesetzt werden soll
 * @param positon Position an die das Zeichen gesetzt werden soll
 * @param c Das Zeichen mit Farbinformation
 */
static inline void buffer_cell_set(vterm_output_t* out, out_buffer_t* buffer,
    position_t position, videomem_cell_t c)
{
    buffer[position.line * out->screen_width + position.column] = c;
}

/**
 * Einzelnes Zeichen aus einem Buffer auslesen
 */
static inline videomem_cell_t buffer_cell_get(vterm_output_t* out,
    out_buffer_t* buffer, position_t position)
{
    return buffer[position.line * out->screen_width + position.column];
}

/**
 * Eine Zeile in einem Puffer leeren
 *
 * @param line Zeilennummer
 */
static void buffer_line_clear(vterm_output_t* out, out_buffer_t* buffer,
    size_t line)
{
    position_t position;
    videomem_cell_t c;
    size_t i;
    
    c.color.raw = 0x7;
    c.ascii = ' ';

    position.line = line;
    
    // Jetzt wird sie Zeichenweise geleert
    for (i = 0; i < out->screen_width; i++) {
        position.column = i;
        buffer_cell_set(out, buffer, position, c);
    }
}






/**
 * Aktuelle Position im Puffer aktualisieren
 */
void output_set_position(vterm_output_t* out, size_t line, size_t column,
                         bool forward)
{
    int new_line_unwrapped;
    int old_line = out->buffer_pos.line;
    int i;

    out->buffer_pos.column = column;
    out->buffer_pos.line = line;

    // Zeilenumbruch?
    if (column >= out->screen_width) {
        out->buffer_pos.line += out->buffer_pos.column / out->screen_width;
        out->buffer_pos.column %= out->screen_width;
    }

    new_line_unwrapped = out->buffer_pos.line;
    out->buffer_pos.line %= out->buffer_lines;

    // Wenn der Puffer nach unten erweitert wird, müssen die neu eingefügten
    // Zeilen geleert werden
    if (forward &&
        out->buffer_last_line >= old_line &&
        out->buffer_last_line < new_line_unwrapped)
    {
        i = out->buffer_last_line;
        do {
            i = (i + 1) % out->buffer_lines;
            buffer_line_clear(out, out->buffer, i);
        } while (i != out->buffer_pos.line);

        out->buffer_last_line = out->buffer_pos.line;
    }
}

/**
 * Einzelnes Zeichen hinzufuegen und Bildschirminhalt ggf aktualisieren.
 *
 * @param c Zeichen
 */
static void output_add_cell(vterm_output_t* out, videomem_cell_t c,
                            bool append_line)
{
    position_t screen_pos;
    
    // Zeichen setzen
    buffer_cell_set(out, out->buffer, out->buffer_pos, c);
    
    // Zeichen gegebenenfalls auch noch auf dem Bildschirm ausgeben
    if ((out->active) &&  (screen_position(out, out->buffer_pos, &screen_pos)
        == true))
    {
        // Zeichen in den Videospeicher schreiben
        buffer_cell_set(out, video_memory, screen_pos, c);
    }

    // Position aktalisieren
    output_set_position(out, out->buffer_pos.line, out->buffer_pos.column + 1,
                        append_line);
}

/**
 * Den festgelegten Bereich im Puffer und auf dem Bildschirm leeren
 *
 * @param count Anzahl der zu loeschenden Zeichen
 */
void output_clear(vterm_output_t* out, size_t count)
{
    size_t i;
    videomem_cell_t c;
    // Position retten
    position_t pos = out->buffer_pos;
    
    c.color = out->current_color;
    c.ascii = ' ';
    for (i = 0; i < count; i++) {
        output_add_cell(out, c, false);
    }

    // Position wiederherstellen
    out->buffer_pos = pos;
}

/**
 * Neue Zeile in Ausgabe anfangen
 */
static void output_newline(vterm_output_t* out)
{
    output_set_position(out, out->buffer_pos.line + 1, 0, true);
}

/**
 * An der aktuellen Cursorposition neue Zeilen einfügen
 */
void output_insert_lines(vterm_output_t* out, int num)
{
    int first_scrolled, last_scrolled, i;

    if (num <= 0) {
        return;
    }

    // FIXME Das ist ziemlich blöd so, aber wir haben weder Anfang noch Ende
    // des Ringpuffers, deswegen approximieren wir die Grenze mal mit einem
    // Bildschirm über der aktuellen Cursorposition.
    first_scrolled = out->buffer_pos.line;
    last_scrolled = (first_scrolled - out->screen_height + out->buffer_lines) %
                     out->buffer_lines;

    // Zeilen nach unten verschieben
    i = last_scrolled;
    while (true) {
        int j = (i + out->buffer_lines - num) % out->buffer_lines;

        memcpy(&out->buffer[i * out->screen_width],
               &out->buffer[j * out->screen_width],
               sizeof(*out->buffer) * out->screen_width);

        if (j == first_scrolled) {
            break;
        }

        i = (i + out->buffer_lines - 1) % out->buffer_lines;
    }

    // Zeilen löschen
    i = first_scrolled;
    while (num--) {
        buffer_line_clear(out, out->buffer, i);
        i = (i + 1) % out->buffer_lines;
    }

    out->screen_redraw = true;
}
