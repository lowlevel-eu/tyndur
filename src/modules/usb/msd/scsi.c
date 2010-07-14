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

#include "msd.h"
#include "scsi.h"

const char *dev_type[] =
{
    "magnetic disk",
    "magnetic tape",
    "printer",
    "processor",
    "write-once device",
    "CD-ROM",
    "scanner",
    "optical medium",
    "medium changer",
    "communications device",
    "?",
    "?",
    "RAID",
    "enclosure services device",
    "magnetic disk (simplified)",
    "card reader/writer (optical)",
    "?",
    "object-based storage",
    "automation/drive interface",
    "?",
    "?",
    "?",
    "?",
    "?",
    "?",
    "?",
    "?",
    "?",
    "?",
    "?",
    "well known LUN",
    "?"
};

static int do_pkt(struct msc_gen_dev* dev, int receive, void* buffer1,
    size_t length1, void* buffer2, size_t length2)
{
    while (dev->issue_scsi_cmd(dev, buffer1, length1, !!(receive == 1),
        length2))
    {
        struct scsi_sense sense;
        struct scsi_cmd_request_sense req_sense = {
            .cmd = {
                .operation = SCSI_REQUEST_SENSE
            },
            .alloc_len = sizeof(sense),
            .lun = dev->lun
        };

        if (dev->issue_scsi_cmd(dev, &req_sense, sizeof(req_sense), true,
            sizeof(sense)))
        {
            fprintf(stderr, "[usb-msd] Could not request sense\n");
            return -1;
        }

        if (dev->recv(dev, &sense, sizeof(sense))) {
            fprintf(stderr, "[usb-msd] Could not sense\n");
            return -1;
        }
    }

    if (receive == 1) {
        return dev->recv(dev, buffer2, length2);
    } else if (!receive) {
        return dev->send(dev, buffer2, length2);
    }

    return 0;
}

int msd_scsi_init(struct msc_gen_dev* dev, uint64_t* blocks, uint64_t* bsize)
{
    struct scsi_inq_answer inq_ans;
    struct scsi_cmd_inquiry inq = {
        .cmd = {
            .operation = SCSI_INQUIRY
        },
        .alloc_len = htobe16(sizeof(inq_ans)),
        .lun = dev->lun
    };

    if (do_pkt(dev, 1, &inq, sizeof(inq), &inq_ans, sizeof(inq_ans))) {
        return 0;
    }

    printf("[usb-msd] LUN %i: %s, ", dev->lun, dev_type[inq_ans.dev_type]);

    int last;
    for (last = (int)sizeof(inq_ans.vendor) - 1; last >= 0; last--) {
        if (inq_ans.vendor[last] != ' ') {
            break;
        }
    }

    last++;

    int i;

    if (last) {
        for (i = 0; i < last; i++) {
            printf("%c", inq_ans.vendor[i]);
        }
        printf(" ");
    }


    for (last = (int)sizeof(inq_ans.product) - 1; last >= 0; last--) {
        if (inq_ans.product[last] != ' ') {
            break;
        }
    }

    last++;

    for (i = 0; i < last; i++) {
        printf("%c", inq_ans.product[i]);
    }
    printf("\n");

    // Erklär mir mal einer, wieso man das braucht
    // I'd appreciate any explanation why this is required
    int8_t zero[6] = { 0 };
    if (do_pkt(dev, -1, &zero, sizeof(zero), NULL, 0)) {
        return 0;
    }

    uint32_t cap_ans[2];
    struct scsi_cmd_read_capacity cap = {
        .cmd = {
            .operation = SCSI_READ_CAPACITY
        }
    };

    if (do_pkt(dev, 1, &cap, sizeof(cap), &cap_ans, sizeof(cap_ans))) {
        return 0;
    }

    *blocks = (uint64_t)betoh32(cap_ans[0]) + 1LL;
    *bsize = betoh32(cap_ans[1]);

    return 1;
}

#define SCSI_READ 10

size_t msd_scsi_read(struct msc_gen_dev* dev, uint64_t start_lba, void* dest,
    uint64_t blocks)
{
    #if SCSI_READ == 10
    struct scsi_cmd_read10 r10 = {
        .cmd = {
            .operation = SCSI_READ10
        },
        .lun = dev->lun,
        .lba = htobe32(start_lba),
        .length = htobe16(blocks)
    };
    #elif SCSI_READ == 6
    struct scsi_cmd_read6 r6 = {
        .cmd = {
            .operation = SCSI_READ6
        },
        .lun = dev->lun,
        .lba_lo = htobe16(start_lba),
        .lba_hi = start_lba >> 16,
        .length = blocks
    };
    #else
    struct scsi_cmd_read12 r12 = {
        .cmd = {
            .operation = SCSI_READ12
        },
        .lun = dev->lun,
        .lba = htobe32(start_lba),
        .length = htobe32(blocks)
    };
    #endif

    #if SCSI_READ == 10
    if ((start_lba >> 32) || (blocks >> 16)) {
    #elif SCSI_READ == 6
    if ((start_lba >> 21) || (blocks >> 8)) {
    #else
    if ((start_lba >> 32) || (blocks >> 32)) {
    #endif
        return 0;
    }

    #if SCSI_READ == 10
    int ret = do_pkt(dev, 1, &r10, sizeof(r10), dest, blocks * dev->block_size);
    #elif SCSI_READ == 6
    int ret = do_pkt(dev, 1, &r6, sizeof(r6), dest, blocks * dev->block_size);
    #else
    int ret = do_pkt(dev, 1, &r12, sizeof(r12), dest, blocks * dev->block_size);
    #endif

    if (ret < 0) {
        return 0;
    }
    return blocks * dev->block_size;
}
