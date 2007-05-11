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


bool fat_res_not_found_handler(char** path, byte args, pid_t pid, 
    io_resource_t* pipe_source)
{
    if (pipe_source == NULL) {
        return FALSE;
    }
    
    char* new_path;
    char* root_path;
    bool result;
    lostio_filehandle_t* filehandle;
    
    //current_pipe_source = pipe_source;
    
    if ((**path == '/') && (strlen(*path) == 1)) {
        (*path)++;
    }

    //FIXME: dword-Cast entferen, sobald asprintf keine Probleme mehr macht
    if (
        (asprintf(&root_path, "/%d/%d/root",  pipe_source->pid, 
            (dword)pipe_source->resid) == -1) ||
        (asprintf(&new_path, "/%d/%d/root%s",  pipe_source->pid, 
            (dword)pipe_source->resid, *path) == -1))
    {
        return FALSE; 
    }
    
    //Ueberpruefen, ob diese Quelle schon benutzt wurde
    result = TRUE;
    if (vfstree_get_node_by_path(root_path) == NULL) {
        filehandle = fat_mount_source(path, args, pid, pipe_source);
        if (filehandle == NULL) {
            puts("null");
            result = FALSE;
        } else {
            vfstree_node_t* node = vfstree_get_node_by_path(root_path);
            if ((node == NULL)) {
                result = FALSE;
            }
        }
    } else {
        char* resid_dir;
        filehandle = malloc(sizeof(lostio_filehandle_t));
        asprintf(&resid_dir, "/%d/%d",  pipe_source->pid, 
            (dword)pipe_source->resid);

        vfstree_node_t* root_node = vfstree_get_node_by_path(resid_dir);
        free(resid_dir);
        
        filehandle->data = root_node->data;
        filehandle->source = pipe_source;
    }
    
    if ((result == TRUE) && (vfstree_get_node_by_path(new_path) != NULL)) {
        result = TRUE;
        *path = new_path;
    } else if ((result == TRUE) && 
        (vfstree_get_node_by_path(root_path) != NULL))
    {
        char* parent_path = vfstree_dirname(new_path);
        *path = new_path;
        result = TRUE;
        
        vfstree_node_t* node = vfstree_get_node_by_path(new_path);
        vfstree_node_t* parent_node = vfstree_get_node_by_path(parent_path);
        vfstree_node_t* work_node;

        //Wenn der Knoten nicht gefunden wurde und das Elternverzeichnis nicht
        // geladen ist
        if ((node == NULL) && (((parent_node != NULL) && 
            (((fat_res_info_t*)(parent_node->data))->loaded == FALSE)) || 
               (parent_node == NULL)))
        {
            //Zuerst eine Kopie des Pfades erstellen, da er veaendert wird
            size_t work_path_len = strlen(new_path);
            size_t root_path_len = strlen(root_path);
            char* work_path = strclone(new_path);
            char* prev_part = strclone(root_path);
            int i;
            for (i = root_path_len + 1; i <= work_path_len; i++) {
                if ((work_path[i] == '/') || (work_path[i] == (char) 0)) {
                    work_path[i] = (char) 0;
                    
                    work_node = vfstree_get_node_by_path(work_path);
                    //Wenn dieser Pfadteil nicht praesent ist, wird versucht 
                    // zu laden
                    if ((work_node == NULL) || 
                        (((fat_res_info_t*)work_node->data)->loaded == FALSE))
                    {
                        vfstree_node_t* previous_handle = 
                            vfstree_get_node_by_path(prev_part);

                        fat_res_info_t* res_info = previous_handle->data;
                        

                        fat_load_directory(filehandle, res_info, prev_part);
                        
                        if (vfstree_get_node_by_path(work_path) == NULL) {
                            break;
                        }
                    }
                    
                    free(prev_part);
                    //Diesen Pfadteil abspeichern
                    prev_part = strclone(work_path);

                    work_path[i] = '/';
                }
            }
            free(work_path);
            free(prev_part);
        }
        
        node = vfstree_get_node_by_path(new_path);
        parent_node = vfstree_get_node_by_path(parent_path);
        //Wenn das Elternverzeichnis geladen ist, aber der Knoten nicht 
        // gefunden
        //werden konnte, wird ueberprueft, ob die Datei erstellt werden soll
        if ((node == NULL) && (parent_node != NULL) && ((args & 
                IO_OPEN_MODE_CREATE) != 0))
        {
            if (((parent_node->flags & LOSTIO_FLAG_BROWSABLE)!= 0) && 
                    ((args & IO_OPEN_MODE_DIRECTORY) == 0))
            {
                fat_create_file(filehandle, parent_node, new_path);
            } else if (((parent_node->flags & LOSTIO_FLAG_BROWSABLE)!= 0) &&
                    ((args & IO_OPEN_MODE_DIRECTORY) != 0))
            {
                fat_create_directory(filehandle, parent_node, new_path);
            }
        }
        free(parent_path);

    } else {
        result = FALSE;
    }
    free(filehandle);
    free(root_path);

    return result;
}
    

