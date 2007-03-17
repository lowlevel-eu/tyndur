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

#define _USE_START_
#include "init.h"

#define FAT_LOSTIO_TYPE_DIR 255
#define FAT_LOSTIO_TYPE_FILE 254

#define FAT_ATTR_READ_ONLY  1
#define FAT_ATTR_HIDDEN     2
#define FAT_ATTR_SYSTEM     4
#define FAT_ATTR_VOLUME_ID  8
#define FAT_ATTR_DIRECTORY  16
#define FAT_ATTR_ARCHIVE    32
#define FAT_ATTR_LONGFNAME  (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

typedef struct
{
    ///Sprungbefehl ueber den BPB
    byte    bs_jump[3];
    ///Name der Softwarem die die Formatierung erstellt hat
    char    oem_name[8];
    ///
    word    bytes_per_sector;
    ///Die kleinste allokierbare groesse
    byte    sectors_per_cluster;
    ///Anzahl der reservierten sektoren
    word    reserved_sector_count;
    ///Anzahl der FATs
    byte    fat_count;
    ///Anzahl der Eintraege im Rootdir (0 bei FAT32)
    word    root_dir_entry_count;
    ///Anzahl der Sektoren auf dem Datentraeger (0 bei FAT32)
    word    total_sector_count;
    ///Typ des Mediums
    byte    media_type;
    ///Anzahl der Sektoren die eine FAT belegt (0 bei FAT32)
    word    fat_sector_count;
    ///Geometrie: Anzahl der Sektoren pro Spur
    word    sectors_per_track;
    ///Geometrie: Anzahl der Lese-/Schreibkoepfe
    word    head_count;
    ///Anzahl der versteckten Sektoren(Auf unpartitionierten Medien irrelevant)
    word    hidden_sectors_count;
    ///[FAT32] Anzahl der Sektoren auf dem Datentraeger
    dword   total_sector_count_32;
    
    //Ab hier kommen dann unterschiedliche Strukturen fuer FAT12&FAT16 /  FAT32
} __attribute__ ((packed)) fat_bios_parameter_block_t;

typedef struct
{
    ///Name des eintrags
    char    name[11];
    ///Attribute
    byte    attributes;
    ///Reserviert
    byte    res;
    ///Zehntelssekunden der Erstellungszeit
    byte    ctime_tenth;
    ///Erstellungszeit
    word    ctime;
    ///Erstellungsdatum
    word    cdate;
    ///Letzte Zugriffszeit
    word    atime;
    ///High-Word der Clusternummer (0 bei FAT32)
    word    first_cluster_hi;
    ///Letzte Schreibzugriffszeit
    word    wtime;
    ///Letztes Schreibzugriffsdatum
    word    wdate;
    ///Low-Word der Clusternummer
    word    first_cluster_lo;
    ///Dateigroesse
    dword   size;
} __attribute__ ((packed)) fat_directory_entry_t;

typedef struct
{
    byte    order;
    char    name_part_1[10];
    byte    attribute;
    byte    type;
    byte    checksum;
    char    name_part_2[12];
    word    first_cluster_lo;
    char    name_part_3[4];
} __attribute__ ((packed)) fat_vfat_entry_t;

typedef struct
{
    fat_bios_parameter_block_t      bios_parameter_block;
    byte*                           file_allocation_table;
} fat_rootdir_handle_t;

typedef struct
{
    bool    loaded;
    dword   cluster;
    dword   size;
    void*   data;
} fat_res_info_t;


fat_rootdir_handle_t* current_rootdir_handle;
io_resource_t* current_pipe_source;


bool fat_null_pre_open_handler(char** path, byte args, pid_t pid, io_resource_t* pipe_source);
bool fat_dir_pre_open_handler(char** path, byte args, pid_t pid, io_resource_t* pipe_source);

read_hdl_reply fat_file_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount);
int fat_file_close_handler(lostio_filehandle_t* filehandle);


