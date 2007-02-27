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
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"

#define _USE_START_
#include "init.h"





void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_seek(pid_t pid, dword correlation_id, size_t data_size, void* data);


void putn(unsigned long long x, int radix, int pad, char padchar);
void service_register(const char* name);


char* testfile;
size_t testfile_position;
size_t testfile_size;

int main(int argc, char* argv[])
{
    puts("\n\033[1;33mRegistriere Server beim init Prozess...\033[0;37m");
    init_service_register("init_test");
   
    /*
    //Bevor die pid des Treibers geholt werden kann, muss der sich erst registrieren
    yield();
    puts("\n\033[1;33mHole die PID des Tastaturtreibers bei init...\033[0;37m");
    printf(" PID=%d\n", init_service_get("keyboard")); 

    puts("\n\033[1;33mVersuche die PID eines inexistenten Treibers bei init zu holen...\033[0;37m");
    printf("\nPID des Maustreibers erfragen... PID=%d\n", init_service_get("mouse"));
    */
    
    register_message_handler("IO_OPEN ", &rpc_io_open);
    register_message_handler("IO_CLOSE", &rpc_io_close);
    register_message_handler("IO_READ ", &rpc_io_read);
    register_message_handler("IO_WRITE", &rpc_io_write);
    register_message_handler("IO_SEEK ", &rpc_io_seek);

    while(1) yield();
    return 0;
}



void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_resource_t io_res;
    
    //Wird bei pipes als quelle verwendet
    //io_resource_t* io_source = (io_resource_t*) ((dword)data + 1);

    //Modus zu oeffnen
    //byte* attr = (byte*) data;
    //char* path = (char*) ((dword)data + 1 + sizeof(io_resource_t));

    io_res.pid = init_service_get("init_test");
    io_res.id = 1;

    testfile_size = 0x400;
    testfile = malloc(testfile_size);
    testfile_position = 0;
    
    rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
}


void rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    rpc_send_dword_response(pid, correlation_id, 0);
}


void rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_read_request_t* read_request = (io_read_request_t*) data;
    size_t size = read_request->blocksize * read_request->blockcount;
    
    if(size > (testfile_size - testfile_position))
    {
        size = testfile_size - testfile_position;
    }
    rpc_send_response(pid, correlation_id, size, (char*)((dword)testfile + testfile_position));
}


void rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_write_request_t* write_request = (io_write_request_t*) data;

    size_t size = write_request->blocksize * write_request->blockcount;
    if(size > (testfile_size - testfile_position))
    {
        size = testfile_size - testfile_position;
    }

    memcpy((void*)((dword)testfile + testfile_position), write_request->data, size);

    rpc_send_dword_response(pid, correlation_id, size);
}


void rpc_io_seek(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    int result = 0;
    io_seek_request_t* seek_request = (io_seek_request_t*) data;

    switch(seek_request->origin)
    {
        case SEEK_SET:
        {
            testfile_position = seek_request->offset;
            break;
        }

        case SEEK_CUR:
        {
            testfile_position += seek_request->offset;
            break;
        }
        
        case SEEK_END:
        {
            testfile_position = testfile_size + seek_request->offset;
            break;
        }
    }
    
    rpc_send_dword_response(pid, correlation_id, result);
}

