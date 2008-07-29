/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#include <types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <collections.h>

#include <readline/readline.h>

#define BUFFER_SIZE 255


#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_UP 72
#define KEY_DOWN 80

#define KEY_HOME 71
#define KEY_END 79
#define KEY_INS 82
#define KEY_DEL 83

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
 * @return Das eingelesene Zeichen oder NULL, wenn das Ende der Eingabedatei
 * erreicht ist.
 */
static char keyboard_read_char(void)
{
    char c = 0;
    
    while (!fread(&c, 1, 1, stdin)) {
        // FIXME Das gehoert eigentlich rein, geht aber nicht, weil vterm ein EOF
        // liefert, wenn der Eingabepuffer gerade leer ist.
        /*
        if (feof(stdin)) {
            return 0;
        }
        */
    }

    return c;
}

/**
 * Loescht ein Zeichen aus dem Puffer
 */
static void delchar(char* buffer, int pos)
{
    int i;
    size_t len = strlen(buffer);

    if (len < pos) {
        return;
    }

    for (i = pos; i < BUFFER_SIZE - 1; i++) {
        buffer[i] = buffer[i + 1];
        if (buffer[i] == '\0') {
            break;
        }
    }

    buffer[BUFFER_SIZE - 1] = '\0';
}

/**
 * Fuegt ein Zeichen in den Buffer ein
 */
static void inschar(char* buffer, int pos, char c)
{
    int i;
    size_t len = strlen(buffer);

    if ((len < pos) || (len + 1 >= BUFFER_SIZE)) {
        return;
    }

    for (i = len + 1; i > pos; i--) {
        buffer[i] = buffer[i - 1];
    }

    buffer[pos] = c;
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

    //printf("match_count: %d\n\n", match_count);
    // TODO:
    if (match_count > 1) {
        int i;
        char* match;

        printf("\n");
        for (i = 0; (match = matches[i]); i++) {
            printf("%s ", match);
        }
        printf("\n");

        free_matches_array(matches);
        return TRUE;
    } else {
        replacement = matches[0];
        replacement_len = strlen(replacement);
    }

    // Pruefen ob das ganze nachher noch in den Puffer passt
    if (*len + replacement_len > BUFFER_SIZE) {
        return FALSE;
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

    return FALSE;
}

/**
 * Prompt anzeigen und Zeile von der Tastatur einlesen
 */
char* readline(const char* prompt)
{
    // FIXME Das gehoert eigentlich rein, geht aber nicht, weil vterm ein EOF
    // liefert, wenn der Eingabepuffer gerade leer ist.
    /*
    if (feof(stdin)) {
        return NULL;
    }
    */

    char* buffer = malloc(BUFFER_SIZE);
    int pos, size;
    bool enter = FALSE;
    int history_pos = -1;

    rl_line_buffer = buffer;

    printf("%s", prompt);
    fflush(stdout);

    memset(buffer, 0, BUFFER_SIZE);
    pos = size = 0;
    while (!enter)
    {
        char c = keyboard_read_char();
       
        switch (c) {
            case '\0':
                c = keyboard_read_char();
                switch (c) {
                    // Ende der Eingabedatei
                    case '\0':
                        enter = TRUE;  
                        
                        printf("\r\n");
                        fflush(stdout);
                        break;

                    // Links
                    case KEY_LEFT:
                        if (pos > 0) {
                            printf("\033[1D");
                            fflush(stdout);
                            pos--;
                        }
                        break;

                    // Rechts
                    case KEY_RIGHT:
                        if (pos < size) {
                            printf("\033[1C");
                            fflush(stdout);
                            pos++;
                        }
                        break;

                    // Hoch/Runter
                    case KEY_UP:
                    case KEY_DOWN:
                        if (c == KEY_UP) {
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
            
                    // Entfernen
                    case KEY_DEL:
                        if (pos < size) {
                            delchar(buffer, pos);
                            size--;
                            printf("\033[K\033[s%s\033[u", &buffer[pos]);
                            fflush(stdout);
                        }
                        break;
                    
                    // Pos1
                    case KEY_HOME:
                    {
                        char* format;
                        asprintf(&format, "\033[%dD", pos);
                        printf("%s", format);
                        free(format);


                        pos = 0;
                        fflush(stdout);
                        break;
                    }
                    
                    // Ende
                    case KEY_END:
                    {
                        char* format;
                        asprintf(&format, "\033[%dC", strlen(buffer) - pos);
                        printf("%s", format);
                        free(format);

                        pos = strlen(buffer);
                        delchar(buffer, pos);
                        printf("\033[K\033[s%s\033[u", &buffer[pos]);
                        fflush(stdout);
                        break;
                    }
                }
                break;

            case '\b':
                if (pos > 0) {
                    pos--;
                    size--;
                    delchar(buffer, pos);
                    printf("\033[1D\033[K\033[s%s\033[u", &buffer[pos]);
                    fflush(stdout);
                }
                break;

            case '\n':
                enter = TRUE;  
                
                printf("\r\n");
                fflush(stdout);
                break;
            

            case '\t':
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

            default:
                if (pos < BUFFER_SIZE - 1) {
                    inschar(buffer, pos, c);
                    printf("\033[K\033[s%s\033[u\033[1C", &buffer[pos]);
                    pos++;
                    size++;
                    fflush(stdout);
                }
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
