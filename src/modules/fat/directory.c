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

/**
 * Verzeichniseintraege in den VFS-Baum einlesen
 *
 * @param dir_ptr Pointer auf das Verzeichnis
 * @param dir_entry_count Anzahl der Verzeichniseintraege
 * @param 
 */
void fat_parse_directory(lostio_filehandle_t* filehandle, fat_directory_entry_t* dir_ptr, size_t dir_entry_count, char* path)
{
    int i,j,k;
    char* path_begin = malloc(1024);
    char* filename_begin = path_begin + strlen(path) + 1;
    char* filename_end = filename_begin;
    size_t filename_len;
    bool long_filename = false;
    
    memcpy(path_begin, path, strlen(path));
    path_begin[strlen(path)] = '/';
    

    for(i = 0; i < dir_entry_count; i++)
    {
        //Keine weiteren Eintraege
        if(dir_ptr[i].name[0] == (char) 0)
        {
            break;
        }
        //Leerer Eintrag oder .-Eintrag
        else if((dir_ptr[i].name[0] == (char) 0xe5) ||
                    (dir_ptr[i].name[0] == ' ') ||
                    ((dir_ptr[i].name[0] == '.') && (dir_ptr[i].name[1] == ' ')) ||
                    ((dir_ptr[i].name[0] == '.') && (dir_ptr[i].name[1] == '.') && (dir_ptr[i].name[2] == ' '))
                )
        {
            continue;
        }
        //Teil eines langen Dateinamen
        else if((dir_ptr[i].attributes & FAT_ATTR_LONGFNAME) == FAT_ATTR_LONGFNAME)
        {
            #ifndef CONFIG_FAT_USE_LFN
            continue;
            #endif

            if(filename_len == 0)
            {
                filename_len = 0;

            }
            
            if(long_filename != true)
            {
                filename_end = filename_begin;
                long_filename = true;
            }
            

            fat_vfat_entry_t* vfat_entry = (fat_vfat_entry_t*) &(dir_ptr[i]);
            filename_len += 14;
            k = 0;
            for(j = 0; j < 10; j += 2)
            {
                filename_end[k++] = vfat_entry->name_part_1[j];
            }

            for(j = 0; j < 12; j += 2)
            {
                filename_end[k++] = vfat_entry->name_part_2[j];
            }

            for(j = 0; j < 4; j += 2)
            {
                filename_end[k++] = vfat_entry->name_part_3[j];
            }
            filename_end += 14;

            *filename_end = 0;
            continue;
        }
        else
        {
            if(long_filename == false)
            {
                char* converted_filename = fat_convert_from_short_filename(dir_ptr[i].name);
                memcpy(filename_end, converted_filename, strlen(converted_filename) + 1);
                
                free(converted_filename);
            }
            
            
            fat_res_info_t* res_info = malloc(sizeof(fat_res_info_t));
            //Die Resource ist noch nicht im Speicher
            res_info->loaded = false;
            //Groesse eintragen
            res_info->size = dir_ptr[i].size;
            //Startcluster eintragen
            res_info->cluster = ((dir_ptr[i].first_cluster_lo) | (dir_ptr[i].first_cluster_hi << 16));
            
            vfstree_node_t* node = vfstree_get_node_by_path(path_begin);

            if(node == NULL)
            {
                if ((dir_ptr[i].attributes & FAT_ATTR_DIRECTORY) == 0) {
                    vfstree_create_node(path_begin, FAT_LOSTIO_TYPE_FILE, res_info->size, (void*)res_info, 0);
                } else if((dir_ptr[i].attributes & FAT_ATTR_LOST_SYMLINK) == 
                    FAT_ATTR_LOST_SYMLINK)
                {
                    vfstree_create_node(path_begin, FAT_LOSTIO_TYPE_FILE, res_info->size, (void*)res_info, LOSTIO_FLAG_SYMLINK);
                } else {   
                    vfstree_create_node(path_begin, FAT_LOSTIO_TYPE_DIR, 0, (void*)res_info, LOSTIO_FLAG_BROWSABLE);
                }
            }
            else
            {
                //Bei einer Datei wird noch die Groesse neu gesetzt
                if((dir_ptr[i].attributes & FAT_ATTR_DIRECTORY) == 0)
                {
                    node->size = res_info->size;
                }
            }

            filename_len = 0;
            filename_end = filename_begin;
            long_filename = false;
        }
    }
    
    free(path_begin);
}


void fat_load_directory(lostio_filehandle_t* filehandle, fat_res_info_t* res_info, char* path)
{
    if(res_info->loaded != true)
    {
        uint32_t clusters = fat_load_clusters(filehandle, res_info->cluster, &(res_info->data));
        
        res_info->size = clusters * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster;
        fat_parse_directory(filehandle, (fat_directory_entry_t*) (res_info->data), res_info->size / sizeof(fat_directory_entry_t), path);

        res_info->loaded = true;
    }
}


