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

#ifndef UHCI_H
#define UHCI_H

#include <stdint.h>
#include <lock.h>

#include "usb-hc.h"
#include "usb-trans.h"


#define TYNDUR_IRQ_OFFSET 0x20
#define IRQ_COUNT         0x10

// Befehlsregister (uint16_t)
#define UHCI_USBCMD    0x00
// Statusregister (uint16_t)
#define UHCI_USBSTS    0x02
// Interruptregister (uint16_t)
#define UHCI_USBINTR   0x04
// Aktuelle Framenummer (uint16_t)
#define UHCI_FRNUM     0x06
// Pointer zur Frameliste (uint32_t)
#define UHCI_FRBASEADD 0x08
// Bestimmt die genaue Länge eines Frames (uint8_t)
#define UHCI_SOFMOD    0x0C
// Array von Informationen über je einen Rootport (je uint16_t)
#define UHCI_RPORTS    0x10

#define MAXP           (1 << 7)
#define CONFIGURE      (1 << 6)
// Resumesignal treiben
#define FORCE_GRESUME  (1 << 4)
// Alle Rootports befinden sich im Suspendmodus
#define GLOB_SUSP_MODE (1 << 3)
// RESET-Signal wird an allen Rootports getrieben
#define GRESET         (1 << 2)
// HC wird zurückgesetzt
#define HCRESET        (1 << 1)
// Normaler Ausführungsmodus
#define USB_RUN        (1 << 0)

// Setzt einen Rootport zurück
#define RPORT_RESET    (1 << 9)
// Gibt an, ob ein Low-Speed-Gerät angeschlossen ist
#define RPORT_LOSPD    (1 << 8)
// Treibt das Resume-Signal auf dem Bus
#define RPORT_RESUME   (1 << 6)
// Schaltet den Rootport ein
#define RPORT_ENABLE   (1 << 2)
// Wird gesetzt, wenn ein Gerät angeschlossen oder entfernt wird
#define RPORT_CSC      (1 << 1)
// Gibt an, ob ein Gerät angeschlossen ist
#define RPORT_DEVICE   (1 << 0)

// Das Legacy-Support-Register im PCI-Konfigurationsraum (uint16_t)
#define UHCI_PCI_LEGSUP 0xC0

// Alle Statusbits, die mittels Schreiben einer 1 gelöscht werden
#define UHCI_LEGSUP_STATUS 0x8F00
// Unveränderliche Bits (RO oder reserviert)
#define UHCI_LEGSUP_NO_CHG 0x5040
// Interrupt wird als PCI-IRQ ausgeführt
#define UHCI_LEGSUP_PIRQ   0x2000


struct uhci_td {
    volatile uint32_t next;

    union {
        volatile uint32_t flags;
        struct {
            volatile unsigned act_len : 11;
            unsigned rsvd1 : 6;
            volatile unsigned e_bitstuff : 1;
            volatile unsigned e_crc_timeout : 1;
            volatile unsigned e_nak : 1;
            volatile unsigned e_babble : 1;
            volatile unsigned e_buffer : 1;
            volatile unsigned e_stall : 1;
            volatile unsigned active : 1;
            unsigned ioc : 1;
            unsigned isochronous : 1;
            unsigned low_speed : 1;
            volatile unsigned errors : 2;
            unsigned spd : 1;
            unsigned rsvd2 : 2;
        } __attribute__((packed)) bits;
    } __attribute__((packed)) flags;

    unsigned pid : 8;
    unsigned addr : 7;
    unsigned ep : 4;
    unsigned toggle : 1;
    unsigned rsvd : 1;
    unsigned maxlen : 11;

    uint32_t buffer;

    // Diese Felder sind für den HCD

    // Nächster TD in der Warteschlange
    struct uhci_td* q_next;

    // Pointer zu einer Variable, die einen errno-Wert aufnimmt
    volatile int* result;

