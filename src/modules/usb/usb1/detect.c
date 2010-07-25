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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <collections.h>
#include <rpc.h>
#include <syscall.h>

#include "usb.h"
#include "usb-ddrv.h"
#include "usb-hc.h"
#include "usb-hub.h"

extern void usb_found_hub(struct usb_device* dev);

extern list_t* usb_ddrvs;
extern list_t* unhandled_devs;

static const char* speed_name(int speed)
{
    switch (speed) {
        case USB_LOW_SPEED:
            return "low speed";
        case USB_FULL_SPEED:
            return "full speed";
        case USB_HIGH_SPEED:
            return "high speed";
        default:
            return "unknown speed";
    }
}

static int fits(int is, int want)
{
    if (want == -1)
        return -1;
    return !!(is == want);
}

static int provide_to(struct usb_device* dev, int ifc_num,
    struct usb_interface_descriptor* ifc, struct usb_ddrv* drv)
{
    struct usb_prov_dev pd = {
        .is_interface = (ifc == NULL) ? false : true,
        .dev_id = dev,
        .interface_id = ifc_num,
        .type = {
            .vendor_id = dev->dev_desc->vendor_id,
            .device_id = dev->dev_desc->device_id,
            .class = (ifc == NULL) ? dev->dev_desc->class : ifc->class,
            .subclass = (ifc == NULL) ? dev->dev_desc->subclass : ifc->subclass,
            .protocol = (ifc == NULL) ? dev->dev_desc->protocol : ifc->protocol
        }
    };

    return rpc_get_int(drv->pid, RPC_USB_DDRV_PLUG_FNN, sizeof(pd), (char*)&pd);
}

static void dev_is_unhandled(struct usb_device* dev, int ifc_num,
    struct usb_interface_descriptor* ifc)
{
    struct usb_prov_dev* pd = calloc(1, sizeof(*pd));

    pd->is_interface = (ifc == NULL) ? false : true;
    pd->dev_id = dev;
    pd->interface_id = ifc_num;
    pd->type.vendor_id = dev->dev_desc->vendor_id;
    pd->type.device_id = dev->dev_desc->device_id;
    pd->type.class = (ifc == NULL) ? dev->dev_desc->class : ifc->class;
    pd->type.subclass = (ifc == NULL) ? dev->dev_desc->subclass : ifc->subclass;
    pd->type.protocol = (ifc == NULL) ? dev->dev_desc->protocol : ifc->protocol;

    p();
    list_push(unhandled_devs, pd);
    v();
}

