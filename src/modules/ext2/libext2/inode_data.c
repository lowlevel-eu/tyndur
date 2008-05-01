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

/**
 * Holt eine Blocknummer aus einem indirekten Block
 * 
 * @param indir_block Blocknummer des Indirekten Block
 * @param offset Abstand des gewuenschten Blockes vom Anfang des indirekten
 *               Blocks (index innerhalb des indirekten Blocks).
 *
 * @return Gewuenschte Blocknummer
 */
static inline uint32_t ext2_inode_single_indir_blockno(
    ext2_root_handle_t* root_handle, uint32_t indir_block, uint32_t offset)
{
    // Indirekten Block einlesen
    uint32_t indir_buff[ext2_block_size(root_handle) / sizeof(uint32_t)];
    ext2_block_read(root_handle, indir_block, 1, indir_buff);
    
    /*printf("indir_buff[%d-4] = %d\n", offset, indir_buff[offset-4]);
    printf("indir_buff[%d-3] = %d\n", offset, indir_buff[offset-3]);
    printf("indir_buff[%d-2] = %d\n", offset, indir_buff[offset-2]);
    printf("indir_buff[%d-1] = %d\n", offset, indir_buff[offset-1]);
    printf("indir_buff[%d] = %d\n", offset, indir_buff[offset]);
    printf("indir_buff[%d+1] = %d\n", offset, indir_buff[offset+1]);*/
    return indir_buff[offset];
}

/**
 * Schreibt eine Blocknummer in einen Indirekten Block
 * 
 * @param indir_block Blocknummer des Indirekten Block
 * @param offset Abstand des gewuenschten Blockes vom Anfang des indirekten
 *               Blocks (index innerhalb des indirekten Blocks).
 * @param block Zu schreibende Blocknummer
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
static inline int ext2_inode_set_indir_blockno(ext2_root_handle_t* root_handle,
    uint32_t indir_block, uint32_t offset, uint32_t block)
{
    // Indirekten Block einlesen
    uint32_t indir_buff[ext2_block_size(root_handle) / sizeof(uint32_t)];
    if (ext2_block_read(root_handle, indir_block, 1, indir_buff) != 0) {
        return -1;
    }
    
    // Block setzen
    indir_buff[offset] = block;

    // Indirekten Block wieder schreiben
    if (ext2_block_write(root_handle, indir_block, 1, indir_buff) != 0) {
        return -2;
    }

    return 0;
}

/**
 * Anzahl der Blocks in einem indirekten
 * 
 * @param level Gibt an, wieviel fach der Block indirekt ist 
 * 
 * @return Anzahl
 */
static inline uint32_t ext2_inode_blocks_indir(ext2_root_handle_t* root_handle,
    int level)
{
    // Soviele Blocknummern sind in einem indirekten Block
    uint32_t per_level = ext2_block_size(root_handle) / sizeof(uint32_t);
    uint32_t single_indir = 1;
    
    // Potenzieren
    // FIXME: Libc?
    while (level-- > 0) {
        single_indir *= per_level;
    }
    
    return single_indir;
}

/**
 * Index auf fuer die verschiedenen Ebenen von indirekten Blocks berechnen
 *
 * @param block Blocknummer
 * @param level Gesucher level
 *
 * @return Index
 */
static inline uint32_t ext2_inode_block_indir_index(
    ext2_root_handle_t* root_handle, uint32_t block, int level)
{
    return (block % ext2_inode_blocks_indir(root_handle, level)) /
        ext2_inode_blocks_indir(root_handle, level - 1);
}

/**
 * Herausfinden wie oft ein Block indirekt ist
 * 
 * @param block Index im Inode
 *
 * @return Anzahl
 */
