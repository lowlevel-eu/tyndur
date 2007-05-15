/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Kevin Wolf.
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

#include "stdio.h"
#include "types.h"
#include "ports.h"
#include "stdlib.h"
#include "io.h"
#include "collections.h"
#include "lostio.h"

#include "pci.h"
#include "pcihw.h"

#define _USE_START_
#include "init.h"

int main(int argc, char* argv[])
{
    pci_request_io_ports();

    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();

    scanpci();


    // Nun wird das Ramfile erstellt
    char* pciinfo_buffer = pciinfo();
/*    vfstree_node_t* root_node = vfstree_get_node_by_path("/");
    root_node->type = LOSTIO_TYPES_RAMFILE;
    root_node->data = (void*) pciinfo_buffer;
    root_node->size = strlen(pciinfo_buffer) + 1;*/

    vfstree_create_node("/info", LOSTIO_TYPES_RAMFILE, 
        strlen(pciinfo_buffer) + 1, (void*) pciinfo_buffer, 0);

    init_service_register("pci");
    
    while(1) {
        wait_for_rpc();
    }

    return 0;
}
