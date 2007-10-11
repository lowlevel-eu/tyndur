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
#include "init.h"
#include "fat.h"

/**
 * FAT laden
 */
void fat_load_fat(lostio_filehandle_t* filehandle)
{
    DEBUG_MSG("anfang");
    
    fseek(filehandle->source, FAT_CUR_RD_HANDLE->bios_parameter_block.reserved_sector_count * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector, SEEK_SET);
    
    fread(FAT_CUR_RD_HANDLE->file_allocation_table,
        FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector,
        FAT_CUR_RD_HANDLE->bios_parameter_block.fat_sector_count, 
        filehandle->source);
    
    DEBUG_MSG("ende");
}


/**
 * FAT wieder auf das Medium zurueckschreiben
 */
void fat_save_fat(lostio_filehandle_t* filehandle)
{
    DEBUG_MSG("anfang");
    puts("save fats");
    int i;
    fseek(filehandle->source, FAT_CUR_RD_HANDLE->bios_parameter_block.reserved_sector_count * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector, SEEK_SET);

    for (i = 0; i < FAT_CUR_RD_HANDLE->bios_parameter_block.fat_count; i++) {
        fwrite(FAT_CUR_RD_HANDLE->file_allocation_table,
            FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector,
            FAT_CUR_RD_HANDLE->bios_parameter_block.fat_sector_count,
            filehandle->source);
    }
    DEBUG_MSG("ende");
}


/**
 * Den Inhalt des FAT-Eintrages an 'cluster_id'
 *
 * @param cluster_id Clusternummer
 *
 * @return Inhalt des FAT-Eintrages
 */
dword fat_get_fat_entry(lostio_filehandle_t* filehandle, dword cluster_id)
{
    dword result;
    
    if (FAT_CUR_RD_HANDLE->fat_size == 12) {
        if (((cluster_id * 12) % 8) == 0) {
            result = FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8] |
                ((FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8 + 1] & 0x0F) << 8);
        }
        else
        {
            result = ((FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8] & 0xF0) >> 4) |
                (FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8 + 1] << 4);
        }
    } else if (FAT_CUR_RD_HANDLE->fat_size == 16) {
        result = FAT_CUR_RD_HANDLE->file_allocation_table[cluster_id * 2] |
                ((FAT_CUR_RD_HANDLE->file_allocation_table[cluster_id * 2 + 1]) << 8);
    } else {
        printf("[  FAT  ] FAT%d wird noch nicht unterstuetzt!\n", FAT_CUR_RD_HANDLE->fat_size);
        result = 0;
    }

    return result;
}


/**
 * Den Inhalt des FAT-Eintrages an 'cluster_id' ueberschreiben
 *
 * @param cluster_id Clusternummer
 * @param cluster_content Neuer Inhalt fuer den Cluster
 */
void fat_set_fat_entry(lostio_filehandle_t* filehandle, dword cluster_id, dword cluster_content)
{
    if (FAT_CUR_RD_HANDLE->fat_size == 12) {
        if (((cluster_id * 12) % 8) == 0) {
            FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8] = cluster_content & 0xFF;
            FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8 + 1] &= 0xF0;
            FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8 + 1] |= (cluster_content >> 8) & 0xF;
        } else {
            FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8] &= 0xF;
            FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8] |= (cluster_content << 4) & 0xF0;
            
            FAT_CUR_RD_HANDLE->file_allocation_table[(cluster_id * 12) / 8 + 1] = (cluster_content >> 4) & 0xFF;
        }
    } else if (FAT_CUR_RD_HANDLE->fat_size == 16) {
        FAT_CUR_RD_HANDLE->file_allocation_table[cluster_id * 2] = cluster_content & 0xFF;
        FAT_CUR_RD_HANDLE->file_allocation_table[cluster_id * 2 + 1] = (cluster_content << 8)& 0xFF;
    } else {
        printf("[  FAT  ] FAT%d wird noch nicht unterstuetzt!\n", FAT_CUR_RD_HANDLE->fat_size);
    }
}


/**
 * Einen unbenutzten Cluster suchen
 *
 */
