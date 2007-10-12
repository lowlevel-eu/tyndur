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
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "io.h"
#include "lostio.h"
#include "ide.h"

//FIXME: Hier sollte ueberall noch eine Datentraegergroesse rein.

read_hdl_reply ide_device_read_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount)
{
    read_hdl_reply reply;
    size_t size = blocksize * blockcount;
    
    /*if(size + filehandle->pos > filehandle->node->size)
    {
        size = filehandle->node->size - filehandle->pos;
    }*/
    
    size_t read_end = size + filehandle->pos;

    //Startsektor berechnen
    qword sector_start = filehandle->pos / IDE_SECTOR_SIZE;
    
    //Sektorenanzahl berechnen
    dword sector_count = (read_end / IDE_SECTOR_SIZE) - (filehandle->pos / 
        IDE_SECTOR_SIZE) + 1;

    //Buffer allokieren
    //FIXME: Bei mehreren gleichzeitigen Zugriffen rummst es hier, oder die
    // Daten vom ersten read gehen verloren.
    static byte* buffer = NULL;
    buffer = realloc(buffer, sector_count * IDE_SECTOR_SIZE);
    
    reply.data = (void*) (buffer + (filehandle->pos % IDE_SECTOR_SIZE));
    reply.size = size;
    if (ide_read_sectors(filehandle->node->data, filehandle->node->size,
        sector_start, sector_count, buffer) == FALSE) {
        //TODO:Set error-Flag
        reply.data = NULL;
        reply.size = 0;
    }
    filehandle->pos += size;

    //Auf EOF ueberpruefen
    /*if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }*/

    return reply;
   
}


size_t ide_drive_write_handler(lostio_filehandle_t* filehandle, size_t blocksize,
    size_t blockcount, void* data)
{
    return 0;
}


int ide_drive_seek_handler(lostio_filehandle_t* filehandle, int offset,
    int origin)
{
    switch(origin)
    {
        case SEEK_SET:
        {
            /*if(offset > filehandle->node->size)
            {
                return -1;
            }*/

            filehandle->pos = offset;
            break;
        }

        case SEEK_CUR:
        {
            /*if((offset + filehandle->pos) > filehandle->node->size)
            {
                return -1;
            }*/
            filehandle->pos += offset;
            break;
        }

        case SEEK_END:
        {
            //filehandle->pos = filehandle->node->size;
            break;
        }
    }
    return 0;
}


read_hdl_reply ide_partition_read_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount)
{
    read_hdl_reply reply;
    size_t size = blocksize * blockcount;

    byte drive_num = filehandle->node->size & 0xffff;
    byte part_num = filehandle->node->size >> 16;

    ide_hw_interface_t* hwif = filehandle->node->data;
    ide_hw_interface_drive_t* drive = &(hwif->drives[drive_num]);
    partition_table_entry_t* pte = list_get_element_at(drive->partitions,
        part_num);

    if(size + filehandle->pos > pte->size * IDE_SECTOR_SIZE)
    {
        size = pte->size * IDE_SECTOR_SIZE - filehandle->pos;
    }
    
    size_t read_end = size + filehandle->pos;

    //Startsektor berechnen
    qword sector_start = filehandle->pos / IDE_SECTOR_SIZE;
    
    //Sektorenanzahl berechnen
    dword sector_count = (read_end / IDE_SECTOR_SIZE) - (filehandle->pos / 
        IDE_SECTOR_SIZE) + 1;

    //Buffer allokieren
    //p();
    //FIXME: Bei mehreren gleichzeitigen Zugriffen rummst es hier, oder die
    // Daten vom ersten read gehen verloren.
    static byte* buffer = NULL;
    buffer = realloc(buffer, sector_count * IDE_SECTOR_SIZE);
    
    reply.data = (void*) (buffer + (filehandle->pos % IDE_SECTOR_SIZE));
    reply.size = size;

    sector_start += pte->lba_start;
    if (ide_read_sectors(hwif, drive_num, (qword)(sector_start), sector_count,
        buffer) == FALSE)
    {
        puts("[   IDE   ] Fehler beim lesen");
        //TODO:Set error-Flag
        reply.data = NULL;
        reply.size = 0;
    }
    /*for (i = 0; i < 32; i++)
    printf(" 0x%x ", buffer[i]);*/
    filehandle->pos += size;

    //Auf EOF ueberpruefen
    if(filehandle->pos == pte->size * IDE_SECTOR_SIZE)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    //v();
    return reply;
   
}


size_t ide_partition_write_handler(lostio_filehandle_t* filehandle, size_t blocksize,
    size_t blockcount, void* data)
{
    return 0;
}


int ide_partition_seek_handler(lostio_filehandle_t* filehandle, int offset,
    int origin)
{
    byte drive_num = filehandle->node->size & 0xffff;
    byte part_num = filehandle->node->size >> 16;

    ide_hw_interface_t* hwif = filehandle->node->data;
    ide_hw_interface_drive_t* drive = &(hwif->drives[drive_num]);
    
    partition_table_entry_t* pte = list_get_element_at(drive->partitions,
        part_num);
    
    switch(origin)
    {
        case SEEK_SET:
        {
            if(offset > pte->size * IDE_SECTOR_SIZE)
            {
                return -1;
            }

            filehandle->pos = offset;
            break;
        }

        case SEEK_CUR:
        {
            if((offset + filehandle->pos) > pte->size * IDE_SECTOR_SIZE)
            {
                return -1;
            }
            filehandle->pos += offset;
            break;
        }

        case SEEK_END:
        {
            filehandle->pos = pte->size * IDE_SECTOR_SIZE;
            break;
        }
    }
    return 0;
}

