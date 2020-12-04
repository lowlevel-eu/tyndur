/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include "kernel.h"
#include "kprintf.h"

#include "lostio/client.h"
#include "lostio_int.h"

/** Prüft, ob ein gegebener LIO-Pfad absolut ist */
static bool lio_path_absolute(const char* path)
{
    for (; *path; path++) {
        if (path[0] == ':' && path[1] == '/') {
            return true;
        } else if (*path == '/' || *path == '|') {
            return false;
        }
    }

    return false;
}

struct lio_resource* lio_do_get_resource(const char* path, int follow_symlinks,
                                         int depth);

/**
 * Versucht den übergebenen Symlink aufzulösen. Wenn die Resource, auf die er
 * zeigt, existiert, wird ein Zeiger darauf zurückgegeben, NULL sonst.
 *
 * @param link Symlink, der aufgelöst werden soll
 * @param path Absoluter Pfad des Symlinks
 * @param n_service Anzahl der Zeichen vom Beginn des Pfads, die den Service
 *                  beschreiben (einschließlich :/)
 * @param n_rel Anzahl der Zeichen vom Beginn des servicerelativen Teils des
 *              Pfads bis zum Anfang des Dateinamens des Symlinks selbst
 * @param depth Symlink-Tiefe
 *
 * @return Zeiger auf das Ziel, oder NULL falls der Link nicht aufgelöst werden
 *         konnte.
 */
static struct lio_resource* resolve_symlink(struct lio_resource* link,
    const char* path, size_t n_service, size_t n_rel, int depth)
{
    char buf[n_service + n_rel + 1 + link->size + 1];
    struct lio_stream* stream;
    ssize_t len;

    if (depth > SYMLOOP_MAX) {
        return NULL;
    }

    if (!(stream = lio_open(link, LIO_READ | LIO_SYMLINK))) {
        return NULL;
    }

    len = lio_read(stream, link->size, buf, 0);
    lio_close(stream);

    if (len < 0) {
        return NULL;
    }
    buf[len] = 0;

    if (*buf == '/') {
        // Service-relativer Symlink
        memmove(&buf[n_service - 1], buf, link->size + 1);
        memcpy(buf, path, n_service - 1);
    } else if (!lio_path_absolute(buf)) {
        // Relativer Symlink
        memmove(&buf[n_service + n_rel + 1], buf, link->size + 1);
        memcpy(buf, path, n_service + n_rel + 1);
        buf[n_service + n_rel] = '/';
    }

    return lio_do_get_resource(buf, 1, depth + 1);
}

static int get_node_in_dir(struct lio_resource* dir, const char* name,
                           struct lio_node** node)
{
    struct lio_node* n;
    int ret;
    int i;

    // Falls die Liste der Kinder nicht geladen ist, nachholen
    if (dir->children == NULL) {
        ret = dir->tree->service->lio_ops.load_children(dir);
        if (ret < 0) {
            return ret;
        }
    }

    // Passendes Kind raussuchen
    for (i = 0; (n = list_get_element_at(dir->children, i)); i++) {
        if (strcmp(n->name, name) == 0) {
            if (node) {
                *node = n;
            }
            return 0;
        }
    }

    return -ENOENT;
}

struct lio_resource* lio_do_get_resource(const char* path, int follow_symlinks,
                                         int depth)
{
    struct lio_tree* tree;
    struct lio_resource* res;
    struct lio_node* node;
    const char* rel_path;
    char* tokenize_path;
    char* name;
    char* old_name = NULL;
    int ret;

    // Passenden Baum raussuchen
    tree = lio_get_tree(path, &rel_path, depth);
    if (tree == NULL) {
        return NULL;
    }

    node = tree->root;

    // Von der Wurzel aus durchhangeln
    char* saveptr;
    tokenize_path = strdup(rel_path);
    name = strtok_r(tokenize_path, "/", &saveptr);
    while (name != NULL) {
        res = node->res;

        // Falls wir es mit einem Symlink zu tun haben, versuchen wir doch mal
        // dem zu folgen.
        if (res->resolvable) {
            res = resolve_symlink(res, path, rel_path - path,
                                  old_name ? old_name - tokenize_path : 0,
                                  depth);
            if (res == NULL) {
                goto out;
            }
        }

        tree = res->tree;

        // Wir suchen noch ein Kind, aber diese Ressource hat keine
        if (!res->browsable) {
            res = NULL;
            goto out;
        }

        // Passendes Kind raussuchen
        ret = get_node_in_dir(res, name, &node);
        if (ret < 0) {
            res = NULL;
            goto out;
        }

        // Passender Kindknoten wurde gefunden, weiter im Pfad
        old_name = name;
        name = strtok_r(NULL, "/", &saveptr);
    }

    res = node->res;

    // Auch das koennte noch mal ein Symlink sein, den wir aufloesen muessen
    if (res && follow_symlinks && res->resolvable) {
        res = resolve_symlink(res, path, rel_path - path,
                              old_name ? old_name - tokenize_path : 0,
                              depth);
    }

out:
    free(tokenize_path);
    return res;
}

