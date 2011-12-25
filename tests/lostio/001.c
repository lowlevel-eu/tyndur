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

static const char* test_name;

static void __attribute__((noreturn)) quit_qemu(void)
{
    request_ports(0xb000, 8);
    outw(0xb004, 0x2000);

    /* Wird nie erreicht */
    while(1) {
        asm volatile("ud2");
    }
}

static void _test_assert(bool expr, const char* text, int line)
{
    if (!expr) {
        printf("* FAIL %s: %s [" __FILE__ ":%d]\n", test_name, text, line);
        quit_qemu();
    }
}

#define test_assert(x) \
    _test_assert((x), #x, __LINE__)

static void test_write(lio_stream_t s)
{
    uint8_t* buf;
    int ret;

    /*
     * Sequentiell schreiben:
     * - Ein voller Cache-Cluster
     * - Zwei Cluster
     * - Ein Block
     * - Zwei Blöcke in einem Cluster
     * - Zwei Blöcke in angrenzenden Clustern
     * - Ein halber Block vom Blockanfang
     * - Ein halber Block zum Blockende
     * - Ein halber Block über eine Blockgrenze
     * - Irgendwas ganz krummes über:
     *   - das Ende eines Blocks
     *   - einen vollen Block
     *   - einen vollen Cluster
     *   - nochmal einen vollen Block
     *   - den Anfang eines Blocks
     */
    buf = malloc(2 * 32 * 1024);

    memset(buf, 0x11, 0x10000);
    ret = lio_pwrite(s, 0, 0x8000, buf);
    test_assert(ret == 0x8000);

    memset(buf, 0x22, 0x10000);
    ret = lio_pwrite(s, 0x8000, 0x10000, buf);
    test_assert(ret == 0x10000);

    memset(buf, 0x33, 0x10000);
    ret = lio_pwrite(s, 0x18000, 0x400, buf);
    test_assert(ret == 0x400);

    memset(buf, 0x44, 0x10000);
    ret = lio_pwrite(s, 0x18400, 0x800, buf);
    test_assert(ret == 0x800);

    memset(buf, 0x55, 0x10000);
    ret = lio_pwrite(s, 0x1fc00, 0x800, buf);
    test_assert(ret == 0x800);

    memset(buf, 0x66, 0x10000);
    ret = lio_pwrite(s, 0x20800, 0x200, buf);
    test_assert(ret == 0x200);

    memset(buf, 0x77, 0x10000);
    ret = lio_pwrite(s, 0x20e00, 0x200, buf);
    test_assert(ret == 0x200);

    memset(buf, 0x88, 0x10000);
    ret = lio_pwrite(s, 0x21f00, 0x200, buf);
    test_assert(ret == 0x200);

    memset(buf, 0x99, 0x10000);
    ret = lio_pwrite(s, 0x2fa00, 0x8b00, buf);
    test_assert(ret == 0x8b00);

    free(buf);
}

static void test_read(lio_stream_t s)
{
    uint8_t* buf;
    uint8_t* rbuf;
    int ret;

    buf = malloc(2 * 32 * 1024);
    rbuf = malloc(2 * 32 * 1024);

    memset(buf, 0x11, 0x10000);
    ret = lio_pread(s, 0, 0x8000, rbuf);
    test_assert(ret == 0x8000);
    test_assert(!memcmp(buf, rbuf, 0x8000));

    memset(buf, 0x22, 0x10000);
    ret = lio_pread(s, 0x8000, 0x10000, rbuf);
    test_assert(ret == 0x10000);
    test_assert(!memcmp(buf, rbuf, 0x10000));

    memset(buf, 0x33, 0x10000);
    ret = lio_pread(s, 0x18000, 0x400, rbuf);
    test_assert(ret == 0x400);
    test_assert(!memcmp(buf, rbuf, 0x400));

    memset(buf, 0x44, 0x10000);
    ret = lio_pread(s, 0x18400, 0x800, rbuf);
    test_assert(ret == 0x800);
    test_assert(!memcmp(buf, rbuf, 0x800));

    memset(buf, 0x55, 0x10000);
    ret = lio_pread(s, 0x1fc00, 0x800, rbuf);
    test_assert(ret == 0x800);
    test_assert(!memcmp(buf, rbuf, 0x800));

    memset(buf, 0x66, 0x10000);
    ret = lio_pread(s, 0x20800, 0x200, rbuf);
    test_assert(ret == 0x200);
    test_assert(!memcmp(buf, rbuf, 0x200));

    memset(buf, 0x77, 0x10000);
    ret = lio_pread(s, 0x20e00, 0x200, rbuf);
    test_assert(ret == 0x200);
    test_assert(!memcmp(buf, rbuf, 0x200));

    memset(buf, 0x88, 0x10000);
    ret = lio_pread(s, 0x21f00, 0x200, rbuf);
    test_assert(ret == 0x200);
    test_assert(!memcmp(buf, rbuf, 0x200));

    memset(buf, 0x99, 0x10000);
    ret = lio_pread(s, 0x2fa00, 0x8b00, rbuf);
    test_assert(ret == 0x8b00);
    test_assert(!memcmp(buf, rbuf, 0x8b00));

    free(rbuf);
    free(buf);
}

static void test1(void)
{
    lio_resource_t tmp_root;
    lio_resource_t tmp_file;
    lio_stream_t s;

    test_name = "Einfache Dateien in tmp";

    /* Testdatei anlegen und öffnen */
    tmp_root = lio_resource("tmp:/", false);
    test_assert(tmp_root >= 0);

    tmp_file = lio_mkfile(tmp_root, "test");
    test_assert(tmp_file >= 0);

    s = lio_open(tmp_file, LIO_READ | LIO_WRITE);
    test_assert(s >= 0);

    /* Einmal die Datei anlegen und einmal überschreiben */
    test_write(s);
    test_write(s);

    /* Und das ganze wieder zurücklesen */
    test_read(s);

    printf("* PASS %s\n", test_name);
}

static void test4(void)
{
    lio_resource_t tmp_root;
    lio_resource_t tmp_file;
    lio_stream_t s;
    int64_t r;
    char *data = "DATA";

    test_name = "Suchen in Dateien";

    /* Testdatei anlegen und öffnen */
    tmp_root = lio_resource("tmp:/", false);
    test_assert(tmp_root >= 0);

    tmp_file = lio_mkfile(tmp_root, "test_seek");
    test_assert(tmp_file >= 0);

    s = lio_open(tmp_file, LIO_WRITE);
    test_assert(s >= 0);

    /* In Datei schreiben */
    r = lio_write(s, 4, data);
    test_assert(r == 4);

    /* Ein paar Bytes überspringen (sollten mit 0 aufgefüllt werden) */
    r = lio_seek(s, 4, LIO_SEEK_CUR);
    test_assert(r == 8);

    /* Noch was in die Datei schreiben */
    r = lio_write(s, 4, data);
    test_assert(r == 4);

    /* Datei schließen und erneut öffnen */
    r = lio_close(s);
    test_assert(r >= 0);

    char buffer[8];

    s = lio_open(tmp_file, LIO_READ);
    test_assert(tmp_file >= 0);

    /* Relativ zum Ende der Datei springen */
    r = lio_seek(s, -8, LIO_SEEK_END);
    test_assert(r == 4);

    /* Daten einlesen und überprüfen */
    r = lio_read(s, 8, buffer);
    test_assert(r == 8);
    test_assert(memcmp("\0\0\0\0DATA", buffer, 8) == 0);

    /* Auch die Daten vom Anfang einlesen */
    r = lio_seek(s, 0, LIO_SEEK_SET);
    test_assert(r == 0);

    r = lio_read(s, 4, buffer);
    test_assert(r == 4);
    test_assert(memcmp("DATA", buffer, 4) == 0);

    /* Datei entfernen */
    r = lio_unlink(tmp_root, "test_seek");
    test_assert(r >= 0);

    printf("* PASS %s\n", test_name);
}

static void test5(void)
{
    lio_resource_t tmp_root;
    lio_resource_t tmp_file;
    lio_stream_t s;
    int64_t r;

    test_name = "Fehlerfaelle in lio_seek";

    /* Testdatei anlegen und öffnen */
    tmp_root = lio_resource("tmp:/", false);
    test_assert(tmp_root >= 0);

    tmp_file = lio_mkfile(tmp_root, "test_seek");
    test_assert(tmp_file >= 0);

    s = lio_open(tmp_file, LIO_WRITE);
    test_assert(s >= 0);

    r = lio_seek(s, 32, LIO_SEEK_CUR);
    test_assert(r == 32);

    /* Negatives Offset */
    r = lio_seek(s, -12, LIO_SEEK_SET);
    test_assert(r == -EINVAL);
    r = lio_seek(s, 0, LIO_SEEK_CUR);
    test_assert(r == 32);


    r = lio_seek(s, -64, LIO_SEEK_CUR);
    test_assert(r == -EINVAL);
    r = lio_seek(s, 0, LIO_SEEK_CUR);
    test_assert(r == 32);

    r = lio_seek(s, -12, LIO_SEEK_END);
    test_assert(r == -EINVAL);
    r = lio_seek(s, 0, LIO_SEEK_CUR);
    test_assert(r == 32);

    /* Ungueltiger Modus */
    r = lio_seek(s, 42, 1337);
    test_assert(r == -EINVAL);
    r = lio_seek(s, 0, LIO_SEEK_CUR);
    test_assert(r == 32);

    printf("* PASS %s\n", test_name);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("* ERROR Zu wenige Parameter\n");
        quit_qemu();
    }

    switch (atoi(argv[1])) {
        case 1:
            test1();
            break;
        case 4:
            test4();
            break;
        case 5:
            test5();
            break;

        default:
            printf("* ERROR Unbekannter Testfall\n");
            break;
    }

    quit_qemu();
}