static int ext2_inode_block_level(ext2_root_handle_t* root_handle,
    uint32_t block)
{
    if (block < EXT2_DIR_BLOCKS) {
        // Direkt
        return 0;
    } else if (block < ext2_inode_blocks_indir(root_handle, 1) +
        EXT2_DIR_BLOCKS)
    {
        // Einfach indirekt
        return 1;
    } else if (block < ext2_inode_blocks_indir(root_handle, 2) +
        ext2_inode_blocks_indir(root_handle, 1) + EXT2_DIR_BLOCKS)
    {
        // Doppelt indirekt
        return 2;
    } else {
        // Dreifach indirekt
        return 3;
    }
}

/**
 * Block in einem Inode allozieren
 *
 * @param inode Pointer auf die Inode-Struktur
 * @param block Index im Inode
 *
 * @return 0 
 */
static int ext2_inode_block_real(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode, uint32_t block, uint32_t* dest_block, int allocate)
{
    uint32_t offset;
    uint32_t indir_index;
    uint32_t indir_block;
    uint32_t prev_indir_block;
    int i;
    int level;
    uint32_t inode_blocksize = ext2_block_size(root_handle) / 512;
    
    //printf("block_real: %d", block);
    
    // Zuerst muss entschieden werden, ob und um einen wieviel fach indirekten
    // Block handelt.
    level = ext2_inode_block_level(root_handle, block);
    switch (level) {
        case 0:
            // Direkter Block: Da ist es ganz einfach, nur das entsprechende
            // Element in der Block-Tabelle setzen
            offset = 0;
            indir_index = block;
            break;

        case 1:
            // Indirekter Block
            offset = block - EXT2_DIR_BLOCKS;
            indir_index = EXT2_INDIR_BLOCK;
            break;

        case 2:
            // Doppelt indirekter Block
            offset = block - EXT2_DIR_BLOCKS - ext2_inode_blocks_indir(
                root_handle, 1);
            indir_index = EXT2_DINDIR_BLOCK;
            break;
        
        default:
            // Dreifach indirekter Block
            offset = block - EXT2_DIR_BLOCKS - ext2_inode_blocks_indir(
                root_handle, 1) - ext2_inode_blocks_indir(root_handle, 2);
            indir_index = EXT2_TINDIR_BLOCK;
            break;
    }
    //printf(" level=%d  offset=%d indir_ind=%d ", level, offset, indir_index);
    // Sicherstellen, dass die Oberste Block-Tabelle verfuegbar ist. Falls sie
    // das nicht ist, wird sie alloziert.
    if ((inode->data->blocks[indir_index] == 0) && (allocate != 0))
    {
        uint32_t prev_data_block;

        // Um eine zusammenhaengende Allokation zu ermoeglichen muss der
        // Allokationsfunktion der vorherige Block uebergeben werden.
        prev_data_block = 0;
        if (indir_index != 0) {
            prev_data_block = inode->data->blocks[indir_index - 1];
        }
        

        indir_block = ext2_block_allocate(root_handle, prev_data_block);
        //printf("toplevel_alloc: %d\n", indir_block);
        if (indir_block == 0) {
            return -1;
        }

        // Der neue Block wird erstmal mit 0 initialisiert, wenn es sich nicht
        // um einen direkten handelt.
        if (level != 0) {
            size_t block_size = ext2_block_size(root_handle);
            uint8_t init_data[block_size];
            memset(init_data, 0, block_size);
            if (ext2_block_write(root_handle, indir_block, 1, init_data) != 0)
            {
                return -2;
            }
        }
       
        inode->data->block_count += inode_blocksize; 
        inode->data->blocks[indir_index] = indir_block;
        inode->modified = 1;
    } else if (inode->data->blocks[indir_index] == 0) {
        // Falls der Block noch nicht alloziert ist, und auch nicht alloziert
        // werden darf, muss abgebrochen werden
        return 1;
    } else {
        indir_block = inode->data->blocks[indir_index];
    }
    
    for (i = level - 1;  i >= 0; i--) {
        // Index in der Tabelle auf der aktuellen Ebene errechnen
        indir_index = ext2_inode_block_indir_index(root_handle, offset, i + 1);
        
        // Blocknummer des aktuellen indirekten Blocks speichern, da sie
        // notwendig ist, um spaeter die neue Blocknummer einzutragen, falls
        // der Block alloziert werden muss.
        prev_indir_block = indir_block;

        indir_block = ext2_inode_single_indir_blockno(root_handle, indir_block,
            indir_index);
        //printf("  level=%d  ind_block=%d  index=%d  block=%d\n", i + 1,
        //  prev_indir_block, indir_index, indir_block);
        // Wenn eine Blocknummer von 0 eingetragen ist, wurde der Block bis
        // jetzt noch nicht benutzt und muss alloziert werden.
        if ((indir_block == 0) && (allocate == 1)) {
            uint32_t prev_data_block = 0;
            uint32_t new_block;

            // Block mit nutzdaten
            if (i == 0) {
                ext2_inode_block_real(root_handle, inode, block - 1,
                    &prev_data_block, 0);
            }

            new_block = ext2_block_allocate(root_handle, prev_data_block);
            if (new_block == 0) {
                // Bereits allozierte Bloecke sind jetzt halt noch eingetragen,
                // was aber nicht weiter tragisch ist, da sie ja
                // bei weiteren Zugriffen benutzt werden koennen.
                return -1;
            }

            // Der neue Block wird erstmal mit 0 initialisiert, aber nur, wenn
            // es sich nicht um einen Nutzdatenblock handelt.
            if (i != 0) {
                size_t block_size = ext2_block_size(root_handle);
                uint8_t init_data[block_size];
                memset(init_data, 0, block_size);
                if (ext2_block_write(root_handle, new_block, 1, init_data)
                    !=0)
                {
                    return -2;
                }
            }

            inode->data->block_count += inode_blocksize;
            inode->modified = 1;

            // Blocknummer in der Tabelle eintragen
            ext2_inode_set_indir_blockno(root_handle, prev_indir_block,
                indir_index, new_block);
            indir_block = new_block;
        } else if ((indir_block == 0) && (allocate == 0)) {
            // Block muesste alloziert werden
            return 1;
        }
    }
    
    *dest_block = indir_block;
    return 0;
}