/**
 * Loest einen Pfad zu einer Ressource auf
 *
 * @return Die zum Pfad gehoerende Ressource oder NULL, wenn der Pfad keine
 * existierende Ressource beschreibt.
 */
struct lio_resource* lio_get_resource(const char* path, int follow_symlinks)
{
    return lio_do_get_resource(path, follow_symlinks, 0);
}

/**
 * Erstellt eine neue Pipe und gibt einen Stream auf beide Enden zurück
 *
 * @param bidirectional Wenn false, dann kann auf writer nur geschrieben und
 * auf reader nur gelesen werden. Wenn true, dann funktioniert auch die andere
 * Richtung.
 *
 * @return 0 bei Erfolg, -errno im Fehlerfall
 */
int lio_pipe(struct lio_stream **reader, struct lio_stream **writer,
             bool bidirectional)
{
    /* TODO Eigentlich braucht es res_b nur für bidirectional */
    struct lio_resource* res_a = lio_create_pipe();
    struct lio_resource* res_b = lio_create_pipe();
    struct lio_stream* s_reader = NULL;
    struct lio_stream* s_writer = NULL;
    int ret;

    *reader = NULL;
    *writer = NULL;

    /* XXX Das ist ein Hack, weil lio_open() ansonsten beim ersten Mal
     * res->excl_stream setzt und uns zweimal denselben Stream zurückgibt. */
    res_a->seekable = true;
    res_b->seekable = true;

    if (!res_a|| !res_b) {
        ret = -ENOMEM;
        goto fail;
    }

    s_reader = lio_open(res_a, LIO_READ);
    if (s_reader == NULL) {
        ret = -EIO;
        goto fail;
    }

    if (bidirectional) {
        s_writer = lio_open(res_b, LIO_WRITE);
        if (s_writer == NULL) {
            ret = -EIO;
            goto fail;
        }
    }

    *reader = lio_composite_stream(s_reader, s_writer);
    if (*reader == NULL) {
        ret = -ENOMEM;
        goto fail;
    }

    lio_close(s_reader);
    lio_close(s_writer);

    s_reader = NULL;
    s_writer = NULL;

    if (bidirectional) {
        s_reader = lio_open(res_b, LIO_READ);
        if (s_reader == NULL) {
            ret = -EIO;
            goto fail;
        }
    }

    s_writer = lio_open(res_a, LIO_WRITE);
    if (s_writer == NULL) {
        ret = -EIO;
        goto fail;
    }

    *writer = lio_composite_stream(s_reader, s_writer);
    if (*writer == NULL) {
        ret = -ENOMEM;
        goto fail;
    }

    lio_close(s_reader);
    lio_close(s_writer);

    res_a->seekable = false;
    res_b->seekable = false;

    return 0;

fail:
    if (res_a->ref == 0) {
        lio_destroy_pipe(res_a);
    }
    if (res_b->ref == 0) {
        lio_destroy_pipe(res_b);
    }

    BUG_ON(*writer);
    lio_close(*reader);
    lio_close(s_reader);
    lio_close(s_writer);
    return ret;
}

/**
 * Oeffnet einen Stream zu einer Ressource
 *
 * @param flags Bitmaske für den Modus, in dem der Stream arbeiten soll (siehe
 * enum lio_flags)
 *
 * @return Stream zur Ressource oder NULL, wenn die Ressource nicht erfolgreich
 * geoeffnet werden konnte.
 */
