/*  
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <collections.h>
#include <errno.h>
#include <lost/config.h>
#include <stdarg.h>
#include <lostio.h>
#include <sys/select.h>

/// Ein Element das einen Unix-Dateideskriptor mit einem Normalen verknuepft
struct fd_list_element {
    io_resource_t* io_res;
    int fd;

    int fd_flags;
    int file_flags;
};

/// Liste mit den Dateideskriptoren
static list_t* fd_list = NULL;

/// Naechste Deskriptornummer
static int next_fd = 0;


/**
 * Posix-Dateiemulation initialisieren
 */
static void posix_files_init(void)
{
    if (fd_list) {
        return;
    }

    fd_list = list_create();

    // Deskriptoren fuer stdio
    fileno(stdin);
    fileno(stdout);
    fileno(stderr);
}


/**
 * Unix-Dateideskriptor holen fuer eine Datei
 *
 * @param io_res Dateihandle
 *
 * @return Unix-Dateideskriptor oder -1 im Fehlerfall
 */
int __iores_fileno(io_resource_t* io_res)
{
    if (io_res == NULL) {
        errno = EBADF;
        return -1;
    }
    
    // Wenn die Liste noch nicht initialisiert wurde, wird das jetzt erledigt
    if (fd_list == NULL) {
        posix_files_init();
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
        fdl_element->fd_flags = 0;
        fdl_element->file_flags = 0;
    }

    return fdl_element->fd;
}

int fileno(FILE* io_res)
{
    if (io_res == NULL) {
        errno = EBADF;
        return -1;
    }

    return __iores_fileno(io_res->res);
}

/**
 * Dateihandle andhand des Unix-Deskriptors suchen
 *
 * @param fd Unix-Dateideskriptor
 *
 * @return Pointer auf das Dateihandle oder NULL im Fehlerfall
 */
static io_resource_t* fd_to_file(int fd)
{
    if (fd_list == NULL) {
        posix_files_init();
    }

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

extern FILE* __create_file_from_iores(io_resource_t* io_res);

/**
 * Einen Dateideskriptor als Stream oeffnen.
 */
FILE* fdopen(int fd, const char* mode)
{
    io_resource_t* iores;

    // TODO: Modus pruefen
    iores = fd_to_file(fd);
    if (iores == NULL) {
        return NULL;
    }
    return __create_file_from_iores(iores);
}

/**
 * Fuehrt unterschiedliche Aktionen (F_*) auf Dateien aus
 */
int fcntl(int fd, int cmd, ...)
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
        errno = EBADF;
        return -1;
    }

    // Aktion durchfuehren
    switch (cmd) {
        case F_GETFD:
            return fdl_element->fd_flags;

        case F_GETFL:
            return fdl_element->file_flags;

        case F_SETFD:
        {
            va_list ap;
            int arg;

            va_start(ap, cmd);
            arg = va_arg(ap, int);
            va_end(ap);

            if (arg != fdl_element->fd_flags) {
                fprintf(stderr, "Warnung: S_SETFD ändert Flags (%#x => %#x) "
                    "ohne Effekt\n", fdl_element->fd_flags, arg);
            }

            fdl_element->fd_flags = arg;
            return 0;
        }

        case F_SETFL:
        {
            va_list ap;
            int arg;

            va_start(ap, cmd);
            arg = va_arg(ap, int);
            va_end(ap);

            // Wenn die Flags bisher 0 sind, ist es open, das aendert. In
            // diesem Fall wollen wir keine Warnung.
            if ((arg != fdl_element->file_flags) && fdl_element->file_flags) {
                fprintf(stderr, "Warnung: S_SETFL ändert Flags (%#x => %#x) "
                    "ohne Effekt\n", fdl_element->file_flags, arg);
            }

            fdl_element->file_flags = arg;
            return 0;
        }
    }

    errno = EINVAL;
    return -1;
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
    int lio_flags;
    int fd;

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

    // Open-Flags auf LIO-Kompatible uebersetzen
    lio_flags = 0;
    if ((flags & O_RDONLY)) {
        lio_flags |= IO_OPEN_MODE_READ;
    } else if ((flags & O_WRONLY) && (flags & O_TRUNC)) {
        lio_flags |= IO_OPEN_MODE_WRITE | IO_OPEN_MODE_TRUNC;
    } else if ((flags & O_RDWR) && (flags & O_TRUNC)) {
        lio_flags |= IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE |
                     IO_OPEN_MODE_TRUNC;
    } else if ((flags & O_WRONLY)) {
        /* FIXME IO_OPEN_MODE_READ gehört hier eigentlich nicht rein */
        lio_flags |= IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE;
    } else if ((flags & O_RDWR)) {
        lio_flags |= IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE;
    }
    // Append-Flag
    if ((flags & O_APPEND)) {
        lio_flags |= IO_OPEN_MODE_APPEND;
    }

    io_resource_t* file = lio_compat_open(filename, lio_flags);
    if (file == NULL) {
        errno = ENOENT;
        return -1;
    }

    fd = __iores_fileno(file);
    fcntl(fd, F_SETFL, flags);

    return fd;
}

