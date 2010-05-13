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
#include "init.h"


/*fat_rootdir_handle_t* current_rootdir_handle;
io_resource_t* current_pipe_source;
*/
int main(int argc, char* argv[])
{
    lostio_init();
    lostio_type_directory_use();

    typehandle_t* typehandle    = malloc(sizeof(typehandle_t));
    typehandle->id              = 0;
    typehandle->not_found       = &fat_res_not_found_handler;
    typehandle->pre_open        = &fat_res_pre_open_handler;
    typehandle->post_open       = NULL;
    typehandle->read            = NULL;
    typehandle->write           = NULL;
    typehandle->seek            = NULL;
    typehandle->close           = NULL;
    typehandle->link            = NULL;
    typehandle->unlink          = NULL;
    lostio_register_typehandle(typehandle);

    //Den seek-handler aus Ramfile benutzen
    lostio_type_ramfile_use_as(FAT_LOSTIO_TYPE_FILE);
    typehandle = get_typehandle(FAT_LOSTIO_TYPE_FILE);
    typehandle->not_found       = NULL;
    typehandle->pre_open        = &fat_res_pre_open_handler;
    typehandle->post_open       = &fat_res_post_open_handler;
    typehandle->read            = &fat_file_read_handler;
    typehandle->write           = &fat_file_write_handler;
    typehandle->close           = &fat_file_close_handler;
    typehandle->seek            = &fat_file_seek_handler;

    //Die Directoryhandler auch fuer FAT-Verzeichnisse benutzen.
    //Dafuer jedoch in einer leicht modifizierten Variante;
    lostio_type_directory_use_as(FAT_LOSTIO_TYPE_DIR);
    get_typehandle(FAT_LOSTIO_TYPE_DIR)->pre_open = &fat_res_pre_open_handler;
    get_typehandle(FAT_LOSTIO_TYPE_DIR)->not_found = &fat_res_not_found_handler;
    get_typehandle(FAT_LOSTIO_TYPE_DIR)->post_open = &fat_res_post_open_handler;
    get_typehandle(FAT_LOSTIO_TYPE_DIR)->close = &fat_directory_close_handler;
    
    

    //Hier wird der Typ des rootdirs auf 0 gesetzt, damit die richtigen Handler
    //aufgerufen werden.
    vfstree_get_node_by_path("/")->type = FAT_LOSTIO_TYPE_DIR;

    
    init_service_register("fat");

    while(true) {
        wait_for_rpc();
    }
}