struct lio_stream* lio_open(struct lio_resource* res, int flags)
{
    struct lio_stream* s;

    if (res == NULL) {
        return NULL;
    }

    // Nur lesbare Ressourcen duerfen mit dem LIO_READ-Flag geoeffnet werden,
    // und nur Schreibbare mit LIO_WRITE oder LIO_TRUNC.
    bool symlink = !!(flags & LIO_SYMLINK);
    if (((flags & LIO_READ) && !res->readable && !symlink) ||
        ((flags & (LIO_WRITE | LIO_TRUNC) && !res->writable && !symlink)) ||
        ((flags & LIO_READ) && !res->resolvable && symlink) ||
        ((flags & (LIO_WRITE | LIO_TRUNC) && !res->retargetable && symlink)))
    {
        return NULL;
    }

    // Symlinks duerfen nur als solche geoeffnet werden
    if (res->resolvable && !(flags & LIO_SYMLINK)) {
        return NULL;
    }

    // Non-seekable Streams dürfen nur einmal geöffnet sein.  Vorläufig den
    // schon existierenden Stream zurückgeben, falls die Flags passen.
    // TODO Immer Fehler zurückgeben sobald der Userspace damit klarkommt
    if (res->excl_stream) {
        if (res->excl_stream->flags != flags) {
            return NULL;
        }
        res->excl_stream->ref++;
        return res->excl_stream;
    }

    // Stream anlegen und initialisieren
    s = calloc(1, sizeof(*s));
    s->simple.res = res;
    s->flags = flags;
    s->eof = false;
    s->ref = 1;

    // Wenn es sich um eine Pipe handelt, muessen wir entsprechende Ressourcen
    // anlegen.
    if (res->ispipe && res->tree->service->lio_ops.pipe) {
        struct lio_stream* s_serv;
        int ret;

        ret = lio_pipe(&s, &s_serv, true);
        if (ret < 0) {
            return NULL;
        }

        if (!res->tree->service->lio_ops.pipe ||
            res->tree->service->lio_ops.pipe(res, s_serv, s_serv->flags))
        {
            lio_close(s);
            lio_close(s_serv);
            return NULL;
        }

        return s;
    }

    if (res->tree->service->lio_ops.preopen &&
        res->tree->service->lio_ops.preopen(s))
    {
        goto fail;
    }


    // Datei leeren, wenn LIO_TRUNC gesetzt ist
    if (flags & LIO_TRUNC) {
        if (lio_truncate(s, 0) < 0) {
            goto fail;
        }
    }

    // Non-seekable Streams dürfen nur einmal geöffnet sein
    if (!res->seekable) {
        res->excl_stream = s;
    }

    res->ref++;
    return s;

fail:
    free(s);
    return NULL;
}

/**
 * Setzt einen neuen Stream zusammen, der Lesezugriffe an read und
 * Schreibzugriffe an write writerleitet. Wenn read oder write NULL sind, ist
 * der zusammengesetzt Stream entsprechend nicht les- oder schreibbar.
 *
 * @return Zusammengesetzter Stream oder NULL, wenn der Stream nicht
 * erfolgreich erstellt werden konnte.
 */
struct lio_stream* lio_composite_stream(struct lio_stream* read,
                                        struct lio_stream* write)
{
    struct lio_stream* s;
    bool use_reader = read && (read->flags & LIO_READ);
    bool use_writer = write && (write->flags & LIO_WRITE);

    s = calloc(1, sizeof(*s));
    if (s == NULL) {
        return NULL;
    }

    *s = (struct lio_stream) {
        .is_composite       = true,
        .composite.read     = read,
        .composite.write    = write,
        .flags              = (use_reader ? LIO_READ : 0) | (use_writer ? LIO_WRITE : 0),
        .eof                = false,
        .ref                = 1,
    };

    if (use_reader) {
        read->ref++;
    }
    if (use_writer) {
        write->ref++;
    }

    return s;
}

static struct lio_stream* stream_get_root(struct lio_stream *s, bool read)
{
    while (s && s->is_composite) {
        s = read ? s->composite.read : s->composite.write;
    }
    return s;
}

static uint64_t stream_getpos(struct lio_stream* s, bool read)
{
    s = stream_get_root(s, read);
    return s ? s->simple.pos : 0;
}

