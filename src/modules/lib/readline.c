/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <collections.h>
#include <wchar.h>

#include <readline/readline.h>

#define BUFFER_SIZE 255


/** Typ mit allen Befehlen, die eingegeben werden koennen, fuer readline. */
typedef enum {
    UNKNOWN_COMMAND = 0,

    // Cursor Bewegen
    /// Cursor um ein Zeichen nach vorne bewegen
    FORWARD_CHAR,
    /// Cursor um ein Zeichen nach hinten bewegen
    BACKWARD_CHAR,
    /// Cursor an den Anfang der Zeile bewegen
    BEGINNING_OF_LINE,
    /// Cursor an das Ende der Zeile bewegen
    END_OF_LINE,

    // Text editieren
    /// Zeile abschliessen
    ACCEPT_LINE,
    /// Zeichen unter dem Cursor loeschen
    DELETE_CHAR,
    /// Zeichen hinter dem Cursor loeschen
    BACKWARD_DELETE_CHAR,
    /// Vervollstaendigen
    COMPLETE,

    // History
    /// Eine Zeile zurueck in der History
    PREVIOUS_HISTORY,
    /// Eine Zeile vorwaerts in der History
    NEXT_HISTORY,
} rl_command_t;

/** Eingebaute Tastensequenzen fuer die Befehle */
static struct {
    wint_t*        seq;
    rl_command_t    cmd;
} builtin_commands [] = {
    /// Pfeil rechts
    { L"\033[C", FORWARD_CHAR },
    /// Pfeil links
    { L"\033[D", BACKWARD_CHAR },
    /// Pos1
    { L"\033[H", BEGINNING_OF_LINE },
    /// Ende
    { L"\033[F", END_OF_LINE },

    /// Enter
    { L"\n", ACCEPT_LINE },
    /// Del
    { L"\033[3~", DELETE_CHAR },
    /// Ruecktaste
    { L"\b", BACKWARD_DELETE_CHAR },
    /// Tab
    { L"\t", COMPLETE },

    /// Pfeil hoch
    { L"\033[A", PREVIOUS_HISTORY },
    /// Pfeil runter
    { L"\033[B", NEXT_HISTORY },

    { NULL, UNKNOWN_COMMAND },
};

static list_t* history = NULL;

/**
 * Pointer auf den attemped_completion-Handler, der aufgerufen wird, wenn Tab
 * gedrueckt wird.
 * */
__rl_attemped_completion_func_t* rl_attempted_completion_function = NULL;

/** Pointer auf den Zeilenpuffer von Readline */
char* rl_line_buffer;

/**
 * Einzelnes Zeichen von der Tastatur einlesen
 *
 * @return Das eingelesene Zeichen oder WEOF, wenn das Ende der Eingabedatei
 * erreicht ist.
 */
static wint_t keyboard_read_char(void)
{
    wint_t c = 0;

    while ((c = fgetwc(stdin)) == WEOF) {
        if (feof(stdin)) {
            return WEOF;
        }
    }

    return c;
}

/**
 * Loescht ein Zeichen aus dem Puffer
 */
static void delchar(char* buffer, int pos, int* size)
{
    int i;
    size_t len = strlen(buffer);
    int clen;

    if (len < pos) {
        return;
    }

    clen = mblen(buffer + pos, len - pos + 1);
    for (i = pos; i < BUFFER_SIZE - clen; i++) {
        buffer[i] = buffer[i + clen];
        if (buffer[i] == '\0') {
            break;
        }
    }

    buffer[BUFFER_SIZE - clen] = '\0';
    *size -= clen;
}

/**
 * Fuegt ein Zeichen in den Buffer ein
 */
