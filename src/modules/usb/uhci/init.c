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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <init.h>
#include <pci.h>
#include <ports.h>
#include <rpc.h>
#include <sleep.h>
#include <syscall.h>

#include "uhci.h"

extern pid_t usb_pid, pci_pid;


extern struct uhci* uhcis[IRQ_COUNT];
extern void uhci_irq_handler(uint8_t irq);

static uint16_t pci_inw(struct pci_device* dev, int reg)
{
    struct pci_config_space_access csa = {
        .type = PCI_CSA_READ16,
        .bus = dev->bus,
        .device = dev->device,
        .function = dev->function,
        .reg = reg
    };

    return rpc_get_dword(pci_pid, RPC_PCI_CONFIG_SPACE_ACCESS, sizeof(csa),
        (char*)&csa);
}

static void pci_outw(struct pci_device* dev, int reg, uint16_t val)
{
    struct pci_config_space_access csa = {
        .type = PCI_CSA_WRITE16,
        .bus = dev->bus,
        .device = dev->device,
        .function = dev->function,
        .reg = reg,
        .value = val
    };

    rpc_get_dword(pci_pid, RPC_PCI_CONFIG_SPACE_ACCESS, sizeof(csa),
        (char*)&csa);
}

void init_uhc(struct pci_device* dev, size_t devlen)
{
    devlen -= sizeof(*dev);
    int8_t* data = (int8_t*)(dev + 1);

    struct pci_resource pres;
    uint16_t iobase = 0, iosize = 0;

    while (devlen >= sizeof(pres)) {
        memcpy(&pres, data, sizeof(pres));

        devlen -= sizeof(pres);
        data += sizeof(pres);

        if (pres.type == PCI_RESOURCE_PORT) {
            iobase = pres.start;
            iosize = pres.length;
            break;
        }
    }

    if (!iobase) {
        return;
    }

    struct uhci* ndev = calloc(1, sizeof(*ndev));
    ndev->pci = dev;
    ndev->iobase = iobase;
    ndev->rh = calloc(1, sizeof(*ndev->rh));

    ndev->rh->ports = (iosize - 0x10) >> 1;

    request_ports(iobase, iosize);
    uhcis[dev->irq] = ndev;
    register_intr_handler(TYNDUR_IRQ_OFFSET + dev->irq, &uhci_irq_handler);

    outw(iobase + UHCI_USBCMD, GRESET);
    msleep(50);
    outw(iobase + UHCI_USBCMD, 0);

    int i;
    for (i = 2; i < ndev->rh->ports; i++) {
        if (!(inw(iobase + UHCI_RPORTS + i*2) & 0x0080) ||
            (inw(iobase + UHCI_RPORTS + i*2) == 0xFFFF))
        {
            // Ungültige Werte für einen Rootport, also ist das hier keiner
            ndev->rh->ports = i;
            break;
        }
    }

    // Mehr als sieben Rootports sind leicht ungewöhnlich
    if (ndev->rh->ports > 7) {
        ndev->rh->ports = 2;
    }

    int legsup = pci_inw(dev, UHCI_PCI_LEGSUP);
    int cmd = inw(iobase + UHCI_USBCMD);
    int intr = inw(iobase + UHCI_USBINTR);

    if ((legsup &
        ~(UHCI_LEGSUP_STATUS | UHCI_LEGSUP_NO_CHG | UHCI_LEGSUP_PIRQ))  ||
        (cmd & USB_RUN) || (cmd & CONFIGURE) || !(cmd & GLOB_SUSP_MODE) ||
        (intr & 0x000F))
    {
        // Damit uns nicht die IRQs um die Ohren fliegen
        outw(iobase + UHCI_USBSTS, 0x3F);

        // Einen Frame abwarten
        msleep(1);

        // Alle LEGSUP-Statusbits löschen
        pci_outw(dev, UHCI_PCI_LEGSUP, UHCI_LEGSUP_STATUS);

        // Resetten
        outw(iobase + UHCI_USBCMD, HCRESET);
        // Ende des Resets abwarten
        for (i = 0; (inw(iobase + UHCI_USBCMD) & HCRESET) && (i < 50); i++) {
            msleep(10);
        }

        // Interrupts erstmal deaktivieren
        outw(iobase + UHCI_USBINTR, 0);
        // Den gesamten HC ebenso
        outw(iobase + UHCI_USBCMD, 0);

        // Alle Ports "runterfahren"
        for (i = 0; i < ndev->rh->ports; i++) {
            outw(iobase + UHCI_RPORTS + i * 2, 0);
        }
    }

    ndev->frame_list = mem_allocate(4096, 0);
    assert(ndev->frame_list);
    ndev->p_frame_list = (uintptr_t)get_phys_addr(ndev->frame_list);
    assert(ndev->p_frame_list);
    ndev->frame_list_lock = 0;

    ndev->queue_head = mem_allocate(sizeof(*ndev->queue_head), 0);
    assert(ndev->queue_head);
    ndev->p_queue_head = (uintptr_t)get_phys_addr(ndev->queue_head);
    assert(ndev->p_queue_head);
    ndev->queue_head_lock = 0;

    // Alle Pointer auf "invalid" setzen
    ndev->queue_head->next = 1;
    ndev->queue_head->transfer = 1;
    ndev->queue_head->q_first = NULL;
    ndev->queue_head->q_last = NULL;

    for (i = 0; i < 1024; i++) {
        ndev->frame_list[i] = ndev->p_queue_head | 2;
    }

    // Alle 1000 µs ein Frame (Standardwert)
    outb(iobase + UHCI_SOFMOD, 0x40);
    // Unsere Frameliste eintragen
    outl(iobase + UHCI_FRBASEADD, ndev->p_frame_list);
    // Framenummer zurücksetzen
    outw(iobase + UHCI_FRNUM, 0);

    // Hier setzen wir das PIRQ-Bit und deaktivieren sämtlichen Legacy Support
    pci_outw(dev, UHCI_PCI_LEGSUP, UHCI_LEGSUP_PIRQ);

    // HC starten
    outw(iobase + UHCI_USBCMD, USB_RUN | CONFIGURE | MAXP);
    // Alle Interrupts aktivieren
    outw(iobase + UHCI_USBINTR, 0xF);

    // Connect-Status-Change-Bit zurücksetzen
    for (i = 0; i < ndev->rh->ports; i++) {
        outw(iobase + UHCI_RPORTS + i * 2, RPORT_CSC);
    }

    // Resumesignal treiben (egal, obs was bringt)
    outw(iobase + UHCI_USBCMD, USB_RUN | CONFIGURE | MAXP | FORCE_GRESUME);
    msleep(20);
    outw(iobase + UHCI_USBCMD, USB_RUN | CONFIGURE | MAXP);
    msleep(100);

    register_uhci(ndev);
}

int check_pci(void)
{
    struct init_dev_desc* devs;

    int ret = init_dev_list(&devs);
    if (ret < 0) {
        return 0;
    }

    int got_something = 0;

    int8_t* data = (int8_t*)devs;
    for (;;) {
        size_t size;

        if (ret < sizeof(*devs)) {
            break;
        }

        devs = (struct init_dev_desc*)data;
        size = sizeof(*devs) + devs->bus_data_size;
        if (ret < size) {
            break;
        }

        if (devs->type == 10) {
            struct pci_device* dev = (struct pci_device*)devs->bus_data;
            if ((dev->class_id == 0x0C) && (dev->subclass_id == 0x03) &&
                (dev->interface_id == 0x00))
            {
                got_something = 1;
                init_uhc(dev, size);
            }
        }

        ret -= size;
        data += size;
    }

    return got_something;
}
