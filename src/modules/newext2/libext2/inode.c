/*
 * Copyright (c) 2008 The LOST Project. All rights reserved.
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

#include <string.h>
#include <stdio.h>
#include "ext2.h"

int ext2_bg_read(ext2_fs_t* fs, int group_nr, ext2_blockgroup_t* bg)
{
    if (!fs->dev_read(
        ((fs->sb_block + 1) * ext2_sb_blocksize(fs->sb)) + group_nr *
        sizeof(ext2_blockgroup_t), sizeof(ext2_blockgroup_t), bg,
        fs->dev_private))
    {
        return 0;
    }

    return 1;
}

int ext2_bg_update(ext2_fs_t* fs, int group_nr, ext2_blockgroup_t* bg)
{
    if (!fs->dev_write(
        ((fs->sb_block + 1) * ext2_sb_blocksize(fs->sb)) + group_nr *
        sizeof(ext2_blockgroup_t), sizeof(ext2_blockgroup_t), bg,
        fs->dev_private))
    {
        return 0;
    }

    return 1;
}

int ext2_inode_read(ext2_fs_t* fs, uint64_t inode_nr, ext2_inode_t* inode)
{
    size_t inode_size;
    uint64_t inode_offset;
    ext2_blockgroup_t bg;

    ext2_bg_read(fs, inode_nr / fs->sb->inodes_per_group, &bg);

    inode_size = ext2_sb_inodesize(fs->sb);
    inode_offset = ext2_sb_blocksize(fs->sb) * bg.inode_table
        + inode_size * (inode_nr % fs->sb->inodes_per_group);

    if (!fs->dev_read(inode_offset, sizeof(inode->raw), &inode->raw,
        fs->dev_private))
    {
        return 0;
    }

    inode->fs = fs;
    inode->number = inode_nr;

    return 1;
}

int ext2_inode_update(ext2_inode_t* inode)
{
    size_t inode_size;
    uint64_t inode_offset;
    ext2_blockgroup_t bg;
    ext2_fs_t* fs = inode->fs;

    ext2_bg_read(fs, inode->number / fs->sb->inodes_per_group, &bg);

    inode_size = ext2_sb_inodesize(fs->sb);
    inode_offset = ext2_sb_blocksize(fs->sb) * bg.inode_table
        + inode_size * (inode->number % fs->sb->inodes_per_group);

    if (!fs->dev_write(inode_offset, sizeof(inode->raw), &inode->raw,
        fs->dev_private))
    {
        return 0;
    }

    return 1;
}

static uint64_t block_alloc(ext2_fs_t* fs)
{
    uint32_t bitmap[fs->sb->blocks_per_group];
    uint64_t bitmap_offset;
    uint32_t i, j;
    size_t   block_size = ext2_sb_blocksize(fs->sb);
    uint8_t  buf[block_size];

    ext2_blockgroup_t bg;
    ext2_bg_read(fs, 0, &bg);

    // Bitmap laden
    bitmap_offset = ext2_sb_blocksize(fs->sb) * bg.block_bitmap;
    if (!fs->dev_read(bitmap_offset, 4 * fs->sb->blocks_per_group, bitmap,
        fs->dev_private))
    {
        return 0;
    }

    // Freies Bit suchen
    for (i = 0; i < fs->sb->blocks_per_group; i++) {
        if (bitmap[i] != ~0) {
            for (j = 0; j < 32; j++) {
                if ((bitmap[i] & (1 << j)) == 0) {
                    goto found;
                }
            }
        }
    }

    return 0;

found:

    // Mit Nullen initialisieren
    memset(buf, 0, block_size);
    if (!fs->dev_write((i * 32 + j) * block_size, block_size, buf,
        fs->dev_private))
    {
        return 0;
    }

    // Als besetzt markieren
    bitmap[i] |= (1 << j);
    if (!fs->dev_write(bitmap_offset, 4 * fs->sb->blocks_per_group, bitmap,
        fs->dev_private))
    {
        return 0;
    }

    fs->sb->free_blocks--;
    ext2_sb_update(fs, fs->sb);

    bg.free_blocks--;
    ext2_bg_update(fs, 0, &bg);

    return (i * 32) + j;
}

static uint64_t get_block_offset(
    ext2_inode_t* inode, uint64_t block, int alloc)
{
    size_t   block_size;
    uint64_t block_nr;

    ext2_fs_t* fs = inode->fs;
    block_size = ext2_sb_blocksize(fs->sb);

    // TODO Doppelt und dreifach indirekte Blocks allozieren
    uint64_t get_indirect_block_nr(uint64_t index, uint64_t table_block)
    {
        uint32_t table[block_size / 4];

        // TODO table_block == 0 => return 0

        if (!fs->dev_read(table_block * block_size, block_size, table,
            fs->dev_private))
        {
            return 0;
        }

        if ((table[index] == 0) && alloc) {
            table[index] = block_alloc(fs);
            inode->raw.block_count += block_size / 512;
            if (!fs->dev_write(table_block * block_size, block_size, table,
                fs->dev_private))
            {
                return 0;
            }
        }

        return table[index];
    }

    if ((block < 12) && (block < ((block_size / 4) + 12))) {
        block_nr = inode->raw.blocks[block];
        if ((block_nr == 0) && alloc) {
            block_nr = inode->raw.blocks[block] = block_alloc(fs);
            inode->raw.block_count += block_size / 512;
        }
    } else if (block < ((block_size / 4) + 12)) {
        if (inode->raw.blocks[12] == 0) {
            inode->raw.blocks[12] = block_alloc(fs);
            inode->raw.block_count += block_size / 512;
        }
        block_nr =
            get_indirect_block_nr(block - 12, inode->raw.blocks[12]);
    } else {
        if (inode->raw.blocks[13] == 0) {
            inode->raw.blocks[13] = block_alloc(fs);
            inode->raw.block_count += block_size / 512;
        }
        block -= 12 + (block_size / 4);
        block_nr = get_indirect_block_nr(block % (block_size / 4),
            get_indirect_block_nr(block / (block_size / 4),
            inode->raw.blocks[13]));
    }
    return block_size * block_nr;
}

int ext2_inode_readblk(ext2_inode_t* inode, uint64_t block, void* buf)
{
    ext2_fs_t* fs = inode->fs;

    uint64_t block_offset = get_block_offset(inode, block, 0);
    size_t   block_size = ext2_sb_blocksize(fs->sb);

    // Ein paar Nullen fuer Sparse Files
    if (block_offset == 0) {
        memset(buf, 0, block_size);
        return 1;
    }

    if (!fs->dev_read(block_offset, block_size, buf, fs->dev_private)) {
        return 0;
    }

    return 1;
}

static int writeblk(ext2_inode_t* inode, uint64_t block, void* buf)
{
    ext2_fs_t* fs = inode->fs;

    uint64_t block_offset = get_block_offset(inode, block, 1);
    size_t   block_size = ext2_sb_blocksize(fs->sb);

    if (block_offset == 0) {
        return 0;
    }

    if (!fs->dev_write(block_offset, block_size, buf, fs->dev_private)) {
        return 0;
    }

    return 1;
}

int ext2_inode_writeblk(ext2_inode_t* inode, uint64_t block, void* buf)
{
    size_t block_size = ext2_sb_blocksize(inode->fs->sb);
    int ret = writeblk(inode, block, buf);

    if (block * block_size > inode->raw.size) {
        inode->raw.size = block * block_size;
    }

    return ret;
}

int ext2_inode_readdata(
    ext2_inode_t* inode, uint64_t start, size_t len, void* buf)
{
    size_t block_size = ext2_sb_blocksize(inode->fs->sb);
    uint64_t start_block = start / block_size;
    uint64_t end_block = (start + len - 1) / block_size;
    size_t block_count = end_block - start_block + 1;
    char localbuf[block_count * block_size];
    int i, ret;

    for (i = 0; i < block_count; i++) {
        ret = ext2_inode_readblk(inode, start_block + i,
            localbuf + i * block_size);

        if (!ret) {
            return 0;
        }
    }
    memcpy(buf, localbuf + (start % block_size), len);

    return 0;
}

int ext2_inode_writedata(
    ext2_inode_t* inode, uint64_t start, size_t len, void* buf)
{
    size_t block_size = ext2_sb_blocksize(inode->fs->sb);
    uint64_t start_block = start / block_size;
    uint64_t end_block = (start + len - 1) / block_size;
    size_t block_count = end_block - start_block + 1;
    char localbuf[block_count * block_size];
    int i, ret;

    for (i = 0; i < block_count; i++) {
        ret = ext2_inode_readblk(inode, start_block + i,
            localbuf + i * block_size);

        if (!ret) {
            return 0;
        }
    }

    memcpy(localbuf + (start % block_size), buf, len);

    for (i = 0; i < block_count; i++) {
        ret = writeblk(inode, start_block + i, localbuf + i * block_size);

        if (!ret) {
            return 0;
        }
    }

    if (start + len > inode->raw.size) {
        inode->raw.size = start + len;
    }

    return 1;
}
