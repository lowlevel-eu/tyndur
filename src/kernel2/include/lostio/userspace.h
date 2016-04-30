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

#ifndef _LOSTIO_USERSPACE_H
#define _LOSTIO_USERSPACE_H

#include "lostio/core.h"



/***** Streams ******/

/** Weist einem Stream eine Userspace-ID zu */
struct lio_usp_stream* lio_usp_add_stream(pm_process_t* proc,
    struct lio_stream* stream);

/** Weist einem Stream eine gegebene Userspace-ID zu */
struct lio_usp_stream* lio_usp_do_add_stream(pm_process_t* proc,
    struct lio_stream* stream, lio_usp_stream_t id);

/**
 * Gibt den zur Userspace-ID gehörenden Stream zurück, oder NULL wenn es
 * keinen passenden Stream gibt.
 */
struct lio_usp_stream* lio_usp_get_stream(pm_process_t* proc,
    lio_usp_stream_t id);

/**
 * Gibt die Userspace-ID des Streams wieder frei und schließt den Stream, wenn
 * das die letzte Userspace-Referenz war.
 */
int lio_usp_unref_stream(pm_process_t* proc, struct lio_usp_stream* stream);



/***** Ressourcen ******/

/**
 * Gibt die zur Userspace-ID gehörende Ressource zurück, oder NULL wenn es
 * keine passende Ressource gibt.
 */
struct lio_resource* lio_usp_get_resource(lio_usp_resource_t id);

/**
 * Gitb die Userspace-ID zu einer gegebenen Ressource zurück.
 */
lio_usp_resource_t lio_usp_get_id(struct lio_resource* res);



/***** Verzeichnisbäume ******/

/**
 * Gibt den zur Userspace-ID gehörenden Verzeichnisbaum zurück, oder NULL
 * wenn es keinen passenden Verzeichnisbaum gibt.
 */
struct lio_tree* lio_usp_get_tree(lio_usp_tree_t id);

#endif
