/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <stdio.h>
#include <rpc.h>
#include <stdlib.h>
#include <lostio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <syscall.h>
#include <errno.h>

#define IO_BUFFER_MODE_NONE 0
#define IO_BUFFER_MODE_FULL 1
#define IO_BUFFER_MODE_LINE 2

extern char* io_get_absolute_path(const char* path);

#define IS_LIO2_FILE(f) IS_LIO2(f->res)

static bool is_valid_res(FILE* io_res)
{
    return (io_res != NULL) && (IS_LIO2(io_res->res) || io_res->res->pid != 0);
}

FILE* __create_file_from_iores(io_resource_t* io_res)
{
    // FILE-Struktur anlegen
    FILE* int_res;

    int_res = calloc(1, sizeof(*int_res));
    int_res->res = io_res;

    int_res->buffer_mode = IO_BUFFER_MODE_NONE;
    int_res->buffer_ptr = NULL;
    int_res->buffer_size = 0;
    int_res->buffer_pos = 0;
    int_res->free_buffer = false;
    setvbuf(int_res, malloc(BUFSIZ), _IOFBF, BUFSIZ);
    int_res->free_buffer = true;

    int_res->cur_pos = 0;
    int_res->error = false;
    int_res->os_eof = false;

    return int_res;
}

FILE* __create_file_from_lio_stream(lio_stream_t s)
{
    io_resource_t* iores = malloc(sizeof(*iores));
    *iores = (io_resource_t) {
        .lio2_stream    =   s,
        .lio2_res       =   1, /* XXX Hässlich, aber wird nur auf > 0 geprüft */
        .flags          =   IO_RES_FLAG_READAHEAD,
    };
    return __create_file_from_iores(iores);
}

/**
 * Datei oeffnen
 *
 * @param filename Pfad
 * @param mode String der den Zugriffsmodus festlegt
 *
 * @return Dateihandle oder NULL wenn ein Fehler aufgetreten ist.
 */
FILE* fopen (const char* filename, const char* mode)
{
    uint8_t attr;

    //Attribute aus dem mode-String parsen
    attr = 0;
    int i;
    for (i = 0; i < strlen(mode); i++) {
        switch (mode[i]) {
            case 'r':
                attr |= IO_OPEN_MODE_READ;
                if (mode[i + 1] == '+') {
                    attr |= IO_OPEN_MODE_WRITE;
                }
                break;

            case 'w':
                attr |= IO_OPEN_MODE_WRITE | IO_OPEN_MODE_CREATE |
                    IO_OPEN_MODE_TRUNC;

                // Bei w+ muss Lesen auch aktiviert werden.
                if (mode[i + 1] == '+') {
                    attr |= IO_OPEN_MODE_READ;
                }
                break;

            case 'a':
                attr |= IO_OPEN_MODE_WRITE | IO_OPEN_MODE_CREATE
                    | IO_OPEN_MODE_APPEND;
                if (mode[i + 1] == '+') {
                    attr |= IO_OPEN_MODE_READ;
                }
                break;

            case 'd':
                attr |= IO_OPEN_MODE_DIRECTORY;
                break;
        }
    }

    // Datei oeffnen
    io_resource_t* io_res = lio_compat_open(filename, attr);
    if (io_res == NULL) {
        return NULL;
    }

    return __create_file_from_iores(io_res);
}

/**
 * Neue Datei mit dem selben Stream oeffnen
 *
 * @param path Pfad
 * @param mode Modus
 * @param stream Stream
 *
 * @return stream bei Erfolg, im Fehlerfall NULL
 */
FILE* freopen(const char* path, const char* mode, FILE* stream)
{
    // Neue Datei in anderem Stream oeffnen
    FILE* new_file = fopen(path, mode);
    if (new_file == NULL) {
        return NULL;
    }
    
    // Inhalte der Streams wechseln
    FILE tmp_file = *stream;
    *stream = *new_file;
    *new_file = tmp_file;
    
    // Altes Handle im neuen Stream schliessen
    fclose(new_file);
    return stream;
}

/**
 * Alle Buffer flushen und Dateihandle schliessen.
 *
 * @param io_res Dateihandle
 *
 * @return 0 Wenn die Datei erfolgreich geschlosswn wurde, sonst EOF
 */
