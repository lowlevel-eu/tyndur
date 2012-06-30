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

#include "dhcp.h"
#include "udp.h"
#include "ip.h"
#include "lostio_if.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syscall.h>
#include <stdint.h>
#include <ctype.h>
#include <rpc.h>

struct dhcp_packet {
    uint8_t     op;
    uint8_t     htype;
    uint8_t     hlen;
    uint8_t     hops;
    uint32_t    xid;
    uint16_t    secs;
    uint16_t    flags;
    uint32_t    ciaddr; /* Client IP address */
    uint32_t    yiaddr; /* 'your' (client) address */
    uint32_t    siaddr;
    uint32_t    giaddr;
    uint8_t     chaddr[16];
    char        sname[64];
    char        file[128];
    uint8_t     options[312];
} __attribute__((packed));

enum {
    BOOTREQUEST = 1,
    BOOTREPLY   = 2,
};

struct dhcp_msgtype {
    uint8_t     magic[4];
    uint8_t     code;
    uint8_t     len;
    uint8_t     type;
} __attribute__((packed));

struct dhcp_serverid {
    uint8_t     code;
    uint8_t     len;
    uint32_t    addr;
} __attribute__((packed));

struct dhcp_requested_ip {
    uint8_t     code;
    uint8_t     len;
    uint32_t    addr;
} __attribute__((packed));

struct dhcp_end {
    uint8_t     code;
} __attribute__((packed));

struct dhcp_discover_opts {
    struct dhcp_msgtype     msgtype;
    struct dhcp_end         end;
} __attribute__((packed));

struct dhcp_request_opts {
    struct dhcp_msgtype     msgtype;
    struct dhcp_serverid    serverid;
    struct dhcp_requested_ip req_ip;
    struct dhcp_end         end;
} __attribute__((packed));

enum {
    DHCPDISCOVER    = 1,
    DHCPOFFER       = 2,
    DHCPREQUEST     = 3,
    DHCPDECLINE     = 4,
    DHCPACK         = 5,
    DHCPNAK         = 6,
    DHCPRELEASE     = 7,
    DHCPINFORM      = 8,
};

static void dhcp_fill_packet(struct device* device, struct dhcp_packet* p,
    struct dhcp_msgtype* m, struct dhcp_end* end, int type)
{
    uint64_t mac = device->dev.mac;

    *p = (struct dhcp_packet) {
        .op     = BOOTREQUEST,
        .htype  = 0x01,
        .hlen   = 6,
        .xid    = 0,
        .chaddr = { mac >>  0, mac >>  8, mac >> 16,
                    mac >> 24, mac >> 32, mac >> 40 },
    };

    *m = (struct dhcp_msgtype) {
        .magic  = { 99, 130, 83, 99 },
        .code   = 53,
        .len    = 1,
        .type   = type,
    };

    *end = (struct dhcp_end) {
        .code   = 255,
    };
}

static int dhcp_discover(struct device* device, struct udp_socket* s)
{
    struct dhcp_discover_opts* opts;
    struct dhcp_packet p;

    opts = (void*) &p.options;
    dhcp_fill_packet(device, &p, &opts->msgtype, &opts->end, DHCPDISCOVER);

    return udp_write(s, &p, sizeof(p));
}

static void dhcp_parse_options(struct device* device, struct dhcp_packet *p)
{
    int i;

    /* Optionen parsen (DHCP-Magic überspringen) */
    i = 4;

    while (i < sizeof(p->options) - 1) {
        uint8_t* option = &p->options[i];
        uint8_t op = option[0];

        if (op == 0xff) {
            break;
        } else if (op == 0x0) {
            i++;
            continue;
        }

        i += 2 + option[1];
        if (i > sizeof(p->options)) {
            break;
        }

        switch (op) {
            case 1: /* Subnet Mask */
                device->dhcp.subnet = *(uint32_t*) &option[2];
                break;

            case 3: /* Router */
                device->dhcp.gateway = *(uint32_t*) &option[2];
                break;

            case 6: /* Domain Name Server */
                device->dhcp.nameserver = *(uint32_t*) &option[2];
                break;

            case 51: /* IP Address Lease Time */
                /* TODO */
                break;

            case 53: /* Message Type */
                device->dhcp.last_op = option[2];
                break;

            case 54: /* Server Identifier */
                device->dhcp.server_ip = *(uint32_t*) &option[2];
                break;
        }
    }
}

static void do_nothing(void) {}

static int dhcp_wait_response(struct device* device, struct udp_socket* s,
    uint8_t msgtype, uint8_t naktype)
{
    int ret;
    uint8_t buf[1024];
    struct dhcp_packet* p = (void*) buf;

again:
    ret = udp_read(s, buf, sizeof(*p));

    /* Mit Timeout auf die Antwort warten */
    if (ret != sizeof(struct dhcp_packet)) {
        uint64_t timeout;

        timeout = get_tick_count() + 4 * 1000000;
        timer_register(do_nothing, 5 * 1000000);

        p();
        while ((ret != sizeof(struct dhcp_packet))
            && (get_tick_count() < timeout))
        {
            v_and_wait_for_rpc();
            p();
            ret = udp_read(s, buf, sizeof(buf));
        }
        v();
    }

    if (ret != sizeof(struct dhcp_packet)) {
        return -ETIMEDOUT;
    }

    /* Antwort auswerten */
    device->dhcp.client_ip = p->yiaddr;
    dhcp_parse_options(device, p);

    if (naktype && device->dhcp.last_op == naktype) {
        return -1;
    } else if (device->dhcp.last_op != msgtype) {
        goto again;
    }

    return 0;
}

