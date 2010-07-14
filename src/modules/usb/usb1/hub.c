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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <collections.h>
#include <rpc.h>
#include <sleep.h>

#include "usb.h"
#include "usb-hc.h"

static int scan_hub_port(struct usb_hub* hub, int port)
{
    uint32_t status;

    usb_control_transfer(hub->dev.usb->ep0,
        USB_SETUP_IN | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH, USB_GET_STATUS,
        0, port + 1, 4, &status);

    if (!(status & (1 << 0))) {
        return 0;
    }

    if (status & (1 << 9)) {
        return USB_HUB_DEVICE | USB_HUB_LOW_SPEED;
    } else if (status & (1 << 10)) {
        return USB_HUB_DEVICE | USB_HUB_HIGH_SPEED;
    } else {
        return USB_HUB_DEVICE | USB_HUB_FULL_SPEED;
    }
}

static void disable_hub_port(struct usb_hub *hub, int port)
{
    // Deaktivieren
    usb_control_transfer(hub->dev.usb->ep0,
        USB_SETUP_OUT | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH,
        USB_CLEAR_FEATURE, 1, port + 1, 0, NULL);
}

static void reset_hub_port(struct usb_hub* hub, int port)
{
    uint32_t cur;

    port++;

    usb_control_transfer(hub->dev.usb->ep0,
        USB_SETUP_IN | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH, USB_GET_STATUS,
        0, port, 4, &cur);

    // Zurücksetzen
    usb_control_transfer(hub->dev.usb->ep0,
        USB_SETUP_OUT | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH,
        USB_SET_FEATURE, 4, port, 0, NULL);

    msleep(20);

    if (!(cur & (1 << 1)) || (cur & (1 << 2))) {
        // Vorher deaktiviert oder im Suspendmodus, Resume treiben
        usb_control_transfer(hub->dev.usb->ep0,
            USB_SETUP_OUT | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH,
            USB_CLEAR_FEATURE, 2, port, 0, NULL);
        msleep(30);
    }
}

void usb_found_hub(struct usb_device* dev)
{
    struct usb_config_descriptor* ccfg;
    struct usb_interface_descriptor* cint;
    struct usb_hub* hub;
    struct usb_hub_descriptor* chub;
    uint8_t chub_buf[256];

    dev->cur_cfg = NULL;

    int c, i;

    for (c = 0; c < dev->dev_desc->num_configurations; c++) {
        ccfg = dev->cfg_desc[c];
        cint = (struct usb_interface_descriptor*)(dev->cfg_desc[c] + 1);
        for (i = 0; i < ccfg->num_interfaces; i++) {
            if (cint->class == 9) {
                dev->cur_cfg = ccfg;
                break;
            }

            cint = (struct usb_interface_descriptor*)
                ((struct usb_endpoint_descriptor*)(cint + 1) +
                cint->num_endpoints);
        }
    }

    if (dev->cur_cfg == NULL) {
        return;
    }

    usb_set_configuration(dev, dev->cur_cfg->configuration_value);

    hub = calloc(1, sizeof(*hub));
    if (hub == NULL) {
        return;
    }

    hub->dev.usb = dev;
    hub->root_hub = false;

    usb_control_transfer(dev->ep0,
        USB_SETUP_IN | USB_SETUP_REQ_CLS | USB_SETUP_REC_DEV,
        USB_GET_DESCRIPTOR, USB_DESC_HUB, 0, 256, chub_buf);

    chub = (struct usb_hub_descriptor*)chub_buf;

    hub->ports = chub->nbr_ports;

    for (i = 1; i <= hub->ports; i++) {
        // Strom einschalten
        usb_control_transfer(dev->ep0,
            USB_SETUP_OUT | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH,
            USB_SET_FEATURE, 8, i, 0, NULL);

        msleep(chub->pwr_on_2_pwr_good * 2);
    }

    msleep(100);

    for (i = 1; i <= hub->ports; i++) {
        uint32_t status;

        usb_control_transfer(dev->ep0,
            USB_SETUP_IN | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH,
            USB_GET_STATUS, 0, i, 4, &status);

        if (status & (1 << 1)) {
            // Port deaktivieren
            usb_control_transfer(dev->ep0,
                USB_SETUP_OUT | USB_SETUP_REQ_CLS | USB_SETUP_REC_OTH,
                USB_CLEAR_FEATURE, 1, i, 0, NULL);
        }
    }

    hub->add_pipe = dev->parent->add_pipe;
    hub->disable_port = &disable_hub_port;
    hub->reset_port = &reset_hub_port;
    hub->scan_port = &scan_hub_port;

    scan_hub(hub);
}