/**
 * Block aus einem Inode einlesen
 * 
 * @param inode Pointer auf die Inode-Struktur
 * @param block Erste Blocknummer (Index im Inode)
 * @param count Anzahl der Bloecke
 * @param buffer Pointer auf den Puffer
 *
 * @return Anzahl der geladenen Bloecke
 */
static uint32_t ext2_inode_blocks_read(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode, uint32_t block, uint32_t count, void* buffer)
{
    uint32_t i;
    uint32_t continous_blocks;
    uint32_t blocks_left = count;
    uint32_t current_block = block;

    while (blocks_left > 0) {
        uint32_t first_num = 0;
        uint32_t prev_num = 0;
        uint32_t num;
        int exit_error = 0;
        
        continous_blocks = 0;
        for (i = 0; i < blocks_left; i++) {
            int result = ext2_inode_block_real(root_handle, inode,
                current_block + i, &num, 0);

            // Wenn der block nicht existiert, wird abgebrochen
            if (result < 0) {
                exit_error = 1;
                break;
            } if ((continous_blocks == 0) && (result > 0)) {
                // Luecke => Nullen beim Lesen
                num = 0;
            } else if ((continous_blocks != 0) && (result > 0)) {
                break;
            }


            if (i == 0) {
                // Der erste Block kriegt eine spezialbehandlung ;-)
                first_num = num;
            } else if (num - 1 != prev_num) {
                // Nicht mehr zusammenhaengend
                break;
            }

            continous_blocks++;
            prev_num = num;
        }

        // Wenn kein Block gelesen werden kann, wird abgebrochen
        if (exit_error != 0) {
            break;
        }
        
        // Pruefen ob der Block mit Nullen gefuellt, oder eingelesen werden
        // soll
        if (num == 0) {
            memset(buffer, 0, continous_blocks * ext2_block_size(root_handle));
        } else {
            // Sonst wird er jetzt eingelesen
            int result = ext2_block_read(root_handle, first_num,
                continous_blocks, buffer);
        
            // Pruefen ob das geklappt hat, wenn nicht ist hier Enstation
            if (result != 0) {
                break;
            }
        }

        // Sonst wird der Buffer-Pointer noch ein Wenig nach vorne auf den
        // naechsten Block geschoben
        buffer = (void*) ((uintptr_t) buffer + ext2_block_size(root_handle) *
            continous_blocks);
        blocks_left -= continous_blocks;
        current_block += continous_blocks;
    }
    return count - blocks_left;
}