static void stream_updatepos(struct lio_stream* s, bool read, uint64_t pos)
{
    s = stream_get_root(s, read);
    BUG_ON(!s);
    s->simple.pos = pos;
}

/**
 * Liest aus einem Stream aus.
 *
 * @return Anzahl der gelesenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_read(struct lio_stream* s, size_t bytes, void* buf, int flags)
{
    ssize_t result;
    uint64_t pos;

    pos = stream_getpos(s, true);

    result = lio_pread(s, pos, bytes, buf, flags);
    if (result > 0) {
        stream_updatepos(s, true, pos + result);
    }

    return result;
}

/**
 * Liest aus einem Stream aus, ohne die Position des streams zu benutzen oder
 * zu veraendern. Diese Funktion blockiert nie, sondern gibt -EAGAIN zurück,
 * wenn es nichts zu lesen gibt.
 *
 * @return Anzahl der gelesenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_pread_nonblock(struct lio_stream* s, uint64_t offset, size_t bytes,
    void* buf)
{
    ssize_t ret = 0;
    struct lio_resource* res;

    s = stream_get_root(s, true);

    // Stream muss existieren und zum Lesen geoeffnet sein
    if (!s || (s->flags & LIO_READ) == 0) {
        return -EBADF;
    }

    res = s->simple.res;

    // Position pruefen
    if (offset >= res->size) {
        if (res->moredata) {
            return -EAGAIN;
        }
        if (offset > res->size) {
            return 0;
        }
    }

    // Abschneiden, wenn ueber das Dateiende hinausgelesen wird
    if (offset + bytes > res->size) {
        bytes = res->size - offset;
    }

    // Daten einlesen
    uint64_t offset_aligned = ROUND_TO_BLOCK_DOWN(offset, res->blocksize);
    size_t bytes_aligned = ROUND_TO_NEXT_BLOCK(
        bytes + (offset - offset_aligned), res->blocksize);

    struct lio_driver* drv = &res->tree->service->lio_ops;
    uint8_t* bounce = malloc(bytes_aligned);
    uint8_t* p = bounce;
    uint64_t curoffset = offset_aligned;

    ret = drv->read(res, curoffset, bytes_aligned, p);
    if (ret >= 0) {
        if (bytes == 0 && res->moredata) {
            ret = -EAGAIN;
        } else {
            ret = bytes;
            memcpy(buf, bounce + (offset - offset_aligned), ret);
        }
    } else if (ret == -EAGAIN && !res->moredata) {
        ret = 0;
    }

    free(bounce);

    return ret;
}

static void lio_pread_unblock_cb(void* opaque)
{
    pm_thread_t* t = opaque;

    BUG_ON(t->status != PM_STATUS_WAIT_FOR_DATA);
    t->status = PM_STATUS_READY;
}

/**
 * Liest aus einem Stream aus, ohne die Position des streams zu benutzen oder
 * zu veraendern. Diese Funktion blockiert, bis etwas gelesen werden kann.
 *
 * @return Anzahl der gelesenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_pread(struct lio_stream* s, uint64_t offset, size_t bytes,
    void* buf, int flags)
{
    bool blocking = (flags & LIO_REQ_BLOCKING);
    ssize_t ret;

    s = stream_get_root(s, true);

    ret = lio_pread_nonblock(s, offset, bytes, buf);
    while (ret == -EAGAIN && blocking) {
        BUG_ON(s->is_composite);
        notifier_add(&s->simple.res->on_resize,
                     lio_pread_unblock_cb, current_thread, true);
        pm_scheduler_kern_yield(0, PM_STATUS_WAIT_FOR_DATA);
        ret = lio_pread_nonblock(s, offset, bytes, buf);
    }

    return ret;
}

/**
 * Schreibt in einen Stream
 *
 * @return Anzahl der geschriebenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_write(struct lio_stream* s, size_t bytes, const void* buf)
{
    ssize_t result;
    uint64_t pos;

    pos = stream_getpos(s, false);

    result = lio_pwrite(s, pos, bytes, buf);
    if (result > 0) {
        stream_updatepos(s, false, pos + result);
    }

    return result;
}

/**
 * Schreibt in einen Stream, ohne dabei die Position des Streams zu benutzen
 * oder zu veraendern.
 *
 * @return Anzahl der geschriebenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_pwrite(struct lio_stream* s, uint64_t offset, size_t bytes,
    const void* buf)
{
    ssize_t ret;
    struct lio_resource* res;

    s = stream_get_root(s, false);

    // Stream muss existieren und zum Schreiben geoeffnet sein
    if (!s || (s->flags & LIO_WRITE) == 0) {
        return -EBADF;
    }

    res = s->simple.res;
    if (!res->writable && (!(s->flags & LIO_SYMLINK) || !res->retargetable)) {
        return -EBADF;
    }

    // Falls noetig Ressource vergroessern
    if ((offset + bytes) > res->size) {
        ret = lio_truncate(s, offset + bytes);
        if (ret < 0) {
            return ret;
        }
    }

    // Daten einlesen
    uint64_t offset_aligned = ROUND_TO_BLOCK_DOWN(offset, res->blocksize);
    size_t bytes_aligned = ROUND_TO_NEXT_BLOCK(
        bytes + (offset - offset_aligned), res->blocksize);

    struct lio_driver* drv = &res->tree->service->lio_ops;
    uint8_t* bounce = calloc(1, bytes_aligned);
    uint8_t* p = bounce;
    uint64_t curoffset = offset_aligned;
    int old_flags;

    if (offset != offset_aligned || bytes != bytes_aligned) {
        old_flags = s->flags;
        // FIXME Wenn das mal nicht racy ist...
        s->flags |= LIO_READ;
        ret = lio_pread(s, offset_aligned, bytes_aligned, bounce, 0);
        s->flags = old_flags;
    }

    if (ret < 0) {
        goto out;
    }
    memcpy(bounce + (offset - offset_aligned), buf, bytes);

    ret = drv->write(res, curoffset, bytes_aligned, p);
    if (ret >= 0) {
        ret = bytes;
    }

    // Offset zeigt jetzt auf das erste Byte nach dem letzten geschriebenen
    if (ret > 0){
        if (res->size < offset) {
            lio_resource_update_size(res, offset);
        }
    }

out:
    free(bounce);
    return ret;
}

/**
 * Position veraendern. Funktioniert nur, wenn die Ressource seekable ist, und
 * es sich um keinen zusammengesetzten Stream (wie z.B. eine Pipe) handelt.
 *
 * @param offset Offset um den die Position angepasst werden soll. Wie sie genau
 *               angepasst wird, haengt vom Parameter whence ab.
 * @param whence Einer der drei Werte aus enum lio_seek_whence.
 *                  - LIO_SEEK_SET: Position auf den angegebenen Wert setzen.
 *                  - LIO_SEEK_CUR: Angegebenen Wert zur aktuellen Position
 *                                  dazu addieren.
 *                  - LIO_SEEK_END: Position auf das Dateiende setzen.
 *
 * @return Neue Position oder < 0 im Fehlerfall.
 */