dword fat_allocate_cluster(lostio_filehandle_t* filehandle)
{
    DEBUG_MSG("anfang");
    dword i;
    dword result = 0;
    dword cluster;
    
    dword clusters = FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.fat_sector_count;
    if (FAT_CUR_RD_HANDLE->fat_size == 12) {
        clusters = clusters * 2 / 3;
    } else if (FAT_CUR_RD_HANDLE->fat_size == 16) {
        clusters /= 2;
    } else if (FAT_CUR_RD_HANDLE->fat_size == 32) {
        clusters /= 4;
    }

    //Alle cluster durchsuchen, bis ein unbenutzter gefunden wird
    for(i = 3; i < clusters; i++)
    {
        cluster = fat_get_fat_entry(filehandle, i);
        if(cluster == 0x0)
        {
            result = i;
            break;
        }
    }
    
    DEBUG_MSG("ende");
    return result;
}


/**
 *
 */
void fat_read_cluster(lostio_filehandle_t* filehandle, dword cluster_id, void* dest, size_t count)
{
    //Startadresse der FAT berechnen
    dword fat_sector_start = (
            FAT_CUR_RD_HANDLE->bios_parameter_block.reserved_sector_count + 
            (FAT_CUR_RD_HANDLE->bios_parameter_block.fat_count * 
                FAT_CUR_RD_HANDLE->bios_parameter_block.fat_sector_count) + 
            (FAT_CUR_RD_HANDLE->bios_parameter_block.root_dir_entry_count * 
                sizeof(fat_directory_entry_t) / FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector)
        );
    
    fseek(filehandle->source, (fat_sector_start + (cluster_id - 2) * 
        FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster )* 
        FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector, SEEK_SET);

    fread(dest, FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector, 
        FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster * count,
        filehandle->source);
}


/**
 *
 */
void fat_write_cluster(lostio_filehandle_t* filehandle, dword cluster_id, void* src)
{
    //Startadresse der FAT berechnen
    dword fat_sector_start = (
            FAT_CUR_RD_HANDLE->bios_parameter_block.reserved_sector_count + 
            (FAT_CUR_RD_HANDLE->bios_parameter_block.fat_count * 
                FAT_CUR_RD_HANDLE->bios_parameter_block.fat_sector_count) + 
            (FAT_CUR_RD_HANDLE->bios_parameter_block.root_dir_entry_count * 
                sizeof(fat_directory_entry_t) / FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector)
        ) * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector;
    
    fseek(filehandle->source, fat_sector_start + (cluster_id - 2) * 
        FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * 
        FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster, SEEK_SET);

    fwrite(src, 1, FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * 
        FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster, 
        filehandle->source);   
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
dword fat_load_clusters(lostio_filehandle_t* filehandle, dword cluster_id, void** dest)
{
    DEBUG_MSG("anfang");
    //Das RootDir muss speziell verarbeitet werden
    if(cluster_id == 1)
    {
        cluster_id = (FAT_CUR_RD_HANDLE->bios_parameter_block.reserved_sector_count + 
            FAT_CUR_RD_HANDLE->bios_parameter_block.fat_count * FAT_CUR_RD_HANDLE->bios_parameter_block.fat_sector_count);
        dword size = FAT_CUR_RD_HANDLE->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t);
        
        *dest = malloc(size);

        fseek(filehandle->source, cluster_id * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector, SEEK_SET);
        fread(*dest, 1, size, filehandle->source);

        DEBUG_MSG("ende1");
        return size / (FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector);
    }

    dword cnt = 0;
    dword old_cluster_id = cluster_id;
    //Die Buffergroesse berechenn
    while(TRUE)
    {
        if(((cluster_id * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster) > FAT_CUR_RD_HANDLE->total_sector_count))
        {
            break;
        }
        
        cnt++;
        
        //Naechsten Cluster ausfindig machen
        cluster_id = fat_get_fat_entry(filehandle, cluster_id);
    }

    *dest = malloc(FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster * cnt);

    cluster_id = old_cluster_id;
    cnt = 0;
    while(TRUE)
    {
        //Sicherstellen, dass der Cluster nicht ausserhalb der Mediumsgrenzen
        // ist.
        if(((cluster_id * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster) 
                > FAT_CUR_RD_HANDLE->total_sector_count))
        {
            break;
        }
        
        // Aneinanderhaengende Cluster pruefen
        dword next_cluster = cluster_id;
        dword cont_clusters = 0;
        
        // FIXME: Das 0x9000 ist nur ein Workaround fuer die
        // groessenbeschraenkung von RPC
        while (next_cluster == cluster_id + cont_clusters)
        {
            next_cluster = fat_get_fat_entry(filehandle, next_cluster);
            cont_clusters++;
            if (next_cluster * FAT_CUR_RD_HANDLE->bios_parameter_block.
                sectors_per_cluster > FAT_CUR_RD_HANDLE->total_sector_count) {
                break;
            }
        }
        
        //Cluster einlesen
        fat_read_cluster(filehandle, cluster_id, ((void*) ((dword)*dest +
            FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector *
            FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster * cnt)
            ), cont_clusters);
        
        cnt += cont_clusters;
        cluster_id = next_cluster;
    }
    DEBUG_MSG("ende2");
    return cnt;
}


