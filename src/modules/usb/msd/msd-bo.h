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

#ifndef MSD_BO_H
#define MSD_BO_H

#include <stdbool.h>
#include <stddef.h>

#include "msd.h"
#include "usb-ddrv.h"

#define CBW_IN  0x80
#define CBW_OUT 0x00

#define CBW_SIGNATURE 0x43425355 //'USBC' => USB command
#define CSW_SIGNATURE 0x53425355 //'USBS' => USB status


// Beschreibt ein MSC-Bulk-Only-Gerät
struct msc_bo_dev {
    // Das MSD an sich
    struct msc_gen_dev gen;

    // Bulk-Endpunkte
    struct usb_endpoint_descriptor* bulk_in;
    struct usb_endpoint_descriptor* bulk_out;

    // Pipes zu diesen Endpunkten
    struct usb_pipe* bip;
    struct usb_pipe* bop;
};

// Command Block Wrapper für BO-MSDs
struct msd_bo_cbw {
    // CBW_SIGNATURE
    uint32_t signature;
    // Ein Wert, der im CSW wiederholt wird
    uint32_t tag;
    // Länge des Datentransfers
    uint32_t data_transfer_length;
    // Flags (vorrangig Richtung des Transfers)
    uint8_t flags;
    // LUN
    uint8_t lun;
    // Länge des Befehls
    uint8_t cb_length;
    // SCSI-Befehl
    uint8_t cb[16];
} __attribute__((packed));

// Command Status Wrapper für BO-MSDs
struct msd_bo_csw {
    // CSW_SIGNATURE
    uint32_t signature;
    // Tagwert aus dem CBW
    uint32_t tag;
    // Größe der Daten, die erwartet, aber nicht übertragen wurden
    uint32_t data_residue;
    // Status
    uint8_t status;
} __attribute__((packed));

// Sendet einen SCSI-Befehl
int msd_bo_issue_scsi_cmd(struct msc_gen_dev* dev, const void* buffer,
    size_t cmd_length, bool receive, size_t req_length);
// Sendet Daten an das Gerät
int msd_bo_send(struct msc_gen_dev* dev, const void* buffer, size_t size);
// Empfängt Daten vom Gerät
int msd_bo_recv(struct msc_gen_dev* dev, void* buffer, size_t size);

#endif