/**
 * Pipe einrichten
 */
int pipe(int fd[2])
{
    lio_stream_t reader, writer;
    io_resource_t* reader_file;
    io_resource_t* writer_file;
    int ret;

    ret = lio_pipe(&reader, &writer, false);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    /* Datenstruktur anlegen */
    reader_file = malloc(sizeof(*reader_file));
    writer_file = malloc(sizeof(*writer_file));

    if (!reader_file || !writer_file) {
        free(reader_file);
        free(writer_file);
        errno = -ENOMEM;
        return -1;
    }

    *reader_file = (io_resource_t) {
        .lio2_stream    =   reader,
        .lio2_res       =   0x42424242, /* Muss != 0 sein für LIOv2 */
        .flags          =   IO_RES_FLAG_READAHEAD,
    };
    fd[0] = __iores_fileno(reader_file);

    *writer_file = (io_resource_t) {
        .lio2_stream    =   writer,
        .lio2_res       =   0x42424242, /* Muss != 0 sein für LIOv2 */
        .flags          =   IO_RES_FLAG_READAHEAD,
    };
    fd[1] = __iores_fileno(writer_file);

    return 0;
}

/**
 * Dateideskriptor duplizieren
 */
int dup(int fd)
{
    io_resource_t* file = fd_to_file(fd);
    lio_stream_t nfd;
    io_resource_t* nfile;

    /* Für LIOv1 handeln wir das halt in der POSIX-Emulationsschicht ab */
    if (file->lio2_res == 0) {
        struct fd_list_element* fdl_element;

        fdl_element = malloc(sizeof(struct fd_list_element));
        if (fdl_element == NULL) {
            return -1;
        }
        fd_list = list_push(fd_list, fdl_element);

        fdl_element->io_res = file;
        fdl_element->fd = next_fd++;
        fdl_element->fd_flags = 0;
        fdl_element->file_flags = 0;

        return fdl_element->fd;
    }

    /* LIOv2 kann dup() nativ */
    nfd = lio_dup(file->lio2_stream, -1);
    if (nfd < 0) {
        errno = -nfd;
        return -1;
    }

    nfile = malloc(sizeof(*nfile));
    if (nfile == NULL) {
        lio_close(nfd);
        errno = ENOMEM;
        return -1;
    }

    *nfile = (io_resource_t) {
        .lio2_stream    =   nfd,
        .lio2_res       =   file->lio2_res,
        .flags          =   file->flags,
    };
    return __iores_fileno(nfile);
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
    io_resource_t* file = fd_to_file(fd);
    ssize_t ret;

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

retry:
    ret = lio_compat_read(buffer, 1, size, file);
    if (ret == -EAGAIN) {
        yield();
        goto retry;
    }

    if (ret < 0) {
        errno = -ret;
        ret = -1;
    }
    return ret;
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
    io_resource_t* file = fd_to_file(fd);
    ssize_t ret;

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

    ret = lio_compat_write(buffer, 1, size, file);
    if (ret < 0) {
        errno = -ret;
        ret = -1;
    }
    return ret;
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
    io_resource_t* file = fd_to_file(fd);

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

    // Seek ausfuehren
    if (!lio_compat_seek(file, offset, origin)) {
        return -1;
    }

    // Rueckgabewert ist die neue Position
    return lio_compat_tell(file);
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
    if ((fdl_element == NULL) || (lio_compat_close(fdl_element->io_res) != 0)) {
        errno = EBADF;
        return -1;
    }
    free(fdl_element);
    return 0;
}

/// Aus einer Datei mit gegebenem Offset lesen
ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    io_resource_t* file = fd_to_file(fd);
    off_t old_pos;
    ssize_t ret;

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

    old_pos = lio_compat_tell(file);
    if (!lio_compat_seek(file, offset, SEEK_SET)) {
        return -1;
    }

    ret = lio_compat_readahead(buf, count, file);

    if (!lio_compat_seek(file, old_pos, SEEK_SET)) {
        return -1;
    }

    return ret;
}

/// In eine Datei mit gegebenem Offset schreiben
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    io_resource_t* file = fd_to_file(fd);
    off_t old_pos;
    ssize_t ret;

    // Bei einem ungueltigen Deskriptor wird abgebrochen
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }

    old_pos = lio_compat_tell(file);
    if (!lio_compat_seek(file, offset, SEEK_SET)) {
        return -1;
    }

    ret = lio_compat_write(buf, count, 1, file);

    if (!lio_compat_seek(file, old_pos, SEEK_SET)) {
        return -1;
    }

    return ret;
}