/**
 *
 */
bool fat_save_clusters(lostio_filehandle_t* filehandle, dword cluster_id, void* src, size_t size)
{
    DEBUG_MSG("anfang");
    dword cluster_count = size / (FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster);
    size_t size_done = 0;
    
    if((size % (FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster)) != 0)
    {
        cluster_count++;
    }
    

    //printf("save clusters %d, size=%x, cout=%x\n", cluster_id, size, cluster_count);

    //Das RootDir muss speziell verarbeitet werden
    if(cluster_id == 1)
    {
        size = FAT_CUR_RD_HANDLE->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t);
        DEBUG_MSG("Lese Rootdir");
        cluster_id = (FAT_CUR_RD_HANDLE->bios_parameter_block.reserved_sector_count + 
            FAT_CUR_RD_HANDLE->bios_parameter_block.fat_count * FAT_CUR_RD_HANDLE->bios_parameter_block.fat_sector_count);

        fseek(filehandle->source, cluster_id * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector, SEEK_SET);
        DEBUG_MSG("Seek abgeschlossen");
        fwrite(src, 1, size, filehandle->source);
        DEBUG_MSG("ende");
        return TRUE;
    }
    

    dword i;
    dword last_cluster_id = 0;
    
    for(i = 0; i < cluster_count; i++)
    {
        size_t size_current;
        if((size_done + (FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster)) > size)
        {
            size_current = size - (i * FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster); 
        }
        else
        {
            size_current = FAT_CUR_RD_HANDLE->bios_parameter_block.bytes_per_sector * FAT_CUR_RD_HANDLE->bios_parameter_block.sectors_per_cluster;
        }
        
        //Cluster schreiben
        fat_write_cluster(filehandle, cluster_id, ((void*) ((dword)src + size_done)));

        last_cluster_id = cluster_id;
        cluster_id = fat_get_fat_entry(filehandle, cluster_id);
        
        
        //Falls der Schreibvorgang noch nicht vollendet ist, aber der letzte
        // eingetragene Cluster schon geschrieben worden ist, muss ein neuer
        // Cluster allokiert wird.
        if((i < (cluster_count - 1)) && (cluster_id * FAT_CUR_RD_HANDLE->
                bios_parameter_block.sectors_per_cluster > FAT_CUR_RD_HANDLE->
                total_sector_count))
        {
            //Einen weiteren Cluster allokieren
            cluster_id = fat_allocate_cluster(filehandle);
            
            if(cluster_id == 0)
            {
                //FIXME: Sollte nicht haengen bleiben, sondern Fehler zurueckgeben
                printf("Kein freier Cluster mehr vorhanden!");
                while(TRUE) asm("nop");
            }

            //Eintraege in der FAT richtig setzen
            fat_set_fat_entry(filehandle, last_cluster_id, cluster_id);
            fat_set_fat_entry(filehandle, cluster_id, 0xfff);
        }
        size_done += size_current;
    }
    DEBUG_MSG("ende");
    return TRUE;
}

