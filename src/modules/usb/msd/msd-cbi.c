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
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "msd.h"
#include "msd-cbi.h"
#include "usb-lib.h"

int msd_cbi_issue_scsi_cmd(struct msc_gen_dev* dev, const void* cmd_buffer,
    size_t cmd_length, bool receive, size_t req_length)
{
    struct msc_cbi_dev* cbi = (struct msc_cbi_dev*)dev;
    int8_t cmd[12] = { 0 };
    struct usb_setup_packet setup = {
        .type = USB_SETUP_OUT | USB_SETUP_REQ_CLS | USB_SETUP_REC_IF,
        .request = 0,
        .value = 0,
        .index = cbi->ifc_id,
        .length = sizeof(cmd)
    };

    if (cmd_length > 12) {
        cmd_length = 12;
    }

    memcpy(cmd, cmd_buffer, cmd_length);

    int ret = usb_control_transfer(cbi->dev, NULL, &setup, cmd);

    /*
    if (ret == -EHOSTDOWN) {
        // kA, ob das etwas bringt
        if (receive == true) {
            usb_clear_halt(cbi->bip);
        } else {
            usb_clear_halt(cbi->bop);
        }
    }
    */

    return ret;
}

int msd_cbi_send(struct msc_gen_dev* dev, const void* buffer, size_t size)
{
    struct msc_cbi_dev* cbi = (struct msc_cbi_dev*)dev;
    return usb_bulk_transfer(cbi->bop, (void*)buffer, size, false);
}

int msd_cbi_recv(struct msc_gen_dev* dev, void* buffer, size_t size)
{
    struct msc_cbi_dev* cbi = (struct msc_cbi_dev*)dev;
    return usb_bulk_transfer(cbi->bip, buffer, size, true);
}
