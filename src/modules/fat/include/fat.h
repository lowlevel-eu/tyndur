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

#ifndef _FAT_H_
#define _FAT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "types.h"
#include "lostio.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
//#define DEBUG_MSG(s) printf("[  FAT  ] debug: %s() '%s'\n", __FUNCTION__, s)
#define DEBUG_MSG(s) //


#undef CONFIG_FAT_USE_LFN

#define FAT_LOSTIO_TYPE_DIR 255
#define FAT_LOSTIO_TYPE_FILE 254

#define FAT_ATTR_READ_ONLY  1
#define FAT_ATTR_HIDDEN     2
#define FAT_ATTR_SYSTEM     4
#define FAT_ATTR_VOLUME_ID  8
#define FAT_ATTR_DIRECTORY  16
#define FAT_ATTR_ARCHIVE    32
//#define FAT_ATTR_LONGFNAME  //(AT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID | FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE)
#define FAT_ATTR_LONGFNAME  0xF
#define FAT_ATTR_LOST_SYMLINK 0xFF

#define FAT_CUR_RD_HANDLE ((fat_rootdir_handle_t*)filehandle->data)


typedef struct
{
    ///Sprungbefehl ueber den BPB
    uint8_t    bs_jump[3];
    ///Name der Softwarem die die Formatierung erstellt hat
    char    oem_name[8];
    ///
    uint16_t    bytes_per_sector;
    ///Die kleinste allokierbare groesse
    uint8_t    sectors_per_cluster;
    ///Anzahl der reservierten sektoren
    uint16_t    reserved_sector_count;
    ///Anzahl der FATs
    uint8_t    fat_count;
    ///Anzahl der Eintraege im Rootdir (0 bei FAT32)
    uint16_t    root_dir_entry_count;
    ///Anzahl der Sektoren auf dem Datentraeger (0 bei FAT32)
    uint16_t    total_sector_count;
    ///Typ des Mediums
    uint8_t    media_type;
    ///Anzahl der Sektoren die eine FAT belegt (0 bei FAT32)
    uint16_t    fat_sector_count;
    ///Geometrie: Anzahl der Sektoren pro Spur
    uint16_t    sectors_per_track;
    ///Geometrie: Anzahl der Lese-/Schreibkoepfe
    uint16_t    head_count;
    ///Anzahl der versteckten Sektoren(Auf unpartitionierten Medien irrelevant)
    uint32_t   hidden_sectors_count;
    ///[FAT32] Anzahl der Sektoren auf dem Datentraeger
    uint32_t   total_sector_count_32;
    
    //Ab hier kommen dann unterschiedliche Strukturen fuer FAT12&FAT16 /  FAT32
} __attribute__ ((packed)) fat_bios_parameter_block_t;

typedef struct
{
    ///Name des eintrags
    char    name[11];
    ///Attribute
    uint8_t    attributes;
    ///Reserviert
    uint8_t    res;
    ///Zehntelssekunden der Erstellungszeit
    uint8_t    ctime_tenth;
    ///Erstellungszeit
    uint16_t    ctime;
    ///Erstellungsdatum
    uint16_t    cdate;
    ///Letzte Zugriffszeit
    uint16_t    atime;
    ///High-Word der Clusternummer (0 bei FAT32)
    uint16_t    first_cluster_hi;
    ///Letzte Schreibzugriffszeit
    uint16_t    wtime;
    ///Letztes Schreibzugriffsdatum
    uint16_t    wdate;
    ///Low-Word der Clusternummer
    uint16_t    first_cluster_lo;
    ///Dateigroesse
    uint32_t   size;
} __attribute__ ((packed)) fat_directory_entry_t;

typedef struct
{
    uint8_t    order;
    char    name_part_1[10];
    uint8_t    attribute;
    uint8_t    type;
    uint8_t    checksum;
    char    name_part_2[12];
    uint16_t    first_cluster_lo;
    char    name_part_3[4];
} __attribute__ ((packed)) fat_vfat_entry_t;

typedef struct
{
    uint8_t                        fat_size;
    fat_bios_parameter_block_t  bios_parameter_block;
    size_t                      total_sector_count;
    uint8_t*                       file_allocation_table;
} fat_rootdir_handle_t;

typedef struct
{
    bool    loaded;
    uint32_t   cluster;
    uint32_t   size;
    void*   data;
} fat_res_info_t;


