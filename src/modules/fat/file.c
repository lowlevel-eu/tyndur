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

#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dir.h"
#include "lostio.h"
#include "fat.h"

/**
 * Erstellen einer neuen Datei
 */
void fat_create_file(lostio_filehandle_t* filehandle, vfstree_node_t* node, char* path)
{
    fat_res_info_t* new_res_info = malloc(sizeof(fat_res_info_t));
    memset(new_res_info, 0, sizeof(fat_res_info_t));
    
    new_res_info->loaded = TRUE;

    vfstree_create_node(path, FAT_LOSTIO_TYPE_FILE, 0, (void*)new_res_info, 0);
}


/**
 * fread() auf einem FAT-Handle
 */
read_hdl_reply fat_file_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount)
{
    fat_res_info_t* res_info = filehandle->node->data;
    read_hdl_reply reply;
    qword size = blocksize * blockcount;

    //Wenn die Datei noch nicht geladen ist, dann ist es jetzt hoechste Zeit es
    // nachzuholen.
    if(res_info->loaded == FALSE)
    {
        puts("nicht geladen");
        fat_load_resource(filehandle, res_info);
    }
    

    if((filehandle->pos + size) > res_info->size)
    {
        size = res_info->size - filehandle->pos;
    }
        
    //Auf EOF ueberpruefen
    if(filehandle->pos == res_info->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    else
    {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }
    
    //Antwort zusammenbauen
    reply.size = size;
    reply.data = (void*) ((dword)(res_info->data) + (dword)filehandle->pos);
    
    //Position vorwaerts bewegen
    filehandle->pos += size;


    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    else
    {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }


    return reply;
}


/**
 * Seek auf einem FAT-File
 */
int fat_file_close_handler(lostio_filehandle_t* filehandle)
{
    fat_res_info_t* res_info = filehandle->node->data;
    
    if((filehandle->flags & IO_OPEN_MODE_WRITE) != 0)
    {
        vfstree_node_t* parent_node = (vfstree_node_t*) (filehandle->node->parent);
        fat_save_directory(filehandle, parent_node);
    }

    free(res_info->data);
    res_info->loaded = FALSE;
    return 0;
}


/**
 * fwrite() auf ein FAT-Handle
 */
size_t fat_file_write_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data)
{
    size_t size = blocksize * blockcount;
    fat_res_info_t* res_info = filehandle->node->data;
    
    if(res_info->cluster == 0)
    {
        res_info->cluster = fat_allocate_cluster(filehandle);
        fat_set_fat_entry(filehandle, res_info->cluster, 0xfff);
        fat_save_fat(filehandle);
    }

    if(size > (filehandle->node->size - filehandle->pos))
    {
        res_info->size = filehandle->node->size + (size - (filehandle->node->size - filehandle->pos));
        filehandle->node->size = res_info->size;
        // FIXME Fehlender Nullcheck nach realloc, Memleak im Fehlerfall
        res_info->data = realloc(res_info->data, res_info->size);
    }
    
    memcpy((void*) (res_info->data + filehandle->pos), data, size);    
    fat_save_clusters(filehandle, res_info->cluster, res_info->data, res_info->size);
    
    filehandle->pos += size;

    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    else
    {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }

    return size;
}
