/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <rpc.h>

#include "file.h"

static void client_print_usage(void)
{
    puts("Aufruf: file mount file:/<Quelle> <Ziel>");
}

int client(int argc, char* argv[], pid_t file_service)
{
    if (argc < 2) {
        client_print_usage();
        return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "mount") && (argc == 4)) {
        size_t len_src = strlen(argv[2]) + 1;
        size_t len_dst = strlen(argv[3]) + 1;

        if (file_service) {
            size_t len = RPC_FUNCTION_NAME_LENGTH + len_src + len_dst;
            char data[len];

            memset(data, 0, RPC_FUNCTION_NAME_LENGTH);
            strncpy(data, "MOUNT", RPC_FUNCTION_NAME_LENGTH);
            strncpy(data + RPC_FUNCTION_NAME_LENGTH, argv[2], len_src);
            strncpy(data + RPC_FUNCTION_NAME_LENGTH + len_src, argv[3],
                len_dst);

            send_message(file_service, RPC_MESSAGE, 0, len, data);
        } else {
            lio_resource_t res = -2;
            lio_stream_t s = -2;

            if (strcmp(argv[2], "file:/") ||
                ((res = lio_resource("file:/", 0)) < 0) ||
                ((s = lio_open(res, LIO_SYMLINK | LIO_WRITE | LIO_TRUNC)) < 0))
            {
                printf("Err: (%s) %llx %llx\n", argv[2], res, s);
                return EXIT_FAILURE;
            }
            lio_write(s, len_dst - 1, argv[3]);
            lio_close(s);
        }

        return EXIT_SUCCESS;
    } else {
        client_print_usage();
        return EXIT_FAILURE;
    }
}
