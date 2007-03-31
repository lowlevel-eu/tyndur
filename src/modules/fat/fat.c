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
#include "init.h"
#include "fat.h"

/**
 * FAT laden
 */
void fat_load_fat()
{
    DEBUG_MSG("anfang");
    fseek(current_pipe_source, current_rootdir_handle->bios_parameter_block.reserved_sector_count * current_rootdir_handle->bios_parameter_block.bytes_per_sector, SEEK_SET);
    fread(current_rootdir_handle->file_allocation_table, 
            current_rootdir_handle->bios_parameter_block.bytes_per_sector,
            current_rootdir_handle->bios_parameter_block.fat_sector_count, 
            current_pipe_source);
    DEBUG_MSG("ende");
}


/**
 * FAT wieder auf das Medium zurueckschreiben
 */
void fat_save_fat()
{
    DEBUG_MSG("anfang");
    
    byte i;
    for(i = 0; i < current_rootdir_handle->bios_parameter_block.fat_count; i++)
    {
        fseek(current_pipe_source, current_rootdir_handle->bios_parameter_block.reserved_sector_count * current_rootdir_handle->bios_parameter_block.bytes_per_sector +
            i * current_rootdir_handle->bios_parameter_block.fat_sector_count * current_rootdir_handle->bios_parameter_block.bytes_per_sector, SEEK_SET);
        fwrite(current_rootdir_handle->file_allocation_table,
            current_rootdir_handle->bios_parameter_block.bytes_per_sector,
            current_rootdir_handle->bios_parameter_block.fat_sector_count,
            current_pipe_source);
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
 * Den Inhalt des FAT-Eintrages an 'cluster_id' ueberschreiben
 *
 * @param cluster_id Clusternummer
 * @param cluster_content Neuer Inhalt fuer den Cluster
 */
void fat_set_fat_entry(dword cluster_id, dword cluster_content)
{
    //TODO: Zwischen den einzelnen FAT-Dateisystemen unterscheiden
    //Das Hier funktioniert nur mit FAT12
    if (((cluster_id * 12) % 8) == 0)
    {
        current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8] = cluster_content & 0xFF;
        current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8 + 1] &= 0xF0;
        current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8 + 1] |= (cluster_content >> 8) & 0xF;
    }
    else
    {
        current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8] &= 0xF;
        current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8] |= (cluster_content << 4) & 0xF0;
        
        current_rootdir_handle->file_allocation_table[(cluster_id * 12) / 8 + 1] = (cluster_content >> 4) & 0xFF;
    }
}


/**
 * Einen unbenutzten Cluster suchen
 *
 */
