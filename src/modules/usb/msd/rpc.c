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

#include <ports.h>
#include <rpc.h>
#include <syscall.h>

#include "msd.h"
#include "usb-ddrv.h"
#include "usb-server.h"

extern pid_t usb_pid;

void msd_got_device_handler(pid_t src, uint32_t corr_id, size_t length,
    void* data)
{
    struct usb_prov_dev* pd;
    int ret;

    if (length < sizeof(*pd)) {
        rpc_send_int_response(src, corr_id, 0);
        return;
    }

    pd = malloc(sizeof(*pd));
    memcpy(pd, data, sizeof(*pd));

    ret = is_valid_msd(pd);
    rpc_send_int_response(src, corr_id, ret);
    if (ret) {
        got_msd(pd);
    }

}

void init_rpc_interface(void)
{
    register_message_handler(RPC_USB_DDRV_PLUG_FNN, &msd_got_device_handler);
}

void register_msddrv(void)
{
    struct usb_handle_devs hd = {
        .vendor_id = -1,
        .device_id = -1,
        .class = 8, // MSC
        .subclass = -1,
        .protocol = -1
    };

    if (!rpc_get_int(usb_pid, RPC_USB_SV_REGDD_FNN, sizeof(hd), (char*)&hd)) {
        fprintf(stderr, "Could not register USB MSD driver.\n");
    }
}