void fat_save_directory(lostio_filehandle_t* filehandle, vfstree_node_t* dir)
{
    DEBUG_MSG("anfang");

    fat_res_info_t* res_info = (fat_res_info_t*) dir->data;
    fat_res_info_t* node_res_info;
    fat_directory_entry_t* dir_entry;

    //Die einzelnen Eintraege durchgehen, und aktualisieren
    vfstree_node_t* node;
    uint32_t i = 0;
    while((node = list_get_element_at(dir->children, i)))
    {
        node_res_info = (fat_res_info_t*) node->data;
        dir_entry = fat_get_directory_entry(filehandle, res_info, node->name);
        
        //Der eintrag ist noch nicht vorhanden
        if(dir_entry == NULL)
        {
            dir_entry = fat_get_free_directory_entry(filehandle, res_info);

            //Namen setzen
            char* short_name = fat_convert_to_short_filename(node->name);
            memcpy(dir_entry->name, short_name, 11);
            free(short_name);
        }
               
        dir_entry->first_cluster_lo = node_res_info->cluster & 0xffff;
        dir_entry->first_cluster_hi = (node_res_info->cluster >> 16) & 0xffff;
        dir_entry->size = node_res_info->size;

        if(node->type == FAT_LOSTIO_TYPE_DIR)
        {
            dir_entry->attributes = FAT_ATTR_DIRECTORY;
            dir_entry->size = 0;
        }
        
        //Wenn es ein Symlink ist, muss dieser jetzt enstprehcend gesetzt
        //werden.
        if ((node->flags & LOSTIO_FLAG_SYMLINK) != 0) {
            dir_entry->attributes = FAT_ATTR_LOST_SYMLINK;
        }
        i++;
    }
/*    
    //Wenn kein Dot-Eintrag existiert, wird der angelegt
    if((fat_get_directory_entry(res_info, ".") == NULL) && (res_info->cluster != 1))
    {
        dir_entry = fat_get_free_directory_entry(res_info)
    }

    //Wenn kein DotDot-Eintrag existiert, wird der noch angelegt
    if((fat_get_directory_entry(res_info, "..") == NULL) && (res_info->cluster != 1)
        dir_entry = fat_get_free_directory_entry(res_info);
    }
*/  
    fat_save_resource(filehandle, res_info);
    DEBUG_MSG("ende");
}


/**
 * Einen rohen kurzen Dateinamen in einen "normalen" umwandeln.
 *
 * @param short_filename Pointer
 *
 * @return Pointer auf den neuen Dateinamen
 */
char* fat_convert_from_short_filename(char* short_filename)
{
    uint32_t i;
    char* filename = malloc(13);

    //Ende des Dateinamen suchen
    for(i = 7; i >= 0; i--)
    {
        if(short_filename[i] != ' ')
            break;
    }
    memcpy(filename, short_filename, i+1);
    //Wenn eine Endung vorhanden ist, wird sie kopiert
    if(short_filename[8] != ' ')
    {
        filename[i+1] = '.';
        memcpy(&(filename[i+2]), &(short_filename[8]), 3);
        filename[i+5] = 0;
    }
    else
    {
        filename[i+1] = 0;
    }
    
    //Das Ganze noch in Kleinbuchstaben umwandeln
    i = 0;
    while(filename[i] != 0)
    {
        if((filename[i] >= 'A') && (filename[i] <= 'Z'))
        {
            filename[i] = filename[i] + ('a' - 'A');
        }
        i++;
    }
    return filename;
}


/**
 * Einen "normalen" Dateinamen in einen rohen, kurzen Dateinamen umwandeln
 *
 * @param filename Pointer
 *
 * @return Pointer auf den neuen Dateinamen
 */
char* fat_convert_to_short_filename(char* name)
{
    char* filename = malloc(12);
    char* dot = strchr(name, '.');
    
    memset(filename, ' ', 11);
    filename[11] = 0;

    if(dot != NULL)
    {
        *dot = 0;
    }

    if(strlen(name) > 8)
    {
        memcpy(filename, name, 8);
    }
    else
    {
        memcpy(filename, name, strlen(name));
    }
    
    if(dot != NULL)
    {
        dot++;
        memcpy((filename + 8), dot, 3);
        dot--;
        *dot = '.';
    }

    //Noch in Grossbuchstaben verwandln
    filename[11] = 0;
    int i = 0;
    while(filename[i] != 0)
    {
        if((filename[i] >= 'a') && (filename[i] <= 'z'))
        {
            filename[i] = filename[i] - ('a' - 'A');
        }
        i++;
    }
   return filename;
}