//extern fat_rootdir_handle_t* current_rootdir_handle;
//extern io_resource_t* current_pipe_source;

///Verzeichnis in den VFS-Baum einlesen
void fat_parse_directory(lostio_filehandle_t* filehandle, fat_directory_entry_t* dir_ptr, size_t dir_entry_count, char* path);

///Verzeichnis vom Datentraeger laden
void fat_load_directory(lostio_filehandle_t* filehandle, fat_res_info_t* res_info, char* path);

///Verzeichnis auf den Datentraeger speichern
void fat_save_directory(lostio_filehandle_t* filehandle, vfstree_node_t* dir);

///Verzeichnis erstellen
void fat_create_directory(lostio_filehandle_t* filehandle, vfstree_node_t* node, char* path);

///Neue Datei erstellen
void fat_create_file(lostio_filehandle_t* filehandle, vfstree_node_t* node, char* path);

///Neuen Symlink erstellen
void fat_create_symlink(lostio_filehandle_t* filehandle, vfstree_node_t* node, char* path);

///Die Groesse einer Datei aendern
void fat_file_set_size(uint32_t size);

///Rohen kurzen Dateinamen in einen "normalen" umwandeln
char* fat_convert_from_short_filename(char* short_filename);

///Normalen Dateinamen in einen kurzen umwandeln
char* fat_convert_to_short_filename(char* short_filename);

///Einen freien Verzeichniseintrag suchen
fat_directory_entry_t* fat_get_free_directory_entry(lostio_filehandle_t* filehandle, fat_res_info_t* res_info);

///Den Verzeichniseintrag mit dem angegebenen Namen suchen
fat_directory_entry_t* fat_get_directory_entry(lostio_filehandle_t* filehandle, fat_res_info_t* res_info, char* filename);


lostio_filehandle_t* fat_mount_source(char** path, uint8_t args, pid_t pid, FILE* pipe_source);
bool fat_res_pre_open_handler(char** path, uint8_t args, pid_t pid, FILE* pipe_source);
bool fat_res_not_found_handler(char** path, uint8_t args, pid_t pid, FILE* pipe_source);
void fat_res_post_open_handler(lostio_filehandle_t* filehandle);

int fat_file_seek_handler(lostio_filehandle_t* filehandle, uint64_t offset,
    int origin);
size_t fat_file_read_handler(lostio_filehandle_t* filehandle,
    void* buf, size_t blocksize, size_t blockcount);
size_t fat_file_write_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data);
int fat_file_close_handler(lostio_filehandle_t* filehandle);

int fat_directory_close_handler(lostio_filehandle_t* filehandle);



///FAT von dem Datentraeger lesen
void fat_load_fat(lostio_filehandle_t* filehandle);

///FAT auf den Datentraeger speichern
void fat_save_fat(lostio_filehandle_t* filehandle);

///Inhalt eines FAT-Eintrags auslesen
uint32_t fat_get_fat_entry(lostio_filehandle_t*, uint32_t cluster_id);

///Inhalt eines FAT-Eintrags ueberschreiben
void fat_set_fat_entry(lostio_filehandle_t* filehandle, uint32_t cluster_id, uint32_t cluster_content);

///Einen unbenutzen Cluster suchen
uint32_t fat_allocate_cluster(lostio_filehandle_t* filehandle);

///Einen Cluster von dem Datentraeger lesen
void fat_read_cluster(lostio_filehandle_t* filehandle, uint32_t cluster_id, void* dest, size_t count);

///Einen Cluster auf den Datentraeger speichern
void fat_write_cluster(lostio_filehandle_t* filehandle, uint32_t cluster_id, void* src);

///Alle Cluster einer Datei/eines Verzeichnises einlesen
uint32_t fat_load_clusters(lostio_filehandle_t* filehandle, uint32_t cluster_id, void** dest);

///Alle Cluster einer Datei/eines Verzeichnises abspeichern
bool fat_save_clusters(lostio_filehandle_t* filehandle, uint32_t cluster_id, void* src, size_t size);



///FAT-Resource von dem Datentraeger laden
void fat_load_resource(lostio_filehandle_t* filehandle, fat_res_info_t* res_info);

///FAT-Resource auf den Datentraeger speichern
void fat_save_resource(lostio_filehandle_t* filehandle, fat_res_info_t* res_info);



///Einen String kopieren
char* strclone(char* str);

#endif

