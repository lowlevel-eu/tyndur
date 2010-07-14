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
#include <sys/types.h>

#include <init.h>
#include <ports.h>
#include <rpc.h>
#include <syscall.h>

#include "uhci.h"
#include "usb-hc.h"
#include "usb-server.h"

// #define DEBUG
// #define SHOW_ERRORS

pid_t usb_pid, pci_pid;

struct uhci* uhcis[IRQ_COUNT] = { NULL };

void uhci_irq_handler(uint8_t irq)
{
    if ((irq < TYNDUR_IRQ_OFFSET) || (irq >= TYNDUR_IRQ_OFFSET + IRQ_COUNT)) {
        return;
    }

    struct uhci* hc = uhcis[irq - TYNDUR_IRQ_OFFSET];

    int status = inw(hc->iobase + UHCI_USBSTS);

    if (!status) {
        return;
    }

    if ((status & 0x10) || (status & 0x08)) {
        fprintf(stderr, "[uhci] SERIOUS HOST ERROR CONDITION (STS: 0x%02X)\n", status);
        return;
    }

    if (status & 0x20) {
        fprintf(stderr, "[uhci] Erm, wtf. HC halted...\n");
        return;
    }

    if (status & ~0x03) {
        return;
    }

    struct uhci_td *td, *ntd;

    lock(&hc->queue_head_lock);

    td = (struct uhci_td*)hc->queue_head->q_first;

    while (td != NULL) {
        /*
        #ifdef DEBUG
        printf("Active: %i (next: 0x%08X)\n", td->flags.bits.active, td->next);
        #endif
        */
        if (td->flags.bits.active) {
            if (td->active_count--) {
                break;
            } else {
                #ifdef DEBUG
                printf("Active timeout on 0x%08X/0x%08X, QH -> 0x%08X/0x%08X\n",
                    td, td->ptd, hc->queue_head->q_first,
                    hc->queue_head->transfer);
                #endif
                td->flags.bits.active = 0;
            }
        }

        #ifdef DEBUG
        printf("Finished td %s %i:%i, flags: 0x%08X\n",
            (td->pid == 0x69) ? "from" : "to", td->addr, td->ep,
            td->flags.flags);
        #endif

        if (td->result != NULL) {
            if (td->flags.bits.spd) {
                #ifdef SHOW_ERRORS
                printf("[uhci] Short packet detected\n");
                #endif

                if (hc->queue_head->transfer == td->ptd) {
                    hc->queue_head->transfer = td->next;
                }

                *td->result = EMSGSIZE;
            } else if ((status == 0x01) && !td->flags.bits.e_stall) {
                #ifdef DEBUG
                printf("[uhci] Fine, actual len: %i; max len: %i\n",
                    td->flags.bits.act_len, td->maxlen);
                #endif
                if (((td->flags.bits.act_len + 1) & 0x7FF) < ((td->maxlen + 1) & 0x7FF)) {
                    *td->result = EMSGSIZE;
                } else {
                    *td->result = 0;
                }
            } else {
                td->pipe->toggle = td->toggle;
                // TODO Weitere Transfers aus der Liste entfernen
                #ifdef SHOW_ERRORS
                printf("[uhci] Something failed, errors: %c%c%c%c%c%c\n",
                    td->flags.bits.e_bitstuff ? 'b' : '-',
                    td->flags.bits.e_crc_timeout ? 'c' : '-',
                    td->flags.bits.e_nak ? 'n' : '-',
                    td->flags.bits.e_babble ? 'q' : '-',
                    td->flags.bits.e_buffer ? 'f' : '-',
                    td->flags.bits.e_stall ? 's' : '-');
                printf("[uhci] Next'll be: 0x%08X/0x%08X\n", td->next,
                    td->q_next);
                #endif

                if (hc->queue_head->transfer == td->ptd) {
                    hc->queue_head->transfer = td->next;
                }

                if (td->flags.bits.e_stall) {
                    *td->result = EHOSTDOWN;
                } else {
                    *td->result = EAGAIN;
                }
            }
        }

        ntd = td->q_next;
        if (td->free_on_completion) {
            mem_free(td, 4096);
        }

        if (td->next & 1) {
            td = ntd;
            break;
        }

        td = ntd;
    }

    hc->queue_head->q_first = td;
    if (td == NULL) {
        hc->queue_head->q_last = NULL;
        hc->queue_head->transfer = 1;
    }

    unlock(&hc->queue_head_lock);

    outw(hc->iobase + UHCI_USBSTS, status);
}