/**
 * Block in ein Inode schreiben
 * 
 * @param inode_struct Pointer auf die Inode-Struktur
 * @param block Erste Blocknummer (Index im Inode)
 * @param count Anzahl der Bloecke
 * @param buffer Pointer auf den Puffer
 *
 * @return Anzahl der geschriebenen Bloecke
 */
/*static uint32_t ext2_inode_blocks_write(ext2_root_handle_t* root_handle,
    ext2_inode_t* inode_struct, uint32_t block, uint32_t count, void* buffer)
{
    uint32_t i;
    int result;
    uint32_t num;
    for (i = 0; i < count; i++) {
        // Reale Blocknummer holen
        result = ext2_inode_block_real(root_handle, inode_struct, block + i,
            &num, 1);
        printf("block_alloc: %d\n", num);
        // Wenn das nicht geklappt hat, sind keine freien Bloecke mehr
        // vorhanden
        if (result != 0) {
            print_error("Block konnte nicht alloziert werden");
            return i;
        }

        // Sonst wird er jetzt geschrieben
        result = ext2_block_write(root_handle, num, 1, buffer);

        // Pruefen ob das geklappt hat, wenn nicht ist hier Enstation
        if (result != 0) {
            return i;
        }

        // Sonst wird der Buffer-Pointer noch ein Wenig nach vorne auf den
        // naechsten Block geschoben
        buffer = (void*) ((uintptr_t) buffer + ext2_block_size(root_handle));
    }
    return i;
}*/
static uint32_t ext2_inode_blocks_write(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode, uint32_t block, uint32_t count,
    const void* buffer)
{
    uint32_t i;
    uint32_t continous_blocks;
    uint32_t blocks_left = count;
    uint32_t current_block = block;
    
    while (blocks_left > 0) {
        uint32_t first_num = 0;
        uint32_t prev_num = 0;
        uint32_t num;
        int exit_error = 0;
        int result;
        
        continous_blocks = 0;
        for (i = 0; i < blocks_left; i++) {
            result = ext2_inode_block_real(root_handle, inode, current_block +
                i, &num, 1);

            // Wenn der block nicht existiert, wird abgebrochen
            if (result < 0) {
                exit_error = 1;
                break;
            } else if ((continous_blocks != 0) && (result > 0)) {
                break;
            }

            if (i == 0) {
                // Der erste Block kriegt eine spezialbehandlung ;-)
                first_num = num;
            } else if (num - 1 != prev_num) {
                // Nicht mehr zusammenhaengend
                break;
            }

            continous_blocks++;
            prev_num = num;
        }

        // Wenn kein Block geschrieben werden kann, wird abgebrochen
        if (exit_error != 0) {
            break;
        }
        
        // Sonst wird er jetzt geschrieben
        result = ext2_block_write(root_handle, first_num, continous_blocks,
            buffer);
    
        // Pruefen ob das geklappt hat, wenn nicht ist hier Enstation
        if (result != 0) {
            break;
        }
        

        // Sonst wird der Buffer-Pointer noch ein Wenig nach vorne auf den
        // naechsten Block geschoben
        buffer = (void*) ((uintptr_t) buffer + ext2_block_size(root_handle) *
            continous_blocks);
        blocks_left -= continous_blocks;
        current_block += continous_blocks;
    }

    return count - blocks_left;
}