/**
 * Prüft, ob es sich beim Dateideskriptor @fd um ein Terminal handelt.
 *
 * Gibt 1 zurück, wenn es ein Terminal ist; andernfalls 0 und errno wird
 * gesetzt.
 */
int isatty(int fd)
{
    io_resource_t* file = fd_to_file(fd);
    int ret;

    if (file == NULL || !IS_LIO2(file)) {
        errno = EBADF;
        return 0;
    }

    ret = lio_ioctl(file->lio2_stream, LIO_IOCTL_ISATTY);
    if (ret < 0) {
        errno = -ret;
        return 0;
    }

    return 1;
}


#ifndef CONFIG_LIBC_NO_STUBS
/**
 * Dateideskriptor duplizieren
 */
int dup2(int fd, int newfd)
{
    io_resource_t* src = fd_to_file(fd);
    io_resource_t* dest = fd_to_file(newfd);
    int ret;

    /* Nur für LIOv2 implementiert */
    if (src == NULL || !IS_LIO2(src)) {
        errno = EBADF;
        return -1;
    }

    /* Und auch nur, wenn wir einen bestehenden Deskriptor überschreiben,
     * sonst geht nämlich die primitive FD-Allokationslogik (statoscjer
     * Zähler) kaputt. */
    if (dest == NULL || !IS_LIO2(dest)) {
        fprintf(stderr, "Nicht implementiertes dup2()\n");
        errno = EMFILE;
        return -1;
    }

    /* Wir ersetzen den Stream auf LIO-Ebene, damit das in allen Interfaces
     * konsistent ankommt (vor allem für stdio). */
    ret = lio_dup(src->lio2_stream, dest->lio2_stream);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

/**
 * Prueft, welche der gegebenen Dateideskriptoren bereit zum Lesen oder
 * Schreiben sind oder ein Fehler fuer sie aufgetreten ist. Dateideskriptoren
 * die nicht bereit bzw. in einem Fehlerzustand sind, werden aus der Menge
 * entfernt.
 *
 * @param number_fds Nummer des hoechsten Dateideskriptors in einem der
 * uebergebenen Mengen.
 * @param readfds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * zum Lesen bereit sind.
 * @param writefds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * zum Schreiben bereit sind.
 * @param errfds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * in einem Fehlerzustand sind.
 * @param timeout Maximales Timeout, das gewartet werden soll, falls kein
 * Deskriptor bereit ist. NULL fuer dauerhaftes Blockieren.
 *
 * @return Anzahl der Dateideskriptoren, die bereit bzw. in einem Fehlerzustand
 * sind
 */
int select(int number_fds, fd_set* readfds, fd_set* writefds,
    fd_set* errfds, struct timeval* timeout)
{
    uint32_t* rd = (uint32_t*) readfds;
    uint32_t* wr = (uint32_t*) writefds;
    uint32_t* err = (uint32_t*) errfds;
    int c;
    io_resource_t* f;
    int ret;
    int fds;

    uint32_t orig_rd = rd ? *rd : 0;
    uint32_t orig_wr = wr ? *wr : 0;

    uint64_t timeout_tick;
    if (timeout != NULL) {
        timeout_tick = get_tick_count() +
            1000ULL * 1000ULL * timeout->tv_sec +
            timeout->tv_usec;
    } else {
        timeout_tick = (uint64_t) -1;
    }

    // tyndur ist so toll, bei uns gibt es keine Fehler
    if (err) {
        *err = 0;
    }

    do {
        int i;

        // Wieder alles zuruecksetzen
        fds = 0;
        if (rd) {
            *rd = orig_rd;
        }
        if (wr) {
            *wr = orig_wr;
        }


        for (i = 0; i < 32; i++) {

            // Versuchsweise ein Zeichen auslesen ohne den Dateizeiger zu
            // verändern. Wenn das funktioniert, als lesbar werten.
            if (rd && (*rd & (1 << i))) {
                f = fd_to_file(i);

                if (f != NULL) {
                    ret = lio_compat_readahead(&c, 1, f);
                    if (ret == -EAGAIN) {
                        *rd &= ~(1 << i);
                    } else {
                        // Auch eine Datei, die zu Ende ist, gilt als lesbar
                        fds++;
                    }
                }
            }

            // Schreiben geht immer
            if (wr && (*wr & (1 << i))) {
                fds++;
            }
        }

        // Busy Wait waere irgendwie auch doof
        if (fds == 0) {
            yield();
        }

    } while ((fds == 0) && (timeout_tick > get_tick_count()));

    return fds;
}

#endif // ndef CONFIG_LIBC_NO_STUBS