    // Ist dieses Flag gesetzt, so wird der TD nach Abarbeitung vom IRQ-Handler
    // automatisch freigegeben
    int free_on_completion;

    // Physische Adresse
    uintptr_t ptd;

    // Entsprechende Pipe
    struct usb_trans_pipe* pipe;

    int active_count;
} __attribute__((packed));

struct uhci_qh {
    volatile uint32_t next;
    volatile uint32_t transfer;

    // Diese Felder sind für den HCD

    // Erster TD in der Warteschlange (oder NULL, wenn keiner)
    volatile struct uhci_td* q_first;

    // Letzter TD in der Warteschlange (oder NULL, wenn keiner)
    volatile struct uhci_td* q_last;
} __attribute__((packed));

// Beschreibt ein UHCI
struct uhci {
    // PCI-Geräteinformationen
    struct pci_device* pci;
    // Beginn des I/O-Raums
    uint16_t iobase;

    // Physische Adresse der Framelist
    uintptr_t p_frame_list;
    // Virtuelle Adresse der Framelist
    uint32_t* frame_list;
    // Lock für den Zugriff auf diese Liste
    lock_t frame_list_lock;

    // Physische Adresse des (bisher einzigen) QHs
    uintptr_t p_queue_head;
    // Virtuelle Adresse des QHs
    struct uhci_qh* queue_head;
    // Lock für den Zugriff auf diesen
    lock_t queue_head_lock;

    // Roothub, der durch das UHCI bereitgestellt wird
    struct usb_rh* rh;
};

// Beschreibt einen Roothub (nur für UHCI)
struct usb_rh {
    // Anzahl der Rootports
    int ports;
};


/**
 * Durchsucht alle verfügbaren PCI-Geräte nach einem UHCI.
 *
 * @return 1, wenn ein UHCI gefunden wurde, sonst 0.
 */
int check_pci(void);

/**
 * Initialisiert das RPC-Interface (Handler)
 */
void init_rpc_interface(void);

/**
 * Registriert einen UHCI als HC beim USB-Treiber.
 *
 * @param hc Das zu registrierende UHCI
 */
void register_uhci(struct uhci* hc);

/**
 * Führt einen Bulktransfer durch.
 *
 * @param hc UHCI
 * @param pipe Pipe, durch die die Übertragung stattfinden soll
 * @param receive true, wenn empfangen werden soll, sonst false
 * @param buffer Zielspeicherbereich
 * @param length (Angestrebte) Länge des Transfers
 *
 * @return 0 bei Erfolg, sonst -errno
 */
int uhci_do_bulk(struct uhci* hc, struct usb_trans_pipe* pipe, bool receive,
    void* buffer, size_t length);

/**
 * Führt einen Controltransfer durch.
 *
 * @param hc UHCI
 * @param pipe Pipe, durch die die Übertragung stattfinden soll
 * @param setup SETUP-Paket
 * @param buffer Speicherbereich, mit dem während der Datenphase gearbeitet
 *               werden soll.
 *
 * @return 0 bei Erfolg, sonst -errno
 */
int uhci_do_control(struct uhci* hc, struct usb_trans_pipe* pipe,
    struct usb_setup_packet* setup, void* buffer);

/**
 * Treibt das Resetsignal auf einem Port und aktiviert ihn.
 *
 * @param hc UHCI
 * @param port Der Portindex
 */
void uhci_reset_port(struct uhci* hc, int port);

/**
 * Überprüft einen Port.
 *
 * @param hc UHCI
 * @param port Zu überprüfender Port
 *
 * @return Status (USB_HUB_...)
 */
int uhci_scan_port(struct uhci* hc, int port);

/**
 * Aktiviert oder deaktiviert einen Port.
 *
 * @param hc UHCI
 * @param port Portindex
 * @param status 1 zum Aktivieren, 0 zum Deaktivieren
 */
void uhci_set_port_status(struct uhci* hc, int port, int status);

#endif