/**
 * Teil eines Datenblocks aus einem Inode lesen
 *
 * @param inode Pointer auf die Inode-Struktur
 * @param block Blocknummer (Index im Inode)
 * @param offset Offset von dem an der Block kopiert werden soll
 * @param size Anzahl der Bytes die ab diesem Offset kopiert werden (wird
 *             zugeschnitten, wenn zu gross)
 * @param buffer Hier werden die Daten reinkopiert
 *
 * @return size bei Erfolg, 0 im Fehlerfall
 */
static inline size_t ext2_inode_block_read_part(
    ext2_root_handle_t* root_handle, ext2_ih_t* inode, uint32_t block,
    size_t offset, size_t size, void* buffer)
{
    uint8_t full_buffer[ext2_block_size(root_handle)];
    uint32_t result;

    // Block zuerst in den lokalen Buffer laden und danach den gewuenschten
    // Teil kopieren
    result = ext2_inode_blocks_read(root_handle, inode, block, 1,
        &full_buffer);

    // Falls das laden nicht geklappt hat, ist hier schon Endstation
    if (result == 0) {
        return 0;
    }
    
    // Wenn nicht, werden die Daten jetzt in den Buffer kopiert
    memcpy(buffer, &(full_buffer[offset]), size); 
    return size;
}

/**
 * Teil eines Datenblocks in ein Inode speichern
 *
 * @param inode Pointer auf die Inode-Struktur
 * @param block Blocknummer (Index im Inode)
 * @param offset Offset an den der die Daten kopiert werden sollen
 * @param size Anzahl der Bytes die an diesen Offset kopiert werden (wird
 *             zugeschnitten, wenn zu gross)
 * @param buffer Hier werden die Daten reinkopiert
 *
 * @return size bei Erfolg, 0 im Fehlerfall
 */
static inline size_t ext2_inode_block_write_part(
    ext2_root_handle_t* root_handle, ext2_ih_t* inode, uint32_t block,
    size_t offset, size_t size, const void* buffer)
{
    uint8_t full_buffer[ext2_block_size(root_handle)];
    uint32_t result;

    // Block zuerst in den lokalen Buffer laden und danach den gewuenschten
    // Teil kopieren und den Block wieder speichern
    result = ext2_inode_blocks_read(root_handle, inode, block, 1,
        &full_buffer);

    // Falls das laden nicht geklappt hat, ist hier schon Endstation
    if (result == 0) {
        return 0;
    }
    
    // Wenn nicht, werden die Daten jetzt aus dem Buffer kopiert
    memcpy(&(full_buffer[offset]), buffer, size); 

    // Block speichern
    result = ext2_inode_blocks_write(root_handle, inode, block, 1,
        &full_buffer);
    if (result != 1) {
        return 0;
    }

    return size;
}

/**
 * Nutzdaten aus einem Inode Lesen
 *
 * @param inode Pointer auf die Inode-Struktur
 * @param start Adresse an der mit lesen angefangen werden soll (in Bytes)
 * @param size Groesse der zu lesenden Daten
 * @param buffer Puffer in den die Daten gespeichert werden sollen
 *
 * @return Anzahl der gelesenen Bytes
 */
