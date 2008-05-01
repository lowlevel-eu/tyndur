/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ext2.h"

static void ext2_inode_cache_write(block_cache_t* cache,
    bc_element_t* element);


/**
 * Inode-Cache initialisieren
 */
int ext2_inode_init(ext2_root_handle_t* root_handle)
{
    root_handle->inode_cache = bc_create(4096, sizeof(ext2_inode_t),
        ext2_inode_cache_write);
    
    if (root_handle->block_cache == NULL) {
        return 0;
    }

    root_handle->inode_cache->data = (void*) root_handle;
    return 1;
}

/**
 * Inode-Cache synchronisieren
 */
int ext2_inode_sync(ext2_root_handle_t* root_handle)
{
    return !bc_sync(root_handle->inode_cache);
}

/**
 * Veraenderten Inode aus dem Cache schreiben.
 */
static void ext2_inode_cache_write(block_cache_t* cache, bc_element_t* element)
{
    ext2_root_handle_t* root_handle = (ext2_root_handle_t*) cache->data;
    ext2_ih_t ih = {
        .number = element->index,
        .data = element->data
    };

    ext2_inode_write(root_handle, &ih);
}

/**
 * Inode aus dem Cache holen
 *
 * @param inode Inode-Handle
 *
 * @return 1 bei Erfolg, 0 sonst
 */
int ext2_inode_cache_get(ext2_root_handle_t* root_handle, ext2_ih_t* inode)
{
    bc_element_t* element = bc_get(root_handle->inode_cache, inode->number);
    
    // Inode einlesen falls noetig
    if (!element && (bc_insert(root_handle->inode_cache, inode->
        number, NULL, 0, 0)) && (element = bc_get(root_handle->inode_cache,
        inode->number)))
    {
        inode->data = element->data;

        if (ext2_inode_read(root_handle, inode)) {
            print_error("Fehler beim Einlesen des Inodes");
            inode->data = NULL;
            return 0;
        }
    } else if (!element) {
        print_error("Keinen Freien Inode im Cache gefunden!");
        return 0;
    }

    (element->flags.locked)++;
    inode->data = element->data;
    return 1;
}

/**
 * Inode wieder zurueck in Cache legen
 *
 * @param inode Inode-Handle
 *
 * @return 1 bei Erfolg, 0 sonst
 */
int ext2_inode_cache_put(ext2_root_handle_t* root_handle, ext2_ih_t* inode)
{
    bc_element_t* element = bc_get(root_handle->inode_cache, inode->number);
    
    if (!element || !element->flags.locked) {
        print_error("Versuch Inode in den Cache zu legen, der nicht drin liegt"
            " oder nicht gesperrt ist.");
        return 0;
    }
    
    if (inode->modified) {
        element->flags.dirty = 1;
    }
    (element->flags.locked)--;
    return 1;
}