int fclose (FILE* io_res)
{
    int result;

    if (!is_valid_res(io_res)) {
        return EOF;
    }

    fflush(io_res);
    setvbuf(io_res, NULL, _IONBF, 0);

    result = lio_compat_close(io_res->res);
    free(io_res);

    return result ? EOF : 0;
}

lio_stream_t file_get_stream(FILE *io_res)
{
    if (!is_valid_res(io_res) || !IS_LIO2_FILE(io_res)) {
        return -EBADF;
    }

    return io_res->res->lio2_stream;
}

bool file_is_terminal(FILE* io_res)
{
    int ret;

    if (!is_valid_res(io_res) || !IS_LIO2_FILE(io_res)) {
        return -EBADF;
    }

    ret = lio_ioctl(io_res->res->lio2_stream, LIO_IOCTL_ISATTY);
    return (ret >= 0);
}

static void prepare_buffer(FILE* io_res, bool is_write)
{
    if (io_res->buffer_writes == is_write) {
        return;
    }

    if (io_res->buffer_writes == false) {
        lio_compat_seek(io_res->res, io_res->buffer_pos, LIO_SEEK_CUR);
        io_res->cur_pos += io_res->buffer_pos;
    } else {
        fflush(io_res);
    }

    io_res->buffer_pos = 0;
    io_res->buffer_filled = 0;
    io_res->buffer_writes = is_write;
}

size_t fread_buffered(void* dest, size_t bytes, FILE* io_res)
{
    ssize_t ret;

    if ((io_res->res->flags & IO_RES_FLAG_READAHEAD) == 0) {
        return 0;
    }

    if (io_res->buffer_ptr == NULL) {
        return 0;
    }

    if (io_res->buffer_pos < io_res->buffer_filled) {
        size_t len = io_res->buffer_filled - io_res->buffer_pos;
        if (len > bytes) {
            len = bytes;
        }
        memcpy(dest, io_res->buffer_ptr + io_res->buffer_pos, len);
        io_res->buffer_pos += len;
        return len;
    }

    io_res->buffer_filled = 0;
    lio_compat_seek(io_res->res, io_res->buffer_pos, LIO_SEEK_CUR);
    io_res->cur_pos += io_res->buffer_pos;
    io_res->buffer_pos = 0;

    if (bytes > io_res->buffer_size / 4) {
        return 0;
    }

    ret = lio_compat_readahead(io_res->buffer_ptr, io_res->buffer_size,
                               io_res->res);
    if (ret < 0 || ret < bytes) {
        io_res->buffer_filled = 0;
        return 0;
    }
    io_res->buffer_filled = ret;

    memcpy(dest, io_res->buffer_ptr, bytes);
    io_res->buffer_pos = bytes;

    return bytes;
}

/**
 * Teil aus der Datei lesen
 *
 * @param dest Pointer auf den Buffer wohin die Daten geladen werden
 * @param blocksize Blockgroesse
 * @param blockcount Anzahl der Bloecke
 * @param io_res Dateihandle
 *
 * @return Laenge der gelesenen Daten
 */
