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

io_resource_t* lio_compat_open(const char* filename, uint8_t attr)
{
    char* full_path = io_get_absolute_path(filename);
    io_resource_t* result;

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

int lio_compat_close(io_resource_t* io_res)
{
    uint32_t result;
    result = rpc_get_dword(io_res->pid, "IO_CLOSE",
        sizeof(io_resource_id_t), (char*) &(io_res->id));
    free(io_res);

    return result;
}

size_t lio_compat_read(void* dest, size_t blocksize, size_t blockcount,
    io_resource_t* io_res)
{
    io_read_request_t read_request = {
        .id         = io_res->id,
        .blocksize  = blocksize,
        .blockcount = blockcount,
    };

    // Wenn mehr als 256 Bytes gelesen werden sollen, wird SHM benutzt
    if ((blocksize * blockcount) > 256) {

        get_shm(blocksize * blockcount);
        read_request.shared_mem_id = shm_id;

        response_t* resp = rpc_get_response(io_res->pid, "IO_READ ",
            sizeof(read_request), (char*) &(read_request));
        size_t size = *((size_t*) resp->data);

        // Wenn zuviele Daten gekommen sind, wird nur der notwendige Teil
        // kopiert.
        if (size > blocksize * blockcount) {
            size = blocksize * blockcount;
        }

        memcpy(dest, shm_ptr, size);

        free(resp->data);
        free(resp);

        return size;
    } else {
        read_request.shared_mem_id = 0;

        response_t* resp = rpc_get_response(io_res->pid, "IO_READ ",
            sizeof(read_request), (char*) &(read_request));
        size_t size = resp->data_length;

        // Wenn zuviele Daten gekommen sind, wird nur der notwendige Teil
        // kopiert.
        if (size > blocksize * blockcount) {
            size = blocksize * blockcount;
        }
        memcpy(dest, resp->data, size);

        free(resp->data);
        free(resp);

        return size;
    }
}

size_t lio_compat_write(const void* src, size_t blocksize, size_t blockcount,
    io_resource_t* io_res)
{
    size_t data_size = blockcount * blocksize;
    size_t request_size = sizeof(io_write_request_t);

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
    return resp;
}

int lio_compat_eof(io_resource_t* io_res)
{
    io_eof_request_t eof_request;
    eof_request.id = io_res->id;

    return rpc_get_dword(io_res->pid, "IO_EOF  ", sizeof(io_eof_request_t),
        (char*) &eof_request);
}
