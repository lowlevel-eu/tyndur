/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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

#include <syscall.h>
#include <string.h>

/**
 * Sucht eine Ressource anhand eines Pfads und gibt ihre ID zurück.
 *
 * @param follow_symlink Wenn follow_symlink true ist und der Pfad auf eine
 * symbolische Verknüpfung zeigt, dann wird die Verknüfung aufgelöst. Ist
 * follow_symlink false, wird in diesem Fall die Verknüpfung selbst
 * zurückgegeben.
 *
 * @return Ressourcen-ID bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -ENOENT     Es wurde keine Ressource mit dem gegebenen Pfad gefunden
 */
lio_resource_t lio_resource(const char* path, bool follow_symlink)
{
    volatile lio_resource_t result;
    size_t len;

    len = strlen(path) + 1;
    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : : "i" (SYSCALL_LIO_RESOURCE), "r" (path), "r" (len),
        "r" ((uint32_t) follow_symlink), "r" (&result) : "memory");

    return result;
}

/**
 * Erstellt eine neue Pipe und gibt Streams auf beide Enden zurück.
 *
 * @param reader Wird bei Erfolg auf das erste Ende der Pipe gesetzt
 * @param writer Wird bei Erfolg auf das zweite Ende der Pipe gesetzt
 * @param bidirectional Wenn false, dann kann auf writer nur geschrieben und
 * auf reader nur gelesen werden. Wenn true, dann funktioniert auch die andere
 * Richtung.
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
int lio_pipe(lio_stream_t* reader, lio_stream_t* writer, bool bidirectional)
{
    volatile lio_stream_t rd;
    volatile lio_stream_t wr;
    int result;

    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
    : "=a" (result)
    : "i" (SYSCALL_LIO_PIPE), "r" (&rd), "r" (&wr),
      "r" ((uint32_t) bidirectional)
    : "memory");

    if (result == 0) {
        *reader = rd;
        *writer = wr;
    }

    return result;
}

/**
 * Öffnet eine Ressource und gibt die ID des entstandenen Streams zurück.
 *
 * @param resource ID der zu öffnenden Ressource
 * @param flags Bitmaske aus LIO_*-Flags
 *
 * @return Stream-ID bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -EINVAL     Es gibt keine Ressource mit der gegebenen ID
 *      -EACCES     Die Ressource unterstützt ein gesetztes Flag nicht
 */
lio_stream_t lio_open(lio_resource_t resource, int flags)
{
    volatile lio_stream_t result;

    asm(
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
    : : "i" (SYSCALL_LIO_OPEN), "r" (&resource), "r" (flags), "r" (&result)
    : "memory");

    return result;
}

/**
 * Setzt einen neuen Stream zusammen, der die Eingaberessource von read und
 * die Ausgaberessource von write benutzt.
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
lio_stream_t lio_composite_stream(lio_stream_t read, lio_stream_t write)
{
    volatile lio_stream_t composite;
    int result;

    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
    : "=a" (result)
    : "i" (SYSCALL_LIO_COMPOSITE_STREAM), "r" (&read), "r" (&write),
      "r" (&composite)
    : "memory");

    if (result < 0) {
        return result;
    }

    return composite;
}

/**
 * Gibt Informationen zu einer Ressource zurück.
 *
 * @param resource ID der Ressource
 * @param sbuf Puffer für die Informationen
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -EINVAL     Es gibt keine Ressource mit der gegebenen ID
 */
int lio_stat(lio_resource_t resource, struct lio_stat* sbuf)
{
    int result;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (result) : "i" (SYSCALL_LIO_STAT), "r" (&resource), "r" (sbuf)
    : "memory");

    return result;
}

/**
 * Erstellt eine zusätzliche Stream-ID für einen Stream.
 *
 * @param source Der zu duplizierende Stream
 * @param dest Neue ID, die  der Stream erhalten soll, oder -1 für eine
 * beliebige freie ID. Wenn diese ID bereits einen Stream bezeichnet, wird
 * dieser geschlossen.
 *
 * @return Neue Stream-ID bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -EBADF      Ein der Stream-IDs ist ungültig
 */