size_t fread(void* dest, size_t blocksize, size_t blockcount, FILE* io_res)
{
    ssize_t ret;
    size_t read_bytes = 0;

    if (!is_valid_res(io_res)) {
        return 0;
    }

    if (!blocksize || !blockcount) {
        return 0;
    }

    // Wenn Daten im ungetc-Buffer sind, werden die zuerst ausgelesen.
    if (io_res->ungetc_count != 0) {
        read_bytes = (io_res->ungetc_count <= blocksize * blockcount ? io_res->
            ungetc_count : blocksize * blockcount);
        
        int i;
        // Der Puffer muss in umgekehrter Reihenfolge ausgelesen werden
        for (i = 1; i <= read_bytes; i++) {
            *((uint8_t*) ((uintptr_t) dest + i - 1)) = io_res->ungetc_buffer[
                io_res->ungetc_count - i];
        }

        // Buffer verkleinern
        io_res->ungetc_count -= read_bytes;
        uint8_t* new_buffer = realloc(io_res->ungetc_buffer, io_res->
            ungetc_count);
        if ((io_res->ungetc_count == 0) && (new_buffer == NULL)) {
            io_res->ungetc_buffer = NULL;
        } else if ((io_res->ungetc_count == 0) && (new_buffer != NULL)) {
            free(new_buffer);
            io_res->ungetc_buffer = NULL;
        } else if ((io_res->ungetc_count != 0) && (new_buffer != NULL)) {
            io_res->ungetc_buffer = new_buffer;
        }

        if (read_bytes == (blocksize * blockcount)) {
            return read_bytes; 
        }
    }

    // Ein Schreibpuffer wird noch geleert, damit auch wirklich die Aktuellsten
    // Daten gelesen werden.
    prepare_buffer(io_res, false);

    read_bytes += fread_buffered(dest + read_bytes,
        (blocksize * blockcount) - read_bytes, io_res);

    if (blocksize * blockcount > read_bytes) {
        lio_compat_seek(io_res->res, io_res->buffer_pos, LIO_SEEK_CUR);
        io_res->cur_pos += io_res->buffer_pos;

        io_res->buffer_pos = 0;
        io_res->buffer_filled = 0;

        while (blocksize * blockcount > read_bytes) {
            ret = lio_compat_read(dest + read_bytes,
                (blocksize * blockcount) - read_bytes, 1, io_res->res);

            if (ret == -EAGAIN) {
                if (read_bytes == 0) {
                    yield();
                }
                break;
            } else if (ret < 0) {
                io_res->error = true;
                break;
            } else if (!ret) {
                io_res->os_eof = true;
                break;
            } else {
                read_bytes += ret;
                io_res->cur_pos += ret;
            }
        }
    }

    return read_bytes / blocksize;
}


/**
 * Einzelnes Zeichen aus der Datei lesen
 *
 * @param io_res Dateihandle
 *
 * @return Das gelesene Zeichen oder EOF wenn ein Fehler aufgetreten ist.
 */
int fgetc(FILE* io_res)
{
    unsigned char c;

    if ((feof(io_res) == 0) && fread(&c, 1, 1, io_res) == 1) {
        return c;
    } else {
        return EOF;
    }
}

/**
 * Zeilen aus einer Datei lesen.
 *
 * @param dest Pointer auf den Buffer, worein die Daten gelesen werden sollen
 * @param length Anzahl der maximal zu lesenden Bytes
 * @param io_res Dateihandle
 *
 * @return Wenn Daten gelesen wurde, wird der dest-Pointer zugrueck gegeben.
 *          Falls beim lesen Fehler aufgetreten sind, oder der Cursor schon am
 *          Ende der Datei ist, und keine Daten gelesen werden konnten, wird
 *          NULL zurueck gegeben werden.
 */
char* fgets(char* dest, int length, FILE *io_res)
{
    int i,c;
    
    if ((feof(io_res) != 0) || (ferror(io_res) != 0)) {
        return NULL;
    }

    //Die einzelnen Zeichen einlesen.
    //TODO: Hier koennten warscheinlich ein paar Sachen noch optimiert werden
    for (i = 0; i < (length - 1); i++) {
        c = fgetc(io_res);
        
        if ((c == EOF) && (ferror(io_res) != 0)) {
            i = 0;
            break;
        } else if (feof(io_res)) {
            break;
        } else if (c == EOF) {
            i--;
            continue;
        }
        
        *dest = (char) c;
        dest++;

        if (c == '\n') {
            i++;
            break;
        }
    }
    
    *dest = '\0';
    if (i == 0) {
        return NULL;
    } else {
        return dest;
    }
}

/**
 * Ein byte zurueck in den Stream schieben. Es wird aber nicht gespeichert
 * sondern nur bei weiteren Lesevorgaengen zurueck gegeben.
 *
 * @param c Das Zeichen (wird auf unsigned char gecasted)
 * @param io_res Dateihandle
 *
 * @return c bei Erfolg, EOF im Fehlerfall
 */
int ungetc(int c, FILE* io_res)
{
    if (io_res == NULL) {
        return EOF;
    }
    
    // Versuchen den Buffer zu vergroessern
    uint8_t* new_buffer = realloc(io_res->ungetc_buffer, io_res->ungetc_count +
        1);
    if (new_buffer == NULL) {
        return EOF;
    } else {
        io_res->ungetc_buffer = new_buffer;
        io_res->ungetc_count++;
    }
    
    io_res->ungetc_buffer[io_res->ungetc_count - 1] = (unsigned char) c;
    return c;
}

