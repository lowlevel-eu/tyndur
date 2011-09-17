/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#include <stdint.h>
#include <stdbool.h>
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dir.h"
#include "lostio.h"
#include "fat.h"


bool fat_res_not_found_handler(char** path, uint8_t args, pid_t pid, 
    FILE* pipe_source)
{
    if (pipe_source == NULL) {
        return false;
    }
    
    char* new_path;
    char* root_path;
    bool result;
    lostio_filehandle_t* filehandle;
    
    //current_pipe_source = pipe_source;
    
    if ((**path == '/') && (strlen(*path) == 1)) {
        (*path)++;
    }

    //FIXME: uint32_t-Cast entferen, sobald asprintf keine Probleme mehr macht
    if (
        (asprintf(&root_path, "/%d/%d/root",  pipe_source->res->pid, 
            (uint32_t)pipe_source->res->resid) == -1) ||
        (asprintf(&new_path, "/%d/%d/root%s",  pipe_source->res->pid, 
            (uint32_t)pipe_source->res->resid, *path) == -1))
    {
        return false; 
    }
    
    p();

    //Ueberpruefen, ob diese Quelle schon benutzt wurde
    result = true;
    if (vfstree_get_node_by_path(root_path) == NULL) {
        filehandle = fat_mount_source(path, args, pid, pipe_source);
        if (filehandle == NULL) {
            result = false;
        } else {
            vfstree_node_t* node = vfstree_get_node_by_path(root_path);
            if ((node == NULL)) {
                result = false;
            }
        }
    } else {
        char* resid_dir;
        filehandle = malloc(sizeof(lostio_filehandle_t));
        asprintf(&resid_dir, "/%d/%d",  pipe_source->res->pid, 
            (uint32_t)pipe_source->res->resid);

        vfstree_node_t* root_node = vfstree_get_node_by_path(resid_dir);
        free(resid_dir);
        
        filehandle->data = root_node->data;
        filehandle->source = pipe_source;
    }
    
    if ((result == true) && (vfstree_get_node_by_path(new_path) != NULL)) {
        result = true;
        *path = new_path;
    } else if ((result == true) && 
        (vfstree_get_node_by_path(root_path) != NULL))
    {
        char* parent_path = vfstree_dirname(new_path);
        *path = new_path;
        result = true;
        
        vfstree_node_t* node = vfstree_get_node_by_path(new_path);
        vfstree_node_t* parent_node = vfstree_get_node_by_path(parent_path);
        vfstree_node_t* work_node;

        //Wenn der Knoten nicht gefunden wurde und das Elternverzeichnis nicht
        // geladen ist
        if ((node == NULL) && (((parent_node != NULL) && 
            (((fat_res_info_t*)(parent_node->data))->loaded == false)) || 
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
                        (((fat_res_info_t*)work_node->data)->loaded == false))
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
            if ((args & IO_OPEN_MODE_LINK) != 0) {
                fat_create_symlink(filehandle, parent_node, new_path);
            } else if (((parent_node->flags & LOSTIO_FLAG_BROWSABLE)!= 0) && 
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
        result = false;
    }
    free(filehandle);
    free(root_path);
    v();
    return result;
}
    

/**
 * Wird bei jedem OPEN von einer Datei/Verzeichnis hier aufgerufen.
 */
bool fat_res_pre_open_handler(char** path, uint8_t args, pid_t pid, 
    FILE* pipe_source)
{
    bool result = true;
    lostio_filehandle_t* filehandle;
    p();
    if ((**path == '/') && (strlen(*path) == 1)) {
        if ((fat_res_not_found_handler(path, args, pid, pipe_source) == 
            false)) 
        {
            v();
            return false;
        }
    }

    filehandle = malloc(sizeof(lostio_filehandle_t));
    char* root_dir;
    asprintf(&root_dir, "/%d/%d", pipe_source->res->pid, pipe_source->res->resid);
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
            result = false;
        } else {
            if (res_info->loaded != true) {
                if (node->type == FAT_LOSTIO_TYPE_DIR) {
                    fat_load_directory(filehandle, res_info, *path);
                } else {
                    fat_load_resource(filehandle, res_info);
                }
            }
            result = true;
        }
    } else {
        result = false;
    }

    free(filehandle);
    v();
    return result;
}

void fat_res_post_open_handler(lostio_filehandle_t* filehandle)
{
    p();
    char* root_dir;
    asprintf(&root_dir, "/%d/%d", filehandle->source->res->pid, 
        filehandle->source->res->resid);

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
    v();
}

/**
 * Einbinden einer neuen Quelle
 */