lio_stream_t lio_dup(lio_stream_t source, lio_stream_t dest)
{
    volatile lio_stream_t d = dest;
    int result;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (result) : "i" (SYSCALL_LIO_DUP), "r" (&source), "r" (&d)
    : "memory");

    if (result < 0) {
        return result;
    } else {
        return d;
    }
}

/**
 * Schließt einen Stream.
 *
 * @param s Der zu schließende Stream
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_close(lio_stream_t s)
{
    int result;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (result): "i" (SYSCALL_LIO_CLOSE), "r" (&s) : "memory");

    return result;
}

/**
 * Gibt einen Stream an einen anderen Prozess ab. Der Stream wird in diesem
 * Prozess dabei ungültig.
 *
 * @param s Der abzugebenden Stream
 * @param pid Zielprozess, der den Stream erhalten soll
 *
 * @return Stream-ID im Zielprozess bei Erfolg, negativ im Fehlerfall.
 * Insbesondere:
 *
 *      -EINVAL     Es gibt keinen Prozess mit der gegebenen PID
 *      -EBADF      Die Stream-ID ist ungültig
 */
lio_stream_t lio_pass_fd(lio_stream_t s, pid_t pid)
{
    volatile lio_stream_t stream = s;
    int result;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (result) : "i" (SYSCALL_LIO_PASS_FD), "r" (&stream), "r" (pid)
    : "memory");

    if (result < 0) {
        return result;
    } else {
        return stream;
    }
}

/**
 * Gibt den Prozess zurück, der den Stream an den aktuellen Prozess übergeben
 * hat.
 *
 * @param s Der zu prüfende Stream
 * @return Falls der Stream von einem anderen Prozess übergeben wurde, dessen
 * PID. Falls er von diesem Prozess geöffnet wurde, die eigene PID. Negativ
 * im Fehlerfall, insbesondere:
 *
 *      -EBADF      Die Stream-ID ist ungültig
 */
int lio_stream_origin(lio_stream_t s)
{
    int result;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (result): "i" (SYSCALL_LIO_STREAM_ORIGIN), "r" (&s) : "memory");

    return result;
}

ssize_t lio_readf(lio_stream_t s, uint64_t offset, size_t bytes,
    void* buf, int flags)
{
    volatile ssize_t result;

    asm(
        "pushl %6;"
        "pushl %5;"
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x18, %%esp;"
    : : "i" (SYSCALL_LIO_READ), "r" (&s), "r" (&offset), "r" (bytes),
        "r" (buf), "r" (flags), "g" (&result) : "memory");

    return result;
}

