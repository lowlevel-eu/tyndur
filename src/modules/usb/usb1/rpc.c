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

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <collections.h>
#include <init.h>
#include <rpc.h>
#include <syscall.h>

#include "usb.h"
#include "usb-ddrv.h"
#include "usb-hc.h"
#include "usb-server.h"

list_t* usb_hcs;
list_t* usb_ddrvs;
list_t* unhandled_devs;

extern void scan_hc(struct usb_hc* hc);

static void hcd_handler(pid_t src, uint32_t corr_id, size_t length, void* data)
{
    struct usb_hc* hc;

    if (length < sizeof(*hc)) {
        rpc_send_int_response(src, corr_id, 0);
        return;
    }

    hc = malloc(sizeof(*hc));
    memcpy(hc, data, sizeof(*hc));

    hc->pid = src;
    hc->addr_counter = 1;
    list_push(usb_hcs, hc);

    scan_hc(hc);

    rpc_send_int_response(src, corr_id, 42);
}

#define check(name) if ((dev->type.name != hd->name) && (hd->name != -1)) \
    { continue; }

static void ddrv_handler(pid_t src, uint32_t corr_id, size_t length, void* data)
{
    struct usb_handle_devs* hd = data;

    if (length < sizeof(*hd)) {
        rpc_send_int_response(src, corr_id, 0);
        return;
    }

    p();
    struct usb_ddrv* drv = calloc(1, sizeof(*drv));
    drv->pid = src;
    drv->handle = *hd;
    list_push(usb_ddrvs, drv);

    int i, s = list_size(unhandled_devs);
    for (i = 0; i < s; i++) {
        struct usb_prov_dev* dev = list_get_element_at(unhandled_devs, i);

        check(vendor_id)
        check(device_id)
        check(class)
        check(subclass)
        check(protocol)

        if (rpc_get_int(src, RPC_USB_DDRV_PLUG_FNN, sizeof(*dev), (char*)dev)) {
            list_remove(unhandled_devs, i--);
            s--;
        }
    }
    v();

    rpc_send_int_response(src, corr_id, 42);
}