dword fat_allocate_cluster()
{
    DEBUG_MSG("anfang");
    dword i;
    dword result = 0;
    dword cluster;

    //Alle cluster durchsuchen, bis ein unbenutzter gefunden wird
    for(i = 3; i < (current_rootdir_handle->bios_parameter_block.bytes_per_sector * current_rootdir_handle->bios_parameter_block.fat_sector_count) * 3 / 2; i++)
    {
        cluster = fat_get_fat_entry(i);
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
void fat_read_cluster(dword cluster_id, void* dest)
{
    //Startadresse der FAT berechnen
    dword fat_sector_start = (
            current_rootdir_handle->bios_parameter_block.reserved_sector_count + 
            (current_rootdir_handle->bios_parameter_block.fat_count * 
                current_rootdir_handle->bios_parameter_block.fat_sector_count) + 
            (current_rootdir_handle->bios_parameter_block.root_dir_entry_count * 
                sizeof(fat_directory_entry_t) / current_rootdir_handle->bios_parameter_block.bytes_per_sector)
        );
    
    fseek(current_pipe_source, (fat_sector_start + (cluster_id - 2) )* 
        current_rootdir_handle->bios_parameter_block.bytes_per_sector * 
        current_rootdir_handle->bios_parameter_block.sectors_per_cluster, SEEK_SET);

    fread(dest, current_rootdir_handle->bios_parameter_block.bytes_per_sector, 
        current_rootdir_handle->bios_parameter_block.sectors_per_cluster, 
        current_pipe_source);

}


/**
 *
 */
void fat_write_cluster(dword cluster_id, void* src)
{
    //Startadresse der FAT berechnen
    dword fat_sector_start = (
            current_rootdir_handle->bios_parameter_block.reserved_sector_count + 
            (current_rootdir_handle->bios_parameter_block.fat_count * 
                current_rootdir_handle->bios_parameter_block.fat_sector_count) + 
            (current_rootdir_handle->bios_parameter_block.root_dir_entry_count * 
                sizeof(fat_directory_entry_t) / current_rootdir_handle->bios_parameter_block.bytes_per_sector)
        ) * current_rootdir_handle->bios_parameter_block.bytes_per_sector;
    
    fseek(current_pipe_source, fat_sector_start + (cluster_id - 2) * 
        current_rootdir_handle->bios_parameter_block.bytes_per_sector * 
        current_rootdir_handle->bios_parameter_block.sectors_per_cluster, SEEK_SET);

    fwrite(src, 1, current_rootdir_handle->bios_parameter_block.bytes_per_sector * 
        current_rootdir_handle->bios_parameter_block.sectors_per_cluster, 
        current_pipe_source);   
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
dword fat_load_clusters(dword cluster_id, void** dest)
{
    //Das RootDir muss speziell verarbeitet werden
    if(cluster_id == 1)
    {
        cluster_id = (current_rootdir_handle->bios_parameter_block.reserved_sector_count + 
            current_rootdir_handle->bios_parameter_block.fat_count * current_rootdir_handle->bios_parameter_block.fat_sector_count);
        dword size = current_rootdir_handle->bios_parameter_block.root_dir_entry_count * sizeof(fat_directory_entry_t);
        
        *dest = malloc(size);

        fseek(current_pipe_source, cluster_id * current_rootdir_handle->bios_parameter_block.bytes_per_sector, SEEK_SET);
        fread(*dest, 1, size, current_pipe_source);

                                           
        return size / (current_rootdir_handle->bios_parameter_block.sectors_per_cluster * current_rootdir_handle->bios_parameter_block.bytes_per_sector);
    }

    dword cnt = 0;
    
    //Zielpointer auf 0 setzen, damit er mit realloc allokiert wird
    *dest = NULL;

    while(TRUE)
    {
        //Sicherstellen, dass der Cluster nicht ausserhalb der Mediumsgrenzen
        // ist.
        if( (cluster_id != 0xfff) && 
            ((cluster_id * current_rootdir_handle->bios_parameter_block.sectors_per_cluster) 
                > current_rootdir_handle->bios_parameter_block.total_sector_count))
        {
            printf("  FAT: Warnung, das Dateisystem scheint beschaedigt zu sein (Ein Cluster scheint auf einen anderen, ausserhalb der Grenzen dieses Mediums zu verweisen.)\n");
            break;
        }

        //Hier aufhoeren, wenn das der letzte Cluster war.
        if((cluster_id == 0xfff))
        {
            break;
        }

        //Anzahl der eingelesenen Cluster erhoehen
        cnt++;
        
        //Buffer vergroessern
        *dest = realloc(*dest, cnt * current_rootdir_handle->bios_parameter_block.bytes_per_sector);
        
        //Cluster einlesen
        fat_read_cluster(cluster_id, 
            ((void*) ((dword)*dest + current_rootdir_handle->bios_parameter_block.bytes_per_sector * (cnt - 1))));

        //Naechsten Cluster ausfindig machen
        cluster_id = fat_get_fat_entry(cluster_id);
    }

    return cnt;
}


/**
 *
 */
bool fat_save_clusters(dword cluster_id, void* src, size_t size)
{
    DEBUG_MSG("anfang");
    
    dword cluster_count = size / current_rootdir_handle->bios_parameter_block.bytes_per_sector;
    size_t size_done = 0;
    
    if((size % current_rootdir_handle->bios_parameter_block.bytes_per_sector) != 0)
    {
        cluster_count++;
    }
    
    //Das RootDir muss speziell verarbeitet werden
    if(cluster_id == 1)
    {
        DEBUG_MSG("Lese Rootdir");
        cluster_id = (current_rootdir_handle->bios_parameter_block.reserved_sector_count + 
            current_rootdir_handle->bios_parameter_block.fat_count * current_rootdir_handle->bios_parameter_block.fat_sector_count);

        fseek(current_pipe_source, cluster_id * 512, SEEK_SET);
        DEBUG_MSG("Seek abgeschlossen");
        fwrite(src, 1, size, current_pipe_source);
        DEBUG_MSG("ende");
        return TRUE;
    }
    

    dword i;
    dword last_cluster_id = 0;
    
    for(i = 0; i < cluster_count; i++)
    {
        size_t size_current;
        if((size_done + current_rootdir_handle->bios_parameter_block.bytes_per_sector) > size)
        {
            size_current = size - (i * current_rootdir_handle->bios_parameter_block.bytes_per_sector);
        }
        else
        {
            size_current = current_rootdir_handle->bios_parameter_block.bytes_per_sector;
        }
        
        //Cluster schreiben
        fat_write_cluster(cluster_id, ((void*) ((dword)src + size_done)));

        last_cluster_id = cluster_id;
        cluster_id = fat_get_fat_entry(cluster_id);
        
        
        //Falls der Schreibvorgang noch nicht vollendet ist, aber der letzte
        // eingetragene Cluster schon geschrieben worden ist, muss ein neuer
        // Cluster allokiert wird.
        if((i < (cluster_count - 1)) && (cluster_id == 0xfff))
        {
            //Einen weiteren Cluster allokieren
            cluster_id = fat_allocate_cluster();
            
            if(cluster_id == 0)
            {
                //FIXME: Sollte nicht haengen bleiben, sondern Fehler zurueckgeben
                printf("Kein freier Cluster mehr vorhanden!");
                while(TRUE) asm("nop");
            }

            //Eintraege in der FAT richtig setzen
            fat_set_fat_entry(last_cluster_id, cluster_id);
            fat_set_fat_entry(cluster_id, 0xfff);
        }

        size_done += size_current;
    }
    DEBUG_MSG("ende");
    return TRUE;
}

