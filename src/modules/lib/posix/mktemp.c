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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/**
 * Temporaeren Dateinamen erstellen
 *
 * @param template Vorlage fuer den Dateinamen. Sie wird von der Funktion
 *                  abgeandert. Am schluss muessen mindestens 6 X stehen.
 * 
 * @return template bei Erfolg, NULL im Fehlerfall
 */
char* mktemp(char* template)
{
    // FIXME: eigentlich nur workaround
    static int fileid = 0;
    size_t len = strlen(template);
    int i;

    // Wenn der String zu kurz ist, ist eh fertig.
    if (len < 6) {
        errno = EINVAL;
        return NULL;
    }

    // Jetzt wird ueberprueft, ob die letzten 6 Zeichen X sind
    for (i = len - 6; i < len; i++) {
        if (template[i] != 'X') {
            errno = EINVAL;
            return NULL;
        }
    }
    
    // Hier wird aus pid und parent pid ein 3 stelliger string erstellt und
    // fuer die ersten 3 Zeichen eingesetzt
    int num = getpid() * getppid() % (35 * 35 * 35);
    for (i = 0; i < 3; i++) {
        template[len - 6 + i] = "1qay2wsx3edc4rfv5tgb6zhn7ujm8ik9ol0p"[num %
            35];
        num /= 35;
    }
    
    num = fileid;
    for (i = 0; i < 3; i++) {
        template[len - 3 + i] = "1qay2wsx3edc4rfv5tgb6zhn7ujm8ik9ol0p"[num %
            35];
        num /= 35;
    }
    fileid++;

    return template;
}

/**
 * Temporaere Datei erstellen. Dabei wird so lange gesucht, bis eine gefunden
 * wird, die noch nicht existiert.
 *
 * @param template Vorlage fuer den Dateinamen. Sie wird von der Funktion
 *                 abgeandert. Am schluss muessen mindestens 6 X stehen.
 *
 * @return Dateideskriptor
 */
int mkstemp(char* template)
{
    int fd = -1;
    size_t len = strlen(template);
    char buf[len + 1];

    do {
        strcpy(buf, template);
        if (!mktemp(buf)) {
            return fd;
        }

        fd = open(buf, O_EXCL | O_CREAT | O_RDWR);
    } while (fd == -1 && (errno == EEXIST || errno == EINTR));

    strcpy(template, buf);
    return fd;
}

