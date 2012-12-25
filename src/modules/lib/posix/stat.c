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

#include <sys/stat.h>
#include <stdio.h>
#include <lostio.h>
#include <string.h>
#include <dir.h>
#include <errno.h>

/**
 * Inodenummer niemals doppelt vergeben
 * FIXME: Das ist natuerlich schwachsinn, aber gcc prueft so, ob zwei Dateien
 * identisch sind.
 */
static ino_t inode_num = 0;

/**
 * Modus einer Datei Aendern. Der Modus entscheidet unter anderem ueber
 * Zugriffsberechtigungen.
 *
 * @param filename Dateiname
 * @param mode Modus
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int chmod(const char* filename, mode_t mode)
{
    // TODO
    return 0;
}

/**
 * Modus einer geoeffneten Datei aendern. Der Modus entscheidet unter Anderem
 * ueber Zugriffsberechtigungen.
 *
 * @param file Dateihandle
 * @param mode Modus
 * 
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int fchmod(int file, mode_t mode)
{
    // TODO
    return 0;
}

/**
 * Informationen zu einer Datei anhand des Handles auslesen
 *
 * @param f Handle
 * @param stat_buf Pointer auf die stat-Struktur in der die Informationen
 *        abgespeichert werden sollen.
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
static int lost_stat(FILE* f, struct stat* stat_buf)
{
    long pos;

    // Zuerst die ganze Struktur auf 0 Setzen
    memset(stat_buf, 0, sizeof(struct stat));

    stat_buf->st_mode = 0777;
    stat_buf->st_mode |= S_IFREG;
    
    pos = ftell(f);

    // TODO im Moment wird nur die Groesse eingetragen
    fseek(f, 0, SEEK_END);
    stat_buf->st_size = ftell(f);
    
    fseek(f, pos, SEEK_SET);

    stat_buf->st_blksize = 512;
    stat_buf->st_blocks = stat_buf->st_size / stat_buf->st_blksize;
    if (stat_buf->st_size % stat_buf->st_blksize != 0) {
        stat_buf->st_blocks++;
    }
    stat_buf->st_uid = 0;
    stat_buf->st_gid = 0;
    stat_buf->st_ino = ++inode_num;

    return 0;
}

/**
 * Informationen zu einer Datei auslesen
 *
 * @param filename Dateinamen
 * @param stat_buf Pointer auf die stat-Struktur in der die Informationen
 *                 abgespeichert werden sollen.
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int stat(const char* filename, struct stat* stat_buf)
{
    int status = 0;
    FILE* file = fopen(filename, "r");
    
    // Wenn die Datei nicht geoeffnet werden kann, koennte es sich um ein
    // Verzeichnis handeln, falls nicht wird sofort abgebrochen
    if (file == NULL) {
        struct dir_handle* dir = directory_open(filename);
        if (dir == NULL) {
            errno = ENOENT;
            return -1;
        }
        directory_close(dir);
        
        // Zuerst die ganze Struktur auf 0 Setzen
        memset(stat_buf, 0, sizeof(struct stat));

        stat_buf->st_mode = 0777;
        stat_buf->st_mode |= S_IFDIR;
        stat_buf->st_uid = 0;
        stat_buf->st_gid = 0;
        stat_buf->st_ino = ++inode_num;
    } else {
        status = lost_stat(file, stat_buf);
        fclose(file);
    }

    return status;
}

/**
 * Informationen zu einer Verknuepfung(nicht zu der Datei auf die sie zeigt)
 *
 * @param filename Dateinamen
 * @param stat_buf Pointer auf die stat-Struktur in der die Informationen
 *                 abgespeichert werden sollen.
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int lstat(const char* filename, struct stat* stat_buf)
{
    int result = stat(filename, stat_buf);
    io_resource_t* f;

    if (!result &&
        (f = lio_compat_open(filename, IO_OPEN_MODE_READ | IO_OPEN_MODE_LINK)))
    {
        char buf[256];
        stat_buf->st_mode = ~(stat_buf->st_mode & S_IFMT) | S_IFLNK;
        stat_buf->st_size = lio_compat_read(buf, 1, sizeof(buf), f);
        stat_buf->st_blocks = 1;
        lio_compat_close(f);
    }

    return result;
}

/**
 * Informationen zu einer geoeffneten Datei auslesen
 *
 * @param file Dateihandle
 * @param stat_buf Pointer auf die stat-Struktur in der die Informationen
 *                 abgespeichert werden sollen.
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int fstat(int file, struct stat* stat_buf)
{
    FILE* f = fdopen(file, "r");
    if (f == NULL) {
        errno = EBADF;
        return -1;
    }

    return lost_stat(f, stat_buf);
}

/**
 * Verzeichnis erstellen
 *
 * @param path Pfad
 * @param mode Modus der dem Verzeichnis zugewiesen wird (=> chmod)
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int mkdir(const char* path, mode_t mode)
{
    if (directory_create(path) == false) {
        return -1;
    }
    
    // TODO: chmod
    return 0;
}

/**
 * FIFO-Datei erstellen
 *
 * @param filename Dateiname
 * @param mode Modus
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int mkfifo(const char* filename, mode_t mode)
{
    // TODO
    return -1;
}

/**
 * Geraetedatei erstellen
 *
 * @param filename Dateiname
 * @param mode Modus
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int mknod(const char* filename, mode_t mode, dev_t device)
{
    // TODO
    return -1;
}

/**
 * Setzt die Dateierstellungsmaske
 *
 * @param mode Neue Maske
 * 
 * @return Alter wert der Maske
 */
mode_t umask(mode_t mask)
{
    // TODO;
    return 666;
}

