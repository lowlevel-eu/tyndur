/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "config.h"

void pipe_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_pipe(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc != 2) {
        pipe_display_usage();
        return -1;
    }
    
    char* path = argv[1];
    FILE* f = fopen(path, "r+");
    if (f != NULL) {
        char outbuf[1024];
        int outbuf_pos = 0;
        
        while (!feof(f)) { 
            char buf[1024];
            size_t length = fread(buf, 1023, 1, f);
            if (length > 0) {
                buf[length] = '\0';
                printf("%s", buf);
                fflush(stdout);
            }
            
            char c;
            if (fread(&c, 1, 1, stdin) && (c != 0)) {
                if (c == '\b') 
                {
                    if (outbuf_pos) {
                        outbuf_pos--;
                        printf("\033[1D \033[1D");
                        fflush(stdout);
                    }
                } else {
                    printf("%c", c);
                    fflush(stdout);
                    
                    outbuf[outbuf_pos++] = c;
                }
                
                if ((c == '\n') || (outbuf_pos == 1024)) {
                    if (!strncmp(outbuf, "EOF.\n", outbuf_pos)) {
                        break;
                    } else {
                        fwrite(outbuf, outbuf_pos, 1, f);
                        outbuf_pos = 0;
                    }
                }
            }
        }
        fclose(f);
        printf("\nVerbindung beendet.\n");
    } else {
        puts("Konnte Datei nicht oeffnen");
    }

    return EXIT_SUCCESS;
}

void pipe_display_usage()
{
    puts("\nAufruf: pipe <Dateiname>");
}