/**
 * Daten in die Datei schreiben.
 *
 * @param data Pointer auf die Quelldaten
 * @param blocksize Blockgroesse
 * @param blockcount Anzahl der Bloecke
 * @param io_res Dateihandle
 */
size_t fwrite(const void* data, size_t blocksize, size_t blockcount,
    FILE* io_res)
{
    size_t data_size = blocksize * blockcount;
    ssize_t ret;

    if (!is_valid_res(io_res) || (blocksize == 0)) {
        return 0;
    }

    // Wenn Daten im ungetc-Buffer sind, wird dieser geleert
    if (io_res->ungetc_count != 0) {
        free(io_res->ungetc_buffer);
        io_res->ungetc_buffer = NULL;
        io_res->ungetc_count = 0;
    }

    // Puffer leeren, wenn es ein Lesepuffer ist
    prepare_buffer(io_res, true);

	//Wenn kein Buffer gesetzt ist, aber das Zwischenspeichern nicht deaktivert
	// ist, wird das hier gemacht. Das schuetzt vor Pagefaults ;-).
	if ((io_res->buffer_ptr == NULL) && (io_res->buffer_mode != 
		IO_BUFFER_MODE_NONE))
	{
		io_res->buffer_mode = IO_BUFFER_MODE_NONE;
	}

    //Wenn die Daten groesser als 64 Kilobyte sind, werden sie nicht im Buffer
    // gespeichert, sondern direkt an den Zielprozess gesendet.
    switch (io_res->buffer_mode) {
        case IO_BUFFER_MODE_NONE:
            ret = lio_compat_write(data, blocksize, blockcount, io_res->res);
            if (ret < 0) {
                io_res->error = true;
                return 0;
            } else {
                io_res->cur_pos += ret;
                return ret / blocksize;
            }
            break;

        case IO_BUFFER_MODE_FULL:
            //Ueberpruefen, ob genug Platz im Buffer ist, dann werden die Daten
            // in den Buffer kopiert, sonst wird der Buffer geleert, und die
            // Daten werden direkt geschrieben.
            if (io_res->buffer_size >= (io_res->buffer_pos + data_size)) {
                //Daten in den Buffer kopieren.
                memcpy(io_res->buffer_ptr + io_res->buffer_pos, data,
                        data_size);
                io_res->buffer_pos += data_size;
                return data_size / blocksize;
            } else {
                if (fflush(io_res) == EOF) {
                    return 0;
                }
                ret = lio_compat_write(data, blocksize, blockcount, io_res->res);
                if (ret < 0) {
                    io_res->error = true;
                    return 0;
                } else {
                    io_res->cur_pos += ret;
                    return ret / blocksize;
                }
            }
            break;

        case IO_BUFFER_MODE_LINE: {
            //Ueberpruefen, ob genug Platz im Buffer ist, sonst wird der Buffer
            // geleert, und die Daten werden auch direkt geschrieben. Auch wenn
            // die Daten mindestens einen Zeilenumbruch enthalten, wird der
            // Buffer geleert.
            if ((io_res->buffer_size < (io_res->buffer_pos + data_size)) ||
                (memchr(data, '\n', data_size) != NULL))
            {
                if (fflush(io_res) == EOF) {
                    return 0;
                }
                ret = lio_compat_write(data, blocksize, blockcount, io_res->res);
                if (ret < 0) {
                    io_res->error = true;
                    return 0;
                } else {
                    io_res->cur_pos += ret;
                    return ret / blocksize;
                }
            } else {
                //Daten in den Buffer kopieren.
                memcpy((void*)((uint32_t) io_res->buffer_ptr +
                    io_res->buffer_pos), data, data_size);
                io_res->buffer_pos += data_size;
                return data_size / blocksize;
            }
        }
    }

    return 0;
}


/**
 * Einzelnes Zeichen in die Datei schreiben
 *
 * @param c Zeichen
 * @param io_res Dateihandle
 *
 * @return Das geschriebene Zeichen oder EOF bei Fehler
 */