lostio_filehandle_t* fat_mount_source(char** path, uint8_t args, pid_t pid, FILE* pipe_source)
{
    if (pipe_source == NULL) {
        return NULL;
    }
    char* pid_dir = NULL;
    char* resid_dir = NULL;
    char* root_dir = NULL;
    char* bpb_path = NULL;
    char* new_path = NULL;
    lostio_filehandle_t* filehandle;
    uint8_t sector[512];
    
    if(
        (pipe_source == NULL) ||
        (asprintf(&pid_dir, "/%d", pipe_source->res->pid) == -1) ||
        (asprintf(&resid_dir, "/%d/%d", pipe_source->res->pid, pipe_source->res->resid) == -1) ||
        (asprintf(&root_dir, "/%d/%d/root", pipe_source->res->pid, pipe_source->res->resid) == -1)||
        (asprintf(&bpb_path, "/%d/%d/root/bpb", pipe_source->res->pid, pipe_source->res->resid) == -1) ||
        //FIXME: uint32_t-Cast entferen, sobald asprintf keine Probleme mehr macht
        (asprintf(&new_path, "/%d/%d/root%s", pipe_source->res->pid, (uint32_t)pipe_source->res->resid, *path) == -1)
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
        if (fread(sector, 1, 512, pipe_source) != 512) {
            puts("[  FAT  ] Konnte den BIOS-Parameterblock nicht vollstaendig einlesen!");
            vfstree_delete_node(resid_dir);
            return NULL;
        }

        memcpy(&rootdir_handle->bios_parameter_block, sector,
            sizeof(fat_bios_parameter_block_t));

        // Sektoranzahl suchen. Wenn < 0x10000 dann liegt sie im
        // bios_parameter_block.total_sector_count sonst in
        // bios_parameter_block.total_sector_count_32
        if (rootdir_handle->bios_parameter_block.total_sector_count != 0) {
            rootdir_handle->total_sector_count = rootdir_handle->bios_parameter_block.total_sector_count;
        } else {
            rootdir_handle->total_sector_count = rootdir_handle->bios_parameter_block.total_sector_count_32;
        }
        //Ueberpruefen ob es sich uberhaupt um FAT handelt
        char fs_type[3];
        memcpy(fs_type, &sector[54], 3);

        if ((fs_type[0] != 'F') || (fs_type[1] != 'A') || (fs_type[2] != 'T'))
        {
            puts("[  FAT  ] Kein FAT-Dateisystem!");
            vfstree_delete_node(resid_dir);
            return NULL;
        }

        //FAT-Typ feststellen (Direkt aus der Spezifikation von M$
        uint32_t rootdir_sector_count = ((rootdir_handle->bios_parameter_block.root_dir_entry_count * 32) + 
            (rootdir_handle->bios_parameter_block.bytes_per_sector - 1)) / 
            rootdir_handle->bios_parameter_block.bytes_per_sector;

        uint32_t data_sector_count = rootdir_handle->total_sector_count -
            (rootdir_handle->bios_parameter_block.reserved_sector_count + 
            (rootdir_handle->bios_parameter_block.fat_sector_count * rootdir_handle->bios_parameter_block.fat_count) +
            rootdir_sector_count);

        uint32_t cluster_count = data_sector_count / rootdir_handle->bios_parameter_block.sectors_per_cluster;
       
       //Achtung: Diese Zahlen muessen so krumm sein, das sind keine Fehler!
        if (cluster_count < 4086) {
            rootdir_handle->fat_size = 12;
        } else if (cluster_count < 65525) {
            rootdir_handle->fat_size = 16;
        } else {
            rootdir_handle->fat_size = 32;
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
            dir_info->loaded = false;
            dir_info->cluster = 1;
            
            fat_load_directory(filehandle, dir_info, root_dir);
            //Den Pfad anpassen
            *path = root_dir;
        }
        else
        {
            printf("FAT32 wird noch nicht unterstuetzt (%s)\n", *path);
            vfstree_delete_node(resid_dir);
            return NULL;
        }
    }
    else
    {
        filehandle = malloc(sizeof(lostio_filehandle_t));
        filehandle->data = vfstree_get_node_by_path(root_dir)->data;
        filehandle->source = pipe_source;
    }

    return filehandle;
}


/**
 * FAT-Resource von dem Datentraeger laden
 */
void fat_load_resource(lostio_filehandle_t* filehandle, fat_res_info_t* res_info)
{
    DEBUG_MSG("anfang");
    if(res_info->loaded != true)
    {
        fat_load_clusters(filehandle, res_info->cluster, &(res_info->data));
        res_info->loaded = true;
    }
    DEBUG_MSG("ende");
}


/**
 * FAT-Resource auf den Datentraeger speichern
 */
void fat_save_resource(lostio_filehandle_t* filehandle, fat_res_info_t* res_info)
{
    DEBUG_MSG("anfang");
    if ((res_info->loaded == true) && (res_info->data != NULL)) {
        fat_save_clusters(filehandle, res_info->cluster, res_info->data, res_info->size);
    }
    DEBUG_MSG("ende");
}

