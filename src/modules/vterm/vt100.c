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

#include <stdbool.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <lostio.h>
#include "vterm.h"
#include <sleep.h>

struct vt100_sequence_handler {
    const char* sequence;
    void (*handler)(vterminal_t* vterm, bool have_n1, bool have_n2, int n1,
        int n2);
};

static void vt100_buffer_flush(vterminal_t* vterm);
static bool vt100_buffer_validate(vterminal_t* vterm);
static void vt100_safe_curpos(vterminal_t* vterm, int x, int y);

static void vt100_cursor_up(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_cursor_down(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_cursor_left(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_cursor_right(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_cursor_bol_down(vterminal_t* vterm, bool have_n1,
    bool have_n2, int n1, int n2);
static void vt100_cursor_bol_up(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_cursor_direct(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_cursor_save(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_cursor_restore(vterminal_t* vterm, bool have_n1,
    bool have_n2, int n1, int n2);

static void vt100_clear_screen(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_clear_line(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_insert_line(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);
static void vt100_text_attr(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2);


static struct vt100_sequence_handler handler_list[] = {
    // ANSI
    {"\033[\1A",        vt100_cursor_up},
    {"\033[\1B",        vt100_cursor_down},
    {"\033[\1C",        vt100_cursor_right},
    {"\033[\1D",        vt100_cursor_left},
    {"\033[\1E",        vt100_cursor_bol_down},
    {"\033[\1F",        vt100_cursor_bol_up},
    {"\033[\1;\1f",     vt100_cursor_direct},
    {"\033[\1;\1H",     vt100_cursor_direct},
    {"\033[H",          vt100_cursor_direct},
    {"\033[s",          vt100_cursor_save},
    {"\033[u",          vt100_cursor_restore},

    {"\033[J",          vt100_clear_screen},
    {"\033[\1J",        vt100_clear_screen},
    {"\033[K",          vt100_clear_line},
    {"\033[\1K",        vt100_clear_line},
    {"\033[L",          vt100_insert_line},
    {"\033[\1L",        vt100_insert_line},
    {"\033[m",          vt100_text_attr},
    {"\033[\1m",        vt100_text_attr},
    {"\033[\1;\1m",     vt100_text_attr}

    // VT100
    // ...
};

/**
 * Ausgaben durch vt100-Emulation verarbeiten
 */
void vt100_process_output(vterminal_t* vterm, char* data, size_t length)
{
    char c;
    int i;
    char* last_flush = data;
    size_t text_len = 0;

    // Der String wird jetzt Zeichenweise durchsucht. Nach einem Escape-Zeichen
    // werden die Zeichen so lange gepuffert, bis die Sequenz fertig ist, oder
    // feststeht, dass sie ungueltig ist. In diesem Fall, werden alle Zeichen
    // ausser dem Escape-Zeichen ausgegeben.
    for (i = 0; i < length; i++) {
        c = data[i];
        
        // Die vorherigen Zeichen waren alle normaler Text, imd auch das
        // Aktuelle ist kein Beginn einer Sequenz. Die Zeichen werden also
        // aufbewahrt, bis wir am Ende sind, oder eine Escape-Sequenz anfaengt.
        if ((vterm->vt100_buffer_offset == 0) && (c != '\033')) {
            text_len++;
        } else if (c == '\033') {
            if (vterm->vt100_buffer_offset == 0) {
                // Der Beginn einer Sequenz. Jetzt wird ausgegeben, was bis
                // jetzt gekommen ist.
                if (text_len != 0)  {
                    vterm_output_text(vterm, last_flush, text_len);
                }
            } else {
                // Wir haben noch eine Sequenz im Puffer, die allerdings nich
                // gueltig ist.
                vt100_buffer_flush(vterm);
            }
            last_flush = data + i + 1;
            text_len = 0;

            // Jetzt wird das Escape-Zeichen in den Puffer gelegt.
            vterm->vt100_buffer[0] = '\033';
            vterm->vt100_buffer_offset = 1;
        } else {
            // Ein Zeichen in einer Escape-Sequenz
            
            // Wenn der Puffer voll ist, dann stimmt sowieso was nicht.
            if (vterm->vt100_buffer_offset == VT100_BUFFER_LEN) {
                vt100_buffer_flush(vterm);

                // Das aktuelle Zeichen ist dann reiner Text
                last_flush = data + i;
                text_len = 1;
            } else {
                // Sonst wird das Zeichen an den Buffer angehaengt, und
                // geprueft, ob die Sequenz schon abgeschlossen, oder noch
                // gueltig ist.
                vterm->vt100_buffer[vterm->vt100_buffer_offset] = c;
                vterm->vt100_buffer_offset++;

                if (vt100_buffer_validate(vterm) == false) {
                    // Wenn die Sequenz nicht mehr gueltig ist, wird sie
                    // ausgegeben.
                    vt100_buffer_flush(vterm);
                }

                // Fruehstens das naechste Zeichen ist dann vernuenftiger Text
                last_flush = data + i + 1;
                text_len = 0;
            }
        }
    }

    // Falls noch etwas uebrig blieb, wird das jetzt ausgegeben
    if (text_len != 0) {
        vterm_output_text(vterm, last_flush, text_len);
    }
}

/**
 * VT100-Puffer leeren.
 */
static void vt100_buffer_flush(vterminal_t* vterm)
{
    // Alles ausser dem Escape-Zeichen ausgeben
    if (vterm->vt100_buffer_offset > 1) {
        vterm_output_text(vterm, vterm->vt100_buffer + 1, vterm->
            vt100_buffer_offset - 1);
    }
    vterm->vt100_buffer_offset = 0;
}

/**
 * Pfuefen der VT100-Escapesequenz. Falls sie vollstaendig ist, wird sie
 * ausgefuehrt.
 *
 * @return true falls die Sequenz gueltig ist/war, false sonst
 */
static bool vt100_buffer_validate(vterminal_t* vterm)
{
    // Laufvariable fuer Sequenz
    int i;

    // Laufvariable fuer Zeichen im Sequenzmuste
    int j;

    // true wenn der ensprechende Parameter schon gefunden wurde
    bool have_n1;
    bool have_n2;

    // true wenn ein Semikolon, das zum trennen von Parametern benutzt wird,
    // gefunden wurde
    bool have_sep = false;

    // Die Werte der Parameter; Nur korrekt falls have_nX = true
    int n1;
    int n2;

    bool matches;

    // Pointer auf das Zeichen im vt100-Puffer, das gerade verarbeitet wird. 
    char* cur_pos= vterm->vt100_buffer;
    // Offset im v100-Puffer
    size_t offset = 0;

    // Anzahl der Seqenzen im Array.
    size_t sequences = sizeof(handler_list) / sizeof(struct
        vt100_sequence_handler);

    // Alle Sequenzen in der Liste durchgehen und schauen ob eine passt.
    for (i = 0;  i < sequences; i++) {
        // Initialwerte fuer jeden Durchlauf
        matches = true;
        have_n1 = false;
        have_n2 = false;
        have_sep = false;
        cur_pos= vterm->vt100_buffer;
        offset = 0;

        // Sequenzmuster vergleichen
        for (j = 0; (j <= strlen(handler_list[i].sequence)) && matches; j++) {
            char c = handler_list[i].sequence[j]; 

            // Wenn wir am Bufferende angekommen sind, und das Muster noch
            // nicht zu Ende ist, wird abgebrochen, denn dann ist die Sequenz
            // noch gueltig.
            if ((offset >= vterm->vt100_buffer_offset) && (c != '\0')) {
                break;
            }
            switch (c) {
                case '\1': {
                    int count;
                    int* n;
                    bool* have;

                    // Entscheiden ob wir jetzt an n1 oder n2 rumspielen. Das
                    // haengt allein davon ab, ob der Seperator schon gekommen
                    // ist oder nicht.
                    if (have_sep == false) {
                        n = &n1;
                        have = &have_n1;
                    } else {
                        n = &n2;
                        have = &have_n2;
                    }
                    
                    // Initialwerte, falls keine Ziffern gefunden werden
                    *n = 0;
                    *have = false;
                    count = 0;

                    // Ein numerisches Argument darf mehrere Ziffern haben
                    while ((*cur_pos >= '0') && (*cur_pos <= '9') && (offset <
                        vterm->vt100_buffer_offset)) 
                    {
                        *n = *n * 10 + (*cur_pos - '0');

                        cur_pos++;
                        offset++;
                        count++;
                    }

                    if (count != 0) {
                        *have = true;
                    }
                    break;
                }

                case ';':
                    // Der Seperator _muss_ auch in der Sequenz im Puffer
                    if (*cur_pos != ';') {
                        matches = false;
                        break;
                    }

                    // Mehrere Seperatoren dürfen nicht vorkommen
                    if (have_sep == true) {
                        matches = false;
                    } else {
                        have_sep = true;
                    }
                    cur_pos++;
                    offset++;
                    break;
                    
                case '\0':
                    // Ende der Sequenz erreicht, sie ist vollstaendig im
                    // Puffer und kann verarbeitet werden.
                    handler_list[i].handler(vterm, have_n1, have_n2, n1, n2);
                    // Puffer leeren
                    vterm->vt100_buffer_offset = 0;
                    return true;
                    break;

                default:
                    // Alle anderen Zeichen muessen 1:1 stimmen
                    if (c != *cur_pos) {
                        matches = false;
                    }

                    cur_pos++;
                    offset++;
                    break;
            }
        }
        
        // Diese Seqeuenz ist zwar noch nicht vollstaendig, passt aber bis
        // jetzt.
        if (matches == true) {
            return true;
        }
    }

    // Wenn bis hier nichts gefunden wurde, ist die Sequenz ungueltig
    return false;
}

/**
 * Cursorposiiton setzen; Liegt die Position ausserhalb des sichtbaren
 * Bereichs, wird der Cursor an den naechsten Rand positioniert. Fuer die erste
 * Zeile/Spalte wird die Nummer 0 benutzt
 */
static void vt100_safe_curpos(vterminal_t* vterm, int x, int y)
{
    vterm_output_t* out = &(vterm->output);
    int x_max = out->screen_width - 1;
    int y_max = out->screen_height - 1;
    position_t screen_pos;
    position_t buffer_pos;

    // Raender Oben und Links pruefen
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }

    // Unten und Rechts
    if (x > x_max) {
        x = x_max;
    }
    if (y > y_max) {
        y = y_max;
    }

    // In positionstruktur einfuellen
    screen_pos.line = y;
    screen_pos.column = x;

    buffer_position(out, screen_pos, &buffer_pos);
    output_set_position(out, buffer_pos.line, buffer_pos.column, false);
}


/**
 * ESC[nA
 * Cursor um n1 Zeilen nach oben bewegen. Wenn n1 nicht angegeben wird, wird
 * n1 = 1 angenommen.
 */
static void vt100_cursor_up(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    position_t pos;
    vterm_output_t* out = &(vterm->output);
    
    // Standardwert fuer n1 setzen
    if (!have_n1) {
        n1 = 1;
    }

    // Versuchen die Position auf dem Bildschirm zu ermitteln
    if (screen_position(out, out->buffer_pos, &pos)) {
        if (pos.line >= n1) {
            // Neue Position setzen
            vt100_safe_curpos(vterm, pos.column,
                pos.line - n1);
        }
    }
}

/**
 * ESC[nB
 * Cursor um n1 Zeilen nach unten bewegen. Wenn n1 nicht angegeben wird, wird
 * n1 = 1 angenommen.
 */
static void vt100_cursor_down(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    position_t pos;
    vterm_output_t* out = &(vterm->output);
    
    // Standardwert fuer n1 setzen
    if (!have_n1) {
        n1 = 1;
    }

    // Versuchen die Position auf dem Bildschirm zu ermitteln
    if (screen_position(out, out->buffer_pos, &pos)) {
        if (pos.line < out->screen_height - n1) {
            // Neue Position setzen
            vt100_safe_curpos(vterm, pos.column,
                pos.line + n1);
        }
    }
}

/**
 * ESC[nC
 * Cursor um n1 Spalten nach links bewegen. Wenn n1 nicht angegeben wird, wird
 * n1 = 1 angenommen.
 */
static void vt100_cursor_left(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    position_t pos;
    vterm_output_t* out = &(vterm->output);
    
    // Standardwert fuer n1 setzen
    if (!have_n1) {
        n1 = 1;
    }

    // Versuchen die Position auf dem Bildschirm zu ermitteln
    if (screen_position(out, out->buffer_pos, &pos)) {
        if (pos.column >= n1) {
            // Neue Position setzen
            vt100_safe_curpos(vterm, pos.column - n1,
                pos.line);
        }
    }
}

/**
 * ESC[nD
 * Cursor um n1 Spalten nach rechts bewegen. Wenn n1 nicht angegeben wird, wird
 * n1 = 1 angenommen.
 */
static void vt100_cursor_right(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    position_t pos;
    vterm_output_t* out = &(vterm->output);
    
    // Standardwert fuer n1 setzen
    if (!have_n1) {
        n1 = 1;
    }

    // Versuchen die Position auf dem Bildschirm zu ermitteln
    if (screen_position(out, out->buffer_pos, &pos)) {
        // Neue Position setzen
        vt100_safe_curpos(vterm, pos.column + n1, pos.line);
    }
}

/**
 * ESC[nE
 * Den Cursor an den Anfang der Zeile, die n1 Zeilen unterhalb der Aktuellen
 * ist, setzen. Wenn n1 nicht angegeben wird, wird n1 = 1 angenommen.
 */
static void vt100_cursor_bol_down(vterminal_t* vterm, bool have_n1,
    bool have_n2, int n1, int n2)
{
    vterm_output_t* out = &(vterm->output);
    position_t pos;

    // Standardwert fuer n1 setzen
    if (!have_n1) {
        n1 = 1;
    }

    // Neue Position setzen
    if (screen_position(out, out->buffer_pos, &pos)) {
        vt100_safe_curpos(vterm, 0, pos.line + n1);
    }
}

/**
 * ESC[nF
 * Den Cursor an den Anfang der Zeile, die n1 Zeilen oberhalb der Aktuellen
 * ist, setzen. Wenn n1 nicht angegebern wird, wird n1 = 1 angenommen.
 */
static void vt100_cursor_bol_up(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    vterm_output_t* out = &(vterm->output);
    position_t pos;

    // Standardwert fuer n1 setzen
    if (!have_n1) {
        n1 = 1;
    }

    // Neue Position setzen
    if (screen_position(out, out->buffer_pos, &pos)) {
        vt100_safe_curpos(vterm, 0, pos.line - n1);
    }
}

/**
 * ESC[n;nf
 * ESC[n;nH
 * Cursorposition direkt setzen. n1 = Zeile, n2 = Spalte
 *
 * Wenn keiner der Parameter angegeben wird, wird der Cursor in die obere linke
 * Ecke positioniert.
 */
static void vt100_cursor_direct(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    // Standardwerte hernehmen, wenn nichts angegben wurde
    if (!have_n1) {
        n1 = 1;
    }
    if (!have_n2) {
        n2 = 1;
    }
    
    vt100_safe_curpos(vterm, n2 - 1, n1 - 1);
}

/**
 * ESC[s
 * Cursorposition abspeichern, um sie spaeter mit ESC[u wiederherstellen zu
 * koennen.
 */
static void vt100_cursor_save(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    position_t pos;
    vterm_output_t* out = &(vterm->output);
    
    // Versuchen die Position auf dem Bildschirm zu ermitteln
    if (screen_position(out, out->buffer_pos, &pos)) {
        out->vt100_pos_backup = pos;
        out->vt100_color_backup = out->current_color;
        out->vt100_backup_valid = true;
    }
}

/**
 * ESC[u
 * Cursorposition wiederherstellen
 */
static void vt100_cursor_restore(vterminal_t* vterm, bool have_n1,
    bool have_n2, int n1, int n2)
{
    vterm_output_t* out = &(vterm->output);

    // Wenn die Positionen wirklich gespeichert wurden, werden sie jetzt
    // wiederhergestellt.
    if (out->vt100_backup_valid) {
        buffer_position(out, out->vt100_pos_backup, &(out->buffer_pos));
        out->current_color = out->vt100_color_backup;
        out->vt100_backup_valid = false;
    }
}

/**
 * ESC[#J
 * Bildschirm leeren.
 */
static void vt100_clear_screen(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    size_t count = 0;
    vterm_output_t* out = &(vterm->output);
    position_t screen_pos;

    screen_position(out, out->buffer_pos, &screen_pos);

    if( !have_n1 ) {
        n1 = 0;
    }

    switch( n1 ) {
        case 0:
            // Bildschirm ab dem Cursor leeren
            count = (out->screen_height - screen_pos.line) * out->screen_width
                - screen_pos.column;
            break;
        case 1:
            // Bildschirm bis zum Cursor leeren
            vt100_safe_curpos(vterm,0,0);
            count = screen_pos.line * out->screen_width
                + screen_pos.column + 1;
            break;
        case 2:
            // gesamten Bildschirm leeren
            vt100_safe_curpos(vterm,0,0);
            count = out->screen_height * out->screen_width;
            break;
    }

    // Zeichen loeschen
    if (count != 0) {
        output_clear(out, count);
    }

    // Cursorposition wieder herstellen
    vt100_safe_curpos(vterm,screen_pos.column,screen_pos.line);
}

/**
 * ESC[#K
 * Zeile leeren.
 */
static void vt100_clear_line(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    size_t count = 0;
    vterm_output_t* out = &(vterm->output);

    if( !have_n1 ) {
        n1 = 0;
    }

    int column = out->buffer_pos.column;
    switch(n1) {
        case 0:
            // Zeile ab dem Cursor leeren
            count = out->screen_width - out->buffer_pos.column;
            break;
        case 1:
            // Zeile bis zum Cursor leeren
            count = out->buffer_pos.column + 1;
            out->buffer_pos.column = 0;
            break;
        case 2:
            // gesamte Zeile leeren
            count = out->screen_width;
            out->buffer_pos.column = 0;
    }

    // Zeichen loeschen
    if (count != 0) {
        output_clear(out, count);
    }

    // Cursorposition wieder herstellen
    out->buffer_pos.column = column;
}

/**
 * ESC[#L
 * Zeile einfügen.
 */
static void vt100_insert_line(vterminal_t* vterm, bool have_n1, bool have_n2,
                              int n1, int n2)
{
    output_insert_lines(&vterm->output, have_n1 ? n1 : 1);
}

/**
 * Farben invertieren
 */
static void vt100_reversed_colors(vterminal_t* vterm, bool reverse)
{
    vterm_output_t* out = &(vterm->output);
    int oldfg;

    if (out->vt100_color_reversed == reverse) {
        return;
    }

    // Farben tauschen
    oldfg = out->current_color.foreground;
    out->current_color.foreground = out->current_color.background;
    out->current_color.background = oldfg;

    out->vt100_color_reversed = reverse;
}

/**
 * Parameter fuer Textattribute verarbeiten
 */
static void vt100_text_attr_param(vterminal_t* vterm, int n)
{
    vterm_output_t* out = &(vterm->output);
    // Zum Umrechnen der ANSI Farbcodes in die VGA Palette.
    static char colors[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

    switch(n)
    {
        case 0: // Normal
            vt100_reversed_colors(vterm, false);
            out->current_color.raw = 0x07;
            break;

        case 1: // Fett
            out->current_color.bold = 1;
            break;

        case 5: // Blinken
            out->current_color.blink = 1;
            break;

        case 7: // Hintergrundfarbe auf Vordergrundfarbe
            vt100_reversed_colors(vterm, true);
            break;

        case 30 ... 37: // Vordergrundfarbe
            out->current_color.foreground = colors[n - 30];
            break;

        case 40 ... 47: // Hintergrundfarbe
            out->current_color.background = colors[n - 40];
            break;
    }

}

/**
 * ESC[n;nm
 * Textattribute einstellen
 */
static void vt100_text_attr(vterminal_t* vterm, bool have_n1, bool have_n2,
    int n1, int n2)
{
    if(!have_n1) {
        n1 = 0;
    }
    vt100_text_attr_param(vterm, n1);

    if (have_n2)  {
        vt100_text_attr_param(vterm, n2);
    }
}

