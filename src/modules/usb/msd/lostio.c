/*****************************************************************************
* Copyright (c) 2009-2010 Max Reitz                                          *
*                                                                            *
* THE COKE-WARE LICENSE                                                      *
*                                                                            *
* Redistribution  and use  in  source  and  binary  forms,  with or  without *
* modification,  are permitted  provided that the  following  conditions are *
* met:                                                                       *
*                                                                            *
*   1. Redistributions  of  source code  must  retain  the  above  copyright *
*      notice, this list of conditions and the following disclaimer.         *
*   2. Redistribution in  binary form  must reproduce  the  above  copyright *
*      notice  and either  the full  list  of  conditions and  the following *
*      disclaimer or a link to both.                                         *
*   3. If we meet one day and you think this stuff is worth it,  you may buy *
*      me a coke in return.                                                  *
*                                                                            *
* THIS SOFTWARE  IS PROVIDED BY THE REGENTS AND CONTRIBUTORS “AS IS” AND ANY *
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO, THE IMPLIED *
* WARRANTIES  OF MERCHANTABILITY  AND FITNESS  FOR A PARTICULAR PURPOSE  ARE *
* DISCLAIMED.  IN  NO  EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR *
* ANY DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL *
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR *
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER *
* CAUSED  AND  ON  ANY  THEORY OF  LIABILITY,  WHETHER  IN CONTRACT,  STRICT *
* LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY *
* OUT OF THE USE  OF THIS  SOFTWARE,  EVEN IF ADVISED OF THE  POSSIBILITY OF *
* SUCH DAMAGE.                                                               *
*****************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <init.h>
#include <lostio.h>

#include "msd.h"

static size_t msd_read(lostio_filehandle_t* liofh, void* dest,
    size_t blocksize, size_t blockcount);
static size_t msd_write(lostio_filehandle_t* liofh, size_t blocksize,
    size_t blockcount, void* data);
static int msd_seek(lostio_filehandle_t* liofh, uint64_t offset, int origin);

struct partition {
    struct msc_gen_dev* dev;
    uint64_t block_offset;
    uint64_t blocks;
};

struct parttbl_entry {
    uint8_t active;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t start;
    uint32_t size;
} __attribute__((packed));

void init_lostio_interface(void)
{
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();

    typehandle_t* msds = calloc(1, sizeof(*msds));

    msds->id = 42;
    msds->read = &msd_read;
    msds->write = &msd_write;
    msds->seek = &msd_seek;

    lostio_register_typehandle(msds);
}

static void detect_partitions(int msd_number, struct msc_gen_dev* dev)
{
    uint8_t* bootsec = malloc(512);

    msd_scsi_read(dev, 0, bootsec, 1);

    if (*(uint16_t*)&bootsec[510] != 0xAA55) {
        free(bootsec);
        return;
    }

    struct parttbl_entry* ptbl = (struct parttbl_entry*)&bootsec[446];

    int p;
    for (p = 0; p < 4; p++) {
        if (ptbl[p].size) {
            if ((ptbl[p].type == 0x0F) || (ptbl[p].type == 0x85)) {
                continue;
            }

            struct partition* part = calloc(1, sizeof(*part));
            part->dev = dev;
            part->block_offset = ptbl[p].start;
            part->blocks = ptbl[p].size;

            char name[13];
            sprintf(name, "/msd%i_p%i", msd_number, p);

            vfstree_create_node(name, 42, ptbl[p].size * dev->block_size, part,
                0);
        }
    }

    free(bootsec);
}

void provide_msd(struct msc_gen_dev* dev)
{
    static int msds = 0;

    uint64_t bc, bs;
    if (!msd_scsi_init(dev, &bc, &bs)) {
        return;
    }

    printf("[usb-msd] New SCSI block device, %lli %lli-byte blocks: ", bc, bs);
    uint64_t fs = bc * bs;
    if (fs >= 10ULL << 30) {
        printf("%lli GB\n", fs >> 30);
    } else if (fs >= 10 << 20) {
        printf("%lli MB\n", fs >> 20);
    } else if (fs >= 10 << 10) {
        printf("%lli kB\n", fs >> 10);
    } else {
        printf("%lli B\n", fs);
    }

    dev->block_size = bs;
    dev->blocks = bc;

    char name[10];
    int num = msds++;
    sprintf(name, "/msd%i", num);

    struct partition* part = calloc(1, sizeof(*part));
    part->dev = dev;
    part->blocks = bc;

    vfstree_create_node(name, 42, fs, part, 0);

    // Bei 2880 Sektoren ist es vermutlich eine Diskette (ja, es gibt noch
    // andere Diskettenarten als 3,5" HD, aber die sind halt selten), und die
    // hat keine Partitionen (verhindert das Erkennen scheinbarer Partitionen)
    if (bc != 2880) {
        detect_partitions(num, dev);
    }

    // TODO: Das vielleicht erst irgendwie registrieren, wenn alle Geräte
    // angekommen sind
    init_service_register("usb-msd");

    uint16_t* mem = calloc(1, 512);
    msd_scsi_read(dev, 0, mem, 1);
}

static size_t msd_read(lostio_filehandle_t* liofh, void* dest,
    size_t blocksize, size_t blockcount)
{
    struct partition* part = liofh->node->data;
    struct msc_gen_dev* dev = part->dev;

    uint64_t begin = liofh->pos / dev->block_size + part->block_offset;

    // Diese Debuginformationen mögen nervig erscheinen, sind aber sehr
    // wichtig, da man oft nur so erkennen kann, dass überhaupt etwas
    // geschieht
    printf("[usb-msd] read from %lli (%i blocks)\n", begin,
        (blockcount * blocksize + dev->block_size - 1) / dev->block_size);
    size_t ret = msd_scsi_read(dev, begin, dest,
        (blockcount * blocksize + dev->block_size - 1) / dev->block_size);
    msd_seek(liofh, ret, SEEK_CUR);

    return ret;
}

static size_t msd_write(lostio_filehandle_t* liofh, size_t blocksize,
    size_t blockcount, void* data)
{
    return 0;
}

static int msd_seek(lostio_filehandle_t* liofh, uint64_t offset, int origin)
{
    struct partition* part = liofh->node->data;
    struct msc_gen_dev* dev = part->dev;
    uint64_t new_pos = liofh->pos;
    uint64_t size = part->blocks * dev->block_size;

    switch (origin)  {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos += offset;
            break;
        case SEEK_END:
            new_pos = size;
            break;
    }

    if (new_pos > size) {
        return -1;
    } else if (new_pos == size) {
        liofh->flags |= LOSTIO_FLAG_EOF;
    } else {
        liofh->flags &= ~LOSTIO_FLAG_EOF;
    }

    liofh->pos = new_pos;
    return 0;
}
