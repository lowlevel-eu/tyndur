/*  
 * Copyright (c) 2006-2007 The LOST Project. All rights reserved.
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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <collections.h>
#include <errno.h>
#include <config.h>

/// Ein Element das einen Unix-Dateideskriptor mit einem Normalen verknuepft
struct fd_list_element {
    FILE* io_res;
    int fd;
};

/// Liste mit den Dateideskriptoren
static list_t* fd_list = NULL;

/// Naechste Deskriptornummer
static int next_fd = 1;

/**
 * Unix-Dateideskriptor holen fuer eine Datei
 *
 * @param io_res Dateihandle
 *
 * @return Unix-Dateideskriptor oder -1 im Fehlerfall
 */
int fileno(FILE* io_res)
{
    if (io_res == NULL) {
        errno = EBADF;
        return -1;
    }
    
    // Wenn die Liste noch nicht initialisiert wurde, wird das jetzt erledigt
    if (fd_list == NULL) {
        fd_list = list_create();
        if (fd_list == NULL) {
            puts("libc fileno(): Fehler beim erstellen der"
                "Dateideskriptorliste");
            return -1;
        }
    }
    
    // Liste mit den Dateideskriptoren durchsuchen
    struct fd_list_element* fdl_element = NULL;
    int i = 0;
    while ((fdl_element = list_get_element_at(fd_list, i))) {
        if (fdl_element->io_res == io_res) {
            break;
        }
        i++;
    }
    
    // Wenn kein Deskriptor gefunden wurde, wird einer angelegt
    if (fdl_element == NULL) {
        fdl_element = malloc(sizeof(struct fd_list_element));
        if (fdl_element == NULL) {
            return -1;
        }
        fd_list = list_push(fd_list, fdl_element);

        fdl_element->io_res = io_res;
        fdl_element->fd = next_fd++;
    }

    return fdl_element->fd;
}

/**
 * Dateihandle andhand des Unix-Deskriptors suchen
 *
 * @param fd Unix-Dateideskriptor
 *
 * @return Pointer auf das Dateihandle oder NULL im Fehlerfall
 */
static FILE* fd_to_file(int fd)
{
    // Liste mit den Dateideskriptoren durchsuchen
    struct fd_list_element* fdl_element = NULL;
    int i = 0;
    while ((fdl_element = list_get_element_at(fd_list, i))) {
        if (fdl_element->fd == fd) {
            break;
        }
        i++;
    }
    
    if (fdl_element == NULL) {
        return NULL;
    } else {
        return fdl_element->io_res;
    }
}

/**
 * Einen Dateideskriptor als Stream oeffnen.
 */
FILE* fdopen(int fd, const char* mode)
{
    // TODO: Modus pruefen
    return fd_to_file(fd);
}

/**
 * Eine Datei als Unix-Dateideskriptor oeffnen
 *
 * @param filename Dateiname
 * @param flags Flags die das Verhalten des handles Beeinflussen
 * @param mode Modus der benutzt werden soll, falls die Datei neu erstellt wird
 *
 * @return Dateideskriptor bei Erfolg, -1 im Fehlerfall
 */
int open(const char* filename, int flags, ...)
{
    char* fopen_flags = "   ";
    size_t flags_size = 0;

    // Wenn O_CREAT und O_EXCL gleichzeitig gesetzt ist, muessen wir abbrechen
    // wenn die Datei existiert.
    if ((flags & O_CREAT) && (flags & O_EXCL)) {
        FILE* f = fopen(filename, "r");
        if (f != NULL) {
            fclose(f);
            errno = EEXIST;
            return -1;
        }
    } else if ((flags & O_CREAT) == 0) {
        // Wenn die Datei nicht existiert und nicht angelegt werden soll, ist
        // der Fall auch klar.
        FILE* f = fopen(filename, "r");
        if (f == NULL) {
            errno = ENOENT;
            return -1;
        }
        fclose(f);
    }
    
    if (flags & O_CREAT) {
        // Sicherstellen, dass die Datei existiert
        FILE* f = fopen(filename, "r");
        if (f == NULL) {
            f = fopen(filename, "w");
        }
        fclose(f);
    }

    // Open-Flags auf fopen-Kompatible uebersetzen
    if ((flags & O_RDONLY)) {
        fopen_flags[flags_size++] = 'r';
    } else if ((flags & O_WRONLY) && (flags & O_TRUNC)) {
        fopen_flags[flags_size++] = 'w';
    } else if ((flags & O_WRONLY)) {
        fopen_flags[flags_size++] = 'r';
        fopen_flags[flags_size++] = '+';
    } else if ((flags & O_RDWR)) {
        fopen_flags[flags_size++] = 'r';
        fopen_flags[flags_size++] = '+';
    }
    // Append-Flag
    if ((flags & O_APPEND)) {
        fopen_flags[flags_size++] = 'a';
    }
    fopen_flags[flags_size++] = '\0';

    FILE* file = fopen(filename, fopen_flags);
    if (file == NULL) {
        errno = ENOENT;
        return -1;
    }
    return fileno(file);
}

