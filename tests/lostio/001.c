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

static void test2(void)
{
    lio_resource_t tmp_file;
    lio_stream_t s;
    int ret;

    test_name = "Einfaches Schreiben und Lesen auf ata";

    /* Testdatei anlegen und öffnen */
    servmgr_need("ata");
    tmp_file = lio_resource("ata:/ata00", false);
    test_assert(tmp_file >= 0);

    s = lio_open(tmp_file, LIO_READ | LIO_WRITE);
    test_assert(s >= 0);

    /* Einmal die Datei anlegen und einmal überschreiben */
    test_write(s);
    test_write(s);

    /* Und das ganze wieder zurücklesen */
    test_read(s);

    ret = lio_sync_all();
    test_assert(ret >= 0);

    printf("* PASS %s\n", test_name);
}

static void test3(void)
{
    lio_resource_t tmp_file;
    lio_stream_t s;

    test_name = "Lesen auf ata nach Reboot";

    /* Testdatei anlegen und öffnen */
    servmgr_need("ata");
    tmp_file = lio_resource("ata:/ata00", false);
    test_assert(tmp_file >= 0);

    s = lio_open(tmp_file, LIO_READ | LIO_WRITE);
    test_assert(s >= 0);

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

static void test6(void)
{
    lio_resource_t tmp_file;
    lio_stream_t s;
    uint8_t buf[512];
    uint8_t rbuf[512];
    int ret;

    test_name = "ata: Clusterteile nachladen";

    /* Testdatei anlegen und öffnen */
    servmgr_need("ata");
    tmp_file = lio_resource("ata:/ata00", false);
    test_assert(tmp_file >= 0);

    s = lio_open(tmp_file, LIO_READ | LIO_WRITE);
    test_assert(s >= 0);

    /* Einmal schreiben */
    memset(buf, 0xa5, 512);
    ret = lio_pwrite(s, 512, 512, buf);
    test_assert(ret == 512);

    /* Und direkt zurücklesen */
    ret = lio_pread(s, 512, 512, rbuf);
    test_assert(ret == 512);
    test_assert(!memcmp(rbuf, buf, 512));

    /* Benachbarte Sektoren von der Platte lesen */
    ret = lio_pread(s, 0, 512, rbuf);
    test_assert(ret == 512);
    ret = lio_pread(s, 1024, 512, rbuf);
    test_assert(ret == 512);

    /* Ist noch alles da? */
    ret = lio_pread(s, 512, 512, rbuf);
    test_assert(ret == 512);
    test_assert(!memcmp(rbuf, buf, 512));

    printf("* PASS %s\n", test_name);
}


static void rw(lio_stream_t s, int64_t offset, size_t size, void* buf,
    void *rbuf, bool write)
{
    int ret;

    if (write) {
        ret = lio_pwrite(s, offset, size, buf);
        test_assert(ret == size);
    } else {
        ret = lio_pread(s, offset, size, rbuf);
        test_assert(ret == size);
        test_assert(memcmp(buf, rbuf, size) == 0);

    }
}

static void test_7_8_rw(bool write)
{
    lio_resource_t tmp_file;
    lio_stream_t s;
    uint8_t* buf = malloc(512 * 1024);
    uint8_t* rbuf = malloc(512 * 1024);
    int ret;
    int i;

    test_name = write ? "ata: Ganz viel write" : "ata: Ganz viel read";

    /* Testdatei anlegen und öffnen */
    servmgr_need("ata");
    tmp_file = lio_resource("ata:/ata00", false);
    test_assert(tmp_file >= 0);

    s = lio_open(tmp_file, LIO_READ | LIO_WRITE);
    test_assert(s >= 0);

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

    ret = lio_close(s);
    test_assert(ret >= 0);

    ret = lio_sync_all();
    test_assert(ret >= 0);

    free(buf);
    free(rbuf);

    printf("* PASS %s\n", test_name);
}

static void test7(void)
{
    test_7_8_rw(true);
}

static void test8(void)
{
    test_7_8_rw(false);
}

static void test9(void)
{
    lio_resource_t tmp_root;
    lio_resource_t tmp_file;
    lio_resource_t tmp_orig_file;
    lio_resource_t tmp_link;
    lio_stream_t s;
    uint8_t buf[512];
    uint8_t rbuf[512];
    int ret;

    test_name = "Symlinks in tmp";

    /* Testdatei anlegen und befüllen */
    tmp_root = lio_resource("tmp:/", false);
    test_assert(tmp_root >= 0);

    tmp_file = lio_mkfile(tmp_root, "test");
    test_assert(tmp_file >= 0);
    tmp_orig_file = tmp_file;

    s = lio_open(tmp_file, LIO_WRITE);
    test_assert(s >= 0);

    memset(buf, 0xa5, 512);
    ret = lio_pwrite(s, 0, 512, buf);
    test_assert(ret == 512);

    ret = lio_close(s);
    test_assert(ret >= 0);

    /* Ein absoluter Symlink */
    tmp_link = lio_mksymlink(tmp_root, "sym_abs", "tmp:/test");
    test_assert(tmp_link >= 0);

    tmp_file = lio_resource("tmp:/sym_abs", false);
    test_assert(tmp_file == tmp_link);

    tmp_file = lio_resource("tmp:/sym_abs", true);
    test_assert(tmp_file == tmp_orig_file);

    s = lio_open(tmp_file, LIO_READ);
    test_assert(s >= 0);

    memset(rbuf, 0, 512);
    ret = lio_pread(s, 0, 512, rbuf);
    test_assert(ret == 512);
    test_assert(!memcmp(rbuf, buf, 512));

    ret = lio_close(s);
    test_assert(ret >= 0);

    /* Ein servicerelativer Symlink */
    tmp_link = lio_mksymlink(tmp_root, "sym_servrel", "/test");
    test_assert(tmp_link >= 0);

    tmp_file = lio_resource("tmp:/sym_servrel", false);
    test_assert(tmp_file == tmp_link);

    tmp_file = lio_resource("tmp:/sym_servrel", true);
    test_assert(tmp_file == tmp_orig_file);

    s = lio_open(tmp_file, LIO_READ);
    test_assert(s >= 0);

    memset(rbuf, 0, 512);
    ret = lio_pread(s, 0, 512, rbuf);
    test_assert(ret == 512);
    test_assert(!memcmp(rbuf, buf, 512));

    ret = lio_close(s);
    test_assert(ret >= 0);

    /* Ein relativer Symlink */
    tmp_link = lio_mksymlink(tmp_root, "sym_rel", "test");
    test_assert(tmp_link >= 0);

    tmp_file = lio_resource("tmp:/sym_rel", false);
    test_assert(tmp_file == tmp_link);

    tmp_file = lio_resource("tmp:/sym_rel", true);
    test_assert(tmp_file == tmp_orig_file);

    s = lio_open(tmp_file, LIO_READ);
    test_assert(s >= 0);

    memset(rbuf, 0, 512);
    ret = lio_pread(s, 0, 512, rbuf);
    test_assert(ret == 512);
    test_assert(!memcmp(rbuf, buf, 512));

    ret = lio_close(s);
    test_assert(ret >= 0);

    /* Link auf tmp:/ zurück */
    tmp_link = lio_mksymlink(tmp_root, "sym_tmp", "tmp:/");
    test_assert(tmp_link >= 0);

    tmp_file = lio_resource("tmp:/sym_tmp", false);
    test_assert(tmp_file == tmp_link);

    tmp_file = lio_resource("tmp:/sym_tmp/test", false);
    test_assert(tmp_file == tmp_orig_file);

    tmp_file = lio_resource("tmp:/sym_tmp/test", true);
    test_assert(tmp_file == tmp_orig_file);

    tmp_file = lio_resource("tmp:/sym_tmp/sym_tmp/test", true);
    test_assert(tmp_file == tmp_orig_file);

    /* Link auf / zurück */
    tmp_link = lio_mksymlink(tmp_root, "sym_root", "/");
    test_assert(tmp_link >= 0);

    tmp_file = lio_resource("tmp:/sym_root", false);
    test_assert(tmp_file == tmp_link);

    tmp_file = lio_resource("tmp:/sym_root/test", false);
    test_assert(tmp_file == tmp_orig_file);

    tmp_file = lio_resource("tmp:/sym_root/test", true);
    test_assert(tmp_file == tmp_orig_file);

    tmp_file = lio_resource("tmp:/sym_root/sym_tmp/sym_root/test", true);
    test_assert(tmp_file == tmp_orig_file);

    /* Rekursiver Symlink */
    tmp_link = lio_mksymlink(tmp_root, "sym_loop", "sym_loop");
    test_assert(tmp_link >= 0);

    tmp_file = lio_resource("tmp:/sym_loop", false);
    test_assert(tmp_file == tmp_link);

    tmp_file = lio_resource("tmp:/sym_loop", true);
    test_assert(tmp_file < 0); /* TODO -ELOOP */

    printf("* PASS %s\n", test_name);
}

static void test10(void)
{
    lio_resource_t tmp_root;
    lio_resource_t tmp_file;
    lio_resource_t tmp_link;
    lio_resource_t tmp_dir;
    int ret;

    test_name = "Loeschen in tmp";

    /* Testdatei anlegen */
    tmp_root = lio_resource("tmp:/", false);
    test_assert(tmp_root >= 0);

    tmp_file = lio_mkfile(tmp_root, "test");
    test_assert(tmp_file >= 0);

    /* Absoluten Symlink anlegen und löschen */
    tmp_link = lio_mksymlink(tmp_root, "sym_abs", "tmp:/test");
    test_assert(tmp_link >= 0);

    ret = lio_unlink(tmp_root, "sym_abs");
    test_assert(ret == 0);

    /* Ein servicerelativer Symlink */
    tmp_link = lio_mksymlink(tmp_root, "sym_servrel", "/test");
    test_assert(tmp_link >= 0);

    ret = lio_unlink(tmp_root, "sym_servrel");
    test_assert(ret == 0);

    /* Ein relativer Symlink */
    tmp_link = lio_mksymlink(tmp_root, "sym_rel", "test");
    test_assert(tmp_link >= 0);

    ret = lio_unlink(tmp_root, "sym_rel");
    test_assert(ret == 0);

    /* Rekursiver Symlink */
    tmp_link = lio_mksymlink(tmp_root, "sym_loop", "sym_loop");
    test_assert(tmp_link >= 0);

    ret = lio_unlink(tmp_root, "sym_loop");
    test_assert(ret == 0);

    /* Verzeichnis */
    tmp_dir = lio_mkdir(tmp_root, "testdir");
    test_assert(tmp_dir >= 0);

    ret = lio_unlink(tmp_root, "testdir");
    test_assert(ret == 0);

    /* Die Datei selber auch noch löschen */
    ret = lio_unlink(tmp_root, "test");
    test_assert(ret == 0);

    /* Prüfen, ob wirklich alles gelöscht ist */
    tmp_file = lio_resource("tmp:/file", true);
    test_assert(tmp_file == -ENOENT);
    tmp_file = lio_resource("tmp:/sym_abs", true);
    test_assert(tmp_file == -ENOENT);
    tmp_file = lio_resource("tmp:/sym_servrel", true);
    test_assert(tmp_file == -ENOENT);
    tmp_file = lio_resource("tmp:/sym_rel", true);
    test_assert(tmp_file == -ENOENT);
    tmp_file = lio_resource("tmp:/sym_loop", true);
    test_assert(tmp_file == -ENOENT);
    tmp_file = lio_resource("tmp:/testdir", true);
    test_assert(tmp_file == -ENOENT);

    printf("* PASS %s\n", test_name);
}

static void test11(void)
{
    lio_stream_t rd, wr;
    char buf[512];
    char rbuf[512];
    int ret, len;

    test_name = "Pipes";

    /* Unidirektionale Pipe: Einfache Kommunikation */
    ret = lio_pipe(&rd, &wr, false);
    test_assert(ret == 0);

    strcpy(buf, "Hallo Welt!");
    len = strlen(buf) + 1;
    ret = lio_write(wr, len, buf);
    test_assert(ret == len);

    ret = lio_read(rd, 2, rbuf);
    test_assert(ret == 2);
    test_assert(!memcmp(rbuf, "Ha", 2));

    ret = lio_read(rd, 32, rbuf);
    test_assert(ret == len - 2);
    test_assert(rbuf[len - 3] == '\0');
    test_assert(!strcmp(rbuf, "llo Welt!"));

    ret = lio_readf(rd, 0, 32, rbuf, LIO_REQ_FILEPOS);
    test_assert(ret == -EAGAIN);

    /* Unidirektionale Pipe: Falschrum benutzen */
    ret = lio_write(rd, len, buf);
    test_assert(ret == -EBADF);

    ret = lio_read(wr, 2, rbuf);
    test_assert(ret == -EBADF);

    /* Seek sollte auch nicht tun */
    ret = lio_seek(rd, SEEK_SET, 0);
    test_assert(ret == -EINVAL);

    ret = lio_seek(wr, SEEK_SET, 0);
    test_assert(ret == -EINVAL);

    /* Unidirektionale Pipe: Schreiben, wr schließen, lesen */
    strcpy(buf, "Signifant");
    len = strlen(buf) + 1;

    ret = lio_write(wr, 4, buf);
    test_assert(ret == 4);
    ret = lio_write(wr, len - 4, &buf[4]);
    test_assert(ret == len - 4);

    lio_close(wr);

    ret = lio_write(wr, len, buf);
    test_assert(ret == -EBADF);

    ret = lio_read(rd, 32, rbuf);
    test_assert(ret == len);
    test_assert(rbuf[len - 1] == '\0');
    test_assert(!strcmp(rbuf, "Signifant"));

    ret = lio_read(rd, 32, rbuf);
    test_assert(ret == 0);

    lio_close(rd);

    /* Bidirektionale Pipe */
    ret = lio_pipe(&rd, &wr, true);
    test_assert(ret == 0);

    memcpy(buf, "Noch ein\0Teststring", 20);
    ret = lio_write(wr, 9, buf);
    test_assert(ret == 9);
    ret = lio_write(rd, 11, &buf[9]);
    test_assert(ret == 11);

    ret = lio_read(wr, 2, rbuf);
    test_assert(ret == 2);
    test_assert(!memcmp(rbuf, "Te", 2));

    ret = lio_read(rd, 2, rbuf);
    test_assert(ret == 2);
    test_assert(!memcmp(rbuf, "No", 2));

    ret = lio_read(wr, 32, rbuf);
    test_assert(ret == 9);
    test_assert(rbuf[8] == '\0');
    test_assert(!strcmp(rbuf, "ststring"));

    ret = lio_read(rd, 32, rbuf);
    test_assert(ret == 7);
    test_assert(rbuf[6] == '\0');
    test_assert(!strcmp(rbuf, "ch ein"));

    lio_close(rd);
    lio_close(wr);

    printf("* PASS %s\n", test_name);
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
static void (*const test_fns[])(void) = {
    test1,
    test2,
    test3,
    test4,
    test5,
    test6,
    test7,
    test8,
    test9,
    test10,
    test11,
};

int main(int argc, char* argv[])
{
    int nr;

    if (argc < 2) {
        printf("* ERROR Zu wenige Parameter\n");
        quit_qemu();
    }

    nr = atoi(argv[1]);
    if (nr < 1 || nr > ARRAY_SIZE(test_fns)) {
        printf("* ERROR Unbekannter Testfall\n");
    } else {
        test_fns[nr - 1]();
    }

    quit_qemu();
}
