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
#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_
#include <sys/types.h>
#include <time.h>

// Modus Format: 3 Bits fuer Dateityp

/// Zurgriffsberechtigungen
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#define S_IRWXU 0700
#define S_IRWXG 0070
#define S_IRWXO 0007

/* FIXME Sowas haben wir momentan eh nicht */
#define S_ISUID (1 << 5)
#define S_ISGID (1 << 5)
#define S_ISVTX (1 << 5)

/// Modus: Maske fuer Dateityp
#define S_IFMT (0x7 << 12)

/// Modus: Blockdatei
#define S_IFBLK (0x0 << 12)

/// Modus: Character Datei
#define S_IFCHR (0x1 << 12)

/// Modus: Regulaere Datei
#define S_IFREG (0x2 << 12)

/// Modus: FIFO
#define S_IFIFO (0x3 << 12)

/// Modus: Verzeichnis
#define S_IFDIR (0x4 << 12)

/// Modus: Symlink
#define S_IFLNK (0x5 << 12)

/// Modus: Socket
#define S_IFSOCK (0x6 << 12)


/// Ueberprueft ob es sich bei einem st_mode-Feld um eine Regulaere Datei
/// handelt
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

/// Ueberprueft ob es sich bei einem st_mode-Feld um ein Verzeichnis handelt
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

/// Ueberprueft ob es sich bei einem st_mode-Feld um einen Symlink handelt
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)

/// Spezialdateien aus UNIX
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)

/// Information zu einer Datei
struct stat {
    dev_t     st_dev;       /// Geraet auf dem die Datei liegt
    ino_t     st_ino;       /// Serielle Dateinummer
    mode_t    st_mode;      /// Modus (u.A Zugriffsberechtigungen)
    nlink_t   st_nlink;     /// Anzahl der Verknuepfungen auf diese Datei
    uid_t     st_uid;       /// Benutzernummer
    gid_t     st_gid;       /// Gruppennummer
    dev_t     st_rdev;      /// Geraetenummer bei speziellen Dateien wie
                            /// Geraetedateien.
    off_t     st_size;      /// Dateigroesse in Bytes (nur bei normalen
                            /// Dateien)
    time_t    st_atime;     /// Letzter Zugriff
    time_t    st_mtime;     /// Letzte Aenderung
    time_t    st_ctime;     /// Letzter Statuswechsel
    blksize_t st_blksize;   /// Ideale Blockgroesse
    blkcnt_t  st_blocks;    /// Anzahl der Blocks
};


#ifdef __cplusplus
extern "C" {
#endif

/// Modus einer Datei aendern
int chmod(const char* filename, mode_t mode);

/// Modus einer geoeffneten Datei aendern
int fchmod(int file, mode_t mode);

/// Informationen zu einer Datei auslesen
int stat(const char* filename, struct stat* stat_buf);

/// Informationen zu einer Verknuepfung auslesen
int lstat(const char* filename, struct stat* stat_buf);

/// Informationen zu einer geoeffneten Datei auslesen
int fstat(int file, struct stat* stat_buf);

/// Verzeichnis erstellen
int mkdir(const char* path, mode_t mode);

/// FIFO erstellen
int mkfifo(const char* filename, mode_t mode);

/// Geraetedatei erstellen
int mknod(const char* filename, mode_t mode, dev_t device);

/// Modus fuer neue Dateien festlegen
mode_t umask(mode_t mode);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //ifndef _SYS_STAT_H_
