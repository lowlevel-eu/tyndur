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

#ifndef USB_HC_H
#define USB_HC_H

#include <stdbool.h>
#include <sys/types.h>

#include "usb-structs.h"
#include "usb-trans.h"

// Beschreibt einen USB-Hostcontroller
struct usb_hc {
    // Anzahl der Rootports
    int root_ports;
    // Für den HCD frei verfügbar
    void* hcd_avail;
    // PID des HCD
    pid_t pid;
    // USB-Adressenzähler
    int addr_counter;
};

// RPC-Funktion für USB-Bustreiber (vorrangig Roothubfunktionen)
#define RPC_USB_HC_CONTROL_FNN "HC_CONTROL"
// RPC-Funktion für USB-Bustreiber (vorrangig Transferfunktionen)
#define RPC_USB_HC_TRANSFER_FNN "HC_TRANSFER"

// PIDs für IN, OUT und SETUP
#define USB_OUT   0xE1
#define USB_IN    0x69
#define USB_SETUP 0x2D

// Roothubfunktionen
enum usb_hc_ctrl_rt {
    // Portstatus überprüfen
    HC_CTRL_SCAN_PORT = 0,
    // Port deaktivieren
    HC_CTRL_DSBL_PORT,
    // Resetsignal treiben
    HC_CTRL_RESET,
    // Pipe erstellen
    HC_CTRL_ADD_PIPE
};


// Allgemeiner HC_CONTROL-Request
struct usb_hc_ctrl_req {
    // Funktion
    enum usb_hc_ctrl_rt type;
    // Hostcontroller
    struct usb_hc hc;
};

// Rootport überprüfen
struct usb_hc_ctrl_scan {
    struct usb_hc_ctrl_req req;
    // Portindex
    int port;
};

// Rootport deaktivieren
struct usb_hc_ctrl_dsbl {
    struct usb_hc_ctrl_req req;
    // Portindex
    int port;
};

// Resetsignal an einem Rootport treiben
struct usb_hc_ctrl_rst {
    struct usb_hc_ctrl_req req;
    // Portindex
    int port;
};

// Pipe hinzufügen
struct usb_hc_ctrl_appe {
    struct usb_hc_ctrl_req req;
    // Informationen zur neuen Pipe
    struct usb_trans_pipe pipe;
};

// Allgemeine Transferanfrage
struct usb_hc_trs {
    // Funktion
    enum usb_trans_type type;
    // Hostcontroller
    struct usb_hc hc;
    // Pipe, durch die der Transfer laufen soll
    struct usb_trans_pipe pipe;
};

// Controltransfer
struct usb_hc_trs_ctrl {
    struct usb_hc_trs trs;
    // SETUP-Paket
    struct usb_setup_packet setup;
    // SHM-Bereich für die Datenphase
    uint32_t shm;
};

// Bulktransfer
struct usb_hc_trs_bulk {
    struct usb_hc_trs trs;
    // true zum Empfangen
    bool receive;
    // Anzahl der zu übertragenden Bytes
    size_t length;
    // SHM-Bereich für die übertragenen Daten
    uint32_t shm;
};

#endif
