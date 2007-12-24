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

/**
 * Einzelnes Zeichen von der Tastatur einlesen
 */
static char keyboard_read_char(void)
{
    char c = 0;
    while (!fread(&c, 1, 1, stdin));
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
 * Prompt anzeigen und Zeile von der Tastatur einlesen
 */
char* readline(const char* prompt)
{
    char* buffer = malloc(BUFFER_SIZE);
    int pos, size;
    bool enter = FALSE;

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
