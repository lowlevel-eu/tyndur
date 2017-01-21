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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <lostio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <lost/config.h>
#include <tms.h>

void pipe_display_usage(void);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_pipe(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    io_resource_t* f;
    int mode;
    char outbuf[1024];
    int outbuf_pos = 0;

    char* path = NULL;
    int parse_options = 1;
    bool no_eof = false;
    bool append_file = false;
    int i;

    for (i = 1; i < argc; i++) {
        if (parse_options && argv[i][0] == '-') {

            if (!strcmp(argv[i], "-c")) {
                no_eof = true;
            } else if (!strcmp(argv[i], "-a")) {
                no_eof = true;
                append_file = true;
            } else if (!strcmp(argv[i], "--")) {
                parse_options = 0;
            } else {
                pipe_display_usage();
                return -1;
            }

        } else {
            if (path == NULL) {
                path = argv[i];
            } else {
                pipe_display_usage();
                return -1;
            }
        }
    }

    if (path == NULL) {
        pipe_display_usage();
        return -1;
    }

    mode = IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE;
    if (no_eof) {
        mode |= IO_OPEN_MODE_CREATE;
        mode |= append_file ? IO_OPEN_MODE_APPEND :IO_OPEN_MODE_TRUNC;
    }

    f = lio_compat_open(path, mode);
    if (f == NULL) {
        printf(TMS(pipe_opening_error, "Konnte Datei nicht öffnen!\n"));
        return EXIT_FAILURE;
    }

    while (true) {
        char buf[1025];
        char c;
        ssize_t length;

        length = lio_compat_read_nonblock(buf, 1024, f);
        if (length == 0 && !no_eof) {
            break;
        } else if (length < 0 && length != -EAGAIN) {
            fprintf(stderr, "Lesefehler: %s\n", strerror(-length));
            goto out;
        }

        if (length > 0) {
            buf[length] = '\0';
            printf("%s", buf);
            fflush(stdout);
        }

        if (lio_compat_readahead(&c, 1, stdin->res) > 0) {
            if (fread(&c, 1, 1, stdin) && (c != 0)) {
                if (c == '\b') {
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

                if (c == '\n' || outbuf_pos == sizeof(outbuf)) {
                    if (!strncmp(outbuf, "EOF.\n", outbuf_pos)) {
                        break;
                    } else {
                        lio_compat_write(outbuf, 1, outbuf_pos, f);
                        outbuf_pos = 0;
                    }
                }
            }
        }
    }

out:
    lio_compat_close(f);
    printf(TMS(pipe_close, "\nVerbindung beendet.\n"));

    return EXIT_SUCCESS;
}

void pipe_display_usage()
{
    printf(TMS(pipe_usage, "\nAufruf: pipe [-c|-a] <Dateiname>\n"));
}
