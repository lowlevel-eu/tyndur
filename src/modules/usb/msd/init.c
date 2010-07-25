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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msd.h"
#include "msd-bo.h"
#include "msd-cbi.h"
#include "usb-lib.h"
#include "usb-structs.h"
#include "usb-trans.h"

void got_bo_msd(struct usb_prov_dev* dev);
void got_cbi_msd(struct usb_prov_dev* dev);

int is_valid_msd(struct usb_prov_dev* dev)
{
    if (dev->type.protocol == 0x50) { // Bulk-only
        return 1;
    } else if ((dev->type.subclass == 0x04) && !dev->type.protocol) { // UFI/CBI
        return 1;
    }

    return 0;
}

void got_msd(struct usb_prov_dev* dev)
{
    if (dev->type.protocol == 0x50) { // Bulk-only
        got_bo_msd(dev);
    } else if ((dev->type.subclass == 0x04) && !dev->type.protocol) { // UFI/CBI
        got_cbi_msd(dev);
    } else {
        abort();
    }
}

void got_bo_msd(struct usb_prov_dev* dev)
{
    int endpoints = usb_get_number_of_endpoints(dev);
    int ifc_id = usb_get_interface_id(dev);
    struct usb_endpoint_descriptor* epd = calloc(1, sizeof(*epd));
    struct usb_endpoint_descriptor* bulk_in = malloc(sizeof(*bulk_in));
    struct usb_endpoint_descriptor* bulk_out = malloc(sizeof(*bulk_out));
    int bin = -1, bon = -1;

    int i;
    for (i = 0; i < endpoints; i++) {
        usb_get_endpoint_info(dev, i, epd);

        if (epd->type == USB_EP_BULK) {
            if (epd->direction) {
                memcpy(bulk_in, epd, sizeof(*epd));
                bin = i;
            } else {
                memcpy(bulk_out, epd, sizeof(*epd));
                bon = i;
            }
        }
    }

    free(epd);

    if ((bin == -1) || (bon == -1)) {
        free(bulk_in);
        free(bulk_out);
        return;
    }

    struct usb_pipe* bip = calloc(1, sizeof(*bip));
    struct usb_pipe* bop = calloc(1, sizeof(*bop));

    usb_create_pipe(dev, bin, bip);
    usb_create_pipe(dev, bon, bop);

    int max_lun = 0;
    usb_control_transfer(dev, NULL, &(struct usb_setup_packet){
            .type = USB_SETUP_IN | USB_SETUP_REQ_CLS | USB_SETUP_REC_IF,
            .request = 0xFE, // GET MAX LUN
            .index = ifc_id,
            .length = 1,
        }, &max_lun);

    for (i = 0; i <= max_lun; i++) {
        struct msc_bo_dev* new = calloc(1, sizeof(*new));

        new->gen.lun = i;
        new->gen.issue_scsi_cmd = &msd_bo_issue_scsi_cmd;
        new->gen.send = &msd_bo_send;
        new->gen.recv = &msd_bo_recv;

        new->bulk_in = bulk_in;
        new->bulk_out = bulk_out;

        new->bip = bip;
        new->bop = bop;

        provide_msd(&new->gen);
    }
}

void got_cbi_msd(struct usb_prov_dev* dev)
{
    int endpoints = usb_get_number_of_endpoints(dev);
    int ifc_id = usb_get_interface_id(dev);
    struct usb_endpoint_descriptor* epd = calloc(1, sizeof(*epd));
    struct usb_endpoint_descriptor* bulk_in = malloc(sizeof(*bulk_in));
    struct usb_endpoint_descriptor* bulk_out = malloc(sizeof(*bulk_out));
    struct usb_endpoint_descriptor* interrupt = malloc(sizeof(*interrupt));
    int bin = -1, bon = -1, ien = -1;

    int i;
    for (i = 0; i < endpoints; i++) {
        usb_get_endpoint_info(dev, i, epd);

        if (epd->type == USB_EP_BULK) {
            if (epd->direction) {
                memcpy(bulk_in, epd, sizeof(*epd));
                bin = i;
            } else {
                memcpy(bulk_out, epd, sizeof(*epd));
                bon = i;
            }
        } else if (epd->type == USB_EP_INTERRUPT) {
            memcpy(interrupt, epd, sizeof(*epd));
            ien = i;
        }
    }

    free(epd);

    if (dev->type.protocol || (ien == -1)) {
        free(interrupt);
        ien = -1;
    }

    if ((bin == -1) || (bon == -1)) {
        free(bulk_in);
        free(bulk_out);
        return;
    }

    struct usb_pipe* bip = calloc(1, sizeof(*bip));
    struct usb_pipe* bop = calloc(1, sizeof(*bop));
    struct usb_pipe* ip;

    usb_create_pipe(dev, bin, bip);
    usb_create_pipe(dev, bon, bop);

    if (ien != -1) {
        ip = calloc(1, sizeof(*ip));
        usb_create_pipe(dev, ien, ip);
    }

    struct msc_cbi_dev* new = calloc(1, sizeof(*new));

    new->gen.lun = 0;
    new->gen.issue_scsi_cmd = &msd_cbi_issue_scsi_cmd;
    new->gen.send = &msd_cbi_send;
    new->gen.recv = &msd_cbi_recv;

    new->dev = dev;
    new->ifc_id = ifc_id;

    new->bulk_in = bulk_in;
    new->bulk_out = bulk_out;

    new->bip = bip;
    new->bop = bop;

    if (ien != -1) {
        new->interrupt = interrupt;
        new->ip = ip;
    }

    provide_msd(&new->gen);
}
