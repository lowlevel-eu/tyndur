/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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

#include <libgen.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>

/**
 * Trennt den Dateinamen aus einem Pfad heraus. Wenn path ein Nullpointer ist,
 * wird "." zurueckgegeben. Der Rueckgabestring befindet sich in einem
 * statischen Puffer.
 */
char* basename(char* path)
{
    static char result[256];
    char* ret;

    if (!path) {
        result[0] = '.';
        result[1] = '\0';
        return result;
    }

    ret = io_split_filename(path);
    strncpy(result, ret, 255);
    result[255] = '\0';
    free(ret);

    return result;
}

/**
 * Trennt den Verzeichnisnamen aus einem Pfad heraus. Wenn path ein Nullpointer
 * ist, wird "." zurueckgegeben. Der Rueckgabestring befindet sich in einem
 * statischen Puffer.
 */
char* dirname(char* path)
{
    static char result[256];
    char* ret;

    if (!path) {
        result[0] = '.';
        result[1] = '\0';
        return result;
    }

    ret = io_split_dirname(path);
    strncpy(result, ret, 255);
    result[255] = '\0';
    free(ret);

    return result;
}