size_t ext2_inode_data_read(ext2_root_handle_t* root_handle, ext2_ih_t* inode,
    uint64_t start, size_t size, void* buffer)
{
    size_t result = 0;
    uint32_t read_bytes = 0;
    uint32_t size_todo = size;
    // Start- und Endblock (also Blockindex im Inode) und Offset zu den
    // Blcokgrenzen errechnen.
    uint32_t start_block = start / ext2_block_size(root_handle);
    uint32_t start_offset = start % ext2_block_size(root_handle);
    uint32_t end_block = (start + size) / ext2_block_size(root_handle);
    uint32_t end_offset = (start + size) % ext2_block_size(root_handle);
    
    // Wenn start nicht auf einer Blockgrenze liegt, muss es einzeln eingelesen
    // werden
    if ((start_offset != 0) || (size < ext2_block_size(root_handle))) {
        uint32_t start_size = ext2_block_size(root_handle) - start_offset;
        if (start_size > size) {
            start_size = size;
        }

        // Teil einlesen
        result = ext2_inode_block_read_part(root_handle, inode, start_block,
            start_offset, start_size, buffer);
        
        // Wenn das nicht geklappt hat, wird abgebrochen
        if (result < start_size) {
            return result;
        }

        // Der Start-Block ist nun natuerlich nicht mehr der selbe
        start_block++;
        // Mit dem Buffer verhält es sich genau gleich
        buffer = (void*) ((uintptr_t) buffer + start_size);
        size_todo -= start_size;
    }

    // Wenn weniger als ein ganzer Block gelesen wurde, kann es ein, dass wir
    // schon am Ende angelangt sind
    if (result >= size) {
        return result;
    }

    // Die restlichen Blocks einlesen
    read_bytes = ext2_block_size(root_handle) * ext2_inode_blocks_read(
        root_handle, inode, start_block, size_todo / ext2_block_size(
        root_handle), buffer);
    result += read_bytes;

    // Pruefen ob die Blocks komplett gelesen wurden
    if (result + end_offset != size) {
        // Wenn das nicht der Fall ist, wird der End-Block nicht mehr gelesen
        return result;
    }
    
    // Buffer wieder umbiegen
    buffer = (void*) ((uintptr_t) buffer + read_bytes);

    // Falls noch ein paar zerquetschte Bytes da sind werden die noch in den
    // letzten Block geschrieben
    if (end_offset != 0) {
        result += ext2_inode_block_read_part(root_handle, inode, end_block, 0,
            end_offset, buffer);
    }
    
    return result;
}

/**
 * Block in einem Inode freigeben
 *
 * @param inode Pointer auf die Inode-Struktur
 * @param block Index im Inode
 *
 * @return 0 
 */
