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
#include "types.h"
#include "stdio.h"
#include "rpc.h"
#include "stdlib.h"
#include "lostio.h"

FILE* fopen (const char* filename, const char* mode)
{
    //RPC-Daten zusammenstellen
    const char* msg = malloc(strlen(filename) + 2);
    byte* attr = (byte*) msg;
    
    //TODO: Attribute aus dem mode String parsen
    *attr = 0;
    int i;
    for(i = 0; i < strlen(mode); i++)
    {
        switch(mode[i])
        {
            case 'r':
            {
                *attr |= LOSTIO_MODE_READ;
                break;
            }

            case 'w':
            {
                *attr |= LOSTIO_MODE_WRITE;
                break;
            }

            case 'd':
            {
                *attr |= LOSTIO_MODE_BROWSE;
                break;
            }
        }
    }

    //Dateiname inkl Null-Byte kopieren
    memcpy((void*) ((dword)msg + 1), filename, strlen(filename) + 1);


    
    response_t* resp = rpc_get_response(1, "IO_OPEN ", strlen(filename) + 2, (char*) msg);
    free(msg);
    
    FILE* io_res = (io_resource_t*) resp->data;

    free(resp->data);
    free(resp);
    
    //Wenn ein Fehler beim oeffnen aufgetreten ist => return Null
    if(io_res->pid == 0)
    {
        io_res = NULL;
    }
    return io_res;
}



size_t fread(void* dest, size_t blocksize, size_t blockcount, FILE* io_res)
{
    if((io_res == NULL) || (io_res->pid == 0))
    {
        return 0;
    }
    
    io_read_request_t read_request;
    read_request.id = io_res->id;
    read_request.blocksize = blocksize;
    read_request.blockcount = blockcount;
    


    response_t* resp = rpc_get_response(io_res->pid, "IO_READ ", sizeof(read_request), (char*) &(read_request));
    
    memcpy(dest, resp->data, resp->data_length);
    
    free(resp);
    free(resp->data);

    return resp->data_length;
}



size_t fwrite(const void* src, size_t blocksize, size_t blockcount, FILE* io_res)
{   
    if((io_res == NULL) || (io_res->pid == 0))
    {
        return 0;
    }

    //byte* write_request_raw = malloc(blocksize * blockcount + sizeof(io_write_request_t);
    io_write_request_t* write_request = (io_write_request_t*) malloc(blocksize * blockcount + sizeof(io_write_request_t));

    write_request->id = io_res->id;
    write_request->blocksize = blocksize;
    write_request->blockcount = blockcount;
    
    memcpy((write_request->data), src, blocksize * blockcount);
    
    size_t resp = rpc_get_dword(io_res->pid, "IO_WRITE", blocksize * blockcount + sizeof(io_write_request_t), (char*) write_request);
    
    free(write_request);

    return resp;
}


int fseek (FILE* io_res, long int offset, int origin)
{
    if((io_res == NULL) || (io_res->pid == 0))
    {
        return -1;
    }

    io_seek_request_t seek_request;
    seek_request.id = io_res->id;
    seek_request.offset = offset;
    seek_request.origin = origin;

    return rpc_get_dword(io_res->pid, "IO_SEEK ", sizeof(io_seek_request_t), (char*) &seek_request);
}


int feof(FILE* io_res)
{
    if((io_res == NULL) || (io_res->pid == 0))
    {
        return EOF;
    }

    io_eof_request_t eof_request;
    eof_request.id = io_res->id;

    return rpc_get_dword(io_res->pid, "IO_EOF  ", sizeof(io_eof_request_t), (char*) &eof_request);
}


long ftell(FILE* io_res)
{
    if((io_res == NULL) || (io_res->pid == 0))
    {
        return EOF;
    }
    
    io_tell_request_t tell_request;
    tell_request.id = io_res->id;
    
    response_t* resp = rpc_get_response(io_res->pid, "IO_TELL ", sizeof(io_tell_request_t), (char*) &(tell_request));

    long result;
    memcpy(&result, resp->data, sizeof(long));

    free(resp);
    free(resp->data);

    return result;
}


int fclose (FILE* io_res)
{
    if((io_res == NULL) || (io_res->pid == 0))
    {
        return EOF;
    }

    return rpc_get_dword(io_res->pid, "IO_CLOSE", sizeof(io_resource_id_t), (char*) &(io_res->id));
}