void uhci_hct_handler(pid_t src, uint32_t corr_id, size_t length, void* data)
{
    struct usb_hc_trs* trs = data;
    void* shm;
    int result;

    if (length < sizeof(*trs)) {
        rpc_send_int_response(src, corr_id, -EINVAL);
        return;
    }

    switch (trs->type)
    {
        case USB_TRST_CONTROL:
            if (length < sizeof(struct usb_hc_trs_ctrl)) {
                rpc_send_int_response(src, corr_id, -EINVAL);
                return;
            }
            struct usb_hc_trs_ctrl* ctrl = data;

            shm = open_shared_memory(ctrl->shm);
            result = uhci_do_control(trs->hc.hcd_avail, &trs->pipe,
                &ctrl->setup, shm);
            close_shared_memory(ctrl->shm);
            rpc_send_int_response(src, corr_id, result);
            break;
        case USB_TRST_BULK:
            if (length < sizeof(struct usb_hc_trs_bulk)) {
                rpc_send_int_response(src, corr_id, -EINVAL);
                return;
            }
            struct usb_hc_trs_bulk* bulk = data;

            shm = open_shared_memory(bulk->shm);
            result = uhci_do_bulk(trs->hc.hcd_avail, &trs->pipe,
                bulk->receive, shm, bulk->length);
            close_shared_memory(bulk->shm);

            if (!result) {
                // Irgendwie muss der Status des Bits ja zurück
                result = trs->pipe.toggle;
            }
            rpc_send_int_response(src, corr_id, result);
            break;
        default:
            rpc_send_int_response(src, corr_id, -EINVAL);
            break;
    }
}

void uhci_hcc_handler(pid_t src, uint32_t corr_id, size_t length, void* data)
{
    struct usb_hc_ctrl_req* rq = data;
    struct usb_hc_ctrl_scan* rqsc = data;
    struct usb_hc_ctrl_dsbl* rqdbl = data;
    struct usb_hc_ctrl_rst* rqrst = data;

    if (length < sizeof(*rq)) {
        rpc_send_int_response(src, corr_id, 0);
        return;
    }

    switch (rq->type)
    {
        case HC_CTRL_SCAN_PORT:
            if (length < sizeof(*rqsc)) {
                rpc_send_int_response(src, corr_id, 0);
                return;
            }
            int result = uhci_scan_port(rq->hc.hcd_avail, rqsc->port);
            rpc_send_int_response(src, corr_id, result);
            break;
        case HC_CTRL_DSBL_PORT:
            if (length < sizeof(*rqdbl)) {
                rpc_send_int_response(src, corr_id, 0);
                return;
            }
            uhci_set_port_status(rq->hc.hcd_avail, rqdbl->port, 0);
            rpc_send_int_response(src, corr_id, 1);
            break;
        case HC_CTRL_RESET:
            if (length < sizeof(*rqrst)) {
                rpc_send_int_response(src, corr_id, 0);
                return;
            }
            uhci_reset_port(rq->hc.hcd_avail, rqrst->port);
            rpc_send_int_response(src, corr_id, 1);
            break;
        case HC_CTRL_ADD_PIPE:
            rpc_send_int_response(src, corr_id, 1);
            break;
        default:
            rpc_send_int_response(src, corr_id, 0);
            break;
    }
}

void init_rpc_interface(void)
{
    register_message_handler(RPC_USB_HC_CONTROL_FNN, &uhci_hcc_handler);
    register_message_handler(RPC_USB_HC_TRANSFER_FNN, &uhci_hct_handler);
}

void register_uhci(struct uhci* hc)
{
    struct usb_hc rhc = {
        .root_ports = hc->rh->ports,
        .hcd_avail = hc
    };

    if (!rpc_get_int(usb_pid, RPC_USB_SV_REGHC_FNN, sizeof(rhc), (char*)&rhc)) {
        fprintf(stderr, "Could not register UHCI.\n");
    }
}