static int ext2_inode_block_free(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode, uint32_t block)
{
    uint32_t offset;
    uint32_t indir_index;
    uint32_t indir_block;
    uint32_t inode_blocksize = ext2_block_size(root_handle) / 512;
    int level;
    
    // Zuerst muss entschieden werden, ob und um einen wieviel fach indirekten
    // Block handelt, und die Startwerte entsprechend gestezt werden.
    level = ext2_inode_block_level(root_handle, block);
    switch (level) {
        case 0:
            // Direkter Block
            offset = 0;
            indir_index = block;
            break;

        case 1:
            // Indirekter Block
            offset = block - EXT2_DIR_BLOCKS;
            indir_index = EXT2_INDIR_BLOCK;
            break;

        case 2:
            // Doppelt indirekter Block
            offset = block - EXT2_DIR_BLOCKS - ext2_inode_blocks_indir(
                root_handle, 1);
            indir_index = EXT2_DINDIR_BLOCK;
            break;
        
        default:
            // Dreifach indirekter Block
            offset = block - EXT2_DIR_BLOCKS - ext2_inode_blocks_indir(
                root_handle, 1) - ext2_inode_blocks_indir(root_handle, 2);
            indir_index = EXT2_TINDIR_BLOCK;
            break;
    }
    
    // Oberste Blocktabelle freigeben, oder falls noch Nutzdaten vorhanden
    // sind, Blocknummer des untersten Blockes auslesen.
    if (offset == 0) {
        indir_block = inode->data->blocks[indir_index];
        
        // Diese darf nicht 0 sein...
        if (indir_block == 0) {
            print_error("Block Nummer 0 soll aus Inode freigegeben werden");
            return -3;
        }

        // Block freigeben
        if (ext2_block_free(root_handle, indir_block) != 0) {
            return -1;
        }

        inode->data->block_count -= inode_blocksize;
        inode->data->blocks[indir_index] = 0;
        inode->modified = 1;
    } else {
        // In dem Indirekten Block sind noch Nutzdatenblocks vorhanden.
        // Deswegen wird die Blocknummer nur ausgelesen und nicht freigegeben.
        indir_block = inode->data->blocks[indir_index];
    }
    

    int i;
    uint32_t prev_indir_block;
    // Jetzt werden indirekte Blocks Ebene fuer Ebene abgearbeitet. Begonnen
    // wird auf der obersten Ebene, also da wo die Blocks mit den Nutzdaten
    // eingetragen sind.
    for (i = level - 1;  i >= 0; i--) {
        // Offset im aktuellen indirekten Block berechnen, damit die
        // Blocknummer ausgelesen und ueberschrieben werden kann.
        indir_index = ext2_inode_block_indir_index(root_handle, offset, i + 1);

        // Die Blocknummer des aktuellen indirekten Blocks muss
        // gespeichert werden, damit der aktuelle Eintrag ueberschrieben werden
        // kann.
        prev_indir_block = indir_block;

        // Die neue Blocknummer aus dem indirekten Block auslesen
        indir_block = ext2_inode_single_indir_blockno(root_handle, indir_block,
            indir_index);
        
        // Sparse
        if (indir_block == 0) {
            break;
        }
        
        // Hier wird entschieden, ob der aktuelle Block freigegeben werden
        // muss. Das geschieht unter den folgenden Bedingungen:
        if (
            // Wenn es sich um den letzten Block in der Hirarchie handelt
            (i == 0) ||
            
            // Wenn es sich um den zweitletzten handelt und keine weiteren
            // Blocks mehr eingetragen sind
            ((i == 1) &&
            (ext2_inode_block_indir_index(root_handle,offset, 1) == 0)) ||
            
            // Und wenn es sich um den drittletzten handelt, und auch keine
            // Blocks  mehr eingetragen sind
            ((level == 2) &&
            (ext2_inode_block_indir_index(root_handle, offset, 1) == 0) &&
            (ext2_inode_block_indir_index(root_handle, offset, 2) == 0)))
        {
            // Anzahl belegter Blocks im Inode aktualisieren
            inode->data->block_count -= inode_blocksize;
            inode->modified = 1;

            // Block freigeben
            if (ext2_block_free(root_handle, indir_block) != 0) {
                // TODO: Kann hier noch was etwas gerettet werden?
                return -2;
            }

            // Blocknummer in der Tabelle mit 0 ueberschreiben, damit sie nicht
            // als benutzt erkannt wird.
            ext2_inode_set_indir_blockno(root_handle, prev_indir_block,
                indir_index, 0);
        }
    }
    
    // Huch, das waere geschafft. ;-)
    return 0;
}


/**
 * Nutzdatengroesse im Inode anpassen
 *
 * @param inode Inode-Struktur
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall
 */
int ext2_inode_data_truncate(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode, uint64_t new_size)
{
    uint32_t old_block_count;
    uint32_t block_count;
    uint32_t i;

    old_block_count = ext2_block_count(root_handle, inode->data->size);
    // Neue Block-Anzahl
    block_count = ext2_block_count(root_handle, new_size);
    
    // Wenn die Daten verkleinert werden, muessen die freien Blocks freigegeben
    // werden.
    if (block_count < old_block_count) {
        // Zu loeschende Bloecke rueckwaerts durchgehen
        i = old_block_count - 1;
        for (; (i + 1) > block_count; i--) {
            if (ext2_inode_block_free(root_handle, inode, i) != 0) {
                print_error2("Fehler beim Freigeben des Blocks", i);
                return -2;
            }
        }
    }
    inode->data->size = new_size;
    inode->modified = 1;
    return 0;
}

