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

#ifndef USB_SERVER_H
#define USB_SERVER_H

#include <stdbool.h>
#include <stdint.h>

#include "usb-ddrv.h"
#include "usb-server.h"
#include "usb-structs.h"
#include "usb-trans.h"

// Funktion zum Registrieren von Hostcontrollern
#define RPC_USB_SV_REGHC_FNN "HCD_REG"
// Funktion zum Registrieren von Gerätetreibern
#define RPC_USB_SV_REGDD_FNN "DDRV_REG"
// Funktion zur Steuerung von Geräten (Daten austauschen, etc.)
#define RPC_USB_SV_DEVCT_FNN "DEV_CTL"

// Beschreibt eine USB-Pipe
struct usb_pipe {
    // Da die von normalen Treibern sowieso nicht dereferenziert werden dürfen,
    // können diese Strukturen auch undefiniert bleiben.
    // USB-Gerät (nur für den Bustreiber)
    struct usb_device* dev;
    // Endpunktdeskriptor (nur für den Bustreiber)
    struct usb_endpoint_descriptor* ep;

    // Pipe, zur Kommunikation mit dem Hostcontroller
    struct usb_trans_pipe trs;
};

// Funktionen für DEV_CTRL (mit Rückgabetypen)
enum usb_devctrl_reqtype {
    // Gibt Anzahl der Endpunkte zurück
    USB_DC_NUM_ENDPOINTS = 0, // int
    // Gibt die Interface-ID zurück
    USB_DC_GET_IFC_ID, // int
    // Gibt den Endpunktdeskriptor zurück
    USB_DC_GET_ENDPOINT, // response
    // Gibt eine Pipe zurück (und erstellt sie)
    USB_DC_GET_PIPE, // response
    // Entstallt eine Pipe/den Endpunkt
    USB_DC_CLEAR_HALT, // int
    // Führt einen Controltransfer durch
    USB_DC_TRSF_CONTROL, // int
    // Führt einen Bulktransfer durch
    USB_DC_TRSF_BULK // int
};

// Allgemeine Anfrage für DEV_CTRL
struct usb_devctrl_req {
    // Typ der Anrage
    enum usb_devctrl_reqtype type;
};

// Gibt die Anzahl der Endpunkte zurück
struct usb_devctrl_gne {
    struct usb_devctrl_req req;
    // Abzufragendes USB-Gerät
    struct usb_prov_dev dev;
};

// Gibt die Interface-ID zurück
struct usb_devctrl_gii {
    struct usb_devctrl_req req;
    // Abzufrasgendes USB-Gerät
    struct usb_prov_dev dev;
};

// Gibt den Endpunktdeskriptor zurück
struct usb_devctrl_gep {
    struct usb_devctrl_req req;
    // USB-Gerät
    struct usb_prov_dev dev;
    // Nummer des Endpunkts (nicht die ID)
    int ep_number;
};

// Erstellt eine USB-Pipe und gibt sie zurück
struct usb_devctrl_gpp {
    struct usb_devctrl_req req;
    // USB-Gerät
    struct usb_prov_dev dev;
    // Nummer des Zielendpunkts
    int ep_number;
};

// Entstallt eine Pipe resp. den Endpunkt
struct usb_devctrl_clh {
    struct usb_devctrl_req req;
    // USB-Pipe
    struct usb_pipe pipe;
};

// Führt einen Controltransfer durch
struct usb_devctrl_tct {
    struct usb_devctrl_req req;
    // Ist EP0 das Ziel, so muss dev_id auf den Wert aus usb_prov_dev gesetzt
    // werden, sonst pipe auf die entsprechende USB-Pipe.
    union {
        struct usb_pipe pipe;
        void* dev_id;
    } dst;
    // true, wenn durch die Default Control Pipe übertragen werden soll
    bool ep0;
    // SETUP-Paket
    struct usb_setup_packet setup;
    // SHM-Bereich für die Datenphase
    uint32_t shm;
};

// Führt einen Bulktransfer durch
struct usb_devctrl_tbk {
    struct usb_devctrl_req req;
    // USB-Pipe
    struct usb_pipe pipe;
    // Anzahl der zu übertragenden Bytes
    size_t length;
    // SHM-Bereich für die Daten
    uint32_t shm;
};

#endif
