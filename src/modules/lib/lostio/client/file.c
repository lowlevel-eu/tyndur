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

#include <lostio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <rpc.h>
#include <syscall.h>
#include <errno.h>
#include <lock.h>

static void* shm_ptr;
static uint32_t shm_id;
static size_t shm_size;

static void* get_shm(size_t size)
{
    if (shm_size >= size) {
        return shm_ptr;
    }

    if (shm_id != 0) {
        close_shared_memory(shm_id);
    }

    shm_size = size;
    shm_id = create_shared_memory(size);
    shm_ptr = open_shared_memory(shm_id);

    return shm_ptr;
}

static io_resource_t* lio2_open(const char* filename, uint8_t attr)
{
    char* full_path;
    uint8_t lio2_attr = 0;
    bool m_append = false;
    bool m_create = false;

    lio_stream_t s;
    lio_resource_t r;
    io_resource_t* result = NULL;

    full_path = io_get_absolute_path(filename);
    if (!full_path) {
        return NULL;
    }

    /* Modus auswerten */
    if (attr & ~(IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE | IO_OPEN_MODE_CREATE
        | IO_OPEN_MODE_TRUNC | IO_OPEN_MODE_APPEND))
    {
        goto out_err;
    }

    if (attr & IO_OPEN_MODE_READ)   lio2_attr |= LIO_READ;
    if (attr & IO_OPEN_MODE_WRITE)  lio2_attr |= LIO_WRITE;
    if (attr & IO_OPEN_MODE_CREATE) m_create = true;
    if (attr & IO_OPEN_MODE_TRUNC)  lio2_attr  |= LIO_TRUNC;
    if (attr & IO_OPEN_MODE_APPEND) m_append = true;

    /* Datei erzeugen, falls nötig */
    r = lio_resource(full_path, 1);
    if ((r < 0) && m_create) {
        lio_resource_t parent;
        char* dirname = io_split_dirname(full_path);
        char* filename = io_split_filename(full_path);

        if ((parent = lio_resource(dirname, 1)) > 0) {
            r = lio_mkfile(parent, filename);
        }

        free(dirname);
        free(filename);
    }

    if (r < 0) {
        goto out_err;
    }

    /* Datei öffnen */
    s = lio_open(r, lio2_attr);
    if (s < 0) {
        goto out_err;
    }

    if (m_append) {
        lio_seek(s, 0, LIO_SEEK_END);
    }

    /* Datenstruktur anlegen */
    result = malloc(sizeof(*result));
    *result = (io_resource_t) {
        .lio2_stream    =   s,
        .lio2_res       =   r,
        .flags          =   IO_RES_FLAG_READAHEAD,
    };

out_err:
    free(full_path);
    return result;
}

io_resource_t* lio_compat_open(const char* filename, uint8_t attr)
{
    char* full_path;
    io_resource_t* result;

    result = lio2_open(filename, attr);
    if (result) {
        return result;
    }

    full_path = io_get_absolute_path(filename);
    if (!full_path) {
        return NULL;
    }

    // RPC-Daten zusammenstellen
    char msg[strlen(full_path) + 2];
    msg[0] = attr;
    memcpy(msg + 1, full_path, strlen(full_path) + 1);

    response_t* resp = rpc_get_response(1, "IO_OPEN ", strlen(full_path) + 2,
        (char*) msg);
    if (resp == NULL || resp->data_length == 0) {
        result = NULL;
        goto out;
    };

    result = resp->data;
    if (result->pid == 0) {
        result = NULL;
        goto out;
    }

out:
    free(full_path);
    free(resp);

    return result;
}

static int lio2_fclose(io_resource_t* stream)
{
    int result = 0;

    if (stream->lio2_stream > 0) {
        result = lio_close(stream->lio2_stream);
    }
    free(stream);

    return result;
}


int lio_compat_close(io_resource_t* io_res)
{
    uint32_t result;

    if (IS_LIO2(io_res)) {
        return lio2_fclose(io_res);
    }

    result = rpc_get_dword(io_res->pid, "IO_CLOSE",
        sizeof(io_resource_id_t), (char*) &(io_res->id));
    free(io_res);

    return result;
}

/**
 * Liest aus einer Ressource
 *
 * @return Anzahl gelesener Bytes; 0 bei Dateiende; negativ im Fehlerfall.
 */