int main(int argc, char* argv[])
{
    init_service_register("fat");
    
    lostio_init();
    lostio_type_directory_use();

    typehandle_t* typehandle    = malloc(sizeof(typehandle_t));
    typehandle->id              = 0;
    typehandle->pre_open        = &fat_dir_pre_open_handler;
    typehandle->post_open       = NULL;
    typehandle->read            = NULL;
    typehandle->write           = NULL;
    typehandle->seek            = NULL;
    typehandle->close           = NULL;
    lostio_register_typehandle(typehandle);

    //Den seek-handler aus Ramfile benutzen
    lostio_type_ramfile_use_as(FAT_LOSTIO_TYPE_FILE);
    typehandle = get_typehandle(FAT_LOSTIO_TYPE_FILE);
    typehandle->pre_open        = NULL;
    typehandle->post_open       = NULL;
    typehandle->read            = &fat_file_read_handler;
    typehandle->write           = NULL;
    typehandle->close           = &fat_file_close_handler;

    //Die Directoryhandler auch fuer FAT-Verzeichnisse benutzen.
    //Dafuer jedoch in einer leicht modifizierten Variante;
    lostio_type_directory_use_as(FAT_LOSTIO_TYPE_DIR);
    get_typehandle(FAT_LOSTIO_TYPE_DIR)->pre_open = &fat_dir_pre_open_handler;
    
    

    //Hier wird der Typ des rootdirs auf 0 gesetzt, damit die richtigen Handler
    //aufgerufen werden.
    vfstree_get_node_by_path("/")->type = FAT_LOSTIO_TYPE_DIR;

    while(TRUE) yield();
}


void fat_load_fat()
{

}

void fat_save_fat()
{

}


/**
 * Den Inhalt des FAT-Eintrages an 'cluster_id'
 *
 * @param cluster_id Clusternummer
 *
 * @return Inhalt des FAT-Eintrages
 */
dword fat_get_fat_entry(dword cluster_id)
{
    dword result;
    //TODO: Zwischen den einzelnen FAT-Dateisystemen unterscheiden
    //Das Hier funktioniert nur mit FAT12
    if (((cluster_id * 12) % 8) == 0)
    {
        result = current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8] |
                    ((current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8 + 1] & 0x0F) << 8);
    }
    else
    {
        result = ((current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8] & 0xF0) >> 4) |
                    (current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8 + 1] << 4);
    }

    return result;
}


/**
 * Eine Datei oder ein Verzeichnis anhand des Startclusters in den Speicher 
 * laden.
 *
 * @param cluster_id Startcluster
 * @param buffer Pointer auf den Bufferpointer
 *
 * @return Die anzahl der gelesenen Cluster
 */
dword fat_load_clusters(dword cluster_id, void** buffer)
{
    *buffer = NULL;
    dword cnt = 0;
    dword fat_sector_start = (
                                    current_rootdir_handle->bios_parameter_block.reserved_sector_count + 
                                    (current_rootdir_handle->bios_parameter_block.fat_count * current_rootdir_handle->bios_parameter_block.fat_sector_count) + 
                                    (current_rootdir_handle->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t) / current_rootdir_handle->bios_parameter_block.bytes_per_sector)
                                ) * current_rootdir_handle->bios_parameter_block.bytes_per_sector;
    
    while(TRUE)
    {
        if((cluster_id != 0xfff) && ((cluster_id * current_rootdir_handle->bios_parameter_block.sectors_per_cluster) > current_rootdir_handle->bios_parameter_block.total_sector_count))
        {
            printf("  FAT: Warung, das Dateisystem scheint beschaedigt zu sein (ein Cluster scheint auf einen anderen, ausserhalb der Grenzen dieses Mediums)\n");
            break;
        }

        cnt++;
        *buffer = realloc(*buffer, cnt * current_rootdir_handle->bios_parameter_block.bytes_per_sector);
        
        fseek(current_pipe_source, fat_sector_start + (cluster_id - 2) * current_rootdir_handle->bios_parameter_block.bytes_per_sector, SEEK_SET);
        fread(((void*) ((dword)*buffer + current_rootdir_handle->bios_parameter_block.bytes_per_sector * (cnt - 1))), 1, current_rootdir_handle->bios_parameter_block.bytes_per_sector, current_pipe_source);
        
        if((cluster_id == 0xfff))
        {
            break;
        }

        
        //printf("cluster: %d  =>", cluster_id);
        cluster_id = fat_get_fat_entry(cluster_id);
        //printf("  %d\n", cluster_id);
    }
    return cnt;
}


/**
 * Verzeichniseintraege in den VFS-Baum einlesen
 *
 * @param dir_ptr Pointer auf das Verzeichnis
 * @param dir_entry_count Anzahl der Verzeichniseintraege
 * @param 
 */