int fputc(int c, FILE *io_res)
{
    char ch = c;
    if (fwrite(&ch, 1, 1, io_res) == 1) {
        return c;
    } else {
        return EOF;
    }
}


/**
 * Zeichenkette in die Datei schreiben
 *
 * @param str Zeichenkette
 * @param io_res Dateihandle
 *
 * @return 1 wenn die Zeichenkette erfolgreich geschrieben wurde, sonst EOF
 */
int fputs(const char *str, FILE *io_res)
{
    if (fwrite(str, 1, strlen(str), io_res) == strlen(str)) {
        return 1;
    } else {
        return EOF;
    }
}


/**
 * Cursorposition im Dateihandle setzen
 *
 * @param io_res Dateihandle
 * @param offset Offset bezogen auf den mit origin festgelegten Ursprung
 * @param origin Ursprung. Moeglichkeiten: 
 *                  - SEEK_SET Bezogen auf Dateianfang
 *                  - SEEK_CUR Bezogen auf die aktuelle Position
 *                  - SEEK_END Bezogen auf das Ende der Datei
 *
 * @return 0 wenn die Position erfolgreich gesetzt wurde, sonst != 0
 */
int fseek (FILE* io_res, long int offset, int origin)
{
    int ret;

    // Ungueltige Handles abfangen
    if (!is_valid_res(io_res)) {
        return -1;
    }

    // Wenn innerhalb von einem Lesepuffer herumgesprungen wird, brauchen wir
    // nichts neues von der Platte zu laden
    if (io_res->buffer_writes == false) {
        switch (origin) {
            case SEEK_SET:
                if (offset >= io_res->cur_pos &&
                    offset < io_res->cur_pos + io_res->buffer_filled)
                {
                    io_res->buffer_pos = offset - io_res->cur_pos;
                    ret = 0;
                    goto out;
                }
                break;
            case SEEK_CUR:
                if (io_res->buffer_pos + offset >= 0 &&
                    io_res->buffer_pos + offset < io_res->buffer_filled)
                {
                    io_res->buffer_pos += offset;
                    ret = 0;
                    goto out;
                }
                break;
        }
    }

    // Ansonsten müssen wir den Puffer wegwerfen bzw. zurückschreiben
    prepare_buffer(io_res, true);
    fflush(io_res);

    ret = (lio_compat_seek(io_res->res, offset, origin) ? 0 : -1);

    if (ret == 0) {
        switch (origin) {
            case SEEK_SET:
                io_res->cur_pos = offset;
                break;
            case SEEK_CUR:
                io_res->cur_pos += offset;
                break;
            case SEEK_END:
                io_res->ungetc_count = 0;
                io_res->buffer_pos = 0;
                io_res->cur_pos = ftell(io_res);
                break;
        }
    }

out:
    // ungetc-Buffer leeren
    if (ret == 0) {
        if (io_res->ungetc_count != 0) {
            free(io_res->ungetc_buffer);
            io_res->ungetc_buffer = NULL;
            io_res->ungetc_count = 0;
        }
    }
    return ret;
}


/**
 * Cursorposition ausfindig machen
 *
 * @param io_res Handle
 * 
 * @return Cursorposition
 */
long ftell(FILE* io_res)
{
    long result;

    //Ungueltige Handles abfangen
    if (!is_valid_res(io_res)) {
        return EOF;
    }

    if (!IS_LIO2_FILE(io_res)) {
        //Sonst könnte die Angabe falsch sein
        fflush(io_res);
    }

    result = lio_compat_tell(io_res->res);
    if (result >= 0) {
        result += io_res->buffer_pos - io_res->ungetc_count;
        if (result < 0) {
            result = 0;
        }
    }
    return result;
}


/**
 * Testen ob der Cursor im Handle schon das Ende der Datei erreicht hat.
 *
 * @param io_res Handle
 *
 * @return 0 Wenn das Dateiende noch nicht erreicht wurde. Sonst != 0
 */
