/*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
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

#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ports.h>
#include <services.h>
#include <errno.h>
#include <testlib.h>

const char* test_name;

static void rw(lio_stream_t s, int64_t offset, size_t size, void* buf,
    void *rbuf, bool write)
{
    int ret;

    if (write) {
        ret = lio_pwrite(s, offset, size, buf);
        test_assert(ret == size);
    } else {
        ret = lio_pread(s, offset, size, rbuf);
        printf("read: %llx + %x:'%d [%llx...]\n",
            offset, size, ret, *(uint64_t*)rbuf);
        test_assert(ret == size);
        test_assert(memcmp(buf, rbuf, size) == 0);

    }
}

static void test_1_2_rw(bool write, const char* _test_name)
{
    lio_resource_t tmp_root;
    lio_resource_t tmp_file;
    lio_stream_t s;
    uint8_t* buf = malloc(512 * 1024);
    uint8_t* rbuf = malloc(512 * 1024);
    int ret;
    int i;

    test_name = _test_name;
    printf("start %s\n", test_name);

    /* Testdatei anlegen und öffnen */
    if (write) {
        tmp_root = lio_resource("ata:/ata00|ext2:/", false);
        test_assert(tmp_root >= 0);

        tmp_file = lio_mkfile(tmp_root, "test");
        test_assert(tmp_file >= 0);
    } else {
        tmp_file = lio_resource("ata:/ata00|ext2:/test", false);
        test_assert(tmp_file >= 0);
    }
    printf("have res\n");

    s = lio_open(tmp_file, LIO_READ | LIO_WRITE);
    test_assert(s >= 0);

    printf("starte rw...\n");
    /* Puffer befuellen */
    for (i = 0; i < 512 * 1024; i++) {
        buf[i] = (19 + i) % 43;
    }

    /* 1. Durchlauf */
    for (i = 0; i < 16; i++) {
        rw(s,
            384 + i * 1536 * 1024,
            512 * 1024,
            buf, rbuf, write);
    }

    /* 2. Durchlauf */
    for (i = 0; i < 16; i++) {
        rw(s,
            384 + 1024 * 1024 + i * 1536 * 1024,
            512 * 1024,
            buf, rbuf, write);
    }

    /* 3. Durchlauf */
    for (i = 0; i < 16; i++) {
        rw(s,
            384 + 512 * 1024 + i * 1536 * 1024,
            512 * 1024,
            buf, rbuf, write);
    }

    /* FIXME lio_sync_all() sollte das erledigen. Tut's aber nicht. */
    ret = lio_sync(s);
    test_assert(ret >= 0);

    ret = lio_close(s);
    test_assert(ret >= 0);

    ret = lio_sync_all();
    test_assert(ret >= 0);

    free(buf);
    free(rbuf);

    printf("* PASS %s\n", test_name);
}

static void test1(void)
{
    test_1_2_rw(true, "ext2: Ganz viel write");
}

static void test2(void)
{
    test_1_2_rw(false, "ext2: Ganz viel read nach Reboot");
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("* ERROR Zu wenige Parameter\n");
        quit_qemu();
    }

    servmgr_need("ata");
    servmgr_need("ext2");

    switch (atoi(argv[1])) {
        case 1:
            test1();
            break;
        case 2:
            test2();
            break;

        default:
            printf("* ERROR Unbekannter Testfall\n");
            break;
    }

    quit_qemu();
}
