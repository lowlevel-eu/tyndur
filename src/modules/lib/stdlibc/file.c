/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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
#include <types.h>
#include <stdio.h>
#include "rpc.h"
#include <stdlib.h>
#include "lostio.h"
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define IO_BUFFER_MODE_NONE 0
#define IO_BUFFER_MODE_FULL 1
#define IO_BUFFER_MODE_LINE 2


char* io_get_absolute_path(const char* path)
{
    const char* cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        cwd = path;
    }
    char* buffer = malloc(strlen(path) + strlen(cwd) + 2);
    size_t cwd_len = strlen(cwd);
    
    //Ueberpruefen, ob ein relativer pfad vorliegt
    if(strcmp(path, "..") == 0)
    {
        //CWD kopieren
        memcpy(buffer, cwd, cwd_len + 1);

        int i = strlen(buffer) - 2;
        for(; i > 0; i--)
        {
            if(buffer[i] == '/')
            {
                if(buffer[i-1] == ':')
                {
                    i++;
                }
                buffer[i] = 0;
                break;
            }
        }
    }
    else if((strcmp(path, ".") == 0) && (strlen(path) == 1))
    {
        memcpy(buffer, cwd, cwd_len);
        buffer[cwd_len] = 0;
    }
    else if(strstr(path, ":/") == NULL)
    {
        //CWD kopieren
        memcpy(buffer, cwd, cwd_len);


        if(buffer[cwd_len-1] != '/')
        {
            buffer[cwd_len] = '/';
            memcpy((void*)((dword)buffer + cwd_len + 1), path, strlen(path) + 1);
        }
        else
        {
            memcpy((void*)((dword)buffer + cwd_len), path, strlen(path) + 1);
        }
    }
    else
    {
        memcpy(buffer, path, strlen(path) + 1);
    }
    
    if (cwd != path) {
        free((char*) cwd);
    }
    return buffer;
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
    char* full_path = io_get_absolute_path(filename);

    //RPC-Daten zusammenstellen
    char* msg = malloc(strlen(full_path) + 2);
    byte* attr = (byte*) msg;
    
    //Attribute aus dem mode-String parsen
    *attr = 0;
    int i;
    for (i = 0; i < strlen(mode); i++) {
        switch (mode[i]) {
            case 'r':
                *attr |= IO_OPEN_MODE_READ;
                break;

            case 'w':
                *attr |= IO_OPEN_MODE_WRITE | IO_OPEN_MODE_CREATE;
                break;

            case 'a':
                *attr |= IO_OPEN_MODE_WRITE | IO_OPEN_MODE_CREATE 
                    | IO_OPEN_MODE_APPEND;
                break;

            case 'd':
                *attr |= IO_OPEN_MODE_DIRECTORY;
                break;

            case 'l':
                *attr |= IO_OPEN_MODE_LINK;
                break;

            case 's':
                *attr |= IO_OPEN_MODE_SYNC;
                break;

        }
    }

    //Dateiname inkl Null-Byte kopieren
    memcpy((void*) ((dword)msg + 1), full_path, strlen(full_path) + 1);

    response_t* resp = rpc_get_response(1, "IO_OPEN ", strlen(full_path) + 2, 
        (char*) msg);

    //Da der Pfad vorhin allokiert wurde, muss er hier wieder freigegeben
    //weren.
    free(full_path);


    free(msg);
    
    FILE* io_res = (io_resource_t*) resp->data;
    //Wenn ein Fehler beim oeffnen aufgetreten ist, wird NULL zurueck gegeben.
    if ((io_res == NULL) || (io_res->pid == 0) || (resp->data_length == 0)) {
        io_res = NULL;
    } else {
        io_res->buffer_mode = IO_BUFFER_MODE_NONE;
        io_res->buffer_ptr = NULL;
        io_res->buffer_size = 0;
        io_res->buffer_pos = 0;
    }
    free(resp);
    
    return io_res;
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
    dword result;
    
    if((io_res == NULL) || (io_res->pid == 0))
    {
        return EOF;
    }

    result = rpc_get_dword(io_res->pid, "IO_CLOSE", sizeof(io_resource_id_t),
        (char*) &(io_res->id));
    free(io_res);

    return result;
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
    if ((io_res == NULL) || (io_res->pid == 0)) {
        return 0;
    }
    
    size_t read_bytes = 0;

    // Wenn Daten im ungetc-Buffer sind, werden die zuerst ausgelesen.
    if (io_res->ungetc_count != 0) {
        read_bytes = (io_res->ungetc_count <= blocksize * blockcount ? io_res->
            ungetc_count : blocksize * blockcount);
        
        int i;
        // Der Puffer muss in umgekehrter Reihenfolge ausgelesen werden
        for (i = 1; i <= read_bytes; i++) {
            *((byte*) ((uintptr_t) dest + i - 1)) = io_res->ungetc_buffer[
                io_res->ungetc_count - i];
        }

        // Buffer verkleinern
        io_res->ungetc_count -= read_bytes;
        byte* new_buffer = realloc(io_res->ungetc_buffer, io_res->
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
    
    io_read_request_t read_request;
    read_request.id = io_res->id;
    read_request.blocksize = blocksize;
    read_request.blockcount = blockcount;

    // Wenn mehr als 1 Kb gelesen werden soll, wird SHM benutzt
    if ((blocksize * blockcount) - read_bytes > 1024) {
        read_request.shared_mem_id = create_shared_memory(read_request.
            blocksize * read_request.blockcount);
        void* shm_ptr = open_shared_memory(read_request.shared_mem_id);

        response_t* resp = rpc_get_response(io_res->pid, "IO_READ ",
            sizeof(read_request), (char*) &(read_request));
        size_t size = *((size_t*) resp->data);
        
        // Wenn zuviele Daten gekommen sind, wird nur der notwendige Teil
        // kopiert.
        if (size + read_bytes > (blocksize * blockcount)) {
            size = (blocksize * blockcount) - read_bytes;
        }
        
        memcpy((void*) ((uintptr_t) dest + read_bytes), shm_ptr, size);

        close_shared_memory(read_request.shared_mem_id);
        free(resp->data); 
        free(resp);

        return size + read_bytes;
    } else {
        read_request.shared_mem_id = 0;
    
        // Der Buffer wird noch geleert, damit auch wirklich die Aktuellsten
        // Daten gelesen werden.
        fflush(io_res);

        response_t* resp = rpc_get_response(io_res->pid, "IO_READ ", 
            sizeof(read_request), (char*) &(read_request));
        size_t size = resp->data_length;
    
        // Wenn zuviele Daten gekommen sind, wird nur der notwendige Teil
        // kopiert.
        if (size + read_bytes > (blocksize * blockcount)) {
            size = (blocksize * blockcount) - read_bytes;
        }
        memcpy((void*) ((uintptr_t) dest + read_bytes), resp->data, size);
        
        
        free(resp->data);
        free(resp);
    
        return size + read_bytes;
    }
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
    char c;

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
        } else if (c == EOF) {
            continue;
        } else if (c == '\n') {
            break;
        }
        
        *dest = (char) c;
        dest++;
    }
    
    if (i == 0) {
        return NULL;
    } else {
        *dest = '\0';
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
    byte* new_buffer = realloc(io_res->ungetc_buffer, io_res->ungetc_count +
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
 *
 */
static size_t io_write(const void* src, size_t blocksize, size_t blockcount,
    FILE* io_res)
{   
    void* data;
    io_write_request_t* write_request;
    // Bei mehr als 1 Kb kommt Shared Memory zum Einsatz
    if (blocksize * blockcount > 1024) {
        write_request = (io_write_request_t*) malloc(
            sizeof(io_write_request_t));

        write_request->shared_mem_id = create_shared_memory(blocksize *
            blockcount);
        data = open_shared_memory(write_request->shared_mem_id);
    } else {
        write_request = (io_write_request_t*) malloc(
            blocksize * blockcount + sizeof(io_write_request_t));
        data = write_request->data;
        write_request->shared_mem_id = 0;
    }

    write_request->id = io_res->id;
    write_request->blocksize = blocksize;
    write_request->blockcount = blockcount;
    
    memcpy(data, src, blocksize * blockcount);
    size_t resp;
    if (write_request->shared_mem_id == 0) {
        resp = rpc_get_dword(io_res->pid, "IO_WRITE", blocksize 
            * blockcount + sizeof(io_write_request_t), (char*) write_request);
    } else {
        resp = rpc_get_dword(io_res->pid, "IO_WRITE", sizeof(
            io_write_request_t), (char*) write_request); 
        close_shared_memory(write_request->shared_mem_id);
    }
    free(write_request);

    return resp;
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
	
    if ((io_res == NULL) || (io_res->pid == 0)) {
        return 0;
    }

    // Wenn Daten im ungetc-Buffer sind, wird dieser geleert
    if (io_res->ungetc_count != 0) {
        io_res->ungetc_count = 0;
        free(io_res->ungetc_buffer);
    }

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
			return io_write(data, blocksize, blockcount, io_res);
			break;
		
		case IO_BUFFER_MODE_FULL:
			//Ueberpruefen, ob genug Platz im Buffer ist, dann werden die Daten
			// in den Buffer kopiert, sonst wird der Buffer geleert, und die
			// Daten werden direkt geschrieben.
			if (io_res->buffer_size < (io_res->buffer_pos + data_size))
			{
				//Daten in den Buffer kopieren.
				memcpy(io_res->buffer_ptr, data, data_size);
				io_res->buffer_pos += data_size;
                return data_size;
			} else {
				if (fflush(io_res) == EOF) {
                    return 0;
                }
				return io_write(data, blocksize, blockcount, io_res);
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
				return io_write(data, blocksize, blockcount, io_res);
			} else {
				//Daten in den Buffer kopieren.
				memcpy((void*)((dword) io_res->buffer_ptr + io_res->buffer_pos), data, data_size);
				io_res->buffer_pos += data_size;
                return data_size;
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
    //Ungueltige Handles abfangen
    if ((io_res == NULL) || (io_res->pid == 0)) {
        return -1;
    }
    
    // ungetc-Buffer leeren
    if (io_res->ungetc_count != 0) {
        io_res->ungetc_count = 0;
        free(io_res->ungetc_buffer);
    }

    io_seek_request_t seek_request;
    seek_request.id = io_res->id;
    seek_request.offset = offset;
    seek_request.origin = origin;

    return rpc_get_dword(io_res->pid, "IO_SEEK ", sizeof(io_seek_request_t),
        (char*) &seek_request);
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
    //Ungueltige Handles abfangen
    if ((io_res == NULL) || (io_res->pid == 0)) {
        return EOF;
    }
        
    io_tell_request_t tell_request;
    long result;
    tell_request.id = io_res->id;
    
    //Sonst könnte die Angabe falsch sein
    fflush(io_res);

    //Da fuer qwords keine Funktion existiert um sie direkt uber RPC zu
    // empfangen, muss hier ein kleiner Umweg gemacht werden.
    response_t* resp = rpc_get_response(io_res->pid, "IO_TELL ",
        sizeof(io_tell_request_t), (char*) &(tell_request));
    memcpy(&result, resp->data, sizeof(long));

    //Antwort und Daten wieder freigeben
    free(resp->data);
    free(resp);
    
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
    if ((io_res == NULL) || (io_res->pid == 0)) {
        return EOF;
    }

    // Wenn Daten im ungetc-Buffer sind, ist der Cursor noch nicht am Ende.
    if (io_res->ungetc_count != 0) {
        return 0;
    }
    
    io_eof_request_t eof_request;
    eof_request.id = io_res->id;
    
    //Auch hier könnte mit Daten im Buffer ein falsches Ergebnis rauskommen
    fflush(io_res);

    return rpc_get_dword(io_res->pid, "IO_EOF  ", sizeof(io_eof_request_t),
        (char*) &eof_request);
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
    //FIXME
    return 0;
}


/**
 * Fehler-Flag loeschen
 *
 * @param io_res Dateihandle
 */
void clearerr(FILE* io_res)
{
    // TODO
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
int fflush(io_resource_t* io_res)
{
    //Ungueltige Handles abfangen
    if ((io_res == NULL) || (io_res->pid == 0)) {
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

    size_t size = io_write(io_res->buffer_ptr, 1, io_res->buffer_pos, io_res);
    
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
int fpurge(io_resource_t* io_res)
{
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
    // Wenn das Dateihandle nicht in Ordung ist, wird abgebrochen
    if (io_res == NULL) {
        return -1;
    }

	if (buffer == NULL) {
		io_res->buffer_ptr = NULL;
		io_res->buffer_mode = IO_BUFFER_MODE_NONE;
		return 0;
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
			io_res->buffer_mode = IO_BUFFER_MODE_NONE;
			break;
			
		//Volles zwischenspeichern
		case _IOFBF:
			io_res->buffer_ptr = buffer;
			io_res->buffer_pos = 0;
			io_res->buffer_size = size;
			io_res->buffer_mode = IO_BUFFER_MODE_FULL;
			break;
		
		//Zeilenweise zwischenspeichern
		case _IOLBF:
			io_res->buffer_ptr = buffer;
			io_res->buffer_pos = 0;
			io_res->buffer_size = size;
			io_res->buffer_mode = IO_BUFFER_MODE_LINE;
			break;
		
		default:
			return -1;
	}
	
	return 0;
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
    printf("[file.c] FIXME: remove(%s) aufgerufen, aber noch nicht implementiert\n", filename);
    return 0;
}


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

