/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#ifndef _LOSTIO_IF_H_
#define _LOSTIO_IF_H_

#include <lostio.h>

#include "main.h"


#define LOSTIO_TYPES_TCPPORT 255
#define LOSTIO_TYPES_NETCONFIG 254
#define LOSTIO_TYPES_DNS 253
#define LOSTIO_TYPES_ROUTE 252
#define LOSTIO_TYPES_SERVER  251
#define LOSTIO_TYPES_SERVER_DIR  250
#define LOSTIO_TYPES_SERVER_CONN 249
#define LOSTIO_TYPES_UDP_DIR 248
#define LOSTIO_TYPES_UDP 247
#define LOSTIO_TYPES_DHCP 246


void init_lostio_interface(void);
void lostio_add_driver(struct driver *driver);
void lostio_add_device(struct device *device);

size_t lostio_tcp_read(lostio_filehandle_t* fh, void* buf,
    size_t blocksize, size_t count);
size_t lostio_tcp_write
    (lostio_filehandle_t* fh, size_t blocksize, size_t count, void* data);
int lostio_tcp_close(lostio_filehandle_t* filehandle);



#endif