int feof(FILE* io_res)
{
    //Ungueltige Handles abfangen
    if (!is_valid_res(io_res)) {
        return EOF;
    }

    // Wenn Daten im ungetc-Buffer sind, ist der Cursor noch nicht am Ende.
    if (io_res->ungetc_count != 0) {
        return 0;
    }

    if (io_res->buffer_writes == false &&
        io_res->buffer_pos < io_res->buffer_filled)
    {
        return 0;
    }

    if (IS_LIO2_FILE(io_res)) {
        // FIXME: EOF ist hier eigentlich doof, aber irgend ein Idiot hat das
        // mal so implementiert und das wird teilweise so geprüft... ;-)
        return (io_res->os_eof ? EOF : 0);
    }

    //Auch hier könnte mit Daten im Buffer ein falsches Ergebnis rauskommen
    prepare_buffer(io_res, true);
    fflush(io_res);

    return lio_compat_eof(io_res->res);
}


/**
 * Ueberprueft ob ein Fehler beim Lesen oder Schreiben der Datei aufgetreten
 * ist.
 *
 * @param io_res Dateihandle
 *
 * @return 0 wenn kein Fehler aufgetreten ist, sonst != 0
 */
int ferror(FILE* io_res)
{
    return io_res->error;
}


/**
 * Fehler-Flag loeschen
 *
 * @param io_res Dateihandle
 */
void clearerr(FILE* io_res)
{
    io_res->error = false;
    io_res->os_eof = false;
}


/**
 * Dateihandle zuruecksetzen und Fehler-Flag loeschen.
 *
 * @param io_res Dateihandle
 */
void rewind(FILE* io_res)
{
    fseek(io_res, 0, SEEK_SET);
    clearerr(io_res);
}


/**
 * Falls vorhanden wird der Buffer-Inhalt geschrieben
 *
 * @param io_res Dateihandle
 * 
 * @return 0 wenn kein Fehler aufgetreten ist, sonst EOF
 */
int fflush(FILE* io_res)
{
    //Ungueltige Handles abfangen
    if (!is_valid_res(io_res)) {
        return EOF;
    }
    
    //Wenn kein Buffer vorhanden ist, oder wenn er leer ist, wird hier
    // abgebrochen.
    if ((io_res->buffer_mode == IO_BUFFER_MODE_NONE) || 
        (io_res->buffer_ptr == NULL) || (io_res->buffer_size == 0) || 
        (io_res->buffer_pos == 0))
    {
        return 0;
    }

    // Wenn es ein Lesepuffer ist, gibt es auch nichts zu schreiben
    if (!io_res->buffer_writes) {
        return 0;
    }

    size_t size = lio_compat_write(io_res->buffer_ptr,
        1, io_res->buffer_pos, io_res->res);
    if (size > 0) {
        io_res->cur_pos += size;
    }

    if (size == io_res->buffer_pos) {
        io_res->buffer_pos = 0;
        return 0;
    } else {
        io_res->buffer_pos = 0;
        return EOF;
    }
}


/**
 * Falls vorhanden wird der Buffer-Inhalt verworfen
 *
 * @param io_res Dateihandle
 * 
 * @return 0 wenn kein Fehler aufgetreten ist, sonst EOF
 */
int fpurge(FILE* io_res)
{
    prepare_buffer(io_res, true);
	io_res->buffer_pos = 0;
    return 0;
}


/**
 * Buffereinstellungen
 *
 * @param io_res Dateihandle
 * @param buffer Pointer auf den Buffer
 * @param mode Moeglichkeiten:
 *              - _IONBF Kein
 *              - _IOFBF Voll
 *              - _IOLBF Zeilenweise
 * @param size Groesse des Buffers
 *
 * @return 0 wenn kein Fehler aufgetreten ist, sonst != 0
 */