int64_t lio_seek(struct lio_stream* s, int64_t offset, int whence)
{
    bool seekable;;
    uint64_t pos;

    if (s->is_composite) {
        seekable = false;
        s = stream_get_root(s, true);
        if (s == NULL) {
            return -EINVAL;
        }
    } else {
        seekable = s->simple.res->seekable;
    }

    /* LIO_SEEK_CUR zum Herausfinden der aktuellen Position oder zum
     * Überspringen von Daten geht auch auf Ressourcen, die eigentlich kein
     * seek unterstützen. */
    if (!seekable && (whence != LIO_SEEK_CUR || offset < 0)) {
        return -EINVAL;
    }

    pos = stream_getpos(s, true);
    switch (whence) {
        case LIO_SEEK_SET:
            if (offset < 0) {
                return -EINVAL;
            } else {
                stream_updatepos(s, true, offset);
            }
            break;

        case LIO_SEEK_CUR:
            if ((int64_t)(pos + offset) < 0) {
                return -EINVAL;
            } else {
                stream_updatepos(s, true, pos + offset);
            }
            break;

        case LIO_SEEK_END:
            if ((int64_t)(s->simple.res->size + offset) < 0) {
                return -EINVAL;
            } else {
                stream_updatepos(s, true, s->simple.res->size + offset);
            }
            break;
        default:
            return -EINVAL;
    }

    return stream_getpos(s, true);
}

