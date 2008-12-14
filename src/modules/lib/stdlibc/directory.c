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
#include "types.h"
#include "stdio.h"
#include "rpc.h"
#include "stdlib.h"
#include "dir.h"

io_resource_t* directory_open(const char* dirname)
{
    return fopen(dirname, "rd");
}


int directory_close(io_resource_t* io_res)
{
    return fclose((FILE*)io_res);
}


io_direntry_t* directory_read(io_resource_t* io_res)
{
    if ((io_res == NULL) || (feof(io_res) != 0)) {
        return NULL;
    }
                                
    io_direntry_t* result = malloc(sizeof(io_direntry_t));

    fread(result, sizeof(io_direntry_t), 1, (FILE*)io_res);
    
    return result;
}


int directory_seek(io_resource_t* io_res, long int offset, int origin)
{
    return fseek((FILE*)io_res, offset, origin);
}

bool directory_create(const char* dirname)
{
    FILE* io_res = fopen(dirname, "wd");

    if(io_res == NULL)
    {
        return FALSE;
    }

    fclose(io_res);
    return TRUE;
}

bool is_directory(const char* dirname)
{
    io_resource_t* dir = directory_open(dirname);
    if (dir != NULL) {
        directory_close(dir);
        return TRUE;
    } else {
        return FALSE;
    }
}