/**
 * Liest aus dem Stream ab der aktuellen Position des Dateizeigers und bewegt
 * den Dateizeiger hinter das letzte gelesene Byte.
 *
 * @param s Auszulesender Stream
 * @param bytes Zu lesende Bytes
 * @param buf Zielpuffer, in dem die ausgelesenen Daten abgelegt werden
 *
 * @return Die Anzahl der erfolgreich gelesenen Bytes ("short reads" sind
 * möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_read(lio_stream_t s, size_t bytes, void* buf)
{
    return lio_readf(s, 0, bytes, buf, LIO_REQ_FILEPOS | LIO_REQ_BLOCKING);
}

/**
 * Liest aus dem Stream ab der gegebenen Position.
 *
 * @param s Auszulesender Stream
 * @param offset Position, ab der gelesen werden soll, in Bytes vom Dateianfang
 * @param bytes Zu lesende Bytes
 * @param buf Zielpuffer, in dem die ausgelesenen Daten abgelegt werden
 *
 * @return Die Anzahl der erfolgreich gelesenen Bytes ("short reads" sind
 * möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_pread(lio_stream_t s, uint64_t offset, size_t bytes, void* buf)
{
    return lio_readf(s, offset, bytes, buf, LIO_REQ_BLOCKING);
}

static ssize_t write_syscall(lio_stream_t s, uint64_t offset, size_t bytes,
    const void* buf, int updatepos)
{
    volatile ssize_t result;

    asm(
        "pushl %6;"
        "pushl %5;"
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x18, %%esp;"
    : : "i" (SYSCALL_LIO_WRITE), "r" (&s), "r" (&offset), "r" (bytes),
        "r" (buf), "r" (updatepos), "g" (&result) : "memory");

    return result;
}

/**
 * Schreibt in den Stream ab der aktuellen Position des Dateizeigers und bewegt
 * den Dateizeiger hinter das letzte geschriebene Byte.
 *
 * @param s Zu schreibender Stream
 * @param bytes Zu schreibende Bytes
 * @param buf Quellpuffer, dessen Inhalt geschrieben werden soll
 *
 * @return Die Anzahl der erfolgreich gegeschriebenen Bytes ("short writes"
 * sind möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_write(lio_stream_t s, size_t bytes, const void* buf)
{
    return write_syscall(s, 0, bytes, buf, 1);
}

/**
 * Schreibt in den Stream ab der gegebenen Position.
 *
 * @param s Zu schreibender Stream
 * @param offset Position, ab der geschrieben werden soll, in Bytes vom
 * Dateianfang
 * @param bytes Zu schreibende Bytes
 * @param buf Quellpuffer, dessen Inhalt geschrieben werden soll
 *
 * @return Die Anzahl der erfolgreich gegeschriebenen Bytes ("short writes"
 * sind möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_pwrite(lio_stream_t s, uint64_t offset, size_t bytes,
    const void* buf)
{
    return write_syscall(s, offset, bytes, buf, 0);
}

/**
 * Bewegt den Dateizeiger des Streams und gibt seine aktuelle Position zurück
 *
 * @param s Stream, dessen Dateizeiger verändert werden soll
 * @param offset Neue Dateizeigerposition als Offset in Bytes
 * @param whence Gibt an, wozu das Offset relativ ist (LIO_SEEK_*-Konstanten)
 *
 * @return Die neue Position des Dateizeigers in Bytes vom Dateianfang, oder
 * negativ im Fehlerfall.
 */
int64_t lio_seek(lio_stream_t s, uint64_t offset, int whence)
{
    volatile int64_t result;

    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : : "i" (SYSCALL_LIO_SEEK), "r" (&s), "r" (&offset), "r" (whence),
        "r" (&result) : "memory");

    return result;
}

/**
 * Ändert die Dateigröße der Ressource eines Streams
 * TODO Wieso nimmt das einen Stream und keine Ressource?
 *
 * @param s Zu ändernder Stream
 * @param size Neue Dateigröße in Bytes
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_truncate(lio_stream_t s, uint64_t size)
{
    int result;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x08, %%esp;"
    : "=a" (result)
    : "i" (SYSCALL_LIO_TRUNCATE), "r" (&s), "r" (&size)
    : "memory");

    return result;
}

/**
 * Führt einen ressourcenspezifischen Befehl auf einem Stream aus.
 *
 * @param s Stream, auf den sich der Befehl bezieht
 * @param cmd Auszuführender Befehl (LIO_IOCTL_*-Konstanten)
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_ioctl(lio_stream_t s, int cmd)
{
    int result;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : "=a" (result)
    : "i" (SYSCALL_LIO_IOCTL), "r" (&s), "r" (cmd)
    : "memory");

    return result;
}

/**
 * Liest Einträge einer Verzeichnisressource aus.
 *
 * @param res Auszulesende Verzeichnisressource
 * @param start Index des ersten zurückzugebenden Verzeichniseintrags
 * @param num Maximale Anzahl zurückzugebender Verzeichniseinträge
 * (Puffergröße)
 * @param buf Array von Verzeichniseinträgen, die befüllt werden
 *
 * @return Anzahl der zurückgegebenen Verzeichniseinträge, oder negativ im
 * Fehlerfall.
 */
ssize_t lio_read_dir(lio_resource_t res, size_t start, size_t num,
    struct lio_dir_entry* buf)
{
    volatile ssize_t result;

    asm(
        "pushl %5;"
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x14, %%esp;"
    : : "i" (SYSCALL_LIO_READ_DIR), "r" (&res), "r" (start), "r" (num),
        "r" (buf), "r" (&result) : "memory");

