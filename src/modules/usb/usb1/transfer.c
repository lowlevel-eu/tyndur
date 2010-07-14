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
#include <string.h>

#include <rpc.h>
#include <syscall.h>

#include "usb.h"
#include "usb-ddrv.h"
#include "usb-hc.h"

int usb_control_transfer(struct usb_pipe* pipe, int type, int request,
    int value, int index, int length, void* buffer)
{
    uint32_t shm = 0;
    void *shm_addr = NULL;

    if (length) {
        shm = create_shared_memory(length);
        shm_addr = open_shared_memory(shm);

        if ((buffer != NULL) && !(type & USB_SETUP_IN)) {
            memcpy(shm_addr, buffer, length);
        }
    }

    int ret = usb_shm_control_transfer(pipe, type, request, value, index,
        length, shm);

    if (length && (buffer != NULL) && (type & USB_SETUP_IN)) {
        memcpy(buffer, shm_addr, length);
    }

    if (shm_addr != NULL) {
        close_shared_memory(shm);
    }

    return ret;
}

int usb_shm_control_transfer(struct usb_pipe* pipe, int type, int request,
    int value, int index, int length, uint32_t shm)
{
    struct usb_hc_trs_ctrl trs = {
        .trs = {
            .type = USB_TRST_CONTROL,
            .hc = *pipe->dev->hc,
            .pipe = pipe->trs
        },
        .setup = {
            .type = type,
            .request = request,
            .value = value,
            .index = index,
            .length = length
        },
        .shm = shm
    };

    return rpc_get_int(pipe->dev->hc->pid, RPC_USB_HC_TRANSFER_FNN,
        sizeof(trs), (char*)&trs);
}

int usb_shm_bulk_transfer(struct usb_pipe* pipe, uint32_t shm, size_t length)
{
    struct usb_hc_trs_bulk trs = {
        .trs = {
            .type = USB_TRST_BULK,
            .hc = *pipe->dev->hc,
            .pipe = pipe->trs
        },
        .receive = pipe->ep->direction ? true : false,
        .length = length,
        .shm = shm
    };

    return rpc_get_int(pipe->dev->hc->pid, RPC_USB_HC_TRANSFER_FNN,
        sizeof(trs), (char*)&trs);
}

void usb_set_configuration(struct usb_device* dev, int config)
{
    usb_control_transfer(dev->ep0,
        USB_SETUP_OUT | USB_SETUP_REQ_STD | USB_SETUP_REC_DEV,
        USB_SET_CONFIGURATION, config, 0, 0, NULL);
    dev->state = USB_CONFIGURED;
}

void usb_clear_halt(struct usb_pipe* pipe)
{
    usb_control_transfer(pipe->dev->ep0,
        USB_SETUP_OUT | USB_SETUP_REQ_STD | USB_SETUP_REC_EP,
        USB_CLEAR_FEATURE, USB_ENDPOINT_HALT, pipe->ep->number, 0, NULL);
}
