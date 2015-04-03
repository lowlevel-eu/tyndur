/*
 * Copyright (C) 2008 Mathias Gottschlag
 * Copyright (C) 2009 Kevin Wolf
 * Copyright (C) 2015 Max Reitz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cdi/lists.h>
#include <cdi/misc.h>
#include <cdi/scsi.h>
#include <cdi/storage.h>

static int cdi_scsi_disk_read(struct cdi_storage_device* device,
    uint64_t start, uint64_t count, void* buffer);

static int cdi_scsi_disk_write(struct cdi_storage_device* device,
    uint64_t start, uint64_t count, void* buffer);

struct cdi_storage_driver cdi_scsi_disk_driver = {
    .drv = {
        .name       = "scsidisk",
        .type       = CDI_STORAGE,
        .bus        = CDI_SCSI,
    },
    .read_blocks    = cdi_scsi_disk_read,
    .write_blocks   = cdi_scsi_disk_write,
};


typedef struct scsi_storage_device {
    struct cdi_storage_device dev;
    struct cdi_scsi_device* backend;
    int lun;
} scsi_storage_device_t;


#define SCSI_REQUEST_SENSE      0x03
#define SCSI_INQUIRY            0x12
#define SCSI_START_STOP_UNIT    0x1b
#define SCSI_READ_CAPACITY      0x25
#define SCSI_READ10             0x28
#define SCSI_WRITE10            0x2a
#define SCSI_READ16             0x88
#define SCSI_WRITE16            0x8a

struct scsi_cmd {
    uint8_t operation;
} __attribute__((packed));

struct scsi_cmd_request_sense {
    struct scsi_cmd cmd;
    uint8_t rsvd1;
    uint16_t rsvd2;
    uint8_t alloc_len;
    uint8_t control;
} __attribute__((packed));

struct scsi_cmd_inquiry {
    struct scsi_cmd cmd;
    uint8_t rsvd;
    uint8_t page;
    uint16_t alloc_len;
    uint8_t control;
} __attribute__((packed));

struct scsi_cmd_start_stop_unit {
    struct scsi_cmd cmd;
    uint8_t immed;
    uint8_t rsvd;
    uint8_t power_condition_modifier;
    uint8_t flags_power_condition;
    uint8_t control;
} __attribute__((packed));

struct scsi_cmd_read_capacity {
    struct scsi_cmd cmd;
    uint8_t rsvd1;
    uint32_t lba;
    uint32_t rsvd2;
} __attribute__((packed));

struct scsi_cmd_read10 {
    struct scsi_cmd cmd;
    uint8_t rsvd1;
    uint32_t lba;
    uint8_t rsvd2;
    uint16_t length;
    uint8_t control;
} __attribute__((packed));

struct scsi_cmd_write10 {
    struct scsi_cmd cmd;
    uint8_t rsvd1;
    uint32_t lba;
    uint8_t rsvd2;
    uint16_t length;
    uint8_t control;
} __attribute__((packed));

struct scsi_cmd_read16 {
    struct scsi_cmd cmd;
    uint8_t flags;
    uint64_t lba;
    uint32_t length;
    uint8_t group;
    uint8_t control;
} __attribute__((packed));

struct scsi_cmd_write16 {
    struct scsi_cmd cmd;
    uint8_t flags;
    uint64_t lba;
    uint32_t length;
    uint8_t group;
    uint8_t control;
} __attribute__((packed));

struct scsi_sense {
    uint8_t resp_code;
    uint8_t segment;
    uint8_t flags;
    uint32_t information;
    uint8_t add_sense_length;
    uint32_t cmd_spec_info;
    uint8_t add_sense_code;
    uint8_t add_sense_code_qual;
    uint8_t field_replacable_unit_code;
    uint8_t sense_key_spec1;
    uint8_t sense_key_spec2;
    uint8_t sense_key_spec3;
} __attribute__((packed));

struct scsi_inq_answer {
    uint8_t dev_type;
    uint8_t type_mod;
    uint8_t version;
    uint8_t resp_data_format;
    uint8_t add_length;
    uint8_t flags1, flags2, flags3;
    char vendor[8];
    char product[16];
    uint32_t revision;
    char serial[8];
    uint8_t vendor_unique[12];
    uint8_t clocking;
    uint8_t rsvd1;
    uint16_t version_desc[8];
    uint8_t rsvd2[22];
} __attribute__((packed));


static int scsi_read10(scsi_storage_device_t* dev, uint32_t sector,
                       uint16_t nb_sectors, char* buffer)
{
    struct cdi_scsi_driver* drv = CDI_UPCAST(dev->backend->dev.driver,
                                             struct cdi_scsi_driver, drv);
    struct scsi_cmd_read10 r10 = {
        .cmd = {
            .operation = SCSI_READ10,
        },
        .lba    = cdi_cpu_to_be32(sector),
        .length = cdi_cpu_to_be16(nb_sectors),
    };
    struct cdi_scsi_packet pkt = {
        .lun        = dev->lun,
        .buffer     = buffer,
        .bufsize    = (size_t)nb_sectors * dev->dev.block_size,
        .direction  = CDI_SCSI_READ,
        .cmdsize    = sizeof(r10),
    };
    memcpy(pkt.command, &r10, sizeof(r10));

    if (drv->request(dev->backend, &pkt)) {
        return -1;
    }

    return 0;
}

static int scsi_write10(scsi_storage_device_t* dev, uint32_t sector,
                        uint16_t nb_sectors, char* buffer)
{
    struct cdi_scsi_driver* drv = CDI_UPCAST(dev->backend->dev.driver,
                                             struct cdi_scsi_driver, drv);
    struct scsi_cmd_write10 w10 = {
        .cmd = {
            .operation = SCSI_WRITE10,
        },
        .lba    = cdi_cpu_to_be32(sector),
        .length = cdi_cpu_to_be16(nb_sectors),
    };
    struct cdi_scsi_packet pkt = {
        .lun        = dev->lun,
        .buffer     = buffer,
        .bufsize    = (size_t)nb_sectors * dev->dev.block_size,
        .direction  = CDI_SCSI_WRITE,
        .cmdsize    = sizeof(w10),
    };
    memcpy(pkt.command, &w10, sizeof(w10));

    if (drv->request(dev->backend, &pkt)) {
        return -1;
    }

    return 0;
}

static int scsi_read16(scsi_storage_device_t* dev, uint64_t sector,
                       uint32_t nb_sectors, char* buffer)
{
    struct cdi_scsi_driver* drv = CDI_UPCAST(dev->backend->dev.driver,
                                             struct cdi_scsi_driver, drv);
    struct scsi_cmd_read16 r16 = {
        .cmd = {
            .operation = SCSI_READ16,
        },
        .lba    = cdi_cpu_to_be64(sector),
        .length = cdi_cpu_to_be32(nb_sectors),
    };
    struct cdi_scsi_packet pkt = {
        .lun        = dev->lun,
        .buffer     = buffer,
        .bufsize    = (size_t)nb_sectors * dev->dev.block_size,
        .direction  = CDI_SCSI_READ,
        .cmdsize    = sizeof(r16),
    };
    memcpy(pkt.command, &r16, sizeof(r16));

    if (drv->request(dev->backend, &pkt)) {
        return -1;
    }

    return 0;
}

static int scsi_write16(scsi_storage_device_t* dev, uint64_t sector,
                        uint32_t nb_sectors, char* buffer)
{
    struct cdi_scsi_driver* drv = CDI_UPCAST(dev->backend->dev.driver,
                                             struct cdi_scsi_driver, drv);
    struct scsi_cmd_write16 w16 = {
        .cmd = {
            .operation = SCSI_WRITE16,
        },
        .lba    = cdi_cpu_to_be64(sector),
        .length = cdi_cpu_to_be32(nb_sectors),
    };
    struct cdi_scsi_packet pkt = {
        .lun        = dev->lun,
        .buffer     = buffer,
        .bufsize    = (size_t)nb_sectors * dev->dev.block_size,
        .direction  = CDI_SCSI_WRITE,
        .cmdsize    = sizeof(w16),
    };
    memcpy(pkt.command, &w16, sizeof(w16));

    if (drv->request(dev->backend, &pkt)) {
        return -1;
    }

    return 0;
}

static int scsi_get_capacity(scsi_storage_device_t* dev)
{
    struct cdi_scsi_driver* drv = CDI_UPCAST(dev->backend->dev.driver,
                                             struct cdi_scsi_driver, drv);
    uint32_t capacity[2] = { 0 };
    struct scsi_cmd_read_capacity read_cap = {
        .cmd = {
            .operation = SCSI_READ_CAPACITY,
        },
    };
    struct cdi_scsi_packet pkt = {
        .lun        = dev->lun,
        .buffer     = capacity,
        .bufsize    = sizeof(capacity),
        .direction  = CDI_SCSI_READ,
        .cmdsize    = sizeof(read_cap),
    };
    memcpy(pkt.command, &read_cap, sizeof(read_cap));

    if (drv->request(dev->backend, &pkt)) {
        return -1;
    }

    dev->dev.block_count = (uint64_t)cdi_be32_to_cpu(capacity[0]) + 1;
    dev->dev.block_size  = cdi_be32_to_cpu(capacity[1]);

    return 0;
}

#if 0
static int cdrom_lock_device(FsSCSIDevice *device, int index, int lock)
{
    // Lock drive
    FsSCSIPacket packet;
    memset(&packet, 0, sizeof(FsSCSIPacket));
    packet.direction = CDI_SCSI_NO_DATA;
    ((union cdrom_command*)packet.command)->dfl.opcode = 0x1E;
    packet.command[4] = 1;
    packet.buffer = 0;
    packet.length = 0;
    packet.commandsize = 12;
    uint32_t status = device->request(device, &packet);
    if (status)
    {
        kePrint("cdrom %d: Lock failed, %x\n", index, status);
        fsCloseSCSIDevice(device);
        return KE_ERROR_UNKNOWN;
    }
    return 0;
}

static void cdrom_sense(FsSCSIDevice *device, uint32_t index)
{
    char sense_data[18];
    FsSCSIPacket packet;
    packet.direction = CDI_SCSI_READ;
    packet.buffer = sense_data;
    packet.length = 18;
    packet.commandsize = 12;
    ((union cdrom_command*)packet.command)->dfl.opcode = 0x12;
}
#endif

static int scsi_init(scsi_storage_device_t* dev)
{
    struct cdi_scsi_driver* drv = CDI_UPCAST(dev->backend->dev.driver,
                                             struct cdi_scsi_driver, drv);
    struct scsi_inq_answer inqdata;

    struct scsi_cmd_inquiry inq = {
        .cmd = {
            .operation = SCSI_INQUIRY,
        },
        .alloc_len = cdi_cpu_to_be16(sizeof(inqdata)),
    };
    struct cdi_scsi_packet inq_pkt = {
        .lun        = dev->lun,
        .buffer     = &inqdata,
        .bufsize    = sizeof(inqdata),
        .direction  = CDI_SCSI_READ,
        .cmdsize    = sizeof(inq),
    };
    memcpy(inq_pkt.command, &inq, sizeof(inq));

    if (drv->request(dev->backend, &inq_pkt)) {
        return -1;
    }

#if 0
    if (cdrom_lock_device(device, index, 1))
        return KE_ERROR_UNKNOWN;
#endif

    struct scsi_cmd_start_stop_unit ssu = {
        .cmd = {
            .operation = SCSI_START_STOP_UNIT,
        },
        .flags_power_condition = 0x3, // LOEJ+START
    };
    struct cdi_scsi_packet ssu_pkt = {
        .lun        = dev->lun,
        .direction  = CDI_SCSI_NODATA,
        .cmdsize    = sizeof(ssu),
    };
    memcpy(ssu_pkt.command, &ssu, sizeof(ssu));

    if ((inqdata.dev_type & 0x1f) == 0x05) { // CD/DVD
        if (drv->request(dev->backend, &ssu_pkt)) {
            printf("cdrom: Start failed\n");
            return -1;
        }
        dev->dev.block_size = 2048;
    } else {
        dev->dev.block_size = 512;
    }

    return 0;
}

static int cdi_scsi_disk_read(struct cdi_storage_device* dev, uint64_t start,
                              uint64_t count, void* buffer)
{
    scsi_storage_device_t* scsi_dev = CDI_UPCAST(dev, scsi_storage_device_t,
                                                 dev);
    bool read16 = (start >> 32) || ((start + count) >> 32);

    while (count > 0) {
        int ret;
        uint32_t it_count;

        if (read16) {
            it_count = count > 0xffffffffu ? 0xffffffffu : count;
            ret = scsi_read16(scsi_dev, start, it_count, buffer);
        } else {
            it_count = count > 0xffff ? 0xffff : count;
            ret = scsi_read10(scsi_dev, start, it_count, buffer);
        }

        if (ret < 0) {
            return -1;
        }

        start += it_count;
        count -= it_count;

        buffer = (void*) ((uintptr_t)buffer + it_count * dev->block_size);
    }

    return 0;
}

static int cdi_scsi_disk_write(struct cdi_storage_device* dev, uint64_t start,
                               uint64_t count, void* buffer)
{
    scsi_storage_device_t* scsi_dev = CDI_UPCAST(dev, scsi_storage_device_t,
                                                 dev);
    bool write16 = (start >> 32) || ((start + count) >> 32);

    while (count > 0) {
        int ret;
        uint32_t it_count;

        if (write16) {
            it_count = count > 0xffffffffu ? 0xffffffffu : count;
            ret = scsi_write16(scsi_dev, start, it_count, buffer);
        } else {
            it_count = count > 0xffff ? 0xffff : count;
            ret = scsi_write10(scsi_dev, start, it_count, buffer);
        }

        if (ret < 0) {
            return -1;
        }

        start += it_count;
        count -= it_count;

        buffer = (void*) ((uintptr_t)buffer + it_count * dev->block_size);
    }

    return 0;
}

static void driver_init(struct cdi_driver* drv)
{
    static int initialised = 0;

    if (!initialised) {
        cdi_scsi_disk_driver.drv.name = strdup(drv->name);

        cdi_driver_register(&cdi_scsi_disk_driver.drv);
        cdi_storage_driver_init(&cdi_scsi_disk_driver);
    }
}

int cdi_scsi_disk_init(struct cdi_scsi_device* device)
{
    int luns = device->lun_count ?: 1;

    driver_init(device->dev.driver);

    for (int lun = 0; lun < luns; lun++) {
        scsi_storage_device_t* dev = calloc(1, sizeof(*dev));
        bool inquiry_failed;

        dev->backend = device;
        dev->lun = lun;

        inquiry_failed = scsi_init(dev) < 0;

        if (scsi_get_capacity(dev) < 0) {
            if (inquiry_failed) {
                /* We're fine as long as either INQUIRY or READ_CAPACITY work -
                 * but if both fail, there's not a lot we can do. */
                fprintf(stderr,
                        "[scsi] Failed to initialize LUN %i of device %s\n",
                        lun, device->dev.name);

                free(dev);
                continue;
            }

            /* Apparently this is common for CD-ROM devices; at least the old
             * SCSI implementation didn't care about it. A default block_size
             * has been set by scsi_init(). */
            dev->dev.block_count = 0;
        }

        dev->dev.dev.driver = (struct cdi_driver*) &cdi_scsi_disk_driver;
        if (luns > 1) {
            asprintf((char**) &dev->dev.dev.name, "%s_l%i",
                     device->dev.name, lun);
        } else {
            dev->dev.dev.name = strdup(device->dev.name);
        }

        cdi_storage_device_init(&dev->dev);
    }

    return 0;
}
