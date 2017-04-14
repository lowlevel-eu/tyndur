/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#ifndef _LOSTIO_CLIENT_H_
#define _LOSTIO_CLIENT_H_

#include "lostio/core.h"

struct lio_dir_entry {
    struct lio_resource* resource;
    char                 name[LIO_MAX_FNLEN + 1];
    struct lio_stat      stat;
};

/**
 * Loest einen Pfad zu einer Ressource auf
 *
 * @return Die zum Pfad gehoerende Ressource oder NULL, wenn der Pfad keine
 * existierende Ressource beschreibt.
 */
struct lio_resource* lio_get_resource(const char* path, int follow_symlinks);

/** Service finden, der die Ressource als Pipequelle akzeptiert */
int lio_probe_service(struct lio_resource* res,
                      struct lio_probe_service_result* probe_data);

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
             bool bidirectional);

/**
 * Oeffnet einen Stream zu einer Ressource
 *
 * @param flags Bitmaske für den Modus, in dem der Stream arbeiten soll (siehe
 * enum lio_flags)
 *
 * @return Stream zur Ressource oder NULL, wenn die Ressource nicht erfolgreich
 * geoeffnet werden konnte.
 */
struct lio_stream* lio_open(struct lio_resource* res, int flags);

/**
 * Setzt einen neuen Stream zusammen, der die Eingaberessource von read und
 * die Ausgaberessource von write benutzt.
 *
 * @return Zusammengesetzter Stream oder NULL, wenn der Stream nicht
 * erfolgreich erstellt werden konnte.
 */
struct lio_stream* lio_composite_stream(struct lio_stream* read,
                                        struct lio_stream* write);

/**
 * Liest aus einem Stream aus.
 *
 * @return Anzahl der gelesenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_read(struct lio_stream* s, size_t bytes, void* buf, int flags);

/**
 * Liest aus einem Stream aus, ohne die Position des streams zu benutzen oder
 * zu veraendern.
 *
 * @return Anzahl der gelesenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_pread(struct lio_stream* s, uint64_t offset, size_t bytes,
    void* buf, int flags);

/**
 * Schreibt in einen Stream
 *
 * @return Anzahl der geschriebenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_write(struct lio_stream* s, size_t bytes, const void* buf);

/**
 * Schreibt in einen Stream, ohne dabei die Position des Streams zu benutzen
 * oder zu veraendern.
 *
 * @return Anzahl der geschriebenen Bytes oder negativ im Fehlerfall
 */
ssize_t lio_pwrite(struct lio_stream* s, uint64_t offset, size_t bytes,
    const void* buf);

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
int64_t lio_seek(struct lio_stream* s, int64_t offset, int whence);

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
int lio_sync_blocks(struct lio_resource* res, uint64_t offset, size_t bytes);

/**
 * Schreibt alle zum Zeitpunkt des Aufrufs der Funktion vorhandenen
 * Aenderungen, die bisher nur im Cache sind, zurueck
 *
 * @return 0 bei Erfolg, ungleich 0 im Fehlerfall
 * @attention Diese Funktion ist unterbrechbar
 */
int lio_sync(struct lio_stream* s);

/**
 * Alle veraenderten Blocks im Cache rausschreiben
 *
 * @param soft Wenn true, wird nicht blockiert, wenn ein Cluster nicht gesynct
 * werden kann. Es wird auch kein Fehler zurückgegeben. (Ziel ist es, einfach
 * irgendwelche Dirty-Cluster loszuwerden)
 *
 * @return 0 bei Erfolg, -errno im Fehlerfall
 */
int lio_sync_all(bool soft);

/**
 * Aendert die Dateigroesse
 *
 * @param size Neue Dateigroesse
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
int lio_truncate(struct lio_stream* s, uint64_t size);

/**
 * Schliesst einen Stream
 *
 * @return 0 bei Erfolg, ungleich 0 im Fehlerfall
 */
int lio_close(struct lio_stream* s);

/**
 * Führt einen ressourcenspezifischen Befehl auf einem Stream aus.
 *
 * @param s Stream, auf den sich der Befehl bezieht
 * @param cmd Auszuführender Befehl (LIO_IOCTL_*-Konstanten)
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_ioctl(struct lio_stream* s, int cmd);

/**
 * Inhalt eines Verzeichnisses auslesen
 */
ssize_t lio_read_dir(struct lio_resource* dir, size_t start, size_t num,
    struct lio_dir_entry* buf);

/** Erstellt eine neue Datei */
int lio_mkfile(struct lio_resource* parent, const char* name,
               struct lio_resource** res);

/** Erstellt ein neues Verzeichnis */
int lio_mkdir(struct lio_resource* parent, const char* name,
               struct lio_resource** res);

/** Erstellt einen neuen Symlink */
int lio_mksymlink(struct lio_resource* parent, const char* name,
                  const char* target, struct lio_resource** res);

/**
 * Gibt Informationen zu einer Ressource zurueck
 */
int lio_stat(struct lio_resource* resource, struct lio_stat* sbuf);

/** Verzeichniseintrag loeschen */
int lio_unlink(struct lio_resource* parent, const char* name);

#endif
