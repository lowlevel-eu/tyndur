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

#ifndef MSD_CBI_H
#define MSD_CBI_H

#include <stdbool.h>
#include <stddef.h>

#include "msd.h"
#include "usb-ddrv.h"

// Beschreibt ein MSC-CBI-Gerät
struct msc_cbi_dev {
    // Das MSD an sich
    struct msc_gen_dev gen;

    // Das zugrunde liegende USB-Gerät
    struct usb_prov_dev* dev;
    // Die Interface-ID (MSDs sind immer Interfaces)
    int ifc_id;

    // Alle wichtigen Endpunkte (EP0 ist in dev enthalten), interrupt darf NULL
    // sein
    struct usb_endpoint_descriptor* bulk_in;
    struct usb_endpoint_descriptor* bulk_out;
    struct usb_endpoint_descriptor* interrupt;

    // Die Pipes zu diesen Endpunkten, ip darf NULL sein
    struct usb_pipe* bip;
    struct usb_pipe* bop;
    struct usb_pipe* ip;
};


// Sendet einen SCSI-Befehl
int msd_cbi_issue_scsi_cmd(struct msc_gen_dev* dev, const void* buffer,
    size_t cmd_length, bool receive, size_t req_length);
// Sendet Daten zum Gerät
int msd_cbi_send(struct msc_gen_dev* dev, const void* buffer, size_t size);
// Empfängt Daten vom Gerät
int msd_cbi_recv(struct msc_gen_dev* dev, void* buffer, size_t size);

#endif