/**
 * Eine Datei erstellen und als Unix-Dateideskriptor oeffnen
 *
 * @param filename Dateiname
 * @param mode Modus der benutzt werden soll, falls die Datei neu erstellt wird
 *
 * @return Dateideskriptor bei Erfolg, -1 im Fehlerfall
 */
int creat(const char* filename, mode_t mode)
{
    return open(filename, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

/**
 * Aus einem Unix-Dateideskriptor lesen.
 *
 * @param fd Unix-Dateideskriptor
 * @param buffer Puffer in den die aten glesen werden sollen
 * @param size Anzahl der Bytes die gelesen werden sollen
 *
 * @return Anzahl der gelesenen Bytes bei Erfolg, -1 im Fehlerfall
 */
ssize_t read(int fd, void* buffer, size_t size)
{
    FILE* file = fd_to_file(fd);

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

    size_t bytes = fread(buffer, 1, size, file);
    // FIXME: Fehlerbehandlung
    return bytes;
}

/**
 * Auf einen Unix-Dateideskriptor schreiben
 *
 * @param fd Unix-Dateideskriptor
 * @param buffer Puffer aus dem die Daten gelesen werden sollen
 * @param size Anzahl der Bytes die geschrieben werden sollen
 *
 * @return Anzahl der geschriebenen Bytes bei Erfolg, -1 im Fehlerfall
 */
ssize_t write(int fd, const void* buffer, size_t size)
{
    FILE* file = fd_to_file(fd);

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

    size_t bytes = fwrite(buffer, 1, size, file);
    // FIXME: Fehlerbehandlung
    return bytes;
}

/**
 * Position in einer durch Unix-Dateideskriptor gegebenen Datei setzen
 *
 * @param fd Unix-Dateideskriptor
 * @param offset Offset bezogen auf den mit origin festgelegten Ursprung
 * @param origin Ursprung. Moeglichkeiten: 
 *                  - SEEK_SET Bezogen auf Dateianfang
 *                  - SEEK_CUR Bezogen auf die aktuelle Position
 *                  - SEEK_END Bezogen auf das Ende der Datei
 */
off_t lseek(int fd, off_t offset, int origin)
{
    FILE* file = fd_to_file(fd);

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

    // Seek ausfuehren
    if (fseek(file, offset, origin)) {
        return -1;
    }

    // Rueckgabewert ist die neue Position
    return ftell(file);
}

/**
 * Unix-Dateideskriptor schliessen
 *
 * @param fd Unix-Dateideskriptor
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int close(int fd)
{
    // Liste mit den Dateideskriptoren durchsuchen
    struct fd_list_element* fdl_element = NULL;
    int i = 0;
    while ((fdl_element = list_get_element_at(fd_list, i))) {
        if (fdl_element->fd == fd) {
            list_remove(fd_list, i);
            break;
        }
        i++;
    }

    // Wenn der Deskriptor ungueltig ist, wird abgebrochen
    if ((fdl_element == NULL) || (fclose(fdl_element->io_res) != 0)) {
        errno = EBADF;
        return -1;
    }
    free(fdl_element);
    return 0;
}


#ifndef CONFIG_LIBC_NO_STUBS
/**
 * Dateideskriptor duplizieren
 */
int dup(int fd)
{
    errno = EMFILE;
    return -1;
}

/**
 * Dateideskriptor duplizieren
 */
int dup2(int fd, int newfd)
{
    errno = EMFILE;
    return -1;
}

#endif // ndef CONFIG_LIBC_NO_STUBS