void fat_parse_directory(fat_directory_entry_t* dir_ptr, size_t dir_entry_count, char* path)
{
    int i,j,k;

    char* new_filename;
    char* path_begin = malloc(1024);
    char* filename_begin = path_begin + strlen(path) + 1;
    char* filename_end = NULL;
    size_t filename_len;
    bool long_filename = FALSE;

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
            if(filename_len == 0)
            {
                filename_len = 0;

            }
            
            if(long_filename != TRUE)
            {
                filename_end = filename_begin;
                long_filename = TRUE;
            }
            

            fat_vfat_entry_t* vfat_entry = &(dir_ptr[i]);
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
            memcpy(filename_end, dir_ptr[i].name, 11);
                
            new_filename = filename_end;
                
            filename_end += 11;
            filename_len += 11;
            *filename_end = 0;
                
            //Ende des Dateinamen suchen
            for(j = 7; j >= 0; j--)
            {
                if(new_filename[j] != ' ')
                    break;
            }
            
            //Wenn eine Endung vorhanden ist, wird sie kopiert
            if(dir_ptr[i].name[8] != ' ')
            {
                new_filename[j+1] = '.';
                memcpy(&(new_filename[j+2]), &(dir_ptr[i].name[8]), 3);
                new_filename[j+5] = 0;
            }
            else
            {
                new_filename[j+1] = 0;
            }

            //Das Ganze noch in Kleinbuchstaben umwandeln
            j = 0;
            while(new_filename[j] != 0)
            {
                if((new_filename[j] >= 'A') && (new_filename[j] <= 'Z'))
                {
                    new_filename[j] = new_filename[j] + ('a' - 'A');
                }
                j++;
            }
            
            //filename_len += strlen(path);
            //filename_begin = realloc(filename_begin, filename_len);
            //filename_end = filename_begin + strlen(path);
            
            
            fat_res_info_t* res_info = malloc(sizeof(fat_res_info_t));
            //Die Resource ist noch nicht im Speicher
            res_info->loaded = FALSE;
            //Groesse eintragen
            res_info->size = dir_ptr[i].size;
            //Startcluster eintragen
            res_info->cluster = ((dir_ptr[i].first_cluster_lo) | (dir_ptr[i].first_cluster_hi << 16));
            //printf("'%s' @%d\n", path_begin, res_info->cluster);
            if((dir_ptr[i].attributes & FAT_ATTR_DIRECTORY) == 0)
            {
                vfstree_create_node(path_begin, FAT_LOSTIO_TYPE_FILE, res_info->size, (void*)res_info);
            }
            else
            {   
                vfstree_create_node(path_begin, FAT_LOSTIO_TYPE_DIR, 0, (void*)res_info);
            }
            
            filename_len = 0;
            long_filename = FALSE;
        }
    }
    //free(file_path);
}


bool fat_null_pre_open_handler(char** path, byte args, pid_t pid, io_resource_t* pipe_source)
{
    char* pid_dir = NULL;
    char* resid_dir = NULL;
    char* root_dir = NULL;
    char* bpb_path = NULL;
    char* new_path = NULL;
    dword i;
    
    if(
        (pipe_source == NULL) ||
        (asprintf(&pid_dir, "/%d", pipe_source->pid) == -1) ||
        (asprintf(&resid_dir, "/%d/%d", pipe_source->pid, pipe_source->resid) == -1) ||
        (asprintf(&root_dir, "/%d/%d/root", pipe_source->pid, pipe_source->resid) == -1)||
        (asprintf(&bpb_path, "/%d/%d/root/bpb", pipe_source->pid, pipe_source->resid) == -1) ||
        //FIXME: dword-Cast entferen, sobald asprintf keine Probleme mehr macht
        (asprintf(&new_path, "/%d/%d%s", pipe_source->pid, (dword)pipe_source->resid, *path) == -1)
    )
    {
        return FALSE;
    }
       

    if(vfstree_get_node_by_path(root_dir) == NULL)
    {
        
        if(vfstree_get_node_by_path(resid_dir) == NULL)
        {
            if(vfstree_get_node_by_path(pid_dir) == NULL)
            {
                vfstree_create_node(pid_dir, LOSTIO_TYPES_DIRECTORY, 0, NULL);
            }

            vfstree_create_node(resid_dir, LOSTIO_TYPES_DIRECTORY, 0, NULL);
        }
        fat_res_info_t* dir_info = malloc(sizeof(fat_res_info_t));
        dir_info->loaded = TRUE;

        vfstree_create_node(root_dir, LOSTIO_TYPES_DIRECTORY, 0, dir_info);
        
        
        //Rootdir-Handle Bereit machen
        fat_rootdir_handle_t* rootdir_handle = malloc(sizeof(fat_rootdir_handle_t));
        current_rootdir_handle = rootdir_handle;
        
        //BIOS-Parameterblock einlesen
        fseek(pipe_source, 0, SEEK_SET);
        fread(&(rootdir_handle->bios_parameter_block), 1, sizeof(fat_bios_parameter_block_t), pipe_source);
        
        //FAT-Einlesen
        rootdir_handle->file_allocation_table = malloc(rootdir_handle->bios_parameter_block.fat_sector_count * current_rootdir_handle->bios_parameter_block.bytes_per_sector);
        fseek(pipe_source, rootdir_handle->bios_parameter_block.reserved_sector_count * current_rootdir_handle->bios_parameter_block.bytes_per_sector, SEEK_SET);
        fread(rootdir_handle->file_allocation_table, current_rootdir_handle->bios_parameter_block.bytes_per_sector, rootdir_handle->bios_parameter_block.fat_sector_count, pipe_source);

        
        void* rootdir_buffer;
        if(rootdir_handle->bios_parameter_block.root_dir_entry_count != 0)
        {
            rootdir_buffer = malloc(rootdir_handle->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t));
            
            //Die Root-Directory-Sektoren einlesen
            fseek(pipe_source, (    
                                    rootdir_handle->bios_parameter_block.reserved_sector_count + 
                                    (rootdir_handle->bios_parameter_block.fat_count * rootdir_handle->bios_parameter_block.fat_sector_count)
                               ) *  current_rootdir_handle->bios_parameter_block.bytes_per_sector 
                            , SEEK_SET);
            
            fread(rootdir_buffer, 1, (rootdir_handle->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t)), pipe_source);
            
            //Verzeichnisbaum einlesen
            fat_parse_directory(rootdir_buffer, rootdir_handle->bios_parameter_block.root_dir_entry_count, root_dir);
            //Buffer wieder Freigeben
            free(rootdir_buffer);

            //Den Pfad anpassen
            *path = root_dir;
        }
        else
        {
            puts("FAT32 wird noch nicht unterstuetzt");
            return FALSE;
        }
    }
    return TRUE;
}