static void inschar(char* buffer, int* pos, int* size, wint_t c)
{
    int i;
    size_t len = strlen(buffer);
    char mbbuf[MB_CUR_MAX];
    int clen;


    clen = wctomb(mbbuf, c);
    if ((len < *pos) || (len + clen >= BUFFER_SIZE) || (clen == -1)) {
        return;
    }

    for (i = len + clen; i > *pos; i--) {
        buffer[i] = buffer[i - clen];
    }

    memcpy(buffer + *pos, mbbuf, clen);

    *pos += clen;
    *size += clen;
}

/**
 * Veraendert die Pufferposition; positiv => rechts, negativ => links
 */
static void move_cursor(char* buffer, int* pos, int size, int offset)
{
    int clen;

    if (offset >  0) {
        // Rechts
        if ((clen = mblen(buffer + *pos, size - *pos + 1)) > 0) {
            *pos += clen;
        }
    } else {
        // Links, bis wir ein ganzes Zeichen haben
        while (--(*pos) && (mblen(buffer + *pos, size - *pos + 1) == -1)) { }
    }
}

/**
 * Zaehlt die Anzahl der angezeigten Zeichen im Puffer
 *
 * @param start Position von der angefangen werden soll zu zaehlen
 * @param size  Anzahl Zeichen im Puffer
 */
static int count_visible_chars(char* buffer, int start, int size)
{
    int clen;
    int count = 0;

    while ((start < size)) {
        if ((clen = mblen(buffer + start, size - start + 1)) == -1) {
            // Unvollstaendiges MB-Zeichen wird uebersprungen
            start++;
        } else {
            count++;
            start += clen;
        }
    }

    return count;
}

/**
 * Array mit Matches freigeben
 *
 * @param matches Liste
 */
static void free_matches_array(char** matches)
{
    int i;
    char* match;

    for (i = 0; (match = matches[i]); i++) {
        free(match);
    }
    free(matches);
}


/**
 * Wort vervollstaendigen
 * Position im Puffer und Laenge des Inhalts wird angepasst
 *
 * @param buffer    Pointer auf den Zeilenpuffer
 * @param pos       Pointer auf die Variable, die die Position im Puffer
 *                  beinhaltet
 * @param len       Pointer auf die Variable, die die Laenge des Pufferinhalts
 *                  beinheltet.
 *
 * @return TRUE wenn eine Liste mit Matches angezeigt wurde und deswegen die
 *         Zeile umgebrochen wurde, FALSE wenn entweder nichts gemacht wurde,
 *         oder nur ein passender Match geliefert wurde.
 */
static bool complete_word(char* buffer, int* pos, int* len)
{
    int word_pos;
    int match_count;
    int word_len;
    char** matches;
    char* replacement;
    int replacement_len;
    bool matches_list_displayed = FALSE;


    // Zuerst das zu ergaenzende Wort heraussuchen (den Anfang)
    for (word_pos = *pos; ((word_pos == *pos) || (buffer[word_pos] != ' ')) &&
        (word_pos > 0); word_pos--);

    if ((buffer[word_pos] == ' ') && (*pos > word_pos)) {
        word_pos++;
    }
    word_len = *pos - word_pos;

    // Wort in einen eigenen Buffer kopieren
    char word[word_len + 1];
    strncpy(word, buffer + word_pos, word_len);
    word[word_len] = 0;

    matches = rl_attempted_completion_function(word, word_pos, *pos);

    // Wir haben nochmal Glueck gehabt, es gibt nicht zu vervollstaendigen ;-)
    if (matches == NULL) {
        return FALSE;
    }

    // Und sonst tun wir im Moment nur was, wenns genau eine Moeglichkeit
    // gibt.
    for (match_count = 0; matches[match_count] != NULL; match_count++);


    if (match_count > 1) {
        int i;
        char* match;
        int matching_chars;

        // Versuchen soviel zu ergaenzen wie moeglich dafuer werden einfach alle
        // Matches mit dem ersten verglichen und die Anzahl der passenden
        // Zeichen wird immer weiter verringert bis alle Matches gleich sind, in
        // diesem Bereich
        matching_chars = strlen(matches[0]);
        for (i = 0; (match = matches[i]); i++) {
            while (strncmp(matches[0], matches[i], matching_chars) != 0) {
                matching_chars--;
            }
        }

        // Liste mit den Matches anzeigen, wenn garnichts ergaenzt werden konnte
        if (matching_chars <= strlen(word)) {
            printf("\n");
            for (i = 0; (match = matches[i]); i++) {
                printf("%s ", match);
            }
            printf("\n");
            matches_list_displayed = TRUE;
        }


        replacement = matches[0];
        replacement_len = matching_chars;
    } else {
        replacement = matches[0];
        replacement_len = strlen(replacement);
    }

    // Pruefen ob das ganze nachher noch in den Puffer passt
    if (*len + replacement_len > BUFFER_SIZE) {
        return matches_list_displayed;
    }

    // Text, der dem zu ersetzenden Wort folgt nach hinten schieben
    memmove(buffer + word_pos + replacement_len, buffer + *pos, (*len - word_len) -
        word_pos + 1);

    // Wort ersetzen
    memcpy(buffer + word_pos, replacement, replacement_len);

    // Position und Laenge des Puffers korrigieren
    *pos = word_pos + replacement_len;
    *len = *len + replacement_len - word_len ;

    free_matches_array(matches);

    return matches_list_displayed;
}