/**
 * Schreibt Aenderungen, die bisher nur im Cache sind, zurueck. Dabei werden
 * nur die Cluster beruecksichtigt, die im angegebenen Bereich liegen.
 *
 * @param s Stream, dessen Aendungen geschrieben werden sollen
 * @param offset Start des zurueckzuschreibenden Bereichs (in Bytes)
 * @param bytes Laenge des zurueckzuschreibenden Bereichs (in Bytes) oder 0,
 * falls die gesamte Datei zurueckgeschrieben werden soll
 *
 * @return 0 bei Erfolg, ungleich 0 im Fehlerfall
 *
 * @attention Diese Funktion ist unterbrechbar
 */
int lio_sync_blocks(struct lio_resource* res, uint64_t offset, size_t bytes)
{
    return 0;
}

/**
 * Schreibt alle zum Zeitpunkt des Aufrufs der Funktion vorhandenen
 * Aenderungen, die bisher nur im Cache sind, zurueck
 *
 * @return 0 bei Erfolg, ungleich 0 im Fehlerfall
 * @attention Diese Funktion ist unterbrechbar
 */
int lio_sync(struct lio_stream* s)
{
    struct lio_resource* res;
    struct lio_driver* drv;

    s = stream_get_root(s, false);
    if (s == NULL) {
        return -EBADF;
    }

    res = s->simple.res;
    drv = &res->tree->service->lio_ops;

    /*
     * TODO Muss z.B. ata nochmal gesynct werden, nachdem ext2 dran war? Ist es
     * Aufgabe des Treibers oder von LIO?
     */
    if (drv->sync) {
        drv->sync(res);
    }

    return 0;
}

/**
 * Alle veraenderten Blocks im Cache rausschreiben
 *
 * @param soft Wenn true, wird nicht blockiert, wenn ein Cluster nicht gesynct
 * werden kann. Es wird auch kein Fehler zurückgegeben. (Ziel ist es, einfach
 * irgendwelche Dirty-Cluster loszuwerden)
 *
 * @return 0 bei Erfolg, -errno im Fehlerfall
 */
int lio_sync_all(bool soft)
{
    return 0;
}

/**
 * Aendert die Dateigroesse
 *
 * @param size Neue Dateigroesse
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
int lio_truncate(struct lio_stream* s, uint64_t size)
{
    struct lio_resource* res;
    struct lio_driver* drv;

    s = stream_get_root(s, false);
    if (s == NULL) {
        return -EBADF;
    }

    res = s->simple.res;
    drv = &res->tree->service->lio_ops;

    /* Zunaechst den Treiber informieren */
    if (drv->truncate) {
        int ret = drv->truncate(res, size);
        if (ret < 0) {
            return ret;
        }
    }

    /* Anschliessend die Ressourcengroesse anpassen (drv->truncate kann das
     * u.U. auch schon gemacht haben) */
    lio_resource_update_size(res, size);

    return 0;
}

/**
 * Schliesst einen Stream
 *
 * @return 0 bei Erfolg, ungleich 0 im Fehlerfall
 */
int lio_close(struct lio_stream* s)
{
    if (!s || --s->ref) {
        return 0;
    }

    if (s->is_composite) {
        s->flags = 0;
        lio_close(s->composite.read);
        lio_close(s->composite.write);
        free(s);
        return 0;
    }

    // Sicherstellen, dass lio_open() den Stream nicht rausrückt, während wir
    // ihn gerade schließen
    s->flags = 0;
    if (s->simple.res->excl_stream == s) {
        s->simple.res->excl_stream = NULL;
    }

    BUG_ON(s->simple.res->ref <= 0);
    s->simple.res->ref--;

    if (s->simple.res->tree->service->lio_ops.close) {
        s->simple.res->tree->service->lio_ops.close(s);
        s->simple.res = NULL;
    }

    free(s);
    return 0;
}

/**
 * Führt einen ressourcenspezifischen Befehl auf einem Stream aus.
 *
 * @param s Stream, auf den sich der Befehl bezieht
 * @param cmd Auszuführender Befehl (LIO_IOCTL_*-Konstanten)
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_ioctl(struct lio_stream* s, int cmd)
{
    s = stream_get_root(s, true);
    if (s == NULL) {
        return -EBADF;
    }

    switch (cmd) {
        case LIO_IOCTL_ISATTY:
            if (s->simple.res->is_terminal) {
                return 0;
            }
            return -ENOTTY;
        default:
            return -ENOTTY;
    }
}

/**
 * Inhalt eines Verzeichnisses auslesen
 */