/**
 * Wird bei jedem OPEN von einer Datei/Verzeichnis hier aufgerufen.
 */
bool fat_res_pre_open_handler(char** path, byte args, pid_t pid, 
    io_resource_t* pipe_source)
{
    bool result = TRUE;
    lostio_filehandle_t* filehandle;
    
    if ((**path == '/') && (strlen(*path) == 1)) {
        if ((fat_res_not_found_handler(path, args, pid, pipe_source) == 
            FALSE)) 
        {
            return FALSE;
        }
    }

    filehandle = malloc(sizeof(lostio_filehandle_t));
    char* root_dir;
    asprintf(&root_dir, "/%d/%d", pipe_source->pid, pipe_source->resid);
    vfstree_node_t* root_node = vfstree_get_node_by_path(root_dir);
    free(root_dir);
    filehandle->data = root_node->data;
    filehandle->source = pipe_source;
    
    
    //Uberpruefen, ob der Knoten im VFS ist
    vfstree_node_t* node = vfstree_get_node_by_path(*path);
    if (node != NULL) {
        fat_res_info_t* res_info = (fat_res_info_t*) node->data;
           
        if (((args & IO_OPEN_MODE_DIRECTORY) != 0) && 
            ((node->flags & LOSTIO_FLAG_BROWSABLE) == 0))
        {
            result = FALSE;
        } else {
            if (res_info->loaded != TRUE) {
                if (node->type == FAT_LOSTIO_TYPE_DIR) {
                    fat_load_directory(filehandle, res_info, *path);
                } else {
                    fat_load_resource(filehandle, res_info);
                }
            }
            result = TRUE;
        }
    } else {
        result = FALSE;
    }

    free(filehandle);
    return result;
}

void fat_res_post_open_handler(lostio_filehandle_t* filehandle)
{
    char* root_dir;
    asprintf(&root_dir, "/%d/%d", filehandle->source->pid, 
        filehandle->source->resid);

    vfstree_node_t* root_node = vfstree_get_node_by_path(root_dir);
    free(root_dir);
    filehandle->data = root_node->data;
    
    if ((filehandle->flags & IO_OPEN_MODE_APPEND) != 0) {
        filehandle->pos = filehandle->node->size;
    }

    //Auf EOF ueberpruefen
    if (filehandle->pos == filehandle->node->size) {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    } else {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }
}

/**
 * Einbinden einer neuen Quelle
 */
