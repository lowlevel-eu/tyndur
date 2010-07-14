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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <rpc.h>
#include <syscall.h>

#include "usb-ddrv.h"
#include "usb-server.h"
#include "usb-structs.h"
#include "usb-trans.h"

pid_t usb_pid;

int usb_get_number_of_endpoints(struct usb_prov_dev* dev)
{
    struct usb_devctrl_gne rq = {
        .req = {
            .type = USB_DC_NUM_ENDPOINTS
        },
        .dev = *dev
    };

    return rpc_get_int(usb_pid, RPC_USB_SV_DEVCT_FNN, sizeof(rq), (char*)&rq);
}

int usb_get_interface_id(struct usb_prov_dev* dev)
{
    struct usb_devctrl_gii giid = {
        .req = {
            .type = USB_DC_GET_IFC_ID
        },
        .dev = *dev
    };

    return rpc_get_int(usb_pid, RPC_USB_SV_DEVCT_FNN, sizeof(giid),
        (char*)&giid);
}

void usb_get_endpoint_info(struct usb_prov_dev* dev, int ep_number,
    struct usb_endpoint_descriptor* ep_desc)
{
    struct usb_devctrl_gep gep = {
        .req = {
            .type = USB_DC_GET_ENDPOINT
        },
        .dev = *dev,
        .ep_number = ep_number
    };

    response_t* r = rpc_get_response(usb_pid, RPC_USB_SV_DEVCT_FNN,
        sizeof(gep), (char*)&gep);

    memcpy(ep_desc, r->data, sizeof(*ep_desc));
    free(r->data);
    free(r);
}

void usb_create_pipe(struct usb_prov_dev* dev, int ep_number,
    struct usb_pipe* pipe)
{
    struct usb_devctrl_gpp gp = {
        .req = {
            .type = USB_DC_GET_PIPE
        },
        .dev = *dev,
        .ep_number = ep_number
    };

    response_t* r = rpc_get_response(usb_pid, RPC_USB_SV_DEVCT_FNN,
        sizeof(gp), (char*)&gp);

    memcpy(pipe, r->data, sizeof(*pipe));
    free(r->data);
    free(r);
}

int usb_control_transfer(struct usb_prov_dev* dev, struct usb_pipe* pipe,
    struct usb_setup_packet* setup, void* buffer)
{
    uint32_t shm = 0;
    void* shm_addr = NULL;

    if (setup->length) {
        shm = create_shared_memory(setup->length);
        shm_addr = open_shared_memory(shm);

        if ((buffer != NULL) && !(setup->type & USB_SETUP_IN)) {
            memcpy(shm_addr, buffer, setup->length);
        }
    }

    struct usb_devctrl_tct tctrq = {
        .req = {
            .type = USB_DC_TRSF_CONTROL
        },
        .setup = *setup,
        .shm = shm
    };

    if (pipe != NULL) {
        tctrq.dst.pipe = *pipe;
        tctrq.ep0 = false;
    } else {
        tctrq.dst.dev_id = dev->dev_id;
        tctrq.ep0 = true;
    }

    int ret = rpc_get_int(usb_pid, RPC_USB_SV_DEVCT_FNN, sizeof(tctrq),
        (char*)&tctrq);

    if (setup->length && (buffer != NULL) && (setup->type & USB_SETUP_IN)) {
        memcpy(buffer, shm_addr, setup->length);
    }

    if (shm_addr != NULL) {
        close_shared_memory(shm);
    }

    return ret;
}

int usb_bulk_transfer(struct usb_pipe* pipe, void* buffer, size_t length,
    bool receive)
{
    uint32_t shm = 0;
    void* shm_addr = NULL;

    if (length) {
        shm = create_shared_memory(length);
        shm_addr = open_shared_memory(shm);

        if ((buffer != NULL) && !receive) {
            memcpy(shm_addr, buffer, length);
        }
    }

    struct usb_devctrl_tbk tbkrq = {
        .req = {
            .type = USB_DC_TRSF_BULK
        },
        .pipe = *pipe,
        .length = length,
        .shm = shm
    };

    int ret = rpc_get_int(usb_pid, RPC_USB_SV_DEVCT_FNN, sizeof(tbkrq),
        (char*)&tbkrq);

    if (length && (buffer != NULL) && receive) {
        memcpy(buffer, shm_addr, length);
    }

    if (shm_addr != NULL) {
        close_shared_memory(shm);
    }

    if (ret >= 0) {
        pipe->trs.toggle = ret;
        ret = 0;
    }

    return ret;
}

void usb_clear_halt(struct usb_pipe* pipe)
{
    struct usb_devctrl_clh clh = {
        .req = {
            .type = USB_DC_CLEAR_HALT
        },
        .pipe = *pipe
    };

    rpc_get_int(usb_pid, RPC_USB_SV_DEVCT_FNN, sizeof(clh), (char*)&clh);
}