static int dhcp_request(struct device* device, struct udp_socket* s)
{
    struct dhcp_request_opts* opts;
    struct dhcp_packet p;

    opts = (void*) &p.options;
    dhcp_fill_packet(device, &p, &opts->msgtype, &opts->end, DHCPREQUEST);

    opts->serverid = (struct dhcp_serverid) {
        .code       = 54,
        .len        = 4,
        .addr       = device->dhcp.server_ip,
    };
    opts->req_ip = (struct dhcp_requested_ip) {
        .code       = 50,
        .len        = 4,
        .addr       = device->dhcp.client_ip,
    };

    return udp_write(s, &p, sizeof(p));
}

/**
 * Sendet eine DHCP-Anfrage und holt die Informationen vom Server
 */
static int dhcp_fetch(struct device* device)
{
    struct udp_socket* s;
    int ret;

    struct routing_entry route = {
        .target     = 0,
        .subnet     = 0xffffffff,
        .gateway    = 0,
        .device     = device,
    };

    device->ip = 0;
    memset(&device->dhcp, 0, sizeof(device->dhcp));

    s = udp_open_route(68, 0xffffffff, 67, &route);
    if (s == NULL) {
        return -EIO;
    }

restart:
    ret = dhcp_discover(device, s);
    if (ret < 0) {
        goto out;
    }

    ret = dhcp_wait_response(device, s, DHCPOFFER, 0);
    if (ret < 0) {
        goto out;
    }

    dhcp_request(device, s);
    if (ret < 0) {
        goto out;
    }

    dhcp_wait_response(device, s, DHCPACK, DHCPNAK);
    if (ret == -1) {
        goto restart;
    } else if (ret < 0) {
        goto out;
    }

    printf("ip  = %08x\n", device->dhcp.client_ip);
    printf("sub = %08x\n", device->dhcp.subnet);
    printf("gw  = %08x\n", device->dhcp.gateway);
    printf("dns = %08x\n", device->dhcp.nameserver);

    device->dhcp.valid = true;
    ret = 0;
out:
    if (ret != 0) {
        printf("DHCP-Anfrage für %s/%d fehlgeschlagen: %s\n",
            device->driver->name, device->dev.number, strerror(-ret));
    }
    udp_close(s);

    return ret;
}

/**
 * Konfiguriert eine Netzwerkkarte über DHCP
 */
int dhcp_configure(struct device* device)
{
    int ret;
    struct routing_entry* entry;

    /* Falls die DHCP-Daten noch nicht geholt sind, erstmal machen */
    if (!device->dhcp.valid) {
        ret = dhcp_fetch(device);
        if (ret < 0) {
            return ret;
        }
    }

    /* Netzwerkkarte entsprechend umkonfigurieren */
    device->ip = device->dhcp.client_ip;

    remove_device_routes(device);

    if (device->dhcp.gateway) {
        entry = malloc(sizeof(*entry));
        *entry = (struct routing_entry) {
            .target     = 0,
            .subnet     = 0,
            .gateway    = device->dhcp.gateway,
            .device     = device,
        };
        add_routing_entry(entry);
    }

    entry = malloc(sizeof(*entry));
    *entry = (struct routing_entry) {
        .target = device->dhcp.client_ip & device->dhcp.subnet,
        .subnet = device->dhcp.subnet,
        .device = device,
    };
    add_routing_entry(entry);

    if (!options.nameserver) {
        options.nameserver = device->dhcp.nameserver;
    }

    return 0;
}

/**
 * TODO Lesen aus der DHCP-Datei
 */
static size_t lio_dhcp_read(lostio_filehandle_t* fh, void* buf,
    size_t blocksize, size_t count)
{
    return 0;
}

/**
 * Behandelt Schreibzugriffe in die DHCP-Datei. Mögliche Werte sind "fetch",
 * "configure" und "release"
 */
static size_t lio_dhcp_write(lostio_filehandle_t* fh, size_t blocksize,
    size_t count, void* buf)
{
    struct device *device = fh->node->data;
    size_t len = count * blocksize;
    char* p;
    int ret;

    /* Sicherstellen, dass der Puffer nullterminiert ist */
    p = buf;
    p[len - 1] = '\0';

    /* Alles abschneiden, was keine Buchstaben sind */
    while (isalpha(*p)) {
        p++;
    }
    *p = '\0';

    /* Entsprechende Aktion ausführen */
    if (!strcmp(buf, "fetch")) {
        ret = dhcp_fetch(device);
    } else if (!strcmp(buf, "configure")) {
        ret = dhcp_configure(device);
    } else {
        ret = -EINVAL;
    }

    return ret == 0 ? len : 0;
}

/** Typ fuer die DHCP-Steuerdatei */
static typehandle_t lio_type_dhcp = {
    .id          = LOSTIO_TYPES_DHCP,
    .read        = &lio_dhcp_read,
    .write       = &lio_dhcp_write,
};

/**
 * Registriert die LostIO-Typehandles, die fuer DHCP benoetigt werden
 */
void dhcp_register_lostio(void)
{
    lostio_register_typehandle(&lio_type_dhcp);
}
