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

#include "kernel.h"
#include "kprintf.h"

#include "lostio/client.h"
#include "lostio_int.h"

/**
 * Versucht den uebergebenen Symlink aufzuloesen. Wenn die Resource, auf die er
 * Zeigt, existiert wird ein Zeiger darauf zurueckgegeben, NULL sonst.
 *
 * @return Zeiger auf das Ziel, oder NULL falls der Link nicht aufgeloest werden
 *         konnte.
 */
static struct lio_resource* resolve_symlink(struct lio_resource* link)
{
    char buf[link->size + 1];
    struct lio_stream* stream;
    ssize_t len;

    if (!(stream = lio_open(link, LIO_READ | LIO_SYMLINK))) {
        return NULL;
    }

    len = lio_read(stream, link->size, buf);
    lio_close(stream);

    if (len < 0) {
        return NULL;
    }
    buf[len] = 0;

    // TODO: Relative Symlinks
    return lio_get_resource(buf, 1);
}

/**
 * Loest einen Pfad zu einer Ressource auf
 *
 * @return Die zum Pfad gehoerende Ressource oder NULL, wenn der Pfad keine
 * existierende Ressource beschreibt.
 */
struct lio_resource* lio_get_resource(const char* path, int follow_symlinks)
{
    struct lio_tree* tree;
    struct lio_resource* res;
    struct lio_node* node;
    const char* rel_path;
    char* tokenize_path;
    char* name;
    int i;

    // Passenden Baum raussuchen
    tree = lio_get_tree(path, &rel_path);
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
            res = resolve_symlink(res);
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

        // Falls die Liste der Kinder nicht geladen ist, nachholen
        if (res->children == NULL) {
            if (tree->service->lio_ops.load_children(res)) {
                res = NULL;
                goto out;
            }
        }

        // Passendes Kind raussuchen
        for (i = 0; (node = list_get_element_at(res->children, i)); i++) {
            if (strcmp(node->name, name) == 0) {
                goto found;
            }
        }
        res = NULL;
        goto out;

found:
        // Passender Kindknoten wurde gefunden, weiter im Pfad
        name = strtok_r(NULL, "/", &saveptr);
    }

    res = node->res;

    // Auch das koennte noch mal ein Symlink sein, den wir aufloesen muessen
    if (res && follow_symlinks && res->resolvable) {
        res = resolve_symlink(res);
    }

out:
    free(tokenize_path);
    return res;
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

    // Stream anlegen und initialisieren
    s = calloc(1, sizeof(*s));
    s->res = s->res_read = s->res_write = res;
    s->flags = flags;
    s->eof = false;

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

    return s;

fail:
    free(s);
    return NULL;
}

/**
 * Liest aus einem Stream aus.
 *
 * @return Anzahl der gelesenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_read(struct lio_stream* s, size_t bytes, void* buf)
{
    ssize_t result;

    result = lio_pread(s, s->pos_read, bytes, buf);
    if (result > 0) {
        s->pos_read += result;
    }

    return result;
}

/**
 * Liest aus einem Stream aus, ohne die Position des streams zu benutzen oder
 * zu veraendern.
 *
 * @return Anzahl der gelesenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_pread(struct lio_stream* s, uint64_t offset, size_t bytes,
    void* buf)
{
    ssize_t ret = 0;
    struct lio_resource* res;

    // Stream muss existieren und zum Lesen geoeffnet sein
    if (!s || (s->flags & LIO_READ) == 0) {
        return -EBADF;
    }

    res = s->res_read;

    // Position pruefen
    if (offset >= res->size) {
        if (res->moredata) {
            return -EAGAIN;
        }
        return 0;
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
        ret = bytes;
        memcpy(buf, bounce + (offset - offset_aligned), ret);
    }

    free(bounce);

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

    pos = ((s->res_read == s->res_write) ? s->pos_read : s->pos_write);

    result = lio_pwrite(s, pos, bytes, buf);
    if (result > 0) {
        if (s->res_read == s->res_write) {
            s->pos_read += result;
        } else {
            s->pos_write += result;
        }
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

    // Stream muss existieren und zum Schreiben geoeffnet sein
    if (!s || (s->flags & LIO_WRITE) == 0) {
        return -EBADF;
    }

    res = s->res_write;
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

    old_flags = s->flags;
    s->flags |= LIO_READ;
    ret = lio_pread(s, offset_aligned, bytes_aligned, bounce);
    s->flags = old_flags;

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
            res->size = offset;
        }
    }

out:
    free(bounce);
    return ret;
}

/**
 * Position veraendern. Funktioniert nur, wenn die Ressource seekable ist, und
 * es sich um keine Pipe handelt.
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
    if ((s->res_read != s->res_write) || !s->res_read->seekable) {
        return -EINVAL;
    }

    switch (whence) {
        case LIO_SEEK_SET:
            if (offset < 0) {
                return -EINVAL;
            } else {
                s->pos_read = offset;
            }
            break;

        case LIO_SEEK_CUR:
            if ((int64_t)(s->pos_read + offset) < 0) {
                return -EINVAL;
            } else {
                s->pos_read += offset;
            }
            break;

        case LIO_SEEK_END:
            if ((int64_t)(s->res_read->size + offset) < 0) {
                return -EINVAL;
            } else {
                s->pos_read = s->res_read->size + offset;
            }
            break;
        default:
            return -EINVAL;
    }

    return s->pos_read;
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
    struct lio_resource* res = s->res_write;
    struct lio_driver* drv = &res->tree->service->lio_ops;

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
    struct lio_resource* res = s->res_write;
    struct lio_driver* drv = &res->tree->service->lio_ops;

    /* Zunaechst den Treiber informieren */
    if (drv->truncate) {
        int ret = drv->truncate(res, size);
        if (ret < 0) {
            return ret;
        }
    }

    /* Anschliessend die Ressourcengroesse anpassen (drv->truncate kann das
     * u.U. auch schon gemacht haben) */
    res->size = size;

    return 0;
}

/**
 * Schliesst einen Stream
 *
 * @return 0 bei Erfolg, ungleich 0 im Fehlerfall
 */
int lio_close(struct lio_stream* s)
{
    if (s->res->tree->service->lio_ops.close) {
        s->res->tree->service->lio_ops.close(s);
    }

    free(s);
    return 0;
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
struct lio_resource* lio_mkfile(struct lio_resource* parent, const char* name)
{
    if (!parent->tree->service->lio_ops.make_file || !parent->changeable) {
        return NULL;
    }

    return parent->tree->service->lio_ops.make_file(parent, name);
}

/** Erstellt ein neues Verzeichnis */
struct lio_resource* lio_mkdir(struct lio_resource* parent, const char* name)
{
    if (!parent->tree->service->lio_ops.make_dir || !parent->changeable) {
        return NULL;
    }

    return parent->tree->service->lio_ops.make_dir(parent, name);
}

/** Erstellt einen neuen Symlink */
struct lio_resource* lio_mksymlink(struct lio_resource* parent,
    const char* name, const char* target)
{
    if (!parent->tree->service->lio_ops.make_symlink || !parent->changeable) {
        return NULL;
    }

    return parent->tree->service->lio_ops.make_symlink(parent, name, target);
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