/**
 * Anfang des uebergebenen Puffers mit moeglichen Befehlstasten vergleichen.
 * Wird eine Uebereinstimmung gefunden, wird der gefundene Befehl in *cmd
 * abgelegt.
 *
 * @return Wird eine vollstaendige Befehlssequenz gefunden, wird die Anzahl
 *         Bytes zurueckgegeben, die die Sequenz in Anspruch genommen hat. Falls
 *         eine unvollstaendige Sequenz am Anfang des Puffers liegt, die aber
 *         mit weiteren Zeichen noch zu einem gueltigen Befehl wird, wird 0
 *         zurueckgegeben. Passt die Sequenz hingegen zu keinem Befehl, wird -1
 *         zurueckgegeben.
 */
static int find_command(const wchar_t* buf, size_t len, rl_command_t* cmd)
{
    int found_part = 0;
    size_t seq_len;
    size_t min_len;
    size_t i;

    for (i = 0; builtin_commands[i].seq; i++) {
        seq_len = wcslen(builtin_commands[i].seq);
        min_len = (len < seq_len ? len : seq_len);

        if (!wcsncmp(buf, builtin_commands[i].seq, min_len)) {
            // Passt genau
            if (seq_len <= len) {
                *cmd = builtin_commands[i].cmd;
                return seq_len;
            } else {
                // Es fehlt noch ein bisschen was, koennte aber noch werden
                found_part++;
            }
        }
    }

    return (found_part > 0 ? 0 : -1);
}

/**
 * Prompt anzeigen und Zeile von der Tastatur einlesen
 */
