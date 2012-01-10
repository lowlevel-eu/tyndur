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

/*! \addtogroup LostIO
 * @{
 */

#ifndef _LOSTIO_INTERNAL_H_
#define _LOSTIO_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "collections.h"
#include "io.h"
#include "lostio.h"


extern vfstree_node_t  vfstree_root;
extern list_t*         filehandles;
extern list_t*         lostio_types;


///RPC-Handler fuer ein open
void    rpc_io_open(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

///RPC-Handler fuer ein close
void    rpc_io_close(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

///RPC-Handler fuer ein read
void    rpc_io_read(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);
//
///RPC-Handler fuer ein readahead
void    rpc_io_readahead(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

///RPC-Handler fuer ein write
void    rpc_io_write(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

///RPC-Handler fuer ein seek
void    rpc_io_seek(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

///RPC-Handler fuer ein eof
void    rpc_io_eof(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

///RPC-Handler fuer ein tell
void    rpc_io_tell(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

/// RPC-Handler zum erstellen eines Links
void    rpc_io_link(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

/// RPC-Handler zum loeschen eines Links
void    rpc_io_unlink(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);


///Knoten loeschen
//bool vfstree_delete_node(char* path);



///Filehandle aus der Liste holen
lostio_filehandle_t* get_filehandle(pid_t pid, uint32_t id);

///Verarbeiten von Anfragen fuer synchrones Lesen
void lostio_sync_dispatch(void);

/*! @} */

#endif