ssize_t lio_read_dir(struct lio_resource* dir, size_t start, size_t num,
    struct lio_dir_entry* buf)
{
    struct lio_node* node;
    size_t cnt = 0;

    /* Die Ressource muss auch wirklich ein Verzeichnis sein */
    if (!dir->browsable) {
        return -EBADF;
    }

    /* Wenn der Aufrufer keine Eintraege will, sind wir fertig */
    if (!num) {
        return 0;
    }

    /* Zunaechst kommt der Eintrag fuer . */
    if (start == 0) {
        strcpy(buf[0].name, ".");
        buf[0].resource = dir;
        lio_stat(dir, &buf[0].stat);
        cnt++;
    }

    /* Und dann der ganzer Rest */
    if (!dir->children) {
        dir->tree->service->lio_ops.load_children(dir);
    }

    for (; (cnt < num) &&
        (node = list_get_element_at(dir->children, start - 1 + cnt));)
    {
        buf[cnt].resource = node->res;
        strncpy(buf[cnt].name, node->name, LIO_MAX_FNLEN);
        buf[cnt].name[LIO_MAX_FNLEN] = 0;
        lio_stat(node->res, &buf[cnt].stat);
        cnt++;
    }

    return cnt;
}

/** Erstellt eine neue Datei */
int lio_mkfile(struct lio_resource* parent, const char* name,
               struct lio_resource** res)
{
    if (!parent->tree->service->lio_ops.make_file || !parent->changeable) {
        return -EACCES;
    }

    if (get_node_in_dir(parent, name, NULL) == 0) {
        return -EEXIST;
    }

    *res = parent->tree->service->lio_ops.make_file(parent, name);
    return *res ? 0 : -EIO;
}

/** Erstellt ein neues Verzeichnis */
int lio_mkdir(struct lio_resource* parent, const char* name,
               struct lio_resource** res)
{
    if (!parent->tree->service->lio_ops.make_dir || !parent->changeable) {
        return -EACCES;
    }

    if (get_node_in_dir(parent, name, NULL) == 0) {
        return -EEXIST;
    }

    *res = parent->tree->service->lio_ops.make_dir(parent, name);
    return *res ? 0 : -EIO;
}

/** Erstellt einen neuen Symlink */
int lio_mksymlink(struct lio_resource* parent, const char* name,
                  const char* target, struct lio_resource** res)
{
    if (!parent->tree->service->lio_ops.make_symlink || !parent->changeable) {
        return -EACCES;
    }

    if (get_node_in_dir(parent, name, NULL) == 0) {
        return -EEXIST;
    }

    *res = parent->tree->service->lio_ops.make_symlink(parent, name, target);
    return *res ? 0 : -EIO;
}

/**
 * Gibt Informationen zu einer Ressource zurueck
 */
int lio_stat(struct lio_resource* resource, struct lio_stat* sbuf)
{
    memset(sbuf, 0, sizeof(*sbuf));
    sbuf->size = resource->size;

    sbuf->flags = (resource->readable ? LIO_FLAG_READABLE : 0) |
        (resource->writable ? LIO_FLAG_WRITABLE : 0) |
        (resource->browsable ? LIO_FLAG_BROWSABLE : 0) |
        (resource->changeable ? LIO_FLAG_CHANGEABLE : 0) |
        (resource->resolvable ? LIO_FLAG_RESOLVABLE : 0) |
        (resource->retargetable ? LIO_FLAG_RETARGETABLE : 0) |
        (resource->seekable ? LIO_FLAG_SEEKABLE : 0) |
        (resource->ispipe ? LIO_FLAG_PIPE : 0);
    return 0;
}

/** Verzeichniseintrag loeschen */
int lio_unlink(struct lio_resource* parent, const char* name)
{
    if (!parent->tree->service->lio_ops.unlink) {
        return -EINVAL;
    }

    return parent->tree->service->lio_ops.unlink(parent, name);
}

