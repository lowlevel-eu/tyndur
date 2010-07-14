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
#include <sys/types.h>

#include <ports.h>
#include <sleep.h>

#include "uhci.h"
#include "usb-hub.h"

void uhci_reset_port(struct uhci* hc, int port)
{
    if (hc == NULL)
        return;

    outw(hc->iobase + UHCI_RPORTS + port * 2, RPORT_ENABLE | RPORT_RESET | RPORT_CSC);
    msleep(50);
    uhci_set_port_status(hc, port, 1);
    msleep(10);
}

int uhci_scan_port(struct uhci* hc, int port)
{
    int ret;

    if (hc == NULL)
        return 0;

    uint_fast16_t val = inw(hc->iobase + UHCI_RPORTS + port * 2);

    if (!(val & RPORT_DEVICE)) {
        ret = 0;
    } else {
        ret = USB_HUB_DEVICE;
        if (val & RPORT_LOSPD) {
            ret |= USB_HUB_LOW_SPEED;
        } else {
            ret |= USB_HUB_FULL_SPEED;
        }

        if (val & RPORT_CSC) {
            ret |= USB_HUB_CHANGE;
            outw(hc->iobase + UHCI_RPORTS + port * 2, (val & RPORT_ENABLE) | RPORT_CSC);
        }
    }

    return ret;
}

void uhci_set_port_status(struct uhci* hc, int port, int status)
{
    if (hc == NULL)
        return;

    int previous = inw(hc->iobase + UHCI_RPORTS + port * 2) & RPORT_ENABLE;
    outw(hc->iobase + UHCI_RPORTS + port * 2, (status ? RPORT_ENABLE : 0) | RPORT_CSC);
    msleep(50);
    if (status && !previous)
    {
        outw(hc->iobase + UHCI_RPORTS + port * 2, RPORT_ENABLE | RPORT_RESUME);
        msleep(20);
        outw(hc->iobase + UHCI_RPORTS + port * 2, RPORT_ENABLE | RPORT_CSC);
        msleep(10);
    }
}