#define check(svar, lname) int svar##f = fits(svar, drv->handle.lname); \
    if (!svar##f) { continue; }

#define provto(check) if (check && provide_to(dev, ifc_num, ifc, drv)) \
    { break; }

static void provide(struct usb_device* dev, int ifc_num,
    struct usb_interface_descriptor* ifc)
{
    int vid, did, cls, scl, ptc;

    vid = dev->dev_desc->vendor_id;
    did = dev->dev_desc->device_id;
    if (ifc == NULL) {
        cls = dev->dev_desc->class;
        scl = dev->dev_desc->subclass;
        ptc = dev->dev_desc->protocol;
    } else {
        cls = ifc->class;
        scl = ifc->subclass;
        ptc = ifc->protocol;
    }

    int i, s = list_size(usb_ddrvs);
    for (i = 0; i < s; i++) {
        struct usb_ddrv* drv = list_get_element_at(usb_ddrvs, i);

        check(vid, vendor_id)
        check(did, device_id)
        check(cls, class)
        check(scl, subclass)
        check(ptc, protocol)

        // Wenn jemandem noch eine sinnvolle Kombination einfällt, kann er die
        // gern hier eintragen.
        // If anyone knows another combination making sense, he is encouraged
        // to insert it here.
        provto(vidf && didf)
        provto(vidf &&         clsf && sclf && ptcf)
        provto(                clsf && sclf && ptcf)
        provto(vidf &&         clsf && sclf)
        provto(                clsf && sclf)
        provto(vidf &&         clsf)
        provto(                clsf)
    }

    if (i == s) {
        dev_is_unhandled(dev, ifc_num, ifc);
    }
}

void update_ep_trs_info(struct usb_pipe* ep)
{
    ep->trs.usb_addr = ep->dev->addr;
    ep->trs.ep_id = ep->ep->number;
    ep->trs.mps = ep->ep->mps;
    ep->trs.type = ep->ep->type;
    ep->trs.speed = ep->dev->speed;
}

#define TNC(fnc) if ((ret = fnc)) { \
    printf("[usb1] err@%i, ret is %i\n" # fnc "\n", __LINE__, ret); return; }

void enumerate_device(struct usb_device* dev)
{
    struct usb_hub* hub = dev->parent;
    int p = dev->hub_port;

    printf("[usb1] Enumerating %s device on port %i of some hub\n", speed_name(dev->speed), dev->hub_port);

    hub->reset_port(hub, p);
    dev->addr = 0;
    dev->state = USB_DEFAULT;

    struct usb_pipe* ep0;
    dev->ep0 = ep0 = calloc(1, sizeof(*ep0));

    ep0->dev = dev;
    ep0->ep = calloc(1, sizeof(*ep0->ep));
    ep0->ep->number = 0;
    ep0->ep->mps = 8;
    ep0->ep->type = USB_EP_CONTROL;

    update_ep_trs_info(ep0);

    ep0->trs.toggle = 0;
    ep0->trs.stalled = false;

    if (hub->add_pipe != NULL) {
        hub->add_pipe(hub, ep0);
    }

    dev->dev_desc = calloc(1, sizeof(*dev->dev_desc));

    int ret = usb_control_transfer(ep0,
        USB_SETUP_IN | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV,
        USB_GET_DESCRIPTOR, USB_DESC_DEVICE | 0, 0, 8, dev->dev_desc);

    printf("[usb1] mps0 is %i B\n", dev->dev_desc->mps0);

    if (ret) {
        fprintf(stderr, "[usb1] Warning: first transfer returned %i\n", ret);
    }

    if (!dev->dev_desc->mps0) {
        return;
    }

    hub->reset_port(hub, p);
    ep0->ep->mps = dev->dev_desc->mps0;
    ep0->trs.stalled = false;
    update_ep_trs_info(ep0);

    int address = dev->hc->addr_counter++;
    TNC(usb_control_transfer(ep0,
        USB_SETUP_OUT | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV, USB_SET_ADDRESS,
        address, 0, 0, NULL));
    dev->addr = address;
    update_ep_trs_info(ep0);

    if (hub->add_pipe != NULL) {
        hub->add_pipe(hub, dev->ep0);
    }

    dev->state = USB_ADDRESS;

    TNC(usb_control_transfer(ep0,
        USB_SETUP_IN | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV,
        USB_GET_DESCRIPTOR, USB_DESC_DEVICE | 0, 0, sizeof(*dev->dev_desc),
        dev->dev_desc));

    printf("[usb1] 0x%04X:0x%04X (%i configs)\n", dev->dev_desc->vendor_id,
        dev->dev_desc->device_id, dev->dev_desc->num_configurations);

    int i;

    if (dev->dev_desc->i_manufacturer) {
        uint8_t name[255];

        TNC(usb_control_transfer(ep0,
            USB_SETUP_IN | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV,
            USB_GET_DESCRIPTOR, USB_DESC_STRING | dev->dev_desc->i_manufacturer,
            0, 255, name));

        printf("[usb1]  -> Manufacturer: ");
        for (i = 2; i < name[0]; i += 2) {
            printf("%c", name[i]);
        }
        printf("\n");
    }
    if (dev->dev_desc->i_product) {
        uint8_t name[255];

        TNC(usb_control_transfer(ep0,
            USB_SETUP_IN | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV,
            USB_GET_DESCRIPTOR, USB_DESC_STRING | dev->dev_desc->i_product,
            0, 255, name));

        printf("[usb1]  -> Product: ");
        for (i = 2; i < name[0]; i += 2) {
            printf("%c", name[i]);
        }
        printf("\n");
    }

    dev->cfg_desc = calloc(dev->dev_desc->num_configurations, sizeof(*dev->cfg_desc));
    for (i = 0; i < dev->dev_desc->num_configurations; i++) {
        uint8_t tmp[9];

        // Erstmal Länge aller Deskriptoren herausfinden
        TNC(usb_control_transfer(ep0,
            USB_SETUP_IN | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV,
            USB_GET_DESCRIPTOR, USB_DESC_CONFIGURATION | i, 0, 9, tmp));
        dev->cfg_desc[i] = calloc(1, tmp[2] | (tmp[3] << 8));
        TNC(usb_control_transfer(ep0,
            USB_SETUP_IN | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV,
            USB_GET_DESCRIPTOR, USB_DESC_CONFIGURATION | i, 0,
            tmp[2] | (tmp[3] << 8), dev->cfg_desc[i]));
    }

    if (dev->dev_desc->class == 9) {
        usb_found_hub(dev);
        return;
    }

    if (dev->dev_desc->class) {
        provide(dev, 0, NULL);
    }

    struct usb_config_descriptor* best_cfg = dev->cfg_desc[0];

    int c;
    for (c = 0; c < dev->dev_desc->num_configurations; c++) {
        struct usb_config_descriptor* cur_cfg = dev->cfg_desc[c];

        if (cur_cfg->self_powered && !best_cfg->self_powered) {
            // Wir wollen möglichst Self-powered
            best_cfg = cur_cfg;
        } else if (cur_cfg->max_power > best_cfg->max_power) {
            // Und möglichst hohen Stromverbrauch (FIXME: Das aber nicht immer)
            best_cfg = cur_cfg;
        }
    }

    dev->cur_cfg = best_cfg;
    usb_set_configuration(dev, best_cfg->configuration_value);

    struct usb_interface_descriptor* cur_ifc = (struct usb_interface_descriptor*)(best_cfg + 1);

    for (i = 0; i < best_cfg->num_interfaces; i++) {
        if (cur_ifc->class && !cur_ifc->alternate_setting) {
            provide(dev, i, cur_ifc);
        }

        cur_ifc = (struct usb_interface_descriptor*)
            ((struct usb_endpoint_descriptor*)(cur_ifc + 1) +
            cur_ifc->num_endpoints);
    }
}

void scan_hub(struct usb_hub* hub)
{
    int p;

    printf("[usb1] Hub with %i ports detected.\n", hub->ports);

    for (p = 0; p < hub->ports; p++) {
        hub->disable_port(hub, p);
    }

    for (p = 0; p < hub->ports; p++) {
        int stat = hub->scan_port(hub, p);

        if (stat & USB_HUB_DEVICE) {
            struct usb_device* dev = calloc(1, sizeof(*dev));

            dev->parent = hub;
            dev->hub_port = p;

            if (hub->root_hub) {
                dev->hc = hub->dev.hc;
            } else {
                dev->hc = hub->dev.usb->hc;
            }

            dev->addr = 0;
            dev->state = USB_ATTACHED;
            dev->speed = stat & USB_HUB_SPEED;

            enumerate_device(dev);
        }
    }
}

static void rh_addpipe(struct usb_hub* hub, struct usb_pipe* pipe)
{
    struct usb_hc* hc;

    if (hub->root_hub) {
        hc = hub->dev.hc;
    } else {
        hc = hub->dev.usb->hc;
    }

    struct usb_hc_ctrl_appe appe = {
        .req = {
            .type = HC_CTRL_ADD_PIPE,
            .hc = *hc
        },
        .pipe = pipe->trs
    };

    rpc_get_int(hc->pid, RPC_USB_HC_CONTROL_FNN, sizeof(appe),
        (char*)&appe);
}

static void rh_rst(struct usb_hub* hub, int port)
{
    struct usb_hc_ctrl_rst rqrst = {
        .req = {
            .type = HC_CTRL_RESET,
            .hc = *hub->dev.hc
        },
        .port = port
    };

    rpc_get_int(hub->dev.hc->pid, RPC_USB_HC_CONTROL_FNN, sizeof(rqrst),
        (char*)&rqrst);
}

static int rh_scan(struct usb_hub* hub, int port)
{
    struct usb_hc_ctrl_scan rqsc = {
        .req = {
            .type = HC_CTRL_SCAN_PORT,
            .hc = *hub->dev.hc
        },
        .port = port
    };

    return rpc_get_int(hub->dev.hc->pid, RPC_USB_HC_CONTROL_FNN, sizeof(rqsc),
        (char*)&rqsc);
}

static void rh_disable(struct usb_hub* hub, int port)
{
    struct usb_hc_ctrl_dsbl rqdbl = {
        .req = {
            .type = HC_CTRL_DSBL_PORT,
            .hc = *hub->dev.hc
        },
        .port = port
    };

    rpc_get_int(hub->dev.hc->pid, RPC_USB_HC_CONTROL_FNN, sizeof(rqdbl),
        (char*)&rqdbl);
}

void scan_hc(struct usb_hc* hc)
{
    struct usb_hub* rh = malloc(sizeof(*rh));

    *rh = (struct usb_hub) {
        .dev = {
            .hc = hc
        },
        .root_hub = true,
        .ports = hc->root_ports,
        .add_pipe = &rh_addpipe,
        .disable_port = &rh_disable,
        .reset_port = &rh_rst,
        .scan_port = &rh_scan
    };

    scan_hub(rh);
}
