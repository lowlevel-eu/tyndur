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

#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "usb-ddrv.h"
#include "usb-hc.h"
#include "usb-hub.h"
#include "usb-server.h"
#include "usb-structs.h"
#include "usb-trans.h"

struct usb_pipe;

// Beschreibt einen Hub (sowohl Roothub als auch einen normalen).
struct usb_hub {
    // Da ein Hub immer nur entweder Root- oder normaler Hub sein kann, macht
    // sich eine Union hier ganz gut.
    union {
        struct usb_device* usb;
        struct usb_hc* hc;
    } dev;
    // true, wenn es sich um einen Roothub handelt.
    bool root_hub;
    // Anzahl der Ports am Hub.
    int ports;

    /**
     * Wird aufgerufen, wenn eine neue Pipe benötigt wird. Ist bereits eine
     * Pipe zur angegebenen Geräte-/Endpunkt-Kombination vorhanden, so sollte
     * diese entfernt werden. Der Pointer darf NULL sein, um anzuzeigen, dass
     * die Funktion nicht benötigt wird. Normale Hubs müssen jedoch dafür
     * sorgen, dass die vom Hostcontroller (Roothub) bereitgestellte Funktion
     * aufgerufen wird (es genügt, einfach den Pointer vom übergeordneten Hub
     * zu kopieren, der Roothub muss dies erkennen können).
     *
     * @param hub Der Hub, an dem das Gerät hängt.
     * @param pipe Die neu hinzuzufügende Pipe.
     */
    void (*add_pipe)(struct usb_hub* hub, struct usb_pipe* pipe);

    /**
     * Deaktiviert einen Port (zum Aktivieren wird reset_port verwendet).
     *
     * @param hub Hub mit dem zu deaktivierenden Port.
     * @param port Zu deaktivierender Port.
     */
    void (*disable_port)(struct usb_hub* hub, int port);

    /**
     * Treibt das Resetsignal auf einem Port und aktiviert ihn. Wenn nötig,
     * muss außerdem das Resume-Signal getrieben werden, damit nach dem Aufruf
     * dieser Funktion mit dem Gerät kommuniziert werden kann.
     *
     * @param hub Hub, an dem das Gerät hängt.
     * @param port Port, an dem es hängt.
     */
    void (*reset_port)(struct usb_hub* hub, int port);

    /**
     * Überprüft einen Port und gibt dessen Status zurück.
     *
     * @param hub Zu untersuchender Hub.
     * @param port Entsprechender Port.
     *
     * @return Kombination aus den in usb-hub.h mit USB_HUB beginnenden Flags.
     */
    int (*scan_port)(struct usb_hub* hub, int port);
};

// Beschreibt ein USB-Gerät
struct usb_device {
    // Hub, an dem es hängt
    struct usb_hub* parent;
    // Index des Ports, an dem es sich befindet
    int hub_port;

    // Zuständiger Hostcontroller
    struct usb_hc* hc;
    // Dort eindeutige USB-Adresse
    int addr;

    // Aktueller Status
    enum usb_state state;
    // Aktuelle Geschwindigkeit
    int speed;

    // Default Control Pipe
    struct usb_pipe* ep0;
    // Device-Descriptor
    struct usb_device_descriptor* dev_desc;
    // Alle Configuration-Descriptors
    struct usb_config_descriptor** cfg_desc;
    // Aktuelle Konfiguration
    struct usb_config_descriptor* cur_cfg;
};

// USB-Gerätetreiber
struct usb_ddrv {
    // Prozess-ID
    pid_t pid;
    // Gibt an, welche Geräte er behandeln kann
    struct usb_handle_devs handle;
};

// Bisher nicht behandeltes USB-Gerät (oder Interface)
struct usb_unhandled {
    // Das Gerät
    struct usb_device* dev;
    // Das Interface
    struct usb_interface_descriptor* ifc;
};


/**
 * Initialisiert die RPC-Handler und trägt sich als usb1 bei init ein-
 */
void register_rpc_interface(void);

/**
 * Enumeriert alle Geräte an einem Hub
 *
 * @param hub Der Hub
 */
void scan_hub(struct usb_hub* hub);

/**
 * Entstallt einen Endpunkt.
 *
 * @param pipe Zum Endpunkt gehörige Pipe
 */
void usb_clear_halt(struct usb_pipe* pipe);

/**
 * Führt einen Controltransfer durch.
 *
 * @param pipe Die Pipe
 * @param type type-Feld des SETUP-Pakets
 * @param request request-Feld des SETUP-Pakets
 * @param value value-Feld des SETUP-Pakets
 * @param index index-Feld des SETUP-Pakets
 * @param length length-Feld des SETUP-Pakets
 * @param buffer Speicherbereich, mit dem während der Datenphase gearbeitet
 *               werden soll.
 *
 * @return 0 bei Erfolg, sonst -errno.
 */
int usb_control_transfer(struct usb_pipe* pipe, int type, int request,
    int value, int index, int length, void* buffer);

/**
 * Führt einen Bulktransfer durch.
 *
 * @param pipe Die Pipe für den Transfer
 * @param shm SHM-Bereich, mit dem gearbeitet werden soll
 * @param length Länge des Transfers
 *
 * @return -errno bei Fehler, sonst neuer Wert des Toggle-Bits.
 */
int usb_shm_bulk_transfer(struct usb_pipe* pipe, uint32_t shm, size_t length);

/**
 * Führt einen Controltransfer durch. Entspricht usb_control_transfer, außer
 * dass ein SHM-Bereich verwendet wird.
 */
int usb_shm_control_transfer(struct usb_pipe* pipe, int type, int request,
    int value, int index, int length, uint32_t shm);

/**
 * Setzt die aktuelle Konfiguration eines Geräts.
 *
 * @param dev USB-Gerät
 * @param config ID der neuen Konfiguration
 */
void usb_set_configuration(struct usb_device* dev, int config);

#endif