char* readline(const char* prompt)
{
    if (feof(stdin)) {
        return NULL;
    }

    char* buffer = malloc(BUFFER_SIZE);
    int old_pos, pos, size;
    bool enter = FALSE;
    int history_pos = -1;

    rl_command_t command;
    wchar_t cmd_buf[8];
    size_t cmd_pos = 0;

    rl_line_buffer = buffer;

    printf("%s", prompt);
    fflush(stdout);

    memset(buffer, 0, BUFFER_SIZE);
    pos = size = 0;
    while (!enter)
    {
        int result;

        command = UNKNOWN_COMMAND;
        old_pos = pos;

        if ((cmd_buf[cmd_pos++] = keyboard_read_char()) == WEOF) {
            // Mehr gibts nicht, auch das WEOF wird nicht mehr gebraucht
            if (!--cmd_pos) {
                break;
            }
            enter = TRUE;
        }
again:
        result = find_command(cmd_buf, cmd_pos, &command);
        if (result > 0) {
            cmd_pos -= result;
            wmemmove(cmd_buf, cmd_buf + result, cmd_pos);
        } else if (result < 0) {
            inschar(buffer, &pos, &size, cmd_buf[0]);
            wmemmove(cmd_buf, cmd_buf + 1, cmd_pos);

            // Neu eingefuegtes Zeichen und allenfalls Folgende auf den
            // Bildschirm bringen
            printf("\033[K\033[s%s\033[u\033[1C", &buffer[old_pos]);
            fflush(stdout);

            // Erstes Zeichen weg(raus aus dem Befehlspuffer und rein in den
            // Datenpuffer) und erneut versuchen
            cmd_pos--;
            if (cmd_pos) {
                goto again;
            }
        }

        switch (command) {
            case BACKWARD_CHAR:
                if (pos > 0) {
                    printf("\033[1D");
                    fflush(stdout);
                    move_cursor(buffer, &pos, size, -1);
                }
                break;

            case FORWARD_CHAR:
                if (pos < size) {
                    printf("\033[1C");
                    fflush(stdout);
                    move_cursor(buffer, &pos, size, 1);
                }
                break;

            case BEGINNING_OF_LINE:
                printf("\033[%dD", count_visible_chars(buffer, 0, pos));
                pos = 0;
                fflush(stdout);
                break;

            case END_OF_LINE:
                printf("\033[%dC",
                    count_visible_chars(buffer, pos, size));
                pos = strlen(buffer);
                fflush(stdout);
                break;


            case PREVIOUS_HISTORY:
            case NEXT_HISTORY:
                if (command == PREVIOUS_HISTORY) {
                    if (history_pos < (int) list_size(history) - 1) {
                        history_pos++;
                    } else {
                        break;
                    }
                } else {
                    if (history_pos > -1) {
                        history_pos--;
                    } else {
                        break;
                    }
                }

                memset(buffer, 0, BUFFER_SIZE);
                if (history_pos > -1) {
                    strncpy(buffer,
                        list_get_element_at(history, history_pos),
                        BUFFER_SIZE - 1);
                }

                char* format;
                asprintf(&format, "\033[%dD", pos);
                printf("%s\033[K%s", format, buffer);
                free(format);
                fflush(stdout);

                pos = size = strlen(buffer);
                break;


            case DELETE_CHAR:
                if (pos < size) {
                    delchar(buffer, pos, &size);
                    printf("\033[K\033[s%s\033[u", &buffer[pos]);
                    fflush(stdout);
                }
                break;

            case BACKWARD_DELETE_CHAR:
                if (pos > 0) {
                    move_cursor(buffer, &pos, size, -1);
                    delchar(buffer, pos, &size);
                    printf("\033[1D\033[K\033[s%s\033[u", &buffer[pos]);
                    fflush(stdout);
                }
                break;


            case ACCEPT_LINE:
                enter = TRUE;

                printf("\r\n");
                fflush(stdout);
                break;

            case COMPLETE:
                if (rl_attempted_completion_function != NULL) {
                    int old_pos = pos;

                    if (!complete_word(buffer, &pos, &size)) {
                        // Wenn die Match-Liste nicht angezeigt wurde, muss die
                        // Zeile aktualisiert werden
                        printf("\033[s\033[%dD%s\033[u", old_pos, buffer);
                        if (old_pos != pos) {
                            printf("\033[%dC", pos - old_pos);
                        }
                    } else {
                        printf("%s%s\033[%dD", prompt, buffer, size - pos);
                    }

                    fflush(stdout);
                }
                break;

            case UNKNOWN_COMMAND:
                break;
        }
    }

    return realloc(buffer, strlen(buffer) + 1);
}

/**
 * Fuegt eine Zeile der History hinzu 
 */
void add_history(char* line)
{
    char* copy;
    
    if (history == NULL) {
        history = list_create();
    }

    copy = malloc(strlen(line) + 1);
    if (copy) {
        strcpy(copy, line);
        list_push(history, copy);
    }
}