/**
 * 
 */
bool fat_dir_pre_open_handler(char** path, byte args, pid_t pid, io_resource_t* pipe_source)
{
    //puts("fat_dir_pre_open_handler");
    char* new_path;
    char* root_path;
    bool result;

    
    if((pipe_source == NULL))
    {
        return FALSE;
    }
    current_pipe_source = pipe_source;
    
    if((**path == '/') && (strlen(*path) == 1))
    {
        (*path)++;
    }

    //FIXME: dword-Cast entferen, sobald asprintf keine Probleme mehr macht
    if(
        (asprintf(&root_path, "/%d/%d/root",  pipe_source->pid, (dword)pipe_source->resid) == -1) ||
        (asprintf(&new_path, "/%d/%d/root%s",  pipe_source->pid, (dword)pipe_source->resid, *path) == -1))
    {
        return FALSE; 
    }

    //Ueberpruefen, ob diese Quelle schon benutzt wurde
    if(vfstree_get_node_by_path(root_path) == NULL)
    {
        result = fat_null_pre_open_handler(path, args, pid, pipe_source);
    }
    else
    {
        *path = new_path;
        result = TRUE;

        vfstree_node_t* node = vfstree_get_node_by_path(new_path);

        
        if(node != NULL)
        {
            fat_res_info_t* res_info = node->data;
            

            if(res_info->loaded == FALSE)
            {
                void* buffer;
                dword size = fat_load_clusters(res_info->cluster, &buffer);
                if(node->type == FAT_LOSTIO_TYPE_DIR)
                {
                    fat_parse_directory((fat_directory_entry_t*) buffer, size * 512 / sizeof(fat_directory_entry_t), new_path);
                    free(buffer);
                }
                else
                {
                    res_info->data = buffer;
                }
                res_info->loaded = TRUE;
            }
        }
    }

    
    free(root_path);

    return TRUE;
}


read_hdl_reply fat_file_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount)
{
    fat_res_info_t* res_info = filehandle->node->data;
    read_hdl_reply reply;
    qword size = blocksize * blockcount;

    if((filehandle->pos + size) > filehandle->node->size)
    {
        size = filehandle->node->size - filehandle->pos;
    }

    
    
    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }

    reply.size = size;
    reply.data = (void*) ((dword)(res_info->data) + filehandle->pos);

    filehandle->pos += size;
    return reply;
}


int fat_file_close_handler(lostio_filehandle_t* filehandle)
{
    fat_res_info_t* res_info = filehandle->node->data;
    res_info->loaded = FALSE;
    free(res_info->data);
    return 0;
}




