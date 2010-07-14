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

#ifndef USB_LIB_H
#define USB_LIB_H

#include <stdbool.h>
#include <stddef.h>

#include "usb-ddrv.h"
#include "usb-server.h"
#include "usb-structs.h"
#include "usb-trans.h"

/*
 * Hinweis: Wenn hier die Rede von einem USB-Gerät ist, so ist ein logisches
 * USB-Gerät gemeint. Dies kann sowohl ein echtes (physisches) Gerät sein oder
 * aber auch nur ein Interface dieses Geräts (meistens letzteres, da so gut
 * wie alle USB-Geräteklassen auf Interfaceebene arbeiten und somit die meisten
 * physischen USB-Geräte genau ein Interface besitzen).
 */

/**
 * Führt einen USB-Bulktransfer durch.
 *
 * @param pipe Zu verwendende Pipe
 * @param buffer Zu bearbeitender Speicherbereich
 * @param length Länge des Transfers
 * @param receive true, wenn empfangen werden soll, sonst false.
 *
 * @return 0 bei Erfolg, sonst -errno.
 */
int usb_bulk_transfer(struct usb_pipe* pipe, void* buffer, size_t length,
    bool receive);

/**
 * Entstallt einen Endpunkt.
 *
 * @param pipe Eine zum Endpunkt gehörige Pipe.
 */
void usb_clear_halt(struct usb_pipe* pipe);

/**
 * Führt einen USB-Controltransfer durch.
 *
 * @param dev Gerät
 * @param pipe Zu verwendende Pipe oder NULL für die Default Control Pipe
 * @param setup SETUP-Paket
 * @param buffer Speicherbereich für die Datenphase
 *
 * @return 0 bei Erfolg, sonst -errno.
 */
int usb_control_transfer(struct usb_prov_dev* dev, struct usb_pipe* pipe,
    struct usb_setup_packet* setup, void* buffer);

/**
 * Erstellt eine USB-Pipe.
 *
 * @param dev Zielgerät
 * @param ep_number Nummer des Zielendpunkts (Nummer innerhalb des Interfaces,
 *                  nicht ID!)
 * @param pipe Speicherbereich, in dem die Daten der Pipe abgelegt werden
 *             sollen.
 */
void usb_create_pipe(struct usb_prov_dev* dev, int ep_number,
    struct usb_pipe* pipe);

/**
 * Empfängt Informationen zu einem Endpunkt.
 *
 * @param dev Gerät
 * @param ep_number Nummer des Endpunkts
 * @param ep_desc Pointer zu einem Speicherbereich, der einen Endpoint-
 *                Descriptor aufnehmen kann.
 */
void usb_get_endpoint_info(struct usb_prov_dev* dev, int ep_number,
    struct usb_endpoint_descriptor* ep_desc);

/**
 * Gibt die Interface-ID des logischen USB-Geräts zurück.
 *
 * @param dev Gerät
 *
 * @return Interface-ID
 */
int usb_get_interface_id(struct usb_prov_dev* dev);

/**
 * Gibt die Anzahl an Endpunkten des USB-Geräts zurück.
 *
 * @param dev Logisches Gerät
 *
 * @return Anzahl Endpunkte
 */
int usb_get_number_of_endpoints(struct usb_prov_dev* dev);

#endif
