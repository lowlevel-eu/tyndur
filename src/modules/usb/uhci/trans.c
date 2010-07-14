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

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <syscall.h>

#include "uhci.h"
#include "usb-hc.h"
#include "usb-trans.h"

#define USB_SETUP_OUT (0 << 7)
#define USB_SETUP_IN  (1 << 7)

int insert_td(struct uhci* hc, uintptr_t ptd, struct uhci_td* td);

int uhci_do_control(struct uhci* hc, struct usb_trans_pipe* pipe,
            struct usb_setup_packet* setup, void* buffer)
{
    if ((pipe->speed != USB_HUB_LOW_SPEED) &&
        (pipe->speed != USB_HUB_FULL_SPEED))
    {
        return -EPROTOTYPE;
    }

    struct uhci_td* td = mem_allocate(sizeof(*td), 0);
    assert(td);
    uintptr_t ptd = (uintptr_t)get_phys_addr(td);
    assert(ptd);

    volatile int status;

    do {
        status = EALREADY;

        td->next = 1;
        if (pipe->speed == USB_HUB_LOW_SPEED) {
            td->flags.flags = 0x1D800000;
        } else {
            td->flags.flags = 0x19800000;
        }
        td->pid = USB_SETUP;
        td->toggle = 0;
        td->addr = pipe->usb_addr;
        td->ep = pipe->ep_id;

        td->maxlen = sizeof(*setup) - 1;
        td->buffer = (uintptr_t)get_phys_addr(setup);

        td->result = &status;
        td->free_on_completion = 0;

        td->pipe = pipe;

        while (insert_td(hc, ptd, td));

        while (status == EALREADY);
    } while (status == EAGAIN);

    if (status) {
        if (status == EHOSTDOWN) {
            pipe->stalled = 1;
        }

        mem_free(td, 4096);
        return -status;
    }

    pipe->toggle = 1;

    int remaining = setup->length;
    int direction, done = 0;

    if (setup->type & USB_SETUP_IN) {
        direction = USB_IN;
    } else {
        direction = USB_OUT;
    }

    while (remaining) {
        int this_len = (remaining > pipe->mps) ? pipe->mps : remaining;

        remaining -= this_len;

        do {
            status = EALREADY;

            td->next = 1;
            if (pipe->speed == USB_HUB_LOW_SPEED) {
                td->flags.flags = 0x1D800000;
            } else {
                td->flags.flags = 0x19800000;
            }
            td->pid = direction;
            td->toggle = pipe->toggle;
            td->addr = pipe->usb_addr;
            td->ep = pipe->ep_id;

            td->maxlen = (unsigned int)(this_len - 1) & 0x7FF;
            td->buffer = (uintptr_t)get_phys_addr((void*)((uintptr_t)buffer + done));

            td->result = &status;
            td->free_on_completion = 0;

            while (insert_td(hc, ptd, td));

            while (status == EALREADY);
        } while (status == EAGAIN);

        if (status && (status != EMSGSIZE)) {
            if (status == EHOSTDOWN) {
                pipe->stalled = 1;
            }

            mem_free(td, 4096);
            return -status;
        }

        pipe->toggle ^= 1;
        done += this_len;

        if (status == EMSGSIZE)
            break;
    }

    if (direction == USB_IN) {
        direction = USB_OUT;
    } else {
        direction = USB_IN;
    }

    do {
        status = EALREADY;

        td->next = 1;
        if (pipe->speed == USB_HUB_LOW_SPEED) {
            td->flags.flags = 0x1D800000;
        } else {
            td->flags.flags = 0x19800000;
        }
        td->pid = direction;
        td->toggle = 1;
        td->addr = pipe->usb_addr;
        td->ep = pipe->ep_id;

        td->maxlen = 0x7FF;
        td->buffer = 0;

        td->result = &status;
        td->free_on_completion = 0;

        while (insert_td(hc, ptd, td));

        while (status == EALREADY);
    } while (status == EAGAIN);

    if (status == EHOSTDOWN) {
        pipe->stalled = 1;
    }

    pipe->toggle = 0;
    mem_free(td, sizeof(*td));

    return -status;
}

int uhci_do_bulk(struct uhci* hc, struct usb_trans_pipe* pipe, bool receive,
    void* buffer, size_t length)
{
    if ((pipe->speed != USB_HUB_LOW_SPEED) &&
        (pipe->speed != USB_HUB_FULL_SPEED))
    {
        return -EPROTOTYPE;
    }

    if (!length) {
        return -EINVAL;
    }

    // printf("Issuing bulk (%i B expected)\n", length);

    struct uhci_td* td = mem_allocate(sizeof(*td), 0);
    assert(td);
    uintptr_t ptd = (uintptr_t)get_phys_addr(td);
    assert(ptd);

    volatile int res;

    int remaining = length;
    int done = 0;

    while (remaining) {
        // printf("%i B to go\n", remaining);
        int this_len = (remaining > pipe->mps) ? pipe->mps : remaining;

        remaining -= this_len;

        td->next = 1;
        if (pipe->speed == USB_LOW_SPEED) {
            td->flags.flags = 0x1D800000;
        } else {
            td->flags.flags = 0x19800000;
        }
        if (receive) {
            td->pid = USB_IN;
        } else {
            td->pid = USB_OUT;
        }
        td->toggle = pipe->toggle;
        td->addr = pipe->usb_addr;
        td->ep = pipe->ep_id;

        td->maxlen = (unsigned int)(this_len - 1) & 0x7FF;
        td->buffer = (uintptr_t)
            get_phys_addr((void *)((uintptr_t)buffer + done));

        res = EALREADY;
        td->result = &res;
        td->free_on_completion = 0;
        td->pipe = pipe;

        // printf("Inserting TD\n");
        while (insert_td(hc, ptd, td));

        // printf("Waiting for transmission\n");
        while (res == EALREADY);
        // printf("Thru\n");

        if (res == EAGAIN) {
            remaining += this_len;
            continue;
        }

        if (res) {
            return -res;
        }

        pipe->toggle ^= 1;

        done += this_len;
    }

    if (res == EHOSTDOWN) {
        pipe->stalled = 1;
    }

    mem_free(td, sizeof(*td));

    return -res;
}

int insert_td(struct uhci* hc, uintptr_t ptd, struct uhci_td* td)
{
    struct uhci_td *last;

    lock(&hc->queue_head_lock);

    td->ptd = ptd;
    td->q_next = NULL;
    td->active_count = 10;

    if (hc->queue_head->transfer & 1) {
        // Insert as first element to be done
        if (hc->queue_head->q_last != NULL) {
            hc->queue_head->q_last->q_next = td;
        } else {
            hc->queue_head->q_first = td;
        }

        hc->queue_head->q_last = td;
        hc->queue_head->transfer = ptd;
    } else {
        last = (struct uhci_td*)hc->queue_head->q_last;
        #ifdef DEBUG
        printf("[uhci] Will append to 0x%08X (p 0x%08X)\n", last, hc->queue_head->transfer);
        #endif

        last->q_next = td;
        hc->queue_head->q_last = td;
        last->next = ptd | 4;
    }

    unlock(&hc->queue_head_lock);

    return 0;
}