    return result;

}

/**
 * Erstellt eine neue Datei.
 *
 * @param parent Verzeichnis, das die neue Datei enthalten soll
 * @param name Name der neuen Datei
 *
 * @return Ressourcen-ID der neuen Datei bei Erfolg, negativ im Fehlerfall.
 */
lio_resource_t lio_mkfile(lio_resource_t parent, const char* name)
{
    volatile lio_resource_t result;
    size_t len;

    len = strlen(name) + 1;
    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : : "i" (SYSCALL_LIO_MKFILE), "r" (&parent), "r" (name), "r" (len),
        "r" (&result) : "memory");

    return result;
}

/**
 * Erstellt ein neues Verzeichnis.
 *
 * @param parent Verzeichnis, das das neue Unterverzeichnis enthalten soll
 * @param name Name des neuen Verzeichnisses
 *
 * @return Ressourcen-ID des neuen Verzeichnisses bei Erfolg, negativ im
 * Fehlerfall.
 */
lio_resource_t lio_mkdir(lio_resource_t parent, const char* name)
{
    volatile lio_resource_t result;
    size_t len;

    len = strlen(name) + 1;
    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : : "i" (SYSCALL_LIO_MKDIR), "r" (&parent), "r" (name), "r" (len),
        "r" (&result) : "memory");

    return result;
}

/**
 * Erstellt eine neue symbolische Verknüpfung
 *
 * @param parent Verzeichnis, das die neue Verknüofung enthalten soll
 * @param name Name der neuen Verknüpfung
 *
 * @return Ressourcen-ID der neuen Verknüpfung bei Erfolg, negativ im
 * Fehlerfall.
 */
lio_resource_t lio_mksymlink(lio_resource_t parent,
    const char* name, const char* target)
{
    volatile lio_resource_t result;
    size_t name_len;
    size_t target_len;

    name_len = strlen(name) + 1;
    target_len = strlen(target) + 1;
    asm(
        "pushl %6;"
        "pushl %5;"
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x18, %%esp;"
    : : "i" (SYSCALL_LIO_MKSYMLINK), "r" (&parent), "r" (name), "r" (name_len),
        "r" (target), "r" (target_len), "g" (&result) : "memory");

    return result;
}

/*
 * Sofern die Ressource grundsätzlich auf persistentem Speicher liegt, aber
 * unter Umständen bisher nur in einem nichtpersistentem Cache liegt, werden
 * alle Änderungen, die an diesem Stream durchgeführt worden sind, persistent
 * gemacht.
 *
 * @param s Zurückzuschreibender Stream
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_sync(lio_stream_t s)
{
    int result;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (result): "i" (SYSCALL_LIO_SYNC), "r" (&s) : "memory");

    return result;
}

/*
 * Löscht den Verzeichniseintrag einer Ressource aus ihrem Verzeichnis. Wenn
 * es keine weiteren Verweise (Hardlinks) auf die Ressource mehr gibt, wird sie
 * gelöscht.
 *
 * @param parent Verzeichnis, das die zu löschende Ressource enthält
 * @param name Dateiname der zu löschenden Ressource
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_unlink(lio_resource_t parent, const char* name)
{
    int result;
    size_t name_len;

    name_len = strlen(name) + 1;
    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
    : "=a" (result) : "i" (SYSCALL_LIO_UNLINK), "r" (&parent), "r" (name),
        "r" (name_len) : "memory");

    return result;
}

/**
 * Schreibt alle Änderungen im Kernelcache zurück.
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_sync_all(void)
{
    int result;

    asm(
        "pushl $0;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (result) : "0" (SYSCALL_LIO_SYNC_ALL));

    return result;
}

/**
 * Versucht einen Service zu finden, der die angegebene Ressource als
 * Pipequelle verwenden kann (z.B. passendes Dateisystem für Image)
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
int lio_probe_service(lio_resource_t res, struct lio_probe_service_result* ret)
{
    int result;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (result) : "i" (SYSCALL_LIO_PROBE_SERVICE), "r" (&res), "r" (ret)
    : "memory");

    return result;
}
