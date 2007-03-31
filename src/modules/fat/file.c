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
#include "dirent.h"
#include "lostio.h"
#include "fat.h"

/**
 * Erstellen einer neuen Datei
 */
void fat_create_file(fat_res_info_t* res_info, char* path)
{
    char* name = vfstree_basename(path);
    char* short_filename = fat_convert_to_short_filename(name);

    fat_directory_entry_t* dir_ptr = fat_get_free_directory_entry(res_info);
    
    
    memcpy(dir_ptr->name, short_filename, 11);
    dir_ptr->attributes = 0;
    dir_ptr->size = 0;
    //dword cluster = fat_allocate_cluster();
    //dir_ptr[i].first_cluster_lo = cluster & 0xff;
    //dir_ptr[i].first_cluster_hi = (cluster >> 16) & 0xff;
    //fat_set_fat_entry(cluster, 0xfff);

    //Verzeichnis aktualisieren
    char* dirname = vfstree_dirname(path);
    fat_update_directory(res_info, dirname);
    free(dirname);

    free(short_filename);
    
    //Verzeichnis auf die Diskette schreiben
    fat_save_directory(res_info);

    //FATs auf die Diskette speichern.
    fat_save_fat();
}


/**
 * fread() auf einem FAT-Handle
 */
read_hdl_reply fat_file_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount)
{
    //puts("read");
    fat_res_info_t* res_info = filehandle->node->data;
    read_hdl_reply reply;
    qword size = blocksize * blockcount;

    //printf("data:%08x    %08x    %d\n", (dword) filehandle->node->data, res_info, res_info->size);
    if(res_info->loaded == FALSE)
    {
        fat_load_resource(res_info);
        //puts("not loaded!");
    }
    //printf("data:%08x    %08x    %d\n", (dword) filehandle->node->data, res_info, res_info->size);
    
    if((filehandle->pos + size) > res_info->size)
    {
        size = res_info->size - filehandle->pos;
    }

    //printf("cl:%d, fs:%d, rs:%d  s:%d\n", res_info->cluster, filehandle->node->size, res_info->size, size);
    
    //Auf EOF ueberpruefen
    if(filehandle->pos == res_info->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    else
    {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }

    reply.size = size;
    reply.data = (void*) ((dword)(res_info->data) + (dword)filehandle->pos);

    filehandle->pos += size;
    return reply;
}


/**
 * Seek auf einem FAT-File
 */
int fat_file_close_handler(lostio_filehandle_t* filehandle)
{
    /*fat_res_info_t* res_info = filehandle->node->data;
    res_info->loaded = FALSE;
    free(res_info->data);*/
    printf("close '%s'\n", filehandle->node->name);
    return 0;
}


/**
 * fwrite() auf ein FAT-Handle
 */
size_t fat_file_write_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data)
{
    puts("write");
    size_t size = blocksize * blockcount;
    fat_res_info_t* res_info = filehandle->node->data;

    if(size > (filehandle->node->size - filehandle->pos))
    {
        puts("vergr");
        res_info->size = filehandle->node->size + (size - (filehandle->node->size - filehandle->pos));
        filehandle->node->size = res_info->size;
        printf("%08x   %d    %d   %d\n", (dword) res_info->data, size, res_info->size, filehandle->pos);
        res_info->data = realloc(res_info->data, res_info->size);
        printf("%08x   %d\n", (dword) res_info->data, size);
    
        
        //Das Verzeichnis aktualisieren
        vfstree_node_t* parent_node = (vfstree_node_t*) (filehandle->node->parent);
        fat_res_info_t* parent_res_info = (fat_res_info_t*) (parent_node->data);
        fat_directory_entry_t* dir_ptr = parent_res_info->data;
        dword i;
        for(i = 0; i < (parent_res_info->size / sizeof(fat_directory_entry_t)); i++)
        {
            dword cluster = ((dir_ptr[i].first_cluster_lo) | (dir_ptr[i].first_cluster_hi << 16));

            if((cluster == res_info->cluster) && (dir_ptr[i].name[0] != (char) 0xe5) && (dir_ptr[i].name[0] != (char) 0x0))
            {
                dir_ptr[i].name[0] = 'E';
                puts(dir_ptr[i].name);
                dir_ptr[i].size = res_info->size;
                
                fat_save_clusters(parent_res_info->cluster, parent_res_info->data, parent_res_info->size);
                break;
            }
        }
    }

    memcpy((void*) ((dword) (res_info->data) + filehandle->pos), data, size);
    
    fat_save_clusters(res_info->cluster, res_info->data, res_info->size);
    return size;
}
