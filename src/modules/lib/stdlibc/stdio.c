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
#include <io.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

FILE* stdin = NULL;
FILE* stdout = NULL;
FILE* stderr = NULL;

/// Puffer fuer die Ausgabe
static char stdout_buf[512];

/**
 * Initialisiert die Ein- und Ausgabe Filehandles
 */
void stdio_init(void)
{
    FILE* path_file;
    char path[129];
    size_t size;
    

    // stdin aktualisieren
    if (!stdin) {
        path_file = fopen("console:/stdin",  "r");
        if (path_file == NULL) {
            // Kein stdin
            stdin = NULL;
        } else {
            // Pfad einlesen
            size = fread(path, 1, 128, path_file);
            path[size] = 0;

            // Falls stdin schon geoeffnet wurde, wird freopen benutzt, damit siche
            // nichts durcheinander kommt
            if (stdin) {
                freopen(path, "rs", stdin);
            } else {
                stdin  = fopen(path,  "rs");
            }
            fclose(path_file);
        }
    }
    if (stdin) {
        // stdin muss nicht gepuffert sein
        setvbuf(stdin, NULL, _IONBF, 0);
    }

    // stdout aktualisieren
    if (!stdout) {
        path_file = fopen("console:/stdout",  "r");
        if (path_file == NULL) {
            // Kein stdin
            stdout = NULL;
        } else {
            // Pfad einlesen
            size = fread(path, 1, 128, path_file);
            path[size] = 0;

            // Siehe oben bei stdin
            if (stdout) {
                freopen(path, "a", stdout);
            } else {
                stdout = fopen(path,  "a");
            }
            fclose(path_file);
        }
    }
    if (stdout) {
        // Zeilenpuffer fuer stdout aktivieren
        setvbuf(stdout, stdout_buf, _IOLBF, sizeof(stdout_buf));
    }

    // stderr
    if (!stderr) {
        path_file = fopen("console:/stderr",  "r");
        if (path_file == NULL) {
            // Kein stdin
            stderr = NULL;
        } else {
            // Pfad einlesen
            size = fread(path, 1, 128, path_file);
            path[size] = 0;

            // Siehe oben bei stdin
            if (stderr) {
                freopen(path, "a", stderr);
            } else {
                stderr = fopen(path,  "a");
            }

            fclose(path_file);
        }
    }
    if (stderr) {
        // stderr soll ungepuffert sein
        setvbuf(stderr, NULL, _IONBF, 0);
    }
}

/**
 * Gibt alle Zeichen innerhalb des angegebenen Bereiches aus. Nullbytes werden
 * ignoriert, ein Zeilenumbruch wird danach _NICHT_ ausgegeben.
 *
 * @param n Anzahl der Zeichen
 * @param str Pointer auf die Zeichenkette
 *
 * @return Anzahl der ausgegebenen Zeichen
 */
int putsn(unsigned int n, const char* str)
{
    if (stdout == NULL) {
        syscall_putsn(n, str);
        return n;
    } else {
        if (fwrite(str, n, 1, stdout) == EOF) {
            return EOF;
        } else {
            return n;
        }
    }
}


/**
 * Eine Zeichenkette, und danach einen Zeilenumbruch ausgeben. Es werden alle
 * Zeichen ausgegeben, bis ein Nullbyte gefunden wird.
 *
 * @param str Pointer auf den String
 *
 * @return Anzahl der ausgegebenen Zeichen
 */
int puts(const char* str)
{
    if (putsn(strlen(str), str) == EOF) {
        return EOF;
    } else {
        if (stdout == NULL) {
            syscall_putsn(1, "\n");
        } else {
            putchar('\n');
        }
    }
    return 1;
}


/**
 * Ein einzelnes Zeichen in eine Datei schreiben.
 * 
 * @param c Zeichen
 * @param io_res Dateihandle
 *
 * @return Das ausgegebene Zeichen oder EOF wenn ein Fehler aufgetreten ist.
 */
int putc(int c, FILE *io_res)
{
    if(stdout == NULL)
    {
        char cs = c;
        syscall_putsn(1, &cs);
    }
    else
    {
        return fputc(c, io_res);
    }
    return c;
}


/**
 * Ein einzelnes Zeichen ausgeben.
 *
 * @param c Zeichen
 *
 * @return Das ausgebene Zeichen oder EOF wenn ein Fehler aufgetreten ist.
 */
int putchar(int c)
{
    if(stdout == NULL)
    {
        char cs = c;
        syscall_putsn(1, &cs);
    }
    else
    {
        return fputc(c, stdout);
    }
    return c;
}


/**
 * Ein einzelnes Zeichen aus einer Datei lesen.
 *
 * @param io_res Dateihandle
 *
 * @return Zeichen
 */
int getc(FILE* io_res)
{
    return fgetc(io_res);
}


/**
 * Ein einzelnes Zeichen von der Standardeingabe einlesen.
 *
 * @return Das Zeichen, oder EOF wenn ein Fehler aufgetreten ist.
 */
int getchar()
{
    return fgetc(stdin);
}


/**
 * Eine Zeile von der Standardeingabe einlesen. Diese funktion fuehr sehr
 * schnell zu Buffer-Overflow.
 *
 * @param dest Pointer auf den Bereich, wo die Daten hingeschrieben werden
 *              sollen.
 * 
 * @return Den uebergebenen Pointer, oder NULL wen ein Fehler aufgetreten ist.
 */
char* gets(char* dest)
{
    int pos = 0;
    int c;
    while (!ferror(stdin)) {
        //Zeichen einlesen
        c = getchar();
        if(c == EOF) {
            //Wenn ein Fehler aufgetreten ist, wird abgebrochen
            if (ferror(stdin) != 0) {
                return NULL;
            }

            //Wenn die Datei zu Ende ist, wird aufgehoert Zeichen einzulesen.
            if (feof(stdin) != 0) {
                break;
            }
            //Wenn kein Fehler aufgetreten ist, wird 
            continue;
        }

        putchar(c);
        
        if (c == '\n') {
            break;
        }
        
        dest[pos++] = (char) c;
    }

    dest[pos] = (char) 0;
    return dest;
}

/**
 * Fehlermeldung zur aktuellen Errno ausgeben.
 *
 * @param message Dieser String wird der Fehlermeldung als Prefix hinzugefuegt
 */
void perror(const char* message)
{
    // TODO: Eigentliche Funktionalitaet ;-)
    printf("%s: errno=%d\n", message, errno);
}