static ssize_t lio1_read_fn(void* dest, size_t blocksize, size_t blockcount,
    io_resource_t* io_res, char* rpc_fn)
{
    size_t size;

    io_read_request_t read_request = {
        .id         = io_res->id,
        .blocksize  = blocksize,
        .blockcount = blockcount,
    };

    // Wenn mehr als 256 Bytes gelesen werden sollen, wird SHM benutzt
    if ((blocksize * blockcount) > 256) {
        // TODO Mehrere SHMs gleichzeitig zulassen
        static mutex_t shm_lock = 0;

        mutex_lock(&shm_lock);
        get_shm(blocksize * blockcount);
        read_request.shared_mem_id = shm_id;

        response_t* resp = rpc_get_response(io_res->pid, rpc_fn,
            sizeof(read_request), (char*) &(read_request));
        size = *((size_t*) resp->data);

        // Wenn zuviele Daten gekommen sind, wird nur der notwendige Teil
        // kopiert.
        if (size > blocksize * blockcount) {
            size = blocksize * blockcount;
        }

        memcpy(dest, shm_ptr, size);
        mutex_unlock(&shm_lock);

        free(resp->data);
        free(resp);
    } else {
        read_request.shared_mem_id = 0;

        response_t* resp = rpc_get_response(io_res->pid, rpc_fn,
            sizeof(read_request), (char*) &(read_request));
        size = resp->data_length;

        // Wenn zuviele Daten gekommen sind, wird nur der notwendige Teil
        // kopiert.
        if (size > blocksize * blockcount) {
            size = blocksize * blockcount;
        }
        memcpy(dest, resp->data, size);

        free(resp->data);
        free(resp);
    }

    // EOF ist Rückgabewert 0, Fehler sind negativ
    if (size == 0  && !lio_compat_eof(io_res)) {
        return -EAGAIN;
    }
    return size;
}

ssize_t lio_compat_read(void* dest, size_t blocksize, size_t blockcount,
    io_resource_t* io_res)
{
    if (IS_LIO2(io_res)) {
        return lio_read(io_res->lio2_stream, blocksize * blockcount, dest);
    }

    return lio1_read_fn(dest, blocksize, blockcount, io_res, "IO_READ ");
}

ssize_t lio_compat_read_nonblock(void* dest, size_t size,
                                 io_resource_t* io_res)
{
    if (IS_LIO2(io_res)) {
        return lio_readf(io_res->lio2_stream, 0, size, dest, LIO_REQ_FILEPOS);
    }

    return lio1_read_fn(dest, 1, size, io_res, "IO_READ ");
}

/**
 * Schreibt in eine Ressource
 *
 * @return Anzahl geschriebener Bytes; negativ im Fehlerfall.
 */
ssize_t lio_compat_write(const void* src, size_t blocksize, size_t blockcount,
    io_resource_t* io_res)
{
    size_t data_size = blockcount * blocksize;
    size_t request_size = sizeof(io_write_request_t);

    if (IS_LIO2(io_res)) {
        return lio_write(io_res->lio2_stream, blocksize * blockcount, src);
    }

    // Bei mehr als einem Kilobyte wird shared memory fuer die Daten benutzt
    if (data_size < 1024) {
        request_size += data_size;
    }
    uint8_t request[request_size];

    io_write_request_t* write_request = (io_write_request_t*) request;
    write_request->id = io_res->id;
    write_request->blocksize = blocksize;
    write_request->blockcount = blockcount;

    // Wenn kein SHM benutzt wird, werden die Daten direkt an den Request
    // angehaengt
    if (data_size < 1024) {
        write_request->shared_mem_id = 0;
        memcpy(write_request->data, src, data_size);
    } else {
        write_request->shared_mem_id = create_shared_memory(data_size);
        void* data = open_shared_memory(write_request->shared_mem_id);
        memcpy(data, src, data_size);
    }

    size_t resp = rpc_get_dword(io_res->pid, "IO_WRITE",
        request_size, (char*) request);

    if (data_size >= 1024) {
        close_shared_memory(write_request->shared_mem_id);
    }

    // Rückgabewert für Fehler ist -1
    if (resp == 0) {
        return -1;
    }
    return resp;
}

int lio_compat_eof(io_resource_t* io_res)
{
    io_eof_request_t eof_request;

    if (IS_LIO2(io_res)) {
        return 0;
    }

    eof_request.id = io_res->id;

    return rpc_get_dword(io_res->pid, "IO_EOF  ", sizeof(io_eof_request_t),
        (char*) &eof_request);
}

/**
 * Liest aus einer Ressource, ohne den Dateizeiger zu verändern und ohne zu
 * blockieren
 */
ssize_t lio_compat_readahead(void* dest, size_t size, io_resource_t* io_res)
{
    if (IS_LIO2(io_res)) {
        int64_t pos = lio_seek(io_res->lio2_stream, 0, LIO_SEEK_CUR);
        if (pos < 0) {
            return pos;
        }

        return lio_readf(io_res->lio2_stream, pos, size, dest, 0);
    } else {
        return lio1_read_fn(dest, 1, size, io_res, "IO_READA");
    }
}