lostio_filehandle_t* fat_mount_source(char** path, byte args, pid_t pid, io_resource_t* pipe_source)
{
    char* pid_dir = NULL;
    char* resid_dir = NULL;
    char* root_dir = NULL;
    char* bpb_path = NULL;
    char* new_path = NULL;
    lostio_filehandle_t* filehandle;
    
    if(
        (pipe_source == NULL) ||
        (asprintf(&pid_dir, "/%d", pipe_source->pid) == -1) ||
        (asprintf(&resid_dir, "/%d/%d", pipe_source->pid, pipe_source->resid) == -1) ||
        (asprintf(&root_dir, "/%d/%d/root", pipe_source->pid, pipe_source->resid) == -1)||
        (asprintf(&bpb_path, "/%d/%d/root/bpb", pipe_source->pid, pipe_source->resid) == -1) ||
        //FIXME: dword-Cast entferen, sobald asprintf keine Probleme mehr macht
        (asprintf(&new_path, "/%d/%d/root%s", pipe_source->pid, (dword)pipe_source->resid, *path) == -1)
    )
    {
        return NULL;
    }
    


    if(vfstree_get_node_by_path(root_dir) == NULL)
    {
        
        if(vfstree_get_node_by_path(resid_dir) == NULL)
        {
            if(vfstree_get_node_by_path(pid_dir) == NULL)
            {
                vfstree_create_node(pid_dir, LOSTIO_TYPES_DIRECTORY, 0, NULL, LOSTIO_FLAG_BROWSABLE);
            }

            vfstree_create_node(resid_dir, LOSTIO_TYPES_DIRECTORY, 0, NULL, LOSTIO_FLAG_BROWSABLE);
        }
        fat_res_info_t* dir_info = malloc(sizeof(fat_res_info_t));
        vfstree_create_node(root_dir, LOSTIO_TYPES_DIRECTORY, 0, dir_info, LOSTIO_FLAG_BROWSABLE);
        
        filehandle = malloc(sizeof(lostio_filehandle_t));
        //Rootdir-Handle Bereit machen
        fat_rootdir_handle_t* rootdir_handle = malloc(sizeof(fat_rootdir_handle_t));
        vfstree_get_node_by_path(resid_dir)->data = rootdir_handle;
        filehandle->data = rootdir_handle;
        filehandle->source = pipe_source;
        
        //BIOS-Parameterblock einlesen
        fseek(pipe_source, 0, SEEK_SET);
        if (fread(&(rootdir_handle->bios_parameter_block), 1, sizeof(fat_bios_parameter_block_t), pipe_source) != sizeof(fat_bios_parameter_block_t)) {
            puts("[  FAT  ] Konnte den BIOS-Parameterblock nicht vollstaendig einlesen!");
            vfstree_delete_node(resid_dir);
            return NULL;
        }
        
        //FAT-Einlesen
        rootdir_handle->file_allocation_table = malloc(rootdir_handle->bios_parameter_block.fat_sector_count * rootdir_handle->bios_parameter_block.bytes_per_sector);
        fat_load_fat(filehandle);
        
        //void* rootdir_buffer;
        if(rootdir_handle->bios_parameter_block.root_dir_entry_count != 0)
        {
            //rootdir_buffer = malloc(rootdir_handle->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t));
            dir_info->size = rootdir_handle->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t);
            dir_info->data = NULL;
            dir_info->loaded = FALSE;
            dir_info->cluster = 1;
            
            fat_load_directory(filehandle, dir_info, root_dir);
            //Den Pfad anpassen
            *path = root_dir;
        }
        else
        {
            puts("FAT32 wird noch nicht unterstuetzt");
            return NULL;
        }
    }
    return filehandle;
}


/**
 * FAT-Resource von dem Datentraeger laden
 */
void fat_load_resource(lostio_filehandle_t* filehandle, fat_res_info_t* res_info)
{
    DEBUG_MSG("anfang");
    if(res_info->loaded != TRUE)
    {
        fat_load_clusters(filehandle, res_info->cluster, &(res_info->data));
        res_info->loaded = TRUE;
    }
    DEBUG_MSG("ende");
}


/**
 * FAT-Resource auf den Datentraeger speichern
 */
void fat_save_resource(lostio_filehandle_t* filehandle, fat_res_info_t* res_info)
{
    DEBUG_MSG("anfang");
    if ((res_info->loaded == TRUE) && (res_info->data != NULL)) {
        fat_save_clusters(filehandle, res_info->cluster, res_info->data, res_info->size);
    }
    DEBUG_MSG("ende");
}