static void devctl_handler(pid_t src, uint32_t corr_id, size_t length,
    void* data)
{
    struct usb_devctrl_req* rq = data;

    if (length < sizeof(*rq)) {
        rpc_send_int_response(src, corr_id, 0);
        return;
    }

    struct usb_device* dev;
    struct usb_interface_descriptor* ifc;
    int ifcs, ifc_num, i, ret;

    switch (rq->type) {
        case USB_DC_NUM_ENDPOINTS:
        {
            if (length < sizeof(struct usb_devctrl_gne)) {
                rpc_send_int_response(src, corr_id, 0);
                return;
            }

            struct usb_devctrl_gne* ne = data;
            dev = ne->dev.dev_id;
            ifc = (struct usb_interface_descriptor*)(dev->cur_cfg + 1);

            ifcs = dev->cur_cfg->num_interfaces;
            ifc_num = ne->dev.interface_id;
            if ((ifc_num < 0) || (ifc_num >= ifcs)) {
                ifc_num = 0;
            }

            for (i = 0; i < ifcs; i++) {
                if (i == ifc_num) {
                    rpc_send_int_response(src, corr_id, ifc->num_endpoints);
                    return;
                }

                ifc = (struct usb_interface_descriptor*)
                    ((struct usb_endpoint_descriptor*)(ifc + 1) +
                    ifc->num_endpoints);
            }
            // Hierhin sollte der Code nie gelangen
            // Execution should never get here
            break;
        }

        case USB_DC_GET_IFC_ID:
        {
            if (length < sizeof(struct usb_devctrl_gii)) {
                rpc_send_int_response(src, corr_id, 0);
                return;
            }

            struct usb_devctrl_gii* ii = data;
            dev = ii->dev.dev_id;
            ifc = (struct usb_interface_descriptor*)(dev->cur_cfg + 1);

            ifcs = dev->cur_cfg->num_interfaces;
            // Das ist nicht die, die wir suchen
            // This is not the one we're looking for
            ifc_num = ii->dev.interface_id;
            if ((ifc_num < 0) || (ifc_num >= ifcs)) {
                ifc_num = 0;
            }

            for (i = 0; i < ifcs; i++) {
                if (i == ifc_num) {
                    rpc_send_int_response(src, corr_id, ifc->interface_number);
                    return;
                }

                ifc = (struct usb_interface_descriptor*)
                    ((struct usb_endpoint_descriptor*)(ifc + 1) +
                    ifc->num_endpoints);
            }
            // s. o.
            break;
        }

        case USB_DC_GET_ENDPOINT:
        {
            if (length < sizeof(struct usb_devctrl_gep)) {
                // Hm, geht wohl nicht anders (und vertretbar, da dieser Fall
                // nie auftreten sollte)
                rpc_send_response(src, corr_id, 0, NULL);
                return;
            }

            struct usb_devctrl_gep* ge = data;
            dev = ge->dev.dev_id;
            ifc = (struct usb_interface_descriptor*)(dev->cur_cfg + 1);

            ifcs = dev->cur_cfg->num_interfaces;
            ifc_num = ge->dev.interface_id;
            if ((ifc_num < 0) || (ifc_num >= ifcs)) {
                ifc_num = 0;
            }

            for (i = 0; i < ifcs; i++) {
                if (i == ifc_num) {
                    struct usb_endpoint_descriptor* epd =
                        (struct usb_endpoint_descriptor*)(ifc + 1);
                    rpc_send_response(src, corr_id, sizeof(*epd),
                        (char*)&epd[ge->ep_number]);
                    return;
                }

                ifc = (struct usb_interface_descriptor*)
                    ((struct usb_endpoint_descriptor*)(ifc + 1) +
                    ifc->num_endpoints);
            }
            // s. o.
            break;
        }

        case USB_DC_GET_PIPE:
        {
            if (length < sizeof(struct usb_devctrl_gpp)) {
                // Auch hier wohl nur so möglich
                rpc_send_response(src, corr_id, 0, NULL);
                return;
            }

            struct usb_devctrl_gpp* gp = data;
            dev = gp->dev.dev_id;
            ifc = (struct usb_interface_descriptor*)(dev->cur_cfg + 1);

            ifcs = dev->cur_cfg->num_interfaces;
            ifc_num = gp->dev.interface_id;
            if ((ifc_num < 0) || (ifc_num >= ifcs)) {
                ifc_num = 0;
            }

            for (i = 0; i < ifcs; i++) {
                if (i == ifc_num) {
                    if ((gp->ep_number < 0) ||
                        (gp->ep_number >= ifc->num_endpoints))
                    {
                        gp->ep_number = 0;
                    }

                    struct usb_endpoint_descriptor* epd =
                        (struct usb_endpoint_descriptor*)(ifc + 1);
                    epd += gp->ep_number;

                    struct usb_pipe pp = {
                        .dev = dev,
                        .ep = epd,
                        .trs = {
                            .usb_addr = dev->addr,
                            .ep_id = epd->number,
                            .mps = epd->mps,
                            .type = epd->type,
                            .speed = dev->speed,
                            .toggle = 0,
                            .stalled = false
                        }
                    };

                    if (dev->parent->add_pipe != NULL) {
                        dev->parent->add_pipe(dev->parent, &pp);
                    }

                    rpc_send_response(src, corr_id, sizeof(pp), (char*)&pp);
                    return;
                }

                ifc = (struct usb_interface_descriptor*)
                    ((struct usb_endpoint_descriptor*)(ifc + 1) +
                    ifc->num_endpoints);
            }
            // s. o.
            break;
        }

        case USB_DC_CLEAR_HALT:
        {
            if (length < sizeof(struct usb_devctrl_clh)) {
                rpc_send_int_response(src, corr_id, -EINVAL);
                return;
            }

            struct usb_devctrl_clh* clrh = data;
            usb_clear_halt(&clrh->pipe);
            rpc_send_int_response(src, corr_id, 0);
            break;
        }

        case USB_DC_TRSF_CONTROL:
        {
            if (length < sizeof(struct usb_devctrl_tct)) {
                rpc_send_int_response(src, corr_id, -EINVAL);
                return;
            }

            struct usb_devctrl_tct* tctrq = data;
            if (tctrq->ep0) {
                dev = tctrq->dst.dev_id;
                ret = usb_shm_control_transfer(dev->ep0, tctrq->setup.type,
                    tctrq->setup.request, tctrq->setup.value,
                    tctrq->setup.index, tctrq->setup.length, tctrq->shm);
            } else {
                ret = usb_shm_control_transfer(&tctrq->dst.pipe,
                    tctrq->setup.type, tctrq->setup.request, tctrq->setup.value,
                    tctrq->setup.index, tctrq->setup.length, tctrq->shm);
            }

            rpc_send_int_response(src, corr_id, ret);
            break;
        }

        case USB_DC_TRSF_BULK:
        {
            if (length < sizeof(struct usb_devctrl_tbk)) {
                rpc_send_int_response(src, corr_id, -EINVAL);
                return;
            }

            struct usb_devctrl_tbk* tbkrq = data;
            ret = usb_shm_bulk_transfer(&tbkrq->pipe, tbkrq->shm,
                tbkrq->length);

            // Senden des Togglebitstatus erwünscht
            rpc_send_int_response(src, corr_id, ret);
            break;
        }
    }
}

void register_rpc_interface(void)
{
    register_message_handler(RPC_USB_SV_REGHC_FNN, &hcd_handler);
    register_message_handler(RPC_USB_SV_REGDD_FNN, &ddrv_handler);
    register_message_handler(RPC_USB_SV_DEVCT_FNN, &devctl_handler);
    init_service_register("usb1");
}
