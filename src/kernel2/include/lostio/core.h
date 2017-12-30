/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#ifndef _LOSTIO_CORE_H_
#define _LOSTIO_CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include <collections.h>
#include <syscall_structs.h>

struct lio_resource;
struct lio_tree;

struct lio_stream {
    union {
        struct {
            struct lio_resource*    res;
            uint64_t                pos;

        } simple;
        struct {
            struct lio_stream*      read;
            struct lio_stream*      write;
        } composite;
    };

    bool                    is_composite;
    int                     flags;
    bool                    eof;

    /* Der Userspace-Handle wird in usp_refcount separat verwaltet; wenn ein
     * Stream mehrfach an den Userspace herausgegeben wird, wird ref trotzdem
     * nur einmal erhöht. */
    int                     ref;
    int                     usp_refcount;
};

struct lio_usp_stream {
    struct lio_stream*      stream;

    /** Id des Streams fuer Userspace-Programme */
    lio_usp_stream_t        usp_id;

    /** Sortierung nach usp_id im Baum */
    struct tree_item        usp_item;

    /** PID des Prozesses, der den Stream dem aktuellen übergeben hat */
    pid_t                   origin;
};

struct lio_node {
    struct lio_resource*    res;

    // Fuer den Wurzelknoten eines Baums ist der Name der Pfad der Quelle und
    // hoert mit einem / auf (z.B. ata:/ata00p0|ext2:/)
    char*                   name;
};

/**
 * Initialisiert interne Datenstrukturen für LostIO
 */
void lio_init(void);

#endif
