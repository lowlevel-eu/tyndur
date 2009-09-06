/*  
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
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

#include "types.h"
#include "dirent.h"
#include "stdlib.h"
#include "dir.h"
#include <string.h>

/**
 * Öffnet ein Verzeichnis zum Auslesen der Verzeichniseinträge
 *
 * @param name Name des zu öffnenden Verzeichnisses
 * @return Verzeichnis oder NULL im Fehlerfall
 */
DIR* opendir(const char * name)
{
    return directory_open(name);
}

/**
 * Schließt ein geöffnetes Verzeichnis
 *
 * @param dir Zu schließendes Verzeichnis
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int closedir(DIR * dir)
{
    return (directory_close(dir) == 0) ? 0 : -1;
}

/**
 * Liest einen Verzeichniseintrag
 *
 * @param dir Verzeichnis, aus dem der nächste Verzeichniseintrag gelesen
 * werden soll
 *
 * @return Nächster Verzeichniseintrag, NULL wenn keine weiteren Einträge
 * vorhanden sind.
 */
struct dirent* readdir(DIR * dir)
{
    io_direntry_t* direntry = directory_read(dir);
    
    if (direntry == NULL) {
        return NULL;
    }

    size_t name_length = strlen(direntry->name);
    
    if (name_length > NAME_MAX) {
        name_length = NAME_MAX;
    }

    struct dirent* posix_direntry = malloc(sizeof(struct dirent));
    posix_direntry->d_reclen = sizeof(struct dirent);
    memcpy(posix_direntry->d_name, direntry->name, name_length + 1);
    posix_direntry->d_name[NAME_MAX] = '\0';

    free(direntry);
    return posix_direntry;
}

/**
 * TODO
 */
long telldir(DIR * dir)
{
    printf("telldir: Nicht implementiert\n");
    return 0;
}

/**
 * Setzt das Verzeichnis zurück, so dass der nächste per readdir gelesene
 * Eintrag der erste Verzeichniseintrag ist.
 *
 * @param dir Zurückzusetzendes Verzeichnis
 */
void rewinddir(DIR * dir)
{
    directory_seek(dir, 0, SEEK_SET);
}

/**
 * Setzt die aktuelle Position im Verzeichnis und bestimmt so den nächstem
 * per readdir abrufbaren Verzeichniseintrag.
 *
 * @param dir Verzeichnis, dessen Position gesetzt werden soll
 */
void seekdir(DIR * dir, long offset)
{
    directory_seek(dir, offset, SEEK_SET);
}