fat_directory_entry_t* fat_get_directory_entry(lostio_filehandle_t* filehandle, fat_res_info_t* res_info, char* filename)
{
    DEBUG_MSG("anfang");
    fat_directory_entry_t* dir_entry = (fat_directory_entry_t*) (res_info->data);
    fat_directory_entry_t* result = NULL;
    char* short_filename = fat_convert_to_short_filename(filename);

    uint32_t i;

    for(i = 0; i < (res_info->size / sizeof(fat_directory_entry_t)); i++)
    {
        if(memcmp(dir_entry[i].name, short_filename, 11) == 0)
        {
            result = &(dir_entry[i]);
            break;
        }
    }

    free(short_filename);
    DEBUG_MSG("ende");
    return result;
}


fat_directory_entry_t* fat_get_free_directory_entry(lostio_filehandle_t* filehandle, fat_res_info_t* res_info)
{
    DEBUG_MSG("anfang");
    uint32_t i;
    fat_directory_entry_t* dir_ptr = (fat_directory_entry_t*) (res_info->data);

    for(i = 2; i < (res_info->size / sizeof(fat_directory_entry_t)); i++)
    {
        if((dir_ptr[i].name[0] == (char)0x00) || (dir_ptr[i].name[0] == (char) 0xe5))
        {
            DEBUG_MSG("ende");
            return &(dir_ptr[i]);
        }
    }
    
    //Wenn der Eintrag im Rootdir gewuenscht wird haben wir ein Problem,
    //da das Rootdir eine Feste Groesse hat.
    if(res_info-> cluster == 1)
    {
        puts("PANIC: Kein freier Eintrag im Rootdir");
        while(true) asm("nop");
    }

    //Bei Bedarf neue Cluster fuer das Verzeichnis allokieren.
    // FIXME Fehlender Nullcheck nach realloc, Memleak im Fehlerfall
    res_info->data = realloc(res_info->data, res_info->size + FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster);
    memset(res_info->data, 0, FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster);
    res_info->size += FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster;
    return (void*) (res_info->data + FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster);
}


void fat_create_directory(lostio_filehandle_t* filehandle, vfstree_node_t* node, char* path)
{
    fat_res_info_t* parent_res_info = (fat_res_info_t*) (node->data);
    fat_res_info_t* new_res_info = malloc(sizeof(fat_res_info_t));
    
    //Dem ersten Cluster des Verzeichnises allokieren und auf 0 setzen.
    new_res_info->size = FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster;
    new_res_info->data = malloc(new_res_info->size);
    new_res_info->cluster = fat_allocate_cluster(filehandle);
    if (new_res_info->cluster == 0) {
        free(new_res_info->data);
        free(new_res_info);
        puts("fat: Keine freien Cluster mehr vorhanden!");
        return;
    }
    fat_set_fat_entry(filehandle, new_res_info->cluster, 0xfff);
    memset(new_res_info->data, 0, new_res_info->size);
    
    
    //Dot und Dot-Dot Eintraege anlegen
    fat_directory_entry_t* dir_ptr = (fat_directory_entry_t*) (new_res_info->data);
    dir_ptr[0].first_cluster_lo = new_res_info->cluster & 0xffff;
    dir_ptr[0].first_cluster_hi = (new_res_info->cluster >> 16) & 0xffff;
    dir_ptr[0].attributes = FAT_ATTR_DIRECTORY;
    memcpy(dir_ptr[0].name, ".          ", 11);
    
    //Beim root-dir muss der cluster Eintrag auf 0 gesetzt sein
    if(parent_res_info->cluster == 1)
    {
        dir_ptr[1].first_cluster_lo = 0;
        dir_ptr[1].first_cluster_hi = 0;
    }
    else
    {
        dir_ptr[1].first_cluster_lo = parent_res_info->cluster & 0xffff;
        dir_ptr[1].first_cluster_hi = (parent_res_info->cluster >> 16) & 0xffff;
    }
    dir_ptr[1].attributes = FAT_ATTR_DIRECTORY;
    memcpy(dir_ptr[1].name, "..         ", 11);
    
    //Den Cluster auf das Medium schreiben
    fat_write_cluster(filehandle, new_res_info->cluster, new_res_info->data);
    new_res_info->loaded = true;
    
    //Den Knoten im VFS erstellen
    vfstree_create_node(path, FAT_LOSTIO_TYPE_DIR, 0, (void*)new_res_info, LOSTIO_FLAG_BROWSABLE);
}


/**
 * Seek auf einem FAT-Verzeichnis
 */
int fat_directory_close_handler(lostio_filehandle_t* filehandle)
{
    fat_res_info_t* res_info = filehandle->node->data;

    if((filehandle->flags & IO_OPEN_MODE_WRITE) != 0)
    {
        vfstree_node_t* parent_node = (vfstree_node_t*) (filehandle->node->parent);
        fat_save_directory(filehandle, parent_node);
        fat_save_fat(filehandle);
    }

    free(res_info->data);
    res_info->loaded = false;
    return 0;
}


