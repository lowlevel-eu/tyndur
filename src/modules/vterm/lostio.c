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
#include <syscall.h>
#include "vterm.h"
#include "lostio.h"
/// Handler fuer Lesen aus in
size_t terminal_read(lostio_filehandle_t* handle, void* buf,
    size_t blocksize, size_t blockcount);

/// Handler fuer schreiben in out
size_t terminal_write(lostio_filehandle_t* handle, size_t blocksize,
    size_t blockcount, void* data);

/// Handler fuer close auf in
static int lio_in_close(lostio_filehandle_t* fh);


/**
 * LostIO-Interface vorbereiten, ueber das die Prozesse mit vterm kommunizieren
 * werden.
 */
void init_lostio_interface()
{
    lostio_init();
    lostio_type_directory_use();

    // Typehandle fuer Ausgabekanaele
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = LOSTIO_TYPES_OUT;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = NULL;
    typehandle->write       = &terminal_write;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);

    // Typehandle fuer Eingabekanaele
    typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = LOSTIO_TYPES_IN;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = &terminal_read;
    typehandle->write       = NULL;
    typehandle->seek        = NULL;
    typehandle->close       = lio_in_close;
    typehandle->link        = NULL;
    typehandle->unlink      = NULL;
    lostio_register_typehandle(typehandle);
}

/**
 * LostIO-Handler um in den Ausgabekanal zu schreiben
 */
size_t terminal_write(lostio_filehandle_t* file, size_t blocksize,
    size_t blockcount, void* data)
{
    p();
    vterm_process_output((vterminal_t*) file->node->data, data, blocksize *
        blockcount);
    v();
    return blocksize * blockcount;
}

/**
 * LostIO-Handller um aus dem Eingabekanal zu lesen
 */
size_t terminal_read(lostio_filehandle_t* file, void* buf,
    size_t blocksize, size_t blockcount)
{
    p();
    size_t size = blocksize * blockcount;
    vterminal_t* vterm = (vterminal_t*) file->node->data;

    if (vterm->set_eof) {
        vterm->set_eof = FALSE;
        file->flags |= LOSTIO_FLAG_EOF;
        v();
        return 0;
    }

    if (vterm->in_buffer_size == 0) {
        v();
        return 0;
    }
    
    if (vterm->in_buffer_size < size) {
        size = vterm->in_buffer_size;
    }

    memcpy(buf, vterm->in_buffer, size);
    
    vterm->in_buffer_size -= size;
    // Die uebrigen Daten am ende des Buffers an den Anfang schieben
    if (vterm->in_buffer_size != 0) {
        memmove(vterm->in_buffer, vterm->in_buffer + size,
            vterm->in_buffer_size);
    } else {
        free(vterm->in_buffer);
        vterm->in_buffer = NULL;
    }

    v();
    return size;
}

/**
 * LostIO-Handler der einen allenfalls allozierten Buffer im Filehandle freigibt
 */
static int lio_in_close(lostio_filehandle_t* fh)
{
    if (fh->data) {
        free(fh->data);
    }

    return 0;
}
