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
#include "msd-bo.h"
#include "usb-lib.h"

int check_csw(struct msc_bo_dev* dev)
{
    struct msd_bo_csw csw;

    int ret = usb_bulk_transfer(dev->bip, &csw, sizeof(csw), true);
    if (ret) {
        return ret;
    }

    // Man könnte wohl noch ein bisschen was überprüfen...
    // Some checks might be nice...
    return 0;
}

int msd_bo_issue_scsi_cmd(struct msc_gen_dev* dev, const void* cmd_buffer,
    size_t cmd_length, bool receive, size_t req_length)
{
    struct msc_bo_dev* bo = (struct msc_bo_dev*)dev;

    if (cmd_length > 16) {
        cmd_length = 16;
    }

    struct msd_bo_cbw cbw = {
        .signature = CBW_SIGNATURE,
        .data_transfer_length = req_length,
        .flags = receive ? CBW_IN : CBW_OUT,
        .lun = dev->lun,
        .cb_length = cmd_length
    };

    memcpy(cbw.cb, cmd_buffer, cmd_length);

    int ret = usb_bulk_transfer(bo->bop, &cbw, sizeof(cbw), false);
    if (ret || req_length) {
        return ret;
    }

    return check_csw(bo);
}

int msd_bo_send(struct msc_gen_dev* dev, const void* buffer, size_t size)
{
    struct msc_bo_dev* bo = (struct msc_bo_dev*)dev;

    int ret = usb_bulk_transfer(bo->bop, (void*)buffer, size, false);
    if (ret) {
        return ret;
    }

    return check_csw(bo);
}

int msd_bo_recv(struct msc_gen_dev* dev, void* buffer, size_t size)
{
    struct msc_bo_dev* bo = (struct msc_bo_dev*)dev;

    int ret = usb_bulk_transfer(bo->bip, buffer, size, true);
    if (ret) {
        return ret;
    }

    return check_csw(bo);
}