int setvbuf(FILE* io_res, char* buffer, int mode, size_t size)
{
    struct lostio_internal_file* int_res =
        (struct lostio_internal_file*) io_res;
    char* old_buffer;

    // Wenn das Dateihandle nicht in Ordung ist, wird abgebrochen
    if (io_res == NULL) {
        return -1;
    }

    old_buffer = io_res->buffer_ptr;

	if (buffer == NULL) {
		io_res->buffer_ptr = NULL;
		io_res->buffer_mode = IO_BUFFER_MODE_NONE;
        goto out;
	}

	//Hier wird ein switch benutzt, damit keine nicht Definierten Werte fuer
	// den Modus benutzt werden können.
	switch (mode)
	{
		//Kein Zwischenspeichern
		case _IONBF:
			io_res->buffer_ptr = NULL;
            io_res->buffer_pos = 0;
            io_res->buffer_size = 0;
            io_res->buffer_filled = 0;
            io_res->buffer_writes = true;
			io_res->buffer_mode = IO_BUFFER_MODE_NONE;
			break;
			
		//Volles zwischenspeichern
		case _IOFBF:
			io_res->buffer_ptr = buffer;
			io_res->buffer_pos = 0;
			io_res->buffer_size = size;
            io_res->buffer_filled = 0;
            io_res->buffer_writes = true;
			io_res->buffer_mode = IO_BUFFER_MODE_FULL;
			break;
		
		//Zeilenweise zwischenspeichern
		case _IOLBF:
			io_res->buffer_ptr = buffer;
			io_res->buffer_pos = 0;
			io_res->buffer_size = size;
            io_res->buffer_filled = 0;
            io_res->buffer_writes = true;
			io_res->buffer_mode = IO_BUFFER_MODE_LINE;
			break;
		
		default:
			return -1;
	}

out:
    // Wenn bis jetzt ein intern Alloziierter Puffer benutzt wurde, muss er
    // freigegeben werden.
    if (int_res->free_buffer && old_buffer && (old_buffer != buffer)) {
        free(old_buffer);
        int_res->free_buffer = false;
    }

	return 0;
}

/**
 * Diese Funktionen sind alle von setvbuf abgeleitet
 */
int setbuf(FILE* io_res, char* buffer)
{
    return setvbuf(io_res, buffer, (buffer != NULL) ? _IOFBF : _IONBF,
        BUFSIZ);
}

int setbuffer(FILE* io_res, char* buffer, size_t size)
{
    return setvbuf(io_res, buffer, (buffer != NULL) ? _IOFBF : _IONBF, size);
}

int setlinebuf(FILE* io_res)
{
    return setvbuf(io_res, (char*) NULL, _IOLBF, 0);
}


/**
 * Datei loeschen
 *
 * @param filename Pfad zur Datei
 *
 * @return 0 wenn kein Fehler aufgetreten ist, sonst != 0
 */
int remove(const char* filename)
{
    return io_remove_link(filename);
}

#ifndef CONFIG_LIBC_NO_STUBS
#include <errno.h>
#include <dir.h>
/**
 * Datei verschieben
 *
 * TODO Verzeichnisse verschieben
 */
int rename(const char* oldpath, const char* newpath)
{
    uint8_t buffer[4096];
    const char* src_path = oldpath;
    const char* dst_path = newpath;

    FILE* src = fopen(src_path, "r");
    if (src == NULL) {
        fprintf(stderr, "rename: Konnte die Quelldatei nicht oeffnen\n");
        errno = ENOENT;
        return -1;
    }

    FILE* dst;
    if (is_directory(dst_path)) {
        fprintf(stderr, "rename: Ziel ist ein Verzeichnis\n");
        errno = EEXIST;
        return -1;
    } else {
        dst = fopen(dst_path, "w");
    }

    if (dst == NULL) {
        fprintf(stderr, "rename: Konnte die Zieldatei nicht oeffnen\n");
        fclose(src);
        errno = EACCES;
        return -1;
    }

    while (!feof(src)) {
        size_t length = fread(buffer, 1, sizeof(buffer), src);

        if (length != 0) {
            if (fwrite(buffer, 1, length, dst) != length) {
                length = 0;
            }
        }

        if (length == 0) {
            fprintf(stderr, "rename: Fehler beim Kopieren: %d\n", length);
            fclose(src);
            fclose(dst);
            errno = EIO;
            return -1;
        }
    }

    fclose(src);
    fclose(dst);

    unlink(src_path);

    return 0;
}
#endif

/**
 * Temporaere Datei erstellen, die nur solange besteht, wie sie geoeffnet ist.
 * Dazu wird das tmp-Modul benoetigt.
 *
 * @return Dateihandle oder NULL wenn keines erstellt werden konnte.
 */
FILE* tmpfile()
{
    return fopen("tmp:/create", "w+b");
}

