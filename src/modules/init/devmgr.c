/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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
#include <rpc.h>
#include <init.h>
#include <collections.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

struct device {
    struct init_dev_desc* public;
};

list_t* devices = NULL;

void rpc_dev_register(pid_t pid, uint32_t cid, size_t size, void* data)
{
    struct init_dev_desc* desc;
    struct device* dev;

    if (size < sizeof(struct init_dev_desc)) {
        rpc_send_int_response(pid, cid, -1);
        return;
    }

    desc = data;
    if (size != desc->bus_data_size + sizeof(struct init_dev_desc)) {
        rpc_send_int_response(pid, cid, -1);
        return;
    }

    desc = malloc(size);
    memcpy(desc, data, size);
    desc->name[sizeof(desc->name) - 1] = '\0';

    dev = malloc(sizeof(*dev));
    dev->public = desc;

    p();
    if (devices == NULL) {
        devices = list_create();
    }

    list_push(devices, dev);
    v();

    rpc_send_int_response(pid, cid, 0);
}

void rpc_dev_list(pid_t pid, uint32_t cid, size_t size, void* data)
{
    size_t bytes = 0;
    int i;
    struct device* dev;

    p();
    for (i = 0; (dev = list_get_element_at(devices, i)); i++) {
        bytes += dev->public->bus_data_size + sizeof(struct init_dev_desc);
    }

    uint8_t buf[bytes];
    size_t pos = 0;

    for (i = 0; (dev = list_get_element_at(devices, i)); i++) {
        size_t s = dev->public->bus_data_size + sizeof(struct init_dev_desc);
        memcpy(&buf[pos], dev->public, s);
        pos += s;
    }
    v();

    rpc_send_response(pid, cid, bytes, (char*) buf);
}