/**
 * Nutzdaten im Inode speichern
 *
 * @param inode Inode-Struktur
 * @param start Position an der mit Schreiben angefangen werden soll
 * @param size Anzahl der Bytes die geschrieben werden sollen
 * @param buffer Puffer aus dem die Daten gelesen werden sollen
 *
 * @return Anzahl der geschribenen Bytes
 */
size_t ext2_inode_data_write(ext2_root_handle_t* root_handle,
    ext2_ih_t* inode, uint64_t start, size_t size,
    const void* buffer)
{
    size_t result = 0;
    uint32_t read_bytes = 0;
    uint32_t size_todo = size;

    // Start- und Endblock (also Blockindex im Inode) und Offset zu den
    // Blcokgrenzen errechnen.
    uint32_t start_block = start / ext2_block_size(root_handle);
    uint32_t start_offset = start % ext2_block_size(root_handle);
    uint32_t end_block = (start + size) / ext2_block_size(root_handle);
    uint32_t end_offset = (start + size) % ext2_block_size(root_handle);
    
    // Wenn start nicht auf einer Blockgrenze liegt, muss es einzeln
    // geschrieben werden
    if ((start_offset != 0) || (size < ext2_block_size(root_handle))) {
        uint32_t start_size = ext2_block_size(root_handle) - start_offset;
        if (start_size > size) {
            start_size = size;
        }

        // Teil schreiben
        result = ext2_inode_block_write_part(root_handle, inode, start_block,
            start_offset, start_size, buffer);
        
        // Wenn das nicht geklappt hat, wird abgebrochen
        if (result < start_size) {
            // Wenn vergroessert wurde, wird das jetzt im Inode eingetragen
            if (start + result > inode->data->size) {
                inode->data->size = start + result; 
                inode->modified = 1;
            }
            return result;
        }

        // Der Start-Block ist nun natuerlich nicht mehr der selbe
        start_block++;
        // Mit dem Buffer verhält es sich genau gleich
        buffer = (void*) ((uintptr_t) buffer + start_size);
        size_todo -= start_size;
    }

    // Wenn weniger als ein ganzer Block gelesen wurde, kann es ein, dass wir
    // schon am Ende angelangt sind
    if (result >= size) {
        // Wenn vergroessert wurde, wird das jetzt im Inode eingetragen
        if (start + result > inode->data->size) {
            inode->data->size = start + result;
            inode->modified = 1;
        }
        return result;
    }

    // Die restlichen Blocks schreiben
    read_bytes = ext2_block_size(root_handle) * ext2_inode_blocks_write(
        root_handle, inode, start_block, size_todo / ext2_block_size(
        root_handle), buffer);
    result += read_bytes;

    // Pruefen ob die Blocks komplett geschriben wurden
    if (result + end_offset != size) {
        // Wenn das nicht der Fall ist, wird der End-Block nicht mehr
        // geschrieben

        // Wenn vergroessert wurde, wird das jetzt im Inode eingetragen
        if (start + result > inode->data->size) {
            inode->data->size = start + result;
            inode->modified = 1;
        }

        return result;
    }
    
    // Buffer wieder umbiegen
    buffer = (void*) ((uintptr_t) buffer + read_bytes);

    // Falls noch ein paar zerquetschte Bytes da sind werden die noch in den
    // letzten Block geschrieben
    if (end_offset != 0) {
        result += ext2_inode_block_write_part(root_handle, inode, end_block, 0,
            end_offset, buffer);
    }
    
    // Wenn vergroessert wurde, wird das jetzt im Inode eingetragen
    if (start + result > inode->data->size) {
        inode->data->size = start + result;
        inode->modified = 1;
    }

    return result;
}

